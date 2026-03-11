#include "AIHttpHelper.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "LatentActions.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

#include "Json.h"
#include "JsonUtilities.h"

#include "Misc/Guid.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"

// =====================================================
// PLAYER ID (สร้างครั้งเดียว ต่อเครื่อง)
// =====================================================

FString GetOrCreatePlayerID()
{
    FString SavePath = FPaths::ProjectSavedDir() + TEXT("player_id.txt");

    FString PlayerID;

    if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*SavePath))
    {
        FFileHelper::LoadFileToString(PlayerID, *SavePath);
        return PlayerID;
    }

    PlayerID = FGuid::NewGuid().ToString();

    FFileHelper::SaveStringToFile(PlayerID, *SavePath);

    return PlayerID;
}

// =====================================================
// LATENT ACTION
// =====================================================

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

// =====================================================
// SEND CHAT
// =====================================================

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

    // =========================
    // SERVER
    // =========================

    Request->SetURL(TEXT("https://api.theriffgame.org/chat"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    FString EscapedPrompt = PlayerText.ReplaceCharWithEscapedChar();

    // =========================
    // PLAYER ID ต่อเครื่อง
    // =========================

    FString PlayerID = GetOrCreatePlayerID();

    FString Body = FString::Printf(TEXT(R"(
{
    "player_id": "%s",
    "message": "%s"
}
)"), *PlayerID, *EscapedPrompt);

    Request->SetContentAsString(Body);

    // =========================
    // RESPONSE
    // =========================

    Request->OnProcessRequestComplete().BindLambda(
        [Action](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess)
        {
            if (!bSuccess || !Res.IsValid())
            {
                Action->OutResponse = TEXT("[Server Request Failed]");
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

            FString Reply;

            if (Json->TryGetStringField(TEXT("reply"), Reply))
            {
                Action->OutResponse = Reply;
            }
            else
            {
                Action->OutResponse = TEXT("[No Reply Field]");
            }

            Action->bFinished = true;
        }
    );

    Request->ProcessRequest();
}