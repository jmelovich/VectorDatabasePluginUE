#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "VectorDatabaseTypes.generated.h"

UENUM(BlueprintType)
enum class EEntryType : uint8
{
    String,
    Object,
    Struct
};

UCLASS(BlueprintType)
class VECTORSEARCH_API UVectorEntryWrapper : public UObject
{
    GENERATED_BODY()

public:
    UVectorEntryWrapper()
        : StringValue(TEXT("")),
          ObjectValue(nullptr),
          StructType(nullptr),
          EntryType(EEntryType::String)
    {
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString StringValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UObject* ObjectValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UScriptStruct* StructType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<uint8> StructData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EEntryType EntryType;

    void SetStructData(UScriptStruct* InStructType, const void* InStructData);
};

USTRUCT(BlueprintType)
struct VECTORSEARCH_API FVectorDatabaseResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Vector Database")
    float Distance;

    UPROPERTY(BlueprintReadOnly, Category = "Vector Database")
    UScriptStruct* StructType;

    UPROPERTY(BlueprintReadOnly, Category = "Vector Database")
    TArray<uint8> StructData;
};

USTRUCT(BlueprintType)
struct VECTORSEARCH_API FVectorDatabaseEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Vector Database")
    float Distance;

    UPROPERTY(BlueprintReadOnly, Category = "Vector Database")
    TArray<float> Vector;

    UPROPERTY(BlueprintReadOnly, Category = "Vector Database")
    UVectorEntryWrapper* Entry;
};

UCLASS(BlueprintType, Blueprintable)
class VECTORSEARCH_API UVectorDatabase : public UObject
{
    GENERATED_BODY()

public:
    UVectorDatabase();
    virtual ~UVectorDatabase();

    void AddEntry(const TArray<float>& Vector, UVectorEntryWrapper* Entry);
    
    void AddStructEntry(const TArray<float>& Vector, UScriptStruct* StructType, const void* StructPtr);

    TArray<UVectorEntryWrapper*> GetTopNMatches(const TArray<float>& QueryVector, int32 N, EEntryType EntryType) const;

    TArray<FVectorDatabaseResult> GetTopNStructMatches(const TArray<float>& QueryVector, int32 N) const;

    TArray<FVectorDatabaseEntry> GetTopNEntriesWithDetails(const TArray<float>& QueryVector, int32 N) const;

    int32 GetNumberOfEntries() const;

    int32 GetNumberOfStringEntries() const;

    int32 GetNumberOfObjectEntries() const;

    int32 GetNumberOfStructEntries() const;

    bool RemoveEntry(const TArray<float>& Vector, bool bRemoveAllOccurrences = false, float RemovalRange = 0.0f);

private:
    UPROPERTY()
    TArray<UVectorEntryWrapper*> Entries;

    TArray<TArray<float>> Vectors;

    float CalculateDistance(const TArray<float>& Vec1, const TArray<float>& Vec2) const;
};