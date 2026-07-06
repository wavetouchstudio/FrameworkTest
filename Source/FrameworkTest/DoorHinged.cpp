#include "DoorHinged.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

// Constructor for ADoorHinged class
// Initializes hinged door with pivot, frame, arm, mesh, and constraint components
ADoorHinged::ADoorHinged()
{
    PrimaryActorTick.bCanEverTick = true;

    HingePivot = CreateDefaultSubobject<USceneComponent>(TEXT("HingePivot"));
    SetRootComponent(HingePivot);

    FrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameMesh"));
    FrameMesh->SetupAttachment(HingePivot);

    HingeArm = CreateDefaultSubobject<USceneComponent>(TEXT("HingeArm"));
    HingeArm->SetupAttachment(HingePivot);

    DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
    DoorMesh->SetupAttachment(HingeArm);

    HingeConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("HingeConstraint"));
    HingeConstraint->SetupAttachment(HingePivot);
}

// Called when the game starts or when spawned
// Sets up initial door state and physics
void ADoorHinged::BeginPlay()
{
    Super::BeginPlay();

    bIsLocked = bStartLocked;
    ClosedRot = HingeArm->GetRelativeRotation();

    if (bFreeSwing)
    {
        SetupFreeSwing();
        return;
    }

    bIsOpen = bStartOpen;
    CurrentAngle = bStartOpen ? OpenAngle : 0.f;
    TargetAngle = CurrentAngle;
    ApplyMeshRotation();
    SetActorTickEnabled(false);
}

// Called every frame
// Handles door opening/closing animation
void ADoorHinged::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    CurrentAngle = FMath::FInterpTo(CurrentAngle, TargetAngle, DeltaTime, OpenSpeed);
    ApplyMeshRotation();

    if (FMath::IsNearlyEqual(CurrentAngle, TargetAngle, 0.01f))
    {
        CurrentAngle = TargetAngle;
        ApplyMeshRotation();
        SetActorTickEnabled(false);
    }
}

// Applies current rotation to the door mesh
void ADoorHinged::ApplyMeshRotation()
{
    FRotator NewRot = ClosedRot;
    if (HingeAxis == EHingeAxis::Yaw) NewRot.Yaw   += CurrentAngle;
    else                              NewRot.Pitch  += CurrentAngle;
    HingeArm->SetRelativeRotation(NewRot);
}

// Sets up physics-based free swinging door
void ADoorHinged::SetupFreeSwing()
{
    SetActorTickEnabled(false);

    if (HingeAxis == EHingeAxis::Yaw)
        HingeConstraint->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
    else
        HingeConstraint->SetRelativeRotation(FRotator(0.f, 0.f, -90.f));

    DoorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    DoorMesh->SetSimulatePhysics(true);

    HingeConstraint->SetConstrainedComponents(nullptr, NAME_None, DoorMesh, NAME_None);
    HingeConstraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
    HingeConstraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
    HingeConstraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.f);

    EAngularConstraintMotion TwistMotion = bLimitSwingAngle
        ? EAngularConstraintMotion::ACM_Limited
        : EAngularConstraintMotion::ACM_Free;

    HingeConstraint->SetAngularTwistLimit(TwistMotion, bLimitSwingAngle ? MaxSwingAngle : 0.f);
    HingeConstraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.f);
    HingeConstraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.f);
}

// Toggles door between open and closed states
void ADoorHinged::ToggleDoor()
{
    if (bIsLocked || bFreeSwing) return;
    bIsOpen ? CloseDoor() : OpenDoor();
}

// Opens the door with animation
void ADoorHinged::OpenDoor()
{
    if (bIsLocked || bIsOpen || bFreeSwing) return;
    bIsOpen = true;
    SetActorTickEnabled(true);

    if (bRotateAwayFromPlayer && HingeAxis == EHingeAxis::Yaw)
    {
        float Angle = FMath::Abs(OpenAngle);
        APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
        APawn* Pawn = PC ? PC->GetPawn() : nullptr;
        if (Pawn)
        {
            FVector ToPawn = Pawn->GetActorLocation() - HingePivot->GetComponentLocation();
            float Dot = FVector::DotProduct(ToPawn, HingePivot->GetRightVector());
            TargetAngle = (Dot >= 0.f) ? -Angle : Angle;
        }
        else
        {
            TargetAngle = OpenAngle;
        }
    }
    else
    {
        TargetAngle = OpenAngle;
    }

    OnOpened();
}

// Closes the door with animation
void ADoorHinged::CloseDoor()
{
    if (!bIsOpen || bFreeSwing) return;
    bIsOpen = false;
    TargetAngle = 0.f;
    SetActorTickEnabled(true);
    OnClosed();
}

// Locks the door to prevent opening
void ADoorHinged::LockDoor()
{
    bIsLocked = true;
    OnLocked();
}

// Unlocks the door to allow opening
void ADoorHinged::UnlockDoor()
{
    bIsLocked = false;
    OnUnlocked();
}
