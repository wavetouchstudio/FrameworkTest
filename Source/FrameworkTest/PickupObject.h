#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupObject.generated.h"

class UStaticMeshComponent;
class UCurveFloat;
class UNiagaraComponent;
class USpringArmComponent;
class USceneComponent;
class USphereComponent;

UENUM(BlueprintType)
enum class EPickupState : uint8
{
    Idle            UMETA(DisplayName = "Idle"),
    LerpingToHold   UMETA(DisplayName = "Lerping To Hold"),
    Held            UMETA(DisplayName = "Held"),
    Placing         UMETA(DisplayName = "Placing"),
    LerpingToDrop   UMETA(DisplayName = "Lerping To Drop")
};

UCLASS(Blueprintable, meta=(PrioritizeCategories="Pickup Carry Stacking"))
class FRAMEWORKTEST_API APickupObject : public AActor
{
    GENERATED_BODY()

public:
    APickupObject();
//
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* Mesh;

    // Optional identifier checked by ABlockSocket. Leave None to work with any socket.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
    FName BlockID;

    // Place this at the top of the block in Blueprint — incoming blocks snap here
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* TopSocket;

    // Sphere that detects dropped blocks for magnetic stacking. Resize in Blueprint.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* StackDetection;

    // Impulse magnitude (N) that knocks the stacked block off on hit
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stacking")
    float DislodgeImpulse = 500.f;

    // Lerp speed for the magnetic snap onto this block's TopSocket
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stacking", meta = (ClampMin = "0.1"))
    float StackSnapSpeed = 8.f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stacking|State")
    APickupObject* StackedBlock = nullptr;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Stacking|State")
    bool bIsStacked = false;

    // Knock the stacked block off programmatically
    UFUNCTION(BlueprintCallable, Category = "Stacking")
    void DislodgeStack();

    UFUNCTION(BlueprintImplementableEvent, Category = "Stacking")
    void OnBlockStacked();

    UFUNCTION(BlueprintImplementableEvent, Category = "Stacking")
    void OnBlockUnstacked();

    // Flat ring/disc mesh shown in placement mode. Assign in Blueprint; hidden by default.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* PlacementIndicator;

    // Niagara beam drawn between player and object while carried. Assign a Beam system in Blueprint.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UNiagaraComponent* BeamEffect;

    // Starting distance from camera along aim direction
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry")
    float CarryForwardDistance = 150.f;

    // Minimum scroll-in distance
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry")
    float MinCarryDistance = 60.f;

    // Maximum scroll-out distance
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry")
    float MaxCarryDistance = 500.f;

    // Height above player root while held (fallback only, not used when PlayerController is valid)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry")
    float CarryHeightOffset = 80.f;

    // Yaw degrees per second while held. 0 = no spin.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry")
    float CarryRotationSpeed = 0.f;

    // Vertical bob amplitude in cm
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry")
    float FloatAmplitude = 8.f;

    // Bob cycles per second
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry")
    float FloatSpeed = 2.f;

    // Seconds to lerp from ground to hold position
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry", meta = (ClampMin = "0.05"))
    float PickupLerpDuration = 0.35f;

    // Optional ease curve for pickup lerp — maps [0,1] to [0,1]. Falls back to SmoothStep.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry")
    UCurveFloat* PickupCurve;

    // Height above player root where physics re-enables on drop
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry")
    float DropReleaseHeight = 30.f;

    // Seconds to lerp from hold position down to drop release point
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry", meta = (ClampMin = "0.05"))
    float DropLerpDuration = 0.25f;

    // Optional ease curve for drop lerp — maps [0,1] to [0,1]. Falls back to SmoothStep.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry")
    UCurveFloat* DropCurve;

    // Minimum seconds between ToggleCarry calls — prevents rapid fire
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry", meta = (ClampMin = "0.0"))
    float ToggleCooldown = 0.4f;

    // Max horizontal distance from player the object can be aimed in placement mode
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry|Placement")
    float PlacementRadius = 300.f;

    // Yaw degrees per second when Q/E held in placement mode
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry|Placement")
    float PlacementRotationSpeed = 120.f;

    // If true, cancelling placement (X) drops the block at its current spot instead of pulling it back.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry|Placement")
    bool bKeepBlockAtPlacementLocationOnCancel = false;

    // cm/s the block rises or falls when Space/C is held in placement mode
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry|Placement")
    float PlacementHeightSpeed = 100.f;

    // Spring arm length in placement mode (camera pulls back further)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry|Placement|Camera")
    float PlacementCameraArmLength = 800.f;

    // Socket offset applied to the spring arm in placement mode (raises camera)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry|Placement|Camera")
    FVector PlacementCameraSocketOffset = FVector(0.f, 0.f, 300.f);

    // Controller pitch snapped to when entering placement mode (negative = look down)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry|Placement|Camera")
    float PlacementCameraPitch = -60.f;

    // Lerp speed for spring arm transitions (higher = snappier)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry|Placement|Camera")
    float CameraLerpSpeed = 4.f;

    // Niagara vector parameter name for the beam start point
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry|Beam")
    FName BeamStartParamName = TEXT("BeamStart");

    // Niagara vector parameter name for the beam end point
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry|Beam")
    FName BeamEndParamName = TEXT("BeamEnd");

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Carry")
    bool bIsCarried = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Carry")
    EPickupState PickupState = EPickupState::Idle;

    // Call this from Event Interact in your Blueprint child
    UFUNCTION(BlueprintCallable, Category = "Pickup")
    void ToggleCarry();

    // Feed +scroll (push away) or -scroll (pull toward) to move the object while held
    UFUNCTION(BlueprintCallable, Category = "Pickup")
    void AdjustCarryDistance(float Delta);

    // While Held: enters placement mode (object follows camera crosshair)
    UFUNCTION(BlueprintCallable, Category = "Pickup|Placement")
    void StartPlacement();

    // While Placing: drops the object at the aimed position immediately
    UFUNCTION(BlueprintCallable, Category = "Pickup|Placement")
    void ConfirmPlacement();

    // While Placing: cancels back to Held without dropping
    UFUNCTION(BlueprintCallable, Category = "Pickup|Placement")
    void CancelPlacement();

    // While Placing: fires the held block as a projectile in the camera aim direction
    UFUNCTION(BlueprintCallable, Category = "Pickup|Placement")
    void ThrowBlock();

    // Projectile speed in cm/s
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry|Throw")
    float ThrowSpeed = 3000.f;

    // Upward velocity boost applied to the throw for an arc
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry|Throw")
    float ThrowArcZ = 200.f;

    // Sphere radius around the impact point — stacked blocks within range are dislodged
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Carry|Throw")
    float ThrowImpactRadius = 100.f;

    // Feed -1 (Q held), 0 (neither), or 1 (E held) to rotate object in placement mode
    UFUNCTION(BlueprintCallable, Category = "Pickup|Placement")
    void SetPlacementRotationInput(float Input);

    // Feed 1 (Space held) to raise or -1 (C held) to lower the block's Z in placement mode
    UFUNCTION(BlueprintCallable, Category = "Pickup|Placement")
    void SetPlacementVerticalInput(float Input);

    // Fires when picked up — use for sounds, VFX, etc.
    UFUNCTION(BlueprintImplementableEvent, Category = "Pickup")
    void OnPickedUp();

    // Fires when fully dropped and physics re-enabled
    UFUNCTION(BlueprintImplementableEvent, Category = "Pickup")
    void OnPlaced();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    ACharacter* Carrier = nullptr;
    USpringArmComponent* CachedSpringArm = nullptr;
    float DefaultArmLength = 0.f;
    FVector DefaultSocketOffset = FVector::ZeroVector;
    float DefaultControllerPitch = 0.f;
    float FloatTime = 0.f;
    float LastToggleTime = -10.f;
    float LerpAlpha = 0.f;
    float PlacementRotationInput = 0.f;
    float PlacementVerticalInput = 0.f;
    float PlacementHeightAdjust = 0.f;
    int32 DefaultJumpMaxCount = 1;
    float CurrentCarryDistance = 0.f;
    FVector LerpStartPosition = FVector::ZeroVector;
    bool bThrowing = false;

    void PickUp(ACharacter* InCarrier);
    void BeginDrop();
    void FinalizeDrop();

    FVector GetHoldPosition() const;
    FVector GetDropPosition() const;

    void InitiateStack(APickupObject* Block);

    APickupObject* PendingStack = nullptr;
    bool bSnappingStack = false;

    virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp,
        bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse,
        const FHitResult& Hit) override;

    UFUNCTION()
    void OnStackSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnStackSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
