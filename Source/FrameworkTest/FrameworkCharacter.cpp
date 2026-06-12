#include "FrameworkCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"

AFrameworkCharacter::AFrameworkCharacter()
{
    UCharacterMovementComponent* Movement = GetCharacterMovement();

    Movement->MaxWalkSpeed = MaxWalkSpeed;
    Movement->JumpZVelocity = JumpZVelocity;
    Movement->AirControl = AirControl;
    Movement->GravityScale = GravityScale;
    Movement->MaxAcceleration = MaxAcceleration;
    Movement->BrakingDecelerationWalking = BrakingDecelerationWalking;
    Movement->RotationRate = FRotator(0.f, RotationYawRate, 0.f);
    Movement->bOrientRotationToMovement = bOrientRotationToMovement;

    bUseControllerRotationYaw = bUseControllerRotationYawSetting;

    // 2, not 1: engine's CanJumpInternal() blocks Jump() while falling with
    // JumpCurrentCount==0 unless JumpCurrentCount+1 < JumpMaxCount. Needed for
    // coyote-time jumps (falling, but no jump performed yet). Our own
    // bJumpedThisAirtime/bDoubleJumpUsed flags gate everything else.
    JumpMaxCount = 2;
}

void AFrameworkCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    UCharacterMovementComponent* Movement = GetCharacterMovement();

    if (bIsLedgeHanging)
    {
        UpdateLedgeHang(DeltaTime);
        return;
    }

    if (Movement->IsFalling())
    {
        FHitResult LedgeWallHit;
        FVector LedgeSurfacePoint;
        if (TraceForLedge(LedgeWallHit, LedgeSurfacePoint))
        {
            EnterLedgeHang(LedgeWallHit, LedgeSurfacePoint);
            return;
        }

        UpdateWallSlide(DeltaTime);
    }
    else if (bIsWallSliding)
    {
        EndWallSlide();
    }
}

void AFrameworkCharacter::RequestJump()
{
    UCharacterMovementComponent* Movement = GetCharacterMovement();

    if (bIsLedgeHanging)
    {
        const FVector Forward = GetActorForwardVector();
        const FVector ClimbTarget = LedgeClimbTargetLocation + FVector(0.f, 0.f, LedgeClimbUpOffset) + Forward * LedgeClimbForwardOffset;

        UCapsuleComponent* Capsule = GetCapsuleComponent();
        FHitResult ClimbHit;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(LedgeClimb), false, this);
        const bool bBlocked = Capsule != nullptr
            && GetWorld()->SweepSingleByChannel(ClimbHit, GetActorLocation(), ClimbTarget, GetActorQuat(),
                WallTraceChannel, Capsule->GetCollisionShape(), Params);

        ExitLedgeHang();

        if (bBlocked)
        {
            const FVector AwayVelocity = LedgeWallNormal * LedgeJumpAwaySpeed + FVector(0.f, 0.f, LedgeJumpZVelocity);
            LaunchCharacter(AwayVelocity, true, true);
        }
        else
        {
            SetActorLocation(ClimbTarget, true);
        }

        bDoubleJumpUsed = false;
        bJumpedThisAirtime = false;
        return;
    }

    if (bIsWallSliding)
    {
        const float AngleRad = FMath::DegreesToRadians(WallJumpAngle);
        const FVector LaunchVelocity = WallSlideNormal * (WallJumpSpeed * FMath::Cos(AngleRad)) + FVector(0.f, 0.f, WallJumpSpeed * FMath::Sin(AngleRad));
        EndWallSlide();

        StoredJumpZVelocity = Movement->JumpZVelocity;
        bJumpedThisAirtime = true;
        LaunchCharacter(LaunchVelocity, true, true);
        return;
    }

    const bool bGrounded = !Movement->IsFalling();
    const bool bCoyoteWindow = !bGrounded && !bJumpedThisAirtime
        && (GetWorld()->GetTimeSeconds() - LeftGroundTime) <= CoyoteTime;

    if (bGrounded || bCoyoteWindow)
    {
        StoredJumpZVelocity = Movement->JumpZVelocity;
        FirstJumpTime = GetWorld()->GetTimeSeconds();
        bDoubleJumpUsed = false;
        bJumpedThisAirtime = true;
        Jump();
    }
    else if (!bDoubleJumpUsed && (GetWorld()->GetTimeSeconds() - FirstJumpTime) >= DoubleJumpDelay)
    {
        bDoubleJumpUsed = true;
        LaunchCharacter(FVector(0.f, 0.f, StoredJumpZVelocity), false, true);
    }
}

void AFrameworkCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);
    bDoubleJumpUsed = false;
    bJumpedThisAirtime = false;

    if (bIsWallSliding)
    {
        EndWallSlide();
    }

    if (bIsLedgeHanging)
    {
        ExitLedgeHang();
    }
}

void AFrameworkCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
    Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

    if (PrevMovementMode == MOVE_Walking && GetCharacterMovement()->MovementMode == MOVE_Falling && !bJumpedThisAirtime)
    {
        LeftGroundTime = GetWorld()->GetTimeSeconds();
    }

    if (PrevMovementMode == MOVE_Flying && bIsLedgeHanging)
    {
        ExitLedgeHang();
    }
}

void AFrameworkCharacter::TraceForWall(FHitResult& OutHit) const
{
    const FVector Start = GetActorLocation();
    const FVector End = Start + GetActorForwardVector() * WallTraceDistance;

    FCollisionQueryParams Params(SCENE_QUERY_STAT(WallTrace), false, this);
    GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, WallTraceChannel, Params);
}

void AFrameworkCharacter::UpdateWallSlide(float DeltaTime)
{
    UCharacterMovementComponent* Movement = GetCharacterMovement();

    if (Movement->Velocity.Z >= -WallSlideMinFallSpeed)
    {
        if (bIsWallSliding)
        {
            EndWallSlide();
        }
        return;
    }

    FHitResult WallHit;
    TraceForWall(WallHit);

    if (!WallHit.IsValidBlockingHit())
    {
        if (bIsWallSliding)
        {
            EndWallSlide();
        }
        return;
    }

    const float ApproachDot = FVector::DotProduct(GetActorForwardVector(), -WallHit.Normal);
    if (ApproachDot < WallApproachDotThreshold)
    {
        if (bIsWallSliding)
        {
            EndWallSlide();
        }
        return;
    }

    bIsWallSliding = true;
    WallSlideNormal = WallHit.Normal;

    FVector Velocity = Movement->Velocity;
    Velocity.Z = FMath::Max(Velocity.Z, -WallSlideSpeed);
    Movement->Velocity = Velocity;
}

void AFrameworkCharacter::EndWallSlide()
{
    bIsWallSliding = false;
    WallSlideNormal = FVector::ZeroVector;
}

bool AFrameworkCharacter::TraceForLedge(FHitResult& OutWallHit, FVector& OutLedgeSurfacePoint) const
{
    UCapsuleComponent* Capsule = GetCapsuleComponent();
    if (!Capsule)
    {
        return false;
    }

    const FVector ActorLocation = GetActorLocation();
    const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    const float FeetZ = ActorLocation.Z - CapsuleHalfHeight;

    const FVector Forward = GetActorForwardVector();
    const FVector HandOrigin = FVector(ActorLocation.X, ActorLocation.Y, FeetZ + LedgeGrabHeight);

    FCollisionQueryParams Params(SCENE_QUERY_STAT(LedgeTrace), false, this);

    // 1. Hand-height wall trace
    const FVector WallEnd = HandOrigin + Forward * LedgeWallTraceDistance;
    FHitResult WallHit;
    const bool bHitWall = GetWorld()->LineTraceSingleByChannel(WallHit, HandOrigin, WallEnd, WallTraceChannel, Params);

    if (bDebugDrawLedgeTraces)
    {
        DrawDebugLine(GetWorld(), HandOrigin, WallEnd, bHitWall ? FColor::Red : FColor::Green, false, 0.f, 0, 1.5f);
    }

    if (!bHitWall || !WallHit.IsValidBlockingHit())
    {
        return false;
    }

    // 2. Clearance trace above hand height — must be clear
    const FVector ClearanceOrigin = HandOrigin + FVector(0.f, 0.f, LedgeClearanceHeight);
    const FVector ClearanceEnd = ClearanceOrigin + Forward * LedgeWallTraceDistance;
    FHitResult ClearanceHit;
    const bool bClearanceBlocked = GetWorld()->LineTraceSingleByChannel(ClearanceHit, ClearanceOrigin, ClearanceEnd, WallTraceChannel, Params);

    if (bDebugDrawLedgeTraces)
    {
        DrawDebugLine(GetWorld(), ClearanceOrigin, ClearanceEnd, bClearanceBlocked ? FColor::Red : FColor::Green, false, 0.f, 0, 1.5f);
    }

    if (bClearanceBlocked && ClearanceHit.IsValidBlockingHit())
    {
        return false;
    }

    // 3. Downward trace to find the ledge surface
    const FVector ForwardReachPoint = WallHit.ImpactPoint + Forward * LedgeForwardReach;
    const FVector SurfaceTraceStart = FVector(ForwardReachPoint.X, ForwardReachPoint.Y, HandOrigin.Z + LedgeSurfaceSearchHeight);
    const FVector SurfaceTraceEnd = FVector(ForwardReachPoint.X, ForwardReachPoint.Y, HandOrigin.Z - LedgeClearanceHeight);

    FHitResult SurfaceHit;
    const bool bHitSurface = GetWorld()->LineTraceSingleByChannel(SurfaceHit, SurfaceTraceStart, SurfaceTraceEnd, WallTraceChannel, Params);

    if (bDebugDrawLedgeTraces)
    {
        DrawDebugLine(GetWorld(), SurfaceTraceStart, SurfaceTraceEnd, bHitSurface ? FColor::Blue : FColor::Yellow, false, 0.f, 0, 1.5f);
    }

    if (!bHitSurface || !SurfaceHit.IsValidBlockingHit())
    {
        return false;
    }

    const float SurfaceAngle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(SurfaceHit.Normal, FVector::UpVector)));
    if (SurfaceAngle > LedgeMaxWalkableAngle)
    {
        return false;
    }

    OutWallHit = WallHit;
    OutLedgeSurfacePoint = SurfaceHit.ImpactPoint;
    return true;
}

void AFrameworkCharacter::UpdateLedgeHang(float DeltaTime)
{
    UCharacterMovementComponent* Movement = GetCharacterMovement();
    Movement->Velocity = FVector::ZeroVector;

    FHitResult WallHit;
    FVector SurfacePoint;
    if (!TraceForLedge(WallHit, SurfacePoint))
    {
        ExitLedgeHang();
        return;
    }

    const FVector ActorLocation = GetActorLocation();
    const FVector TargetLocation = FVector(ActorLocation.X, ActorLocation.Y, SurfacePoint.Z + LedgeHangVerticalOffset);
    SetActorLocation(TargetLocation, false);

    LedgeWallNormal = WallHit.Normal;
    LedgeClimbTargetLocation = SurfacePoint;
}

void AFrameworkCharacter::EnterLedgeHang(const FHitResult& WallHit, const FVector& SurfacePoint)
{
    UCharacterMovementComponent* Movement = GetCharacterMovement();

    bIsLedgeHanging = true;
    LedgeWallNormal = WallHit.Normal;
    LedgeClimbTargetLocation = SurfacePoint;

    if (bIsWallSliding)
    {
        EndWallSlide();
    }

    const FVector ActorLocation = GetActorLocation();
    const FVector HangLocation = FVector(ActorLocation.X, ActorLocation.Y, SurfacePoint.Z + LedgeHangVerticalOffset);

    Movement->SetMovementMode(MOVE_Flying);
    Movement->StopMovementImmediately();
    SetActorLocation(HangLocation, false);

    bDoubleJumpUsed = false;
    bJumpedThisAirtime = false;
}

void AFrameworkCharacter::ExitLedgeHang()
{
    bIsLedgeHanging = false;
    LedgeWallNormal = FVector::ZeroVector;
    LedgeClimbTargetLocation = FVector::ZeroVector;

    UCharacterMovementComponent* Movement = GetCharacterMovement();
    if (Movement->MovementMode == MOVE_Flying)
    {
        Movement->SetMovementMode(MOVE_Falling);
    }
}
