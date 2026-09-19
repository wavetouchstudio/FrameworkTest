# Mechanics Inventory — Unreal C++ → S&box C# Migration Reference

Source: `Source/FrameworkTest/`. Each entry lists the exact tunable values, state machine, and behavior needed for a 1:1 port. Values are C++ defaults; treat as the baseline unless a level overrides them via the editor (not captured here — check placed instances too).

---

## 1. Player Character (`FrameworkCharacter.h/.cpp`) — `AFrameworkCharacter`

### 1a. Movement Profiles (`FCharacterMovementProfile` struct, swappable at runtime via `SetMovementProfile(int32)`)
Array of profiles (`MovementProfiles`), active one selected by `CurrentProfileIndex`. Fields:
- CapsuleScale = 1.0, Mass = 80
- WalkSpeed = 550, SprintSpeed = 850
- JumpZVelocity = 1400, DoubleJumpZVelocity = 1400
- bEnableWallJump = true, bEnableWallSlide = true, bWallJumpResetsDoubleJump = true, bEnableDoubleJump = true
- AirControl = 0.3, GravityScale = 4.0, MaxAcceleration = 1500, BrakingDecelerationWalking = 2000
- RotationYawRate = 750, bOrientRotationToMovement = true, bUseControllerRotationYawSetting = false
- WallSlideMinFallSpeed = 200, WallSlideSpeed = 150, WallApproachDotThreshold = 0.3, WallSlideMaxDuration = 0 (0/neg = unlimited)
- WallJumpSpeed = 1300, WallJumpAngle = 65° (from horizontal)
- bEnableCarry = true, bEnablePlacementMode = true
- bEnableGlide = true, GlideMaxFallSpeed = 75, GlideZInterpSpeed = 8, GlideForwardSpeed = 800, GlideMaxDuration = 3, GlideRechargeRate = 1/sec, GlideLaunchSpeed = 300, GlideLaunchAngle = 35°
- WallTraceChannel = Visibility, WallTraceDistance = 60
- CameraArmLength = 300, CameraSocketOffset = 0, CameraFOV = 90, bEnableCameraLag = true, CameraLagSpeed = 10

### 1b. Double Jump
- `RequestJump()` — called from input Started.
- `DoubleJumpDelay` = 0.25s minimum gap after first jump before double jump can trigger.
- `CoyoteTime` = 0.15s — jump still counts as grounded jump within this window after leaving ground.
- State: `bDoubleJumpUsed`, `bJumpedThisAirtime`, `FirstJumpTime`, `LeftGroundTime`.
- Debug: `bDebugInfiniteJump` — bypasses once-per-airtime limit.

### 1c. Sprint
- `SetSprinting(bool)` — called on input Started(true)/Completed(false).
- Applies WalkSpeed vs SprintSpeed from active profile (`ApplyWalkSpeed()`).

### 1d. Wall Slide
- Trace forward `WallTraceDistance` along `WallTraceChannel` (`TraceForWall`).
- Triggers when falling faster than `WallSlideMinFallSpeed` and wall-approach dot ≥ `WallApproachDotThreshold`.
- Slide speed clamp = `WallSlideSpeed` (scaled by `UWallSlideSurfaceComponent.SlideSpeedMultiplier` if the wall actor has one).
- `WallSlideMaxDuration` — auto end after N seconds if > 0.
- Per-surface override component (`WallSlideSurfaceComponent`): `bAllowWallSlide`, `ApproachDotThresholdOverride` (-1 = inherit), `SlideSpeedMultiplier`.
- State: `bIsWallSliding`, `WallSlideNormal`, `WallSlideElapsedTime`. Ends via `EndWallSlide()`.

### 1e. Wall Jump
- Only if `bEnableWallJump` and currently wall-sliding.
- Launch speed = `WallJumpSpeed` (1300) at `WallJumpAngle` (65°) from horizontal, direction = wall normal blended with angle.
- `bWallJumpResetsDoubleJump` — refreshes double jump availability.

### 1f. Ledge Hang / Climb
- Detection (`TraceForLedge`): hand-height trace at `LedgeGrabHeight` (120) forward `LedgeWallTraceDistance` (60), clearance check `LedgeClearanceHeight` (40) above, then downward surface search from `LedgeSurfaceSearchHeight` (60) above hit point, forward reach `LedgeForwardReach` (40) past wall hit. Surface must be ≤ `LedgeMaxWalkableAngle` (45°) from up to count as ledge top.
- Hang position: `LedgeHangVerticalOffset` = -80 relative to surface.
- Back-out jump: `LedgeJumpAwaySpeed` (400) horizontal, `LedgeJumpZVelocity` (800) vertical.
- Climb-up: forward offset `LedgeClimbForwardOffset` (60), up offset `LedgeClimbUpOffset` (90).
- Debug draw toggle: `bDebugDrawLedgeTraces`.
- State: `bIsLedgeHanging`, `LedgeWallNormal`, `LedgeClimbTargetLocation`. Managed by `EnterLedgeHang`/`UpdateLedgeHang`/`ExitLedgeHang`.

### 1g. Glide
- Starts via `RequestJump()` pressed again mid-air (double-jump-slot reuse); ends early via `SetGliding(false)` on input Completed/Canceled.
- Caps downward speed (Velocity.Z) toward `-GlideMaxFallSpeed` (75) with interp speed `GlideZInterpSpeed` (8).
- One-time launch impulse `GlideLaunchSpeed` (300) at `GlideLaunchAngle` (35°) from horizontal on engage ("wind catches you" kick).
- Continuous forward push `GlideForwardSpeed` (800 cm/s) along forward vector.
- Stamina-like gauge: `GlideMaxDuration` (3s) drains while active, `GlideRechargeRate` (1/s) regenerates while inactive. Exposed as `GlideTimeRemaining` (BlueprintReadOnly).
- VFX hook: `GlideTrailEffect` (Niagara component), active only while gliding.
- State: `bIsGliding`, driven by `UpdateGlide/StartGlide/EndGlide`.

### 1h. Grapple
- **Two anchor types** (`AGrappleAnchor.AnchorType`, `EGrappleAnchorType`: `Pull` or `Swing`), set on the anchor actor — anchor now carries logic via this enum, not "no own logic".
- Targeting: scans within `GrappleTargetingRadius` (2000) every `GrappleScanInterval` (0.1s) for `AGrappleAnchor` actors whose direction from camera is within `GrappleTargetingDotThreshold` (0.95 dot) of camera forward. Highlighted anchor gets `OnHighlightChanged(true/false)` callback.
- `RequestGrapple()` on input Started: if an anchor is highlighted, begins pull; else fires `OnGrappleMiss()` (BP event).
- Pull speed toward anchor: `GrapplePullSpeed` (2000 cm/s). Auto-releases within `GrappleReleaseDistance` (100) of anchor (Pull anchors only).
- **Swing anchors** (pendulum): on grab, `Movement->SetMovementMode(MOVE_Falling)` (keeps gravity/velocity, so momentum carries into swing) and records `GrappleSwingRadius` = distance to anchor. `UpdateGrappleSwing` each tick strips the radial velocity component (leaving only tangential swing) and snaps position back onto the fixed-radius sphere. Released via jump/grapple input.
- Swing release: `EndGrapple()` applies `GrappleSwingReleaseBoost` (1.15×) to current swing velocity + `GrappleSwingReleaseUpKick` (300 cm/s) upward kick.
- Visuals: `GrappleCable` (Cable Component) rendered between player and anchor; `GrappleBeamEffect` Niagara beam using vector params named by `GrappleBeamStartParamName`/`GrappleBeamEndParamName` (default "BeamStart"/"BeamEnd").
- Debug draw toggle: `bDebugDrawGrappleTargeting`.
- State: `bIsGrappling`, `HighlightedAnchor`, `GrappleTargetAnchor`, `GrappleScanTimer`, `CurrentGrappleType` (`EGrappleAnchorType`), `GrappleSwingRadius`. Managed by `UpdateGrappleTargeting/UpdateGrapple/UpdateGrappleSwing/EndGrapple`.
- `AGrappleAnchor` actor: static mesh (`AnchorMesh`) + `AnchorType` enum + `OnHighlightChanged(bool)` BP event.

### 1i. Fall Damage
- Tracks `PeakFallSpeed` during fall; on landing (`Landed()` override), if peak downward speed exceeds `FallDamageSafeSpeed` (1400 cm/s), damage = `(PeakSpeed - SafeSpeed) * FallDamageRatePerUnit(0.05) * (CurrentMass / FallDamageReferenceMass(80)) * FallDamageMultiplier`.
- Runtime modifiers: `AddFallDamageModifier(FName ID, float MultiplierDelta)` / `RemoveFallDamageModifier(FName ID)` — stack additively in `FallDamageModifiers` map; `GetFallDamageMultiplier()` = `1 + sum(deltas)`, clamped ≥ 0.
- Damage application: `ApplyFallDamage(float)` is `BlueprintNativeEvent` — override in BP/subclass to hook into health; default C++ impl exists too (`ApplyFallDamage_Implementation`).

### 1j. Health (test/debug pool)
- `MaxHealth` = 100, `CurrentHealth` = 100 (BlueprintReadOnly, exposed for damage sources like `RotationHazard`).
- `bDebugHealOnZeroHealth` — if true, hitting 0 just refills to max in place (no death/respawn); if false, real death/respawn path fires (see `FrameworkGameInstance.RespawnAtLastBonfire`).
- `GetMaxHealth()` = `MaxHealth + GetModifierSum("MaxHealth")` — all reads (BeginPlay init, heal-to-max branches, debug display) go through this, not raw `MaxHealth`. See 1o.

### 1m. Stamina
- `MaxStamina` = 100, `CurrentStamina` = 100, `StaminaDrainRate` = 20/s, `StaminaRechargeRate` = 15/s.
- `GetMaxStamina()` = `MaxStamina + GetModifierSum("MaxStamina")` (see 1o) — drain/recharge clamp and `OnStaminaChanged` both use this, not raw `MaxStamina`.
- `bEnableSprintDrain` (default true) — `SetSprinting(true/false)` calls `StartCustomAction()`/`EndCustomAction()`, which set `bIsStaminaDraining`. No lockout at 0 — sprint keeps working, stamina just sits at 0 until released.
- `StartCustomAction()`/`EndCustomAction()` are `BlueprintCallable` — any other action can cost stamina by wrapping itself in these two calls.
- Ticked every frame via `UpdateStamina(DeltaTime)`; fires `OnStaminaChanged(float NewValue, float Max)` (`BlueprintImplementableEvent`) only when value actually changes (`FMath::IsNearlyEqual` guard against `LastFiredStamina`).
- Debug display: on-screen key 7727, `Stamina: X / Y`.

### 1n. Dodge Roll (i-frames)
- Input: `RequestDodge(float ForwardAxis, float RightAxis)` — bound to `IA_Dodge` (Left Alt) Started, fed the current `IA_Move` axis values.
- Direction: camera yaw-only forward/right axes (pitch/roll stripped) combined with the passed move axes, computed **once** at press time and cached in `DodgeDirection` — not re-read during the roll. If both axes are zero, dodges along the character's current forward vector.
- Top-priority Tick early-return, inserted after ledge-hang / before the falling branch — same precedence tier as grapple. Blocked while already dodging, grappling, ledge-hanging, or airborne (`Movement->IsFalling()`).
- Movement: **velocity-based**, not root motion. `DodgeSpeed = DodgeRollDistance / DodgeRollDuration` (defaults 500 / 0.4s = 1250 cm/s) applied directly to `CharacterMovement->Velocity` every tick of the roll.
- Capsule: shrinks to `DodgeCrouchHalfHeight` (44, tunable) via `SetCapsuleHalfHeight(..., true)` for the roll duration; restored to `PreDodgeCapsuleHalfHeight` in `EndDodge()`.
- Optional `DodgeMontage` (`UAnimMontage*`) — plays automatically if assigned, left null by default (no anim wired yet).
- i-frames: `DodgeIFrameDuration` (0.35s, separate tunable from `DodgeRollDuration`, can be shorter than the roll). `IsDodgeInvincible()` = `bIsDodging && DodgeElapsedTime < DodgeIFrameDuration`. Currently only consumer is `ApplyFallDamage_Implementation()` (early-return, no fall damage while invincible) — **no enemy/combat damage system exists in this codebase yet**; `IsDodgeInvincible()` is public and ready for whenever one is added.
- `OnDodgeIFrameChanged(bool bActive)` (`BlueprintImplementableEvent`) — fires true on dodge start, false the instant the i-frame window ends (independent of roll end).
- State: `bIsDodging`, `DodgeElapsedTime`, `DodgeDirection`, `PreDodgeCapsuleHalfHeight`. Managed by `UpdateDodge(DeltaTime)`/`EndDodge()`.
- Debug display: `Mechanic State: Dodging` / `Dodging (i-frames)`, same key 7723 as other mechanic states, top priority branch.

### 1o. Gear-Based Stat Modifiers
- `TMap<FName, float> StatModifiers` — generalizes the pre-existing `FallDamageModifiers` pattern (7a) to any named stat.
- `GetModifierSum(FName StatName)` — returns the map value for that name or 0. **Overwrite semantics, not additive** — `AddStatModifier` replaces any existing entry for that name (single modifier slot per stat name, not a stacking list like `FallDamageModifiers`).
- `AddStatModifier(FName StatName, float Value)` / `RemoveStatModifier(FName StatName)` — fire `OnStatModified(FName, float)` (`BlueprintImplementableEvent`), then call `SetMovementProfile(CurrentProfileIndex)` to immediately reapply `JumpZVelocity`/`MaxAcceleration`/`WalkSpeed` with the new modifier folded in.
- Special-cased stat names `"MaxHealth"` / `"MaxStamina"`: adding or removing these also shifts `CurrentHealth`/`CurrentStamina` by the delta (`FMath::Clamp` to `[0, GetMaxHealth()/GetMaxStamina()]`) — expanding a pool's max fills the current value by the same amount instead of leaving you at a lower % of the new max; shrinking clamps current down if it now exceeds the new max.
- Wired read sites (exact strings required to hit them): `"WalkSpeed"` (`ApplyWalkSpeed`), `"JumpZVelocity"` / `"MaxAcceleration"` (`SetMovementProfile`), `"MaxHealth"` (`GetMaxHealth`, see 1j), `"MaxStamina"` (`GetMaxStamina`, see 1m). Other names are stored but have no reader — add a call site if a new stat needs to matter.

### 1k. Interact System
- Detection: overlap sphere `InteractSphereRadius` (200) finds candidates implementing `InteractableInterfaceClass` (assign BPI_Interactable in BP defaults); confirmed via line trace on `InteractTraceChannel` (Visibility) up to `MaxInteractDistance` (150).
- Exposes `bCanInteract`, `CurrentInteractable` (BlueprintReadWrite, so BP can call Interact on it).
- Prompt UI: `InteractPromptWidgetClass` shown/hidden automatically based on `bCanInteract`.

### 1l. Debug/Misc
- `bShowMechanicDebugText` — on-screen text of current mechanic/state (`UpdateMechanicDebugDisplay`, `UpdateHealthDebugDisplay`, `UpdateProfileDebugDisplay`).
- `bIsHoldingObject`, `bIsInPlacementMode` — external flags set by BP/PickupObject for the debug display.
- Drop-shadow decals: `UpdateDropShadowVisibility()` toggles cached `UDecalComponent*` list — likely visibility tied to falling/airborne state.
- `OnMovementModeChanged` override — likely resets wall-slide/ledge/glide state on mode transitions.

---

## 2. Pickup / Carry / Placement / Throw (`PickupObject.h/.cpp`) — `APickupObject`

### 2a. State machine
`EPickupState`: Idle → LerpingToHold → Held → (Placing) → LerpingToDrop → Idle.

### 2b. Carry
- `ToggleCarry()` — call from Interact. `ToggleCooldown` = 0.4s min between calls (prevents double-fire, historically fixed a double-ToggleCarry bug per earlier project notes — preserve the cooldown guard exactly).
- Hold position: `CarryForwardDistance` = 150 from camera along aim; scroll range `MinCarryDistance` 60 – `MaxCarryDistance` 500 via `AdjustCarryDistance(float delta)`; eased with `CarryDistanceInterpSpeed` = 8.
- `CarryHeightOffset` = 80 (fallback only — real height comes from PlayerController camera when valid).
- `CarryRotationSpeed` = 0 (deg/s spin while held, 0 = off).
- Idle float/bob: `FloatAmplitude` = 8cm, `FloatSpeed` = 2 cycles/s (`FloatTime` accumulator).
- Pickup lerp: `PickupLerpDuration` = 0.35s, optional `PickupCurve` (fallback SmoothStep).
- Drop lerp: `DropReleaseHeight` = 30 above player root, `DropLerpDuration` = 0.25s, optional `DropCurve` (fallback SmoothStep).
- "Standing on top" pickup-block check: `StandingOnTopTolerance` = 10cm vertical tolerance (prevents picking up a block you're literally standing on, or handles it specially).
- Caches carrier's spring arm (`CachedSpringArm`) and controller pitch to restore camera on drop/placement exit; caches/restores `DefaultJumpMaxCount`.

### 2c. Placement Mode
- `StartPlacement()` (Held→Placing), `ConfirmPlacement()` (drop at aimed spot), `CancelPlacement()` (back to Held; if `bKeepBlockAtPlacementLocationOnCancel` is true, drops in place instead of retracting).
- Aim radius `PlacementRadius` = 300 max horizontal distance from player.
- Rotation input: `SetPlacementRotationInput(-1/0/1)` for Q/E, `PlacementRotationSpeed` = 120°/s.
- Vertical input: `SetPlacementVerticalInput(-1/0/1)` for C/Space, `PlacementHeightSpeed` = 100 cm/s.
- Start height: `GetPlacementStartHeight()` is `BlueprintNativeEvent` (overridable). Default = `PlacementStartHeight` (100) unless `bPlacementStartHeightFromCapsule` (true), in which case = carrier capsule half-height × `PlacementStartHeightCapsuleMultiplier` (1.0).
- Position smoothing: `PlacementPositionInterpSpeed` = 12 (0 = instant).
- Snap search: `PlacementSnapRadius` = 80 around block base for a nearby `BlockSocket`; final snap still gated by the target block's own `StackDetection` sphere.
- Placement camera: pulls spring arm to `PlacementCameraArmLength` = 800, offset `PlacementCameraSocketOffset` = (0,0,300), snaps controller pitch to `PlacementCameraPitch` = -60°, all eased via `CameraLerpSpeed` = 4.
- Visual: `PlacementIndicator` mesh (flat ring/disc), hidden except during placement mode.

### 2d. Stacking (magnetic block stacking)
- `TopSocket` (scene component at top of block) + `StackDetection` sphere trigger detect dropped blocks landing on top.
- `InitiateStack()`/snap eased at `StackSnapSpeed` = 8 onto `TopSocket`.
- `bIsStacked`, `StackedBlock` ref track relationship.
- `DislodgeStack()` / `DislodgeImpulse` = 500 (N) — knocks stacked block off (e.g. on hit).
- `TrySnapToNearbySocket()` — used after placement to snap onto a `BlockSocket` in range.
- Events: `OnBlockStacked()`, `OnBlockUnstacked()`.

### 2e. Throw
- `ThrowBlock()` while Held — fires as physics projectile along camera aim.
- `ThrowSpeed` = 3000 cm/s, `ThrowArcZ` = 200 (upward boost for arc).
- On impact: `ThrowImpactRadius` = 100 — dislodges any stacked blocks within radius of impact point.
- `HitDespawnDelay` = 2s after hit before vanish+respawn; `SafetyDespawnDelay` = 8s failsafe if it never hits anything.
- Respawn: reappears `RespawnHeight` = 300 above original pickup location, plays `RespawnEffect` (Niagara one-shot), fires `OnRespawn()` BP event.
- Uses `NotifyHit` override to detect impact; `DespawnAndRespawn()` handles the vanish/reappear via two timer handles.

### 2f. Beam VFX
- `BeamEffect` Niagara component rendered between player and object while carried, using vector params `BeamStartParamName`/`BeamEndParamName` (default "BeamStart"/"BeamEnd").

### 2g. Identity
- `BlockID` (FName, optional) — checked by `BlockSocket.RequiredBlockID` for matching; None = accepts/works with anything.

### 2h. Events
`OnPickedUp()`, `OnPlaced()`, `OnRespawn()`, `OnBlockStacked()`, `OnBlockUnstacked()` — all BlueprintImplementableEvent (VFX/SFX hooks only, no C++ side effects).

---

## 3. Melee Combo System (`ActionChainComponent.h/.cpp` + `ActionProxy.h/.cpp`)

Generic, actor-agnostic "spawn hitbox along an arc path with a timed window" sequencer — reusable for punches, dashes, puzzle interactions.

### 3a. `UActionChainComponent`
- `ActionChain`: ordered `TArray<FActionStageDef>` — index wraps after final stage (e.g. jab→jab→hook).
- `RequestAction()` — call on input Started. Buffers input during `BufferWindow` (fraction, 0.3 = last 30% of current stage duration) to auto-chain to next stage; otherwise after `ResetDelay` = 0.6s with no buffered input, silently resets to stage 0 (no penalty, "cut short anytime").
- State: `CurrentStageIndex` (BlueprintReadOnly), `bStageInProgress`, `bBuffered`, `StageEndTime`, `ResetDeadline`.

### 3b. `FActionStageDef` (per combo stage)
- `StageName` (cosmetic only, for matching anim montages later — not used by code yet).
- `ProxyClass` — `TSubclassOf<AActionProxy>` spawned for this stage.
- `SpawnPattern`: Single / Sequential (staggered) / Radial (**not implemented — currently behaves same as Sequential**, reserved for future placement math).
- `SpawnCount` = 1 (only relevant if pattern ≠ Single), `SpawnStagger` = 0.05s between spawns.
- Arc path (local space to owning actor): `StartOffset` (50,-40,20), `EndOffset` (50,40,20), `ArcBulge` (20,0,10) — bulge pushes the arc midpoint outward for curve shape (small/zero=jab, large sideways=hook, large upward=uppercut).
- `Duration` = 0.25s, `Damage` = 10.

### 3c. `AActionProxy`
- Spawned once per stage by `SpawnProxyForStage`; `Launch(start, end, arcBulge, duration, damage)` called immediately after spawn — all points world-space.
- Moves mesh + `HitSphere` along the quadratic-bulge arc over `Duration`; damages actors overlapped once (`bHasHit` guard, `OnHitSphereBeginOverlap`).
- Events only, no direct VFX/SFX/camera-shake in C++: `OnSpawned()`, `OnHit(AActor*)`, `OnArcComplete()`.

---

## 4. Doors (four types + shared utility)

### 4a. `ADoorHinged` — single hinged door
- `HingeAxis`: Yaw (side swing) or Pitch (up/trapdoor).
- `OpenAngle` = 90°, `OpenSpeed` = 3 (interp rate, not literal deg/s — check .cpp for exact formula).
- `bFreeSwing` — physics-driven hinge via `UPhysicsConstraintComponent`, no interact needed (push-open); `bLimitSwingAngle` + `MaxSwingAngle` = 90° optional clamp (only meaningful if bFreeSwing).
- `bRotateAwayFromPlayer` = true — auto-picks swing direction away from whoever opens it.
- `bStartLocked`, `bStartOpen` — initial state flags.
- Functions: `ToggleDoor()`, `OpenDoor()`, `CloseDoor()`, `LockDoor()`, `UnlockDoor()`.
- Events: `OnOpened`, `OnClosed`, `OnLocked`, `OnUnlocked`.

### 4b. `ADoorHingedDouble` — double hinged door
- Two independent leaves (`HingePivotA`/`DoorMeshA`, `HingePivotB`/`DoorMeshB`), mirrored motion.
- `OpenAngle` = 90°, `OpenDuration` = 0.6s (fixed-duration lerp, not speed-based like the single door), optional `OpenCurve` (x=0→1 time, y=0→1 angle fraction; steeper start = "push" feel; linear fallback).
- `bRotateAwayFromPlayer` = true (both leaves mirror).
- Same Toggle/Open/Close/Lock/Unlock API and events as single door.
- Internally tracks `SourceAngle`/`LerpAlpha` per-transition (re-lerps from current angle, not always from closed) — preserve for correct interrupt behavior mid-swing.

### 4c. `ADoorSliding` — sliding door(s)
- `SlideDirection` = (0,0,1) normalized (examples: up/down/forward/right), `SlideDistance` = 200cm, `OpenSpeed` = 3.
- `bDoubleDoor` — enables `DoorMeshB` sliding the opposite direction (mirrored placement expected in BP).
- Same Toggle/Open/Close/Lock/Unlock API, `bStartLocked`/`bStartOpen`, same events. Locked = ToggleDoor is a no-op.

### 4d. `ADoorDestructible` — breakable door
- Health: `MaxHealth` = 100, `CurrentHealth`. `ApplyDamage(float)` always applies regardless of lock state (locked doors can still be destroyed). `InteractHit()` convenience = deals `InteractDamage` = 34 (so ~3 hits to break at defaults).
- On destroy: spawns `DebrisCount` = 5 copies of `DebrisClass` (expects a physics-simulated cube BP), `DebrisImpulseStrength` = 500 cm/s launch (mass-independent), each auto-destroys after `DebrisLifetime` = 10s.
- `bStartLocked`, independent `LockDoor()/UnlockDoor()` (separate from destruction state).
- Events: `OnDamaged(float remainingHealth)`, `OnDestroyed()`, `OnLocked()`, `OnUnlocked()`.

### 4e. `DoorLinkUtils.h/.cpp` — shared dispatch
- Free functions `OpenLinkedDoor(AActor*)` / `CloseLinkedDoor(AActor*)` — type-checks the generic `AActor* LinkedDoor` pattern used by `BlockSocket`, `Lever`, `PuzzleTrigger` and dispatches to whichever concrete door type it is. **Single source of truth for the polymorphic-door dispatch — port as one shared helper in C#, not per-caller type checks.**

### 4f. `ADrawbridge` — hinged bridge (pitch-only, single-state variant of hinged door)
- `HingePivot` at wall anchor, `Arm` rotates, `BridgeMesh` child of Arm.
- `OpenAngle` = -90° (negative = rotates outward/down from raised position), `OpenSpeed` = 3.
- `bStartOpen`. Functions: `Open()`, `Close()`, `Toggle()`. Events: `OnOpened()`, `OnClosed()`. No lock state (simpler than the door types).

---

## 5. Levers, Lifts, Puzzles

### 5a. `ALever`
- Optional links: `LiftManager` (calls up/down), `LinkedDoor` (generic AActor, opens on activate/closes on reset via DoorLinkUtils), `LinkedTrigger` (`APuzzleTrigger` with `bUseLeverInstead=true`, feeds a `PuzzleManager`).
- `bStartLocked`, `bIsBottomLever` (true = calls lift DOWN when activated, false = calls lift UP).
- API: `ActivateLever()`, `ResetLever()`, `LockLever()`, `UnlockLever()`. State: `bIsLocked`, `bHasBeenActivated`.
- Events: `OnActivated`, `OnReset`, `OnLocked`, `OnUnlocked`. Has `Audio` component (`UAudioComponent`) and `CalibrationPoint` (rotate this to animate the lever arm visually).

### 5b. `ALiftManager` — vertical platform + integrated button
- Lift: `ELiftState` {Idle_Top, Idle_Bottom, Moving_Up, Moving_Down}. `TravelTime` = 3s, optional `MovementCurve` (ease [0,1]→[0,1]).
- `CallUp()`, `CallDown()`, `ButtonPressed()`; `OnLiftArrived(bool bArrivedAtTop)` event.
- Auto-unlock hooks: `LeversToUnlockAtTop` / `LeversToUnlockAtBottom` arrays, unlocked automatically on arrival.
- `LastActivatingLever` — tracked so BP can reset the lever that triggered the current move on arrival.
- Button sub-mechanic (separate state machine, `EButtonState` {Idle, Pressing, Releasing}): `ButtonTrigger` (box overlap on top of `ButtonMesh`) → after `ButtonPressDelay` = 0.5s, presses down `ButtonPressDepth` = 8cm over `ButtonAnimDuration` = 0.25s; `bWaitingForPlayerToLeave` gates re-press until player steps off. `ButtonRestRelativeLocation` cached for spring-back.
- Uses `ZTop`/`ZBottom` markers (`BottomMarker` billboard placed in-editor) + current mesh Z to compute travel.

### 5c. `APuzzleManager`
- `RegisteredTriggers` — all must be simultaneously active to solve.
- `Platforms` — `PlatformBase` actors activated on solve, deactivated on reset.
- `TriggerActivated(trigger)` / `TriggerDeactivated(trigger)` — called by triggers; maintains `ActiveTriggers` list; solved when `ActiveTriggers.Num() == RegisteredTriggers.Num()` (all registered active), unsolved otherwise. `bIsSolved` flag.
- Events: `OnPuzzleSolved()`, `OnPuzzleReset()`.

### 5d. `APuzzleTrigger`
- Normally a `TriggerBox` (UBoxComponent) overlap trigger; `bUseLeverInstead` = true disables collision and lets a Lever call `ActivateByLever()`/`DeactivateByLever()` instead.
- `AcceptedActor` — optional single actor filter; None = player only (`IsAcceptedActor`).
- `bAcceptsPickupSwap` = true — special rule: if a carried `PickupObject` is dropped on the trigger while the player is the current active actor, the block **replaces** the player as trigger-holder (`NotifyPickupDropped`) so the player can walk away and the puzzle stays solved; picking the block back up swaps the player back in if they're still standing on it, else deactivates (`NotifyPickupLifted`).
- `LinkedDoor` (generic, via DoorLinkUtils) — opens when triggered, closes when released.
- Feeds `PuzzleManager` via `TriggerActivated`/`TriggerDeactivated`.
- Events: `OnTriggerActivated()`, `OnTriggerDeactivated()`. State: `bIsActive`, `ActiveActor`.

### 5e. `ABlockSocket`
- `DetectionSphere` around `SnapPoint` (top of `BaseMesh`) detects dropped `PickupObject`s.
- `RequiredBlockID` — None accepts any block, else must match `PickupObject.BlockID`.
- Snap eased at `SnapSpeed` = 8 onto `SnapPoint`. `bOccupied`, `OccupiedBy`.
- `LinkedDoor` (generic via DoorLinkUtils) — opens when block placed, closes when removed.
- Events: `OnBlockPlaced()`, `OnBlockRemoved()`.

---

## 6. Moving Platforms

### 6a. `APlatformBase` — two-point (start/end) travel platform
- State machine `EPlatformState`: Idle_Start → Delay_ToEnd → Moving_ToEnd → Idle_End → Delay_ToStart → Moving_ToStart → (loop).
- Travel offset from spawn: `XTravelDistance`=0, `YTravelDistance`=0, `ZTravelDistance`=300 (defines EndLoc relative to StartLoc).
- `TravelTime` = 1s, optional `MovementCurve` easing [0,1]→[0,1].
- `ActivateDelay` = 0.3s — wait before moving on `Activate()` (matches original BP behavior note).
- API: `Activate()` (→ end position), `Deactivate()` (→ start position). Events: `OnActivated()`, `OnDeactivated()`.

### 6b. `ASplinePlatform` — spline-follower with independent rotation
- `SplinePath` (USplineComponent) defines path; `MovementMode`: Loop or PingPong.
- Rotation (independent toggle): `bRotationEnabled`, `RotationAxis` (shared `EHazardRotationAxis` enum {X,Y,Z} — reuse RotationHazard's enum), `RotationSpeed` = 30°/s.
- Movement (independent toggle): `bMovementEnabled`, `MovementSpeed` = 100 cm/s along spline, tracked via `DistanceAlongSpline` + `MovementDir` (±1, flips at spline ends for PingPong; wraps for Loop).

---

## 7. Hazards

### 7a. `ARotationHazard` — spinning damage hazard
- `RotationAxis` (X/Y/Z), `RotationSpeed` = 90°/s.
- On overlap: `ContactDamage` = 10, `ImpulseStrength` = 600 (physics knockback), `DamageCooldown` = 1s per-actor (tracked via `TMap<TWeakObjectPtr<AActor>, float> LastDamageTime` — same actor can't be hit again within cooldown).
- Event: `OnHazardContact(AActor*)`.

---

## 8. Save / Respawn System

### 8a. `ABonfire`
- `BonfireID` (FName) identifies this checkpoint. `bIsLit` state.
- `Light()` — call from Interact; saves game via GameInstance. Event: `OnLit()`.

### 8b. `UFrameworkGameInstance`
- `CurrentSaveSlot` = "Default".
- `SaveAtBonfire(FName BonfireID, ACharacter* Player)` — called from `Bonfire.Light()`; captures player location/rotation/health + BonfireID into save slot.
- `LoadGame()` — returns false if no save exists yet.
- `RespawnAtLastBonfire(ACharacter* Player)` — teleports player to `LastBonfireLocation`/`LastBonfireRotation` and restores health. This is the "real" death path used when `AFrameworkCharacter.bDebugHealOnZeroHealth` is false.
- Caches `LastSavedHealth` (-1 sentinel = no save loaded, use character defaults) and `LastSavedProfileIndex`.

### 8c. `UFrameworkSaveGame` (USaveGame data container)
Fields persisted: `PlayerLocation`, `PlayerRotation`, `CurrentHealth`, `CurrentProfileIndex`, `LastBonfireID`.

---

## Cross-cutting patterns to preserve exactly

1. **Generic `AActor* LinkedDoor` polymorphic dispatch** — `BlockSocket`, `Lever`, `PuzzleTrigger` all point at a plain `AActor*` that may be `DoorHinged`, `DoorHingedDouble`, `DoorSliding`, or `Drawbridge`; resolved through one shared type-switch (`DoorLinkUtils`). Port as a single shared function, not duplicated per caller.
2. **Ease curves with linear/SmoothStep fallback** — `UCurveFloat*` fields (`MovementCurve`, `PickupCurve`, `DropCurve`, `OpenCurve`) are optional; when null, code falls back to a specific default curve (linear for platforms/doors, SmoothStep for pickup/drop lerps). Confirm exact fallback per-class in the .cpp before porting — don't assume linear everywhere.
3. **Cooldown/debounce guards** — `ToggleCooldown` (Pickup), `DamageCooldown` (RotationHazard per-actor map), `BufferWindow`/`ResetDelay` (ActionChain) are deliberate anti-spam/anti-double-fire timers from prior bug fixes. Preserve exact values and reset semantics.
4. **BlueprintImplementableEvent-only cosmetics** — VFX/SFX/camera-shake are never triggered from C++ (`ActionProxy`, `PickupObject`, doors, etc. all just fire empty BP events). In C#/S&box, keep these as overridable virtual hooks / events, not baked-in effects.
5. **Shared enums** — `EHazardRotationAxis` (X/Y/Z) is defined in `RotationHazard.h` but reused by `SplinePlatform`. Port as one shared enum, not two.
6. **State machines are explicit enums, not bools** — `EPickupState`, `EPlatformState`, `ELiftState`, `EButtonState` — port as enums with the same transition graph, not flattened boolean flags, to keep transition guards correct (e.g., can't re-trigger Activate() mid-Moving state).

---

## HANDOFF — Build the Full Transcription Packet

This document (header signatures + defaults only) is the *skeleton*. It is not yet
sufficient to port from — the real behavior (exact tick-order math, easing formulas,
vector blends, physics constraint setup) lives in the `.cpp` files, which have not
been read yet. Agent picking this up: do the following, in order.

### 1. Read every `.cpp` in `Source/FrameworkTest/` paired to the classes above
For each mechanic section in this doc, open the matching `.cpp` and extract what the
header can't show:
- Exact formulas (e.g. wall-jump launch vector blend from `WallJumpAngle`, glide's
  velocity-Z interp math, fall-damage rounding/clamping order).
- Tick-order dependencies (what runs before what inside `Tick`/`TickComponent`).
- Any behavior not exposed as a `UPROPERTY` at all (magic numbers, early-return
  guards, edge cases) — these are invisible from the header and are exactly the kind
  of thing that silently breaks a "looks right" port.
- Component ownership/attachment hierarchy set up in the constructor (`CreateDefaultSubobject` chains, attach parents) — needed to recreate the GameObject/Component tree in S&Box.
Update each section of this doc in place with the confirmed exact logic, replacing
"check .cpp" placeholders (there are a few — e.g. door open-speed formula, curve
fallback per-class).

### 2. Research current S&Box API standards before writing any port instructions
Do not let translation choices come from general C# knowledge or guesswork. Confirm
against the actual current `Sandbox.*` / `GameObject`/`Component` API surface:
- Component base class, lifecycle hooks (`OnUpdate`, `OnFixedUpdate`, `OnStart`,
  `OnEnable`/`OnDisable` — confirm exact current names, they've changed across
  S&Box versions).
- Property/inspector exposure (`[Property]`, `[Sync]`, categories/groups).
- Equivalents for: Niagara (particle/VFX system), Cable Component, Physics
  Constraint Component, Spline Component, timers (`FTimerHandle` → S&Box's
  timer/coroutine pattern), `BlueprintImplementableEvent` (→ C# `Action`/`event`
  exposed the way this specific S&Box project already does it — check existing
  project code for the convention in use, don't invent a new one).
- Check `D:\S&Box\Projects\sbox-scenestaging-main` and any other local S&Box
  project/addon code for the conventions already established, plus official
  s&box docs/issue tracker for anything not covered locally.

### 3. Destination folder (confirmed)
`D:\S&Box\Projects\prototyperebuild` — canonical transcription destination. Write
the packet there (or a docs/ subfolder, per that project's existing convention —
check before adding a new top-level folder). Read its `.sbxproj`/`addon.json` and
existing Components to learn naming/structure conventions already in use before
writing new files, so the packet matches house style instead of introducing a
second convention.

### 4. Follow the existing port plan
`CPP_TO_SBOX_PORT_STRATEGY.md` (repo root) already defines port order and
per-mechanic process — shared base first (`ActionChainComponent`, `ActionProxy`,
`DoorLinkUtils`), then doors, then puzzle system, then standalone mechanics, then
core/player last. Use that order when structuring the packet; don't re-derive it.

### 5. Deliverable: the full transcription packet
One instruction file per mechanic (or per port-order group, matching the strategy
doc's grouping), written into the resolved destination folder (or a docs/
subfolder there — confirm project convention), each containing:
- The confirmed exact C++ behavior (from step 1).
- The confirmed S&Box-side equivalent API calls (from step 2), not generic C#.
- Explicit component/GameObject hierarchy to recreate.
- Exact tunable defaults (already captured in this doc — carry forward unchanged).
- The BlueprintImplementableEvent → C# event/Action mapping for that mechanic.
- A note on anything with no clean 1:1 (flag as a known gap per the strategy doc,
  don't force a fake equivalent).

Each packet file should be self-contained enough that an agent with zero prior
context on this project could execute the port from it alone, build, and run it in
the S&Box editor — per the strategy doc's "port one mechanic at a time, verified
before moving to the next" rule.
