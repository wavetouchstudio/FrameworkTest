#include "FrameworkCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

AFrameworkCharacter::AFrameworkCharacter()
{
    UCharacterMovementComponent* Movement = GetCharacterMovement();

    Movement->MaxWalkSpeed = MaxWalkSpeed;
    Movement->JumpZVelocity = JumpZVelocity;
    Movement->AirControl = AirControl;
    Movement->GravityScale = GravityScale;
    Movement->MaxAcceleration = MaxAcceleration;
    Movement->BrakingDecelerationWalking = BrakingDecelerationWalking;
    Movement->RotationRate = FRotator(0.f, RotationYawRate, 0.f);
    Movement->bOrientRotationToMovement = bOrientRotationToMovement;

    bUseControllerRotationYaw = bUseControllerRotationYawSetting;

    // 2, not 1: engine's CanJumpInternal() blocks Jump() while falling with
    // JumpCurrentCount==0 unless JumpCurrentCount+1 < JumpMaxCount. Needed for
    // coyote-time jumps (falling, but no jump performed yet). Our own
    // bJumpedThisAirtime/bDoubleJumpUsed flags gate everything else.
    JumpMaxCount = 2;
}

void AFrameworkCharacter::RequestJump()
{
    UCharacterMovementComponent* Movement = GetCharacterMovement();

    const bool bGrounded = !Movement->IsFalling();
    const bool bCoyoteWindow = !bGrounded && !bJumpedThisAirtime
        && (GetWorld()->GetTimeSeconds() - LeftGroundTime) <= CoyoteTime;

    if (bGrounded || bCoyoteWindow)
    {
        StoredJumpZVelocity = Movement->JumpZVelocity;
        FirstJumpTime = GetWorld()->GetTimeSeconds();
        bDoubleJumpUsed = false;
        bJumpedThisAirtime = true;
        Jump();
    }
    else if (!bDoubleJumpUsed && (GetWorld()->GetTimeSeconds() - FirstJumpTime) >= DoubleJumpDelay)
    {
        bDoubleJumpUsed = true;
        LaunchCharacter(FVector(0.f, 0.f, StoredJumpZVelocity), false, true);
    }
}

void AFrameworkCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);
    bDoubleJumpUsed = false;
    bJumpedThisAirtime = false;
}

void AFrameworkCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
    Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

    if (PrevMovementMode == MOVE_Walking && GetCharacterMovement()->MovementMode == MOVE_Falling && !bJumpedThisAirtime)
    {
        LeftGroundTime = GetWorld()->GetTimeSeconds();
    }
}
