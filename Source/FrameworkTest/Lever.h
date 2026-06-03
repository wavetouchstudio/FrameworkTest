#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Lever.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UAudioComponent;
class ALiftManager;
class APuzzleTrigger;
class ADoorHinged;
class ADoorSliding;

UCLASS(meta=(PrioritizeCategories="Lever"))
class FRAMEWORKTEST_API ALever : public AActor
{
    GENERATED_BODY()

public:
    ALever();

    // Rotation pivot — rotate this to animate the lever arm
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* CalibrationPoint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* LeverMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UAudioComponent* Audio;

    // Optional — leave null to use lever without a lift
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lever")
    ALiftManager* LiftManager;

    // Optional — any ADoorHinged or ADoorSliding actor. Opens on activate, closes on reset.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lever")
    AActor* LinkedDoor;

    // Optional — assign a PuzzleTrigger (with bUseLeverInstead = true) to feed this lever into a PuzzleManager
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lever")
    APuzzleTrigger* LinkedTrigger;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lever")
    bool bStartLocked = false;

    // True = this lever is at the bottom floor and should call the lift DOWN.
    // False = this lever is at the top floor and should call the lift UP.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lever")
    bool bIsBottomLever = true;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Lever|Debug")
    bool bIsLocked = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Lever|Debug")
    bool bHasBeenActivated = false;

    UFUNCTION(BlueprintCallable, Category = "Lever")
    void ActivateLever();

    UFUNCTION(BlueprintCallable, Category = "Lever")
    void ResetLever();

    UFUNCTION(BlueprintCallable, Category = "Lever")
    void LockLever();

    UFUNCTION(BlueprintCallable, Category = "Lever")
    void UnlockLever();

    UFUNCTION(BlueprintImplementableEvent, Category = "Lever")
    void OnActivated();

    UFUNCTION(BlueprintImplementableEvent, Category = "Lever")
    void OnReset();

    UFUNCTION(BlueprintImplementableEvent, Category = "Lever")
    void OnLocked();

    UFUNCTION(BlueprintImplementableEvent, Category = "Lever")
    void OnUnlocked();

protected:
    virtual void BeginPlay() override;
};
