#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "OpenAIEmbeddingBPLibrary.generated.h"

USTRUCT(BlueprintType)
struct FOpenAIConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OpenAI")
    FString ApiEndpoint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OpenAI")
    FString Model;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OpenAI")
    FString ApiKey;


    FOpenAIConfig()
        : ApiEndpoint(TEXT("https://api.openai.com/v1/embeddings"))
        , Model(TEXT("text-embedding-3-small"))
        , ApiKey(TEXT(""))
    {
    }
};

UCLASS()
class VECTORSEARCH_API UOpenAIEmbeddingBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, meta = (Latent, LatentInfo = "LatentInfo", WorldContext = "WorldContextObject"))
    static void GenerateOpenAIEmbedding(
        UObject* WorldContextObject,
        const FString& InputText,
        const FOpenAIConfig& Config,
        FLatentActionInfo LatentInfo,
        TArray<float>& OutEmbedding);
};