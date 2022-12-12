//-------------------------------------------------------------------------
/*
Copyright (C) 2020 - Christoph Oelckers

This is free software; you can redistribute it and/or
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

*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "global.h"
#include "dukeactor.h"

BEGIN_DUKE_NS

//---------------------------------------------------------------------------
//
// Dispatcher for functions where different variants exist for the two families of games.
//
//---------------------------------------------------------------------------

void initactorflags_d();
void initactorflags_r();

bool checkaccessswitch_d(int snum, int pal, DDukeActor *act, walltype* w);
bool checkaccessswitch_r(int snum, int pal, DDukeActor* act, walltype* w);
void activatebysector_d(sectortype* sect, DDukeActor* j);
void activatebysector_r(sectortype* sect, DDukeActor* j);
void checkhitsprite_d(DDukeActor* i, DDukeActor* sn);
void checkhitsprite_r(DDukeActor* i, DDukeActor* sn);
void checkhitdefault_d(DDukeActor* i, DDukeActor* sn);
void checkhitdefault_r(DDukeActor* i, DDukeActor* sn);
void checksectors_d(int snum);
void checksectors_r(int snum);

void addweapon_d(player_struct* p, int weapon, bool wswitch);
void addweapon_r(player_struct* p, int weapon, bool wswitch);
void hitradius_d(DDukeActor* i, int  r, int  hp1, int  hp2, int  hp3, int  hp4);
void hitradius_r(DDukeActor* i, int  r, int  hp1, int  hp2, int  hp3, int  hp4);
void lotsofmoney_d(DDukeActor* s, int n);
void lotsofmail_d(DDukeActor* s, int n);
void lotsofpaper_d(DDukeActor* s, int n);
void lotsoffeathers_r(DDukeActor* s, int n);
int ifhitbyweapon_r(DDukeActor* sn);
int ifhitbyweapon_d(DDukeActor* sn);
void fall_d(DDukeActor* i, int g_p);
void fall_r(DDukeActor* i, int g_p);
bool spawnweapondebris_d(int picnum);
bool spawnweapondebris_r(int picnum);
void move_d(DDukeActor* i, int g_p, int g_x);
void move_r(DDukeActor* i, int g_p, int g_x);
void incur_damage_d(player_struct* p);
void incur_damage_r(player_struct* p);
void shoot_d(DDukeActor* i, int atwith, PClass* cls);
void shoot_r(DDukeActor* i, int atwith, PClass* cls);
void selectweapon_d(int snum, int j);
void selectweapon_r(int snum, int j);
int doincrements_d(player_struct* p);
int doincrements_r(player_struct* p);
void checkweapons_d(player_struct* p);
void checkweapons_r(player_struct* p);
void processinput_d(int snum);
void processinput_r(int snum);
void displayweapon_d(int snum, double interpfrac);
void displayweapon_r(int snum, double interpfrac);
void displaymasks_d(int snum, int p, double interpfrac);
void displaymasks_r(int snum, int p, double interpfrac);
void think_d();
void think_r();
void movetransports_d();
void movetransports_r();
void animatesprites_d(tspriteArray& tsprites, const DVector2& viewVec, DAngle viewang, double interpfrac);
void animatesprites_r(tspriteArray& tsprites, const DVector2& viewVec, DAngle viewang, double interpfrac);

Dispatcher fi;

void SetDispatcher()
{
	if (!isRR())
	{
		fi = {
		think_d,
		movetransports_d,
		initactorflags_d,
		checkaccessswitch_d,
		activatebysector_d,
		checkhitsprite_d,
		checkhitdefault_d,
		checksectors_d,
		spawninit_d,

		addweapon_d,
		hitradius_d,
		lotsofmoney_d,
		lotsofmail_d,
		lotsofpaper_d,
		ifhitbyweapon_d,
		fall_d,
		spawnweapondebris_d,
		move_d,

		incur_damage_d,
		shoot_d,
		selectweapon_d,
		doincrements_d,
		checkweapons_d,
		processinput_d,
		displayweapon_d,
		displaymasks_d,
		animatesprites_d,
		};
	}
	else
	{
		fi = {
		think_r,
		movetransports_r,
		initactorflags_r,
		checkaccessswitch_r,
		activatebysector_r,
		checkhitsprite_r,
		checkhitdefault_r,
		checksectors_r,
		spawninit_r,

		addweapon_r,
		hitradius_r,
		lotsoffeathers_r,
		lotsoffeathers_r,
		lotsoffeathers_r,
		ifhitbyweapon_r,
		fall_r,
		spawnweapondebris_r,
		move_r,

		incur_damage_r,
		shoot_r,
		selectweapon_r,
		doincrements_r,
		checkweapons_r,
		processinput_r,
		displayweapon_r,
		displaymasks_r,
		animatesprites_r,
		};
	}
}


int TILE_APLAYER;
int TILE_DRONE;
int TILE_SCREENBORDER;
int TILE_VIEWBORDER;
int TILE_APLAYERTOP;
int TILE_CAMCORNER;
int TILE_CAMLIGHT;
int TILE_STATIC;
int TILE_BOTTOMSTATUSBAR;
int TILE_THREEDEE;
int TILE_INGAMEDUKETHREEDEE;
int TILE_FIRE;
int TILE_WATERBUBBLE;
int TILE_SMALLSMOKE;
int TILE_BLOODPOOL;
int TILE_CLOUDYSKIES;
int TILE_HEN;
int TILE_MIRRORBROKE;
int TILE_LOADSCREEN;
int TILE_CROSSHAIR;
int TILE_EGG;

END_DUKE_NS
