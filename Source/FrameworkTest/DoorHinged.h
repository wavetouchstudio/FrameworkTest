#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DoorHinged.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UPhysicsConstraintComponent;

UENUM(BlueprintType)
enum class EHingeAxis : uint8
{
    Yaw   UMETA(DisplayName = "Yaw (side swing)"),
    Pitch UMETA(DisplayName = "Pitch (up / trapdoor swing)")
};

UCLASS(Blueprintable, meta=(PrioritizeCategories="Door"))
class FRAMEWORKTEST_API ADoorHinged : public AActor
{
    GENERATED_BODY()

public:
    ADoorHinged();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* HingePivot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* FrameMesh;

    // Rotates around HingePivot origin — attach DoorMesh here, position it freely
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* HingeArm;

    // Child of HingeArm — position so the hinge edge is at HingePivot origin
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* DoorMesh;

    // Used when bFreeSwing — constrains DoorMesh to the pivot while allowing rotation
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UPhysicsConstraintComponent* HingeConstraint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
    EHingeAxis HingeAxis = EHingeAxis::Yaw;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
    float OpenAngle = 90.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
    float OpenSpeed = 3.f;

    // Physics-driven hinge — player/objects push it, no interact needed
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
    bool bFreeSwing = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (EditCondition = "bFreeSwing"))
    bool bLimitSwingAngle = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (EditCondition = "bFreeSwing && bLimitSwingAngle", ClampMin = "0"))
    float MaxSwingAngle = 90.f;

    // Auto-picks swing direction away from the player on open
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
    bool bRotateAwayFromPlayer = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
    bool bStartLocked = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
    bool bStartOpen = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Door|State")
    bool bIsOpen = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Door|State")
    bool bIsLocked = false;

    UFUNCTION(BlueprintCallable, Category = "Door")
    void ToggleDoor();

    UFUNCTION(BlueprintCallable, Category = "Door")
    void OpenDoor();

    UFUNCTION(BlueprintCallable, Category = "Door")
    void CloseDoor();

    UFUNCTION(BlueprintCallable, Category = "Door")
    void LockDoor();

    UFUNCTION(BlueprintCallable, Category = "Door")
    void UnlockDoor();

    UFUNCTION(BlueprintImplementableEvent, Category = "Door")
    void OnOpened();

    UFUNCTION(BlueprintImplementableEvent, Category = "Door")
    void OnClosed();

    UFUNCTION(BlueprintImplementableEvent, Category = "Door")
    void OnLocked();

    UFUNCTION(BlueprintImplementableEvent, Category = "Door")
    void OnUnlocked();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    float TargetAngle = 0.f;
    float CurrentAngle = 0.f;
    FRotator ClosedRot;

    void ApplyMeshRotation();
    void SetupFreeSwing();
};
