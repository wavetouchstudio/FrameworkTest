#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ActionProxy.generated.h"

class UStaticMeshComponent;
class USphereComponent;

UCLASS(Blueprintable)
class FRAMEWORKTEST_API AActionProxy : public AActor
{
    GENERATED_BODY()

public:
    AActionProxy();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* HitSphere;

    // Called once, right after SpawnActor, by UActionChainComponent::SpawnProxyForStage. All points are world-space.
    UFUNCTION(BlueprintCallable, Category = "Action")
    void Launch(const FVector& InStart, const FVector& InEnd, const FVector& InArcBulge, float InDuration, float InDamage);

    // Hooks for Blueprint — wire VFX/sound/camera shake here. C++ never spawns cosmetic effects directly.
    UFUNCTION(BlueprintImplementableEvent, Category = "Action")
    void OnSpawned();

    UFUNCTION(BlueprintImplementableEvent, Category = "Action")
    void OnHit(AActor* HitActor);

    UFUNCTION(BlueprintImplementableEvent, Category = "Action")
    void OnArcComplete();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    FVector StartLoc = FVector::ZeroVector;
    FVector EndLoc = FVector::ZeroVector;
    FVector ArcBulge = FVector::ZeroVector;
    float Duration = 0.25f;
    float Damage = 10.f;
    float Elapsed = 0.f;
    bool bHasHit = false;

    UFUNCTION()
    void OnHitSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
