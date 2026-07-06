#pragma once

class AActor;

// Shared dispatch for the generic "AActor* LinkedDoor" pattern used by
// ABlockSocket, ALever, and APuzzleTrigger. Covers every door/bridge type;
// update here once instead of drifting per-caller.
void OpenLinkedDoor(AActor* LinkedDoor);
void CloseLinkedDoor(AActor* LinkedDoor);
