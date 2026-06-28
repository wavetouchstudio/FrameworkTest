#include "Bonfire.h"
#include "FrameworkGameInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

ABonfire::ABonfire()
{
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Root);
}

void ABonfire::Light()
{
    bIsLit = true;

    if (UFrameworkGameInstance* GI = Cast<UFrameworkGameInstance>(GetGameInstance()))
    {
        if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
        {
            if (ACharacter* PlayerChar = Cast<ACharacter>(PC->GetPawn()))
            {
                GI->SaveAtBonfire(BonfireID, PlayerChar);
            }
        }
    }

    OnLit();
}
