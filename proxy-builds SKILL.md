---
name: proxy-builds
description: >
  Blueprint for building physical objects in Unreal Engine via the editor MCP
  plugin. Use whenever the user (or a previous session) asks to place, build,
  move, verify, or scene a new object in a live Unreal level using the "editor"
  toolset: actor transform changes, primitive builds (houses, prop clusters,
  prototypes), geometry placement around a reference, or scene verification.
  Applies to any object type, any level, any shape. Do NOT use to edit
  player movement/interaction mechanics (double-jump, glide, grapple, doors,
  lifts).
arguments: ""
license: MIT
---

# Proxy Build (Unreal Editor via MCP)

Build, move, verify, and organize physical objects inside a live Unreal Engine
level. The whole loop runs through MCP toolsets, and most of the cost in
earlier sessions was wasted tool calls from guessed names, wrong path casing,
and misreading the primitive transform model. This skill is the corrected
playbook — read it BEFORE opening any editor tool.

## The toolset

Everything runs in MCP toolsets. Authoritative set-qualified, case-exact:

- editor_toolset.toolsets.scene.SceneTools — level + actor lifecycle, outliner
  folders, find_actors, level camera.
- editor_toolset.toolsets.actor.ActorTools — actor transforms, components,
  parent/child hierarchy, labels, tags.
- editor_toolset.toolsets.asset.AssetTools — asset filesystem, save.
- editor_toolset.toolsets.primitive.PrimitiveTools — add_cube, add_cone,
  add_cylinder (adds a component to an actor).
- editor_toolset.toolsets.object.ObjectTools — list_properties,
  get_properties, set_properties on any object/component.
- EditorToolset.EditorAppToolset — viewport camera, CaptureViewport, FocusOnActors.
- editor_toolset.toolsets.static_mesh.StaticMeshTools — mesh asset inspection.

Always call describe_toolset before using one you are unsure about. Call names
differ from raw editor tool names. If a call returns "Unknown tool", read the
schema for the real name; don't retry the guessed one.

## The workflow (do in order)

1. Start authoritative, not from memory. Re-read state fresh:
   - SceneTools.get_current_level — confirm the loaded level path.
   - Locate what the previous session built or the reference it is anchored to
     (find_actors by name, or get_components on a known actor ref). Copy any
     refPath you reuse VERBATIM — one casing or missing-colon typo breaks it.
2. Store the primitive component tree before building (so you can inspect
   relative transforms of each piece once placed).
3. Build piece-wise with verification. Add geometry, then get_properties the
   stored relative transform. Compute what each object's world bounds should be
   and confirm the actor-level bounds match. Fix placement with
   set_properties relativeLocation — don't recreate components just to nudge a
   number.
4. Verify each piece's relativeLocation is the intended local offset from its
   parent; confirm wall/roof overlaps by reading resulting bounds, not guessing.
5. One get_actor_bounds(actor) returns the world AABB; compare to plan. Assign
   outliner set_actor_folder and optional set_label in one batch.
6. Save with save_assets([]). Actor instances in-level can't be saved
   individually: save_assets([]) saves all dirty. This takes time (30s+) —
   trigger it once and leave the editor alone; don't hammer it.

## Critical mechanics

- add_cube dimensions are ABSOLUTE cm, not base cube x multiplier. Each cube is
  dimensions = {x, y, z} in cms; the component carries
  relativeScale3D = dimensions / 100 (default Cube.Cube = 100cm). NEVER pass a
  separate scale — it double-scales. dimensions(x:200, y:200, z:200) is a 2m
  cube; adding scale:x:2 makes an 8m cube. Same for add_cone/add_cylinder.
- Reference cube StaticMeshActor_296 is the world anchor: x=-124050, y=-9300,
  z=-10635.4587, identity rotation. Place relative to it. If you must move the
  house root, use set_actor_transform(worldspace: true).
- get_actor_bounds returns an actor's world AABB. StaticMeshTools.get_bounds
  needs a real StaticMesh asset path, not a component ref — component refs
  fail. Verify world geometry through actor bounds only.
- Hierarchy is via component refs: add_to_scene_from_component with a parent ->
  parent-local xform; no parent -> world-space. Set parent-child with
  set_parent_component(component, parent); query with get_parent_component,
  get_root_component, get_component_actor.
- RefPaths are strict: /Game/[...]/<Level>:[Level].PersistentLevel
  .<ActorClass>_<N>.<ComponentName>. Copy verbatim; don't retype casing or drop
  the colon before PersistentLevel.
- set_component_active returns false even on success — ignore the value, the
  change took effect.
- save_actor fails on in-level actors ("not an external actor asset"). Use
  save_assets([]).

## Failure mode -> fix

- "Unknown tool": call describe_toolset for that toolset, read the real name.
- "path not valid ... Actor/Object": refPath casing/colon wrong — re-fetch and
  paste exactly.
- get_properties fails on a property: it doesn't exist — list_properties first,
  then query only valid names.
- set_parent_component error: it needs BOTH component AND parent; parent-only
  fails.
- set_properties nested-JSON shape accepted (true) but didn't change value: the
  key was wrong; re-read the property.

## Verification checklist before finishing

- Each piece's relativeLocation is the intended local offset from its parent.
- Actor-level get_actor_bounds matches intent (no stray oversized AABB from a
  double-scaled cube).
- Component-to-parent hierarchy is as designed (get_parent_component).
- Outliner folder assigned; labels readable.
- save_assets([]) succeeded (or left completing without hammering).
- Nothing the user didn't touch was modified.

## One-line summary

Place objects relative to the world-reference cube, add primitives with
absolute-cm dimensions (no extra scale), verify world bounds via
get_actor_bounds, then one save_assets([]) once the editor finishes.