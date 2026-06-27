#include "BlockSocket.h"
#include "PickupObject.h"
#include "DoorHinged.h"
#include "DoorSliding.h"
#include "DoorDestructible.h"
#include "Drawbridge.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"

ABlockSocket::ABlockSocket()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
    BaseMesh->SetupAttachment(Root);

    SnapPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SnapPoint"));
    SnapPoint->SetupAttachment(Root);

    DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
    DetectionSphere->SetupAttachment(Root);
    DetectionSphere->SetSphereRadius(50.f);
    DetectionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

// Called when the game starts or when spawned
// Sets up block socket components
void ABlockSocket::BeginPlay()
{
    Super::BeginPlay();
    DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ABlockSocket::OnSphereBeginOverlap);
    DetectionSphere->OnComponentEndOverlap.AddDynamic(this, &ABlockSocket::OnSphereEndOverlap);
}

// Called every frame
// Handles block snapping animation
void ABlockSocket::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Wait until the block is fully settled (Idle) before snapping — prevents FinalizeDrop from re-enabling physics mid-snap
    if (IsValid(PendingBlock) && !PendingBlock->bIsCarried
        && PendingBlock->PickupState == EPickupState::Idle)
    {
        InitiateSnap(PendingBlock);
        PendingBlock = nullptr;
    }

    if (bSnapping && IsValid(OccupiedBy))
    {
        if (OccupiedBy->bIsCarried) { ReleaseBlock(); return; }

        const FVector Target = SnapPoint->GetComponentLocation();
        const FVector NewLoc = FMath::VInterpTo(OccupiedBy->GetActorLocation(), Target, DeltaTime, SnapSpeed);
        OccupiedBy->SetActorLocation(NewLoc);

        if (FVector::DistSquared(NewLoc, Target) < 4.f) // within ~2cm
        {
            OccupiedBy->SetActorLocation(Target);
            OccupiedBy->SetActorRotation(SnapPoint->GetComponentRotation());
            bSnapping = false;
            bOccupied = true;
            OnBlockPlaced();
            if (IsValid(LinkedDoor))
            {
                if (ADoorHinged* DH = Cast<ADoorHinged>(LinkedDoor))               { DH->OpenDoor(); }
                else if (ADoorSliding* DS = Cast<ADoorSliding>(LinkedDoor))       { DS->OpenDoor(); }
                else if (ADoorDestructible* DD = Cast<ADoorDestructible>(LinkedDoor)) { DD->UnlockDoor(); }
                else if (ADrawbridge* DB = Cast<ADrawbridge>(LinkedDoor))         { DB->Open(); }
            }
        }
    }
    else if (bOccupied && IsValid(OccupiedBy))
    {
        if (OccupiedBy->bIsCarried)
        {
            ReleaseBlock();
        }
        else
        {
            // Hold block pinned to snap point — physics can't dislodge it
            OccupiedBy->SetActorLocation(SnapPoint->GetComponentLocation());
            OccupiedBy->SetActorRotation(SnapPoint->GetComponentRotation());
        }
    }
}

// Initiates snapping process for a block
void ABlockSocket::InitiateSnap(APickupObject* Block)
{
    if (!IsValid(Block) || bOccupied || bSnapping) return;

    // Guard against stale physics-wake events — verify block is actually inside the sphere
    const float Radius = DetectionSphere->GetScaledSphereRadius();
    if (FVector::DistSquared(Block->GetActorLocation(), DetectionSphere->GetComponentLocation()) > Radius * Radius) return;

    if (!RequiredBlockID.IsNone() && Block->BlockID != RequiredBlockID) return;

    OccupiedBy = Block;
    bSnapping = true;

    // Disable physics for the duration of the snap — PickupObject restores it on next drop
    OccupiedBy->Mesh->SetSimulatePhysics(false);
    OccupiedBy->Mesh->SetEnableGravity(false);
}

// Releases the currently snapped block
void ABlockSocket::ReleaseBlock()
{
    bOccupied = false;
    bSnapping = false;
    OccupiedBy = nullptr;
    OnBlockRemoved();
    if (IsValid(LinkedDoor))
    {
        if (ADoorHinged* DH = Cast<ADoorHinged>(LinkedDoor))               { DH->CloseDoor(); }
        else if (ADoorSliding* DS = Cast<ADoorSliding>(LinkedDoor))       { DS->CloseDoor(); }
        else if (ADoorDestructible* DD = Cast<ADoorDestructible>(LinkedDoor)) { DD->LockDoor(); }
        else if (ADrawbridge* DB = Cast<ADrawbridge>(LinkedDoor))         { DB->Close(); }
    }
}

void ABlockSocket::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (bOccupied || bSnapping) return;

    APickupObject* Block = Cast<APickupObject>(OtherActor);
    if (!IsValid(Block)) return;

    if (Block->bIsCarried || Block->PickupState != EPickupState::Idle)
        PendingBlock = Block; // still settling — wait for Idle before snapping
    else
        InitiateSnap(Block); // fully at rest, snap immediately
}

void ABlockSocket::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (OtherActor == PendingBlock)
        PendingBlock = nullptr; // block carried out of range — cancel pending snap
}
