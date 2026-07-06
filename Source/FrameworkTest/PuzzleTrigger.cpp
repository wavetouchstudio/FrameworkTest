#include "PuzzleTrigger.h"
#include "PuzzleManager.h"
#include "PickupObject.h"
#include "DoorLinkUtils.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

APuzzleTrigger::APuzzleTrigger()
{
    PrimaryActorTick.bCanEverTick = false;

    TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    RootComponent = TriggerBox;
    TriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

// Called when the game starts or when spawned
// Sets up puzzle trigger components
void APuzzleTrigger::BeginPlay()
{
    Super::BeginPlay();

    if (bUseLeverInstead)
    {
        TriggerBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    else
    {
        TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &APuzzleTrigger::OnBoxBeginOverlap);
        TriggerBox->OnComponentEndOverlap.AddDynamic(this, &APuzzleTrigger::OnBoxEndOverlap);
    }
}

// Activates the puzzle trigger via lever
void APuzzleTrigger::ActivateByLever()
{
    if (bIsActive || !IsValid(PuzzleManager)) return;
    bIsActive = true;
    PuzzleManager->TriggerActivated(this);
    OnTriggerActivated();
    OpenLinkedDoor(LinkedDoor);
}

// Deactivates the puzzle trigger via lever
void APuzzleTrigger::DeactivateByLever()
{
    if (!bIsActive || !IsValid(PuzzleManager)) return;
    bIsActive = false;
    PuzzleManager->TriggerDeactivated(this);
    OnTriggerDeactivated();
    CloseLinkedDoor(LinkedDoor);
}

bool APuzzleTrigger::IsAcceptedActor(AActor* Actor) const
{
    if (IsValid(AcceptedActor) && Actor == AcceptedActor) return true;
    ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
    return IsValid(Player) && Actor == Player;
}

void APuzzleTrigger::NotifyPickupDropped(APickupObject* Pickup)
{
    if (!bAcceptsPickupSwap || !bIsActive) return;
    ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (ActiveActor != Player) return;
    ActiveActor = Pickup;
}

void APuzzleTrigger::NotifyPickupLifted(APickupObject* Pickup)
{
    if (!bIsActive || ActiveActor != Pickup) return;
    if (!IsValid(PuzzleManager)) return;

    ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (IsValid(Player) && TriggerBox->IsOverlappingActor(Player))
    {
        // Player is standing on the trigger — hand ownership back to them
        ActiveActor = Player;
    }
    else
    {
        // Nobody valid left on the trigger — deactivate
        ActiveActor = nullptr;
        bIsActive = false;
        PuzzleManager->TriggerDeactivated(this);
        OnTriggerDeactivated();
        CloseLinkedDoor(LinkedDoor);
    }
}

void APuzzleTrigger::OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (bIsActive)
    {
        // Block rolled onto trigger after being dropped nearby — swap if player is active
        if (!bAcceptsPickupSwap) return;
        ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
        if (ActiveActor != Player) return;
        APickupObject* Pickup = Cast<APickupObject>(OtherActor);
        if (Pickup && !Pickup->bIsCarried)
            ActiveActor = Pickup;
        return;
    }

    if (!IsAcceptedActor(OtherActor)) return;
    if (!IsValid(PuzzleManager)) return;

    ActiveActor = OtherActor;
    bIsActive = true;
    PuzzleManager->TriggerActivated(this);
    OnTriggerActivated();
    OpenLinkedDoor(LinkedDoor);
}

void APuzzleTrigger::OnBoxEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (OtherActor != ActiveActor) return;
    if (!IsValid(PuzzleManager)) return;

    ActiveActor = nullptr;
    bIsActive = false;
    PuzzleManager->TriggerDeactivated(this);
    OnTriggerDeactivated();
    CloseLinkedDoor(LinkedDoor);
}
