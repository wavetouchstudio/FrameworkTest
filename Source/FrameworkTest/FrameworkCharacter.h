#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Blueprint/UserWidget.h"
#include "FrameworkCharacter.generated.h"

class UNiagaraComponent;
class UCableComponent;
class AGrappleAnchor;
class UDecalComponent;

USTRUCT(BlueprintType)
struct FCharacterMovementProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CapsuleScale = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Mass = 80.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float WalkSpeed = 550.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SprintSpeed = 850.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float JumpZVelocity = 1400.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DoubleJumpZVelocity = 1400.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bEnableWallJump = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bEnableWallSlide = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bWallJumpResetsDoubleJump = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bEnableDoubleJump = true;

    // --- Movement Feel ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AirControl = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float GravityScale = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxAcceleration = 1500.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BrakingDecelerationWalking = 2000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RotationYawRate = 750.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bOrientRotationToMovement = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bUseControllerRotationYawSetting = false;

    // --- Wall Slide ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float WallSlideMinFallSpeed = 200.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float WallSlideSpeed = 150.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WallApproachDotThreshold = 0.3f;

    // Max seconds a wall slide can be held before falling off; <= 0 means unlimited
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
    float WallSlideMaxDuration = 0.f;

    // --- Wall Jump ---
    // Total launch speed (magnitude) on a wall jump
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
    float WallJumpSpeed = 1300.f;

    // Launch angle measured from horizontal (0 = straight along wall normal, 90 = straight up)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "90.0"))
    float WallJumpAngle = 65.f;

    // --- Interaction Toggles ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bEnableCarry = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bEnablePlacementMode = true;

    // --- Glide ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bEnableGlide = true;

    // Max downward fall speed while gliding (cm/s)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
    float GlideMaxFallSpeed = 75.f;

    // How quickly Velocity.Z eases toward -GlideMaxFallSpeed (higher = snappier)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1"))
    float GlideZInterpSpeed = 8.f;

    // Optional forward boost while gliding (cm/s added along forward vector). Rank up via skills for more.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
    float GlideForwardSpeed = 800.f;

    // Max continuous glide duration in seconds before it forces an end (placeholder for stamina)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
    float GlideMaxDuration = 3.f;

    // Seconds-per-second the glide duration recharges while not gliding (placeholder for stamina regen)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
    float GlideRechargeRate = 1.f;

    // Seconds the jump input must be held before glide actually engages
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
    float GlideHoldThreshold = 0.25f;

    // One-time impulse magnitude added to velocity the instant glide engages (the "wind catches you" kick)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
    float GlideLaunchSpeed = 300.f;

    // Launch angle measured from horizontal (0 = straight forward, 90 = straight up). Keep positive for lift, not a dive.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "90.0"))
    float GlideLaunchAngle = 35.f;

    // --- Wall Trace ---
    // Trace channel used to detect walls
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TEnumAsByte<ECollisionChannel> WallTraceChannel = ECC_Visibility;

    // Forward distance to check for a wall
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
    float WallTraceDistance = 60.f;

    // --- Camera ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
    float CameraArmLength = 300.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector CameraSocketOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "5.0", ClampMax = "170.0"))
    float CameraFOV = 90.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bEnableCameraLag = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
    float CameraLagSpeed = 10.f;
};

UCLASS(Blueprintable)
class FRAMEWORKTEST_API AFrameworkCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AFrameworkCharacter();

    // --- Double Jump ---
    // Min seconds after the first jump before a double jump can trigger
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Double Jump", meta = (ClampMin = "0.0"))
    float DoubleJumpDelay = 0.25f;

    // Call from Blueprint's IA_Jump "Started" event (replaces the old Jump()/LaunchCharacter graph)
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void RequestJump();

    // Call from Blueprint's IA_Sprint "Started"/"Completed" events (true on Started, false on Completed)
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void SetSprinting(bool bNewSprinting);

    // Grace period after walking off a ledge where a jump still counts as a ground jump
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Double Jump", meta = (ClampMin = "0.0"))
    float CoyoteTime = 0.15f;

    // --- Ledge Hang ---
    // Height above the capsule's feet where the hand/wall trace is performed
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Ledge Hang", meta = (ClampMin = "0.0"))
    float LedgeGrabHeight = 120.f;

    // Height above the hand trace used for the clearance check (must be clear of obstacles)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Ledge Hang", meta = (ClampMin = "0.0"))
    float LedgeClearanceHeight = 40.f;

    // How far above the ledge surface the downward trace starts
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Ledge Hang", meta = (ClampMin = "0.0"))
    float LedgeSurfaceSearchHeight = 60.f;

    // Forward distance for the ledge wall trace
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Ledge Hang", meta = (ClampMin = "0.0"))
    float LedgeWallTraceDistance = 60.f;

    // Forward reach beyond the wall hit used to find the ledge surface
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Ledge Hang", meta = (ClampMin = "0.0"))
    float LedgeForwardReach = 40.f;

    // Maximum surface angle (degrees from up) considered walkable for a ledge top
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Ledge Hang", meta = (ClampMin = "0.0", ClampMax = "90.0"))
    float LedgeMaxWalkableAngle = 45.f;

    // Vertical offset from the ledge surface to the character's location while hanging
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Ledge Hang")
    float LedgeHangVerticalOffset = -80.f;

    // Horizontal speed when jumping away from a ledge (back-out jump)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Ledge Hang", meta = (ClampMin = "0.0"))
    float LedgeJumpAwaySpeed = 400.f;

    // Vertical speed when jumping away from a ledge
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Ledge Hang", meta = (ClampMin = "0.0"))
    float LedgeJumpZVelocity = 800.f;

    // Forward offset applied when climbing up onto a ledge
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Ledge Hang")
    float LedgeClimbForwardOffset = 60.f;

    // Upward offset applied when climbing up onto a ledge
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Ledge Hang")
    float LedgeClimbUpOffset = 90.f;

    // Draw debug lines for ledge traces
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Ledge Hang")
    bool bDebugDrawLedgeTraces = false;

    // --- Glide ---
    // Call from Blueprint's jump input action "Ongoing"/"Triggered" (true) and
    // "Completed"/"Canceled" (false) events. Glide only engages while falling.
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void SetGliding(bool bNewGliding);

    // Remaining glide time (placeholder for a future stamina gauge)
    UPROPERTY(BlueprintReadOnly, Category = "Movement Tuning|Glide")
    float GlideTimeRemaining = 0.f;

    // Niagara trail spawned/activated while gliding. Assign in Blueprint.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UNiagaraComponent* GlideTrailEffect;

    // --- Grapple ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UCableComponent* GrappleCable;

    // Niagara beam effect rendered along the cable while grappling. Assign in Blueprint.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UNiagaraComponent* GrappleBeamEffect;

    // Niagara vector parameter name for the beam start point (character end)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Grapple")
    FName GrappleBeamStartParamName = TEXT("BeamStart");

    // Niagara vector parameter name for the beam end point (anchor end)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Grapple")
    FName GrappleBeamEndParamName = TEXT("BeamEnd");

    // Radius around the character within which AGrappleAnchor actors can be targeted
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Grapple", meta = (ClampMin = "0.0"))
    float GrappleTargetingRadius = 2000.f;

    // How closely the camera must point at an anchor to highlight it (dot product threshold)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Grapple", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float GrappleTargetingDotThreshold = 0.95f;

    // Speed the character is pulled toward the anchor (cm/s)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Grapple", meta = (ClampMin = "0.0"))
    float GrapplePullSpeed = 2000.f;

    // Distance from anchor at which the grapple auto-releases
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Grapple", meta = (ClampMin = "0.0"))
    float GrappleReleaseDistance = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Grapple")
    bool bDebugDrawGrappleTargeting = false;

    // Call from Blueprint's IA_Grapple "Started" event
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void RequestGrapple();

    // Blueprint hook - fired when RequestGrapple() is pressed with no anchor currently highlighted
    UFUNCTION(BlueprintImplementableEvent, Category = "Movement")
    void OnGrappleMiss();

    // --- Mechanic State Debug Display ---
    // Show an on-screen text indicator for the player's current mechanic/state
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowMechanicDebugText = true;

    // Set by Blueprint when the player picks up/holds a carryable object
    UPROPERTY(BlueprintReadWrite, Category = "Debug")
    bool bIsHoldingObject = false;

    // Set by Blueprint when the player enters block-placement mode
    UPROPERTY(BlueprintReadWrite, Category = "Debug")
    bool bIsInPlacementMode = false;

    // --- Test Health Pool ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug|Health")
    float MaxHealth = 100.f;

    UPROPERTY(BlueprintReadOnly, Category = "Debug|Health")
    float CurrentHealth = 100.f;

    // --- Movement Profiles ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement Profiles")
    TArray<FCharacterMovementProfile> MovementProfiles;

    UPROPERTY(BlueprintReadOnly, Category = "Movement Profiles")
    int32 CurrentProfileIndex = 0;

    UFUNCTION(BlueprintCallable, Category = "Movement Profiles")
    void SetMovementProfile(int32 ProfileIndex);

    // --- Fall Damage ---
    // Downward speed (cm/s) below which no fall damage is taken
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall Damage", meta = (ClampMin = "0.0"))
    float FallDamageSafeSpeed = 1400.f;

    // Damage per unit of fall speed above the safe speed
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall Damage", meta = (ClampMin = "0.0"))
    float FallDamageRatePerUnit = 0.05f;

    // Mass at which the fall damage rate is considered "1x"
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fall Damage", meta = (ClampMin = "0.0"))
    float FallDamageReferenceMass = 80.f;

    // Add or update a runtime fall damage multiplier modifier (e.g. from items/skills)
    UFUNCTION(BlueprintCallable, Category = "Fall Damage")
    void AddFallDamageModifier(FName ModifierID, float MultiplierDelta);

    // Remove a runtime fall damage multiplier modifier
    UFUNCTION(BlueprintCallable, Category = "Fall Damage")
    void RemoveFallDamageModifier(FName ModifierID);

    // Current combined fall damage multiplier (1 + sum of active modifiers, clamped >= 0)
    UFUNCTION(BlueprintCallable, Category = "Fall Damage")
    float GetFallDamageMultiplier() const;

    // Called when fall damage should be applied. Override in Blueprint to hook up a health system.
    UFUNCTION(BlueprintNativeEvent, Category = "Fall Damage")
    void ApplyFallDamage(float Damage);
    virtual void ApplyFallDamage_Implementation(float Damage);

    // --- Interact Prompt UI ---
    // Widget class shown while CurrentInteractable/bCanInteract is set (assign in BP)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UUserWidget> InteractPromptWidgetClass;

    UPROPERTY(BlueprintReadOnly, Category = "UI")
    UUserWidget* InteractPromptWidget = nullptr;

    // Overlap search radius for nearby interactables
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact", meta = (ClampMin = "0.0"))
    float InteractSphereRadius = 200.f;

    // Final reachability cutoff after the line trace
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact", meta = (ClampMin = "0.0"))
    float MaxInteractDistance = 150.f;

    // Trace channel used to confirm unobstructed line of sight to a candidate
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact")
    TEnumAsByte<ECollisionChannel> InteractTraceChannel = ECC_Visibility;

    // Assign BPI_Interactable here in the character BP defaults
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact")
    TSubclassOf<UInterface> InteractableInterfaceClass;

    UPROPERTY(BlueprintReadWrite, Category = "Interact")
    bool bCanInteract = false;

    UPROPERTY(BlueprintReadWrite, Category = "Interact")
    AActor* CurrentInteractable = nullptr;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void Landed(const FHitResult& Hit) override;
    virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;

private:
    bool bDoubleJumpUsed = false;
    bool bJumpedThisAirtime = false;
    bool bIsSprinting = false;
    float FirstJumpTime = -1000.f;
    float LeftGroundTime = -1000.f;

    void ApplyWalkSpeed();

    // --- Wall Slide state ---
    bool bIsWallSliding = false;
    FVector WallSlideNormal = FVector::ZeroVector;
    float WallSlideElapsedTime = 0.f;

    // --- Fall Damage state ---
    float PeakFallSpeed = 0.f;
    TMap<FName, float> FallDamageModifiers;

    void TraceForWall(FHitResult& OutHit) const;
    void UpdateWallSlide(float DeltaTime);
    void EndWallSlide();

    void UpdateMechanicDebugDisplay() const;
    void UpdateHealthDebugDisplay() const;
    void UpdateProfileDebugDisplay() const;

    // --- Ledge Hang state ---
    bool bIsLedgeHanging = false;
    FVector LedgeWallNormal = FVector::ZeroVector;
    FVector LedgeClimbTargetLocation = FVector::ZeroVector;

    bool TraceForLedge(FHitResult& OutWallHit, FVector& OutLedgeSurfacePoint) const;
    void UpdateLedgeHang(float DeltaTime);
    void EnterLedgeHang(const FHitResult& WallHit, const FVector& SurfacePoint);
    void ExitLedgeHang();

    // --- Glide state ---
    bool bWantsGlide = false;
    bool bJumpInputHeld = false;
    float JumpHeldStartTime = -1.f;
    bool bIsGliding = false;
    void UpdateGlide(float DeltaTime);
    void EndGlide();

    // --- Grapple state ---
    bool bIsGrappling = false;
    TWeakObjectPtr<AGrappleAnchor> HighlightedAnchor;
    TWeakObjectPtr<AGrappleAnchor> GrappleTargetAnchor;
    float GrappleScanInterval = 0.1f;
    float GrappleScanTimer = 0.f;
    void UpdateGrappleTargeting(float DeltaTime);
    void UpdateGrapple(float DeltaTime);
    void EndGrapple();

    void UpdateInteractDetection();
    void UpdateDropShadowVisibility();
    TArray<UDecalComponent*> CachedDropShadowDecals;
};
