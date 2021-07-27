//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2017-2019 Nuke.YKT
Copyright (C) 2020 - Christoph Oelckers

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "global.h"
#include "mapinfo.h"
#include "secrets.h"
#include "statistics.h"
#include "gamestate.h"
#include "automap.h"
#include "dukeactor.h"
#include "interpolate.h"
#include "precache.h"
#include "render.h"
#include "screenjob_.h"

BEGIN_DUKE_NS  

int which_palookup = 9;

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void pickrandomspot(int snum)
{
    struct player_struct *p;
    short i;

    p = &ps[snum];

    if( ud.multimode > 1 && ud.coop != 1)
        i = krand()%numplayersprites;
    else i = snum;

    p->bobposx = p->oposx = p->posx = po[i].ox;
    p->bobposy = p->oposy = p->posy = po[i].oy;
    p->oposz = p->posz = po[i].oz;
    p->angle.oang = p->angle.ang = buildang(po[i].oa);
    p->cursectnum = po[i].os;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void resetplayerstats(int snum)
{
    struct player_struct *p;

    p = &ps[snum];

    gFullMap = 0; 
    p->dead_flag        = 0;
    p->resurrected      = false;
    p->wackedbyactor    = nullptr;
    p->falling_counter  = 0;
    p->quick_kick       = 0;
    p->subweapon        = 0;
    p->last_full_weapon = 0;
    p->ftq              = 0;
    p->tipincs          = 0;
    p->buttonpalette    = 0;
    p->actorsqu         =nullptr;
    p->invdisptime      = 0;
    p->refresh_inventory= 0;
    p->last_pissed_time = 0;
    p->holster_weapon   = 0;
    p->pycount          = 0;
    p->pyoff            = 0;
    p->opyoff           = 0;
    p->loogcnt          = 0;
    p->psectlotag       = 0;
    p->weapon_sway      = 0;
//    p->select_dir       = 0;
    p->extra_extra8     = 0;
    p->show_empty_weapon= 0;
    p->dummyplayersprite=nullptr;
    p->crack_time       = 0;
    p->hbomb_hold_delay = 0;
    p->transporter_hold = 0;
    p->wantweaponfire  = -1;
    p->hurt_delay       = 0;
    p->hurt_delay2      = 0;
    p->footprintcount   = 0;
    p->footprintpal     = 0;
    p->footprintshade   = 0;
    p->jumping_toggle   = 0;
    p->horizon.ohoriz = p->horizon.horiz = q16horiz(40);
    p->horizon.ohorizoff = p->horizon.horizoff = q16horiz(0);
    p->bobcounter       = 0;
    p->on_ground        = 0;
    p->player_par       = 0;
    p->sync.actions |= SB_CENTERVIEW;
    p->airleft          = 15*26;
    p->rapid_fire_hold  = 0;
    p->toggle_key_flag  = 0;
    p->access_spritenum = nullptr;
    if(ud.multimode > 1 && ud.coop != 1 )
        p->got_access = 7;
    else p->got_access      = 0;
    p->random_club_frame= 0;
    p->on_warping_sector = 0;
    p->spritebridge      = 0;

    if(p->steroids_amount < 400 )
    {
        p->steroids_amount = 0;
        p->inven_icon = 0;
    }
    p->heat_on =            0;
    p->jetpack_on =         0;
    p->holoduke_on =       nullptr;

    p->angle.olook_ang = p->angle.look_ang = buildang(512 - (((~currentLevel->levelNumber) & 1) << 10));
    p->angle.orotscrnang = p->angle.rotscrnang = buildang(0);

    p->newOwner          =nullptr;
    p->jumping_counter   = 0;
    p->hard_landing      = 0;
    p->posxv             = 0;                           //!!
    p->posyv             = 0;
    p->poszv             = 0;
    p->fric.x            = 0;
    p->fric.y            = 0;
    p->somethingonplayer =nullptr;
    p->angle.spin        = 0;

    p->on_crane          = nullptr;

    if(p->curr_weapon == PISTOL_WEAPON)
        p->okickback_pic = p->kickback_pic  = isRR()? 22 : 5;
    else p->okickback_pic = p->kickback_pic = 0;

    p->oweapon_pos = p->weapon_pos        = 6;
    p->walking_snd_toggle= 0;
    p->weapon_ang        = 0;

    p->knuckle_incs      = 1;
    p->fist_incs = 0;
    p->knee_incs         = 0;
    p->stairs = 0;
    p->noise_x = 0;
    p->noise_y = 0;
    p->donoise = 0;
    p->noise_radius = 0;
    if (isRR() && ud.multimode > 1 && ud.coop != 1)
    {
        p->keys[0] = 1;
        p->keys[1] = 1;
        p->keys[2] = 1;
        p->keys[3] = 1;
        p->keys[4] = 1;
    }
    else
    {
        p->keys[0] = 0;
        p->keys[1] = 0;
        p->keys[2] = 0;
        p->keys[3] = 0;
        p->keys[4] = 0;
    }
    wupass = 0;
    //p->at582 = 0;
    p->drunkang = 1647;
    p->eatang = 1647;
    p->drink_amt = 0;
    p->eat = 0;
    p->drink_timer = 4096;
    p->eat_timer = 4096;
    p->shotgun_state[0] = 0;
    p->shotgun_state[1] = 0;
    p->detonate_time = 0;
    p->detonate_count = 0;
    p->recoil = 0;
    p->yehaa_timer = 0;
    chickenphase = 0;
    if (p->OnMotorcycle)
    {
        p->OnMotorcycle = 0;
        p->gotweapon[MOTORCYCLE_WEAPON] = false;
        p->curr_weapon = isRRRA()? SLINGBLADE_WEAPON : KNEE_WEAPON;	// just in case this is made available for the other games
    }
    p->lotag800kill = 0;
    p->moto_do_bump = 0;
    p->MotoOnGround = 1;
    p->moto_underwater = 0;
    p->MotoSpeed = 0;
    p->TiltStatus = 0;
    p->moto_drink = 0;
    p->VBumpTarget = 0;
    p->VBumpNow  =0;
    p->moto_bump_fast = 0;
    p->TurbCount = 0;
    p->moto_on_mud = 0;
    p->moto_on_oil = 0;
    if (p->OnBoat)
    {
        p->OnBoat = 0;
        p->gotweapon[BOAT_WEAPON] = false;
        p->curr_weapon = isRRRA()? SLINGBLADE_WEAPON : KNEE_WEAPON;	// just in case this is made available for the other games
    }
    p->NotOnWater = 0;
    p->SeaSick = 0;
    p->nocheat = 0;
    p->DrugMode = 0;
    p->drug_stat[0] = 0;
    p->drug_stat[1] = 0;
    p->drug_stat[2] = 0;
    p->drug_aspect = 0;
    resetlanepics();

    if (numplayers < 2)
    {
        ufospawn = isRRRA()? 3 : std::min(ud.player_skill*4+1, 32);
        ufocnt = 0;
        hulkspawn = ud.player_skill + 1;
    }
    else
    {
        ufospawn = isRRRA()? 0 :32;
        ufocnt = 0;
        hulkspawn = isRRRA()? 0 :2;
    }
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void resetweapons(int snum)
{
    short  weapon;
    struct player_struct *p;

    p = &ps[snum];

    for (weapon = PISTOL_WEAPON; weapon < MAX_WEAPONS; weapon++)
    {
        p->ammo_amount[weapon] = 0;
    }

    memset(p->gotweapon, 0, MAX_WEAPONS);
    p->oweapon_pos = p->weapon_pos = 6;
    p->okickback_pic = p->kickback_pic = 5;
    p->curr_weapon = PISTOL_WEAPON;
    p->gotweapon[PISTOL_WEAPON] = true;
    p->gotweapon[KNEE_WEAPON] = true;
    p->ammo_amount[PISTOL_WEAPON] = std::min<int16_t>(gs.max_ammo_amount[PISTOL_WEAPON], 48);
    p->gotweapon[HANDREMOTE_WEAPON] = true;
    p->last_weapon = -1;

    p->show_empty_weapon= 0;
    p->last_pissed_time = 0;
    p->holster_weapon = 0;

    // Always clear these, even for non-RRRA
    p->OnMotorcycle = 0;
    p->moto_underwater = 0;
    p->OnBoat = 0;
    p->lotag800kill = 0;

    if (isRRRA())
    {
        chickenphase = 0;
        p->ammo_amount[KNEE_WEAPON] = 1;
        p->gotweapon[SLINGBLADE_WEAPON] = true;
        p->ammo_amount[SLINGBLADE_WEAPON] = 1;
    }
    OnEvent(EVENT_RESETWEAPONS, snum, nullptr, -1);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void resetinventory(int snum)
{
    struct player_struct* p;

    p = &ps[snum];

    p->inven_icon = 0;
    p->boot_amount = 0;
    p->scuba_on = 0;
    p->scuba_amount = 0;
    p->heat_amount = 0;
    p->heat_on = 0;
    p->jetpack_on = 0;
    p->jetpack_amount = 0;
    p->shield_amount = gs.max_armour_amount;
    p->holoduke_on = nullptr;
    p->holoduke_amount = 0;
    p->firstaid_amount = 0;
    p->steroids_amount = 0;
    p->inven_icon = 0;

    if (isRR() && ud.multimode > 1 && ud.coop != 1)
    {
        p->keys[0] = 1;
        p->keys[1] = 1;
        p->keys[2] = 1;
        p->keys[3] = 1;
        p->keys[4] = 1;
    }
    else
    {
        p->keys[0] = 0;
        p->keys[1] = 0;
        p->keys[2] = 0;
        p->keys[3] = 0;
        p->keys[4] = 0;
    }

    p->drunkang = 1647;
    p->eatang = 1647;
    p->drink_amt = 0;
    p->eat = 0;
    p->drink_timer = 0;
    p->eat_timer = 0;
    p->shotgun_state[0] = 0;
    p->shotgun_state[1] = 0;
    p->detonate_time = 0;
    p->detonate_count = 0;
    p->recoil = 0;
    p->yehaa_timer = 0;
    resetlanepics();

    if (numplayers < 2)
    {
        ufospawn = std::min(ud.player_skill*4+1, 32);
        ufocnt = 0;
        hulkspawn = ud.player_skill + 1;
    }
    else
    {
        ufospawn = 32;
        ufocnt = 0;
        hulkspawn = 2;
    }
    OnEvent(EVENT_RESETINVENTORY, snum, p->GetActor());
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void resetprestat(int snum,int g)
{
    struct player_struct *p;
    short i;

    p = &ps[snum];

    spriteqloc = 0;
    for(i=0;i<spriteqamount;i++) spriteq[i] = nullptr;

    p->hbomb_on          = 0;
    p->pals.a         = 0;
    p->toggle_key_flag   = 0;
    p->secret_rooms      = 0;
    p->max_secret_rooms  = 0;
    p->actors_killed     = 0;
    p->max_actors_killed = 0;
    p->lastrandomspot = 0;
    p->oweapon_pos = p->weapon_pos = 6;
    p->okickback_pic = p->kickback_pic = 5;
    p->last_weapon = -1;
    p->weapreccnt = 0;
    p->show_empty_weapon= 0;
    p->holster_weapon = 0;
    p->last_pissed_time = 0;

    p->one_parallax_sectnum = -1;
    p->visibility = ud.const_visibility;

    screenpeek              = myconnectindex;
    numanimwalls            = 0;
    numcyclers              = 0;
    animatecnt              = 0;
    parallaxtype            = 0;
    randomseed              = 17L;
    paused             = 0;
    ud.cameraactor =nullptr;
    tempwallptr             = 0;
    camsprite               =nullptr;
    earthquaketime          = 0;

    WindTime = 0;
    WindDir = 0;
    fakebubba_spawn = 0;
    RRRA_ExitedLevel = 0;
    BellTime = 0;
    BellSprite = nullptr;

    if(p->curr_weapon == HANDREMOTE_WEAPON)
    {
        p->ammo_amount[HANDBOMB_WEAPON]++;
        p->curr_weapon = HANDBOMB_WEAPON;
    }

    p->timebeforeexit   = 0;
    p->customexitsound  = 0;

    p->stairs = 0;
    //if (!isRRRA()) p->fogtype = 0;
    p->noise_x = 131072;
    p->noise_y = 131072;
    p->donoise = 0;
    p->noise_radius = 0;

    if (isRR() && ud.multimode > 1 && ud.coop != 1)
    {
        p->keys[0] = 1;
        p->keys[1] = 1;
        p->keys[2] = 1;
        p->keys[3] = 1;
        p->keys[4] = 1;
    }
    else
    {
        p->keys[0] = 0;
        p->keys[1] = 0;
        p->keys[2] = 0;
        p->keys[3] = 0;
        p->keys[4] = 0;
    }

    p->drunkang = 1647;
    p->eatang = 1647;
    p->drink_amt = 0;
    p->eat = 0;
    p->drink_timer = 0;
    p->eat_timer = 0;
    p->shotgun_state[0] = 0;
    p->shotgun_state[1] = 0;
    p->detonate_time = 0;
    p->detonate_count = 0;
    p->recoil = 0;
    p->yehaa_timer = 0;
    resetlanepics();

    if (numplayers < 2)
    {
        ufospawn = std::min(ud.player_skill*4+1, 32);
        ufocnt = 0;
        hulkspawn = ud.player_skill + 1;
    }
    else
    {
        ufospawn = 32;
        ufocnt = 0;
        hulkspawn = 2;
    }

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void resetpspritevars(int g)
{
    int i, j;
    short circ;
    int firstx, firsty;
    spritetype* s;
    int aimmode[MAXPLAYERS];
    STATUSBARTYPE tsbar[MAXPLAYERS];

    EGS(ps[0].cursectnum, ps[0].posx, ps[0].posy, ps[0].posz,
        TILE_APLAYER, 0, 0, 0, ps[0].angle.ang.asbuild(), 0, 0, nullptr, 10);

    if (ud.recstat != 2) for (i = 0; i < MAXPLAYERS; i++)
    {
        aimmode[i] = ps[i].aim_mode;
        if (ud.multimode > 1 && ud.coop == 1 && ud.last_level >= 0)
        {
            for (j = 0; j < MAX_WEAPONS; j++)
            {
                tsbar[i].ammo_amount[j] = ps[i].ammo_amount[j];
                tsbar[i].gotweapon[j] = ps[i].gotweapon[j];
            }

            tsbar[i].shield_amount = ps[i].shield_amount;
            tsbar[i].curr_weapon = ps[i].curr_weapon;
            tsbar[i].inven_icon = ps[i].inven_icon;

            tsbar[i].firstaid_amount = ps[i].firstaid_amount;
            tsbar[i].steroids_amount = ps[i].steroids_amount;
            tsbar[i].holoduke_amount = ps[i].holoduke_amount;
            tsbar[i].jetpack_amount = ps[i].jetpack_amount;
            tsbar[i].heat_amount = ps[i].heat_amount;
            tsbar[i].scuba_amount = ps[i].scuba_amount;
            tsbar[i].boot_amount = ps[i].boot_amount;
        }
    }

    resetplayerstats(0);

    for (i = 1; i < MAXPLAYERS; i++)
        memcpy(&ps[i], &ps[0], sizeof(ps[0]));

    if (ud.recstat != 2) for (i = 0; i < MAXPLAYERS; i++)
    {
        ps[i].aim_mode = aimmode[i];
        if (ud.multimode > 1 && ud.coop == 1 && ud.last_level >= 0)
        {
            for (j = 0; j < MAX_WEAPONS; j++)
            {
                ps[i].ammo_amount[j] = tsbar[i].ammo_amount[j];
                ps[i].gotweapon[j] = tsbar[i].gotweapon[j];
            }
            ps[i].shield_amount = tsbar[i].shield_amount;
            ps[i].curr_weapon = tsbar[i].curr_weapon;
            ps[i].inven_icon = tsbar[i].inven_icon;

            ps[i].firstaid_amount = tsbar[i].firstaid_amount;
            ps[i].steroids_amount = tsbar[i].steroids_amount;
            ps[i].holoduke_amount = tsbar[i].holoduke_amount;
            ps[i].jetpack_amount = tsbar[i].jetpack_amount;
            ps[i].heat_amount = tsbar[i].heat_amount;
            ps[i].scuba_amount = tsbar[i].scuba_amount;
            ps[i].boot_amount = tsbar[i].boot_amount;
        }
    }

    numplayersprites = 0;
    circ = 2048 / ud.multimode;

    which_palookup = 9;
    j = connecthead;
    DukeStatIterator it(STAT_PLAYER);
    while (auto act = it.Next())
    {
        s = act->s;

        if (numplayersprites == MAXPLAYERS)
            I_Error("Too many player sprites (max 16.)");

        if (numplayersprites == 0)
        {
            firstx = ps[0].posx;
            firsty = ps[0].posy;
        }

        po[numplayersprites].ox = s->x;
        po[numplayersprites].oy = s->y;
        po[numplayersprites].oz = s->z;
        po[numplayersprites].oa = s->ang;
        po[numplayersprites].os = s->sectnum;

        numplayersprites++;
        if (j >= 0)
        {
            act->SetOwner(act);
            s->shade = 0;
            s->xrepeat = isRR() ? 24 : 42;
            s->yrepeat = isRR() ? 17 : 36;
            s->cstat = 1 + 256;
            s->xoffset = 0;
            s->clipdist = 64;

            if (ps[j].last_extra == 0)
            {
                ps[j].last_extra = gs.max_player_health;
                s->extra = gs.max_player_health;
            }
            else s->extra = ps[j].last_extra;

            s->yvel = j;

            if (ud.last_level == -1)
            {
                if (s->pal == 0)
                {
                    s->pal = ps[j].palookup = which_palookup;
                    ud.user_pals[j] = which_palookup;
                    which_palookup++;
                    if (which_palookup == 17) which_palookup = 9;
                }
                else ud.user_pals[j] = ps[j].palookup = s->pal;
            }
            else
                s->pal = ps[j].palookup = ud.user_pals[j];

            ps[j].i = act->GetIndex();
            ps[j].frag_ps = j;
            act->SetOwner(act);

            ps[j].bobposx = ps[j].oposx = ps[j].posx = s->x;
            ps[j].bobposy = ps[j].oposy = ps[j].posy = s->y;
            ps[j].oposz = ps[j].posz = s->z;
            s->backuppos();
            ps[j].angle.oang = ps[j].angle.ang = buildang(s->ang);

            updatesector(s->x, s->y, &ps[j].cursectnum);

            j = connectpoint2[j];

        }
        else deletesprite(act);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------
void lava_cleararrays();

void prelevel_common(int g)
{
    int i;

    auto p = &ps[screenpeek];
    p->sea_sick_stat = 0;
    ufospawnsminion = 0;
    pistonsound = 0;
    p->SlotWin = 0;
    enemysizecheat = 0;
    p->MamaEnd = 0;
    banjosound = 0;
    RRRA_ExitedLevel = 0;

    lava_cleararrays();
    geocnt = 0;
    ambientfx = 0;
    thunderon = 0;
    chickenplant = 0;
    WindTime = 0;
    WindDir = 0;
    fakebubba_spawn = 0;
    RRRA_ExitedLevel = 0;
    mamaspawn_count = currentLevel->rr_mamaspawn;
    BellTime = 0;
    BellSprite = nullptr;

    // RRRA E2L1 fog handling.
    fogactive = 0;

    resetprestat(0, g);
    numclouds = 0;

    for (auto& h : hittype) h.clear();
    memset(sectorextra, 0, sizeof(sectorextra));
    memset(shadedsector, 0, sizeof(shadedsector));
    memset(geosectorwarp, -1, sizeof(geosectorwarp));
    memset(geosectorwarp2, -1, sizeof(geosectorwarp2));
    memset(ambienthitag, -1, sizeof(ambienthitag));
    memset(ambientlotag, -1, sizeof(ambientlotag));

    for (i = 0; i < numsectors; i++)
    {
        sector[i].extra = 256;

        switch (sector[i].lotag)
        {
        case 20:
        case 22:
            if (sector[i].floorz > sector[i].ceilingz)
                sector[i].lotag |= 32768;
            continue;
        }

        if (sector[i].ceilingstat & 1)
        {
            //setupbackdrop(sector[i].ceilingpicnum);

            if (sector[i].ceilingpicnum == TILE_CLOUDYSKIES && numclouds < 127)
                clouds[numclouds++] = i;

            if (ps[0].one_parallax_sectnum == -1)
                ps[0].one_parallax_sectnum = i;
        }

        if (sector[i].lotag == 32767) //Found a secret room
        {
            ps[0].max_secret_rooms++;
            continue;
        }

        if (sector[i].lotag == -1)
        {
            ps[0].exitx = wall[sector[i].wallptr].x;
            ps[0].exity = wall[sector[i].wallptr].y;
            continue;
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void resettimevars(void)
{
    cloudclock = 0;
	PlayClock = 0;
    if (camsprite != nullptr)
        camsprite->temp_data[0] = 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void donewgame(MapRecord* map, int sk)
{
    auto p = &ps[0];
    show_shareware = 26 * 34;

    ud.player_skill = sk;
    ud.secretlevel = 0;
    ud.from_bonus = 0;

    ud.last_level = -1;

    M_ClearMenus();
    ResetGameVars();

    if (m_coop != 1)
    {
        if (isWW2GI())
        {
            for (int i = 0; i < 12/*MAX_WEAPONS*/; i++) // aboive 12 have no data defined and would crash.
            {
                if (aplWeaponWorksLike[i][0] == PISTOL_WEAPON)
                {
                    p->curr_weapon = i;
                    p->gotweapon[i] = true;
                    p->ammo_amount[i] = 48;
                }
                else if (aplWeaponWorksLike[i][0] == KNEE_WEAPON || aplWeaponWorksLike[i][0] == HANDREMOTE_WEAPON)
                {
                    p->gotweapon[i] = true;
                }
            }
        }
        else
        {
            p->curr_weapon = PISTOL_WEAPON;
            p->gotweapon[PISTOL_WEAPON] = true;
            p->gotweapon[KNEE_WEAPON] = true;
            p->ammo_amount[PISTOL_WEAPON] = 48;
            p->gotweapon[HANDREMOTE_WEAPON] = true;
            p->last_weapon = -1;
        }

        p->last_weapon = -1;
    }

    display_mirror = 0;

    if (ud.multimode > 1)
    {
        if (numplayers < 2)
        {
            connecthead = 0;
            for (int i = 0; i < MAXPLAYERS; i++) connectpoint2[i] = i + 1;
            connectpoint2[ud.multimode - 1] = -1;
        }
    }
    else
    {
        connecthead = 0;
        connectpoint2[0] = -1;
    }
}

//---------------------------------------------------------------------------
//
// the setup here is very, very sloppy, because mappings are not 1:1.
// Each portal can have multiple sectors, and even extends to unmarked
// neighboring sectors if they got the portal tile as floor or ceiling
//
//---------------------------------------------------------------------------

static void SpawnPortals()
{
    for (int i = 0; i < numwalls; i++)
    {
        if (wall[i].overpicnum == TILE_MIRROR && (wall[i].cstat & CSTAT_WALL_1WAY)) wall[i].portalflags |= PORTAL_WALL_MIRROR;
    }

    portalClear();
    int tag;
    if (!isRR()) tag = 40;
    else if (isRRRA()) tag = 150;
    else return;

    TArray<int> processedTags;
    DukeStatIterator it(STAT_RAROR);
    while (auto act = it.Next())
    {
        auto spr = act->s;
        if (spr->picnum == SECTOREFFECTOR && spr->lotag == tag)
        {
            if (processedTags.Find(spr->hitag) == processedTags.Size())
            {
                DukeStatIterator it2(STAT_RAROR);
                while (auto act2 = it2.Next())
                {
                    auto spr2 = act2->s;
                    if (spr2->picnum == SECTOREFFECTOR && spr2->lotag == tag + 1 && spr2->hitag == spr->hitag)
                    {
                        if (processedTags.Find(spr->hitag) == processedTags.Size())
                        {
                            int s1 = spr->sectnum, s2 = spr2->sectnum;
                            sector[s1].portalflags = PORTAL_SECTOR_FLOOR;
                            sector[s2].portalflags = PORTAL_SECTOR_CEILING;
                            sector[s1].portalnum = portalAdd(PORTAL_SECTOR_FLOOR, s2, spr2->x - spr->x, spr2->y - spr->y, spr->hitag);
                            sector[s2].portalnum = portalAdd(PORTAL_SECTOR_CEILING, s1, spr->x - spr2->x, spr->y - spr2->y, spr->hitag);
                            processedTags.Push(spr->hitag);
                        }
                        else
                        {
                            for (auto& p : allPortals)
                            {
                                if (p.type == PORTAL_SECTOR_FLOOR && p.dz == spr->hitag)
                                {
                                    p.targets.Push(spr2->sectnum);
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                for (auto& p : allPortals)
                {
                    if (p.type == PORTAL_SECTOR_CEILING && p.dz == spr->hitag)
                    {
                        p.targets.Push(spr->sectnum);
                    }
                }
            }
        }
    }
    // Unfortunately the above still isn't enough. We got to do one more check to add stuff to the portals.
    // There is one map where a sector neighboring a portal is not marked as part of the portal itself.
    for (int i = 0; i < numsectors; i++)
    {
        if (sector[i].floorpicnum == FOF && sector[i].portalflags != PORTAL_SECTOR_FLOOR)
        {
            for (auto& pt : allPortals)
            {
                if (pt.type == PORTAL_SECTOR_CEILING)
                {
                    for (auto& t : pt.targets)
                    {
                        if (findwallbetweensectors(i, t) >= 0)
                        {
                            sector[i].portalflags = PORTAL_SECTOR_FLOOR;
                            sector[i].portalnum = uint8_t(1 ^ (&pt - allPortals.Data()));
                            pt.targets.Push(i);
                            goto nexti;
                        }
                    }
                }
            }
        }
        else if (sector[i].ceilingpicnum == FOF && sector[i].portalflags != PORTAL_SECTOR_CEILING)
        {
            for (auto& pt : allPortals)
            {
                if (pt.type == PORTAL_SECTOR_FLOOR)
                {
                    for (auto& t : pt.targets)
                    {
                        if (findwallbetweensectors(i, t) >= 0)
                        {
                            sector[i].portalflags = PORTAL_SECTOR_CEILING;
                            sector[i].portalnum = uint8_t(1 ^ (&pt - allPortals.Data()));
                            pt.targets.Push(i);
                            goto nexti;
                        }
                    }
                }
            }
        }
    nexti:;
    }
    for (auto& p : allPortals) p.dz = 0;
    mergePortals();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static int LoadTheMap(MapRecord *mi, struct player_struct *p, int gamemode)
{
    int16_t lbang;
    if (isShareware() && (mi->flags & MI_USERMAP))
    {
        I_Error("Cannot load user maps with shareware version!\n");
    }

    currentLevel = mi;
    engineLoadBoard(mi->fileName, isShareware(), &p->pos, &lbang, &p->cursectnum);

    SECRET_SetMapName(mi->DisplayName(), mi->name);
    STAT_NewLevel(mi->fileName);

    p->angle.ang = buildang(lbang);

    memset(gotpic, 0, sizeof(gotpic));
    
    if (isRR()) prelevel_r(gamemode);
    else prelevel_d(gamemode);

    SpawnPortals();

    allignwarpelevators();
    resetpspritevars(gamemode);

    if (isRR()) cacheit_r(); else cacheit_d();
    return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void clearfrags(void)
{
    for (int i = 0; i < ud.multimode; i++)
    {
        ps[i].frag = ps[i].fraggedself = 0;
        memset(ps[i].frags, 0, sizeof(ps[i].frags));
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void enterlevel(MapRecord *mi, int gamemode)
{
//    flushpackets();
//    waitforeverybody();

    ud.respawn_monsters = ud.m_respawn_monsters;
    ud.respawn_items = ud.m_respawn_items;
    ud.respawn_inventory = ud.m_respawn_inventory;
    ud.monsters_off = ud.m_monsters_off;
    ud.coop = ud.m_coop;
    ud.ffire = ud.m_ffire;
    lastlevel = 0;

    OnEvent(EVENT_ENTERLEVEL);

    // Stop all sounds
    FX_StopAllSounds();
    FX_SetReverb(0);

    auto p = &ps[0];

    LoadTheMap(mi, p, gamemode);

    // Try this first so that it can disable the CD player if no tracks are found.
    if (isRR())
        S_PlayRRMusic();

    if (ud.recstat != 2)
    {
        S_PlayLevelMusic(mi);
    }

    for (int i = connecthead; i >= 0; i = connectpoint2[i])
    {
        bool clearweapon = !!(currentLevel->flags & LEVEL_CLEARWEAPONS);
        int pn = sector[ps[i].GetActor()->s->sectnum].floorpicnum;
        if (pn == TILE_HURTRAIL || pn == TILE_FLOORSLIME || pn == TILE_FLOORPLASMA)
        {
            resetinventory(i);
            clearweapon = true;
        }
        if (clearweapon)
        {
            resetweapons(i);
            ps[i].gotweapon[PISTOL_WEAPON] = false;
            ps[i].ammo_amount[PISTOL_WEAPON] = 0;
            ps[i].curr_weapon = KNEE_WEAPON;
            ps[i].kickback_pic = 0;
            ps[i].okickback_pic = ps[i].kickback_pic = 0;
        }
        if (currentLevel->flags & LEVEL_CLEARINVENTORY) resetinventory(i);
    }
    resetmys();

    everyothertime = 0;
    global_random = 0;

    ud.last_level = 1;
    ps[myconnectindex].over_shoulder_on = 0;
    clearfrags();
    resettimevars();  // Here we go
	setLevelStarted(mi);
    if (isRRRA() && ps[screenpeek].sea_sick_stat == 1)
    {
        for (int i = 0; i < MAXWALLS; i++)
        {
            if (wall[i].picnum == 7873 || wall[i].picnum == 7870)
                StartInterpolation(i, Interp_Wall_PanX);
        }
    }
}

//---------------------------------------------------------------------------
//
// Start a new game from the menu
//
//---------------------------------------------------------------------------

void GameInterface::NewGame(MapRecord* map, int skill, bool)
{
    for (int i = 0; i != -1; i = connectpoint2[i])
    {
        resetweapons(i);
        resetinventory(i);
    }

    ps[0].last_extra = gs.max_player_health;


    if (skill == -1) skill = ud.player_skill;
    else skill++;
    ud.player_skill = skill;
    ud.m_respawn_monsters = (skill == 4);
    ud.m_monsters_off = ud.monsters_off = 0;
    ud.m_respawn_items = 0;
    ud.m_respawn_inventory = 0;
    ud.multimode = 1;

    donewgame(map, skill);
    enterlevel(map, 0);
    if (isShareware() && ud.recstat != 2) FTA(QUOTE_F1HELP, &ps[myconnectindex]);

    PlayerColorChanged();
    inputState.ClearAllInput();
    gameaction = ga_level;
}

//---------------------------------------------------------------------------
//
// Ideally this will become the only place where map progression gets set up.
//
//---------------------------------------------------------------------------

bool setnextmap(bool checksecretexit)
{
    MapRecord* map = nullptr;
    MapRecord* from_bonus = nullptr;

    if (ud.eog && !(currentLevel->flags & LEVEL_FORCENOEOG))
    {
    }
    else if (checksecretexit && ud.from_bonus == 0)
    {
        if (ud.secretlevel > 0)
        {
            // allow overriding the secret exit destination to make episode compilation easier with maps containing secret exits.
            if (currentLevel->flags & LEVEL_SECRETEXITOVERRIDE) map = FindNextSecretMap(currentLevel);
            if (!map) map = FindMapByIndex(currentLevel->cluster, ud.secretlevel);

            if (map)
            {
                from_bonus = FindNextMap(currentLevel);
            }
        }
    }
    else if (ud.from_bonus && currentLevel->NextMap.IsEmpty())	// if the current level has an explicit link, use that instead of ud.from_bonus.
    {
        map = FindMapByLevelNum(ud.from_bonus);
    }
    else
    {
        map = FindNextMap(currentLevel);
    }

    // Make sure these two are cleared in case the map check errors out.
    ud.from_bonus = 0;
    ud.secretlevel = 0;
    if (map)
    {
        // If the map doesn't exist, abort with a meaningful message instead of crashing.
        if (fileSystem.FindFile(map->fileName) < 0)
        {
            I_Error("Trying to open non-existent %s", map->fileName.GetChars());
        }
        ud.from_bonus = from_bonus? from_bonus->levelNumber : 0;
    }
    CompleteLevel(map);
    return false;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void exitlevel(MapRecord* nextlevel)
{
    bool endofgame = nextlevel == nullptr;
    STAT_Update(endofgame);
    StopCommentary();

    SummaryInfo info{};

    info.kills = ps[0].actors_killed;
    info.maxkills = ps[0].max_actors_killed;
    info.secrets = ps[0].secret_rooms;
    info.maxsecrets = ps[0].max_secret_rooms;
    info.time = ps[0].player_par / GameTicRate;
    info.endofgame = endofgame;
    Mus_Stop();

    if (playerswhenstarted > 1 && ud.coop != 1)
        {
        // MP scoreboard
        ShowScoreboard(playerswhenstarted, [=](bool)
            {
            // Clear potentially loaded per-map ART only after the bonus screens.
            artClearMapArt();
            gameaction = ga_level;
            ud.eog = false;
            if (endofgame)
            {
                    auto nextlevel = FindMapByLevelNum(0);
                    if (!nextlevel)
                    {
                        gameaction = ga_startup;
                        return;
                    }
                    else gameaction = ga_nextlevel;
                }
                else
                gameaction = ga_nextlevel;

        });
    }
    else if (ud.multimode <= 1)
    {
        // SP cutscene + summary
        ShowIntermission(currentLevel, nextlevel, &info, [=](bool)
        {
            // Clear potentially loaded per-map ART only after the bonus screens.
            artClearMapArt();
            ud.eog = false;
            gameaction = endofgame? ga_startup : ga_nextlevel;
        });
    }
}


END_DUKE_NS  
