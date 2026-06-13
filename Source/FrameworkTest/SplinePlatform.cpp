#include "SplinePlatform.h"

#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"

ASplinePlatform::ASplinePlatform()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    SplinePath = CreateDefaultSubobject<USplineComponent>(TEXT("SplinePath"));
    SplinePath->SetupAttachment(SceneRoot);

    PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
    PlatformMesh->SetupAttachment(SceneRoot);
    PlatformMesh->SetMobility(EComponentMobility::Movable);
}

void ASplinePlatform::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bRotationEnabled)
    {
        FVector Axis = FVector::UpVector;
        switch (RotationAxis)
        {
            case EHazardRotationAxis::X: Axis = FVector::ForwardVector; break;
            case EHazardRotationAxis::Y: Axis = FVector::RightVector; break;
            case EHazardRotationAxis::Z: Axis = FVector::UpVector; break;
        }

        PlatformMesh->AddLocalRotation(FQuat(Axis, FMath::DegreesToRadians(RotationSpeed * DeltaTime)));
    }

    if (bMovementEnabled && SplinePath->GetNumberOfSplinePoints() >= 2)
    {
        const float SplineLen = SplinePath->GetSplineLength();
        DistanceAlongSpline += MovementSpeed * DeltaTime * MovementDir;

        if (MovementMode == ESplineMovementMode::Loop)
        {
            DistanceAlongSpline = FMath::Fmod(DistanceAlongSpline + SplineLen, SplineLen);
        }
        else // PingPong
        {
            if (DistanceAlongSpline >= SplineLen)
            {
                DistanceAlongSpline = SplineLen;
                MovementDir = -1;
            }
            else if (DistanceAlongSpline <= 0.f)
            {
                DistanceAlongSpline = 0.f;
                MovementDir = 1;
            }
        }

        PlatformMesh->SetRelativeLocation(SplinePath->GetLocationAtDistanceAlongSpline(DistanceAlongSpline, ESplineCoordinateSpace::Local));
    }
}
