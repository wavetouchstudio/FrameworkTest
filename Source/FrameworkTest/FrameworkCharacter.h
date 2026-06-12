#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FrameworkCharacter.generated.h"

UCLASS(Blueprintable)
class FRAMEWORKTEST_API AFrameworkCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AFrameworkCharacter();

    // --- Movement Tuning (applied to CharacterMovement in constructor) ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning")
    float MaxWalkSpeed = 550.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning")
    float JumpZVelocity = 1400.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning")
    float AirControl = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning")
    float GravityScale = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning")
    float MaxAcceleration = 1500.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning")
    float BrakingDecelerationWalking = 2000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning")
    float RotationYawRate = 750.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning")
    bool bOrientRotationToMovement = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning")
    bool bUseControllerRotationYawSetting = false;

    // --- Double Jump ---
    // Min seconds after the first jump before a double jump can trigger
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Double Jump", meta = (ClampMin = "0.0"))
    float DoubleJumpDelay = 0.25f;

    // Call from Blueprint's IA_Jump "Started" event (replaces the old Jump()/LaunchCharacter graph)
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void RequestJump();

    // Grace period after walking off a ledge where a jump still counts as a ground jump
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Double Jump", meta = (ClampMin = "0.0"))
    float CoyoteTime = 0.15f;

protected:
    virtual void Landed(const FHitResult& Hit) override;
    virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;

private:
    bool bDoubleJumpUsed = false;
    bool bJumpedThisAirtime = false;
    float StoredJumpZVelocity = 0.f;
    float FirstJumpTime = -1000.f;
    float LeftGroundTime = -1000.f;
};
