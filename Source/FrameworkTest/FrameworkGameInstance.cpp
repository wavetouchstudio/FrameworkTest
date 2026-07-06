#include "FrameworkGameInstance.h"
#include "FrameworkSaveGame.h"
#include "FrameworkCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

void UFrameworkGameInstance::SaveAtBonfire(FName BonfireID, ACharacter* Player)
{
    if (!IsValid(Player)) return;

    UFrameworkSaveGame* Save = Cast<UFrameworkSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UFrameworkSaveGame::StaticClass()));
    if (!IsValid(Save)) return;

    Save->PlayerLocation = Player->GetActorLocation();
    Save->PlayerRotation = Player->GetActorRotation();
    Save->LastBonfireID = BonfireID;

    if (AFrameworkCharacter* FC = Cast<AFrameworkCharacter>(Player))
    {
        Save->CurrentHealth = FC->CurrentHealth;
        Save->CurrentProfileIndex = FC->CurrentProfileIndex;
    }

    UGameplayStatics::SaveGameToSlot(Save, CurrentSaveSlot, 0);

    LastBonfireLocation = Save->PlayerLocation;
    LastBonfireRotation = Save->PlayerRotation;
}

bool UFrameworkGameInstance::LoadGame()
{
    if (!UGameplayStatics::DoesSaveGameExist(CurrentSaveSlot, 0)) return false;

    UFrameworkSaveGame* Save = Cast<UFrameworkSaveGame>(
        UGameplayStatics::LoadGameFromSlot(CurrentSaveSlot, 0));
    if (!IsValid(Save)) return false;

    LastBonfireLocation = Save->PlayerLocation;
    LastBonfireRotation = Save->PlayerRotation;
    LastSavedHealth = Save->CurrentHealth;
    LastSavedProfileIndex = Save->CurrentProfileIndex;
    return true;
}

void UFrameworkGameInstance::RespawnAtLastBonfire(ACharacter* Player)
{
    if (!IsValid(Player)) return;

    Player->SetActorLocation(LastBonfireLocation);
    Player->SetActorRotation(LastBonfireRotation);

    if (AFrameworkCharacter* FC = Cast<AFrameworkCharacter>(Player))
    {
        FC->CurrentHealth = (LastSavedHealth >= 0.f) ? LastSavedHealth : FC->MaxHealth;
        FC->SetMovementProfile(LastSavedProfileIndex);
    }
}
