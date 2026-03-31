#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PuzzleAPI.generated.h"

UCLASS()
class THE_RIFF_API UPuzzleAPI : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    UFUNCTION(
        BlueprintCallable,
        Category = "Puzzle",
        meta = (
            WorldContext = "WorldContextObject",
            Latent,
            LatentInfo = "LatentInfo"
            )
    )
    static void GetPuzzle(
        UObject* WorldContextObject,
        FLatentActionInfo LatentInfo,
        FString& OutQuestion,
        FString& OutAnswer
    );
};