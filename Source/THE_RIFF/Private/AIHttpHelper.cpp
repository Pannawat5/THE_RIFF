#include "AIHttpHelper.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "LatentActions.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

#include "Json.h"
#include "JsonUtilities.h"

class FOpenAILatentAction : public FPendingLatentAction
{
public:
    FString& OutResponse;
    bool bFinished = false;

    FName ExecutionFunction;
    int32 OutputLink;
    FWeakObjectPtr CallbackTarget;

    FOpenAILatentAction(
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

    if (LatentManager.FindExistingAction<FOpenAILatentAction>(
        LatentInfo.CallbackTarget,
        LatentInfo.UUID))
    {
        return;
    }

    FOpenAILatentAction* Action =
        new FOpenAILatentAction(OutResponse, LatentInfo);

    LatentManager.AddNewAction(
        LatentInfo.CallbackTarget,
        LatentInfo.UUID,
        Action
    );

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
        FHttpModule::Get().CreateRequest();

    Request->SetURL(TEXT("https://api.openai.com/v1/chat/completions"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    Request->SetHeader(
        TEXT("Authorization"),
        TEXT("Bearer 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")
    );

    FString EscapedPrompt = PlayerText.ReplaceCharWithEscapedChar();

    FString Body = FString::Printf(TEXT(R"(
{
    "model": "gpt-4o-mini",
    "messages": [
        {
            "role": "system",
            "content": "You are the Expert NPC trapped in a reversed dimension called The Riff. The player must defeat four directional bosses before facing Jigen, the Dimensional Overseer. East: Homura (Dormitory Specter) drops Dimensional Control Cube. West: Dokuma (Venomous Caterer) drops Sealed Magic Plate. North: Kurona (Whispering Librarian) drops Soul Reflection Fragment. South: Kagura (Phantom Sprinter) drops Dimensional Speed Core. Speak briefly in 1-2 short sentences. Stay serious and grounded. Avoid poetic language. Do not provide detailed explanations or programming help. Answer according to this world's lore."
        },
        {
            "role": "user",
            "content": "%s"
        }
    ],
    "temperature": 0.3,
    "max_tokens": 50
}
)"), *EscapedPrompt);

    Request->SetContentAsString(Body);

    Request->OnProcessRequestComplete().BindLambda(
        [Action](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess)
        {
            if (!bSuccess || !Res.IsValid())
            {
                Action->OutResponse = TEXT("[HTTP Request Failed]");
                Action->bFinished = true;
                return;
            }

            int32 Code = Res->GetResponseCode();
            FString Raw = Res->GetContentAsString();

            if (Code != 200)
            {
                Action->OutResponse = FString::Printf(
                    TEXT("[HTTP %d] %s"),
                    Code,
                    *Raw
                );
                Action->bFinished = true;
                return;
            }

            TSharedPtr<FJsonObject> Json;
            TSharedRef<TJsonReader<>> Reader =
                TJsonReaderFactory<>::Create(Raw);

            if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
            {
                Action->OutResponse = TEXT("[JSON Parse Error]");
                Action->bFinished = true;
                return;
            }

            const TArray<TSharedPtr<FJsonValue>>* Choices;

            if (Json->TryGetArrayField(TEXT("choices"), Choices) &&
                Choices->Num() > 0)
            {
                TSharedPtr<FJsonObject> MessageObject =
                    (*Choices)[0]->AsObject()->GetObjectField(TEXT("message"));

                FString Text = MessageObject->GetStringField(TEXT("content"));

                int32 SentenceCount = 0;
                FString Result;

                for (int32 i = 0; i < Text.Len(); i++)
                {
                    Result.AppendChar(Text[i]);

                    if (Text[i] == '.' || Text[i] == '!' || Text[i] == '?')
                    {
                        SentenceCount++;
                        if (SentenceCount >= 2)
                            break;
                    }
                }

                Action->OutResponse = Result.TrimStartAndEnd();
            }
            else
            {
                Action->OutResponse = TEXT("[No Choices]");
            }

            Action->bFinished = true;
        }
    );

    Request->ProcessRequest();
}
