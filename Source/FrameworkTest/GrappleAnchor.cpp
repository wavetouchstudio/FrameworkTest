#include "GrappleAnchor.h"
#include "Components/StaticMeshComponent.h"

AGrappleAnchor::AGrappleAnchor()
{
    PrimaryActorTick.bCanEverTick = false;

    AnchorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AnchorMesh"));
    RootComponent = AnchorMesh;
}
