#include "BlockSocket.h"
#include "PickupObject.h"
#include "DoorLinkUtils.h"
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
        if (InitiateSnap(PendingBlock))
        {
            PendingBlock = nullptr;
        }
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
            OpenLinkedDoor(LinkedDoor);
        }
    }
    else if (bOccupied && IsValid(OccupiedBy))
    {
        if (OccupiedBy->bIsCarried)
        {
            ReleaseBlock();
        }
        else if (!OccupiedBy->GetActorLocation().Equals(SnapPoint->GetComponentLocation(), 0.01f))
        {
            // Hold block pinned to snap point — physics can't dislodge it.
            // Only re-write the transform if it actually drifted; avoids a no-op SetActorLocation every frame.
            OccupiedBy->SetActorLocation(SnapPoint->GetComponentLocation());
            OccupiedBy->SetActorRotation(SnapPoint->GetComponentRotation());
        }
    }
}

// Initiates snapping process for a block. Returns true if the snap actually started.
bool ABlockSocket::InitiateSnap(APickupObject* Block)
{
    if (!IsValid(Block) || bOccupied || bSnapping) return false;

    // Guard against stale physics-wake events — verify block is actually inside the sphere
    const float Radius = DetectionSphere->GetScaledSphereRadius();
    if (FVector::DistSquared(Block->GetActorLocation(), DetectionSphere->GetComponentLocation()) > Radius * Radius) return false;

    if (!RequiredBlockID.IsNone() && Block->BlockID != RequiredBlockID) return false;

    OccupiedBy = Block;
    bSnapping = true;

    // Disable physics for the duration of the snap — PickupObject restores it on next drop
    OccupiedBy->Mesh->SetSimulatePhysics(false);
    OccupiedBy->Mesh->SetEnableGravity(false);
    return true;
}

// Releases the currently snapped block
void ABlockSocket::ReleaseBlock()
{
    bOccupied = false;
    bSnapping = false;
    OccupiedBy = nullptr;
    OnBlockRemoved();
    CloseLinkedDoor(LinkedDoor);
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
