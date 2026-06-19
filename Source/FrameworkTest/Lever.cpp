#include "Lever.h"

#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "LiftManager.h"
#include "PuzzleTrigger.h"
#include "DoorHinged.h"
#include "DoorSliding.h"
#include "Drawbridge.h"

// Constructor for ALever class
// Initializes lever with scene components and audio
ALever::ALever()
{
    PrimaryActorTick.bCanEverTick = false;

    USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    CalibrationPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CalibrationPoint"));
    CalibrationPoint->SetupAttachment(RootComponent);

    LeverMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeverMesh"));
    LeverMesh->SetupAttachment(CalibrationPoint);

    Audio = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio"));
    Audio->SetupAttachment(RootComponent);
    Audio->bAutoActivate = false;
}

// Called when the game starts or when spawned
// Sets up initial lever state
void ALever::BeginPlay()
{
    Super::BeginPlay();
    bIsLocked = bStartLocked;
}

// Activates the lever if not locked
// Triggers connected mechanisms like lifts, doors, etc.
void ALever::ActivateLever()
{
    if (bIsLocked || bHasBeenActivated) return;

    // If a lift is assigned its state must allow activation before anything fires
    if (IsValid(LiftManager))
    {
        const ELiftState State = LiftManager->LiftState;
        if (bIsBottomLever && State != ELiftState::Idle_Top)    return;
        if (!bIsBottomLever && State != ELiftState::Idle_Bottom) return;
    }

    bHasBeenActivated = true;

    if (IsValid(LiftManager))
    {
        LiftManager->LastActivatingLever = this;
        if (bIsBottomLever) LiftManager->CallDown();
        else LiftManager->CallUp();
    }

    if (IsValid(LinkedDoor))
    {
        if (ADoorHinged* DH = Cast<ADoorHinged>(LinkedDoor))          { DH->OpenDoor(); }
        else if (ADoorSliding* DS = Cast<ADoorSliding>(LinkedDoor))  { DS->OpenDoor(); }
        else if (ADrawbridge* DB = Cast<ADrawbridge>(LinkedDoor))    { DB->Open(); }
    }

    if (IsValid(LinkedTrigger)) LinkedTrigger->ActivateByLever();

    OnActivated();
}

void ALever::ResetLever()
{
    bHasBeenActivated = false;

    if (IsValid(LinkedDoor))
    {
        if (ADoorHinged* DH = Cast<ADoorHinged>(LinkedDoor))          { DH->CloseDoor(); }
        else if (ADoorSliding* DS = Cast<ADoorSliding>(LinkedDoor))  { DS->CloseDoor(); }
        else if (ADrawbridge* DB = Cast<ADrawbridge>(LinkedDoor))    { DB->Close(); }
    }

    if (IsValid(LinkedTrigger)) LinkedTrigger->DeactivateByLever();

    OnReset();
}

void ALever::LockLever()
{
    bIsLocked = true;
    OnLocked();
}

void ALever::UnlockLever()
{
    bIsLocked = false;
    OnUnlocked();
}
