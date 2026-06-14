#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrappleAnchor.generated.h"

class UStaticMeshComponent;

UCLASS(Blueprintable)
class FRAMEWORKTEST_API AGrappleAnchor : public AActor
{
    GENERATED_BODY()

public:
    AGrappleAnchor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* AnchorMesh;

    // Called by the character when this anchor becomes the highlighted grapple target / stops being one.
    UFUNCTION(BlueprintImplementableEvent, Category = "Grapple")
    void OnHighlightChanged(bool bHighlighted);
};
