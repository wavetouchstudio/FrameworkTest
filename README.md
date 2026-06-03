# FrameworkTest — UE5 Platformer & RPG Toolkit

A modular framework built in **Unreal Engine 5.6** for third-person platformer and RPG mechanics. The goal is a reusable toolkit of interactables, environmental systems, and early combat scaffolding — engine-native C++ with Blueprint exposure throughout.

## What's Here

**Doors & Gates**
- Hinged (single and double), sliding, destructible, drawbridge variants

**Platform Systems**
- `PlatformBase` — foundation class for moving/stationary platforms
- `LiftManager` — coordinates multi-platform lift sequences

**Puzzle Infrastructure**
- `PuzzleManager` — tracks puzzle state across actors
- `PuzzleTrigger` — event-driven trigger volumes
- `Lever` — toggle input actor wired to the puzzle system
- `BlockSocket` — socket-based block placement validation

**Pickups & Interaction**
- `PickupObject` — base class for interactable world items

## Engine & Tooling

- Unreal Engine 5.6
- C++ module: `FrameworkTest` (Runtime)
- Marketplace plugins: BlockoutTools, BlueprintAssist, BlueprintAnalyzer, BP2AI
- MCP integration via UnrealMCP for AI-assisted development

## Purpose

This repo is one half of a dual-engine toolkit. The same mechanics are being developed in parallel for S&Box (see [FrameworkSBoxPort](https://github.com/wavetouchstudio/FrameworkSBoxPort)), with the intent of building whichever engine best facilitates the project at hand.
