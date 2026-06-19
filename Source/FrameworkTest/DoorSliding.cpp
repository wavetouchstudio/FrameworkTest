#include "DoorSliding.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"

// Constructor for ADoorSliding class
// Initializes sliding door with root, frame, and door mesh components
ADoorSliding::ADoorSliding()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    FrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameMesh"));
    FrameMesh->SetupAttachment(Root);

    DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
    DoorMesh->SetupAttachment(Root);

    DoorMeshB = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMeshB"));
    DoorMeshB->SetupAttachment(Root);
    DoorMeshB->SetVisibility(false);
}

// Called when the game starts or when spawned
// Sets up initial door state and positions
void ADoorSliding::BeginPlay()
{
    Super::BeginPlay();

    bIsLocked = bStartLocked;
    DoorMeshB->SetVisibility(bDoubleDoor);

    // Snapshot closed positions so slide offsets are relative to wherever the designer placed the meshes
    ClosedPosA = DoorMesh->GetRelativeLocation();
    ClosedPosB = DoorMeshB->GetRelativeLocation();

    if (bStartOpen)
    {
        FVector Offset = SlideDirection.GetSafeNormal() * SlideDistance;
        bIsOpen = true;
        CurrentPosA = ClosedPosA + Offset;
        CurrentPosB = ClosedPosB - Offset;
        DoorMesh->SetRelativeLocation(CurrentPosA);
        if (bDoubleDoor) DoorMeshB->SetRelativeLocation(CurrentPosB);
    }
    else
    {
        CurrentPosA = ClosedPosA;
        CurrentPosB = ClosedPosB;
    }
}

// Called every frame
// Handles door sliding animation and interpolation
void ADoorSliding::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    FVector Offset = SlideDirection.GetSafeNormal() * SlideDistance;
    FVector TargetA = bIsOpen ? ClosedPosA + Offset : ClosedPosA;

    CurrentPosA = FMath::VInterpTo(CurrentPosA, TargetA, DeltaTime, OpenSpeed);
    DoorMesh->SetRelativeLocation(CurrentPosA);

    if (bDoubleDoor && DoorMeshB)
    {
        FVector TargetB = bIsOpen ? ClosedPosB - Offset : ClosedPosB;
        CurrentPosB = FMath::VInterpTo(CurrentPosB, TargetB, DeltaTime, OpenSpeed);
        DoorMeshB->SetRelativeLocation(CurrentPosB);
    }
}

// Toggles door between open and closed states
// Only works if door is not locked
void ADoorSliding::ToggleDoor()
{
    if (bIsLocked) return;
    bIsOpen ? CloseDoor() : OpenDoor();
}

// Opens the door if not locked or already open
void ADoorSliding::OpenDoor()
{
    if (bIsLocked || bIsOpen) return;
    bIsOpen = true;
    OnOpened();
}

// Closes the door if it's currently open
void ADoorSliding::CloseDoor()
{
    if (!bIsOpen) return;
    bIsOpen = false;
    OnClosed();
}

// Locks the door to prevent opening
void ADoorSliding::LockDoor()
{
    bIsLocked = true;
    OnLocked();
}

// Unlocks the door to allow opening
void ADoorSliding::UnlockDoor()
{
    bIsLocked = false;
    OnUnlocked();
}
