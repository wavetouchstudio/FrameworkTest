#include "GrappleAnchor.h"
#include "Components/StaticMeshComponent.h"

// Constructor for AGrappleAnchor class
// Initializes grapple anchor with static mesh component
AGrappleAnchor::AGrappleAnchor()
{
    PrimaryActorTick.bCanEverTick = false;

    AnchorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AnchorMesh"));
    RootComponent = AnchorMesh;
}
