# Needs-Editor Plan: Consolidate DoorHinged / DoorHingedDouble

Deferred from the Source code cleanup pass (2026-07) because it changes a class
hierarchy that existing Blueprints and placed level instances depend on — not
safe to do blind from C++ alone.

## Problem

`ADoorHinged` and `ADoorHingedDouble` are two independent `AActor` subclasses
with near-identical public APIs:

- Same UFUNCTIONs: `ToggleDoor`, `OpenDoor`, `CloseDoor`, `LockDoor`, `UnlockDoor`
- Same BlueprintImplementableEvents: `OnOpened`, `OnClosed`, `OnLocked`, `OnUnlocked`
- Same state UPROPERTYs: `bIsOpen`, `bIsLocked`, `bStartLocked`, `bStartOpen`,
  `bRotateAwayFromPlayer`, `HingeAxis`, `OpenAngle`
- Each reimplements its own angle-interpolation Tick logic and its own
  lock/open/close state transitions from scratch

`ADoorHingedDouble` already `#include "DoorHinged.h"` just to reuse the
`EHingeAxis` enum — it doesn't inherit from it.

## Why not just do it in this pass

Introducing a shared base class (e.g. `ADoorHingedBase`) and re-parenting both
under it would:

- Change the class each Blueprint child currently inherits from — any BP
  child of `ADoorHinged`/`ADoorHingedDouble` needs to be reopened and
  recompiled in the editor to confirm it doesn't silently break (component
  reparenting inside a BP can orphan or reset EditAnywhere values set on
  placed instances).
- Risk component hierarchy changes if shared components move to the base
  class — any level actor placed from these classes needs a visual check
  that meshes/pivots still look correct after reparenting.

Both fall under the user's "no editor-side verification without asking"
constraint, so this is a plan, not a diff.

## Proposed approach (for a future editor session)

1. Add `ADoorHingedBase : public AActor` with the shared surface:
   - Shared UPROPERTYs: `HingeAxis`, `OpenAngle`, `bRotateAwayFromPlayer`,
     `bStartLocked`, `bStartOpen`, `bIsOpen`, `bIsLocked`
   - Shared UFUNCTIONs: `ToggleDoor`, `OpenDoor`, `CloseDoor`, `LockDoor`,
     `UnlockDoor` (common lock/state-guard logic lives here; each subclass
     overrides a protected virtual like `ApplyMeshRotation()` /
     `StartTransition()` for its own single-leaf vs. double-leaf mesh work)
   - Shared BlueprintImplementableEvents: `OnOpened`, `OnClosed`, `OnLocked`,
     `OnUnlocked`
2. Re-parent `ADoorHinged` and `ADoorHingedDouble` onto the new base, moving
   the identical members out of each and keeping only what's genuinely
   different (single `HingePivot`/`HingeArm`/`DoorMesh`/physics-constraint
   free-swing vs. dual `HingePivotA/B`, `DoorMeshA/B`, curve-driven lerp).
3. Open the editor, confirm every existing Blueprint child of both classes
   still compiles cleanly and every placed instance in each level still
   renders/opens/closes correctly before treating this as done.
4. Delete this file once the consolidation is verified in-editor.
