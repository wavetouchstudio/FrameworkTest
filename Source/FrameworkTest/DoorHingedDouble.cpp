#include "DoorHingedDouble.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
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

    ClosedRotA = HingePivotA->GetRelativeRotation();
    ClosedRotB = HingePivotB->GetRelativeRotation();

    bIsOpen = bStartOpen;
    CurrentAngle = bStartOpen ? OpenAngle : 0.f;
    TargetAngle = CurrentAngle;
    ApplyMeshRotation();
    SetActorTickEnabled(false);
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
    else
    {
        SetActorTickEnabled(false);
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

// Starts the open animation, picking a swing direction away from the player if enabled
void ADoorHingedDouble::StartOpenAnimation()
{
    SetActorTickEnabled(true);
    TargetAngle = ComputeOpenAngle(HingePivotA->GetComponentLocation(), HingePivotA->GetRightVector());
    SourceAngle = CurrentAngle;
    LerpAlpha = 0.f;
}

// Starts the close animation
void ADoorHingedDouble::StartCloseAnimation()
{
    TargetAngle = 0.f;
    SourceAngle = CurrentAngle;
    LerpAlpha = 0.f;
    SetActorTickEnabled(true);
}
