# Task — Interact Prompt Widget (WBP_InteractPrompt)

## Context Gathered (read this first, no exploration needed)

- Target BP: `/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter` (parent class: Character, implements `BPI_TouchInterface_C`)
- Relevant existing BP variables (already there, Editable, DO NOT recreate):
  - `bCanInteract : bool`
  - `CurrentInteractable : object<Actor>`
- Relevant components on BP_ThirdPersonCharacter: `CameraBoom` (SpringArm), `FollowCamera` (Camera), `Sphere_Interact` (Sphere), `CarryPoint` (Scene)
- Existing `BeginPlay` graph: `Event BeginPlay → Get Player Controller → Get EnhancedInputLocalPlayerSubsystem → Add Mapping Context(IMC_Interact, Priority=0)`. New Create Widget/Add to Viewport nodes must be ADDED to this chain without breaking it (e.g. branch off after AddMappingContext, or sequence node).
- Existing overlap graphs (Sphere_Interact Begin/End Overlap) already set `CurrentInteractable`/`bCanInteract` — do not touch.
- `/Game/Widgets/` folder does NOT exist yet — must be created when making `WBP_InteractPrompt`.

## Goal
Simple UMG widget showing "Press E to Interact" on screen, visible only when
player is near an interactable. First experiment task for local-AI build,
Claude verifies after.

## What to build

### 1. Widget Blueprint
- Create `/Game/Widgets/WBP_InteractPrompt` (parent: UserWidget)
- Canvas Panel (root, default)
- Add `Border` component named `PromptBorder`
  - Position: bottom-center of screen, e.g. anchor (0.5, 0.85), size ~300x60
  - Background color: dark semi-transparent (e.g. RGBA 0,0,0,0.5), use brush transparency on
- Add `TextBlock` named `PromptText` inside `PromptBorder`
  - Text: "Press E to Interact"
  - Font size ~20, centered

### 2. Visibility binding
`PromptBorder` Visibility should be bound via a function (not a tick-poll binding
if avoidable, but a simple `Get Visibility` binding function is fine for v1):
- Get Player Character (or Get Owning Player Pawn) → Cast to `BP_ThirdPersonCharacter`
- Read `bCanInteract` (existing BP variable on BP_ThirdPersonCharacter)
- Return `Visible` if true, `Hidden` if false

### 3. Add to viewport
In `BP_ThirdPersonCharacter` `BeginPlay` (or its PlayerController, whichever is
simpler given existing graph), add:
- Create Widget (Class = WBP_InteractPrompt, Owning Player = Get Controller)
- Add to Viewport

## Constraints / don't touch
- Do NOT modify `CurrentInteractable`/`bCanInteract` logic itself — read-only consumer.
- Do NOT remove or rewire existing IA_Interact / overlap event graphs.
- Don't add new C++ — this is BP + widget only.

## Verification checklist (Claude, post-build)
1. `WBP_InteractPrompt` exists, compiles clean (`compile_blueprint`).
2. `get_widget_component_layout` shows Border+TextBlock hierarchy, reasonable position/size.
3. `BP_ThirdPersonCharacter` graph: Create Widget → Add to Viewport added without
   breaking/duplicating existing BeginPlay nodes (`find_blueprint_nodes`, check for dupes).
4. Visibility binding function reads `bCanInteract` correctly (cast succeeds, no
   dangling/unconnected pins).
5. PIE test: walk near a pickup/interactable → prompt appears; walk away → disappears.
