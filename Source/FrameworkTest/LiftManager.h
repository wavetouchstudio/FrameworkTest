#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LiftManager.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UBillboardComponent;
class UCurveFloat;
class ALever;

UENUM(BlueprintType)
enum class ELiftState : uint8
{
    Idle_Top    UMETA(DisplayName = "Idle Top"),
    Idle_Bottom UMETA(DisplayName = "Idle Bottom"),
    Moving_Up   UMETA(DisplayName = "Moving Up"),
    Moving_Down UMETA(DisplayName = "Moving Down")
};

UENUM()
enum class EButtonState : uint8
{
    Idle,
    Pressing,
    Releasing
};

UCLASS(meta=(PrioritizeCategories="Lift"))
class FRAMEWORKTEST_API ALiftManager : public AActor
{
    GENERATED_BODY()

public:
    ALiftManager();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* PlatformMesh;

    // Cube button mesh — position it on top of your cylinder pedestal in the editor
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* ButtonMesh;

    // Overlap trigger attached to ButtonMesh — resize to cover the top face in the editor
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* ButtonTrigger;

    // Drag this to where the top surface of the platform mesh should sit at the bottom stop
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBillboardComponent* BottomMarker;

    // Total travel time in seconds
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lift|Tuning", meta = (ClampMin = "0.1"))
    float TravelTime = 3.f;

    // Optional ease curve — maps [0,1] to [0,1]. Flat diagonal = linear, S-curve = ease in/out
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lift|Tuning")
    UCurveFloat* MovementCurve;

    // How far the button sinks when pressed (cm)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button|Tuning", meta = (ClampMin = "0.5"))
    float ButtonPressDepth = 8.f;

    // Time for the button to travel down (and back up)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button|Tuning", meta = (ClampMin = "0.05"))
    float ButtonAnimDuration = 0.25f;

    // Delay after stepping on the trigger before the button starts pressing
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button|Tuning", meta = (ClampMin = "0.0"))
    float ButtonPressDelay = 0.5f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lift")
    ELiftState LiftState = ELiftState::Idle_Top;

    // The last lever that triggered this lift — used by Blueprint to reset it on arrival
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Lift|Debug")
    ALever* LastActivatingLever = nullptr;

    // Levers unlocked automatically when the lift reaches the top stop
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lift")
    TArray<ALever*> LeversToUnlockAtTop;

    // Levers unlocked automatically when the lift reaches the bottom stop
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lift")
    TArray<ALever*> LeversToUnlockAtBottom;

    UFUNCTION(BlueprintCallable, Category = "Lift")
    void ButtonPressed();

    UFUNCTION(BlueprintCallable, Category = "Lift")
    void CallUp();

    UFUNCTION(BlueprintCallable, Category = "Lift")
    void CallDown();

    UFUNCTION(BlueprintImplementableEvent, Category = "Lift")
    void OnLiftArrived(bool bArrivedAtTop);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    UPROPERTY(VisibleInstanceOnly, Category = "Lift|Debug")
    float ZTop = 0.f;

    UPROPERTY(VisibleInstanceOnly, Category = "Lift|Debug")
    float ZBottom = 0.f;

    float LiftElapsedTime = 0.f;

    EButtonState ButtonState = EButtonState::Idle;
    float ButtonAnimAlpha = 0.f;
    float ButtonPressDelayRemaining = 0.f;
    FVector ButtonRestRelativeLocation = FVector::ZeroVector;

    UFUNCTION()
    void OnButtonOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnButtonOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    bool bWaitingForPlayerToLeave = false;
};
