#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PuzzleTrigger.generated.h"

class UBoxComponent;
class APuzzleManager;
class APickupObject;

UCLASS(Blueprintable, meta=(PrioritizeCategories="Trigger"))
class FRAMEWORKTEST_API APuzzleTrigger : public AActor
{
    GENERATED_BODY()

public:
    APuzzleTrigger();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* TriggerBox;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger")
    APuzzleManager* PuzzleManager;

    // When true: box collision is disabled and this trigger is activated by a Lever instead of overlap.
    // Set LinkedTrigger on the Lever to point at this actor.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger")
    bool bUseLeverInstead = false;

    // Optional — any ADoorHinged or ADoorSliding actor. Opens when triggered, closes when released.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger")
    AActor* LinkedDoor;

    // Called by ALever when bUseLeverInstead is true
    UFUNCTION(BlueprintCallable, Category = "Trigger")
    void ActivateByLever();

    UFUNCTION(BlueprintCallable, Category = "Trigger")
    void DeactivateByLever();

    // Optional: specific actor that also activates this trigger. If null, player only.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger")
    AActor* AcceptedActor;

    // If true, a carried block dropped on this trigger while the player is active will
    // replace the player as the triggering actor — letting the player walk away freely.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger")
    bool bAcceptsPickupSwap = true;

    // Called by APickupObject when it is dropped; swaps it in if the player is currently active.
    UFUNCTION(BlueprintCallable, Category = "Trigger")
    void NotifyPickupDropped(APickupObject* Pickup);

    // Called by APickupObject when it is picked up; swaps back to the player if they're on the
    // trigger, otherwise deactivates.
    UFUNCTION(BlueprintCallable, Category = "Trigger")
    void NotifyPickupLifted(APickupObject* Pickup);

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Trigger|Debug")
    bool bIsActive = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Trigger|Debug")
    AActor* ActiveActor = nullptr;

    UFUNCTION(BlueprintImplementableEvent, Category = "Trigger")
    void OnTriggerActivated();

    UFUNCTION(BlueprintImplementableEvent, Category = "Trigger")
    void OnTriggerDeactivated();

protected:
    virtual void BeginPlay() override;

private:
    UFUNCTION()
    void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnBoxEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    bool IsAcceptedActor(AActor* Actor) const;
};
