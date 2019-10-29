
#ifndef __player_h__
#define __player_h__

#include "compat.h"

void PlayerInterruptKeys();
void RestoreSavePoint(int nPlayer, int *x, int *y, int *z, short *nSector, short *nAngle);
void SetSavePoint(int nPlayer, int x, int y, int z, short nSector, short nAngle);
void InitPlayer();
void InitPlayerKeys(short nPlayer);
void DoKenTest();
int GrabPlayer();
void InitPlayerInventory(short nPlayer);
void RestartPlayer(short nPlayer);

void FuncPlayer(int nSector, int nSprite, int nRun);

#define kMaxPlayers			8
#define kDefaultLives		3
#define kMaxPlayerLives		5
#define kMaxHealth			800

extern int nLocalPlayer;

extern int lPlayerXVel;
extern int lPlayerYVel;
extern int nPlayerDAng;

struct Player
{
    short nHealth;
    short field_2;
    short nAction;
    short nSprite;
    short bIsMummified;
    short someNetVal;
    short invincibility;
    short nAir;
    short nSeq;
    short nMaskAmount;
    uint16_t keys;
    short nMagic;
    char items[8];
    short nAmmo[7]; // TODO - kMaxWeapons?
    short pad[2];

    short nCurrentWeapon;
    short field_34;
    short bIsFiring;
    short field_38;
    short field_3A;
    short field_3C;
    short nRun;

    fix16_t q16angle;
};

extern short PlayerCount;

extern short nPlayerTorch[];

extern short nPlayerLives[];
extern short nPlayerItem[];
extern Player PlayerList[];
extern short nPlayerInvisible[];
extern short nPlayerDouble[];
extern short nPlayerViewSect[];
extern short nPlayerFloorSprite[];

extern short nTauntTimer[];

extern short nDoppleSprite[];

extern uint16_t nPlayerWeapons[];

extern short nPlayerOldWeapon[];
extern short nPlayerGrenade[kMaxPlayers];
extern short nGrenadePlayer[50];

extern short nPistolClip[];

extern short nPlayerScore[];

extern short nPlayerClip[];

extern short bobangle;

extern int totalvel[];
extern short eyelevel[];

extern short nNetStartSprite[kMaxPlayers];
extern short nNetStartSprites;
extern short nCurStartSprite;

extern int nXDamage[kMaxPlayers];
extern int nYDamage[kMaxPlayers];

extern int nPlayerDY[kMaxPlayers];
extern int nPlayerDX[kMaxPlayers];

short GetPlayerFromSprite(short nSprite);
void SetPlayerMummified(int nPlayer, int bIsMummified);
int AddAmmo(int nPlayer, int nWeapon, int nAmmoAmount);
void ShootStaff(int nPlayer);

#endif
