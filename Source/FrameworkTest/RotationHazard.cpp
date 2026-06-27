#include "RotationHazard.h"

#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "FrameworkCharacter.h"
#include "PickupObject.h"

// Constructor for ARotationHazard class
// Initializes rotating hazard with mesh component
ARotationHazard::ARotationHazard()
{
    PrimaryActorTick.bCanEverTick = true;

    HazardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HazardMesh"));
    RootComponent = HazardMesh;

    HazardMesh->SetGenerateOverlapEvents(true);
    HazardMesh->OnComponentBeginOverlap.AddDynamic(this, &ARotationHazard::OnMeshOverlap);
}

// Called every frame
// Handles hazard rotation based on configured axis and speed
void ARotationHazard::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    FVector Axis = FVector::UpVector;
    switch (RotationAxis)
    {
        case EHazardRotationAxis::X: Axis = FVector::ForwardVector; break;
        case EHazardRotationAxis::Y: Axis = FVector::RightVector; break;
        case EHazardRotationAxis::Z: Axis = FVector::UpVector; break;
    }

    HazardMesh->AddLocalRotation(FQuat(Axis, FMath::DegreesToRadians(RotationSpeed * DeltaTime)));
}

// Called when hazard mesh overlaps with another actor
// Handles damage to characters and objects
void ARotationHazard::OnMeshOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == this)
    {
        return;
    }

    // Ignore overlaps from non-physical detection volumes (e.g. the player's interact sphere) —
    // only react to the character's capsule or the block's actual mesh.
    if (ACharacter* OtherChar = Cast<ACharacter>(OtherActor))
    {
        if (OtherComp != OtherChar->GetCapsuleComponent())
        {
            return;
        }
    }
    else if (APickupObject* OtherBlock = Cast<APickupObject>(OtherActor))
    {
        if (OtherComp != OtherBlock->Mesh)
        {
            return;
        }
    }

    const float Now = GetWorld()->GetTimeSeconds();
    if (const float* LastTime = LastDamageTime.Find(OtherActor))
    {
        if (Now - *LastTime < DamageCooldown)
        {
            return;
        }
    }

    const FVector Dir = (OtherActor->GetActorLocation() - HazardMesh->GetComponentLocation()).GetSafeNormal();

    if (ACharacter* Char = Cast<ACharacter>(OtherActor))
    {
        if (AFrameworkCharacter* FrameworkChar = Cast<AFrameworkCharacter>(Char))
        {
            FrameworkChar->CurrentHealth = FMath::Max(0.f, FrameworkChar->CurrentHealth - ContactDamage);
        }

        Char->LaunchCharacter(Dir * ImpulseStrength, true, true);
    }
    else if (APickupObject* Block = Cast<APickupObject>(OtherActor))
    {
        if (Block->Mesh)
        {
            Block->Mesh->AddImpulse(Dir * ImpulseStrength, NAME_None, true);
        }
    }
    else
    {
        return;
    }

    LastDamageTime.Add(OtherActor, Now);
    OnHazardContact(OtherActor);
}
