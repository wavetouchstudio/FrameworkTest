#include "DoorDestructible.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

ADoorDestructible::ADoorDestructible()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    FrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameMesh"));
    FrameMesh->SetupAttachment(Root);

    DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
    DoorMesh->SetupAttachment(Root);
}

// Called when the game starts or when spawned
// Sets up destructible door components
void ADoorDestructible::BeginPlay()
{
    Super::BeginPlay();
    CurrentHealth = MaxHealth;
    bIsLocked = bStartLocked;
}

// Applies damage to the destructible door
void ADoorDestructible::ApplyDamage(float Amount)
{
    if (bIsDestroyed) return;

    CurrentHealth = FMath::Max(0.f, CurrentHealth - Amount);
    OnDamaged(CurrentHealth);

    if (CurrentHealth <= 0.f)
    {
        bIsDestroyed = true;
        DoorMesh->SetVisibility(false);
        DoorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        SpawnDebris();
        OnDestroyed();
    }
}

// Spawns debris when door is destroyed
void ADoorDestructible::SpawnDebris()
{
    if (!DebrisClass || DebrisCount <= 0) return;

    const FVector DoorLoc = GetActorLocation();

    // Direction away from the player — debris bursts outward from the door
    FVector AwayDir = GetActorForwardVector();
    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (PC && PC->GetPawn())
    {
        FVector ToPawn = PC->GetPawn()->GetActorLocation() - DoorLoc;
        ToPawn.Z = 0.f;
        if (!ToPawn.IsNearlyZero())
            AwayDir = -ToPawn.GetSafeNormal();
    }

    // Tilt upward so pieces arc rather than slide along the ground
    const FVector ImpulseBase = (AwayDir + FVector(0.f, 0.f, 0.6f)).GetSafeNormal();

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    for (int32 i = 0; i < DebrisCount; ++i)
    {
        // Scatter spawn positions across the door face
        const FVector SpawnOffset = FVector(
            FMath::RandRange(-30.f, 30.f),
            FMath::RandRange(-30.f, 30.f),
            FMath::RandRange(0.f, 60.f));

        AActor* Piece = GetWorld()->SpawnActor<AActor>(DebrisClass, DoorLoc + SpawnOffset, FRotator::ZeroRotator, Params);
        if (!Piece) continue;

        // Auto-destroy after DebrisLifetime seconds
        Piece->SetLifeSpan(DebrisLifetime);

        UPrimitiveComponent* Prim = Piece->FindComponentByClass<UPrimitiveComponent>();
        if (Prim)
        {
            // Debris shouldn't push the player or pull the camera spring arm
            Prim->SetCollisionResponseToChannel(ECC_Pawn,   ECR_Ignore);
            Prim->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

            if (Prim->IsSimulatingPhysics())
            {
                const FVector Dir = FMath::VRandCone(ImpulseBase, FMath::DegreesToRadians(40.f));
                Prim->AddImpulse(Dir * DebrisImpulseStrength, NAME_None, true);
            }
        }
    }
}

void ADoorDestructible::InteractHit()
{
    ApplyDamage(InteractDamage);
}

void ADoorDestructible::LockDoor()
{
    bIsLocked = true;
    OnLocked();
}

void ADoorDestructible::UnlockDoor()
{
    bIsLocked = false;
    OnUnlocked();
}
