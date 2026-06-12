#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FrameworkCharacter.generated.h"

UCLASS(Blueprintable)
class FRAMEWORKTEST_API AFrameworkCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AFrameworkCharacter();

    // --- Movement Tuning (applied to CharacterMovement in constructor) ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning")
    float MaxWalkSpeed = 550.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning")
    float JumpZVelocity = 1400.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning")
    float AirControl = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning")
    float GravityScale = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning")
    float MaxAcceleration = 1500.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning")
    float BrakingDecelerationWalking = 2000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning")
    float RotationYawRate = 750.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning")
    bool bOrientRotationToMovement = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning")
    bool bUseControllerRotationYawSetting = false;

    // --- Double Jump ---
    // Min seconds after the first jump before a double jump can trigger
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Double Jump", meta = (ClampMin = "0.0"))
    float DoubleJumpDelay = 0.25f;

    // Call from Blueprint's IA_Jump "Started" event (replaces the old Jump()/LaunchCharacter graph)
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void RequestJump();

    // Grace period after walking off a ledge where a jump still counts as a ground jump
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Double Jump", meta = (ClampMin = "0.0"))
    float CoyoteTime = 0.15f;

    // --- Wall Slide ---
    // Trace channel used to detect walls
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Wall Slide")
    TEnumAsByte<ECollisionChannel> WallTraceChannel = ECC_Visibility;

    // Forward distance to check for a wall
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Wall Slide", meta = (ClampMin = "0.0"))
    float WallTraceDistance = 60.f;

    // Minimum downward speed before wall sliding can engage
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Wall Slide", meta = (ClampMin = "0.0"))
    float WallSlideMinFallSpeed = 200.f;

    // Downward speed clamp while wall sliding
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Wall Slide", meta = (ClampMin = "0.0"))
    float WallSlideSpeed = 150.f;

    // Minimum dot(forward, -wallNormal) to count as "facing" the wall
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Wall Slide", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WallApproachDotThreshold = 0.3f;

    // --- Wall Jump ---
    // Total launch speed (magnitude) on a wall jump
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Wall Jump", meta = (ClampMin = "0.0"))
    float WallJumpSpeed = 1300.f;

    // Launch angle measured from horizontal (0 = straight along wall normal, 90 = straight up)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Wall Jump", meta = (ClampMin = "0.0", ClampMax = "90.0"))
    float WallJumpAngle = 65.f;

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

protected:
    virtual void Tick(float DeltaTime) override;
    virtual void Landed(const FHitResult& Hit) override;
    virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;

private:
    bool bDoubleJumpUsed = false;
    bool bJumpedThisAirtime = false;
    float StoredJumpZVelocity = 0.f;
    float FirstJumpTime = -1000.f;
    float LeftGroundTime = -1000.f;

    // --- Wall Slide state ---
    bool bIsWallSliding = false;
    FVector WallSlideNormal = FVector::ZeroVector;

    void TraceForWall(FHitResult& OutHit) const;
    void UpdateWallSlide(float DeltaTime);
    void EndWallSlide();

    // --- Ledge Hang state ---
    bool bIsLedgeHanging = false;
    FVector LedgeWallNormal = FVector::ZeroVector;
    FVector LedgeClimbTargetLocation = FVector::ZeroVector;

    bool TraceForLedge(FHitResult& OutWallHit, FVector& OutLedgeSurfacePoint) const;
    void UpdateLedgeHang(float DeltaTime);
    void EnterLedgeHang(const FHitResult& WallHit, const FVector& SurfacePoint);
    void ExitLedgeHang();
};
