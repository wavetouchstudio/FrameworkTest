#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActionChainComponent.generated.h"

class AActionProxy;

UENUM(BlueprintType)
enum class EActionSpawnPattern : uint8
{
    Single      UMETA(DisplayName = "Single"),
    Sequential  UMETA(DisplayName = "Sequential (staggered)"),
    Radial      UMETA(DisplayName = "Radial (not yet implemented)")
};

USTRUCT(BlueprintType)
struct FActionStageDef
{
    GENERATED_BODY()

    // Cosmetic label for matching an animation montage later — "Jab", "Hook", "Uppercut", etc. Not used by code yet.
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName StageName;

    // Actor class spawned for this stage.
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<AActionProxy> ProxyClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EActionSpawnPattern SpawnPattern = EActionSpawnPattern::Single;

    // Only used when SpawnPattern != Single. Radial is reserved — placement math isn't implemented yet,
    // so it currently behaves the same as Sequential (staggered spawns at the same arc position).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
    int32 SpawnCount = 1;

    // Seconds between each spawn when SpawnPattern != Single
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
    float SpawnStagger = 0.05f;

    // Local-space (X=forward, Y=right, Z=up) start point of the arc, relative to whatever actor owns the component
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector StartOffset = FVector(50.f, -40.f, 20.f);

    // Local-space end point of the arc
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector EndOffset = FVector(50.f, 40.f, 20.f);

    // Local-space offset pushing the arc's midpoint outward — this is what gives the swing its curve.
    // Small/zero for a jab, larger sideways for a hook, larger upward for an uppercut.
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector ArcBulge = FVector(20.f, 0.f, 10.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.05"))
    float Duration = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
    float Damage = 10.f;
};

// Generic spawn-object-along-a-path-with-a-timed-hitbox sequencer. Attach to any actor — not specific to
// the player character. The punch combo is the first user of this; future users (a dash, a puzzle
// interaction) reuse it by attaching another instance with a different ActionChain.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FRAMEWORKTEST_API UActionChainComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UActionChainComponent();

    // The chain, in order — A1, A2, A3... index wraps once the last entry is reached.
    // Reordering/retheming a combo (jab-jab-hook vs hook-hook-uppercut) is just editing this array.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Chain")
    TArray<FActionStageDef> ActionChain;

    // Fraction of the current stage's Duration (counting from the end) during which a fresh
    // RequestAction() call is buffered and auto-fires the next stage when the current one finishes.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Chain", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BufferWindow = 0.3f;

    // Seconds after a stage finishes, with no buffered follow-up, before the chain resets to stage 0.
    // No penalty — it just quietly lapses back to idle. This is the "cut short at any time" behavior.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Chain", meta = (ClampMin = "0.0"))
    float ResetDelay = 0.6f;

    UPROPERTY(BlueprintReadOnly, Category = "Action Chain")
    int32 CurrentStageIndex = 0;

    // Call from any input action's "Started" event on whatever actor owns this component
    UFUNCTION(BlueprintCallable, Category = "Action Chain")
    void RequestAction();

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    bool bStageInProgress = false;
    bool bBuffered = false;
    float StageEndTime = -1000.f;
    float ResetDeadline = -1000.f;

    void StartStage(int32 Index);
    void SpawnProxyForStage(FActionStageDef Stage, FVector StartWorld, FVector EndWorld, FVector BulgeWorld);
};
