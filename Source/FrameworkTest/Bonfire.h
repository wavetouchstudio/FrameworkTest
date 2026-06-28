#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Bonfire.generated.h"

class UStaticMeshComponent;
class USceneComponent;

UCLASS(Blueprintable, meta=(PrioritizeCategories="Bonfire"))
class FRAMEWORKTEST_API ABonfire : public AActor
{
    GENERATED_BODY()

public:
    ABonfire();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bonfire")
    FName BonfireID;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Bonfire|State")
    bool bIsLit = false;

    // Wire this to BPI_Interactable's Interact event in the BP subclass
    UFUNCTION(BlueprintCallable, Category = "Bonfire")
    void Light();

    UFUNCTION(BlueprintImplementableEvent, Category = "Bonfire")
    void OnLit();
};
