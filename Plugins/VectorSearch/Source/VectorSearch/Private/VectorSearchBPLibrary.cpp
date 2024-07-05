#include "VectorSearchBPLibrary.h"
#include "VectorSearch.h"
#include "VectorSearchTypes.h" // Add this line to include FUserStructWrapper

UVectorDatabase* UVectorSearchBPLibrary::CreateVectorDatabase()
{
    return NewObject<UVectorDatabase>();
}

void UVectorSearchBPLibrary::AddStringEntryToVectorDatabase(UVectorDatabase* Database, const TArray<float>& Vector, FString Entry)
{
    if (Database)
    {
        UVectorEntryWrapper* Wrapper = NewObject<UVectorEntryWrapper>();
        Wrapper->StringValue = Entry;
        Wrapper->EntryType = EEntryType::String;
        Database->AddEntry(Vector, Wrapper);
    }
}

void UVectorSearchBPLibrary::AddObjectEntryToVectorDatabase(UVectorDatabase* Database, const TArray<float>& Vector, UObject* Entry)
{
    if (Database && Entry)
    {
        UVectorEntryWrapper* Wrapper = NewObject<UVectorEntryWrapper>();
        Wrapper->ObjectValue = Entry;
        Wrapper->EntryType = EEntryType::Object;
        Database->AddEntry(Vector, Wrapper);
    }
}

TArray<FString> UVectorSearchBPLibrary::GetTopNStringMatches(UVectorDatabase* Database, const TArray<float>& QueryVector, int32 N)
{
    TArray<FString> Results;
    if (Database)
    {
        TArray<UVectorEntryWrapper*> Matches = Database->GetTopNMatches(QueryVector, N, EEntryType::String);
        for (UVectorEntryWrapper* Match : Matches)
        {
            Results.Add(Match->StringValue);
        }
    }
    return Results;
}

TArray<UObject*> UVectorSearchBPLibrary::GetTopNObjectMatches(UVectorDatabase* Database, const TArray<float>& QueryVector, int32 N)
{
    TArray<UObject*> Results;
    if (Database)
    {
        TArray<UVectorEntryWrapper*> Matches = Database->GetTopNMatches(QueryVector, N, EEntryType::Object);
        for (UVectorEntryWrapper* Match : Matches)
        {
            if (Match->ObjectValue)
            {
                Results.Add(Match->ObjectValue);
            }
        }
    }
    return Results;
}

void UVectorSearchBPLibrary::DeepCopyStruct(UScriptStruct* StructType, void* Dest, const void* Src)
{
    if (!StructType || !Dest || !Src)
    {
        UE_LOG(LogTemp, Error, TEXT("Arg invalid, failed to deepcopy struct. Returning..."));
        return;
    }

    // Iterate over all properties of the struct
    for (TFieldIterator<FProperty> It(StructType); It; ++It)
    {
        FProperty* Property = *It;

        if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
        {
            // Find pointers to the nested structs in both the destination and source
            void* DestNestedStruct = StructProp->ContainerPtrToValuePtr<void>(Dest);
            const void* SrcNestedStruct = StructProp->ContainerPtrToValuePtr<const void>(Src);

            UScriptStruct* NestedStructType = StructProp->Struct;
            if (DestNestedStruct && SrcNestedStruct && NestedStructType)
            {
                // Initialize the destination nested struct instance
                NestedStructType->InitializeStruct(DestNestedStruct);

                // Recursively copy the contents of the nested struct
                DeepCopyStruct(NestedStructType, DestNestedStruct, SrcNestedStruct);
            }
        }
        else
        {
            // For simple properties, use property-based assignment
            void* DestValuePtr = Property->ContainerPtrToValuePtr<void>(Dest);
            const void* SrcValuePtr = Property->ContainerPtrToValuePtr<const void>(Src);
            Property->CopyCompleteValue(DestValuePtr, SrcValuePtr);
        }
    }
}

DEFINE_FUNCTION(UVectorSearchBPLibrary::execAddStructEntryToVectorDatabase)
{
    P_GET_OBJECT(UVectorDatabase, Database);
    P_GET_TARRAY(float, Vector);

    Stack.StepCompiledIn<FStructProperty>(nullptr);
    void* StructPtr = nullptr;
    FStructProperty* StructProperty = CastField<FStructProperty>(Stack.MostRecentProperty);

    if (StructProperty)
    {
        void* ContainerPtr = Stack.MostRecentPropertyContainer;
        StructPtr = StructProperty->ContainerPtrToValuePtr<void>(ContainerPtr);

        // Create a new instance of the struct and copy data into it
        void* NewStructInstance = FMemory::Malloc(StructProperty->Struct->GetStructureSize());
        if (NewStructInstance)
        {
            StructProperty->Struct->InitializeStruct(NewStructInstance);
            DeepCopyStruct(StructProperty->Struct, NewStructInstance, StructPtr);

            // Update StructPtr to point to the new struct instance
            StructPtr = NewStructInstance;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to allocate memory for new struct instance"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Struct property not found"));
    }

    P_FINISH;


    P_NATIVE_BEGIN;
    if (StructProperty)
    {
        TUniquePtr<uint8[]> NewStructInstance(new uint8[StructProperty->Struct->GetStructureSize()]);
        if (NewStructInstance)
        {
            StructProperty->Struct->InitializeStruct(NewStructInstance.Get());
            DeepCopyStruct(StructProperty->Struct, NewStructInstance.Get(), StructPtr);

            if (Database)
            {
                UVectorEntryWrapper* Wrapper = NewObject<UVectorEntryWrapper>();
                Wrapper->SetStructData(StructProperty->Struct, NewStructInstance.Get());
                Wrapper->EntryType = EEntryType::Struct;
                Database->AddEntry(Vector, Wrapper);
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to allocate memory for new struct instance"));
        }
    }
    P_NATIVE_END;
}

DEFINE_FUNCTION(UVectorSearchBPLibrary::execGetTopNStructMatches)
{
    P_GET_OBJECT(UVectorDatabase, Database);
    P_GET_TARRAY(float, QueryVector);
    P_GET_PROPERTY(FIntProperty, N);

    Stack.StepCompiledIn<FArrayProperty>(nullptr);
    void* OutStructArrayPtr = Stack.MostRecentPropertyAddress;
    FArrayProperty* OutStructArrayProp = CastField<FArrayProperty>(Stack.MostRecentProperty);
    FStructProperty* OutStructProp = CastField<FStructProperty>(OutStructArrayProp->Inner);

    P_FINISH;

    P_NATIVE_BEGIN;
    if (Database && OutStructArrayPtr && OutStructProp)
    {
        TArray<FVectorDatabaseResult> Results = Database->GetTopNStructMatches(QueryVector, N);
        FScriptArrayHelper OutStructArray(OutStructArrayProp, OutStructArrayPtr);
        OutStructArray.EmptyAndAddUninitializedValues(Results.Num());

        for (int32 i = 0; i < Results.Num(); ++i)
        {
            if (Results[i].StructType == OutStructProp->Struct)
            {
                void* DestStructPtr = OutStructArray.GetRawPtr(i);

                // Ensure proper initialization of the destination struct
                OutStructProp->InitializeValue(DestStructPtr);

                // Copy the structure data directly into the destination pointer.
                OutStructProp->Struct->CopyScriptStruct(DestStructPtr, Results[i].StructData.GetData());
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Struct type mismatch in GetTopNStructMatches"));
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get top N struct matches. Database: %p, OutStructArrayPtr: %p, OutStructProp: %p"), 
               Database, OutStructArrayPtr, OutStructProp);
    }
    P_NATIVE_END;
}

int32 UVectorSearchBPLibrary::GetNumberOfEntriesInVectorDatabase(UVectorDatabase* Database)
{
    if (Database)
    {
        return Database->GetNumberOfEntries();
    }
    return 0;
}

int32 UVectorSearchBPLibrary::GetNumberOfStringEntriesInVectorDatabase(UVectorDatabase* Database)
{
    if (Database)
    {
        return Database->GetNumberOfStringEntries();
    }
    return 0;
}

int32 UVectorSearchBPLibrary::GetNumberOfObjectEntriesInVectorDatabase(UVectorDatabase* Database)
{
    if (Database)
    {
        return Database->GetNumberOfObjectEntries();
    }
    return 0;
}

int32 UVectorSearchBPLibrary::GetNumberOfStructEntriesInVectorDatabase(UVectorDatabase* Database)
{
    if (Database)
    {
        return Database->GetNumberOfStructEntries();
    }
    return 0;
}

bool UVectorSearchBPLibrary::RemoveEntryFromVectorDatabase(UVectorDatabase* Database, const TArray<float>& Vector, bool bRemoveAllOccurrences, float RemovalRange)
{
    if (Database)
    {
        return Database->RemoveEntry(Vector, bRemoveAllOccurrences, RemovalRange);
    }
    return false;
}

TArray<FVectorDatabaseEntry> UVectorSearchBPLibrary::GetTopNEntriesWithDetails(UVectorDatabase* Database, const TArray<float>& QueryVector, int32 N)
{
    if (Database)
    {
        return Database->GetTopNEntriesWithDetails(QueryVector, N);
    }
    return TArray<FVectorDatabaseEntry>();
}

DEFINE_FUNCTION(UVectorSearchBPLibrary::execGetStructFromVectorDatabaseEntry)
{
    P_GET_STRUCT_REF(FVectorDatabaseEntry, Entry);

    Stack.StepCompiledIn<FStructProperty>(nullptr);
    void* OutStructPtr = Stack.MostRecentPropertyAddress;
    FStructProperty* OutStructProp = CastField<FStructProperty>(Stack.MostRecentProperty);

    P_FINISH;

    P_NATIVE_BEGIN;
    if (OutStructPtr && OutStructProp && Entry.Entry && Entry.Entry->EntryType == EEntryType::Struct)
    {
        if (Entry.Entry->StructType == OutStructProp->Struct)
        {
            // Ensure proper initialization of the destination struct
            OutStructProp->InitializeValue(OutStructPtr);

            // Copy the structure data directly into the destination pointer.
            OutStructProp->Struct->CopyScriptStruct(OutStructPtr, Entry.Entry->StructData.GetData());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Struct type mismatch in GetStructFromVectorDatabaseEntry"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get struct from VectorDatabaseEntry. OutStructPtr: %p, OutStructProp: %p, Entry.Entry: %p"), 
               OutStructPtr, OutStructProp, Entry.Entry);
    }
    P_NATIVE_END;
}