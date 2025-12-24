#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AIHttpHelper.generated.h"

UCLASS()
class THE_RIFF_API UAIHttpHelper : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    UFUNCTION(
        BlueprintCallable,
        Category = "AI",
        meta = (
            WorldContext = "WorldContextObject",
            Latent,
            LatentInfo = "LatentInfo"
        )
    )
    static void SendChatToOllama(
        UObject* WorldContextObject,
        FLatentActionInfo LatentInfo,
        const FString& PlayerText,
        FString& OutResponse
    );
};
