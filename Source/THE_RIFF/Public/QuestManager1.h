#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "QuestManager1.generated.h"

/**
 * Struct เก็บข้อมูลเควส 1 เควส
 */
USTRUCT(BlueprintType)
struct FQuestData
{
    GENERATED_BODY()

    // ชื่อเควส
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FString QuestName;

    // เควสสำเร็จหรือยัง
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bCompleted = false;

    // เควสปลดล็อคหรือยัง
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bUnlocked = true;
};

/**
 * Quest Manager
 */
UCLASS()
class THE_RIFF_API AQuestManager1 : public AActor
{
    GENERATED_BODY()

public:
    // Constructor
    AQuestManager1();

    // รายการเควสทั้งหมด
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FQuestData> QuestList;

    // นับจำนวนบอสย่อยที่ฆ่าแล้ว
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 SubBossKilled = 0;

    // เรียกเมื่อบอสย่อยตาย
    UFUNCTION(BlueprintCallable)
    void OnSubBossKilled(int32 QuestIndex);
};
