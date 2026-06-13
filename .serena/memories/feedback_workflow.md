---
name: feedback-workflow
description: Working loop and plan structure preferences for this project
metadata:
  type: feedback
---

Working loop: scheduled docs → plan → implement → bug fix → push → document in Obsidian → repeat.

**Plan structure (2026-06-13):** When handing off a build task (e.g. to local AI via SESSION_HANDOFF.md, or structuring own multi-step work), write plans in two phases:
1. **Research/planning phase** — gather all necessary file paths, relevant code snippets, existing variable/function names, constraints up front. Front-load context so the executor doesn't need exploratory tool calls mid-task.
2. **Execution phase** — procedural task list, looped through one at a time until complete.

Why: reduces back-and-forth tool calls during execution, especially for handoffs to less-capable local models (see [[feedback_unreal_python]]). Keeps execution focused/mechanical.

How to apply: when writing SESSION_HANDOFF.md or similar task specs, include a "Context gathered" section (file paths, existing var/function names, current relevant code) before the task checklist.
