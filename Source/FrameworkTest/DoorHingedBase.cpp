#include "DoorHingedBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

void ADoorHingedBase::BeginPlay()
{
    Super::BeginPlay();
    bIsLocked = bStartLocked;
}

void ADoorHingedBase::ToggleDoor()
{
    if (bIsLocked || !IsInteractable()) return;
    bIsOpen ? CloseDoor() : OpenDoor();
}

void ADoorHingedBase::OpenDoor()
{
    if (bIsLocked || bIsOpen || !IsInteractable()) return;
    bIsOpen = true;
    StartOpenAnimation();
    OnOpened();
}

void ADoorHingedBase::CloseDoor()
{
    if (!bIsOpen || !IsInteractable()) return;
    bIsOpen = false;
    StartCloseAnimation();
    OnClosed();
}

void ADoorHingedBase::LockDoor()
{
    bIsLocked = true;
    OnLocked();
}

void ADoorHingedBase::UnlockDoor()
{
    bIsLocked = false;
    OnUnlocked();
}

// Flips OpenAngle's sign so the door swings away from whichever side the player stands on
float ADoorHingedBase::ComputeOpenAngle(const FVector& PivotLocation, const FVector& PivotRight) const
{
    if (!bRotateAwayFromPlayer || HingeAxis != EHingeAxis::Yaw) return OpenAngle;

    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    APawn* Pawn = PC ? PC->GetPawn() : nullptr;
    if (!Pawn) return OpenAngle;

    const float Angle = FMath::Abs(OpenAngle);
    const FVector ToPawn = Pawn->GetActorLocation() - PivotLocation;
    const float Dot = FVector::DotProduct(ToPawn, PivotRight);
    return (Dot >= 0.f) ? -Angle : Angle;
}
