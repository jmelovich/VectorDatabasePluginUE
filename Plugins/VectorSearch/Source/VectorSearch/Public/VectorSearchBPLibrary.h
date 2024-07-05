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
    static void AddStringEntryToVectorDatabase(UVectorDatabase* Database, const TArray<float>& Vector, FString Entry);

    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    static void AddObjectEntryToVectorDatabase(UVectorDatabase* Database, const TArray<float>& Vector, UObject* Entry);

    UFUNCTION(BlueprintCallable, CustomThunk, Category = "Vector Database", meta = (CustomStructureParam = "StructValue"))
    static void AddStructEntryToVectorDatabase(UVectorDatabase* Database, const TArray<float>& Vector, const int32& StructValue);

    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    static TArray<FString> GetTopNStringMatches(UVectorDatabase* Database, const TArray<float>& QueryVector, int32 N);

    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    static TArray<UObject*> GetTopNObjectMatches(UVectorDatabase* Database, const TArray<float>& QueryVector, int32 N);

    UFUNCTION(BlueprintCallable, CustomThunk, Category = "Vector Database", meta = (ArrayParm = "OutStructValues", CustomStructureParam = "OutStructValues"))
    static void GetTopNStructMatches(UVectorDatabase* Database, const TArray<float>& QueryVector, int32 N, TArray<int32>& OutStructValues);

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

    static void DeepCopyStruct(UScriptStruct* StructType, void* Dest, const void* Src);

    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    static TArray<FVectorDatabaseEntry> GetTopNEntriesWithDetails(UVectorDatabase* Database, const TArray<float>& QueryVector, int32 N);

    UFUNCTION(BlueprintCallable, CustomThunk, Category = "Vector Database", meta = (CustomStructureParam = "OutStruct"))
    static void GetStructFromVectorDatabaseEntry(const FVectorDatabaseEntry& Entry, int32& OutStruct);

    DECLARE_FUNCTION(execGetStructFromVectorDatabaseEntry);

    DECLARE_FUNCTION(execAddStructEntryToVectorDatabase);
    DECLARE_FUNCTION(execGetTopNStructMatches);
};