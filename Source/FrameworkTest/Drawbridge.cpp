#include "Drawbridge.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"

// Constructor for ADrawbridge class
// Initializes drawbridge with pivot, arm, and bridge mesh components
ADrawbridge::ADrawbridge()
{
    PrimaryActorTick.bCanEverTick = true;

    HingePivot = CreateDefaultSubobject<USceneComponent>(TEXT("HingePivot"));
    SetRootComponent(HingePivot);

    Arm = CreateDefaultSubobject<USceneComponent>(TEXT("Arm"));
    Arm->SetupAttachment(HingePivot);

    BridgeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BridgeMesh"));
    BridgeMesh->SetupAttachment(Arm);
}

// Called when the game starts or when spawned
// Sets up initial drawbridge rotation
void ADrawbridge::BeginPlay()
{
    Super::BeginPlay();

    ClosedRot = Arm->GetRelativeRotation();
    bIsOpen = bStartOpen;
    CurrentAngle = bStartOpen ? OpenAngle : 0.f;
    TargetAngle = CurrentAngle;

    FRotator StartRot = ClosedRot;
    StartRot.Pitch += CurrentAngle;
    Arm->SetRelativeRotation(StartRot);
    SetActorTickEnabled(false);
}

// Called every frame
// Handles drawbridge opening/closing animation
void ADrawbridge::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    CurrentAngle = FMath::FInterpTo(CurrentAngle, TargetAngle, DeltaTime, OpenSpeed);
    FRotator NewRot = ClosedRot;
    NewRot.Pitch += CurrentAngle;
    Arm->SetRelativeRotation(NewRot);

    if (FMath::IsNearlyEqual(CurrentAngle, TargetAngle, 0.01f))
    {
        CurrentAngle = TargetAngle;
        SetActorTickEnabled(false);
    }
}

// Opens the drawbridge
void ADrawbridge::Open()
{
    if (bIsOpen) return;
    bIsOpen = true;
    TargetAngle = OpenAngle;
    SetActorTickEnabled(true);
    OnOpened();
}

// Closes the drawbridge
void ADrawbridge::Close()
{
    if (!bIsOpen) return;
    bIsOpen = false;
    TargetAngle = 0.f;
    SetActorTickEnabled(true);
    OnClosed();
}

void ADrawbridge::Toggle()
{
    bIsOpen ? Close() : Open();
}
