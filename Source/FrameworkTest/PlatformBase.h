#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlatformBase.generated.h"

class UStaticMeshComponent;
class UCurveFloat;

UENUM(BlueprintType)
enum class EPlatformState : uint8
{
    Idle_Start      UMETA(DisplayName = "Idle Start"),
    Delay_ToEnd     UMETA(DisplayName = "Delay To End"),
    Moving_ToEnd    UMETA(DisplayName = "Moving To End"),
    Idle_End        UMETA(DisplayName = "Idle End"),
    Delay_ToStart   UMETA(DisplayName = "Delay To Start"),
    Moving_ToStart  UMETA(DisplayName = "Moving To Start")
};

UCLASS(Blueprintable, meta=(PrioritizeCategories="Platform"))
class FRAMEWORKTEST_API APlatformBase : public AActor
{
    GENERATED_BODY()

public:
    APlatformBase();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* PlatformMesh;

    // Movement offset from spawn location
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform|Travel")
    float XTravelDistance = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform|Travel")
    float YTravelDistance = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform|Travel")
    float ZTravelDistance = 300.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform|Tuning", meta = (ClampMin = "0.1"))
    float TravelTime = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform|Tuning")
    UCurveFloat* MovementCurve;

    // Seconds to wait before beginning movement (matches BP original of 0.3)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform|Tuning", meta = (ClampMin = "0.0"))
    float ActivateDelay = 0.3f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Platform")
    EPlatformState PlatformState = EPlatformState::Idle_Start;

    UFUNCTION(BlueprintCallable, Category = "Platform")
    void Activate();

    UFUNCTION(BlueprintCallable, Category = "Platform")
    void Deactivate();

    UFUNCTION(BlueprintImplementableEvent, Category = "Platform")
    void OnActivated();

    UFUNCTION(BlueprintImplementableEvent, Category = "Platform")
    void OnDeactivated();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    FVector StartLoc = FVector::ZeroVector;
    FVector EndLoc = FVector::ZeroVector;
    float TravelElapsed = 0.f;
    float DelayElapsed = 0.f;
};
