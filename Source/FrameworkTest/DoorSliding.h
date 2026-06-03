#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DoorSliding.generated.h"

class UStaticMeshComponent;
class USceneComponent;

UCLASS(Blueprintable, meta=(PrioritizeCategories="Door"))
class FRAMEWORKTEST_API ADoorSliding : public AActor
{
    GENERATED_BODY()

public:
    ADoorSliding();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* FrameMesh;

    // Primary door panel — slides along SlideDirection when opening
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* DoorMesh;

    // Second panel for double sliding doors — slides in the opposite direction. Hidden unless bDoubleDoor.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* DoorMeshB;

    // Normalized direction DoorMesh travels when opening.
    // Examples: (0,0,1)=up  (0,0,-1)=down  (1,0,0)=forward  (0,1,0)=right
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
    FVector SlideDirection = FVector(0.f, 0.f, 1.f);

    // Distance in cm from closed to fully open
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
    float SlideDistance = 200.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
    float OpenSpeed = 3.f;

    // DoorMeshB slides in the opposite direction — place it mirrored from DoorMesh in the Blueprint
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
    bool bDoubleDoor = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
    bool bStartLocked = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
    bool bStartOpen = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Door|State")
    bool bIsOpen = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Door|State")
    bool bIsLocked = false;

    // Opens if closed and unlocked; closes if open. No-op when locked.
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
    FVector ClosedPosA;
    FVector ClosedPosB;
    FVector CurrentPosA;
    FVector CurrentPosB;
};
