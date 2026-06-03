#include "Drawbridge.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"

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
}

void ADrawbridge::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    CurrentAngle = FMath::FInterpTo(CurrentAngle, TargetAngle, DeltaTime, OpenSpeed);
    FRotator NewRot = ClosedRot;
    NewRot.Pitch += CurrentAngle;
    Arm->SetRelativeRotation(NewRot);
}

void ADrawbridge::Open()
{
    if (bIsOpen) return;
    bIsOpen = true;
    TargetAngle = OpenAngle;
    OnOpened();
}

void ADrawbridge::Close()
{
    if (!bIsOpen) return;
    bIsOpen = false;
    TargetAngle = 0.f;
    OnClosed();
}

void ADrawbridge::Toggle()
{
    bIsOpen ? Close() : Open();
}
