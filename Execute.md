# Plan: Stamina / Dodge Roll (i-frames) / Gear-Based Stat System

## Context

**Implementation status (verified 2026-07-25 against current `Source/FrameworkTest/FrameworkCharacter.h`/`.cpp`): none of this is built yet.** Grepping the current source for `bIsDodging`, `RequestDodge`, `Stamina`, `CustomAction`, and `StatModifiers` returns zero matches anywhere in `FrameworkCharacter.h` or `FrameworkCharacter.cpp`. This whole document is still a plan, not a status report — Stamina, Dodge Roll, and the generic gear-stat system described below do not exist in code. The only real precedent in the codebase is the narrow `FallDamageModifiers` `TMap<FName,float>` (see below), which this plan proposes generalizing. This is independently confirmed by a later, more rigorously-verified sibling doc, `WORKING/Batches/13 breakable_slam_fury.md`: *"Verified against the codebase: none of Stamina, Dodge Roll, or a generic `StatModifiers` map from `1 Soulslike.md` have actually been implemented — `FrameworkCharacter.h`/`.cpp` currently has no `StartCustomAction`/`EndCustomAction`, no dodge state, and only the narrow `FallDamageModifiers` map."`

**Game Design vault grounding:** checked `WaveTouch Studio/Projects/Cozno Project/Game Design/Systems/` (Bonfire Save & Reset, Inventory & Chest, Random Drops, Size & Ability Inheritance) and `.../Game Design/Character Abilities/` (Badger.md, Raccoon.md, Squirrel.md). **No design doc for a generic player Stamina system or a Gear-Based Stat system exists anywhere in the vault** — this plan predates that vault structure and there is nothing to align terminology against for Systems 1 and 3 below; treat their names/values as invented-for-this-plan, not design-sourced. The closest design-adjacent concept is `Badger.md` MECHANIC 047 ("Badger Sprint & Roll / Roll Attack"), which specifies a roll with "a brief invincibility window" and an Unreal engine note — *"Invincibility via custom damage immunity flag on the character. Sprint state checked at roll input time to determine which animation and hitbox to use."* — but that mechanic is Badger-specific (sprint-roll-attack with a hitbox), not a generic dodge-roll-with-i-frames system for the base player character as scoped in System 2 below. No stamina resource or gear-stat system is referenced by any character ability doc either. Treat System 2's i-frame/dodge shape as loosely precedented by MECHANIC 047's invincibility-flag idiom, and Systems 1 and 3 as unprecedented in the design vault.

Three systems bundled into one plan because they share plumbing: stamina drains into both sprint and custom actions, dodge roll gates on grounded state and fires i-frame hooks, gear stats ride the same `TMap<FName,float>` modifier pattern as fall damage. Built as a foundation — extend by adding items to arrays or tweaking profile values, no new abstractions required. All three slot into existing Tick() early-return chain without restructuring.

## Existing Patterns to Reuse

- **State machine** (`Source/FrameworkTest/FrameworkCharacter.cpp:144-166`, `AFrameworkCharacter::Tick`): priority early-return chain — `if (bIsGrappling) { ... return; }` (L152), `if (bIsLedgeHanging) { ... return; }` (L159), then `if (Movement->IsFalling())` (L166). Dodge would slot in at the same top level, before the `IsFalling()` branch. Wall slide/glide remain non-returning updates inside the falling branch. *(Verified 2026-07-25 — corrects a stale `~L313-320` citation from the original draft; recent commits shifted line numbers.)*
- **Modifier pattern** (`Source/FrameworkTest/FrameworkCharacter.h:410` declares `TMap<FName, float> FallDamageModifiers`; impl at `FrameworkCharacter.cpp:952-969` — `AddFallDamageModifier` (L952), `RemoveFallDamageModifier` (L957), `GetFallDamageMultiplier` (L962, sums modifiers + 1.f, clamped `>= 0`)). Self-contained per-system. Proposed here to extend to a generic `StatModifiers` map — **this generalization does not exist yet**; only the narrow fall-damage-specific map does. *(Verified 2026-07-25 — corrects a stale `~L730-760` citation.)*
- **Blueprint hooks** (`Source/FrameworkTest/FrameworkCharacter.h:287-288`): `UFUNCTION(BlueprintImplementableEvent) void OnGrappleMiss();` — C++ fires, BP overrides. New hooks proposed here (`OnStaminaChanged()`, `OnDodgeIFrameChanged()`, `OnStatModified()`) do not exist yet.
- **Debug display** (`Source/FrameworkTest/FrameworkCharacter.cpp:995-1072`, `UpdateMechanicDebugDisplay()` / `UpdateHealthDebugDisplay()`): `GEngine->AddOnScreenDebugMessage(key, ...)` with stable keys 7723 (mechanic state), 7724 (health), 7725 (profile), 7726 (secondary line). Proposed extension to show stamina/dodge state would need a new stable key (e.g. 7727+) to avoid colliding with these.

## Per-Task Breakdown

### System 1: Stamina / Endurance

**(unverified / not implemented)** — no `Stamina`-named member exists anywhere in `FrameworkCharacter.h`/`.cpp` as of 2026-07-25. Everything below is proposed, not built.

- **Header additions** (`Source/FrameworkTest/FrameworkCharacter.h`, private section):
```cpp
// --- Stamina ---
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
float MaxStamina = 100.f;

UPROPERTY(BlueprintReadOnly, Category = "Stamina")
float CurrentStamina = 100.f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = "0.0"))
float StaminaDrainRate = 20.f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina", meta = (ClampMin = "0.1"))
float StaminaRechargeRate = 15.f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stamina")
bool bEnableSprintDrain = false;

UFUNCTION(BlueprintCallable, Category = "Stamina")
void StartCustomAction();

UFUNCTION(BlueprintCallable, Category = "Stamina")
void EndCustomAction();

UFUNCTION(BlueprintImplementableEvent, Category = "Stamina")
void OnStaminaChanged(float NewValue, float Max);

// Private state
bool bIsStaminaDraining = false;
float StaminaRechargeTimer = 0.f;
float LastFiredStamina = -1.f;
```

- **Source additions** (`Source/FrameworkTest/FrameworkCharacter.cpp`):
  - In `Tick()`, after existing state early-returns but before `UpdateGrappleTargeting`:
    ```cpp
    UpdateStamina(DeltaTime);
    ```
  - `UpdateStamina(DeltaTime)`: drain when `bIsStaminaDraining`, recharge otherwise. Clamp to `[0, MaxStamina]`. Fire `OnStaminaChanged` only on actual change (track `LastFiredStamina`).
  - `StartCustomAction()`/`EndCustomAction()`: toggle `bIsStaminaDraining`.
  - Integrate with sprint: in `SetSprinting()` (`FrameworkCharacter.h:178` / impl `FrameworkCharacter.cpp:935`), if `bEnableSprintDrain`, call `StartCustomAction()` on true / `EndCustomAction()` on false. Glide already has `GlideTimeRemaining` — leave separate but fire same `OnStaminaChanged` hook for unified UI gauge.

- **Integration point**: `SetSprinting()` (`FrameworkCharacter.h:178`, impl `FrameworkCharacter.cpp:935`), stamina drain gates sprint when enabled.

- **Blueprint hooks**: `OnStaminaChanged(float, float)` — fire on threshold crossings for UI/sound.

### System 2: Dodge Roll with I-Frames

**(unverified / not implemented)** — no `bIsDodging`, `RequestDodge`, or dodge-related member exists anywhere in `FrameworkCharacter.h`/`.cpp` as of 2026-07-25 (confirmed by grep; also independently confirmed by `WORKING/Batches/13 breakable_slam_fury.md`'s Context section). Everything below is proposed, not built. Design-vault precedent for the i-frame *shape* (not the generic system) is `Badger.md` MECHANIC 047's invincibility flag — see Context above.

- **Header additions** (`Source/FrameworkTest/FrameworkCharacter.h`):
```cpp
// --- Dodge Roll ---
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Dodge")
float DodgeRollDuration = 0.4f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Dodge")
float DodgeRollSpeed = 800.f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement Tuning|Dodge", meta = (ClampMin = "0.0"))
float DodgeIFrameDuration = 0.35f;

UFUNCTION(BlueprintCallable, Category = "Movement")
void RequestDodge();

UFUNCTION(BlueprintImplementableEvent, Category = "Movement")
void OnDodgeIFrameChanged(bool bActive);

// Private state
bool bIsDodging = false;
float DodgeElapsedTime = 0.f;
float DodgeDirectionX = 0.f;
float DodgeDirectionY = 0.f;
```

- **Source additions** (`Source/FrameworkTest/FrameworkCharacter.cpp`):
  - In `Tick()`, add early-return **before** `if (Movement->IsFalling())` (currently `FrameworkCharacter.cpp:166`):
    ```cpp
    if (bIsDodging) { UpdateDodge(DeltaTime); UpdateMechanicDebugDisplay(); return; }
    ```
  - `RequestDodge()`: set `bIsDodging = true`, `DodgeElapsedTime = 0.f`, read input direction into `DodgeDirectionX/Y`. Fire `OnDodgeIFrameChanged(true)`. Guard: only if `!Movement->IsFalling()`.
  - `UpdateDodge(DeltaTime)`: increment `DodgeElapsedTime`. Apply velocity along `DodgeDirection` using `DodgeRollSpeed`. On completion, call `EndDodge()` which fires `OnDodgeIFrameChanged(false)`.
  - **I-frame integration**: In `ApplyFallDamage_Implementation()` (declared `FrameworkCharacter.h:355-356`, impl `FrameworkCharacter.cpp:972`), add check — if `bIsDodging && DodgeElapsedTime < DodgeIFrameDuration`, return early without applying damage.

- **Integration point**: `RequestDodge()` hooks into existing IA_Dodge Blueprint input (this input action was not verified to exist — confirm before wiring). I-frame check slots into `ApplyFallDamage_Implementation()`.

- **Blueprint hooks**: `OnDodgeIFrameChanged(bool)` — fire on i-frame toggle for VFX/shader hooks.

### System 3: Gear-Based Stat System

**(unverified / not implemented)** — no `StatModifiers` member exists anywhere in `FrameworkCharacter.h`/`.cpp` as of 2026-07-25; only the narrow, fall-damage-specific `FallDamageModifiers` map exists (`FrameworkCharacter.h:410`). Everything below is proposed, not built. No design-vault doc covers gear stats (see Context above).

- **Header additions** (`Source/FrameworkTest/FrameworkCharacter.h`):
```cpp
// --- Gear-Based Stats ---
UPROPERTY(BlueprintReadWrite, Category = "Stats")
TMap<FName, float> StatModifiers;

UFUNCTION(BlueprintCallable, Category = "Stats")
float GetModifierSum(FName StatName) const;

UFUNCTION(BlueprintCallable, Category = "Stats")
void AddStatModifier(FName StatName, float Value);

UFUNCTION(BlueprintCallable, Category = "Stats")
void RemoveStatModifier(FName StatName);

UFUNCTION(BlueprintImplementableEvent, Category = "Stats")
void OnStatModified(FName StatName, float NewTotalValue);
```

- **Source additions** (`Source/FrameworkTest/FrameworkCharacter.cpp`):
  - `GetModifierSum(FName)`: return `StatModifiers.Find(StatName)->Value` or 0.f if absent.
  - `AddStatModifier(FName, float)`: `StatModifiers.AddOrReplace(StatName, Value)`. Fire `OnStatModified`.
  - `RemoveStatModifier(FName)`: `StatModifiers.Remove(StatName)`. Fire `OnStatModified` with 0.
  - **Integration with existing systems**: In `ApplyWalkSpeed()` (declared `FrameworkCharacter.h:401`, impl `FrameworkCharacter.cpp:918`), replace direct `Profile.WalkSpeed` reads with `Profile.WalkSpeed + GetModifierSum("WalkSpeed")`. Same for `JumpZVelocity`, `MaxAcceleration`, etc. — inline at each read site. No FName→field lookup table. Note: `WORKING/Batches/13 breakable_slam_fury.md` (Week 12) independently proposes a near-identical but narrower `SpeedModifiers` map for its Fury system, explicitly declining to assume this generic `StatModifiers` system exists — confirming it is still unbuilt as of that later doc too.

- **Integration point**: `ApplyWalkSpeed()` (`FrameworkCharacter.h:401`, impl `FrameworkCharacter.cpp:918`), plus any other profile field read sites (jump, acceleration, fall damage multiplier). Gear modifiers apply transparently.

- **Blueprint hooks**: `OnStatModified(FName, float)` — fire on any modifier change for UI/reaction.

## File Changes Summary

| File | Changes |
|------|---------|
| `Source/FrameworkTest/FrameworkCharacter.h` | Add stamina/dodge/gear structs, state members, BP event declarations |
| `Source/FrameworkTest/FrameworkCharacter.cpp` | Add `UpdateStamina`, `UpdateDodge`, `GetModifierSum`; integrate i-frame check into `ApplyFallDamage_Implementation`; route profile reads inline with `Profile.X + GetModifierSum()` |

## Verification

1. **Compile check**: All new methods have matching .h/.cpp declarations. No missing implementations.
2. **Editor check**: Character BP compiles — new `BlueprintCallable` and `BlueprintImplementableEvent` functions visible in graph.
3. **Play-test stamina**: Sprint drains when `bEnableSprintDrain=true`, recharges when grounded. Glide uses own `GlideTimeRemaining` but fires same `OnStaminaChanged` — confirm UI gauge responds to both.
4. **Play-test dodge**: Press dodge input while grounded → character rolls in input direction for `DodgeRollDuration`. No damage during i-frame window (`DodgeIFrameDuration`). Air dodge blocked.
5. **Play-test gear stats**: `AddStatModifier("WalkSpeed", 100.f)` — verify `Profile.WalkSpeed + GetModifierSum("WalkSpeed")` returns base + 100. Confirm sprint speed reflects change. Remove modifier — confirm reverts to base.

## Open Questions

None on the mechanics themselves — all three systems reuse existing patterns: bool flags + Tick early-return chain, `TMap<FName,float>` modifiers, `BlueprintImplementableEvent` hooks, debug display. No new architecture invented. Open question added on verification: since no Game Design vault doc backs Stamina or Gear-Based Stats, confirm with design whether these two systems are still wanted as scoped here, or whether they should be deferred until a design doc exists (as `13 breakable_slam_fury.md` did for its own narrower systems).
