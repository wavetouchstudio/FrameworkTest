#include "DoorHinged.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

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
}

void ADoorHinged::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    CurrentAngle = FMath::FInterpTo(CurrentAngle, TargetAngle, DeltaTime, OpenSpeed);
    ApplyMeshRotation();
}

void ADoorHinged::ApplyMeshRotation()
{
    FRotator NewRot = ClosedRot;
    if (HingeAxis == EHingeAxis::Yaw) NewRot.Yaw   += CurrentAngle;
    else                              NewRot.Pitch  += CurrentAngle;
    HingeArm->SetRelativeRotation(NewRot);
}

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

void ADoorHinged::ToggleDoor()
{
    if (bIsLocked || bFreeSwing) return;
    bIsOpen ? CloseDoor() : OpenDoor();
}

void ADoorHinged::OpenDoor()
{
    if (bIsLocked || bIsOpen || bFreeSwing) return;
    bIsOpen = true;

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

void ADoorHinged::CloseDoor()
{
    if (!bIsOpen || bFreeSwing) return;
    bIsOpen = false;
    TargetAngle = 0.f;
    OnClosed();
}

void ADoorHinged::LockDoor()
{
    bIsLocked = true;
    OnLocked();
}

void ADoorHinged::UnlockDoor()
{
    bIsLocked = false;
    OnUnlocked();
}
