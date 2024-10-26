#include "OpenAIEmbeddingBPLibrary.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "LatentActions.h"
#include "Misc/OutputDeviceDebug.h"

class FGenerateOpenAIEmbeddingAction : public FPendingLatentAction
{
public:
    FString InputText;
    FOpenAIConfig Config;
    TArray<float> GeneratedEmbedding;  // Own the array during async operation
    TArray<float>* OutputPtr;          // Pointer to the output array
    FName ExecutionFunction;
    int32 OutputLink;
    FWeakObjectPtr CallbackTarget;
    bool bRequestComplete;

    FGenerateOpenAIEmbeddingAction(
        const FString& InInputText,
        const FOpenAIConfig& InConfig,
        TArray<float>* InOutputPtr,
        const FLatentActionInfo& LatentInfo
    )
        : InputText(InInputText)
        , Config(InConfig)
        , OutputPtr(InOutputPtr)
        , ExecutionFunction(LatentInfo.ExecutionFunction)
        , OutputLink(LatentInfo.Linkage)
        , CallbackTarget(LatentInfo.CallbackTarget)
        , bRequestComplete(false)
    {
        //GeneratedEmbedding.Reserve(1536);  // Reserve space in our owned array
        MakeRequest();
    }

    void MakeRequest()
    {
        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
        Request->OnProcessRequestComplete().BindRaw(this, &FGenerateOpenAIEmbeddingAction::OnResponseReceived);
        Request->SetURL(Config.ApiEndpoint);
        Request->SetVerb(TEXT("POST"));
        Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
        Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Config.ApiKey));

        TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
        JsonObject->SetStringField(TEXT("input"), InputText);
        JsonObject->SetStringField(TEXT("model"), Config.Model);

        FString RequestBody;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

        Request->SetContentAsString(RequestBody);
        Request->ProcessRequest();
    }

    void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful && Response.IsValid())
        {
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
            if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
            {
                const TArray<TSharedPtr<FJsonValue>>* DataArray;
                if (JsonObject->TryGetArrayField(TEXT("data"), DataArray) && DataArray->Num() > 0)
                {
                    const TSharedPtr<FJsonObject>* FirstDataObject;
                    if ((*DataArray)[0]->TryGetObject(FirstDataObject))
                    {
                        const TArray<TSharedPtr<FJsonValue>>* EmbeddingArray;
                        if ((*FirstDataObject)->TryGetArrayField(TEXT("embedding"), EmbeddingArray))
                        {
                            for (const TSharedPtr<FJsonValue>& Value : *EmbeddingArray)
                            {
                                if (Value.IsValid() && Value->Type == EJson::Number)
                                {
                                    GeneratedEmbedding.Add(static_cast<float>(Value->AsNumber()));
                                }
                            }
                        }
                    }
                }
            }
        }
        
        bRequestComplete = true;
    }

    virtual void UpdateOperation(FLatentResponse& Response) override
    {
        if (bRequestComplete)
        {
            // Only copy the results when we're sure the target object is still valid
            if (UObject* Target = CallbackTarget.Get())
            {
                if (OutputPtr)
                {
                    *OutputPtr = MoveTemp(GeneratedEmbedding);  // Move our results to the output array
                }
            }
            Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
        }
    }
};

void UOpenAIEmbeddingBPLibrary::GenerateOpenAIEmbedding(
    UObject* WorldContextObject,
    const FString& InputText,
    const FOpenAIConfig& Config,
    FLatentActionInfo LatentInfo,
    TArray<float>& OutEmbedding)
{
    if (UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject))
    {
        FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
        if (LatentActionManager.FindExistingAction<FGenerateOpenAIEmbeddingAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr)
        {
            FGenerateOpenAIEmbeddingAction* NewAction = new FGenerateOpenAIEmbeddingAction(
                InputText,
                Config,
                &OutEmbedding,
                LatentInfo
            );
            LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, NewAction);
        }
    }
}