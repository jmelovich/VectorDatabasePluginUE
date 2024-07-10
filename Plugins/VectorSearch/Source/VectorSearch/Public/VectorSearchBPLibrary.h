#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "VectorDatabaseTypes.h"
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

    static void DeepCopyStruct(UScriptStruct* StructType, void* Dest, const void* Src);

    DECLARE_FUNCTION(execGetStructFromVectorDatabaseEntry);
    DECLARE_FUNCTION(execAddStructEntryToVectorDatabase);
    DECLARE_FUNCTION(execGetTopNStructMatches);
};