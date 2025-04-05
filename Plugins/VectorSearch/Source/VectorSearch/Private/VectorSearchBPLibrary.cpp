#include "VectorSearchBPLibrary.h"
#include "VectorSearch.h"
#include "VectorSearchTypes.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"

UVectorDatabase* UVectorSearchBPLibrary::CreateVectorDatabase()
{
    return NewObject<UVectorDatabase>();
}

void UVectorSearchBPLibrary::AddStringEntryToVectorDatabase(UVectorDatabase* Database, const TArray<float>& Vector, FString Entry, FString Category)
{
    if (Database)
    {
        UVectorEntryWrapper* Wrapper = NewObject<UVectorEntryWrapper>();
        Wrapper->StringValue = Entry;
        Wrapper->EntryType = EEntryType::String;
        Database->AddEntry(Vector, Wrapper, Category);
    }
}

void UVectorSearchBPLibrary::AddObjectEntryToVectorDatabase(UVectorDatabase* Database, const TArray<float>& Vector, UObject* Entry, FString Category)
{
    if (Database && Entry)
    {
        UVectorEntryWrapper* Wrapper = NewObject<UVectorEntryWrapper>();
        Wrapper->ObjectValue = Entry;
        Wrapper->EntryType = EEntryType::Object;
        Database->AddEntry(Vector, Wrapper, Category);
    }
}

TArray<FString> UVectorSearchBPLibrary::GetTopNStringMatches(UVectorDatabase* Database, const TArray<float>& QueryVector, int32 N, const TArray<FString>& Categories)
{
    TArray<FString> Results;
    if (Database)
    {
        TArray<UVectorEntryWrapper*> Matches = Database->GetTopNMatches(QueryVector, N, EEntryType::String, Categories);
        for (UVectorEntryWrapper* Match : Matches)
        {
            Results.Add(Match->StringValue);
        }
    }
    return Results;
}

TArray<UObject*> UVectorSearchBPLibrary::GetTopNObjectMatches(UVectorDatabase* Database, const TArray<float>& QueryVector, int32 N, const TArray<FString>& Categories)
{
    TArray<UObject*> Results;
    if (Database)
    {
        TArray<UVectorEntryWrapper*> Matches = Database->GetTopNMatches(QueryVector, N, EEntryType::Object, Categories);
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

    for (TFieldIterator<FProperty> It(StructType); It; ++It)
    {
        FProperty* Property = *It;

        if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
        {
            void* DestNestedStruct = StructProp->ContainerPtrToValuePtr<void>(Dest);
            const void* SrcNestedStruct = StructProp->ContainerPtrToValuePtr<const void>(Src);

            UScriptStruct* NestedStructType = StructProp->Struct;
            if (DestNestedStruct && SrcNestedStruct && NestedStructType)
            {
                NestedStructType->InitializeStruct(DestNestedStruct);
                DeepCopyStruct(NestedStructType, DestNestedStruct, SrcNestedStruct);
            }
        }
        else
        {
            void* DestValuePtr = Property->ContainerPtrToValuePtr<void>(Dest);
            const void* SrcValuePtr = Property->ContainerPtrToValuePtr<const void>(Src);
            Property->CopyCompleteValue(DestValuePtr, SrcValuePtr);
        }
    }
}

// DEFINE_FUNCTION(UVectorSearchBPLibrary::execAddStructEntryToVectorDatabase)
// {
//     P_GET_OBJECT(UVectorDatabase, Database);
//     P_GET_TARRAY(float, Vector);

//     Stack.StepCompiledIn<FStructProperty>(nullptr);
//     void* StructPtr = nullptr;
//     FStructProperty* StructProperty = CastField<FStructProperty>(Stack.MostRecentProperty);

//     P_GET_PROPERTY(FStrProperty, Category);

//     P_FINISH;

//     P_NATIVE_BEGIN;
//     if (StructProperty)
//     {
//         TUniquePtr<uint8[]> NewStructInstance(new uint8[StructProperty->Struct->GetStructureSize()]);
//         if (NewStructInstance)
//         {
//             StructProperty->Struct->InitializeStruct(NewStructInstance.Get());
//             DeepCopyStruct(StructProperty->Struct, NewStructInstance.Get(), StructPtr);

//             if (Database)
//             {
//                 Database->AddStructEntry(Vector, StructProperty->Struct, NewStructInstance.Get(), Category);
//             }
//         }
//         else
//         {
//             UE_LOG(LogTemp, Error, TEXT("Failed to allocate memory for new struct instance"));
//         }
//     }
//     P_NATIVE_END;
// }

DEFINE_FUNCTION(UVectorSearchBPLibrary::execAddStructEntryToVectorDatabase)
{
    P_GET_OBJECT(UVectorDatabase, Database);
    P_GET_TARRAY(float, Vector);

    Stack.StepCompiledIn<FStructProperty>(nullptr);
    void* StructPtr = nullptr;
    FStructProperty* StructProperty = CastField<FStructProperty>(Stack.MostRecentProperty);

    P_GET_PROPERTY(FStrProperty, Category);

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
                Database->AddEntry(Vector, Wrapper, Category);
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

    // Initialize Categories as an empty array by default
    TArray<FString> Categories;

    // Check if there's a Categories array provided in the blueprint
    FArrayProperty* CategoriesArrayProp = nullptr;
    Stack.StepCompiledIn<FArrayProperty>(nullptr);
    CategoriesArrayProp = CastField<FArrayProperty>(Stack.MostRecentProperty);
    if (CategoriesArrayProp)
    {
        Categories = *reinterpret_cast<TArray<FString>*>(Stack.MostRecentPropertyAddress);
    }

    Stack.StepCompiledIn<FArrayProperty>(nullptr);
    void* OutStructArrayPtr = Stack.MostRecentPropertyAddress;
    FArrayProperty* OutStructArrayProp = CastField<FArrayProperty>(Stack.MostRecentProperty);
    FStructProperty* OutStructProp = CastField<FStructProperty>(OutStructArrayProp->Inner);

    P_FINISH;

    P_NATIVE_BEGIN;
    if (Database && OutStructArrayPtr && OutStructProp)
    {
        TArray<FVectorDatabaseResult> Results = Database->GetTopNStructMatches(QueryVector, N, Categories);
        FScriptArrayHelper OutStructArray(OutStructArrayProp, OutStructArrayPtr);
        OutStructArray.EmptyAndAddUninitializedValues(Results.Num());

        for (int32 i = 0; i < Results.Num(); ++i)
        {
            if (Results[i].StructType == OutStructProp->Struct)
            {
                void* DestStructPtr = OutStructArray.GetRawPtr(i);
                OutStructProp->InitializeValue(DestStructPtr);
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

TArray<FVectorDatabaseEntry> UVectorSearchBPLibrary::GetTopNEntriesWithDetails(UVectorDatabase* Database, const TArray<float>& QueryVector, int32 N, const TArray<FString>& Categories)
{
    if (Database)
    {
        return Database->GetTopNEntriesWithDetails(QueryVector, N, Categories);
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
            OutStructProp->InitializeValue(OutStructPtr);
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

UVectorDatabaseAsset* UVectorSearchBPLibrary::CreateVectorDatabaseAsset(UVectorDatabase* Database, FString AssetName, FString PackagePath)
{
    if (!Database)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid Database provided to CreateVectorDatabaseAsset"));
        return nullptr;
    }

    // Ensure the PackagePath starts with /Game/
    if (!PackagePath.StartsWith(TEXT("/Game/")))
    {
        PackagePath = FString::Printf(TEXT("/Game/%s"), *PackagePath);
    }

    // Create a unique package name
    FString PackageName = PackagePath / AssetName;
    PackageName = PackageName.Replace(TEXT("//"), TEXT("/"));

    // Create the package
    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create package for VectorDatabaseAsset"));
        return nullptr;
    }

    // Create the asset
    UVectorDatabaseAsset* NewAsset = NewObject<UVectorDatabaseAsset>(Package, *AssetName, RF_Public | RF_Standalone);
    if (!NewAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create VectorDatabaseAsset"));
        return nullptr;
    }

    // Save the database to the asset
    NewAsset->SaveFromVectorDatabase(Database);

    // Save the package
    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
    if (PackageFileName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to generate package filename for VectorDatabaseAsset"));
        return nullptr;
    }

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    bool bSaved = UPackage::SavePackage(Package, NewAsset, *PackageFileName, SaveArgs);
    if (!bSaved)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save VectorDatabaseAsset package"));
        return nullptr;
    }

    // Notify the asset registry
    FAssetRegistryModule::AssetCreated(NewAsset);

    return NewAsset;
}

UVectorDatabase* UVectorSearchBPLibrary::LoadVectorDatabaseFromAsset(UVectorDatabaseAsset* Asset)
{
    if (!Asset)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid Asset provided to LoadVectorDatabaseFromAsset"));
        return nullptr;
    }

    return Asset->LoadToVectorDatabase();
}

bool UVectorSearchBPLibrary::SaveVectorDatabaseToFile(UVectorDatabaseAsset* Asset, const FString& FilePath)
{
    if (!Asset)
    {
        UE_LOG(LogTemp, Error, TEXT("SaveVectorDatabaseToFile: Invalid Asset"));
        return false;
    }

    return Asset->SaveToFile(FilePath);
}

bool UVectorSearchBPLibrary::LoadVectorDatabaseFromFile(UVectorDatabaseAsset* Asset, const FString& FilePath)
{
    if (!Asset)
    {
        UE_LOG(LogTemp, Error, TEXT("LoadVectorDatabaseFromFile: Invalid Asset"));
        return false;
    }

    return Asset->LoadFromFile(FilePath);
}

TArray<FString> UVectorSearchBPLibrary::GetUniqueCategoriesFromDatabase(UVectorDatabase* Database)
{
    if (!Database)
    {
        UE_LOG(LogTemp, Error, TEXT("GetUniqueCategoriesFromDatabase: Invalid Database"));
        return TArray<FString>();
    }

    return Database->GetUniqueCategories();
}

int32 UVectorSearchBPLibrary::GetEntryCountForCategoryInDatabase(UVectorDatabase* Database, const FString& Category)
{
    if (!Database)
    {
        UE_LOG(LogTemp, Error, TEXT("GetEntryCountForCategoryInDatabase: Invalid Database"));
        return 0;
    }

    return Database->GetEntryCountForCategory(Category);
}

TArray<FVectorDatabaseEntry> UVectorSearchBPLibrary::GetEntriesForCategoryFromDatabase(UVectorDatabase* Database, const FString& Category)
{
    if (!Database)
    {
        UE_LOG(LogTemp, Error, TEXT("GetEntriesForCategoryFromDatabase: Invalid Database"));
        return TArray<FVectorDatabaseEntry>();
    }

    return Database->GetEntriesForCategory(Category);
}

void UVectorSearchBPLibrary::SetVectorDatabaseDistanceMetric(UVectorDatabase* Database, EVectorDistanceMetric Metric)
{
    if (!Database)
    {
        UE_LOG(LogTemp, Error, TEXT("SetVectorDatabaseDistanceMetric: Invalid Database"));
        return;
    }

    Database->SetDistanceMetric(Metric);
}

EVectorDistanceMetric UVectorSearchBPLibrary::GetVectorDatabaseDistanceMetric(UVectorDatabase* Database)
{
    if (!Database)
    {
        UE_LOG(LogTemp, Error, TEXT("GetVectorDatabaseDistanceMetric: Invalid Database"));
        return EVectorDistanceMetric::Euclidean;
    }

    return Database->GetDistanceMetric();
}

void UVectorSearchBPLibrary::ClearVectorDatabase(UVectorDatabase* Database)
{
    if (!Database)
    {
        UE_LOG(LogTemp, Error, TEXT("ClearVectorDatabase: Invalid Database"));
        return;
    }

    Database->ClearDatabase();
}

bool UVectorSearchBPLibrary::IsVectorDatabaseEmpty(UVectorDatabase* Database)
{
    if (!Database)
    {
        UE_LOG(LogTemp, Error, TEXT("IsVectorDatabaseEmpty: Invalid Database"));
        return true;
    }

    return Database->IsEmpty();
}

int32 UVectorSearchBPLibrary::GetVectorDimensionFromDatabase(UVectorDatabase* Database)
{
    if (!Database)
    {
        UE_LOG(LogTemp, Error, TEXT("GetVectorDimensionFromDatabase: Invalid Database"));
        return 0;
    }

    return Database->GetVectorDimension();
}

bool UVectorSearchBPLibrary::HasConsistentVectorDimensionInDatabase(UVectorDatabase* Database)
{
    if (!Database)
    {
        UE_LOG(LogTemp, Error, TEXT("HasConsistentVectorDimensionInDatabase: Invalid Database"));
        return false;
    }

    return Database->HasConsistentVectorDimension();
}

void UVectorSearchBPLibrary::NormalizeVectorsInDatabase(UVectorDatabase* Database)
{
    if (!Database)
    {
        UE_LOG(LogTemp, Error, TEXT("NormalizeVectorsInDatabase: Invalid Database"));
        return;
    }

    Database->NormalizeVectors();
}

FVectorDatabaseStats UVectorSearchBPLibrary::GetVectorDatabaseStats(UVectorDatabase* Database)
{
    if (!Database)
    {
        UE_LOG(LogTemp, Error, TEXT("GetVectorDatabaseStats: Invalid Database"));
        return FVectorDatabaseStats();
    }

    return Database->GetDatabaseStats();
}

TArray<FString> UVectorSearchBPLibrary::GetUniqueCategoriesFromAsset(UVectorDatabaseAsset* Asset)
{
    if (!Asset)
    {
        UE_LOG(LogTemp, Error, TEXT("GetUniqueCategoriesFromAsset: Invalid Asset"));
        return TArray<FString>();
    }

    return Asset->GetUniqueCategories();
}

int32 UVectorSearchBPLibrary::GetEntryCountForCategoryInAsset(UVectorDatabaseAsset* Asset, const FString& Category)
{
    if (!Asset)
    {
        UE_LOG(LogTemp, Error, TEXT("GetEntryCountForCategoryInAsset: Invalid Asset"));
        return 0;
    }

    return Asset->GetEntryCountForCategory(Category);
}

TArray<FVectorDatabaseEntry> UVectorSearchBPLibrary::GetEntriesForCategoryFromAsset(UVectorDatabaseAsset* Asset, const FString& Category)
{
    if (!Asset)
    {
        UE_LOG(LogTemp, Error, TEXT("GetEntriesForCategoryFromAsset: Invalid Asset"));
        return TArray<FVectorDatabaseEntry>();
    }

    return Asset->GetEntriesForCategory(Category);
}
