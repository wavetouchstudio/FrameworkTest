#include "PickupObject.h"

#include "Engine/OverlapResult.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "CollisionShape.h"
#include "PuzzleTrigger.h"

APickupObject::APickupObject()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;
    Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Mesh->SetSimulatePhysics(true);
    Mesh->SetLinearDamping(0.8f);
    Mesh->SetAngularDamping(4.f);

    PlacementIndicator = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlacementIndicator"));
    PlacementIndicator->SetupAttachment(Mesh);
    PlacementIndicator->SetHiddenInGame(true);
    PlacementIndicator->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    BeamEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("BeamEffect"));
    BeamEffect->SetupAttachment(Mesh);
    BeamEffect->SetHiddenInGame(true);
    BeamEffect->bAutoActivate = false;

    TopSocket = CreateDefaultSubobject<USceneComponent>(TEXT("TopSocket"));
    TopSocket->SetupAttachment(Mesh);

    StackDetection = CreateDefaultSubobject<USphereComponent>(TEXT("StackDetection"));
    StackDetection->SetupAttachment(TopSocket);
    StackDetection->SetSphereRadius(60.f);
    StackDetection->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

// Called when the game starts or when spawned
// Sets up pickup object components and physics
void APickupObject::BeginPlay()
{
    Super::BeginPlay();
    StackDetection->OnComponentBeginOverlap.AddDynamic(this, &APickupObject::OnStackSphereBeginOverlap);
    StackDetection->OnComponentEndOverlap.AddDynamic(this, &APickupObject::OnStackSphereEndOverlap);

    // Auto-fit detection radius to the mesh so different block sizes don't need manual re-tuning
    if (Mesh->GetStaticMesh())
        StackDetection->SetSphereRadius(Mesh->Bounds.SphereRadius * 1.1f);
}

// Toggles carrying state of the pickup object
void APickupObject::ToggleCarry()
{
    if (PickupState == EPickupState::Placing) return;
    if (IsValid(StackedBlock)) return; // bottom block is not interactable while something is stacked on it

    const float Now = GetWorld()->GetTimeSeconds();
    if (Now - LastToggleTime < ToggleCooldown) return;
    LastToggleTime = Now;

    if (bIsCarried)
    {
        BeginDrop();
    }
    else
    {
        ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
        if (IsValid(Player) && !IsPlayerStandingOnTop(Player))
            PickUp(Player);
    }
}

// Checks if player is standing on top of the pickup object
bool APickupObject::IsPlayerStandingOnTop(ACharacter* Player) const
{
    UCapsuleComponent* Capsule = Player->GetCapsuleComponent();
    if (!Capsule) return false;

    const FVector Origin = Mesh->Bounds.Origin;
    const FVector BoxExtent = Mesh->Bounds.BoxExtent;

    const FVector PlayerLoc = Player->GetActorLocation();
    const float PlayerBottomZ = PlayerLoc.Z - Capsule->GetScaledCapsuleHalfHeight();
    const float BlockTopZ = Origin.Z + BoxExtent.Z;

    const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
    const bool bHorizontalOverlap =
        FMath::Abs(PlayerLoc.X - Origin.X) <= BoxExtent.X + CapsuleRadius &&
        FMath::Abs(PlayerLoc.Y - Origin.Y) <= BoxExtent.Y + CapsuleRadius;

    return bHorizontalOverlap && PlayerBottomZ >= BlockTopZ - StandingOnTopTolerance;
}

// Picks up the object by the specified character
void APickupObject::PickUp(ACharacter* InCarrier)
{
    Carrier = InCarrier;
    DefaultJumpMaxCount = Carrier->JumpMaxCount; // ponytail: capture here too, not just in StartPlacement — FinalizeDrop restores from this even if placement was never entered
    bIsCarried = true;
    FloatTime = 0.f;
    LerpAlpha = 0.f;
    CurrentCarryDistance = CarryForwardDistance;
    TargetCarryDistance = CarryForwardDistance;
    LerpStartPosition = GetActorLocation();
    PickupLocation = GetActorLocation();
    PickupState = EPickupState::LerpingToHold;

    Mesh->SetSimulatePhysics(false);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // kinematic — has physical presence but doesn't simulate

    // Notify any trigger the block was holding so it can swap back to the player or deactivate
    TArray<AActor*> Overlapping;
    Mesh->GetOverlappingActors(Overlapping, APuzzleTrigger::StaticClass());
    for (AActor* Actor : Overlapping)
        if (APuzzleTrigger* Trigger = Cast<APuzzleTrigger>(Actor))
            Trigger->NotifyPickupLifted(this);

    Carrier->bUseControllerRotationYaw = true;
    Carrier->GetCharacterMovement()->bOrientRotationToMovement = false;

    BeamEffect->SetHiddenInGame(false);
    BeamEffect->Activate(true);

    if (APlayerController* PC = Cast<APlayerController>(Carrier->GetController()))
        EnableInput(PC);

    CachedSpringArm = Carrier->FindComponentByClass<USpringArmComponent>();
    if (CachedSpringArm)
    {
        DefaultArmLength = CachedSpringArm->TargetArmLength;
        DefaultSocketOffset = CachedSpringArm->SocketOffset;
    }

    SetActorTickEnabled(true);
    OnPickedUp();
}

// Begins the drop process for the pickup object
void APickupObject::BeginDrop()
{
    bIsCarried = false;
    LerpAlpha = 0.f;
    LerpStartPosition = GetActorLocation();
    PlacementRotationInput = 0.f;
    PlacementIndicator->SetHiddenInGame(true);
    BeamEffect->Deactivate();
    BeamEffect->SetHiddenInGame(true);
    PickupState = EPickupState::LerpingToDrop;
}

void APickupObject::FinalizeDrop()
{
    PickupState = EPickupState::Idle;
    PlacementIndicator->SetHiddenInGame(true);
    BeamEffect->Deactivate();
    BeamEffect->SetHiddenInGame(true);

    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Mesh->SetSimulatePhysics(true);
    Mesh->SetEnableGravity(true); // called after body is created so it updates the active physics actor

    Carrier->bUseControllerRotationYaw = false;
    Carrier->GetCharacterMovement()->bOrientRotationToMovement = true;
    Carrier->JumpMaxCount = DefaultJumpMaxCount;
    PlacementVerticalInput = 0.f;
    PlacementHeightAdjust = 0.f;

    if (APlayerController* PC = Cast<APlayerController>(Carrier->GetController()))
        DisableInput(PC);

    // Notify any trigger we're already sitting inside so it can swap the player out
    TArray<AActor*> Overlapping;
    Mesh->GetOverlappingActors(Overlapping, APuzzleTrigger::StaticClass());
    for (AActor* Actor : Overlapping)
        if (APuzzleTrigger* Trigger = Cast<APuzzleTrigger>(Actor))
            Trigger->NotifyPickupDropped(this);

    Carrier = nullptr;
    // Tick stays enabled so the spring arm can lerp back; it disables itself once the arm reaches default.
    OnPlaced();
}

void APickupObject::StartPlacement()
{
    if (PickupState != EPickupState::Held) return;

    // Seed distance from camera to block so the block doesn't snap on first Placing tick
    if (APlayerController* PC = Cast<APlayerController>(Carrier->GetController()))
    {
        FVector CamLoc; FRotator CamRot;
        PC->GetPlayerViewPoint(CamLoc, CamRot);
        CurrentCarryDistance = FMath::Clamp(FVector::Dist(GetActorLocation(), CamLoc), MinCarryDistance, MaxCarryDistance);
        TargetCarryDistance = CurrentCarryDistance;
    }

    PickupState = EPickupState::Placing;
    bPlacementJustStarted = true;

    // Query-only while placing — kinematic block shouldn't shove other dynamic pickups as it's dragged around
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    PlacementRotationInput = 0.f;
    PlacementVerticalInput = 0.f;
    PlacementHeightAdjust = FMath::Clamp(GetPlacementStartHeight(), 0.f, MaxCarryDistance);
    PlacementIndicator->SetHiddenInGame(false);

    if (IsValid(Carrier))
    {
        Carrier->bUseControllerRotationYaw = false;
        DefaultJumpMaxCount = Carrier->JumpMaxCount;
        Carrier->JumpMaxCount = 0;

        if (APlayerController* PC = Cast<APlayerController>(Carrier->GetController()))
        {
            DefaultControllerPitch = PC->GetControlRotation().Pitch;
            PC->SetControlRotation(FRotator(PlacementCameraPitch, PC->GetControlRotation().Yaw, 0.f));
        }
    }
}

void APickupObject::ConfirmPlacement()
{
    if (PickupState != EPickupState::Placing) return;
    PlacementIndicator->SetHiddenInGame(true);

    if (IsValid(Carrier))
        if (APlayerController* PC = Cast<APlayerController>(Carrier->GetController()))
            PC->SetControlRotation(FRotator(DefaultControllerPitch, PC->GetControlRotation().Yaw, 0.f));

    bIsCarried = false;
    FinalizeDrop();
    TrySnapToNearbySocket();
}

void APickupObject::CancelPlacement()
{
    if (PickupState != EPickupState::Placing) return;
    if (bPlacementJustStarted) return;

    if (bKeepBlockAtPlacementLocationOnCancel)
    {
        ConfirmPlacement();
        return;
    }

    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    PlacementIndicator->SetHiddenInGame(true);
    PlacementRotationInput = 0.f;
    PlacementVerticalInput = 0.f;
    PlacementHeightAdjust = 0.f;
    CurrentCarryDistance = CarryForwardDistance;
    TargetCarryDistance = CarryForwardDistance;
    FloatTime = 0.f;

    LerpStartPosition = GetActorLocation();
    LerpAlpha = 0.f;
    PickupState = EPickupState::LerpingToHold;

    if (IsValid(Carrier))
    {
        Carrier->bUseControllerRotationYaw = true;
        Carrier->JumpMaxCount = DefaultJumpMaxCount;

        if (APlayerController* PC = Cast<APlayerController>(Carrier->GetController()))
            PC->SetControlRotation(FRotator(DefaultControllerPitch, PC->GetControlRotation().Yaw, 0.f));
    }
}

void APickupObject::ThrowBlock()
{
    if (PickupState != EPickupState::Held) return;

    PlacementIndicator->SetHiddenInGame(true);

    FVector ThrowVelocity = GetActorForwardVector() * ThrowSpeed + FVector(0.f, 0.f, ThrowArcZ);
    if (IsValid(Carrier))
    {
        if (APlayerController* PC = Cast<APlayerController>(Carrier->GetController()))
        {
            FVector CamLoc; FRotator CamRot;
            PC->GetPlayerViewPoint(CamLoc, CamRot);
            ThrowVelocity = CamRot.Vector() * ThrowSpeed + FVector(0.f, 0.f, ThrowArcZ);
        }
    }

    bIsCarried = false;
    FinalizeDrop();

    bThrowing = true;
    Mesh->SetPhysicsLinearVelocity(ThrowVelocity);

    GetWorldTimerManager().SetTimer(SafetyDespawnTimer, this, &APickupObject::DespawnAndRespawn, SafetyDespawnDelay, false);
}

void APickupObject::SetPlacementRotationInput(float Input)
{
    if (PickupState != EPickupState::Placing) return;
    PlacementRotationInput = FMath::Clamp(Input, -1.f, 1.f);
}

void APickupObject::SetPlacementVerticalInput(float Input)
{
    if (PickupState != EPickupState::Placing) return;
    PlacementVerticalInput = FMath::Clamp(Input, -1.f, 1.f);
}

FVector APickupObject::GetHoldPosition() const
{
    AController* Controller = Carrier->GetController();
    const FRotator ControlRot = Controller ? Controller->GetControlRotation() : Carrier->GetActorRotation();
    const FVector Forward = FRotationMatrix(FRotator(0.f, ControlRot.Yaw, 0.f)).GetUnitAxis(EAxis::X);
    return Carrier->GetActorLocation()
        + Forward * CurrentCarryDistance
        + FVector(0.f, 0.f, CarryHeightOffset);
}

void APickupObject::AdjustCarryDistance(float Delta)
{
    if (PickupState != EPickupState::Placing) return;
    TargetCarryDistance = FMath::Clamp(TargetCarryDistance + Delta, MinCarryDistance, MaxCarryDistance);
}

float APickupObject::GetPlacementStartHeight_Implementation() const
{
    if (bPlacementStartHeightFromCapsule && IsValid(Carrier))
    {
        if (UCapsuleComponent* Capsule = Carrier->GetCapsuleComponent())
            return Capsule->GetScaledCapsuleHalfHeight() * 2.f * PlacementStartHeightCapsuleMultiplier;
    }
    return PlacementStartHeight;
}

FVector APickupObject::GetDropPosition() const
{
    return FVector(GetActorLocation().X, GetActorLocation().Y,
        Carrier->GetActorLocation().Z + DropReleaseHeight);
}

void APickupObject::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    bPlacementJustStarted = false;

    // Runs independently of carry state so the arm can lerp back after the block is dropped
    if (CachedSpringArm)
    {
        const bool bIsPlacing = (PickupState == EPickupState::Placing);
        CachedSpringArm->TargetArmLength = FMath::FInterpTo(
            CachedSpringArm->TargetArmLength,
            bIsPlacing ? PlacementCameraArmLength : DefaultArmLength,
            DeltaTime, CameraLerpSpeed);
        CachedSpringArm->SocketOffset = FMath::VInterpTo(
            CachedSpringArm->SocketOffset,
            bIsPlacing ? PlacementCameraSocketOffset : DefaultSocketOffset,
            DeltaTime, CameraLerpSpeed);

        if (PickupState == EPickupState::Idle
            && FMath::IsNearlyEqual(CachedSpringArm->TargetArmLength, DefaultArmLength, 1.f)
            && CachedSpringArm->SocketOffset.Equals(DefaultSocketOffset, 1.f))
        {
            CachedSpringArm->TargetArmLength = DefaultArmLength;
            CachedSpringArm->SocketOffset = DefaultSocketOffset;
            CachedSpringArm = nullptr;
            if (!IsValid(StackedBlock) && !bSnappingStack)
                SetActorTickEnabled(false);
        }
    }

    // --- Stacking ---
    // Pending: block was carried into the detection sphere — wait until fully settled (Idle)
    if (IsValid(PendingStack) && !bIsCarried
        && PendingStack->PickupState == EPickupState::Idle
        && !PendingStack->bIsCarried)
    {
        InitiateStack(PendingStack);
        PendingStack = nullptr;
    }

    if (bSnappingStack && IsValid(StackedBlock))
    {
        const FVector Target = TopSocket->GetComponentLocation();
        const FVector NewLoc = FMath::VInterpTo(StackedBlock->GetActorLocation(), Target, DeltaTime, StackSnapSpeed);
        StackedBlock->SetActorLocation(NewLoc);

        if (FVector::DistSquared(NewLoc, Target) < 4.f)
        {
            StackedBlock->SetActorLocation(Target);
            bSnappingStack = false;
            OnBlockStacked();
        }
    }
    else if (IsValid(StackedBlock))
    {
        if (StackedBlock->bIsCarried)
        {
            // Player picked the top block off — unfreeze this block
            StackedBlock->bIsStacked = false;
            StackedBlock = nullptr;
            if (!bIsStacked)
            {
                Mesh->SetSimulatePhysics(true);
                Mesh->SetEnableGravity(true);
            }
            OnBlockUnstacked();
        }
        else if (bIsCarried)
        {
            DislodgeStack();
        }
        else
        {
            // Pinned — lock location only, never force rotation (causes upside-down flips)
            StackedBlock->SetActorLocation(TopSocket->GetComponentLocation());
        }
    }
    // ----------------

    if (!IsValid(Carrier)) return;

    switch (PickupState)
    {
    case EPickupState::LerpingToHold:
    {
        LerpAlpha = FMath::Min(LerpAlpha + DeltaTime / PickupLerpDuration, 1.f);
        const float Curved = PickupCurve
            ? PickupCurve->GetFloatValue(LerpAlpha)
            : FMath::SmoothStep(0.f, 1.f, LerpAlpha);
        SetActorLocation(FMath::Lerp(LerpStartPosition, GetHoldPosition(), Curved), true);
        if (LerpAlpha >= 1.f)
            PickupState = EPickupState::Held;
        break;
    }
    case EPickupState::Held:
    {
        CurrentCarryDistance = FMath::FInterpTo(CurrentCarryDistance, TargetCarryDistance, DeltaTime, CarryDistanceInterpSpeed);
        FloatTime += DeltaTime;
        const float FloatOffset = FMath::Sin(FloatTime * FloatSpeed * PI) * FloatAmplitude;
        SetActorLocation(GetHoldPosition() + FVector(0.f, 0.f, FloatOffset), true);
        if (CarryRotationSpeed != 0.f)
        {
            FRotator Rot = GetActorRotation();
            Rot.Yaw += CarryRotationSpeed * DeltaTime;
            SetActorRotation(Rot);
        }
        break;
    }
    case EPickupState::Placing:
    {
        CurrentCarryDistance = FMath::FInterpTo(CurrentCarryDistance, TargetCarryDistance, DeltaTime, CarryDistanceInterpSpeed);

        APlayerController* PC = Cast<APlayerController>(Carrier->GetController());
        if (IsValid(PC))
        {
            FVector CamLoc;
            FRotator CamRot;
            PC->GetPlayerViewPoint(CamLoc, CamRot);

            // Camera trace for XY target only — mouse movement never affects Z
            const FVector CamTraceEnd = CamLoc + CamRot.Vector() * 5000.f;

            FCollisionQueryParams Params;
            Params.AddIgnoredActor(this);
            Params.AddIgnoredActor(Carrier);

            FHitResult Hit;
            FVector2D TargetXY;
            if (GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, CamTraceEnd, ECC_Visibility, Params))
            {
                // Pull back along the hit normal by the block's horizontal radius so it doesn't clip into walls.
                // Floor normals are ~vertical, so this leaves floor placement XY unchanged.
                const float HorizontalRadius = FMath::Max(Mesh->Bounds.BoxExtent.X, Mesh->Bounds.BoxExtent.Y);
                const FVector Adjusted = Hit.ImpactPoint + Hit.ImpactNormal * HorizontalRadius;
                TargetXY = FVector2D(Adjusted.X, Adjusted.Y);
            }
            else
                TargetXY = FVector2D(CamTraceEnd.X, CamTraceEnd.Y);

            // Clamp XY distance from player (scroll wheel controls this)
            const FVector PlayerPos = Carrier->GetActorLocation();
            const FVector2D PlayerXY(PlayerPos.X, PlayerPos.Y);
            const FVector2D DeltaXY = TargetXY - PlayerXY;
            if (DeltaXY.Size() > CurrentCarryDistance)
                TargetXY = PlayerXY + DeltaXY.GetSafeNormal() * CurrentCarryDistance;

            // Downward trace at target XY to find the surface below.
            // Ignore other pickups so SurfaceZ doesn't flip discretely between a neighbor block's
            // top and the floor as the target crosses its edge — that caused jittery placement.
            FCollisionQueryParams DownParams = Params;
            TArray<AActor*> OtherPickups;
            UGameplayStatics::GetAllActorsOfClass(this, APickupObject::StaticClass(), OtherPickups);
            DownParams.AddIgnoredActors(OtherPickups);

            const float HalfHeight = Mesh->Bounds.BoxExtent.Z;
            const FVector DownStart(TargetXY.X, TargetXY.Y, PlayerPos.Z + 5000.f);
            const FVector DownEnd  (TargetXY.X, TargetXY.Y, PlayerPos.Z - 5000.f);
            FHitResult DownHit;
            float SurfaceZ = PlayerPos.Z;
            if (GetWorld()->LineTraceSingleByChannel(DownHit, DownStart, DownEnd, ECC_Visibility, DownParams))
                SurfaceZ = DownHit.ImpactPoint.Z + HalfHeight + 2.f;

            // Space/C accumulate height above the surface — never below it
            if (PlacementVerticalInput != 0.f)
            {
                PlacementHeightAdjust += PlacementVerticalInput * PlacementHeightSpeed * DeltaTime;
                PlacementHeightAdjust = FMath::Clamp(PlacementHeightAdjust, 0.f, MaxCarryDistance);
            }

            FloatTime += DeltaTime;
            const float FloatOffset = FMath::Sin(FloatTime * FloatSpeed * PI) * FloatAmplitude;
            const FVector TargetLocation(TargetXY.X, TargetXY.Y, SurfaceZ + PlacementHeightAdjust + FloatOffset);
            const FVector NewLocation = PlacementPositionInterpSpeed > 0.f
                ? FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaTime, PlacementPositionInterpSpeed)
                : TargetLocation;
            SetActorLocation(NewLocation);
        }

        if (PlacementRotationInput != 0.f)
        {
            FRotator Rot = GetActorRotation();
            Rot.Yaw += PlacementRotationInput * PlacementRotationSpeed * DeltaTime;
            SetActorRotation(Rot);
        }
        break;
    }
    case EPickupState::LerpingToDrop:
    {
        LerpAlpha = FMath::Min(LerpAlpha + DeltaTime / DropLerpDuration, 1.f);
        const float Curved = DropCurve
            ? DropCurve->GetFloatValue(LerpAlpha)
            : FMath::SmoothStep(0.f, 1.f, LerpAlpha);
        SetActorLocation(FMath::Lerp(LerpStartPosition, GetDropPosition(), Curved));
        if (LerpAlpha >= 1.f)
            FinalizeDrop();
        break;
    }
    default: break;
    }

    // Keep beam endpoints updated while carried
    if (PickupState != EPickupState::Idle && IsValid(BeamEffect) && BeamEffect->IsActive())
    {
        BeamEffect->SetVariableVec3(BeamStartParamName, Carrier->GetActorLocation());
        BeamEffect->SetVariableVec3(BeamEndParamName, GetActorLocation());
    }
}

void APickupObject::TrySnapToNearbySocket()
{
    TArray<FOverlapResult> Overlaps;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(PlacementSnapRadius);
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity,
        FCollisionObjectQueryParams(ECC_WorldDynamic), Sphere, Params);

    APickupObject* Best = nullptr;
    float BestDistSq = TNumericLimits<float>::Max();

    for (const FOverlapResult& Overlap : Overlaps)
    {
        APickupObject* Candidate = Cast<APickupObject>(Overlap.GetActor());
        if (!IsValid(Candidate) || Candidate == this) continue;
        if (Candidate->bIsCarried || Candidate->PickupState != EPickupState::Idle) continue;
        if (GetActorLocation().Z < Candidate->GetActorLocation().Z) continue;

        const float Radius = Candidate->StackDetection->GetScaledSphereRadius();
        const float DistSq = FVector::DistSquared(GetActorLocation(), Candidate->TopSocket->GetComponentLocation());
        if (DistSq > Radius * Radius) continue;

        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            Best = Candidate;
        }
    }

    if (Best)
    {
        Best->InitiateStack(this);
    }
}

void APickupObject::InitiateStack(APickupObject* Block)
{
    if (!IsValid(Block) || IsValid(StackedBlock) || bIsCarried || bIsStacked) return;
    if (Block->bIsCarried || Block == this || Block->bIsStacked) return;
    if (Block->PickupState != EPickupState::Idle) { PendingStack = Block; return; }

    // Distance check — prevents stale PendingStack from snapping across a distance
    const float Radius = StackDetection->GetScaledSphereRadius();
    if (FVector::DistSquared(Block->GetActorLocation(), StackDetection->GetComponentLocation()) > Radius * Radius) return;

    StackedBlock = Block;
    StackedBlock->bIsStacked = true;
    bSnappingStack = true;

    StackedBlock->Mesh->SetSimulatePhysics(false);
    StackedBlock->Mesh->SetEnableGravity(false);

    // Lock the bottom block in place while the stack is on top
    Mesh->SetSimulatePhysics(false);
    Mesh->SetEnableGravity(false);

    SetActorTickEnabled(true);
}

void APickupObject::DislodgeStack()
{
    if (!IsValid(StackedBlock)) return;

    StackedBlock->Mesh->SetSimulatePhysics(true);
    StackedBlock->Mesh->SetEnableGravity(true);
    StackedBlock->bIsStacked = false;
    StackedBlock = nullptr;
    bSnappingStack = false;

    // Unfreeze this block — only if it isn't itself part of a higher stack
    if (!bIsStacked)
    {
        Mesh->SetSimulatePhysics(true);
        Mesh->SetEnableGravity(true);
    }

    OnBlockUnstacked();
}

void APickupObject::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp,
    bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
    Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

    if (IsValid(StackedBlock) && NormalImpulse.Size() > DislodgeImpulse)
        DislodgeStack();

    if (bThrowing)
    {
        bThrowing = false;
        GetWorldTimerManager().ClearTimer(SafetyDespawnTimer);
        GetWorldTimerManager().SetTimer(HitDespawnTimer, this, &APickupObject::DespawnAndRespawn, HitDespawnDelay, false);

        TArray<FOverlapResult> Overlaps;
        FCollisionShape Sphere = FCollisionShape::MakeSphere(ThrowImpactRadius);
        GetWorld()->OverlapMultiByObjectType(Overlaps, HitLocation, FQuat::Identity,
            FCollisionObjectQueryParams(ECC_WorldDynamic), Sphere);
        for (const FOverlapResult& Overlap : Overlaps)
        {
            APickupObject* Block = Cast<APickupObject>(Overlap.GetActor());
            if (IsValid(Block) && Block != this && IsValid(Block->StackedBlock))
                Block->DislodgeStack();
        }
    }
}

void APickupObject::DespawnAndRespawn()
{
    GetWorldTimerManager().ClearTimer(HitDespawnTimer);
    GetWorldTimerManager().ClearTimer(SafetyDespawnTimer);

    // Block was re-picked up before the timer fired — leave it alone
    if (bIsCarried || PickupState != EPickupState::Idle) return;

    const FVector SpawnPos = PickupLocation + FVector(0.f, 0.f, RespawnHeight);
    SetActorLocation(SpawnPos);
    SetActorRotation(FRotator::ZeroRotator);

    Mesh->SetSimulatePhysics(true);
    Mesh->SetEnableGravity(true);
    Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
    Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

    if (RespawnEffect)
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, RespawnEffect, SpawnPos);

    OnRespawn();
}

void APickupObject::OnStackSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // bIsStacked guard prevents the bottom block from being pulled up by the block sitting on top of it
    if (bIsCarried || bIsStacked || IsValid(StackedBlock) || bSnappingStack) return;

    APickupObject* Block = Cast<APickupObject>(OtherActor);
    if (!IsValid(Block) || Block == this || Block->bIsStacked) return;

    // Only pull in blocks that are ABOVE us — prevents the bottom block from being lifted up by the top block's sphere
    if (Block->GetActorLocation().Z <= GetActorLocation().Z) return;

    // Store as pending whether carried or still settling — InitiateStack checks Idle state
    if (Block->bIsCarried || Block->PickupState != EPickupState::Idle)
        PendingStack = Block;
    else
        InitiateStack(Block);
}

void APickupObject::OnStackSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (OtherActor == PendingStack)
        PendingStack = nullptr;
}
