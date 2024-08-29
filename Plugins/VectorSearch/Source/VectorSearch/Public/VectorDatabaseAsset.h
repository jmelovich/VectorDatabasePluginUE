#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VectorDatabaseTypes.h"
#include "VectorDatabaseAsset.generated.h"

UCLASS(BlueprintType)
class VECTORSEARCH_API UVectorDatabaseAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vector Database")
    TArray<FVectorDatabaseEntry> Entries;

    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    void SaveFromVectorDatabase(UVectorDatabase* Database);

    UFUNCTION(BlueprintCallable, Category = "Vector Database")
    UVectorDatabase* LoadToVectorDatabase() const;

    static void DeepCopyStruct(UScriptStruct* StructType, void* Dest, const void* Src);

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};