#include "VectorDatabaseTypes.h"
#include "Algo/Sort.h"

UVectorDatabase::UVectorDatabase()
{
    Entries.Empty();
    Vectors.Empty();
}

UVectorDatabase::~UVectorDatabase()
{
    for (UVectorEntryWrapper* Entry : Entries)
    {
        if (Entry && Entry->IsValidLowLevel())
        {
            Entry->ConditionalBeginDestroy();
        }
    }
    Entries.Empty();
    Vectors.Empty();
}

void UVectorDatabase::AddEntry(const TArray<float>& Vector, UVectorEntryWrapper* Entry, const FString& Category)
{
    if (!Entry || !IsValid(Entry))
    {
        UE_LOG(LogTemp, Error, TEXT("AddEntry: Invalid Entry"));
        return;
    }

    Entry->Category = Category;

    int32 NewIndex = Entries.Add(Entry);
    if (NewIndex != INDEX_NONE)
    {
        Vectors.Add(Vector);
        
        Entry->Rename(nullptr, this);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("AddEntry: Failed to add Entry to Entries array"));
    }
}

void UVectorDatabase::AddStructEntry(const TArray<float>& Vector, UScriptStruct* StructType, const void* StructPtr, const FString& Category)
{
    if (!StructType || !StructPtr)
    {
        UE_LOG(LogTemp, Error, TEXT("AddStructEntry: Invalid StructType or StructPtr"));
        return;
    }

    UVectorEntryWrapper* Wrapper = NewObject<UVectorEntryWrapper>(this);
    if (!Wrapper)
    {
        UE_LOG(LogTemp, Error, TEXT("AddStructEntry: Failed to create UVectorEntryWrapper"));
        return;
    }

    Wrapper->SetStructData(StructType, StructPtr);
    Wrapper->EntryType = EEntryType::Struct;
    Wrapper->Category = Category;
    
    UE_LOG(LogTemp, Log, TEXT("AddStructEntry: StructType: %s, StructSize: %d"), *StructType->GetName(), StructType->GetStructureSize());
    UE_LOG(LogTemp, Log, TEXT("AddStructEntry: Wrapper->StructData.Num(): %d"), Wrapper->StructData.Num());

    AddEntry(Vector, Wrapper, Category);
}

TArray<UVectorEntryWrapper*> UVectorDatabase::GetTopNMatches(const TArray<float>& QueryVector, int32 N, EEntryType EntryType, const TArray<FString>& Categories) const
{
    TArray<TPair<float, UVectorEntryWrapper*>> DistanceEntryPairs;
    
    for (int32 i = 0; i < Vectors.Num(); ++i)
    {
        if (Entries[i]->EntryType == EntryType && QueryVector.Num() == Vectors[i].Num() && ShouldIncludeEntry(Entries[i], Categories))
        {
            float Distance = CalculateDistance(QueryVector, Vectors[i]);
            DistanceEntryPairs.Add(TPair<float, UVectorEntryWrapper*>(Distance, Entries[i]));
        }
    }

    DistanceEntryPairs.Sort([](const TPair<float, UVectorEntryWrapper*>& A, const TPair<float, UVectorEntryWrapper*>& B) {
        return A.Key < B.Key;
    });

    TArray<UVectorEntryWrapper*> Result;
    for (int32 i = 0; i < FMath::Min(N, DistanceEntryPairs.Num()); ++i)
    {
        Result.Add(DistanceEntryPairs[i].Value);
    }

    return Result;
}

TArray<FVectorDatabaseResult> UVectorDatabase::GetTopNStructMatches(const TArray<float>& QueryVector, int32 N, const TArray<FString>& Categories) const
{
    TArray<TPair<float, UVectorEntryWrapper*>> DistanceEntryPairs;
    
    for (int32 i = 0; i < Vectors.Num(); ++i)
    {
        if (Entries[i]->EntryType == EEntryType::Struct && QueryVector.Num() == Vectors[i].Num() && ShouldIncludeEntry(Entries[i], Categories))
        {
            float Distance = CalculateDistance(QueryVector, Vectors[i]);
            DistanceEntryPairs.Add(TPair<float, UVectorEntryWrapper*>(Distance, Entries[i]));
        }
    }

    DistanceEntryPairs.Sort([](const TPair<float, UVectorEntryWrapper*>& A, const TPair<float, UVectorEntryWrapper*>& B) {
        return A.Key < B.Key;
    });

    TArray<FVectorDatabaseResult> Results;
    for (int32 i = 0; i < FMath::Min(N, DistanceEntryPairs.Num()); ++i)
    {
        FVectorDatabaseResult Result;
        Result.Distance = DistanceEntryPairs[i].Key;
        Result.StructType = DistanceEntryPairs[i].Value->StructType;
        Result.StructData = DistanceEntryPairs[i].Value->StructData;
        Result.Category = DistanceEntryPairs[i].Value->Category;
        Results.Add(Result);
    }

    return Results;
}

TArray<FVectorDatabaseEntry> UVectorDatabase::GetTopNEntriesWithDetails(const TArray<float>& QueryVector, int32 N, const TArray<FString>& Categories) const
{
    TArray<TPair<float, int32>> DistanceIndexPairs;
    
    for (int32 i = 0; i < Vectors.Num(); ++i)
    {
        if (QueryVector.Num() == Vectors[i].Num() && ShouldIncludeEntry(Entries[i], Categories))
        {
            float Distance = CalculateDistance(QueryVector, Vectors[i]);
            DistanceIndexPairs.Add(TPair<float, int32>(Distance, i));
        }
    }

    DistanceIndexPairs.Sort([](const TPair<float, int32>& A, const TPair<float, int32>& B) {
        return A.Key < B.Key;
    });

    TArray<FVectorDatabaseEntry> Results;
    for (int32 i = 0; i < FMath::Min(N, DistanceIndexPairs.Num()); ++i)
    {
        FVectorDatabaseEntry Result;
        Result.Distance = DistanceIndexPairs[i].Key;
        Result.Vector = Vectors[DistanceIndexPairs[i].Value];
        Result.Entry = Entries[DistanceIndexPairs[i].Value];
        Results.Add(Result);
    }

    return Results;
}

TArray<FVectorDatabaseEntry> UVectorDatabase::GetAllVectorEntries(const TArray<FString>& Categories) const
{
    TArray<FVectorDatabaseEntry> Results;

    int32 NumEntries = Entries.Num();
    for (int32 i = 0; i < NumEntries; ++i)
    {
        if(Categories.Num() == 0 || Categories.Contains(Entries[i]->Category))
        {
            FVectorDatabaseEntry Result;
            Result.Distance = 0.0f;
            Result.Vector = Vectors[i];
            Result.Entry = Entries[i];
            Results.Add(Result);
        }
    }

    return Results;
}



float UVectorDatabase::CalculateDistance(const TArray<float>& Vec1, const TArray<float>& Vec2) const
{
    if (Vec1.Num() != Vec2.Num())
    {
        return MAX_FLT;
    }

    float SumSquaredDiff = 0.0f;
    for (int32 i = 0; i < Vec1.Num(); ++i)
    {
        float Diff = Vec1[i] - Vec2[i];
        SumSquaredDiff += (Diff * Diff);
    }

    return FMath::Sqrt(SumSquaredDiff);
}

bool UVectorDatabase::ShouldIncludeEntry(const UVectorEntryWrapper* Entry, const TArray<FString>& Categories) const
{
    return Categories.Num() == 0 || Categories.Contains(Entry->Category);
}


int32 UVectorDatabase::GetNumberOfEntries() const
{
    return Entries.Num();
}

int32 UVectorDatabase::GetNumberOfStringEntries() const
{
    return Entries.FilterByPredicate([](const UVectorEntryWrapper* Entry) {
        return Entry->EntryType == EEntryType::String;
    }).Num();
}

int32 UVectorDatabase::GetNumberOfObjectEntries() const
{
    return Entries.FilterByPredicate([](const UVectorEntryWrapper* Entry) {
        return Entry->EntryType == EEntryType::Object;
    }).Num();
}

int32 UVectorDatabase::GetNumberOfStructEntries() const
{
    return Entries.FilterByPredicate([](const UVectorEntryWrapper* Entry) {
        return Entry->EntryType == EEntryType::Struct;
    }).Num();
}

bool UVectorDatabase::RemoveEntry(const TArray<float>& Vector, bool bRemoveAllOccurrences, float RemovalRange)
{
    bool bEntryRemoved = false;

    // Validate the input vector
    if (Vector.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("RemoveEntry called with an empty vector."));
        return false;
    }

    //clamp removalrange to above 0
    if(RemovalRange < 0.0f){
        RemovalRange = 0.0f;
    }

    // Loop through the Vectors array
    for (int32 i = 0; i < Vectors.Num(); ++i)
    {
        // Check if the current vector should be removed based on the distance or exact match
        if ((RemovalRange > 0.0f && CalculateDistance(Vectors[i], Vector) <= RemovalRange) || Vectors[i] == Vector)
        {
            // Remove the vector and corresponding entry
            Vectors.RemoveAt(i);
            Entries.RemoveAt(i);
            bEntryRemoved = true;

            // If not removing all occurrences, return immediately after the first removal
            if (!bRemoveAllOccurrences)
            {
                return true;
            }

            // Adjust index to account for the removed element
            --i;
        }
    }

    return bEntryRemoved;
}

// TArray<FVectorDatabaseEntry> UVectorDatabase::GetTopNEntriesWithDetails(const TArray<float>& QueryVector, int32 N) const
// {
//     TArray<TPair<float, int32>> DistanceIndexPairs;
    
//     for (int32 i = 0; i < Vectors.Num(); ++i)
//     {
//         if (QueryVector.Num() == Vectors[i].Num())
//         {
//             float Distance = CalculateDistance(QueryVector, Vectors[i]);
//             DistanceIndexPairs.Add(TPair<float, int32>(Distance, i));
//         }
//     }

//     DistanceIndexPairs.Sort([](const TPair<float, int32>& A, const TPair<float, int32>& B) {
//         return A.Key < B.Key;
//     });

//     TArray<FVectorDatabaseEntry> Results;
//     for (int32 i = 0; i < FMath::Min(N, DistanceIndexPairs.Num()); ++i)
//     {
//         FVectorDatabaseEntry Result;
//         Result.Distance = DistanceIndexPairs[i].Key;
//         Result.Vector = Vectors[DistanceIndexPairs[i].Value];
//         Result.Entry = Entries[DistanceIndexPairs[i].Value];
//         Results.Add(Result);
//     }

//     return Results;
// }

void UVectorEntryWrapper::SetStructData(UScriptStruct* InStructType, const void* InStructData)
{
    if (!InStructType || !InStructData)
    {
        UE_LOG(LogTemp, Error, TEXT("SetStructData: Invalid StructType or StructData"));
        return;
    }

    // Clean up previous data if it exists
    if (StructType && StructData.Num() > 0)
    {
        StructType->DestroyStruct(StructData.GetData());
    }

    StructType = InStructType;
    const int32 StructSize = StructType->GetStructureSize();

    StructData.SetNum(StructSize, false);

    // Use placement new to initialize the struct in-place
    StructType->InitializeStruct(StructData.GetData());

    // Use the struct's copy constructor for proper copying
    StructType->CopyScriptStruct(StructData.GetData(), InStructData);
}