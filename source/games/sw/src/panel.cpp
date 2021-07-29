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

#undef MAIN
#include "build.h"

#include "names2.h"
#include "panel.h"
#include "lists.h"
#include "misc.h"
#include "network.h"
#include "pal.h"
#include "player.h"
#include "v_2ddrawer.h"
#include "gamehud.h"

#include "weapon.h"
#include "razemenu.h"
#include "raze_sound.h"

BEGIN_SW_NS


int InitSwordAttack(PLAYERp pp);
PANEL_SPRITEp InitWeaponUziSecondaryReload(PANEL_SPRITEp);
PANEL_SPRITEp InitWeaponUzi2(PANEL_SPRITEp);
int InitShotgun(PLAYERp pp);
int InitRail(PLAYERp pp);
int InitEMP(PLAYERp pp);
int InitMicro(PLAYERp pp);
int InitRocket(PLAYERp pp);
int InitNuke(PLAYERp pp);
int InitGrenade(PLAYERp pp);
int InitMine(PLAYERp pp);
int InitFistAttack(PLAYERp pp);

#define PANF_UZI_XFLIP (BIT(21))

#define XDIM 320
#define YDIM 200

typedef struct
{
    short xoff, yoff, skip;
    int lo_jump_speed, hi_jump_speed, lo_xspeed, hi_xspeed;
    PANEL_STATEp state[2];
} PANEL_SHRAP, *PANEL_SHRAPp;

void PanelInvTestSuicide(PANEL_SPRITEp psp);

void InsertPanelSprite(PLAYERp pp, PANEL_SPRITEp psp);
void pKillSprite(PANEL_SPRITEp psp);
void pWeaponBob(PANEL_SPRITEp psp, short condition);
void pSuicide(PANEL_SPRITEp psp);
void pNextState(PANEL_SPRITEp psp);
void pStatePlusOne(PANEL_SPRITEp psp);
void pSetState(PANEL_SPRITEp psp, PANEL_STATEp panel_state);
void pStateControl(PANEL_SPRITEp psp);

int DoPanelFall(PANEL_SPRITEp psp);
int DoBeginPanelFall(PANEL_SPRITEp psp);
int DoPanelJump(PANEL_SPRITEp psp);
int DoBeginPanelJump(PANEL_SPRITEp psp);
void SpawnHeartBlood(PANEL_SPRITEp psp);
void SpawnUziShell(PANEL_SPRITEp psp);

bool pWeaponUnHideKeys(PANEL_SPRITEp psp, PANEL_STATEp state);
bool pWeaponHideKeys(PANEL_SPRITEp psp, PANEL_STATEp state);
void pHotHeadOverlays(PANEL_SPRITEp psp, short mode);

char UziRecoilYadj = 0;

extern short screenpeek;

ANIMATOR NullAnimator;
pANIMATOR pNullAnimator;
int InitStar(PLAYERp);
int ChangeWeapon(PLAYERp);

ANIMATOR InitFire;

int
NullAnimator(short)
{
    return 0;
}

void pNullAnimator(PANEL_SPRITEp)
{
    return;
}

PANEL_SPRITEp pFindMatchingSprite(PLAYERp pp, int x, int y, short pri)
{
    PANEL_SPRITEp psp=nullptr, next;

    TRAVERSE(&pp->PanelSpriteList, psp, next)
    {
        // early out
        if (psp->priority > pri)
            return nullptr;

        if (psp->x == x && psp->y == y && psp->priority == pri)
        {
            return psp;
        }
    }

    return nullptr;
}

PANEL_SPRITEp pFindMatchingSpriteID(PLAYERp pp, short id, int x, int y, short pri)
{
    PANEL_SPRITEp psp=nullptr, next;

    TRAVERSE(&pp->PanelSpriteList, psp, next)
    {
        // early out
        if (psp->priority > pri)
            return nullptr;

        if (psp->ID == id && psp->x == x && psp->y == y && psp->priority == pri)
        {
            return psp;
        }
    }

    return nullptr;
}

bool pKillScreenSpiteIDs(PLAYERp pp, short id)
{
    PANEL_SPRITEp psp=nullptr, next;
    bool found = false;

    // Kill ALL sprites with the correct id
    TRAVERSE(&pp->PanelSpriteList, psp, next)
    {
        if (psp->ID == id)
        {
            pKillSprite(psp);
            found = true;
        }
    }

    return found;
}


// Used to sprites in the view at correct aspect ratio and x,y location.

void pSetSuicide(PANEL_SPRITEp psp)
{
    //SET(psp->flags, PANF_SUICIDE);
    //psp->State = nullptr;
    psp->PanelSpriteFunc = pSuicide;
}

void pToggleCrosshair(void)
{
	cl_crosshair = !cl_crosshair;
}

// Player has a chance of yelling out during combat, when firing a weapon.
void DoPlayerChooseYell(PLAYERp pp)
{
    int choose_snd = 0;

    if (RANDOM_RANGE(1000) < 990) return;

    choose_snd = STD_RANDOM_RANGE(MAX_YELLSOUNDS);

    if (pp == Player+myconnectindex)
        PlayerSound(PlayerYellVocs[choose_snd], v3df_follow|v3df_dontpan,pp);
}

void ArmorCalc(int damage_amt, int *armor_damage, int *player_damage)
{
    int damage_percent;

    if (damage_amt == 1)
    {
        *player_damage = RANDOM_P2(2<<8)>>8;
        *armor_damage = 1;
        return;
    }

    // note: this could easily be converted to a mulscale and save a
    // bit of processing for floats
    damage_percent = int((0.6 * damage_amt)+0.5);

    *player_damage = damage_amt - damage_percent;
    *armor_damage = damage_percent;
}


void PlayerUpdateHealth(PLAYERp pp, short value)
{
    USERp u = User[pp->PlayerSprite].Data();
    short x,y;

    if (Prediction)
        return;

    if (GodMode)
    {
        if (value < 0)
            return;
    }

    if (TEST(pp->Flags, PF_DEAD))
        return;

    if (value < 0)
    {
        bool IsChem = false;
        bool NoArmor = false;

        if (value <= -2000)
        {
            value += 2000;
            NoArmor = true;
        }
        else if (value <= -1000)
        {
            //DSPRINTF(ds,"value = %d\n",value);
            MONO_PRINT(ds);

            value += 1000;
            IsChem = true;
        }

        // TAKE SOME DAMAGE
        u->LastDamage = -value;

        // adjust for armor
        if (pp->Armor && !NoArmor)
        {
            int armor_damage, player_damage;
            ArmorCalc(labs(value), &armor_damage, &player_damage);
            PlayerUpdateArmor(pp, -armor_damage);
            value = -player_damage;
        }

        u->Health += value;

        if (value < 0)
        {
            int choosesnd = 0;

            choosesnd = RANDOM_RANGE(MAX_PAIN);

            if (u->Health > 50)
            {
                PlayerSound(PlayerPainVocs[choosesnd],v3df_dontpan|v3df_doppler|v3df_follow,pp);
            }
            else
            {
                PlayerSound(PlayerLowHealthPainVocs[choosesnd],v3df_dontpan|v3df_doppler|v3df_follow,pp);
            }

        }
        // Do redness based on damage taken.
        if (pp == Player + screenpeek)
        {
            if (IsChem)
                SetFadeAmt(pp,-40,144);  // ChemBomb green color
            else
            {
                if (value <= -10)
                    SetFadeAmt(pp,value,112);
                else if (value > -10 && value < 0)
                    SetFadeAmt(pp,-20,112);
            }
        }
        if (u->Health <= 100)
            pp->MaxHealth = 100; // Reset max health if sank below 100
    }
    else
    {
        // ADD SOME HEALTH
        if (value > 1000)
            u->Health += (value-1000);
        else
        {
            if (u->Health < pp->MaxHealth)
            {
                u->Health+=value;
            }
        }


        if (value >= 1000)
            value -= 1000;  // Strip out high value
    }

    if (u->Health < 0)
        u->Health = 0;

    if (u->Health > pp->MaxHealth)
        u->Health = pp->MaxHealth;
}

void PlayerUpdateAmmo(PLAYERp pp, short UpdateWeaponNum, short value)
{
    USERp u = User[pp->PlayerSprite].Data();
    short x,y;
    short WeaponNum;

    if (Prediction)
        return;

    if (DamageData[UpdateWeaponNum].max_ammo == -1)
    {
        return;
    }

    WeaponNum = UpdateWeaponNum;

    // get the WeaponNum of the ammo
    if (DamageData[UpdateWeaponNum].with_weapon != -1)
    {
        WeaponNum = DamageData[UpdateWeaponNum].with_weapon;
    }

    pp->WpnAmmo[WeaponNum] += value;

    if (pp->WpnAmmo[WeaponNum] <= 0)
    {
        // star and mine
        if (TEST(WeaponIsAmmo, BIT(WeaponNum)))
            RESET(pp->WpnFlags, BIT(WeaponNum));

        pp->WpnAmmo[WeaponNum] = 0;
    }

    if (pp->WpnAmmo[WeaponNum] > DamageData[WeaponNum].max_ammo)
    {
        pp->WpnAmmo[WeaponNum] = DamageData[WeaponNum].max_ammo;
    }
}

void PlayerUpdateWeapon(PLAYERp pp, short WeaponNum)
{
    USERp u = User[pp->PlayerSprite].Data();

    // Weapon Change
    if (Prediction)
        return;

    u->WeaponNum = int8_t(WeaponNum);
}

void PlayerUpdateKills(PLAYERp pp, short value)
{
    if (Prediction)
        return;

    if (gNet.MultiGameType == MULTI_GAME_COOPERATIVE)
        return;

    // Team play
    if (gNet.MultiGameType == MULTI_GAME_COMMBAT && gNet.TeamPlay)
    {
        short pnum;
        PLAYERp opp;

        TRAVERSE_CONNECT(pnum)
        {
            opp = Player + pnum;

            // for everyone on the same team
            if (opp != pp && User[opp->PlayerSprite]->spal == User[pp->PlayerSprite]->spal)
            {
                opp->Kills += value;
                if (opp->Kills > 999)
                    opp->Kills = 0;
                if (opp->Kills < -99)
                    opp->Kills = -99;
            }
        }
    }

    pp->Kills += value;
    if (pp->Kills > 999)
        pp->Kills = 0;
    if (pp->Kills < -99)
        pp->Kills = -99;
}

void PlayerUpdateArmor(PLAYERp pp, short value)
{
    if (Prediction)
        return;

    if (value >= 1000)
        pp->Armor = value-1000;
    else
        pp->Armor += value;

    if (pp->Armor > 100)
        pp->Armor = 100;
    if (pp->Armor < 0)
        pp->Armor = 0;
}


int WeaponOperate(PLAYERp pp)
{
    short weapon;
    int DoPlayerSpriteReset(short SpriteNum);
    USERp u = User[pp->PlayerSprite].Data();


    InventoryKeys(pp);

    // UziType >= 3 means they are reloading
    if (pp->WpnUziType >= 3) return true;

    //if (CheatInputMode)
    //    return (0);

    if (pp->FirePause != 0)
    {
        pp->FirePause -= synctics;
        if (pp->FirePause <= 0)
            pp->FirePause = 0;
    }

    if (pp->sop)
    {
        switch (pp->sop->track)
        {
        case SO_VEHICLE:
        case SO_TURRET:
        case SO_TURRET_MGUN:
        // case SO_SPEED_BOAT:

            if (!TEST(pp->sop->flags, SOBJ_HAS_WEAPON))
                break;

            if (pp->input.actions & SB_FIRE)
            {
                if (pp->KeyPressBits & SB_FIRE)
                {
                    if (!pp->FirePause)
                    {
                        InitSobjGun(pp);
                    }
                }
            }

            return 0;
        }
    }

    weapon = pp->input.getNewWeapon();

    if (weapon)
    {
        if (pp->KeyPressBits & SB_FIRST_WEAPON_BIT)
        {
            pp->KeyPressBits &= ~SB_FIRST_WEAPON_BIT;

            weapon -= 1;

            // Special uzi crap
            if (weapon != WPN_UZI) pp->WpnUziType = 2; // This means we aren't on uzi

            switch (weapon)
            {
            case WPN_FIST:
                //case WPN_SWORD:
                if (u->WeaponNum == WPN_FIST
                    || u->WeaponNum == WPN_SWORD)
                {
                    // toggle
                    if (pp->WpnFirstType == WPN_FIST)
                        pp->WpnFirstType = WPN_SWORD;
                    else if (pp->WpnFirstType == WPN_SWORD)
                        pp->WpnFirstType = WPN_FIST;
                }

                switch (pp->WpnFirstType)
                {
                case WPN_SWORD:
                    InitWeaponSword(pp);
                    break;
                case WPN_FIST:
                    InitWeaponFist(pp);
                    break;
                }
                break;
            case WPN_STAR:
                InitWeaponStar(pp);
                break;
            case WPN_UZI:
                if (u->WeaponNum == WPN_UZI)
                {
                    if (TEST(pp->Flags, PF_TWO_UZI))
                    {
                        pp->WpnUziType++;
                        PlaySound(DIGI_UZI_UP, pp, v3df_follow);
                        if (pp->WpnUziType > 1)
                            pp->WpnUziType = 0;
                    }
                    else
                        pp->WpnUziType = 1; // Use retracted state for single uzi
                }
                InitWeaponUzi(pp);
                break;
            case WPN_MICRO:
                if (u->WeaponNum == WPN_MICRO)
                {
                    pp->WpnRocketType++;
                    PlaySound(DIGI_ROCKET_UP, pp, v3df_follow);
                    if (pp->WpnRocketType > 2)
                        pp->WpnRocketType = 0;
                    if (pp->WpnRocketType == 2 && pp->WpnRocketNuke == 0)
                        pp->WpnRocketType = 0;
                    if (pp->WpnRocketType == 2)
                        pp->TestNukeInit = true; // Init the nuke
                    else
                        pp->TestNukeInit = false;
                }
                InitWeaponMicro(pp);
                break;
            case WPN_SHOTGUN:
                if (u->WeaponNum == WPN_SHOTGUN)
                {
                    pp->WpnShotgunType++;
                    if (pp->WpnShotgunType > 1)
                        pp->WpnShotgunType = 0;
                    PlaySound(DIGI_SHOTGUN_UP, pp, v3df_follow);
                }
                InitWeaponShotgun(pp);
                break;
            case WPN_RAIL:
                if (!SW_SHAREWARE)
                {
#if 0
                    if (u->WeaponNum == WPN_RAIL)
                    {
                        pp->WpnRailType++;
                        if (pp->WpnRailType > 1)
                            pp->WpnRailType = 0;
                    }
                    if (pp->WpnRailType == 1)
                        PlaySound(DIGI_RAIL_UP, pp, v3df_follow);
#endif
                    InitWeaponRail(pp);
                }
                else
                {
                    PutStringInfo(pp,"Order the full version");
                }
                break;
            case WPN_HOTHEAD:
                if (!SW_SHAREWARE)
                {
                    if (u->WeaponNum == WPN_HOTHEAD
                        || u->WeaponNum == WPN_RING
                        || u->WeaponNum == WPN_NAPALM)
                    {
                        pp->WpnFlameType++;
                        if (pp->WpnFlameType > 2)
                            pp->WpnFlameType = 0;
//                      if(pp->Wpn[WPN_HOTHEAD])
                        pHotHeadOverlays(pp->Wpn[WPN_HOTHEAD], pp->WpnFlameType);
                        PlaySound(DIGI_HOTHEADSWITCH, pp, v3df_dontpan|v3df_follow);
                    }

                    InitWeaponHothead(pp);
                }
                else
                {
                    PutStringInfo(pp,"Order the full version");
                }
                break;
            case WPN_HEART:
                if (!SW_SHAREWARE)
                {
                    InitWeaponHeart(pp);
                }
                else
                {
                    PutStringInfo(pp,"Order the full version");
                }
                break;
            case WPN_GRENADE:
                InitWeaponGrenade(pp);
                break;
            case WPN_MINE:
                //InitChops(pp);
                InitWeaponMine(pp);
                break;
            case 13:
                pp->WpnFirstType = WPN_FIST;
                InitWeaponFist(pp);
                break;
            case 14:
                pp->WpnFirstType = WPN_SWORD;
                InitWeaponSword(pp);
                break;
            }
        }
    }
    else
    {
        pp->KeyPressBits |= SB_FIRST_WEAPON_BIT;
    }

    // Shut that computer chick up if weapon has changed!
    // This really should be handled better, but since there's no usable tracking state for the sounds, the easiest way to handle them is to play on otherwise unused channels.
    if (pp->WpnRocketType != 2 || pp->CurWpn != pp->Wpn[WPN_MICRO])
    {
        pp->InitingNuke = false;
        soundEngine->StopSound(SOURCE_Player, pp, CHAN_WEAPON);
    }
    if (pp->CurWpn != pp->Wpn[WPN_RAIL])
    {
        soundEngine->StopSound(SOURCE_Player, pp, CHAN_ITEM);
    }

    return 0;
}

bool
WeaponOK(PLAYERp pp)
{
    USERp u;
    short min_ammo, WeaponNum, FindWeaponNum;
    static char wpn_order[] = {2,3,4,5,6,7,8,9,1,0};
    unsigned wpn_ndx=0;

    if ((unsigned)pp->PlayerSprite >= MAXSPRITES)
        return(false);

    u = User[pp->PlayerSprite].Data();

    if (u == nullptr)
        return(false);

    // sword
    if (DamageData[u->WeaponNum].max_ammo == -1)
        return true;

    WeaponNum = u->WeaponNum;
    FindWeaponNum = u->WeaponNum;

    min_ammo = DamageData[WeaponNum].min_ammo;

    // if ran out of ammo switch to something else
    if (pp->WpnAmmo[WeaponNum] < min_ammo)
    {
        if (u->WeaponNum == WPN_UZI) pp->WpnUziType = 2; // Set it for retract

        // Still got a nuke, it's ok.
        if (WeaponNum == WPN_MICRO && pp->WpnRocketNuke)
        {
            //pp->WpnRocketType = 2; // Set it to Nuke
            if (!pp->NukeInitialized) pp->TestNukeInit = true;

            u->WeaponNum = WPN_MICRO;
            (*DamageData[u->WeaponNum].Init)(pp);

            return true;
        }

        pp->KeyPressBits &= ~SB_FIRE;

        FindWeaponNum = WPN_SHOTGUN; // Start at the top

        while (true)
        {
            // ran out of weapons - choose SWORD
            if (wpn_ndx > sizeof(wpn_order))
            {
                FindWeaponNum = WPN_SWORD;
                break;
            }

            // if you have the weapon and the ammo is greater than 0
            if (TEST(pp->WpnFlags, BIT(FindWeaponNum)) && pp->WpnAmmo[FindWeaponNum] >= min_ammo)
                break;

            wpn_ndx++;
            FindWeaponNum = wpn_order[wpn_ndx];
        }

        u->WeaponNum = int8_t(FindWeaponNum);

        if (u->WeaponNum == WPN_HOTHEAD)
        {
            pp->WeaponType = WPN_HOTHEAD;
            pp->WpnFlameType = 0;
        }

        (*DamageData[u->WeaponNum].Init)(pp);

        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// X/Y POSITION INLINES
//
//////////////////////////////////////////////////////////////////////////////////////////

inline double pspSinVel(PANEL_SPRITEp const psp, int const ang = INT_MAX)
{
    return psp->vel * synctics * bsinf(ang == INT_MAX ? psp->ang : ang, -22);
}

inline double pspCosVel(PANEL_SPRITEp const psp, int const ang = INT_MAX)
{
    return psp->vel * synctics * bcosf(ang == INT_MAX ? psp->ang : ang, -22);
}

inline double pspPresentRetractScale(int const picnum, double const defaultheight)
{
    double const picheight = tileHeight(picnum);
    return picheight == defaultheight ? 1 : picheight / defaultheight;
}


//////////////////////////////////////////////////////////////////////////////////////////
//
// SWORD
//
//////////////////////////////////////////////////////////////////////////////////////////

static short SwordAngTable[] =
{
    82,
    168,
    256+64
};

short SwordAng = 0;

void
SwordBlur(PANEL_SPRITEp psp)
{
    psp->kill_tics -= synctics;
    if (psp->kill_tics <= 0)
    {
        pKillSprite(psp);
        return;
    }
    else if (psp->kill_tics <= 6)
    {
        SET(psp->flags, PANF_TRANS_FLIP);
    }

    psp->shade += 10;
    // change sprites priority
    REMOVE(psp);
    psp->priority--;
    InsertPanelSprite(psp->PlayerP, psp);
}

void
SpawnSwordBlur(PANEL_SPRITEp psp)
{
    PANEL_SPRITEp nsp;
    //PICITEMp pip;

    if (psp->PlayerP->SwordAng > 200)
        return;

    nsp = pSpawnSprite(psp->PlayerP, nullptr, PRI_BACK, psp->x, psp->y);

    SET(nsp->flags, PANF_WEAPON_SPRITE);
    nsp->ang = psp->ang;
    nsp->vel = psp->vel;
    nsp->PanelSpriteFunc = SwordBlur;
    nsp->kill_tics = 9;
    nsp->shade = psp->shade + 10;

    nsp->picndx = -1;
    nsp->picnum = psp->picndx;

    if (TEST(psp->State->flags, psf_Xflip))
        SET(nsp->flags, PANF_XFLIP);

    nsp->rotate_ang = psp->rotate_ang;
    nsp->scale = psp->scale;

    SET(nsp->flags, PANF_TRANSLUCENT);
}

void pSwordPresent(PANEL_SPRITEp psp);
void pSwordRetract(PANEL_SPRITEp psp);
void pSwordAction(PANEL_SPRITEp psp);
void pSwordRest(PANEL_SPRITEp psp);
void pSwordAttack(PANEL_SPRITEp psp);
void pSwordSlide(PANEL_SPRITEp psp);
void pSwordSlideDown(PANEL_SPRITEp psp);
void pSwordHide(PANEL_SPRITEp psp);
void pSwordUnHide(PANEL_SPRITEp psp);

void pSwordSlideR(PANEL_SPRITEp psp);
void pSwordSlideDownR(PANEL_SPRITEp psp);

extern PANEL_STATE ps_SwordSwing[];
extern PANEL_STATE ps_ReloadSword[];

#define Sword_BEAT_RATE 24
#define Sword_ACTION_RATE 10

PANEL_STATE ps_PresentSword[] =
{
    {ID_SwordPresent0, Sword_BEAT_RATE, pSwordPresent, &ps_PresentSword[0], 0,0,0}
};

PANEL_STATE ps_SwordRest[] =
{
    {ID_SwordPresent0, Sword_BEAT_RATE, pSwordRest, &ps_SwordRest[0], 0,0,0}
};

PANEL_STATE ps_SwordHide[] =
{
    {ID_SwordPresent0, Sword_BEAT_RATE, pSwordHide, &ps_SwordHide[0], 0,0,0}
};

#define SWORD_PAUSE_TICS 10
//#define SWORD_SLIDE_TICS 14
#define SWORD_SLIDE_TICS 10
#define SWORD_MID_SLIDE_TICS 14

PANEL_STATE ps_SwordSwing[] =
{
    {ID_SwordSwing0, SWORD_PAUSE_TICS,                    pNullAnimator,      &ps_SwordSwing[1], 0,0,0},
    {ID_SwordSwing1, SWORD_SLIDE_TICS, /* start slide */ pSwordSlide,        &ps_SwordSwing[2], 0,0,0},
    {ID_SwordSwing1, 0, /* damage */ pSwordAttack,       &ps_SwordSwing[3], psf_QuickCall, 0,0},
    {ID_SwordSwing2, SWORD_MID_SLIDE_TICS, /* mid slide */ pSwordSlideDown,    &ps_SwordSwing[4], 0,0,0},

    {ID_SwordSwing2, 99, /* end slide */ pSwordSlideDown,    &ps_SwordSwing[4], 0,0,0},

    {ID_SwordSwingR1, SWORD_SLIDE_TICS, /* start slide */ pSwordSlideR,       &ps_SwordSwing[6], psf_Xflip, 0,0},
    {ID_SwordSwingR2, 0, /* damage */ pSwordAttack,       &ps_SwordSwing[7], psf_QuickCall|psf_Xflip, 0,0},
    {ID_SwordSwingR2, SWORD_MID_SLIDE_TICS, /* mid slide */ pSwordSlideDownR,   &ps_SwordSwing[8], psf_Xflip, 0,0},

    {ID_SwordSwingR2, 99, /* end slide */ pSwordSlideDownR,   &ps_SwordSwing[8], psf_Xflip, 0,0},
    {ID_SwordSwingR2, 2, /* end slide */ pNullAnimator,      &ps_SwordSwing[1], psf_Xflip, 0,0},
};

PANEL_STATE ps_RetractSword[] =
{
    {ID_SwordPresent0, Sword_BEAT_RATE, pSwordRetract, &ps_RetractSword[0], 0,0,0}
};

#define SWORD_SWAY_AMT 12

// left swing
#define SWORD_XOFF (320 + SWORD_SWAY_AMT)
#define SWORD_YOFF 200

// right swing
#define SWORDR_XOFF (0 - 80)

#define SWORD_VEL 1700
#define SWORD_POWER_VEL 2500


void SpecialUziRetractFunc(PANEL_SPRITEp psp)
{
    psp->backupy();
    psp->y += 4 * synctics;

    if (psp->y >= 200 + tileHeight(psp->picnum))
    {
        pKillSprite(psp);
    }
}

void RetractCurWpn(PLAYERp pp)
{
    // Retract old weapon
    if (pp->CurWpn)
    {
        PANEL_SPRITEp cur,nxt;

        if ((pp->CurWpn == pp->Wpn[WPN_UZI] && pp->WpnUziType == 2) || pp->CurWpn != pp->Wpn[WPN_UZI])
        {
            pSetState(pp->CurWpn, pp->CurWpn->RetractState);
            SET(pp->Flags, PF_WEAPON_RETRACT);
        }

        if (pp->CurWpn->sibling)
        {
            // primarily for double uzi to take down the second uzi
            pSetState(pp->CurWpn->sibling, pp->CurWpn->sibling->RetractState);
        }
        else
        {
            // check for any outstanding siblings that need to go away also
            TRAVERSE(&pp->PanelSpriteList, cur, nxt)
            {
                if (cur->sibling && cur->sibling == pp->CurWpn)
                {
                    // special case for uzi reload pieces
                    cur->picnum = cur->picndx;
                    cur->State = nullptr;
                    cur->PanelSpriteFunc = SpecialUziRetractFunc;
                    cur->sibling = nullptr;
                }
            }
        }
    }
}

void
InitWeaponSword(PLAYERp pp)
{
    PANEL_SPRITEp psp;
    short rnd_num;


    if (Prediction)
        return;

    if (!TEST(pp->WpnFlags, BIT(WPN_SWORD)) ||
        TEST(pp->Flags, PF_WEAPON_RETRACT))
        return;

    // needed for death sequence when the SWORD was your weapon when you died
    if (pp->Wpn[WPN_SWORD] && TEST(pp->Wpn[WPN_SWORD]->flags, PANF_DEATH_HIDE))
    {
        RESET(pp->Wpn[WPN_SWORD]->flags, PANF_DEATH_HIDE);
        RESET(pp->Flags, PF_WEAPON_RETRACT|PF_WEAPON_DOWN);
        pSetState(pp->CurWpn, pp->CurWpn->PresentState);
        return;
    }


    if (!pp->Wpn[WPN_SWORD])
    {
        psp = pp->Wpn[WPN_SWORD] = pSpawnSprite(pp, ps_PresentSword, PRI_MID, SWORD_XOFF, SWORD_YOFF);
        psp->y += tileHeight(psp->picndx);
        psp->backupy();
    }

    if (pp->CurWpn == pp->Wpn[WPN_SWORD])
    {
        return;
    }

    PlayerUpdateWeapon(pp, WPN_SWORD);

    pp->WpnUziType = 2; // Make uzi's go away!
    RetractCurWpn(pp);

    // Set up the new Weapon variables
    psp = pp->CurWpn = pp->Wpn[WPN_SWORD];
    SET(psp->flags, PANF_WEAPON_SPRITE);
    psp->ActionState = ps_SwordSwing;
    psp->RetractState = ps_RetractSword;
    psp->PresentState = ps_PresentSword;
    psp->RestState = ps_SwordRest;
    pSetState(psp, psp->PresentState);

    PlaySound(DIGI_SWORD_UP, pp, v3df_follow|v3df_dontpan);

    if (pp == Player+myconnectindex && PlayClock > 0)
    {
        rnd_num = STD_RANDOM_RANGE(1024);
        if (rnd_num > 900)
            PlaySound(DIGI_TAUNTAI2, pp, v3df_follow|v3df_dontpan);
        else if (rnd_num > 800)
            PlaySound(DIGI_PLAYERYELL1, pp, v3df_follow|v3df_dontpan);
        else if (rnd_num > 700)
            PlaySound(DIGI_PLAYERYELL2, pp, v3df_follow|v3df_dontpan);
        else if (rnd_num > 600)
            PlayerSound(DIGI_ILIKESWORD, v3df_follow|v3df_dontpan,pp);
    }

    psp->PlayerP->KeyPressBits |= SB_FIRE;
}


void
pSwordPresent(PANEL_SPRITEp psp)
{
    if (TEST(psp->PlayerP->Flags, PF_WEAPON_RETRACT))
        return;

    psp->backupy();
    psp->y -= 3 * synctics * pspPresentRetractScale(psp->picndx, 136);

    if (psp->y < SWORD_YOFF)
    {
        psp->oy = psp->y = SWORD_YOFF;
        psp->backupboby();
        pSetState(psp, psp->RestState);
    }
}

//
// LEFT SWING
//

void
pSwordSlide(PANEL_SPRITEp psp)
{
    if (!cl_nomeleeblur)
    {
        SpawnSwordBlur(psp);
    }

    psp->backupcoords();

    psp->x += pspCosVel(psp);
    psp->y -= pspSinVel(psp);

    psp->vel += 24 * synctics;
}

void
pSwordSlideDown(PANEL_SPRITEp psp)
{
    if (!cl_nomeleeblur)
    {
        SpawnSwordBlur(psp);
    }

    auto ang = SwordAng + psp->ang + psp->PlayerP->SwordAng;

    psp->backupcoords();

    psp->x += pspCosVel(psp, ang);
    psp->y -= pspSinVel(psp, ang);

    psp->vel += 20 * synctics;

    if (psp->x < -40)
    {
        // if still holding down the fire key - continue swinging
        if (psp->PlayerP->input.actions & SB_FIRE)
        {
            if (psp->PlayerP->KeyPressBits & SB_FIRE)
            {
                DoPlayerChooseYell(psp->PlayerP);
                // continue to next state to swing right
                pStatePlusOne(psp);
                psp->ox = psp->x = SWORDR_XOFF;
                psp->oy = psp->y = SWORD_YOFF;
                psp->backupboby();
                psp->ang = 1024;
                psp->PlayerP->SwordAng = SwordAngTable[RANDOM_RANGE(SIZ(SwordAngTable))];
                psp->vel = 2500;
                DoPlayerSpriteThrow(psp->PlayerP);
                return;
            }
        }

        // NOT still holding down the fire key - stop swinging
        pSetState(psp, psp->PresentState);
        psp->ox = psp->x = SWORD_XOFF;
        psp->oy = psp->y = SWORD_YOFF + tileHeight(psp->picndx);
        psp->backupboby();
    }
}

//
// RIGHT SWING
//

void
pSwordSlideR(PANEL_SPRITEp psp)
{
    if (!cl_nomeleeblur)
    {
        SpawnSwordBlur(psp);
    }

    psp->backupcoords();

    psp->x -= pspCosVel(psp);
    psp->y += pspSinVel(psp);

    psp->vel += 24 * synctics;
}

void
pSwordSlideDownR(PANEL_SPRITEp psp)
{
    if (!cl_nomeleeblur)
    {
        SpawnSwordBlur(psp);
    }

    auto ang = SwordAng + psp->ang - psp->PlayerP->SwordAng;

    psp->backupcoords();

    psp->x -= pspCosVel(psp, ang);
    psp->y += pspSinVel(psp, ang);

    psp->vel += 24 * synctics;

    if (psp->x > 350)
    {
        // if still holding down the fire key - continue swinging
        if (psp->PlayerP->input.actions & SB_FIRE)
        {
            if (psp->PlayerP->KeyPressBits & SB_FIRE)
            {
                DoPlayerChooseYell(psp->PlayerP);
                // back to action state
                pStatePlusOne(psp);
                psp->ox = psp->x = SWORD_XOFF + 80;
                psp->oy = psp->y = SWORD_YOFF;
                psp->backupboby();
                psp->PlayerP->SwordAng = SwordAngTable[RANDOM_RANGE(SIZ(SwordAngTable))];
                psp->ang = 1024;
                psp->vel = 2500;
                DoPlayerSpriteThrow(psp->PlayerP);
                return;
            }
        }

        // NOT still holding down the fire key - stop swinging
        pSetState(psp, psp->PresentState);
        psp->ox = psp->x = SWORD_XOFF;
        psp->oy = psp->y = SWORD_YOFF + tileHeight(psp->picndx);
        psp->backupboby();
    }
}

void
pSwordBobSetup(PANEL_SPRITEp psp)
{
    if (TEST(psp->flags, PANF_BOB))
        return;

    psp->backupbobcoords();

    psp->sin_amt = SWORD_SWAY_AMT;
    psp->sin_ndx = 0;
    psp->bob_height_divider = 8;
}

void
pSwordHide(PANEL_SPRITEp psp)
{
    short picnum = psp->picndx;

    psp->backupy();
    psp->y += 3 * synctics;

    if (psp->y >= SWORD_YOFF + tileHeight(picnum))
    {
        psp->oy = psp->y = SWORD_YOFF + tileHeight(picnum);
        psp->ox = psp->x = SWORD_XOFF;

        pWeaponUnHideKeys(psp, psp->PresentState);
    }
}

void
pSwordRest(PANEL_SPRITEp psp)
{
    bool force = !!TEST(psp->flags, PANF_UNHIDE_SHOOT);

    if (pWeaponHideKeys(psp, ps_SwordHide))
        return;

    psp->yorig += synctics;

    if (psp->yorig > SWORD_YOFF)
    {
        psp->yorig = SWORD_YOFF;
    }

    pSwordBobSetup(psp);
    pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP));

    force = !!TEST(psp->flags, PANF_UNHIDE_SHOOT);

    if ((psp->PlayerP->input.actions & SB_FIRE) || force)
    {
        if ((psp->PlayerP->KeyPressBits & SB_FIRE) || force)
        {
            RESET(psp->flags, PANF_UNHIDE_SHOOT);

            pSetState(psp, psp->ActionState);

            psp->vel = 2500;

            psp->ang = 1024;
            psp->PlayerP->SwordAng = SwordAngTable[RANDOM_RANGE(SIZ(SwordAngTable))];
            DoPlayerSpriteThrow(psp->PlayerP);
        }
    }
}

void
pSwordAction(PANEL_SPRITEp psp)
{
    pSwordBobSetup(psp);
    pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP));
}


void
pSwordAttack(PANEL_SPRITEp psp)
{

    InitSwordAttack(psp->PlayerP);
}

void
pSwordRetract(PANEL_SPRITEp psp)
{
    short picnum = psp->picndx;

    psp->backupy();
    psp->y += 3 * synctics * pspPresentRetractScale(picnum, 136);

    if (psp->y >= SWORD_YOFF + tileHeight(picnum))
    {
        RESET(psp->PlayerP->Flags, PF_WEAPON_RETRACT);
        psp->PlayerP->Wpn[WPN_SWORD] = nullptr;
        pKillSprite(psp);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// STAR
//
//////////////////////////////////////////////////////////////////////////////////////////

void pStarPresent(PANEL_SPRITEp psp);
void pStarRetract(PANEL_SPRITEp psp);
void pStarAction(PANEL_SPRITEp psp);
void pStarRest(PANEL_SPRITEp psp);
void pStarThrow(PANEL_SPRITEp psp);

void pStarGrab(PANEL_SPRITEp psp);
void pStarDone(PANEL_SPRITEp psp);
void pStarFollowThru(PANEL_SPRITEp psp);
void pStarFollowThru2(PANEL_SPRITEp psp);
void pStarStartThrow(PANEL_SPRITEp psp);

void pStarFollowUp(PANEL_SPRITEp psp);
void pStarFollowDown(PANEL_SPRITEp psp);

void pStarHide(PANEL_SPRITEp psp);
void pStarRestTest(PANEL_SPRITEp psp);

extern PANEL_STATE ps_StarThrow[];

#define PRESENT_STAR_RATE 5

PANEL_STATE ps_PresentStar[] =
{
    {ID_StarPresent0, PRESENT_STAR_RATE, pStarPresent, &ps_PresentStar[0], 0,0,0}
};

PANEL_STATE ps_StarHide[] =
{
    {ID_StarPresent0, PRESENT_STAR_RATE, pStarHide, &ps_StarHide[0], 0,0,0}
};

#define Star_RATE 2  // was 5

PANEL_STATE ps_StarRest[] =
{
    {ID_StarPresent0, Star_RATE,        pStarRest,          &ps_StarRest[0], 0,0,0},
};

PANEL_STATE ps_ThrowStar[] =
{
    {ID_StarDown0, Star_RATE+3,          pNullAnimator,      &ps_ThrowStar[1], 0,0,0},
    {ID_StarDown1, Star_RATE+3,          pNullAnimator,      &ps_ThrowStar[2], 0,0,0},
    {ID_StarDown1, Star_RATE*2,          pNullAnimator,      &ps_ThrowStar[3], psf_Invisible, 0,0},
    {ID_StarDown1, Star_RATE,            pNullAnimator,      &ps_ThrowStar[4], 0,0,0},
    {ID_StarDown0, Star_RATE,           pNullAnimator,      &ps_ThrowStar[5], 0,0,0},
    {ID_ThrowStar0, 1,                  pNullAnimator,       &ps_ThrowStar[6], 0,0,0},
    {ID_ThrowStar0, Star_RATE,          pStarThrow,         &ps_ThrowStar[7], psf_QuickCall, 0,0},
    {ID_ThrowStar0, Star_RATE,          pNullAnimator,      &ps_ThrowStar[8], 0,0,0},
    {ID_ThrowStar1, Star_RATE,          pNullAnimator,      &ps_ThrowStar[9], 0,0,0},
    {ID_ThrowStar2, Star_RATE*2,          pNullAnimator,      &ps_ThrowStar[10], 0,0,0},
    {ID_ThrowStar3, Star_RATE*2,          pNullAnimator,      &ps_ThrowStar[11], 0,0,0},
    {ID_ThrowStar4, Star_RATE*2,          pNullAnimator,      &ps_ThrowStar[12], 0,0,0},
    // start up
    {ID_StarDown1, Star_RATE+3,         pNullAnimator,         &ps_ThrowStar[13], 0,0,0},
    {ID_StarDown0, Star_RATE+3,         pNullAnimator,         &ps_ThrowStar[14], 0,0,0},
    {ID_StarPresent0, Star_RATE+3,         pNullAnimator,         &ps_ThrowStar[15], 0,0,0},
    // maybe to directly to rest state
    {ID_StarDown0, 3,                    pStarRestTest,      &ps_ThrowStar[16], psf_QuickCall, 0,0},
    // if holding the fire key we get to here
    {ID_ThrowStar4, 3,                    pNullAnimator,      &ps_ThrowStar[5], 0,0,0},
};

PANEL_STATE ps_RetractStar[] =
{
    {ID_StarPresent0, PRESENT_STAR_RATE, pStarRetract, &ps_RetractStar[0], 0,0,0}
};

//
// Right hand star routines
//

//#define STAR_YOFF 220
//#define STAR_XOFF (160+25)
#define STAR_YOFF 208
#define STAR_XOFF (160+80)

void
pStarRestTest(PANEL_SPRITEp psp)
{
    if (psp->PlayerP->input.actions & SB_FIRE)
    {
        if (psp->PlayerP->KeyPressBits & SB_FIRE)
        {
            if (!WeaponOK(psp->PlayerP))
                return;

            // continue to next state to swing right
            DoPlayerChooseYell(psp->PlayerP);
            pStatePlusOne(psp);
            return;
        }
    }

    pSetState(psp, psp->RestState);
}

void
InitWeaponStar(PLAYERp pp)
{
    PANEL_SPRITEp psp = nullptr;

    if (Prediction)
        return;

    if (!TEST(pp->WpnFlags, BIT(WPN_STAR)) ||
        pp->WpnAmmo[WPN_STAR] < 3 ||
        TEST(pp->Flags, PF_WEAPON_RETRACT))
    {
        //pp->WpnFirstType = WPN_SWORD;
        //InitWeaponSword(pp);
        return;
    }

    // needed for death sequence when the STAR was your weapon when you died
    if (pp->Wpn[WPN_STAR] && TEST(pp->Wpn[WPN_STAR]->flags, PANF_DEATH_HIDE))
    {
        RESET(pp->Wpn[WPN_STAR]->flags, PANF_DEATH_HIDE);
        RESET(pp->Flags, PF_WEAPON_RETRACT);
        pSetState(pp->CurWpn, pp->CurWpn->PresentState);
        return;
    }

    if (!pp->Wpn[WPN_STAR])
    {
        psp = pp->Wpn[WPN_STAR] = pSpawnSprite(pp, ps_PresentStar, PRI_MID, STAR_XOFF, STAR_YOFF);
        psp->y += tileHeight(psp->picndx);
        psp->backupy();
    }

    if (pp->CurWpn == pp->Wpn[WPN_STAR])
    {
        return;
    }

    PlayerUpdateWeapon(pp, WPN_STAR);

    pp->WpnUziType = 2; // Make uzi's go away!
    RetractCurWpn(pp);

    // Set up the new Weapon variables
    pp->CurWpn = pp->Wpn[WPN_STAR];
    SET(psp->flags, PANF_WEAPON_SPRITE);
    psp->ActionState = ps_ThrowStar;
    //psp->ActionState = &ps_ThrowStar[1];
    psp->RetractState = ps_RetractStar;
    psp->PresentState = ps_PresentStar;
    psp->RestState = ps_StarRest;
    //psp->RestState = ps_ThrowStar;
    pSetState(psp, psp->PresentState);

    PlaySound(DIGI_PULL, pp, v3df_follow|v3df_dontpan);
    if (STD_RANDOM_RANGE(1000) > 900 && pp == Player+myconnectindex)
    {
        if (!sw_darts)
            PlayerSound(DIGI_ILIKESHURIKEN, v3df_follow|v3df_dontpan,pp);
    }

    psp->PlayerP->KeyPressBits |= SB_FIRE;
}

void
pStarPresent(PANEL_SPRITEp psp)
{
    if (TEST(psp->PlayerP->Flags, PF_WEAPON_RETRACT))
        return;

    psp->backupy();
    psp->y -= 3 * synctics;

    if (psp->y < STAR_YOFF)
    {
        psp->oy = psp->y = STAR_YOFF;
    }

    if (psp->y <= STAR_YOFF)
    {
        pSetState(psp, psp->RestState);
    }
}

void
pStarBobSetup(PANEL_SPRITEp psp)
{
    if (TEST(psp->flags, PANF_BOB))
        return;

    psp->backupbobcoords();

    psp->sin_amt = 10;
    psp->sin_ndx = 0;
    psp->bob_height_divider = 8;
}

void
pLStarBobSetup(PANEL_SPRITEp psp)
{
    if (TEST(psp->flags, PANF_BOB))
        return;

    psp->backupbobcoords();

    psp->sin_amt = 6;
    psp->sin_ndx = 0;
    psp->bob_height_divider = 16;
}

void
pStarHide(PANEL_SPRITEp psp)
{
    short picnum = psp->picndx;

    psp->backupy();
    psp->y += 3 * synctics;

    if (psp->y >= STAR_YOFF + tileHeight(picnum))
    {
        psp->oy = psp->y = STAR_YOFF + tileHeight(picnum);

        pWeaponUnHideKeys(psp, psp->PresentState);
    }
}

void
pStarRest(PANEL_SPRITEp psp)
{
    bool force = !!TEST(psp->flags, PANF_UNHIDE_SHOOT);

    if (pWeaponHideKeys(psp, ps_StarHide))
        return;

    pStarBobSetup(psp);
    pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP));

    if ((psp->PlayerP->input.actions & SB_FIRE) || force)
    {
        if ((psp->PlayerP->KeyPressBits & SB_FIRE) || force)
        {
            RESET(psp->flags, PANF_UNHIDE_SHOOT);

            if (!WeaponOK(psp->PlayerP))
                return;

//            //DSPRINTF(ds,"StarFire");
//            MONO_PRINT(ds);

            DoPlayerChooseYell(psp->PlayerP);

            pSetState(psp, psp->ActionState);
            DoPlayerSpriteThrow(psp->PlayerP);
        }
    }
    else
        WeaponOK(psp->PlayerP);
}


void
pStarAction(PANEL_SPRITEp psp)
{
    pStarBobSetup(psp);
    pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP));
}


void
pStarThrow(PANEL_SPRITEp psp)
{
    InitStar(psp->PlayerP);
}

void
pStarRetract(PANEL_SPRITEp psp)
{
    short picnum = psp->picndx;

    psp->backupy();
    psp->y += 3 * synctics;

    if (psp->y >= STAR_YOFF + tileHeight(picnum))
    {
        RESET(psp->PlayerP->Flags, PF_WEAPON_RETRACT);

        // kill only in its own routine
        psp->PlayerP->Wpn[WPN_STAR] = nullptr;
        pKillSprite(psp);
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
//
// UZI
//
//////////////////////////////////////////////////////////////////////////////////////////

void pUziPresent(PANEL_SPRITEp);
void pUziFire(PANEL_SPRITEp);
void pUziRetract(PANEL_SPRITEp);
void pUziAction(PANEL_SPRITEp);
void pUziRest(PANEL_SPRITEp);
void pUziHide(PANEL_SPRITEp);
void pUziPresentReload(PANEL_SPRITEp);

void pSpawnUziClip(PANEL_SPRITEp);
void pSpawnUziReload(PANEL_SPRITEp);
void pUziReload(PANEL_SPRITEp);
void pUziReloadRetract(PANEL_SPRITEp);
void pUziClip(PANEL_SPRITEp);
void pUziDoneReload(PANEL_SPRITEp);

void pUziEjectDown(PANEL_SPRITEp);
void pUziEjectUp(PANEL_SPRITEp);

// CTW MODIFICATION
//void SetVisNorm(void);
int SetVisNorm(void);
// CTW MODIFICATION END
pANIMATOR pSetVisNorm;

// Right Uzi
PANEL_STATE ps_FireUzi[] =
{
    {ID_UziPresent0, 3, pUziRest, &ps_FireUzi[0], 0,0,0},
    {ID_UziFire0, 1, pUziAction, &ps_FireUzi[2], psf_ShadeHalf, 0,0},

    {ID_UziFire1, 0, pUziFire, &ps_FireUzi[3], psf_ShadeNone|psf_QuickCall, 0,0},
    {ID_UziFire1, 4, pUziAction, &ps_FireUzi[4], psf_ShadeNone, 0,0},
    {ID_UziFire1, 0, pSetVisNorm, &ps_FireUzi[5], psf_ShadeNone|psf_QuickCall, 0,0},
    {ID_UziFire1, 4, pUziAction, &ps_FireUzi[6], psf_ShadeNone, 0,0},
    {ID_UziFire1, 0, pUziFire, &ps_FireUzi[7], psf_ShadeNone|psf_QuickCall, 0,0},
    {ID_UziFire1, 4, pUziAction, &ps_FireUzi[8], psf_ShadeNone, 0,0},
    {ID_UziFire1, 0, pSetVisNorm, &ps_FireUzi[9], psf_ShadeNone, 0,0},

    {ID_UziFire0, 4, pUziAction, &ps_FireUzi[10], psf_ShadeHalf, 0,0},
    {ID_UziFire0, 0, pUziFire, &ps_FireUzi[11], psf_QuickCall, 0,0},
    {ID_UziFire0, 4, pUziAction, &ps_FireUzi[12], psf_ShadeHalf, 0,0},
    {ID_UziFire0, 4, pUziAction, &ps_FireUzi[13], psf_ShadeHalf, 0,0},

    {ID_UziFire1, 5, pUziRest, &ps_FireUzi[0], psf_ShadeNone|psf_QuickCall, 0,0},
};

#define PRESENT_UZI_RATE 6
#define RELOAD_UZI_RATE 1

PANEL_STATE ps_UziNull[] =
{
    {ID_UziPresent0, PRESENT_UZI_RATE, pNullAnimator, &ps_UziNull[0], 0,0,0}
};

PANEL_STATE ps_UziHide[] =
{
    {ID_UziPresent0, PRESENT_UZI_RATE, pUziHide, &ps_UziHide[0], 0,0,0}
};

PANEL_STATE ps_PresentUzi[] =
{
    {ID_UziPresent0, PRESENT_UZI_RATE, pUziPresent, &ps_PresentUzi[0], 0,0,0},
};

// present of secondary uzi for reload needs to be faster
PANEL_STATE ps_PresentUziReload[] =
{
    {ID_UziPresent0, RELOAD_UZI_RATE, pUziPresentReload, &ps_PresentUziReload[0], 0,0,0},
};

PANEL_STATE ps_RetractUzi[] =
{
    {ID_UziPresent0, PRESENT_UZI_RATE, pUziRetract, &ps_RetractUzi[0], 0,0,0},
};

// Left Uzi

PANEL_STATE ps_FireUzi2[] =
{
    {ID_Uzi2Present0, 3, pUziRest, &ps_FireUzi2[0], psf_Xflip, 0,0},
    {ID_Uzi2Fire0, 1, pUziAction, &ps_FireUzi2[2], psf_ShadeHalf|psf_Xflip, 0,0},

    {ID_Uzi2Fire1, 0, pUziFire, &ps_FireUzi2[3], psf_ShadeNone|psf_QuickCall|psf_Xflip, 0,0},
    {ID_Uzi2Fire1, 4, pUziAction, &ps_FireUzi2[4], psf_ShadeNone|psf_Xflip, 0,0},
    {ID_Uzi2Fire1, 4, pUziAction, &ps_FireUzi2[5], psf_ShadeNone|psf_Xflip, 0,0},
    {ID_Uzi2Fire1, 0, pUziFire, &ps_FireUzi2[6], psf_ShadeNone|psf_QuickCall|psf_Xflip, 0,0},
    {ID_Uzi2Fire1, 4, pUziAction, &ps_FireUzi2[7], psf_ShadeNone|psf_Xflip, 0,0},

    {ID_Uzi2Fire0, 4, pUziAction, &ps_FireUzi2[8], psf_ShadeHalf|psf_Xflip, 0,0},
    {ID_Uzi2Fire0, 0, pUziFire, &ps_FireUzi2[9], psf_ShadeHalf|psf_QuickCall|psf_Xflip, 0,0},
    {ID_Uzi2Fire0, 4, pUziAction, &ps_FireUzi2[10], psf_ShadeHalf|psf_Xflip, 0,0},
    {ID_Uzi2Fire0, 4, pUziAction, &ps_FireUzi2[11], psf_ShadeHalf|psf_Xflip, 0,0},

    {ID_Uzi2Fire1, 5, pUziRest, &ps_FireUzi2[0], psf_QuickCall, 0,0},
};


PANEL_STATE ps_PresentUzi2[] =
{
    {ID_Uzi2Present0, PRESENT_UZI_RATE, pUziPresent, &ps_PresentUzi2[0], psf_Xflip, 0,0},
};

PANEL_STATE ps_Uzi2Hide[] =
{
    {ID_Uzi2Present0, PRESENT_UZI_RATE, pUziHide, &ps_Uzi2Hide[0], psf_Xflip, 0,0},
};

PANEL_STATE ps_RetractUzi2[] =
{
    {ID_Uzi2Present0, PRESENT_UZI_RATE, pUziRetract, &ps_RetractUzi2[0], psf_Xflip, 0,0},
};

PANEL_STATE ps_Uzi2Suicide[] =
{
    {ID_Uzi2Present0, PRESENT_UZI_RATE, pSuicide, &ps_Uzi2Suicide[0], psf_Xflip, 0,0}
};

PANEL_STATE ps_Uzi2Null[] =
{
    {ID_Uzi2Present0, PRESENT_UZI_RATE, pNullAnimator, &ps_Uzi2Null[0], psf_Xflip, 0,0}
};

PANEL_STATE ps_UziEject[] =
{
    {ID_UziPresent0, 1, pNullAnimator, &ps_UziEject[1], 0,0,0},
    {ID_UziPresent0, RELOAD_UZI_RATE, pUziEjectDown, &ps_UziEject[1], 0,0,0},
    {ID_UziEject0, RELOAD_UZI_RATE, pUziEjectUp, &ps_UziEject[2], 0,0,0},
    {ID_UziEject0, 1, pNullAnimator, &ps_UziEject[4], 0,0,0},
    {ID_UziEject0, RELOAD_UZI_RATE, pSpawnUziClip, &ps_UziEject[5], psf_QuickCall, 0,0},
    {ID_UziEject0, RELOAD_UZI_RATE, pNullAnimator, &ps_UziEject[5], 0,0,0},
};

PANEL_STATE ps_UziClip[] =
{
    {ID_UziClip0, RELOAD_UZI_RATE, pUziClip, &ps_UziClip[0], 0,0,0}
};

PANEL_STATE ps_UziReload[] =
{
    {ID_UziReload0, RELOAD_UZI_RATE, pUziReload, &ps_UziReload[0], 0,0,0},
    {ID_UziReload0, RELOAD_UZI_RATE, pUziReloadRetract, &ps_UziReload[1], 0,0,0}
};

PANEL_STATE ps_UziDoneReload[] =
{
    {ID_UziEject0, RELOAD_UZI_RATE, pUziDoneReload, &ps_UziDoneReload[0], 0,0,0}
};

#define CHAMBER_REST 0
#define CHAMBER_FIRE 1
#define CHAMBER_RELOAD 2
void
pUziOverlays(PANEL_SPRITEp psp, short mode)
{
#define UZI_CHAMBER_XOFF 32
#define UZI_CHAMBER_YOFF -73

#define UZI_CHAMBERRELOAD_XOFF 14
#define UZI_CHAMBERRELOAD_YOFF -100

    if (!TEST(psp->flags, PANF_SECONDARY)) return;

    if (psp->over[0].xoff == -1)
    {
        psp->over[0].xoff = UZI_CHAMBER_XOFF;
        psp->over[0].yoff = UZI_CHAMBER_YOFF;
    }

    switch (mode)
    {
    case 0: // At rest
        psp->over[0].pic = UZI_COPEN;
        break;
    case 1: // Firing
        psp->over[0].pic = UZI_CLIT;
        break;
    case 2: // Reloading
        psp->over[0].pic = UZI_CRELOAD;
        psp->over[0].xoff = UZI_CHAMBERRELOAD_XOFF;
        psp->over[0].yoff = UZI_CHAMBERRELOAD_YOFF;
        break;
    }
}

#define UZI_CLIP_XOFF 16
#define UZI_CLIP_YOFF (-84)

//#define UZI_XOFF (80)
#define UZI_XOFF (100)
#define UZI_YOFF 208

#define UZI_RELOAD_YOFF 200


//
// Uzi Reload
//

void
pUziEjectDown(PANEL_SPRITEp gun)
{
    gun->oy = gun->y;
    gun->y += 5 * synctics;

    if (gun->y > 260)
    {
        gun->oy = gun->y = 260;
        pStatePlusOne(gun);
    }
}

void
pUziEjectUp(PANEL_SPRITEp gun)
{

    pUziOverlays(gun, CHAMBER_RELOAD);

    gun->oy = gun->y;
    gun->y -= 5 * synctics;

    if (gun->y < UZI_RELOAD_YOFF)
    {
        gun->oy = gun->y = UZI_RELOAD_YOFF;
        pStatePlusOne(gun);
    }
}


void
pSpawnUziClip(PANEL_SPRITEp gun)
{
    PANEL_SPRITEp New;

    PlaySound(DIGI_REMOVECLIP, gun->PlayerP,v3df_follow|v3df_dontpan|v3df_doppler|v3df_follow);

    if (TEST(gun->flags, PANF_XFLIP))
    {
        New = pSpawnSprite(gun->PlayerP, ps_UziClip, PRI_BACK, gun->x - UZI_CLIP_XOFF, gun->y + UZI_CLIP_YOFF);
        SET(New->flags, PANF_XFLIP);
        New->ang = NORM_ANGLE(1024 + 256 + 22);
        New->ang = NORM_ANGLE(New->ang + 512);
    }
    else
    {
        New = pSpawnSprite(gun->PlayerP, ps_UziClip, PRI_BACK, gun->x + UZI_CLIP_XOFF, gun->y + UZI_CLIP_YOFF);
        New->ang = NORM_ANGLE(1024 + 256 - 22);
    }

    New->vel = 1050;
    SET(New->flags, PANF_WEAPON_SPRITE);


    // carry Eject sprite with clip
    New->sibling = gun;
}

void
pSpawnUziReload(PANEL_SPRITEp oclip)
{
    PANEL_SPRITEp nclip;

    nclip = pSpawnSprite(oclip->PlayerP, ps_UziReload, PRI_BACK, oclip->x, UZI_RELOAD_YOFF);
    SET(nclip->flags, PANF_WEAPON_SPRITE);

    if (TEST(oclip->flags, PANF_XFLIP))
        SET(nclip->flags, PANF_XFLIP);

    // move Reload in oposite direction of clip
    nclip->ang = NORM_ANGLE(oclip->ang + 1024);
    nclip->vel = 900;

    // move gun sprite from clip to reload
    nclip->sibling = oclip->sibling;
}

void
pUziReload(PANEL_SPRITEp nclip)
{
    PANEL_SPRITEp gun = nclip->sibling;

    nclip->vel += 14 * synctics;

    nclip->ox = nclip->x;
    nclip->oy = nclip->y;

    nclip->x += pspCosVel(nclip);
    nclip->y -= pspSinVel(nclip);

    gun->ox = gun->x;
    gun->oy = gun->y;

    gun->x -= pspCosVel(gun);
    gun->y += pspSinVel(gun);

    if (TEST(nclip->flags, PANF_XFLIP))
    {
        if (nclip->x < gun->x)
        {
            PlaySound(DIGI_REPLACECLIP, nclip->PlayerP,v3df_follow|v3df_dontpan|v3df_doppler);

            nclip->ox = nclip->x = gun->x - UZI_CLIP_XOFF;
            nclip->oy = nclip->y = gun->y + UZI_CLIP_YOFF;
            nclip->vel = 680;
            nclip->ang = NORM_ANGLE(nclip->ang - 128 - 64);
            // go to retract phase
            pSetState(nclip, &ps_UziReload[1]);
        }
    }
    else
    {
        if (nclip->x > gun->x)
        {
            PlaySound(DIGI_REPLACECLIP, nclip->PlayerP,v3df_follow|v3df_dontpan|v3df_doppler);

            nclip->ox = nclip->x = gun->x + UZI_CLIP_XOFF;
            nclip->oy = nclip->y = gun->y + UZI_CLIP_YOFF;
            nclip->vel = 680;
            nclip->ang = NORM_ANGLE(nclip->ang + 128 + 64);
            // go to retract phase
            pSetState(nclip, &ps_UziReload[1]);
        }
    }
}

void
pUziReloadRetract(PANEL_SPRITEp nclip)
{
    PANEL_SPRITEp gun = nclip->sibling;

    double xadj = pspCosVel(nclip);
    double yadj = pspSinVel(nclip);

    nclip->vel += 18 * synctics;

    nclip->backupcoords();

    nclip->x -= xadj;
    nclip->y += yadj;

    gun->backupcoords();

    gun->x -= xadj;
    gun->y += yadj;

    if (gun->y > UZI_RELOAD_YOFF + tileHeight(gun->picndx))
    {
        pSetState(gun, ps_UziDoneReload);
        pKillSprite(nclip);
    }
}

void
pUziDoneReload(PANEL_SPRITEp psp)
{
    PLAYERp pp = psp->PlayerP;


    if (TEST(psp->flags, PANF_PRIMARY) && pp->WpnUziType == 3)
    {
        // if 2 uzi's and the first one has been reloaded
        // kill the first one and make the second one the CurWeapon
        // Set uzi's back to previous state
        PANEL_SPRITEp New;


        if (pp->WpnUziType > 2)
            pp->WpnUziType -= 3;

        New = InitWeaponUziSecondaryReload(psp);
        pp->Wpn[WPN_UZI] = New;
        pp->CurWpn = New;
        pp->CurWpn->sibling = nullptr;

        pKillSprite(psp);
        return;
    }
    else
    {
        // Reset everything

        // Set uzi's back to previous state
        if (pp->WpnUziType > 2)
            pp->WpnUziType -= 3;

        // reset uzi variable
        pp->Wpn[WPN_UZI] = nullptr;
        pp->CurWpn = nullptr;

        // kill uzi eject sequence for good
        pKillSprite(psp);

        // give the uzi back
        InitWeaponUzi(pp);
    }
}

void
pUziClip(PANEL_SPRITEp oclip)
{

    oclip->ox = oclip->x;
    oclip->oy = oclip->y;

    oclip->x += pspCosVel(oclip);
    oclip->y -= pspSinVel(oclip);

    if (oclip->y > UZI_RELOAD_YOFF)
    {
        PANEL_SPRITEp gun = oclip->sibling;

        // as synctics gets bigger, oclip->x can be way off
        // when clip goes off the screen - recalc oclip->x from scratch
        // so it will end up the same for all synctic values
        for (oclip->x = oclip->ox, oclip->y = oclip->oy; oclip->y < UZI_RELOAD_YOFF; )
        {
            oclip->x += oclip->vel * bcosf(oclip->ang, -22);
            oclip->y -= oclip->vel * bsinf(oclip->ang, -22);
        }

        oclip->ox = oclip->x;
        oclip->oy = oclip->y = UZI_RELOAD_YOFF;

        gun->vel = 800;
        gun->ang = NORM_ANGLE(oclip->ang + 1024);

        pSpawnUziReload(oclip);
        pKillSprite(oclip);
    }
}

//
// Uzi Basic Stuff
//

void
InitWeaponUzi(PLAYERp pp)
{
    PANEL_SPRITEp psp = nullptr;

    if (Prediction)
        return;

    pp->WeaponType = WPN_UZI;

    // make sure you have the uzi, uzi ammo, and not retracting another
    // weapon
    if (!TEST(pp->WpnFlags, BIT(WPN_UZI)) ||
//        pp->WpnAmmo[WPN_UZI] <= 0 ||
        TEST(pp->Flags, PF_WEAPON_RETRACT))
        return;

    // if players uzi is null
    if (!pp->Wpn[WPN_UZI])
    {
        psp = pp->Wpn[WPN_UZI] = pSpawnSprite(pp, ps_PresentUzi, PRI_MID, 160 + UZI_XOFF, UZI_YOFF);
        psp->y += tileHeight(psp->picndx);
        psp->backupy();
    }

    // if Current weapon is uzi
    if (pp->CurWpn == pp->Wpn[WPN_UZI])
    {
        // Retracting other uzi?
        if (pp->CurWpn->sibling && pp->WpnUziType == 1)
        {
            RetractCurWpn(pp);
        }
        else
        // Is player toggling between one and two uzi's?
        if (pp->CurWpn->sibling && TEST(pp->Wpn[WPN_UZI]->flags, PANF_PRIMARY) && pp->WpnUziType == 0)
        {
            if (!TEST(pp->CurWpn->flags, PANF_RELOAD))
                InitWeaponUzi2(pp->Wpn[WPN_UZI]);
        }

        // if actually picked an uzi up and don't currently have double uzi
        if (TEST(pp->Flags, PF_PICKED_UP_AN_UZI) && !TEST(pp->Wpn[WPN_UZI]->flags, PANF_PRIMARY))
        {
            RESET(pp->Flags, PF_PICKED_UP_AN_UZI);

            if (!TEST(pp->CurWpn->flags, PANF_RELOAD))
                InitWeaponUzi2(pp->Wpn[WPN_UZI]);
        }
        return;
    }
    else
    {
        RESET(pp->Flags, PF_PICKED_UP_AN_UZI);
    }

    PlayerUpdateWeapon(pp, WPN_UZI);

    RetractCurWpn(pp);

    // Set up the new Weapon variables
    pp->CurWpn = pp->Wpn[WPN_UZI];
    SET(psp->flags, PANF_WEAPON_SPRITE);
    psp->ActionState = &ps_FireUzi[1];
    psp->RetractState = ps_RetractUzi;
    psp->PresentState = ps_PresentUzi;
    psp->RestState = ps_FireUzi;
    pSetState(psp, psp->PresentState);

    // power up
    // NOTE: PRIMARY is ONLY set when there is a powerup
    if (TEST(pp->Flags, PF_TWO_UZI))
    {
        InitWeaponUzi2(psp);
    }

    PlaySound(DIGI_UZI_UP, pp, v3df_follow);

    psp->PlayerP->KeyPressBits |= SB_FIRE;
}

PANEL_SPRITEp
InitWeaponUzi2(PANEL_SPRITEp uzi_orig)
{
    PANEL_SPRITEp New;
    PLAYERp pp = uzi_orig->PlayerP;


    // There is already a second uzi, or it's retracting
    if (pp->WpnUziType == 1 || pp->CurWpn->sibling || TEST(pp->Flags, PF_WEAPON_RETRACT)) return nullptr;

    // NOTE: PRIMARY is ONLY set when there is a powerup
    SET(uzi_orig->flags, PANF_PRIMARY);

    // Spawning a 2nd uzi, set weapon mode
    pp->WpnUziType = 0; // 0 is up, 1 is retract

    New = pSpawnSprite(pp, ps_PresentUzi2, PRI_MID, 160 - UZI_XOFF, UZI_YOFF);
    New->y += tileHeight(New->picndx);
    New->oy = New->y;
    uzi_orig->sibling = New;

    // Set up the New Weapon variables
    SET(New->flags, PANF_WEAPON_SPRITE);
    New->ActionState = &ps_FireUzi2[1];
    New->RetractState = ps_RetractUzi2;
    New->PresentState = ps_PresentUzi2;
    New->RestState = ps_FireUzi2;
    pSetState(New, New->PresentState);

    New->sibling = uzi_orig;
    SET(New->flags, PANF_SECONDARY);
    pUziOverlays(New, CHAMBER_REST);

    return New;
}

PANEL_SPRITEp
InitWeaponUziSecondaryReload(PANEL_SPRITEp uzi_orig)
{
    PANEL_SPRITEp New;
    PLAYERp pp = uzi_orig->PlayerP;

    New = pSpawnSprite(pp, ps_PresentUzi, PRI_MID, 160 - UZI_XOFF, UZI_YOFF);
    New->y += tileHeight(New->picndx);
    New->oy = New->y;

    SET(New->flags, PANF_XFLIP);

    // Set up the New Weapon variables
    SET(New->flags, PANF_WEAPON_SPRITE);
    New->ActionState = ps_UziEject;
    New->RetractState = ps_RetractUzi;
    New->PresentState = ps_PresentUzi;
    New->RestState = ps_UziEject;
    // pSetState(New, New->PresentState);
    pSetState(New, ps_PresentUziReload);

    New->sibling = uzi_orig;
    SET(New->flags, PANF_SECONDARY|PANF_RELOAD);

    return New;
}

void
pUziPresent(PANEL_SPRITEp psp)
{
    if (TEST(psp->PlayerP->Flags, PF_WEAPON_RETRACT))
        return;

    psp->backupy();
    psp->y -= 3 * synctics;

    if (psp->PlayerP->WpnUziType)
    {
        psp->PlayerP->WpnReloadState = 2;
    }

    if (psp->y < UZI_YOFF)
    {
        RESET(psp->flags, PANF_RELOAD);

        psp->oy = psp->y = UZI_YOFF;
        psp->backupx();
        psp->backupbobcoords();
        pSetState(psp, psp->RestState);
    }
}

// same as pUziPresent only faster for reload sequence
void
pUziPresentReload(PANEL_SPRITEp psp)
{
    if (TEST(psp->PlayerP->Flags, PF_WEAPON_RETRACT))
        return;

    psp->backupy();
    psp->y -= 5 * synctics;

    psp->PlayerP->WpnReloadState = 2;

    if (psp->y < UZI_YOFF)
    {
        psp->oy = psp->y = UZI_YOFF;
        psp->backupx();
        psp->backupbobcoords();
        pSetState(psp, psp->RestState);
    }
}

void
pUziBobSetup(PANEL_SPRITEp psp)
{
    if (TEST(psp->flags, PANF_BOB))
        return;

    psp->backupbobcoords();

    psp->sin_amt = 12;
    psp->sin_ndx = 0;
    psp->bob_height_divider = 8;
}

void
pUziStartReload(PANEL_SPRITEp psp)
{
    SetVisNorm();

    psp->backupx();

    // Set uzi's to reload state
    if (psp->PlayerP->WpnUziType < 3)
        psp->PlayerP->WpnUziType += 3;

    // Uzi #1 reload - starting from a full up position
    pSetState(psp, ps_UziEject);

    SET(psp->flags, PANF_RELOAD);

    if (TEST(psp->flags, PANF_PRIMARY) && psp->sibling)
    {
        // this is going to KILL Uzi #2 !!!
        pSetState(psp->sibling, psp->sibling->RetractState);
    }
}

void
pUziHide(PANEL_SPRITEp psp)
{
    short picnum = psp->picndx;

    psp->backupy();
    psp->y += 3 * synctics;

    if (psp->y >= 200 + tileHeight(picnum))
    {
        psp->oy = psp->y = 200 + tileHeight(picnum);

        if (TEST(psp->flags, PANF_PRIMARY) && psp->PlayerP->WpnUziType != 1)
        {
            if (pWeaponUnHideKeys(psp, psp->PresentState))
                pSetState(psp->sibling, psp->sibling->PresentState);
        }
        else if (!TEST(psp->flags, PANF_SECONDARY))
        {
            pWeaponUnHideKeys(psp, psp->PresentState);
        }
    }
}

void
pUziRest(PANEL_SPRITEp psp)
{
    bool shooting;
    bool force = !!TEST(psp->flags, PANF_UNHIDE_SHOOT);


    // If you have two uzi's, but one didn't come up, spawn it
    if (TEST(psp->PlayerP->Flags, PF_TWO_UZI) && psp->sibling == nullptr)
    {
        InitWeaponUzi2(psp);
    }

    if (TEST(psp->flags, PANF_PRIMARY) && psp->sibling)
    {
        if (pWeaponHideKeys(psp, ps_UziHide))
        {
            if (psp->sibling != nullptr) // !JIM! Without this line, will ASSERT if reloading here
                pSetState(psp->sibling, ps_Uzi2Hide);
            return;
        }
    }
    else if (!TEST(psp->flags, PANF_SECONDARY))
    {
        if (pWeaponHideKeys(psp, ps_UziHide))
            return;
    }

    if (TEST(psp->flags, PANF_SECONDARY))
        pUziOverlays(psp, CHAMBER_REST);

    SetVisNorm();

    shooting = (psp->PlayerP->input.actions & SB_FIRE) && (psp->PlayerP->KeyPressBits & SB_FIRE);
    shooting |= force;

    pUziBobSetup(psp);
    pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP) || shooting);

    if (shooting)
    {
        if (!WeaponOK(psp->PlayerP))
            return;

        RESET(psp->flags, PANF_UNHIDE_SHOOT);

        pSetState(psp, psp->ActionState);
    }
    else
        WeaponOK(psp->PlayerP);
}

void
pUziAction(PANEL_SPRITEp psp)
{
    char shooting;
    static int alternate = 0;

    shooting = (psp->PlayerP->input.actions & SB_FIRE) && (psp->PlayerP->KeyPressBits & SB_FIRE);

    if (shooting)
    {
        if (TEST(psp->flags, PANF_SECONDARY))
        {
            alternate++;
            if (alternate > 6) alternate = 0;
            if (alternate <= 3)
                pUziOverlays(psp, CHAMBER_FIRE);
            else
                pUziOverlays(psp, CHAMBER_REST);
        }
        // Only Recoil if shooting
        pUziBobSetup(psp);
        UziRecoilYadj = DIV256(RANDOM_P2(1024));        // global hack for
        // Weapon Bob
        pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP) || shooting);
        UziRecoilYadj = 0;              // reset my global hack
        if (RANDOM_P2(1024) > 990)
            DoPlayerChooseYell(psp->PlayerP);
    }
    else
    {
        if (TEST(psp->flags, PANF_SECONDARY))
            pUziOverlays(psp, CHAMBER_REST);
        pUziBobSetup(psp);
        pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP) || shooting);
    }
}

void
pUziFire(PANEL_SPRITEp psp)
{
    PLAYERp pp = psp->PlayerP;

    if (!WeaponOK(psp->PlayerP))
        return;

    if (TEST(psp->flags, PANF_SECONDARY) && pp->WpnUziType > 0) return;

    InitUzi(psp->PlayerP);
    SpawnUziShell(psp);

    // If its the second Uzi, give the shell back only if it's a reload count to keep #'s even
    if (TEST(psp->flags, PANF_SECONDARY))
    {
        if (TEST(pp->Flags, PF_TWO_UZI) && psp->sibling)
        {
            if ((pp->WpnAmmo[WPN_UZI] % 100) == 0)
                pp->WpnAmmo[WPN_UZI]++;
        }
        else if ((pp->WpnAmmo[WPN_UZI] % 50) == 0)
            pp->WpnAmmo[WPN_UZI]++;
    }
    else
    {
        SpawnVis(psp->PlayerP->PlayerSprite, -1, -1, -1, -1, 32);

        if (!WeaponOK(psp->PlayerP))
            return;

        // Reload if done with clip
        if (TEST(pp->Flags, PF_TWO_UZI) && psp->sibling)
        {
            if ((pp->WpnAmmo[WPN_UZI] % 100) == 0)
                pUziStartReload(psp);
        }
        else if ((pp->WpnAmmo[WPN_UZI] % 50) == 0)
        {
            // clip has run out
            pUziStartReload(psp);
        }
    }
}

void
pUziRetract(PANEL_SPRITEp psp)
{
    // PANEL_SPRITEp sib = psp->sibling;
    short picnum = psp->picndx;

    psp->backupy();
    psp->y += 3 * synctics;

    if (psp->y >= 200 + tileHeight(picnum))
    {
        // if in the reload phase and its retracting then get rid of uzi
        // no matter whether it is PRIMARY/SECONDARY/neither.
        if (TEST(psp->flags, PANF_RELOAD))
        {
            RESET(psp->PlayerP->Flags, PF_WEAPON_RETRACT);
            psp->PlayerP->Wpn[WPN_UZI] = nullptr;
        }
        else
        {
            // NOT reloading here
            if (TEST(psp->flags, PANF_PRIMARY))
            {
                // only reset when primary goes off the screen
                RESET(psp->PlayerP->Flags, PF_WEAPON_RETRACT);
                psp->PlayerP->Wpn[WPN_UZI] = nullptr;
            }
            else if (TEST(psp->flags, PANF_SECONDARY))
            {
                // primarily for beginning of reload sequence where seconary
                // is taken off of the screen.  Lets the primary know that
                // he is alone.
                if (psp->sibling && psp->sibling->sibling == psp)
                    psp->sibling->sibling = nullptr;
            }
            else
            {
                // only one uzi here is retracting
                RESET(psp->PlayerP->Flags, PF_WEAPON_RETRACT);
                psp->PlayerP->Wpn[WPN_UZI] = nullptr;
            }
        }


        pKillSprite(psp);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// UZI SHELL
//
//////////////////////////////////////////////////////////////////////////////////////////

void pUziShell(PANEL_SPRITEp);

#define UZI_SHELL_RATE 10

PANEL_STATE ps_UziShell[] =
{
    {ID_UziShell0, UZI_SHELL_RATE, pUziShell, &ps_UziShell[1], 0,0,0},
    {ID_UziShell1, UZI_SHELL_RATE, pUziShell, &ps_UziShell[2], 0,0,0},
    {ID_UziShell2, UZI_SHELL_RATE, pUziShell, &ps_UziShell[3], 0,0,0},
    {ID_UziShell3, UZI_SHELL_RATE, pUziShell, &ps_UziShell[4], 0,0,0},
    {ID_UziShell4, UZI_SHELL_RATE, pUziShell, &ps_UziShell[5], 0,0,0},
    {ID_UziShell5, UZI_SHELL_RATE, pUziShell, &ps_UziShell[0], 0,0,0},
};

PANEL_STATE ps_Uzi2Shell[] =
{
    {ID_Uzi2Shell0, UZI_SHELL_RATE, pUziShell, &ps_Uzi2Shell[1], psf_Xflip, 0,0},
    {ID_Uzi2Shell1, UZI_SHELL_RATE, pUziShell, &ps_Uzi2Shell[2], psf_Xflip, 0,0},
    {ID_Uzi2Shell2, UZI_SHELL_RATE, pUziShell, &ps_Uzi2Shell[3], psf_Xflip, 0,0},
    {ID_Uzi2Shell3, UZI_SHELL_RATE, pUziShell, &ps_Uzi2Shell[4], psf_Xflip, 0,0},
    {ID_Uzi2Shell4, UZI_SHELL_RATE, pUziShell, &ps_Uzi2Shell[5], psf_Xflip, 0,0},
    {ID_Uzi2Shell5, UZI_SHELL_RATE, pUziShell, &ps_Uzi2Shell[0], psf_Xflip, 0,0},
};

void
SpawnUziShell(PANEL_SPRITEp psp)
{
    PLAYERp pp = psp->PlayerP;

    if (psp->State && TEST(psp->State->flags, psf_Xflip))
    {
        // LEFT side
        pp->UziShellLeftAlt = !pp->UziShellLeftAlt;
        if (pp->UziShellLeftAlt)
            SpawnShell(pp->PlayerSprite,-3);
    }
    else
    {
        // RIGHT side
        pp->UziShellRightAlt = !pp->UziShellRightAlt;
        if (pp->UziShellRightAlt)
            SpawnShell(pp->PlayerSprite,-2);
    }
}

void
pUziShell(PANEL_SPRITEp psp)
{
    psp->backupx();

    if (psp->State && TEST(psp->State->flags, psf_Xflip))
    {
        psp->x -= 3 * synctics;
    }
    else
    {
        psp->x += 3 * synctics;
    }

    // increment the ndx into the sin table and wrap at 1024
    psp->sin_ndx = (psp->sin_ndx + (synctics << psp->sin_arc_speed) + 1024) & 1023;

    // get height
    psp->oy = psp->y = psp->yorig;
    psp->y += psp->sin_amt * -bsinf(psp->sin_ndx, -14);

    // if off of the screen kill them
    if (psp->x > 330 || psp->x < -10)
    {
        pKillSprite(psp);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// SHOTGUN SHELL
//
//////////////////////////////////////////////////////////////////////////////////////////

void pShotgunShell(PANEL_SPRITEp);

#define SHOTGUN_SHELL_RATE 7

PANEL_STATE ps_ShotgunShell[] =
{
    {ID_ShotgunShell0, SHOTGUN_SHELL_RATE, pShotgunShell, &ps_ShotgunShell[1], 0,0,0},
    {ID_ShotgunShell1, SHOTGUN_SHELL_RATE, pShotgunShell, &ps_ShotgunShell[2], 0,0,0},
    {ID_ShotgunShell2, SHOTGUN_SHELL_RATE, pShotgunShell, &ps_ShotgunShell[3], 0,0,0},
    {ID_ShotgunShell3, SHOTGUN_SHELL_RATE, pShotgunShell, &ps_ShotgunShell[4], 0,0,0},
    {ID_ShotgunShell4, SHOTGUN_SHELL_RATE, pShotgunShell, &ps_ShotgunShell[5], 0,0,0},
    {ID_ShotgunShell5, SHOTGUN_SHELL_RATE, pShotgunShell, &ps_ShotgunShell[6], 0,0,0},
    {ID_ShotgunShell6, SHOTGUN_SHELL_RATE, pShotgunShell, &ps_ShotgunShell[7], 0,0,0},
    {ID_ShotgunShell6, SHOTGUN_SHELL_RATE, pShotgunShell, &ps_ShotgunShell[0], 0,0,0},
};

void
SpawnShotgunShell(PANEL_SPRITEp psp)
{
    PLAYERp pp = psp->PlayerP;
    SpawnShell(pp->PlayerSprite,-4);
}

void
pShotgunShell(PANEL_SPRITEp psp)
{
    if (TEST(psp->flags, PANF_JUMPING))
    {
        DoPanelJump(psp);
    }
    else if (TEST(psp->flags, PANF_FALLING))
    {
        DoPanelFall(psp);
    }

    psp->backupx();

    psp->x += psp->xspeed * (1. / FRACUNIT);

    if (psp->x > 320 || psp->x < 0 || psp->y > 200)
    {
        pKillSprite(psp);
        return;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// SHOTGUN
//
//////////////////////////////////////////////////////////////////////////////////////////

void pShotgunPresent(PANEL_SPRITEp psp);
void pShotgunRetract(PANEL_SPRITEp psp);
void pShotgunAction(PANEL_SPRITEp psp);
void pShotgunRest(PANEL_SPRITEp psp);
void pShotgunFire(PANEL_SPRITEp psp);
void pShotgunHide(PANEL_SPRITEp psp);
void pShotgunRestTest(PANEL_SPRITEp psp);

void pShotgunReloadDown(PANEL_SPRITEp psp);
void pShotgunReloadUp(PANEL_SPRITEp psp);
void pShotgunBobSetup(PANEL_SPRITEp psp);
void pShotgunRecoilUp(PANEL_SPRITEp psp);
void pShotgunRecoilDown(PANEL_SPRITEp psp);

bool pShotgunReloadTest(PANEL_SPRITEp psp);

extern PANEL_STATE ps_ShotgunReload[];

#define Shotgun_BEAT_RATE 24
#define Shotgun_ACTION_RATE 4

PANEL_STATE ps_PresentShotgun[] =
{
    {ID_ShotgunPresent0, Shotgun_BEAT_RATE, pShotgunPresent, &ps_PresentShotgun[0], 0,0,0}
};

PANEL_STATE ps_ShotgunRest[] =
{
    {ID_ShotgunPresent0, Shotgun_BEAT_RATE, pShotgunRest, &ps_ShotgunRest[0], 0,0,0}
};

PANEL_STATE ps_ShotgunHide[] =
{
    {ID_ShotgunPresent0, Shotgun_BEAT_RATE, pShotgunHide, &ps_ShotgunHide[0], 0,0,0}
};

PANEL_STATE ps_ShotgunRecoil[] =
{
    // recoil
    {ID_ShotgunReload0, Shotgun_ACTION_RATE, pShotgunRecoilDown, &ps_ShotgunRecoil[0], 0,0,0},
    {ID_ShotgunReload0, Shotgun_ACTION_RATE, pShotgunRecoilUp, &ps_ShotgunRecoil[1], 0,0,0},
    // reload
    {ID_ShotgunReload0, Shotgun_ACTION_RATE*5,  pNullAnimator,      &ps_ShotgunRecoil[3], 0,0,0},
    {ID_ShotgunReload1, Shotgun_ACTION_RATE,    pNullAnimator,      &ps_ShotgunRecoil[4], 0,0,0},
    {ID_ShotgunReload2, Shotgun_ACTION_RATE*5,  pNullAnimator,      &ps_ShotgunRecoil[5], 0,0,0},
    {ID_ShotgunPresent0,Shotgun_ACTION_RATE,    pShotgunRestTest,   &ps_ShotgunRecoil[6], 0,0,0},
    {ID_ShotgunPresent0,Shotgun_ACTION_RATE/2,  pShotgunAction,     &ps_ShotgunRecoil[7], 0,0,0},
    {ID_ShotgunPresent0,Shotgun_ACTION_RATE/2,  pShotgunAction,     &ps_ShotgunRecoil[8], 0,0,0},
    // ready to fire again
    {ID_ShotgunPresent0, 3, pNullAnimator, &ps_ShotgunRest[0], 0,0,0}
};

PANEL_STATE ps_ShotgunRecoilAuto[] =
{
    // recoil
    {ID_ShotgunReload0, 1,    pShotgunRecoilDown, &ps_ShotgunRecoilAuto[0], 0,0,0},
    {ID_ShotgunReload0, 1,    pShotgunRecoilUp,   &ps_ShotgunRecoilAuto[1], 0,0,0},
    // Reload
    {ID_ShotgunReload0, 1,    pNullAnimator,      &ps_ShotgunRecoilAuto[3], 0,0,0},
    {ID_ShotgunReload0, 1,    pNullAnimator,      &ps_ShotgunRecoilAuto[4], 0,0,0},
    {ID_ShotgunReload0, 1,    pNullAnimator,      &ps_ShotgunRecoilAuto[5], 0,0,0},

    {ID_ShotgunPresent0,1,    pShotgunRestTest,   &ps_ShotgunRecoilAuto[6], 0,0,0},
    {ID_ShotgunPresent0,1,    pShotgunAction,     &ps_ShotgunRecoilAuto[7], 0,0,0},
    {ID_ShotgunPresent0,1,    pShotgunRest,       &ps_ShotgunRest[0],psf_QuickCall, 0,0},
};

PANEL_STATE ps_ShotgunFire[] =
{
    {ID_ShotgunFire0,   Shotgun_ACTION_RATE,    pShotgunAction,     &ps_ShotgunFire[1], psf_ShadeHalf, 0,0},
    {ID_ShotgunFire1,   Shotgun_ACTION_RATE,    pShotgunFire,       &ps_ShotgunFire[2], psf_ShadeNone|psf_QuickCall, 0,0},
    {ID_ShotgunFire1,   Shotgun_ACTION_RATE,    pShotgunAction,     &ps_ShotgunFire[3], psf_ShadeNone, 0,0},
    {ID_ShotgunReload0, 0,                      SpawnShotgunShell,  &ps_ShotgunFire[4], psf_QuickCall, 0,0},
    {ID_ShotgunReload0, Shotgun_ACTION_RATE,    pShotgunAction,     &ps_ShotgunRecoil[0], 0,0,0}
};


#if 1
PANEL_STATE ps_ShotgunAutoFire[] =
{
    {ID_ShotgunFire1,   2,    pShotgunAction,     &ps_ShotgunAutoFire[1], psf_ShadeHalf, 0,0},
    {ID_ShotgunFire1,   2,    pShotgunFire,       &ps_ShotgunAutoFire[2], psf_ShadeNone, 0,0},
    {ID_ShotgunFire1,   2,    pShotgunAction,     &ps_ShotgunAutoFire[3], psf_ShadeNone, 0,0},
    {ID_ShotgunReload0, 0,    SpawnShotgunShell,  &ps_ShotgunAutoFire[4], psf_QuickCall, 0,0},
    {ID_ShotgunReload0, 1,    pShotgunAction,     &ps_ShotgunRecoilAuto[0], 0,0,0}
};
#endif

#if 1
PANEL_STATE ps_ShotgunReload[] =
{
    {ID_ShotgunPresent0, Shotgun_BEAT_RATE, pShotgunReloadDown, &ps_ShotgunReload[0], 0,0,0},
    {ID_ShotgunPresent0, 30,            pNullAnimator, &ps_ShotgunReload[2], 0,0,0},
    // make reload sound here
    {ID_ShotgunPresent0, Shotgun_BEAT_RATE, pNullAnimator, &ps_ShotgunReload[3], psf_QuickCall, 0,0},
    {ID_ShotgunPresent0, 30,            pNullAnimator, &ps_ShotgunReload[4], 0,0,0},
    {ID_ShotgunPresent0, Shotgun_BEAT_RATE, pShotgunReloadUp, &ps_ShotgunReload[4], 0,0,0},
    {ID_ShotgunPresent0, 3, pNullAnimator, &ps_ShotgunRest[0], 0,0,0}
};
#endif

PANEL_STATE ps_RetractShotgun[] =
{
    {ID_ShotgunPresent0, Shotgun_BEAT_RATE, pShotgunRetract, &ps_RetractShotgun[0], 0,0,0}
};

#define SHOTGUN_YOFF 200
#define SHOTGUN_XOFF (160+42)

void
InitWeaponShotgun(PLAYERp pp)
{
    PANEL_SPRITEp psp = nullptr;

    if (Prediction)
        return;

    pp->WeaponType = WPN_SHOTGUN;

    if (!TEST(pp->WpnFlags, BIT(pp->WeaponType)) ||
//        pp->WpnAmmo[pp->WeaponType] <= 0 ||
        TEST(pp->Flags, PF_WEAPON_RETRACT))
        return;

    if (!pp->Wpn[pp->WeaponType])
    {
        psp = pp->Wpn[pp->WeaponType] = pSpawnSprite(pp, ps_PresentShotgun, PRI_MID, SHOTGUN_XOFF, SHOTGUN_YOFF);
        psp->y += tileHeight(psp->picndx);
        psp->backupy();
    }

    if (pp->CurWpn == pp->Wpn[pp->WeaponType])
    {
        return;
    }

    psp->WeaponType = pp->WeaponType;
    PlayerUpdateWeapon(pp, pp->WeaponType);

    pp->WpnUziType = 2; // Make uzi's go away!
    RetractCurWpn(pp);

    // Set up the new Weapon variables
    psp = pp->CurWpn = pp->Wpn[pp->WeaponType];
    SET(psp->flags, PANF_WEAPON_SPRITE);
    psp->ActionState = ps_ShotgunFire;
    //psp->ActionState = ps_ShotgunAutoFire;
    psp->RetractState = ps_RetractShotgun;
    psp->PresentState = ps_PresentShotgun;
    psp->RestState = ps_ShotgunRest;
    pSetState(psp, psp->PresentState);

    PlaySound(DIGI_SHOTGUN_UP, pp, v3df_follow);

    psp->PlayerP->KeyPressBits |= SB_FIRE;
}

void
pShotgunSetRecoil(PANEL_SPRITEp psp)
{
    psp->vel = 900;
    psp->ang = NORM_ANGLE(-256);
}

void
pShotgunRecoilDown(PANEL_SPRITEp psp)
{
    int targetvel = psp->PlayerP->WpnShotgunType == 1 ? 890 : 780;

    psp->backupcoords();

    psp->x += pspCosVel(psp);
    psp->y -= pspSinVel(psp);

    psp->vel -= 24 * synctics;

    if (psp->vel < targetvel)
    {
        psp->vel = targetvel;
        psp->ang = NORM_ANGLE(psp->ang + 1024);

        pStatePlusOne(psp);
    }
}

void
pShotgunRecoilUp(PANEL_SPRITEp psp)
{
    psp->backupcoords();

    psp->x += pspCosVel(psp);
    psp->y -= pspSinVel(psp);

    psp->vel += 15 * synctics;

    if (psp->y < SHOTGUN_YOFF)
    {
        psp->oy = psp->y = SHOTGUN_YOFF;
        psp->ox = psp->x = SHOTGUN_XOFF;

        pShotgunSetRecoil(psp);

        pStatePlusOne(psp);
        RESET(psp->flags, PANF_BOB);
    }
}

#if 1
void
pShotgunReloadDown(PANEL_SPRITEp psp)
{
    short picnum = psp->picndx;

    psp->backupy();
    psp->y += 3 * synctics;

    if (psp->y >= SHOTGUN_YOFF + (tileHeight(picnum)/2))
    {
        PlaySound(DIGI_ROCKET_UP, psp->PlayerP,v3df_follow|v3df_dontpan|v3df_doppler);

        psp->oy = psp->y = SHOTGUN_YOFF + (tileHeight(picnum)/2);

        pStatePlusOne(psp);
    }
}

void
pShotgunReloadUp(PANEL_SPRITEp psp)
{
    psp->ox = psp->x = SHOTGUN_XOFF;
    psp->backupy();
    psp->y -= 3 * synctics;

    psp->PlayerP->WpnReloadState = 2;

    if (psp->y < SHOTGUN_YOFF)
    {
        PlaySound(DIGI_SHOTGUN_UP, psp->PlayerP,v3df_follow|v3df_dontpan|v3df_doppler);

        psp->oy = psp->y = SHOTGUN_YOFF;

        pStatePlusOne(psp);
        RESET(psp->flags, PANF_BOB);
    }
}
#endif

void
pShotgunPresent(PANEL_SPRITEp psp)
{
    if (TEST(psp->PlayerP->Flags, PF_WEAPON_RETRACT))
        return;

    // Needed for recoil
    psp->ang = NORM_ANGLE(256 + 128);
    pShotgunSetRecoil(psp);
    ///

    psp->backupy();
    psp->y -= 3 * synctics;

    if (psp->y < SHOTGUN_YOFF)
    {
        psp->oy = psp->y = SHOTGUN_YOFF;
        psp->backupboby();
        pSetState(psp, psp->RestState);
    }
}

void
pShotgunBobSetup(PANEL_SPRITEp psp)
{
    if (TEST(psp->flags, PANF_BOB))
        return;

    psp->backupbobcoords();

    psp->sin_amt = 12;
    psp->sin_ndx = 0;
    psp->bob_height_divider = 8;
}

bool
pShotgunOverlays(PANEL_SPRITEp psp)
{
#define SHOTGUN_AUTO_XOFF 28
#define SHOTGUN_AUTO_YOFF -17

    if (psp->over[SHOTGUN_AUTO_NUM].xoff == -1)
    {
        psp->over[SHOTGUN_AUTO_NUM].xoff = SHOTGUN_AUTO_XOFF;
        psp->over[SHOTGUN_AUTO_NUM].yoff = SHOTGUN_AUTO_YOFF;
    }

    //if(psp->PlayerP->WpnShotgunAuto == 0 && psp->PlayerP->WpnRocketType == 1)
    //psp->PlayerP->WpnShotgunType--;

    switch (psp->PlayerP->WpnShotgunType)
    {
    case 0:
        psp->over[SHOTGUN_AUTO_NUM].pic = -1;
        SET(psp->over[SHOTGUN_AUTO_NUM].flags, psf_ShadeNone);
        return false;
    case 1:
        psp->over[SHOTGUN_AUTO_NUM].pic = SHOTGUN_AUTO;
        SET(psp->over[SHOTGUN_AUTO_NUM].flags, psf_ShadeNone);
        return false;
    }
    return false;
}

PANEL_STATE ps_ShotgunFlash[] =
{
    {SHOTGUN_AUTO, 30, nullptr, &ps_ShotgunFlash[1], 0,0,0},
    {0,            30, nullptr, &ps_ShotgunFlash[2], 0,0,0},
    {SHOTGUN_AUTO, 30, nullptr, &ps_ShotgunFlash[3], 0,0,0},
    {0,            30, nullptr, &ps_ShotgunFlash[4], 0,0,0},
    {SHOTGUN_AUTO, 30, nullptr, &ps_ShotgunFlash[5], 0,0,0},
    {0,            30, nullptr, &ps_ShotgunFlash[6], 0,0,0},
    {SHOTGUN_AUTO, 30, nullptr, &ps_ShotgunFlash[7], 0,0,0},
    {0,            30, nullptr, &ps_ShotgunFlash[8], 0,0,0},
    {SHOTGUN_AUTO, 30, nullptr, &ps_ShotgunFlash[9], 0,0,0},
    {0,             0, nullptr, nullptr, 0,0,0}
};


void
pShotgunHide(PANEL_SPRITEp psp)
{
    short picnum = psp->picndx;

    psp->backupy();
    psp->y += 3 * synctics;

    if (psp->y >= SHOTGUN_YOFF + tileHeight(picnum))
    {
        psp->oy = psp->y = SHOTGUN_YOFF + tileHeight(picnum);

        pWeaponUnHideKeys(psp, psp->PresentState);
    }
}

#if 1
bool
pShotgunReloadTest(PANEL_SPRITEp psp)
{
    //short ammo = psp->PlayerP->WpnAmmo[psp->PlayerP->WeaponType];
    short ammo = psp->PlayerP->WpnAmmo[WPN_SHOTGUN];

    // Reload if done with clip
    if (ammo > 0 && (ammo % 4) == 0)
    {
        // clip has run out
        RESET(psp->flags, PANF_REST_POS);
        pSetState(psp, ps_ShotgunReload);
        return true;
    }

    return false;
}
#endif

void
pShotgunRest(PANEL_SPRITEp psp)
{
    bool force = !!TEST(psp->flags, PANF_UNHIDE_SHOOT);
    //short ammo = psp->PlayerP->WpnAmmo[psp->PlayerP->WeaponType];
    short ammo = psp->PlayerP->WpnAmmo[WPN_SHOTGUN];
    char lastammo = psp->PlayerP->WpnShotgunLastShell;

    if (pWeaponHideKeys(psp, ps_ShotgunHide))
        return;

    if (psp->PlayerP->WpnShotgunType == 1 && ammo > 0 && ((ammo % 4) != 0) && lastammo != ammo && TEST(psp->flags, PANF_REST_POS))
    {
        force = true;
    }

    pShotgunOverlays(psp);

    pShotgunBobSetup(psp);
    pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP));


    if ((psp->PlayerP->input.actions & SB_FIRE) || force)
    {
        if ((psp->PlayerP->KeyPressBits & SB_FIRE) || force)
        {
            RESET(psp->flags, PANF_UNHIDE_SHOOT);

            if (!WeaponOK(psp->PlayerP))
                return;

            SET(psp->flags, PANF_REST_POS); // Used for reload checking in autofire

            if (psp->PlayerP->WpnShotgunType == 0)
                psp->PlayerP->WpnShotgunLastShell = ammo-1;

            DoPlayerChooseYell(psp->PlayerP);
            if (psp->PlayerP->WpnShotgunType==0)
                pSetState(psp, ps_ShotgunFire);
            else
                pSetState(psp, ps_ShotgunAutoFire);
        }
        if (!WeaponOK(psp->PlayerP))
            return;
    }
    else
        WeaponOK(psp->PlayerP);
}

void
pShotgunRestTest(PANEL_SPRITEp psp)
{
    bool force = !!TEST(psp->flags, PANF_UNHIDE_SHOOT);

    if (psp->PlayerP->WpnShotgunType == 1 && !pShotgunReloadTest(psp))
        force = true;

    if (pShotgunReloadTest(psp))
        return;

    if (pWeaponHideKeys(psp, ps_ShotgunHide))
        return;

    pShotgunBobSetup(psp);
    pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP));

    if ((psp->PlayerP->input.actions & SB_FIRE) || force)
    {
        if ((psp->PlayerP->KeyPressBits & SB_FIRE) || force)
        {
            RESET(psp->flags, PANF_UNHIDE_SHOOT);

            if (!WeaponOK(psp->PlayerP))
                return;

            DoPlayerChooseYell(psp->PlayerP);
            return;
        }
    }

    pSetState(psp, psp->RestState);
}

void
pShotgunAction(PANEL_SPRITEp psp)
{
    pShotgunBobSetup(psp);
    pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP));
}


void
pShotgunFire(PANEL_SPRITEp psp)
{
    SpawnVis(psp->PlayerP->PlayerSprite, -1, -1, -1, -1, 32);
    InitShotgun(psp->PlayerP);
    //SpawnShotgunShell(psp);
}

void
pShotgunRetract(PANEL_SPRITEp psp)
{
    short picnum = psp->picndx;

    psp->backupy();
    psp->y += 3 * synctics;

    if (psp->y >= SHOTGUN_YOFF + tileHeight(picnum) + 50)
    {
        RESET(psp->PlayerP->Flags, PF_WEAPON_RETRACT);
        psp->PlayerP->Wpn[psp->WeaponType] = nullptr;
        pKillSprite(psp);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// RAIL
//
//////////////////////////////////////////////////////////////////////////////////////////

void pRailPresent(PANEL_SPRITEp psp);
void pRailRetract(PANEL_SPRITEp psp);
void pRailAction(PANEL_SPRITEp psp);
void pRailRest(PANEL_SPRITEp psp);
void pRailFire(PANEL_SPRITEp psp);
void pRailHide(PANEL_SPRITEp psp);
void pRailRestTest(PANEL_SPRITEp psp);
void pRailOkTest(PANEL_SPRITEp psp);
void pRailRecoilUp(PANEL_SPRITEp psp);
void pRailRecoilDown(PANEL_SPRITEp psp);

void pRailBobSetup(PANEL_SPRITEp psp);

bool pRailReloadTest(PANEL_SPRITEp psp);

#define Rail_BEAT_RATE 24
#define Rail_ACTION_RATE 3  // !JIM! Was 10
#define Rail_CHARGE_RATE 3

PANEL_STATE ps_PresentRail[] =
{
    {ID_RailPresent0, Rail_BEAT_RATE, pRailPresent, &ps_PresentRail[0], psf_ShadeNone, 0,0}
};

PANEL_STATE ps_RailRest[] =
{
    {ID_RailRest0, Rail_BEAT_RATE, pRailRest, &ps_RailRest[1], psf_ShadeNone, 0,0},
    {ID_RailRest1, Rail_BEAT_RATE, pRailRest, &ps_RailRest[2], psf_ShadeNone, 0,0},
    {ID_RailRest2, Rail_BEAT_RATE, pRailRest, &ps_RailRest[3], psf_ShadeNone, 0,0},
    {ID_RailRest3, Rail_BEAT_RATE, pRailRest, &ps_RailRest[4], psf_ShadeNone, 0,0},
    {ID_RailRest4, Rail_BEAT_RATE, pRailRest, &ps_RailRest[0], psf_ShadeNone, 0,0},
};

PANEL_STATE ps_RailHide[] =
{
    {ID_RailPresent0, Rail_BEAT_RATE, pRailHide, &ps_RailHide[0], psf_ShadeNone, 0,0}
};

PANEL_STATE ps_RailRecoil[] =
{
    // recoil
    {ID_RailPresent0, Rail_BEAT_RATE, pRailRecoilDown, &ps_RailRecoil[0], 0,0,0},
    {ID_RailPresent0, Rail_BEAT_RATE, pRailRecoilUp, &ps_RailRecoil[1], 0,0,0},
    // ready to fire again
    {ID_RailPresent0, 3, pNullAnimator, &ps_RailRest[0], 0,0,0}
};

PANEL_STATE ps_RailFire[] =
{
    {ID_RailCharge0,       Rail_CHARGE_RATE, pRailAction,    &ps_RailFire[1], psf_ShadeNone, 0,0},
    {ID_RailCharge1,       Rail_CHARGE_RATE, pRailAction,    &ps_RailFire[2], psf_ShadeNone, 0,0},
    {ID_RailCharge2,       Rail_CHARGE_RATE, pRailAction,    &ps_RailFire[3], psf_ShadeNone, 0,0},
    {ID_RailCharge1,       Rail_CHARGE_RATE, pRailAction,    &ps_RailFire[4], psf_ShadeNone, 0,0},

    {ID_RailFire0,       Rail_ACTION_RATE, pRailAction,    &ps_RailFire[5], psf_ShadeNone, 0,0},
    {ID_RailFire1,       Rail_ACTION_RATE, pRailAction,    &ps_RailFire[6], psf_ShadeNone, 0,0},
    {ID_RailFire1,       Rail_ACTION_RATE, pRailAction,    &ps_RailFire[7], psf_ShadeNone, 0,0},
    {ID_RailFire1,       0,                pRailFire,      &ps_RailFire[8], psf_ShadeNone|psf_QuickCall, 0,0},

    // recoil
    {ID_RailPresent0,      Rail_BEAT_RATE, pRailRecoilDown,  &ps_RailFire[8], psf_ShadeNone, 0,0},
    {ID_RailPresent0,      Rail_BEAT_RATE, pRailRecoilUp,    &ps_RailFire[9], psf_ShadeNone, 0,0},
    // !JIM! I added these to introduce firing delay, that looks like a charge down.
    {ID_RailCharge0,       Rail_CHARGE_RATE, pRailOkTest,    &ps_RailFire[11], psf_ShadeNone, 0,0},
    {ID_RailCharge1,       Rail_CHARGE_RATE, pRailAction,    &ps_RailFire[12], psf_ShadeNone, 0,0},
    {ID_RailCharge2,       Rail_CHARGE_RATE, pRailOkTest,    &ps_RailFire[13], psf_ShadeNone, 0,0},
    {ID_RailCharge1,       Rail_CHARGE_RATE, pRailAction,    &ps_RailFire[14], psf_ShadeNone, 0,0},
    {ID_RailCharge0,       Rail_CHARGE_RATE+1, pRailAction,    &ps_RailFire[15], psf_ShadeNone, 0,0},
    {ID_RailCharge1,       Rail_CHARGE_RATE+1, pRailAction,    &ps_RailFire[16], psf_ShadeNone, 0,0},
    {ID_RailCharge2,       Rail_CHARGE_RATE+1, pRailAction,    &ps_RailFire[17], psf_ShadeNone, 0,0},
    {ID_RailCharge1,       Rail_CHARGE_RATE+2, pRailAction,    &ps_RailFire[18], psf_ShadeNone, 0,0},
    {ID_RailCharge0,       Rail_CHARGE_RATE+2, pRailAction,    &ps_RailFire[19], psf_ShadeNone, 0,0},
    {ID_RailCharge1,       Rail_CHARGE_RATE+2, pRailAction,    &ps_RailFire[20], psf_ShadeNone, 0,0},
    {ID_RailCharge2,       Rail_CHARGE_RATE+3, pRailAction,    &ps_RailFire[21], psf_ShadeNone, 0,0},
    {ID_RailCharge1,       Rail_CHARGE_RATE+3, pRailAction,    &ps_RailFire[22], psf_ShadeNone, 0,0},
    {ID_RailCharge0,       Rail_CHARGE_RATE+4, pRailAction,    &ps_RailFire[23], psf_ShadeNone, 0,0},
    {ID_RailCharge1,       Rail_CHARGE_RATE+4, pRailAction,    &ps_RailFire[24], psf_ShadeNone, 0,0},
    {ID_RailCharge2,       Rail_CHARGE_RATE+4, pRailAction,    &ps_RailFire[25], psf_ShadeNone, 0,0},
    {ID_RailCharge0,       Rail_CHARGE_RATE+5, pRailAction,    &ps_RailFire[26], psf_ShadeNone, 0,0},
    {ID_RailCharge1,       Rail_CHARGE_RATE+5, pRailAction,    &ps_RailFire[27], psf_ShadeNone, 0,0},
    {ID_RailCharge2,       Rail_CHARGE_RATE+5, pRailAction,    &ps_RailFire[28], psf_ShadeNone, 0,0},

    {ID_RailCharge0,      Rail_ACTION_RATE, pRailRestTest,  &ps_RailFire[29], psf_ShadeNone, 0,0},
    {ID_RailCharge1,      Rail_ACTION_RATE, pRailRest,      &ps_RailRest[0], psf_ShadeNone, 0,0},
};

PANEL_STATE ps_RailFireEMP[] =
{
    {ID_RailCharge0,       Rail_CHARGE_RATE, pRailAction,    &ps_RailFireEMP[1], psf_ShadeNone, 0,0},
    {ID_RailCharge1,       Rail_CHARGE_RATE, pRailAction,    &ps_RailFireEMP[2], psf_ShadeNone, 0,0},
    {ID_RailCharge2,       Rail_CHARGE_RATE, pRailAction,    &ps_RailFireEMP[3], psf_ShadeNone, 0,0},
    {ID_RailCharge1,       Rail_CHARGE_RATE, pRailAction,    &ps_RailFireEMP[4], psf_ShadeNone, 0,0},

    {ID_RailFire0,       Rail_ACTION_RATE, pRailAction,    &ps_RailFireEMP[5], psf_ShadeNone, 0,0},
    {ID_RailFire1,       Rail_ACTION_RATE, pRailAction,    &ps_RailFireEMP[6], psf_ShadeNone, 0,0},
    {ID_RailFire1,       Rail_ACTION_RATE, pRailAction,    &ps_RailFireEMP[7], psf_ShadeNone, 0,0},
    {ID_RailFire1,       0,                pRailFire,      &ps_RailFireEMP[8], psf_ShadeNone|psf_QuickCall, 0,0},

    {ID_RailCharge0,      Rail_ACTION_RATE, pRailRestTest,  &ps_RailFireEMP[9], psf_ShadeNone, 0,0},
    {ID_RailCharge1,      Rail_ACTION_RATE, pRailRest,      &ps_RailRest[0], psf_ShadeNone, 0,0},
};


PANEL_STATE ps_RetractRail[] =
{
    {ID_RailPresent0, Rail_BEAT_RATE, pRailRetract, &ps_RetractRail[0], psf_ShadeNone, 0,0}
};

#define RAIL_YOFF 200
//#define RAIL_XOFF (160+60)
#define RAIL_XOFF (160+6)

void
InitWeaponRail(PLAYERp pp)
{
    PANEL_SPRITEp psp = nullptr;

    if (SW_SHAREWARE) return;

    if (Prediction)
        return;

    pp->WeaponType = WPN_RAIL;

    if (!TEST(pp->WpnFlags, BIT(pp->WeaponType)) ||
//        pp->WpnAmmo[pp->WeaponType] <= 0 ||
        TEST(pp->Flags, PF_WEAPON_RETRACT))
        return;

    if (!pp->Wpn[pp->WeaponType])
    {
        psp = pp->Wpn[pp->WeaponType] = pSpawnSprite(pp, ps_PresentRail, PRI_MID, RAIL_XOFF, RAIL_YOFF);
        psp->y += tileHeight(psp->picndx);
        psp->backupy();
    }

    if (pp->CurWpn == pp->Wpn[pp->WeaponType])
    {
        return;
    }

    psp->WeaponType = pp->WeaponType;
    PlayerUpdateWeapon(pp, pp->WeaponType);

    pp->WpnUziType = 2; // Make uzi's go away!
    RetractCurWpn(pp);

    // Set up the new Weapon variables
    psp = pp->CurWpn = pp->Wpn[pp->WeaponType];
    SET(psp->flags, PANF_WEAPON_SPRITE);
    psp->ActionState = ps_RailFire;
    psp->RetractState = ps_RetractRail;
    psp->PresentState = ps_PresentRail;
    psp->RestState = ps_RailRest;
    pSetState(psp, psp->PresentState);

    PlaySound(DIGI_RAIL_UP, pp, v3df_follow);
    PlaySound(DIGI_RAILREADY, pp, v3df_follow | v3df_dontpan, CHAN_ITEM); // this one needs to be on a dedicated channel to allow switching it off without too many checks.
    Set3DSoundOwner(psp->PlayerP->PlayerSprite);

    psp->PlayerP->KeyPressBits |= SB_FIRE;
}

void
pRailSetRecoil(PANEL_SPRITEp psp)
{
    psp->vel = 900;
    psp->ang = NORM_ANGLE(-256);
}

void
pRailRecoilDown(PANEL_SPRITEp psp)
{
    psp->backupcoords();

    psp->x += pspCosVel(psp);
    psp->y -= pspSinVel(psp);

    psp->vel -= 24 * synctics;

    if (psp->vel < 800)
    {
        psp->vel = 800;
        psp->ang = NORM_ANGLE(psp->ang + 1024);

        pStatePlusOne(psp);
    }
}

void
pRailRecoilUp(PANEL_SPRITEp psp)
{
    psp->backupcoords();

    psp->x += pspCosVel(psp);
    psp->y -= pspSinVel(psp);

    psp->vel += 15 * synctics;

    if (psp->y < RAIL_YOFF)
    {
        psp->oy = psp->y = RAIL_YOFF;
        psp->ox = psp->x = RAIL_XOFF;

        pRailSetRecoil(psp);

        pStatePlusOne(psp);
        RESET(psp->flags, PANF_BOB);
    }
}

void
pRailPresent(PANEL_SPRITEp psp)
{
    if (TEST(psp->PlayerP->Flags, PF_WEAPON_RETRACT))
        return;

    // Needed for recoil
    psp->ang = NORM_ANGLE(256 + 128);
    pRailSetRecoil(psp);
    ///

    psp->backupy();
    psp->y -= 3 * synctics;

    if (psp->y < RAIL_YOFF)
    {
        psp->oy = psp->y = RAIL_YOFF;
        psp->backupboby();
        pSetState(psp, psp->RestState);
    }
}

void
pRailBobSetup(PANEL_SPRITEp psp)
{
    if (TEST(psp->flags, PANF_BOB))
        return;

    psp->backupbobcoords();

    psp->sin_amt = 12;
    psp->sin_ndx = 0;
    psp->bob_height_divider = 8;
}

void
pRailHide(PANEL_SPRITEp psp)
{
    short picnum = psp->picndx;

    psp->backupy();
    psp->y += 3 * synctics;

    if (psp->y >= RAIL_YOFF + tileHeight(picnum))
    {
        psp->oy = psp->y = RAIL_YOFF + tileHeight(picnum);

        pWeaponUnHideKeys(psp, psp->PresentState);
    }
}

void
pRailOkTest(PANEL_SPRITEp psp)
{
    if (pWeaponHideKeys(psp, ps_RailHide))
        return;

    WeaponOK(psp->PlayerP);
}

void
pRailRest(PANEL_SPRITEp psp)
{
    int InitLaserSight(PLAYERp pp);
    bool force = !!TEST(psp->flags, PANF_UNHIDE_SHOOT);

    if (SW_SHAREWARE) return;

    if (pWeaponHideKeys(psp, ps_RailHide))
        return;

    pRailBobSetup(psp);
    pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP));

    if ((psp->PlayerP->input.actions & SB_FIRE) || force)
    {
        if ((psp->PlayerP->KeyPressBits & SB_FIRE) || force)
        {
            RESET(psp->flags, PANF_UNHIDE_SHOOT);

            if (!WeaponOK(psp->PlayerP))
                return;

            //PlaySound(DIGI_RAILPWRUP, &psp->PlayerP->posx, &psp->PlayerP->posy, &psp->PlayerP->posz, v3df_follow);

            DoPlayerChooseYell(psp->PlayerP);
            if (psp->PlayerP->WpnRailType == 0)
                pSetState(psp, ps_RailFire);
            else
                pSetState(psp, ps_RailFireEMP);
        }
    }
    else
        WeaponOK(psp->PlayerP);
}

void
pRailRestTest(PANEL_SPRITEp psp)
{
    bool force = !!TEST(psp->flags, PANF_UNHIDE_SHOOT);

    if (pWeaponHideKeys(psp, ps_RailHide))
        return;

    pRailBobSetup(psp);
    pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP));

    if ((psp->PlayerP->input.actions & SB_FIRE) || force)
    {
        if ((psp->PlayerP->KeyPressBits & SB_FIRE) || force)
        {
            RESET(psp->flags, PANF_UNHIDE_SHOOT);

            if (!WeaponOK(psp->PlayerP))
                return;

            DoPlayerChooseYell(psp->PlayerP);
            return;
        }
    }

    pSetState(psp, psp->RestState);
}

void
pRailAction(PANEL_SPRITEp psp)
{
    pRailBobSetup(psp);
    pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP));
}


void
pRailFire(PANEL_SPRITEp psp)
{
    SpawnVis(psp->PlayerP->PlayerSprite, -1, -1, -1, -1, 16);
    if (psp->PlayerP->WpnRailType == 0)
        InitRail(psp->PlayerP);
    else
        InitEMP(psp->PlayerP);
}

void
pRailRetract(PANEL_SPRITEp psp)
{
    short picnum = psp->picndx;

    psp->backupy();
    psp->y += 3 * synctics;

    if (psp->y >= RAIL_YOFF + tileHeight(picnum) + 50)
    {
        RESET(psp->PlayerP->Flags, PF_WEAPON_RETRACT);
        psp->PlayerP->Wpn[psp->WeaponType] = nullptr;
        DeleteNoSoundOwner(psp->PlayerP->PlayerSprite);
        pKillSprite(psp);
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
//
// HOTHEAD
//
//////////////////////////////////////////////////////////////////////////////////////////

void pHotheadPresent(PANEL_SPRITEp psp);
void pHotheadRetract(PANEL_SPRITEp psp);
void pHotheadAction(PANEL_SPRITEp psp);
void pHotheadActionCenter(PANEL_SPRITEp psp);
void pHotheadRest(PANEL_SPRITEp psp);
void pHotheadRestCenter(PANEL_SPRITEp psp);
void pHotheadAttack(PANEL_SPRITEp psp);
void pHotheadRing(PANEL_SPRITEp psp);
void pHotheadNapalm(PANEL_SPRITEp psp);
void pHotheadHide(PANEL_SPRITEp psp);
void pHotheadRestTest(PANEL_SPRITEp psp);

extern PANEL_STATE ps_HotheadAttack[];
extern PANEL_STATE ps_ReloadHothead[];
extern PANEL_STATE ps_HotheadTurn[];


#define Hothead_BEAT_RATE 24
#define Hothead_ACTION_RATE_PRE 2  // !JIM! Was 1
#define Hothead_ACTION_RATE_POST 7

PANEL_STATE ps_PresentHothead[] =
{
    {ID_HotheadPresent0, Hothead_BEAT_RATE, pHotheadPresent, &ps_PresentHothead[0], 0,0,0}
};

PANEL_STATE ps_HotheadHide[] =
{
    {ID_HotheadRest0, Hothead_BEAT_RATE, pHotheadHide, &ps_HotheadHide[0], 0,0,0}
};

PANEL_STATE ps_RetractHothead[] =
{
    {ID_HotheadPresent0, Hothead_BEAT_RATE, pHotheadRetract, &ps_RetractHothead[0], 0,0,0}
};

PANEL_STATE ps_HotheadRest[] =
{
    {ID_HotheadRest0, Hothead_BEAT_RATE, pHotheadRest, &ps_HotheadRest[0], 0,0,0}
};

PANEL_STATE ps_HotheadRestRing[] =
{
    {ID_HotheadRest0, Hothead_BEAT_RATE, pHotheadRest, &ps_HotheadRest[0], 0,0,0}
};

PANEL_STATE ps_HotheadRestNapalm[] =
{
    {ID_HotheadRest0, Hothead_BEAT_RATE, pHotheadRest, &ps_HotheadRest[0], 0,0,0}
};
// Turns - attacks

PANEL_STATE ps_HotheadAttack[] =
{
    {ID_HotheadAttack0, Hothead_ACTION_RATE_PRE,  pHotheadAction, &ps_HotheadAttack[1], psf_ShadeHalf, 0,0},
    {ID_HotheadAttack0, 3,                         pHotheadAction, &ps_HotheadAttack[2], psf_ShadeHalf, 0,0},
    {ID_HotheadAttack0, 0,                         pHotheadAttack, &ps_HotheadAttack[3], psf_QuickCall, 0,0},
    {ID_HotheadAttack0, 3,                         pHotheadAction, &ps_HotheadAttack[4], psf_ShadeHalf, 0,0},
    {ID_HotheadAttack0, 0,                         pHotheadRestTest, &ps_HotheadAttack[4], psf_QuickCall, 0,0},
    {ID_HotheadAttack0, 0,                         pHotheadAction, &ps_HotheadAttack[0], psf_ShadeHalf, 0,0}
};

PANEL_STATE ps_HotheadRing[] =
{
    {ID_HotheadAttack0, Hothead_ACTION_RATE_PRE,  pHotheadAction, &ps_HotheadRing[1], psf_ShadeHalf, 0,0},
    {ID_HotheadAttack0, 10,                        pHotheadAction, &ps_HotheadRing[2], psf_ShadeHalf, 0,0},
    {ID_HotheadAttack0, 0,                         pHotheadAttack, &ps_HotheadRing[3], psf_QuickCall, 0,0},
    {ID_HotheadAttack0, 40,                        pHotheadAction, &ps_HotheadRing[4], psf_ShadeHalf, 0,0},
    {ID_HotheadAttack0, 0,                         pHotheadRestTest, &ps_HotheadRing[4], psf_QuickCall, 0,0},
    {ID_HotheadAttack0, 3,                         pHotheadAction, &ps_HotheadRing[0], psf_ShadeHalf, 0,0}
};

PANEL_STATE ps_HotheadNapalm[] =
{
    {ID_HotheadAttack0, Hothead_ACTION_RATE_PRE,  pHotheadAction, &ps_HotheadNapalm[1], psf_ShadeHalf, 0,0},
    {ID_HotheadAttack0, 3,                        pHotheadAction, &ps_HotheadNapalm[2], psf_ShadeHalf, 0,0},
    {ID_HotheadAttack0, 0,                         pHotheadAttack, &ps_HotheadNapalm[3], psf_QuickCall, 0,0},
    {ID_HotheadAttack0, 50,                        pHotheadAction, &ps_HotheadNapalm[4], psf_ShadeHalf, 0,0},
    {ID_HotheadAttack0, 0,                         pHotheadRestTest, &ps_HotheadNapalm[4], psf_QuickCall, 0,0},
    {ID_HotheadAttack0, 3,                         pHotheadAction, &ps_HotheadNapalm[0], psf_ShadeHalf, 0,0}
};

// Turns - can do three different turns

PANEL_STATE ps_HotheadTurn[] =
{
    {ID_HotheadTurn0, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurn[1], 0,0,0},
    {ID_HotheadTurn1, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurn[2], 0,0,0},
    {ID_HotheadTurn2, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurn[3], 0,0,0},
    {ID_HotheadTurn3, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurn[4], 0,0,0},
    {ID_HotheadChomp0, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurn[5], 0,0,0},
    {ID_HotheadTurn3, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurn[6], 0,0,0},
    {ID_HotheadChomp0, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurn[7], 0,0,0},
    {ID_HotheadTurn3, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurn[8], 0,0,0},
    {ID_HotheadTurn2, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurn[9], 0,0,0},
    {ID_HotheadTurn1, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurn[10], 0,0,0},
    {ID_HotheadTurn0, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurn[11], 0,0,0},
    {ID_HotheadTurn0, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadRest[0], 0,0,0}
};

PANEL_STATE ps_HotheadTurnRing[] =
{
    {ID_HotheadTurn0, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurnRing[1], 0,0,0},
    {ID_HotheadTurn1, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurnRing[2], 0,0,0},
    {ID_HotheadTurn2, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurnRing[3], 0,0,0},
    {ID_HotheadTurn3, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurnRing[4], 0,0,0},
    {ID_HotheadChomp0, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurnRing[5], 0,0,0},
    {ID_HotheadTurn3, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurnRing[6], 0,0,0},
    {ID_HotheadTurn2, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurnRing[7], 0,0,0},
    {ID_HotheadTurn1, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurnRing[8], 0,0,0},
    {ID_HotheadTurn0, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurnRing[9], 0,0,0},
    {ID_HotheadTurn0, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadRestRing[0], 0,0,0}
};

PANEL_STATE ps_HotheadTurnNapalm[] =
{
    {ID_HotheadTurn0, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurnNapalm[1], 0,0,0},
    {ID_HotheadTurn1, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurnNapalm[2], 0,0,0},
    {ID_HotheadTurn2, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurnNapalm[3], 0,0,0},
    {ID_HotheadTurn3, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurnNapalm[4], 0,0,0},
    {ID_HotheadTurn3, Hothead_BEAT_RATE*2, pNullAnimator, &ps_HotheadTurnNapalm[5], 0,0,0},
    {ID_HotheadTurn2, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurnNapalm[6], 0,0,0},
    {ID_HotheadTurn1, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurnNapalm[7], 0,0,0},
    {ID_HotheadTurn0, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadTurnNapalm[8], 0,0,0},
    {ID_HotheadTurn0, Hothead_BEAT_RATE, pNullAnimator, &ps_HotheadRestNapalm[0], 0,0,0}
};


PANEL_STATEp HotheadAttackStates[] =
{
    ps_HotheadAttack,
    ps_HotheadRing,
    ps_HotheadNapalm
};

PANEL_STATEp HotheadRestStates[] =
{
    ps_HotheadRest,
    ps_HotheadRestRing,
    ps_HotheadRestNapalm
};

PANEL_STATEp HotheadTurnStates[] =
{
    ps_HotheadTurn,
    ps_HotheadTurnRing,
    ps_HotheadTurnNapalm
};

#define FIREBALL_MODE 0
#define     RING_MODE 1
#define   NAPALM_MODE 2
void
pHotHeadOverlays(PANEL_SPRITEp psp, short mode)
{
#define HOTHEAD_FINGER_XOFF 0
#define HOTHEAD_FINGER_YOFF 0

    switch (mode)
    {
    case 0: // Great balls o' fire
        psp->over[0].pic = HEAD_MODE1;
        break;
    case 1: // Ring of fire
        psp->over[0].pic = HEAD_MODE2;
        break;
    case 2: // I love the smell of napalm in the morning
        psp->over[0].pic = HEAD_MODE3;
        break;
    }
}

#define HOTHEAD_BOB_X_AMT 10

#define HOTHEAD_XOFF (200 + HOTHEAD_BOB_X_AMT + 6)
#define HOTHEAD_YOFF 200

void
InitWeaponHothead(PLAYERp pp)
{
    PANEL_SPRITEp psp = nullptr;

    if (SW_SHAREWARE) return;

    if (Prediction)
        return;

    if (!TEST(pp->WpnFlags, BIT(WPN_HOTHEAD)) ||
//        pp->WpnAmmo[WPN_HOTHEAD] <= 0 ||
        TEST(pp->Flags, PF_WEAPON_RETRACT))
        return;

    if (!pp->Wpn[WPN_HOTHEAD])
    {
        psp = pp->Wpn[WPN_HOTHEAD] = pSpawnSprite(pp, ps_PresentHothead, PRI_MID, HOTHEAD_XOFF, HOTHEAD_YOFF);
        psp->y += tileHeight(psp->picndx);
        psp->backupy();
    }

    if (pp->CurWpn == pp->Wpn[WPN_HOTHEAD])
    {
        return;
    }

    psp->WeaponType = WPN_HOTHEAD;
    PlayerUpdateWeapon(pp, WPN_HOTHEAD);

    pp->WpnUziType = 2; // Make uzi's go away!
    RetractCurWpn(pp);

    // Set up the new Weapon variables
    psp = pp->CurWpn = pp->Wpn[WPN_HOTHEAD];
    SET(psp->flags, PANF_WEAPON_SPRITE);
    psp->ActionState = ps_HotheadAttack;
    psp->PresentState = ps_PresentHothead;
    psp->RestState = HotheadRestStates[psp->PlayerP->WpnFlameType];
    psp->RetractState = ps_RetractHothead;
    pSetState(psp, psp->PresentState);
    psp->ang = 768;
    psp->vel = 512;

    psp->PlayerP->KeyPressBits |= SB_FIRE;
    pHotHeadOverlays(psp, pp->WpnFlameType);
    psp->over[0].xoff = HOTHEAD_FINGER_XOFF;
    psp->over[0].yoff = HOTHEAD_FINGER_YOFF;

    PlaySound(DIGI_GRDALERT, pp, v3df_follow|v3df_dontpan);
}

void
pHotheadRestTest(PANEL_SPRITEp psp)
{
    if (psp->PlayerP->input.actions & SB_FIRE)
    {
        if (psp->PlayerP->KeyPressBits & SB_FIRE)
        {
            //if (!TEST(psp->PlayerP->Flags,PF_DIVING))
            {
                if (!WeaponOK(psp->PlayerP))
                    return;

                if (psp->PlayerP->WpnAmmo[WPN_HOTHEAD] < 10)
                {
                    psp->PlayerP->WpnFlameType = 0;
                    WeaponOK(psp->PlayerP);
                }

                DoPlayerChooseYell(psp->PlayerP);
            }

            pStatePlusOne(psp);
            return;
        }
    }

    pSetState(psp, HotheadRestStates[psp->PlayerP->WpnFlameType]);
    psp->over[0].xoff = HOTHEAD_FINGER_XOFF;
    psp->over[0].yoff = HOTHEAD_FINGER_YOFF;
}

void
pHotheadPresent(PANEL_SPRITEp psp)
{
    if (TEST(psp->PlayerP->Flags, PF_WEAPON_RETRACT))
        return;

    psp->backupy();
    psp->y -= 3 * synctics;

    if (psp->y < HOTHEAD_YOFF)
    {
        psp->oy = psp->y = HOTHEAD_YOFF;
        psp->backupboby();
        pSetState(psp, psp->RestState);
        //pSetState(psp, HotheadTurnStates[psp->PlayerP->WpnFlameType]);
    }
}

void
pHotheadBobSetup(PANEL_SPRITEp psp)
{
    if (TEST(psp->flags, PANF_BOB))
        return;

    psp->backupbobcoords();

    psp->sin_amt = HOTHEAD_BOB_X_AMT;
    psp->sin_ndx = 0;
    psp->bob_height_divider = 4;
}

void
pHotheadHide(PANEL_SPRITEp psp)
{
    short picnum = psp->picndx;

    psp->backupx();
    psp->x += 3 * synctics;

    if (psp->x >= HOTHEAD_XOFF + tileWidth(picnum) || psp->y >= HOTHEAD_YOFF + tileHeight(picnum))
    {
        psp->ox = psp->x = HOTHEAD_XOFF;
        psp->oy = psp->y = HOTHEAD_YOFF + tileHeight(picnum);

        pWeaponUnHideKeys(psp, psp->PresentState);
    }
}


void
pHotheadRest(PANEL_SPRITEp psp)
{
    bool force = !!TEST(psp->flags, PANF_UNHIDE_SHOOT);

    if (SW_SHAREWARE) return;

    if (pWeaponHideKeys(psp, ps_HotheadHide))
        return;

    if (HotheadRestStates[psp->PlayerP->WpnFlameType] != psp->RestState)
    {
        psp->RestState = HotheadRestStates[psp->PlayerP->WpnFlameType];
        pSetState(psp, HotheadRestStates[psp->PlayerP->WpnFlameType]);
    }

    // in rest position - only bob when in rest pos
    pHotheadBobSetup(psp);
    pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP));

    if ((psp->PlayerP->input.actions & SB_FIRE) || force)
    {
        if ((psp->PlayerP->KeyPressBits & SB_FIRE) || force)
        {
            RESET(psp->flags, PANF_UNHIDE_SHOOT);

            //if (TEST(psp->PlayerP->Flags,PF_DIVING))
            //    return;

            if (!WeaponOK(psp->PlayerP))
                return;

            if (psp->PlayerP->WpnAmmo[WPN_HOTHEAD] < 10)
            {
                psp->PlayerP->WpnFlameType = 0;
                WeaponOK(psp->PlayerP);
            }

            DoPlayerChooseYell(psp->PlayerP);
            //pSetState(psp, psp->ActionState);
            pSetState(psp, HotheadAttackStates[psp->PlayerP->WpnFlameType]);
            psp->over[0].xoff = HOTHEAD_FINGER_XOFF-1;
            psp->over[0].yoff = HOTHEAD_FINGER_YOFF-10;
        }
    }
    else
        WeaponOK(psp->PlayerP);
}

void
pHotheadAction(PANEL_SPRITEp psp)
{
    char shooting;

    shooting = (psp->PlayerP->input.actions & SB_FIRE) && (psp->PlayerP->KeyPressBits & SB_FIRE);

    if (shooting)
    {
        pUziBobSetup(psp);
        pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP) || shooting);
    }
}

void
pHotheadAttack(PANEL_SPRITEp psp)
{
    switch (psp->PlayerP->WpnFlameType)
    {
    case 0:
        SpawnVis(psp->PlayerP->PlayerSprite, -1, -1, -1, -1, 32);
        InitFireball(psp->PlayerP);
        break;
    case 1:
        SpawnVis(psp->PlayerP->PlayerSprite, -1, -1, -1, -1, 20);
        InitSpellRing(psp->PlayerP);
        break;
    case 2:
        SpawnVis(psp->PlayerP->PlayerSprite, -1, -1, -1, -1, 16);
        InitSpellNapalm(psp->PlayerP);
        break;
    }
}

void
pHotheadRetract(PANEL_SPRITEp psp)
{
    short picnum = psp->picndx;

    psp->backupy();
    psp->y += 3 * synctics;

    if (psp->y >= HOTHEAD_YOFF + tileHeight(picnum))
    {
        RESET(psp->PlayerP->Flags, PF_WEAPON_RETRACT);
        psp->PlayerP->Wpn[WPN_HOTHEAD] = nullptr;
        pKillSprite(psp);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// HOTHEAD ON_FIRE
//
//////////////////////////////////////////////////////////////////////////////////////////

void pOnFire(PANEL_SPRITEp);

#define ON_FIRE_RATE 10

PANEL_STATE ps_OnFire[] =
{
    {ID_OnFire0, ON_FIRE_RATE, pOnFire, &ps_OnFire[1], 0,0,0},
    {ID_OnFire1, ON_FIRE_RATE, pOnFire, &ps_OnFire[2], 0,0,0},
    {ID_OnFire2, ON_FIRE_RATE, pOnFire, &ps_OnFire[3], 0,0,0},
    {ID_OnFire3, ON_FIRE_RATE, pOnFire, &ps_OnFire[4], 0,0,0},
    {ID_OnFire4, ON_FIRE_RATE, pOnFire, &ps_OnFire[5], 0,0,0},
    {ID_OnFire5, ON_FIRE_RATE, pOnFire, &ps_OnFire[6], 0,0,0},
    {ID_OnFire6, ON_FIRE_RATE, pOnFire, &ps_OnFire[7], 0,0,0},
    {ID_OnFire7, ON_FIRE_RATE, pOnFire, &ps_OnFire[8], 0,0,0},
    {ID_OnFire8, ON_FIRE_RATE, pOnFire, &ps_OnFire[9], 0,0,0},
    {ID_OnFire9, ON_FIRE_RATE, pOnFire, &ps_OnFire[10], 0,0,0},
    {ID_OnFire10, ON_FIRE_RATE, pOnFire, &ps_OnFire[11], 0,0,0},
    {ID_OnFire11, ON_FIRE_RATE, pOnFire, &ps_OnFire[12], 0,0,0},
    {ID_OnFire12, ON_FIRE_RATE, pOnFire, &ps_OnFire[0], 0,0,0},
};

#define ON_FIRE_Y_TOP 190
#define ON_FIRE_Y_BOT 230

void
SpawnOnFire(PLAYERp pp)
{
    PANEL_SPRITEp fire;
    short x = 50;

    while (x < 320)
    {
        fire = pSpawnSprite(pp, &ps_OnFire[RANDOM_P2(8<<8)>>8], PRI_FRONT, x, ON_FIRE_Y_BOT);
        SET(fire->flags, PANF_WEAPON_SPRITE);
        x += tileWidth(fire->picndx);
    }
}

void
pOnFire(PANEL_SPRITEp psp)
{
    // Kill immediately - in case of death/water
    if (User[psp->PlayerP->PlayerSprite]->flame <= -2)
    {
        pKillSprite(psp);
        return;
    }

    psp->backupy();    

    if (User[psp->PlayerP->PlayerSprite]->flame == -1)
    {
        // take flames down and kill them
        psp->y += 1;
        if (psp->y > ON_FIRE_Y_BOT)
        {
            pKillSprite(psp);
            return;
        }
    }
    else
    {
        // bring flames up
        psp->y -= 2;
        if (psp->y < ON_FIRE_Y_TOP)
            psp->oy = psp->y = ON_FIRE_Y_TOP;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// MICRO
//
//////////////////////////////////////////////////////////////////////////////////////////

void pMicroPresent(PANEL_SPRITEp psp);
void pMicroRetract(PANEL_SPRITEp psp);
void pMicroAction(PANEL_SPRITEp psp);
void pMicroRest(PANEL_SPRITEp psp);
void pMicroFire(PANEL_SPRITEp psp);
void pMicroHide(PANEL_SPRITEp psp);
void pMicroUnHide(PANEL_SPRITEp psp);
void pMicroRecoilDown(PANEL_SPRITEp psp);
void pMicroRecoilUp(PANEL_SPRITEp psp);
void pMicroBobSetup(PANEL_SPRITEp psp);
void pMicroReloadUp(PANEL_SPRITEp psp);
void pMicroReloadDown(PANEL_SPRITEp psp);
void pMicroStandBy(PANEL_SPRITEp psp);
void pMicroCount(PANEL_SPRITEp psp);
void pMicroReady(PANEL_SPRITEp psp);
void pNukeAction(PANEL_SPRITEp psp);

extern PANEL_STATE ps_MicroReload[];

#define Micro_REST_RATE 24
#define Micro_ACTION_RATE 6  // !JIM! was 9

PANEL_STATE ps_PresentMicro[] =
{
    {ID_MicroPresent0, Micro_REST_RATE, pMicroPresent, &ps_PresentMicro[0], 0,0,0}
};

PANEL_STATE ps_MicroRest[] =
{
    {ID_MicroPresent0, Micro_REST_RATE, pMicroRest, &ps_MicroRest[0], 0,0,0}
};

PANEL_STATE ps_MicroHide[] =
{
    {ID_MicroPresent0, Micro_REST_RATE, pMicroHide, &ps_MicroHide[0], 0,0,0}
};

PANEL_STATE ps_InitNuke[] =
{
    {ID_MicroPresent0, Micro_ACTION_RATE,pNukeAction,  &ps_InitNuke[1], 0,0,0},
    {ID_MicroPresent0, 0,               pMicroStandBy,  &ps_InitNuke[2], psf_QuickCall, 0,0},
    {ID_MicroPresent0, 120*2,           pNukeAction, &ps_InitNuke[3], 0,0,0},
    {ID_MicroPresent0, 0,               pMicroCount,  &ps_InitNuke[4], psf_QuickCall, 0,0},
    {ID_MicroPresent0, 120*3,           pNukeAction, &ps_InitNuke[5], 0,0,0},
    {ID_MicroPresent0, 0,               pMicroReady,  &ps_InitNuke[6], psf_QuickCall, 0,0},
    {ID_MicroPresent0, 120*2,           pNukeAction, &ps_InitNuke[7], 0,0,0},
    {ID_MicroPresent0, 3,               pNukeAction, &ps_MicroRest[0], 0,0,0}
};

PANEL_STATE ps_MicroRecoil[] =
{
    // recoil
    {ID_MicroPresent0, Micro_ACTION_RATE, pMicroRecoilDown, &ps_MicroRecoil[0], 0,0,0},
    {ID_MicroPresent0, Micro_ACTION_RATE, pMicroRecoilUp,   &ps_MicroRecoil[1], 0,0,0},

    // Firing delay.
    {ID_MicroPresent0, 30, pNullAnimator,   &ps_MicroRecoil[3], 0,0,0},
    // ready to fire again
    {ID_MicroPresent0, 3, pNullAnimator, &ps_MicroRest[0], 0,0,0}
};

PANEL_STATE ps_MicroFire[] =
{
    {ID_MicroFire0, Micro_ACTION_RATE, pMicroAction, &ps_MicroFire[1], psf_ShadeNone, 0,0},
    {ID_MicroFire1, Micro_ACTION_RATE, pMicroAction, &ps_MicroFire[2], psf_ShadeNone, 0,0},
    {ID_MicroFire2, Micro_ACTION_RATE, pMicroAction, &ps_MicroFire[3], psf_ShadeHalf, 0,0},
    {ID_MicroFire3, Micro_ACTION_RATE, pMicroAction, &ps_MicroFire[4], psf_ShadeHalf, 0,0},
    {ID_MicroPresent0, 0,              pMicroFire, &ps_MicroFire[5], psf_ShadeNone|psf_QuickCall, 0,0},


    // !JIM! After firing delay so rockets can't fire so fast!
    // Putting a BIG blast radius for rockets, this is better than small and fast for this weap.
    {ID_MicroPresent0, 120, pMicroAction, &ps_MicroFire[6], 0,0,0},

    {ID_MicroPresent0, 3, pMicroAction, &ps_MicroRecoil[0], 0,0,0}
};

#define Micro_SINGLE_RATE 8
#define Micro_DISSIPATE_RATE 6

PANEL_STATE ps_MicroSingleFire[] =
{
    {ID_MicroSingleFire0, Micro_SINGLE_RATE,    pMicroAction,   &ps_MicroSingleFire[1], psf_ShadeHalf, 0,0},
    {ID_MicroSingleFire1, Micro_SINGLE_RATE,    pMicroAction,   &ps_MicroSingleFire[2], psf_ShadeNone, 0,0},
    {ID_MicroSingleFire1, 0,                    pMicroFire,     &ps_MicroSingleFire[3], psf_ShadeNone|psf_QuickCall, 0,0},
    {ID_MicroSingleFire2, Micro_DISSIPATE_RATE, pMicroAction,   &ps_MicroSingleFire[4], psf_ShadeNone, 0,0},
    {ID_MicroSingleFire3, Micro_DISSIPATE_RATE, pMicroAction,   &ps_MicroSingleFire[5], psf_ShadeHalf, 0,0},

    // !JIM! Put in firing delay.
    //{ID_MicroPresent0, 60, pMicroAction,   &ps_MicroSingleFire[6]},

    {ID_MicroPresent0,    3,                    pMicroAction,   &ps_MicroRecoil[0], 0,0,0}
};

PANEL_STATE ps_RetractMicro[] =
{
    {ID_MicroPresent0, Micro_REST_RATE, pMicroRetract, &ps_RetractMicro[0], 0,0,0}
};

#define MICRO_BOB_X_AMT 10
#define MICRO_YOFF 205
#define MICRO_XOFF (150+MICRO_BOB_X_AMT)

void
pMicroSetRecoil(PANEL_SPRITEp psp)
{
    psp->vel = 900;
    psp->ang = NORM_ANGLE(-256);
}

void
InitWeaponMicro(PLAYERp pp)
{
    PANEL_SPRITEp psp;

    if (Prediction)
        return;

    if (!TEST(pp->WpnFlags, BIT(WPN_MICRO)) ||
//        pp->WpnAmmo[WPN_MICRO] <= 0 ||
        TEST(pp->Flags, PF_WEAPON_RETRACT))
        return;

    if (!pp->Wpn[WPN_MICRO])
    {
        psp = pp->Wpn[WPN_MICRO] = pSpawnSprite(pp, ps_PresentMicro, PRI_MID, MICRO_XOFF, MICRO_YOFF);
        psp->y += tileHeight(psp->picndx);
        psp->backupy();
    }

    if (pp->CurWpn == pp->Wpn[WPN_MICRO])
    {
        if (pp->TestNukeInit && pp->WpnRocketType == 2 && !pp->InitingNuke && pp->WpnRocketNuke && !pp->NukeInitialized)
        {
            pp->TestNukeInit = false;
            pp->InitingNuke = true;
            psp = pp->Wpn[WPN_MICRO];
            pSetState(psp, ps_InitNuke);
        }
        return;
    }

    PlayerUpdateWeapon(pp, WPN_MICRO);

    pp->WpnUziType = 2; // Make uzi's go away!
    RetractCurWpn(pp);

    // Set up the new Weapon variables
    psp = pp->CurWpn = pp->Wpn[WPN_MICRO];
    SET(psp->flags, PANF_WEAPON_SPRITE);
    psp->ActionState = ps_MicroFire;
    psp->RetractState = ps_RetractMicro;
    psp->RestState = ps_MicroRest;
    psp->PresentState = ps_PresentMicro;
    pSetState(psp, psp->PresentState);

    if (pp->WpnRocketType == 2 && !pp->InitingNuke && !pp->NukeInitialized)
        pp->TestNukeInit = pp->InitingNuke = true;

    PlaySound(DIGI_ROCKET_UP, pp, v3df_follow);

    psp->PlayerP->KeyPressBits |= SB_FIRE;

}


void
pMicroRecoilDown(PANEL_SPRITEp psp)
{
    psp->backupcoords();

    psp->x += pspCosVel(psp);
    psp->y -= pspSinVel(psp);

    psp->vel -= 24 * synctics;

    if (psp->vel < 550)
    {
        psp->vel = 550;
        psp->ang = NORM_ANGLE(psp->ang + 1024);

        pStatePlusOne(psp);
    }
}

void
pMicroRecoilUp(PANEL_SPRITEp psp)
{
    psp->backupcoords();

    psp->x += pspCosVel(psp);
    psp->y -= pspSinVel(psp);

    psp->vel += 15 * synctics;

    if (psp->y < MICRO_YOFF)
    {
        psp->oy = psp->y = MICRO_YOFF;
        psp->ox = psp->x = MICRO_XOFF;

        pMicroSetRecoil(psp);

        pStatePlusOne(psp);
        RESET(psp->flags, PANF_BOB);
    }
}

void
pMicroPresent(PANEL_SPRITEp psp)
{
    PLAYERp pp = psp->PlayerP;

    if (TEST(psp->PlayerP->Flags, PF_WEAPON_RETRACT))
        return;

    // Needed for recoil
    psp->ang = NORM_ANGLE(256 + 96);
    pMicroSetRecoil(psp);
    ///

    psp->backupy();
    psp->y -= 3 * synctics;

    if (psp->y < MICRO_YOFF)
    {
        psp->oy = psp->y = MICRO_YOFF;
        psp->backupboby();
        if (pp->WpnRocketType == 2 && !pp->NukeInitialized)
        {
            pp->TestNukeInit = false;
            pSetState(psp, ps_InitNuke);
        }
        else
            pSetState(psp, psp->RestState);
    }
}

void
pMicroBobSetup(PANEL_SPRITEp psp)
{
    if (TEST(psp->flags, PANF_BOB))
        return;

    psp->backupbobcoords();

    psp->sin_amt = MICRO_BOB_X_AMT;
    psp->sin_ndx = 0;
    psp->bob_height_divider = 8;
}

void
pMicroHide(PANEL_SPRITEp psp)
{
    short picnum = psp->picndx;

    psp->backupy();
    psp->y += 3 * synctics;

    if (psp->y >= MICRO_YOFF + tileHeight(picnum) + 20)
    {
        psp->oy = psp->y = MICRO_YOFF + tileHeight(picnum) + 20;
        psp->ox = psp->x = MICRO_XOFF;

        pWeaponUnHideKeys(psp, psp->PresentState);
    }
}

bool
pMicroOverlays(PANEL_SPRITEp psp)
{
#define MICRO_SIGHT_XOFF 29
#define MICRO_SIGHT_YOFF -58

#define MICRO_SHOT_XOFF 65
#define MICRO_SHOT_YOFF -41

#define MICRO_HEAT_XOFF 78
#define MICRO_HEAT_YOFF -51

    if (psp->over[MICRO_SIGHT_NUM].xoff == -1)
    {
        psp->over[MICRO_SIGHT_NUM].xoff = MICRO_SIGHT_XOFF;
        psp->over[MICRO_SIGHT_NUM].yoff = MICRO_SIGHT_YOFF;
        psp->over[MICRO_SHOT_NUM].xoff = MICRO_SHOT_XOFF;
        psp->over[MICRO_SHOT_NUM].yoff = MICRO_SHOT_YOFF;
        psp->over[MICRO_HEAT_NUM].xoff = MICRO_HEAT_XOFF;
        psp->over[MICRO_HEAT_NUM].yoff = MICRO_HEAT_YOFF;

    }

    if (psp->PlayerP->WpnRocketNuke == 0 && psp->PlayerP->WpnRocketType == 2)
        psp->PlayerP->WpnRocketType=0;

    switch (psp->PlayerP->WpnRocketType)
    {
    case 0:
        psp->over[MICRO_SIGHT_NUM].pic = MICRO_SIGHT;
        psp->over[MICRO_SHOT_NUM].pic = MICRO_SHOT_1;
        SET(psp->over[MICRO_SHOT_NUM].flags, psf_ShadeNone);
        psp->over[MICRO_HEAT_NUM].pic = -1;
        return false;
    case 1:
        if (psp->PlayerP->WpnRocketHeat)
        {
            psp->over[MICRO_SIGHT_NUM].pic = MICRO_SIGHT;
            psp->over[MICRO_SHOT_NUM].pic = MICRO_SHOT_1;
            SET(psp->over[MICRO_SHOT_NUM].flags, psf_ShadeNone);

            ASSERT(psp->PlayerP->WpnRocketHeat < 6);

            psp->over[MICRO_HEAT_NUM].pic = MICRO_HEAT + (5 - psp->PlayerP->WpnRocketHeat);
        }
        else
        {
            psp->over[MICRO_SIGHT_NUM].pic = MICRO_SIGHT;
            psp->over[MICRO_SHOT_NUM].pic = MICRO_SHOT_1;
            SET(psp->over[MICRO_SHOT_NUM].flags, psf_ShadeNone);
            psp->over[MICRO_HEAT_NUM].pic = -1;
        }

        return false;
    case 2:
        psp->over[MICRO_SIGHT_NUM].pic = -1;
        psp->over[MICRO_HEAT_NUM].pic = -1;

        psp->over[MICRO_SHOT_NUM].pic = MICRO_SHOT_20;
        SET(psp->over[MICRO_SHOT_NUM].flags, psf_ShadeNone);
        SET(psp->over[MICRO_HEAT_NUM].flags, psf_ShadeNone);
        return true;
    }
    return false;
}

PANEL_STATE ps_MicroHeatFlash[] =
{
    {MICRO_HEAT, 30, nullptr, &ps_MicroHeatFlash[1], 0,0,0},
    {0,             30, nullptr, &ps_MicroHeatFlash[2], 0,0,0},
    {MICRO_HEAT,    30, nullptr, &ps_MicroHeatFlash[3], 0,0,0},
    {0,             30, nullptr, &ps_MicroHeatFlash[4], 0,0,0},
    {MICRO_HEAT,    30, nullptr, &ps_MicroHeatFlash[5], 0,0,0},
    {0,             30, nullptr, &ps_MicroHeatFlash[6], 0,0,0},
    {MICRO_HEAT,    30, nullptr, &ps_MicroHeatFlash[7], 0,0,0},
    {0,             30, nullptr, &ps_MicroHeatFlash[8], 0,0,0},
    {MICRO_HEAT,    30, nullptr, &ps_MicroHeatFlash[9], 0,0,0},
    {0,              0, nullptr, nullptr, 0,0,0}
};

PANEL_STATE ps_MicroNukeFlash[] =
{
    {MICRO_SHOT_20, 30, nullptr, &ps_MicroNukeFlash[1], 0,0,0},
    {0,             30, nullptr, &ps_MicroNukeFlash[2], 0,0,0},
    {MICRO_SHOT_20, 30, nullptr, &ps_MicroNukeFlash[3], 0,0,0},
    {0,             30, nullptr, &ps_MicroNukeFlash[4], 0,0,0},
    {MICRO_SHOT_20, 30, nullptr, &ps_MicroNukeFlash[5], 0,0,0},
    {0,             30, nullptr, &ps_MicroNukeFlash[6], 0,0,0},
    {MICRO_SHOT_20, 30, nullptr, &ps_MicroNukeFlash[7], 0,0,0},
    {0,             30, nullptr, &ps_MicroNukeFlash[8], 0,0,0},
    {MICRO_SHOT_20, 30, nullptr, &ps_MicroNukeFlash[9], 0,0,0},
    {0,              0, nullptr, nullptr, 0,0,0}
};

void
pMicroRest(PANEL_SPRITEp psp)
{
    PLAYERp pp = psp->PlayerP;
    bool force = !!TEST(psp->flags, PANF_UNHIDE_SHOOT);

    if (pWeaponHideKeys(psp, ps_MicroHide))
        return;

    pMicroOverlays(psp);

    pMicroBobSetup(psp);
    pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP));

    if (pp->InitingNuke)
    {
        int choose_voc=0;

        pp->InitingNuke = false;
        pp->NukeInitialized = true;

        if (pp == Player+myconnectindex)
        {
            choose_voc = STD_RANDOM_RANGE(1024);
            if (choose_voc > 600)
                PlayerSound(DIGI_TAUNTAI2,v3df_dontpan|v3df_follow,psp->PlayerP);
            else if (choose_voc > 300)
                PlayerSound(DIGI_TAUNTAI4,v3df_dontpan|v3df_follow,psp->PlayerP);
        }
    }

    if ((psp->PlayerP->input.actions & SB_FIRE) || force)
    {
        if ((psp->PlayerP->KeyPressBits & SB_FIRE) || force)
        {
            RESET(psp->flags, PANF_UNHIDE_SHOOT);

            if (!WeaponOK(psp->PlayerP))
                return;

            if (psp->PlayerP->WpnAmmo[WPN_MICRO] <= 0 && psp->PlayerP->WpnRocketType != 2)
            {
                psp->PlayerP->WpnRocketNuke = 0;
                WeaponOK(psp->PlayerP);
                psp->PlayerP->WpnRocketNuke = 1;
                return;
            }

            switch (psp->PlayerP->WpnRocketType)
            {
            case 0:
            case 1:
                pSetState(psp, ps_MicroSingleFire);
                DoPlayerChooseYell(psp->PlayerP);
                break;
            case 2:
                if (psp->PlayerP->WpnRocketNuke > 0)
                    pSetState(psp, ps_MicroSingleFire);
                break;
            }
        }
    }
    else
        WeaponOK(psp->PlayerP);
}

void
pMicroAction(PANEL_SPRITEp psp)
{
    pMicroBobSetup(psp);
    pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP));
}


void
pMicroFire(PANEL_SPRITEp psp)
{
    SpawnVis(psp->PlayerP->PlayerSprite, -1, -1, -1, -1, 20);
    switch (psp->PlayerP->WpnRocketType)
    {
    case 0:
        if (sw_bunnyrockets)
            InitBunnyRocket(psp->PlayerP);
        else
            InitRocket(psp->PlayerP);
        break;
    case 1:
        if (sw_bunnyrockets)
            InitBunnyRocket(psp->PlayerP);
        else
            InitRocket(psp->PlayerP);
        break;
    case 2:
        PlaySound(DIGI_WARNING,psp->PlayerP,v3df_dontpan|v3df_follow);
        InitNuke(psp->PlayerP);
        psp->PlayerP->NukeInitialized = false;
        break;
    }
}

void
pMicroRetract(PANEL_SPRITEp psp)
{
    short picnum = psp->picndx;

    psp->backupy();
    psp->y += 3 * synctics;

    if (psp->y >= MICRO_YOFF + tileHeight(picnum))
    {
        RESET(psp->PlayerP->Flags, PF_WEAPON_RETRACT);
        psp->PlayerP->Wpn[WPN_MICRO] = nullptr;
        pKillSprite(psp);
    }
}

void
pNukeAction(PANEL_SPRITEp psp)
{
    PLAYERp pp = psp->PlayerP;

#if 0 // Code commented out as it's causing interpolation issues when initialising a nuke.
    psp->backupy();
    psp->y -= 3 * synctics;

    if (psp->y < MICRO_YOFF)
    {
        psp->oy = psp->y = MICRO_YOFF;
        psp->backupboby();
    }
#endif

    pMicroBobSetup(psp);
    pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP));
    if (!pp->InitingNuke)
        pSetState(psp, psp->PresentState);
}

void
pMicroStandBy(PANEL_SPRITEp psp)
{
    PLAYERp pp = psp->PlayerP;

    pMicroOverlays(psp);
    PlaySound(DIGI_NUKESTDBY, pp, v3df_follow|v3df_dontpan, CHAN_WEAPON);
}

void
pMicroCount(PANEL_SPRITEp psp)
{
    PLAYERp pp = psp->PlayerP;

    PlaySound(DIGI_NUKECDOWN, pp, v3df_follow|v3df_dontpan, CHAN_WEAPON);
}

void
pMicroReady(PANEL_SPRITEp psp)
{
    PLAYERp pp = psp->PlayerP;

    PlaySound(DIGI_NUKEREADY, pp, v3df_follow|v3df_dontpan, CHAN_WEAPON);
    pp->NukeInitialized = true;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// HEART
//
//////////////////////////////////////////////////////////////////////////////////////////

void pHeartPresent(PANEL_SPRITEp psp);
void pHeartRetract(PANEL_SPRITEp psp);
void pHeartAction(PANEL_SPRITEp psp);
void pHeartRest(PANEL_SPRITEp psp);
void pHeartAttack(PANEL_SPRITEp psp);
void pHeartHide(PANEL_SPRITEp psp);
void pHeartActionBlood(PANEL_SPRITEp psp);
void SpawnSmallHeartBlood(PANEL_SPRITEp psp);

extern PANEL_STATE ps_HeartAttack[];
extern PANEL_STATE ps_ReloadHeart[];


#define Heart_BEAT_RATE 60
#define Heart_ACTION_RATE 10

PANEL_STATE ps_PresentHeart[] =
{
    {ID_HeartPresent0, Heart_BEAT_RATE, pHeartPresent, &ps_PresentHeart[1], 0,0,0},
    {ID_HeartPresent1, Heart_BEAT_RATE, pHeartPresent, &ps_PresentHeart[0], 0,0,0}
};

PANEL_STATE ps_HeartRest[] =
{
    {ID_HeartPresent0, Heart_BEAT_RATE, pHeartRest, &ps_HeartRest[1], 0,0,0},
    {ID_HeartPresent1, Heart_BEAT_RATE, pHeartRest, &ps_HeartRest[2], 0,0,0},
    {ID_HeartPresent1, Heart_BEAT_RATE, SpawnSmallHeartBlood, &ps_HeartRest[3], psf_QuickCall, 0,0},
    {ID_HeartPresent1, 0,               pHeartRest, &ps_HeartRest[0], 0,0,0},
};

PANEL_STATE ps_HeartHide[] =
{
    {ID_HeartPresent0, Heart_BEAT_RATE, pHeartHide, &ps_HeartHide[1], 0,0,0},
    {ID_HeartPresent1, Heart_BEAT_RATE, pHeartHide, &ps_HeartHide[0], 0,0,0}
};

PANEL_STATE ps_HeartAttack[] =
{
    // squeeze
    {ID_HeartAttack0, Heart_ACTION_RATE, pHeartActionBlood, &ps_HeartAttack[1], psf_ShadeHalf, 0,0},
    {ID_HeartAttack1, Heart_ACTION_RATE, pHeartActionBlood, &ps_HeartAttack[2], psf_ShadeNone, 0,0},
    {ID_HeartAttack1, Heart_ACTION_RATE, pHeartActionBlood, &ps_HeartAttack[3], psf_ShadeNone, 0,0},
    // attack
    {ID_HeartAttack1, Heart_ACTION_RATE, pHeartAttack, &ps_HeartAttack[4], psf_QuickCall, 0,0},
    // unsqueeze
    {ID_HeartAttack1, Heart_ACTION_RATE, pHeartAction, &ps_HeartAttack[5], psf_ShadeNone, 0,0},
    {ID_HeartAttack1, Heart_ACTION_RATE, pHeartAction, &ps_HeartAttack[6], psf_ShadeNone, 0,0},
    {ID_HeartAttack0, Heart_ACTION_RATE, pHeartAction, &ps_HeartAttack[7], psf_ShadeHalf, 0,0},

    {ID_HeartAttack0, Heart_ACTION_RATE, pHeartAction, &ps_HeartRest[0], psf_ShadeHalf, 0,0},
};

PANEL_STATE ps_RetractHeart[] =
{
    {ID_HeartPresent0, Heart_BEAT_RATE, pHeartRetract, &ps_RetractHeart[1], 0,0,0},
    {ID_HeartPresent1, Heart_BEAT_RATE, pHeartRetract, &ps_RetractHeart[0], 0,0,0}
};

#define HEART_YOFF 212

void
InitWeaponHeart(PLAYERp pp)
{
    PANEL_SPRITEp psp;

    if (SW_SHAREWARE) return;

    if (Prediction)
        return;

    if (!TEST(pp->WpnFlags, BIT(WPN_HEART)) ||
//        pp->WpnAmmo[WPN_HEART] <= 0 ||
        TEST(pp->Flags, PF_WEAPON_RETRACT))
        return;

    if (!pp->Wpn[WPN_HEART])
    {
        psp = pp->Wpn[WPN_HEART] = pSpawnSprite(pp, ps_PresentHeart, PRI_MID, 160 + 10, HEART_YOFF);
        psp->y += tileHeight(psp->picndx);
        psp->backupy();
    }

    if (pp->CurWpn == pp->Wpn[WPN_HEART])
    {
        return;
    }

    PlayerUpdateWeapon(pp, WPN_HEART);

    pp->WpnUziType = 2; // Make uzi's go away!
    RetractCurWpn(pp);

    PlaySound(DIGI_HEARTBEAT, pp, v3df_follow|v3df_dontpan|v3df_doppler);

    // Set up the new Weapon variables
    psp = pp->CurWpn = pp->Wpn[WPN_HEART];
    SET(psp->flags, PANF_WEAPON_SPRITE);
    psp->ActionState = ps_HeartAttack;
    psp->RetractState = ps_RetractHeart;
    psp->PresentState = ps_PresentHeart;
    psp->RestState = ps_HeartRest;
    pSetState(psp, psp->PresentState);

    psp->PlayerP->KeyPressBits |= SB_FIRE;
}

void
pHeartPresent(PANEL_SPRITEp psp)
{
    if (TEST(psp->PlayerP->Flags, PF_WEAPON_RETRACT))
        return;

    psp->backupy();
    psp->y -= 3 * synctics;

    if (psp->y < HEART_YOFF)
    {
        psp->oy = psp->y = HEART_YOFF;
        psp->backupboby();
        pSetState(psp, psp->RestState);
    }
}

void
pHeartBobSetup(PANEL_SPRITEp psp)
{
    if (TEST(psp->flags, PANF_BOB))
        return;

    psp->backupbobcoords();

    psp->sin_amt = 12;
    psp->sin_ndx = 0;
    psp->bob_height_divider = 8;
}

void
pHeartHide(PANEL_SPRITEp psp)
{
    short picnum = psp->picndx;

    psp->backupy();
    psp->y += 3 * synctics;

    if (psp->y >= HEART_YOFF + tileHeight(picnum))
    {
        psp->oy = psp->y = HEART_YOFF + tileHeight(picnum);

        pWeaponUnHideKeys(psp, psp->PresentState);
    }
}

void
pHeartRest(PANEL_SPRITEp psp)
{
    bool force = !!TEST(psp->flags, PANF_UNHIDE_SHOOT);

    if (pWeaponHideKeys(psp, ps_HeartHide))
        return;

    psp->yorig += synctics;

    if (psp->yorig > HEART_YOFF)
    {
        psp->yorig = HEART_YOFF;
    }

    pHeartBobSetup(psp);
    pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP));

    if ((psp->PlayerP->input.actions & SB_FIRE) || force)
    {
        if ((psp->PlayerP->KeyPressBits & SB_FIRE) || force)
        {
            psp->PlayerP->KeyPressBits &= ~SB_FIRE;
            RESET(psp->flags, PANF_UNHIDE_SHOOT);

            if (!WeaponOK(psp->PlayerP))
                return;

            DoPlayerChooseYell(psp->PlayerP);
            pSetState(psp, psp->ActionState);
        }
    }
    else
    {
        psp->PlayerP->KeyPressBits |= SB_FIRE;
        WeaponOK(psp->PlayerP);
    }
}

void
pHeartAction(PANEL_SPRITEp psp)
{
    psp->yorig -= synctics;

    if (psp->yorig < 200)
    {
        psp->yorig = 200;
    }

    pHeartBobSetup(psp);
    pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP));
}

void
pHeartActionBlood(PANEL_SPRITEp psp)
{
    psp->yorig -= synctics;

    if (psp->yorig < 200)
    {
        psp->yorig = 200;
    }

    psp->backupy();
    psp->y = psp->yorig;

    pHeartBobSetup(psp);
    pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP));

    SpawnHeartBlood(psp);
}

void InitHeartAttack(PLAYERp pp);

void
pHeartAttack(PANEL_SPRITEp psp)
{
    PLAYERp pp = psp->PlayerP;
    // CTW MODIFICATION
    //int InitHeartAttack(PLAYERp pp);
    // CTW MODIFICATION END

    PlaySound(DIGI_HEARTFIRE, pp, v3df_follow|v3df_dontpan);
    if (RANDOM_RANGE(1000) > 800)
        PlayerSound(DIGI_JG9009, v3df_follow|v3df_dontpan,pp);
    InitHeartAttack(psp->PlayerP);
}

void
pHeartRetract(PANEL_SPRITEp psp)
{
    short picnum = psp->picndx;

    psp->backupy();
    psp->y += 3 * synctics;

    if (psp->y >= HEART_YOFF + tileHeight(picnum))
    {
        RESET(psp->PlayerP->Flags, PF_WEAPON_RETRACT);
        psp->PlayerP->Wpn[WPN_HEART] = nullptr;
        pKillSprite(psp);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// HEART BLOOD
//
//////////////////////////////////////////////////////////////////////////////////////////

void pHeartBlood(PANEL_SPRITEp);

#define HEART_BLOOD_RATE 10

PANEL_STATE ps_HeartBlood[] =
{
    {ID_HeartBlood0, HEART_BLOOD_RATE, pHeartBlood, &ps_HeartBlood[1], 0,0,0},
    {ID_HeartBlood1, HEART_BLOOD_RATE, pHeartBlood, &ps_HeartBlood[2], 0,0,0},
    {ID_HeartBlood2, HEART_BLOOD_RATE, pHeartBlood, &ps_HeartBlood[3], 0,0,0},
    {ID_HeartBlood3, HEART_BLOOD_RATE, pHeartBlood, &ps_HeartBlood[4], 0,0,0},
    {ID_HeartBlood4, HEART_BLOOD_RATE, pHeartBlood, &ps_HeartBlood[5], 0,0,0},
    {ID_HeartBlood5, HEART_BLOOD_RATE, pHeartBlood, &ps_HeartBlood[6], 0,0,0},
    {ID_HeartBlood5, HEART_BLOOD_RATE, pSuicide, &ps_HeartBlood[6], 0,0,0},
};

#define HEART_BLOOD_SMALL_RATE 7

PANEL_STATE ps_HeartBloodSmall[] =
{
    {ID_HeartBlood0, HEART_BLOOD_SMALL_RATE, pHeartBlood, &ps_HeartBlood[1], 0,0,0},
    {ID_HeartBlood1, HEART_BLOOD_SMALL_RATE, pHeartBlood, &ps_HeartBlood[2], 0,0,0},
    {ID_HeartBlood2, HEART_BLOOD_SMALL_RATE, pHeartBlood, &ps_HeartBlood[3], 0,0,0},
    {ID_HeartBlood3, HEART_BLOOD_SMALL_RATE, pHeartBlood, &ps_HeartBlood[4], 0,0,0},
    {ID_HeartBlood4, HEART_BLOOD_SMALL_RATE, pHeartBlood, &ps_HeartBlood[5], 0,0,0},
    {ID_HeartBlood5, HEART_BLOOD_SMALL_RATE, pHeartBlood, &ps_HeartBlood[6], 0,0,0},
    {ID_HeartBlood5, HEART_BLOOD_SMALL_RATE, pSuicide, &ps_HeartBlood[6], 0,0,0},
};

void
SpawnHeartBlood(PANEL_SPRITEp psp)
{
    PLAYERp pp = psp->PlayerP;
    PANEL_SPRITEp blood;
    PANEL_SHRAPp hsp;

    static PANEL_SHRAP HeartShrap[] =
    {
        {-10, -80, 2, -FIXED(1,32000), -FIXED(2,32000), -FIXED(5,32000), -FIXED(3,32000), {ps_HeartBlood, ps_HeartBlood}},
        {0, -85, 0, -FIXED(3,32000), -FIXED(8,32000), -FIXED(3,32000), -FIXED(1,32000), {ps_HeartBlood, ps_HeartBlood}},
        {10, -85, 2, -FIXED(1,32000), -FIXED(2,32000), FIXED(2,32000), FIXED(3,32000), {ps_HeartBlood, ps_HeartBlood}},
        {25, -80, 2, -FIXED(1,32000), -FIXED(2,32000), FIXED(5,32000), FIXED(6,32000), {ps_HeartBlood, ps_HeartBlood}},
        {0, 0, 0, 0, 0, 0, 0, {0, 0}},
    };

    for (hsp = HeartShrap; hsp->lo_jump_speed; hsp++)
    {
        if (hsp->skip == 2)
        {
            if (MoveSkip2 != 0)
                continue;
        }

        // RIGHT side
        blood = pSpawnSprite(pp, hsp->state[RANDOM_P2(2<<8)>>8], PRI_BACK, 0, 0);
        blood->x = psp->x + hsp->xoff;
        blood->ox = blood->x;
        blood->y = psp->y + hsp->yoff;
        blood->oy = blood->y;
        blood->xspeed = hsp->lo_xspeed + (RANDOM_RANGE((hsp->hi_xspeed - hsp->lo_xspeed)>>4) << 4);
        SET(blood->flags, PANF_WEAPON_SPRITE);

        blood->scale = 20000 + RANDOM_RANGE(50000 - 20000);

        blood->jump_speed = hsp->lo_jump_speed + (RANDOM_RANGE((hsp->hi_jump_speed + hsp->lo_jump_speed)>>4) << 4);
        DoBeginPanelJump(blood);
    }
}

void
SpawnSmallHeartBlood(PANEL_SPRITEp psp)
{
    PLAYERp pp = psp->PlayerP;
    PANEL_SPRITEp blood;
    PANEL_SHRAPp hsp;

    static PANEL_SHRAP HeartShrap[] =
    {
        {-10, -80, 0, -FIXED(1,0), -FIXED(2,0), -FIXED(1,0), -FIXED(3,0), {ps_HeartBloodSmall, ps_HeartBloodSmall}},
        {0, -85, 0, -FIXED(1,0), -FIXED(5,0), -FIXED(1,0), -FIXED(1,0), {ps_HeartBloodSmall, ps_HeartBloodSmall}},
        {10, -85, 0, -FIXED(1,0), -FIXED(2,0), FIXED(1,0), FIXED(2,0), {ps_HeartBloodSmall, ps_HeartBloodSmall}},
        {25, -80, 0, -FIXED(1,0), -FIXED(2,0), FIXED(3,0), FIXED(4,0), {ps_HeartBloodSmall, ps_HeartBloodSmall}},
        {0, 0, 0, 0, 0, 0, 0, {0,0}},
    };

    PlaySound(DIGI_HEARTBEAT, pp, v3df_follow|v3df_dontpan|v3df_doppler);

    for (hsp = HeartShrap; hsp->lo_jump_speed; hsp++)
    {
        // RIGHT side
        blood = pSpawnSprite(pp, hsp->state[RANDOM_P2(2<<8)>>8], PRI_BACK, 0, 0);
        blood->x = psp->x + hsp->xoff;
        blood->ox = blood->x;
        blood->y = psp->y + hsp->yoff;
        blood->oy = blood->y;
        blood->xspeed = hsp->lo_xspeed + (RANDOM_RANGE((hsp->hi_xspeed - hsp->lo_xspeed)>>4) << 4);
        SET(blood->flags, PANF_WEAPON_SPRITE);

        blood->scale = 10000 + RANDOM_RANGE(30000 - 10000);

        blood->jump_speed = hsp->lo_jump_speed + (RANDOM_RANGE((hsp->hi_jump_speed + hsp->lo_jump_speed)>>4) << 4);
        DoBeginPanelJump(blood);
    }
}

void
pHeartBlood(PANEL_SPRITEp psp)
{
    if (TEST(psp->flags, PANF_JUMPING))
    {
        DoPanelJump(psp);
    }
    else if (TEST(psp->flags, PANF_FALLING))
    {
        DoPanelFall(psp);
    }

    psp->backupx();
    psp->x += psp->xspeed * (1. / FRACUNIT);

    if (psp->x > 320 || psp->x < 0 || psp->y > 200)
    {
        pKillSprite(psp);
        return;
    }
}

int
DoBeginPanelJump(PANEL_SPRITEp psp)
{
#define PANEL_JUMP_GRAVITY FIXED(0,8000)

    SET(psp->flags, PANF_JUMPING);
    RESET(psp->flags, PANF_FALLING);

    // set up individual actor jump gravity
    psp->jump_grav = PANEL_JUMP_GRAVITY;

    DoPanelJump(psp);

    return 0;
}

int
DoPanelJump(PANEL_SPRITEp psp)
{
    // adjust jump speed by gravity - if jump speed greater than 0 player
    // have started falling
    if ((psp->jump_speed += psp->jump_grav) > 0)
    {
        // Start falling
        DoBeginPanelFall(psp);
        return 0;
    }

    // adjust height by jump speed
    psp->backupy();
    psp->y += psp->jump_speed * synctics * (1. / FRACUNIT);

    return 0;
}


int
DoBeginPanelFall(PANEL_SPRITEp psp)
{
    SET(psp->flags, PANF_FALLING);
    RESET(psp->flags, PANF_JUMPING);

    psp->jump_grav = PANEL_JUMP_GRAVITY;

    DoPanelFall(psp);

    return 0;
}


int
DoPanelFall(PANEL_SPRITEp psp)
{
    // adjust jump speed by gravity
    psp->jump_speed += psp->jump_grav;

    // adjust player height by jump speed
    psp->backupy();
    psp->y += psp->jump_speed * synctics * (1. / FRACUNIT);

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// GRENADE
//
//////////////////////////////////////////////////////////////////////////////////////////

void pGrenadePresent(PANEL_SPRITEp psp);
void pGrenadeRetract(PANEL_SPRITEp psp);
void pGrenadeAction(PANEL_SPRITEp psp);
void pGrenadeRest(PANEL_SPRITEp psp);
void pGrenadeFire(PANEL_SPRITEp psp);
void pGrenadeHide(PANEL_SPRITEp psp);
void pGrenadeUnHide(PANEL_SPRITEp psp);
void pGrenadeRecoilDown(PANEL_SPRITEp psp);
void pGrenadeRecoilUp(PANEL_SPRITEp psp);
void pGrenadeBobSetup(PANEL_SPRITEp psp);

extern PANEL_STATE ps_GrenadeRecoil[];

#define Grenade_REST_RATE 24
#define Grenade_ACTION_RATE 6

PANEL_STATE ps_PresentGrenade[] =
{
    {ID_GrenadePresent0, Grenade_REST_RATE, pGrenadePresent, &ps_PresentGrenade[0], 0,0,0}
};

PANEL_STATE ps_GrenadeRest[] =
{
    {ID_GrenadePresent0, Grenade_REST_RATE, pGrenadeRest, &ps_GrenadeRest[0], 0,0,0}
};

PANEL_STATE ps_GrenadeHide[] =
{
    {ID_GrenadePresent0, Grenade_REST_RATE, pGrenadeHide, &ps_GrenadeHide[0], 0,0,0}
};

PANEL_STATE ps_GrenadeFire[] =
{
    {ID_GrenadeFire0, Grenade_ACTION_RATE, pGrenadeAction, &ps_GrenadeFire[1], psf_ShadeHalf, 0,0},
    {ID_GrenadeFire1, Grenade_ACTION_RATE, pGrenadeAction, &ps_GrenadeFire[2], psf_ShadeNone, 0,0},
    {ID_GrenadeFire2, Grenade_ACTION_RATE, pGrenadeAction, &ps_GrenadeFire[3], psf_ShadeNone, 0,0},

    {ID_GrenadePresent0, 0, pGrenadeFire, &ps_GrenadeFire[4], psf_QuickCall, 0,0},
    {ID_GrenadePresent0, 3, pGrenadeAction, &ps_GrenadeRecoil[0], 0,0,0}
};

PANEL_STATE ps_GrenadeRecoil[] =
{
    // recoil
    {ID_GrenadePresent0, Grenade_REST_RATE, pGrenadeRecoilDown, &ps_GrenadeRecoil[0], 0,0,0},
    {ID_GrenadePresent0, Grenade_REST_RATE, pGrenadeRecoilUp, &ps_GrenadeRecoil[1], 0,0,0},
    // reload
    {ID_GrenadeReload0, Grenade_REST_RATE/2, pNullAnimator, &ps_GrenadeRecoil[3], 0,0,0},
    {ID_GrenadeReload1, Grenade_REST_RATE/2, pNullAnimator, &ps_GrenadeRecoil[4], 0,0,0},
    // ready to fire again
    {ID_GrenadePresent0, 3, pNullAnimator, &ps_GrenadeRest[0], 0,0,0}
};

PANEL_STATE ps_RetractGrenade[] =
{
    {ID_GrenadePresent0, Grenade_REST_RATE, pGrenadeRetract, &ps_RetractGrenade[0], 0,0,0}
};

#define GRENADE_YOFF 200
#define GRENADE_XOFF (160+20)

void
pGrenadeSetRecoil(PANEL_SPRITEp psp)
{
    psp->vel = 900;
    psp->ang = NORM_ANGLE(-256);
}

void
pGrenadePresentSetup(PANEL_SPRITEp psp)
{
    psp->rotate_ang = 1800;
    psp->backupcoords();
    psp->y += 34;
    psp->x -= 45;
    psp->ang = 256 + 128;
    psp->vel = 680;
}

void
InitWeaponGrenade(PLAYERp pp)
{
    PANEL_SPRITEp psp;

    if (Prediction)
        return;

    if (!TEST(pp->WpnFlags, BIT(WPN_GRENADE)) ||
//        pp->WpnAmmo[WPN_GRENADE] <= 0 ||
        TEST(pp->Flags, PF_WEAPON_RETRACT))
        return;

    if (!pp->Wpn[WPN_GRENADE])
    {
        psp = pp->Wpn[WPN_GRENADE] = pSpawnSprite(pp, ps_PresentGrenade, PRI_MID, GRENADE_XOFF, GRENADE_YOFF);
        psp->y += tileHeight(psp->picndx);
        psp->backupy();
    }

    if (pp->CurWpn == pp->Wpn[WPN_GRENADE])
    {
        return;
    }

    PlayerUpdateWeapon(pp, WPN_GRENADE);

    pp->WpnUziType = 2; // Make uzi's go away!
    RetractCurWpn(pp);

    // Set up the new Weapon variables
    psp = pp->CurWpn = pp->Wpn[WPN_GRENADE];
    psp = pp->CurWpn = pp->Wpn[WPN_GRENADE];
    SET(psp->flags, PANF_WEAPON_SPRITE);
    psp->ActionState = ps_GrenadeFire;
    psp->RetractState = ps_RetractGrenade;
    psp->PresentState = ps_PresentGrenade;
    psp->RestState = ps_GrenadeRest;
    pSetState(psp, psp->PresentState);

    pGrenadePresentSetup(psp);

    PlaySound(DIGI_GRENADE_UP, pp, v3df_follow);

    psp->PlayerP->KeyPressBits |= SB_FIRE;
}

void
pGrenadeRecoilDown(PANEL_SPRITEp psp)
{
//    short picnum = psp->picndx;

    psp->backupcoords();

    psp->x += pspCosVel(psp);
    psp->y -= pspSinVel(psp);

    psp->vel -= 24 * synctics;

    // if (psp->y >= GRENADE_YOFF + tileHeight(picnum))
    if (psp->vel < 400)
    {
        // psp->ox = psp->y = GRENADE_YOFF + tileHeight(picnum);

        psp->vel = 400;
        psp->ang = NORM_ANGLE(psp->ang + 1024);

        pStatePlusOne(psp);
    }
}

void
pGrenadeRecoilUp(PANEL_SPRITEp psp)
{
    psp->backupcoords();

    psp->x += pspCosVel(psp);
    psp->y -= pspSinVel(psp);

    psp->vel += 15 * synctics;

    if (psp->y < GRENADE_YOFF)
    {
        psp->oy = psp->y = GRENADE_YOFF;
        psp->ox = psp->x = GRENADE_XOFF;

        pGrenadeSetRecoil(psp);

        pStatePlusOne(psp);
        RESET(psp->flags, PANF_BOB);
    }
}

void
pGrenadePresent(PANEL_SPRITEp psp)
{
    if (TEST(psp->PlayerP->Flags, PF_WEAPON_RETRACT))
        return;

    psp->backupcoords();

    psp->x += pspCosVel(psp);
    psp->y -= pspSinVel(psp);

    psp->rotate_ang = NORM_ANGLE(psp->rotate_ang + (6 * synctics));

    if (psp->rotate_ang < 1024)
        psp->rotate_ang = 0;

    if (psp->y < GRENADE_YOFF)
    {
        pGrenadeSetRecoil(psp);
        psp->ox = psp->x = GRENADE_XOFF;
        psp->oy = psp->y = GRENADE_YOFF;
        psp->rotate_ang = 0;
        psp->backupboby();
        pSetState(psp, psp->RestState);
    }
}

void
pGrenadeBobSetup(PANEL_SPRITEp psp)
{
    if (TEST(psp->flags, PANF_BOB))
        return;

    psp->backupbobcoords();

    psp->sin_amt = 12;
    psp->sin_ndx = 0;
    psp->bob_height_divider = 8;
}

void
pGrenadeHide(PANEL_SPRITEp psp)
{
    short picnum = psp->picndx;

    psp->backupy();
    psp->y += 3 * synctics;

    if (psp->y >= GRENADE_YOFF + tileHeight(picnum))
    {
        psp->oy = psp->y = GRENADE_YOFF + tileHeight(picnum);
        psp->ox = psp->x = GRENADE_XOFF;

        pGrenadePresentSetup(psp);

        pWeaponUnHideKeys(psp, psp->PresentState);
    }
}

void
pGrenadeRest(PANEL_SPRITEp psp)
{
    bool force = !!TEST(psp->flags, PANF_UNHIDE_SHOOT);

    if (pWeaponHideKeys(psp, ps_GrenadeHide))
        return;

    pGrenadeBobSetup(psp);
    pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP));

    if ((psp->PlayerP->input.actions & SB_FIRE) || force)
    {
        if ((psp->PlayerP->KeyPressBits & SB_FIRE) || force)
        {
            RESET(psp->flags, PANF_UNHIDE_SHOOT);

            if (!WeaponOK(psp->PlayerP))
                return;

            DoPlayerChooseYell(psp->PlayerP);
            pSetState(psp, psp->ActionState);
        }
    }
    else
        WeaponOK(psp->PlayerP);
}

void
pGrenadeAction(PANEL_SPRITEp psp)
{
    pGrenadeBobSetup(psp);
    pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP));
}


void
pGrenadeFire(PANEL_SPRITEp psp)
{
    SpawnVis(psp->PlayerP->PlayerSprite, -1, -1, -1, -1, 32);
    InitGrenade(psp->PlayerP);
}

void
pGrenadeRetract(PANEL_SPRITEp psp)
{
    short picnum = psp->picndx;

    psp->backupy();
    psp->y += 3 * synctics;

    if (psp->y >= GRENADE_YOFF + tileHeight(picnum))
    {
        RESET(psp->PlayerP->Flags, PF_WEAPON_RETRACT);
        psp->PlayerP->Wpn[WPN_GRENADE] = nullptr;
        pKillSprite(psp);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// MINE
//
//////////////////////////////////////////////////////////////////////////////////////////

void pMinePresent(PANEL_SPRITEp psp);
void pMineRetract(PANEL_SPRITEp psp);
void pMineAction(PANEL_SPRITEp psp);
void pMineRest(PANEL_SPRITEp psp);
void pMineThrow(PANEL_SPRITEp psp);
void pMineLower(PANEL_SPRITEp psp);
void pMineRaise(PANEL_SPRITEp psp);
void pMineHide(PANEL_SPRITEp psp);
void pMineUnHide(PANEL_SPRITEp psp);
void pMineBobSetup(PANEL_SPRITEp psp);
void pMineUpSound(PANEL_SPRITEp psp);

#define Mine_REST_RATE 24
#define Mine_ACTION_RATE 6

PANEL_STATE ps_PresentMine[] =
{
    {ID_MinePresent0, Mine_REST_RATE, pMinePresent, &ps_PresentMine[0], 0,0,0}
};

PANEL_STATE ps_MineRest[] =
{
    {ID_MinePresent0, 36,               pMineRest,      &ps_MineRest[1], 0,0,0},
    {ID_MinePresent0, 0,                pMineUpSound,   &ps_MineRest[2], psf_QuickCall, 0,0},
    {ID_MinePresent1, Mine_REST_RATE,   pMineRest,      &ps_MineRest[2], 0,0,0},
};

PANEL_STATE ps_MineHide[] =
{
    {ID_MinePresent0, Mine_REST_RATE, pMineHide, &ps_MineHide[0], 0,0,0}
};

PANEL_STATE ps_MineThrow[] =
{
    {ID_MineThrow0,   3,                pNullAnimator,  &ps_MineThrow[1], 0,0,0},
    {ID_MineThrow0, Mine_ACTION_RATE, pMineThrow,       &ps_MineThrow[2],psf_QuickCall, 0,0},
    {ID_MineThrow0, Mine_ACTION_RATE, pMineLower,       &ps_MineThrow[2], 0,0,0},
    {ID_MineThrow0, Mine_ACTION_RATE*5, pNullAnimator,  &ps_MineThrow[4], 0,0,0},
    {ID_MinePresent0, Mine_ACTION_RATE, pMineRaise,     &ps_MineThrow[4], 0,0,0},
    {ID_MinePresent0, Mine_ACTION_RATE, pNullAnimator,  &ps_MineThrow[6], 0,0,0},
    {ID_MinePresent0, 3, pMineAction, &ps_MineRest[0], 0,0,0}
};

PANEL_STATE ps_RetractMine[] =
{
    {ID_MinePresent0, Mine_REST_RATE, pMineRetract, &ps_RetractMine[0], 0,0,0}
};

#define MINE_YOFF 200
//#define MINE_XOFF (160+20)
#define MINE_XOFF (160+50)

void
InitWeaponMine(PLAYERp pp)
{
    PANEL_SPRITEp psp;

    if (Prediction)
        return;

    if (pp->WpnAmmo[WPN_MINE] <= 0)
        PutStringInfo(pp,"Out of Sticky Bombs!");

    if (!TEST(pp->WpnFlags, BIT(WPN_MINE)) ||
        pp->WpnAmmo[WPN_MINE] <= 0 ||
        TEST(pp->Flags, PF_WEAPON_RETRACT))
        return;

    if (!pp->Wpn[WPN_MINE])
    {
        psp = pp->Wpn[WPN_MINE] = pSpawnSprite(pp, ps_PresentMine, PRI_MID, MINE_XOFF, MINE_YOFF);
        psp->y += tileHeight(psp->picndx);
        psp->backupy();
    }

    if (pp->CurWpn == pp->Wpn[WPN_MINE])
    {
        return;
    }

    PlayerUpdateWeapon(pp, WPN_MINE);

    pp->WpnUziType = 2; // Make uzi's go away!
    RetractCurWpn(pp);

    // Set up the new Weapon variables
    psp = pp->CurWpn = pp->Wpn[WPN_MINE];
    SET(psp->flags, PANF_WEAPON_SPRITE);
    psp->ActionState = ps_MineThrow;
    psp->RetractState = ps_RetractMine;
    psp->PresentState = ps_PresentMine;
    psp->RestState = ps_MineRest;
    pSetState(psp, psp->PresentState);

    PlaySound(DIGI_PULL, pp, v3df_follow|v3df_dontpan);

    psp->PlayerP->KeyPressBits |= SB_FIRE;
}

void
pMineUpSound(PANEL_SPRITEp psp)
{
    PLAYERp pp = psp->PlayerP;

    PlaySound(DIGI_MINE_UP, pp, v3df_follow);
}

void
pMineLower(PANEL_SPRITEp psp)
{
    psp->backupy();
    psp->y += 4 * synctics;

    if (psp->y > MINE_YOFF + tileHeight(psp->picndx))
    {
        if (!WeaponOK(psp->PlayerP))
            return;
        psp->oy = psp->y = MINE_YOFF + tileHeight(psp->picndx);
        pStatePlusOne(psp);
    }
}

void
pMineRaise(PANEL_SPRITEp psp)
{
    psp->backupy();
    psp->y -= 4 * synctics;

    if (psp->y < MINE_YOFF)
    {
        psp->oy = psp->y = MINE_YOFF;
        pStatePlusOne(psp);
    }
}

void
pMinePresent(PANEL_SPRITEp psp)
{
    if (TEST(psp->PlayerP->Flags, PF_WEAPON_RETRACT))
        return;

    psp->backupy();
    psp->y -= 3 * synctics;

    if (psp->y < MINE_YOFF)
    {
        psp->oy = psp->y = MINE_YOFF;
        psp->rotate_ang = 0;
        psp->backupboby();
        pSetState(psp, psp->RestState);
    }
}

void
pMineBobSetup(PANEL_SPRITEp psp)
{
    if (TEST(psp->flags, PANF_BOB))
        return;

    psp->backupbobcoords();

    psp->sin_amt = 12;
    psp->sin_ndx = 0;
    psp->bob_height_divider = 8;
}

void
pMineHide(PANEL_SPRITEp psp)
{
    short picnum = psp->picndx;

    psp->backupy();
    psp->y += 3 * synctics;

    if (psp->y >= MINE_YOFF + tileHeight(picnum))
    {
        psp->oy = psp->y = MINE_YOFF + tileHeight(picnum);
        psp->ox = psp->x = MINE_XOFF;

        pWeaponUnHideKeys(psp, psp->PresentState);
    }
}

void
pMineRest(PANEL_SPRITEp psp)
{
    bool force = !!TEST(psp->flags, PANF_UNHIDE_SHOOT);

    if (pWeaponHideKeys(psp, ps_MineHide))
        return;

    pMineBobSetup(psp);
    pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP));

    if ((psp->PlayerP->input.actions & SB_FIRE) || force)
    {
        if ((psp->PlayerP->KeyPressBits & SB_FIRE) || force)
        {
            RESET(psp->flags, PANF_UNHIDE_SHOOT);

//            if (!WeaponOK(psp->PlayerP))
//                return;

            DoPlayerChooseYell(psp->PlayerP);
            pSetState(psp, psp->ActionState);
        }
    }
    else
        WeaponOK(psp->PlayerP);
}

void
pMineAction(PANEL_SPRITEp psp)
{
    pMineBobSetup(psp);
    pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP));
}


void
pMineThrow(PANEL_SPRITEp psp)
{
    InitMine(psp->PlayerP);
}

void
pMineRetract(PANEL_SPRITEp psp)
{
    short picnum = psp->picndx;

    psp->backupy();
    psp->y += 3 * synctics;

    if (psp->y >= MINE_YOFF + tileHeight(picnum))
    {
        RESET(psp->PlayerP->Flags, PF_WEAPON_RETRACT);
        psp->PlayerP->Wpn[WPN_MINE] = nullptr;
        pKillSprite(psp);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// CHOP STICKS
//
//////////////////////////////////////////////////////////////////////////////////////////

void pChopsUp(PANEL_SPRITEp psp);
void pChopsDown(PANEL_SPRITEp psp);
void pChopsDownSlow(PANEL_SPRITEp psp);
void pChopsWait(PANEL_SPRITEp psp);
void pChopsShake(PANEL_SPRITEp psp);
void pChopsClick(PANEL_SPRITEp psp);

void pChopsRetract(PANEL_SPRITEp psp);

#define Chops_REST_RATE 24
#define Chops_ACTION_RATE 6

#define ID_ChopsRest  2000
#define ID_ChopsOpen  2001
#define ID_ChopsClose 2002

PANEL_STATE ps_ChopsAttack1[] =
{
    {ID_ChopsRest,  Chops_REST_RATE*3,  pNullAnimator,  &ps_ChopsAttack1[1], 0,0,0},
    {ID_ChopsRest,  Chops_REST_RATE,    pChopsUp,       &ps_ChopsAttack1[1], 0,0,0},
    {ID_ChopsOpen,  Chops_REST_RATE/3,  pNullAnimator,  &ps_ChopsAttack1[3], 0,0,0},
    {ID_ChopsClose, 0,                  pChopsClick,    &ps_ChopsAttack1[4], psf_QuickCall, 0,0},
    {ID_ChopsClose, Chops_REST_RATE/3,  pNullAnimator,  &ps_ChopsAttack1[5], 0,0,0},
    {ID_ChopsClose, Chops_REST_RATE,    pChopsDown,     &ps_ChopsAttack1[5], 0,0,0},
};

PANEL_STATE ps_ChopsAttack2[] =
{
    {ID_ChopsOpen,  Chops_REST_RATE*3,  pNullAnimator,  &ps_ChopsAttack2[1], 0,0,0},
    {ID_ChopsOpen,  Chops_REST_RATE,    pChopsUp,       &ps_ChopsAttack2[1], 0,0,0},
    {ID_ChopsOpen,  0,                  pChopsClick,    &ps_ChopsAttack2[3], psf_QuickCall, 0,0},
    {ID_ChopsOpen,  8,                  pNullAnimator,  &ps_ChopsAttack2[4], 0,0,0},
    {ID_ChopsRest,  Chops_REST_RATE,    pNullAnimator,  &ps_ChopsAttack2[5], 0,0,0},
    {ID_ChopsRest,  Chops_REST_RATE,    pChopsDown,     &ps_ChopsAttack2[5], 0,0,0},
};

PANEL_STATE ps_ChopsAttack3[] =
{
    {ID_ChopsOpen,  Chops_REST_RATE*3,  pNullAnimator,  &ps_ChopsAttack3[1], 0,0,0},
    {ID_ChopsOpen,  Chops_REST_RATE,    pChopsUp,       &ps_ChopsAttack3[1], 0,0,0},

    {ID_ChopsRest,  0,                  pNullAnimator,  &ps_ChopsAttack3[3], 0,0,0},
    {ID_ChopsRest,  0,                  pChopsClick,    &ps_ChopsAttack3[4], psf_QuickCall, 0,0},

    {ID_ChopsRest,  Chops_REST_RATE,    pNullAnimator,  &ps_ChopsAttack3[5], 0,0,0},
    {ID_ChopsRest,  24,                 pNullAnimator,  &ps_ChopsAttack3[6], 0,0,0},
    {ID_ChopsOpen,  16,                 pNullAnimator,  &ps_ChopsAttack3[7], 0,0,0},

    {ID_ChopsRest,  0,                  pChopsClick,    &ps_ChopsAttack3[8], psf_QuickCall, 0,0},
    {ID_ChopsRest,  16,                 pNullAnimator,  &ps_ChopsAttack3[9], 0,0,0},
    {ID_ChopsOpen,  16,                 pNullAnimator,  &ps_ChopsAttack3[10], 0,0,0},

    {ID_ChopsOpen,  8,                  pChopsDownSlow, &ps_ChopsAttack3[11], 0,0,0},
    {ID_ChopsRest,  10,                 pChopsDownSlow, &ps_ChopsAttack3[12], 0,0,0},
    {ID_ChopsRest,  0,                  pChopsClick,    &ps_ChopsAttack3[13], psf_QuickCall, 0,0},
    {ID_ChopsRest,  10,                 pChopsDownSlow, &ps_ChopsAttack3[14], 0,0,0},
    {ID_ChopsOpen,  10,                 pChopsDownSlow, &ps_ChopsAttack3[11], 0,0,0},
};

PANEL_STATE ps_ChopsAttack4[] =
{
    {ID_ChopsOpen,  Chops_REST_RATE*3,  pNullAnimator,  &ps_ChopsAttack4[1], 0,0,0},
    {ID_ChopsOpen,  Chops_REST_RATE,    pChopsUp,       &ps_ChopsAttack4[1], 0,0,0},

    {ID_ChopsOpen,  0,                  pChopsClick,    &ps_ChopsAttack4[3], psf_QuickCall, 0,0},
    {ID_ChopsOpen,  8,                  pNullAnimator,  &ps_ChopsAttack4[4], 0,0,0},

    {ID_ChopsRest,  Chops_REST_RATE,    pNullAnimator,  &ps_ChopsAttack4[5], 0,0,0},
    {ID_ChopsRest,  Chops_REST_RATE*4,  pChopsShake,    &ps_ChopsAttack4[6], 0,0,0},
    {ID_ChopsRest,  Chops_REST_RATE,    pChopsDown,     &ps_ChopsAttack4[6], 0,0,0},
};

PANEL_STATEp psp_ChopsAttack[] = {ps_ChopsAttack1, ps_ChopsAttack2, ps_ChopsAttack3, ps_ChopsAttack4};

PANEL_STATE ps_ChopsWait[] =
{
    {ID_ChopsRest,  Chops_REST_RATE, pChopsWait,     &ps_ChopsWait[0], 0,0,0},
};

PANEL_STATE ps_ChopsRetract[] =
{
    {ID_ChopsRest, Chops_REST_RATE, pChopsRetract, &ps_ChopsRetract[0], 0,0,0}
};

#define CHOPS_YOFF 200
#define CHOPS_XOFF (160+20)

void
InitChops(PLAYERp pp)
{
    PANEL_SPRITEp psp;

    if (!pp->Chops)
    {
        psp = pp->Chops = pSpawnSprite(pp, ps_ChopsAttack1, PRI_MID, CHOPS_XOFF, CHOPS_YOFF);
        psp->y += tileHeight(psp->picndx);
        psp->backupy();
    }

    if (Prediction)
        return;

    // Set up the new Weapon variables
    psp = pp->Chops;

    SET(psp->flags, PANF_WEAPON_SPRITE);
    psp->ActionState = ps_ChopsAttack1;
    psp->PresentState = ps_ChopsAttack1;
    psp->RetractState = ps_ChopsRetract;
    psp->RestState = ps_ChopsAttack1;

    PlaySound(DIGI_BUZZZ, psp->PlayerP,v3df_none);

    if (RANDOM_RANGE(1000) > 750)
        PlayerSound(DIGI_MRFLY,v3df_follow|v3df_dontpan,psp->PlayerP);

}

void
pChopsClick(PANEL_SPRITEp psp)
{
    int16_t rnd_rng;
    PlaySound(DIGI_CHOP_CLICK,psp->PlayerP,v3df_none);

    rnd_rng = RANDOM_RANGE(1000);
    if (rnd_rng > 950)
        PlayerSound(DIGI_SEARCHWALL,v3df_follow|v3df_dontpan,psp->PlayerP);
    else if (rnd_rng > 900)
        PlayerSound(DIGI_EVADEFOREVER,v3df_follow|v3df_dontpan,psp->PlayerP);
    else if (rnd_rng > 800)
        PlayerSound(DIGI_SHISEISI,v3df_follow|v3df_dontpan,psp->PlayerP);
}

void
pChopsUp(PANEL_SPRITEp psp)
{
    psp->backupy();
    psp->y -= 3 * synctics;

    if (psp->y < CHOPS_YOFF)
    {
        psp->oy = psp->y = CHOPS_YOFF;
        pStatePlusOne(psp);
    }
}

void
pChopsDown(PANEL_SPRITEp psp)
{
    psp->backupy();
    psp->y += 3 * synctics;

    if (psp->y > CHOPS_YOFF+110)
    {
        psp->oy = psp->y = CHOPS_YOFF+110;
        pSetState(psp, ps_ChopsWait);
    }
}

void
pChopsDownSlow(PANEL_SPRITEp psp)
{
    psp->backupy();
    psp->y += 1 * synctics;

    if (psp->y > CHOPS_YOFF+110)
    {
        psp->oy = psp->y = CHOPS_YOFF+110;
        pSetState(psp, ps_ChopsWait);
    }
}

void
pChopsShake(PANEL_SPRITEp psp)
{
    psp->backupcoords();

    psp->x += (RANDOM_P2(4<<8) * (1. / 256.)) - 2;
    psp->y += (RANDOM_P2(4<<8) * (1. / 256.)) - 2;

    if (psp->y < CHOPS_YOFF)
    {
        psp->oy = psp->y = CHOPS_YOFF;
    }
}

void
pChopsWait(PANEL_SPRITEp psp)
{
    //if (!paused && RANDOM_P2(1024) < 10)
    if (RANDOM_P2(1024) < 10)
    {
        // random x position
        // do a random attack here
        psp->ox = psp->x = CHOPS_XOFF + (RANDOM_P2(128) - 64);

        PlaySound(DIGI_BUZZZ,psp->PlayerP,v3df_none);
        pSetState(psp, psp_ChopsAttack[RANDOM_RANGE(SIZ(psp_ChopsAttack))]);
    }
}

void ChopsSetRetract(PLAYERp pp)
 {
    if (pp == nullptr || pp->Chops == nullptr)
        return;

    pSetState(pp->Chops, pp->Chops->RetractState);
}

void
pChopsRetract(PANEL_SPRITEp psp)
{
    short picnum = psp->picndx;

    psp->backupy();
    psp->y += 6 * synctics;

    if (psp->y >= CHOPS_YOFF + tileHeight(picnum))
    {
        if (RANDOM_RANGE(1000) > 800)
            PlayerSound(DIGI_GETTINGSTIFF,v3df_follow|v3df_dontpan,psp->PlayerP);
        psp->PlayerP->Chops = nullptr;
        pKillSprite(psp);
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
//
// FIST
//
// KungFu Move Numbers: (Used to calculate damages, etc....
// 0: Normal Punch
// 1: Palm Heel Strike
// 2: Kick
// 3: Block
// 4: FISTS OF FURY - (Flury of fast punches) DUCK, then BLOCK and ATTACK
// 5: CHI STRIKE - (Devastating Palm Heel)    DUCK 2 secs, then ATTACK
// 6: DEATH TOUCH - (Make enemy bleed and does alot of damage) JUMP, DUCK, BLOCK, ATTACK
// 7: FIREBALL - BLOCK 4 secs, then ATTACK
// 8: HAMMER KICK - Run forward while holding BLOCK, JUMP, ATTACK
// 9: CYCLONE KICK - (Spin 5 circles with leg out to hit multiples)
//       180 KEY, BLOCK, JUMP, ATTACK
// 10: DESPERATION - Finish Move (Cling to enemy like sticky bomb and beat the crap out of him)
//       Player is invincible until he unattaches
//       LOW HEALTH < 20, TOUCH ENEMY, BLOCK then ATTACK
// 11: SUPER NOVA - Become a human nuclear bomb!
//       SUPER SECRET MOVE
//       >150 Health + >50 Armor + Shadow Spell ACTIVE, DUCK + BLOCK 3 secs, JUMP, ATTACK
//       Player will die, so it's only useful if you get more than 1 frag in the deal.
// 12: TOTAL INVISIBILITY - Worse than shadow spell, you go 100% invisible for 10 secs!
//       SUPER SECRET MOVE
//       >180 Health + Shadow Spell ACTIVE, BLOCK 4 secs, JUMP, ATTACK
// 13: FREEZE - Turn all enemies in your view to ice for 10 secs
//       SUPER SECRET MOVE
//       100 Armor + Shadow Spell ACTIVE, BLOCK 5 secs, DUCK, ATTACK
//
//////////////////////////////////////////////////////////////////////////////////////////

static short FistAngTable[] =
{
    82,
    168,
    256+64
};

short FistAng = 0;

void
FistBlur(PANEL_SPRITEp psp)
{
    psp->kill_tics -= synctics;
    if (psp->kill_tics <= 0)
    {
        pKillSprite(psp);
        return;
    }
    else if (psp->kill_tics <= 6)
    {
        SET(psp->flags, PANF_TRANS_FLIP);
    }

    psp->shade += 10;
    // change sprites priority
    REMOVE(psp);
    psp->priority--;
    InsertPanelSprite(psp->PlayerP, psp);
}

void
SpawnFistBlur(PANEL_SPRITEp psp)
{
    PANEL_SPRITEp nsp;
    //PICITEMp pip;

    if (psp->PlayerP->FistAng > 200)
        return;

    nsp = pSpawnSprite(psp->PlayerP, nullptr, PRI_BACK, psp->x, psp->y);

    SET(nsp->flags, PANF_WEAPON_SPRITE);
    nsp->ang = psp->ang;
    nsp->vel = psp->vel;
    nsp->PanelSpriteFunc = FistBlur;
    nsp->kill_tics = 9;
    nsp->shade = psp->shade + 10;

    nsp->picndx = -1;
    nsp->picnum = psp->picndx;

    if (TEST(psp->State->flags, psf_Xflip))
        SET(nsp->flags, PANF_XFLIP);

    nsp->rotate_ang = psp->rotate_ang;
    nsp->scale = psp->scale;

    SET(nsp->flags, PANF_TRANSLUCENT);
}

void pFistPresent(PANEL_SPRITEp psp);
void pFistRetract(PANEL_SPRITEp psp);
void pFistAction(PANEL_SPRITEp psp);
void pFistRest(PANEL_SPRITEp psp);
void pFistAttack(PANEL_SPRITEp psp);
void pFistSlide(PANEL_SPRITEp psp);
void pFistSlideDown(PANEL_SPRITEp psp);
void pFistHide(PANEL_SPRITEp psp);
void pFistUnHide(PANEL_SPRITEp psp);

void pFistSlideR(PANEL_SPRITEp psp);
void pFistSlideDownR(PANEL_SPRITEp psp);
void pFistBlock(PANEL_SPRITEp psp);

extern PANEL_STATE ps_FistSwing[];
extern PANEL_STATE ps_ReloadFist[];

#define Fist_BEAT_RATE 16
#define Fist_ACTION_RATE 5

PANEL_STATE ps_PresentFist[] =
{
    {ID_FistPresent0, Fist_BEAT_RATE, pFistPresent, &ps_PresentFist[0], 0,0,0}
};

PANEL_STATE ps_FistRest[] =
{
    {ID_FistPresent0, Fist_BEAT_RATE, pFistRest, &ps_FistRest[0], 0,0,0}
};

PANEL_STATE ps_FistHide[] =
{
    {ID_FistPresent0, Fist_BEAT_RATE, pFistHide, &ps_FistHide[0], 0,0,0}
};

PANEL_STATE ps_PresentFist2[] =
{
    {ID_Fist2Present0, Fist_BEAT_RATE, pFistPresent, &ps_PresentFist2[0], 0,0,0}
};

PANEL_STATE ps_Fist2Rest[] =
{
    {ID_Fist2Present0, Fist_BEAT_RATE, pFistRest, &ps_Fist2Rest[0], 0,0,0}
};

PANEL_STATE ps_Fist2Hide[] =
{
    {ID_Fist2Present0, Fist_BEAT_RATE, pFistHide, &ps_Fist2Hide[0], 0,0,0}
};


#define FIST_PAUSE_TICS 6
#define FIST_SLIDE_TICS 6
#define FIST_MID_SLIDE_TICS 16

PANEL_STATE ps_FistSwing[] =
{
    {ID_FistSwing0, FIST_PAUSE_TICS,                    pNullAnimator,      &ps_FistSwing[1], 0,0,0},
    {ID_FistSwing1, FIST_SLIDE_TICS, /* start slide */ pNullAnimator,        &ps_FistSwing[2], 0,0,0},
    {ID_FistSwing2, 0, /* damage */ pFistAttack,       &ps_FistSwing[3], psf_QuickCall, 0,0},
    {ID_FistSwing2, FIST_MID_SLIDE_TICS, /* mid slide */ pFistSlideDown,    &ps_FistSwing[4], 0,0,0},

    {ID_FistSwing2, 2, /* end slide */ pFistSlideDown,    &ps_FistSwing[4], 0,0,0},

    {ID_FistSwing1, FIST_SLIDE_TICS, /* start slide */ pFistSlideR,       &ps_FistSwing[6], psf_Xflip, 0,0},
    {ID_FistSwing2, 0, /* damage */ pFistAttack,       &ps_FistSwing[7], psf_QuickCall|psf_Xflip, 0,0},
    {ID_FistSwing2, FIST_MID_SLIDE_TICS, /* mid slide */ pFistSlideDownR,   &ps_FistSwing[8], psf_Xflip, 0,0},

    {ID_FistSwing2, 2, /* end slide */ pFistSlideDownR,   &ps_FistSwing[8], psf_Xflip, 0,0},
    {ID_FistSwing2, 2, /* end slide */ pNullAnimator,      &ps_FistSwing[1], psf_Xflip, 0,0},
};

PANEL_STATE ps_Fist2Swing[] =
{
    {4058, FIST_PAUSE_TICS,                    pNullAnimator,      &ps_Fist2Swing[1], 0,0,0},
    {4058, FIST_SLIDE_TICS, /* start slide */ pNullAnimator,      &ps_Fist2Swing[2], 0,0,0},
    {4058, 0, /* damage */ pFistBlock,         &ps_Fist2Swing[0], psf_QuickCall, 0,0},
    {4058, FIST_MID_SLIDE_TICS+5, /* mid slide */ pFistSlideDown,     &ps_Fist2Swing[4], 0,0,0},

    {4058, 2, /* end slide */ pFistSlideDown,    &ps_Fist2Swing[4], 0,0,0},
};

PANEL_STATE ps_Fist3Swing[] =
{
    {ID_Fist3Swing0, FIST_PAUSE_TICS+25,                    pNullAnimator,      &ps_Fist3Swing[1], 0,0,0},
    {ID_Fist3Swing1, 0, /* damage */ pFistAttack,       &ps_Fist3Swing[2], psf_QuickCall, 0,0},
    {ID_Fist3Swing2, FIST_PAUSE_TICS+10,                    pNullAnimator,      &ps_Fist3Swing[3], 0,0,0},
    {ID_Fist3Swing2, FIST_MID_SLIDE_TICS+3, /* mid slide */ pFistSlideDown,    &ps_Fist3Swing[4], 0,0,0},

    {ID_Fist3Swing2, 8, /* end slide */ pFistSlideDown,    &ps_Fist3Swing[4], 0,0,0},

    {ID_Fist3Swing1, FIST_SLIDE_TICS+20, /* start slide */ pFistSlideR,       &ps_Fist3Swing[6], psf_Xflip, 0,0},
    {ID_Fist3Swing2, 0, /* damage */ pFistAttack,       &ps_Fist3Swing[7], psf_QuickCall|psf_Xflip, 0,0},
    {ID_Fist3Swing2, FIST_MID_SLIDE_TICS+3, /* mid slide */ pFistSlideDownR,   &ps_Fist3Swing[8], psf_Xflip, 0,0},

    {ID_Fist3Swing2, 8, /* end slide */ pFistSlideDownR,   &ps_Fist3Swing[8], psf_Xflip, 0,0},
    {ID_Fist3Swing2, 8, /* end slide */ pNullAnimator,      &ps_Fist3Swing[1], psf_Xflip, 0,0},
};

#define KICK_PAUSE_TICS 40
#define KICK_SLIDE_TICS 30
#define KICK_MID_SLIDE_TICS 20

PANEL_STATE ps_Kick[] =
{
    {ID_Kick0, KICK_PAUSE_TICS,                    pNullAnimator,     &ps_Kick[1], 0,0,0},
    {ID_Kick1, 0, /* damage */ pFistAttack,       &ps_Kick[2], psf_QuickCall, 0,0},
    {ID_Kick1, KICK_SLIDE_TICS, /* start slide */ pNullAnimator,     &ps_Kick[3], 0,0,0},
    {ID_Kick1, KICK_MID_SLIDE_TICS, /* mid slide */ pFistSlideDown,    &ps_Kick[4], 0,0,0},

    {ID_Kick1, 30, /* end slide */ pFistSlideDown,    &ps_Kick[4], 0,0,0},

    {ID_Kick0, KICK_SLIDE_TICS, /* start slide */ pNullAnimator,     &ps_Kick[6], psf_Xflip, 0,0},
    {ID_Kick1, 0, /* damage */ pFistAttack,       &ps_Kick[7], psf_QuickCall|psf_Xflip, 0,0},
    {ID_Kick1, KICK_MID_SLIDE_TICS,/* mid slide */ pFistSlideDownR,   &ps_Kick[8], psf_Xflip, 0, 0},

    {ID_Kick1, 30, /* end slide */ pFistSlideDownR,   &ps_Kick[8], psf_Xflip, 0,0},
    {ID_Kick1, 30, /* end slide */ pNullAnimator,     &ps_Kick[1], psf_Xflip, 0,0},
};

PANEL_STATE ps_RetractFist[] =
{
    {ID_FistPresent0, Fist_BEAT_RATE, pFistRetract, &ps_RetractFist[0], 0,0,0}
};

#define FIST_SWAY_AMT 12

// left swing
#define FIST_XOFF (290 + FIST_SWAY_AMT)
#define FIST_YOFF 200

// right swing
#define FISTR_XOFF (0 - 80)

#define FIST_VEL 3000
#define FIST_POWER_VEL 3000

void
InitWeaponFist(PLAYERp pp)
{
    PANEL_SPRITEp psp;
    short rnd_num;


    if (Prediction)
        return;

    if (!TEST(pp->WpnFlags, BIT(WPN_FIST)) ||
        //pp->WpnAmmo[WPN_FIST] <= 0 ||
        TEST(pp->Flags, PF_WEAPON_RETRACT))
    {
        pp->WpnFirstType = WPN_SWORD;
        InitWeaponSword(pp);
        return;
    }

    if (!pp->Wpn[WPN_FIST])
    {
        psp = pp->Wpn[WPN_FIST] = pSpawnSprite(pp, ps_PresentFist, PRI_MID, FIST_XOFF, FIST_YOFF);
        psp->y += tileHeight(psp->picndx);
        psp->backupy();
    }

    if (pp->CurWpn == pp->Wpn[WPN_FIST])
    {
        return;
    }

    PlayerUpdateWeapon(pp, WPN_FIST);

    pp->WpnUziType = 2; // Make uzi's go away!
    RetractCurWpn(pp);

    // Set up the new Weapon variables
    psp = pp->CurWpn = pp->Wpn[WPN_FIST];
    SET(psp->flags, PANF_WEAPON_SPRITE);
    psp->ActionState = ps_FistSwing;
    psp->RetractState = ps_RetractFist;
    psp->PresentState = ps_PresentFist;
    psp->RestState = ps_FistRest;
    pSetState(psp, psp->PresentState);

    pp->WpnKungFuMove = 0; // Set to default strike

    rnd_num = RANDOM_P2(1024);
    if (rnd_num > 900)
        PlaySound(DIGI_TAUNTAI2, pp, v3df_follow|v3df_dontpan);
    else if (rnd_num > 800)
        PlaySound(DIGI_PLAYERYELL1, pp, v3df_follow|v3df_dontpan);
    else if (rnd_num > 700)
        PlaySound(DIGI_PLAYERYELL2, pp, v3df_follow|v3df_dontpan);

    psp->PlayerP->KeyPressBits |= SB_FIRE;
}


void
pFistPresent(PANEL_SPRITEp psp)
{
    int rnd;

    if (TEST(psp->PlayerP->Flags, PF_WEAPON_RETRACT))
        return;

    psp->backupy();
    psp->y -= 3 * synctics;

    if (psp->y < FIST_YOFF)
    {
        psp->oy = psp->y = FIST_YOFF;
        psp->backupboby();

        rnd = RANDOM_RANGE(1000);
        if (rnd > 500)
        {
            psp->PresentState = ps_PresentFist;
            psp->RestState = ps_FistRest;
        }
        else
        {
            psp->PresentState = ps_PresentFist2;
            psp->RestState = ps_Fist2Rest;
        }
        pSetState(psp, psp->RestState);
    }
}

//
// LEFT SWING
//

void
pFistSlide(PANEL_SPRITEp psp)
{
    if (!cl_nomeleeblur)
    {
        SpawnFistBlur(psp);
    }

    //psp->backupx();
    psp->backupy();

    //psp->x += pspSinVel(psp);
    psp->y -= pspSinVel(psp);

    psp->vel += 68 * synctics;
}

void
pFistSlideDown(PANEL_SPRITEp psp)
{
    if (!cl_nomeleeblur)
    {
        SpawnFistBlur(psp);
    }

    short vel = 3500;
    auto ang = FistAng + psp->ang + psp->PlayerP->FistAng;

    psp->backupcoords();

    if (psp->ActionState == ps_Kick || psp->PlayerP->WpnKungFuMove == 3)
    {
        psp->y -= pspSinVel(psp, ang);
    }
    else
    {
        psp->x -= pspSinVel(psp, ang);
        psp->y -= pspSinVel(psp, ang) * synctics;
    }

    psp->vel += 48 * synctics;

    if (psp->y > 440)
    {
        // if still holding down the fire key - continue swinging
        if (psp->PlayerP->input.actions & SB_FIRE)
        {
            if (psp->PlayerP->KeyPressBits & SB_FIRE)
            {
                DoPlayerChooseYell(psp->PlayerP);

                if (RANDOM_RANGE(1000) > 500)
                {
                    //if(RANDOM_RANGE(1000) > 300)
                    //    {
                    psp->ActionState = ps_FistSwing;
                    psp->PlayerP->WpnKungFuMove = 0;
                    //    } else
                    //    {
                    //    psp->ActionState = ps_Fist3Swing;
                    //    psp->PlayerP->WpnKungFuMove = 1;
                    //    }
                    pSetState(psp, psp->ActionState);

                    psp->ox = psp->x = FIST_XOFF;
                    psp->oy = psp->y = FIST_YOFF;
                    psp->backupboby();
                    psp->PlayerP->FistAng = FistAngTable[RANDOM_RANGE(SIZ(FistAngTable))];
                    psp->ang = 1024;
                    psp->vel = vel;
                    DoPlayerSpriteThrow(psp->PlayerP);
                    return;
                }
                else
                {
                    pSetState(psp, ps_FistSwing+(psp->State - psp->ActionState)+1);
                    psp->ActionState = ps_FistSwing;
                    psp->PlayerP->WpnKungFuMove = 0;
                }

                psp->ox = psp->x = FISTR_XOFF+100;
                psp->oy = psp->y = FIST_YOFF;
                psp->backupboby();
                psp->ang = 1024;
                psp->PlayerP->FistAng = FistAngTable[RANDOM_RANGE(SIZ(FistAngTable))];
                psp->vel = vel;
                DoPlayerSpriteThrow(psp->PlayerP);
                return;
            }
        }

        // NOT still holding down the fire key - stop swinging
        pSetState(psp, psp->PresentState);
        psp->ox = psp->x = FIST_XOFF;
        psp->oy = psp->y = FIST_YOFF + tileHeight(psp->picndx);
        psp->backupboby();
    }
}

//
// RIGHT SWING
//

void
pFistSlideR(PANEL_SPRITEp psp)
{
    if (!cl_nomeleeblur)
    {
        SpawnFistBlur(psp);
    }

    //psp->backupx();
    psp->backupy();

    //psp->x += pspSinVel(psp);
    psp->y += pspSinVel(psp);

    psp->vel += 68 * synctics;
}

void
pFistSlideDownR(PANEL_SPRITEp psp)
{
    if (!cl_nomeleeblur)
    {
        SpawnFistBlur(psp);
    }

    short vel = 3500;
    auto ang = FistAng + psp->ang + psp->PlayerP->FistAng;

    psp->backupcoords();

    if (psp->ActionState == ps_Kick || psp->PlayerP->WpnKungFuMove == 3)
    {
        psp->y -= pspSinVel(psp, ang);
    }
    else
    {
        psp->x -= pspSinVel(psp, ang);
        psp->y -= pspSinVel(psp, ang) * synctics;
    }

    psp->vel += 48 * synctics;

    if (psp->y > 440)
    {
        // if still holding down the fire key - continue swinging
        if (psp->PlayerP->input.actions & SB_FIRE)
        {
            if (psp->PlayerP->KeyPressBits & SB_FIRE)
            {
                DoPlayerChooseYell(psp->PlayerP);

                if (RANDOM_RANGE(1000) > 500)
                {
                    psp->ActionState = ps_FistSwing+5;
                    psp->PlayerP->WpnKungFuMove = 0;
                    pSetState(psp, psp->ActionState);

                    psp->ox = psp->x = FISTR_XOFF+100;
                    psp->oy = psp->y = FIST_YOFF;
                    psp->backupboby();
                    psp->ang = 1024;
                    psp->PlayerP->FistAng = FistAngTable[RANDOM_RANGE(SIZ(FistAngTable))];
                    psp->vel = vel;
                    DoPlayerSpriteThrow(psp->PlayerP);
                    return;
                }
                else
                {
                    pSetState(psp, ps_FistSwing+(psp->State - psp->ActionState)+1);
                    psp->ActionState = ps_FistSwing;
                    psp->PlayerP->WpnKungFuMove = 0;
                }

                psp->ox = psp->x = FIST_XOFF;
                psp->oy = psp->y = FIST_YOFF;
                psp->backupboby();
                psp->PlayerP->FistAng = FistAngTable[RANDOM_RANGE(SIZ(FistAngTable))];
                psp->ang = 1024;
                psp->vel = vel;
                DoPlayerSpriteThrow(psp->PlayerP);
                return;
            }
        }

        // NOT still holding down the fire key - stop swinging
        pSetState(psp, psp->PresentState);
        psp->ox = psp->x = FIST_XOFF;
        psp->oy = psp->y = FIST_YOFF + tileHeight(psp->picndx);
        psp->backupboby();
    }
}

void
pFistBobSetup(PANEL_SPRITEp psp)
{
    if (TEST(psp->flags, PANF_BOB))
        return;

    psp->backupbobcoords();

    psp->sin_amt = FIST_SWAY_AMT;
    psp->sin_ndx = 0;
    psp->bob_height_divider = 8;
}

void
pFistHide(PANEL_SPRITEp psp)
{
    short picnum = psp->picndx;

    psp->backupy();
    psp->y += 3 * synctics;

    if (psp->y >= FIST_YOFF + tileHeight(picnum))
    {
        psp->oy = psp->y = FIST_YOFF + tileHeight(picnum);

        pWeaponUnHideKeys(psp, psp->PresentState);
    }
}

void
pFistRest(PANEL_SPRITEp psp)
{
    bool force = !!TEST(psp->flags, PANF_UNHIDE_SHOOT);

    if (pWeaponHideKeys(psp, ps_FistHide))
        return;

    psp->yorig += synctics;

    if (psp->yorig > FIST_YOFF)
    {
        psp->yorig = FIST_YOFF;
    }

    pFistBobSetup(psp);
    pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP));

    force = !!TEST(psp->flags, PANF_UNHIDE_SHOOT);

    if (psp->ActionState == ps_Kick)
        psp->ActionState = ps_FistSwing;

    // Reset move to default
    psp->PlayerP->WpnKungFuMove = 0;

    if ((psp->PlayerP->input.actions & SB_FIRE) || force)
    {
        if ((psp->PlayerP->KeyPressBits & SB_FIRE) || force)
        {
            RESET(psp->flags, PANF_UNHIDE_SHOOT);

            psp->ActionState = ps_FistSwing;
            psp->PlayerP->WpnKungFuMove = 0;

            pSetState(psp, psp->ActionState);

            psp->vel = 5500;

            psp->ang = 1024;
            psp->PlayerP->FistAng = FistAngTable[RANDOM_RANGE(SIZ(FistAngTable))];
            DoPlayerSpriteThrow(psp->PlayerP);
        }
    }
}

void
pFistAction(PANEL_SPRITEp psp)
{
    pFistBobSetup(psp);
    pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP));
}


void
pFistAttack(PANEL_SPRITEp psp)
{
    InitFistAttack(psp->PlayerP);
}

void
pFistRetract(PANEL_SPRITEp psp)
{
    short picnum = psp->picndx;

    psp->backupy();
    psp->y += 3 * synctics;

    if (psp->y >= FIST_YOFF + tileHeight(picnum))
    {
        RESET(psp->PlayerP->Flags, PF_WEAPON_RETRACT);
        psp->PlayerP->Wpn[WPN_FIST] = nullptr;
        pKillSprite(psp);
    }
}

void
pFistBlock(PANEL_SPRITEp psp)
{
    psp->yorig += synctics;

    if (psp->yorig > FIST_YOFF)
    {
        psp->yorig = FIST_YOFF;
    }

    pFistBobSetup(psp);
    pWeaponBob(psp, PLAYER_MOVING(psp->PlayerP));

    if (!(psp->PlayerP->input.actions & SB_OPEN))
    {
        pStatePlusOne(psp);
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
//
// PANEL SPRITE GENERAL ROUTINES
//
//////////////////////////////////////////////////////////////////////////////////////////



void
pWeaponForceRest(PLAYERp pp)
{
    pSetState(pp->CurWpn, pp->CurWpn->RestState);
}

bool
pWeaponUnHideKeys(PANEL_SPRITEp psp, PANEL_STATEp state)
{
    // initing the other weapon will take care of this
    if (TEST(psp->flags, PANF_DEATH_HIDE))
    {
        return false;
    }

    if (TEST(psp->flags, PANF_WEAPON_HIDE))
    {
        if (!TEST(psp->PlayerP->Flags, PF_WEAPON_DOWN))
        {
            RESET(psp->flags, PANF_WEAPON_HIDE);
            pSetState(psp, state);
            return true;
        }

        return false;
    }

    if (psp->PlayerP->input.actions & SB_HOLSTER)
    {
        if (psp->PlayerP->KeyPressBits & SB_HOLSTER)
        {
            psp->PlayerP->KeyPressBits &= ~SB_HOLSTER;
            pSetState(psp, state);
            return true;
        }
    }
    else
    {
        psp->PlayerP->KeyPressBits |= SB_HOLSTER;
    }

    if (psp->PlayerP->input.actions & SB_FIRE)
    {
        if (psp->PlayerP->KeyPressBits & SB_FIRE)
        {
            SET(psp->flags, PANF_UNHIDE_SHOOT);
            pSetState(psp, state);
            return true;
        }
    }

    return false;
}

bool
pWeaponHideKeys(PANEL_SPRITEp psp, PANEL_STATEp state)
{
    if (TEST(psp->PlayerP->Flags, PF_DEAD))
    {
        SET(psp->flags, PANF_DEATH_HIDE);
        pSetState(psp, state);
        return true;
    }

    if (TEST(psp->PlayerP->Flags, PF_WEAPON_DOWN))
    {
        SET(psp->flags, PANF_WEAPON_HIDE);
        pSetState(psp, state);
        return true;
    }

    if (psp->PlayerP->input.actions & SB_HOLSTER)
    {
        if (psp->PlayerP->KeyPressBits & SB_HOLSTER)
        {
            psp->PlayerP->KeyPressBits &= ~SB_HOLSTER;
            PutStringInfo(psp->PlayerP,"Weapon Holstered");
            pSetState(psp, state);
            return true;
        }
    }
    else
    {
        psp->PlayerP->KeyPressBits |= SB_HOLSTER;
    }

    return false;

}

void
InsertPanelSprite(PLAYERp pp, PANEL_SPRITEp psp)
{
    PANEL_SPRITEp cur, nxt;

    ASSERT(psp);

    // if list is empty, insert at front
    if (EMPTY(&pp->PanelSpriteList))
    {
        INSERT(&pp->PanelSpriteList, psp);
        return;
    }

    // if new pri is <= first pri in list, insert at front
    if (psp->priority <= pp->PanelSpriteList.Next->priority)
    {
        INSERT(&pp->PanelSpriteList, psp);
        return;
    }

    // search for first pri in list thats less than the new pri
    TRAVERSE(&pp->PanelSpriteList, cur, nxt)
    {
        // if the next pointer is the end of the list, insert it
        if ((LIST) cur->Next == (LIST) &pp->PanelSpriteList)
        {
            INSERT(cur, psp);
            return;
        }


        // if between cur and next, insert here
        if (psp->priority >= cur->priority && psp->priority <= cur->Next->priority)
        {
            INSERT(cur, psp);
            return;
        }
    }
}


PANEL_SPRITEp
pSpawnSprite(PLAYERp pp, PANEL_STATEp state, uint8_t priority, double x, double y)
{
    unsigned i;
    PANEL_SPRITEp psp;


    ASSERT(pp);

    psp = (PANEL_SPRITEp)CallocMem(sizeof(PANEL_SPRITE), 1);

    PRODUCTION_ASSERT(psp);

    psp->priority = priority;
    InsertPanelSprite(pp, psp);
    // INSERT(&pp->PanelSpriteList, psp);

    psp->PlayerP = pp;

    psp->ox = psp->x = x;
    psp->oy = psp->y = y;
    pSetState(psp, state);
    if (state == nullptr)
        psp->picndx = -1;
    else
        psp->picndx = state->picndx;
    psp->ang = 0;
    psp->vel = 0;
    psp->rotate_ang = 0;
    psp->scale = FRACUNIT;
    psp->ID = 0;

    for (i = 0; i < SIZ(psp->over); i++)
    {
        psp->over[i].State = nullptr;
        psp->over[i].pic = -1;
        psp->over[i].xoff = -1;
        psp->over[i].yoff = -1;
    }

    return psp;
}

void
pSuicide(PANEL_SPRITEp psp)
{
    pKillSprite(psp);
}

void
pKillSprite(PANEL_SPRITEp psp)
{
    PRODUCTION_ASSERT(psp);

    REMOVE(psp);

    FreeMem(psp);
}

void
pClearSpriteList(PLAYERp pp)
{
    PANEL_SPRITEp psp=nullptr, next_psp=nullptr;

    TRAVERSE(&pp->PanelSpriteList, psp, next_psp)
    {
        pKillSprite(psp);
    }
}

void
pWeaponBob(PANEL_SPRITEp psp, short condition)
{
    double xdiff = 0, ydiff = 0;

    double bobvel = fFindDistance2D(psp->PlayerP->xvect, psp->PlayerP->yvect) * (1. / 32768.);
    bobvel = bobvel + (bobvel * (1. / 4.));
    bobvel = min(bobvel, 128.);

    if (condition)
    {
        SET(psp->flags, PANF_BOB);
    }
    else
    {
        if (labs((psp->sin_ndx & 1023) - 0) < 70)
        {
            RESET(psp->flags, PANF_BOB);
            psp->sin_ndx = (RANDOM_P2(1024) < 512) ? 1024 : 0;
        }
    }

    if (cl_weaponsway && TEST(psp->flags, PANF_BOB))
    {
        // //
        // sin_xxx moves the weapon left-right
        // //

        // increment the ndx into the sin table
        psp->sin_ndx = (psp->sin_ndx + (synctics << 3) + (cl_swsmoothsway ? (synctics << 2) : RANDOM_P2(8) * synctics)) & 2047;

        // get height
        xdiff = psp->sin_amt * bsinf(psp->sin_ndx, -14);

        // //
        // bob_xxx moves the weapon up-down
        // //

        // as the weapon moves left-right move the weapon up-down in the same
        // proportion
        double bob_ndx = (psp->sin_ndx + 512) & 1023;

        // base bob_amt on the players velocity - Max of 128
        double bob_amt = bobvel / psp->bob_height_divider;
        ydiff = bob_amt * bsinf(bob_ndx, -14);
    }

    // Back up current coordinates for interpolating.
    psp->backupcoords();

    psp->x = psp->xorig + xdiff;
    psp->y = psp->yorig + ydiff + UziRecoilYadj;
}


//////////////////////////////////////////////////////////////////////////////////////////
//
// PANEL SPRITE CONTROL ROUTINES
//
//////////////////////////////////////////////////////////////////////////////////////////

bool DrawBeforeView = false;
void
pDisplaySprites(PLAYERp pp, double smoothratio)
{
    USERp u = User[pp->PlayerSprite].Data();
    PANEL_SPRITEp psp=nullptr, next=nullptr;
    short shade, picnum, overlay_shade = 0;
    double x, y;
    unsigned i;

    uint8_t pal = 0;
    short ang;
    int flags;

    double const look_anghalf = pp->angle.look_anghalf(smoothratio);
    double const looking_arc = pp->angle.looking_arc(smoothratio);

    TRAVERSE(&pp->PanelSpriteList, psp, next)
    {
        ang = psp->rotate_ang;
        shade = 0;
        flags = 0;
        if (cl_hudinterpolation)
        {
            x = interpolatedvaluef(psp->ox, psp->x, smoothratio);
            y = interpolatedvaluef(psp->oy, psp->y, smoothratio);

        }
        else
        {
            x = psp->x;
            y = psp->y;
        }

        x -= look_anghalf;
        y += looking_arc;

        // initilize pal here - jack with it below
        pal = uint8_t(psp->pal);

        if (DrawBeforeView)
            if (!TEST(psp->flags, PANF_DRAW_BEFORE_VIEW))
                continue;

        if (TEST(psp->flags, PANF_SUICIDE))
        {
            //pKillSprite(psp);
            continue;
        }

        // if the state is null get the picnum for other than picndx
        if (psp->picndx == -1 || !psp->State)
            picnum = psp->picnum;
        else
            picnum = psp->picndx;

        // UK panzies have to have darts instead of shurikens.
        if (sw_darts)
            switch (picnum)
            {
            case STAR_REST:
                picnum = 2510;
                break;
            case STAR_REST + 1:
                picnum = 2511;
                break;
            case STAR_REST + 2:
                picnum = 2512;
                break;
            case STAR_REST + 3:
                picnum = 2513;
                break;
            case STAR_REST + 4:
                picnum = 2514;
                break;
            case STAR_REST + 5:
                picnum = 2515;
                break;
            case STAR_REST + 6:
                picnum = 2516;
                break;
            case STAR_REST + 7:
                picnum = 2517;
                break;
            }

        if (pp->Bloody)
        {
            switch (picnum)
            {
            case SWORD_REST:
                picnum = BLOODYSWORD_REST;
                break;
            case SWORD_SWING0:
                picnum = BLOODYSWORD_SWING0;
                break;
            case SWORD_SWING1:
                picnum = BLOODYSWORD_SWING1;
                break;
            case SWORD_SWING2:
                picnum = BLOODYSWORD_SWING2;
                break;

            case FIST_REST:
                picnum = 4077;
                break;
            case FIST2_REST:
                picnum = 4051;
                break;
            case FIST_SWING0:
                picnum = 4074;
                break;
            case FIST_SWING1:
                picnum = 4075;
                break;
            case FIST_SWING2:
                picnum = 4076;
                break;

            case STAR_REST:
            case 2510:
                if (!sw_darts)
                    picnum = 2138;
                else
                    picnum = 2518; // Bloody Dart Hand
                break;
            }
        }

        if (pp->WpnShotgunType == 1)
        {
            switch (picnum)
            {
            case SHOTGUN_REST:
            case SHOTGUN_RELOAD0:
                picnum = 2227;
                break;
            case SHOTGUN_RELOAD1:
                picnum = 2226;
                break;
            case SHOTGUN_RELOAD2:
                picnum = 2225;
                break;
            }
        }

        // don't draw
        if (TEST(psp->flags, PANF_INVISIBLE))
            continue;

        if (psp->State && TEST(psp->State->flags, psf_Invisible))
            continue;

        // if its a weapon sprite and the view is set to the outside don't draw the sprite
        if (TEST(psp->flags, PANF_WEAPON_SPRITE))
        {
            SECT_USERp sectu = nullptr;
            int16_t floorshade = 0;
            if (pp->cursectnum >= 0)
            {
                sectu = SectUser[pp->cursectnum].Data();
                pal = sector[pp->cursectnum].floorpal;
                floorshade = sector[pp->cursectnum].floorshade;

                if (pal != PALETTE_DEFAULT)
                {
                    if (sectu && TEST(sectu->flags, SECTFU_DONT_COPY_PALETTE))
                        pal = PALETTE_DEFAULT;
                }

                if (pal == PALETTE_FOG || pal == PALETTE_DIVE || pal == PALETTE_DIVE_LAVA)
                    pal = uint8_t(psp->pal); // Set it back
            }

            ///////////

            if (pp->InventoryActive[INVENTORY_CLOAK])
            {
                SET(flags, RS_TRANS1);
            }

            //shade = overlay_shade = DIV2(sector[pp->cursectnum].floorshade + sector[pp->cursectnum].ceilingshade);
            shade = overlay_shade = floorshade - 10;

            if (TEST(psp->PlayerP->Flags, PF_VIEW_FROM_OUTSIDE))
            {
                if (!TEST(psp->PlayerP->Flags, PF_VIEW_OUTSIDE_WEAPON))
                    continue;
            }

            if (TEST(psp->PlayerP->Flags, PF_VIEW_FROM_CAMERA))
                continue;

            // !FRANK - this was moved from BELOW this IF statement
            // if it doesn't have a picflag or its in the view
            if (sectu && TEST(sectu->flags, SECTFU_DONT_COPY_PALETTE))
                pal = 0;
        }

        if (TEST(psp->flags, PANF_TRANSLUCENT))
            SET(flags, RS_TRANS1);

        SET(flags, TEST(psp->flags, PANF_TRANS_FLIP));

        if (TEST(psp->flags, PANF_CORNER))
            SET(flags, RS_TOPLEFT);

        if ((psp->State && TEST(psp->State->flags, psf_Xflip)) || TEST(psp->flags, PANF_XFLIP))
        {
            SET(flags, RS_XFLIPHUD);
        }

        // shading
        if (psp->State && TEST(psp->State->flags, psf_ShadeHalf|psf_ShadeNone))
        {
            if (TEST(psp->State->flags, psf_ShadeNone))
                shade = 0;
            else if (TEST(psp->State->flags, psf_ShadeHalf))
                shade /= 2;
        }

        if (pal == PALETTE_DEFAULT)
        {
            switch (picnum)
            {
            case 4080:
            case 4081:
            case 2220:
            case 2221:
                pal = u->spal;
                break;
            }
        }

        // temporary hack to fix fist artifacts until a solution is found in the panel system itself
        switch (picnum)
        {
            case FIST_SWING0:
            case FIST_SWING1:
            case FIST_SWING2:
            case FIST2_SWING0:
            case FIST2_SWING1:
            case FIST2_SWING2:
            case FIST3_SWING0:
            case FIST3_SWING1:
            case FIST3_SWING2:
            case BLOODYFIST_SWING0:
            case BLOODYFIST_SWING1:
            case BLOODYFIST_SWING2:
            case BLOODYFIST2_SWING0:
            case BLOODYFIST2_SWING1:
            case BLOODYFIST2_SWING2:
            case BLOODYFIST3_SWING0:
            case BLOODYFIST3_SWING1:
            case BLOODYFIST3_SWING2:
                if (TEST(flags, RS_XFLIPHUD) && x > 160)
                    x = 65;
                else if (!TEST(flags, RS_XFLIPHUD) && x < 160)
                    x = 345;
                break;
            default:
                break;
        }

		hud_drawsprite(x, y, psp->scale, ang, picnum, shade, pal, flags);

        // do overlays (if any)
        for (i = 0; i < SIZ(psp->over); i++)
        {
            // get pic from state
            if (psp->over[i].State)
                picnum = psp->over[i].State->picndx;
            else
            // get pic from over variable
            if (psp->over[i].pic >= 0)
                picnum = psp->over[i].pic;
            else
                continue;

            if (TEST(psp->over[i].flags, psf_ShadeNone))
                overlay_shade = 0;

            if (picnum)
            {
                hud_drawsprite((x + psp->over[i].xoff), (y + psp->over[i].yoff), psp->scale, ang, picnum, overlay_shade, pal, flags);
            }
        }
    }
}

void
pSpriteControl(PLAYERp pp)
{
    PANEL_SPRITEp psp=nullptr, next=nullptr;

    TRAVERSE(&pp->PanelSpriteList, psp, next)
    {
        // reminder - if these give an assertion look for pKillSprites
        // somewhere else other than by themselves
        // RULE: Sprites can only kill themselves
        PRODUCTION_ASSERT(psp);

        if (psp->State)
            pStateControl(psp);
        else
        // for sprits that are not state controled but still need to call
        // something
        if (psp->PanelSpriteFunc)
        {
            (*psp->PanelSpriteFunc)(psp);
        }
    }
}

void
pSetState(PANEL_SPRITEp psp, PANEL_STATEp panel_state)
{
    PRODUCTION_ASSERT(psp);
    psp->tics = 0;
    psp->State = panel_state;
    psp->picndx = panel_state ? panel_state->picndx : 0;
}


void
pNextState(PANEL_SPRITEp psp)
{
    // Transition to the next state
    psp->State = psp->State->NextState;

    if (TEST(psp->State->flags, psf_QuickCall))
    {
        (*psp->State->Animator)(psp);
        psp->State = psp->State->NextState;
    }
}

void
pStatePlusOne(PANEL_SPRITEp psp)
{
    psp->tics = 0;
    psp->State++;

    if (TEST(psp->State->flags, psf_QuickCall))
    {
        (*psp->State->Animator)(psp);
        psp->State = psp->State->NextState;
    }
}


void
pStateControl(PANEL_SPRITEp psp)
{
    unsigned i;
    short tics = synctics;

    psp->tics += tics;

    // Skip states if too much time has passed
    while (psp->tics >= psp->State->tics)
    {
        // Set Tics
        psp->tics -= psp->State->tics;

        pNextState(psp);
    }

    // Set picnum to the correct pic
    psp->picndx = psp->State->picndx;

    // do overlay states
    for (i = 0; i < SIZ(psp->over); i++)
    {
        if (!psp->over[i].State)
            continue;

        psp->over[i].tics += tics;

        // Skip states if too much time has passed
        while (psp->over[i].tics >= psp->over[i].State->tics)
        {
            // Set Tics
            psp->over[i].tics -= psp->over[i].State->tics;
            psp->over[i].State = psp->over[i].State->NextState;

            if (!psp->over[i].State)
                break;
        }
    }

    // Call the correct animator
    if (psp->State->Animator)
        (*psp->State->Animator)(psp);

}


void
UpdatePanel(double smoothratio)
{
    short pnum;

    TRAVERSE_CONNECT(pnum)
    {
        if (pnum == screenpeek)
            pDisplaySprites(Player + pnum, smoothratio);
    }
}

void
PreUpdatePanel(double smoothratio)
{
    short pnum;
    DrawBeforeView = true;

    //if (DrawBeforeView)
    TRAVERSE_CONNECT(pnum)
    {
        if (pnum == screenpeek)
            pDisplaySprites(Player + pnum, smoothratio);
    }

    DrawBeforeView = false;
}

#define EnvironSuit_RATE 10
#define Fly_RATE 10
#define Cloak_RATE 10
#define Night_RATE 10
#define Box_RATE 10
#define Medkit_RATE 10
#define RepairKit_RATE 10
#define ChemBomb_RATE 10
#define FlashBomb_RATE 10
#define SmokeBomb_RATE 10
#define Caltrops_RATE 10

#define ID_PanelEnvironSuit 2397
PANEL_STATE ps_PanelEnvironSuit[] =
{
    {ID_PanelEnvironSuit, EnvironSuit_RATE, PanelInvTestSuicide, &ps_PanelEnvironSuit[0], 0,0,0}
};


#include "saveable.h"

static saveable_code saveable_panel_code[] =
{
    SAVE_CODE(pSuicide),
    SAVE_CODE(SwordBlur),
    SAVE_CODE(SpecialUziRetractFunc),
    SAVE_CODE(FistBlur),
};

static saveable_data saveable_panel_data[] =
{
    SAVE_DATA(ps_PresentSword),
    SAVE_DATA(ps_SwordRest),
    SAVE_DATA(ps_SwordHide),
    SAVE_DATA(ps_SwordSwing),
    SAVE_DATA(ps_RetractSword),

    SAVE_DATA(ps_PresentStar),
    SAVE_DATA(ps_StarHide),
    SAVE_DATA(ps_StarRest),
    SAVE_DATA(ps_ThrowStar),
    SAVE_DATA(ps_RetractStar),

    SAVE_DATA(ps_FireUzi),
    SAVE_DATA(ps_UziNull),
    SAVE_DATA(ps_UziHide),
    SAVE_DATA(ps_PresentUzi),
    SAVE_DATA(ps_PresentUziReload),
    SAVE_DATA(ps_RetractUzi),
    SAVE_DATA(ps_FireUzi2),
    SAVE_DATA(ps_PresentUzi2),
    SAVE_DATA(ps_Uzi2Hide),
    SAVE_DATA(ps_RetractUzi2),
    SAVE_DATA(ps_Uzi2Suicide),
    SAVE_DATA(ps_Uzi2Null),
    SAVE_DATA(ps_UziEject),
    SAVE_DATA(ps_UziClip),
    SAVE_DATA(ps_UziReload),
    SAVE_DATA(ps_UziDoneReload),
    SAVE_DATA(ps_UziShell),
    SAVE_DATA(ps_Uzi2Shell),

    SAVE_DATA(ps_ShotgunShell),
    SAVE_DATA(ps_PresentShotgun),
    SAVE_DATA(ps_ShotgunRest),
    SAVE_DATA(ps_ShotgunHide),
    SAVE_DATA(ps_ShotgunRecoil),
    SAVE_DATA(ps_ShotgunRecoilAuto),
    SAVE_DATA(ps_ShotgunFire),
    SAVE_DATA(ps_ShotgunAutoFire),
    SAVE_DATA(ps_ShotgunReload),
    SAVE_DATA(ps_RetractShotgun),
    SAVE_DATA(ps_ShotgunFlash),

    SAVE_DATA(ps_PresentRail),
    SAVE_DATA(ps_RailRest),
    SAVE_DATA(ps_RailHide),
    SAVE_DATA(ps_RailRecoil),
    SAVE_DATA(ps_RailFire),
    SAVE_DATA(ps_RailFireEMP),
    SAVE_DATA(ps_RetractRail),

    SAVE_DATA(ps_PresentHothead),
    SAVE_DATA(ps_HotheadHide),
    SAVE_DATA(ps_RetractHothead),
    SAVE_DATA(ps_HotheadRest),
    SAVE_DATA(ps_HotheadRestRing),
    SAVE_DATA(ps_HotheadRestNapalm),
    SAVE_DATA(ps_HotheadAttack),
    SAVE_DATA(ps_HotheadRing),
    SAVE_DATA(ps_HotheadNapalm),
    SAVE_DATA(ps_HotheadTurn),
    SAVE_DATA(ps_HotheadTurnRing),
    SAVE_DATA(ps_HotheadTurnNapalm),
    SAVE_DATA(ps_OnFire),

    SAVE_DATA(ps_PresentMicro),
    SAVE_DATA(ps_MicroRest),
    SAVE_DATA(ps_MicroHide),
    SAVE_DATA(ps_InitNuke),
    SAVE_DATA(ps_MicroRecoil),
    SAVE_DATA(ps_MicroFire),
    SAVE_DATA(ps_MicroSingleFire),
    SAVE_DATA(ps_RetractMicro),
    SAVE_DATA(ps_MicroHeatFlash),
    SAVE_DATA(ps_MicroNukeFlash),

    SAVE_DATA(ps_PresentHeart),
    SAVE_DATA(ps_HeartRest),
    SAVE_DATA(ps_HeartHide),
    SAVE_DATA(ps_HeartAttack),
    SAVE_DATA(ps_RetractHeart),
    SAVE_DATA(ps_HeartBlood),
    SAVE_DATA(ps_HeartBloodSmall),
    SAVE_DATA(ps_HeartBlood),

    SAVE_DATA(ps_PresentGrenade),
    SAVE_DATA(ps_GrenadeRest),
    SAVE_DATA(ps_GrenadeHide),
    SAVE_DATA(ps_GrenadeFire),
    SAVE_DATA(ps_GrenadeRecoil),
    SAVE_DATA(ps_RetractGrenade),

    SAVE_DATA(ps_PresentMine),
    SAVE_DATA(ps_MineRest),
    SAVE_DATA(ps_MineHide),
    SAVE_DATA(ps_MineThrow),
    SAVE_DATA(ps_RetractMine),

    SAVE_DATA(ps_ChopsAttack1),
    SAVE_DATA(ps_ChopsAttack2),
    SAVE_DATA(ps_ChopsAttack3),
    SAVE_DATA(ps_ChopsAttack4),
    SAVE_DATA(ps_ChopsWait),
    SAVE_DATA(ps_ChopsRetract),

    SAVE_DATA(ps_PresentFist),
    SAVE_DATA(ps_FistRest),
    SAVE_DATA(ps_FistHide),
    SAVE_DATA(ps_PresentFist2),
    SAVE_DATA(ps_Fist2Rest),
    SAVE_DATA(ps_Fist2Hide),
    /*
    SAVE_DATA(ps_PresentFist3),
    SAVE_DATA(ps_Fist3Rest),
    SAVE_DATA(ps_Fist3Hide),
    */
    SAVE_DATA(ps_FistSwing),
    SAVE_DATA(ps_Fist2Swing),
    SAVE_DATA(ps_Fist3Swing),
    SAVE_DATA(ps_Kick),
    SAVE_DATA(ps_RetractFist),

    SAVE_DATA(ps_PanelEnvironSuit),
};

saveable_module saveable_panel =
{
    // code
    saveable_panel_code,
    SIZ(saveable_panel_code),

    // data
    saveable_panel_data,
    SIZ(saveable_panel_data)
};

END_SW_NS
