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

void premapcontroller(DDukeActor* ac)
{
	switch (ac->spr.picnum)
	{
	case ACTIVATOR:
	case ACTIVATORLOCKED:
	case LOCATORS:
	case MASTERSWITCH:
	case MUSICANDSFX:
	case RESPAWN:
	case SECTOREFFECTOR:
	case TOUCHPLATE:
		ac->spr.cstat &= ~(CSTAT_SPRITE_BLOCK | CSTAT_SPRITE_BLOCK_HITSCAN | CSTAT_SPRITE_ALIGNMENT_MASK);
		break;

	case GPSPEED:
		ac->sector()->extra = ac->spr.lotag;
		deletesprite(ac);
		break;

	case CYCLER:
		if (numcyclers >= MAXCYCLERS)
			I_Error("Too many cycling sectors.");
		cyclers[numcyclers].sector = ac->sector();
		cyclers[numcyclers].lotag = ac->spr.lotag;
		cyclers[numcyclers].shade1 = ac->spr.shade;
		cyclers[numcyclers].shade2 = ac->sector()->floorshade;
		cyclers[numcyclers].hitag = ac->spr.hitag;
		cyclers[numcyclers].state = (ac->int_ang() == 1536);
		numcyclers++;
		deletesprite(ac);
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void pickrandomspot(int snum)
{
	player_struct* p;
	int i;

	p = &ps[snum];

	if( ud.multimode > 1 && ud.coop != 1)
		i = krand()%numplayersprites;
	else i = snum;

	p->pos = po[i].opos;
	p->backupxyz();
	p->setbobpos();
	p->angle.oang = p->angle.ang = DAngle::fromBuild(po[i].oa);
	p->setCursector(po[i].os);
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void resetplayerstats(int snum)
{
	player_struct* p;

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
	p->otipincs = p->tipincs = 0;
	p->buttonpalette    = 0;
	p->actorsqu         =nullptr;
	p->invdisptime      = 0;
	p->refresh_inventory= 0;
	p->last_pissed_time = 0;
	p->holster_weapon   = 0;
	p->pycount          = 0;
	p->pyoff            = 0;
	p->opyoff           = 0;
	p->oloogcnt = p->loogcnt = 0;
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

	p->angle.olook_ang = p->angle.look_ang = DAngle::fromBuild(512 - (((~currentLevel->levelNumber) & 1) << 10));
	p->angle.orotscrnang = p->angle.rotscrnang = nullAngle;

	p->newOwner          =nullptr;
	p->jumping_counter   = 0;
	p->hard_landing      = 0;
	p->vel.X             = 0;                           //!!
	p->vel.Y             = 0;
	p->vel.Z             = 0;
	p->fric.X            = 0;
	p->fric.Y            = 0;
	p->somethingonplayer = nullptr;
	p->angle.spin        = nullAngle;

	p->on_crane          = nullptr;

	if(p->curr_weapon == PISTOL_WEAPON)
		p->okickback_pic = p->kickback_pic  = isRR()? 22 : 5;
	else p->okickback_pic = p->kickback_pic = 0;

	p->oweapon_pos = p->weapon_pos        = 6;
	p->walking_snd_toggle= 0;
	p->weapon_ang        = 0;

	p->knuckle_incs      = 1;
	p->ofist_incs = p->fist_incs = 0;
	p->oknee_incs = p->knee_incs = 0;
	p->stairs = 0;
	p->noise.X = 0;
	p->noise.Y = 0;
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
		ufospawn = isRRRA()? 3 : min(ud.player_skill*4+1, 32);
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
	int weapon;
	player_struct* p;

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
	p->ammo_amount[PISTOL_WEAPON] = min<int16_t>(gs.max_ammo_amount[PISTOL_WEAPON], 48);
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
	player_struct* p;

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
		ufospawn = min(ud.player_skill*4+1, 32);
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
	player_struct* p;
	int i;

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

	p->one_parallax_sectnum = nullptr;
	p->visibility = ud.const_visibility;

	screenpeek              = myconnectindex;
	numanimwalls            = 0;
	numcyclers              = 0;
	animatecnt              = 0;
	randomseed              = 17L;
	paused             = 0;
	ud.cameraactor =nullptr;
	tempwallptr             = 0;
	cranes.Clear();
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
	p->noise.X = 131072;
	p->noise.Y = 131072;
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
		ufospawn = min(ud.player_skill*4+1, 32);
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
	int circ;
	int firstx, firsty;
	int aimmode[MAXPLAYERS];
	STATUSBARTYPE tsbar[MAXPLAYERS];

	EGS(ps[0].cursector, ps[0].player_int_pos().X, ps[0].player_int_pos().Y, ps[0].player_int_pos().Z,
		TILE_APLAYER, 0, 0, 0, ps[0].angle.ang.Buildang(), 0, 0, nullptr, 10);

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
		ps[i] = ps[0];

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
		if (numplayersprites == MAXPLAYERS)
			I_Error("Too many player sprites (max 16.)");

		if (numplayersprites == 0)
		{
			firstx = ps[0].player_int_pos().X;
			firsty = ps[0].player_int_pos().Y;
		}

		po[numplayersprites].opos = act->spr.pos;
		po[numplayersprites].oa = act->int_ang();
		po[numplayersprites].os = act->sector();

		numplayersprites++;
		if (j >= 0)
		{
			act->SetOwner(act);
			act->spr.shade = 0;
			act->spr.xrepeat = isRR() ? 24 : 42;
			act->spr.yrepeat = isRR() ? 17 : 36;
			act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
			act->spr.xoffset = 0;
			act->spr.clipdist = 64;

			if (ps[j].last_extra == 0)
			{
				ps[j].last_extra = gs.max_player_health;
				act->spr.extra = gs.max_player_health;
			}
			else act->spr.extra = ps[j].last_extra;

			act->spr.yvel = j;

			if (ud.last_level == -1)
			{
				if (act->spr.pal == 0)
				{
					act->spr.pal = ps[j].palookup = which_palookup;
					ud.user_pals[j] = which_palookup;
					which_palookup++;
					if (which_palookup == 17) which_palookup = 9;
				}
				else ud.user_pals[j] = ps[j].palookup = act->spr.pal;
			}
			else
				act->spr.pal = ps[j].palookup = ud.user_pals[j];

			ps[j].actor = act;
			ps[j].frag_ps = j;
			act->SetOwner(act);

			ps[j].getposfromactor(act);
			ps[j].backupxyz();
			ps[j].setbobpos();
			act->backuppos();
			ps[j].angle.oang = ps[j].angle.ang = act->spr.angle;

			updatesector(act->spr.pos, &ps[j].cursector);

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

	memset(geosectorwarp, -1, sizeof(geosectorwarp));
	memset(geosectorwarp2, -1, sizeof(geosectorwarp2));
	memset(ambienthitag, -1, sizeof(ambienthitag));
	memset(ambientlotag, -1, sizeof(ambientlotag));

	for(auto&sec: sector)
	{
		auto sectp = &sec;
		sectp->extra = 256;

		switch (sectp->lotag)
		{
		case 20:
		case 22:
			if (sectp->floorz > sectp->ceilingz)
				sectp->lotag |= 32768;
			continue;
		}

		if (sectp->ceilingstat & CSTAT_SECTOR_SKY)
		{
			//setupbackdrop(sectp->ceilingpicnum);

			if (sectp->ceilingpicnum == TILE_CLOUDYSKIES && numclouds < 127)
				clouds[numclouds++] = sectp;

			if (ps[0].one_parallax_sectnum == nullptr)
				ps[0].one_parallax_sectnum = sectp;
		}

		if (sectp->lotag == 32767) //Found a secret room
		{
			ps[0].max_secret_rooms++;
			continue;
		}

		if (sectp->lotag == -1)
		{
			ps[0].exit.X = sectp->firstWall()->wall_int_pos().X;
			ps[0].exit.Y = sectp->firstWall()->wall_int_pos().Y;
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
				if (aplWeaponWorksLike(i, 0) == PISTOL_WEAPON)
				{
					p->curr_weapon = i;
					p->gotweapon[i] = true;
					p->ammo_amount[i] = 48;
				}
				else if (aplWeaponWorksLike(i, 0) == KNEE_WEAPON || aplWeaponWorksLike(i, 0) == HANDREMOTE_WEAPON)
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
	for (auto& wal : wall)
	{
		if (wal.overpicnum == TILE_MIRROR && (wal.cstat & CSTAT_WALL_1WAY)) wal.portalflags |= PORTAL_WALL_MIRROR;
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
		if (act->spr.picnum == SECTOREFFECTOR && act->spr.lotag == tag)
		{
			if (processedTags.Find(act->spr.hitag) == processedTags.Size())
			{
				DukeStatIterator it2(STAT_RAROR);
				while (auto act2 = it2.Next())
				{
					if (act2->spr.picnum == SECTOREFFECTOR && act2->spr.lotag == tag + 1 && act2->spr.hitag == act->spr.hitag)
					{
						if (processedTags.Find(act->spr.hitag) == processedTags.Size())
						{
							sectortype* s1 = act->sector(), *s2 = act2->sector();
							s1->portalflags = PORTAL_SECTOR_FLOOR;
							s2->portalflags = PORTAL_SECTOR_CEILING;
							s1->portalnum = portalAdd(PORTAL_SECTOR_FLOOR, sectnum(s2), act2->int_pos().X - act->int_pos().X, act2->int_pos().Y - act->int_pos().Y, act->spr.hitag);
							s2->portalnum = portalAdd(PORTAL_SECTOR_CEILING, sectnum(s1), act->int_pos().X - act2->int_pos().X, act->int_pos().Y - act2->int_pos().Y, act->spr.hitag);
							processedTags.Push(act->spr.hitag);
						}
						else
						{
							for (auto& p : allPortals)
							{
								if (p.type == PORTAL_SECTOR_FLOOR && p.dz == act->spr.hitag)
								{
									p.targets.Push(act2->sectno());
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
					if (p.type == PORTAL_SECTOR_CEILING && p.dz == act->spr.hitag)
					{
						p.targets.Push(act->sectno());
					}
				}
			}
		}
	}
	// Unfortunately the above still isn't enough. We got to do one more check to add stuff to the portals.
	// There is one map where a sector neighboring a portal is not marked as part of the portal itself.
	for (unsigned i = 0; i < sector.Size(); i++)
	{
		auto sectp = &sector[i];
		if (sectp->floorpicnum == FOF && sectp->portalflags != PORTAL_SECTOR_FLOOR)
		{
			for (auto& pt : allPortals)
			{
				if (pt.type == PORTAL_SECTOR_CEILING)
				{
					for (auto& t : pt.targets)
					{
						if (sectorsConnected(i, t))
						{
							sectp->portalflags = PORTAL_SECTOR_FLOOR;
							sectp->portalnum = uint8_t(1 ^ (&pt - allPortals.Data()));
							pt.targets.Push(i);
							goto nexti;
						}
					}
				}
			}
		}
		else if (sectp->ceilingpicnum == FOF && sectp->portalflags != PORTAL_SECTOR_CEILING)
		{
			for (auto& pt : allPortals)
			{
				if (pt.type == PORTAL_SECTOR_FLOOR)
				{
					for (auto t : pt.targets)
					{
						if (sectorsConnected(i, t))
						{
							sectp->portalflags = PORTAL_SECTOR_CEILING;
							sectp->portalnum = uint8_t(1 ^ (&pt - allPortals.Data()));
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
// this is just a dummy for now to provide the intended setup.
//
//---------------------------------------------------------------------------

static TArray<DDukeActor*> spawnactors(SpawnSpriteDef& sprites)
{
	TArray<DDukeActor*> spawns(sprites.sprites.Size(), true);
	InitSpriteLists();
	int j = 0;
	for (unsigned i = 0; i < sprites.sprites.Size(); i++)
	{
		if (sprites.sprites[i].statnum == MAXSTATUS)
		{
			spawns.Pop();
			continue;
		}
		auto sprt = &sprites.sprites[i];
		auto actor = static_cast<DDukeActor*>(InsertActor(RUNTIME_CLASS(DDukeActor), sprt->sectp, sprt->statnum));
		spawns[j++] = actor;
		actor->spr = sprites.sprites[i];
		actor->time = i;
		if (sprites.sprext.Size()) actor->sprext = sprites.sprext[i];
		else actor->sprext = {};
		actor->spsmooth = {};
	}
	leveltimer = sprites.sprites.Size();
	return spawns;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static int LoadTheMap(MapRecord *mi, player_struct*p, int gamemode)
{
	int16_t lbang;
	if (isShareware() && (mi->flags & MI_USERMAP))
	{
		I_Error("Cannot load user maps with shareware version!\n");
	}

	currentLevel = mi;
	int sect;
	SpawnSpriteDef sprites;
	DVector3 pos;
	loadMap(mi->fileName, isShareware(), &pos, &lbang, &sect, sprites);
	p->pos = pos;
	p->cursector = &sector[sect];

	SECRET_SetMapName(mi->DisplayName(), mi->name);
	STAT_NewLevel(mi->fileName);
	TITLE_InformName(mi->name);
	
	p->angle.ang = DAngle::fromBuild(lbang);

	gotpic.Zero();

	auto actorlist = spawnactors(sprites);

	if (isRR()) prelevel_r(gamemode, actorlist);
	else prelevel_d(gamemode, actorlist);

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
		int pn = ps[i].GetActor()->sector()->floorpicnum;
		if (gs.tileinfo[pn].flags & TFLAG_CLEARINVENTORY)
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
		for (auto& wal : wall)
		{
			if (wal.picnum == 7873 || wal.picnum == 7870)
				StartInterpolation(&wal, Interp_Wall_PanX);
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
