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
#include "sbar.h"
#include "automap.h"
#include "dukeactor.h"

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
    p->dummyplayersprite=-1;
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
    p->palette = 0;

    if(p->steroids_amount < 400 )
    {
        p->steroids_amount = 0;
        p->inven_icon = 0;
    }
    p->heat_on =            0;
    p->jetpack_on =         0;
    p->holoduke_on =       nullptr;

    p->angle.olook_ang = p->angle.look_ang = buildlook(512 - ((currentLevel->levelNumber & 1) << 10));
    p->angle.orotscrnang = p->angle.rotscrnang = buildlook(0);

    p->newowner          =-1;
    p->jumping_counter   = 0;
    p->hard_landing      = 0;
    p->posxv             = 0;                           //!!
    p->posyv             = 0;
    p->poszv             = 0;
    p->fric.x            = 0;
    p->fric.y            = 0;
    p->somethingonplayer =nullptr;
    p->angle.spin        = bamlook(0);

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
    setpal(p);
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
        p->gotweapon.Clear(MOTORCYCLE_WEAPON);
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
        p->gotweapon.Clear(BOAT_WEAPON);
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

    p->gotweapon.Zero();
    p->oweapon_pos = p->weapon_pos = 6;
    p->okickback_pic = p->kickback_pic = 5;
    p->curr_weapon = PISTOL_WEAPON;
    p->gotweapon.Set(PISTOL_WEAPON);
    p->gotweapon.Set(KNEE_WEAPON);
    p->ammo_amount[PISTOL_WEAPON] = std::min<int16_t>(max_ammo_amount[PISTOL_WEAPON], 48);
    p->gotweapon.Set(HANDREMOTE_WEAPON);
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
        p->gotweapon.Set(SLINGBLADE_WEAPON);
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
    p->shield_amount = max_armour_amount;
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
    ud.camerasprite         =-1;
    tempwallptr             = 0;
    camsprite               =-1;
    earthquaketime          = 0;

    WindTime = 0;
    WindDir = 0;
    fakebubba_spawn = 0;
    RRRA_ExitedLevel = 0;
    BellTime = 0;
    BellSprite = 0;

    numinterpolations = 0;
    //startofdynamicinterpolations = 0;

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
    int aimmode[MAXPLAYERS], autoaim[MAXPLAYERS];
    STATUSBARTYPE tsbar[MAXPLAYERS];

    EGS(ps[0].cursectnum, ps[0].posx, ps[0].posy, ps[0].posz,
        TILE_APLAYER, 0, 0, 0, ps[0].angle.ang.asbuild(), 0, 0, nullptr, 10);

    if (ud.recstat != 2) for (i = 0; i < MAXPLAYERS; i++)
    {
        aimmode[i] = ps[i].aim_mode;
        autoaim[i] = ps[i].auto_aim;
        if (ud.multimode > 1 && ud.coop == 1 && ud.last_level >= 0)
        {
            for (j = 0; j < MAX_WEAPONS; j++)
            {
                tsbar[i].ammo_amount[j] = ps[i].ammo_amount[j];
                tsbar[i].gotweapon.Set(j, ps[i].gotweapon[j]);
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
        ps[i].auto_aim = autoaim[i];
        if (ud.multimode > 1 && ud.coop == 1 && ud.last_level >= 0)
        {
            for (j = 0; j < MAX_WEAPONS; j++)
            {
                ps[i].ammo_amount[j] = tsbar[i].ammo_amount[j];
                ps[i].gotweapon.Set(j, tsbar[i].gotweapon[j]);
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
    StatIterator it(STAT_PLAYER);
    while ((i = it.NextIndex()) >= 0)
    {
        s = &sprite[i];

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
            s->owner = i;
            s->shade = 0;
            s->xrepeat = isRR() ? 24 : 42;
            s->yrepeat = isRR() ? 17 : 36;
            s->cstat = 1 + 256;
            s->xoffset = 0;
            s->clipdist = 64;

            if (ps[j].last_extra == 0)
            {
                ps[j].last_extra = max_player_health;
                s->extra = max_player_health;
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

            ps[j].i = i;
            ps[j].frag_ps = j;
            hittype[i].owner = i;

            hittype[i].bposx = ps[j].bobposx = ps[j].oposx = ps[j].posx = s->x;
            hittype[i].bposy = ps[j].bobposy = ps[j].oposy = ps[j].posy = s->y;
            hittype[i].bposz = ps[j].oposz = ps[j].posz = s->z;
            ps[j].angle.oang = ps[j].angle.ang = buildang(s->ang);

            updatesector(s->x, s->y, &ps[j].cursectnum);

            j = connectpoint2[j];

        }
        else deletesprite(i);
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
    mamaspawn_count = 15;
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
    mamaspawn_count = 15;
    BellTime = 0;
    BellSprite = 0;

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
	ud.levelclock = 0;
    if (camsprite >= 0)
        hittype[camsprite].temp_data[0] = 0;
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

    //ud.nextLevel = map;
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
                    p->gotweapon.Set(i);
                    p->ammo_amount[i] = 48;
                }
                else if (aplWeaponWorksLike[i][0] == KNEE_WEAPON || aplWeaponWorksLike[i][0] == HANDREMOTE_WEAPON)
                {
                    p->gotweapon.Set(i);
                }
            }
        }
        else
        {
            p->curr_weapon = PISTOL_WEAPON;
            p->gotweapon.Set(PISTOL_WEAPON);
            p->gotweapon.Set(KNEE_WEAPON);
            p->ammo_amount[PISTOL_WEAPON] = 48;
            p->gotweapon.Set(HANDREMOTE_WEAPON);
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

template<class func>
void newgame(MapRecord* map, int sk, func completion)
{
    auto completion1 = [=](bool res)
    {
        if (!isRR() && map->levelNumber == levelnum(3, 0) && (ud.multimode < 2))
        {
            e4intro([=](bool) { donewgame(map, sk); completion(res); });
        }
        else
        {
            donewgame(map, sk);
            completion(res);
        }
    };

    if (ud.m_recstat != 2 && ud.last_level >= 0 && ud.multimode > 1 && ud.coop != 1)
        dobonus(1, completion1);

#if 0 // this is one lousy hack job that's hopefully not needed anymore.
    else if (isRR() && !isRRRA() && map->levelNumber == levelnum(0, 6))
        dobonus(0, completion1);
#endif

    else completion1(false);
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

    engineLoadBoard(mi->fileName, isShareware(), &p->pos, &lbang, &p->cursectnum);

    currentLevel = mi;
    SECRET_SetMapName(mi->DisplayName(), mi->name);
    STAT_NewLevel(mi->fileName);

    if (isRR() && !isRRRA() && mi->levelNumber == levelnum(1, 1))
    {
        for (int i = PISTOL_WEAPON; i < MAX_WEAPONS; i++)
            ps[0].ammo_amount[i] = 0;
        ps[0].gotweapon.Clear(KNEE_WEAPON);
    }
    p->angle.ang = buildang(lbang);

    memset(gotpic, 0, sizeof(gotpic));
    
    if (isRR()) prelevel_r(gamemode);
    else prelevel_d(gamemode);

    if (isRRRA() && mi->levelNumber == levelnum(2, 0))
    {
        for (int i = PISTOL_WEAPON; i < MAX_WEAPONS; i++)
            ps[0].ammo_amount[i] = 0;
        ps[0].gotweapon.Clear(KNEE_WEAPON);
        ps[0].gotweapon.Set(SLINGBLADE_WEAPON);
        ps[0].ammo_amount[SLINGBLADE_WEAPON] = 1;
        ps[0].curr_weapon = SLINGBLADE_WEAPON;
    }

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
    }
    memset(frags, 0, sizeof(frags));
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

    OnEvent(EVENT_ENTERLEVEL);

    // Stop all sounds
    S_ResumeSound(false);
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

    if (isShareware() && mi->levelNumber == 0 && ud.recstat != 2) FTA(QUOTE_F1HELP, &ps[myconnectindex]);

    for (int i = connecthead; i >= 0; i = connectpoint2[i])
    {
        int pn = sector[sprite[ps[i].i].sectnum].floorpicnum;
        if (pn == TILE_HURTRAIL || pn == TILE_FLOORSLIME || pn == TILE_FLOORPLASMA)
        {
            resetweapons(i);
            resetinventory(i);
            ps[i].gotweapon.Clear(PISTOL_WEAPON);
            ps[i].ammo_amount[PISTOL_WEAPON] = 0;
            ps[i].curr_weapon = KNEE_WEAPON;
            ps[i].okickback_pic = ps[i].kickback_pic = 0;
        }
    }
    resetmys();
    setpal(&ps[myconnectindex]);

    everyothertime = 0;
    global_random = 0;

    ud.last_level = currentLevel->levelNumber;
    for (int i=numinterpolations-1; i>=0; i--) bakipos[i] = *curipos[i];
    ps[myconnectindex].over_shoulder_on = 0;
    clearfrags();
    resettimevars();  // Here we go
	setLevelStarted(mi);
}

//---------------------------------------------------------------------------
//
// Start a new game from the menu
//
//---------------------------------------------------------------------------

void startnewgame(MapRecord* map, int skill)
{
    if (skill == -1) skill = ud.player_skill;
    ud.player_skill = skill;
    ud.m_respawn_monsters = (skill == 4);
    ud.m_monsters_off = ud.monsters_off = 0;
    ud.m_respawn_items = 0;
    ud.m_respawn_inventory = 0;
    ud.multimode = 1;

    newgame(map, skill, [=](bool)
        {
            enterlevel(map, 0);
            ud.showweapons = cl_showweapon;
            setlocalplayerinput(&ps[myconnectindex]);
            PlayerColorChanged();
            inputState.ClearAllInput();
            gameaction = ga_level;
        });
}

//---------------------------------------------------------------------------
//
// Ideally this will become the only place where map progression gets set up.
//
//---------------------------------------------------------------------------

bool setnextmap(bool checksecretexit)
{
    MapRecord* map = nullptr;;
    int from_bonus = 0;

    if (ud.eog)
    {
    }
    else if (checksecretexit && ud.from_bonus == 0)
    {
        if (ud.secretlevel > 0)
        {
            int newlevnum = levelnum(volfromlevelnum(currentLevel->levelNumber), ud.secretlevel-1);
            map = FindMapByLevelNum(newlevnum);
            if (map)
            {
                from_bonus = currentLevel->levelNumber + 1;
            }
        }
    }
    else if (ud.from_bonus && currentLevel->nextLevel == -1)	// if the current level has an explicit link, use that instead of ud.from_bonus.
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
        ud.from_bonus = from_bonus;
    }
    CompleteLevel(map);
    return false;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void exitlevel(MapRecord *nextlevel)
{
    bool endofgame = nextlevel == nullptr;
    STAT_Update(endofgame);
    setpal(&ps[myconnectindex]);
    StopCommentary();

    dobonus(endofgame? -1 : 0, [=](bool)
        {

            // Clear potentially loaded per-map ART only after the bonus screens.
            artClearMapArt();
            gameaction = ga_level;
            ud.eog = false;
            if (endofgame)
            {
                if (ud.multimode < 2)
                {
                    if (isShareware())
                        doorders([](bool) { gameaction = ga_startup; });
                    else gameaction = ga_startup;
                    return;
                }
                else
                {
                    auto nextlevel = FindMapByLevelNum(0);
                    if (!nextlevel)
                    {
                        gameaction = ga_startup;
                        return;
                    }
                    else gameaction = ga_nextlevel;
                }
            }
            else 
                gameaction = ga_nextlevel;

        });
}


END_DUKE_NS  
