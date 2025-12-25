#include "QuestBoss.h"
#include "QuestManager1.h"          // ✅ include ที่นี่ได้
#include "Kismet/GameplayStatics.h"

AQuestBoss::AQuestBoss()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AQuestBoss::BossDied()
{
    AQuestManager1* QuestManager =
        Cast<AQuestManager1>(
            UGameplayStatics::GetActorOfClass(
                GetWorld(),
                AQuestManager1::StaticClass()
            )
        );

    if (QuestManager)
    {
        QuestManager->OnSubBossKilled(QuestIndex);
    }

    Destroy();
}
