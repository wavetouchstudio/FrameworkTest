#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "FrameworkGameInstance.generated.h"

class ACharacter;

UCLASS()
class FRAMEWORKTEST_API UFrameworkGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    FString CurrentSaveSlot = TEXT("Default");

    // Call from ABonfire::Light()
    UFUNCTION(BlueprintCallable, Category = "Save")
    void SaveAtBonfire(FName BonfireID, ACharacter* Player);

    // Returns false if no save exists yet
    UFUNCTION(BlueprintCallable, Category = "Save")
    bool LoadGame();

    // Moves Player to the last saved/loaded bonfire and restores health
    UFUNCTION(BlueprintCallable, Category = "Save")
    void RespawnAtLastBonfire(ACharacter* Player);

    UPROPERTY(BlueprintReadOnly, Category = "Save")
    FVector LastBonfireLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Save")
    FRotator LastBonfireRotation = FRotator::ZeroRotator;
};
