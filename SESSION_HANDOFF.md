# Handoff — Pickup System Input Migration

## Current Problem: All pickup controls broken except E (interact/pickup)

The system was mid-migration when the session ended. A C++ rebuild was also pending
(respawn system added to PickupObject). Start by rebuilding, then run the checklist below.

---

## What Was Being Built

**Goal:** Block throw + respawn. X enters placement mode, LMB throws the block, block
despawns after landing and respawns above its original pickup location.

**Architecture constraint:** `StartPlacement`, `ConfirmPlacement`, `CancelPlacement`,
`ThrowBlock` all live on `APickupObject` (an Actor). No Player Controller access from there.
All `Add/Remove Mapping Context` calls must be in `BP_ThirdPersonCharacter`.

---

## Blueprint State Going Into This Chat

### BP_ThirdPersonCharacter

**BeginPlay:**
- Add `IMC_Interact` (priority 0) — direct subsystem call ✓
- Add `IMC_PlacementMode` (priority 1) — direct subsystem call ✓
- `IMC_Throw` is NOT added here (was a bug, was fixed)

**New variable:** `CarriedBlock` (type: Pickup Object reference)

**IA_Interact Started:**
```
bCanInteract → Interact (interface msg) → Cast to PickupObject
    Success → Branch bIsCarried → True: Set CarriedBlock / False: Clear CarriedBlock
    Failed  → Clear CarriedBlock
```

**IA_PlacementMode Triggered (X):**
```
Is Valid (CarriedBlock)
    → Branch PickupState == Placing?
        True  → CancelPlacement() → Remove IMC_Throw
        False → Branch PickupState == Held?
            True  → Add IMC_Throw (priority 1) → StartPlacement()
            False → end
```

**IA_Throw Triggered (LMB, only active when IMC_Throw loaded):**
```
Is Valid (CarriedBlock) → ThrowBlock() → Remove IMC_Throw
```

**IA_PlacementConfirm Triggered (F):**
```
Is Valid (CarriedBlock) → ConfirmPlacement() → Remove IMC_Throw
```

All Add/Remove Mapping Context nodes use the **direct subsystem call** (⚡ not ✉).

### BP_PickupObjectcpp

X and F InputKey nodes deleted. E key no longer chains ToggleCarry (was double-firing).
Kept: Q/E rotation, Space/C vertical, scroll distance, **Event Interact → ToggleCarry**.

---

## Diagnostic Checklist (run in this order)

**1. Rebuild C++** — respawn system changes require a full compile.

**2. Check Event Interact in BP_PickupObjectcpp**
Confirm `Event Interact → ToggleCarry()` is still connected. May have been accidentally
disconnected during cleanup. Without it, E never picks up or drops.

**3. Check IMC bindings** — open each asset and confirm:

| IMC | Action | Key |
|-----|--------|-----|
| `IMC_Interact` | `IA_Interact` | E |
| `IMC_PlacementMode` | `IA_PlacementMode` | X |
| `IMC_Throw` | `IA_Throw` | Left Mouse Button |
| `IMC_Interact` | `IA_PlacementConfirm` | F |

If X isn't bound inside `IMC_PlacementMode`, pressing X does nothing silently.

**4. Verify CarriedBlock is being set**
Add a temporary Print String after `Set CarriedBlock` in `IA_Interact`. Pick up a block.
If it doesn't print, the cast to PickupObject is failing.

**5. Verify IA_PlacementMode fires at all**
Add a temporary Print String at the top of the `IA_PlacementMode` handler. Press X while
holding a block. If nothing prints, the IMC_PlacementMode binding is missing or wrong.

**6. Full flow test**
E (pickup) → X (placement, camera pulls back) → X (cancel, camera returns)
→ E, X, LMB (throw) → block flies → lands → waits 2s → vanishes → reappears above pickup point

---

## Rules to Not Break

- `IMC_Throw` only loaded during placement — added on X, removed on throw/confirm/cancel
- `IMC_PlacementMode` always loaded (BeginPlay), never removed
- `CarriedBlock` set/cleared in `IA_Interact` only, not in overlap events
- Block BP keeps Q/E/Space/C/scroll — don't move those to the character
