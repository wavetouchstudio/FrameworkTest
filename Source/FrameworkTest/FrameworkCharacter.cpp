#include "FrameworkCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"

AFrameworkCharacter::AFrameworkCharacter()
{
    // 2, not 1: engine's CanJumpInternal() blocks Jump() while falling with
    // JumpCurrentCount==0 unless JumpCurrentCount+1 < JumpMaxCount. Needed for
    // coyote-time jumps (falling, but no jump performed yet). Our own
    // bJumpedThisAirtime/bDoubleJumpUsed flags gate everything else.
    JumpMaxCount = 2;

    FCharacterMovementProfile Profile0;
    Profile0.CapsuleScale = 1.f;
    Profile0.Mass = 80.f;
    Profile0.WalkSpeed = 550.f;
    Profile0.SprintSpeed = 850.f;
    Profile0.JumpZVelocity = 1400.f;
    Profile0.DoubleJumpZVelocity = 1400.f;
    Profile0.bEnableWallJump = true;
    Profile0.bEnableWallSlide = true;
    Profile0.bWallJumpResetsDoubleJump = true;
    MovementProfiles.Add(Profile0);

    FCharacterMovementProfile Profile1;
    Profile1.CapsuleScale = 1.f;
    Profile1.Mass = 80.f;
    Profile1.WalkSpeed = 550.f;
    Profile1.SprintSpeed = 850.f;
    Profile1.JumpZVelocity = 1400.f;
    Profile1.DoubleJumpZVelocity = 1400.f;
    Profile1.bEnableWallJump = true;
    Profile1.bEnableWallSlide = true;
    Profile1.bWallJumpResetsDoubleJump = false;
    MovementProfiles.Add(Profile1);

    FCharacterMovementProfile Profile2;
    Profile2.CapsuleScale = 1.3f;
    Profile2.Mass = 140.f;
    Profile2.WalkSpeed = 450.f;
    Profile2.SprintSpeed = 700.f;
    Profile2.JumpZVelocity = 1200.f;
    Profile2.DoubleJumpZVelocity = 1200.f;
    Profile2.bEnableWallJump = false;
    Profile2.bEnableWallSlide = true;
    Profile2.bWallJumpResetsDoubleJump = true;
    Profile2.bEnableDoubleJump = false;
    MovementProfiles.Add(Profile2);
}

void AFrameworkCharacter::BeginPlay()
{
    Super::BeginPlay();

    CurrentHealth = MaxHealth;
    UpdateHealthDebugDisplay();
    SetMovementProfile(CurrentProfileIndex);
}

void AFrameworkCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    UCharacterMovementComponent* Movement = GetCharacterMovement();

    if (bIsLedgeHanging)
    {
        UpdateLedgeHang(DeltaTime);
        UpdateMechanicDebugDisplay();
        return;
    }

    if (Movement->IsFalling())
    {
        PeakFallSpeed = FMath::Max(PeakFallSpeed, -Movement->Velocity.Z);

        FHitResult LedgeWallHit;
        FVector LedgeSurfacePoint;
        if (TraceForLedge(LedgeWallHit, LedgeSurfacePoint))
        {
            EnterLedgeHang(LedgeWallHit, LedgeSurfacePoint);
            UpdateMechanicDebugDisplay();
            return;
        }

        UpdateWallSlide(DeltaTime);
    }
    else if (bIsWallSliding)
    {
        EndWallSlide();
    }

    UpdateMechanicDebugDisplay();
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
        PeakFallSpeed = 0.f;
        return;
    }

    if (bIsWallSliding && MovementProfiles[CurrentProfileIndex].bEnableWallJump)
    {
        const float AngleRad = FMath::DegreesToRadians(WallJumpAngle);
        const FVector LaunchVelocity = WallSlideNormal * (WallJumpSpeed * FMath::Cos(AngleRad)) + FVector(0.f, 0.f, WallJumpSpeed * FMath::Sin(AngleRad));
        EndWallSlide();

        bJumpedThisAirtime = true;
        if (MovementProfiles[CurrentProfileIndex].bWallJumpResetsDoubleJump)
        {
            bDoubleJumpUsed = false;
        }
        LaunchCharacter(LaunchVelocity, true, true);
        return;
    }

    const bool bGrounded = !Movement->IsFalling();
    const bool bCoyoteWindow = !bGrounded && !bJumpedThisAirtime
        && (GetWorld()->GetTimeSeconds() - LeftGroundTime) <= CoyoteTime;

    if (bGrounded || bCoyoteWindow)
    {
        FirstJumpTime = GetWorld()->GetTimeSeconds();
        bDoubleJumpUsed = false;
        bJumpedThisAirtime = true;
        Jump();
    }
    else if (!bIsWallSliding && MovementProfiles[CurrentProfileIndex].bEnableDoubleJump && !bDoubleJumpUsed && (GetWorld()->GetTimeSeconds() - FirstJumpTime) >= DoubleJumpDelay)
    {
        bDoubleJumpUsed = true;
        LaunchCharacter(FVector(0.f, 0.f, MovementProfiles[CurrentProfileIndex].DoubleJumpZVelocity), false, true);
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

    if (PeakFallSpeed > FallDamageSafeSpeed)
    {
        const float ExcessSpeed = PeakFallSpeed - FallDamageSafeSpeed;
        const FCharacterMovementProfile& Profile = MovementProfiles[CurrentProfileIndex];
        const float Damage = ExcessSpeed * FallDamageRatePerUnit
            * (Profile.Mass / FallDamageReferenceMass) * GetFallDamageMultiplier();
        ApplyFallDamage(Damage);
    }
    PeakFallSpeed = 0.f;
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

    if (!MovementProfiles[CurrentProfileIndex].bEnableWallSlide)
    {
        if (bIsWallSliding)
        {
            EndWallSlide();
        }
        return;
    }

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
    PeakFallSpeed = 0.f;
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

void AFrameworkCharacter::SetMovementProfile(int32 ProfileIndex)
{
    if (MovementProfiles.Num() == 0)
    {
        return;
    }

    CurrentProfileIndex = FMath::Clamp(ProfileIndex, 0, MovementProfiles.Num() - 1);
    const FCharacterMovementProfile& Profile = MovementProfiles[CurrentProfileIndex];

    SetActorScale3D(FVector(Profile.CapsuleScale));

    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetMassOverrideInKg(NAME_None, Profile.Mass, true);
    }

    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->JumpZVelocity = Profile.JumpZVelocity;
    }

    ApplyWalkSpeed();
    UpdateProfileDebugDisplay();
}

void AFrameworkCharacter::ApplyWalkSpeed()
{
    if (MovementProfiles.Num() == 0)
    {
        return;
    }

    UCharacterMovementComponent* Movement = GetCharacterMovement();
    if (!Movement)
    {
        return;
    }

    const FCharacterMovementProfile& Profile = MovementProfiles[CurrentProfileIndex];
    Movement->MaxWalkSpeed = bIsSprinting ? Profile.SprintSpeed : Profile.WalkSpeed;
}

void AFrameworkCharacter::SetSprinting(bool bNewSprinting)
{
    bIsSprinting = bNewSprinting;
    ApplyWalkSpeed();
}

void AFrameworkCharacter::UpdateProfileDebugDisplay() const
{
    if (!GEngine)
    {
        return;
    }

    GEngine->AddOnScreenDebugMessage(7725, 5.f, FColor::Cyan,
        FString::Printf(TEXT("Movement Profile: %d"), CurrentProfileIndex));
}

void AFrameworkCharacter::AddFallDamageModifier(FName ModifierID, float MultiplierDelta)
{
    FallDamageModifiers.Add(ModifierID, MultiplierDelta);
}

void AFrameworkCharacter::RemoveFallDamageModifier(FName ModifierID)
{
    FallDamageModifiers.Remove(ModifierID);
}

float AFrameworkCharacter::GetFallDamageMultiplier() const
{
    float Multiplier = 1.f;
    for (const TPair<FName, float>& Modifier : FallDamageModifiers)
    {
        Multiplier += Modifier.Value;
    }
    return FMath::Max(Multiplier, 0.f);
}

void AFrameworkCharacter::ApplyFallDamage_Implementation(float Damage)
{
    CurrentHealth -= Damage;

    if (CurrentHealth <= 0.f)
    {
        CurrentHealth = MaxHealth;
    }

    UpdateHealthDebugDisplay();
}

void AFrameworkCharacter::UpdateMechanicDebugDisplay() const
{
    if (!bShowMechanicDebugText || !GEngine)
    {
        return;
    }

    FString StateText;
    FColor StateColor = FColor::White;

    if (bIsInPlacementMode)
    {
        StateText = TEXT("Placement Mode");
        StateColor = FColor::Cyan;
    }
    else if (bIsHoldingObject)
    {
        StateText = TEXT("Holding");
        StateColor = FColor::Cyan;
    }
    else if (bIsLedgeHanging)
    {
        StateText = TEXT("Ledge Hang");
        StateColor = FColor::Green;
    }
    else if (bIsWallSliding)
    {
        StateText = TEXT("Wall Slide");
        StateColor = FColor::Yellow;
    }
    else if (bDoubleJumpUsed)
    {
        StateText = TEXT("Jump 2 (Double Jump)");
        StateColor = FColor::Orange;
    }
    else if (bJumpedThisAirtime)
    {
        StateText = TEXT("Jump 1");
        StateColor = FColor::Orange;
    }
    else
    {
        StateText = TEXT("Normal");
        StateColor = FColor::White;
    }

    GEngine->AddOnScreenDebugMessage(7723, 0.f, StateColor, FString::Printf(TEXT("Mechanic State: %s"), *StateText));
}

void AFrameworkCharacter::UpdateHealthDebugDisplay() const
{
    if (!GEngine)
    {
        return;
    }

    GEngine->AddOnScreenDebugMessage(7724, 5.f, FColor::Red, FString::Printf(TEXT("Health: %.0f / %.0f"), CurrentHealth, MaxHealth));
}
