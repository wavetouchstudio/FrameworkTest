#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DoorHingedBase.generated.h"

UENUM(BlueprintType)
enum class EHingeAxis : uint8
{
    Yaw   UMETA(DisplayName = "Yaw (side swing)"),
    Pitch UMETA(DisplayName = "Pitch (up / trapdoor swing)")
};

// Shared lock/open/close state machine and away-from-player angle math for hinged
// doors. Subclasses (ADoorHinged, ADoorHingedDouble) own their own mesh components
// and interpolation method via StartOpenAnimation/StartCloseAnimation/IsInteractable.
UCLASS(Abstract, meta=(PrioritizeCategories="Door"))
class FRAMEWORKTEST_API ADoorHingedBase : public AActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
    EHingeAxis HingeAxis = EHingeAxis::Yaw;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
    float OpenAngle = 90.f;

    // Auto-picks swing direction away from the player on open
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

    // False while the door can't be toggled/opened/closed at all (e.g. a free-swinging physics door)
    virtual bool IsInteractable() const { return true; }

    // Subclass starts its own tick/animation and computes its TargetAngle
    virtual void StartOpenAnimation() PURE_VIRTUAL(ADoorHingedBase::StartOpenAnimation, );
    virtual void StartCloseAnimation() PURE_VIRTUAL(ADoorHingedBase::StartCloseAnimation, );

    // Shared away-from-player angle sign flip, given a subclass's own pivot transform
    float ComputeOpenAngle(const FVector& PivotLocation, const FVector& PivotRight) const;
};
