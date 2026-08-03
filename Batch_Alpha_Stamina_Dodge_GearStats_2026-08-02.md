# Batch Alpha: Stamina / Dodge Roll (i-frames) / Gear-Based Stats

**Date:** 2026-08-02
**By:** deedlyjrment (implemented by Claude)
**Source of plan:** `Execute.md`
**Files changed:** `Source/FrameworkTest/FrameworkCharacter.h`, `Source/FrameworkTest/FrameworkCharacter.cpp`

## What was built

### 1. Stamina
- `MaxStamina` / `CurrentStamina` / `StaminaDrainRate` / `StaminaRechargeRate` (all tunable, `EditAnywhere`).
- `bEnableSprintDrain` (default **true**) — sprinting now drains stamina automatically via `StartCustomAction()`/`EndCustomAction()`, wired into `SetSprinting()`. Sprint is **not** blocked at 0 stamina — no design doc specifies lockout behavior, so it currently just sits at 0 and recharges when you stop.
- `StartCustomAction()`/`EndCustomAction()` are public `BlueprintCallable` — any other action (future) can cost stamina by calling these.
- `OnStaminaChanged(float NewValue, float Max)` — `BlueprintImplementableEvent`, fires only on actual value change.
- Runs every Tick via `UpdateStamina()`, called right before `UpdateGrappleTargeting()`.
- Debug display: on-screen key **7727**, `Stamina: X / Y`.

### 2. Dodge Roll with i-frames
- New top-level Tick early-return (after ledge-hang, before the falling branch) — dodge takes full priority like grapple/ledge-hang.
- `RequestDodge(float ForwardAxis, float RightAxis)` — call from Blueprint's `IA_Dodge` **Started** event, passing the current `IA_Move` axis values. Direction is computed as `(CameraForward * ForwardAxis) + (CameraRight * RightAxis)`, yaw-only (pitch/roll stripped), captured once at press time — not re-read during the roll. If both axes are zero, dodges along the character's current forward vector.
- Guarded against re-trigger while already dodging, grappling, ledge-hanging, or airborne (`Movement->IsFalling()`).
- Movement: **velocity-based**, not root motion. `DodgeSpeed = DodgeRollDistance / DodgeRollDuration`, applied to `CharacterMovement->Velocity` every tick of the roll. Simpler and doesn't require an anim asset to move correctly — root motion is a valid future upgrade if you want the anim to fully drive displacement instead.
- Capsule shrinks to `DodgeCrouchHalfHeight` (default 44, tunable) for the duration via `SetCapsuleHalfHeight(..., true)` (updates overlaps), restored to the pre-dodge height in `EndDodge()`.
- `DodgeMontage` (optional `UAnimMontage*`) — plays automatically if set. Left null by default; point it at something built from the animations already in `Content/Dodge_and_Evade_Anims/` (e.g. `Dodge_Roll_Forward`) or the ARPG pack's dodge anims.
- **i-frames**: `DodgeIFrameDuration` is a separate tunable from `DodgeRollDuration` (can be shorter than the roll). `IsDodgeInvincible()` is `bIsDodging && DodgeElapsedTime < DodgeIFrameDuration`. Wired into `ApplyFallDamage_Implementation()` — no fall damage while invincible.
- `OnDodgeIFrameChanged(bool bActive)` — `BlueprintImplementableEvent`, fires true on dodge start, false the instant the i-frame window ends (handles the edge case where the roll ends before the i-frame window would have, too).
- **Not done — noted for later:** there is no enemy damage / hit-detection system anywhere in this codebase yet (only fall damage exists). `IsDodgeInvincible()` is public and ready — whenever combat/enemy-hit code is added, it must check this before applying damage. This is a TODO comment directly above the function in `FrameworkCharacter.h`.
- Debug display: `Mechanic State: Dodging` / `Dodging (i-frames)` while active, same key 7723 as other states.

### 3. Gear-Based Stats
- `TMap<FName, float> StatModifiers`, `AddStatModifier`/`RemoveStatModifier`/`GetModifierSum`, `OnStatModified` — generalizes the existing `FallDamageModifiers` pattern.
- Wired into all 3 real profile-driven read sites found in the codebase (grepped, not guessed):
  - `SetMovementProfile()`: `JumpZVelocity` and `MaxAcceleration`.
  - `ApplyWalkSpeed()`: `WalkSpeed`/`SprintSpeed`.
- Naming convention for `StatName`: use the exact strings `"WalkSpeed"`, `"JumpZVelocity"`, `"MaxAcceleration"` to hit these. Other names are stored but currently unused — add a read site if you need one.
- `AddStatModifier`/`RemoveStatModifier` call `SetMovementProfile(CurrentProfileIndex)` to immediately reapply all three fields with the modifier folded in (not just `ApplyWalkSpeed()`, since jump/accel also live in `SetMovementProfile`).

## Compile status

UnrealHeaderTool ran clean (`UHT processed FrameworkTestEditor in 259s, 3 generated files written`) — all `UPROPERTY`/`UFUNCTION`/reflection macros are valid. The actual C++ compile step was blocked by **Live Coding being active in the open editor** (`Unable to build while Live Coding is active`). Two ways to finish verifying:
1. In the editor: **Ctrl+Alt+F11** (Live Coding compile), or
2. Close the editor, then rerun `Build.bat FrameworkTestEditor Win64 Development -project=FrameworkTest.uproject`.

Neither was run to completion — do this before relying on the code.

## Manual step required (not code)

`IA_Dodge` Input Action does **not exist** in `Content/Input/Actions/`. Create it and bind it (matches the existing `IA_Jump`/`IA_Sprint`/`IA_Grapple` pattern):
1. Create `IA_Dodge` (Digital/bool) in `Content/Input/Actions/`.
2. Add it to `IMC_Default` (or whichever context the player pawn uses), bound to **Left Alt**.
3. In the Character Blueprint's Enhanced Input event graph, on `IA_Dodge` **Started**, read the current `IA_Move` axis value and call `RequestDodge(Forward, Right)`.
4. (Optional) Assign a `UAnimMontage` to the new `Dodge Montage` property in the Character BP defaults.

## How to test in-editor

1. Turn on mechanic debug text (`bShowMechanicDebugText`) to see live state (keys 7723 Mechanic State, 7727 Stamina).
2. **Stamina**: hold sprint — watch stamina drain (20/s default), release — watch it recharge (15/s default). Confirm `OnStaminaChanged` fires if you hook a UI widget to it.
3. **Dodge**: press Left Alt while grounded and not grappling/ledge-hanging — character should snap-move ~500 units (default) over 0.4s in the direction of current move input relative to camera, capsule visibly shrinks, `Mechanic State` reads `Dodging (i-frames)` for the first 0.35s. Walk off a ledge, fall, dodge mid-air should be blocked (only works grounded). Take fall damage during the dodge window — confirm no damage while i-frames active, confirm damage resumes right after.
4. **Gear stats**: from a Blueprint or console, call `AddStatModifier("WalkSpeed", 100.0)` — confirm walk speed increases immediately (fires `SetMovementProfile` refresh). Call `RemoveStatModifier("WalkSpeed")` — confirm it reverts. Repeat for `"JumpZVelocity"` and `"MaxAcceleration"`.

## Known gaps / deferred (flagged, not silently skipped)

- No design-vault doc backs Stamina or Gear-Stats (per `Execute.md`'s own Open Question) — built anyway per direct instruction, values are placeholder-tunable, not design-sourced.
- No sprint-stamina lockout (sprint doesn't get cut off at 0 stamina) — add if wanted.
- Dodge uses velocity, not root motion — swap to root motion later if the animation needs to fully own displacement.
- `IsDodgeInvincible()` has no consumer yet outside fall damage — no enemy/combat damage system exists in this codebase to wire it into.
