#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PuzzleManager.generated.h"

class APuzzleTrigger;
class APlatformBase;

UCLASS(Blueprintable, meta=(PrioritizeCategories="Puzzle"))
class FRAMEWORKTEST_API APuzzleManager : public AActor
{
    GENERATED_BODY()

public:
    APuzzleManager();

    // All triggers that must be active simultaneously to solve the puzzle
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle")
    TArray<APuzzleTrigger*> RegisteredTriggers;

    // Platforms activated when the puzzle is solved, deactivated when it resets
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle")
    TArray<APlatformBase*> Platforms;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Puzzle|Debug")
    TArray<APuzzleTrigger*> ActiveTriggers;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Puzzle|Debug")
    bool bIsSolved = false;

    UFUNCTION(BlueprintCallable, Category = "Puzzle")
    void TriggerActivated(APuzzleTrigger* CallingTrigger);

    UFUNCTION(BlueprintCallable, Category = "Puzzle")
    void TriggerDeactivated(APuzzleTrigger* CallingTrigger);

    // Blueprint hooks for VFX, sound, etc.
    UFUNCTION(BlueprintImplementableEvent, Category = "Puzzle")
    void OnPuzzleSolved();

    UFUNCTION(BlueprintImplementableEvent, Category = "Puzzle")
    void OnPuzzleReset();
};
