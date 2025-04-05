#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "VectorDatabaseTypes.h"
#include "VectorDatabaseAsset.h"
#include "VectorSearchBPLibrary.generated.h"

UCLASS()
class VECTORSEARCH_API UVectorSearchBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    static UVectorDatabase* CreateVectorDatabase();

    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    static void AddStringEntryToVectorDatabase(UVectorDatabase* Database, const TArray<float>& Vector, FString Entry, FString Category);

    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    static void AddObjectEntryToVectorDatabase(UVectorDatabase* Database, const TArray<float>& Vector, UObject* Entry, FString Category);

    UFUNCTION(BlueprintCallable, CustomThunk, Category = "Vector Database", meta = (CustomStructureParam = "StructValue"))
    static void AddStructEntryToVectorDatabase(UVectorDatabase* Database, const TArray<float>& Vector, const int32& StructValue, FString Category);

    UFUNCTION(BlueprintCallable, Category = "Vector Database", meta = (AutoCreateRefTerm = "Categories"))
    static TArray<FString> GetTopNStringMatches(UVectorDatabase* Database, const TArray<float>& QueryVector, int32 N, const TArray<FString>& Categories);

    UFUNCTION(BlueprintCallable, Category = "Vector Database", meta = (AutoCreateRefTerm = "Categories"))
    static TArray<UObject*> GetTopNObjectMatches(UVectorDatabase* Database, const TArray<float>& QueryVector, int32 N, const TArray<FString>& Categories);

    UFUNCTION(BlueprintCallable, CustomThunk, Category = "Vector Database", meta = (ArrayParm = "OutStructValues", CustomStructureParam = "OutStructValues", AutoCreateRefTerm = "Categories"))
    static void GetTopNStructMatches(UVectorDatabase* Database, const TArray<float>& QueryVector, int32 N, const TArray<FString>& Categories, TArray<int32>& OutStructValues);

    UFUNCTION(BlueprintPure, Category = "Vector Database")
    static int32 GetNumberOfEntriesInVectorDatabase(UVectorDatabase* Database);

    UFUNCTION(BlueprintPure, Category = "Vector Database")
    static int32 GetNumberOfStringEntriesInVectorDatabase(UVectorDatabase* Database);

    UFUNCTION(BlueprintPure, Category = "Vector Database")
    static int32 GetNumberOfObjectEntriesInVectorDatabase(UVectorDatabase* Database);

    UFUNCTION(BlueprintPure, Category = "Vector Database")
    static int32 GetNumberOfStructEntriesInVectorDatabase(UVectorDatabase* Database);

    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    static bool RemoveEntryFromVectorDatabase(UVectorDatabase* Database, const TArray<float>& Vector, bool bRemoveAllOccurrences = false, float RemovalRange = 0.0f);

    UFUNCTION(BlueprintCallable, Category = "Vector Database", meta = (AutoCreateRefTerm = "Categories"))
    static TArray<FVectorDatabaseEntry> GetTopNEntriesWithDetails(UVectorDatabase* Database, const TArray<float>& QueryVector, int32 N, const TArray<FString>& Categories);

    UFUNCTION(BlueprintCallable, CustomThunk, Category = "Vector Database", meta = (CustomStructureParam = "OutStruct"))
    static void GetStructFromVectorDatabaseEntry(const FVectorDatabaseEntry& Entry, int32& OutStruct);

    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    static UVectorDatabaseAsset* CreateVectorDatabaseAsset(UVectorDatabase* Database, FString AssetName, FString PackagePath);

    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    static UVectorDatabase* LoadVectorDatabaseFromAsset(UVectorDatabaseAsset* Asset);

    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    static bool SaveVectorDatabaseToFile(UVectorDatabaseAsset* Asset, const FString& FilePath);

    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    static bool LoadVectorDatabaseFromFile(UVectorDatabaseAsset* Asset, const FString& FilePath);

    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    static TArray<FString> GetUniqueCategoriesFromDatabase(UVectorDatabase* Database);

    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    static int32 GetEntryCountForCategoryInDatabase(UVectorDatabase* Database, const FString& Category);

    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    static TArray<FVectorDatabaseEntry> GetEntriesForCategoryFromDatabase(UVectorDatabase* Database, const FString& Category);

    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    static void SetVectorDatabaseDistanceMetric(UVectorDatabase* Database, EVectorDistanceMetric Metric);

    UFUNCTION(BlueprintPure, Category = "Vector Database")
    static EVectorDistanceMetric GetVectorDatabaseDistanceMetric(UVectorDatabase* Database);

    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    static void ClearVectorDatabase(UVectorDatabase* Database);

    UFUNCTION(BlueprintPure, Category = "Vector Database")
    static bool IsVectorDatabaseEmpty(UVectorDatabase* Database);

    UFUNCTION(BlueprintPure, Category = "Vector Database")
    static int32 GetVectorDimensionFromDatabase(UVectorDatabase* Database);

    UFUNCTION(BlueprintPure, Category = "Vector Database")
    static bool HasConsistentVectorDimensionInDatabase(UVectorDatabase* Database);

    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    static void NormalizeVectorsInDatabase(UVectorDatabase* Database);

    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    static FVectorDatabaseStats GetVectorDatabaseStats(UVectorDatabase* Database);

    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    static TArray<FString> GetUniqueCategoriesFromAsset(UVectorDatabaseAsset* Asset);

    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    static int32 GetEntryCountForCategoryInAsset(UVectorDatabaseAsset* Asset, const FString& Category);

    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    static TArray<FVectorDatabaseEntry> GetEntriesForCategoryFromAsset(UVectorDatabaseAsset* Asset, const FString& Category);

    static void DeepCopyStruct(UScriptStruct* StructType, void* Dest, const void* Src);

    DECLARE_FUNCTION(execGetStructFromVectorDatabaseEntry);
    DECLARE_FUNCTION(execAddStructEntryToVectorDatabase);
    DECLARE_FUNCTION(execGetTopNStructMatches);
};