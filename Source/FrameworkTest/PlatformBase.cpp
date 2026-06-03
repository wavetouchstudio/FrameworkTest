#include "PlatformBase.h"

#include "Components/StaticMeshComponent.h"
#include "Curves/CurveFloat.h"

APlatformBase::APlatformBase()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
    RootComponent = PlatformMesh;
}

void APlatformBase::BeginPlay()
{
    Super::BeginPlay();
    StartLoc = GetActorLocation();
    EndLoc = StartLoc + FVector(XTravelDistance, YTravelDistance, ZTravelDistance);
}

void APlatformBase::Activate()
{
    if (PlatformState == EPlatformState::Idle_End) return;

    if (ActivateDelay > 0.f)
    {
        PlatformState = EPlatformState::Delay_ToEnd;
        DelayElapsed = 0.f;
    }
    else
    {
        PlatformState = EPlatformState::Moving_ToEnd;
        TravelElapsed = 0.f;
    }
    SetActorTickEnabled(true);
}

void APlatformBase::Deactivate()
{
    if (PlatformState == EPlatformState::Idle_Start) return;

    if (ActivateDelay > 0.f)
    {
        PlatformState = EPlatformState::Delay_ToStart;
        DelayElapsed = 0.f;
    }
    else
    {
        PlatformState = EPlatformState::Moving_ToStart;
        TravelElapsed = 0.f;
    }
    SetActorTickEnabled(true);
}

void APlatformBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // --- Delay phase ---
    if (PlatformState == EPlatformState::Delay_ToEnd || PlatformState == EPlatformState::Delay_ToStart)
    {
        DelayElapsed += DeltaTime;
        if (DelayElapsed >= ActivateDelay)
        {
            if (PlatformState == EPlatformState::Delay_ToEnd)
            {
                PlatformState = EPlatformState::Moving_ToEnd;
                TravelElapsed = 0.f;
            }
            else
            {
                PlatformState = EPlatformState::Moving_ToStart;
                TravelElapsed = 0.f;
            }
        }
        return;
    }

    // --- Movement phase ---
    if (PlatformState == EPlatformState::Moving_ToEnd || PlatformState == EPlatformState::Moving_ToStart)
    {
        TravelElapsed = FMath::Min(TravelElapsed + DeltaTime, TravelTime);
        const float RawAlpha = TravelElapsed / TravelTime;
        const float CurveAlpha = MovementCurve ? MovementCurve->GetFloatValue(RawAlpha) : RawAlpha;

        const FVector NewLoc = (PlatformState == EPlatformState::Moving_ToEnd)
            ? FMath::Lerp(StartLoc, EndLoc, CurveAlpha)
            : FMath::Lerp(EndLoc, StartLoc, CurveAlpha);

        SetActorLocation(NewLoc);

        if (RawAlpha >= 1.f)
        {
            if (PlatformState == EPlatformState::Moving_ToEnd)
            {
                PlatformState = EPlatformState::Idle_End;
                SetActorLocation(EndLoc);
                OnActivated();
            }
            else
            {
                PlatformState = EPlatformState::Idle_Start;
                SetActorLocation(StartLoc);
                OnDeactivated();
            }
            SetActorTickEnabled(false);
        }
    }
}
