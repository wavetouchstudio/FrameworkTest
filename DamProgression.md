# CoZno — Dam Interior Progression & Level Architecture

## Design constraints enforced (from the brief)

- The 7x4 grid is architecture, not a checklist of 28 playable levels. A subset is actually playable.
- Architecture first. Hydroelectric machinery sits where it would physically live, not one theme per floor.
- Vertical character gradient: buried / wet / dark / dungeonlike (low) -> mixed industrial (mid) -> open / developed / inhabited (high) -> surface town.
- Souls-like spatial logic: shortcuts, drained areas, seen-before-reached, cross-segment links.
- Horizontal structure: thick walls gate movement; not every segment connects at every level.
- Animal ability gates overlap the same geography (squirrel=vertical/elevated, raccoon=ground/stealth/manipulation, badger=breaking/burrowing/structural) — same spaces, different access, NOT separate levels.
- Lore = Mechanism -> Space -> History, never lore rooms with exposition.

---

## A. Dam Physical Layout — 7x4 conceptual matrix

Orientation: **7 levels** L1 (lowest) -> L7 (crest); **4 structural segments** A (left/hillside, oldest abutment, closest to the caves) -> D (right abutment, spillway + the canal to the reservoir island). Thick concrete walls segment the dam; a cell is playable only if a route connects to it.

Legend: P=playable+traversed, PA=partially accessible, T=transit (pass-through), F=flooded/submerged, C=collapsed, S=sealed, AB=abandoned, V=visible-but-unreachable, O=later-opener (locked, opened by player), At=atmosphere/lore-only.

| Level | A left/oldest | B intake/turbine | C electrical/control | D spillway/canal |
|-------|---------------|------------------|----------------------|------------------|
| L7 crest/town | AB old intake (overlooks reservoir) | V reservoir edge / tower tops | **P Surface Town** (inhabited, act terminus) | **P spillway crest + Crane deck** |
| L6 upper service | C collapsed worker gallery (blocks A early) | P/T maintenance galleries | **P repurposed workshop (inhabited)** | O/P spillway service tunnel |
| L5 intake | S sealed gallery | **P trash-rack / intake tower (squirrel)** | **P gate-control gallery (raccoon)** | PA overflow gallery (wet) |
| L4 electrical | AB old relay room | T penstock-crossing vault | **P relay & switchyard (badger-breakable)** | PA tunnel to upper dam |
| L3 penstock | C penstock collapse (blocks A) | **P mid penstock gallery** | **P low penstock gallery** | F overflow gallery |
| L2 turbine | C/F half-submerged bay | **P the Drowned Turbine** | **P turbine continuation (flooded)** | AB bays (visible across) |
| L1 foundation | **P tailwater gallery (first area)** | P/F sump (drains via lever) | T/F cave void (transit) | O collapsed cave (badger) |

Playable subset is roughly half the physical dam (~13 P-cells). Everything else is deliberately buried, sealed, flooded, collapsed, or visible-but-unreachable — so the dam appears ~2x larger than the walked route.

---

## B. Player Progression — actual walked route (12 playable areas)

An upward-winding path with lateral detours and backtracking. Codenames in italics.

1. Twilight Sump &amp; Tailwater Gallery (L1.A) — player emerges from the sewer. Partly flooded foundation: drainage channels, sump pit, low collapse. Establishes buried/wet/dark/confined. First bonfire + first broken wall (badger) = first "leaving the tutorial" beat.

2. The Drowned Turbine (L2.B) — first major dam dungeon. One enormous turbine in a flooded hall, penstock shaft rising into darkness. Verticality + machinery + floodwater introduced together.

3. Bilge Run (L2.C -> L1.B) — pumped BACK DOWN through flooded drainage between turbine bays into the sump. Contradicts "go up" and trains the Souls-like return.

4. Low Penstock Gallery (L3.C) — penstocks sweep across the ceiling; first grapple anchors; first lever that opens the gate out of the powerhouse. Hydroelectric + verticality introduced together.

5. The Gate Room (L3.C -> L5.C) — intake gate-control gallery: a raccoon manipulation puzzle (gate lever/blocks) that opens a passage up into the intake structure.

6. Relay & Switch (L4.C) — electrical vaults: badger-breakable interior, salvaged Old World switchgear. The manufactured-water shortage and failing power system surface MECHANICALLY here (gate positions inconsistent with the low reservoir).

7. Trash-Rack Tower (L5.B) — intake structure + trash racks: the squirrel vertical route (grapple/glide up the tower) toward the upper dam.

8. Overflow Gallery (L6.D) — near the spillway; wetter, louder, water pressure audible through walls. First sight-line to the spillway crest above.

9. Cozno's Workshop (L6.C) — a repurposed, inhabited service bay: harnesses, salvaged tools, crew habitations. Strongest "people have occupied and modified this machine" beat; rest before the crest.

10. The Service Corridor (L6.B -> D) — transit: lifts, service tunnels, the vertical backbone that lets earlier areas be revisited.

11. Crest & Crane Deck (L7.D) — open, above-water. Reaches the crane = the dam-interior arc terminus.

12. Surface Town (L7.C) — inhabited frontier town at the dam crest: the upper-dam payout and the transition to Far End -> Crane -> Reservoir Island -> Mech.

Endpoints (next act, carried for arc): reaching the reservoir island and entering the mech = a full climb-as-level; then a moving-mech boss; then a RE-CLIMB where routes and machinery are now live and hostile; finally the mech brain/villain. The second climb must read as the SAME structure transformed — machinery relocated, previously-safe areas dangerous, new traversal open, structure understood from the first climb.

---

## C. Connections

Vertical (backbone):
- Penstock/turbine shaft (L2.L3, B): the great vertical. A ladder-lift plus an alternate free-climb/rappel/spring-arm line. Squirrel grapps to the top; raccoon/badger use the lift. Opens early; used repeatedly over multiple area revisits.
- Intake-tower vertical (L5 to L3): squirrel-only grapple/glide line up the tower. Opens early; used to shortcut between intake levels.
- Main lift (C/D, L3-L6): the primary vertical axis, lever-gated, unlocks on both ends.
- Drainage sump pump (L1.B): draining the water permanently opens a large flooded area.

Cross-segment (walls that gate, not just decorate):
- L1.A breach -> L1.D: the player opens this gap between the two abutments in the first ten minutes, establishing that the map is a network, not the corridor of the tutorial.
- L3.A blocked wall: opens much later, so by then the map has roughly doubled in size.
- Overflow gallery (L5/D) connecting to other sections: water-level and structural gating.
- L4.A sealed relay room -> L4.C accessible: sealed-then-released, the visible-but-unreachable-then-reachable loop that sells "bigger than the path."
- Flooded tunnels toggle to dry (drain-pump) and vice versa, so routes come and go.

Collapsed/sealed boundaries:
- L3.A total collapse (reached only much later) — a wall that looks permanent until the second half.
- L1.D cave collapse (badger) — the first place animal ability opens a shortcut.
- Various sealed doors/rooms unlocked via lever + puzzle logic.

Lifts, shafts, transit:
- Penstock shaft-lift, intake-tower grapple chain, main lift, and the Service Corridor (L6 as the hub).
- Ladder-lifts, spring-arm swings, cable lines, and drainage pipes supply the vertical.
- Block placement + stacking used in one flooded sluice to redirect water and open a route.

Bonchain (respawn):
- Bonfires placed at each progression node so backtracking after death is meaningful; the map reopens around the player as they return to earlier areas with a faster spine.

---

## D. Overall Arc — spatial character evolution

From sewer to surface town, the dam's character thickens as the player climbs.

Sewer — buried, wet, dark: the emergence point. Confined, dungeonlike. No human footprint yet.

Lower dam (L1-L2): buried industrial dungeon. Floodwater, darkness, drainage channels, collapse, and massive silent machinery. The player is working into the depths of something enormous.

Middle dam (L3-L4): the dam reveals how it works — penstocks, grapple anchors, gate controls, then electrical vaults. Machinery, maintenance platforms, forgotten vaults, a vertical shaft, animal passages all coexist in the same floor bands. The failing power system and manufactured shortage are learned through inconsistency, not cutscenes.

Upper dam (L5-L6): inhabited and repurposed. Intake towers, spillway pressure, then a workshop and inhabited service bays — proof people occupied, modified, and built on the machine. The dam reads as civilization, not just a machine.

Crest/town (L7): open, developed, connected to the world above. The payoff and the bridge to the reservoir island / steam mech arc.

The whole route is an increasingly complex path through a structure the player only gradually maps. Unreachable segments and flooded passages are shown early, then unlocked — so the dam becomes increasingly legible as the player climbs.

---

Deliberate non-deliverables (to keep scope honest): no individual-room design, none of the 28 cells forced into levels, no new enemies/mechanics invented. The mech act, the two-climb structure, and the villain are carried as the established next act, not resolved here.