#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DoorHinged.h"
#include "DoorHingedDouble.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UCurveFloat;

UCLASS(Blueprintable, meta=(PrioritizeCategories="Door"))
class FRAMEWORKTEST_API ADoorHingedDouble : public AActor
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
    EHingeAxis HingeAxis = EHingeAxis::Yaw;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
    float OpenAngle = 90.f;

    // How long a full open or close takes in seconds
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (ClampMin = "0.05"))
    float OpenDuration = 0.6f;

    // Optional curve — x: 0→1 normalised time, y: 0→1 angle fraction.
    // Steeper start = push feel. Falls back to linear if unset.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
    UCurveFloat* OpenCurve;

    // Auto-picks swing direction away from the player — both leaves mirror each other
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
    bool bRotateAwayFromPlayer = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
    bool bStartLocked = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
    bool bStartOpen = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Door|State")
    bool bIsOpen = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Door|State")
    bool bIsLocked = false;

    UFUNCTION(BlueprintCallable, Category = "Door")
    void ToggleDoor();

    UFUNCTION(BlueprintCallable, Category = "Door")
    void OpenDoor();

    UFUNCTION(BlueprintCallable, Category = "Door")
    void CloseDoor();

    UFUNCTION(BlueprintCallable, Category = "Door")
    void LockDoor();

    UFUNCTION(BlueprintCallable, Category = "Door")
    void UnlockDoor();

    UFUNCTION(BlueprintImplementableEvent, Category = "Door")
    void OnOpened();

    UFUNCTION(BlueprintImplementableEvent, Category = "Door")
    void OnClosed();

    UFUNCTION(BlueprintImplementableEvent, Category = "Door")
    void OnLocked();

    UFUNCTION(BlueprintImplementableEvent, Category = "Door")
    void OnUnlocked();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    float TargetAngle = 0.f;
    float CurrentAngle = 0.f;
    float SourceAngle = 0.f;   // angle when current motion started
    float LerpAlpha = 0.f;
    FRotator ClosedRotA;
    FRotator ClosedRotB;

    void ApplyMeshRotation();
};
