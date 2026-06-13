#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RotationHazard.h"
#include "SplinePlatform.generated.h"

class USplineComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ESplineMovementMode : uint8
{
    Loop,
    PingPong
};

UCLASS(Blueprintable, meta=(PrioritizeCategories="Platform"))
class FRAMEWORKTEST_API ASplinePlatform : public AActor
{
    GENERATED_BODY()

public:
    ASplinePlatform();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USplineComponent* SplinePath;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* PlatformMesh;

    // --- Rotation ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform|Rotation")
    bool bRotationEnabled = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform|Rotation")
    EHazardRotationAxis RotationAxis = EHazardRotationAxis::Z;

    // Degrees per second
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform|Rotation")
    float RotationSpeed = 30.f;

    // --- Movement ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform|Movement")
    bool bMovementEnabled = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform|Movement")
    ESplineMovementMode MovementMode = ESplineMovementMode::Loop;

    // Centimeters per second along the spline
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform|Movement", meta = (ClampMin = "0.0"))
    float MovementSpeed = 100.f;

protected:
    virtual void Tick(float DeltaTime) override;

private:
    float DistanceAlongSpline = 0.f;
    int32 MovementDir = 1;
};
