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

UENUM(BlueprintType)
enum class EVectorDistanceMetric : uint8
{
    Euclidean UMETA(DisplayName = "Euclidean Distance"),
    Cosine UMETA(DisplayName = "Cosine Similarity"),
    Manhattan UMETA(DisplayName = "Manhattan Distance"),
    DotProduct UMETA(DisplayName = "Dot Product")
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
          EntryType(EEntryType::String),
          Category(TEXT("")),
          Metadata()
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Category;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FString, FString> Metadata;

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

    UPROPERTY(BlueprintReadOnly, Category = "Vector Database")
    FString Category;

    UPROPERTY(BlueprintReadOnly, Category = "Vector Database")
    TMap<FString, FString> Metadata;
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

USTRUCT(BlueprintType)
struct VECTORSEARCH_API FVectorDatabaseStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Vector Database")
    int32 TotalEntries;

    UPROPERTY(BlueprintReadOnly, Category = "Vector Database")
    int32 StringEntries;

    UPROPERTY(BlueprintReadOnly, Category = "Vector Database")
    int32 ObjectEntries;

    UPROPERTY(BlueprintReadOnly, Category = "Vector Database")
    int32 StructEntries;

    UPROPERTY(BlueprintReadOnly, Category = "Vector Database")
    int32 VectorDimension;

    UPROPERTY(BlueprintReadOnly, Category = "Vector Database")
    TArray<FString> Categories;

    UPROPERTY(BlueprintReadOnly, Category = "Vector Database")
    TMap<FString, int32> CategoryCounts;
};

UCLASS(BlueprintType, Blueprintable)
class VECTORSEARCH_API UVectorDatabase : public UObject
{
    GENERATED_BODY()

public:
    UVectorDatabase();
    virtual ~UVectorDatabase();

    /** Add a string entry to the database */
    void AddEntry(const TArray<float>& Vector, UVectorEntryWrapper* Entry, const FString& Category);
    
    /** Add a struct entry to the database */
    void AddStructEntry(const TArray<float>& Vector, UScriptStruct* StructType, const void* StructPtr, const FString& Category);

    /** Get the top N matches for a query vector */
    TArray<UVectorEntryWrapper*> GetTopNMatches(const TArray<float>& QueryVector, int32 N, EEntryType EntryType, const TArray<FString>& Categories) const;

    /** Get the top N struct matches for a query vector */
    TArray<FVectorDatabaseResult> GetTopNStructMatches(const TArray<float>& QueryVector, int32 N, const TArray<FString>& Categories) const;

    /** Get the top N entries with details for a query vector */
    TArray<FVectorDatabaseEntry> GetTopNEntriesWithDetails(const TArray<float>& QueryVector, int32 N, const TArray<FString>& Categories) const;

    /** Get all vector entries in the database */
    TArray<FVectorDatabaseEntry> GetAllVectorEntries(const TArray<FString>& Categories) const;

    /** Get the number of entries in the database */
    int32 GetNumberOfEntries() const;

    /** Get the number of string entries in the database */
    int32 GetNumberOfStringEntries() const;

    /** Get the number of object entries in the database */
    int32 GetNumberOfObjectEntries() const;

    /** Get the number of struct entries in the database */
    int32 GetNumberOfStructEntries() const;

    /** Remove an entry from the database */
    bool RemoveEntry(const TArray<float>& Vector, bool bRemoveAllOccurrences = false, float RemovalRange = 0.0f);

    /** Get database statistics */
    FVectorDatabaseStats GetDatabaseStats() const;

    /** Get all unique categories in the database */
    TArray<FString> GetUniqueCategories() const;

    /** Get the number of entries in a specific category */
    int32 GetEntryCountForCategory(const FString& Category) const;

    /** Get all entries in a specific category */
    TArray<FVectorDatabaseEntry> GetEntriesForCategory(const FString& Category) const;

    /** Set the distance metric for the database */
    void SetDistanceMetric(EVectorDistanceMetric InMetric);

    /** Get the current distance metric */
    EVectorDistanceMetric GetDistanceMetric() const;

    /** Clear all entries from the database */
    void ClearDatabase();

    /** Check if the database is empty */
    bool IsEmpty() const;

    /** Get the vector dimension of the database */
    int32 GetVectorDimension() const;

    /** Check if all vectors in the database have the same dimension */
    bool HasConsistentVectorDimension() const;

    /** Normalize all vectors in the database */
    void NormalizeVectors();

private:
    UPROPERTY()
    TArray<UVectorEntryWrapper*> Entries;

    TArray<TArray<float>> Vectors;

    EVectorDistanceMetric DistanceMetric;

    float CalculateDistance(const TArray<float>& Vec1, const TArray<float>& Vec2) const;

    bool ShouldIncludeEntry(const UVectorEntryWrapper* Entry, const TArray<FString>& Categories) const;

    void UpdateVectorDimension();
};