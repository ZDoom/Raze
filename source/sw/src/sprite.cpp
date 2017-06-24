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
#include "build.h"

#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "ai.h"
#include "warp.h"
#include "light.h"
#include "break.h"
#include "net.h"

#include "pal.h"

#include "sounds.h"
#include "interp.h"
#include "sprite.h"
#include "weapon.h"
#include "jsector.h"
#include "text.h"
#include "slidor.h"
#include "player.h"


SWBOOL FAF_Sector(short sectnum);
SWBOOL MoveSkip4, MoveSkip2, MoveSkip8;

extern STATE s_CarryFlag[];
extern STATE s_CarryFlagNoDet[];

static int globhiz, globloz, globhihit, globlohit;
short wait_active_check_offset;
int PlaxCeilGlobZadjust, PlaxFloorGlobZadjust;
void SetSectorWallBits(short sectnum, int bit_mask, SWBOOL set_sectwall, SWBOOL set_nextwall);
int DoActorDebris(short SpriteNum);
void ActorWarpUpdatePos(short SpriteNum,short sectnum);
void ActorWarpType(SPRITEp sp, SPRITEp sp_warp);
int MissileZrange(short SpriteNum);

#define ACTIVE_CHECK_TIME (3*120)

/*
short GetDeltaAngle(short ang1, short ang2);
short GetRotation(short sn);
int StateControl(int16_t SpriteNum);
void PreCacheRange(short, short);
*/

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

extern SWBOOL DebugActor;
extern int score;

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

STATEp s_Key[] =
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

STATEp s_Key[] =
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

STATEp s_KeyStatue[] =
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

void
SetOwner(short owner, short child)
{
    SPRITEp op;
    SPRITEp cp = &sprite[child];

    if (owner == 0)
    {
        ////DSPRINTF(ds, "Set Owner possible problem - owner is 0, child %d", child);
        //MONO_PRINT(ds);
    }

    if (owner >= 0)
    {
        op = &sprite[owner];
        ASSERT(User[owner]);
        SET(User[owner]->Flags2, SPR2_CHILDREN);
    }
    else
    {
        ////DSPRINTF(ds,"Owner is -1 !!!!!!!!!!!!!!!!!!!!!");
        //MONO_PRINT(ds);
    }

    cp->owner = owner;
}

void
SetAttach(short owner, short child)
{
    SPRITEp op = &sprite[owner];
    SPRITEp cp = &sprite[child];
    USERp cu = User[child];

    ASSERT(cu);

    ASSERT(User[owner]);
    SET(User[owner]->Flags2, SPR2_CHILDREN);
    cu->Attach = owner;
}

void
KillSprite(int16_t SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    short i,nexti;
    unsigned stat;
    short statnum,sectnum;
    //extern short Zombies;

    ASSERT(!Prediction);

    ASSERT(sp->statnum < MAXSTATUS);

    //////////////////////////////////////////////
    //    Check sounds list to kill attached sounds
    DeleteNoSoundOwner(SpriteNum);
    DeleteNoFollowSoundOwner(SpriteNum);
    //////////////////////////////////////////////

    if (u)
    {
        PLAYERp pp;
        short pnum;

        if (u->WallShade)
        {
            FreeMem(u->WallShade);
            u->WallShade = NULL;
        }

        // doing a MissileSetPos - don't allow killing
        if (TEST(u->Flags, SPR_SET_POS_DONT_KILL))
            return;

        // for attached sprites that are getable make sure they don't have
        // any Anims attached
        AnimDelete(&u->sz);
        AnimDelete(&sp->z);
        stopinterpolation(&sp->x);
        stopinterpolation(&sp->y);
        stopinterpolation(&sp->z);

        //if (TEST(u->Flags2, SPR2_DONT_TARGET_OWNER))
        //    Zombies--;

        // adjust sprites attached to sector objects
        if (TEST(u->Flags, SPR_SO_ATTACHED))
        {
            SECTOR_OBJECTp sop;
            short sn, FoundSpriteNdx = -1;

            for (sop = SectorObject; sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++)
            {
                for (sn = 0; sop->sp_num[sn] != -1; sn++)
                {
                    if (sop->sp_num[sn] == SpriteNum)
                    {
                        FoundSpriteNdx = sn;
                    }
                }

                if (FoundSpriteNdx >= 0)
                {
                    // back up sn so it points to the last valid sprite num
                    sn--;
                    ASSERT(sop->sp_num[sn] >= 0);

                    // replace the one to be deleted with the last ndx
                    sop->sp_num[FoundSpriteNdx] = sop->sp_num[sn];
                    // the last ndx is not -1
                    sop->sp_num[sn] = -1;

                    break;
                }
            }

            ASSERT(FoundSpriteNdx >= 0);
        }

        // if a player is dead and watching this sprite
        // reset it.
        TRAVERSE_CONNECT(pnum)
        {
            pp = Player + pnum;

            if (pp->Killer > -1)
            {
                if (pp->Killer == SpriteNum)
                {
                    pp->Killer = -1;
                }
            }
        }

        // if on a track and died reset the track to non-occupied
        if (sp->statnum == STAT_ENEMY)
        {
            if (u->track != -1)
            {
                if (Track[u->track].flags)
                    RESET(Track[u->track].flags, TF_TRACK_OCCUPIED);
            }
        }

        // if missile is heading for the sprite, the missile need to know
        // that it is already dead
        if (TEST(sp->extra, SPRX_PLAYER_OR_ENEMY))
        {
            USERp mu;
            static short MissileStats[] = {STAT_MISSILE, STAT_MISSILE_SKIP4};

            for (stat = 0; stat < SIZ(MissileStats); stat++)
            {
                TRAVERSE_SPRITE_STAT(headspritestat[MissileStats[stat]], i, nexti)
                {
                    mu = User[i];

                    if (mu->WpnGoal == SpriteNum)
                    {
                        mu->WpnGoal = -1;
                    }
                }
            }
        }

        // much faster
        if (TEST(u->Flags2, SPR2_CHILDREN))
        //if (TEST(sp->extra, SPRX_CHILDREN))
        {
            // check for children and allert them that the owner is dead
            // don't bother th check if you've never had children
            for (stat = 0; stat < STAT_DONT_DRAW; stat++)
            {
                TRAVERSE_SPRITE_STAT(headspritestat[stat], i, nexti)
                {
                    if (sprite[i].owner == SpriteNum)
                    {
                        sprite[i].owner = -1;
                    }

                    if (User[i] && User[i]->Attach == SpriteNum)
                    {
                        User[i]->Attach = -1;
                    }
                }
            }
        }

        if (sp->statnum == STAT_ENEMY)
        {
            TRAVERSE_SPRITE_STAT(headspritestat[STAT_ENEMY], i, nexti)
            {
                if (User[i]->tgt_sp == sp)
                {
                    DoActorPickClosePlayer(i);
                }
            }
        }

        if (u->flame >= 0)
        {
            SetSuicide(u->flame);
        }

        if (u->rotator)
        {
            if (u->rotator->origx)
                FreeMem(u->rotator->origx);
            if (u->rotator->origy)
                FreeMem(u->rotator->origy);
            FreeMem(u->rotator);
        }

        FreeMem(User[SpriteNum]);
        User[SpriteNum] = 0;
    }

    deletesprite(SpriteNum);
    // shred your garbage - but not statnum
    statnum = sp->statnum;
    sectnum = sp->sectnum;
    memset(sp, 0xCC, sizeof(SPRITE));
    sp->statnum = statnum;
    sp->sectnum = sectnum;
}

void ChangeState(short SpriteNum, STATEp statep)
{
    USERp u = User[SpriteNum];

    u->Tics = 0;
    u->State = u->StateStart = statep;
    // Just in case
    PicAnimOff(u->State->Pic);
}

void
change_sprite_stat(short SpriteNum, short stat)
{
    USERp u = User[SpriteNum];

    changespritestat(SpriteNum, stat);

    if (u)
    {
        RESET(u->Flags, SPR_SKIP2|SPR_SKIP4);

        if (stat >= STAT_SKIP4_START && stat <= STAT_SKIP4_END)
            SET(u->Flags, SPR_SKIP4);

        if (stat >= STAT_SKIP2_START && stat <= STAT_SKIP2_END)
            SET(u->Flags, SPR_SKIP2);

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
            u->wait_active_check = wait_active_check_offset;
            // don't do a break here
            SET(u->Flags, SPR_SHADOW);
            break;
        }

    }
}

USERp
SpawnUser(short SpriteNum, short id, STATEp state)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u;

    ASSERT(!Prediction);

    User[SpriteNum] = u = (USERp) CallocMem(sizeof(USER), 1);

    PRODUCTION_ASSERT(u != NULL);

    // be careful State can be NULL
    u->State = u->StateStart = state;

    change_sprite_stat(SpriteNum, sp->statnum);

    u->ID = id;
    u->Health = 100;
    u->WpnGoal = -1;                     // for weapons
    u->Attach = -1;
    u->track = -1;
    u->tgt_sp = Player[0].SpriteP;
    u->Radius = 220;
    u->Sibling = -1;
    u->flame = -1;
    u->SpriteP = &sprite[SpriteNum];
    u->SpriteNum = SpriteNum;
    u->WaitTics = 0;
    u->OverlapZ = Z(4);
    u->WallShade = NULL;
    u->rotator = NULL;
    u->bounce = 0;

    u->motion_blur_num = 0;
    u->motion_blur_dist = 256;

    u->ox = sp->x;
    u->oy = sp->y;
    u->oz = sp->z;

    u->active_range = MIN_ACTIVE_RANGE;

    // default

    // based on clipmove z of 48 pixels off the floor
    u->floor_dist = Z(48) - Z(28);
    u->ceiling_dist = Z(8);

    // Problem with sprites spawned really close to white sector walls
    // cant do a getzrange there
    // Just put in some valid starting values
#if 0
    DoActorZrange(SpriteNum);
#else
    u->loz = sector[sp->sectnum].floorz;
    u->hiz = sector[sp->sectnum].ceilingz;
    u->lo_sp = NULL;
    u->hi_sp = NULL;
    u->lo_sectp = &sector[sp->sectnum];
    u->hi_sectp = &sector[sp->sectnum];
#endif

    return u;
}

SECT_USERp
GetSectUser(short sectnum)
{
    SECT_USERp sectu;

    if (SectUser[sectnum])
        return SectUser[sectnum];

    sectu = SectUser[sectnum] = (SECT_USERp) CallocMem(sizeof(SECT_USER), 1);

    ASSERT(sectu != NULL);

    return sectu;
}


int16_t
SpawnSprite(short stat, short id, STATEp state, short sectnum, int x, int y, int z, int init_ang, int vel)
{
    SPRITEp sp;
    int16_t SpriteNum;
    USERp u;

    ASSERT(!Prediction);

    PRODUCTION_ASSERT(sectnum >= 0 && sectnum < MAXSECTORS);

    SpriteNum = COVERinsertsprite(sectnum, stat);

    ASSERT(SpriteNum >= 0 && SpriteNum <= MAXSPRITES);

    sp = &sprite[SpriteNum];

    sp->pal = 0;
    sp->x = x;
    sp->y = y;
    sp->z = z;
    sp->cstat = 0;

    User[SpriteNum] = u = SpawnUser(SpriteNum, id, state);

    // be careful State can be NULL
    if (u->State)
    {
        sp->picnum = u->State->Pic;
        PicAnimOff(sp->picnum);
    }

    sp->shade = 0;
    sp->xrepeat = 64;
    sp->yrepeat = 64;
    sp->ang = NORM_ANGLE(init_ang);

    sp->xvel = vel;
    sp->zvel = 0;
    sp->owner = -1;
    sp->lotag = 0L;
    sp->hitag = 0L;
    sp->extra = 0;
    sp->xoffset = 0;
    sp->yoffset = 0;
    sp->clipdist = 0;

    return SpriteNum;
}

void
PicAnimOff(short picnum)
{
    int i;
    short anim_type = TEST(picanm[picnum].sf, PICANM_ANIMTYPE_MASK) >> PICANM_ANIMTYPE_SHIFT;
    short num;

    ASSERT(picnum >= 0 && picnum < MAXTILES);

    if (!anim_type)
        return;

    /*
    num = picanm[picnum].num;
    ASSERT(num < 20);

    for (i = 0; i < num; i++)
    {
    RESET(picanm[picnum + i].sf, PICANM_ANIMTYPE_MASK);
    }
    */

    RESET(picanm[picnum].sf, PICANM_ANIMTYPE_MASK);
}

SWBOOL
IconSpawn(SPRITEp sp)
{
    // if multi item and not a modem game
    if (TEST(sp->extra, SPRX_MULTI_ITEM))
    {
        if (numplayers <= 1 || gNet.MultiGameType == MULTI_GAME_COOPERATIVE)
            return FALSE;
    }

    return TRUE;
}

SWBOOL
ActorTestSpawn(SPRITEp sp)
{
    if (sp->statnum == STAT_DEFAULT && sp->lotag == TAG_SPAWN_ACTOR)
    {
        short New;
        short SpriteNum = sp - sprite;
        New = COVERinsertsprite(sp->sectnum, STAT_DEFAULT);
        memcpy(&sprite[New], sp, sizeof(SPRITE));
        change_sprite_stat(New, STAT_SPAWN_TRIGGER);
        RESET(sprite[New].cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
        return FALSE;
    }

#if 1
    if (DebugActor)
        return FALSE;
#else
    if (DebugActor && !TEST(sp->extra, SPRX_BLOCK))
    {
        RESET(sp->extra, SPRX_BLOCK);  //reset so it can be used elsewhere
        return FALSE;
    }
#endif

    // Skill ranges from -1 (No Monsters) to 3
    if (TEST(sp->extra, SPRX_SKILL) > Skill)
    {

        // JBF: hack to fix Wanton Destruction's missing Sumos, Serpents, and Zilla on Skill < 2
        if (((sp->picnum == SERP_RUN_R0 || sp->picnum == SUMO_RUN_R0) && sp->lotag > 0 &&
             sp->lotag != TAG_SPAWN_ACTOR && sp->extra > 0) || sp->picnum == ZILLA_RUN_R0)
        {
            const char *c;

            // NOTE: Wanton's $boat.map has two sumos, neither of which actually activate
            // anything but are spawned in, and one of them has a skill level 2 mask. This
            // hack however forces both sumos to appear on the easy levels. Bummer.
            switch (sp->picnum)
            {
            case SERP_RUN_R0: c = "serpent"; break;
            case SUMO_RUN_R0: c = "sumo"; break;
            case ZILLA_RUN_R0: c = "zilla"; break;
            default: c = "?"; break;
            }
            buildprintf("WARNING: skill-masked %s at %d,%d,%d not being killed because it "
                        "activates something\n", c,sp->x,sp->y,sp->z);
            return TRUE;
        }

        return FALSE;
    }

    return TRUE;
}


void PreCacheRipper(void);
void PreCacheRipper2(void);
void PreCacheCoolie(void);
void PreCacheSerpent(void);
void PreCacheGuardian(void);
void PreCacheNinja(void);
void PreCacheSumo(void);
void PreCacheEel(void);
void PreCacheToiletGirl(void);
void PreCacheWashGirl(void);
void PreCacheTrash(void);
void PreCacheBunny(void);
void PreCacheSkel(void);
void PreCacheHornet(void);
void PreCacheSkull(void);
void PreCacheBetty(void);
void PreCachePachinko(void);

SWBOOL
ActorSpawn(SPRITEp sp)
{
    int ret = TRUE;
    short SpriteNum = sp - sprite;

    switch (sp->picnum)
    {
    case COOLIE_RUN_R0:
    {
        ANIMATOR SetupCoolie;

        //PreCacheCoolie();

        if (!ActorTestSpawn(sp))
        {
            KillSprite(SpriteNum);
            return FALSE;
        }

        PicAnimOff(sp->picnum);
        SetupCoolie(SpriteNum);

        break;
    }

    case NINJA_RUN_R0:
    case NINJA_CRAWL_R0:
    {
        ANIMATOR SetupNinja;

        //PreCacheNinja();

        if (!ActorTestSpawn(sp))
        {
            KillSprite(SpriteNum);
            return FALSE;
        }

        PicAnimOff(sp->picnum);
        SetupNinja(SpriteNum);

        break;
    }

    case GORO_RUN_R0:
    {
        ANIMATOR SetupGoro;

        //PreCacheGuardian();

        if (!ActorTestSpawn(sp))
        {
            KillSprite(SpriteNum);
            return FALSE;
        }

        PicAnimOff(sp->picnum);
        SetupGoro(SpriteNum);
        break;
    }

    case 1441:
    case COOLG_RUN_R0:
    {
        ANIMATOR SetupCoolg;

        //PreCacheGhost();

        if (!ActorTestSpawn(sp))
        {
            KillSprite(SpriteNum);
            return FALSE;
        }

        PicAnimOff(sp->picnum);
        SetupCoolg(SpriteNum);
        break;
    }

    case EEL_RUN_R0:
    {
        ANIMATOR SetupEel;

        //PreCacheEel();

        if (!ActorTestSpawn(sp))
        {
            KillSprite(SpriteNum);
            return FALSE;
        }

        PicAnimOff(sp->picnum);
        SetupEel(SpriteNum);
        break;
    }

    case SUMO_RUN_R0:
    {
        ANIMATOR SetupSumo;

        //PreCacheSumo();

        if (!ActorTestSpawn(sp))
        {
            KillSprite(SpriteNum);
            return FALSE;
        }

        PicAnimOff(sp->picnum);
        SetupSumo(SpriteNum);

        break;
    }

    case ZILLA_RUN_R0:
    {
        ANIMATOR SetupZilla;

        //PreCacheSumo();

        if (!ActorTestSpawn(sp))
        {
            KillSprite(SpriteNum);
            return FALSE;
        }

        PicAnimOff(sp->picnum);
        SetupZilla(SpriteNum);

        break;
    }

    case TOILETGIRL_R0:
    {
        ANIMATOR SetupToiletGirl;

        //PreCacheToiletGirl();

        if (!ActorTestSpawn(sp))
        {
            KillSprite(SpriteNum);
            return FALSE;
        }

        PicAnimOff(sp->picnum);
        SetupToiletGirl(SpriteNum);

        break;
    }

    case WASHGIRL_R0:
    {
        ANIMATOR SetupWashGirl;

        //PreCacheWashGirl();

        if (!ActorTestSpawn(sp))
        {
            KillSprite(SpriteNum);
            return FALSE;
        }

        PicAnimOff(sp->picnum);
        SetupWashGirl(SpriteNum);

        break;
    }

    case CARGIRL_R0:
    {
        ANIMATOR SetupCarGirl;

        //PreCacheCarGirl();

        if (!ActorTestSpawn(sp))
        {
            KillSprite(SpriteNum);
            return FALSE;
        }

        PicAnimOff(sp->picnum);
        SetupCarGirl(SpriteNum);

        break;
    }

    case MECHANICGIRL_R0:
    {
        ANIMATOR SetupMechanicGirl;

        //PreCacheMechanicGirl();

        if (!ActorTestSpawn(sp))
        {
            KillSprite(SpriteNum);
            return FALSE;
        }

        PicAnimOff(sp->picnum);
        SetupMechanicGirl(SpriteNum);

        break;
    }

    case SAILORGIRL_R0:
    {
        ANIMATOR SetupSailorGirl;

        //PreCacheSailorGirl();

        if (!ActorTestSpawn(sp))
        {
            KillSprite(SpriteNum);
            return FALSE;
        }

        PicAnimOff(sp->picnum);
        SetupSailorGirl(SpriteNum);

        break;
    }

    case PRUNEGIRL_R0:
    {
        ANIMATOR SetupPruneGirl;

        //PreCachePruneGirl();

        if (!ActorTestSpawn(sp))
        {
            KillSprite(SpriteNum);
            return FALSE;
        }

        PicAnimOff(sp->picnum);
        SetupPruneGirl(SpriteNum);

        break;
    }

    case TRASHCAN:
    {
        ANIMATOR SetupTrashCan;

        //PreCacheTrash();
        PicAnimOff(sp->picnum);
        SetupTrashCan(SpriteNum);

        break;
    }

    case BUNNY_RUN_R0:
    {
        ANIMATOR SetupBunny;

        //PreCacheBunny();

        if (!ActorTestSpawn(sp))
        {
            KillSprite(SpriteNum);
            return FALSE;
        }

        PicAnimOff(sp->picnum);
        SetupBunny(SpriteNum);
        break;
    }

    case RIPPER_RUN_R0:
    {
        ANIMATOR SetupRipper;

        //PreCacheRipper();

        if (!ActorTestSpawn(sp))
        {
            KillSprite(SpriteNum);
            return FALSE;
        }

        PicAnimOff(sp->picnum);
        SetupRipper(SpriteNum);
        break;
    }

    case RIPPER2_RUN_R0:
    {
        ANIMATOR SetupRipper2;

        //PreCacheRipper2();

        if (!ActorTestSpawn(sp))
        {
            KillSprite(SpriteNum);
            return FALSE;
        }

        PicAnimOff(sp->picnum);
        SetupRipper2(SpriteNum);
        break;
    }

    case SERP_RUN_R0:
    {
        ANIMATOR SetupSerp;

        //PreCacheSerpent();

        if (!ActorTestSpawn(sp))
        {
            KillSprite(SpriteNum);
            return FALSE;
        }

        PicAnimOff(sp->picnum);
        SetupSerp(SpriteNum);
        break;
    }

    case LAVA_RUN_R0:
    {
        ANIMATOR SetupLava;

        if (!ActorTestSpawn(sp))
        {
            KillSprite(SpriteNum);
            return FALSE;
        }

        PicAnimOff(sp->picnum);
        SetupLava(SpriteNum);
        break;
    }

    case SKEL_RUN_R0:
    {
        ANIMATOR SetupSkel;

        //PreCacheSkel();

        if (!ActorTestSpawn(sp))
        {
            KillSprite(SpriteNum);
            return FALSE;
        }

        PicAnimOff(sp->picnum);
        SetupSkel(SpriteNum);
        break;
    }

    case HORNET_RUN_R0:
    {
        ANIMATOR SetupHornet;

        //PreCacheHornet();

        if (!ActorTestSpawn(sp))
        {
            KillSprite(SpriteNum);
            return FALSE;
        }

        PicAnimOff(sp->picnum);
        SetupHornet(SpriteNum);
        break;
    }

    case SKULL_R0:
    {
        ANIMATOR SetupSkull;

        //PreCacheSkull();

        if (!ActorTestSpawn(sp))
        {
            KillSprite(SpriteNum);
            return FALSE;
        }

        PicAnimOff(sp->picnum);
        SetupSkull(SpriteNum);
        break;
    }

    case BETTY_R0:
    {
        ANIMATOR SetupBetty;

        //PreCacheBetty();

        if (!ActorTestSpawn(sp))
        {
            KillSprite(SpriteNum);
            return FALSE;
        }

        PicAnimOff(sp->picnum);
        SetupBetty(SpriteNum);
        break;
    }

    case 623:   // Pachinko win light
    {
        ANIMATOR SetupPachinkoLight;

        //PreCachePachinko();
        PicAnimOff(sp->picnum);
        SetupPachinkoLight(SpriteNum);
        break;
    }

    case PACHINKO1:
    {
        ANIMATOR SetupPachinko1;

        //PreCachePachinko();
        PicAnimOff(sp->picnum);
        SetupPachinko1(SpriteNum);
        break;
    }

    case PACHINKO2:
    {
        ANIMATOR SetupPachinko2;

        //PreCachePachinko();
        PicAnimOff(sp->picnum);
        SetupPachinko2(SpriteNum);
        break;
    }

    case PACHINKO3:
    {
        ANIMATOR SetupPachinko3;

        //PreCachePachinko();
        PicAnimOff(sp->picnum);
        SetupPachinko3(SpriteNum);
        break;
    }

    case PACHINKO4:
    {
        ANIMATOR SetupPachinko4;

        //PreCachePachinko();
        PicAnimOff(sp->picnum);
        SetupPachinko4(SpriteNum);
        break;
    }

    case GIRLNINJA_RUN_R0:
    {
        ANIMATOR SetupGirlNinja;

        if (!ActorTestSpawn(sp))
        {
            KillSprite(SpriteNum);
            return FALSE;
        }

        PicAnimOff(sp->picnum);
        SetupGirlNinja(SpriteNum);

        break;
    }

    default:
        ret = FALSE;
        break;
    }

    return ret;
}


void
IconDefault(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    //if (sp->statnum == STAT_ITEM)
    change_sprite_stat(SpriteNum, STAT_ITEM);

    RESET(sp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
    u->Radius = 650;

    DoActorZrange(SpriteNum);
}

void PreMapCombineFloors(void)
{
#define MAX_FLOORS 32
    SPRITEp sp;
    int xoff,yoff;
    int i, j, k;
    int16_t SpriteNum, NextSprite;
    WALLp wp;
    int base_offset;
    PLAYERp pp = &Player[myconnectindex];
    int dx,dy;
    short sectlist[MAXSECTORS];
    short sectlistplc, sectlistend, dasect, startwall, endwall, nextsector;
    short pnum;

    typedef struct
    {
        SPRITEp offset;
    } BOUND_LIST;

    BOUND_LIST BoundList[MAX_FLOORS];

    memset(BoundList, 0, MAX_FLOORS * sizeof(BOUND_LIST));

    TRAVERSE_SPRITE_STAT(headspritestat[0], SpriteNum, NextSprite)
    {
        sp = &sprite[SpriteNum];

        if (sp->picnum != ST1)
            continue;

        if (sp->hitag == BOUND_FLOOR_OFFSET || sp->hitag == BOUND_FLOOR_BASE_OFFSET)
        {
            ASSERT(sp->lotag < MAX_FLOORS);
            BoundList[sp->lotag].offset = sp;
            change_sprite_stat(SpriteNum, STAT_FAF);
        }
    }

    for (i = base_offset = 0; i < MAX_FLOORS; i++)
    {
        // blank so continue
        if (!BoundList[i].offset)
            continue;

        if (BoundList[i].offset->hitag == BOUND_FLOOR_BASE_OFFSET)
        {
            base_offset = i;
            continue;
        }

        dx = BoundList[base_offset].offset->x - BoundList[i].offset->x;
        dy = BoundList[base_offset].offset->y - BoundList[i].offset->y;

        sectlist[0] = BoundList[i].offset->sectnum;
        sectlistplc = 0; sectlistend = 1;
        while (sectlistplc < sectlistend)
        {
            dasect = sectlist[sectlistplc++];

            for (j=headspritesect[dasect]; j>=0; j=nextspritesect[j])
            {
                sprite[j].x += dx;
                sprite[j].y += dy;
            }

            startwall = sector[dasect].wallptr;
            endwall = startwall + sector[dasect].wallnum;
            for (j=startwall; j<endwall; j++)
            {
                wall[j].x += dx;
                wall[j].y += dy;

                nextsector = wall[j].nextsector;
                if (nextsector < 0) continue;

                for (k=sectlistend-1; k>=0; k--)
                    if (sectlist[k] == nextsector)
                        break;
                if (k < 0)
                    sectlist[sectlistend++] = nextsector;
            }

        }

        TRAVERSE_CONNECT(pnum)
        {
            PLAYERp pp = &Player[pnum];
            dasect = pp->cursectnum;
            for (j=0; j<sectlistend; j++)
            {
                if (sectlist[j] == dasect)
                {
                    pp->posx += dx;
                    pp->posy += dy;
                    pp->oposx = pp->oldposx = pp->posx;
                    pp->oposy = pp->oldposy = pp->posy;
                    break;
                }
            }
        }

    }

    // get rid of the sprites used
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_FAF], SpriteNum, NextSprite)
    {
        KillSprite(SpriteNum);
    }
}

#if 0
// example of way to traverse through sectors from closest to farthest
void TraverseSectors(short start_sect)
{
    int i, j, k;
    short sectlist[MAXSECTORS];
    short sectlistplc, sectlistend, sect, startwall, endwall, nextsector;

    sectlist[0] = start_sect;
    sectlistplc = 0; sectlistend = 1;
    while (sectlistplc < sectlistend)
    {
        sect = sectlist[sectlistplc++];

        startwall = sector[sect].wallptr;
        endwall = startwall + sector[sect].wallnum;

        for (j=startwall; j<endwall; j++)
        {
            nextsector = wall[j].nextsector;

            if (nextsector < 0)
                continue;

            // make sure its not on the list
            for (k = sectlistend-1; k >= 0; k--)
            {
                if (sectlist[k] == nextsector)
                    break;
            }

            // if its not on the list add it to the end
            if (k < 0)
                sectlist[sectlistend++] = nextsector;
        }

    }

    // list is finished - can now traverse it
#if 0
    sect = pp->cursectnum;
    for (j=0; j<sectlistend; j++)
    {
        if (sectlist[j] == sect)
        {
            break;
        }
    }
#endif
}
#endif


void
SpriteSetupPost(void)
{
    SPRITEp ds;
    USERp u;
    short SpriteNum, NextSprite;
    short i, nexti;
    int cz,fz;

    // Post processing of some sprites after gone through the main SpriteSetup()
    // routine

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_FLOOR_PAN], SpriteNum, NextSprite)
    {
        TRAVERSE_SPRITE_SECT(headspritesect[sprite[SpriteNum].sectnum],i,nexti)
        {
            ds = &sprite[i];

            if (ds->picnum == ST1)
                continue;

            if (TEST(ds->cstat, CSTAT_SPRITE_WALL|CSTAT_SPRITE_FLOOR))
                continue;

            if (User[i])
                continue;

            getzsofslope(ds->sectnum, ds->x, ds->y, &cz, &fz);
            if (labs(ds->z - fz) > Z(4))
                continue;

            u = SpawnUser(i, 0, NULL);
            change_sprite_stat(i, STAT_NO_STATE);
            u->ceiling_dist = Z(4);
            u->floor_dist = -Z(2);

            u->ActorActionFunc = DoActorDebris;

            SET(ds->cstat, CSTAT_SPRITE_BREAKABLE);
            SET(ds->extra, SPRX_BREAKABLE);
        }
    }
}


void
SpriteSetup(void)
{
    SPRITEp sp;
    short SpriteNum = 0, NextSprite, ndx;
    USERp u;
    TRACK_POINTp tp;
    TRACKp t;
    short i, num;
    int cz,fz;

    // special case for player
    PicAnimOff(PLAYER_NINJA_RUN_R0);

    // Clear Sprite Extension structure
    memset(&SectUser[0], 0, sizeof(SectUser));

    // Clear all extra bits - they are set by sprites
    for (i = 0; i < numsectors; i++)
    {
        sector[i].extra = 0;
    }

    // Clear PARALLAX_LEVEL overrides
    parallaxyscale_override = 0;
    pskybits_override = -1;

    // Call my little sprite setup routine first
    JS_SpriteSetup();

    TRAVERSE_SPRITE_STAT(headspritestat[0], SpriteNum, NextSprite)
    {
        sp = &sprite[SpriteNum];


        // not used yetv
        getzsofslope(sp->sectnum, sp->x, sp->y, &cz, &fz);
        if (sp->z > DIV2(cz + fz))
        {
            // closer to a floor
            SET(sp->cstat, CSTAT_SPRITE_CLOSE_FLOOR);
        }

        // CSTAT_SPIN is insupported - get rid of it
        if (TEST(sp->cstat, CSTAT_SPRITE_SLAB) == CSTAT_SPRITE_SLAB)
            RESET(sp->cstat, CSTAT_SPRITE_SLAB);

        // if BLOCK is set set BLOCK_HITSCAN
        // Hope this doesn't screw up anything
        if (TEST(sp->cstat, CSTAT_SPRITE_BLOCK))
            SET(sp->cstat, CSTAT_SPRITE_BLOCK_HITSCAN);

        ////////////////////////////////////////////
        //
        // BREAKABLE CHECK
        //
        ////////////////////////////////////////////

        // USER SETUP - TAGGED BY USER
        // Non ST1 sprites that are tagged like them
        if (TEST_BOOL1(sp) && sp->picnum != ST1)
        {
            RESET(sp->extra,
                  SPRX_BOOL4|
                  SPRX_BOOL5|
                  SPRX_BOOL6|
                  SPRX_BOOL7|
                  SPRX_BOOL8|
                  SPRX_BOOL9|
                  SPRX_BOOL10);

            switch (sp->hitag)
            {
            case BREAKABLE:
                // need something that tells missiles to hit them
                // but allows actors to move through them
                sp->clipdist = SPRITEp_SIZE_X(sp);
                SET(sp->extra, SPRX_BREAKABLE);
                SET(sp->cstat, CSTAT_SPRITE_BREAKABLE);
                break;
            }
        }
        else
        {
            // BREAK SETUP TABLE AUTOMATED
            SetupSpriteForBreak(sp);
        }

        if (sp->lotag == TAG_SPRITE_HIT_MATCH)
        {
            // if multi item and not a modem game
            if (TEST(sp->extra, SPRX_MULTI_ITEM))
            {
                if (numplayers <= 1 || gNet.MultiGameType == MULTI_GAME_COOPERATIVE)
                {
                    KillSprite(SpriteNum);
                    continue;
                }
            }


            // crack sprite
            if (sp->picnum == 80)
            {
                RESET(sp->cstat, CSTAT_SPRITE_BLOCK);
                SET(sp->cstat, CSTAT_SPRITE_BLOCK_HITSCAN|CSTAT_SPRITE_BLOCK_MISSILE);;
            }
            else
            {
                RESET(sp->cstat, CSTAT_SPRITE_BLOCK);
                SET(sp->cstat, CSTAT_SPRITE_BLOCK_HITSCAN|CSTAT_SPRITE_BLOCK_MISSILE);;
                SET(sp->cstat, CSTAT_SPRITE_INVISIBLE);;
            }

            if (TEST(SP_TAG8(sp), BIT(0)))
                SET(sp->cstat, CSTAT_SPRITE_INVISIBLE); ;

            if (TEST(SP_TAG8(sp), BIT(1)))
                RESET(sp->cstat, CSTAT_SPRITE_INVISIBLE);

            change_sprite_stat(SpriteNum, STAT_SPRITE_HIT_MATCH);
            continue;
        }

        if (sprite[SpriteNum].picnum >= TRACK_SPRITE &&
            sprite[SpriteNum].picnum <= TRACK_SPRITE + MAX_TRACKS)
        {
            short track_num;

            // skip this sprite, just for numbering walls/sectors
            if (TEST(sprite[SpriteNum].cstat, CSTAT_SPRITE_WALL))
                continue;

            track_num = sprite[SpriteNum].picnum - TRACK_SPRITE + 0;

            change_sprite_stat(SpriteNum, STAT_TRACK + track_num);

            continue;
        }

        if (ActorSpawn(sp))
            continue;

        switch (sprite[SpriteNum].picnum)
        {
        case ST_QUICK_JUMP:
            change_sprite_stat(SpriteNum, STAT_QUICK_JUMP);
            break;
        case ST_QUICK_JUMP_DOWN:
            change_sprite_stat(SpriteNum, STAT_QUICK_JUMP_DOWN);
            break;
        case ST_QUICK_SUPER_JUMP:
            change_sprite_stat(SpriteNum, STAT_QUICK_SUPER_JUMP);
            break;
        case ST_QUICK_SCAN:
            change_sprite_stat(SpriteNum, STAT_QUICK_SCAN);
            break;
        case ST_QUICK_EXIT:
            change_sprite_stat(SpriteNum, STAT_QUICK_EXIT);
            break;
        case ST_QUICK_OPERATE:
            change_sprite_stat(SpriteNum, STAT_QUICK_OPERATE);
            break;
        case ST_QUICK_DUCK:
            change_sprite_stat(SpriteNum, STAT_QUICK_DUCK);
            break;
        case ST_QUICK_DEFEND:
            change_sprite_stat(SpriteNum, STAT_QUICK_DEFEND);
            break;

        case ST1:
        {
            SPRITEp sp = &sprite[SpriteNum];
            SECT_USERp sectu;
            short tag;
            short bit;

            // get rid of defaults
            if (SP_TAG3(sp) == 32)
                SP_TAG3(sp) = 0;

            tag = sp->hitag;

            RESET(sp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
            SET(sp->cstat, CSTAT_SPRITE_INVISIBLE);

            // for bounding sector objects
            if ((tag >= 500 && tag < 600) || tag == SECT_SO_CENTER)
            {
                // NOTE: These will get deleted by the sector object
                // setup code
                change_sprite_stat(SpriteNum, STAT_ST1);
                break;
            }

            if (tag < 16)
            {
                bit = 1 << (tag);

                SET(sector[sp->sectnum].extra, bit);

                if (TEST(bit, SECTFX_SINK))
                {
                    sectu = GetSectUser(sp->sectnum);
                    sectu->depth = sp->lotag;
                    KillSprite(SpriteNum);
                }
                else if (TEST(bit, SECTFX_OPERATIONAL))
                {
                    KillSprite(SpriteNum);
                }
                else if (TEST(bit, SECTFX_CURRENT))
                {
                    sectu = GetSectUser(sp->sectnum);
                    sectu->speed = sp->lotag;
                    sectu->ang = sp->ang;
                    KillSprite(SpriteNum);
                }
                else if (TEST(bit, SECTFX_NO_RIDE))
                {
                    change_sprite_stat(SpriteNum, STAT_NO_RIDE);
                }
                else if (TEST(bit, SECTFX_DIVE_AREA))
                {
                    sectu = GetSectUser(sp->sectnum);
                    sectu->number = sp->lotag;
                    change_sprite_stat(SpriteNum, STAT_DIVE_AREA);
                }
                else if (TEST(bit, SECTFX_UNDERWATER))
                {
                    sectu = GetSectUser(sp->sectnum);
                    sectu->number = sp->lotag;
                    change_sprite_stat(SpriteNum, STAT_UNDERWATER);
                }
                else if (TEST(bit, SECTFX_UNDERWATER2))
                {
                    sectu = GetSectUser(sp->sectnum);
                    sectu->number = sp->lotag;
                    if (sp->clipdist == 1)
                        SET(sectu->flags, SECTFU_CANT_SURFACE);
                    change_sprite_stat(SpriteNum, STAT_UNDERWATER2);
                }
            }
            else
            {
                switch (tag)
                {
#if 0
                case MULTI_PLAYER_START:
                    change_sprite_stat(SpriteNum, STAT_MULTI_START + sp->lotag);
                    break;
                case MULTI_COOPERATIVE_START:
                    change_sprite_stat(SpriteNum, STAT_CO_OP_START + sp->lotag);
                    break;
#endif

                case SECT_MATCH:
                    sectu = GetSectUser(sp->sectnum);

                    sectu->number = sp->lotag;

                    KillSprite(SpriteNum);
                    break;

                case SLIDE_SECTOR:
                    sectu = GetSectUser(sp->sectnum);
                    SET(sectu->flags, SECTFU_SLIDE_SECTOR);
                    sectu->speed = SP_TAG2(sp);
                    KillSprite(SpriteNum);
                    break;

                case SECT_DAMAGE:
                {
                    sectu = GetSectUser(sp->sectnum);
                    if (TEST_BOOL1(sp))
                        SET(sectu->flags, SECTFU_DAMAGE_ABOVE_SECTOR);
                    sectu->damage = sp->lotag;
                    KillSprite(SpriteNum);
                    break;
                }

                case PARALLAX_LEVEL:
                {
                    parallaxyscale_override = 8192;
                    pskybits_override = sp->lotag;
                    if (SP_TAG4(sp) > 2048)
                        parallaxyscale_override = SP_TAG4(sp);
                    KillSprite(SpriteNum);
                    break;
                }

                case BREAKABLE:
                    // used for wall info
                    change_sprite_stat(SpriteNum, STAT_BREAKABLE);
                    break;

                case SECT_DONT_COPY_PALETTE:
                {
                    sectu = GetSectUser(sp->sectnum);

                    SET(sectu->flags, SECTFU_DONT_COPY_PALETTE);
                    KillSprite(SpriteNum);
                    break;
                }

                case SECT_FLOOR_PAN:
                {
                    short i,nexti;
                    SPRITEp ds;
                    int cz,fz;

                    // if moves with SO
                    if (TEST_BOOL1(sp))
                        sp->xvel = 0;
                    else
                        sp->xvel = sp->lotag;

                    change_sprite_stat(SpriteNum, STAT_FLOOR_PAN);
                    break;
                }

                case SECT_CEILING_PAN:
                {
                    // if moves with SO
                    if (TEST_BOOL1(sp))
                        sp->xvel = 0;
                    else
                        sp->xvel = sp->lotag;
                    change_sprite_stat(SpriteNum, STAT_CEILING_PAN);
                    break;
                }

                case SECT_WALL_PAN_SPEED:
                {
                    short i, found = FALSE;
                    vec3_t hit_pos = { sp->x, sp->y, sp->z - Z(8) };
                    hitdata_t hitinfo;

                    hitscan(&hit_pos, sp->sectnum,    // Start position
                            sintable[NORM_ANGLE(sp->ang + 512)],    // X vector of 3D ang
                            sintable[sp->ang],      // Y vector of 3D ang
                            0,      // Z vector of 3D ang
                            &hitinfo, CLIPMASK_MISSILE);

                    if (hitinfo.wall == -1)
                    {
                        KillSprite(SpriteNum);
                        break;
                    }

                    sp->owner = hitinfo.wall;
                    // if moves with SO
                    if (TEST_BOOL1(sp))
                        sp->xvel = 0;
                    else
                        sp->xvel = sp->lotag;
                    sp->ang = SP_TAG6(sp);
                    // attach to the sector that contains the wall
                    changespritesect(SpriteNum, hitinfo.sect);
                    change_sprite_stat(SpriteNum, STAT_WALL_PAN);
                    break;
                }

                case WALL_DONT_STICK:
                {
                    vec3_t hit_pos = { sp->x, sp->y, sp->z - Z(8) };
                    hitdata_t hitinfo;

                    hitscan(&hit_pos, sp->sectnum,    // Start position
                            sintable[NORM_ANGLE(sp->ang + 512)],    // X vector of 3D ang
                            sintable[sp->ang],      // Y vector of 3D ang
                            0,      // Z vector of 3D ang
                            &hitinfo, CLIPMASK_MISSILE);

                    if (hitinfo.wall == -1)
                    {
                        KillSprite(SpriteNum);
                        break;
                    }

                    SET(wall[hitinfo.wall].extra, WALLFX_DONT_STICK);
                    KillSprite(SpriteNum);
                    break;
                }

                case TRIGGER_SECTOR:
                {
                    SET(sector[sp->sectnum].extra, SECTFX_TRIGGER);
                    change_sprite_stat(SpriteNum, STAT_TRIGGER);
                    break;
                }

                case DELETE_SPRITE:
                {
                    change_sprite_stat(SpriteNum, STAT_DELETE_SPRITE);
                    break;
                }

                case SPAWN_ITEMS:
                {
                    if (TEST(sp->extra, SPRX_MULTI_ITEM))
                    {
                        if (numplayers <= 1 || gNet.MultiGameType == MULTI_GAME_COOPERATIVE)
                        {
                            KillSprite(SpriteNum);
                            break;
                        }
                    }


                    change_sprite_stat(SpriteNum, STAT_SPAWN_ITEMS);
                    break;
                }

                case CEILING_FLOOR_PIC_OVERRIDE:
                {
                    // block hitscans depending on translucency
                    if (SP_TAG7(sp) == 0 || SP_TAG7(sp) == 1)
                    {
                        if (SP_TAG3(sp) == 0)
                            SET(sector[sp->sectnum].ceilingstat, CEILING_STAT_FAF_BLOCK_HITSCAN);
                        else
                            SET(sector[sp->sectnum].floorstat, FLOOR_STAT_FAF_BLOCK_HITSCAN);
                    }
                    else if (TEST_BOOL1(sp))
                    {
                        if (SP_TAG3(sp) == 0)
                            SET(sector[sp->sectnum].ceilingstat, CEILING_STAT_FAF_BLOCK_HITSCAN);
                        else
                            SET(sector[sp->sectnum].floorstat, FLOOR_STAT_FAF_BLOCK_HITSCAN);
                    }

                    // copy tag 7 to tag 6 and pre-shift it
                    SP_TAG6(sp) = SP_TAG7(sp);
                    SP_TAG6(sp) <<= 7;
                    change_sprite_stat(SpriteNum, STAT_CEILING_FLOOR_PIC_OVERRIDE);
                    break;
                }

                case QUAKE_SPOT:
                {
                    change_sprite_stat(SpriteNum, STAT_QUAKE_SPOT);
                    //SP_TAG13(sp) = (SP_TAG6(sp)*10L) * 120L;
                    SET_SP_TAG13(sp, ((SP_TAG6(sp)*10L) * 120L));
                    break;
                }

                case SECT_CHANGOR:
                {
                    change_sprite_stat(SpriteNum, STAT_CHANGOR);
                    break;
                }

#if 0
                case SECT_DEBRIS_SEWER:
                {
                    ANIMATOR DoGenerateSewerDebris;

                    u = SpawnUser(SpriteNum, 0, NULL);

                    ASSERT(u != NULL);
                    u->RotNum = 0;
                    u->WaitTics = sp->lotag * 120;

                    u->ActorActionFunc = DoGenerateSewerDebris;

                    change_sprite_stat(SpriteNum, STAT_NO_STATE);
                    break;
                }
#endif

                case SECT_VATOR:
                {
                    ANIMATOR DoVator, DoVatorAuto;
                    SECTORp sectp = &sector[sp->sectnum];
                    SECT_USERp sectu;
                    short speed,vel,time,type,start_on,floor_vator;
                    u = SpawnUser(SpriteNum, 0, NULL);

                    // vator already set - ceiling AND floor vator
                    if (TEST(sectp->extra, SECTFX_VATOR))
                    {
                        sectu = GetSectUser(sp->sectnum);
                        SET(sectu->flags, SECTFU_VATOR_BOTH);
                    }
                    SET(sectp->extra, SECTFX_VATOR);
                    SetSectorWallBits(sp->sectnum, WALLFX_DONT_STICK, TRUE, TRUE);
                    SET(sector[sp->sectnum].extra, SECTFX_DYNAMIC_AREA);

                    // don't step on toes of other sector settings
                    if (sectp->lotag == 0 && sectp->hitag == 0)
                        sectp->lotag = TAG_VATOR;

                    type = SP_TAG3(sp);
                    speed = SP_TAG4(sp);
                    vel = SP_TAG5(sp);
                    time = SP_TAG9(sp);
                    start_on = !!TEST_BOOL1(sp);
                    floor_vator = TRUE;
                    if (TEST(sp->cstat, CSTAT_SPRITE_YFLIP))
                        floor_vator = FALSE;

                    u->jump_speed = u->vel_tgt = speed;
                    u->vel_rate = vel;
                    u->WaitTics = time*15; // 1/8 of a sec
                    u->Tics = 0;

                    SET(u->Flags, SPR_ACTIVE);

                    switch (type)
                    {
                    case 0:
                        RESET(u->Flags, SPR_ACTIVE);
                        u->ActorActionFunc = DoVator;
                        break;
                    case 1:
                        RESET(u->Flags, SPR_ACTIVE);
                        u->ActorActionFunc = DoVator;
                        break;
                    case 2:
                        u->ActorActionFunc = DoVatorAuto;
                        break;
                    case 3:
                        RESET(u->Flags, SPR_ACTIVE);
                        u->ActorActionFunc = DoVatorAuto;
                        break;
                    }

                    if (floor_vator)
                    {
                        // start off
                        u->sz = sectp->floorz;
                        u->z_tgt = sp->z;
                        if (start_on)
                        {
                            int amt;
                            amt = sp->z - sectp->floorz;

                            // start in the on position
                            //sectp->floorz = sp->z;
                            sectp->floorz += amt;
                            u->z_tgt = u->sz;

                            MoveSpritesWithSector(sp->sectnum, amt, 0); // floor
                        }

                        // set orig z
                        u->oz = sectp->floorz;
                    }
                    else
                    {
                        // start off
                        u->sz = sectp->ceilingz;
                        u->z_tgt = sp->z;
                        if (start_on)
                        {
                            int amt;
                            amt = sp->z - sectp->ceilingz;

                            // starting in the on position
                            //sectp->ceilingz = sp->z;
                            sectp->ceilingz += amt;
                            u->z_tgt = u->sz;

                            MoveSpritesWithSector(sp->sectnum, amt, 1); // ceiling
                        }

                        // set orig z
                        u->oz = sectp->ceilingz;
                    }


                    change_sprite_stat(SpriteNum, STAT_VATOR);
                    break;
                }

                case SECT_ROTATOR_PIVOT:
                {
                    change_sprite_stat(SpriteNum, STAT_ROTATOR_PIVOT);
                    break;
                }

                case SECT_ROTATOR:
                {
                    ANIMATOR DoRotator;
                    SECTORp sectp = &sector[sp->sectnum];
                    SECT_USERp sectu;
                    short time,type;
                    short wallcount,startwall,endwall,w;
                    u = SpawnUser(SpriteNum, 0, NULL);

                    SetSectorWallBits(sp->sectnum, WALLFX_DONT_STICK, TRUE, TRUE);

                    // need something for this
                    sectp->lotag = TAG_ROTATOR;
                    sectp->hitag = sp->lotag;

                    type = SP_TAG3(sp);
                    time = SP_TAG9(sp);

                    u->WaitTics = time*15; // 1/8 of a sec
                    u->Tics = 0;

                    startwall = sector[sp->sectnum].wallptr;
                    endwall = startwall + sector[sp->sectnum].wallnum - 1;

                    // count walls of sector
                    for (w = startwall, wallcount = 0; w <= endwall; w++)
                        wallcount++;

                    u->rotator = CallocMem(sizeof(ROTATOR), 1);
                    u->rotator->num_walls = wallcount;
                    u->rotator->open_dest = SP_TAG5(sp);
                    u->rotator->speed = SP_TAG7(sp);
                    u->rotator->vel = SP_TAG8(sp);
                    u->rotator->pos = 0; // closed
                    u->rotator->tgt = u->rotator->open_dest; // closed
                    u->rotator->origx = CallocMem(sizeof(u->rotator->origx) * wallcount, 1);
                    u->rotator->origy = CallocMem(sizeof(u->rotator->origy) * wallcount, 1);

                    u->rotator->orig_speed = u->rotator->speed;

                    for (w = startwall, wallcount = 0; w <= endwall; w++)
                    {
                        u->rotator->origx[wallcount] = wall[w].x;
                        u->rotator->origy[wallcount] = wall[w].y;
                        wallcount++;
                    }

                    SET(u->Flags, SPR_ACTIVE);

                    switch (type)
                    {
                    case 0:
                        RESET(u->Flags, SPR_ACTIVE);
                        u->ActorActionFunc = DoRotator;
                        break;
                    case 1:
                        RESET(u->Flags, SPR_ACTIVE);
                        u->ActorActionFunc = DoRotator;
                        break;
                    }

                    change_sprite_stat(SpriteNum, STAT_ROTATOR);
                    break;
                }

                case SECT_SLIDOR:
                {
                    ANIMATOR DoSlidor;
                    SECTORp sectp = &sector[sp->sectnum];
                    SECT_USERp sectu;
                    short time,type;
                    short wallcount,startwall,endwall,w;
                    int DoSlidorInstantClose(short SpriteNum);

                    u = SpawnUser(SpriteNum, 0, NULL);

                    SetSectorWallBits(sp->sectnum, WALLFX_DONT_STICK, TRUE, TRUE);

                    // need something for this
                    sectp->lotag = TAG_SLIDOR;
                    sectp->hitag = sp->lotag;

                    type = SP_TAG3(sp);
                    time = SP_TAG9(sp);

                    u->WaitTics = time*15; // 1/8 of a sec
                    u->Tics = 0;

                    u->rotator = CallocMem(sizeof(ROTATOR), 1);
                    u->rotator->open_dest = SP_TAG5(sp);
                    u->rotator->speed = SP_TAG7(sp);
                    u->rotator->vel = SP_TAG8(sp);
                    u->rotator->pos = 0; // closed
                    u->rotator->tgt = u->rotator->open_dest; // closed
                    u->rotator->num_walls = 0;
                    u->rotator->orig_speed = u->rotator->speed;

                    SET(u->Flags, SPR_ACTIVE);

                    switch (type)
                    {
                    case 0:
                        RESET(u->Flags, SPR_ACTIVE);
                        u->ActorActionFunc = DoSlidor;
                        break;
                    case 1:
                        RESET(u->Flags, SPR_ACTIVE);
                        u->ActorActionFunc = DoSlidor;
                        break;
                    }


                    if (TEST_BOOL5(sp))
                    {
                        DoSlidorInstantClose(SpriteNum);
                    }

                    change_sprite_stat(SpriteNum, STAT_SLIDOR);
                    break;
                }

                case SECT_SPIKE:
                {
                    ANIMATOR DoSpike, DoSpikeAuto;
                    short speed,vel,time,type,start_on,floor_vator;
                    int floorz,ceilingz,trash;
                    u = SpawnUser(SpriteNum, 0, NULL);

                    SetSectorWallBits(sp->sectnum, WALLFX_DONT_STICK, FALSE, TRUE);
                    SET(sector[sp->sectnum].extra, SECTFX_DYNAMIC_AREA);

                    type = SP_TAG3(sp);
                    speed = SP_TAG4(sp);
                    vel = SP_TAG5(sp);
                    time = SP_TAG9(sp);
                    start_on = !!TEST_BOOL1(sp);
                    floor_vator = TRUE;
                    if (TEST(sp->cstat, CSTAT_SPRITE_YFLIP))
                        floor_vator = FALSE;

                    u->jump_speed = u->vel_tgt = speed;
                    u->vel_rate = vel;
                    u->WaitTics = time*15; // 1/8 of a sec
                    u->Tics = 0;

                    SET(u->Flags, SPR_ACTIVE);

                    switch (type)
                    {
                    case 0:
                        RESET(u->Flags, SPR_ACTIVE);
                        u->ActorActionFunc = DoSpike;
                        break;
                    case 1:
                        RESET(u->Flags, SPR_ACTIVE);
                        u->ActorActionFunc = DoSpike;
                        break;
                    case 2:
                        u->ActorActionFunc = DoSpikeAuto;
                        break;
                    case 3:
                        RESET(u->Flags, SPR_ACTIVE);
                        u->ActorActionFunc = DoSpikeAuto;
                        break;
                    }

                    getzrangepoint(sp->x, sp->y, sp->z, sp->sectnum, &ceilingz, &trash, &floorz, &trash);

                    if (floor_vator)
                    {
                        u->zclip = floorz;

                        // start off
                        u->sz = u->zclip;
                        u->z_tgt = sp->z;
                        if (start_on)
                        {
                            // start in the on position
                            u->zclip = sp->z;
                            u->z_tgt = u->sz;
                            SpikeAlign(SpriteNum);
                        }

                        // set orig z
                        u->oz = u->zclip;
                    }
                    else
                    {
                        u->zclip = ceilingz;

                        // start off
                        u->sz = u->zclip;
                        u->z_tgt = sp->z;
                        if (start_on)
                        {
                            // starting in the on position
                            u->zclip = sp->z;
                            u->z_tgt = u->sz;
                            SpikeAlign(SpriteNum);
                        }

                        // set orig z
                        u->oz = u->zclip;
                    }

                    change_sprite_stat(SpriteNum, STAT_SPIKE);
                    break;
                }

                case LIGHTING:
                {
                    short w, startwall, endwall;
                    short wallcount;
                    void *void_ptr;
                    int8_t* wall_shade;
                    USERp u;

                    LIGHT_Tics(sp) = 0;

                    if (LIGHT_ShadeInc(sp) == 0)
                        LIGHT_ShadeInc(sp) = 1;

                    // save off original floor and ceil shades
                    LIGHT_FloorShade(sp) = sector[sp->sectnum].floorshade;
                    LIGHT_CeilingShade(sp) = sector[sp->sectnum].ceilingshade;

                    startwall = sector[sp->sectnum].wallptr;
                    endwall = startwall + sector[sp->sectnum].wallnum - 1;

                    // count walls of sector
                    for (w = startwall, wallcount = 0; w <= endwall; w++)
                    {
                        wallcount++;
                        if (TEST_BOOL5(sp))
                        {
                            if (wall[w].nextwall >= 0)
                                wallcount++;
                        }
                    }

                    User[SpriteNum] = u = SpawnUser(SpriteNum, 0, NULL);
                    u->WallCount = wallcount;
                    wall_shade = u->WallShade = CallocMem(u->WallCount * sizeof(*u->WallShade), 1);

                    // save off original wall shades
                    for (w = startwall, wallcount = 0; w <= endwall; w++)
                    {
                        wall_shade[wallcount] = wall[w].shade;
                        wallcount++;
                        if (TEST_BOOL5(sp))
                        {
                            if (wall[w].nextwall >= 0)
                            {
                                wall_shade[wallcount] = wall[wall[w].nextwall].shade;
                                wallcount++;
                            }
                        }
                    }

                    u->spal = sp->pal;

                    // DON'T USE COVER function
                    changespritestat(SpriteNum, STAT_LIGHTING);
                    break;
                }

                case LIGHTING_DIFFUSE:
                {
                    short w, startwall, endwall;
                    short wallcount;
                    void *void_ptr;
                    int8_t* wall_shade;
                    USERp u;

                    LIGHT_Tics(sp) = 0;

                    // save off original floor and ceil shades
                    LIGHT_FloorShade(sp) = sector[sp->sectnum].floorshade;
                    LIGHT_CeilingShade(sp) = sector[sp->sectnum].ceilingshade;

                    startwall = sector[sp->sectnum].wallptr;
                    endwall = startwall + sector[sp->sectnum].wallnum - 1;

                    // count walls of sector
                    for (w = startwall, wallcount = 0; w <= endwall; w++)
                    {
                        wallcount++;
                        if (TEST_BOOL5(sp))
                        {
                            if (wall[w].nextwall >= 0)
                                wallcount++;
                        }
                    }

                    // !LIGHT
                    // make an wall_shade array and put it in User
                    User[SpriteNum] = u = SpawnUser(SpriteNum, 0, NULL);
                    u->WallCount = wallcount;
                    wall_shade = u->WallShade = CallocMem(u->WallCount * sizeof(*u->WallShade), 1);

                    // save off original wall shades
                    for (w = startwall, wallcount = 0; w <= endwall; w++)
                    {
                        wall_shade[wallcount] = wall[w].shade;
                        wallcount++;
                        if (TEST_BOOL5(sp))
                        {
                            if (wall[w].nextwall >= 0)
                            {
                                wall_shade[wallcount] = wall[wall[w].nextwall].shade;
                                wallcount++;
                            }
                        }
                    }

                    // DON'T USE COVER function
                    changespritestat(SpriteNum, STAT_LIGHTING_DIFFUSE);
                    break;
                }

                case SECT_VATOR_DEST:
                    change_sprite_stat(SpriteNum, STAT_VATOR);
                    break;

                case SO_WALL_DONT_MOVE_UPPER:
                    change_sprite_stat(SpriteNum, STAT_WALL_DONT_MOVE_UPPER);
                    break;

                case SO_WALL_DONT_MOVE_LOWER:
                    change_sprite_stat(SpriteNum, STAT_WALL_DONT_MOVE_LOWER);
                    break;

                case FLOOR_SLOPE_DONT_DRAW:
                    change_sprite_stat(SpriteNum, STAT_FLOOR_SLOPE_DONT_DRAW);
                    break;

                case DEMO_CAMERA:
                    sp->yvel = sp->zvel = 100; //attempt horiz control
                    change_sprite_stat(SpriteNum, STAT_DEMO_CAMERA);
                    break;

                case LAVA_ERUPT:
                {
                    ANIMATOR DoLavaErupt;

                    u = SpawnUser(SpriteNum, ST1, NULL);

                    change_sprite_stat(SpriteNum, STAT_NO_STATE);
                    u->ActorActionFunc = DoLavaErupt;

                    // interval between erupts
                    if (SP_TAG10(sp) == 0)
                        SP_TAG10(sp) = 20;

                    // interval in seconds
                    u->WaitTics = RANDOM_RANGE(SP_TAG10(sp)) * 120;

                    // time to erupt
                    if (SP_TAG9(sp) == 0)
                        SP_TAG9(sp) = 10;

                    sp->z += Z(30);

                    break;
                }


                case SECT_EXPLODING_CEIL_FLOOR:
                {
                    SECTORp sectp = &sector[sp->sectnum];

                    SetSectorWallBits(sp->sectnum, WALLFX_DONT_STICK, FALSE, TRUE);

                    if (TEST(sectp->floorstat, FLOOR_STAT_SLOPE))
                    {
                        SP_TAG5(sp) = sectp->floorheinum;
                        RESET(sectp->floorstat, FLOOR_STAT_SLOPE);
                        sectp->floorheinum = 0;
                    }

                    if (TEST(sectp->ceilingstat, CEILING_STAT_SLOPE))
                    {
                        SP_TAG6(sp) = sectp->ceilingheinum;
                        RESET(sectp->ceilingstat, CEILING_STAT_SLOPE);
                        sectp->ceilingheinum = 0;
                    }

                    SP_TAG4(sp) = abs(sectp->ceilingz - sectp->floorz)>>8;

                    sectp->ceilingz = sectp->floorz;

                    change_sprite_stat(SpriteNum, STAT_EXPLODING_CEIL_FLOOR);
                    break;
                }

                case SECT_COPY_SOURCE:
                    change_sprite_stat(SpriteNum, STAT_COPY_SOURCE);
                    break;

                case SECT_COPY_DEST:
                {
                    SECTORp sectp = &sector[sp->sectnum];

                    SetSectorWallBits(sp->sectnum, WALLFX_DONT_STICK, FALSE, TRUE);
                    change_sprite_stat(SpriteNum, STAT_COPY_DEST);
                    break;
                }


                case SECT_WALL_MOVE:
                    change_sprite_stat(SpriteNum, STAT_WALL_MOVE);
                    break;
                case SECT_WALL_MOVE_CANSEE:
                    change_sprite_stat(SpriteNum, STAT_WALL_MOVE_CANSEE);
                    break;

                case SPRI_CLIMB_MARKER:
                {
                    short ns;
                    SPRITEp np;

                    // setup climb marker
                    change_sprite_stat(SpriteNum, STAT_CLIMB_MARKER);

                    // make a QUICK_LADDER sprite automatically
                    ns = COVERinsertsprite(sp->sectnum, STAT_QUICK_LADDER);
                    np = &sprite[ns];

                    np->cstat = 0;
                    np->extra = 0;
                    np->x = sp->x;
                    np->y = sp->y;
                    np->z = sp->z;
                    np->ang = NORM_ANGLE(sp->ang + 1024);
                    np->picnum = sp->picnum;

                    np->x += MOVEx(256+128, sp->ang);
                    np->y += MOVEy(256+128, sp->ang);

                    break;
                }

                case SO_AUTO_TURRET:
#if 0
                    switch (gNet.MultiGameType)
                    {
                    case MULTI_GAME_NONE:
                        change_sprite_stat(SpriteNum, STAT_ST1);
                        break;
                    case MULTI_GAME_COMMBAT:
                        KillSprite(SpriteNum);
                        break;
                    case MULTI_GAME_COOPERATIVE:
                        change_sprite_stat(SpriteNum, STAT_ST1);
                        break;
                    }
#else
                    change_sprite_stat(SpriteNum, STAT_ST1);
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

                    change_sprite_stat(SpriteNum, STAT_ST1);
                    break;
                }

                case SOUND_SPOT:
                    //SP_TAG13(sp) = SP_TAG4(sp);
                    SET_SP_TAG13(sp, SP_TAG4(sp));
                    change_sprite_stat(SpriteNum, STAT_SOUND_SPOT);
                    break;

                case STOP_SOUND_SPOT:
                    change_sprite_stat(SpriteNum, STAT_STOP_SOUND_SPOT);
                    break;

                case SPAWN_SPOT:
                    if (!User[SpriteNum])
                        u = SpawnUser(SpriteNum, ST1, NULL);

                    if (SP_TAG14(sp) == ((64<<8)|64))
                        //SP_TAG14(sp) = 0;
                        SET_SP_TAG14(sp, 0);

                    change_sprite_stat(SpriteNum, STAT_SPAWN_SPOT);
                    break;

                case VIEW_THRU_CEILING:
                case VIEW_THRU_FLOOR:
                {
                    int i,nexti;
                    // make sure there is only one set per level of these
                    TRAVERSE_SPRITE_STAT(headspritestat[STAT_FAF], i, nexti)
                    {
                        if (sprite[i].hitag == sp->hitag && sprite[i].lotag == sp->lotag)
                        {
                            TerminateGame();
                            printf("Two VIEW_THRU_ tags with same match found on level\n1: x %d, y %d \n2: x %d, y %d", sp->x, sp->y, sprite[i].x, sprite[i].y);
                            exit(0);
                        }
                    }
                    change_sprite_stat(SpriteNum, STAT_FAF);
                    break;
                }

                case VIEW_LEVEL1:
                case VIEW_LEVEL2:
                case VIEW_LEVEL3:
                case VIEW_LEVEL4:
                case VIEW_LEVEL5:
                case VIEW_LEVEL6:
                {
                    change_sprite_stat(SpriteNum, STAT_FAF);
                    break;
                }

                case PLAX_GLOB_Z_ADJUST:
                {
                    SET(sector[sp->sectnum].extra, SECTFX_Z_ADJUST);
                    PlaxCeilGlobZadjust = SP_TAG2(sp);
                    PlaxFloorGlobZadjust = SP_TAG3(sp);
                    KillSprite(SpriteNum);
                    break;
                }

                case CEILING_Z_ADJUST:
                {
                    //SET(sector[sp->sectnum].ceilingstat, CEILING_STAT_FAF_BLOCK_HITSCAN);
                    SET(sector[sp->sectnum].extra, SECTFX_Z_ADJUST);
                    change_sprite_stat(SpriteNum, STAT_ST1);
                    break;
                }

                case FLOOR_Z_ADJUST:
                {
                    //SET(sector[sp->sectnum].floorstat, FLOOR_STAT_FAF_BLOCK_HITSCAN);
                    SET(sector[sp->sectnum].extra, SECTFX_Z_ADJUST);
                    change_sprite_stat(SpriteNum, STAT_ST1);
                    break;
                }

                case WARP_TELEPORTER:
                {
                    short start_wall, wall_num;
                    short sectnum = sp->sectnum;

                    SET(sp->cstat, CSTAT_SPRITE_INVISIBLE);
                    SET(sector[sp->sectnum].extra, SECTFX_WARP_SECTOR);
                    change_sprite_stat(SpriteNum, STAT_WARP);

                    // if just a destination teleporter
                    // don't set up flags
                    if (SP_TAG10(sp) == 1)
                        break;

                    // move the the next wall
                    wall_num = start_wall = sector[sectnum].wallptr;

                    // Travel all the way around loop setting wall bits
                    do
                    {
                        // DO NOT TAG WHITE WALLS!
                        if (wall[wall_num].nextwall >= 0)
                        {
                            SET(wall[wall_num].cstat, CSTAT_WALL_WARP_HITSCAN);
                        }

                        wall_num = wall[wall_num].point2;
                    }
                    while (wall_num != start_wall);

                    break;
                }

                case WARP_CEILING_PLANE:
                case WARP_FLOOR_PLANE:
                {
                    SET(sp->cstat, CSTAT_SPRITE_INVISIBLE);
                    SET(sector[sp->sectnum].extra, SECTFX_WARP_SECTOR);
                    change_sprite_stat(SpriteNum, STAT_WARP);
                    break;
                }

                case WARP_COPY_SPRITE1:
                    SET(sp->cstat, CSTAT_SPRITE_INVISIBLE);
                    SET(sector[sp->sectnum].extra, SECTFX_WARP_SECTOR);
                    change_sprite_stat(SpriteNum, STAT_WARP_COPY_SPRITE1);
                    break;
                case WARP_COPY_SPRITE2:
                    SET(sp->cstat, CSTAT_SPRITE_INVISIBLE);
                    SET(sector[sp->sectnum].extra, SECTFX_WARP_SECTOR);
                    change_sprite_stat(SpriteNum, STAT_WARP_COPY_SPRITE2);
                    break;

                case FIREBALL_TRAP:
                case BOLT_TRAP:
                case SPEAR_TRAP:
                {
                    u = SpawnUser(SpriteNum, 0, NULL);
                    sp->owner = -1;
                    change_sprite_stat(SpriteNum, STAT_TRAP);
                    break;
                }

                case SECT_SO_DONT_BOB:
                {
                    sectu = GetSectUser(sp->sectnum);
                    SET(sectu->flags, SECTFU_SO_DONT_BOB);
                    KillSprite(SpriteNum);
                    break;
                }

                case SECT_LOCK_DOOR:
                {
                    sectu = GetSectUser(sp->sectnum);
                    sectu->number = sp->lotag;
                    sectu->stag = SECT_LOCK_DOOR;
                    KillSprite(SpriteNum);
                    break;
                }

                case SECT_SO_SINK_DEST:
                {
                    sectu = GetSectUser(sp->sectnum);
                    SET(sectu->flags, SECTFU_SO_SINK_DEST);
                    sectu->number = sp->lotag;  // acually the offset Z
                    // value
                    KillSprite(SpriteNum);
                    break;
                }

                case SECT_SO_DONT_SINK:
                {
                    sectu = GetSectUser(sp->sectnum);
                    SET(sectu->flags, SECTFU_SO_DONT_SINK);
                    KillSprite(SpriteNum);
                    break;
                }

                case SO_SLOPE_FLOOR_TO_POINT:
                {
                    sectu = GetSectUser(sp->sectnum);
                    SET(sectu->flags, SECTFU_SO_SLOPE_FLOOR_TO_POINT);
                    SET(sector[sp->sectnum].extra, SECTFX_DYNAMIC_AREA);
                    KillSprite(SpriteNum);
                    break;
                }

                case SO_SLOPE_CEILING_TO_POINT:
                {
                    sectu = GetSectUser(sp->sectnum);
                    SET(sectu->flags, SECTFU_SO_SLOPE_CEILING_TO_POINT);
                    SET(sector[sp->sectnum].extra, SECTFX_DYNAMIC_AREA);
                    KillSprite(SpriteNum);
                    break;
                }
                case SECT_SO_FORM_WHIRLPOOL:
                {
                    sectu = GetSectUser(sp->sectnum);
                    sectu->stag = SECT_SO_FORM_WHIRLPOOL;
                    sectu->height = sp->lotag;
                    KillSprite(SpriteNum);
                    break;
                }

                case SECT_ACTOR_BLOCK:
                {
                    short start_wall, wall_num;
                    short sectnum = sp->sectnum;

                    // move the the next wall
                    wall_num = start_wall = sector[sectnum].wallptr;

                    // Travel all the way around loop setting wall bits
                    do
                    {
                        SET(wall[wall_num].cstat, CSTAT_WALL_BLOCK_ACTOR);
                        if (wall[wall_num].nextwall >= 0)
                            SET(wall[wall[wall_num].nextwall].cstat, CSTAT_WALL_BLOCK_ACTOR);
                        wall_num = wall[wall_num].point2;
                    }
                    while (wall_num != start_wall);

                    KillSprite(SpriteNum);
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
                    KillSprite(SpriteNum);
                    break;
                }

                u = SpawnUser(SpriteNum, 0, NULL);

                ASSERT(u != NULL);
                sprite[SpriteNum].picnum = u->ID = sprite[SpriteNum].picnum;

                u->spal = sprite[SpriteNum].pal; // Set the palette from build

                //SET(sp->cstat, CSTAT_SPRITE_WALL);

                ChangeState(SpriteNum, s_Key[num]);

                RESET(picanm[sp->picnum].sf, PICANM_ANIMTYPE_MASK);
                RESET(picanm[sp->picnum + 1].sf, PICANM_ANIMTYPE_MASK);
                change_sprite_stat(SpriteNum, STAT_ITEM);
                RESET(sp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
                u->Radius = 500;
                sp->hitag = LUMINOUS; //Set so keys over ride colored lighting

                DoActorZrange(SpriteNum);
            }

            break;


#if 0
        case RED_KEY_STATUE:
            num = 0;
            goto KeyStatueMain;
        case BLUE_KEY_STATUE:
            num = 1;
            goto KeyStatueMain;
        case GREEN_KEY_STATUE:
            num = 2;
            goto KeyStatueMain;
        case YELLOW_KEY_STATUE:
            num = 3;
KeyStatueMain:

            u = SpawnUser(SpriteNum, 0, NULL);

            ASSERT(u != NULL);
            sprite[SpriteNum].picnum = u->ID = sprite[SpriteNum].picnum;

            u->spal = sp->pal;
            ChangeState(SpriteNum, s_KeyStatue[num]);

            RESET(picanm[sp->picnum].sf, PICANM_ANIMTYPE_MASK);
            RESET(picanm[sp->picnum + 1].sf, PICANM_ANIMTYPE_MASK);

            change_sprite_stat(SpriteNum, STAT_ITEM);

            DoActorZrange(SpriteNum);
            break;
#endif

        // Used for multiplayer locks
        case 1846:
        case 1850:
        case 1852:
        case 2470:

            if (TEST(sprite[SpriteNum].extra, SPRX_MULTI_ITEM))
                if (numplayers <= 1 || gNet.MultiGameType == MULTI_GAME_COOPERATIVE)
                {
                    KillSprite(SpriteNum);
                }

            break;

        case FIRE_FLY0:

            /*
             * u = SpawnUser(SpriteNum, FIRE_FLY0, NULL);
             *
             * u->State = u->StateStart = &s_FireFly[0]; u->RotNum = 0;
             *
             * sp->ang = 0; sp->xvel = 4;
             *
             * if (labs(sp->z - sector[sp->sectnum].floorz) < Z(32)) sp->z =
             * sector[sp->sectnum].floorz - Z(32);
             *
             * u->sz = sp->z;
             *
             * change_sprite_stat(SpriteNum, STAT_MISC);
             */

            break;

        case ICON_REPAIR_KIT:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_REPAIR_KIT, s_RepairKit);

            IconDefault(SpriteNum);
            break;

        case ICON_STAR:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_STAR, s_IconStar);

            IconDefault(SpriteNum);
            break;

        case ICON_LG_MINE:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_LG_MINE, s_IconLgMine);
            IconDefault(SpriteNum);
            break;


        case ICON_MICRO_GUN:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_MICRO_GUN, s_IconMicroGun);

            IconDefault(SpriteNum);
            break;

        case ICON_MICRO_BATTERY:

NUKE_REPLACEMENT:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_MICRO_BATTERY, s_IconMicroBattery);

            IconDefault(SpriteNum);
            break;

        case ICON_UZI:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_UZI, s_IconUzi);
            IconDefault(SpriteNum);
            break;

        case ICON_UZIFLOOR:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_UZIFLOOR, s_IconUziFloor);
            IconDefault(SpriteNum);
            break;

        case ICON_LG_UZI_AMMO:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_LG_UZI_AMMO, s_IconLgUziAmmo);
            IconDefault(SpriteNum);
            break;

        case ICON_GRENADE_LAUNCHER:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_GRENADE_LAUNCHER, s_IconGrenadeLauncher);

            IconDefault(SpriteNum);
            break;

        case ICON_LG_GRENADE:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_LG_GRENADE, s_IconLgGrenade);
            IconDefault(SpriteNum);
            break;

        case ICON_RAIL_GUN:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_RAIL_GUN, s_IconRailGun);

            IconDefault(SpriteNum);
            break;

        case ICON_RAIL_AMMO:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_RAIL_AMMO, s_IconRailAmmo);
            IconDefault(SpriteNum);
            break;


        case ICON_ROCKET:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_ROCKET, s_IconRocket);

            IconDefault(SpriteNum);
            break;

        case ICON_LG_ROCKET:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_LG_ROCKET, s_IconLgRocket);
            IconDefault(SpriteNum);
            break;

        case ICON_SHOTGUN:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_SHOTGUN, s_IconShotgun);

            u->Radius = 350; // Shotgun is hard to pick up for some reason.

            IconDefault(SpriteNum);
            break;

        case ICON_LG_SHOTSHELL:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_LG_SHOTSHELL, s_IconLgShotshell);
            IconDefault(SpriteNum);
            break;

        case ICON_AUTORIOT:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_AUTORIOT, s_IconAutoRiot);
            IconDefault(SpriteNum);
            break;


        case ICON_GUARD_HEAD:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_GUARD_HEAD, s_IconGuardHead);
            IconDefault(SpriteNum);
            break;

        case ICON_FIREBALL_LG_AMMO:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_FIREBALL_LG_AMMO, s_IconFireballLgAmmo);
            IconDefault(SpriteNum);
            break;

        case ICON_HEART:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_HEART, s_IconHeart);
            IconDefault(SpriteNum);
            break;

        case ICON_HEART_LG_AMMO:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_HEART_LG_AMMO, s_IconHeartLgAmmo);
            IconDefault(SpriteNum);
            break;

#if 0
        case ICON_ELECTRO:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_ELECTRO, s_IconElectro);
            IconDefault(SpriteNum);
            break;
#endif

        case ICON_SPELL:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_SPELL, s_IconSpell);
            IconDefault(SpriteNum);

            PicAnimOff(sp->picnum);
            break;

        case ICON_ARMOR:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_ARMOR, s_IconArmor);
            if (sp->pal != PALETTE_PLAYER3)
                sp->pal = u->spal = PALETTE_PLAYER1;
            else
                sp->pal = u->spal = PALETTE_PLAYER3;
            IconDefault(SpriteNum);
            break;

        case ICON_MEDKIT:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_MEDKIT, s_IconMedkit);
            IconDefault(SpriteNum);
            break;

        case ICON_SM_MEDKIT:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_SM_MEDKIT, s_IconSmMedkit);
            IconDefault(SpriteNum);
            break;

        case ICON_CHEMBOMB:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_CHEMBOMB, s_IconChemBomb);
            IconDefault(SpriteNum);
            break;

        case ICON_FLASHBOMB:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_FLASHBOMB, s_IconFlashBomb);
            IconDefault(SpriteNum);
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

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_NUKE, s_IconNuke);
            IconDefault(SpriteNum);
            break;


        case ICON_CALTROPS:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_CALTROPS, s_IconCaltrops);
            IconDefault(SpriteNum);
            break;

        case ICON_BOOSTER:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_BOOSTER, s_IconBooster);
            IconDefault(SpriteNum);
            break;

        case ICON_HEAT_CARD:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_HEAT_CARD, s_IconHeatCard);
            IconDefault(SpriteNum);
            break;

#if 0
        case ICON_ENVIRON_SUIT:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_ENVIRON_SUIT, s_IconEnvironSuit);
            IconDefault(SpriteNum);
            PicAnimOff(sp->picnum);
            break;
#endif

        case ICON_CLOAK:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_CLOAK, s_IconCloak);
            IconDefault(SpriteNum);
            PicAnimOff(sp->picnum);
            break;

        case ICON_FLY:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_FLY, s_IconFly);
            IconDefault(SpriteNum);
            PicAnimOff(sp->picnum);
            break;

        case ICON_NIGHT_VISION:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_NIGHT_VISION, s_IconNightVision);
            IconDefault(SpriteNum);
            PicAnimOff(sp->picnum);
            break;

        case ICON_FLAG:

            if (!IconSpawn(sp))
            {
                KillSprite(SpriteNum);
                break;
            }

            u = SpawnUser(SpriteNum, ICON_FLAG, s_IconFlag);
            u->spal = sp->pal;
            sector[sp->sectnum].hitag = 9000;       // Put flag's color in sect containing it
            sector[sp->sectnum].lotag = u->spal;
            IconDefault(SpriteNum);
            PicAnimOff(sp->picnum);
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
            u = SpawnUser(SpriteNum, sp->picnum, NULL);

            change_sprite_stat(SpriteNum, STAT_STATIC_FIRE);

            u->ID = FIREBALL_FLAMES;
            u->Radius = 200;
            RESET(sp->cstat, CSTAT_SPRITE_BLOCK);
            RESET(sp->cstat, CSTAT_SPRITE_BLOCK_HITSCAN);

            sp->hitag = LUMINOUS; //Always full brightness
            sp->shade = -40;

            break;
        }

        // blades
        case BLADE1:
        case BLADE2:
        case BLADE3:
        case 5011:
        {
            u = SpawnUser(SpriteNum, sp->picnum, NULL);

            change_sprite_stat(SpriteNum, STAT_DEFAULT);

            RESET(sp->cstat, CSTAT_SPRITE_BLOCK);
            SET(sp->cstat, CSTAT_SPRITE_BLOCK_HITSCAN);
            SET(sp->extra, SPRX_BLADE);

            break;
        }

        case BREAK_LIGHT:
        case BREAK_BARREL:
        case BREAK_PEDISTAL:
        case BREAK_BOTTLE1:
        case BREAK_BOTTLE2:
        case BREAK_MUSHROOM:

            //if (TEST(sp->extra, SPRX_BREAKABLE))
            //    break;

            u = SpawnUser(SpriteNum, sp->picnum, NULL);

            sp->clipdist = SPRITEp_SIZE_X(sp);
            SET(sp->cstat, CSTAT_SPRITE_BREAKABLE);
            SET(sp->extra, SPRX_BREAKABLE);
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
            if (TEST(sp->extra, SPRX_MULTI_ITEM))
            {
                if (numplayers <= 1 || gNet.MultiGameType == MULTI_GAME_COOPERATIVE)
                {
                    KillSprite(SpriteNum);
                    break;
                }
            }


            SET(sp->cstat, CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN);
            break;
        }

        }
    }
}

SWBOOL ItemSpotClear(SPRITEp sip, short statnum, short id)
{
    SWBOOL found = FALSE;
    short i,nexti;

    if (TEST_BOOL2(sip))
    {
        TRAVERSE_SPRITE_SECT(headspritesect[sip->sectnum],i,nexti)
        {
            if (sprite[i].statnum == statnum && User[i]->ID == id)
            {
                found = TRUE;
                break;
            }
        }
    }

    return !found;
}

void SetupItemForJump(SPRITEp sip, short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    // setup item for jumping
    if (SP_TAG7(sip))
    {
        change_sprite_stat(SpriteNum, STAT_SKIP4);
        u->ceiling_dist = Z(6);
        u->floor_dist = Z(0);
        u->Counter = 0;

        sp->xvel = (int)SP_TAG7(sip)<<2;
        sp->zvel = -(((int)SP_TAG8(sip))<<5);

        u->xchange = MOVEx(sp->xvel, sp->ang);
        u->ychange = MOVEy(sp->xvel, sp->ang);
        u->zchange = sp->zvel;
    }
}

int ActorCoughItem(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    short New,choose;
    SPRITEp np;


    switch (u->ID)
    {
    case SAILORGIRL_R0:
        ASSERT(sp->sectnum >= 0);
        New = COVERinsertsprite(sp->sectnum, STAT_SPAWN_ITEMS);
        ASSERT(New >= 0);
        np = &sprite[New];
        np->cstat = np->extra = 0;
        np->x = sp->x;
        np->y = sp->y;
        np->z = SPRITEp_MID(sp);
        np->ang = 0;
        np->extra = 0;

        // vel
        SP_TAG7(np) = 1;
        // zvel
        SP_TAG8(np) = 40;

        choose = RANDOM_P2(1024);

        if (choose > 854)
            SP_TAG3(np) = 91; // Match number
        else if (choose > 684)
            SP_TAG3(np) = 48; // Match number
        else if (choose > 514)
            SP_TAG3(np) = 58; // Match number
        else if (choose > 344)
            SP_TAG3(np) = 60; // Match number
        else if (choose > 174)
            SP_TAG3(np) = 62; // Match number
        else
            SP_TAG3(np) = 68; // Match number

        // match
        SP_TAG2(np) = -1;
        // kill
        RESET_BOOL1(np);
        SpawnItemsMatch(-1);
        break;

    case GORO_RUN_R0:
        if (RANDOM_P2(1024) < 700)
            return 0;

        ASSERT(sp->sectnum >= 0);
        New = COVERinsertsprite(sp->sectnum, STAT_SPAWN_ITEMS);
        ASSERT(New >= 0);
        np = &sprite[New];
        np->cstat = np->extra = 0;
        np->x = sp->x;
        np->y = sp->y;
        np->z = SPRITEp_MID(sp);
        np->ang = 0;
        np->extra = 0;

        // vel
        SP_TAG7(np) = 1;
        // zvel
        SP_TAG8(np) = 40;

        SP_TAG3(np) = 69; // Match number

        // match
        SP_TAG2(np) = -1;
        // kill
        RESET_BOOL1(np);
        SpawnItemsMatch(-1);
        break;

    case RIPPER2_RUN_R0:
        if (RANDOM_P2(1024) < 700)
            return 0;

        ASSERT(sp->sectnum >= 0);
        New = COVERinsertsprite(sp->sectnum, STAT_SPAWN_ITEMS);
        ASSERT(New >= 0);
        np = &sprite[New];
        np->cstat = np->extra = 0;
        np->x = sp->x;
        np->y = sp->y;
        np->z = SPRITEp_MID(sp);
        np->ang = 0;
        np->extra = 0;

        // vel
        SP_TAG7(np) = 1;
        // zvel
        SP_TAG8(np) = 40;

        SP_TAG3(np) = 70; // Match number

        // match
        SP_TAG2(np) = -1;
        // kill
        RESET_BOOL1(np);
        SpawnItemsMatch(-1);
        break;

    case NINJA_RUN_R0:

        if (u->PlayerP)
        {
            if (RANDOM_P2(1024) > 200)
                return 0;

            ASSERT(sp->sectnum >= 0);
            New = COVERinsertsprite(sp->sectnum, STAT_SPAWN_ITEMS);
            ASSERT(New >= 0);
            np = &sprite[New];
            np->cstat = 0;
            np->extra = 0;
            np->x = sp->x;
            np->y = sp->y;
            np->z = SPRITEp_MID(sp);
            np->ang = 0;
            np->extra = 0;

            // vel
            SP_TAG7(np) = 1;
            // zvel
            SP_TAG8(np) = 40;

            switch (u->WeaponNum)
            {
            case WPN_UZI:
                SP_TAG3(np) = 0;
                break;
            case WPN_SHOTGUN:
                SP_TAG3(np) = 51;
                break;
            case WPN_STAR:
                if (u->PlayerP->WpnAmmo[WPN_STAR] < 9)
                    break;
                SP_TAG3(np) = 41;
                break;
            case WPN_MINE:
                if (u->PlayerP->WpnAmmo[WPN_MINE] < 5)
                    break;
                SP_TAG3(np) = 42;
                break;
            case WPN_MICRO:
            case WPN_ROCKET:
                SP_TAG3(np) = 43;
                break;
            case WPN_GRENADE:
                SP_TAG3(np) = 45;
                break;
            case WPN_RAIL:
                SP_TAG3(np) = 47;
                break;
            case WPN_HEART:
                SP_TAG3(np) = 55;
                break;
            case WPN_HOTHEAD:
                SP_TAG3(np) = 53;
                break;
            }

            // match
            SP_TAG2(np) = -1;
            // kill
            RESET_BOOL1(np);
            SpawnItemsMatch(-1);
            break;
        }

        if (RANDOM_P2(1024) < 512)
            return 0;

        ASSERT(sp->sectnum >= 0);
        New = COVERinsertsprite(sp->sectnum, STAT_SPAWN_ITEMS);
        ASSERT(New >= 0);
        np = &sprite[New];
        np->cstat = np->extra = 0;
        np->x = sp->x;
        np->y = sp->y;
        np->z = SPRITEp_MID(sp);
        np->ang = 0;
        np->extra = 0;

        // vel
        SP_TAG7(np) = 1;
        // zvel
        SP_TAG8(np) = 40;

        if (u->spal == PAL_XLAT_LT_TAN)
        {
            SP_TAG3(np) = 44;
        }
        else if (u->spal == PAL_XLAT_LT_GREY)
        {
            SP_TAG3(np) = 46;
        }
        else if (u->spal == PALETTE_PLAYER5) // Green Ninja
        {
            if (RANDOM_P2(1024) < 700)
                SP_TAG3(np) = 61;
            else
                SP_TAG3(np) = 60;
        }
        else if (u->spal == PALETTE_PLAYER3) // Red Ninja
        {
            // type
            if (RANDOM_P2(1024) < 800)
                SP_TAG3(np) = 68;
            else
                SP_TAG3(np) = 44;
        }
        else
        {
            if (RANDOM_P2(1024) < 512)
                SP_TAG3(np) = 41;
            else
                SP_TAG3(np) = 68;
        }

        // match
        SP_TAG2(np) = -1;
        // kill
        RESET_BOOL1(np);
        SpawnItemsMatch(-1);
        break;

    case PACHINKO1:
    case PACHINKO2:
    case PACHINKO3:
    case PACHINKO4:

        ASSERT(sp->sectnum >= 0);
        New = COVERinsertsprite(sp->sectnum, STAT_SPAWN_ITEMS);
        ASSERT(New >= 0);
        np = &sprite[New];
        np->cstat = np->extra = 0;
        np->x = sp->x;
        np->y = sp->y;
        np->z = SPRITEp_LOWER(sp)+Z(10);
        np->ang = sp->ang;

        // vel
        SP_TAG7(np) = 10;
        // zvel
        SP_TAG8(np) = 10;

        if (u->ID == PACHINKO1)
        {
            if (RANDOM_P2(1024) < 600)
                SP_TAG3(np) = 64; // Small MedKit
            else
                SP_TAG3(np) = 59; // Fortune Cookie
        }
        else if (u->ID == PACHINKO2)
        {
            if (RANDOM_P2(1024) < 600)
                SP_TAG3(np) = 52; // Lg Shot Shell
            else
                SP_TAG3(np) = 68; // Uzi clip
        }
        else if (u->ID == PACHINKO3)
        {
            if (RANDOM_P2(1024) < 600)
                SP_TAG3(np) = 57;
            else
                SP_TAG3(np) = 63;
        }
        else if (u->ID == PACHINKO4)
        {
            if (RANDOM_P2(1024) < 600)
                SP_TAG3(np) = 60;
            else
                SP_TAG3(np) = 61;
        }

        // match
        SP_TAG2(np) = -1;
        // kill
        RESET_BOOL1(np);
        SpawnItemsMatch(-1);
        break;
    }

    return 0;
}

int SpawnItemsMatch(short match)
{
    short SpriteNum;
    short si, nextsi;
    SPRITEp sp,sip;
    SWBOOL found;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_SPAWN_ITEMS],si,nextsi)
    {
        sip = &sprite[si];

        if (SP_TAG2(sip) != match)
            continue;

        switch (SP_TAG3(sip))
        {
        case 90:
            SpriteNum = BunnyHatch2(si);
            sp = &sprite[SpriteNum];
            User[SpriteNum]->spal = sp->pal = PALETTE_PLAYER8; // Boy
            sp->ang = sip->ang;
            break;
        case 91:
            SpriteNum = BunnyHatch2(si);
            sp = &sprite[SpriteNum];
            User[SpriteNum]->spal = sp->pal = PALETTE_PLAYER0; // Girl
            sp->ang = sip->ang;
            break;
        case 92:
            SpriteNum = BunnyHatch2(si);
            sp = &sprite[SpriteNum];
            sp->ang = sip->ang;
            break;

        case 40:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_REPAIR_KIT))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_REPAIR_KIT, s_RepairKit, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 41:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_STAR))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_STAR, s_IconStar, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 42:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_LG_MINE))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_LG_MINE, s_IconLgMine, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 43:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_MICRO_GUN))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_MICRO_GUN, s_IconMicroGun, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 44:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_MICRO_BATTERY))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_MICRO_BATTERY, s_IconMicroBattery, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 45:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_GRENADE_LAUNCHER))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_GRENADE_LAUNCHER, s_IconGrenadeLauncher, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 46:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_LG_GRENADE))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_LG_GRENADE, s_IconLgGrenade, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 47:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_RAIL_GUN))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_RAIL_GUN, s_IconRailGun, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 48:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_RAIL_AMMO))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_RAIL_AMMO, s_IconRailAmmo, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 49:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_ROCKET))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_ROCKET, s_IconRocket, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 51:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_SHOTGUN))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_SHOTGUN, s_IconShotgun, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 52:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_LG_SHOTSHELL))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_LG_SHOTSHELL, s_IconLgShotshell, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 53:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_GUARD_HEAD))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_GUARD_HEAD, s_IconGuardHead, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 54:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_FIREBALL_LG_AMMO))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_FIREBALL_LG_AMMO, s_IconFireballLgAmmo, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 55:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_HEART))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_HEART, s_IconHeart, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 56:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_HEART_LG_AMMO))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_HEART_LG_AMMO, s_IconHeartLgAmmo, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 57:
        {
            USERp u;
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_ARMOR))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_ARMOR, s_IconArmor, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            sp = &sprite[SpriteNum];
            u = User[SpriteNum];
            SET(u->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);

            if (sp->pal != PALETTE_PLAYER3)
                sp->pal = u->spal = PALETTE_PLAYER1;
            else
                sp->pal = u->spal = PALETTE_PLAYER3;
            break;
        }

        case 58:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_MEDKIT))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_MEDKIT, s_IconMedkit, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 59:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_SM_MEDKIT))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_SM_MEDKIT, s_IconSmMedkit, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 60:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_CHEMBOMB))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_CHEMBOMB, s_IconChemBomb, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 61:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_FLASHBOMB))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_FLASHBOMB, s_IconFlashBomb, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 62:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_NUKE))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_NUKE, s_IconNuke, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 63:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_CALTROPS))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_CALTROPS, s_IconCaltrops, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 64:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_BOOSTER))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_BOOSTER, s_IconBooster, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 65:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_HEAT_CARD))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_HEAT_CARD, s_IconHeatCard, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 66:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_CLOAK))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_CLOAK, s_IconCloak, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 67:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_NIGHT_VISION))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_NIGHT_VISION, s_IconNightVision, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;


        case 68:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_LG_UZI_AMMO))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_LG_UZI_AMMO, s_IconLgUziAmmo, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 69:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_GUARD_HEAD))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_GUARD_HEAD, s_IconGuardHead, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 70:
            if (!ItemSpotClear(sip, STAT_ITEM, ICON_HEART))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_HEART, s_IconHeart, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;

        case 20:

            if (!ItemSpotClear(sip, STAT_ITEM, ICON_UZIFLOOR))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_UZIFLOOR, s_IconUziFloor, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
            break;


        case 32:
        case 0:

            if (!ItemSpotClear(sip, STAT_ITEM, ICON_UZI))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, ICON_UZI, s_IconUzi, sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            SET(User[SpriteNum]->Flags2, SPR2_NEVER_RESPAWN);
            IconDefault(SpriteNum);

            SetupItemForJump(sip, SpriteNum);
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
            USERp u;

            uint8_t KeyPal[] =
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

            num = SP_TAG3(sip) - 1;

            if (!ItemSpotClear(sip, STAT_ITEM, s_Key[num]->Pic))
                break;

            SpriteNum = SpawnSprite(STAT_ITEM, s_Key[num]->Pic, s_Key[num], sip->sectnum, sip->x, sip->y, sip->z, sip->ang, 0);
            u = User[SpriteNum];

            sp = &sprite[SpriteNum];

            ASSERT(u != NULL);
            sprite[SpriteNum].picnum = u->ID = s_Key[num]->Pic;


            // need to set the palette here - suggest table lookup
            u->spal = sprite[SpriteNum].pal = KeyPal[num];
            //SET(sp->cstat, CSTAT_SPRITE_WALL);


            ChangeState(SpriteNum, s_Key[num]);

            RESET(picanm[sp->picnum].sf, PICANM_ANIMTYPE_MASK);
            RESET(picanm[sp->picnum + 1].sf, PICANM_ANIMTYPE_MASK);

            SetupItemForJump(sip, SpriteNum);

            break;
        }
        }

        if (!TEST_BOOL1(sip))
            KillSprite(si);
    }
    return 0;
}

// CTW MODIFICATION
//void
int
// CTW MODIFICATION END
NewStateGroup(short SpriteNum, STATEp StateGroup[])
{
    USERp u = User[SpriteNum];
    int i;

    //if (Prediction)
    //    return;

    if (!StateGroup)
        return 0;

    ASSERT(u);

    // Kind of a goofy check, but it should catch alot of invalid states!
    // BTW, 6144 is the max tile number allowed in editart.
    if (u->State && (u->State->Pic < 0 || u->State->Pic > 6144))    // JBF: verify this!
        return 0;

    u->Rot = StateGroup;
    u->State = u->StateStart = StateGroup[0];
    //sprite[SpriteNum].picnum = u->State->Pic;

    u->Tics = 0;

    // turn anims off because people keep setting them in the
    // art file
    RESET(picanm[sprite[SpriteNum].picnum].sf, PICANM_ANIMTYPE_MASK);
    return 0;
}


SWBOOL
SpriteOverlap(int16_t spritenum_a, int16_t spritenum_b)
{
    SPRITEp spa = &sprite[spritenum_a], spb = &sprite[spritenum_b];

    USERp ua = User[spritenum_a];
    USERp ub = User[spritenum_b];

    int spa_tos, spa_bos, spb_tos, spb_bos, overlap_z;

    if (!ua || !ub) return FALSE;
    if ((unsigned)Distance(spa->x, spa->y, spb->x, spb->y) > ua->Radius + ub->Radius)
    {
        return FALSE;
    }

    spa_tos = SPRITEp_TOS(spa);
    spa_bos = SPRITEp_BOS(spa);

    spb_tos = SPRITEp_TOS(spb);
    spb_bos = SPRITEp_BOS(spb);


    overlap_z = ua->OverlapZ + ub->OverlapZ;

    // if the top of sprite a is below the bottom of b
    if (spa_tos - overlap_z > spb_bos)
    {
        return FALSE;
    }

    // if the top of sprite b is is below the bottom of a
    if (spb_tos - overlap_z > spa_bos)
    {
        return FALSE;
    }

    return TRUE;

}

SWBOOL
SpriteOverlapZ(int16_t spritenum_a, int16_t spritenum_b, int z_overlap)
{
    SPRITEp spa = &sprite[spritenum_a], spb = &sprite[spritenum_b];

    USERp ua = User[spritenum_a];
    USERp ub = User[spritenum_b];

    int spa_tos, spa_bos, spb_tos, spb_bos;

    spa_tos = SPRITEp_TOS(spa);
    spa_bos = SPRITEp_BOS(spa);

    spb_tos = SPRITEp_TOS(spb);
    spb_bos = SPRITEp_BOS(spb);


    // if the top of sprite a is below the bottom of b
    if (spa_tos + z_overlap > spb_bos)
    {
        return FALSE;
    }

    // if the top of sprite b is is below the bottom of a
    if (spb_tos + z_overlap > spa_bos)
    {
        return FALSE;
    }

    return TRUE;

}

void
getzrangepoint(int x, int y, int z, short sectnum,
               int32_t* ceilz, int32_t* ceilhit, int32_t* florz, int32_t* florhit)
{
    spritetype *spr;
    int i, j, k, l, dax, day, daz, xspan, yspan, xoff, yoff;
    int x1, y1, x2, y2, x3, y3, x4, y4, cosang, sinang, tilenum;
    short cstat;
    char clipyou;

    if (sectnum < 0)
    {
        *ceilz = 0x80000000;
        *ceilhit = -1;
        *florz = 0x7fffffff;
        *florhit = -1;
        return;
    }

    // Initialize z's and hits to the current sector's top&bottom
    getzsofslope(sectnum, x, y, ceilz, florz);
    *ceilhit = sectnum + 16384;
    *florhit = sectnum + 16384;

    // Go through sprites of only the current sector
    for (j = headspritesect[sectnum]; j >= 0; j = nextspritesect[j])
    {
        spr = &sprite[j];
        cstat = spr->cstat;
        if ((cstat & 49) != 33)
            continue;                   // Only check blocking floor sprites

        daz = spr->z;

        // Only check if sprite's 2-sided or your on the 1-sided side
        if (((cstat & 64) != 0) && ((z > daz) == ((cstat & 8) == 0)))
            continue;

        // Calculate and store centering offset information into xoff&yoff
        tilenum = spr->picnum;
        xoff = (int)picanm[tilenum].xofs + (int)spr->xoffset;
        yoff = (int)picanm[tilenum].yofs + (int)spr->yoffset;
        if (cstat & 4)
            xoff = -xoff;
        if (cstat & 8)
            yoff = -yoff;

        // Calculate all 4 points of the floor sprite.
        // (x1,y1),(x2,y2),(x3,y3),(x4,y4)
        // These points will already have (x,y) subtracted from them
        cosang = sintable[NORM_ANGLE(spr->ang + 512)];
        sinang = sintable[spr->ang];
        xspan = tilesiz[tilenum].x;
        dax = ((xspan >> 1) + xoff) * spr->xrepeat;
        yspan = tilesiz[tilenum].y;
        day = ((yspan >> 1) + yoff) * spr->yrepeat;
        x1 = spr->x + dmulscale16(sinang, dax, cosang, day) - x;
        y1 = spr->y + dmulscale16(sinang, day, -cosang, dax) - y;
        l = xspan * spr->xrepeat;
        x2 = x1 - mulscale16(sinang, l);
        y2 = y1 + mulscale16(cosang, l);
        l = yspan * spr->yrepeat;
        k = -mulscale16(cosang, l);
        x3 = x2 + k;
        x4 = x1 + k;
        k = -mulscale16(sinang, l);
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
                *ceilhit = j + 49152;
            }
        }
        else
        {
            if (daz < *florz)
            {
                *florz = daz;
                *florhit = j + 49152;
            }
        }
    }
}


void
DoActorZrange(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    int ceilhit, florhit;
    short save_cstat;

    save_cstat = TEST(sp->cstat, CSTAT_SPRITE_BLOCK);
    RESET(sp->cstat, CSTAT_SPRITE_BLOCK);
    FAFgetzrange(sp->x, sp->y, sp->z - DIV2(SPRITEp_SIZE_Z(sp)), sp->sectnum, &u->hiz, &ceilhit, &u->loz, &florhit, (((int) sp->clipdist) << 2) - GETZRANGE_CLIP_ADJ, CLIPMASK_ACTOR);
    SET(sp->cstat, save_cstat);

    u->lo_sectp = u->hi_sectp = NULL;
    u->lo_sp = u->hi_sp = NULL;

    switch (TEST(ceilhit, HIT_MASK))
    {
    case HIT_SPRITE:
        u->hi_sp = &sprite[NORM_SPRITE(ceilhit)];
        break;
    case HIT_SECTOR:
        u->hi_sectp = &sector[NORM_SECTOR(ceilhit)];
        break;
    default:
        ASSERT(TRUE==FALSE);
        break;
    }

    switch (TEST(florhit, HIT_MASK))
    {
    case HIT_SPRITE:
        u->lo_sp = &sprite[NORM_SPRITE(florhit)];
        break;
    case HIT_SECTOR:
        u->lo_sectp = &sector[NORM_SECTOR(florhit)];
        break;
    default:
        ASSERT(TRUE==FALSE);
        break;
    }
}

// !AIC - puts getzrange results into USER varaible u->loz, u->hiz, u->lo_sectp, u->hi_sectp, etc.
// The loz and hiz are used a lot.

int
DoActorGlobZ(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];

    u->loz = globloz;
    u->hiz = globhiz;

    u->lo_sectp = u->hi_sectp = NULL;
    u->lo_sp = u->hi_sp = NULL;

    switch (TEST(globhihit, HIT_MASK))
    {
    case HIT_SPRITE:
        u->hi_sp = &sprite[globhihit & 4095];
        break;
    default:
        u->hi_sectp = &sector[globhihit & 4095];
        break;
    }

    switch (TEST(globlohit, HIT_MASK))
    {
    case HIT_SPRITE:
        u->lo_sp = &sprite[globlohit & 4095];
        break;
    default:
        u->lo_sectp = &sector[globlohit & 4095];
        break;
    }

    return 0;
}


SWBOOL
ActorDrop(short SpriteNum, int x, int y, int z, short new_sector, short min_height)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    int ceilhit, florhit, hiz, loz;
    short save_cstat;

    // look only at the center point for a floor sprite
    save_cstat = TEST(sp->cstat, CSTAT_SPRITE_BLOCK);
    RESET(sp->cstat, CSTAT_SPRITE_BLOCK);
    FAFgetzrangepoint(x, y, z - DIV2(SPRITEp_SIZE_Z(sp)), new_sector, &hiz, &ceilhit, &loz, &florhit);
    SET(sp->cstat, save_cstat);

#if 0
    if (florhit < 0 || ceilhit < 0)
    {
        TerminateGame();
        printf("ERROR: FAFgetzrange() returned -1 for floor or ceiling check.\n");
        printf("Most likely a sprite has been placed too close to a white wall.\n");
        printf("spnum %d, sect %d, x %d, y %d, z %d, florhit %d, pic %d\n", SpriteNum, sp->sectnum, sp->x, sp->y, z - DIV2(SPRITEp_SIZE_Z(sp)), florhit, sp->picnum);
        exit(0);
    }
#else
    if (florhit < 0 || ceilhit < 0)
    {
        //SetSuicide(SpriteNum);
        return TRUE;
    }
#endif


    // ASSERT(florhit >= 0);
    // ASSERT(ceilhit >= 0);

    switch (TEST(florhit, HIT_MASK))
    {
    case HIT_SPRITE:
    {
        SPRITEp hsp = &sprite[florhit & 4095];

        // if its a floor sprite and not too far down
        if (TEST(hsp->cstat, CSTAT_SPRITE_FLOOR) &&
            (labs(loz - z) <= min_height))
        {
            return FALSE;
        }

        break;
    }

    case HIT_SECTOR:
    {
        SECTORp sectp = &sector[florhit & 4095];

        if (labs(loz - z) <= min_height)
        {
            return FALSE;
        }

        break;
    }
    default:
        ASSERT(TRUE == FALSE);
        break;
    }

    return TRUE;
}

// Primarily used in ai.c for now - need to get rid of
SWBOOL
DropAhead(short SpriteNum, short min_height)
{

    SPRITEp sp = &sprite[SpriteNum];
    int dax, day;
    short newsector;

    // dax = sp->x + MOVEx(128, sp->ang);
    // day = sp->y + MOVEy(128, sp->ang);

    dax = sp->x + MOVEx(256, sp->ang);
    day = sp->y + MOVEy(256, sp->ang);

    newsector = sp->sectnum;
    COVERupdatesector(dax, day, &newsector);

    // look straight down for a drop
    if (ActorDrop(SpriteNum, dax, day, sp->z, newsector, min_height))
        return TRUE;

    return FALSE;
}

/*

  !AIC KEY - Called by ai.c routines.  Calls move_sprite which calls clipmove.
  This incapulates move_sprite and makes sure that actors don't walk off of
  ledges.  If it finds itself in mid air then it restores the last good
  position.  This is a hack because Ken had no good way of doing this from his
  code.  ActorDrop() is called from here.

*/

int
move_actor(short SpriteNum, int xchange, int ychange, int zchange)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    int x, y, z, loz, hiz;
    SPRITEp lo_sp, hi_sp;
    SECTORp lo_sectp, hi_sectp;
    short sectnum,sect;
    short dist;
    int cliptype = CLIPMASK_ACTOR;


    if (TEST(u->Flags, SPR_NO_SCAREDZ))
    {
        // For COOLG & HORNETS
        // set to actual z before you move
        sp->z = u->sz;
    }

    // save off x,y values
    x = sp->x;
    y = sp->y;
    z = sp->z;
    loz = u->loz;
    hiz = u->hiz;
    lo_sp = u->lo_sp;
    hi_sp = u->hi_sp;
    lo_sectp = u->lo_sectp;
    hi_sectp = u->hi_sectp;
    sectnum = sp->sectnum;

    clipmoveboxtracenum = 1;
    u->ret = move_sprite(SpriteNum, xchange, ychange, zchange,
                         u->ceiling_dist, u->floor_dist, cliptype, ACTORMOVETICS);
    clipmoveboxtracenum = 3;

    ASSERT(sp->sectnum >= 0);

    // try and determine whether you moved > lo_step in the z direction
    if (!TEST(u->Flags, SPR_NO_SCAREDZ | SPR_JUMPING | SPR_CLIMBING | SPR_FALLING | SPR_DEAD | SPR_SWIMMING))
    {
        if (labs(sp->z - globloz) > u->lo_step)
        {
            // cancel move
            sp->x = x;
            sp->y = y;
            sp->z = z;
            //sp->z = u->loz;             // place on ground in case you are in the air
            u->loz = loz;
            u->hiz = hiz;
            u->lo_sp = lo_sp;
            u->hi_sp = hi_sp;
            u->lo_sectp = lo_sectp;
            u->hi_sectp = hi_sectp;
            u->ret = -1;
            changespritesect(SpriteNum, sectnum);
            return FALSE;
        }

        if (ActorDrop(SpriteNum, sp->x, sp->y, sp->z, sp->sectnum, u->lo_step))
        {
            //printf("cancel move 2\n", sp->z, u->loz);
            // cancel move
            sp->x = x;
            sp->y = y;
            sp->z = z;
            //sp->z = u->loz;             // place on ground in case you are in the air
            u->loz = loz;
            u->hiz = hiz;
            u->lo_sp = lo_sp;
            u->hi_sp = hi_sp;
            u->lo_sectp = lo_sectp;
            u->hi_sectp = hi_sectp;
            u->ret = -1;
            changespritesect(SpriteNum, sectnum);
            return FALSE;
        }
    }

    SET(u->Flags, SPR_MOVED);

    if (!u->ret)
    {
        // Keep track of how far sprite has moved
        dist = Distance(x, y, sp->x, sp->y);
        u->TargetDist -= dist;
        u->Dist += dist;
        u->DistCheck += dist;
        return TRUE;
    }
    else
    {
        return FALSE;
    }

}

int
DoStayOnFloor(short SpriteNum)
{
    sprite[SpriteNum].z = sector[sprite[SpriteNum].sectnum].floorz;
    //sprite[SpriteNum].z = getflorzofslope(sprite[SpriteNum].sectnum, sprite[SpriteNum].x, sprite[SpriteNum].y);
    return 0;
}

int
DoGrating(short SpriteNum)
{
    SPRITEp sp = User[SpriteNum]->SpriteP;
    int16_t x, y;
    int dir;
#define GRATE_FACTOR 3

    // reduce to 0 to 3 value
    dir = sp->ang >> 9;

    if (MOD2(dir) == 0)
    {
        if (dir == 0)
            sp->x += 2 * GRATE_FACTOR;
        else
            sp->x -= 2 * GRATE_FACTOR;
    }
    else
    {
        if (dir == 1)
            sp->y += 2 * GRATE_FACTOR;
        else
            sp->y -= 2 * GRATE_FACTOR;
    }

    sp->hitag -= GRATE_FACTOR;

    if (sp->hitag <= 0)
    {
        change_sprite_stat(SpriteNum, STAT_DEFAULT);
        if (User[SpriteNum])
        {
            FreeMem(User[SpriteNum]);
            User[SpriteNum] = 0;
        }
    }

    setspritez(SpriteNum, (vec3_t *)sp);

    return 0;
}

#if 0
int
DoSpriteFade(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    short i;

    // adjust Shade based on clock

    for (i = 0; i < ACTORMOVETICS; i++)
    {
        if (TEST(u->Flags, SPR_SHADE_DIR))
        {
            sp->shade++;

            if (sp->shade >= 10)
                RESET(u->Flags, SPR_SHADE_DIR);
        }
        else
        {
            sp->shade--;

            if (sp->shade <= -40)
                SET(u->Flags, SPR_SHADE_DIR);
        }
    }
    return 0;
}
#endif

int
SpearOnFloor(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    if (!TEST(u->Flags, SPR_SO_ATTACHED))
    {
        // if on a sprite bridge, stay with the sprite otherwize stay with
        // the floor
        if (u->lo_sp)
            sp->z = u->loz;
        else
            sp->z = sector[sp->sectnum].floorz + u->sz;
    }
    return 0;
}

int
SpearOnCeiling(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    if (!TEST(u->Flags, SPR_SO_ATTACHED))
    {
        // if on a sprite bridge, stay with the sprite otherwize stay with
        // the floor
        if (u->hi_sp)
            sp->z = u->hiz;
        else
            sp->z = sector[sp->sectnum].ceilingz + u->sz;
    }
    return 0;
}

int
DoKey(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;

    sp->ang = NORM_ANGLE(sp->ang + (14 * ACTORMOVETICS));

    //DoSpriteFade(SpriteNum);

    DoGet(SpriteNum);
    return 0;
}

int
DoCoin(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = User[SpriteNum]->SpriteP;
    int offset;

    u->WaitTics -= ACTORMOVETICS * 2;

    if (u->WaitTics <= 0)
    {
        KillSprite(SpriteNum);
        return 0;
    }

    if (u->WaitTics < 10*120)
    {
        if (u->StateStart != s_GreenCoin)
        {
            offset = u->State - u->StateStart;
            ChangeState(SpriteNum, s_GreenCoin);
            u->State = u->StateStart + offset;
        }
    }
    else if (u->WaitTics < 20*120)
    {
        if (u->StateStart != s_YellowCoin)
        {
            offset = u->State - u->StateStart;
            ChangeState(SpriteNum, s_YellowCoin);
            u->State = u->StateStart + offset;
        }
    }

    return 0;
}

int
KillGet(short SpriteNum)
{
    USERp u = User[SpriteNum],nu;
    SPRITEp sp = User[SpriteNum]->SpriteP,np;

    short New;

    switch (gNet.MultiGameType)
    {
    case MULTI_GAME_NONE:
    case MULTI_GAME_COOPERATIVE:
        KillSprite(SpriteNum);
        break;
    case MULTI_GAME_COMMBAT:
    case MULTI_GAME_AI_BOTS:

        if (TEST(u->Flags2, SPR2_NEVER_RESPAWN))
        {
            KillSprite(SpriteNum);
            break;
        }

        u->WaitTics = 30*120;
        SET(sp->cstat, CSTAT_SPRITE_INVISIBLE);

        // respawn markers
        if (!gNet.SpawnMarkers || sp->hitag == TAG_NORESPAWN_FLAG)  // No coin if it's a special flag
            break;

        New = SpawnSprite(STAT_ITEM, Red_COIN, s_RedCoin, sp->sectnum,
                          sp->x, sp->y, sp->z, 0, 0);

        np = &sprite[New];
        nu = User[New];

        np->shade = -20;
        nu->WaitTics = u->WaitTics - 12;

        break;
    }
    return 0;
}

int
KillGetAmmo(short SpriteNum)
{
    USERp u = User[SpriteNum],nu;
    SPRITEp sp = User[SpriteNum]->SpriteP,np;

    short New;

    switch (gNet.MultiGameType)
    {
    case MULTI_GAME_NONE:
    case MULTI_GAME_COOPERATIVE:
        KillSprite(SpriteNum);
        break;

    case MULTI_GAME_COMMBAT:
    case MULTI_GAME_AI_BOTS:

        if (TEST(u->Flags2, SPR2_NEVER_RESPAWN))
        {
            KillSprite(SpriteNum);
            break;
        }

        // No Respawn mode - all ammo goes away
        if (gNet.NoRespawn)
        {
            KillSprite(SpriteNum);
            break;
        }

        u->WaitTics = 30*120;
        SET(sp->cstat, CSTAT_SPRITE_INVISIBLE);

        // respawn markers
        if (!gNet.SpawnMarkers)
            break;

        New = SpawnSprite(STAT_ITEM, Red_COIN, s_RedCoin, sp->sectnum,
                          sp->x, sp->y, sp->z, 0, 0);

        np = &sprite[New];
        nu = User[New];

        np->shade = -20;
        nu->WaitTics = u->WaitTics - 12;

        break;
    }
    return 0;
}

int
KillGetWeapon(short SpriteNum)
{
    USERp u = User[SpriteNum],nu;
    SPRITEp sp = User[SpriteNum]->SpriteP,np;

    short New;

    switch (gNet.MultiGameType)
    {
    case MULTI_GAME_NONE:
        KillSprite(SpriteNum);
        break;

    case MULTI_GAME_COOPERATIVE:
        // don't kill weapons in coop

        // unless told too :)
        if (TEST(u->Flags2, SPR2_NEVER_RESPAWN))
        {
            KillSprite(SpriteNum);
            break;
        }
        break;

    case MULTI_GAME_COMMBAT:
    case MULTI_GAME_AI_BOTS:

        if (TEST(u->Flags2, SPR2_NEVER_RESPAWN))
        {
            KillSprite(SpriteNum);
            break;
        }

        // No Respawn mode - all weapons stay
        // but can only get once
        if (gNet.NoRespawn)
            break;

        u->WaitTics = 30*120;
        SET(sp->cstat, CSTAT_SPRITE_INVISIBLE);

        // respawn markers
        if (!gNet.SpawnMarkers)
            break;

        New = SpawnSprite(STAT_ITEM, Red_COIN, s_RedCoin, sp->sectnum,
                          sp->x, sp->y, sp->z, 0, 0);

        np = &sprite[New];
        nu = User[New];

        np->shade = -20;
        nu->WaitTics = u->WaitTics - 12;

        break;
    }
    return 0;
}

int
DoSpawnItemTeleporterEffect(SPRITEp sp)
{
    extern STATE s_TeleportEffect[];
    short effect;
    USERp eu;
    SPRITEp ep;

    effect = SpawnSprite(STAT_MISSILE, 0, s_TeleportEffect, sp->sectnum,
                         sp->x, sp->y, sp->z - Z(12),
                         sp->ang, 0);

    ep = &sprite[effect];
    eu = User[effect];

    ep->shade = -40;
    ep->xrepeat = ep->yrepeat = 36;
    SET(ep->cstat, CSTAT_SPRITE_YCENTER);
    RESET(ep->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
    return 0;
}

void ChoosePlayerGetSound(PLAYERp pp)
{
    int choose_snd=0;

    if (pp != Player+myconnectindex) return;

    choose_snd = STD_RANDOM_RANGE((MAX_GETSOUNDS-1)<<8)>>8;

    PlayerSound(PlayerGetItemVocs[choose_snd],&pp->posx,&pp->posy,&pp->posz,v3df_follow|v3df_dontpan,pp);
}

//#define MAX_FORTUNES 16
// With PLOCK on, max = 11
char *ReadFortune[MAX_FORTUNES] =
{
    "You never going to score.",
    "26-31-43-82-16-29",
    "Sorry, you no win this time, try again.",
    "You try harder get along. Be a nice man.",
    "No man is island, except Lo Wang.",
    "There is much death in future.",
    "You should kill all business associates.",
    "(c)1997,3DRealms fortune cookie company.",
    "Your chi attracts many chicks.",
    "Don't you know you the scum of society!?",
    "You should not scratch yourself there.",
    "Man who stand on toilet, high on pot.",
    "Man who fart in church sit in own pew.",
    "Man trapped in pantry has ass in jam.",
    "Baseball wrong.  Man with 4 balls cannot walk.",
    "Man who buy drowned cat pay for wet pussy.",
};


SWBOOL CanGetWeapon(PLAYERp pp, short SpriteNum, int WPN)
{
    USERp u = User[SpriteNum], pu;
    SPRITEp sp = User[SpriteNum]->SpriteP;

    switch (gNet.MultiGameType)
    {
    case MULTI_GAME_NONE:
        return TRUE;

    case MULTI_GAME_COOPERATIVE:
        if (TEST(u->Flags2, SPR2_NEVER_RESPAWN))
            return TRUE;

        if (TEST(pp->WpnGotOnceFlags, BIT(WPN)))
            return FALSE;

        return TRUE;

    case MULTI_GAME_COMMBAT:
    case MULTI_GAME_AI_BOTS:

        if (TEST(u->Flags2, SPR2_NEVER_RESPAWN))
            return TRUE;

        // No Respawn - can't get a weapon again if you already got it
        if (gNet.NoRespawn && TEST(pp->WpnGotOnceFlags, BIT(WPN)))
            return FALSE;

        return TRUE;
    }

    return TRUE;
}

char *KeyMsg[MAX_KEYS] =
{
    "Got the RED key!",
    "Got the BLUE key!",
    "Got the GREEN key!",
    "Got the YELLOW key!",
    "Got the GOLD master key!",
    "Got the SILVER master key!",
    "Got the BRONZE master key!",
    "Got the RED master key!"
};

struct InventoryDecl_t InventoryDecls[InvDecl_TOTAL] =
{
    { "Armor Vest +50",           50  },
    { "Kevlar Armor Vest +100",   100 },
    { "MedKit +20",               20  },
    { "Fortune Cookie +50 BOOST", 50  },
    { "Portable MedKit",          100 },
    { "Gas Bomb",                 1   },
    { "Flash Bomb",               2   },
    { "Caltrops",                 3   },
    { "Night Vision Goggles",     100 },
    { "Repair Kit",               100 },
    { "Smoke Bomb",               100 },
};

#define ITEMFLASHAMT  -8
#define ITEMFLASHCLR  144
int
DoGet(short SpriteNum)
{
    USERp u = User[SpriteNum], pu;
    SPRITEp sp = User[SpriteNum]->SpriteP;
    PLAYERp pp;
    short pnum, key_num;
    int dist, a,b,c;
    void InitWeaponRocket(PLAYERp);
    void InitWeaponUzi(PLAYERp);
    SWBOOL can_see;
    int cstat_bak;

    // For flag stuff
    USERp nu;
    SPRITEp np;
    short New;


    // Invisiblility is only used for DeathMatch type games
    // Sprites stays invisible for a period of time and is un-gettable
    // then "Re-Spawns" by becomming visible.  Its never actually killed.
    if (TEST(sp->cstat, CSTAT_SPRITE_INVISIBLE))
    {
        u->WaitTics -= ACTORMOVETICS * 2;
        if (u->WaitTics <= 0)
        {
            PlaySound(DIGI_ITEM_SPAWN, &sp->x, &sp->y, &sp->z, v3df_none);
            DoSpawnItemTeleporterEffect(sp);
            RESET(sp->cstat, CSTAT_SPRITE_INVISIBLE);
        }

        return 0;
    }

    if (sp->xvel)
    {
        if (!DoItemFly(SpriteNum))
        {
            sp->xvel = 0;
            change_sprite_stat(SpriteNum, STAT_ITEM);
        }
    }

    TRAVERSE_CONNECT(pnum)
    {
        pp = &Player[pnum];
        //pu = User[pp->PlayerSprite];
        pu = User[pp->SpriteP - sprite];

        if (TEST(pp->Flags, PF_DEAD))
            continue;

        DISTANCE(pp->posx, pp->posy, sp->x, sp->y, dist, a,b,c);
        if ((unsigned)dist > (pu->Radius + u->Radius))
        {
            continue;
        }

        if (!SpriteOverlap(SpriteNum, pp->SpriteP - sprite))
        {
            continue;
        }

        cstat_bak = sp->cstat;
        SET(sp->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
        can_see = FAFcansee(sp->x, sp->y, sp->z, sp->sectnum,
                            pp->posx, pp->posy, pp->posz, pp->cursectnum);
        sp->cstat = cstat_bak;

        if (!can_see)
        {
            continue;
        }

        switch (u->ID)
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

            PutStringInfo(Player+pnum, KeyMsg[key_num]);

            pp->HasKey[key_num] = TRUE;
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_KEY, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            PlayerUpdateKeys(pp);

            // don't kill keys in coop
            if (gNet.MultiGameType == MULTI_GAME_COOPERATIVE)
                break;

            KillSprite(SpriteNum);
            break;

        case ICON_ARMOR:
            if (pp->Armor < InventoryDecls[InvDecl_Kevlar].amount)
            {
                if (u->spal == PALETTE_PLAYER3)
                {
                    PlayerUpdateArmor(pp, 1000+InventoryDecls[InvDecl_Kevlar].amount);
                    PutStringInfo(Player+pnum, InventoryDecls[InvDecl_Kevlar].name);
                }
                else
                {
                    if (pp->Armor < InventoryDecls[InvDecl_Armor].amount)
                    {
                        PlayerUpdateArmor(pp, 1000+InventoryDecls[InvDecl_Armor].amount);
                        PutStringInfo(Player+pnum, InventoryDecls[InvDecl_Armor].name);
                    }
                    else
                        break;
                }
                SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_BIGITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);

                // override for respawn mode
                if (gNet.MultiGameType == MULTI_GAME_COMMBAT && gNet.NoRespawn)
                {
                    KillSprite(SpriteNum);
                    break;
                }

                KillGet(SpriteNum);
            }
            break;

        //
        // Health - Instant Use
        //

        case ICON_SM_MEDKIT:
            if (pu->Health < 100)
            {
                SWBOOL putbackmax=FALSE;

                PutStringInfo(Player+pnum, InventoryDecls[InvDecl_SmMedkit].name);

                if (pp->MaxHealth == 200)
                {
                    pp->MaxHealth = 100;
                    putbackmax = TRUE;
                }
                PlayerUpdateHealth(pp, InventoryDecls[InvDecl_SmMedkit].amount);

                if (putbackmax) pp->MaxHealth = 200;

                SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);

                // override for respawn mode
                if (gNet.MultiGameType == MULTI_GAME_COMMBAT && gNet.NoRespawn)
                {
                    KillSprite(SpriteNum);
                    break;
                }

                KillGet(SpriteNum);
            }
            break;

        case ICON_BOOSTER:   // Fortune cookie
            pp->MaxHealth = 200;
            if (pu->Health < 200)
            {
                PutStringInfo(Player+pnum, InventoryDecls[InvDecl_Booster].name);
                PlayerUpdateHealth(pp, InventoryDecls[InvDecl_Booster].amount);       // This is for health
                // over 100%
                // Say something witty
                if (pp == Player+myconnectindex && gs.Messages)
                {
                    if (gs.ParentalLock || Global_PLock)
                        sprintf(ds,"Fortune Say: %s\n",ReadFortune[STD_RANDOM_RANGE(10)]);
                    else
                        sprintf(ds,"Fortune Say: %s\n",ReadFortune[STD_RANDOM_RANGE(MAX_FORTUNES)]);
                    CON_Message(ds);
                }

                SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_BIGITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);

                // override for respawn mode
                if (gNet.MultiGameType == MULTI_GAME_COMMBAT && gNet.NoRespawn)
                {
                    KillSprite(SpriteNum);
                    break;
                }

                KillGet(SpriteNum);
            }
            break;

        //
        // Inventory
        //
        case ICON_MEDKIT:

            if (!pp->InventoryAmount[INVENTORY_MEDKIT] || pp->InventoryPercent[INVENTORY_MEDKIT] < InventoryDecls[InvDecl_Medkit].amount)
            {
                PutStringInfo(Player+pnum, InventoryDecls[InvDecl_Medkit].name);
                pp->InventoryPercent[INVENTORY_MEDKIT] = InventoryDecls[InvDecl_Medkit].amount;
                pp->InventoryAmount[INVENTORY_MEDKIT] = 1;
                PlayerUpdateInventory(pp, INVENTORY_MEDKIT);
                SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);

                // override for respawn mode
                if (gNet.MultiGameType == MULTI_GAME_COMMBAT && gNet.NoRespawn)
                {
                    KillSprite(SpriteNum);
                    break;
                }

                KillGet(SpriteNum);
            }
            break;

        case ICON_CHEMBOMB:

            if (pp->InventoryAmount[INVENTORY_CHEMBOMB] < InventoryDecls[InvDecl_ChemBomb].amount)
            {
                PutStringInfo(Player+pnum, InventoryDecls[InvDecl_ChemBomb].name);
                pp->InventoryPercent[INVENTORY_CHEMBOMB] = 0;
                pp->InventoryAmount[INVENTORY_CHEMBOMB]++;
                PlayerUpdateInventory(pp, INVENTORY_CHEMBOMB);
                SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
                KillGet(SpriteNum);
            }
            break;

        case ICON_FLASHBOMB:

            if (pp->InventoryAmount[INVENTORY_FLASHBOMB] < InventoryDecls[InvDecl_FlashBomb].amount)
            {
                PutStringInfo(Player+pnum, InventoryDecls[InvDecl_FlashBomb].name);
                pp->InventoryPercent[INVENTORY_FLASHBOMB] = 0;
                pp->InventoryAmount[INVENTORY_FLASHBOMB]++;
                PlayerUpdateInventory(pp, INVENTORY_FLASHBOMB);
                SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
                KillGet(SpriteNum);
            }
            break;

        case ICON_CALTROPS:

            if (pp->InventoryAmount[INVENTORY_CALTROPS] < InventoryDecls[InvDecl_Caltrops].amount)
            {
                PutStringInfo(Player+pnum, InventoryDecls[InvDecl_Caltrops].name);
                pp->InventoryPercent[INVENTORY_CALTROPS] = 0;
                pp->InventoryAmount[INVENTORY_CALTROPS]+=3;
                if (pp->InventoryAmount[INVENTORY_CALTROPS] > InventoryDecls[InvDecl_Caltrops].amount)
                    pp->InventoryAmount[INVENTORY_CALTROPS] = InventoryDecls[InvDecl_Caltrops].amount;
                PlayerUpdateInventory(pp, INVENTORY_CALTROPS);
                SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
                KillGet(SpriteNum);
            }
            break;

        case ICON_NIGHT_VISION:
            if (!pp->InventoryAmount[INVENTORY_NIGHT_VISION] || pp->InventoryPercent[INVENTORY_NIGHT_VISION] < InventoryDecls[InvDecl_NightVision].amount)
            {
                PutStringInfo(Player+pnum, InventoryDecls[InvDecl_NightVision].name);
                pp->InventoryPercent[INVENTORY_NIGHT_VISION] = InventoryDecls[InvDecl_NightVision].amount;
                pp->InventoryAmount[INVENTORY_NIGHT_VISION] = 1;
                PlayerUpdateInventory(pp, INVENTORY_NIGHT_VISION);
                SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
                KillGet(SpriteNum);
            }
            break;
        case ICON_REPAIR_KIT:
            if (!pp->InventoryAmount[INVENTORY_REPAIR_KIT] || pp->InventoryPercent[INVENTORY_REPAIR_KIT] < InventoryDecls[InvDecl_RepairKit].amount)
            {
                PutStringInfo(Player+pnum, InventoryDecls[InvDecl_RepairKit].name);
                pp->InventoryPercent[INVENTORY_REPAIR_KIT] = InventoryDecls[InvDecl_RepairKit].amount;
                pp->InventoryAmount[INVENTORY_REPAIR_KIT] = 1;
                PlayerUpdateInventory(pp, INVENTORY_REPAIR_KIT);
                SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);

                // don't kill repair kit in coop
                if (gNet.MultiGameType == MULTI_GAME_COOPERATIVE)
                    break;

                KillGet(SpriteNum);
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
                    PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
                KillGet(SpriteNum);
            }
            break;
#endif
        case ICON_CLOAK:
            if (!pp->InventoryAmount[INVENTORY_CLOAK] || pp->InventoryPercent[INVENTORY_CLOAK] < InventoryDecls[InvDecl_Cloak].amount)
            {
                PutStringInfo(Player+pnum, InventoryDecls[InvDecl_Cloak].name);
                pp->InventoryPercent[INVENTORY_CLOAK] = InventoryDecls[InvDecl_Cloak].amount;
                pp->InventoryAmount[INVENTORY_CLOAK] = 1;
                PlayerUpdateInventory(pp, INVENTORY_CLOAK);
                SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
                KillGet(SpriteNum);
            }
            break;
        //
        // Weapon
        //
        case ICON_STAR:

            if (!CanGetWeapon(pp, SpriteNum, WPN_STAR))
                break;

            SET(pp->WpnGotOnceFlags, BIT(WPN_STAR));

            if (pp->WpnAmmo[WPN_STAR] >= DamageData[WPN_STAR].max_ammo)
                break;

#ifdef UK_VERSION
            sprintf(ds,"Darts");
#else
            //sprintf(ds,"Shurikens");
#endif
            PutStringInfo(Player+pnum, DamageData[WPN_STAR].weapon_name);
            PlayerUpdateAmmo(pp, WPN_STAR, DamageData[WPN_STAR].weapon_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            KillGetWeapon(SpriteNum);
            if (TEST(pp->WpnFlags, BIT(WPN_STAR)))
                break;
            SET(pp->WpnFlags, BIT(WPN_STAR));
            if (User[pp->PlayerSprite]->WeaponNum <= WPN_STAR && User[pp->PlayerSprite]->WeaponNum != WPN_SWORD)
                break;
            InitWeaponStar(pp);
            break;

        case ICON_LG_MINE:

            if (!CanGetWeapon(pp, SpriteNum, WPN_MINE))
                break;

            SET(pp->WpnGotOnceFlags, BIT(WPN_MINE));

            if (pp->WpnAmmo[WPN_MINE] >= DamageData[WPN_MINE].max_ammo)
                break;
            //sprintf(ds,"Sticky Bombs");
            PutStringInfo(Player+pnum, DamageData[WPN_MINE].weapon_name);
            PlayerUpdateAmmo(pp, WPN_MINE, DamageData[WPN_MINE].weapon_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            ChoosePlayerGetSound(pp);
            KillGetWeapon(SpriteNum);
            if (TEST(pp->WpnFlags, BIT(WPN_MINE)))
                break;
            SET(pp->WpnFlags, BIT(WPN_MINE));
            if (User[pp->PlayerSprite]->WeaponNum > WPN_MINE && User[pp->PlayerSprite]->WeaponNum != WPN_SWORD)
                break;
            InitWeaponMine(pp);
            break;

        case ICON_UZI:
        case ICON_UZIFLOOR:

            if (!CanGetWeapon(pp, SpriteNum, WPN_UZI))
                break;

            SET(pp->WpnGotOnceFlags, BIT(WPN_UZI));

            if (TEST(pp->Flags, PF_TWO_UZI) && pp->WpnAmmo[WPN_UZI] >= DamageData[WPN_UZI].max_ammo)
                break;
            //sprintf(ds,"UZI Submachine Gun");
            PutStringInfo(Player+pnum, DamageData[WPN_UZI].weapon_name);
//            pp->WpnAmmo[WPN_UZI] += 50;
            PlayerUpdateAmmo(pp, WPN_UZI, DamageData[WPN_UZI].weapon_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            KillGetWeapon(SpriteNum);

            if (TEST(pp->WpnFlags, BIT(WPN_UZI)) && TEST(pp->Flags, PF_TWO_UZI))
                break;
            // flag to help with double uzi powerup - simpler but kludgy
            SET(pp->Flags, PF_PICKED_UP_AN_UZI);
            if (TEST(pp->WpnFlags, BIT(WPN_UZI)))
            {
                SET(pp->Flags, PF_TWO_UZI);
                pp->WpnUziType = 0; // Let it come up
                if (pp == Player+myconnectindex)
                    PlayerSound(DIGI_DOUBLEUZI, &pp->posx, &pp->posy, &pp->posz, v3df_dontpan|v3df_follow, pp);
            }
            else
            {
                SET(pp->WpnFlags, BIT(WPN_UZI));
                ChoosePlayerGetSound(pp);
            }

            if (User[pp->PlayerSprite]->WeaponNum > WPN_UZI && User[pp->PlayerSprite]->WeaponNum != WPN_SWORD)
                break;

            InitWeaponUzi(pp);
            break;

        case ICON_LG_UZI_AMMO:
            if (pp->WpnAmmo[WPN_UZI] >= DamageData[WPN_UZI].max_ammo)
                break;
            //sprintf(ds,"UZI Clip");
            PutStringInfo(Player+pnum, DamageData[WPN_UZI].ammo_name);
            PlayerUpdateAmmo(pp, WPN_UZI, DamageData[WPN_UZI].ammo_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            KillGetAmmo(SpriteNum);
            break;

        case ICON_MICRO_GUN:

            if (!CanGetWeapon(pp, SpriteNum, WPN_MICRO))
                break;

            SET(pp->WpnGotOnceFlags, BIT(WPN_MICRO));

            if (TEST(pp->WpnFlags, BIT(WPN_MICRO)) && pp->WpnAmmo[WPN_MICRO] >= DamageData[WPN_MICRO].max_ammo)
                break;
            //sprintf(ds,"Missile Launcher");
            PutStringInfo(Player+pnum, DamageData[WPN_MICRO].weapon_name);
//            pp->WpnAmmo[WPN_MICRO] += 5;
            PlayerUpdateAmmo(pp, WPN_MICRO, DamageData[WPN_MICRO].weapon_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            ChoosePlayerGetSound(pp);
            KillGetWeapon(SpriteNum);
            if (TEST(pp->WpnFlags, BIT(WPN_MICRO)))
                break;
            SET(pp->WpnFlags, BIT(WPN_MICRO));
            if (User[pp->PlayerSprite]->WeaponNum > WPN_MICRO && User[pp->PlayerSprite]->WeaponNum != WPN_SWORD)
                break;
            InitWeaponMicro(pp);
            break;

        case ICON_MICRO_BATTERY:
            if (pp->WpnAmmo[WPN_MICRO] >= DamageData[WPN_MICRO].max_ammo)
                break;
            //sprintf(ds,"Missiles");
            PutStringInfo(Player+pnum, DamageData[WPN_MICRO].ammo_name);
            PlayerUpdateAmmo(pp, WPN_MICRO, DamageData[WPN_MICRO].ammo_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            KillGetAmmo(SpriteNum);
            break;

        case ICON_NUKE:
            if (pp->WpnRocketNuke != 1)
            {
                //sprintf(ds,"Nuclear Warhead");
                PutStringInfo(Player+pnum, DamageData[DMG_NUCLEAR_EXP].weapon_name);
                pp->WpnRocketNuke = DamageData[DMG_NUCLEAR_EXP].weapon_pickup;
                SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
                if (STD_RANDOM_RANGE(1000) > 800 && pp == Player+myconnectindex)
                    PlayerSound(DIGI_ILIKENUKES, &pp->posx, &pp->posy, &pp->posz,
                                v3df_dontpan|v3df_doppler|v3df_follow,pp);
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

                KillGetAmmo(SpriteNum);
            }
            break;

        case ICON_GRENADE_LAUNCHER:
            if (!CanGetWeapon(pp, SpriteNum, WPN_GRENADE))
                break;

            SET(pp->WpnGotOnceFlags, BIT(WPN_GRENADE));

            if (TEST(pp->WpnFlags, BIT(WPN_GRENADE)) && pp->WpnAmmo[WPN_GRENADE] >= DamageData[WPN_GRENADE].max_ammo)
                break;
            //sprintf(ds,"Grenade Launcher");
            PutStringInfo(Player+pnum, DamageData[WPN_GRENADE].weapon_name);
//            pp->WpnAmmo[WPN_GRENADE] += 6;
            PlayerUpdateAmmo(pp, WPN_GRENADE, DamageData[WPN_GRENADE].weapon_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            //ChoosePlayerGetSound(pp);
            if (STD_RANDOM_RANGE(1000) > 800 && pp == Player+myconnectindex)
                PlayerSound(DIGI_LIKEBIGWEAPONS, &pp->posx, &pp->posy, &pp->posz,
                            v3df_dontpan|v3df_doppler|v3df_follow,pp);
            KillGetWeapon(SpriteNum);
            if (TEST(pp->WpnFlags, BIT(WPN_GRENADE)))
                break;
            SET(pp->WpnFlags, BIT(WPN_GRENADE));
            if (User[pp->PlayerSprite]->WeaponNum > WPN_GRENADE && User[pp->PlayerSprite]->WeaponNum != WPN_SWORD)
                break;
            InitWeaponGrenade(pp);
            break;

        case ICON_LG_GRENADE:
            if (pp->WpnAmmo[WPN_GRENADE] >= DamageData[WPN_GRENADE].max_ammo)
                break;
            //sprintf(ds,"Grenade Shells");
            PutStringInfo(Player+pnum, DamageData[WPN_GRENADE].ammo_name);
            PlayerUpdateAmmo(pp, WPN_GRENADE, DamageData[WPN_GRENADE].ammo_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            KillGetAmmo(SpriteNum);
            break;

#if 0
        case ICON_ROCKET:
            pp->WpnAmmo[WPN_ROCKET] += 15;
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            KillGet(SpriteNum);
            if (TEST(pp->WpnFlags, BIT(WPN_ROCKET)))
                break;
            SET(pp->WpnFlags, BIT(WPN_ROCKET));
            InitWeaponRocket(pp);
            break;

        case ICON_LG_ROCKET:
            sprintf(ds,"20 Missiles");
            PutStringInfo(Player+pnum, ds);
            PlayerUpdateAmmo(pp, WPN_ROCKET, 20);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            KillGet(SpriteNum);
            break;
#endif

        case ICON_RAIL_GUN:
            if (SW_SHAREWARE) { KillSprite(SpriteNum); break; }

            if (!CanGetWeapon(pp, SpriteNum, WPN_RAIL))
                break;

            SET(pp->WpnGotOnceFlags, BIT(WPN_RAIL));

            if (TEST(pp->WpnFlags, BIT(WPN_RAIL)) && pp->WpnAmmo[WPN_RAIL] >= DamageData[WPN_RAIL].max_ammo)
                break;
            //sprintf(ds,"Rail Gun");
            PutStringInfo(Player+pnum, DamageData[WPN_RAIL].weapon_name);
            PlayerUpdateAmmo(pp, WPN_RAIL, DamageData[WPN_RAIL].weapon_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            if (pp == Player+myconnectindex)
            {
                if (STD_RANDOM_RANGE(1000) > 700)
                    PlayerSound(DIGI_LIKEBIGWEAPONS, &pp->posx, &pp->posy, &pp->posz,
                                v3df_dontpan|v3df_doppler|v3df_follow,pp);
                else
                    PlayerSound(DIGI_GOTRAILGUN, &pp->posx, &pp->posy, &pp->posz,
                                v3df_dontpan|v3df_doppler|v3df_follow,pp);
            }
            //ChoosePlayerGetSound(pp);
            KillGetWeapon(SpriteNum);
            if (TEST(pp->WpnFlags, BIT(WPN_RAIL)))
                break;
            SET(pp->WpnFlags, BIT(WPN_RAIL));
            if (User[pp->PlayerSprite]->WeaponNum > WPN_RAIL && User[pp->PlayerSprite]->WeaponNum != WPN_SWORD)
                break;
            InitWeaponRail(pp);
            break;

        case ICON_RAIL_AMMO:
            if (SW_SHAREWARE) { KillSprite(SpriteNum); break; }

            if (pp->WpnAmmo[WPN_RAIL] >= DamageData[WPN_RAIL].max_ammo)
                break;
            //sprintf(ds,"Rail Gun Rods");
            PutStringInfo(Player+pnum, DamageData[WPN_RAIL].ammo_name);
            PlayerUpdateAmmo(pp, WPN_RAIL, DamageData[WPN_RAIL].ammo_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            KillGetAmmo(SpriteNum);
            break;

        case ICON_SHOTGUN:
            if (!CanGetWeapon(pp, SpriteNum, WPN_SHOTGUN))
                break;

            SET(pp->WpnGotOnceFlags, BIT(WPN_SHOTGUN));

            if (TEST(pp->WpnFlags, BIT(WPN_SHOTGUN)) && pp->WpnAmmo[WPN_SHOTGUN] >= DamageData[WPN_SHOTGUN].max_ammo)
                break;
            //sprintf(ds,"Riot Gun");
            PutStringInfo(Player+pnum, DamageData[WPN_SHOTGUN].weapon_name);
//            pp->WpnAmmo[WPN_SHOTGUN] += 10;
            PlayerUpdateAmmo(pp, WPN_SHOTGUN, DamageData[WPN_SHOTGUN].weapon_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            ChoosePlayerGetSound(pp);
            KillGetWeapon(SpriteNum);
            if (TEST(pp->WpnFlags, BIT(WPN_SHOTGUN)))
                break;
            SET(pp->WpnFlags, BIT(WPN_SHOTGUN));
            if (User[pp->PlayerSprite]->WeaponNum > WPN_SHOTGUN && User[pp->PlayerSprite]->WeaponNum != WPN_SWORD)
                break;
            InitWeaponShotgun(pp);
            break;

        case ICON_LG_SHOTSHELL:
            if (pp->WpnAmmo[WPN_SHOTGUN] >= DamageData[WPN_SHOTGUN].max_ammo)
                break;
            //sprintf(ds,"Shotshells");
            PutStringInfo(Player+pnum, DamageData[WPN_SHOTGUN].ammo_name);
            PlayerUpdateAmmo(pp, WPN_SHOTGUN, DamageData[WPN_SHOTGUN].ammo_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            KillGetAmmo(SpriteNum);
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
                    PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
                KillGet(SpriteNum);
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
            if (SW_SHAREWARE) { KillSprite(SpriteNum); break; }

            if (!CanGetWeapon(pp, SpriteNum, WPN_HOTHEAD))
                break;

            SET(pp->WpnGotOnceFlags, BIT(WPN_HOTHEAD));

            if (TEST(pp->WpnFlags, BIT(WPN_HOTHEAD)) && pp->WpnAmmo[WPN_HOTHEAD] >= DamageData[WPN_HOTHEAD].max_ammo)
                break;
            //sprintf(ds,"Guardian Head");
            PutStringInfo(Player+pnum, DamageData[WPN_HOTHEAD].weapon_name);
            PlayerUpdateAmmo(pp, WPN_HOTHEAD, DamageData[WPN_HOTHEAD].weapon_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            //ChoosePlayerGetSound(pp);
            if (STD_RANDOM_RANGE(1000) > 800 && pp == Player+myconnectindex)
                PlayerSound(DIGI_LIKEBIGWEAPONS, &pp->posx, &pp->posy, &pp->posz,
                            v3df_dontpan|v3df_doppler|v3df_follow,pp);
            KillGetWeapon(SpriteNum);
            if (TEST(pp->WpnFlags, BIT(WPN_HOTHEAD)))
                break;
            SET(pp->WpnFlags, BIT(WPN_NAPALM) | BIT(WPN_RING) | BIT(WPN_HOTHEAD));
            if (User[pp->PlayerSprite]->WeaponNum > WPN_HOTHEAD && User[pp->PlayerSprite]->WeaponNum != WPN_SWORD)
                break;
            InitWeaponHothead(pp);
            break;

        case ICON_FIREBALL_LG_AMMO:
            if (SW_SHAREWARE) { KillSprite(SpriteNum); break; }

            if (pp->WpnAmmo[WPN_HOTHEAD] >= DamageData[WPN_HOTHEAD].max_ammo)
                break;
            //sprintf(ds,"Firebursts");
            PutStringInfo(Player+pnum, DamageData[WPN_HOTHEAD].ammo_name);
            PlayerUpdateAmmo(pp, WPN_HOTHEAD, DamageData[WPN_HOTHEAD].ammo_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            KillGetAmmo(SpriteNum);
            break;

        case ICON_HEART:
            if (SW_SHAREWARE) { KillSprite(SpriteNum); break; }

            if (!CanGetWeapon(pp, SpriteNum, WPN_HEART))
                break;

            SET(pp->WpnGotOnceFlags, BIT(WPN_HEART));

            if (TEST(pp->WpnFlags, BIT(WPN_HEART)) && pp->WpnAmmo[WPN_HEART] >= DamageData[WPN_HEART].max_ammo)
                break;
            //sprintf(ds,"Ripper Heart");
            PutStringInfo(Player+pnum, DamageData[WPN_HEART].weapon_name);
            PlayerUpdateAmmo(pp, WPN_HEART, DamageData[WPN_HEART].weapon_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            //ChoosePlayerGetSound(pp);
            if (STD_RANDOM_RANGE(1000) > 800 && pp == Player+myconnectindex)
                PlayerSound(DIGI_LIKEBIGWEAPONS, &pp->posx, &pp->posy, &pp->posz,
                            v3df_dontpan|v3df_doppler|v3df_follow,pp);
            KillGetWeapon(SpriteNum);
            if (TEST(pp->WpnFlags, BIT(WPN_HEART)))
                break;
            SET(pp->WpnFlags, BIT(WPN_HEART));

            if (User[pp->PlayerSprite]->WeaponNum > WPN_HEART && User[pp->PlayerSprite]->WeaponNum != WPN_SWORD)
                break;

            InitWeaponHeart(pp);
            break;

        case ICON_HEART_LG_AMMO:
            if (SW_SHAREWARE) { KillSprite(SpriteNum); break; }

            if (pp->WpnAmmo[WPN_HEART] >= DamageData[WPN_HEART].max_ammo)
                break;
            //sprintf(ds,"Deathcoils");
            PutStringInfo(Player+pnum, DamageData[WPN_HEART].ammo_name);
            PlayerUpdateAmmo(pp, WPN_HEART, DamageData[WPN_HEART].ammo_pickup);
            SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            KillGetAmmo(SpriteNum);
            break;

#if 0
        case ICON_SPELL:
        {
            short w, h, select;

#define TEXT_SPELL_INFO_LINE 20

            static int8_t* SpellName[] =
            {
                "Icon of Flight",
                "EnvironSuit Skin",
                "Strength",
                "Cloak Device",
                "Oxygen",
                "Night Vision"
            };

            select = RANDOM_P2(4 << 8) >> 8;    // Not allowed to get
            // last two items.
            MNU_MeasureString(SpellName[sp->lotag], &w, &h);
            PutStringTimer(pp, TEXT_TEST_COL(w), TEXT_SPELL_INFO_LINE, SpellName[sp->lotag], TEXT_TEST_TIME);
            if (pp == Player+myconnectindex)
                PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
            KillGet(SpriteNum);
            break;
        }
#endif


        case ICON_HEAT_CARD:
            if (pp->WpnRocketHeat != 5)
            {
                //sprintf(ds,"Heat Seeker Card");
                PutStringInfo(Player+pnum, DamageData[DMG_NUCLEAR_EXP].ammo_name);
                pp->WpnRocketHeat = DamageData[DMG_NUCLEAR_EXP].ammo_pickup;
                SetFadeAmt(pp,ITEMFLASHAMT,ITEMFLASHCLR);  // Flash blue on item pickup
                if (pp == Player+myconnectindex)
                    PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);
                KillGet(SpriteNum);

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
            if (sp->pal == sprite[pp->PlayerSprite].pal) break; // Can't pick up your own flag!

            PlaySound(DIGI_ITEM, &sp->x, &sp->y, &sp->z, v3df_dontpan);

            if (sp->hitag == TAG_NORESPAWN_FLAG)
                New = SpawnSprite(STAT_ITEM, ICON_FLAG, s_CarryFlagNoDet, sp->sectnum,
                                  sp->x, sp->y, sp->z, 0, 0);
            else
                New = SpawnSprite(STAT_ITEM, ICON_FLAG, s_CarryFlag, sp->sectnum,
                                  sp->x, sp->y, sp->z, 0, 0);

            np = &sprite[New];
            nu = User[New];
            np->shade = -20;

            // Attach flag to player
            nu->Counter = 0;
            RESET(np->cstat, CSTAT_SPRITE_BLOCK|CSTAT_SPRITE_BLOCK_HITSCAN);
            SET(np->cstat, CSTAT_SPRITE_WALL);
            SetAttach(pp->PlayerSprite, New);
            nu->sz = SPRITEp_MID(&sprite[pp->PlayerSprite]);  // Set mid way up who it hit
            nu->spal = np->pal = sp->pal;   // Set the palette of the flag

            SetOwner(pp->PlayerSprite,New);  // Player now owns the flag
            nu->FlagOwner = SpriteNum;       // Tell carried flag who owns it
            KillGet(SpriteNum);  // Set up for flag respawning
            break;

        default:
            KillSprite(SpriteNum);
        }
    }

    return 0;
}


/*

  !AIC KEY - Set Active and Inactive code is here.  It was tough to make this
  fast.  Just know that the main flag is SPR_ACTIVE.  Should not need to be
  changed except for possibly the u->active_range settings in the future.

*/

void
SetEnemyActive(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;

    SET(u->Flags, SPR_ACTIVE);
    u->inactive_time = 0;
}

void
SetEnemyInactive(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;

    RESET(u->Flags, SPR_ACTIVE);
}


// This function mostly only adjust the active_range field

void
ProcessActiveVars(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
#define TIME_TILL_INACTIVE (4*120)

    if (!TEST(u->Flags, SPR_ACTIVE))
    {
        // if actor has been unaware for more than a few seconds
        u->inactive_time += ACTORMOVETICS;
        if (u->inactive_time > TIME_TILL_INACTIVE)
        {
            // reset to min update range
            u->active_range = MIN_ACTIVE_RANGE;
            // keep time low so it doesn't roll over
            u->inactive_time = TIME_TILL_INACTIVE;
        }
    }

    u->wait_active_check += ACTORMOVETICS;
}

void
AdjustActiveRange(PLAYERp pp, short SpriteNum, int dist)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    SPRITEp psp = pp->SpriteP;
    int look_height;


    // do no FAFcansee before it is time
    if (u->wait_active_check < ACTIVE_CHECK_TIME)
        return;

    u->wait_active_check = 0;

    // check aboslute max
    if (dist > MAX_ACTIVE_RANGE)
        return;

    // do not do a FAFcansee if your already active
    // Actor only becomes INACTIVE in DoActorDecision
    if (TEST(u->Flags, SPR_ACTIVE))
        return;

    //
    // From this point on Actor is INACTIVE
    //

    // if actor can still see the player
    look_height = SPRITEp_TOS(sp);
    if (FAFcansee(sp->x, sp->y, look_height, sp->sectnum, psp->x, psp->y, SPRITEp_UPPER(psp), psp->sectnum))
    {
        // Player is visible
        // adjust update range of this sprite

        // some huge distance
        u->active_range = 75000;
        // sprite is AWARE
        SetEnemyActive(SpriteNum);
    }
}

/*

  !AIC KEY - Main processing loop for sprites.  Sprites are separated and
  traversed by STAT lists.  Note the STAT_MISC, STAT_ENEMY, STAT_VATOR below.
  Most everything here calls StateControl().

*/


#if DEBUG
#define INLINE_STATE 0
#else
#define INLINE_STATE 1
#endif

#define STATE_CONTROL(SpriteNum, sp, u, StateTics)                                  \
    if (!(u)->State)                                                                \
    {                                                                           \
        ASSERT((u)->ActorActionFunc);                                               \
        ((u)->ActorActionFunc)((SpriteNum));                                        \
    }                                                                           \
    else                                                                            \
    {                                                                           \
        if ((sp)->statnum >= STAT_SKIP4_START && (sp)->statnum <= STAT_SKIP4_END)   \
            (u)->Tics += ACTORMOVETICS * 2;                                         \
        else                                                                        \
            (u)->Tics += ACTORMOVETICS;                                             \
                                                                                    \
        while ((u)->Tics >= TEST((u)->State->Tics, SF_TICS_MASK))                   \
        {                                                                       \
            (StateTics) = TEST((u)->State->Tics, SF_TICS_MASK);                     \
                                                                                    \
            if (TEST((u)->State->Tics, SF_TIC_ADJUST))                              \
            {                                                                   \
                ASSERT((u)->Attrib);                                                \
                ASSERT((u)->speed < MAX_SPEED);                                     \
                ASSERT((StateTics) > -(u)->Attrib->TicAdjust[(u)->speed]);          \
                                                                                    \
                (StateTics) += (u)->Attrib->TicAdjust[(u)->speed];                  \
            }                                                                   \
                                                                                    \
            (u)->Tics -= (StateTics);                                               \
                                                                                    \
            (u)->State = (u)->State->NextState;                                     \
                                                                                    \
            while (TEST((u)->State->Tics, SF_QUICK_CALL))                           \
            {                                                                   \
                (*(u)->State->Animator)((SpriteNum));                              \
                ASSERT(u);                                                          \
                                                                                    \
                if (!(u))                                                           \
                    break;                                                          \
                                                                                    \
                if (TEST((u)->State->Tics, SF_QUICK_CALL))                          \
                    (u)->State = (u)->State->NextState;                             \
            }                                                                   \
                                                                                    \
            if (!(u))                                                               \
                break;                                                              \
                                                                                    \
            if (!(u)->State->Pic)                                                   \
            {                                                                   \
                NewStateGroup((SpriteNum), (STATEp *) (u)->State->NextState);       \
            }                                                                   \
        }                                                                       \
                                                                                    \
        if (u)                                                                      \
        {                                                                       \
            if (TEST((u)->State->Tics, SF_WALL_STATE))                              \
            {                                                                   \
                ASSERT((u)->WallP);                                                 \
                (u)->WallP->picnum = (u)->State->Pic;                               \
            }                                                                   \
            else                                                                    \
            {                                                                   \
                if ((u)->RotNum > 1)                                                \
                    (sp)->picnum = (u)->Rot[0]->Pic;                                \
                else                                                                \
                    (sp)->picnum = (u)->State->Pic;                                 \
            }                                                                   \
                                                                                    \
            if ((u)->State->Animator && (u)->State->Animator != NullAnimator)       \
                (*(u)->State->Animator)((SpriteNum));                              \
        }                                                                       \
    }


/*

  !AIC KEY - Reads state tables for animation frame transitions and handles
  calling animators, QUICK_CALLS, etc.  This is handled for many types of
  sprites not just actors.

*/

int
StateControl(int16_t SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = &sprite[SpriteNum];
    short StateTics;

    if (!u->State)
    {
        ASSERT(u->ActorActionFunc);
        (u->ActorActionFunc)(SpriteNum);
        return 0;
    }

    if (sp->statnum >= STAT_SKIP4_START && sp->statnum <= STAT_SKIP4_END)
        u->Tics += ACTORMOVETICS * 2;
    else
        u->Tics += ACTORMOVETICS;

    // Skip states if too much time has passed
    while (u->Tics >= TEST(u->State->Tics, SF_TICS_MASK))
    {
        StateTics = TEST(u->State->Tics, SF_TICS_MASK);

        if (TEST(u->State->Tics, SF_TIC_ADJUST))
        {
            ASSERT(u->Attrib);
            ASSERT(u->speed < MAX_SPEED);
            ASSERT(StateTics > -u->Attrib->TicAdjust[u->speed]);

            StateTics += u->Attrib->TicAdjust[u->speed];
        }

        // Set Tics
        u->Tics -= StateTics;

        // Transition to the next state
        u->State = u->State->NextState;

        // Look for flags embedded into the Tics variable
        while (TEST(u->State->Tics, SF_QUICK_CALL))
        {
            // Call it once and go to the next state
            (*u->State->Animator)(SpriteNum);

            ASSERT(u); //put this in to see if actor was getting killed with in his QUICK_CALL state

            if (!u)
                break;

            // if still on the same QUICK_CALL should you
            // go to the next state.
            if (TEST(u->State->Tics, SF_QUICK_CALL))
                u->State = u->State->NextState;
        }

        if (!u)
            break;

        if (!u->State->Pic)
        {
            NewStateGroup(SpriteNum, (STATEp *) u->State->NextState);
        }
    }

    if (u)
    {
        ASSERT(u->State);
        ASSERT(ValidPtr(u));
        // Set picnum to the correct pic
        if (TEST(u->State->Tics, SF_WALL_STATE))
        {
            ASSERT(u->WallP);
            u->WallP->picnum = u->State->Pic;
        }
        else
        {
            //u->SpriteP->picnum = u->State->Pic;
            if (u->RotNum > 1)
                sp->picnum = u->Rot[0]->Pic;
            else
                sp->picnum = u->State->Pic;
        }

        // Call the correct animator
        if (u->State->Animator && u->State->Animator != NullAnimator)
            (*u->State->Animator)(SpriteNum);
    }

    return 0;
}



void
SpriteControl(void)
{
    int32_t i, nexti, stat;
    SPRITEp sp;
    USERp u;
    short pnum, CloseToPlayer;
    PLAYERp pp;
    int tx, ty, tmin, dist;
    extern SWBOOL DebugActorFreeze;
    short StateTics;

    if (DebugActorFreeze)
        return;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_MISC], i, nexti)
    {
#if INLINE_STATE
        ASSERT(User[i]);
        u = User[i];
        sp = User[i]->SpriteP;
        STATE_CONTROL(i, sp, u, StateTics)
        ASSERT(nexti >= 0 ? User[nexti] != NULL : TRUE);
#else
        ASSERT(User[i]);
        StateControl(i);
        ASSERT(nexti >= 0 ? User[nexti] != NULL : TRUE);
#endif
    }

    FAKETIMERHANDLER();

    // Items and skip2 things
    if (MoveSkip2 == 0)
    {
        for (stat = STAT_SKIP2_START + 1; stat <= STAT_SKIP2_END; stat++)
        {
            TRAVERSE_SPRITE_STAT(headspritestat[stat], i, nexti)
            {
#if INLINE_STATE
                ASSERT(User[i]);
                u = User[i];
                sp = User[i]->SpriteP;
                STATE_CONTROL(i, sp, u, StateTics)
                ASSERT(nexti >= 0 ? User[nexti] != NULL : TRUE);
#else
                ASSERT(User[i]);
                StateControl(i);
                ASSERT(nexti >= 0 ? User[nexti] != NULL : TRUE);
#endif
            }
        }
    }

    FAKETIMERHANDLER();

    if (MoveSkip2 == 0)                 // limit to 20 times a second
    {
        // move bad guys around
        TRAVERSE_SPRITE_STAT(headspritestat[STAT_ENEMY], i, nexti)
        {
            ASSERT(User[i]);

            u = User[i];
            sp = u->SpriteP;


            CloseToPlayer = FALSE;

            ProcessActiveVars(i);

            TRAVERSE_CONNECT(pnum)
            {
                pp = &Player[pnum];

                // Only update the ones closest
                DISTANCE(pp->posx, pp->posy, sp->x, sp->y, dist, tx, ty, tmin);

                AdjustActiveRange(pp, i, dist);

                if (dist < u->active_range)
                {
                    CloseToPlayer = TRUE;
                }
            }

            RESET(u->Flags, SPR_MOVED);

            // Only update the ones close to ANY player
            if (CloseToPlayer)
            {
#if INLINE_STATE
                u = User[i];
                sp = User[i]->SpriteP;
                STATE_CONTROL(i, sp, u, StateTics)
                ASSERT(nexti >= 0 ? User[nexti] != NULL : TRUE);
#else
                StateControl(i);
                ASSERT(nexti >= 0 ? User[nexti] != NULL : TRUE);
#endif
                ASSERT(nexti >= 0 ? User[nexti] != NULL : TRUE);
            }
            else
            {
                // to far away to be attacked
                RESET(u->Flags, SPR_ATTACKED);
            }
        }
    }

    FAKETIMERHANDLER();


    // Skip4 things
    if (MoveSkip4 == 0)                 // limit to 10 times a second
    {
        for (stat = STAT_SKIP4_START; stat <= STAT_SKIP4_END; stat++)
        {
            TRAVERSE_SPRITE_STAT(headspritestat[stat], i, nexti)
            {
#if INLINE_STATE
                ASSERT(User[i]);
                u = User[i];
                sp = User[i]->SpriteP;
                STATE_CONTROL(i, sp, u, StateTics)
                ASSERT(nexti >= 0 ? User[nexti] != NULL : TRUE);
#else
                ASSERT(User[i]);
                StateControl(i);
                ASSERT(nexti >= 0 ? User[nexti] != NULL : TRUE);
#endif
            }
        }
    }

    FAKETIMERHANDLER();

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_NO_STATE], i, nexti)
    {
        if (User[i] && User[i]->ActorActionFunc)
            (*User[i]->ActorActionFunc)(i);
        ASSERT(nexti >= 0 ? sprite[nexti].statnum != MAXSTATUS : TRUE);
    }

    if (MoveSkip8 == 0)
    {
        TRAVERSE_SPRITE_STAT(headspritestat[STAT_STATIC_FIRE], i, nexti)
        {
            extern int DoStaticFlamesDamage(short SpriteNum);
            ASSERT(User[i]);
            DoStaticFlamesDamage(i);
        }
    }

    if (MoveSkip4 == 0)                 // limit to 10 times a second
    {
        TRAVERSE_SPRITE_STAT(headspritestat[STAT_WALLBLOOD_QUEUE], i, nexti)
        {
#if INLINE_STATE
            ASSERT(User[i]);
            u = User[i];
            sp = User[i]->SpriteP;
            STATE_CONTROL(i, sp, u, StateTics)
            ASSERT(nexti >= 0 ? User[nexti] != NULL : TRUE);
#else
            ASSERT(User[i]);
            StateControl(i);
            ASSERT(nexti >= 0 ? User[nexti] != NULL : TRUE);
#endif

        }
    }

    // vator/rotator/spike/slidor all have some code to
    // prevent calling of the action func()
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_VATOR], i, nexti)
    {
        u = User[i];

        if (u->Tics)
        {
            if ((u->Tics -= synctics) <= 0)
                SetVatorActive(i);
            else
                continue;
        }

        if (!TEST(u->Flags, SPR_ACTIVE))
            continue;

        (*User[i]->ActorActionFunc)(i);
    }

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_SPIKE], i, nexti)
    {
        u = User[i];

        if (u->Tics)
        {
            if ((u->Tics -= synctics) <= 0)
                SetSpikeActive(i);
            else
                continue;
        }

        if (!TEST(u->Flags, SPR_ACTIVE))
            continue;

        (*User[i]->ActorActionFunc)(i);
    }

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_ROTATOR], i, nexti)
    {
        u = User[i];

        if (u->Tics)
        {
            if ((u->Tics -= synctics) <= 0)
                SetRotatorActive(i);
            else
                continue;
        }

        if (!TEST(u->Flags, SPR_ACTIVE))
            continue;

        (*User[i]->ActorActionFunc)(i);
    }

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_SLIDOR], i, nexti)
    {
        u = User[i];

        if (u->Tics)
        {
            if ((u->Tics -= synctics) <= 0)
                SetSlidorActive(i);
            else
                continue;
        }

        if (!TEST(u->Flags, SPR_ACTIVE))
            continue;

        (*User[i]->ActorActionFunc)(i);
    }

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_SUICIDE], i, nexti)
    {
        KillSprite(i);
    }
}


//
// This moves an actor about with FAFgetzrange clip adjustment
//

/*

  !AIC KEY - calls clipmove - Look through and try to understatnd  Should
  hopefully never have to change this.  Its very delicate.

*/

int
move_sprite(short spritenum, int xchange, int ychange, int zchange, int ceildist, int flordist, uint32_t cliptype, int numtics)
{
    int daz;
    int retval=0, zh;
    short dasectnum, tempshort;
    SPRITEp spr;
    USERp u = User[spritenum];
    short lastsectnum;

    spr = &sprite[spritenum];

    ASSERT(u);

    // Can't modify sprite sectors
    // directly becuase of linked lists
    dasectnum = lastsectnum = spr->sectnum;

    // Must do this if not using the new
    // centered centering (of course)
    daz = spr->z;

    if (TEST(spr->cstat, CSTAT_SPRITE_YCENTER))
    {
        zh = 0;
    }
    else
    {
        // move the center point up for moving
        zh = u->zclip;
        daz -= zh;
    }


//    ASSERT(inside(spr->x,spr->y,dasectnum));

    clipmoveboxtracenum = 1;
    retval = clipmove_old(&spr->x, &spr->y, &daz, &dasectnum,
                      ((xchange * numtics) << 11), ((ychange * numtics) << 11),
                      (((int) spr->clipdist) << 2), ceildist, flordist, cliptype);
    clipmoveboxtracenum = 3;

    //if (TEST(retval, HIT_MASK) == HIT_WALL)
    //    {
    //    CON_Message("retval = %ld",NORM_WALL(retval));
    //    CON_Message("clipdist = %d",spr->clipdist);
    //    }

    if (dasectnum < 0)
    {
        retval = HIT_WALL;
        //ASSERT(TRUE == FALSE);
        return retval;
    }

    if ((dasectnum != spr->sectnum) && (dasectnum >= 0))
        changespritesect(spritenum, dasectnum);

    // took this out - may not be to relevant anymore
    //ASSERT(inside(spr->x,spr->y,dasectnum));

    // Set the blocking bit to 0 temporarly so FAFgetzrange doesn't pick
    // up its own sprite
    tempshort = spr->cstat;
    spr->cstat = 0;
    //RESET(spr->cstat, CSTAT_SPRITE_BLOCK);

    // I subtracted 8 from the clipdist because actors kept going up on
    // ledges they were not supposed to go up on.  Did the same for the
    // player. Seems to work ok!
    FAFgetzrange(spr->x, spr->y, spr->z - zh - 1, spr->sectnum,
                 &globhiz, &globhihit, &globloz, &globlohit,
                 (((int) spr->clipdist) << 2) - GETZRANGE_CLIP_ADJ, cliptype);

    spr->cstat = tempshort;

    // !AIC - puts getzrange results into USER varaible u->loz, u->hiz, u->lo_sectp, u->hi_sectp, etc.
    // Takes info from global variables
    DoActorGlobZ(spritenum);

    daz = spr->z + ((zchange * numtics) >> 3);

    // test for hitting ceiling or floor
    if ((daz - zh <= globhiz) || (daz - zh > globloz))
    {
        if (retval == 0)
        {
            if (TEST(u->Flags, SPR_CLIMBING))
            {
                spr->z = daz;
                return 0;
            }

            retval = HIT_SECTOR|dasectnum;
        }
    }
    else
    {
        spr->z = daz;
    }

    // extra processing for Stacks and warping
    if (FAF_ConnectArea(spr->sectnum))
        setspritez(spritenum, (vec3_t *)spr);

    if (TEST(sector[spr->sectnum].extra, SECTFX_WARP_SECTOR))
    {
        SPRITEp sp_warp;
        if ((sp_warp = WarpPlane(&spr->x, &spr->y, &spr->z, &dasectnum)))
        {
            ActorWarpUpdatePos(spritenum, dasectnum);
            ActorWarpType(spr, sp_warp);
        }

        if (spr->sectnum != lastsectnum)
        {
            if ((sp_warp = Warp(&spr->x, &spr->y, &spr->z, &dasectnum)))
            {
                ActorWarpUpdatePos(spritenum, dasectnum);
                ActorWarpType(spr, sp_warp);
            }
        }
    }

    return retval;
}

// not used - SLOW!

int pushmove_sprite(short SpriteNum)
{
    SPRITEp sp = &sprite[SpriteNum];
    USERp u = User[SpriteNum];
    short sectnum, ret;
    int daz;

    daz = sp->z - u->zclip;
    sectnum = sp->sectnum;
    ret = pushmove_old(&sp->x, &sp->y, &daz, &sectnum,
                   (((int)sp->clipdist)<<2)-GETZRANGE_CLIP_ADJ, u->ceiling_dist, u->floor_dist, CLIPMASK_ACTOR);

    if (sectnum != sp->sectnum && sectnum >= 0)
        changespritesect(SpriteNum, sectnum);

    if (ret < 0)
    {
        //DSPRINTF(ds,"Pushed out!!!!! sp->sectnum %d", sp->sectnum);
        MONO_PRINT(ds);
    }

    sp->z = daz + u->zclip;
    return 0;
}

void MissileWarpUpdatePos(short SpriteNum, short sectnum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    u->ox = sp->x;
    u->oy = sp->y;
    u->oz = sp->z;
    changespritesect(SpriteNum, sectnum);
    MissileZrange(SpriteNum);
}

void ActorWarpUpdatePos(short SpriteNum, short sectnum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    u->ox = sp->x;
    u->oy = sp->y;
    u->oz = sp->z;
    changespritesect(SpriteNum, sectnum);
    DoActorZrange(SpriteNum);
}

void MissileWarpType(SPRITEp sp, SPRITEp sp_warp)
{
    switch (SP_TAG1(sp_warp))
    {
    case WARP_CEILING_PLANE:
    case WARP_FLOOR_PLANE:
        return;
    }

    switch (SP_TAG3(sp_warp))
    {
    case 1:
        break;
    default:
        PlaySound(DIGI_ITEM_SPAWN, &sp->x, &sp->y, &sp->z, v3df_none);
        //DoSpawnTeleporterEffectPlace(sp);
        DoSpawnItemTeleporterEffect(sp);
        break;
    }
}

void ActorWarpType(SPRITEp sp, SPRITEp sp_warp)
{
    switch (SP_TAG3(sp_warp))
    {
    case 1:
        break;
    default:
        PlaySound(DIGI_ITEM_SPAWN, &sp->x, &sp->y, &sp->z, v3df_none);
        DoSpawnTeleporterEffectPlace(sp);
        break;
    }
}

//
// This moves a small projectile with FAFgetzrangepoint
//

int
MissileWaterAdjust(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;

    if (u->lo_sectp)
    {
        SECT_USERp sectu = SectUser[u->lo_sectp - sector];
        if (sectu && sectu->depth)
            u->loz -= Z(sectu->depth);
    }
    return 0;
}

int
MissileZrange(short SpriteNum)
{
    USERp u = User[SpriteNum];
    SPRITEp sp = u->SpriteP;
    short tempshort;

    // Set the blocking bit to 0 temporarly so FAFgetzrange doesn't pick
    // up its own sprite
    tempshort = sp->cstat;
    RESET(sp->cstat, CSTAT_SPRITE_BLOCK);

    FAFgetzrangepoint(sp->x, sp->y, sp->z - 1, sp->sectnum,
                      &globhiz, &globhihit, &globloz, &globlohit);

    sp->cstat = tempshort;

    DoActorGlobZ(SpriteNum);
    return 0;
}


int
move_missile(short spritenum, int xchange, int ychange, int zchange, int ceildist, int flordist, uint32_t cliptype, int numtics)
{
    int daz;
    int retval, zh;
    short dasectnum, tempshort;
    SPRITEp sp;
    USERp u = User[spritenum];
    short lastsectnum;

    sp = &sprite[spritenum];

    ASSERT(u);

    // Can't modify sprite sectors
    // directly becuase of linked lists
    dasectnum = lastsectnum = sp->sectnum;

    // Can't modify sprite sectors
    // directly becuase of linked lists
    daz = sp->z;

    if (TEST(sp->cstat, CSTAT_SPRITE_YCENTER))
    {
        zh = 0;
    }
    else
    {
        zh = u->zclip;
        daz -= zh;
    }


//    ASSERT(inside(sp->x,sp->y,dasectnum));
    clipmoveboxtracenum = 1;
    retval = clipmove_old(&sp->x, &sp->y, &daz, &dasectnum,
                      ((xchange * numtics) << 11), ((ychange * numtics) << 11),
                      (((int) sp->clipdist) << 2), ceildist, flordist, cliptype);
    clipmoveboxtracenum = 3;

    if (dasectnum < 0)
    {
        // we've gone beyond a white wall - kill it
        retval = 0;
        SET(retval, HIT_PLAX_WALL);
        return retval;
    }

    // took this out - may not be to relevant anymore
    //ASSERT(inside(sp->x,sp->y,dasectnum));

    if ((dasectnum != sp->sectnum) && (dasectnum >= 0))
        changespritesect(spritenum, dasectnum);

    // Set the blocking bit to 0 temporarly so FAFgetzrange doesn't pick
    // up its own sprite
    tempshort = sp->cstat;
    RESET(sp->cstat, CSTAT_SPRITE_BLOCK);

    FAFgetzrangepoint(sp->x, sp->y, sp->z - 1, sp->sectnum,
                      &globhiz, &globhihit, &globloz, &globlohit);

    sp->cstat = tempshort;

    DoActorGlobZ(spritenum);

    // getzrangepoint moves water down
    // missiles don't need the water to be down
    MissileWaterAdjust(spritenum);

    daz = sp->z + ((zchange * numtics) >> 3);

    // NOTE: this does not tell you when you hit a floor sprite
    // this case is currently treated like it hit a sector

    // test for hitting ceiling or floor
    if (daz - zh <= u->hiz + ceildist)
    {
        // normal code
        sp->z = u->hiz + zh + ceildist;
        if (retval == 0)
            retval = dasectnum|HIT_SECTOR;
    }
    else if (daz - zh > u->loz - flordist)
    {
        sp->z = u->loz + zh - flordist;
        if (retval == 0)
            retval = dasectnum|HIT_SECTOR;
    }
    else
    {
        sp->z = daz;
    }

    if (FAF_ConnectArea(sp->sectnum))
        setspritez(spritenum, (vec3_t *)sp);

    if (TEST(sector[sp->sectnum].extra, SECTFX_WARP_SECTOR))
    {
        SPRITEp sp_warp;

        if ((sp_warp = WarpPlane(&sp->x, &sp->y, &sp->z, &dasectnum)))
        {
            MissileWarpUpdatePos(spritenum, dasectnum);
            MissileWarpType(sp, sp_warp);
        }

        if (sp->sectnum != lastsectnum)
        {
            if ((sp_warp = Warp(&sp->x, &sp->y, &sp->z, &dasectnum)))
            {
                MissileWarpUpdatePos(spritenum, dasectnum);
                MissileWarpType(sp, sp_warp);
            }
        }
    }

    if (retval && TEST(sector[sp->sectnum].ceilingstat, CEILING_STAT_PLAX))
    {
        if (sp->z < sector[sp->sectnum].ceilingz)
        {
            RESET(retval, HIT_WALL|HIT_SECTOR);
            SET(retval, HIT_PLAX_WALL);
        }
    }

    if (retval && TEST(sector[sp->sectnum].floorstat, FLOOR_STAT_PLAX))
    {
        if (sp->z > sector[sp->sectnum].floorz)
        {
            RESET(retval, HIT_WALL|HIT_SECTOR);
            SET(retval, HIT_PLAX_WALL);
        }
    }

    return retval;
}


int
move_ground_missile(short spritenum, int xchange, int ychange, int zchange, int ceildist, int flordist, uint32_t cliptype, int numtics)
{
    int daz;
    int retval=0, zh;
    short dasectnum, tempshort;
    SPRITEp sp;
    USERp u = User[spritenum];
    short lastsectnum;
    int ox,oy;

    sp = &sprite[spritenum];

    ASSERT(u);

    // Can't modify sprite sectors
    // directly becuase of linked lists
    dasectnum = lastsectnum = sp->sectnum;

    daz = sp->z;
    zh = 0;

    // climbing a wall
    if (u->z_tgt)
    {
        if (labs(u->z_tgt - sp->z) > Z(40))
        {
            if (u->z_tgt > sp->z)
            {
                sp->z += Z(30);
                return retval;
            }
            else
            {
                sp->z -= Z(30);
                return retval;
            }
        }
        else
            u->z_tgt = 0;
    }

    ox = sp->x;
    oy = sp->y;
    sp->x += xchange/2;
    sp->y += ychange/2;

    updatesector(sp->x, sp->y, &dasectnum);

    if (dasectnum < 0)
    {
        // back up and try again
        dasectnum = lastsectnum = sp->sectnum;
        sp->x = ox;
        sp->y = oy;
        clipmoveboxtracenum = 1;
        retval = clipmove_old(&sp->x, &sp->y, &daz, &dasectnum,
                          ((xchange * numtics) << 11), ((ychange * numtics) << 11),
                          (((int) sp->clipdist) << 2), ceildist, flordist, cliptype);
        clipmoveboxtracenum = 3;
    }

    if (dasectnum < 0)
    {
        // we've gone beyond a white wall - kill it
        retval = 0;
        SET(retval, HIT_PLAX_WALL);
        return retval;
    }


    if (retval)  // ran into a white wall
    {
        int new_loz,new_hiz;

        // back up and try to clip UP
        //dasectnum = lastsectnum = sp->sectnum;
        //sp->x = ox;
        //sp->y = oy;

#if 0
        getzsofslope(dasectnum, sp->x, sp->y, &new_hiz, &new_loz);

        if (labs(sp->z - new_hiz) > Z(40))
        {
            u->z_tgt = new_hiz;
            retval = 0;
            return retval;
        }
        else
#endif
        return retval;
    }


    u->z_tgt = 0;
    if ((dasectnum != sp->sectnum) && (dasectnum >= 0))
    {
        int new_loz,new_hiz;
        getzsofslope(dasectnum, sp->x, sp->y, &new_hiz, &new_loz);

        sp->z = new_loz;

#if 0
        if (labs(sp->z - new_loz) > Z(40))
        {
            if (new_loz > sp->z)
            {
                // travelling DOWN
                u->z_tgt = new_loz;
                changespritesect(spritenum, dasectnum);

                getzsofslope(sp->sectnum, sp->x, sp->y, &u->hiz, &u->loz);
                u->hi_sectp = u->lo_sectp = &sector[sp->sectnum];
                u->hi_sp = u->lo_sp = NULL;
                return retval;
            }
            else
            {
                // travelling UP
                u->z_tgt = new_loz;

                // back up and climb wall
                dasectnum = lastsectnum = sp->sectnum;
                sp->x = ox;
                sp->y = oy;
                return retval;
            }
        }
        else
        {
            u->z_tgt = 0;
        }
#endif

        changespritesect(spritenum, dasectnum);
    }

    getzsofslope(sp->sectnum, sp->x, sp->y, &u->hiz, &u->loz);

    u->hi_sectp = u->lo_sectp = &sector[sp->sectnum];
    u->hi_sp = u->lo_sp = NULL;
    sp->z = u->loz - Z(8);

    if (labs(u->hiz - u->loz) < Z(12))
    {
        // we've gone into a very small place - kill it
        retval = 0;
        SET(retval, HIT_PLAX_WALL);
        return retval;
    }

    // getzrangepoint moves water down
    // missiles don't need the water to be down
    //MissileWaterAdjust(spritenum);

    //if (FAF_ConnectArea(sp->sectnum))
    //    setspritez(spritenum, (vec3_t *)sp);

    if (TEST(sector[sp->sectnum].extra, SECTFX_WARP_SECTOR))
    {
        SPRITEp sp_warp;

        if ((sp_warp = WarpPlane(&sp->x, &sp->y, &sp->z, &dasectnum)))
        {
            MissileWarpUpdatePos(spritenum, dasectnum);
            MissileWarpType(sp, sp_warp);
        }

        if (sp->sectnum != lastsectnum)
        {
            if ((sp_warp = Warp(&sp->x, &sp->y, &sp->z, &dasectnum)))
            {
                MissileWarpUpdatePos(spritenum, dasectnum);
                MissileWarpType(sp, sp_warp);
            }
        }
    }

    return retval;
}

/*
int push_check(short SpriteNum)
    {
    switch (sprite[SpriteNum].lotag)
        {
        case TAG_DOOR_S,LIDING:
            {
            SPRITEp sp;
            USERp u;
            short i,nexti,sect;

            //DSPRINTF(ds,"Door Closing %d",sectnum);
            MONO_PRINT(ds);

            TRAVERSE_SPRITE_SECT(headspritesect[sectnum], i, nexti)
                {
                sp = &sprite[i];
                u = User[i];

                sect = pushmove((vec3_t *)sp, &sp->sectnum, (((int)sp->clipdist)<<2)-8, u->ceiling_dist, u->floor_dist, CLIPMASK0);
                if (sect == -1)
                    {
                    KillSprite(i);
                    }
                }
            }
        }
    }
*/


#include "saveable.h"

static saveable_code saveable_sprite_code[] =
{
    SAVE_CODE(DoActorZrange),
    SAVE_CODE(DoActorGlobZ),
    SAVE_CODE(DoStayOnFloor),
    SAVE_CODE(DoGrating),
    SAVE_CODE(SpearOnFloor),
    SAVE_CODE(SpearOnCeiling),
    SAVE_CODE(DoKey),
    SAVE_CODE(DoCoin),
    SAVE_CODE(KillGet),
    SAVE_CODE(KillGetAmmo),
    SAVE_CODE(KillGetWeapon),
    SAVE_CODE(DoSpawnItemTeleporterEffect),
    SAVE_CODE(DoGet),
    SAVE_CODE(SetEnemyActive),
    SAVE_CODE(SetEnemyInactive),
    SAVE_CODE(ProcessActiveVars),
    SAVE_CODE(StateControl),
    SAVE_CODE(SpriteControl),
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

