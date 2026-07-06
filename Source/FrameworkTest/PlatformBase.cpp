#include "PlatformBase.h"

#include "Components/StaticMeshComponent.h"
#include "Curves/CurveFloat.h"

// Constructor for APlatformBase class
// Initializes the platform actor with default settings and components
APlatformBase::APlatformBase()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
    RootComponent = PlatformMesh;
}

// Called when the game starts or when spawned
// Sets up initial platform positions based on travel distances
void APlatformBase::BeginPlay()
{
    Super::BeginPlay();
    StartLoc = GetActorLocation();
    EndLoc = StartLoc + FVector(XTravelDistance, YTravelDistance, ZTravelDistance);
}

// Activates the platform to move to its end position
// Handles delay before movement if configured
void APlatformBase::Activate()
{
    if (PlatformState == EPlatformState::Idle_End) return;

    if (PlatformState == EPlatformState::Moving_ToStart)
    {
        // Interrupted mid-transit — mirror elapsed time (not alpha) so the platform reverses
        // from its current position instead of teleporting to StartLoc. Exact for linear time;
        // for a MovementCurve this is an approximation since alpha isn't linear in time.
        TravelElapsed = TravelTime - TravelElapsed;
        PlatformState = EPlatformState::Moving_ToEnd;
    }
    else if (ActivateDelay > 0.f)
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

// Deactivates the platform to return to its start position
// Handles delay before movement if configured
void APlatformBase::Deactivate()
{
    if (PlatformState == EPlatformState::Idle_Start) return;

    if (PlatformState == EPlatformState::Moving_ToEnd)
    {
        // See Activate() — mirror elapsed time to reverse from the current position, not EndLoc.
        TravelElapsed = TravelTime - TravelElapsed;
        PlatformState = EPlatformState::Moving_ToStart;
    }
    else if (ActivateDelay > 0.f)
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

// Called every frame
// Handles platform movement logic including delay phases and actual movement
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
