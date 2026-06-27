#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BlockSocket.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class USphereComponent;
class APickupObject;
class ADoorHinged;
class ADoorSliding;
class ADoorDestructible;
class ADrawbridge;

UCLASS(Blueprintable, meta=(PrioritizeCategories="Socket"))
class FRAMEWORKTEST_API ABlockSocket : public AActor
{
    GENERATED_BODY()

public:
    ABlockSocket();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* Root;

    // The cylinder base — assign your mesh in Blueprint
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* BaseMesh;

    // Place this at the top of BaseMesh — block snaps here
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* SnapPoint;

    // Dropped blocks inside this radius trigger a snap. Resize in the Blueprint viewport.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* DetectionSphere;

    // If set, only blocks with a matching BlockID are accepted. Leave None to accept any block.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
    FName RequiredBlockID;

    // Optional — any ADoorHinged or ADoorSliding actor. Opens when block placed, closes when removed.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
    AActor* LinkedDoor;

    // Lerp speed for the smooth snap — higher is snappier
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket", meta = (ClampMin = "0.1"))
    float SnapSpeed = 8.f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Socket|State")
    bool bOccupied = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Socket|State")
    APickupObject* OccupiedBy = nullptr;

    // Fires when a block fully arrives at the snap point
    UFUNCTION(BlueprintImplementableEvent, Category = "Socket")
    void OnBlockPlaced();

    // Fires when the block is picked back up off the socket
    UFUNCTION(BlueprintImplementableEvent, Category = "Socket")
    void OnBlockRemoved();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    APickupObject* PendingBlock = nullptr;
    bool bSnapping = false;

    void InitiateSnap(APickupObject* Block);
    void ReleaseBlock();

    UFUNCTION()
    void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
