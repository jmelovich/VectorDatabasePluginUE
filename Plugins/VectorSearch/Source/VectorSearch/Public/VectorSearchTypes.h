#pragma once

#include "CoreMinimal.h"
#include "Engine/UserDefinedStruct.h"
#include "VectorSearchTypes.generated.h"

USTRUCT(BlueprintType)
struct VECTORSEARCH_API FUserStructWrapper
{
    GENERATED_BODY()

    /**
     * Default constructor.
     */
    FUserStructWrapper()
        : StructType(nullptr)
        , RawBytes(TArray<uint8>())
    {}

    /** Structure type pointer. */
    // UPROPERTY(BlueprintReadWrite, Category = "Vector Database", meta = (AllowPrivateAccess = "true"))
    // TWeakObjectPtr<UScriptStruct> StructType;
    UPROPERTY(BlueprintReadWrite, Category = "Vector Database", meta = (AllowPrivateAccess = "true"))
    UScriptStruct* StructType;
    

    /** Raw byte array storing structure data. */
    UPROPERTY(BlueprintReadWrite, Category = "Vector Database", meta = (AllowPrivateAccess = "true"))
    TArray<uint8> RawBytes;

    /**
     * Sets the structure data.
     * 
     * @param InStructType Type of the structure.
     * @param InStructData Data of the structure.
     */
    void SetStructData(UScriptStruct* InStructType, const void* InStructData)
    {
        if (InStructType && InStructData)
        {
            StructType = InStructType;
            int32 StructSize = InStructType->GetStructureSize();
            RawBytes.SetNumUninitialized(StructSize);
            InStructType->CopyScriptStruct(RawBytes.GetData(), InStructData);
        }
    }

    /**
     * Gets the structure data.
     * 
     * @param OutStructData Output location for structure data.
     * @return True if successful, false otherwise.
     */
    bool GetStructData(void* OutStructData) const
    {
        if (StructType != nullptr && RawBytes.Num() > 0 && OutStructData)
        {
            StructType->CopyScriptStruct(OutStructData, RawBytes.GetData());
            return true;
        }
        return false;
    }

};