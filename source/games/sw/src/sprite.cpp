//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------
#include "ns.h"
#include "build.h"

#include "names2.h"
#include "panel.h"
#include "misc.h"
#include "tags.h"
#include "ai.h"
#include "light.h"
#include "break.h"
#include "network.h"

#include "pal.h"

#include "sounds.h"
#include "interpolate.h"
#include "interpso.h"
#include "sprite.h"
#include "weapon.h"
#include "jsector.h"
#include "misc.h"
#include "player.h"
#include "quotemgr.h"
#include "v_text.h"
#include "gamecontrol.h"
#include "gamefuncs.h"

BEGIN_SW_NS

// ugly hack stuff.
int parallaxyscale_override, pskybits_override;


int SetupCoolie(DSWActor*);
int SetupNinja(DSWActor*);
int SetupGoro(DSWActor*);
int SetupCoolg(DSWActor*);
int SetupEel(DSWActor*);
int SetupSumo(DSWActor*);
int SetupZilla(DSWActor*);
int SetupToiletGirl(DSWActor*);
int SetupWashGirl(DSWActor*);
int SetupCarGirl(DSWActor*);
int SetupMechanicGirl(DSWActor*);
int SetupSailorGirl(DSWActor*);
int SetupPruneGirl(DSWActor*);
int SetupTrashCan(DSWActor*);
int SetupBunny(DSWActor*);
int SetupRipper(DSWActor*);
int SetupRipper2(DSWActor*);
int SetupSerp(DSWActor*);
int SetupLava(DSWActor* actor);
int SetupSkel(DSWActor*);
int SetupHornet(DSWActor*);
int SetupSkull(DSWActor*);
int SetupBetty(DSWActor*);
int SetupPachinkoLight(DSWActor*);
int SetupPachinko1(DSWActor*);
int SetupPachinko2(DSWActor*);
int SetupPachinko3(DSWActor*);
int SetupPachinko4(DSWActor*);
int SetupGirlNinja(DSWActor*);
ANIMATOR DoVator, DoVatorAuto;
ANIMATOR DoRotator;
ANIMATOR DoSlidor;
ANIMATOR DoSpike, DoSpikeAuto;
ANIMATOR DoLavaErupt;
int DoSlidorInstantClose(DSWActor*);

void InitWeaponRocket(PLAYER*);
void InitWeaponUzi(PLAYER*);

int MoveSkip4, MoveSkip2, MoveSkip8;
int MinEnemySkill;

extern STATE s_CarryFlag[];
extern STATE s_CarryFlagNoDet[];

// beware of mess... :(
static int globhiz, globloz;
static Collision globhihit, globlohit;

short wait_active_check_offset;
int PlaxCeilGlobZadjust, PlaxFloorGlobZadjust;
void SetSectorWallBits(sectortype* sect, int bit_mask, bool set_sectwall, bool set_nextwall);
int DoActorDebris(DSWActor* actor);
void ActorWarpUpdatePos(DSWActor*,sectortype* sect);
void ActorWarpType(DSWActor* sp, DSWActor* act_warp);
int MissileZrange(DSWActor*);

#define ACTIVE_CHECK_TIME (3*120)

TRACK Track[MAX_TRACKS];
SECTOR_OBJECT SectorObject[MAX_SECTOR_OBJECTS];

int DirArr[] = {NORTH, NE, EAST, SE, SOUTH, SW, WEST, NW, NORTH, NE, EAST, SE, SOUTH, SW, WEST, NW};

#define SCROLL_RATE 20
#define SCROLL_FIRE_RATE 20

STATE s_DebrisNinja[] =
{
    {NINJA_DIE + 3, 100, DoActorDebris, &s_DebrisNinja[0]},
};

STATE s_DebrisRat[] =
{
    {750, 100, DoActorDebris, &s_DebrisRat[0]},
};

STATE s_DebrisCrab[] =
{
    {423, 100, DoActorDebris, &s_DebrisCrab[0]},
};

STATE s_DebrisStarFish[] =
{
    {426, 100, DoActorDebris, &s_DebrisStarFish[0]},
};

ANIMATOR DoGet, DoKey, DoSpriteFade;

// temporary
#define ICON_REPAIR_KIT 1813
#define REPAIR_KIT_RATE 1100
STATE s_RepairKit[] =
{
    {ICON_REPAIR_KIT + 0, REPAIR_KIT_RATE, DoGet, &s_RepairKit[0]}
};

STATE s_GoldSkelKey[] =
{
    {GOLD_SKELKEY, 100, DoGet, &s_GoldSkelKey[0]}
};
STATE s_BlueKey[] =
{
    {BLUE_KEY, 100, DoGet, &s_BlueKey[0]}
};
STATE s_BlueCard[] =
{
    {BLUE_CARD, 100, DoGet, &s_BlueCard[0]}
};

STATE s_SilverSkelKey[] =
{
    {SILVER_SKELKEY, 100, DoGet, &s_SilverSkelKey[0]}
};
STATE s_RedKey[] =
{
    {RED_KEY, 100, DoGet, &s_RedKey[0]}
};
STATE s_RedCard[] =
{
    {RED_CARD, 100, DoGet, &s_RedCard[0]}
};

STATE s_BronzeSkelKey[] =
{
    {BRONZE_SKELKEY, 100, DoGet, &s_BronzeSkelKey[0]}
};
STATE s_GreenKey[] =
{
    {GREEN_KEY, 100, DoGet, &s_GreenKey[0]}
};
STATE s_GreenCard[] =
{
    {GREEN_CARD, 100, DoGet, &s_GreenCard[0]}
};

STATE s_RedSkelKey[] =
{
    {RED_SKELKEY, 100, DoGet, &s_RedSkelKey[0]}
};
STATE s_YellowKey[] =
{
    {YELLOW_KEY, 100, DoGet, &s_YellowKey[0]}
};
STATE s_YellowCard[] =
{
    {YELLOW_CARD, 100, DoGet, &s_YellowCard[0]}
};

STATE* s_Key[] =
{
    s_RedKey,
    s_BlueKey,
    s_GreenKey,
    s_YellowKey,
    s_RedCard,
    s_BlueCard,
    s_GreenCard,
    s_YellowCard,
    s_GoldSkelKey,
    s_SilverSkelKey,
    s_BronzeSkelKey,
    s_RedSkelKey
};

#define KEY_RATE 25

#if 0
STATE s_BlueKey[] =
{
    {BLUE_KEY + 0, KEY_RATE, DoKey, &s_BlueKey[0]}
};

STATE s_RedKey[] =
{
    {RED_KEY + 0, KEY_RATE, DoKey, &s_RedKey[0]}
};

STATE s_GreenKey[] =
{
    {GREEN_KEY + 0, KEY_RATE, DoKey, &s_GreenKey[0]}
};

STATE s_YellowKey[] =
{
    {YELLOW_KEY + 0, KEY_RATE, DoKey, &s_YellowKey[0]}
};

STATE* s_Key[] =
{
    s_RedKey,
    s_BlueKey,
    s_GreenKey,
    s_YellowKey,
    s_RedCard,
    s_BlueCard,
    s_GreenCard,
    s_YellowCard,
    s_GoldSkelKey,
    s_SilverSkelKey,
    s_BronzeSkelKey,
    s_RedSkelKey
};
#endif

#if 0
STATE s_BlueKeyStatue[] =
{
    {BLUE_KEY_STATUE + 0, KEY_RATE, DoSpriteFade, &s_BlueKeyStatue[0]}
};

STATE s_RedKeyStatue[] =
{
    {RED_KEY_STATUE + 0, KEY_RATE, DoSpriteFade, &s_RedKeyStatue[0]}
};

STATE s_GreenKeyStatue[] =
{
    {GREEN_KEY_STATUE + 0, KEY_RATE, DoSpriteFade, &s_GreenKeyStatue[0]}
};

STATE s_YellowKeyStatue[] =
{
    {YELLOW_KEY_STATUE + 0, KEY_RATE, DoSpriteFade, &s_YellowKeyStatue[0]}
};

STATE* s_KeyStatue[] =
{
    s_RedKeyStatue,
    s_BlueKeyStatue,
    s_GreenKeyStatue,
    s_YellowKeyStatue,
};
#endif

#define Red_COIN 2440
#define Yellow_COIN 2450
#define Green_COIN 2460
#define RED_COIN_RATE 10
#define YELLOW_COIN_RATE 8
#define GREEN_COIN_RATE 6
ANIMATOR DoCoin;
STATE s_RedCoin[] =
{
    {Red_COIN + 0, RED_COIN_RATE, DoCoin, &s_RedCoin[1]},
    {Red_COIN + 1, RED_COIN_RATE, DoCoin, &s_RedCoin[2]},
    {Red_COIN + 2, RED_COIN_RATE, DoCoin, &s_RedCoin[3]},
    {Red_COIN + 3, RED_COIN_RATE, DoCoin, &s_RedCoin[4]},
    {Red_COIN + 4, RED_COIN_RATE, DoCoin, &s_RedCoin[5]},
    {Red_COIN + 5, RED_COIN_RATE, DoCoin, &s_RedCoin[6]},
    {Red_COIN + 6, RED_COIN_RATE, DoCoin, &s_RedCoin[7]},
    {Red_COIN + 7, RED_COIN_RATE, DoCoin, &s_RedCoin[0]},
};

// !JIM! Frank, I made coins go progressively faster
STATE s_YellowCoin[] =
{
    {Yellow_COIN + 0, YELLOW_COIN_RATE, DoCoin, &s_YellowCoin[1]},
    {Yellow_COIN + 1, YELLOW_COIN_RATE, DoCoin, &s_YellowCoin[2]},
    {Yellow_COIN + 2, YELLOW_COIN_RATE, DoCoin, &s_YellowCoin[3]},
    {Yellow_COIN + 3, YELLOW_COIN_RATE, DoCoin, &s_YellowCoin[4]},
    {Yellow_COIN + 4, YELLOW_COIN_RATE, DoCoin, &s_YellowCoin[5]},
    {Yellow_COIN + 5, YELLOW_COIN_RATE, DoCoin, &s_YellowCoin[6]},
    {Yellow_COIN + 6, YELLOW_COIN_RATE, DoCoin, &s_YellowCoin[7]},
    {Yellow_COIN + 7, YELLOW_COIN_RATE, DoCoin, &s_YellowCoin[0]},
};

STATE s_GreenCoin[] =
{
    {Green_COIN + 0, GREEN_COIN_RATE, DoCoin, &s_GreenCoin[1]},
    {Green_COIN + 1, GREEN_COIN_RATE, DoCoin, &s_GreenCoin[2]},
    {Green_COIN + 2, GREEN_COIN_RATE, DoCoin, &s_GreenCoin[3]},
    {Green_COIN + 3, GREEN_COIN_RATE, DoCoin, &s_GreenCoin[4]},
    {Green_COIN + 4, GREEN_COIN_RATE, DoCoin, &s_GreenCoin[5]},
    {Green_COIN + 5, GREEN_COIN_RATE, DoCoin, &s_GreenCoin[6]},
    {Green_COIN + 6, GREEN_COIN_RATE, DoCoin, &s_GreenCoin[7]},
    {Green_COIN + 7, GREEN_COIN_RATE, DoCoin, &s_GreenCoin[0]},
};

ANIMATOR DoFireFly;

#if 0
STATE s_FireFly[] =
{
    {FIRE_FLY0, 120 * 3, DoFireFly, &s_FireFly[1]},
    {FIRE_FLY1, 20, DoFireFly, &s_FireFly[2]},
    {FIRE_FLY2, 20, DoFireFly, &s_FireFly[3]},
    {FIRE_FLY3, 20, DoFireFly, &s_FireFly[4]},
    {FIRE_FLY4, 60, DoFireFly, &s_FireFly[0]},
};

#else
STATE s_FireFly[] =
{
    {FIRE_FLY0, FIRE_FLY_RATE * 4, DoFireFly, &s_FireFly[0]}
};

#endif


STATE s_IconStar[] =
{
    {ICON_STAR, 100, DoGet, &s_IconStar[0]}
};

STATE s_IconUzi[] =
{
    {ICON_UZI, 100, DoGet, &s_IconUzi[0]}
};

STATE s_IconLgUziAmmo[] =
{
    {ICON_LG_UZI_AMMO, 100, DoGet, &s_IconLgUziAmmo[0]}
};

STATE s_IconUziFloor[] =
{
    {ICON_UZIFLOOR, 100, DoGet, &s_IconUziFloor[0]}
};

STATE s_IconRocket[] =
{
    {ICON_ROCKET, 100, DoGet, &s_IconRocket[0]}
};

STATE s_IconLgRocket[] =
{
    {ICON_LG_ROCKET, 100, DoGet, &s_IconLgRocket[0]}
};

STATE s_IconShotgun[] =
{
    {ICON_SHOTGUN, 100, DoGet, &s_IconShotgun[0]}
};

STATE s_IconLgShotshell[] =
{
    {ICON_LG_SHOTSHELL, 100, DoGet, &s_IconLgShotshell[0]}
};

STATE s_IconAutoRiot[] =
{
    {ICON_AUTORIOT, 100, DoGet, &s_IconAutoRiot[0]}
};

STATE s_IconGrenadeLauncher[] =
{
    {ICON_GRENADE_LAUNCHER, 100, DoGet, &s_IconGrenadeLauncher[0]}
};

STATE s_IconLgGrenade[] =
{
    {ICON_LG_GRENADE, 100, DoGet, &s_IconLgGrenade[0]}
};


STATE s_IconLgMine[] =
{
    {ICON_LG_MINE, 100, DoGet, &s_IconLgMine[0]}
};

STATE s_IconGuardHead[] =
{
    {ICON_GUARD_HEAD + 0, 15, DoGet, &s_IconGuardHead[0]},
//    {ICON_GUARD_HEAD + 1, 15, DoGet, &s_IconGuardHead[2]},
//    {ICON_GUARD_HEAD + 2, 15, DoGet, &s_IconGuardHead[0]}
};


#define FIREBALL_LG_AMMO_RATE 12
STATE s_IconFireballLgAmmo[] =
{
    {ICON_FIREBALL_LG_AMMO + 0, FIREBALL_LG_AMMO_RATE, DoGet, &s_IconFireballLgAmmo[1]},
    {ICON_FIREBALL_LG_AMMO + 1, FIREBALL_LG_AMMO_RATE, DoGet, &s_IconFireballLgAmmo[2]},
    {ICON_FIREBALL_LG_AMMO + 2, FIREBALL_LG_AMMO_RATE, DoGet, &s_IconFireballLgAmmo[0]},
};

STATE s_IconHeart[] =
{
    {ICON_HEART + 0, 25, DoGet, &s_IconHeart[1]},
    {ICON_HEART + 1, 25, DoGet, &s_IconHeart[0]},
};

#define HEART_LG_AMMO_RATE 12
STATE s_IconHeartLgAmmo[] =
{
    {ICON_HEART_LG_AMMO + 0, HEART_LG_AMMO_RATE, DoGet, &s_IconHeartLgAmmo[1]},
    {ICON_HEART_LG_AMMO + 1, HEART_LG_AMMO_RATE, DoGet, &s_IconHeartLgAmmo[0]},
};

STATE s_IconMicroGun[] =
{
    {ICON_MICRO_GUN, 100, DoGet, &s_IconMicroGun[0]}
};

STATE s_IconMicroBattery[] =
{
    {ICON_MICRO_BATTERY, 100, DoGet, &s_IconMicroBattery[0]}
};

// !JIM!  Added rail crap
STATE s_IconRailGun[] =
{
    {ICON_RAIL_GUN, 100, DoGet, &s_IconRailGun[0]}
};

STATE s_IconRailAmmo[] =
{
    {ICON_RAIL_AMMO, 100, DoGet, &s_IconRailAmmo[0]}
};


STATE s_IconElectro[] =
{
    {ICON_ELECTRO + 0, 25, DoGet, &s_IconElectro[1]},
    {ICON_ELECTRO + 1, 25, DoGet, &s_IconElectro[0]},
};

#define ICON_SPELL_RATE 8

STATE s_IconSpell[] =
{
    {ICON_SPELL + 0, ICON_SPELL_RATE, DoGet, &s_IconSpell[1]},
    {ICON_SPELL + 1, ICON_SPELL_RATE, DoGet, &s_IconSpell[2]},
    {ICON_SPELL + 2, ICON_SPELL_RATE, DoGet, &s_IconSpell[3]},
    {ICON_SPELL + 3, ICON_SPELL_RATE, DoGet, &s_IconSpell[4]},
    {ICON_SPELL + 4, ICON_SPELL_RATE, DoGet, &s_IconSpell[5]},
    {ICON_SPELL + 5, ICON_SPELL_RATE, DoGet, &s_IconSpell[6]},
    {ICON_SPELL + 6, ICON_SPELL_RATE, DoGet, &s_IconSpell[7]},
    {ICON_SPELL + 7, ICON_SPELL_RATE, DoGet, &s_IconSpell[8]},
    {ICON_SPELL + 8, ICON_SPELL_RATE, DoGet, &s_IconSpell[9]},
    {ICON_SPELL + 9, ICON_SPELL_RATE, DoGet, &s_IconSpell[10]},
    {ICON_SPELL + 10, ICON_SPELL_RATE, DoGet, &s_IconSpell[11]},
    {ICON_SPELL + 11, ICON_SPELL_RATE, DoGet, &s_IconSpell[12]},
    {ICON_SPELL + 12, ICON_SPELL_RATE, DoGet, &s_IconSpell[13]},
    {ICON_SPELL + 13, ICON_SPELL_RATE, DoGet, &s_IconSpell[14]},
    {ICON_SPELL + 14, ICON_SPELL_RATE, DoGet, &s_IconSpell[15]},
    {ICON_SPELL + 15, ICON_SPELL_RATE, DoGet, &s_IconSpell[0]},
};

STATE s_IconArmor[] =
{
    {ICON_ARMOR + 0, 15, DoGet, &s_IconArmor[0]},
};

STATE s_IconMedkit[] =
{
    {ICON_MEDKIT + 0, 15, DoGet, &s_IconMedkit[0]},
};

STATE s_IconChemBomb[] =
{
    {ICON_CHEMBOMB, 15, DoGet, &s_IconChemBomb[0]},
};

STATE s_IconFlashBomb[] =
{
    {ICON_FLASHBOMB, 15, DoGet, &s_IconFlashBomb[0]},
};

STATE s_IconNuke[] =
{
    {ICON_NUKE, 15, DoGet, &s_IconNuke[0]},
};

STATE s_IconCaltrops[] =
{
    {ICON_CALTROPS, 15, DoGet, &s_IconCaltrops[0]},
};

#define ICON_SM_MEDKIT 1802
STATE s_IconSmMedkit[] =
{
    {ICON_SM_MEDKIT + 0, 15, DoGet, &s_IconSmMedkit[0]},
};

#define ICON_BOOSTER 1810
STATE s_IconBooster[] =
{
    {ICON_BOOSTER + 0, 15, DoGet, &s_IconBooster[0]},
};

#define ICON_HEAT_CARD 1819
STATE s_IconHeatCard[] =
{
    {ICON_HEAT_CARD + 0, 15, DoGet, &s_IconHeatCard[0]},
};

#if 0
STATE s_IconEnvironSuit[] =
{
    {ICON_ENVIRON_SUIT + 0, 20, DoGet, &s_IconEnvironSuit[0]},
};
#endif

STATE s_IconCloak[] =
{
//   {ICON_CLOAK + 0, 20, DoGet, &s_IconCloak[1]},
//   {ICON_CLOAK + 1, 20, DoGet, &s_IconCloak[2]},
    {ICON_CLOAK + 0, 20, DoGet, &s_IconCloak[0]},
};

STATE s_IconFly[] =
{
    {ICON_FLY + 0, 20, DoGet, &s_IconFly[1]},
    {ICON_FLY + 1, 20, DoGet, &s_IconFly[2]},
    {ICON_FLY + 2, 20, DoGet, &s_IconFly[3]},
    {ICON_FLY + 3, 20, DoGet, &s_IconFly[4]},
    {ICON_FLY + 4, 20, DoGet, &s_IconFly[5]},
    {ICON_FLY + 5, 20, DoGet, &s_IconFly[6]},
    {ICON_FLY + 6, 20, DoGet, &s_IconFly[7]},
    {ICON_FLY + 7, 20, DoGet, &s_IconFly[0]}
};

STATE s_IconNightVision[] =
{
    {ICON_NIGHT_VISION + 0, 20, DoGet, &s_IconNightVision[0]},
};

STATE s_IconFlag[] =
{
    {ICON_FLAG + 0, 32, DoGet, &s_IconFlag[1]},
    {ICON_FLAG + 1, 32, DoGet, &s_IconFlag[2]},
    {ICON_FLAG + 2, 32, DoGet, &s_IconFlag[0]}
};

void SetOwner(DSWActor* ownr, DSWActor* child, bool flag)
{
    if (flag && ownr != nullptr && ownr->hasU())
    {
        ownr->user.Flags2 |= (SPR2_CHILDREN);
    }
    child->ownerActor = ownr;
}


DSWActor* GetOwner(DSWActor* child)
{
    return child ? child->ownerActor.Get() : nullptr;
}

void ClearOwner(DSWActor* child)
{
    if (child) child->ownerActor = nullptr;
}

void SetAttach(DSWActor* ownr, DSWActor* child)
{
    if (child && child->hasU() && ownr->hasU())
    {
        ownr->user.Flags2 |= (SPR2_CHILDREN);
        child->user.attachActor = ownr;
    }
}

void KillActor(DSWActor* actor)
{
    int i;
    unsigned stat;
    //extern short Zombies;

    ASSERT(!Prediction);

    ASSERT(actor->spr.statnum < MAXSTATUS);

    //////////////////////////////////////////////
    //    Check sounds list to kill attached sounds
    DeleteNoSoundOwner(actor);
    DeleteNoFollowSoundOwner(actor);
    //////////////////////////////////////////////

    if (actor->hasU())
    {
        PLAYER* pp;
        int pnum;

        // doing a MissileSetPos - don't allow killing
        if (actor->user.Flags & (SPR_SET_POS_DONT_KILL))
            return;

        // for attached sprites that are getable make sure they don't have
        // any Anims attached
        AnimDelete(ANIM_Userz, 0, actor);
        AnimDelete(ANIM_Spritez, 0, actor);
        StopInterpolation(actor, Interp_Sprite_Z);

        // adjust sprites attached to sector objects
        if (actor->user.Flags & (SPR_SO_ATTACHED))
        {
            SECTOR_OBJECT* sop;
            int sn, FoundSpriteNdx = -1;

            for (sop = SectorObject; sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++)
            {
                for (sn = 0; sop->so_actors[sn] != nullptr; sn++)
                {
                    if (sop->so_actors[sn] == actor)
                    {
                        FoundSpriteNdx = sn;
                    }
                }

                if (FoundSpriteNdx >= 0)
                {
                    // back up sn so it points to the last valid sprite num
                    sn--;
                    ASSERT(sop->so_actors[sn] != nullptr);

                    so_stopspriteinterpolation(sop, actor);
                    // replace the one to be deleted with the last ndx
                    sop->so_actors[FoundSpriteNdx] = sop->so_actors[sn];
                    // the last ndx is not -1
                    sop->so_actors[sn] = nullptr;

                    break;
                }
            }
        }

        // if a player is dead and watching this sprite
        // reset it.
        TRAVERSE_CONNECT(pnum)
        {
            pp = Player + pnum;

            if (pp->KillerActor != nullptr)
            {
                if (pp->KillerActor == actor)
                {
                    pp->KillerActor = nullptr;;
                }
            }
        }

        // if on a track and died reset the track to non-occupied
        if (actor->spr.statnum == STAT_ENEMY)
        {
            if (actor->user.track != -1)
            {
                if (Track[actor->user.track].flags)
                    Track[actor->user.track].flags &= ~(TF_TRACK_OCCUPIED);
            }
        }

        // if missile is heading for the sprite, the missile need to know
        // that it is already dead
        if ((actor->spr.extra & SPRX_PLAYER_OR_ENEMY))
        {
            static int8_t MissileStats[] = {STAT_MISSILE, STAT_MISSILE_SKIP4};

            for (stat = 0; stat < SIZ(MissileStats); stat++)
            {
                SWStatIterator it(MissileStats[stat]);
                while (auto itActor = it.Next())
                {
                    if (!itActor->hasU()) continue;
                    if (itActor->user.WpnGoalActor == itActor)
                    {
                        itActor->user.WpnGoalActor = nullptr;
                    }
                }
            }
        }

        // much faster
        if (actor->user.Flags2 & (SPR2_CHILDREN))
        {
            // check for children and alert them that the Owner is dead
            // don't bother th check if you've never had children
            for (stat = 0; stat < STAT_DONT_DRAW; stat++)
            {
                SWStatIterator it(stat);
                while (auto itActor = it.Next())
                {
                    if (GetOwner(itActor) == actor)
                    {
                        ClearOwner(itActor);
                    }


                    if (itActor->hasU() && itActor->user.attachActor == actor)
                    {
                        itActor->user.attachActor = nullptr;
                    }
                }
            }
        }

        if (actor->spr.statnum == STAT_ENEMY)
        {
            SWStatIterator it(STAT_ENEMY);
            while (auto itActor = it.Next())
            {
                if (itActor->hasU() && itActor->user.targetActor == actor)
                {
                    DoActorPickClosePlayer(itActor);
                }
            }
        }

        if (actor->user.flameActor != nullptr)
        {
            SetSuicide(actor->user.flameActor);
        }
    }
    actor->Destroy();
}

void ChangeState(DSWActor* actor, STATE* statep)
{
    if (!actor->hasU())
        return;

    actor->user.Tics = 0;
    actor->user.State = actor->user.StateStart = statep;
    // Just in case
    PicAnimOff(actor->user.State->Pic);
}

void change_actor_stat(DSWActor* actor, int stat, bool quick)
{

    ChangeActorStat(actor, stat);

    if (actor->hasU() && !quick)
    {
        actor->user.Flags &= ~(SPR_SKIP2|SPR_SKIP4);

        if (stat >= STAT_SKIP4_START && stat <= STAT_SKIP4_END)
            actor->user.Flags |= (SPR_SKIP4);

        if (stat >= STAT_SKIP2_START && stat <= STAT_SKIP2_END)
            actor->user.Flags |= (SPR_SKIP2);

        switch (stat)
        {
            //case STAT_DEAD_ACTOR:
            //case STAT_ITEM:
            //case STAT_SKIP4:
#if 0
        case STAT_PLAYER0:
        case STAT_PLAYER1:
        case STAT_PLAYER2:
        case STAT_PLAYER3:
        case STAT_PLAYER4:
        case STAT_PLAYER5:
        case STAT_PLAYER6:
        case STAT_PLAYER7:
#endif
        case STAT_ENEMY_SKIP4:
        case STAT_ENEMY:
            // for enemys - create offsets so all enemys don't call FAFcansee at once
            wait_active_check_offset += ACTORMOVETICS*3;
            if (wait_active_check_offset > ACTIVE_CHECK_TIME)
                wait_active_check_offset = 0;
            actor->user.wait_active_check = wait_active_check_offset;
            // don't do a break here
            actor->user.Flags |= (SPR_SHADOW);
            break;
        }

    }
}

void SpawnUser(DSWActor* actor, short id, STATE* state)
{
    ASSERT(!Prediction);

    actor->clearUser();    // make sure to delete old, stale content first!
    actor->allocUser();

    PRODUCTION_ASSERT(actor->hasU());

    // be careful State can be nullptr
    actor->user.State = actor->user.StateStart = state;

    change_actor_stat(actor, actor->spr.statnum);

    actor->user.ID = id;
    actor->user.Health = 100;
    actor->user.WpnGoalActor = nullptr;
    actor->user.attachActor = nullptr;
    actor->user.track = -1;
    actor->user.targetActor = Player[0].actor;
    actor->user.Radius = 220;
    actor->user.Sibling = -1;
    actor->user.WaitTics = 0;
    actor->user.OverlapZ = Z(4);
    actor->user.bounce = 0;

    actor->user.motion_blur_num = 0;
    actor->user.motion_blur_dist = 256;

    actor->backuppos();
    actor->user.oz = actor->opos.Z;

    actor->user.active_range = MIN_ACTIVE_RANGE;

    // default

    // based on clipmove z of 48 pixels off the floor
    actor->user.floor_dist = Z(48) - Z(28);
    actor->user.ceiling_dist = Z(8);

    // Problem with sprites spawned really close to white sector walls
    // cant do a getzrange there
    // Just put in some valid starting values
    actor->user.loz = actor->sector()->int_floorz();
    actor->user.hiz = actor->sector()->int_ceilingz();
    actor->user.lowActor = nullptr;
    actor->user.highActor = nullptr;
    actor->user.lo_sectp = actor->sector();
    actor->user.hi_sectp = actor->sector();
}

DSWActor* SpawnActor(int stat, int id, STATE* state, sectortype* sect, int x, int y, int z, int init_ang, int vel)
{
    if (sect == nullptr)
        return nullptr;

    ASSERT(!Prediction);

    auto spawnedActor = insertActor(sect, stat);

    spawnedActor->set_int_pos({ x, y, z });

    SpawnUser(spawnedActor, id, state);

    // be careful State can be nullptr
    if (spawnedActor->user.State)
    {
        spawnedActor->spr.picnum = spawnedActor->user.State->Pic;
        PicAnimOff(spawnedActor->spr.picnum);
    }

    spawnedActor->spr.xrepeat = 64;
    spawnedActor->spr.yrepeat = 64;
    spawnedActor->spr.ang = NORM_ANGLE(init_ang);
    spawnedActor->spr.xvel = vel;

    return spawnedActor;
}

void PicAnimOff(short picnum)
{
    short anim_type = (picanm[picnum].sf & PICANM_ANIMTYPE_MASK) >> PICANM_ANIMTYPE_SHIFT;

    ASSERT(picnum >= 0 && picnum < MAXTILES);

    if (!anim_type)
        return;

    picanm[picnum].sf &= ~(PICANM_ANIMTYPE_MASK);
}

bool IconSpawn(DSWActor* actor)
{
    // if multi item and not a modem game
    if ((actor->spr.extra & SPRX_MULTI_ITEM))
    {
        if (numplayers <= 1 || gNet.MultiGameType == MULTI_GAME_COOPERATIVE)
            return false;
    }
    actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN | CSTAT_SPRITE_ONE_SIDE | CSTAT_SPRITE_ALIGNMENT_FLOOR);
    return true;
}

bool ActorTestSpawn(DSWActor* actor)
{
    if (actor->spr.statnum == STAT_DEFAULT && actor->spr.lotag == TAG_SPAWN_ACTOR)
    {
        auto actorNew = insertActor(actor->sector(), STAT_DEFAULT);
        actorNew->spr = actor->spr;
        change_actor_stat(actorNew, STAT_SPAWN_TRIGGER);
        actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
        return false;
    }

    // Countermeasure for mods that leave the lower skills unpopulated.
    int spawnskill = (actor->spr.extra & SPRX_SKILL);
    if (MinEnemySkill > 0 && spawnskill == MinEnemySkill) spawnskill = 0;

    // Skill ranges from -1 (No Monsters) to 3.
    if (spawnskill > Skill)
    {
        // JBF: hack to fix Wanton Destruction's missing Sumos, Serpents, and Zilla on Skill < 2
        if (((actor->spr.picnum == SERP_RUN_R0 || actor->spr.picnum == SUMO_RUN_R0) && actor->spr.lotag > 0 &&
             actor->spr.lotag != TAG_SPAWN_ACTOR && actor->spr.extra > 0) || actor->spr.picnum == ZILLA_RUN_R0)
        {
            const char *c;

            // NOTE: Wanton's $boat.map has two sumos, neither of which actually activate
            // anything but are spawned in, and one of them has a skill level 2 mask. This
            // hack however forces both sumos to appear on the easy levels. Bummer.
            switch (actor->spr.picnum)
            {
            case SERP_RUN_R0: c = "serpent"; break;
            case SUMO_RUN_R0: c = "sumo"; break;
            case ZILLA_RUN_R0: c = "zilla"; break;
            default: c = "?"; break;
            }
            Printf("WARNING: skill-masked %s at %d,%d,%d not being killed because it "
                        "activates something\n", c, actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z);
            return true;
        }
        //always spawn girls in addons
        if ((actor->spr.picnum == TOILETGIRL_R0 ||
            actor->spr.picnum == WASHGIRL_R0 ||
            actor->spr.picnum == MECHANICGIRL_R0 ||
            actor->spr.picnum == CARGIRL_R0 ||
            actor->spr.picnum == PRUNEGIRL_R0 ||
            actor->spr.picnum == SAILORGIRL_R0) && (g_gameType & GAMEFLAG_ADDON)) return true;

        // spawn Bouncing Betty (mine) in TD map 09 Warehouse 
        if (actor->spr.picnum == 817 && (currentLevel->gameflags & LEVEL_SW_SPAWNMINES))
            return true;

        return false;
    }

    return true;
}


int EnemyCheckSkill()
{
    SWStatIterator it(STAT_DEFAULT);
    int maxskill = INT_MAX;
    while (auto actor = it.Next())
    {
        switch (actor->spr.picnum)
        {
        case COOLIE_RUN_R0:
        case NINJA_RUN_R0:
        case NINJA_CRAWL_R0:
        case GORO_RUN_R0:
        case 1441:
        case COOLG_RUN_R0:
        case EEL_RUN_R0:
        case SUMO_RUN_R0:
        case ZILLA_RUN_R0:
        case RIPPER_RUN_R0:
        case RIPPER2_RUN_R0:
        case SERP_RUN_R0:
        case LAVA_RUN_R0:
        case SKEL_RUN_R0:
        case HORNET_RUN_R0:
        case SKULL_R0:
        case BETTY_R0:
        case GIRLNINJA_RUN_R0:
        {
            int myskill = actor->spr.extra & SPRX_SKILL;
            if (myskill < maxskill) maxskill = myskill;
        }
        }
    }
    if (maxskill < 0 || maxskill == INT_MAX) maxskill = 0;
    return maxskill;
}

bool ActorSpawn(DSWActor* actor)
{
    bool ret = true;
    int picnum = actor->spr.picnum;
    switch (picnum)
    {
    case COOLIE_RUN_R0:
    {
        if (!ActorTestSpawn(actor))
        {
            KillActor(actor);
            return false;
        }

        PicAnimOff(picnum);
        SetupCoolie(actor);

        break;
    }

    case NINJA_RUN_R0:
    case NINJA_CRAWL_R0:
    {
        if (!ActorTestSpawn(actor))
        {
            KillActor(actor);
            return false;
        }

        PicAnimOff(picnum);
        SetupNinja(actor);

        break;
    }

    case GORO_RUN_R0:
    {
        if (!ActorTestSpawn(actor))
        {
            KillActor(actor);
            return false;
        }

        PicAnimOff(picnum);
        SetupGoro(actor);
        break;
    }

    case 1441:
    case COOLG_RUN_R0:
    {
        if (!ActorTestSpawn(actor))
        {
            KillActor(actor);
            return false;
        }

        PicAnimOff(picnum);
        SetupCoolg(actor);
        break;
    }

    case EEL_RUN_R0:
    {
        if (!ActorTestSpawn(actor))
        {
            KillActor(actor);
            return false;
        }

        PicAnimOff(picnum);
        SetupEel(actor);
        break;
    }

    case SUMO_RUN_R0:
    {
        if (!ActorTestSpawn(actor))
        {
            KillActor(actor);
            return false;
        }

        PicAnimOff(picnum);
        SetupSumo(actor);

        break;
    }

    case ZILLA_RUN_R0:
    {
        if (!ActorTestSpawn(actor))
        {
            KillActor(actor);
            return false;
        }

        PicAnimOff(picnum);
        SetupZilla(actor);

        break;
    }

    case TOILETGIRL_R0:
    {
        if (!ActorTestSpawn(actor))
        {
            KillActor(actor);
            return false;
        }

        PicAnimOff(picnum);
        SetupToiletGirl(actor);

        break;
    }

    case WASHGIRL_R0:
    {
        if (!ActorTestSpawn(actor))
        {
            KillActor(actor);
            return false;
        }

        PicAnimOff(picnum);
        SetupWashGirl(actor);

        break;
    }

    case CARGIRL_R0:
    {
        if (!ActorTestSpawn(actor))
        {
            KillActor(actor);
            return false;
        }

        PicAnimOff(picnum);
        SetupCarGirl(actor);

        break;
    }

    case MECHANICGIRL_R0:
    {
        if (!ActorTestSpawn(actor))
        {
            KillActor(actor);
            return false;
        }

        PicAnimOff(picnum);
        SetupMechanicGirl(actor);

        break;
    }

    case SAILORGIRL_R0:
    {
        if (!ActorTestSpawn(actor))
        {
            KillActor(actor);
            return false;
        }

        PicAnimOff(picnum);
        SetupSailorGirl(actor);

        break;
    }

    case PRUNEGIRL_R0:
    {
        if (!ActorTestSpawn(actor))
        {
            KillActor(actor);
            return false;
        }

        PicAnimOff(picnum);
        SetupPruneGirl(actor);

        break;
    }

    case TRASHCAN:
    {
        PicAnimOff(picnum);
        SetupTrashCan(actor);
        break;
    }

    case BUNNY_RUN_R0:
    {
        if (!ActorTestSpawn(actor))
        {
            KillActor(actor);
            return false;
        }

        PicAnimOff(picnum);
        SetupBunny(actor);
        break;
    }

    case RIPPER_RUN_R0:
    {
        if (!ActorTestSpawn(actor))
        {
            KillActor(actor);
            return false;
        }

        PicAnimOff(picnum);
        SetupRipper(actor);
        break;
    }

    case RIPPER2_RUN_R0:
    {
        if (!ActorTestSpawn(actor))
        {
            KillActor(actor);
            return false;
        }

        PicAnimOff(picnum);
        SetupRipper2(actor);
        break;
    }

    case SERP_RUN_R0:
    {
        if (!ActorTestSpawn(actor))
        {
            KillActor(actor);
            return false;
        }

        PicAnimOff(picnum);
        SetupSerp(actor);
        break;
    }

    case LAVA_RUN_R0:
    {

        if (!ActorTestSpawn(actor))
        {
            KillActor(actor);
            return false;
        }

        PicAnimOff(picnum);
        SetupLava(actor);
        break;
    }

    case SKEL_RUN_R0:
    {
        if (!ActorTestSpawn(actor))
        {
            KillActor(actor);
            return false;
        }

        PicAnimOff(picnum);
        SetupSkel(actor);
        break;
    }

    case HORNET_RUN_R0:
    {
        if (!ActorTestSpawn(actor))
        {
            KillActor(actor);
            return false;
        }

        PicAnimOff(picnum);
        SetupHornet(actor);
        break;
    }

    case SKULL_R0:
    {
        if (!ActorTestSpawn(actor))
        {
            KillActor(actor);
            return false;
        }

        PicAnimOff(picnum);
        SetupSkull(actor);
        break;
    }

    case BETTY_R0:
    {
        if (!ActorTestSpawn(actor))
        {
            KillActor(actor);
            return false;
        }

        PicAnimOff(picnum);
        SetupBetty(actor);
        break;
    }

    case 623:   // Pachinko win light
    {
        PicAnimOff(picnum);
        SetupPachinkoLight(actor);
        break;
    }

    case PACHINKO1:
    {
        PicAnimOff(picnum);
        SetupPachinko1(actor);
        break;
    }

    case PACHINKO2:
    {
        PicAnimOff(picnum);
        SetupPachinko2(actor);
        break;
    }

    case PACHINKO3:
    {
        PicAnimOff(picnum);
        SetupPachinko3(actor);
        break;
    }

    case PACHINKO4:
    {
        PicAnimOff(picnum);
        SetupPachinko4(actor);
        break;
    }

    case GIRLNINJA_RUN_R0:
    {
        if (!ActorTestSpawn(actor))
        {
            KillActor(actor);
            return false;
        }

        PicAnimOff(picnum);
        SetupGirlNinja(actor);

        break;
    }

    default:
        ret = false;
        break;
    }

    return ret;
}


void IconDefault(DSWActor* actor)
{
    change_actor_stat(actor, STAT_ITEM);

    actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    actor->user.Radius = 650;

    DoActorZrange(actor);
}

void PreMapCombineFloors(void)
{
    const int MAX_FLOORS = 32;
    int i, j, k;
    int base_offset;
    int dx, dy;
    short pnum;

    struct BOUND_LIST
    {
        DSWActor* offset;
    };

    BOUND_LIST BoundList[MAX_FLOORS];

    memset(BoundList, 0, MAX_FLOORS * sizeof(BOUND_LIST));

    SWStatIterator it(0);
    while (auto actor = it.Next())
    {
        if (actor->spr.picnum != ST1)
            continue;

        if (actor->spr.hitag == BOUND_FLOOR_OFFSET || actor->spr.hitag == BOUND_FLOOR_BASE_OFFSET)
        {
            ASSERT(actor->spr.lotag < MAX_FLOORS);
            BoundList[actor->spr.lotag].offset = actor;
            change_actor_stat(actor, STAT_FAF);
        }
    }

    for (i = base_offset = 0; i < MAX_FLOORS; i++)
    {
        // blank so continue
        if (!BoundList[i].offset)
            continue;

        if (BoundList[i].offset->spr.hitag == BOUND_FLOOR_BASE_OFFSET)
        {
            base_offset = i;
            continue;
        }

        dx = BoundList[base_offset].offset->int_pos().X - BoundList[i].offset->int_pos().X;
        dy = BoundList[base_offset].offset->int_pos().Y - BoundList[i].offset->int_pos().Y;

        BFSSectorSearch search(BoundList[i].offset->sector());
        while (auto dasect = search.GetNext())
        {
            SWSectIterator it2(dasect);
            while (auto jActor = it2.Next())
            {
                jActor->add_int_pos({ dx, dy, 0 });
            }

            for (auto& wal : wallsofsector(dasect))
            {
                wal.movexy(wal.wall_int_pos().X + dx, wal.wall_int_pos().Y + dy);

                if (wal.twoSided())
                    search.Add(wal.nextSector());
            }
        }

        TRAVERSE_CONNECT(pnum)
        {
            PLAYER* pp = &Player[pnum];
            auto dasect = pp->cursector;
            search.Rewind();
            while (auto itsect = search.GetNext())
            {
                if (itsect == dasect)
                {
                    pp->pos.X += dx;
                    pp->pos.Y += dy;
                    pp->opos.X = pp->oldpos.X = pp->pos.X;
                    pp->opos.Y = pp->oldpos.Y = pp->pos.Y;
                    break;
                }
            }
        }

    }

    // get rid of the sprites used
    it.Reset(STAT_FAF);
    while (auto actor = it.Next())
    {
        KillActor(actor);
    }
}


void SpriteSetupPost(void)
{
    int cz,fz;

    // Post processing of some sprites after gone through the main SpriteSetup()
    // routine

    SWStatIterator it(STAT_FLOOR_PAN);
    while (auto iActor = it.Next())
    {
        SWSectIterator it2(iActor->sector());
        while (auto jActor = it.Next())
        {
            if (jActor->spr.picnum == ST1)
                continue;

            if ((jActor->spr.cstat & (CSTAT_SPRITE_ALIGNMENT_WALL|CSTAT_SPRITE_ALIGNMENT_FLOOR)))
                continue;

            if (jActor->hasU())
                continue;

            getzsofslopeptr(jActor->sector(), jActor->int_pos().X, jActor->int_pos().Y, &cz, &fz);
            if (labs(jActor->int_pos().Z - fz) > Z(4))
                continue;

            SpawnUser(jActor, 0, nullptr);
            change_actor_stat(jActor, STAT_NO_STATE);
            jActor->user.ceiling_dist = Z(4);
            jActor->user.floor_dist = -Z(2);

            jActor->user.ActorActionFunc = DoActorDebris;

            jActor->spr.cstat |= CSTAT_SPRITE_BREAKABLE;
            jActor->spr.extra |= SPRX_BREAKABLE;
        }
    }
}


void SpriteSetup(void)
{
    short num;
    int cz,fz;

    MinEnemySkill = EnemyCheckSkill();

    // special case for player
    PicAnimOff(PLAYER_NINJA_RUN_R0);

    // Clear Sprite Extension structure

    // Clear all extra bits - they are set by sprites
    for(auto& sect: sector)
    {
        sect.extra = 0;
    }

    // Clear PARALLAX_LEVEL overrides
    parallaxyscale_override = 0;
    pskybits_override = -1;

    // Call my little sprite setup routine first
    JS_SpriteSetup();

    int minEnemySkill = EnemyCheckSkill();

    SWStatIterator it(STAT_DEFAULT);
    while (auto actor = it.Next())
    {
        // not used yetv
        getzsofslopeptr(actor->sector(), actor->int_pos().X, actor->int_pos().Y, &cz, &fz);
        if (actor->int_pos().Z > ((cz + fz) >> 1))
        {
            // closer to a floor
            actor->spr.cstat |= (CSTAT_SPRITE_CLOSE_FLOOR);
        }

        // CSTAT_SPIN is insupported - get rid of it
        if ((actor->spr.cstat & (CSTAT_SPRITE_ALIGNMENT_MASK)) == CSTAT_SPRITE_ALIGNMENT_SLAB)
            actor->spr.cstat &= ~(CSTAT_SPRITE_ALIGNMENT_SLAB);

        // if BLOCK is set set BLOCK_HITSCAN
        // Hope this doesn't screw up anything
        if (actor->spr.cstat & (CSTAT_SPRITE_BLOCK))
            actor->spr.cstat |= (CSTAT_SPRITE_BLOCK_HITSCAN);

        ////////////////////////////////////////////
        //
        // BREAKABLE CHECK
        //
        ////////////////////////////////////////////

        // USER SETUP - TAGGED BY USER
        // Non ST1 sprites that are tagged like them
        if (TEST_BOOL1(actor) && actor->spr.picnum != ST1)
        {
            actor->spr.extra &= ~(
                  SPRX_BOOL4|
                  SPRX_BOOL5|
                  SPRX_BOOL6|
                  SPRX_BOOL7|
                  SPRX_BOOL8|
                  SPRX_BOOL9|
                  SPRX_BOOL10);

            switch (actor->spr.hitag)
            {
            case BREAKABLE:
                // need something that tells missiles to hit them
                // but allows actors to move through them
                actor->spr.clipdist = ActorSizeX(actor);
                actor->spr.extra |= (SPRX_BREAKABLE);
                actor->spr.cstat |= (CSTAT_SPRITE_BREAKABLE);
                break;
            }
        }
        else
        {
            // BREAK SETUP TABLE AUTOMATED
            SetupSpriteForBreak(actor);
        }

        if (actor->spr.lotag == TAG_SPRITE_HIT_MATCH)
        {
            // if multi item and not a modem game
            if ((actor->spr.extra & SPRX_MULTI_ITEM))
            {
                if (numplayers <= 1 || gNet.MultiGameType == MULTI_GAME_COOPERATIVE)
                {
                    KillActor(actor);
                    continue;
                }
            }


            // crack sprite
            if (actor->spr.picnum == 80)
            {
                actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK);
                actor->spr.cstat |= (CSTAT_SPRITE_BLOCK_HITSCAN|CSTAT_SPRITE_BLOCK_MISSILE);;
            }
            else
            {
                actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK);
                actor->spr.cstat |= (CSTAT_SPRITE_BLOCK_HITSCAN|CSTAT_SPRITE_BLOCK_MISSILE);;
                actor->spr.cstat |= (CSTAT_SPRITE_INVISIBLE);;
            }

            if (SP_TAG8(actor) & BIT(0))
                actor->spr.cstat |= (CSTAT_SPRITE_INVISIBLE);

            if (SP_TAG8(actor) & BIT(1))
                actor->spr.cstat &= ~(CSTAT_SPRITE_INVISIBLE);

            change_actor_stat(actor, STAT_SPRITE_HIT_MATCH);
            continue;
        }

        if (actor->spr.picnum >= TRACK_SPRITE &&
            actor->spr.picnum <= TRACK_SPRITE + MAX_TRACKS)
        {
            short track_num;

            // skip this sprite, just for numbering walls/sectors
            if (actor->spr.cstat & (CSTAT_SPRITE_ALIGNMENT_WALL))
                continue;

            track_num = actor->spr.picnum - TRACK_SPRITE + 0;

            change_actor_stat(actor, STAT_TRACK + track_num);

            continue;
        }

        if (ActorSpawn(actor))
            continue;

        switch (actor->spr.picnum)
        {
        case ST_QUICK_JUMP:
            change_actor_stat(actor, STAT_QUICK_JUMP);
            break;
        case ST_QUICK_JUMP_DOWN:
            change_actor_stat(actor, STAT_QUICK_JUMP_DOWN);
            break;
        case ST_QUICK_SUPER_JUMP:
            change_actor_stat(actor, STAT_QUICK_SUPER_JUMP);
            break;
        case ST_QUICK_SCAN:
            change_actor_stat(actor, STAT_QUICK_SCAN);
            break;
        case ST_QUICK_EXIT:
            change_actor_stat(actor, STAT_QUICK_EXIT);
            break;
        case ST_QUICK_OPERATE:
            change_actor_stat(actor, STAT_QUICK_OPERATE);
            break;
        case ST_QUICK_DUCK:
            change_actor_stat(actor, STAT_QUICK_DUCK);
            break;
        case ST_QUICK_DEFEND:
            change_actor_stat(actor, STAT_QUICK_DEFEND);
            break;

        case ST1:
        {
            sectortype* sectp = actor->sector();
            short tag;
            short bit;

            // get rid of defaults
            if (SP_TAG3(actor) == 32)
                SP_TAG3(actor) = 0;

            tag = actor->spr.hitag;

            actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
            actor->spr.cstat |= (CSTAT_SPRITE_INVISIBLE);

            // for bounding sector objects
            if ((tag >= 500 && tag < 600) || tag == SECT_SO_CENTER)
            {
                // NOTE: These will get deleted by the sector object
                // setup code
                change_actor_stat(actor, STAT_ST1);
                break;
            }

            if (tag < 16)
            {
                bit = 1 << (tag);

                actor->sector()->extra |= (bit);

                if (bit & (SECTFX_SINK))
                {
                    sectp->u_defined = true;
                    sectp->depth_fixed = IntToFixed(actor->spr.lotag);
                    KillActor(actor);
                }
                else if (bit & (SECTFX_OPERATIONAL))
                {
                    KillActor(actor);
                }
                else if (bit & (SECTFX_CURRENT))
                {
                    sectp->u_defined = true;
                    sectp->speed = actor->spr.lotag;
                    sectp->ang = actor->spr.ang;
                    KillActor(actor);
                }
                else if (bit & (SECTFX_NO_RIDE))
                {
                    change_actor_stat(actor, STAT_NO_RIDE);
                }
                else if (bit & (SECTFX_DIVE_AREA))
                {
                    sectp->u_defined = true;
                    sectp->number = actor->spr.lotag;
                    change_actor_stat(actor, STAT_DIVE_AREA);
                }
                else if (bit & (SECTFX_UNDERWATER))
                {
                    sectp->u_defined = true;
                    sectp->number = actor->spr.lotag;
                    change_actor_stat(actor, STAT_UNDERWATER);
                }
                else if (bit & (SECTFX_UNDERWATER2))
                {
                    sectp->u_defined = true;
                    sectp->number = actor->spr.lotag;
                    if (actor->spr.clipdist == 1)
                        sectp->flags |= (SECTFU_CANT_SURFACE);
                    change_actor_stat(actor, STAT_UNDERWATER2);
                }
            }
            else
            {
                switch (tag)
                {
#if 0
                case MULTI_PLAYER_START:
                    change_actor_stat(actor, STAT_MULTI_START + actor->spr.lotag);
                    break;
                case MULTI_COOPERATIVE_START:
                    change_actor_stat(actor, STAT_CO_OP_START + actor->spr.lotag);
                    break;
#endif

                case SECT_MATCH:
                    sectp->u_defined = true;
                    sectp->number = actor->spr.lotag;

                    KillActor(actor);
                    break;

                case SLIDE_SECTOR:
                    sectp->u_defined = true;
                    sectp->flags |= (SECTFU_SLIDE_SECTOR);
                    sectp->speed = SP_TAG2(actor);
                    KillActor(actor);
                    break;

                case SECT_DAMAGE:
                {
                    sectp->u_defined = true;
                    if (TEST_BOOL1(actor))
                        sectp->flags |= (SECTFU_DAMAGE_ABOVE_SECTOR);
                    sectp->damage = actor->spr.lotag;
                    KillActor(actor);
                    break;
                }

                case PARALLAX_LEVEL:
                {
                    parallaxyscale_override = 8192;
                    pskybits_override = actor->spr.lotag;
                    if (SP_TAG4(actor) > 2048)
                        parallaxyscale_override = SP_TAG4(actor);
                    defineSky(DEFAULTPSKY, pskybits_override, nullptr, 0, parallaxyscale_override / 8192.f);
                    KillActor(actor);
                    break;
                }

                case BREAKABLE:
                    // used for wall info
                    change_actor_stat(actor, STAT_BREAKABLE);
                    break;

                case SECT_DONT_COPY_PALETTE:
                {
                    sectp->u_defined = true;
                    sectp->flags |= (SECTFU_DONT_COPY_PALETTE);
                    KillActor(actor);
                    break;
                }

                case SECT_FLOOR_PAN:
                {
                    // if moves with SO
                    if (TEST_BOOL1(actor))
                        actor->spr.xvel = 0;
                    else
                        actor->spr.xvel = actor->spr.lotag;

                    StartInterpolation(actor->sector(), Interp_Sect_FloorPanX);
                    StartInterpolation(actor->sector(), Interp_Sect_FloorPanY);
                    change_actor_stat(actor, STAT_FLOOR_PAN);
                    break;
                }

                case SECT_CEILING_PAN:
                {
                    // if moves with SO
                    if (TEST_BOOL1(actor))
                        actor->spr.xvel = 0;
                    else
                        actor->spr.xvel = actor->spr.lotag;
                    StartInterpolation(actor->sector(), Interp_Sect_CeilingPanX);
                    StartInterpolation(actor->sector(), Interp_Sect_CeilingPanY);
                    change_actor_stat(actor, STAT_CEILING_PAN);
                    break;
                }

                case SECT_WALL_PAN_SPEED:
                {
                    vec3_t hit_pos = { actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z - Z(8) };
                    HitInfo hit{};

                    hitscan(hit_pos, actor->sector(),    // Start position
                        { bcos(actor->spr.ang), bsin(actor->spr.ang), 0 }, hit, CLIPMASK_MISSILE);

                    if (hit.hitWall == nullptr)
                    {
                        KillActor(actor);
                        break;
                    }

                    actor->tempwall = hit.hitWall;
                    // if moves with SO
                    if (TEST_BOOL1(actor))
                        actor->spr.xvel = 0;
                    else
                        actor->spr.xvel = actor->spr.lotag;
                    actor->spr.ang = SP_TAG6(actor);
                    // attach to the sector that contains the wall
                    ChangeActorSect(actor, hit.hitSector);
                    StartInterpolation(hit.hitWall, Interp_Wall_PanX);
                    StartInterpolation(hit.hitWall, Interp_Wall_PanY);
                    change_actor_stat(actor, STAT_WALL_PAN);
                    break;
                }

                case WALL_DONT_STICK:
                {
                    vec3_t hit_pos = { actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z - Z(8) };
                    HitInfo hit{};

                    hitscan(hit_pos, actor->sector(),    // Start position
                        { bcos(actor->spr.ang), bsin(actor->spr.ang), 0 }, hit, CLIPMASK_MISSILE);

                    if (hit.hitWall == nullptr)
                    {
                        KillActor(actor);
                        break;
                    }

                    hit.hitWall->extra |= WALLFX_DONT_STICK;
                    KillActor(actor);
                    break;
                }

                case TRIGGER_SECTOR:
                {
                    actor->sector()->extra |= (SECTFX_TRIGGER);
                    change_actor_stat(actor, STAT_TRIGGER);
                    break;
                }

                case DELETE_SPRITE:
                {
                    change_actor_stat(actor, STAT_DELETE_SPRITE);
                    break;
                }

                case SPAWN_ITEMS:
                {
                    if ((actor->spr.extra & SPRX_MULTI_ITEM))
                    {
                        if (numplayers <= 1 || gNet.MultiGameType == MULTI_GAME_COOPERATIVE)
                        {
                            KillActor(actor);
                            break;
                        }
                    }


                    change_actor_stat(actor, STAT_SPAWN_ITEMS);
                    break;
                }

                case CEILING_FLOOR_PIC_OVERRIDE:
                {
                    // block hitscans depending on translucency
                    if (SP_TAG7(actor) == 0 || SP_TAG7(actor) == 1)
                    {
                        if (SP_TAG3(actor) == 0)
                            actor->sector()->ceilingstat |= (CSTAT_SECTOR_FAF_BLOCK_HITSCAN);
                        else
                            actor->sector()->floorstat |= (CSTAT_SECTOR_FAF_BLOCK_HITSCAN);
                    }
                    else if (TEST_BOOL1(actor))
                    {
                        if (SP_TAG3(actor) == 0)
                            actor->sector()->ceilingstat |= (CSTAT_SECTOR_FAF_BLOCK_HITSCAN);
                        else
                            actor->sector()->floorstat |= (CSTAT_SECTOR_FAF_BLOCK_HITSCAN);
                    }

                    // copy tag 7 to tag 6 and pre-shift it
                    SP_TAG6(actor) = SP_TAG7(actor);
                    SP_TAG6(actor) <<= 7;
                    change_actor_stat(actor, STAT_CEILING_FLOOR_PIC_OVERRIDE);
                    break;
                }

                case QUAKE_SPOT:
                {
                    change_actor_stat(actor, STAT_QUAKE_SPOT);
                    SET_SP_TAG13(actor, ((SP_TAG6(actor)*10L) * 120L));
                    break;
                }

                case SECT_CHANGOR:
                {
                    change_actor_stat(actor, STAT_CHANGOR);
                    break;
                }

#if 0
                case SECT_DEBRIS_SEWER:
                {
                    ANIMATOR DoGenerateSewerDebris;

                    SpawnUser(actor, 0, nullptr);

                    ASSERT(actor->hasU());
                    actor->user.RotNum = 0;
                    actor->user.WaitTics = actor->spr.lotag * 120;

                    actor->user.ActorActionFunc = DoGenerateSewerDebris;

                    change_actor_stat(actor, STAT_NO_STATE);
                    break;
                }
#endif

                case SECT_VATOR:
                {
                    short speed,vel,time,type,start_on,floor_vator;
                    SpawnUser(actor, 0, nullptr);

                    // vator already set - ceiling AND floor vator
                    if ((sectp->extra & SECTFX_VATOR))
                    {
                        sectp->u_defined = true;
                        sectp->flags |= (SECTFU_VATOR_BOTH);
                    }
                    sectp->extra |= (SECTFX_VATOR);
                    SetSectorWallBits(actor->sector(), WALLFX_DONT_STICK, true, true);
                    actor->sector()->extra |= (SECTFX_DYNAMIC_AREA);

                    // don't step on toes of other sector settings
                    if (sectp->lotag == 0 && sectp->hitag == 0)
                        sectp->lotag = TAG_VATOR;

                    type = SP_TAG3(actor);
                    speed = SP_TAG4(actor);
                    vel = SP_TAG5(actor);
                    time = SP_TAG9(actor);
                    start_on = !!TEST_BOOL1(actor);
                    floor_vator = true;
                    if (actor->spr.cstat & (CSTAT_SPRITE_YFLIP))
                        floor_vator = false;

                    actor->user.jump_speed = actor->user.vel_tgt = speed;
                    actor->user.vel_rate = vel;
                    actor->user.WaitTics = time*15; // 1/8 of a sec
                    actor->user.Tics = 0;

                    actor->user.Flags |= (SPR_ACTIVE);

                    switch (type)
                    {
                    case 0:
                        actor->user.Flags &= ~(SPR_ACTIVE);
                        actor->user.ActorActionFunc = DoVator;
                        break;
                    case 1:
                        actor->user.Flags &= ~(SPR_ACTIVE);
                        actor->user.ActorActionFunc = DoVator;
                        break;
                    case 2:
                        actor->user.ActorActionFunc = DoVatorAuto;
                        break;
                    case 3:
                        actor->user.Flags &= ~(SPR_ACTIVE);
                        actor->user.ActorActionFunc = DoVatorAuto;
                        break;
                    }

                    if (floor_vator)
                    {
                        // start off
                        actor->user.pos.Z = sectp->int_floorz();
                        actor->user.z_tgt = actor->int_pos().Z;
                        if (start_on)
                        {
                            int amt;
                            amt = actor->int_pos().Z - sectp->int_floorz();

                            // start in the on position
                            sectp->add_int_floorz(amt);
                            actor->user.z_tgt = actor->user.pos.Z;

                            MoveSpritesWithSector(actor->sector(), amt, false); // floor
                        }

                        // set orig z
                        actor->user.oz = actor->opos.Z = sectp->int_floorz();
                    }
                    else
                    {
                        // start off
                        actor->user.pos.Z = sectp->int_ceilingz();
                        actor->user.z_tgt = actor->int_pos().Z;
                        if (start_on)
                        {
                            int amt;
                            amt = actor->int_pos().Z - sectp->int_ceilingz();

                            // starting in the on position
                            sectp->add_int_ceilingz(amt);
                            actor->user.z_tgt = actor->user.pos.Z;

                            MoveSpritesWithSector(actor->sector(), amt, true); // ceiling
                        }

                        // set orig z
                        actor->user.oz = actor->opos.Z = sectp->int_ceilingz();
                    }


                    change_actor_stat(actor, STAT_VATOR);
                    break;
                }

                case SECT_ROTATOR_PIVOT:
                {
                    change_actor_stat(actor, STAT_ROTATOR_PIVOT);
                    break;
                }

                case SECT_ROTATOR:
                {
                    short time,type;
                    short wallcount,startwall,endwall,w;
                    SpawnUser(actor, 0, nullptr);

                    SetSectorWallBits(actor->sector(), WALLFX_DONT_STICK, true, true);

                    // need something for this
                    sectp->lotag = TAG_ROTATOR;
                    sectp->hitag = actor->spr.lotag;

                    type = SP_TAG3(actor);
                    time = SP_TAG9(actor);

                    actor->user.WaitTics = time*15; // 1/8 of a sec
                    actor->user.Tics = 0;

                    actor->user.rotator.Alloc();
                    actor->user.rotator->open_dest = SP_TAG5(actor);
                    actor->user.rotator->speed = SP_TAG7(actor);
                    actor->user.rotator->vel = SP_TAG8(actor);
                    actor->user.rotator->pos = 0; // closed
                    actor->user.rotator->tgt = actor->user.rotator->open_dest; // closed
                    actor->user.rotator->SetNumWalls(actor->sector()->wallnum);

                    actor->user.rotator->orig_speed = actor->user.rotator->speed;

                    wallcount = 0;
                    for(auto& wal : wallsofsector(actor->sector()))
                    {
                        actor->user.rotator->origX[wallcount] = wal.wall_int_pos().X;
                        actor->user.rotator->origY[wallcount] = wal.wall_int_pos().Y;
                        wallcount++;
                    }

                    actor->user.Flags |= (SPR_ACTIVE);

                    switch (type)
                    {
                    case 0:
                        actor->user.Flags &= ~(SPR_ACTIVE);
                        actor->user.ActorActionFunc = DoRotator;
                        break;
                    case 1:
                        actor->user.Flags &= ~(SPR_ACTIVE);
                        actor->user.ActorActionFunc = DoRotator;
                        break;
                    }

                    change_actor_stat(actor, STAT_ROTATOR);
                    break;
                }

                case SECT_SLIDOR:
                {
                    short time,type;

                    SpawnUser(actor, 0, nullptr);

                    SetSectorWallBits(actor->sector(), WALLFX_DONT_STICK, true, true);

                    // need something for this
                    sectp->lotag = TAG_SLIDOR;
                    sectp->hitag = actor->spr.lotag;

                    type = SP_TAG3(actor);
                    time = SP_TAG9(actor);

                    actor->user.WaitTics = time*15; // 1/8 of a sec
                    actor->user.Tics = 0;

                    actor->user.rotator.Alloc();
                    actor->user.rotator->open_dest = SP_TAG5(actor);
                    actor->user.rotator->speed = SP_TAG7(actor);
                    actor->user.rotator->vel = SP_TAG8(actor);
                    actor->user.rotator->pos = 0; // closed
                    actor->user.rotator->tgt = actor->user.rotator->open_dest; // closed
                    actor->user.rotator->ClearWalls();
                    actor->user.rotator->orig_speed = actor->user.rotator->speed;

                    actor->user.Flags |= (SPR_ACTIVE);

                    switch (type)
                    {
                    case 0:
                        actor->user.Flags &= ~(SPR_ACTIVE);
                        actor->user.ActorActionFunc = DoSlidor;
                        break;
                    case 1:
                        actor->user.Flags &= ~(SPR_ACTIVE);
                        actor->user.ActorActionFunc = DoSlidor;
                        break;
                    }


                    if (TEST_BOOL5(actor))
                    {
                        DoSlidorInstantClose(actor);
                    }

                    change_actor_stat(actor, STAT_SLIDOR);
                    break;
                }

                case SECT_SPIKE:
                {
                    short speed,vel,time,type,start_on,floor_vator;
                    int florz,ceilz;
                    Collision trash;
                    SpawnUser(actor, 0, nullptr);

                    SetSectorWallBits(actor->sector(), WALLFX_DONT_STICK, false, true);
                    actor->sector()->extra |= (SECTFX_DYNAMIC_AREA);

                    type = SP_TAG3(actor);
                    speed = SP_TAG4(actor);
                    vel = SP_TAG5(actor);
                    time = SP_TAG9(actor);
                    start_on = !!TEST_BOOL1(actor);
                    floor_vator = true;
                    if (actor->spr.cstat & (CSTAT_SPRITE_YFLIP))
                        floor_vator = false;

                    actor->user.jump_speed = actor->user.vel_tgt = speed;
                    actor->user.vel_rate = vel;
                    actor->user.WaitTics = time*15; // 1/8 of a sec
                    actor->user.Tics = 0;

                    actor->user.Flags |= (SPR_ACTIVE);

                    switch (type)
                    {
                    case 0:
                        actor->user.Flags &= ~(SPR_ACTIVE);
                        actor->user.ActorActionFunc = DoSpike;
                        break;
                    case 1:
                        actor->user.Flags &= ~(SPR_ACTIVE);
                        actor->user.ActorActionFunc = DoSpike;
                        break;
                    case 2:
                        actor->user.ActorActionFunc = DoSpikeAuto;
                        break;
                    case 3:
                        actor->user.Flags &= ~(SPR_ACTIVE);
                        actor->user.ActorActionFunc = DoSpikeAuto;
                        break;
                    }

                    getzrangepoint(actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->sector(), &ceilz, &trash, &florz, &trash);

                    if (floor_vator)
                    {
                        actor->user.zclip = florz;

                        // start off
                        actor->user.pos.Z = actor->user.zclip;
                        actor->user.z_tgt = actor->int_pos().Z;
                        if (start_on)
                        {
                            // start in the on position
                            actor->user.zclip = actor->int_pos().Z;
                            actor->user.z_tgt = actor->user.pos.Z;
                            SpikeAlign(actor);
                        }

                        // set orig z
                        actor->user.oz = actor->opos.Z = actor->user.zclip;
                    }
                    else
                    {
                        actor->user.zclip = ceilz;

                        // start off
                        actor->user.pos.Z = actor->user.zclip;
                        actor->user.z_tgt = actor->int_pos().Z;
                        if (start_on)
                        {
                            // starting in the on position
                            actor->user.zclip = actor->int_pos().Z;
                            actor->user.z_tgt = actor->user.pos.Z;
                            SpikeAlign(actor);
                        }

                        // set orig z
                        actor->user.oz = actor->opos.Z = actor->user.zclip;
                    }

                    change_actor_stat(actor, STAT_SPIKE);
                    break;
                }

                case LIGHTING:
                {
                    int wallcount = 0;
                    int8_t* wall_shade;

                    LIGHT_Tics(actor) = 0;

                    if (LIGHT_ShadeInc(actor) == 0)
                        LIGHT_ShadeInc(actor) = 1;

                    // save off original floor and ceil shades
                    LIGHT_FloorShade(actor) = actor->sector()->floorshade;
                    LIGHT_CeilingShade(actor) = actor->sector()->ceilingshade;

                    // count walls of sector
                    for(auto& wal : wallsofsector(actor->sector()))
                    {
                        wallcount++;
                        if (TEST_BOOL5(actor))
                        {
                            if (wal.twoSided())
                                wallcount++;
                        }
                    }

                    SpawnUser(actor, 0, nullptr);
                    actor->user.WallShade.Resize(wallcount);
                    wallcount = 0;
                    wall_shade = actor->user.WallShade.Data();

                    // save off original wall shades
                    for(auto& wal : wallsofsector(actor->sector()))
                    {
                        wall_shade[wallcount] =  wal.shade;
                        wallcount++;
                        if (TEST_BOOL5(actor))
                        {
                            if (wal.twoSided())
                            {
                                wall_shade[wallcount] = wal.nextWall()->shade;
                                wallcount++;
                            }
                        }
                    }

                    actor->user.spal = actor->spr.pal;

                    // DON'T USE COVER function
                    change_actor_stat(actor, STAT_LIGHTING, true);
                    break;
                }

                case LIGHTING_DIFFUSE:
                {
                    int wallcount = 0;
                    int8_t* wall_shade;

                    LIGHT_Tics(actor) = 0;

                    // save off original floor and ceil shades
                    LIGHT_FloorShade(actor) = actor->sector()->floorshade;
                    LIGHT_CeilingShade(actor) = actor->sector()->ceilingshade;

                    // count walls of sector
                    for (auto& wal : wallsofsector(actor->sector()))
                    {
                        wallcount++;
                        if (TEST_BOOL5(actor))
                        {
                            if (wal.twoSided())
                                wallcount++;
                        }
                    }

                    // !LIGHT
                    // make an wall_shade array and put it in User
                    SpawnUser(actor, 0, nullptr);
                    actor->user.WallShade.Resize(wallcount);
                    wallcount = 0;
                    wall_shade = actor->user.WallShade.Data();

                    // save off original wall shades
                    for (auto& wal : wallsofsector(actor->sector()))
                    {
                        wall_shade[wallcount] = wal.shade;
                        wallcount++;
                        if (TEST_BOOL5(actor))
                        {
                            if (wal.twoSided())
                            {
                                wall_shade[wallcount] = wal.nextWall()->shade;
                                wallcount++;
                            }
                        }
                    }

                    // DON'T USE COVER function
                    change_actor_stat(actor, STAT_LIGHTING_DIFFUSE, true);
                    break;
                }

                case SECT_VATOR_DEST:
                    change_actor_stat(actor, STAT_VATOR);
                    break;

                case SO_WALL_DONT_MOVE_UPPER:
                    change_actor_stat(actor, STAT_WALL_DONT_MOVE_UPPER);
                    break;

                case SO_WALL_DONT_MOVE_LOWER:
                    change_actor_stat(actor, STAT_WALL_DONT_MOVE_LOWER);
                    break;

                case FLOOR_SLOPE_DONT_DRAW:
                    change_actor_stat(actor, STAT_FLOOR_SLOPE_DONT_DRAW);
                    break;

                case DEMO_CAMERA:
                    actor->spr.yvel = actor->spr.zvel = 100; //attempt horiz control
                    change_actor_stat(actor, STAT_DEMO_CAMERA);
                    break;

                case LAVA_ERUPT:
                {

                    SpawnUser(actor, ST1, nullptr);

                    change_actor_stat(actor, STAT_NO_STATE);
                    actor->user.ActorActionFunc = DoLavaErupt;

                    // interval between erupts
                    if (SP_TAG10(actor) == 0)
                        SP_TAG10(actor) = 20;

                    // interval in seconds
                    actor->user.WaitTics = RandomRange(SP_TAG10(actor)) * 120;

                    // time to erupt
                    if (SP_TAG9(actor) == 0)
                        SP_TAG9(actor) = 10;

                    actor->add_int_z(Z(30));

                    break;
                }


                case SECT_EXPLODING_CEIL_FLOOR:
                {
                    SetSectorWallBits(actor->sector(), WALLFX_DONT_STICK, false, true);

                    if ((sectp->floorstat & CSTAT_SECTOR_SLOPE))
                    {
                        SP_TAG5(actor) = sectp->floorheinum;
                        sectp->setfloorslope(0);
                    }

                    if ((sectp->ceilingstat & CSTAT_SECTOR_SLOPE))
                    {
                        SP_TAG6(actor) = sectp->ceilingheinum;
                        sectp->setceilingslope(0);
                    }

                    SP_TAG4(actor) = abs(sectp->int_ceilingz() - sectp->int_floorz())>>8;

                    sectp->set_int_ceilingz(sectp->int_floorz());

                    change_actor_stat(actor, STAT_EXPLODING_CEIL_FLOOR);
                    break;
                }

                case SECT_COPY_SOURCE:
                    change_actor_stat(actor, STAT_COPY_SOURCE);
                    break;

                case SECT_COPY_DEST:
                {
                    SetSectorWallBits(actor->sector(), WALLFX_DONT_STICK, false, true);
                    change_actor_stat(actor, STAT_COPY_DEST);
                    break;
                }


                case SECT_WALL_MOVE:
                    change_actor_stat(actor, STAT_WALL_MOVE);
                    break;
                case SECT_WALL_MOVE_CANSEE:
                    change_actor_stat(actor, STAT_WALL_MOVE_CANSEE);
                    break;

                case SPRI_CLIMB_MARKER:
                {
                    // setup climb marker
                    change_actor_stat(actor, STAT_CLIMB_MARKER);

                    // make a QUICK_LADDER sprite automatically
                    auto actorNew = insertActor(actor->sector(), STAT_QUICK_LADDER);

                    actorNew->spr.cstat = 0;
                    actorNew->spr.extra = 0;
                    actorNew->set_int_pos(actor->int_pos());
                    actorNew->spr.ang = NORM_ANGLE(actor->spr.ang + 1024);
                    actorNew->spr.picnum = actor->spr.picnum;

                    actorNew->add_int_pos({ MOVEx(256 + 128, actor->spr.ang), MOVEy(256 + 128, actor->spr.ang), 0 });

                    break;
                }

                case SO_AUTO_TURRET:
#if 0
                    switch (gNet.MultiGameType)
                    {
                    case MULTI_GAME_NONE:
                        change_actor_stat(actor, STAT_ST1);
                        break;
                    case MULTI_GAME_COMMBAT:
                        KillActor(actor);
                        break;
                    case MULTI_GAME_COOPERATIVE:
                        change_actor_stat(actor, STAT_ST1);
                        break;
                    }
#else
                    change_actor_stat(actor, STAT_ST1);
#endif
                    break;

                case SO_DRIVABLE_ATTRIB:
                case SO_SCALE_XY_MULT:
                case SO_SCALE_INFO:
                case SO_SCALE_POINT_INFO:
                case SO_TORNADO:
                case SO_FLOOR_MORPH:
                case SO_AMOEBA:
                case SO_SET_SPEED:
                case SO_ANGLE:
                case SO_SPIN:
                case SO_SPIN_REVERSE:
                case SO_BOB_START:
                case SO_BOB_SPEED:
                case SO_TURN_SPEED:
                case SO_SYNC1:
                case SO_SYNC2:
                case SO_LIMIT_TURN:
                case SO_MATCH_EVENT:
                case SO_MAX_DAMAGE:
                case SO_RAM_DAMAGE:
                case SO_SLIDE:
                case SO_KILLABLE:
                case SECT_SO_SPRITE_OBJ:
                case SECT_SO_DONT_ROTATE:
                case SECT_SO_CLIP_DIST:
                {
                    // NOTE: These will get deleted by the sector
                    // object
                    // setup code

                    change_actor_stat(actor, STAT_ST1);
                    break;
                }

                case SOUND_SPOT:
                    SET_SP_TAG13(actor, SP_TAG4(actor));
                    change_actor_stat(actor, STAT_SOUND_SPOT);
                    break;

                case STOP_SOUND_SPOT:
                    change_actor_stat(actor, STAT_STOP_SOUND_SPOT);
                    break;

                case SPAWN_SPOT:
                    if (!actor->hasU())
                        SpawnUser(actor, ST1, nullptr);

                    if (actor->spr.xrepeat == 64 && actor->spr.yrepeat == 64) // clear default xrepeat.
                        actor->spr.xrepeat = actor->spr.yrepeat = 0;

                    change_actor_stat(actor, STAT_SPAWN_SPOT);
                    break;

                case VIEW_THRU_CEILING:
                case VIEW_THRU_FLOOR:
                {
                    // make sure there is only one set per level of these
                    SWStatIterator it2(STAT_FAF);
                    while (auto itActor = it2.Next())
                    {
                        if (itActor->spr.hitag == actor->spr.hitag && itActor->spr.lotag == actor->spr.lotag)
                        {
                            I_Error("Two VIEW_THRU_ tags with same match found on level\n1: x %d, y %d \n2: x %d, y %d", actor->int_pos().X, actor->int_pos().Y, itActor->int_pos().X, itActor->int_pos().Y);
                        }
                    }
                    change_actor_stat(actor, STAT_FAF);
                    break;
                }

                case VIEW_LEVEL1:
                case VIEW_LEVEL2:
                case VIEW_LEVEL3:
                case VIEW_LEVEL4:
                case VIEW_LEVEL5:
                case VIEW_LEVEL6:
                {
                    change_actor_stat(actor, STAT_FAF);
                    break;
                }

                case PLAX_GLOB_Z_ADJUST:
                {
                    actor->sector()->extra |= (SECTFX_Z_ADJUST);
                    PlaxCeilGlobZadjust = SP_TAG2(actor);
                    PlaxFloorGlobZadjust = SP_TAG3(actor);
                    KillActor(actor);
                    break;
                }

                case CEILING_Z_ADJUST:
                {
                    actor->sector()->extra |= (SECTFX_Z_ADJUST);
                    change_actor_stat(actor, STAT_ST1);
                    break;
                }

                case FLOOR_Z_ADJUST:
                {
                    actor->sector()->extra |= (SECTFX_Z_ADJUST);
                    change_actor_stat(actor, STAT_ST1);
                    break;
                }

                case WARP_TELEPORTER:
                {
                    actor->spr.cstat |= (CSTAT_SPRITE_INVISIBLE);
                    actor->sector()->extra |= (SECTFX_WARP_SECTOR);
                    change_actor_stat(actor, STAT_WARP);

                    // if just a destination teleporter
                    // don't set up flags
                    if (SP_TAG10(actor) == 1)
                        break;

                    // move the the next wall
                    auto start_wall = actor->sector()->firstWall();
                    auto wall_num = start_wall;

                    // Travel all the way around loop setting wall bits
                    do
                    {
                        // DO NOT TAG WHITE WALLS!
                        if (wall_num->twoSided())
                        {
                            wall_num->cstat |= (CSTAT_WALL_WARP_HITSCAN);
                        }

                        wall_num = wall_num->point2Wall();
                    }
                    while (wall_num != start_wall);

                    break;
                }

                case WARP_CEILING_PLANE:
                case WARP_FLOOR_PLANE:
                {
                    actor->spr.cstat |= (CSTAT_SPRITE_INVISIBLE);
                    actor->sector()->extra |= (SECTFX_WARP_SECTOR);
                    change_actor_stat(actor, STAT_WARP);
                    break;
                }

                case WARP_COPY_SPRITE1:
                    actor->spr.cstat |= (CSTAT_SPRITE_INVISIBLE);
                    actor->sector()->extra |= (SECTFX_WARP_SECTOR);
                    change_actor_stat(actor, STAT_WARP_COPY_SPRITE1);
                    break;
                case WARP_COPY_SPRITE2:
                    actor->spr.cstat |= (CSTAT_SPRITE_INVISIBLE);
                    actor->sector()->extra |= (SECTFX_WARP_SECTOR);
                    change_actor_stat(actor, STAT_WARP_COPY_SPRITE2);
                    break;

                case FIREBALL_TRAP:
                case BOLT_TRAP:
                case SPEAR_TRAP:
                {
                    SpawnUser(actor, 0, nullptr);
                    ClearOwner(actor);
                    change_actor_stat(actor, STAT_TRAP);
                    break;
                }

                case SECT_SO_DONT_BOB:
                {
                    sectp->u_defined = true;
                    sectp->flags |= (SECTFU_SO_DONT_BOB);
                    KillActor(actor);
                    break;
                }

                case SECT_LOCK_DOOR:
                {
                    sectp->u_defined = true;
                    sectp->number = actor->spr.lotag;
                    sectp->stag = SECT_LOCK_DOOR;
                    KillActor(actor);
                    break;
                }

                case SECT_SO_SINK_DEST:
                {
                    sectp->u_defined = true;
                    sectp->flags |= (SECTFU_SO_SINK_DEST);
                    sectp->number = actor->spr.lotag;  // acually the offset Z
                    // value
                    KillActor(actor);
                    break;
                }

                case SECT_SO_DONT_SINK:
                {
                    sectp->u_defined = true;
                    sectp->flags |= (SECTFU_SO_DONT_SINK);
                    KillActor(actor);
                    break;
                }

                case SO_SLOPE_FLOOR_TO_POINT:
                {
                    sectp->u_defined = true;
                    sectp->flags |= (SECTFU_SO_SLOPE_FLOOR_TO_POINT);
                    sectp->extra |= (SECTFX_DYNAMIC_AREA);
                    KillActor(actor);
                    break;
                }

                case SO_SLOPE_CEILING_TO_POINT:
                {
                    sectp->u_defined = true;
                    sectp->flags |= (SECTFU_SO_SLOPE_CEILING_TO_POINT);
                    sectp->extra |= (SECTFX_DYNAMIC_AREA);
                    KillActor(actor);
                    break;
                }
                case SECT_SO_FORM_WHIRLPOOL:
                {
                    sectp->u_defined = true;
                    sectp->stag = SECT_SO_FORM_WHIRLPOOL;
                    sectp->height = actor->spr.lotag;
                    KillActor(actor);
                    break;
                }

                case SECT_ACTOR_BLOCK:
                {
                    // move the the next wall
                    auto start_wall = actor->sector()->firstWall();
                    auto wall_num = start_wall;

                    // Travel all the way around loop setting wall bits
                    do
                    {
                        wall_num->cstat |= (CSTAT_WALL_BLOCK_ACTOR);
                        if (wall_num->twoSided())
                            wall_num->nextWall()->cstat |= CSTAT_WALL_BLOCK_ACTOR;
                        wall_num = wall_num->point2Wall();
                    }
                    while (wall_num != start_wall);

                    KillActor(actor);
                    break;
                }
                }
            }
        }
        break;

        case RED_CARD:
            num = 4;
            goto KeyMain;
        case RED_KEY:
            num = 0;
            goto KeyMain;
        case BLUE_CARD:
            num = 5;
            goto KeyMain;
        case BLUE_KEY:
            num = 1;
            goto KeyMain;
        case GREEN_CARD:
            num = 6;
            goto KeyMain;
        case GREEN_KEY:
            num = 2;
            goto KeyMain;
        case YELLOW_CARD:
            num = 7;
            goto KeyMain;
        case YELLOW_KEY:
            num = 3;
            goto KeyMain;
        case GOLD_SKELKEY:
            num = 8;
            goto KeyMain;
        case SILVER_SKELKEY:
            num = 9;
            goto KeyMain;
        case BRONZE_SKELKEY:
            num = 10;
            goto KeyMain;
        case RED_SKELKEY:
            num = 11;
KeyMain:
            {

                if (gNet.MultiGameType == MULTI_GAME_COMMBAT || gNet.MultiGameType == MULTI_GAME_AI_BOTS)
                {
                    KillActor(actor);
                    break;
                }

                SpawnUser(actor, 0, nullptr);

                actor->spr.picnum = actor->user.ID = actor->spr.picnum;

                actor->user.spal = actor->spr.pal; // Set the palette from build

                //actor->spr.cstat |= (CSTAT_SPRITE_ALIGNMENT_WALL);

                ChangeState(actor, s_Key[num]);

                picanm[actor->spr.picnum].sf &= ~(PICANM_ANIMTYPE_MASK);
                picanm[actor->spr.picnum + 1].sf &= ~(PICANM_ANIMTYPE_MASK);
                change_actor_stat(actor, STAT_ITEM);
                actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN | CSTAT_SPRITE_ONE_SIDE);
                actor->user.Radius = 500;
                actor->spr.hitag = LUMINOUS; //Set so keys over ride colored lighting

                DoActorZrange(actor);
            }

            break;


        // Used for multiplayer locks
        case 1846:
        case 1850:
        case 1852:
        case 2470:

            if ((actor->spr.extra & SPRX_MULTI_ITEM))
                if (numplayers <= 1 || gNet.MultiGameType == MULTI_GAME_COOPERATIVE)
                {
                    KillActor(actor);
                }

            break;

        case FIRE_FLY0:

            /*
             * SpawnUser(actor, FIRE_FLY0, nullptr);
             *
             * actor->user.State = actor->user.StateStart = &s_FireFly[0]; actor->user.RotNum = 0;
             *
             * actor->spr.ang = 0; actor->spr.xvel = 4;
             *
             * if (labs(actor->spr.z - actor->sector()->int_floorz()) < Z(32)) actor->spr.z =
             * actor->sector()->int_floorz() - Z(32);
             *
             * actor->user.sz = actor->spr.z;
             *
             * change_actor_stat(actor, STAT_MISC);
             */

            break;

        case ICON_REPAIR_KIT:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_REPAIR_KIT, s_RepairKit);

            IconDefault(actor);
            break;

        case ICON_STAR:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_STAR, s_IconStar);

            IconDefault(actor);
            break;

        case ICON_LG_MINE:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_LG_MINE, s_IconLgMine);
            IconDefault(actor);
            break;


        case ICON_MICRO_GUN:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_MICRO_GUN, s_IconMicroGun);

            IconDefault(actor);
            break;

        case ICON_MICRO_BATTERY:

NUKE_REPLACEMENT:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_MICRO_BATTERY, s_IconMicroBattery);

            IconDefault(actor);
            break;

        case ICON_UZI:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_UZI, s_IconUzi);
            IconDefault(actor);
            break;

        case ICON_UZIFLOOR:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_UZIFLOOR, s_IconUziFloor);
            IconDefault(actor);
            break;

        case ICON_LG_UZI_AMMO:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_LG_UZI_AMMO, s_IconLgUziAmmo);
            IconDefault(actor);
            break;

        case ICON_GRENADE_LAUNCHER:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_GRENADE_LAUNCHER, s_IconGrenadeLauncher);

            IconDefault(actor);
            break;

        case ICON_LG_GRENADE:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_LG_GRENADE, s_IconLgGrenade);
            IconDefault(actor);
            break;

        case ICON_RAIL_GUN:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_RAIL_GUN, s_IconRailGun);
            IconDefault(actor);
            break;

        case ICON_RAIL_AMMO:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_RAIL_AMMO, s_IconRailAmmo);
            IconDefault(actor);
            break;


        case ICON_ROCKET:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_ROCKET, s_IconRocket);
            IconDefault(actor);
            break;

        case ICON_LG_ROCKET:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_LG_ROCKET, s_IconLgRocket);
            IconDefault(actor);
            break;

        case ICON_SHOTGUN:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_SHOTGUN, s_IconShotgun);

            actor->user.Radius = 350; // Shotgun is hard to pick up for some reason.

            IconDefault(actor);
            break;

        case ICON_LG_SHOTSHELL:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_LG_SHOTSHELL, s_IconLgShotshell);
            IconDefault(actor);
            break;

        case ICON_AUTORIOT:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_AUTORIOT, s_IconAutoRiot);
            IconDefault(actor);
            break;


        case ICON_GUARD_HEAD:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_GUARD_HEAD, s_IconGuardHead);
            IconDefault(actor);
            break;

        case ICON_FIREBALL_LG_AMMO:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_FIREBALL_LG_AMMO, s_IconFireballLgAmmo);
            IconDefault(actor);
            break;

        case ICON_HEART:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_HEART, s_IconHeart);
            IconDefault(actor);
            break;

        case ICON_HEART_LG_AMMO:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_HEART_LG_AMMO, s_IconHeartLgAmmo);
            IconDefault(actor);
            break;

#if 0
        case ICON_ELECTRO:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_ELECTRO, s_IconElectro);
            IconDefault(actor);
            break;
#endif

        case ICON_SPELL:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_SPELL, s_IconSpell);
            IconDefault(actor);

            PicAnimOff(actor->spr.picnum);
            break;

        case ICON_ARMOR:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_ARMOR, s_IconArmor);
            if (actor->spr.pal != PALETTE_PLAYER3)
                actor->spr.pal = actor->user.spal = PALETTE_PLAYER1;
            else
                actor->spr.pal = actor->user.spal = PALETTE_PLAYER3;
            IconDefault(actor);
            break;

        case ICON_MEDKIT:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_MEDKIT, s_IconMedkit);
            IconDefault(actor);
            break;

        case ICON_SM_MEDKIT:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_SM_MEDKIT, s_IconSmMedkit);
            IconDefault(actor);
            break;

        case ICON_CHEMBOMB:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_CHEMBOMB, s_IconChemBomb);
            IconDefault(actor);
            break;

        case ICON_FLASHBOMB:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_FLASHBOMB, s_IconFlashBomb);
            IconDefault(actor);
            break;

        case ICON_NUKE:

            if (gNet.MultiGameType)
            {
                if (!gNet.Nuke)
                {
                    goto NUKE_REPLACEMENT;
                    break;
                }
            }

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_NUKE, s_IconNuke);
            IconDefault(actor);
            break;


        case ICON_CALTROPS:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_CALTROPS, s_IconCaltrops);
            IconDefault(actor);
            break;

        case ICON_BOOSTER:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_BOOSTER, s_IconBooster);
            IconDefault(actor);
            break;

        case ICON_HEAT_CARD:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_HEAT_CARD, s_IconHeatCard);
            IconDefault(actor);
            break;

#if 0
        case ICON_ENVIRON_SUIT:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_ENVIRON_SUIT, s_IconEnvironSuit);
            IconDefault(actor);
            PicAnimOff(actor->spr.picnum);
            break;
#endif

        case ICON_CLOAK:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_CLOAK, s_IconCloak);
            IconDefault(actor);
            PicAnimOff(actor->spr.picnum);
            break;

        case ICON_FLY:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_FLY, s_IconFly);
            IconDefault(actor);
            PicAnimOff(actor->spr.picnum);
            break;

        case ICON_NIGHT_VISION:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_NIGHT_VISION, s_IconNightVision);
            IconDefault(actor);
            PicAnimOff(actor->spr.picnum);
            break;

        case ICON_FLAG:

            if (!IconSpawn(actor))
            {
                KillActor(actor);
                break;
            }

            SpawnUser(actor, ICON_FLAG, s_IconFlag);
            actor->user.spal = actor->spr.pal;
            actor->sector()->hitag = 9000;       // Put flag's color in sect containing it
            actor->sector()->lotag = actor->user.spal;
            IconDefault(actor);
            PicAnimOff(actor->spr.picnum);
            break;

#if 0
        case 380:
        case 396:
        case 430:
        case 512:
        case 521:
        case 541:
        case 2720:
#endif
        case 3143:
        case 3157:
        {
            SpawnUser(actor, actor->spr.picnum, nullptr);

            change_actor_stat(actor, STAT_STATIC_FIRE);

            actor->user.ID = FIREBALL_FLAMES;
            actor->user.Radius = 200;
            actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK);
            actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK_HITSCAN);

            actor->spr.hitag = LUMINOUS; //Always full brightness
            actor->spr.shade = -40;

            break;
        }

        // blades
        case BLADE1:
        case BLADE2:
        case BLADE3:
        case 5011:
        {
            SpawnUser(actor, actor->spr.picnum, nullptr);

            change_actor_stat(actor, STAT_DEFAULT);

            actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK);
            actor->spr.cstat |= (CSTAT_SPRITE_BLOCK_HITSCAN);
            actor->spr.extra |= (SPRX_BLADE);

            break;
        }

        case BREAK_LIGHT:
        case BREAK_BARREL:
        case BREAK_PEDISTAL:
        case BREAK_BOTTLE1:
        case BREAK_BOTTLE2:
        case BREAK_MUSHROOM:

            //if ((actor->spr.extra & SPRX_BREAKABLE))
            //    break;

            SpawnUser(actor, actor->spr.picnum, nullptr);

            actor->spr.clipdist = ActorSizeX(actor);
            actor->spr.cstat |= (CSTAT_SPRITE_BREAKABLE);
            actor->spr.extra |= (SPRX_BREAKABLE);
            break;

        // switches
        case 581:
        case 582:
        case 558:
        case 559:
        case 560:
        case 561:
        case 562:
        case 563:
        case 564:
        case 565:
        case 566:
        case 567:
        case 568:
        case 569:
        case 570:
        case 571:
        case 572:
        case 573:
        case 574:

        case 551:
        case 552:
        case 575:
        case 576:
        case 577:
        case 578:
        case 579:
        case 589:
        case 583:
        case 584:

        case 553:
        case 554:
        {
            if ((actor->spr.extra & SPRX_MULTI_ITEM))
            {
                if (numplayers <= 1 || gNet.MultiGameType == MULTI_GAME_COOPERATIVE)
                {
                    KillActor(actor);
                    break;
                }
            }


            actor->spr.cstat |= (CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
            break;
        }

        }
    }
}

bool ItemSpotClear(DSWActor* actor, short statnum, short id)
{
    bool found = false;
    int i;

    if (TEST_BOOL2(actor))
    {
        SWSectIterator it(actor->sector());
        while (auto itActor = it.Next())
        {
            if (itActor->spr.statnum == statnum && itActor->user.ID == id)
            {
                found = true;
                break;
            }
        }
    }

    return !found;
}

void SetupItemForJump(DSWActor* spawner, DSWActor* actor)
{
    // setup item for jumping
    if (SP_TAG7(spawner))
    {
        change_actor_stat(actor, STAT_SKIP4);
        actor->user.ceiling_dist = Z(6);
        actor->user.floor_dist = Z(0);
        actor->user.Counter = 0;

        actor->spr.xvel = (int)SP_TAG7(spawner)<<2;
        actor->spr.zvel = -(((int)SP_TAG8(spawner))<<5);

        actor->user.change.X = MOVEx(actor->spr.xvel, actor->spr.ang);
        actor->user.change.Y = MOVEy(actor->spr.xvel, actor->spr.ang);
        actor->user.change.Z = actor->spr.zvel;
    }
}

int ActorCoughItem(DSWActor* actor)
{
    short choose;
    DSWActor* actorNew = nullptr;

    switch (actor->user.ID)
    {
    case SAILORGIRL_R0:
        ASSERT(actor->insector());
        actorNew = insertActor(actor->sector(), STAT_SPAWN_ITEMS);
        actorNew->spr.cstat = 0;
        actorNew->spr.extra = 0;
        actorNew->set_int_pos({ actor->int_pos().X, actor->int_pos().Y, ActorZOfMiddle(actor) });
        actorNew->spr.ang = 0;
        actorNew->spr.extra = 0;

        // vel
        SP_TAG7(actorNew) = 1;
        // zvel
        SP_TAG8(actorNew) = 40;

        choose = RANDOM_P2(1024);

        if (choose > 854)
            SP_TAG3(actorNew) = 91; // Match number
        else if (choose > 684)
            SP_TAG3(actorNew) = 48; // Match number
        else if (choose > 514)
            SP_TAG3(actorNew) = 58; // Match number
        else if (choose > 344)
            SP_TAG3(actorNew) = 60; // Match number
        else if (choose > 174)
            SP_TAG3(actorNew) = 62; // Match number
        else
            SP_TAG3(actorNew) = 68; // Match number

        // match
        SP_TAG2(actorNew) = -1;
        // kill
        RESET_BOOL1(actorNew);
        SpawnItemsMatch(-1);
        break;

    case GORO_RUN_R0:
        if (RANDOM_P2(1024) < 700)
            return 0;

        ASSERT(actor->insector());
        actorNew = insertActor(actor->sector(), STAT_SPAWN_ITEMS);
        actorNew->spr.cstat = 0;
        actorNew->spr.extra = 0;
        actorNew->set_int_pos({ actor->int_pos().X, actor->int_pos().Y, ActorZOfMiddle(actor) });
        actorNew->spr.ang = 0;
        actorNew->spr.extra = 0;

        // vel
        SP_TAG7(actorNew) = 1;
        // zvel
        SP_TAG8(actorNew) = 40;

        SP_TAG3(actorNew) = 69; // Match number

        // match
        SP_TAG2(actorNew) = -1;
        // kill
        RESET_BOOL1(actorNew);
        SpawnItemsMatch(-1);
        break;

    case RIPPER2_RUN_R0:
        if (RANDOM_P2(1024) < 700)
            return 0;

        ASSERT(actor->insector());
        actorNew = insertActor(actor->sector(), STAT_SPAWN_ITEMS);
        actorNew->spr.cstat = 0;
        actorNew->spr.extra = 0;
        actorNew->set_int_pos({ actor->int_pos().X, actor->int_pos().Y, ActorZOfMiddle(actor) });
        actorNew->spr.ang = 0;
        actorNew->spr.extra = 0;

        // vel
        SP_TAG7(actorNew) = 1;
        // zvel
        SP_TAG8(actorNew) = 40;

        SP_TAG3(actorNew) = 70; // Match number

        // match
        SP_TAG2(actorNew) = -1;
        // kill
        RESET_BOOL1(actorNew);
        SpawnItemsMatch(-1);
        break;

    case NINJA_RUN_R0:

        if (actor->user.PlayerP)
        {
            if (RANDOM_P2(1024) > 200)
                return 0;

            ASSERT(actor->insector());
            actorNew = insertActor(actor->sector(), STAT_SPAWN_ITEMS);
            actorNew->spr.cstat = 0;
            actorNew->spr.extra = 0;
            actorNew->set_int_pos({ actor->int_pos().X, actor->int_pos().Y, ActorZOfMiddle(actor) });
            actorNew->spr.ang = 0;
            actorNew->spr.extra = 0;

            // vel
            SP_TAG7(actorNew) = 1;
            // zvel
            SP_TAG8(actorNew) = 40;

            switch (actor->user.WeaponNum)
            {
            case WPN_UZI:
                SP_TAG3(actorNew) = 0;
                break;
            case WPN_SHOTGUN:
                SP_TAG3(actorNew) = 51;
                break;
            case WPN_STAR:
                if (actor->user.PlayerP->WpnAmmo[WPN_STAR] < 9)
                    break;
                SP_TAG3(actorNew) = 41;
                break;
            case WPN_MINE:
                if (actor->user.PlayerP->WpnAmmo[WPN_MINE] < 5)
                    break;
                SP_TAG3(actorNew) = 42;
                break;
            case WPN_MICRO:
            case WPN_ROCKET:
                SP_TAG3(actorNew) = 43;
                break;
            case WPN_GRENADE:
                SP_TAG3(actorNew) = 45;
                break;
            case WPN_RAIL:
                SP_TAG3(actorNew) = 47;
                break;
            case WPN_HEART:
                SP_TAG3(actorNew) = 55;
                break;
            case WPN_HOTHEAD:
                SP_TAG3(actorNew) = 53;
                break;
            }

            // match
            SP_TAG2(actorNew) = -1;
            // kill
            RESET_BOOL1(actorNew);
            SpawnItemsMatch(-1);
            break;
        }

        if (RANDOM_P2(1024) < 512)
            return 0;

        ASSERT(actor->insector());
        actorNew = insertActor(actor->sector(), STAT_SPAWN_ITEMS);
        actorNew->spr.cstat = 0;
        actorNew->spr.extra = 0;
        actorNew->set_int_pos({ actor->int_pos().X, actor->int_pos().Y, ActorZOfMiddle(actor) });
        actorNew->spr.ang = 0;
        actorNew->spr.extra = 0;

        // vel
        SP_TAG7(actorNew) = 1;
        // zvel
        SP_TAG8(actorNew) = 40;

        if (actor->user.spal == PAL_XLAT_LT_TAN)
        {
            SP_TAG3(actorNew) = 44;
        }
        else if (actor->user.spal == PAL_XLAT_LT_GREY)
        {
            SP_TAG3(actorNew) = 46;
        }
        else if (actor->user.spal == PALETTE_PLAYER5) // Green Ninja
        {
            if (RANDOM_P2(1024) < 700)
                SP_TAG3(actorNew) = 61;
            else
                SP_TAG3(actorNew) = 60;
        }
        else if (actor->user.spal == PALETTE_PLAYER3) // Red Ninja
        {
            // type
            if (RANDOM_P2(1024) < 800)
                SP_TAG3(actorNew) = 68;
            else
                SP_TAG3(actorNew) = 44;
        }
        else
        {
            if (RANDOM_P2(1024) < 512)
                SP_TAG3(actorNew) = 41;
            else
                SP_TAG3(actorNew) = 68;
        }

        // match
        SP_TAG2(actorNew) = -1;
        // kill
        RESET_BOOL1(actorNew);
        SpawnItemsMatch(-1);
        break;

    case PACHINKO1:
    case PACHINKO2:
    case PACHINKO3:
    case PACHINKO4:

        ASSERT(actor->insector());
        actorNew = insertActor(actor->sector(), STAT_SPAWN_ITEMS);
        actorNew->spr.cstat = 0;
        actorNew->spr.extra = 0;
        actorNew->set_int_pos({ actor->int_pos().X, actor->int_pos().Y, ActorLowerZ(actor) + Z(10) });
        actorNew->spr.ang = actor->spr.ang;

        // vel
        SP_TAG7(actorNew) = 10;
        // zvel
        SP_TAG8(actorNew) = 10;

        if (actor->user.ID == PACHINKO1)
        {
            if (RANDOM_P2(1024) < 600)
                SP_TAG3(actorNew) = 64; // Small MedKit
            else
                SP_TAG3(actorNew) = 59; // Fortune Cookie
        }
        else if (actor->user.ID == PACHINKO2)
        {
            if (RANDOM_P2(1024) < 600)
                SP_TAG3(actorNew) = 52; // Lg Shot Shell
            else
                SP_TAG3(actorNew) = 68; // Uzi clip
        }
        else if (actor->user.ID == PACHINKO3)
        {
            if (RANDOM_P2(1024) < 600)
                SP_TAG3(actorNew) = 57;
            else
                SP_TAG3(actorNew) = 63;
        }
        else if (actor->user.ID == PACHINKO4)
        {
            if (RANDOM_P2(1024) < 600)
                SP_TAG3(actorNew) = 60;
            else
                SP_TAG3(actorNew) = 61;
        }

        // match
        SP_TAG2(actorNew) = -1;
        // kill
        RESET_BOOL1(actorNew);
        SpawnItemsMatch(-1);
        break;
    }

    return 0;
}

int SpawnItemsMatch(short match)
{
    DSWActor* spawnedActor = nullptr;

    SWStatIterator it(STAT_SPAWN_ITEMS);
    while (auto itActor = it.Next())
    {

        if (SP_TAG2(itActor) != match)
            continue;

        switch (SP_TAG3(itActor))
        {
        case 90:
            spawnedActor = BunnyHatch2(itActor);
            spawnedActor->user.spal = spawnedActor->spr.pal = PALETTE_PLAYER8; // Boy
            spawnedActor->spr.ang = itActor->spr.ang;
            break;
        case 91:
            spawnedActor = BunnyHatch2(itActor);
            spawnedActor->user.spal = spawnedActor->spr.pal = PALETTE_PLAYER0; // Girl
            spawnedActor->spr.ang = itActor->spr.ang;
            break;
        case 92:
            spawnedActor = BunnyHatch2(itActor);
            spawnedActor->spr.ang = itActor->spr.ang;
             break;

        case 40:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_REPAIR_KIT))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_REPAIR_KIT, s_RepairKit, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 41:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_STAR))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_STAR, s_IconStar, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 42:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_LG_MINE))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_LG_MINE, s_IconLgMine, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 43:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_MICRO_GUN))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_MICRO_GUN, s_IconMicroGun, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 44:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_MICRO_BATTERY))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_MICRO_BATTERY, s_IconMicroBattery, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 45:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_GRENADE_LAUNCHER))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_GRENADE_LAUNCHER, s_IconGrenadeLauncher, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 46:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_LG_GRENADE))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_LG_GRENADE, s_IconLgGrenade, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 47:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_RAIL_GUN))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_RAIL_GUN, s_IconRailGun, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 48:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_RAIL_AMMO))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_RAIL_AMMO, s_IconRailAmmo, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 49:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_ROCKET))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_ROCKET, s_IconRocket, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 51:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_SHOTGUN))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_SHOTGUN, s_IconShotgun, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 52:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_LG_SHOTSHELL))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_LG_SHOTSHELL, s_IconLgShotshell, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 53:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_GUARD_HEAD))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_GUARD_HEAD, s_IconGuardHead, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 54:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_FIREBALL_LG_AMMO))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_FIREBALL_LG_AMMO, s_IconFireballLgAmmo, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 55:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_HEART))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_HEART, s_IconHeart, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 56:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_HEART_LG_AMMO))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_HEART_LG_AMMO, s_IconHeartLgAmmo, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 57:
        {
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_ARMOR))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_ARMOR, s_IconArmor, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);

            if (spawnedActor->spr.pal != PALETTE_PLAYER3)
                spawnedActor->spr.pal = spawnedActor->user.spal = PALETTE_PLAYER1;
            else
                spawnedActor->spr.pal = spawnedActor->user.spal = PALETTE_PLAYER3;
            break;
        }

        case 58:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_MEDKIT))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_MEDKIT, s_IconMedkit, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 59:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_SM_MEDKIT))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_SM_MEDKIT, s_IconSmMedkit, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 60:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_CHEMBOMB))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_CHEMBOMB, s_IconChemBomb, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 61:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_FLASHBOMB))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_FLASHBOMB, s_IconFlashBomb, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 62:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_NUKE))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_NUKE, s_IconNuke, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 63:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_CALTROPS))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_CALTROPS, s_IconCaltrops, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 64:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_BOOSTER))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_BOOSTER, s_IconBooster, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 65:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_HEAT_CARD))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_HEAT_CARD, s_IconHeatCard, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 66:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_CLOAK))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_CLOAK, s_IconCloak, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 67:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_NIGHT_VISION))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_NIGHT_VISION, s_IconNightVision, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;


        case 68:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_LG_UZI_AMMO))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_LG_UZI_AMMO, s_IconLgUziAmmo, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 69:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_GUARD_HEAD))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_GUARD_HEAD, s_IconGuardHead, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 70:
            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_HEART))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_HEART, s_IconHeart, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 20:

            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_UZIFLOOR))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_UZIFLOOR, s_IconUziFloor, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;


        case 32:
        case 0:

            if (!ItemSpotClear(itActor, STAT_ITEM, ICON_UZI))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, ICON_UZI, s_IconUzi, itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);
            spawnedActor->user.Flags2 |= SPR2_NEVER_RESPAWN;
            IconDefault(spawnedActor);

            SetupItemForJump(itActor, spawnedActor);
            break;

        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        {
            short num;

            static const uint8_t KeyPal[] =
            {
                PALETTE_PLAYER9,
                PALETTE_PLAYER7,
                PALETTE_PLAYER6,
                PALETTE_PLAYER4,
                PALETTE_PLAYER9,
                PALETTE_PLAYER7,
                PALETTE_PLAYER6,
                PALETTE_PLAYER4,
                PALETTE_PLAYER4,
                PALETTE_PLAYER1,
                PALETTE_PLAYER8,
                PALETTE_PLAYER9
            };

            if (gNet.MultiGameType == MULTI_GAME_COMMBAT || gNet.MultiGameType == MULTI_GAME_AI_BOTS)
                break;

            num = SP_TAG3(itActor) - 1;

            if (!ItemSpotClear(itActor, STAT_ITEM, s_Key[num]->Pic))
                break;

            spawnedActor = SpawnActor(STAT_ITEM, s_Key[num]->Pic, s_Key[num], itActor->sector(), itActor->int_pos().X, itActor->int_pos().Y, itActor->int_pos().Z, itActor->spr.ang, 0);

            spawnedActor->spr.picnum = spawnedActor->user.ID = s_Key[num]->Pic;


            // need to set the palette here - suggest table lookup
            spawnedActor->user.spal = spawnedActor->spr.pal = KeyPal[num];


            ChangeState(spawnedActor, s_Key[num]);

            picanm[spawnedActor->spr.picnum].sf &= ~(PICANM_ANIMTYPE_MASK);
            picanm[spawnedActor->spr.picnum + 1].sf &= ~(PICANM_ANIMTYPE_MASK);

            SetupItemForJump(itActor, spawnedActor);

            break;
        }
        }

        if (!TEST_BOOL1(itActor))
            KillActor(itActor);
    }
    return 0;
}

int NewStateGroup(DSWActor* actor, STATE* StateGroup[])
{
    if (!StateGroup)
        return 0;

    ASSERT(actor->hasU());

    // Kind of a goofy check, but it should catch alot of invalid states!
    // BTW, 6144 is the max tile number allowed in editart.
    if (actor->user.State && (actor->user.State->Pic < 0 || actor->user.State->Pic > MAXTILES))    // JBF: verify this!
        return 0;

    actor->user.Rot = StateGroup;
    actor->user.State = actor->user.StateStart = StateGroup[0];

    actor->user.Tics = 0;

    // turn anims off because people keep setting them in the
    // art file
    picanm[actor->spr.picnum].sf &= ~(PICANM_ANIMTYPE_MASK);
    return 0;
}


bool SpriteOverlap(DSWActor* actor_a, DSWActor* actor_b)
{
    int spa_tos, spa_bos, spb_tos, spb_bos, overlap_z;

    if (!actor_a->hasU() || !actor_b->hasU()) return false;
    if ((unsigned)Distance(actor_a->int_pos().X, actor_a->int_pos().Y, actor_b->int_pos().X, actor_b->int_pos().Y) > actor_a->user.Radius + actor_b->user.Radius)
    {
        return false;
    }

    spa_tos = ActorZOfTop(actor_a);
    spa_bos = ActorZOfBottom(actor_a);

    spb_tos = ActorZOfTop(actor_b);
    spb_bos = ActorZOfBottom(actor_b);


    overlap_z = actor_a->user.OverlapZ + actor_b->user.OverlapZ;

    // if the top of sprite a is below the bottom of b
    if (spa_tos - overlap_z > spb_bos)
    {
        return false;
    }

    // if the top of sprite b is is below the bottom of a
    if (spb_tos - overlap_z > spa_bos)
    {
        return false;
    }

    return true;

}

bool SpriteOverlapZ(DSWActor* actor_a, DSWActor* actor_b, int z_overlap)
{
    int spa_tos, spa_bos, spb_tos, spb_bos;

    spa_tos = ActorZOfTop(actor_a);
    spa_bos = ActorZOfBottom(actor_a);

    spb_tos = ActorZOfTop(actor_b);
    spb_bos = ActorZOfBottom(actor_b);


    // if the top of sprite a is below the bottom of b
    if (spa_tos + z_overlap > spb_bos)
    {
        return false;
    }

    // if the top of sprite b is is below the bottom of a
    if (spb_tos + z_overlap > spa_bos)
    {
        return false;
    }

    return true;

}

void getzrangepoint(int x, int y, int z, sectortype* sect,
               int32_t* ceilz, Collision* ceilhit, int32_t* florz, Collision* florhit)
{
    int j, k, l, dax, day, daz, xspan, yspan, xoff, yoff;
    int x1, y1, x2, y2, x3, y3, x4, y4, cosang, sinang, tilenum;
    short cstat;
    char clipyou;

    if (sect == nullptr)
    {
        *ceilz = 0x80000000;

        *florz = 0x7fffffff;
        florhit->invalidate();
        return;
    }

    // Initialize z's and hits to the current sector's top&bottom
    getzsofslopeptr(sect, x, y, ceilz, florz);
    ceilhit->setSector(sect);
    florhit->setSector(sect);

    // Go through sprites of only the current sector
    SWSectIterator it(sect);
    while (auto itActor = it.Next())
    {
        cstat = itActor->spr.cstat;
        if ((cstat & (CSTAT_SPRITE_ALIGNMENT_MASK | CSTAT_SPRITE_BLOCK)) != (CSTAT_SPRITE_ALIGNMENT_FLOOR|CSTAT_SPRITE_BLOCK))
            continue;                   // Only check blocking floor sprites

        daz = itActor->int_pos().Z;

        // Only check if sprite's 2-sided or your on the 1-sided side
        if (((cstat & CSTAT_SPRITE_ONE_SIDE) != 0) && ((z > daz) == ((cstat & CSTAT_SPRITE_YFLIP) == 0)))
            continue;

        // Calculate and store centering offset information into xoff&yoff
        tilenum = itActor->spr.picnum;
        xoff = (int)tileLeftOffset(tilenum) + (int)itActor->spr.xoffset;
        yoff = (int)tileTopOffset(tilenum) + (int)itActor->spr.yoffset;
        if (cstat & CSTAT_SPRITE_XFLIP)
            xoff = -xoff;
        if (cstat & CSTAT_SPRITE_YFLIP)
            yoff = -yoff;

        // Calculate all 4 points of the floor sprite.
        // (x1,y1),(x2,y2),(x3,y3),(x4,y4)
        // These points will already have (x,y) subtracted from them
        cosang = bcos(itActor->spr.ang);
        sinang = bsin(itActor->spr.ang);
        xspan = tileWidth(tilenum);
        dax = ((xspan >> 1) + xoff) * itActor->spr.xrepeat;
        yspan = tileHeight(tilenum);
        day = ((yspan >> 1) + yoff) * itActor->spr.yrepeat;
        x1 = itActor->int_pos().X + DMulScale(sinang, dax, cosang, day, 16) - x;
        y1 = itActor->int_pos().Y + DMulScale(sinang, day, -cosang, dax, 16) - y;
        l = xspan * itActor->spr.xrepeat;
        x2 = x1 - MulScale(sinang, l, 16);
        y2 = y1 + MulScale(cosang, l, 16);
        l = yspan * itActor->spr.yrepeat;
        k = -MulScale(cosang, l, 16);
        x3 = x2 + k;
        x4 = x1 + k;
        k = -MulScale(sinang, l, 16);
        y3 = y2 + k;
        y4 = y1 + k;

        // Check to see if point (0,0) is inside the 4 points by seeing if
        // the number of lines crossed as a line is shot outward is odd
        clipyou = 0;
        if ((y1 ^ y2) < 0)              // If y1 and y2 have different signs
        // (- / +)
        {
            if ((x1 ^ x2) < 0)
                clipyou ^= (x1 * y2 < x2 * y1) ^ (y1 < y2);
            else if (x1 >= 0)
                clipyou ^= 1;
        }
        if ((y2 ^ y3) < 0)
        {
            if ((x2 ^ x3) < 0)
                clipyou ^= (x2 * y3 < x3 * y2) ^ (y2 < y3);
            else if (x2 >= 0)
                clipyou ^= 1;
        }
        if ((y3 ^ y4) < 0)
        {
            if ((x3 ^ x4) < 0)
                clipyou ^= (x3 * y4 < x4 * y3) ^ (y3 < y4);
            else if (x3 >= 0)
                clipyou ^= 1;
        }
        if ((y4 ^ y1) < 0)
        {
            if ((x4 ^ x1) < 0)
                clipyou ^= (x4 * y1 < x1 * y4) ^ (y4 < y1);
            else if (x4 >= 0)
                clipyou ^= 1;
        }
        if (clipyou == 0)
            continue;                   // Point is not inside, don't clip

        // Clipping time!
        if (z > daz)
        {
            if (daz > *ceilz)
            {
                *ceilz = daz;
                ceilhit->setSprite(itActor);
            }
        }
        else
        {
            if (daz < *florz)
            {
                *florz = daz;
                florhit->setSprite(itActor);
            }
        }
    }
}


void DoActorZrange(DSWActor* actor)
{
    Collision ceilhit, florhit;

    auto save_cstat = actor->spr.cstat & CSTAT_SPRITE_BLOCK;
    actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK);
    vec3_t pos = actor->int_pos();
    pos.Z -= (ActorSizeZ(actor) >> 1);
    FAFgetzrange(pos, actor->sector(), &actor->user.hiz, &ceilhit, &actor->user.loz, &florhit, (((int) actor->spr.clipdist) << 2) - GETZRANGE_CLIP_ADJ, CLIPMASK_ACTOR);
    actor->spr.cstat |= save_cstat;

    actor->user.lo_sectp = actor->user.hi_sectp = nullptr;
    actor->user.highActor = nullptr;
    actor->user.lowActor = nullptr;

    switch (ceilhit.type)
    {
    case kHitSprite:
        actor->user.highActor = ceilhit.actor();
        break;
    case kHitSector:
        actor->user.hi_sectp = ceilhit.hitSector;
        break;
    default:
        ASSERT(true==false);
        break;
    }

    switch (florhit.type)
    {
    case kHitSprite:
        actor->user.lowActor = florhit.actor();
        break;
    case kHitSector:
        actor->user.lo_sectp = florhit.hitSector;
        break;
    default:
        ASSERT(true==false);
        break;
    }
}

// !AIC - puts getzrange results into USER varaible actor->user.loz, actor->user.hiz, actor->user.lo_sectp, actor->user.hi_sectp, etc.
// The loz and hiz are used a lot.

int DoActorGlobZ(DSWActor* actor)
{
    actor->user.loz = globloz;
    actor->user.hiz = globhiz;

    actor->user.lo_sectp = actor->user.hi_sectp = nullptr;
    actor->user.highActor = nullptr;
    actor->user.lowActor = nullptr;

    switch (globhihit.type)
    {
    case kHitSprite:
        actor->user.highActor = globhihit.actor();
        break;
    default:
        actor->user.hi_sectp = globhihit.hitSector;
        break;
    }

    switch (globlohit.type)
    {
    case kHitSprite:
        actor->user.lowActor = globlohit.actor();
        break;
    default:
        actor->user.lo_sectp = globlohit.hitSector;
        break;
    }

    return 0;
}


bool ActorDrop(DSWActor* actor, int x, int y, int z, sectortype* new_sector, short min_height)
{
    int hiz, loz;
    Collision ceilhit, florhit;

    // look only at the center point for a floor sprite
    auto save_cstat = (actor->spr.cstat & CSTAT_SPRITE_BLOCK);
    actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK);
    FAFgetzrangepoint(x, y, z - (ActorSizeZ(actor) >> 1), new_sector, &hiz, &ceilhit, &loz, &florhit);
    actor->spr.cstat |= (save_cstat);

    if (florhit.type < 0 || ceilhit.type < 0)
    {
        return true;
    }

    switch (florhit.type)
    {
    case kHitSprite:
    {
        auto hsp = florhit.actor();

        // if its a floor sprite and not too far down
        if ((hsp->spr.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR) &&
            (labs(loz - z) <= min_height))
        {
            return false;
        }

        break;
    }

    case kHitSector:
    {
        if (labs(loz - z) <= min_height)
        {
            return false;
        }

        break;
    }
    default:
        ASSERT(true == false);
        break;
    }

    return true;
}

// Primarily used in ai.c for now - need to get rid of
bool DropAhead(DSWActor* actor, int  min_height)
{
    int dax, day;

    // dax = actor->spr.x + MOVEx(128, actor->spr.ang);
    // day = actor->spr.y + MOVEy(128, actor->spr.ang);

    dax = actor->int_pos().X + MOVEx(256, actor->spr.ang);
    day = actor->int_pos().Y + MOVEy(256, actor->spr.ang);

    auto newsector = actor->sector();
    updatesector(dax, day, &newsector);

    // look straight down for a drop
    if (ActorDrop(actor, dax, day, actor->int_pos().Z, newsector, min_height))
        return true;

    return false;
}

/*

  !AIC KEY - Called by ai.c routines.  Calls move_sprite which calls clipmove.
  This incapulates move_sprite and makes sure that actors don't walk off of
  ledges.  If it finds itself in mid air then it restores the last good
  position.  This is a hack because Ken had no good way of doing this from his
  code.  ActorDrop() is called from here.

*/

int move_actor(DSWActor* actor, int xchange, int ychange, int zchange)
{
    int x, y, z, loz, hiz;
    DSWActor* highActor;
    DSWActor* lowActor;
    sectortype* lo_sectp,* hi_sectp;
    short dist;
    int cliptype = CLIPMASK_ACTOR;


    if (actor->user.Flags & (SPR_NO_SCAREDZ))
    {
        // For COOLG & HORNETS
        // set to actual z before you move
        actor->set_int_z(actor->user.pos.Z);
    }

    // save off x,y values
    x = actor->int_pos().X;
    y = actor->int_pos().Y;
    z = actor->int_pos().Z;
    loz = actor->user.loz;
    hiz = actor->user.hiz;
    lowActor = actor->user.lowActor;
    highActor = actor->user.highActor;
    lo_sectp = actor->user.lo_sectp;
    hi_sectp = actor->user.hi_sectp;
    auto sect = actor->sector();

    actor->user.coll = move_sprite(actor, xchange, ychange, zchange,
                         actor->user.ceiling_dist, actor->user.floor_dist, cliptype, ACTORMOVETICS);

    ASSERT(actor->insector());

    // try and determine whether you moved > lo_step in the z direction
    if (!(actor->user.Flags & (SPR_NO_SCAREDZ | SPR_JUMPING | SPR_CLIMBING | SPR_FALLING | SPR_DEAD | SPR_SWIMMING)))
    {
        if (labs(actor->int_pos().Z - globloz) > actor->user.lo_step)
        {
            // cancel move
            actor->set_int_pos({ x, y, z });
            //actor->spr.z = actor->user.loz;             // place on ground in case you are in the air
            actor->user.loz = loz;
            actor->user.hiz = hiz;
            actor->user.lowActor = lowActor;
            actor->user.highActor = highActor;
            actor->user.lo_sectp = lo_sectp;
            actor->user.hi_sectp = hi_sectp;
            actor->user.coll.invalidate();
            ChangeActorSect(actor, sect);
            return false;
        }

        if (ActorDrop(actor, actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->sector(), actor->user.lo_step))
        {
            // cancel move
            actor->set_int_pos({ x, y, z });
            //actor->spr.z = actor->user.loz;             // place on ground in case you are in the air
            actor->user.loz = loz;
            actor->user.hiz = hiz;
            actor->user.lowActor = lowActor;
            actor->user.highActor = highActor;
            actor->user.lo_sectp = lo_sectp;
            actor->user.hi_sectp = hi_sectp;
            actor->user.coll.invalidate();
            ChangeActorSect(actor, sect);
            return false;
        }
    }

    actor->user.Flags |= (SPR_MOVED);

    if (actor->user.coll.type == kHitNone)
    {
        // Keep track of how far sprite has moved
        dist = Distance(x, y, actor->int_pos().X, actor->int_pos().Y);
        actor->user.TargetDist -= dist;
        actor->user.Dist += dist;
        actor->user.DistCheck += dist;
        return true;
    }
    else
    {
        return false;
    }

}

int DoStayOnFloor(DSWActor* actor)
{
    actor->set_int_z(actor->sector()->int_floorz());
    return 0;
}

int DoGrating(DSWActor* actor)
{
    int dir;
    const int GRATE_FACTOR = 3;

    // reduce to 0 to 3 value
    dir = actor->spr.ang >> 9;

    int x = 0, y = 0;
    if ((dir & 1) == 0)
    {
        if (dir == 0)
            x = 2 * GRATE_FACTOR;
        else
            x = -2 * GRATE_FACTOR;
    }
    else
    {
        if (dir == 1)
            y= 2 * GRATE_FACTOR;
        else
            y= -2 * GRATE_FACTOR;
    }
    actor->add_int_pos({ x, y, 0 });

    actor->spr.hitag -= GRATE_FACTOR;

    if (actor->spr.hitag <= 0)
    {
        change_actor_stat(actor, STAT_DEFAULT);
        actor->clearUser();
    }

    SetActorZ(actor, actor->int_pos());

    return 0;
}



int DoKey(DSWActor* actor)
{
    actor->spr.ang = NORM_ANGLE(actor->spr.ang + (14 * ACTORMOVETICS));

    DoGet(actor);
    return 0;
}

int DoCoin(DSWActor* actor)
{
    int offset;

    actor->user.WaitTics -= ACTORMOVETICS * 2;

    if (actor->user.WaitTics <= 0)
    {
        KillActor(actor);
        return 0;
    }

    if (actor->user.WaitTics < 10*120)
    {
        if (actor->user.StateStart != s_GreenCoin)
        {
            offset = int(actor->user.State - actor->user.StateStart);
            ChangeState(actor, s_GreenCoin);
            actor->user.State = actor->user.StateStart + offset;
        }
    }
    else if (actor->user.WaitTics < 20*120)
    {
        if (actor->user.StateStart != s_YellowCoin)
        {
            offset = int(actor->user.State - actor->user.StateStart);
            ChangeState(actor, s_YellowCoin);
            actor->user.State = actor->user.StateStart + offset;
        }
    }

    return 0;
}

int KillGet(DSWActor* actor)
{
    switch (gNet.MultiGameType)
    {
    case MULTI_GAME_NONE:
    case MULTI_GAME_COOPERATIVE:
        KillActor(actor);
        break;
    case MULTI_GAME_COMMBAT:
    case MULTI_GAME_AI_BOTS:

        if (actor->user.Flags2 & (SPR2_NEVER_RESPAWN))
        {
            KillActor(actor);
            break;
        }

        actor->user.WaitTics = 30*120;
        actor->spr.cstat |= (CSTAT_SPRITE_INVISIBLE);

        // respawn markers
        if (!gNet.SpawnMarkers || actor->spr.hitag == TAG_NORESPAWN_FLAG)  // No coin if it's a special flag
            break;

        auto actorNew = SpawnActor(STAT_ITEM, Red_COIN, s_RedCoin, actor->sector(),
                          actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, 0, 0);

        actorNew->spr.shade = -20;
        actorNew->user.WaitTics = actor->user.WaitTics - 12;

        break;
    }
    return 0;
}

int KillGetAmmo(DSWActor* actor)
{
    switch (gNet.MultiGameType)
    {
    case MULTI_GAME_NONE:
    case MULTI_GAME_COOPERATIVE:
        KillActor(actor);
        break;

    case MULTI_GAME_COMMBAT:
    case MULTI_GAME_AI_BOTS:

        if (actor->user.Flags2 & (SPR2_NEVER_RESPAWN))
        {
            KillActor(actor);
            break;
        }

        // No Respawn mode - all ammo goes away
        if (gNet.NoRespawn)
        {
            KillActor(actor);
            break;
        }

        actor->user.WaitTics = 30*120;
        actor->spr.cstat |= (CSTAT_SPRITE_INVISIBLE);

        // respawn markers
        if (!gNet.SpawnMarkers)
            break;

        auto actorNew = SpawnActor(STAT_ITEM, Red_COIN, s_RedCoin, actor->sector(),
                          actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, 0, 0);

        actorNew->spr.shade = -20;
        actorNew->user.WaitTics = actor->user.WaitTics - 12;

        break;
    }
    return 0;
}

int KillGetWeapon(DSWActor* actor)
{
    switch (gNet.MultiGameType)
    {
    case MULTI_GAME_NONE:
        KillActor(actor);
        break;

    case MULTI_GAME_COOPERATIVE:
        // don't kill weapons in coop

        // unless told too :)
        if (actor->user.Flags2 & (SPR2_NEVER_RESPAWN))
        {
            KillActor(actor);
            break;
        }
        break;

    case MULTI_GAME_COMMBAT:
    case MULTI_GAME_AI_BOTS:

        if (actor->user.Flags2 & (SPR2_NEVER_RESPAWN))
        {
            KillActor(actor);
            break;
        }

        // No Respawn mode - all weapons stay
        // but can only get once
        if (gNet.NoRespawn)
            break;

        actor->user.WaitTics = 30*120;
        actor->spr.cstat |= (CSTAT_SPRITE_INVISIBLE);

        // respawn markers
        if (!gNet.SpawnMarkers)
            break;

        auto actorNew = SpawnActor(STAT_ITEM, Red_COIN, s_RedCoin, actor->sector(),
                          actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, 0, 0);

        actorNew->spr.shade = -20;
        actorNew->user.WaitTics = actor->user.WaitTics - 12;

        break;
    }
    return 0;
}

int DoSpawnItemTeleporterEffect(DSWActor* actor)
{
    extern STATE s_TeleportEffect[];

    auto effect = SpawnActor(STAT_MISSILE, 0, s_TeleportEffect, actor->sector(),
                         actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z - Z(12),
                         actor->spr.ang, 0);

    effect->spr.shade = -40;
    effect->spr.xrepeat = effect->spr.yrepeat = 36;
    effect->spr.cstat |= CSTAT_SPRITE_YCENTER;
    effect->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
    return 0;
}

void ChoosePlayerGetSound(PLAYER* pp)
{
    int choose_snd=0;

    if (pp != Player+myconnectindex) return;

    choose_snd = StdRandomRange((MAX_GETSOUNDS-1)<<8)>>8;

    PlayerSound(PlayerGetItemVocs[choose_snd], v3df_follow|v3df_dontpan,pp);
}

bool CanGetWeapon(PLAYER* pp, DSWActor* actor, int WPN)
{
    switch (gNet.MultiGameType)
    {
    case MULTI_GAME_NONE:
        return true;

    case MULTI_GAME_COOPERATIVE:
        if (actor->user.Flags2 & (SPR2_NEVER_RESPAWN))
            return true;

        if (pp->WpnGotOnceFlags & BIT(WPN))
            return false;

        return true;

    case MULTI_GAME_COMMBAT:
    case MULTI_GAME_AI_BOTS:

        if (actor->user.Flags2 & (SPR2_NEVER_RESPAWN))
            return true;

        // No Respawn - can't get a weapon again if you already got it
        if (gNet.NoRespawn && (pp->WpnGotOnceFlags & BIT(WPN)))
            return false;

        return true;
    }

    return true;
}

struct InventoryDecl_t InventoryDecls[InvDecl_TOTAL] =
{
    {50  },
    {100 },
    {20  },
    {50  },
    {100 },
    {1   },
    {2   },
    {3   },
    {100 },
    {100 },
    {100 },
};

enum
{
    ITEMFLASHAMT = -8,
    ITEMFLASHCLR = 144
};
int DoGet(DSWActor* actor)
{
    PLAYER* pp;
    short pnum, key_num;
    int dist, a,b,c;
    bool can_see;

    // For flag stuff

    // Invisiblility is only used for DeathMatch type games
    // Sprites stays invisible for a period of time and is un-gettable
    // then "Re-Spawns" by becomming visible.  Its never actually killed.
    if (actor->spr.cstat & (CSTAT_SPRITE_INVISIBLE))
    {
        actor->user.WaitTics -= ACTORMOVETICS * 2;
        if (actor->user.WaitTics <= 0)
        {
            PlaySound(DIGI_ITEM_SPAWN, actor, v3df_none);
            DoSpawnItemTeleporterEffect(actor);
            actor->spr.cstat &= ~(CSTAT_SPRITE_INVISIBLE);
        }

        return 0;
    }

    if (actor->spr.xvel)
    {
        if (!DoItemFly(actor))
        {
            actor->spr.xvel = 0;
            change_actor_stat(actor, STAT_ITEM);
        }
    }

    TRAVERSE_CONNECT(pnum)
    {
        pp = &Player[pnum];
        DSWActor* plActor = pp->actor;

        if (pp->Flags & (PF_DEAD))
            continue;

        DISTANCE(pp->pos.X, pp->pos.Y, actor->int_pos().X, actor->int_pos().Y, dist, a,b,c);
        if ((unsigned)dist > (plActor->user.Radius + actor->user.Radius))
        {
            continue;
        }

        if (!SpriteOverlap(actor, pp->actor))
        {
            continue;
        }

        auto cstat_bak = actor->spr.cstat;
        actor->spr.cstat |= (CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
        can_see = FAFcansee(actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, actor->sector(),
                            pp->pos.X, pp->pos.Y, pp->pos.Z, pp->cursector);
        actor->spr.cstat = cstat_bak;

        if (!can_see)
        {
            continue;
        }

        switch (actor->user.ID)
        {
        //
        // Keys
        //
        case RED_CARD:
        case RED_KEY:
            key_num = 0;
            goto KeyMain;
        case BLUE_CARD:
        case BLUE_KEY:
            key_num = 1;
            goto KeyMain;
        case GREEN_CARD:
        case GREEN_KEY:
            key_num = 2;
            goto KeyMain;
        case YELLOW_CARD:
        case YELLOW_KEY:
            key_num = 3;
            goto KeyMain;
        case GOLD_SKELKEY:
            key_num = 4;
            goto KeyMain;
        case SILVER_SKELKEY:
            key_num = 5;
            goto KeyMain;
        case BRONZE_SKELKEY:
            key_num = 6;
            goto KeyMain;
        case RED_SKELKEY:
            key_num = 7;
KeyMain:

            if (pp->HasKey[key_num])
                break;

            PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_KEYMSG + key_num));

            pp->HasKey[key_num] = true;
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_KEY, actor, v3df_dontpan);

            // don't kill keys in coop
            if (gNet.MultiGameType == MULTI_GAME_COOPERATIVE)
                break;

            KillActor(actor);
            break;

        case ICON_ARMOR:
            if (pp->Armor < InventoryDecls[InvDecl_Kevlar].amount)
            {
                if (actor->user.spal == PALETTE_PLAYER3)
                {
                    PlayerUpdateArmor(pp, 1000+InventoryDecls[InvDecl_Kevlar].amount);
                    PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_INVENTORY + InvDecl_Kevlar));
                }
                else
                {
                    if (pp->Armor < InventoryDecls[InvDecl_Armor].amount)
                    {
                        PlayerUpdateArmor(pp, 1000+InventoryDecls[InvDecl_Armor].amount);
                        PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_INVENTORY + InvDecl_Armor));
                    }
                    else
                        break;
                }
                SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_BIGITEM, actor, v3df_dontpan);

                // override for respawn mode
                if (gNet.MultiGameType == MULTI_GAME_COMMBAT && gNet.NoRespawn)
                {
                    KillActor(actor);
                    break;
                }

                KillGet(actor);
            }
            break;

        //
        // Health - Instant Use
        //

        case ICON_SM_MEDKIT:
            if (plActor->user.Health < 100)
            {
                bool putbackmax=false;

                PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_INVENTORY + InvDecl_SmMedkit));

                if (pp->MaxHealth == 200)
                {
                    pp->MaxHealth = 100;
                    putbackmax = true;
                }
                PlayerUpdateHealth(pp, InventoryDecls[InvDecl_SmMedkit].amount);

                if (putbackmax) pp->MaxHealth = 200;

                SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_ITEM, actor, v3df_dontpan);

                // override for respawn mode
                if (gNet.MultiGameType == MULTI_GAME_COMMBAT && gNet.NoRespawn)
                {
                    KillActor(actor);
                    break;
                }

                KillGet(actor);
            }
            break;

        case ICON_BOOSTER:   // Fortune cookie
            pp->MaxHealth = 200;
            if (plActor->user.Health < 200)
            {
                PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_INVENTORY + InvDecl_Booster));
                PlayerUpdateHealth(pp, InventoryDecls[InvDecl_Booster].amount);       // This is for health
                // over 100%
                // Say something witty
                if (pp == Player+myconnectindex)
                {
                    int cookie = StdRandomRange(MAX_FORTUNES);
                    // print to the console, and the user quote display.
                    FStringf msg("%s %s", GStrings("TXTS_FORTUNE"), quoteMgr.GetQuote(QUOTE_COOKIE + cookie));
                    Printf(PRINT_NONOTIFY, TEXTCOLOR_SAPPHIRE "%s\n", msg.GetChars());
                    if (hud_messages)
                    {
                        strncpy(pp->cookieQuote, msg, 255);
                        pp->cookieQuote[255] = 0;
                        pp->cookieTime = 540;
                    }
                }

                SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_BIGITEM, actor, v3df_dontpan);

                // override for respawn mode
                if (gNet.MultiGameType == MULTI_GAME_COMMBAT && gNet.NoRespawn)
                {
                    KillActor(actor);
                    break;
                }

                KillGet(actor);
            }
            break;

        //
        // Inventory
        //
        case ICON_MEDKIT:

            if (!pp->InventoryAmount[INVENTORY_MEDKIT] || pp->InventoryPercent[INVENTORY_MEDKIT] < InventoryDecls[InvDecl_Medkit].amount)
            {
                PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_INVENTORY + InvDecl_Medkit));
                pp->InventoryPercent[INVENTORY_MEDKIT] = InventoryDecls[InvDecl_Medkit].amount;
                pp->InventoryAmount[INVENTORY_MEDKIT] = 1;
                PlayerUpdateInventory(pp, INVENTORY_MEDKIT);
                SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_ITEM, actor, v3df_dontpan);

                // override for respawn mode
                if (gNet.MultiGameType == MULTI_GAME_COMMBAT && gNet.NoRespawn)
                {
                    KillActor(actor);
                    break;
                }

                KillGet(actor);
            }
            break;

        case ICON_CHEMBOMB:

            if (pp->InventoryAmount[INVENTORY_CHEMBOMB] < InventoryDecls[InvDecl_ChemBomb].amount)
            {
                PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_INVENTORY + InvDecl_ChemBomb));
                pp->InventoryPercent[INVENTORY_CHEMBOMB] = 0;
                pp->InventoryAmount[INVENTORY_CHEMBOMB]++;
                PlayerUpdateInventory(pp, INVENTORY_CHEMBOMB);
                SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_ITEM, actor, v3df_dontpan);
                KillGet(actor);
            }
            break;

        case ICON_FLASHBOMB:

            if (pp->InventoryAmount[INVENTORY_FLASHBOMB] < InventoryDecls[InvDecl_FlashBomb].amount)
            {
                PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_INVENTORY + InvDecl_FlashBomb));
                pp->InventoryPercent[INVENTORY_FLASHBOMB] = 0;
                pp->InventoryAmount[INVENTORY_FLASHBOMB]++;
                PlayerUpdateInventory(pp, INVENTORY_FLASHBOMB);
                SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_ITEM, actor, v3df_dontpan);
                KillGet(actor);
            }
            break;

        case ICON_CALTROPS:

            if (pp->InventoryAmount[INVENTORY_CALTROPS] < InventoryDecls[InvDecl_Caltrops].amount)
            {
                PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_INVENTORY + InvDecl_Caltrops));
                pp->InventoryPercent[INVENTORY_CALTROPS] = 0;
                pp->InventoryAmount[INVENTORY_CALTROPS]+=3;
                if (pp->InventoryAmount[INVENTORY_CALTROPS] > InventoryDecls[InvDecl_Caltrops].amount)
                    pp->InventoryAmount[INVENTORY_CALTROPS] = InventoryDecls[InvDecl_Caltrops].amount;
                PlayerUpdateInventory(pp, INVENTORY_CALTROPS);
                SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_ITEM, actor, v3df_dontpan);
                KillGet(actor);
            }
            break;

        case ICON_NIGHT_VISION:
            if (!pp->InventoryAmount[INVENTORY_NIGHT_VISION] || pp->InventoryPercent[INVENTORY_NIGHT_VISION] < InventoryDecls[InvDecl_NightVision].amount)
            {
                PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_INVENTORY + InvDecl_NightVision));
                pp->InventoryPercent[INVENTORY_NIGHT_VISION] = InventoryDecls[InvDecl_NightVision].amount;
                pp->InventoryAmount[INVENTORY_NIGHT_VISION] = 1;
                PlayerUpdateInventory(pp, INVENTORY_NIGHT_VISION);
                SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_ITEM, actor, v3df_dontpan);
                KillGet(actor);
            }
            break;
        case ICON_REPAIR_KIT:
            if (!pp->InventoryAmount[INVENTORY_REPAIR_KIT] || pp->InventoryPercent[INVENTORY_REPAIR_KIT] < InventoryDecls[InvDecl_RepairKit].amount)
            {
                PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_INVENTORY + InvDecl_RepairKit));
                pp->InventoryPercent[INVENTORY_REPAIR_KIT] = InventoryDecls[InvDecl_RepairKit].amount;
                pp->InventoryAmount[INVENTORY_REPAIR_KIT] = 1;
                PlayerUpdateInventory(pp, INVENTORY_REPAIR_KIT);
                SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_ITEM, actor, v3df_dontpan);

                // don't kill repair kit in coop
                if (gNet.MultiGameType == MULTI_GAME_COOPERATIVE)
                    break;

                KillGet(actor);
            }
            break;
#if 0
        case ICON_ENVIRON_SUIT:
            if (!pp->InventoryAmount[INVENTORY_ENVIRON_SUIT] || pp->InventoryPercent[INVENTORY_ENVIRON_SUIT] < 100)
            {
                pp->InventoryPercent[INVENTORY_ENVIRON_SUIT] = 100;
                pp->InventoryAmount[INVENTORY_ENVIRON_SUIT] = 1;
                PlayerUpdateInventory(pp, INVENTORY_ENVIRON_SUIT);
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_ITEM, actor, v3df_dontpan);
                KillGet(actor);
            }
            break;
#endif
        case ICON_CLOAK:
            if (!pp->InventoryAmount[INVENTORY_CLOAK] || pp->InventoryPercent[INVENTORY_CLOAK] < InventoryDecls[InvDecl_Cloak].amount)
            {
                PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_INVENTORY + InvDecl_Cloak));
                pp->InventoryPercent[INVENTORY_CLOAK] = InventoryDecls[InvDecl_Cloak].amount;
                pp->InventoryAmount[INVENTORY_CLOAK] = 1;
                PlayerUpdateInventory(pp, INVENTORY_CLOAK);
                SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_ITEM, actor, v3df_dontpan);
                KillGet(actor);
            }
            break;
        //
        // Weapon
        //
        case ICON_STAR:

            if (!CanGetWeapon(pp, actor, WPN_STAR))
                break;

            pp->WpnGotOnceFlags |= (BIT(WPN_STAR));

            if (pp->WpnAmmo[WPN_STAR] >= DamageData[WPN_STAR].max_ammo)
                break;

            PutStringInfo(Player+pnum, sw_darts? GStrings("TXTS_DARTS") : quoteMgr.GetQuote(QUOTE_WPNSHURIKEN));
            PlayerUpdateAmmo(pp, WPN_STAR, DamageData[WPN_STAR].weapon_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, actor, v3df_dontpan);
            KillGetWeapon(actor);
            if (pp->WpnFlags & (BIT(WPN_STAR)))
                break;
            pp->WpnFlags |= (BIT(WPN_STAR));

            if (!cl_weaponswitch)
                break;
            if (plActor->user.WeaponNum <= WPN_STAR && plActor->user.WeaponNum != WPN_SWORD)
                break;
            InitWeaponStar(pp);
            break;

        case ICON_LG_MINE:

            if (!CanGetWeapon(pp, actor, WPN_MINE))
                break;

            pp->WpnGotOnceFlags |= (BIT(WPN_MINE));

            if (pp->WpnAmmo[WPN_MINE] >= DamageData[WPN_MINE].max_ammo)
                break;
            //sprintf(ds,"Sticky Bombs");
            PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_WPNSTICKY));
            PlayerUpdateAmmo(pp, WPN_MINE, DamageData[WPN_MINE].weapon_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, actor, v3df_dontpan);
            ChoosePlayerGetSound(pp);
            KillGetWeapon(actor);
            if (pp->WpnFlags & (BIT(WPN_MINE)))
                break;
            pp->WpnFlags |= (BIT(WPN_MINE));

            if (!cl_weaponswitch)
                break;
            if (plActor->user.WeaponNum > WPN_MINE && plActor->user.WeaponNum != WPN_SWORD)
                break;
            InitWeaponMine(pp);
            break;

        case ICON_UZI:
        case ICON_UZIFLOOR:

            if (!CanGetWeapon(pp, actor, WPN_UZI))
                break;

            pp->WpnGotOnceFlags |= (BIT(WPN_UZI));

            if (pp->Flags & (PF_TWO_UZI) && pp->WpnAmmo[WPN_UZI] >= DamageData[WPN_UZI].max_ammo)
                break;
            //sprintf(ds,"UZI Submachine Gun");
            PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_WPNUZI));
//            pp->WpnAmmo[WPN_UZI] += 50;
            PlayerUpdateAmmo(pp, WPN_UZI, DamageData[WPN_UZI].weapon_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, actor, v3df_dontpan);
            KillGetWeapon(actor);

            if (pp->WpnFlags & (BIT(WPN_UZI)) && (pp->Flags & PF_TWO_UZI))
                break;
            // flag to help with double uzi powerup - simpler but kludgy
            pp->Flags |= (PF_PICKED_UP_AN_UZI);
            if (pp->WpnFlags & (BIT(WPN_UZI)))
            {
                pp->Flags |= (PF_TWO_UZI);
                pp->WpnUziType = 0; // Let it come up
                if (pp == Player+myconnectindex)
                    PlayerSound(DIGI_DOUBLEUZI, v3df_dontpan|v3df_follow, pp);
            }
            else
            {
                pp->WpnFlags |= (BIT(WPN_UZI));
                ChoosePlayerGetSound(pp);
            }

            if (!cl_weaponswitch)
                break;

            if (plActor->user.WeaponNum > WPN_UZI && plActor->user.WeaponNum != WPN_SWORD)
                break;

            InitWeaponUzi(pp);
            break;

        case ICON_LG_UZI_AMMO:
            if (pp->WpnAmmo[WPN_UZI] >= DamageData[WPN_UZI].max_ammo)
                break;
            //sprintf(ds,"UZI Clip");
            PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_AMMOUZI));
            PlayerUpdateAmmo(pp, WPN_UZI, DamageData[WPN_UZI].ammo_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, actor, v3df_dontpan);
            KillGetAmmo(actor);
            break;

        case ICON_MICRO_GUN:

            if (!CanGetWeapon(pp, actor, WPN_MICRO))
                break;

            pp->WpnGotOnceFlags |= (BIT(WPN_MICRO));

            if (pp->WpnFlags & (BIT(WPN_MICRO)) && pp->WpnAmmo[WPN_MICRO] >= DamageData[WPN_MICRO].max_ammo)
                break;
            //sprintf(ds,"Missile Launcher");
            PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_WPNLAUNCH));
//            pp->WpnAmmo[WPN_MICRO] += 5;
            PlayerUpdateAmmo(pp, WPN_MICRO, DamageData[WPN_MICRO].weapon_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, actor, v3df_dontpan);
            ChoosePlayerGetSound(pp);
            KillGetWeapon(actor);
            if (pp->WpnFlags & (BIT(WPN_MICRO)))
                break;
            pp->WpnFlags |= (BIT(WPN_MICRO));

            if (!cl_weaponswitch)
                break;
            if (plActor->user.WeaponNum > WPN_MICRO && plActor->user.WeaponNum != WPN_SWORD)
                break;
            InitWeaponMicro(pp);
            break;

        case ICON_MICRO_BATTERY:
            if (pp->WpnAmmo[WPN_MICRO] >= DamageData[WPN_MICRO].max_ammo)
                break;
            //sprintf(ds,"Missiles");
            PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_AMMOLAUNCH));
            PlayerUpdateAmmo(pp, WPN_MICRO, DamageData[WPN_MICRO].ammo_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, actor, v3df_dontpan);
            KillGetAmmo(actor);
            break;

        case ICON_NUKE:
            if (pp->WpnRocketNuke != 1)
            {
                //sprintf(ds,"Nuclear Warhead");
                PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_WPNNUKE));
                pp->WpnRocketNuke =uint8_t(DamageData[DMG_NUCLEAR_EXP].weapon_pickup);
                SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_ITEM, actor, v3df_dontpan);
                if (StdRandomRange(1000) > 800 && pp == Player+myconnectindex)
                    PlayerSound(DIGI_ILIKENUKES, v3df_dontpan|v3df_doppler|v3df_follow,pp);
                if (pp->CurWpn == pp->Wpn[WPN_MICRO])
                {
                    if (pp->WpnRocketType != 2)
                    {
                        extern PANEL_STATE ps_MicroNukeFlash[];
                        pp->CurWpn->over[MICRO_SHOT_NUM].tics = 0;
                        pp->CurWpn->over[MICRO_SHOT_NUM].State = ps_MicroNukeFlash;
                        // Play Nuke available sound here!
                    }

                }

                KillGetAmmo(actor);
            }
            break;

        case ICON_GRENADE_LAUNCHER:
            if (!CanGetWeapon(pp, actor, WPN_GRENADE))
                break;

            pp->WpnGotOnceFlags |= (BIT(WPN_GRENADE));

            if (pp->WpnFlags & (BIT(WPN_GRENADE)) && pp->WpnAmmo[WPN_GRENADE] >= DamageData[WPN_GRENADE].max_ammo)
                break;
            //sprintf(ds,"Grenade Launcher");
            PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_WPNGRENADE));
//            pp->WpnAmmo[WPN_GRENADE] += 6;
            PlayerUpdateAmmo(pp, WPN_GRENADE, DamageData[WPN_GRENADE].weapon_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, actor, v3df_dontpan);
            //ChoosePlayerGetSound(pp);
            if (StdRandomRange(1000) > 800 && pp == Player+myconnectindex)
                PlayerSound(DIGI_LIKEBIGWEAPONS, v3df_dontpan|v3df_doppler|v3df_follow,pp);
            KillGetWeapon(actor);
            if (pp->WpnFlags & (BIT(WPN_GRENADE)))
                break;
            pp->WpnFlags |= (BIT(WPN_GRENADE));

            if (!cl_weaponswitch)
                break;
            if (plActor->user.WeaponNum > WPN_GRENADE && plActor->user.WeaponNum != WPN_SWORD)
                break;
            InitWeaponGrenade(pp);
            break;

        case ICON_LG_GRENADE:
            if (pp->WpnAmmo[WPN_GRENADE] >= DamageData[WPN_GRENADE].max_ammo)
                break;
            //sprintf(ds,"Grenade Shells");
            PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_AMMOGRENADE));
            PlayerUpdateAmmo(pp, WPN_GRENADE, DamageData[WPN_GRENADE].ammo_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, actor, v3df_dontpan);
            KillGetAmmo(actor);
            break;

#if 0
        case ICON_ROCKET:
            pp->WpnAmmo[WPN_ROCKET] += 15;
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, actor, v3df_dontpan);
            KillGet(actor);
            if (pp->WpnFlags & (BIT(WPN_ROCKET)))
                break;
            pp->WpnFlags |= (BIT(WPN_ROCKET));

            if (!cl_weaponswitch)
                break;
            InitWeaponRocket(pp);
            break;

        case ICON_LG_ROCKET:
            sprintf(ds,"20 Missiles");
            PutStringInfo(Player+pnum, ds);
            PlayerUpdateAmmo(pp, WPN_ROCKET, 20);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, actor, v3df_dontpan);
            KillGet(actor);
            break;
#endif

        case ICON_RAIL_GUN:
            if (SW_SHAREWARE) { KillActor(actor); break; }

            if (!CanGetWeapon(pp, actor, WPN_RAIL))
                break;

            pp->WpnGotOnceFlags |= (BIT(WPN_RAIL));

            if (pp->WpnFlags & (BIT(WPN_RAIL)) && pp->WpnAmmo[WPN_RAIL] >= DamageData[WPN_RAIL].max_ammo)
                break;
            //sprintf(ds,"Rail Gun");
            PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_WPNRAILGUN));
            PlayerUpdateAmmo(pp, WPN_RAIL, DamageData[WPN_RAIL].weapon_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, actor, v3df_dontpan);
            if (pp == Player+myconnectindex)
            {
                if (StdRandomRange(1000) > 700)
                    PlayerSound(DIGI_LIKEBIGWEAPONS, v3df_dontpan|v3df_doppler|v3df_follow,pp);
                else
                    PlayerSound(DIGI_GOTRAILGUN, v3df_dontpan|v3df_doppler|v3df_follow,pp);
            }
            //ChoosePlayerGetSound(pp);
            KillGetWeapon(actor);
            if (pp->WpnFlags & (BIT(WPN_RAIL)))
                break;
            pp->WpnFlags |= (BIT(WPN_RAIL));

            if (!cl_weaponswitch)
                break;
            if (plActor->user.WeaponNum > WPN_RAIL && plActor->user.WeaponNum != WPN_SWORD)
                break;
            InitWeaponRail(pp);
            break;

        case ICON_RAIL_AMMO:
            if (SW_SHAREWARE) { KillActor(actor); break; }

            if (pp->WpnAmmo[WPN_RAIL] >= DamageData[WPN_RAIL].max_ammo)
                break;
            //sprintf(ds,"Rail Gun Rods");
            PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_AMMORAILGUN));
            PlayerUpdateAmmo(pp, WPN_RAIL, DamageData[WPN_RAIL].ammo_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, actor, v3df_dontpan);
            KillGetAmmo(actor);
            break;

        case ICON_SHOTGUN:
            if (!CanGetWeapon(pp, actor, WPN_SHOTGUN))
                break;

            pp->WpnGotOnceFlags |= (BIT(WPN_SHOTGUN));

            if (pp->WpnFlags & (BIT(WPN_SHOTGUN)) && pp->WpnAmmo[WPN_SHOTGUN] >= DamageData[WPN_SHOTGUN].max_ammo)
                break;
            //sprintf(ds,"Riot Gun");
            PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_WPNRIOT));
//            pp->WpnAmmo[WPN_SHOTGUN] += 10;
            PlayerUpdateAmmo(pp, WPN_SHOTGUN, DamageData[WPN_SHOTGUN].weapon_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, actor, v3df_dontpan);
            ChoosePlayerGetSound(pp);
            KillGetWeapon(actor);
            if (pp->WpnFlags & (BIT(WPN_SHOTGUN)))
                break;
            pp->WpnFlags |= (BIT(WPN_SHOTGUN));

            if (!cl_weaponswitch)
                break;
            if (plActor->user.WeaponNum > WPN_SHOTGUN && plActor->user.WeaponNum != WPN_SWORD)
                break;
            InitWeaponShotgun(pp);
            break;

        case ICON_LG_SHOTSHELL:
            if (pp->WpnAmmo[WPN_SHOTGUN] >= DamageData[WPN_SHOTGUN].max_ammo)
                break;
            //sprintf(ds,"Shotshells");
            PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_AMMORIOT));
            PlayerUpdateAmmo(pp, WPN_SHOTGUN, DamageData[WPN_SHOTGUN].ammo_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, actor, v3df_dontpan);
            KillGetAmmo(actor);
            break;

#if 0
        case ICON_AUTORIOT:
            if (pp->WpnShotgunAuto != 50)
            {
                sprintf(ds,"Riot Gun TurboDrive, +50 12-Gauge Slugs");
                PutStringInfo(Player+pnum, ds);
                pp->WpnShotgunAuto = 50;
                SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_ITEM, actor, v3df_dontpan);
                KillGet(actor);
                if (pp->CurWpn == pp->Wpn[WPN_SHOTGUN])
                {
                    if (pp->WpnShotgunType != 1)
                    {
                        extern PANEL_STATE ps_ShotgunFlash[];
                        pp->CurWpn->over[SHOTGUN_AUTO_NUM].tics = 0;
                        pp->CurWpn->over[SHOTGUN_AUTO_NUM].State = ps_ShotgunFlash;
                    }

                }
            }
            break;
#endif

        case ICON_GUARD_HEAD:
            if (SW_SHAREWARE) { KillActor(actor); break; }

            if (!CanGetWeapon(pp, actor, WPN_HOTHEAD))
                break;

            pp->WpnGotOnceFlags |= (BIT(WPN_HOTHEAD));

            if (pp->WpnFlags & (BIT(WPN_HOTHEAD)) && pp->WpnAmmo[WPN_HOTHEAD] >= DamageData[WPN_HOTHEAD].max_ammo)
                break;
            //sprintf(ds,"Guardian Head");
            PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_WPNHEAD));
            PlayerUpdateAmmo(pp, WPN_HOTHEAD, DamageData[WPN_HOTHEAD].weapon_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, actor, v3df_dontpan);
            //ChoosePlayerGetSound(pp);
            if (StdRandomRange(1000) > 800 && pp == Player+myconnectindex)
                PlayerSound(DIGI_LIKEBIGWEAPONS, v3df_dontpan|v3df_doppler|v3df_follow,pp);
            KillGetWeapon(actor);
            if (pp->WpnFlags & (BIT(WPN_HOTHEAD)))
                break;
            pp->WpnFlags |= (BIT(WPN_NAPALM) | BIT(WPN_RING) | BIT(WPN_HOTHEAD));

            if (!cl_weaponswitch)
                break;
            if (plActor->user.WeaponNum > WPN_HOTHEAD && plActor->user.WeaponNum != WPN_SWORD)
                break;
            InitWeaponHothead(pp);
            break;

        case ICON_FIREBALL_LG_AMMO:
            if (SW_SHAREWARE) { KillActor(actor); break; }

            if (pp->WpnAmmo[WPN_HOTHEAD] >= DamageData[WPN_HOTHEAD].max_ammo)
                break;
            //sprintf(ds,"Firebursts");
            PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_AMMOHEAD));
            PlayerUpdateAmmo(pp, WPN_HOTHEAD, DamageData[WPN_HOTHEAD].ammo_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, actor, v3df_dontpan);
            KillGetAmmo(actor);
            break;

        case ICON_HEART:
            if (SW_SHAREWARE) { KillActor(actor); break; }

            if (!CanGetWeapon(pp, actor, WPN_HEART))
                break;

            pp->WpnGotOnceFlags |= (BIT(WPN_HEART));

            if (pp->WpnFlags & (BIT(WPN_HEART)) && pp->WpnAmmo[WPN_HEART] >= DamageData[WPN_HEART].max_ammo)
                break;
            //sprintf(ds,"Ripper Heart");
            PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_WPNRIPPER));
            PlayerUpdateAmmo(pp, WPN_HEART, DamageData[WPN_HEART].weapon_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, actor, v3df_dontpan);
            //ChoosePlayerGetSound(pp);
            if (StdRandomRange(1000) > 800 && pp == Player+myconnectindex)
                PlayerSound(DIGI_LIKEBIGWEAPONS, v3df_dontpan|v3df_doppler|v3df_follow,pp);
            KillGetWeapon(actor);
            if (pp->WpnFlags & (BIT(WPN_HEART)))
                break;
            pp->WpnFlags |= (BIT(WPN_HEART));

            if (!cl_weaponswitch)
                break;

            if (plActor->user.WeaponNum > WPN_HEART && plActor->user.WeaponNum != WPN_SWORD)
                break;

            InitWeaponHeart(pp);
            break;

        case ICON_HEART_LG_AMMO:
            if (SW_SHAREWARE) { KillActor(actor); break; }

            if (pp->WpnAmmo[WPN_HEART] >= DamageData[WPN_HEART].max_ammo)
                break;
            //sprintf(ds,"Deathcoils");
            PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_AMMORIPPER));
            PlayerUpdateAmmo(pp, WPN_HEART, DamageData[WPN_HEART].ammo_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, actor, v3df_dontpan);
            KillGetAmmo(actor);
            break;

        case ICON_HEAT_CARD:
            if (pp->WpnRocketHeat != 5)
            {
                //sprintf(ds,"Heat Seeker Card");
                PutStringInfo(Player+pnum, quoteMgr.GetQuote(QUOTE_AMMONUKE));
                pp->WpnRocketHeat = uint8_t(DamageData[DMG_NUCLEAR_EXP].ammo_pickup);
                SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_ITEM, actor, v3df_dontpan);
                KillGet(actor);

                if (pp->CurWpn == pp->Wpn[WPN_MICRO])
                {
                    if (pp->WpnRocketType == 0)
                    {
                        pp->WpnRocketType = 1;
                    }
                    else if (pp->WpnRocketType == 2)
                    {
                        extern PANEL_STATE ps_MicroHeatFlash[];
                        pp->CurWpn->over[MICRO_HEAT_NUM].tics = 0;
                        pp->CurWpn->over[MICRO_HEAT_NUM].State = ps_MicroHeatFlash;
                    }

                }
            }
            break;

        case ICON_FLAG:
        {
            if (actor->spr.pal == pp->actor->spr.pal) break; // Can't pick up your own flag!

            PlaySound(DIGI_ITEM, actor, v3df_dontpan);

            DSWActor* actorNew;
            if (actor->spr.hitag == TAG_NORESPAWN_FLAG)
                actorNew = SpawnActor(STAT_ITEM, ICON_FLAG, s_CarryFlagNoDet, actor->sector(),
                                  actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, 0, 0);
            else
                actorNew = SpawnActor(STAT_ITEM, ICON_FLAG, s_CarryFlag, actor->sector(),
                                  actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, 0, 0);

            actorNew->spr.shade = -20;

            // Attach flag to player
            actorNew->user.Counter = 0;
            actorNew->spr.cstat &= ~(CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
            actorNew->spr.cstat |= (CSTAT_SPRITE_ALIGNMENT_WALL);
            SetAttach(pp->actor, actorNew);
            actorNew->user.pos.Z = ActorZOfMiddle(pp->actor);  // Set mid way up who it hit
            actorNew->user.spal = actorNew->spr.pal = actor->spr.pal;   // Set the palette of the flag

            SetOwner(pp->actor, actorNew);  // Player now owns the flag
            actorNew->user.flagOwnerActor = actor;       // Tell carried flag who owns it
            KillGet(actor);  // Set up for flag respawning
            break;
        }
        default:
            KillActor(actor);
        }
    }

    return 0;
}

// This function mostly only adjust the active_range field

void ProcessActiveVars(DSWActor* actor)
{
    const int TIME_TILL_INACTIVE = (4 * 120);

    if (!(actor->user.Flags & SPR_ACTIVE))
    {
        // if actor has been unaware for more than a few seconds
        actor->user.inactive_time += ACTORMOVETICS;
        if (actor->user.inactive_time > TIME_TILL_INACTIVE)
        {
            // reset to min update range
            actor->user.active_range = MIN_ACTIVE_RANGE;
            // keep time low so it doesn't roll over
            actor->user.inactive_time = TIME_TILL_INACTIVE;
        }
    }

    actor->user.wait_active_check += ACTORMOVETICS;
}

void AdjustActiveRange(PLAYER* pp, DSWActor* actor, int dist)
{
    DSWActor* plActor = pp->actor;
    int look_height;


    // do no FAFcansee before it is time
    if (actor->user.wait_active_check < ACTIVE_CHECK_TIME)
        return;

    actor->user.wait_active_check = 0;

    // check aboslute max
    if (dist > MAX_ACTIVE_RANGE)
        return;

    // do not do a FAFcansee if your already active
    // Actor only becomes INACTIVE in DoActorDecision
    if (actor->user.Flags & (SPR_ACTIVE))
        return;

    //
    // From this point on Actor is INACTIVE
    //

    // if actor can still see the player
    look_height = ActorZOfTop(actor);
    if (FAFcansee(actor->int_pos().X, actor->int_pos().Y, look_height, actor->sector(), plActor->int_pos().X, plActor->int_pos().Y, ActorUpperZ(plActor), plActor->sector()))
    {
        // Player is visible
        // adjust update range of this sprite

        // some huge distance
        actor->user.active_range = 75000;
        // sprite is AWARE
        actor->user.Flags |= (SPR_ACTIVE);
        actor->user.inactive_time = 0;
    }
}


/*

  !AIC KEY - Reads state tables for animation frame transitions and handles
  calling animators, QUICK_CALLS, etc.  This is handled for many types of
  sprites not just actors.

*/

int  StateControl(DSWActor* actor)
{
    short StateTics;

    if (!actor->user.State)
    {
        ASSERT(actor->user.ActorActionFunc);
        (actor->user.ActorActionFunc)(actor);
        return 0;
    }

    if (actor->spr.statnum >= STAT_SKIP4_START && actor->spr.statnum <= STAT_SKIP4_END)
        actor->user.Tics += ACTORMOVETICS * 2;
    else
        actor->user.Tics += ACTORMOVETICS;

    // Skip states if too much time has passed
    while (actor->user.Tics >= (actor->user.State->Tics & SF_TICS_MASK))
    {
        StateTics = (actor->user.State->Tics & SF_TICS_MASK);

        if ((actor->user.State->Tics & SF_TIC_ADJUST))
        {
            ASSERT(actor->user.Attrib);
            ASSERT(actor->user.speed < MAX_SPEED);
            ASSERT(StateTics > -actor->user.Attrib->TicAdjust[actor->user.speed]);

            StateTics += actor->user.Attrib->TicAdjust[actor->user.speed];
        }

        // Set Tics
        actor->user.Tics -= StateTics;

        // Transition to the next state
        actor->user.State = actor->user.State->NextState;

        // Look for flags embedded into the Tics variable
        while ((actor->user.State->Tics & SF_QUICK_CALL))
        {
            // Call it once and go to the next state
            (*actor->user.State->Animator)(actor);

            ASSERT(actor->hasU()); //put this in to see if actor was getting killed with in his QUICK_CALL state

            if (!actor->hasU())
                break;

            // if still on the same QUICK_CALL should you
            // go to the next state.
            if ((actor->user.State->Tics & SF_QUICK_CALL))
                actor->user.State = actor->user.State->NextState;
        }

        if (!actor->hasU())
            break;

        if (!actor->user.State->Pic)
        {
            NewStateGroup(actor, (STATE* *) actor->user.State->NextState);
        }
    }

    if (actor->hasU())
    {
        ASSERT(actor->user.State);
        // Set picnum to the correct pic
        if ((actor->user.State->Tics & SF_WALL_STATE))
        {
            ASSERT(actor->user.WallP);
            actor->user.WallP->picnum = actor->user.State->Pic;
        }
        else
        {
            if (actor->user.RotNum > 1)
                actor->spr.picnum = actor->user.Rot[0]->Pic;
            else
                actor->spr.picnum = actor->user.State->Pic;
        }

        // Call the correct animator
        if (actor->user.State->Animator && actor->user.State->Animator != NullAnimator)
            (*actor->user.State->Animator)(actor);
    }

    return 0;
}



void SpriteControl(void)
{
    int32_t stat;
    short pnum, CloseToPlayer;
    PLAYER* pp;
    int tx, ty, tmin, dist;
    short StateTics;

    SWStatIterator it(STAT_MISC);
    while (auto actor = it.Next())
    {
        StateControl(actor);
    }

    // Items and skip2 things
    if (MoveSkip2 == 0)
    {
        for (stat = STAT_SKIP2_START + 1; stat <= STAT_SKIP2_END; stat++)
        {
            it.Reset(stat);
            while (auto actor = it.Next())
            {
                StateControl(actor);
            }
        }
    }

    if (MoveSkip2 == 0)                 // limit to 20 times a second
    {
        // move bad guys around
        it.Reset(STAT_ENEMY);
        while (auto actor = it.Next())
        {
            if (!actor->hasU()) continue;

            CloseToPlayer = false;

            ProcessActiveVars(actor);

            TRAVERSE_CONNECT(pnum)
            {
                pp = &Player[pnum];

                // Only update the ones closest
                DISTANCE(pp->pos.X, pp->pos.Y, actor->int_pos().X, actor->int_pos().Y, dist, tx, ty, tmin);

                AdjustActiveRange(pp, actor, dist);

                if (dist < actor->user.active_range)
                {
                    CloseToPlayer = true;
                }
            }

            actor->user.Flags &= ~(SPR_MOVED);

            // Only update the ones close to ANY player
            if (CloseToPlayer)
            {
                StateControl(actor);
            }
            else
            {
                // to far away to be attacked
                actor->user.Flags &= ~(SPR_ATTACKED);
            }
        }
    }

    // Skip4 things
    if (MoveSkip4 == 0)                 // limit to 10 times a second
    {
        for (stat = STAT_SKIP4_START; stat <= STAT_SKIP4_END; stat++)
        {
            it.Reset(stat);
            while (auto actor = it.Next())
            {
				StateControl(actor);
            }
        }
    }

    it.Reset(STAT_NO_STATE);
    while (auto actor = it.Next())
    {
        if (actor->hasU() && actor->user.ActorActionFunc)
            actor->user.ActorActionFunc(actor);
    }

    if (MoveSkip8 == 0)
    {
        it.Reset(STAT_STATIC_FIRE);
        while (auto itActor = it.Next())
        {
            DoStaticFlamesDamage(itActor);
        }
    }

    if (MoveSkip4 == 0)                 // limit to 10 times a second
    {
        it.Reset(STAT_WALLBLOOD_QUEUE);
        while (auto actor = it.Next())
        {
			StateControl(actor);
        }
    }

    // vator/rotator/spike/slidor all have some code to
    // prevent calling of the action func()
    it.Reset(STAT_VATOR);
    while (auto actor = it.Next())
    {
        if (!actor->hasU())
            continue;
        if (actor->user.Tics)
        {
            if ((actor->user.Tics -= synctics) <= 0)
                SetVatorActive(actor);
            else
                continue;
        }

        if (!(actor->user.Flags & SPR_ACTIVE))
            continue;

        actor->user.ActorActionFunc(actor);
    }

    it.Reset(STAT_SPIKE);
    while (auto actor = it.Next())
    {
        if (actor->user.Tics)
        {
            if ((actor->user.Tics -= synctics) <= 0)
                SetSpikeActive(actor);
            else
                continue;
        }

        if (!(actor->user.Flags & SPR_ACTIVE))
            continue;

        actor->user.ActorActionFunc(actor);
    }

    it.Reset(STAT_ROTATOR);
    while (auto actor = it.Next())
    {
        if (actor->user.Tics)
        {
            if ((actor->user.Tics -= synctics) <= 0)
                SetRotatorActive(actor);
            else
                continue;
        }

        if (!(actor->user.Flags & SPR_ACTIVE))
            continue;

        actor->user.ActorActionFunc(actor);
    }

    it.Reset(STAT_SLIDOR);
    while (auto actor = it.Next())
    {
        if (actor->user.Tics)
        {
            if ((actor->user.Tics -= synctics) <= 0)
                SetSlidorActive(actor);
            else
                continue;
        }

        if (!(actor->user.Flags & SPR_ACTIVE))
            continue;

        actor->user.ActorActionFunc(actor);
    }

    it.Reset(STAT_SUICIDE);
    while (auto actor = it.Next())
    {
        KillActor(actor);
    }
}


//
// This moves an actor about with FAFgetzrange clip adjustment
//

/*

  !AIC KEY - calls clipmove - Look through and try to understatnd  Should
  hopefully never have to change this.  Its very delicate.

*/

Collision move_sprite(DSWActor* actor, int xchange, int ychange, int zchange, int ceildist, int flordist, uint32_t cliptype, int numtics)
{
    Collision retval{};
    int zh;
	short tempshort;

    ASSERT(actor->hasU());

    vec3_t clippos = actor->int_pos();

    // Can't modify sprite sectors
    // directly becuase of linked lists
    auto dasect = actor->sector();
    auto lastsect = dasect;

    if (actor->spr.cstat & (CSTAT_SPRITE_YCENTER))
    {
        zh = 0;
    }
    else
    {
        // move the center point up for moving
        zh = actor->user.zclip;
        clippos.Z -= zh;
    }


//    ASSERT(inside(actor->spr.x,actor->spr.y,dasectnum));

    clipmove(clippos, &dasect,
                      ((xchange * numtics) << 11), ((ychange * numtics) << 11),
                      (((int) actor->spr.clipdist) << 2), ceildist, flordist, cliptype, retval, 1);


    actor->set_int_xy(clippos.X, clippos.Y);

    if (dasect == nullptr)
    {
        retval.setWall(0); // this is wrong but what the original code did.
        return retval;
    }

    if ((dasect != actor->sector()) && (dasect != nullptr))
        ChangeActorSect(actor, dasect);

    // Set the blocking bit to 0 temporarly so FAFgetzrange doesn't pick
    // up its own sprite
    auto tempstat = actor->spr.cstat;
    actor->spr.cstat = 0;

    // I subtracted 8 from the clipdist because actors kept going up on
    // ledges they were not supposed to go up on.  Did the same for the
    // player. Seems to work ok!
    vec3_t pos = actor->int_pos();
    pos.Z -= zh + 1;
    FAFgetzrange(pos, actor->sector(),
                 &globhiz, &globhihit, &globloz, &globlohit,
                 (((int) actor->spr.clipdist) << 2) - GETZRANGE_CLIP_ADJ, cliptype);

    actor->spr.cstat = tempstat;

    // !AIC - puts getzrange results into USER varaible actor->user.loz, actor->user.hiz, actor->user.lo_sectp, actor->user.hi_sectp, etc.
    // Takes info from global variables
    DoActorGlobZ(actor);

    clippos.Z = actor->int_pos().Z + ((zchange * numtics) >> 3);

    // test for hitting ceiling or floor
    if ((clippos.Z - zh <= globhiz) || (clippos.Z - zh > globloz))
    {
        if (retval.type == kHitNone)
        {
            if (actor->user.Flags & (SPR_CLIMBING))
            {
                actor->set_int_z(clippos.Z);
                return retval;
            }

            retval.setSector(dasect);
        }
    }
    else
    {
        actor->set_int_z(clippos.Z);
    }

    // extra processing for Stacks and warping
    if (FAF_ConnectArea(actor->sector()))
        SetActorZ(actor, actor->int_pos());

    if ((actor->sector()->extra & SECTFX_WARP_SECTOR))
    {
        DSWActor* sp_warp;
        pos = actor->int_pos();
        if ((sp_warp = WarpPlane(&pos.X, &pos.Y, &pos.Z, &dasect)))
        {
            actor->set_int_pos(pos);
            ActorWarpUpdatePos(actor, dasect);
            ActorWarpType(actor, sp_warp);
        }

        if (actor->sector() != lastsect)
        {
            pos = actor->int_pos();
            if ((sp_warp = Warp(&pos.X, &pos.Y, &pos.Z, &dasect)))
            {
                actor->set_int_pos(pos);
                ActorWarpUpdatePos(actor, dasect);
                ActorWarpType(actor, sp_warp);
            }
        }
    }

    return retval;
}

void MissileWarpUpdatePos(DSWActor* actor, sectortype* sect)
{
    actor->backuppos();
    actor->user.oz = actor->opos.Z;
    ChangeActorSect(actor, sect);
    MissileZrange(actor);
}

void ActorWarpUpdatePos(DSWActor* actor, sectortype* sect)
{
    actor->backuppos();
    actor->user.oz = actor->opos.Z;
    ChangeActorSect(actor, sect);
    DoActorZrange(actor);
}

void MissileWarpType(DSWActor* actor, DSWActor* act_warp)
{
    switch (SP_TAG1(act_warp))
    {
    case WARP_CEILING_PLANE:
    case WARP_FLOOR_PLANE:
        return;
    }

    switch (SP_TAG3(act_warp))
    {
    case 1:
        break;
    default:
        PlaySound(DIGI_ITEM_SPAWN, actor, v3df_none);
        DoSpawnItemTeleporterEffect(actor);
        break;
    }
}

void ActorWarpType(DSWActor* actor, DSWActor* act_warp)
{
    switch (SP_TAG3(act_warp))
    {
    case 1:
        break;
    default:
        PlaySound(DIGI_ITEM_SPAWN, actor, v3df_none);
        DoSpawnTeleporterEffectPlace(actor);
        break;
    }
}

//
// This moves a small projectile with FAFgetzrangepoint
//

int MissileWaterAdjust(DSWActor* actor)
{
    auto sectp = actor->user.lo_sectp;
    if (sectp && sectp->hasU())
    {
        if (FixedToInt(sectp->depth_fixed))
            actor->user.loz -= Z(FixedToInt(sectp->depth_fixed));
    }
    return 0;
}

int MissileZrange(DSWActor* actor)
{
    // Set the blocking bit to 0 temporarly so FAFgetzrange doesn't pick
    // up its own sprite
    auto tempshort = actor->spr.cstat;
    actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK);

    FAFgetzrangepoint(actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z - 1, actor->sector(),
                      &globhiz, &globhihit, &globloz, &globlohit);

    actor->spr.cstat = tempshort;

    DoActorGlobZ(actor);
    return 0;
}


Collision move_missile(DSWActor* actor, int xchange, int ychange, int zchange, int ceildist, int flordist, uint32_t cliptype, int numtics)
{
    Collision retval{};
    int zh;

    ASSERT(actor->hasU());

    vec3_t clippos = actor->int_pos();

    // Can't modify sprite sectors
    // directly becuase of linked lists
    auto dasect = actor->sector();
    auto lastsect = dasect;

    if (actor->spr.cstat & (CSTAT_SPRITE_YCENTER))
    {
        zh = 0;
    }
    else
    {
        zh = actor->user.zclip;
        clippos.Z -= zh;
    }


    clipmove(clippos, &dasect,
                      ((xchange * numtics) << 11), ((ychange * numtics) << 11),
                      (((int) actor->spr.clipdist) << 2), ceildist, flordist, cliptype, retval, 1);
    actor->set_int_xy(clippos.X, clippos.Y);

    if (dasect == nullptr)
    {
        // we've gone beyond a white wall - kill it
        retval.setVoid();
        return retval;
    }

    if ((dasect != actor->sector()) && (dasect != nullptr))
        ChangeActorSect(actor, dasect);

    // Set the blocking bit to 0 temporarly so FAFgetzrange doesn't pick
    // up its own sprite
    auto tempshort = actor->spr.cstat;
    actor->spr.cstat &= ~(CSTAT_SPRITE_BLOCK);

    FAFgetzrangepoint(actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z - 1, actor->sector(),
                      &globhiz, &globhihit, &globloz, &globlohit);

    actor->spr.cstat = tempshort;

    DoActorGlobZ(actor);

    // getzrangepoint moves water down
    // missiles don't need the water to be down
    MissileWaterAdjust(actor);

    clippos.Z = actor->int_pos().Z + ((zchange * numtics) >> 3);

    // NOTE: this does not tell you when you hit a floor sprite
    // this case is currently treated like it hit a sector

    // test for hitting ceiling or floor
    if (clippos.Z - zh <= actor->user.hiz + ceildist)
    {
        // normal code
        actor->set_int_z(actor->user.hiz + zh + ceildist);
        if (retval.type == kHitNone)
            retval.setSector(dasect);
    }
    else if (clippos.Z - zh > actor->user.loz - flordist)
    {
        actor->set_int_z(actor->user.loz + zh - flordist);
        if (retval.type == kHitNone)
            retval.setSector(dasect);
    }
    else
    {
        actor->set_int_z(clippos.Z);
    }

    if (FAF_ConnectArea(actor->sector()))
        SetActorZ(actor, actor->int_pos());

    if ((actor->sector()->extra & SECTFX_WARP_SECTOR))
    {
        DSWActor* sp_warp;

        auto pos = actor->int_pos();
        if ((sp_warp = WarpPlane(&pos.X, &pos.Y, &pos.Z, &dasect)))
        {
            actor->set_int_pos(pos);
            MissileWarpUpdatePos(actor, dasect);
            MissileWarpType(actor, sp_warp);
        }

        if (actor->sector() != lastsect)
        {
            pos = actor->int_pos();
            if ((sp_warp = Warp(&pos.X, &pos.Y, &pos.Z, &dasect)))
            {
                actor->set_int_pos(pos);
                MissileWarpUpdatePos(actor, dasect);
                MissileWarpType(actor, sp_warp);
            }
        }
    }

    if (retval.type != kHitNone && (actor->sector()->ceilingstat & CSTAT_SECTOR_SKY))
    {
        if (actor->int_pos().Z < actor->sector()->int_ceilingz())
        {
            retval.setVoid();
        }
    }

    if (retval.type != kHitNone && (actor->sector()->floorstat & CSTAT_SECTOR_SKY))
    {
        if (actor->int_pos().Z > actor->sector()->int_floorz())
        {
            retval.setVoid();
        }
    }

    return retval;
}


Collision move_ground_missile(DSWActor* actor, int xchange, int ychange, int ceildist, int flordist, uint32_t cliptype, int numtics)
{
    int daz;
    Collision retval{};
    int ox,oy;

    ASSERT(actor->hasU());

    // Can't modify sprite sectors
    // directly becuase of linked lists
    auto dasect = actor->sector();
    auto lastsect = dasect;

    vec3_t opos = actor->int_pos();
    daz = actor->int_pos().Z;

    // climbing a wall
    if (actor->user.z_tgt)
    {
        if (labs(actor->user.z_tgt - actor->int_pos().Z) > Z(40))
        {
            if (actor->user.z_tgt > actor->int_pos().Z)
            {
                actor->add_int_z(Z(30));
                return retval;
            }
            else
            {
                actor->add_int_z(-Z(30));
                return retval;
            }
        }
        else
            actor->user.z_tgt = 0;
    }

    actor->add_int_pos({ xchange / 2, ychange / 2, 0 });

    updatesector(actor->int_pos().X, actor->int_pos().Y, &dasect);

    if (dasect == nullptr)
    {
        // back up and try again
        dasect = actor->sector();
        lastsect = dasect;
        opos = actor->int_pos();
        opos.Z = daz;
        clipmove(opos, &dasect,
                          ((xchange * numtics) << 11), ((ychange * numtics) << 11),
                          (((int) actor->spr.clipdist) << 2), ceildist, flordist, cliptype, retval, 1);
        actor->set_int_xy(opos.X, opos.Y);
    }

    if (dasect == nullptr)
    {
        // we've gone beyond a white wall - kill it
        retval.setVoid();
        return retval;
    }


    if (retval.type != kHitNone)  // ran into a white wall
    {
            return retval;
        }


    actor->user.z_tgt = 0;
    if ((dasect != actor->sector()) && (dasect != nullptr))
    {
        int new_loz,new_hiz;
        getzsofslopeptr(dasect, actor->int_pos().X, actor->int_pos().Y, &new_hiz, &new_loz);

        actor->set_int_z(new_loz);
        ChangeActorSect(actor, dasect);
    }

    getzsofslopeptr(actor->sector(), actor->int_pos().X, actor->int_pos().Y, &actor->user.hiz, &actor->user.loz);

    actor->user.hi_sectp = actor->user.lo_sectp = actor->sector();
    actor->user.highActor = nullptr; actor->user.lowActor = nullptr;
    actor->set_int_z(actor->user.loz - Z(8));

    if (labs(actor->user.hiz - actor->user.loz) < Z(12))
    {
        // we've gone into a very small place - kill it
        retval.setVoid();
        return retval;
    }

    if ((actor->sector()->extra & SECTFX_WARP_SECTOR))
    {
        DSWActor* sp_warp;

        auto pos = actor->int_pos();
        if ((sp_warp = WarpPlane(&pos.X, &pos.Y, &pos.Z, &dasect)))
        {
            actor->set_int_pos(pos);
            MissileWarpUpdatePos(actor, dasect);
            MissileWarpType(actor, sp_warp);
        }

        if (actor->sector() != lastsect)
        {
            pos = actor->int_pos();
            if ((sp_warp = Warp(&pos.X, &pos.Y, &pos.Z, &dasect)))
            {
                actor->set_int_pos(pos);
                MissileWarpUpdatePos(actor, dasect);
                MissileWarpType(actor, sp_warp);
            }
        }
    }

    return retval;
}

#include "saveable.h"

static saveable_code saveable_sprite_code[] =
{
    SAVE_CODE(DoGrating),
    SAVE_CODE(DoKey),
    SAVE_CODE(DoCoin),
    SAVE_CODE(DoGet),
};

static saveable_data saveable_sprite_data[] =
{
    SAVE_DATA(Track),
    SAVE_DATA(SectorObject),

    SAVE_DATA(s_DebrisNinja),
    SAVE_DATA(s_DebrisRat),
    SAVE_DATA(s_DebrisCrab),
    SAVE_DATA(s_DebrisStarFish),
    SAVE_DATA(s_RepairKit),
    SAVE_DATA(s_GoldSkelKey),
    SAVE_DATA(s_BlueKey),
    SAVE_DATA(s_BlueCard),
    SAVE_DATA(s_SilverSkelKey),
    SAVE_DATA(s_RedKey),
    SAVE_DATA(s_RedCard),
    SAVE_DATA(s_BronzeSkelKey),
    SAVE_DATA(s_GreenKey),
    SAVE_DATA(s_GreenCard),
    SAVE_DATA(s_RedSkelKey),
    SAVE_DATA(s_YellowKey),
    SAVE_DATA(s_YellowCard),
    SAVE_DATA(s_Key),
    SAVE_DATA(s_BlueKey),
    SAVE_DATA(s_RedKey),
    SAVE_DATA(s_GreenKey),
    SAVE_DATA(s_YellowKey),
    SAVE_DATA(s_Key),
    /*
    SAVE_DATA(s_BlueKeyStatue),
    SAVE_DATA(s_RedKeyStatue),
    SAVE_DATA(s_GreenKeyStatue),
    SAVE_DATA(s_YellowKeyStatue),
    SAVE_DATA(s_KeyStatue),
    */
    SAVE_DATA(s_RedCoin),
    SAVE_DATA(s_YellowCoin),
    SAVE_DATA(s_GreenCoin),
    SAVE_DATA(s_FireFly),
    SAVE_DATA(s_IconStar),
    SAVE_DATA(s_IconUzi),
    SAVE_DATA(s_IconLgUziAmmo),
    SAVE_DATA(s_IconUziFloor),
    SAVE_DATA(s_IconRocket),
    SAVE_DATA(s_IconLgRocket),
    SAVE_DATA(s_IconShotgun),
    SAVE_DATA(s_IconLgShotshell),
    SAVE_DATA(s_IconAutoRiot),
    SAVE_DATA(s_IconGrenadeLauncher),
    SAVE_DATA(s_IconLgGrenade),
    SAVE_DATA(s_IconLgMine),
    SAVE_DATA(s_IconGuardHead),
    SAVE_DATA(s_IconFireballLgAmmo),
    SAVE_DATA(s_IconHeart),
    SAVE_DATA(s_IconHeartLgAmmo),
    SAVE_DATA(s_IconMicroGun),
    SAVE_DATA(s_IconMicroBattery),
    SAVE_DATA(s_IconRailGun),
    SAVE_DATA(s_IconRailAmmo),
    SAVE_DATA(s_IconElectro),
    SAVE_DATA(s_IconSpell),
    SAVE_DATA(s_IconArmor),
    SAVE_DATA(s_IconMedkit),
    SAVE_DATA(s_IconChemBomb),
    SAVE_DATA(s_IconFlashBomb),
    SAVE_DATA(s_IconNuke),
    SAVE_DATA(s_IconCaltrops),
    SAVE_DATA(s_IconSmMedkit),
    SAVE_DATA(s_IconBooster),
    SAVE_DATA(s_IconHeatCard),
    //SAVE_DATA(s_IconEnvironSuit),
    SAVE_DATA(s_IconCloak),
    SAVE_DATA(s_IconFly),
    SAVE_DATA(s_IconNightVision),
    SAVE_DATA(s_IconFlag),
};

saveable_module saveable_sprite =
{
    // code
    saveable_sprite_code,
    SIZ(saveable_sprite_code),

    // data
    saveable_sprite_data,
    SIZ(saveable_sprite_data)
};

END_SW_NS
