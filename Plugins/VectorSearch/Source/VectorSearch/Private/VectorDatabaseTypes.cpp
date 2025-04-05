#include "VectorDatabaseTypes.h"
#include "Algo/Sort.h"
#include "Misc/DefaultValueHelper.h"

UVectorDatabase::UVectorDatabase()
{
    Entries.Empty();
    Vectors.Empty();
    DistanceMetric = EVectorDistanceMetric::Euclidean;
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

    // Validate vector dimension consistency
    if (Vectors.Num() > 0 && Vectors[0].Num() != Vector.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("AddEntry: Vector dimension mismatch. Expected %d, got %d"), 
               Vectors[0].Num(), Vector.Num());
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

    // Sort based on distance metric
    if (DistanceMetric == EVectorDistanceMetric::Cosine || DistanceMetric == EVectorDistanceMetric::DotProduct)
    {
        // For similarity metrics, higher values are better
        DistanceEntryPairs.Sort([](const TPair<float, UVectorEntryWrapper*>& A, const TPair<float, UVectorEntryWrapper*>& B) {
            return A.Key > B.Key;
        });
    }
    else
    {
        // For distance metrics, lower values are better
        DistanceEntryPairs.Sort([](const TPair<float, UVectorEntryWrapper*>& A, const TPair<float, UVectorEntryWrapper*>& B) {
            return A.Key < B.Key;
        });
    }

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

    // Sort based on distance metric
    if (DistanceMetric == EVectorDistanceMetric::Cosine || DistanceMetric == EVectorDistanceMetric::DotProduct)
    {
        // For similarity metrics, higher values are better
        DistanceEntryPairs.Sort([](const TPair<float, UVectorEntryWrapper*>& A, const TPair<float, UVectorEntryWrapper*>& B) {
            return A.Key > B.Key;
        });
    }
    else
    {
        // For distance metrics, lower values are better
        DistanceEntryPairs.Sort([](const TPair<float, UVectorEntryWrapper*>& A, const TPair<float, UVectorEntryWrapper*>& B) {
            return A.Key < B.Key;
        });
    }

    TArray<FVectorDatabaseResult> Results;
    for (int32 i = 0; i < FMath::Min(N, DistanceEntryPairs.Num()); ++i)
    {
        FVectorDatabaseResult Result;
        Result.Distance = DistanceEntryPairs[i].Key;
        Result.StructType = DistanceEntryPairs[i].Value->StructType;
        Result.StructData = DistanceEntryPairs[i].Value->StructData;
        Result.Category = DistanceEntryPairs[i].Value->Category;
        Result.Metadata = DistanceEntryPairs[i].Value->Metadata;
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

    // Sort based on distance metric
    if (DistanceMetric == EVectorDistanceMetric::Cosine || DistanceMetric == EVectorDistanceMetric::DotProduct)
    {
        // For similarity metrics, higher values are better
        DistanceIndexPairs.Sort([](const TPair<float, int32>& A, const TPair<float, int32>& B) {
            return A.Key > B.Key;
        });
    }
    else
    {
        // For distance metrics, lower values are better
        DistanceIndexPairs.Sort([](const TPair<float, int32>& A, const TPair<float, int32>& B) {
            return A.Key < B.Key;
        });
    }

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

    switch (DistanceMetric)
    {
        case EVectorDistanceMetric::Euclidean:
        {
            float SumSquaredDiff = 0.0f;
            for (int32 i = 0; i < Vec1.Num(); ++i)
            {
                float Diff = Vec1[i] - Vec2[i];
                SumSquaredDiff += (Diff * Diff);
            }
            return FMath::Sqrt(SumSquaredDiff);
        }
        
        case EVectorDistanceMetric::Manhattan:
        {
            float SumAbsDiff = 0.0f;
            for (int32 i = 0; i < Vec1.Num(); ++i)
            {
                SumAbsDiff += FMath::Abs(Vec1[i] - Vec2[i]);
            }
            return SumAbsDiff;
        }
        
        case EVectorDistanceMetric::Cosine:
        {
            float DotProduct = 0.0f;
            float Norm1 = 0.0f;
            float Norm2 = 0.0f;
            
            for (int32 i = 0; i < Vec1.Num(); ++i)
            {
                DotProduct += Vec1[i] * Vec2[i];
                Norm1 += Vec1[i] * Vec1[i];
                Norm2 += Vec2[i] * Vec2[i];
            }
            
            Norm1 = FMath::Sqrt(Norm1);
            Norm2 = FMath::Sqrt(Norm2);
            
            if (Norm1 == 0.0f || Norm2 == 0.0f)
            {
                return 0.0f;
            }
            
            // Return 1 - cosine similarity to convert to a distance (0 means identical)
            return 1.0f - (DotProduct / (Norm1 * Norm2));
        }
        
        case EVectorDistanceMetric::DotProduct:
        {
            float DotProduct = 0.0f;
            for (int32 i = 0; i < Vec1.Num(); ++i)
            {
                DotProduct += Vec1[i] * Vec2[i];
            }
            return DotProduct;
        }
        
        default:
            return MAX_FLT;
    }
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

    // Loop through the Vectors array in reverse to avoid index shifting issues
    for (int32 i = Vectors.Num() - 1; i >= 0; --i)
    {
        // Check if the current vector should be removed based on the distance or exact match
        if ((RemovalRange > 0.0f && CalculateDistance(Vectors[i], Vector) <= RemovalRange) || Vectors[i] == Vector)
        {
            // Remove the entry and vector
            if (Entries[i] && Entries[i]->IsValidLowLevel())
            {
                Entries[i]->ConditionalBeginDestroy();
            }
            Entries.RemoveAt(i);
            Vectors.RemoveAt(i);
            bEntryRemoved = true;

            // If we're not removing all occurrences, break after the first match
            if (!bRemoveAllOccurrences)
            {
                break;
            }
        }
    }

    return bEntryRemoved;
}

FVectorDatabaseStats UVectorDatabase::GetDatabaseStats() const
{
    FVectorDatabaseStats Stats;
    Stats.TotalEntries = Entries.Num();
    Stats.StringEntries = GetNumberOfStringEntries();
    Stats.ObjectEntries = GetNumberOfObjectEntries();
    Stats.StructEntries = GetNumberOfStructEntries();
    Stats.VectorDimension = GetVectorDimension();
    
    // Collect categories and counts
    TMap<FString, int32> CategoryCountMap;
    for (const UVectorEntryWrapper* Entry : Entries)
    {
        if (!Entry->Category.IsEmpty())
        {
            if (!CategoryCountMap.Contains(Entry->Category))
            {
                CategoryCountMap.Add(Entry->Category, 0);
                Stats.Categories.Add(Entry->Category);
            }
            CategoryCountMap[Entry->Category]++;
        }
    }
    
    Stats.CategoryCounts = CategoryCountMap;
    
    return Stats;
}

TArray<FString> UVectorDatabase::GetUniqueCategories() const
{
    TSet<FString> UniqueCategories;
    for (const UVectorEntryWrapper* Entry : Entries)
    {
        if (!Entry->Category.IsEmpty())
        {
            UniqueCategories.Add(Entry->Category);
        }
    }
    
    TArray<FString> Result;
    for (const FString& Category : UniqueCategories)
    {
        Result.Add(Category);
    }
    
    return Result;
}

int32 UVectorDatabase::GetEntryCountForCategory(const FString& Category) const
{
    int32 Count = 0;
    for (const UVectorEntryWrapper* Entry : Entries)
    {
        if (Entry->Category == Category)
        {
            Count++;
        }
    }
    return Count;
}

TArray<FVectorDatabaseEntry> UVectorDatabase::GetEntriesForCategory(const FString& Category) const
{
    TArray<FVectorDatabaseEntry> Result;
    
    for (int32 i = 0; i < Entries.Num(); ++i)
    {
        if (Entries[i]->Category == Category)
        {
            FVectorDatabaseEntry Entry;
            Entry.Distance = 0.0f;
            Entry.Vector = Vectors[i];
            Entry.Entry = Entries[i];
            Result.Add(Entry);
        }
    }
    
    return Result;
}

void UVectorDatabase::SetDistanceMetric(EVectorDistanceMetric InMetric)
{
    DistanceMetric = InMetric;
}

EVectorDistanceMetric UVectorDatabase::GetDistanceMetric() const
{
    return DistanceMetric;
}

void UVectorDatabase::ClearDatabase()
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

bool UVectorDatabase::IsEmpty() const
{
    return Entries.Num() == 0;
}

int32 UVectorDatabase::GetVectorDimension() const
{
    if (Vectors.Num() == 0)
    {
        return 0;
    }
    
    return Vectors[0].Num();
}

bool UVectorDatabase::HasConsistentVectorDimension() const
{
    if (Vectors.Num() <= 1)
    {
        return true;
    }
    
    int32 Dimension = Vectors[0].Num();
    for (int32 i = 1; i < Vectors.Num(); ++i)
    {
        if (Vectors[i].Num() != Dimension)
        {
            return false;
        }
    }
    
    return true;
}

void UVectorDatabase::NormalizeVectors()
{
    for (int32 i = 0; i < Vectors.Num(); ++i)
    {
        float Norm = 0.0f;
        for (int32 j = 0; j < Vectors[i].Num(); ++j)
        {
            Norm += Vectors[i][j] * Vectors[i][j];
        }
        
        Norm = FMath::Sqrt(Norm);
        
        if (Norm > 0.0f)
        {
            for (int32 j = 0; j < Vectors[i].Num(); ++j)
            {
                Vectors[i][j] /= Norm;
            }
        }
    }
}

void UVectorDatabase::UpdateVectorDimension()
{
    // This function can be used to validate and update vector dimensions
    // For now, it's just a placeholder for future implementation
}

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