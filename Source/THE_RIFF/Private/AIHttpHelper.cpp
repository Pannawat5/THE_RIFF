#include "AIHttpHelper.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "LatentActions.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"

class FOllamaLatentAction : public FPendingLatentAction
{
public:
    FString& OutResponse;
    bool bFinished = false;

    FName ExecutionFunction;
    int32 OutputLink;
    FWeakObjectPtr CallbackTarget;

    FOllamaLatentAction(
        FString& InOutResponse,
        const FLatentActionInfo& LatentInfo
    )
        : OutResponse(InOutResponse)
        , ExecutionFunction(LatentInfo.ExecutionFunction)
        , OutputLink(LatentInfo.Linkage)
        , CallbackTarget(LatentInfo.CallbackTarget)
    {}

    virtual void UpdateOperation(FLatentResponse& Response) override
    {
        Response.FinishAndTriggerIf(
            bFinished,
            ExecutionFunction,
            OutputLink,
            CallbackTarget
        );
    }
};

void UAIHttpHelper::SendChatToOllama(
    UObject* WorldContextObject,
    FLatentActionInfo LatentInfo,
    const FString& PlayerText,
    FString& OutResponse
)
{
    if (!WorldContextObject) return;

    UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
    FLatentActionManager& LatentManager = World->GetLatentActionManager();

    if (LatentManager.FindExistingAction<FOllamaLatentAction>(
        LatentInfo.CallbackTarget,
        LatentInfo.UUID))
    {
        return;
    }

    FOllamaLatentAction* Action =
        new FOllamaLatentAction(OutResponse, LatentInfo);

    LatentManager.AddNewAction(
        LatentInfo.CallbackTarget,
        LatentInfo.UUID,
        Action
    );

    // ---------- HTTP ----------
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
        FHttpModule::Get().CreateRequest();

    Request->SetURL(TEXT("http://127.0.0.1:11434/api/generate"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    FString EscapedPrompt = PlayerText.ReplaceCharWithEscapedChar();

    const FString Body = FString::Printf(
        TEXT("{\"model\":\"llama3\",\"prompt\":\"%s\",\"stream\":false}"),
        *EscapedPrompt
    );

    Request->SetContentAsString(Body);

    Request->OnProcessRequestComplete().BindLambda(
        [Action](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess)
        {
            if (bSuccess && Res.IsValid())
            {
                TSharedPtr<FJsonObject> Json;
                TSharedRef<TJsonReader<>> Reader =
                    TJsonReaderFactory<>::Create(Res->GetContentAsString());

                if (FJsonSerializer::Deserialize(Reader, Json) && Json.IsValid())
                {
                    Action->OutResponse =
                        Json->GetStringField(TEXT("response"));
                }
                else
                {
                    Action->OutResponse = TEXT("[JSON Parse Error]");
                }
            }
            else
            {
                Action->OutResponse = TEXT("[HTTP Error]");
            }

            Action->bFinished = true;
        }
    );

    Request->ProcessRequest();
}
