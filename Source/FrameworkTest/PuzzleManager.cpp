#include "PuzzleManager.h"
#include "PuzzleTrigger.h"
#include "PlatformBase.h"

#include "Components/SceneComponent.h"

// Constructor for APuzzleManager class
// Initializes puzzle manager with scene component root
APuzzleManager::APuzzleManager()
{
    PrimaryActorTick.bCanEverTick = false;

    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;
}

// Called when a puzzle trigger is activated
// Checks if all triggers are active to solve the puzzle
void APuzzleManager::TriggerActivated(APuzzleTrigger* CallingTrigger)
{
    if (!RegisteredTriggers.Contains(CallingTrigger)) return;
    if (ActiveTriggers.Contains(CallingTrigger)) return;

    ActiveTriggers.Add(CallingTrigger);

    if (ActiveTriggers.Num() >= RegisteredTriggers.Num())
    {
        bIsSolved = true;
        for (APlatformBase* Platform : Platforms)
            if (IsValid(Platform)) Platform->Activate();
        OnPuzzleSolved();
    }
}

// Called when a puzzle trigger is deactivated
// Checks if puzzle should be reset when triggers are deactivated
void APuzzleManager::TriggerDeactivated(APuzzleTrigger* CallingTrigger)
{
    if (!RegisteredTriggers.Contains(CallingTrigger)) return;
    if (!ActiveTriggers.Contains(CallingTrigger)) return;

    ActiveTriggers.Remove(CallingTrigger);

    if (ActiveTriggers.Num() < RegisteredTriggers.Num())
    {
        bIsSolved = false;
        for (APlatformBase* Platform : Platforms)
            if (IsValid(Platform)) Platform->Deactivate();
        OnPuzzleReset();
    }
}
