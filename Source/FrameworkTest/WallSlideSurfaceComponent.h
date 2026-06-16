#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WallSlideSurfaceComponent.generated.h"

// Attach to a wall/obstacle actor to override wall-slide behavior on that surface.
// Absence of this component = use the character's movement profile defaults.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FRAMEWORKTEST_API UWallSlideSurfaceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWallSlideSurfaceComponent();

    // If false, this surface never allows wall slide (use for curved/rounded geometry)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Slide Surface")
    bool bAllowWallSlide = true;

    // -1 = use character profile default
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Slide Surface", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float ApproachDotThresholdOverride = -1.f;

    // Multiplies the profile's WallSlideSpeed (1 = normal, <1 = grippier/slower slide, >1 = slicker/faster)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Slide Surface", meta = (ClampMin = "0.0"))
    float SlideSpeedMultiplier = 1.f;
};
