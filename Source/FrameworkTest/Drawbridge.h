#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Drawbridge.generated.h"

class UStaticMeshComponent;
class USceneComponent;

UCLASS(Blueprintable, meta=(PrioritizeCategories="Drawbridge"))
class FRAMEWORKTEST_API ADrawbridge : public AActor
{
    GENERATED_BODY()

public:
    ADrawbridge();

    // Place this actor so HingePivot sits at the wall anchor — everything rotates around it
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* HingePivot;

    // Rotates around HingePivot. In Blueprint: offset BridgeMesh so its hinge edge is at this component's origin.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* Arm;

    // The bridge plank — child of Arm
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* BridgeMesh;

    // Pitch degrees when fully open. Negative = rotates outward/down from a raised wall position.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drawbridge")
    float OpenAngle = -90.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drawbridge", meta = (ClampMin = "0.1"))
    float OpenSpeed = 3.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drawbridge")
    bool bStartOpen = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Drawbridge|State")
    bool bIsOpen = false;

    UFUNCTION(BlueprintCallable, Category = "Drawbridge")
    void Open();

    UFUNCTION(BlueprintCallable, Category = "Drawbridge")
    void Close();

    UFUNCTION(BlueprintCallable, Category = "Drawbridge")
    void Toggle();

    UFUNCTION(BlueprintImplementableEvent, Category = "Drawbridge")
    void OnOpened();

    UFUNCTION(BlueprintImplementableEvent, Category = "Drawbridge")
    void OnClosed();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    float TargetAngle = 0.f;
    float CurrentAngle = 0.f;
    FRotator ClosedRot;
};
