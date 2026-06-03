#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DoorDestructible.generated.h"

class UStaticMeshComponent;
class USceneComponent;

UCLASS(Blueprintable, meta=(PrioritizeCategories="Door"))
class FRAMEWORKTEST_API ADoorDestructible : public AActor
{
    GENERATED_BODY()

public:
    ADoorDestructible();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* FrameMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* DoorMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Destructible")
    float MaxHealth = 100.f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Door|Destructible")
    float CurrentHealth;

    // Blueprint actor to spawn as debris — use a cube with Simulate Physics enabled on its root mesh
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Destructible")
    TSubclassOf<AActor> DebrisClass;

    // How many debris pieces to spawn
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Destructible", meta = (ClampMin = "0"))
    int32 DebrisCount = 5;

    // Seconds before each debris piece auto-destroys
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Destructible", meta = (ClampMin = "0"))
    float DebrisLifetime = 10.f;

    // Launch strength in cm/s — mass-independent. ~300 = gentle roll, ~700 = dramatic burst
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Destructible")
    float DebrisImpulseStrength = 500.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
    bool bStartLocked = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Door|State")
    bool bIsLocked = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Door|State")
    bool bIsDestroyed = false;

    // Damage dealt per interact press. Set to MaxHealth for one-shot, lower for a multi-hit door.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Destructible")
    float InteractDamage = 34.f;

    // Reduce health by Amount regardless of lock state — locked doors can still be destroyed
    UFUNCTION(BlueprintCallable, Category = "Door")
    void ApplyDamage(float Amount);

    // Convenience — deals InteractDamage. Wire this to the BPI_Interactable Interact event.
    UFUNCTION(BlueprintCallable, Category = "Door")
    void InteractHit();

    UFUNCTION(BlueprintCallable, Category = "Door")
    void LockDoor();

    UFUNCTION(BlueprintCallable, Category = "Door")
    void UnlockDoor();

    UFUNCTION(BlueprintImplementableEvent, Category = "Door")
    void OnDamaged(float RemainingHealth);

    UFUNCTION(BlueprintImplementableEvent, Category = "Door")
    void OnDestroyed();

    UFUNCTION(BlueprintImplementableEvent, Category = "Door")
    void OnLocked();

    UFUNCTION(BlueprintImplementableEvent, Category = "Door")
    void OnUnlocked();

protected:
    virtual void BeginPlay() override;

private:
    void SpawnDebris();
};
