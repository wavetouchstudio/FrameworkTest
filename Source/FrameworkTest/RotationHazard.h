#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RotationHazard.generated.h"

class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EHazardRotationAxis : uint8
{
    X,
    Y,
    Z
};

UCLASS(Blueprintable, meta=(PrioritizeCategories="Hazard"))
class FRAMEWORKTEST_API ARotationHazard : public AActor
{
    GENERATED_BODY()

public:
    ARotationHazard();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* HazardMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
    EHazardRotationAxis RotationAxis = EHazardRotationAxis::Z;

    // Degrees per second
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
    float RotationSpeed = 90.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
    float ContactDamage = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard", meta = (ClampMin = "0.0"))
    float ImpulseStrength = 600.f;

    // Min seconds before the same actor can be damaged/impulsed again
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard", meta = (ClampMin = "0.0"))
    float DamageCooldown = 1.f;

    UFUNCTION(BlueprintImplementableEvent, Category = "Hazard")
    void OnHazardContact(AActor* OtherActor);

protected:
    virtual void Tick(float DeltaTime) override;

private:
    TMap<TWeakObjectPtr<AActor>, float> LastDamageTime;

    UFUNCTION()
    void OnMeshOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
