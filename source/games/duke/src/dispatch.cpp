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
void operateforcefields_r(int s, int low);
void operateforcefields_d(int s, int low);
bool checkhitswitch_d(int snum, int w, int switchtype);
bool checkhitswitch_r(int snum, int w, int switchtype);
void activatebysector_d(int sect, int j);
void activatebysector_r(int sect, int j);
void checkhitwall_d(int spr, int dawallnum, int x, int y, int z, int atwith);
void checkhitwall_r(int spr, int dawallnum, int x, int y, int z, int atwith);
void checkplayerhurt_d(struct player_struct* p, int j);
void checkplayerhurt_r(struct player_struct* p, int j);
bool checkhitceiling_d(int sn);
bool checkhitceiling_r(int sn);
void checkhitsprite_d(int i, int sn);
void checkhitsprite_r(int i, int sn);
void checksectors_d(int snum);
void checksectors_r(int snum);

bool ceilingspace_d(int sectnum);
bool ceilingspace_r(int sectnum);
bool floorspace_d(int sectnum);
bool floorspace_r(int sectnum);
void addweapon_d(struct player_struct* p, int weapon);
void addweapon_r(struct player_struct* p, int weapon);
void hitradius_d(short i, int  r, int  hp1, int  hp2, int  hp3, int  hp4);
void hitradius_r(short i, int  r, int  hp1, int  hp2, int  hp3, int  hp4);
int movesprite_d(short spritenum, int xchange, int ychange, int zchange, unsigned int cliptype);
int movesprite_r(short spritenum, int xchange, int ychange, int zchange, unsigned int cliptype);
void lotsofmoney_d(spritetype* s, short n);
void lotsofmail_d(spritetype* s, short n);
void lotsofpaper_d(spritetype* s, short n);
void lotsoffeathers_r(spritetype* s, short n);
void guts_d(spritetype* s, short gtype, short n, short p);
void guts_r(spritetype* s, short gtype, short n, short p);
void gutsdir_d(spritetype* s, short gtype, short n, short p);
void gutsdir_r(spritetype* s, short gtype, short n, short p);
int ifhitsectors_d(int sectnum);
int ifhitsectors_r(int sectnum);
int ifhitbyweapon_r(int sn);
int ifhitbyweapon_d(int sn);
void fall_d(int g_i, int g_p);
void fall_r(int g_i, int g_p);
bool spawnweapondebris_d(int picnum, int dnum);
bool spawnweapondebris_r(int picnum, int dnum);
void respawnhitag_d(spritetype* g_sp);
void respawnhitag_r(spritetype* g_sp);
void checktimetosleep_d(int g_i);
void checktimetosleep_r(int g_i);
void move_d(int g_i, int g_p, int g_x);
void move_r(int g_i, int g_p, int g_x);
int spawn_d(int j, int pn);
int spawn_r(int j, int pn);
void check_fta_sounds_d(int i);
void check_fta_sounds_r(int i);
void incur_damage_d(struct player_struct* p);
void incur_damage_r(struct player_struct* p);
void shoot_d(int i, int atwith);
void shoot_r(int i, int atwith);
void selectweapon_d(int snum, int j);
void selectweapon_r(int snum, int j);
int doincrements_d(struct player_struct* p);
int doincrements_r(struct player_struct* p);
void checkweapons_d(struct player_struct* p);
void checkweapons_r(struct player_struct* p);
void processinput_d(int snum);
void processinput_r(int snum);
void displayweapon_d(int snum);
void displayweapon_r(int snum);
void displaymasks_d(int snum);
void displaymasks_r(int snum);
void think_d();
void think_r();
void animatesprites_d(int x, int y, int a, int smoothratio);
void animatesprites_r(int x, int y, int a, int smoothratio);

void Logo_d(const CompletionFunc&);
void Logo_r(const CompletionFunc&);
void InitFonts_d();
void InitFonts_r();
void PrintPaused_d();
void PrintPaused_r();


Dispatcher fi;

void SetDispatcher()
{
	if (!isRR())
	{
		fi = {
		Logo_d,
		InitFonts_d,
		PrintPaused_d,

		think_d,
		initactorflags_d,
		isadoorwall_d,
		animatewalls_d,
		operaterespawns_d,
		operateforcefields_d,
		checkhitswitch_d,
		activatebysector_d,
		checkhitwall_d,
		checkplayerhurt_d,
		checkhitceiling_d,
		checkhitsprite_d,
		checksectors_d,

		ceilingspace_d,
		floorspace_d,
		addweapon_d,
		hitradius_d,
		movesprite_d,
		lotsofmoney_d,
		lotsofmail_d,
		lotsofpaper_d,
		guts_d,
		gutsdir_d,
		ifhitsectors_d,
		ifhitbyweapon_d,
		fall_d,
		spawnweapondebris_d,
		respawnhitag_d,
		checktimetosleep_d,
		move_d,
		spawn_d,
		check_fta_sounds_d,

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
		Logo_r,
		InitFonts_r,
		PrintPaused_r,

		think_r,
		initactorflags_r,
		isadoorwall_r,
		animatewalls_r,
		operaterespawns_r,
		operateforcefields_r,
		checkhitswitch_r,
		activatebysector_r,
		checkhitwall_r,
		checkplayerhurt_r,
		checkhitceiling_r,
		checkhitsprite_r,
		checksectors_r,

		ceilingspace_r,
		floorspace_r,
		addweapon_r,
		hitradius_r,
		movesprite_r,
		lotsoffeathers_r,
		lotsoffeathers_r,
		lotsoffeathers_r,
		guts_r,
		gutsdir_r,
		ifhitsectors_r,
		ifhitbyweapon_r,
		fall_r,
		spawnweapondebris_r,
		respawnhitag_r,
		checktimetosleep_r,
		move_r,
		spawn_r,
		check_fta_sounds_r,

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
int TILE_CAMERA1;
int TILE_SCRAP6;
int TILE_APLAYER;
int TILE_DRONE;
int TILE_MENUSCREEN;
int TILE_SCREENBORDER;
int TILE_VIEWBORDER;
int TILE_LASERLINE;
int TILE_APLAYERTOP;
int TILE_CAMCORNER;
int TILE_CAMLIGHT;
int TILE_STATIC;
int TILE_BOTTOMSTATUSBAR;
int TILE_SPINNINGNUKEICON;
int TILE_THREEDEE;
int TILE_INGAMEDUKETHREEDEE;
int TILE_PLUTOPAKSPRITE;
int TILE_MENUBAR;
int TILE_ATOMICHEALTH;
int TILE_FLOORSLIME;
int TILE_JIBS6;
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
int TILE_HURTRAIL;
int TILE_FLOORPLASMA;
int TILE_EGG;

END_DUKE_NS
