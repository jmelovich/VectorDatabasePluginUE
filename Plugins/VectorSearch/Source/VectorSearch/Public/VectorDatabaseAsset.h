#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VectorDatabaseTypes.h"
#include "VectorDatabaseAsset.generated.h"

/**
 * Asset class for storing and loading vector databases
 * Provides efficient serialization and persistence of vector databases
 */
UCLASS(BlueprintType)
class VECTORSEARCH_API UVectorDatabaseAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    /** The vector database entries stored in this asset */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vector Database")
    TArray<FVectorDatabaseEntry> Entries;

    /** Metadata about the database */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vector Database")
    FString DatabaseName;

    /** Description of the database */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vector Database")
    FString Description;

    /** Creation date of the database */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vector Database")
    FDateTime CreationDate;

    /** Last modification date of the database */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vector Database")
    FDateTime LastModifiedDate;

    /** Vector dimension for all entries in this database */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vector Database")
    int32 VectorDimension;

    /** Categories present in this database */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vector Database")
    TArray<FString> Categories;

    /** Save a vector database to this asset */
    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    void SaveFromVectorDatabase(UVectorDatabase* Database);

    /** Load a vector database from this asset */
    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    UVectorDatabase* LoadToVectorDatabase() const;

    /** Save the database to a file */
    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    bool SaveToFile(const FString& FilePath);

    /** Load the database from a file */
    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    bool LoadFromFile(const FString& FilePath);

    /** Get all unique categories in the database */
    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    TArray<FString> GetUniqueCategories() const;

    /** Get the number of entries in a specific category */
    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    int32 GetEntryCountForCategory(const FString& Category) const;

    /** Get all entries in a specific category */
    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    TArray<FVectorDatabaseEntry> GetEntriesForCategory(const FString& Category) const;

    /** Deep copy a struct from source to destination */
    static void DeepCopyStruct(UScriptStruct* StructType, void* Dest, const void* Src);

    /** Constructor */
    UVectorDatabaseAsset();

    /** Initialize the asset */
    virtual void PostInitProperties() override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};