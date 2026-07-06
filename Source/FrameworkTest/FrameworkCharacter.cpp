#include "FrameworkCharacter.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "NiagaraComponent.h"
#include "CableComponent.h"
#include "GrappleAnchor.h"
#include "WallSlideSurfaceComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/DecalComponent.h"
#include "FrameworkGameInstance.h"

// Constructor for AFrameworkCharacter class
// Initializes character with movement profiles and default settings
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

    GlideTrailEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("GlideTrailEffect"));
    GlideTrailEffect->SetupAttachment(GetMesh());
    GlideTrailEffect->SetHiddenInGame(true);
    GlideTrailEffect->bAutoActivate = false;

    GrappleCable = CreateDefaultSubobject<UCableComponent>(TEXT("GrappleCable"));
    GrappleCable->SetupAttachment(GetMesh());
    GrappleCable->SetVisibility(false);

    GrappleBeamEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("GrappleBeamEffect"));
    GrappleBeamEffect->SetupAttachment(GetMesh());
    GrappleBeamEffect->SetHiddenInGame(true);
    GrappleBeamEffect->bAutoActivate = false;
}

// Called when the game starts or when spawned
// Sets up character components and input bindings
void AFrameworkCharacter::BeginPlay()
{
    Super::BeginPlay();

    CurrentHealth = MaxHealth;
    GlideTimeRemaining = MovementProfiles[CurrentProfileIndex].GlideMaxDuration;
    UpdateHealthDebugDisplay();
    SetMovementProfile(CurrentProfileIndex);

    if (InteractPromptWidgetClass)
    {
        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            InteractPromptWidget = CreateWidget<UUserWidget>(PC, InteractPromptWidgetClass);
            if (InteractPromptWidget)
            {
                InteractPromptWidget->AddToViewport();
                InteractPromptWidget->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
    }
}

// Called every frame
// Handles character movement and mechanics updates
void AFrameworkCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateInteractDetection(); // must run before the early-returns below, or the prompt freezes while grappling/ledge-hanging
    UpdateDropShadowVisibility(); // BP_DropShadow_AC has no grounded check of its own — force-hide its decal while on ground

    UCharacterMovementComponent* Movement = GetCharacterMovement();

    if (bIsGrappling)
    {
        UpdateGrapple(DeltaTime);
        UpdateMechanicDebugDisplay();
        return;
    }

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
        UpdateGlide(DeltaTime);
    }
    else
    {
        if (bIsWallSliding)
        {
            EndWallSlide();
        }
        if (bIsGliding)
        {
            EndGlide();
        }
    }

    if (!bIsGliding)
    {
        const FCharacterMovementProfile& Profile = MovementProfiles[CurrentProfileIndex];
        GlideTimeRemaining = FMath::Min(Profile.GlideMaxDuration, GlideTimeRemaining + Profile.GlideRechargeRate * DeltaTime);
    }

    UpdateGrappleTargeting();
    UpdateMechanicDebugDisplay();
}

// Handles jump request from player input
// Manages jump logic including double jumps
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
                MovementProfiles[CurrentProfileIndex].WallTraceChannel, Capsule->GetCollisionShape(), Params);

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
        const FCharacterMovementProfile& WallJumpProfile = MovementProfiles[CurrentProfileIndex];
        const float AngleRad = FMath::DegreesToRadians(WallJumpProfile.WallJumpAngle);
        const FVector LaunchVelocity = WallSlideNormal * (WallJumpProfile.WallJumpSpeed * FMath::Cos(AngleRad)) + FVector(0.f, 0.f, WallJumpProfile.WallJumpSpeed * FMath::Sin(AngleRad));
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
    WallSlideElapsedTime = 0.f;

    if (bIsLedgeHanging)
    {
        ExitLedgeHang();
    }

    if (bIsGliding)
    {
        EndGlide();
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

    if (PrevMovementMode == MOVE_Flying && bIsGrappling)
    {
        EndGrapple();
    }
}

void AFrameworkCharacter::TraceForWall(FHitResult& OutHit) const
{
    const FCharacterMovementProfile& Profile = MovementProfiles[CurrentProfileIndex];
    const FVector Start = GetActorLocation();
    const FVector End = Start + GetActorForwardVector() * Profile.WallTraceDistance;

    FCollisionQueryParams Params(SCENE_QUERY_STAT(WallTrace), false, this);
    GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, Profile.WallTraceChannel, Params);
}

void AFrameworkCharacter::UpdateWallSlide(float DeltaTime)
{
    UCharacterMovementComponent* Movement = GetCharacterMovement();
    const FCharacterMovementProfile& Profile = MovementProfiles[CurrentProfileIndex];

    if (!Profile.bEnableWallSlide)
    {
        if (bIsWallSliding)
        {
            EndWallSlide();
        }
        return;
    }

    if (Movement->Velocity.Z >= -Profile.WallSlideMinFallSpeed)
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

    const UWallSlideSurfaceComponent* SurfaceComp = WallHit.GetActor() ? WallHit.GetActor()->FindComponentByClass<UWallSlideSurfaceComponent>() : nullptr;

    // Curved/non-slideable surface -> reject
    if (SurfaceComp && !SurfaceComp->bAllowWallSlide)
    {
        if (bIsWallSliding)
        {
            EndWallSlide();
        }
        return;
    }

    const float ApproachThreshold = (SurfaceComp && SurfaceComp->ApproachDotThresholdOverride >= 0.f)
        ? SurfaceComp->ApproachDotThresholdOverride
        : Profile.WallApproachDotThreshold;

    const float ApproachDot = FVector::DotProduct(GetActorForwardVector(), -WallHit.Normal);
    if (ApproachDot < ApproachThreshold)
    {
        if (bIsWallSliding)
        {
            EndWallSlide();
        }
        return;
    }

    if (Profile.WallSlideMaxDuration > 0.f && WallSlideElapsedTime >= Profile.WallSlideMaxDuration)
    {
        if (bIsWallSliding)
        {
            EndWallSlide();
        }
        return;
    }

    WallSlideElapsedTime += DeltaTime;

    bIsWallSliding = true;
    WallSlideNormal = WallHit.Normal;

    const float EffectiveSlideSpeed = Profile.WallSlideSpeed * (SurfaceComp ? SurfaceComp->SlideSpeedMultiplier : 1.f);

    FVector Velocity = Movement->Velocity;
    Velocity.Z = FMath::Max(Velocity.Z, -EffectiveSlideSpeed);
    Movement->Velocity = Velocity;
}

void AFrameworkCharacter::EndWallSlide()
{
    bIsWallSliding = false;
    WallSlideNormal = FVector::ZeroVector;
    WallSlideElapsedTime = 0.f;
}

bool AFrameworkCharacter::TraceForLedge(FHitResult& OutWallHit, FVector& OutLedgeSurfacePoint) const
{
    UCapsuleComponent* Capsule = GetCapsuleComponent();
    if (!Capsule)
    {
        return false;
    }

    const ECollisionChannel LedgeTraceChannel = MovementProfiles[CurrentProfileIndex].WallTraceChannel;
    const FVector ActorLocation = GetActorLocation();
    const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    const float FeetZ = ActorLocation.Z - CapsuleHalfHeight;

    const FVector Forward = GetActorForwardVector();
    const FVector HandOrigin = FVector(ActorLocation.X, ActorLocation.Y, FeetZ + LedgeGrabHeight);

    FCollisionQueryParams Params(SCENE_QUERY_STAT(LedgeTrace), false, this);

    // 1. Hand-height wall trace
    const FVector WallEnd = HandOrigin + Forward * LedgeWallTraceDistance;
    FHitResult WallHit;
    const bool bHitWall = GetWorld()->LineTraceSingleByChannel(WallHit, HandOrigin, WallEnd, LedgeTraceChannel, Params);

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
    const bool bClearanceBlocked = GetWorld()->LineTraceSingleByChannel(ClearanceHit, ClearanceOrigin, ClearanceEnd, LedgeTraceChannel, Params);

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
    const bool bHitSurface = GetWorld()->LineTraceSingleByChannel(SurfaceHit, SurfaceTraceStart, SurfaceTraceEnd, LedgeTraceChannel, Params);

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

void AFrameworkCharacter::UpdateGlide(float DeltaTime)
{
    UCharacterMovementComponent* Movement = GetCharacterMovement();
    const FCharacterMovementProfile& Profile = MovementProfiles[CurrentProfileIndex];

    if (bJumpInputHeld && !bWantsGlide && (GetWorld()->GetTimeSeconds() - JumpHeldStartTime) >= Profile.GlideHoldThreshold)
    {
        bWantsGlide = true;
    }

    if (!bWantsGlide || !Profile.bEnableGlide || GlideTimeRemaining <= 0.f || bIsWallSliding)
    {
        if (bIsGliding)
        {
            EndGlide();
        }
        return;
    }

    if (!bIsGliding)
    {
        bIsGliding = true;
        GlideTrailEffect->SetHiddenInGame(false);
        GlideTrailEffect->Activate(true);

        // Wind-catch kick: lift impulse added to current fall velocity, not a downward dive
        const float LaunchAngleRad = FMath::DegreesToRadians(Profile.GlideLaunchAngle);
        const FVector Forward = GetActorForwardVector();
        const FVector LaunchImpulse = Forward * (Profile.GlideLaunchSpeed * FMath::Cos(LaunchAngleRad)) + FVector(0.f, 0.f, Profile.GlideLaunchSpeed * FMath::Sin(LaunchAngleRad));
        Movement->Velocity += LaunchImpulse;
    }

    GlideTimeRemaining = FMath::Max(0.f, GlideTimeRemaining - DeltaTime);

    FVector Velocity = Movement->Velocity;
    if (Velocity.Z < -Profile.GlideMaxFallSpeed)
    {
        Velocity.Z = FMath::FInterpTo(Velocity.Z, -Profile.GlideMaxFallSpeed, DeltaTime, Profile.GlideZInterpSpeed);
    }
    if (Profile.GlideForwardSpeed > 0.f)
    {
        const FVector Forward = GetActorForwardVector() * Profile.GlideForwardSpeed;
        Velocity.X = Forward.X;
        Velocity.Y = Forward.Y;
    }
    Movement->Velocity = Velocity;

    if (GlideTimeRemaining <= 0.f)
    {
        EndGlide();
    }
}

void AFrameworkCharacter::EndGlide()
{
    if (bIsGliding)
    {
        GlideTrailEffect->Deactivate();
        GlideTrailEffect->SetHiddenInGame(true);
    }
    bIsGliding = false;
}

void AFrameworkCharacter::SetGliding(bool bNewGliding)
{
    if (bNewGliding && !bJumpInputHeld)
    {
        JumpHeldStartTime = GetWorld()->GetTimeSeconds();
    }
    bJumpInputHeld = bNewGliding;

    if (!bNewGliding)
    {
        bWantsGlide = false;
        if (bIsGliding)
        {
            EndGlide();
        }
    }
}

void AFrameworkCharacter::UpdateGrappleTargeting()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    AGrappleAnchor* BestCandidate = nullptr;

    if (PC && PC->PlayerCameraManager)
    {
        const FVector CameraLocation = PC->PlayerCameraManager->GetCameraLocation();
        const FVector CameraForward = PC->PlayerCameraManager->GetCameraRotation().Vector();

        TArray<AActor*> Anchors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGrappleAnchor::StaticClass(), Anchors);

        float BestDot = GrappleTargetingDotThreshold;
        for (AActor* Actor : Anchors)
        {
            AGrappleAnchor* Anchor = Cast<AGrappleAnchor>(Actor);
            if (!Anchor)
            {
                continue;
            }

            if (FVector::Dist(GetActorLocation(), Anchor->GetActorLocation()) > GrappleTargetingRadius)
            {
                continue;
            }

            const FVector ToAnchor = (Anchor->GetActorLocation() - CameraLocation).GetSafeNormal();
            const float Dot = FVector::DotProduct(CameraForward, ToAnchor);

            if (Dot >= BestDot)
            {
                BestDot = Dot;
                BestCandidate = Anchor;
            }
        }
    }

    if (BestCandidate != HighlightedAnchor.Get())
    {
        if (HighlightedAnchor.IsValid())
        {
            HighlightedAnchor->OnHighlightChanged(false);
        }
        if (BestCandidate)
        {
            BestCandidate->OnHighlightChanged(true);
        }
        HighlightedAnchor = BestCandidate;
    }

    if (bDebugDrawGrappleTargeting && HighlightedAnchor.IsValid())
    {
        DrawDebugSphere(GetWorld(), HighlightedAnchor->GetActorLocation(), 40.f, 12, FColor::Green, false, 0.f);
    }
}

void AFrameworkCharacter::RequestGrapple()
{
    if (bIsGrappling)
    {
        EndGrapple();
        return;
    }

    if (!HighlightedAnchor.IsValid())
    {
        OnGrappleMiss();
        return;
    }

    if (bIsWallSliding)
    {
        EndWallSlide();
    }
    if (bIsLedgeHanging)
    {
        ExitLedgeHang();
    }
    if (bIsGliding)
    {
        EndGlide();
    }

    GrappleTargetAnchor = HighlightedAnchor;
    bIsGrappling = true;

    UCharacterMovementComponent* Movement = GetCharacterMovement();
    Movement->SetMovementMode(MOVE_Flying);
    Movement->StopMovementImmediately();

    GrappleCable->SetVisibility(true);
    GrappleBeamEffect->SetHiddenInGame(false);
    GrappleBeamEffect->Activate(true);
    GrappleBeamEffect->SetVariableVec3(GrappleBeamStartParamName, GetActorLocation());
    GrappleBeamEffect->SetVariableVec3(GrappleBeamEndParamName, GrappleTargetAnchor->GetActorLocation());
}

void AFrameworkCharacter::UpdateGrapple(float DeltaTime)
{
    if (!GrappleTargetAnchor.IsValid())
    {
        EndGrapple();
        return;
    }

    const FVector AnchorLocation = GrappleTargetAnchor->GetActorLocation();
    const FVector ToAnchor = AnchorLocation - GetActorLocation();
    const FVector Direction = ToAnchor.GetSafeNormal();

    SetActorLocation(GetActorLocation() + Direction * GrapplePullSpeed * DeltaTime, true);

    GrappleCable->EndLocation = GrappleCable->GetComponentTransform().InverseTransformPosition(AnchorLocation);
    GrappleBeamEffect->SetVariableVec3(GrappleBeamStartParamName, GetActorLocation());
    GrappleBeamEffect->SetVariableVec3(GrappleBeamEndParamName, AnchorLocation);

    if (ToAnchor.Size() <= GrappleReleaseDistance)
    {
        EndGrapple();
    }
}

void AFrameworkCharacter::EndGrapple()
{
    if (GrappleTargetAnchor.IsValid())
    {
        const FVector ToAnchor = GrappleTargetAnchor->GetActorLocation() - GetActorLocation();
        SetActorRotation(FRotator(0.f, ToAnchor.Rotation().Yaw, 0.f));
    }

    GrappleCable->SetVisibility(false);
    GrappleBeamEffect->Deactivate();
    GrappleBeamEffect->SetHiddenInGame(true);
    bIsGrappling = false;
    GrappleTargetAnchor = nullptr;

    UCharacterMovementComponent* Movement = GetCharacterMovement();
    if (Movement->MovementMode == MOVE_Flying)
    {
        Movement->SetMovementMode(MOVE_Falling);
    }
}

void AFrameworkCharacter::UpdateInteractDetection()
{
    bCanInteract = false;
    CurrentInteractable = nullptr;

    UWorld* World = GetWorld();
    if (!InteractableInterfaceClass || !World)
    {
        if (InteractPromptWidget)
        {
            InteractPromptWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
        return;
    }

    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(InteractDetection), false, this);
    World->OverlapMultiByChannel(Overlaps, GetActorLocation(), FQuat::Identity, InteractTraceChannel, FCollisionShape::MakeSphere(InteractSphereRadius), QueryParams);

    AActor* BestCandidate = nullptr;
    float BestDistSq = FLT_MAX;

    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* OtherActor = Overlap.GetActor();
        if (!OtherActor || OtherActor == this || !OtherActor->GetClass()->ImplementsInterface(InteractableInterfaceClass))
        {
            continue;
        }

        const float DistSq = FVector::DistSquared(GetActorLocation(), OtherActor->GetActorLocation());
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            BestCandidate = OtherActor;
        }
    }

    if (BestCandidate)
    {
        FHitResult Hit;
        FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(InteractTrace), false, this);
        const bool bBlocked = World->LineTraceSingleByChannel(Hit, GetActorLocation(), BestCandidate->GetActorLocation(), InteractTraceChannel, TraceParams);
        const bool bLineOfSight = !bBlocked || Hit.GetActor() == BestCandidate;

        if (bLineOfSight && FMath::Sqrt(BestDistSq) <= MaxInteractDistance)
        {
            bCanInteract = true;
            CurrentInteractable = BestCandidate;
        }
    }

    if (InteractPromptWidget)
    {
        InteractPromptWidget->SetVisibility(bCanInteract ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}

void AFrameworkCharacter::UpdateDropShadowVisibility()
{
    // BP_DropShadow_AC traces unconditionally every tick and has no grounded check, plus its
    // trace discards any hit that starts in initial overlap — which is the surface you're
    // standing on, so it tunnels to whatever's below. Force-hide the decal while grounded instead
    // of touching that asset's internals.
    const bool bAirborne = !GetCharacterMovement()->IsMovingOnGround();

    TArray<UDecalComponent*> DropShadowDecals;
    GetComponents<UDecalComponent>(DropShadowDecals);
    for (UDecalComponent* Decal : DropShadowDecals)
    {
        Decal->SetVisibility(bAirborne);
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
        Movement->AirControl = Profile.AirControl;
        Movement->GravityScale = Profile.GravityScale;
        Movement->MaxAcceleration = Profile.MaxAcceleration;
        Movement->BrakingDecelerationWalking = Profile.BrakingDecelerationWalking;
        Movement->RotationRate = FRotator(0.f, Profile.RotationYawRate, 0.f);
        Movement->bOrientRotationToMovement = Profile.bOrientRotationToMovement;
        bUseControllerRotationYaw = Profile.bUseControllerRotationYawSetting;
    }

    if (USpringArmComponent* SpringArm = FindComponentByClass<USpringArmComponent>())
    {
        SpringArm->TargetArmLength = Profile.CameraArmLength;
        SpringArm->SocketOffset = Profile.CameraSocketOffset;
        SpringArm->bEnableCameraLag = Profile.bEnableCameraLag;
        SpringArm->CameraLagSpeed = Profile.CameraLagSpeed;
    }

    if (UCameraComponent* Camera = FindComponentByClass<UCameraComponent>())
    {
        Camera->SetFieldOfView(Profile.CameraFOV);
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
        if (UFrameworkGameInstance* GI = Cast<UFrameworkGameInstance>(GetGameInstance()))
            GI->RespawnAtLastBonfire(this);
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

    if (bIsGrappling)
    {
        StateText = TEXT("Grappling");
        StateColor = FColor::Magenta;
    }
    else if (bIsInPlacementMode)
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
    else if (bIsGliding)
    {
        StateText = FString::Printf(TEXT("Gliding (%.1fs left)"), GlideTimeRemaining);
        StateColor = FColor::Cyan;
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

    if (!bIsGrappling && HighlightedAnchor.IsValid())
    {
        GEngine->AddOnScreenDebugMessage(7726, 0.f, FColor::Green,
            FString::Printf(TEXT("Grapple Target: %s"), *HighlightedAnchor->GetName()));
    }
    else if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(7726, 0.f, FColor::Black, TEXT(""));
    }
}

void AFrameworkCharacter::UpdateHealthDebugDisplay() const
{
    if (!GEngine)
    {
        return;
    }

    GEngine->AddOnScreenDebugMessage(7724, 5.f, FColor::Red, FString::Printf(TEXT("Health: %.0f / %.0f"), CurrentHealth, MaxHealth));
}
