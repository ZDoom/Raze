//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#pragma once 

#include "gamecontrol.h"
#include "gameinput.h"

BEGIN_PS_NS

enum
{
	kMaxPlayers			= 8,
	kDefaultLives		= 3,
	kMaxPlayerLives		= 5,
	kMaxHealth			= 800
};

extern int nLocalPlayer;

struct PlayerSave
{
    sectortype* pSector;
    DVector3 pos;
    DAngle nAngle;
};

class DExhumedPlayer final : public DCorePlayer
{
    DECLARE_CLASS(DExhumedPlayer, DCorePlayer)
    HAS_OBJECT_POINTERS
    DExhumedPlayer() = default;
public:
    DExhumedPlayer(uint8_t p) : DCorePlayer(p) {}
    void Serialize(FSerializer& arc) override;
    void Clear()
    {
        Super::Clear();
        // Quick'n dirty clear
        memset(&nHealth, 0, sizeof(DExhumedPlayer) - myoffsetof(DExhumedPlayer, nHealth));
    }

    int16_t nHealth;
    int16_t nLives;
    int16_t nDouble;
    int16_t nInvisible;
    int16_t nTorch;
    int16_t bIsMummified;
    int16_t invincibility;
    int16_t nAir;
    int16_t nMaskAmount;
    uint16_t keys;
    int16_t nMagic;
    int16_t nItem;
    int8_t items[8];
    int16_t nAmmo[7]; // TODO - kMaxWeapons?

    int16_t nCurrentWeapon;
    uint16_t nWeapFrame;
    int16_t bIsFiring;
    int16_t nNextWeapon;
    int16_t nState;
    int16_t nLastWeapon;
    int16_t nRun;

    sectortype* pPlayerPushSect;
    sectortype* pPlayerViewSect;

    int16_t nBreathTimer;
    int16_t nPlayerSwear;
    int16_t nDeathType;
    int16_t nPlayerScore;
    int16_t nPlayerColor;
    int16_t nPistolClip;
    DVector2 nThrust;
    int16_t nPlayerOldWeapon;
    int16_t nPlayerClip;
    int16_t nPlayerPushSound;
    int16_t nTauntTimer;
    uint16_t nPlayerWeapons; // each set bit represents a weapon the player has
    int16_t dVertPan;
    double nQuake;
    int16_t nTemperature;
    double nStandHeight;
    PlayerSave sPlayerSave;
    double ototalvel;
    double totalvel;
    double nPrevBobZ;
    double nBobZ;
    double nIdxBobZ;
    double nPrevWeapBob;
    double nWeapBob;
    bool bUnderwater;
    bool bTouchFloor;
    bool bJumping;
    bool bRebound;
    TObjPtr<DExhumedActor*> pPlayerGrenade;
    TObjPtr<DExhumedActor*> pPlayerFloorSprite;
    TObjPtr<DExhumedActor*> pDoppleSprite;
    TObjPtr<DExhumedActor*> pTarget;

    inline DExhumedActor* GetActor() override
    {
        return static_cast<DExhumedActor*>(actor);
    }

    const double GetMaxInputVel() const override
    {
        return 15.25;
    }
};

extern int PlayerCount;

inline DExhumedPlayer* getPlayer(int index)
{
    return static_cast<DExhumedPlayer*>(PlayerArray[index]);
}

extern TObjPtr<DExhumedActor*> nNetStartSprite[kMaxPlayers];
extern int nNetStartSprites;
extern int nCurStartSprite;

void SetSavePoint(DExhumedPlayer* const pPlayer, const DVector3& pos, sectortype* pSector, DAngle nAngle);
void InitPlayer();
void InitPlayerKeys(DExhumedPlayer* const pPlayer);
int GrabPlayer();
void InitPlayerInventory(DExhumedPlayer* const pPlayer);
void RestartPlayer(DExhumedPlayer* const pPlayer);
int GetPlayerFromActor(DExhumedActor* actor);
void SetPlayerMummified(DExhumedPlayer* const pPlayer, int bIsMummified);
int AddAmmo(DExhumedPlayer* const pPlayer, int nWeapon, int nAmmoAmount);
void ShootStaff(DExhumedPlayer* const pPlayer);
void updatePlayerTarget(DExhumedPlayer* const pPlayer);

inline void doPlayerVertPanning(DExhumedPlayer* const pPlayer, const double nDestVertPan)
{
    const auto nVertPan = (nDestVertPan - pPlayer->ViewAngles.Pitch.Tan() * 128) * 0.25;
    pPlayer->ViewAngles.Pitch += maphoriz(abs(nVertPan) >= 4 ? Sgn(nVertPan) * 4. : nVertPan * 2.);
}

END_PS_NS

