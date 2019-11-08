
#ifndef __gun_h__
#define __gun_h__

#include "compat.h"
#include "sequence.h"

#define kMaxWeapons		7

enum
{
    kWeaponSword = 0,
    kWeaponPistol,
    kWeaponM60,
    kWeaponFlamer,
    kWeaponGrenade,
    kWeaponStaff,
    kWeaponRing,
    kWeaponMummified
};

struct Weapon
{
    short nSeq;
    short b[12]; // seq offsets?
    short nAmmoType;
    short c;
    short d; // default or min ammo? or ammo used per 'shot' ?
    short bFireUnderwater;
//	short pad[15];
};

extern Weapon WeaponInfo[];
extern short nTemperature[];

void RestoreMinAmmo(short nPlayer);
void FillWeapons(short nPlayer);
void ResetPlayerWeapons(short nPlayer);
void InitWeapons();
void SetNewWeapon(short nPlayer, short nWeapon);
void SetNewWeaponImmediate(short nPlayer, short nWeapon);
void SetNewWeaponIfBetter(short nPlayer, short nWeapon);
void SelectNewWeapon(short nPlayer);
void StopFiringWeapon(short nPlayer);
void FireWeapon(short nPlayer);
void CheckClip(short nPlayer);
void MoveWeapons(short nPlayer);
void DrawWeapons(int smooth);

#endif
