#include "ActionChainComponent.h"
#include "ActionProxy.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "Engine/World.h"

UActionChainComponent::UActionChainComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UActionChainComponent::RequestAction()
{
    if (ActionChain.Num() == 0) return;

    if (!bStageInProgress)
    {
        StartStage(CurrentStageIndex);
        return;
    }

    const float TimeLeft = StageEndTime - GetWorld()->GetTimeSeconds();
    const float Threshold = ActionChain[CurrentStageIndex].Duration * BufferWindow;
    if (TimeLeft <= Threshold)
    {
        bBuffered = true; // landed in the buffer window — chain into the next stage automatically
    }
    // else: pressed too early mid-stage — dropped, the current stage just keeps playing
}

void UActionChainComponent::StartStage(int32 Index)
{
    const int32 Wrapped = Index % ActionChain.Num();
    const FActionStageDef& Stage = ActionChain[Wrapped];

    AActor* Owner = GetOwner();
    const FVector Forward = Owner->GetActorForwardVector();
    const FVector Right = Owner->GetActorRightVector();
    const FVector Up = Owner->GetActorUpVector();
    const FVector Origin = Owner->GetActorLocation();

    const FVector StartWorld = Origin + Forward * Stage.StartOffset.X + Right * Stage.StartOffset.Y + Up * Stage.StartOffset.Z;
    const FVector EndWorld = Origin + Forward * Stage.EndOffset.X + Right * Stage.EndOffset.Y + Up * Stage.EndOffset.Z;
    const FVector BulgeWorld = Forward * Stage.ArcBulge.X + Right * Stage.ArcBulge.Y + Up * Stage.ArcBulge.Z;

    const int32 Count = (Stage.SpawnPattern == EActionSpawnPattern::Single) ? 1 : FMath::Max(1, Stage.SpawnCount);
    for (int32 i = 0; i < Count; ++i)
    {
        if (i == 0)
        {
            SpawnProxyForStage(Stage, StartWorld, EndWorld, BulgeWorld);
        }
        else
        {
            FTimerHandle Handle;
            FTimerDelegate Delegate = FTimerDelegate::CreateUObject(
                this, &UActionChainComponent::SpawnProxyForStage, Stage, StartWorld, EndWorld, BulgeWorld);
            GetWorld()->GetTimerManager().SetTimer(Handle, Delegate, Stage.SpawnStagger * i, false);
        }
    }

    CurrentStageIndex = Wrapped;
    bStageInProgress = true;
    bBuffered = false;
    StageEndTime = GetWorld()->GetTimeSeconds() + Stage.Duration;
}

void UActionChainComponent::SpawnProxyForStage(FActionStageDef Stage, FVector StartWorld, FVector EndWorld, FVector BulgeWorld)
{
    if (!Stage.ProxyClass) return;

    AActor* Owner = GetOwner();
    FActorSpawnParameters Params;
    Params.Owner = Owner;
    if (AActionProxy* Proxy = GetWorld()->SpawnActor<AActionProxy>(Stage.ProxyClass, StartWorld, Owner->GetActorRotation(), Params))
    {
        Proxy->Launch(StartWorld, EndWorld, BulgeWorld, Stage.Duration, Stage.Damage);
    }
}

void UActionChainComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    const float Now = GetWorld()->GetTimeSeconds();

    if (bStageInProgress && Now >= StageEndTime)
    {
        bStageInProgress = false;

        if (bBuffered)
        {
            StartStage(CurrentStageIndex + 1); // buffered press caught it in time — chain into the next stage
        }
        else
        {
            ResetDeadline = Now + ResetDelay;
        }
    }

    if (!bStageInProgress && CurrentStageIndex != 0 && Now >= ResetDeadline)
    {
        CurrentStageIndex = 0; // no follow-up came in time — quietly back to stage 0, no penalty
    }
}
