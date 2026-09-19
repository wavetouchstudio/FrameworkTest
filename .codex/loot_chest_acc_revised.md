# Plan: Week 1-2 — Random Drops + Bonfire Chest + Accessories

## Context

**Implementation Status (verified against `Source/FrameworkTest/` on 2026-07-25):** All three systems are **fully unplanned/not started** — no `LootTable`, `BonfireChest`, or `AccessoryItem` header or source file exists anywhere in `Source/FrameworkTest/` (confirmed via glob: `*Loot*`, `*Chest*`, `*Accessor*` all return zero matches). This document is a pre-implementation plan only; every code block below is proposed, not landed. The "Existing Patterns to Reuse" section citations were re-verified against current `FrameworkCharacter.cpp`/`.h` (line numbers drift after recent commits, e.g. the interact-closest-collision-point change) — see corrections inline below.

Week 1 (TASK-031 Random Drops + TASK-030 Bonfire Chest) establishes the loot table and chest spawn logic. Week 2 (TASK-035 Accessories) builds the stat-modifier plumbing that makes gear items meaningful. Grouped because accessories need the gear-stat system to function — without it, equipping items does nothing. The gear-stat system extends the existing `TMap<FName,float>` modifier pattern into a generic `StatModifiers` map, reusing the same add/remove/sum infrastructure.

**Design-vault alignment note:** `WaveTouch Studio/Projects/Cozno Project/Game Design/World Interaction/Accessories.md` (MECHANIC 055) specifies Accessories as a **purely cosmetic system with no gameplay effect** — mesh attaches to a socket/bone, multiple non-conflicting slots, appearance saved with player state. It does not describe stat modifiers anywhere. This plan's TASK-035 (gear-stat system: `StatModifications` map, `AddStatModifier`/`RemoveStatModifier`) is a **different, broader mechanic than what the design doc specifies** — either the design doc is stale relative to intent, or this plan is scope-creeping cosmetic accessories into a stat-gear system. Flagging rather than silently resolving; confirm intended scope before implementing Week 2.

Similarly, `WaveTouch Studio/Projects/Cozno Project/Game Design/Systems/Inventory & Chest.md` (MECHANIC 021, "Bonfire Chest") describes a **player storage container** — deposit/withdraw UI, chest inventory persists separately from world reset, side-by-side inventory transfer. This plan's `UBonfireChest` component is a **loot-roll dispenser** (holds a `LootTable`, rolls and spawns an item on open, single-use via `bHasLoot`) — conceptually closer to a loot chest / treasure chest than the design doc's storage-and-transfer mechanic. Same name, different behavior; flagging for scope confirmation.

## Existing Patterns to Reuse

- **State machine** (`Source/FrameworkTest/FrameworkCharacter.cpp:144` `Tick()`, early-return chain starting `L152`): `Tick()` priority early-return chain — `if (bIsX) { UpdateX(DeltaTime); return; }` (e.g. `bIsGrappling` at L152, `bIsLedgeHanging` at L159). *(Corrected — original citation `~L313-320` was drifted; `Tick()` starts at L144.)* New dodge state slots in at top level before `IsFalling()` branch (L166).
- **Modifier pattern** (`Source/FrameworkTest/FrameworkCharacter.h:410` `FallDamageModifiers`; `FrameworkCharacter.cpp:952-965` `AddFallDamageModifier`/`RemoveFallDamageModifier`/`GetFallDamageMultiplier`). *(Corrected — original citation `~L730-760` was drifted; the three functions are at L952, L957, L962.)* Extended to generic `StatModifiers` for gear.
- **Blueprint hooks** (`Source/FrameworkTest/FrameworkCharacter.h:288` `OnGrappleMiss()`; `GrappleAnchor.h:22` `OnHighlightChanged(bool)`): `UFUNCTION(BlueprintImplementableEvent)` — C++ fires, BP overrides. *(Corrected — original citation `L287` was the `UFUNCTION` macro line, not the function declaration; declaration is L288.)* New hooks: `OnStaminaChanged()`, `OnDodgeIFrameChanged()`, `OnStatModified()`, `OnItemEquipped()`, `OnItemUnequipped()` (unverified — none of these exist yet in `FrameworkCharacter.h`).
- **Debug display** (`Source/FrameworkTest/FrameworkCharacter.cpp`): `GEngine->AddOnScreenDebugMessage(key, ...)` with stable keys — key 7725 at L948 (`UpdateProfileDebugDisplay`), keys 7723/7726 at L1051/L1055/L1060 (`UpdateMechanicDebugDisplay`), key 7724 at L1071 (`UpdateHealthDebugDisplay`). *(Corrected — original citation `~L1064` only matched the health-display function; the other keys live in different functions at different lines.)* Extended to show stamina/dodge/gear state.
- **Pickup state machine** (`Source/FrameworkTest/PickupObject.h:17-24` `EPickupState` enum; `L207` `bIsCarried`; `L210` `PickupState`) — verified, unchanged. Reused for accessory equip/unequip flow.
- **Interact detection** (`Source/FrameworkTest/FrameworkCharacter.h:383` `bCanInteract`; `L386` `CurrentInteractable`) — verified, unchanged.

## Per-Task Breakdown

### Task: TASK-031 Random Drops

Design reference: `WaveTouch Studio/Projects/Cozno Project/Game Design/Systems/Random Drops.md`, MECHANIC 022 — "Random Drop System." Design's Unreal engine note: "Drop table as a TArray of FStructs on the enemy actor or a DataAsset. FMath::RandRange for roll." This plan's `TArray<FDropEntry>` + `FMath::RandRange` cumulative-weight roll matches that shape directly.

- **Header additions** (`Source/FrameworkTest/LootTable.h` — new file):
```cpp
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LootTable.generated.h"

USTRUCT(BlueprintType)
struct FDropEntry
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ItemID;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DropChance = 1.0f; // 0-1 range
};

UCLASS(Blueprintable)
class FRAMEWORKTEST_API ALootTable : public AActor
{
    GENERATED_BODY()
    
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
    TArray<FDropEntry> Drops;
    
    UFUNCTION(BlueprintCallable, Category = "Loot")
    FName RollDrop() const;
};
```

- **Source additions** (`Source/FrameworkTest/LootTable.cpp`):
```cpp
#include "LootTable.h"
#include "Random.h"

FName ALootTable::RollDrop() const
{
    float TotalChance = 0.f;
    for (const FDropEntry& Entry : Drops)
    {
        TotalChance += Entry.DropChance;
    }
    
    if (TotalChance <= 0.f)
    {
        return NAME_None;
    }
    
    float Roll = FMath::RandRange(0.f, TotalChance);
    float Cumulative = 0.f;
    
    for (const FDropEntry& Entry : Drops)
    {
        Cumulative += Entry.DropChance;
        if (Roll <= Cumulative)
        {
            return Entry.ItemID;
        }
    }
    
    return NAME_None;
}
```

- **Integration point**: Call `RollDrop()` from any actor that should spawn loot (bonfire chest, enemy death, etc.). Expose a Blueprint callable `SpawnLoot(FName ItemID, FVector Location)` that spawns the corresponding pickup actor.

- **Blueprint hooks**: None required — pure C++ logic.

### Task: TASK-030 Bonfire Chest

Design reference: `WaveTouch Studio/Projects/Cozno Project/Game Design/Systems/Inventory & Chest.md`, MECHANIC 021 — see scope-mismatch note in Context above. Design describes deposit/withdraw storage with a save-persistence requirement ("chest contents saved separately from world reset data" per the Unreal engine note); this plan's single-use loot-roll `UBonfireChest` does not implement storage, deposit/withdraw, or a save-slot split. If the intent is MECHANIC 021 as specified, this task needs a different design (inventory UI + SaveGame slot); if the intent is a loot-dispensing chest, it should probably not be named "Bonfire Chest" to avoid colliding with the design doc's term.

- **Header additions** (`Source/FrameworkTest/BonfireChest.h` — new file):
```cpp
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BonfireChest.generated.h"

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class FRAMEWORKTEST_API UBonfireChest : public UActorComponent
{
    GENERATED_BODY()
    
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chest")
    ALootTable* LootTable = nullptr;
    
    UPROPERTY(BlueprintReadOnly, Category = "Chest")
    bool bHasLoot = false;
    
    UFUNCTION(BlueprintCallable, Category = "Chest")
    void OpenChest();
    
    UFUNCTION(BlueprintCallable, Category = "Chest")
    FName GetNextDrop();
    
    UFUNCTION(BlueprintImplementableEvent, Category = "Chest")
    void OnChestOpened();
};
```

- **Source additions** (`Source/FrameworkTest/BonfireChest.cpp`):
```cpp
#include "BonfireChest.h"
#include "LootTable.h"
#include "GameFramework/Actor.h"

void UBonfireChest::OpenChest()
{
    if (!LootTable)
    {
        bHasLoot = false;
        return;
    }
    
    FName NextItem = LootTable->RollDrop();
    if (NextItem != NAME_None)
    {
        // Spawn loot actor at chest location
        // Mark as consumed
        bHasLoot = false;
        OnChestOpened();
    }
}

FName UBonfireChest::GetNextDrop()
{
    if (!LootTable || !bHasLoot)
    {
        return NAME_None;
    }
    
    return LootTable->RollDrop();
}
```

- **Integration point**: Hook into existing interaction flow — when player interacts with bonfire, call `OpenChest()`. Reuse `AFrameworkCharacter::bCanInteract` and `CurrentInteractable` pattern (`FrameworkCharacter.h:383,386` — verified) for trigger detection. Note: interact detection recently changed to use closest-collision-point rather than actor origin (commit `9c07f94`, "Interact: use closest collision point, not actor origin") — worth checking `UpdateInteractDetection()` (called from `Tick()` at `FrameworkCharacter.cpp:147`) before wiring the chest trigger, in case the collision-point change affects how a chest's interactable bounds register (unverified — did not trace the full diff of that commit).

- **Blueprint hooks**: `OnChestOpened()` — fire for VFX/sound.

### Task: TASK-035 Accessories (Gear-Based Stat System Extension)

Design reference: `WaveTouch Studio/Projects/Cozno Project/Game Design/World Interaction/Accessories.md`, MECHANIC 055 — see scope-mismatch note in Context above. Design's Unreal engine note: "SkeletalMesh socket attachment. Accessory as a spawned static mesh actor attached to the character's socket" — i.e., the design-doc version of this system is purely visual/attachment-based and has no `StatModifications` map, no `Equip`/`Unequip` stat-mutation calls, and no dependency on the fall-damage-modifier pattern. This plan implements a stat-modifier gear system instead; treat the code below as a distinct mechanic from MECHANIC 055 until the scope question above is resolved.

- **Header additions** (`Source/FrameworkTest/AccessoryItem.h` — new file):
```cpp
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AccessoryItem.generated.h"

UCLASS(Blueprintable)
class FRAMEWORKTEST_API AAccessoryItem : public AActor
{
    GENERATED_BODY()
    
public:
    // Stat modifiers this accessory provides (FName -> float delta)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    TMap<FName, float> StatModifications;
    
    // Equip/unequip hooks
    UFUNCTION(BlueprintCallable, Category = "Accessory")
    void Equip(ACharacter* Carrier);
    
    UFUNCTION(BlueprintCallable, Category = "Accessory")
    void Unequip(ACharacter* Carrier);
    
    UFUNCTION(BlueprintImplementableEvent, Category = "Accessory")
    void OnEquipped();
    
    UFUNCTION(BlueprintImplementableEvent, Category = "Accessory")
    void OnUnequipped();
};
```

- **Source additions** (`Source/FrameworkTest/AccessoryItem.cpp`):
```cpp
#include "AccessoryItem.h"
#include "FrameworkCharacter.h"

void AAccessoryItem::Equip(ACharacter* Carrier)
{
    if (!Carrier) return;
    
    AFrameworkCharacter* Char = Cast<AFrameworkCharacter>(Carrier);
    if (!Char) return;
    
    // Add all stat modifiers from this accessory
    for (auto& Pair : StatModifications)
    {
        Char->AddStatModifier(Pair.Key, Pair.Value);
    }
    
    OnEquipped();
}

void AAccessoryItem::Unequip(ACharacter* Carrier)
{
    if (!Carrier) return;
    
    AFrameworkCharacter* Char = Cast<AFrameworkCharacter>(Carrier);
    if (!Char) return;
    
    // Remove all stat modifiers from this accessory
    for (auto& Pair : StatModifications)
    {
        Char->RemoveStatModifier(Pair.Key);
    }
    
    OnUnequipped();
}
```

- **Integration point**: Hook into existing pickup flow — when player picks up an accessory, call `Equip()`. When unequipped (via inventory UI or drop), call `Unequip()`. Reuse `APickupObject::ToggleCarry()` (`PickupObject.cpp:63` — verified) and carrier pattern for equip/unequip triggers.

- **Blueprint hooks**: `OnEquipped()` / `OnUnequipped()` — fire for VFX/sound/equip animation triggers.

## File Changes Summary

| File | Changes |
|------|---------|
| `Source/FrameworkTest/LootTable.h` | New — drop entry struct, loot table actor |
| `Source/FrameworkTest/LootTable.cpp` | New — weighted random roll logic |
| `Source/FrameworkTest/BonfireChest.h` | New — chest component with loot table reference |
| `Source/FrameworkTest/BonfireChest.cpp` | New — open/roll logic |
| `Source/FrameworkTest/AccessoryItem.h` | New — accessory actor with stat modification map |
| `Source/FrameworkTest/AccessoryItem.cpp` | New — equip/unequip logic |
| `Source/FrameworkTest/FrameworkCharacter.h` | Extend `StatModifiers` TMap, add `AddStatModifier()`/`RemoveStatModifier()`/`GetModifierSum()` (unverified — depends on a prior stamina/dodge plan landing this scaffolding first; not present in current `FrameworkCharacter.h`) |
| `Source/FrameworkTest/FrameworkCharacter.cpp` | Integrate modifier sums into profile field reads |

## Verification

1. **Compile check**: All new files compile — `LootTable`, `BonfireChest`, `AccessoryItem` plus extended `FrameworkCharacter`.
2. **Editor check**: 
   - Place `ALootTable` in level, populate `Drops` array with test entries (e.g., "HealthPotion" 0.7, "Gold" 0.3, "AccessoryRing" 0.1)
   - Place `UBonfireChest` on bonfire actor, assign the loot table reference
   - Place `AAccessoryItem` in level, configure `StatModifications` map (e.g., "WalkSpeed" +50, "MaxStamina" +20)
3. **Play-test drops**: 
   - Trigger chest interaction — verify single drop spawns per roll
   - Verify `bHasLoot` flips to false after opening
   - Re-trigger — verify no loot spawns (chest empty)
   - Test edge case: empty drops array returns `NAME_None` without crashing
4. **Play-test accessories**: 
   - Pick up accessory — verify `AddStatModifier()` fires, `OnStatModified` hooks trigger
   - Verify profile fields (WalkSpeed, MaxStamina) reflect modified values
   - Unequip accessory — verify `RemoveStatModifier()` fires, values revert to base
   - Confirm `OnEquipped()`/`OnUnequipped()` BP hooks fire for VFX

## Open Questions

1. **Accessories scope**: this plan builds a stat-modifier gear system; the design vault (`Accessories.md` MECHANIC 055) specifies cosmetic-only, no gameplay effect. Confirm which is intended before Week 2 starts — implementing the wrong one wastes the week.
2. **Bonfire Chest naming/scope**: this plan builds a single-use loot-roll dispenser; the design vault (`Inventory & Chest.md` MECHANIC 021) specifies a persistent deposit/withdraw storage container. Confirm which is intended, and whether both are needed under different names.
3. Otherwise, all systems reuse existing patterns: bool flags + Tick early-return chain, `TMap<FName,float>` modifiers, `BlueprintImplementableEvent` hooks, debug display. No new architecture invented. Gear-stat system integrates with existing fall damage modifier infrastructure — same add/remove/sum pattern, just broader scope.
