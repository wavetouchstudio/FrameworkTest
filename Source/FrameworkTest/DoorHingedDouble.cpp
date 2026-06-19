#include "DoorHingedDouble.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Curves/CurveFloat.h"

// Constructor for ADoorHingedDouble class
// Initializes double hinged door with root, frame, pivots, and door meshes
ADoorHingedDouble::ADoorHingedDouble()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    FrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameMesh"));
    FrameMesh->SetupAttachment(Root);

    HingePivotA = CreateDefaultSubobject<USceneComponent>(TEXT("HingePivotA"));
    HingePivotA->SetupAttachment(Root);

    DoorMeshA = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMeshA"));
    DoorMeshA->SetupAttachment(HingePivotA);

    HingePivotB = CreateDefaultSubobject<USceneComponent>(TEXT("HingePivotB"));
    HingePivotB->SetupAttachment(Root);

    DoorMeshB = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMeshB"));
    DoorMeshB->SetupAttachment(HingePivotB);
}

// Called when the game starts or when spawned
// Sets up initial door rotation and state
void ADoorHingedDouble::BeginPlay()
{
    Super::BeginPlay();

    bIsLocked = bStartLocked;
    ClosedRotA = HingePivotA->GetRelativeRotation();
    ClosedRotB = HingePivotB->GetRelativeRotation();

    bIsOpen = bStartOpen;
    CurrentAngle = bStartOpen ? OpenAngle : 0.f;
    TargetAngle = CurrentAngle;
    ApplyMeshRotation();
}

// Called every frame
// Handles door opening/closing animation
void ADoorHingedDouble::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!FMath::IsNearlyEqual(CurrentAngle, TargetAngle, 0.01f))
    {
        LerpAlpha = FMath::Min(LerpAlpha + DeltaTime / OpenDuration, 1.f);
        const float Sample = OpenCurve ? OpenCurve->GetFloatValue(LerpAlpha) : LerpAlpha;
        CurrentAngle = FMath::Lerp(SourceAngle, TargetAngle, Sample);
        ApplyMeshRotation();
    }
}

// Applies current rotation to both door meshes
void ADoorHingedDouble::ApplyMeshRotation()
{
    FRotator NewRotA = ClosedRotA;
    FRotator NewRotB = ClosedRotB;

    if (HingeAxis == EHingeAxis::Yaw)
    {
        NewRotA.Yaw += CurrentAngle;
        NewRotB.Yaw -= CurrentAngle;
    }
    else
    {
        NewRotA.Pitch += CurrentAngle;
        NewRotB.Pitch -= CurrentAngle;
    }

    HingePivotA->SetRelativeRotation(NewRotA);
    HingePivotB->SetRelativeRotation(NewRotB);
}

// Toggles door between open and closed states
void ADoorHingedDouble::ToggleDoor()
{
    if (bIsLocked) return;
    bIsOpen ? CloseDoor() : OpenDoor();
}

// Opens the door with animation
void ADoorHingedDouble::OpenDoor()
{
    if (bIsLocked || bIsOpen) return;
    bIsOpen = true;

    if (bRotateAwayFromPlayer && HingeAxis == EHingeAxis::Yaw)
    {
        float Angle = FMath::Abs(OpenAngle);
        APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
        APawn* Pawn = PC ? PC->GetPawn() : nullptr;
        if (Pawn)
        {
            FVector ToPawn = Pawn->GetActorLocation() - HingePivotA->GetComponentLocation();
            float Dot = FVector::DotProduct(ToPawn, HingePivotA->GetRightVector());
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

    SourceAngle = CurrentAngle;
    LerpAlpha = 0.f;
    OnOpened();
}

// Closes the door with animation
void ADoorHingedDouble::CloseDoor()
{
    if (!bIsOpen) return;
    bIsOpen = false;
    TargetAngle = 0.f;
    SourceAngle = CurrentAngle;
    LerpAlpha = 0.f;
    OnClosed();
}

// Locks the door to prevent opening
void ADoorHingedDouble::LockDoor()
{
    bIsLocked = true;
    OnLocked();
}

// Unlocks the door to allow opening
void ADoorHingedDouble::UnlockDoor()
{
    bIsLocked = false;
    OnUnlocked();
}
