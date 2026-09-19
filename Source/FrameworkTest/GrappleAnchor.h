#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrappleAnchor.generated.h"

class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EGrappleAnchorType : uint8
{
    // Pulls the character straight to the anchor and auto-releases on arrival.
    Pull,
    // Character swings from the anchor on a fixed-length rope; released via jump/grapple input.
    Swing
};

UCLASS(Blueprintable)
class FRAMEWORKTEST_API AGrappleAnchor : public AActor
{
    GENERATED_BODY()

public:
    AGrappleAnchor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* AnchorMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple")
    EGrappleAnchorType AnchorType = EGrappleAnchorType::Pull;

    // Called by the character when this anchor becomes the highlighted grapple target / stops being one.
    UFUNCTION(BlueprintImplementableEvent, Category = "Grapple")
    void OnHighlightChanged(bool bHighlighted);
};
