
#ifndef __input_h__
#define __input_h__

#include "typedefs.h"

enum {
    kButtonJump = 0x1,
    kButtonOpen = 0x4,
    kButtonFire = 0x8,
    kButtonCrouch = 0x10,
    kButtonCheatGuns = 0x20,
    kButtonCheatGodMode = 0x40,
    kButtonCheatKeys = 0x80,
    kButtonCheatItems = 0x100,
};

// 32 bytes
struct PlayerInput
{
    int xVel;
    int yVel;
    short nAngle;
    ushort buttons;
    short nTarget;
    int8_t horizon;
    int8_t nItem;
    int h;
    char i;
    char field_15[11];
};

void InitInput();
void ClearAllKeys();
void WaitNoKey(int nSecs, void (*pFunc) (void));
int WaitAnyKey(int nSecs);

void UpdateInputs();

void ClearSpaceBar(short nPlayer);

void GetLocalInput();

extern PlayerInput sPlayerInput[];
extern int nNetMoves;

#endif
