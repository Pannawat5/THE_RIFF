#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "QuestBoss.generated.h"

UCLASS()
class THE_RIFF_API AQuestBoss : public AActor
{
    GENERATED_BODY()

public:
    AQuestBoss();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    int32 QuestIndex = 0;

    UFUNCTION(BlueprintCallable)
    void BossDied();
};
