#include "PuzzleAPI.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "LatentActions.h"

void UPuzzleAPI::GetPuzzle(
    UObject* WorldContextObject,
    FLatentActionInfo LatentInfo,
    FString Category,
    FString& OutQuestion,
    FString& OutAnswer
)
{
    if (!WorldContextObject) return;

    UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
    if (!World) return;

    struct FPuzzleLatentAction : public FPendingLatentAction
    {
        bool bDone = false;

        FString& Question;
        FString& Answer;

        FName ExecutionFunction;
        int32 OutputLink;
        FWeakObjectPtr CallbackTarget;

        FPuzzleLatentAction(
            FString& InQ,
            FString& InA,
            const FLatentActionInfo& LatentInfo
        )
            : Question(InQ)
            , Answer(InA)
            , ExecutionFunction(LatentInfo.ExecutionFunction)
            , OutputLink(LatentInfo.Linkage)
            , CallbackTarget(LatentInfo.CallbackTarget)
        {
        }

        virtual void UpdateOperation(FLatentResponse& Response) override
        {
            Response.FinishAndTriggerIf(bDone, ExecutionFunction, OutputLink, CallbackTarget);
        }
    };

    FLatentActionManager& LatentManager = World->GetLatentActionManager();

    if (LatentManager.FindExistingAction<FPuzzleLatentAction>(LatentInfo.CallbackTarget, LatentInfo.UUID))
    {
        return;
    }

    FPuzzleLatentAction* NewAction = new FPuzzleLatentAction(OutQuestion, OutAnswer, LatentInfo);
    LatentManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, NewAction);

    // =========================
    // HTTP
    // =========================

    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();

    Request->SetURL(TEXT("https://api.theriffgame.org/puzzle"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    FString Body = FString::Printf(
        TEXT("{\"category\":\"%s\"}"),
        *Category
    );

    Request->SetContentAsString(Body);

    Request->OnProcessRequestComplete().BindLambda(
        [NewAction](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess)
        {
            if (!bSuccess || !Res.IsValid())
            {
                NewAction->Question = TEXT("Error");
                NewAction->Answer = TEXT("");
                NewAction->bDone = true;
                return;
            }

            FString JsonStr = Res->GetContentAsString();

            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);

            if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
            {
                NewAction->Question = JsonObject->GetStringField("question");
                NewAction->Answer = JsonObject->GetStringField("answer");
            }
            else
            {
                NewAction->Question = TEXT("Parse Error");
                NewAction->Answer = TEXT("");
            }

            NewAction->bDone = true;
        });

    Request->ProcessRequest();
}