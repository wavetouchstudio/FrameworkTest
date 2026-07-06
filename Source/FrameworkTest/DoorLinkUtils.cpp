#include "DoorLinkUtils.h"
#include "DoorHinged.h"
#include "DoorHingedDouble.h"
#include "DoorSliding.h"
#include "DoorDestructible.h"
#include "Drawbridge.h"

void OpenLinkedDoor(AActor* LinkedDoor)
{
    if (!IsValid(LinkedDoor)) return;

    if (ADoorHinged* DH = Cast<ADoorHinged>(LinkedDoor))                    { DH->OpenDoor(); }
    else if (ADoorHingedDouble* DHD = Cast<ADoorHingedDouble>(LinkedDoor))  { DHD->OpenDoor(); }
    else if (ADoorSliding* DS = Cast<ADoorSliding>(LinkedDoor))            { DS->OpenDoor(); }
    else if (ADoorDestructible* DD = Cast<ADoorDestructible>(LinkedDoor))  { DD->UnlockDoor(); }
    else if (ADrawbridge* DB = Cast<ADrawbridge>(LinkedDoor))              { DB->Open(); }
}

void CloseLinkedDoor(AActor* LinkedDoor)
{
    if (!IsValid(LinkedDoor)) return;

    if (ADoorHinged* DH = Cast<ADoorHinged>(LinkedDoor))                    { DH->CloseDoor(); }
    else if (ADoorHingedDouble* DHD = Cast<ADoorHingedDouble>(LinkedDoor))  { DHD->CloseDoor(); }
    else if (ADoorSliding* DS = Cast<ADoorSliding>(LinkedDoor))            { DS->CloseDoor(); }
    else if (ADoorDestructible* DD = Cast<ADoorDestructible>(LinkedDoor))  { DD->LockDoor(); }
    else if (ADrawbridge* DB = Cast<ADrawbridge>(LinkedDoor))              { DB->Close(); }
}
