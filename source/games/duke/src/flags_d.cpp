//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2020 - Christoph Oelckers

This file is part of Enhanced Duke Nukem 3D version 1.5 - Atomic Edition

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

EDuke enhancements integrated: 04/13/2003 - Matt Saettler

Note: EDuke source was in transition.  Changes are in-progress in the
source as it is released.

*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "global.h"
#include "names_d.h"

BEGIN_DUKE_NS

void initactorflags_d()
{

	gs.actorinfo[COMMANDER].gutsoffset = -24;

	for (auto &fa : gs.actorinfo)
	{
		fa.falladjustz = 24;
	}
	gs.actorinfo[OCTABRAIN].falladjustz = gs.actorinfo[COMMANDER].falladjustz = gs.actorinfo[DRONE].falladjustz = 0;

	setflag(SFLAG_INTERNAL_BADGUY, {
			SHARK,
			RECON,
			DRONE,
			LIZTROOPONTOILET,
			LIZTROOPJUSTSIT,
			LIZTROOPSTAYPUT,
			LIZTROOPSHOOT,
			LIZTROOPJETPACK,
			LIZTROOPDUCKING,
			LIZTROOPRUNNING,
			LIZTROOP,
			OCTABRAIN,
			COMMANDER,
			COMMANDERSTAYPUT,
			PIGCOP,
			EGG,
			PIGCOPSTAYPUT,
			PIGCOPDIVE,
			LIZMAN,
			LIZMANSPITTING,
			LIZMANFEEDING,
			LIZMANJUMP,
			ORGANTIC,
			BOSS1,
			BOSS2,
			BOSS3,
			BOSS4,
			GREENSLIME,
			RAT,
			ROTATEGUN });

	// Some flags taken from RedNukem's init code. This is a good start as any to reduce the insane dependency on tile numbers for making decisions in the play code. A lot more will be added here later.
	setflag(SFLAG_NODAMAGEPUSH, { TANK, BOSS1, BOSS2, BOSS3, BOSS4, RECON, ROTATEGUN });
	setflag(SFLAG_BOSS, { BOSS1, BOSS2, BOSS3, BOSS4, BOSS4STAYPUT, BOSS1STAYPUT });
	if (isWorldTour()) setflag(SFLAG_BOSS, { BOSS2STAYPUT, BOSS3STAYPUT, BOSS5, BOSS5STAYPUT });
	setflag(SFLAG_NOWATERDIP, { OCTABRAIN, COMMANDER, DRONE });
	setflag(SFLAG_GREENSLIMEFOOD, { LIZTROOP, LIZMAN, PIGCOP, NEWBEAST });
	setflag(SFLAG_NOINTERPOLATE, { CRANEPOLE });
	setflag(SFLAG_FLAMMABLEPOOLEFFECT, { TIRE });
	setflag(SFLAG_FALLINGFLAMMABLE, { BOX });
	setflag(SFLAG_INFLAME, { RADIUSEXPLOSION, RPG, FIRELASER, HYDRENT, HEAVYHBOMB });
	setflag(SFLAG_NOFLOORFIRE, { TREE1, TREE2 });
	setflag(SFLAG_HITRADIUS_FLAG1, { BOX, TREE1, TREE2, TIRE, CONE });
	setflag(SFLAG_HITRADIUS_FLAG2, { TRIPBOMB, QUEBALL, STRIPEBALL, DUKELYINGDEAD });
	setflag(SFLAG_CHECKSLEEP, { RUBBERCAN, EXPLODINGBARREL, WOODENHORSE, HORSEONSIDE, CANWITHSOMETHING, FIREBARREL, NUKEBARREL, NUKEBARRELDENTED, NUKEBARRELLEAKED, TRIPBOMB });
	setflag(SFLAG_NOTELEPORT, { TRANSPORTERSTAR, TRANSPORTERBEAM, TRIPBOMB, BULLETHOLE, WATERSPLASH2, BURNING, BURNING2, FIRE, FIRE2, TOILETWATER, LASERLINE });
	setflag(SFLAG_SE24_NOCARRY, { TRIPBOMB, LASERLINE, BOLT1, BOLT2, BOLT3, BOLT4, SIDEBOLT1, SIDEBOLT2, SIDEBOLT3, SIDEBOLT4, CRANE, CRANE1, CRANE2, BARBROKE });
	setflag(SFLAG_SE24_REMOVE, { BLOODPOOL, PUKE, FOOTPRINTS, FOOTPRINTS2, FOOTPRINTS3, FOOTPRINTS4, BULLETHOLE, BLOODSPLAT1, BLOODSPLAT2, BLOODSPLAT3, BLOODSPLAT4 });
	setflag(SFLAG_BLOCK_TRIPBOMB, { TRIPBOMB }); // making this a flag adds the option to let other things block placing trip bombs as well.
	setflag(SFLAG_NOFALLER, { CRACK1, CRACK2, CRACK3, CRACK4, SPEAKER, LETTER, DUCK, TARGET, TRIPBOMB, VIEWSCREEN, VIEWSCREEN2 });
	setflag(SFLAG2_NOROTATEWITHSECTOR, { LASERLINE });
	setflag(SFLAG2_SHOWWALLSPRITEONMAP, { LASERLINE });
	setflag(SFLAG2_NOFLOORPAL, { TRIPBOMB, LASERLINE });
	setflag(SFLAG2_EXPLOSIVE, { FIREEXT, RPG, RADIUSEXPLOSION, SEENINE, OOZFILTER });
	setflag(SFLAG2_BRIGHTEXPLODE, { SEENINE, OOZFILTER });
	setflag(SFLAG2_DOUBLEDMGTHRUST, { RADIUSEXPLOSION, RPG, HYDRENT, HEAVYHBOMB, SEENINE, OOZFILTER, EXPLODINGBARREL });
	setflag(SFLAG2_BREAKMIRRORS, { RADIUSEXPLOSION, RPG, HYDRENT, HEAVYHBOMB, SEENINE, OOZFILTER, EXPLODINGBARREL });
	setflag(SFLAG2_CAMERA, { CAMERA1 });
	setflag(SFLAG2_DONTANIMATE, { TRIPBOMB, LASERLINE });
	setflag(SFLAG2_INTERPOLATEANGLE, { BEARINGPLATE });
	setflag(SFLAG2_GREENBLOOD, { OOZFILTER, NEWBEAST, NUKEBARREL });
	setflag(SFLAG2_ALWAYSROTATE1, { RAT, CAMERA1, CHAIR3 });
	setflag(SFLAG2_ALWAYSROTATE2, { RPG });
	setflag(SFLAG2_DIENOW, { RADIUSEXPLOSION, KNEE });
	setflag(SFLAG2_TRANFERPALTOJIBS, { LIZTROOP });
	setflag(SFLAG2_NORADIUSPUSH, { TANK, ROTATEGUN, RECON });
	setflag(SFLAG2_FREEZEDAMAGE | SFLAG2_REFLECTIVE, { FREEZEBLAST });
	setflag(SFLAG2_ALWAYSROTATE2, { RECON });
	setflag(SFLAG2_SPECIALAUTOAIM, { RECON });
	setflag(SFLAG2_IGNOREHITOWNER, { RECON });
	setflag(SFLAG2_NODAMAGEPUSH, { RECON, TANK, ROTATEGUN });
	setflag(SFLAG2_FLOATING, { DRONE, SHARK, COMMANDER });
	setflag(SFLAG2_NONSMOKYROCKET, { BOSS2 }); // If this wasn't needed for a CON defined actor it could be handled better
	setflag(SFLAG2_MIRRORREFLECT, { SHRINKSPARK, FIRELASER, COOLEXPLOSION1 });
	setflag(SFLAG2_UNDERWATERSLOWDOWN, { RPG });
	setflag(SFLAG3_BROWNBLOOD, { FECES });
	setflag(SFLAG3_DONTDIVEALIVE, { OCTABRAIN, SHARK, GREENSLIME });
	setflag(SFLAG3_LIGHTDAMAGE, { SHOTSPARK1 });

	if (isWorldTour())
	{
		setflag(SFLAG_INTERNAL_BADGUY, { FIREFLY });
		setflag(SFLAG_INTERNAL_BADGUY|SFLAG_NODAMAGEPUSH|SFLAG_BOSS, { BOSS5 });
	}

	setflag(SFLAG_INVENTORY, {
		FIRSTAID,
		STEROIDS,
		HEATSENSOR,
		BOOTS,
		JETPACK,
		HOLODUKE,
		AIRTANK });

	setflag(SFLAG_SHRINKAUTOAIM, {
		GREENSLIME,
		});

	setflag(SFLAG_HITRADIUSCHECK, {
		PODFEM1 ,
		FEM1,
		FEM2,   
		FEM3,
		FEM4,   
		FEM5,
		FEM6,    
		FEM7,
		FEM8,  
		FEM9,
		FEM10,   
		STATUE,
		STATUEFLASH,
		SPACEMARINE,
		QUEBALL,
		STRIPEBALL
		});

	setflag(SFLAG_TRIGGER_IFHITSECTOR, { EXPLOSION2 });

	setflag(SFLAG_MOVEFTA_MAKESTANDABLE, {
		RUBBERCAN,
		EXPLODINGBARREL,
		WOODENHORSE,
		HORSEONSIDE,
		CANWITHSOMETHING,
		CANWITHSOMETHING2,
		CANWITHSOMETHING3,
		CANWITHSOMETHING4,
		FIREBARREL,
		FIREVASE,
		NUKEBARREL,
		NUKEBARRELDENTED,
		NUKEBARRELLEAKED,
		TRIPBOMB
		});

	setflag(SFLAG2_TRIGGERRESPAWN, {
		FEM1,
		FEM2,
		FEM3,
		FEM4,
		FEM5,
		FEM6,
		FEM7,
		FEM8,
		FEM9,
		FEM10,
		PODFEM1,
		NAKED1,
		STATUE,
		TOUGHGAL
		});

	setflag(SFLAG2_FORCESECTORSHADE, { GREENSLIME });
	setflag(SFLAG3_DONTDIVEALIVE, { GREENSLIME, SHARK, OCTABRAIN });
	setflag(SFLAG3_BLOODY, { BLOODPOOL });

	// The feature guarded by this flag does not exist in Duke, it always acts as if the flag was set.
	for (auto& ainf : gs.actorinfo) ainf.flags |= SFLAG_MOVEFTA_CHECKSEE;

	gs.actorinfo[ORGANTIC].aimoffset = 32;
	gs.actorinfo[ROTATEGUN].aimoffset = 32;

	gs.weaponsandammosprites[0] = RPGSPRITE;
	gs.weaponsandammosprites[1] = CHAINGUNSPRITE;
	gs.weaponsandammosprites[2] = DEVISTATORAMMO;
	gs.weaponsandammosprites[3] = RPGAMMO;
	gs.weaponsandammosprites[4] = RPGAMMO;
	gs.weaponsandammosprites[5] = JETPACK;
	gs.weaponsandammosprites[6] = SHIELD;
	gs.weaponsandammosprites[7] = FIRSTAID;
	gs.weaponsandammosprites[8] = STEROIDS;
	gs.weaponsandammosprites[9] = RPGAMMO;
	gs.weaponsandammosprites[10] = RPGAMMO;
	gs.weaponsandammosprites[11] = RPGSPRITE;
	gs.weaponsandammosprites[12] = RPGAMMO;
	gs.weaponsandammosprites[13] = FREEZESPRITE;
	gs.weaponsandammosprites[14] = FREEZEAMMO;
	gs.firstdebris = SCRAP6;

	TILE_W_FORCEFIELD = W_FORCEFIELD;
	TILE_APLAYER = APLAYER;
	TILE_DRONE = DRONE;
	TILE_SCREENBORDER = BIGHOLE;
	TILE_VIEWBORDER = VIEWBORDER;
	TILE_APLAYERTOP = APLAYERTOP;
	TILE_CAMCORNER = CAMCORNER;
	TILE_CAMLIGHT = CAMLIGHT;
	TILE_STATIC = STATIC;
	TILE_BOTTOMSTATUSBAR = isWorldTour()? WIDESCREENSTATUSBAR : BOTTOMSTATUSBAR;
	TILE_ATOMICHEALTH = ATOMICHEALTH;
	TILE_FIRE = FIRE;
	TILE_WATERBUBBLE = WATERBUBBLE;
	TILE_SMALLSMOKE = SMALLSMOKE;
	TILE_BLOODPOOL = BLOODPOOL;
	TILE_CLOUDYSKIES = CLOUDYSKIES;
	TILE_ACCESSSWITCH = ACCESSSWITCH;
	TILE_ACCESSSWITCH2 = ACCESSSWITCH2;
	TILE_MIRROR = MIRROR;
	TILE_LOADSCREEN = LOADSCREEN;
	TILE_CROSSHAIR = CROSSHAIR;
	TILE_EGG = EGG;

}


END_DUKE_NS
