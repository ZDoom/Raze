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

bool isadoorwall_d(int dapic);
bool isadoorwall_r(int dapic);
void animatewalls_d(void);
void animatewalls_r(void);
void operaterespawns_d(int low);
void operaterespawns_r(int low);
void operateforcefields_r(DDukeActor* act, int low);
void operateforcefields_d(DDukeActor* act, int low);
bool checkhitswitch_d(int snum, walltype* w, DDukeActor *act);
bool checkhitswitch_r(int snum, walltype* w, DDukeActor* act);
void activatebysector_d(sectortype* sect, DDukeActor* j);
void activatebysector_r(sectortype* sect, DDukeActor* j);
void checkhitwall_d(DDukeActor* spr, walltype* dawall, const DVector3& pos, int atwith);
void checkhitwall_r(DDukeActor* spr, walltype* dawall, const DVector3& pos, int atwith);
bool checkhitceiling_d(sectortype* sn);
bool checkhitceiling_r(sectortype* sn);
void checkhitsprite_d(DDukeActor* i, DDukeActor* sn);
void checkhitsprite_r(DDukeActor* i, DDukeActor* sn);
void checksectors_d(int snum);
void checksectors_r(int snum);

bool ceilingspace_d(sectortype*);
bool ceilingspace_r(sectortype*);
bool floorspace_d(sectortype*);
bool floorspace_r(sectortype*);
void addweapon_d(player_struct* p, int weapon);
void addweapon_r(player_struct* p, int weapon);
void hitradius_d(DDukeActor* i, int  r, int  hp1, int  hp2, int  hp3, int  hp4);
void hitradius_r(DDukeActor* i, int  r, int  hp1, int  hp2, int  hp3, int  hp4);
void lotsofmoney_d(DDukeActor* s, int n);
void lotsofmail_d(DDukeActor* s, int n);
void lotsofpaper_d(DDukeActor* s, int n);
void lotsoffeathers_r(DDukeActor* s, int n);
void guts_d(DDukeActor* s, int gtype, int n, int p);
void guts_r(DDukeActor* s, int gtype, int n, int p);
int ifhitbyweapon_r(DDukeActor* sn);
int ifhitbyweapon_d(DDukeActor* sn);
void fall_d(DDukeActor* i, int g_p);
void fall_r(DDukeActor* i, int g_p);
bool spawnweapondebris_d(int picnum, int dnum);
bool spawnweapondebris_r(int picnum, int dnum);
void respawnhitag_d(DDukeActor* g_sp);
void respawnhitag_r(DDukeActor* g_sp);
void move_d(DDukeActor* i, int g_p, int g_x);
void move_r(DDukeActor* i, int g_p, int g_x);
void incur_damage_d(player_struct* p);
void incur_damage_r(player_struct* p);
void shoot_d(DDukeActor* i, int atwith);
void shoot_r(DDukeActor* i, int atwith);
void selectweapon_d(int snum, int j);
void selectweapon_r(int snum, int j);
int doincrements_d(player_struct* p);
int doincrements_r(player_struct* p);
void checkweapons_d(player_struct* p);
void checkweapons_r(player_struct* p);
void processinput_d(int snum);
void processinput_r(int snum);
void displayweapon_d(int snum, double smoothratio);
void displayweapon_r(int snum, double smoothratio);
void displaymasks_d(int snum, int p, double smoothratio);
void displaymasks_r(int snum, int p, double smoothratio);
void think_d();
void think_r();
void animatesprites_d(tspriteArray& tsprites, int x, int y, int a, int smoothratio);
void animatesprites_r(tspriteArray& tsprites, int x, int y, int a, int smoothratio);

Dispatcher fi;

void SetDispatcher()
{
	if (!isRR())
	{
		fi = {
		think_d,
		initactorflags_d,
		isadoorwall_d,
		animatewalls_d,
		operaterespawns_d,
		operateforcefields_d,
		checkhitswitch_d,
		activatebysector_d,
		checkhitwall_d,
		checkhitceiling_d,
		checkhitsprite_d,
		checksectors_d,
		spawninit_d,

		ceilingspace_d,
		floorspace_d,
		addweapon_d,
		hitradius_d,
		lotsofmoney_d,
		lotsofmail_d,
		lotsofpaper_d,
		guts_d,
		ifhitbyweapon_d,
		fall_d,
		spawnweapondebris_d,
		respawnhitag_d,
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
		initactorflags_r,
		isadoorwall_r,
		animatewalls_r,
		operaterespawns_r,
		operateforcefields_r,
		checkhitswitch_r,
		activatebysector_r,
		checkhitwall_r,
		checkhitceiling_r,
		checkhitsprite_r,
		checksectors_r,
		spawninit_r,

		ceilingspace_r,
		floorspace_r,
		addweapon_r,
		hitradius_r,
		lotsoffeathers_r,
		lotsoffeathers_r,
		lotsoffeathers_r,
		guts_r,
		ifhitbyweapon_r,
		fall_r,
		spawnweapondebris_r,
		respawnhitag_r,
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


int TILE_BOX;
int TILE_TREE1;
int TILE_TREE2;
int TILE_TIRE;
int TILE_CONE;
int TILE_W_FORCEFIELD;
int TILE_SCRAP6;
int TILE_APLAYER;
int TILE_DRONE;
int TILE_MENUSCREEN;
int TILE_SCREENBORDER;
int TILE_VIEWBORDER;
int TILE_APLAYERTOP;
int TILE_CAMCORNER;
int TILE_CAMLIGHT;
int TILE_STATIC;
int TILE_BOTTOMSTATUSBAR;
int TILE_THREEDEE;
int TILE_INGAMEDUKETHREEDEE;
int TILE_ATOMICHEALTH;
int TILE_JIBS6;
int TILE_FIRE;
int TILE_WATERBUBBLE;
int TILE_SMALLSMOKE;
int TILE_BLOODPOOL;
int TILE_FOOTPRINTS;
int TILE_FOOTPRINTS2;
int TILE_FOOTPRINTS3;
int TILE_FOOTPRINTS4;
int TILE_CLOUDYSKIES;
int TILE_ARROW;
int TILE_ACCESSSWITCH;
int TILE_ACCESSSWITCH2;
int TILE_GLASSPIECES;
int TILE_HEN;
int TILE_BETAVERSION;
int TILE_MIRROR;
int TILE_CLOUDYOCEAN;
int TILE_MOONSKY1;
int TILE_BIGORBIT;
int TILE_LA;
int TILE_LOADSCREEN;
int TILE_CROSSHAIR;
int TILE_BIGORBIT1;
int TILE_EGG;

END_DUKE_NS
