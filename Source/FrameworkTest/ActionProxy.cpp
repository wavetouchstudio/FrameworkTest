#include "ActionProxy.h"
#include "DoorDestructible.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"

AActionProxy::AActionProxy()
{
    PrimaryActorTick.bCanEverTick = true;

    HitSphere = CreateDefaultSubobject<USphereComponent>(TEXT("HitSphere"));
    RootComponent = HitSphere;
    HitSphere->SetSphereRadius(15.f);
    HitSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    HitSphere->OnComponentBeginOverlap.AddDynamic(this, &AActionProxy::OnHitSphereBeginOverlap);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(HitSphere);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // HitSphere does all hit detection, mesh is cosmetic only
}

void AActionProxy::Launch(const FVector& InStart, const FVector& InEnd, const FVector& InArcBulge, float InDuration, float InDamage)
{
    StartLoc = InStart;
    EndLoc = InEnd;
    ArcBulge = InArcBulge;
    Duration = FMath::Max(InDuration, 0.05f);
    Damage = InDamage;
    Elapsed = 0.f;
    SetActorLocation(StartLoc);
}

void AActionProxy::BeginPlay()
{
    Super::BeginPlay();
    OnSpawned();
}

void AActionProxy::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    Elapsed += DeltaTime;
    const float Alpha = FMath::Clamp(Elapsed / Duration, 0.f, 1.f);

    // Quadratic Bezier: control point is the arc's midpoint pushed out by ArcBulge.
    const FVector ControlPoint = FMath::Lerp(StartLoc, EndLoc, 0.5f) + ArcBulge;
    const FVector A = FMath::Lerp(StartLoc, ControlPoint, Alpha);
    const FVector B = FMath::Lerp(ControlPoint, EndLoc, Alpha);
    SetActorLocation(FMath::Lerp(A, B, Alpha));

    if (Alpha >= 1.f)
    {
        OnArcComplete();
        Destroy(); // disappears at the end of the arc, hit or not
    }
}

void AActionProxy::OnHitSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (bHasHit || !IsValid(OtherActor) || OtherActor == GetOwner()) return;

    const bool bHitCapsule = Cast<UCapsuleComponent>(OtherComp) != nullptr;
    ADoorDestructible* Door = Cast<ADoorDestructible>(OtherActor);

    if (!bHitCapsule && !Door) return; // only enemy capsules and destructibles register a hit

    bHasHit = true;

    if (Door)
    {
        Door->ApplyDamage(Damage);
    }
    else
    {
        UGameplayStatics::ApplyDamage(OtherActor, Damage, nullptr, GetOwner(), nullptr);
    }

    OnHit(OtherActor);
}
