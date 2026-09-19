#pragma once

#include "CoreMinimal.h"
#include "DoorHingedBase.h"
#include "DoorHingedDouble.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UCurveFloat;

UCLASS(Blueprintable, meta=(PrioritizeCategories="Door"))
class FRAMEWORKTEST_API ADoorHingedDouble : public ADoorHingedBase
{
    GENERATED_BODY()

public:
    ADoorHingedDouble();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* FrameMesh;

    // Left leaf — position hinge edge at HingePivotA origin
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* HingePivotA;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* DoorMeshA;

    // Right leaf — position hinge edge at HingePivotB origin
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* HingePivotB;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* DoorMeshB;

    // How long a full open or close takes in seconds
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (ClampMin = "0.05"))
    float OpenDuration = 0.6f;

    // Optional curve — x: 0→1 normalised time, y: 0→1 angle fraction.
    // Steeper start = push feel. Falls back to linear if unset.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
    UCurveFloat* OpenCurve;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    virtual void StartOpenAnimation() override;
    virtual void StartCloseAnimation() override;

private:
    float TargetAngle = 0.f;
    float CurrentAngle = 0.f;
    float SourceAngle = 0.f;   // angle when current motion started
    float LerpAlpha = 0.f;
    FRotator ClosedRotA;
    FRotator ClosedRotB;

    void ApplyMeshRotation();
};
