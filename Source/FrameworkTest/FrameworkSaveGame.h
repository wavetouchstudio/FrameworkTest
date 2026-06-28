#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "FrameworkSaveGame.generated.h"

/**
 * Save game object used by the FrameworkTest module.
 * Extends USaveGame to store minimal world-state needed for the Bonfire system.
 */
UCLASS()
class FRAMEWORKTEST_API UFrameworkSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    /** Player's world location at the time of saving */
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FrameworkSaveGame")
    FVector PlayerLocation;

    /** Player's world rotation at the time of saving */
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FrameworkSaveGame")
    FRotator PlayerRotation;

    /** Current health of the player */
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FrameworkSaveGame")
    float CurrentHealth;

    /** Index of the currently-active profile (used for multi-profile switching) */
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FrameworkSaveGame")
    int32 CurrentProfileIndex;

    /** Identifier of the last Bonfire used (name or FName) */
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "FrameworkSaveGame")
    FName LastBonfireID;
};