#include "QuestManager1.h"

AQuestManager1::AQuestManager1()
{
    PrimaryActorTick.bCanEverTick = false;

    // เควสบอสย่อย 4 ตัว
    for (int i = 0; i < 4; i++)
    {
        FQuestData Quest;
        Quest.QuestName = FString::Printf(TEXT("กำจัดบอสย่อยตัวที่ %d"), i + 1);
        Quest.bUnlocked = true;
        QuestList.Add(Quest);
    }

    // เควสบอสหลัก (ล็อคตอนแรก)
    FQuestData MainQuest;
    MainQuest.QuestName = TEXT("กำจัดบอสหลัก");
    MainQuest.bUnlocked = false;
    QuestList.Add(MainQuest);
}

void AQuestManager1::OnSubBossKilled(int32 QuestIndex)
{
    if (!QuestList.IsValidIndex(QuestIndex)) return;

    if (!QuestList[QuestIndex].bCompleted)
    {
        QuestList[QuestIndex].bCompleted = true;
        SubBossKilled++;
    }

    // ครบ 4 บอส → ปลดล็อคบอสหลัก
    if (SubBossKilled >= 4)
    {
        QuestList[4].bUnlocked = true;
    }
}
