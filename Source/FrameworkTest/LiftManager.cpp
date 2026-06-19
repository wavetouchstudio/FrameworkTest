#include "LiftManager.h"
#include "Lever.h"

#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/Character.h"

// Constructor for ALiftManager class
// Initializes lift with platform, button, trigger, and marker components
ALiftManager::ALiftManager()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
    RootComponent = PlatformMesh;

    ButtonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ButtonMesh"));
    ButtonMesh->SetupAttachment(RootComponent);

    // Attach trigger to the button so it moves with it during animation
    ButtonTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("ButtonTrigger"));
    ButtonTrigger->SetupAttachment(ButtonMesh);
    ButtonTrigger->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));

    BottomMarker = CreateDefaultSubobject<UBillboardComponent>(TEXT("BottomMarker"));
    BottomMarker->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
// Calculates lift boundaries and sets up button overlap events
void ALiftManager::BeginPlay()
{
    Super::BeginPlay();

    const FBoxSphereBounds MeshBounds = PlatformMesh->CalcBounds(PlatformMesh->GetComponentTransform());
    const float TopSurfaceZ = MeshBounds.GetBox().Max.Z;
    const float SurfaceToRootOffset = TopSurfaceZ - GetActorLocation().Z;

    ZTop = GetActorLocation().Z;
    ZBottom = BottomMarker->GetComponentLocation().Z - SurfaceToRootOffset;

    ButtonRestRelativeLocation = ButtonMesh->GetRelativeLocation();
    ButtonTrigger->OnComponentBeginOverlap.AddDynamic(this, &ALiftManager::OnButtonOverlapBegin);
    ButtonTrigger->OnComponentEndOverlap.AddDynamic(this, &ALiftManager::OnButtonOverlapEnd);
}

void ALiftManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // --- Button press delay ---
    if (ButtonPressDelayRemaining > 0.f)
    {
        ButtonPressDelayRemaining -= DeltaTime;
        if (ButtonPressDelayRemaining <= 0.f)
        {
            ButtonPressDelayRemaining = 0.f;
            ButtonState = EButtonState::Pressing;
            ButtonAnimAlpha = 0.f;
        }
    }

    // --- Button animation ---
    if (ButtonState != EButtonState::Idle)
    {
        ButtonAnimAlpha = FMath::Min(ButtonAnimAlpha + DeltaTime / ButtonAnimDuration, 1.f);

        const FVector PressedLocation = ButtonRestRelativeLocation - FVector(0.f, 0.f, ButtonPressDepth);
        const FVector NewRelLoc = (ButtonState == EButtonState::Pressing)
            ? FMath::Lerp(ButtonRestRelativeLocation, PressedLocation, ButtonAnimAlpha)
            : FMath::Lerp(PressedLocation, ButtonRestRelativeLocation, ButtonAnimAlpha);

        ButtonMesh->SetRelativeLocation(NewRelLoc);

        if (ButtonAnimAlpha >= 1.f)
        {
            ButtonAnimAlpha = 0.f;
            if (ButtonState == EButtonState::Pressing)
            {
                ButtonState = EButtonState::Releasing;
            }
            else
            {
                ButtonState = EButtonState::Idle;
                ButtonMesh->SetRelativeLocation(ButtonRestRelativeLocation);
                ButtonPressed();
            }
        }
    }

    // --- Lift movement ---
    if (LiftState == ELiftState::Moving_Up || LiftState == ELiftState::Moving_Down)
    {
        LiftElapsedTime = FMath::Min(LiftElapsedTime + DeltaTime, TravelTime);
        const float RawAlpha = LiftElapsedTime / TravelTime;
        const float CurveAlpha = MovementCurve ? MovementCurve->GetFloatValue(RawAlpha) : RawAlpha;

        const float NewZ = (LiftState == ELiftState::Moving_Down)
            ? FMath::Lerp(ZTop, ZBottom, CurveAlpha)
            : FMath::Lerp(ZBottom, ZTop, CurveAlpha);

        SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, NewZ));

        if (RawAlpha >= 1.f)
        {
            const bool bAtTop = (LiftState == ELiftState::Moving_Up);
            LiftState = bAtTop ? ELiftState::Idle_Top : ELiftState::Idle_Bottom;

            // If a character rode the lift and is standing on the button, require them to
            // step off and back on before the button arms — avoids phantom immediate re-press.
            TArray<AActor*> OverlappingActors;
            ButtonTrigger->GetOverlappingActors(OverlappingActors, ACharacter::StaticClass());
            bWaitingForPlayerToLeave = (OverlappingActors.Num() > 0);

            for (ALever* Lever : (bAtTop ? LeversToUnlockAtTop : LeversToUnlockAtBottom))
            {
                if (IsValid(Lever))
                    Lever->UnlockLever();
            }

            OnLiftArrived(bAtTop);
        }
    }

    if (ButtonState == EButtonState::Idle && ButtonPressDelayRemaining <= 0.f &&
        (LiftState == ELiftState::Idle_Top || LiftState == ELiftState::Idle_Bottom))
    {
        SetActorTickEnabled(false);
    }
}

void ALiftManager::CallDown()
{
    if (LiftState != ELiftState::Idle_Top) return;
    LiftState = ELiftState::Moving_Down;
    LiftElapsedTime = 0.f;
    SetActorTickEnabled(true);
}

void ALiftManager::CallUp()
{
    if (LiftState != ELiftState::Idle_Bottom) return;
    LiftState = ELiftState::Moving_Up;
    LiftElapsedTime = 0.f;
    SetActorTickEnabled(true);
}

void ALiftManager::ButtonPressed()
{
    if      (LiftState == ELiftState::Idle_Top)    CallDown();
    else if (LiftState == ELiftState::Idle_Bottom) CallUp();
}

void ALiftManager::OnButtonOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (!Cast<ACharacter>(OtherActor)) return;
    if (LiftState == ELiftState::Moving_Up || LiftState == ELiftState::Moving_Down) return;
    if (bWaitingForPlayerToLeave) return;
    if (ButtonState != EButtonState::Idle) return;

    if (ButtonPressDelay > 0.f)
        ButtonPressDelayRemaining = ButtonPressDelay;
    else
    {
        ButtonState = EButtonState::Pressing;
        ButtonAnimAlpha = 0.f;
    }
    SetActorTickEnabled(true);
}

void ALiftManager::OnButtonOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!Cast<ACharacter>(OtherActor)) return;
    bWaitingForPlayerToLeave = false;
}
