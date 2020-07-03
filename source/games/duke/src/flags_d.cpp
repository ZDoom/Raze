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
#include "zz_actors.h"
#include "names_d.h"

BEGIN_DUKE_NS

void initactorflags_d()
{
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
            GREENSLIME+1,
            GREENSLIME+2,
            GREENSLIME+3,
            GREENSLIME+4,
            GREENSLIME+5,
            GREENSLIME+6,
            GREENSLIME+7,
            RAT,
			ROTATEGUN });
		
	// Some flags taken from RedNukem's init code. This is a good start as any to reduce the insane dependency on tile numbers for making decisions in the play code. A lot more will be added here later.
	setflag(SFLAG_NODAMAGEPUSH, { TANK, BOSS1, BOSS2, BOSS3, BOSS4, RECON, ROTATEGUN });
	setflag(SFLAG_NOWATERDIP, { OCTABRAIN, COMMANDER, DRONE });
	setflag(SFLAG_GREENSLIMEFOOD, { LIZTROOP, LIZMAN, PIGCOP, NEWBEAST });
			
	if (isWorldTour())
	{
		setflag(SFLAG_INTERNAL_BADGUY|SFLAG_NODAMAGEPUSH, { FIREFLY, BOSS5, BOSS5STAYPUT });
	}
	
	settileflag(TFLAG_WALLSWITCH, {
        HANDPRINTSWITCH,
        HANDPRINTSWITCH+1,
        ALIENSWITCH,
        ALIENSWITCH+1,
        MULTISWITCH,
        MULTISWITCH+1,
        MULTISWITCH+2,
        MULTISWITCH+3,
        ACCESSSWITCH,
        ACCESSSWITCH2,
        PULLSWITCH,
        PULLSWITCH+1,
        HANDSWITCH,
        HANDSWITCH+1,
        SLOTDOOR,
        SLOTDOOR+1,
        LIGHTSWITCH,
        LIGHTSWITCH+1,
        SPACELIGHTSWITCH,
        SPACELIGHTSWITCH+1,
        SPACEDOORSWITCH,
        SPACEDOORSWITCH+1,
        FRANKENSTINESWITCH,
        FRANKENSTINESWITCH+1,
        LIGHTSWITCH2,
        LIGHTSWITCH2+1,
        POWERSWITCH1,
        POWERSWITCH1+1,
        LOCKSWITCH1,
        LOCKSWITCH1+1,
        POWERSWITCH2,
        POWERSWITCH2+1,
        DIPSWITCH,
        DIPSWITCH+1,
        DIPSWITCH2,
        DIPSWITCH2+1,
        TECHSWITCH,
        TECHSWITCH+1,
        DIPSWITCH3,
		DIPSWITCH3+1});

    setflag(SFLAG_INVENTORY, {
        FIRSTAID,
        STEROIDS,
        HEATSENSOR,
        BOOTS,
        JETPACK,
        HOLODUKE,
        AIRTANK });

    setflag(SFLAG_ADULT, {
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
        MAN,
        MAN2,
        WOMAN,
        NAKED1,
        PODFEM1,
        FEMMAG1,
        FEMMAG2,
        FEMPIC1,
        FEMPIC2,
        FEMPIC3,
        FEMPIC4,
        FEMPIC5,
        FEMPIC6,
        FEMPIC7,
        BLOODYPOLE,
        FEM6PAD,
        STATUE,
        STATUEFLASH,
        OOZ,
        OOZ2,
        WALLBLOOD1,
        WALLBLOOD2,
        WALLBLOOD3,
        WALLBLOOD4,
        WALLBLOOD5,
        WALLBLOOD7,
        WALLBLOOD8,
        SUSHIPLATE1,
        SUSHIPLATE2,
        SUSHIPLATE3,
        SUSHIPLATE4,
        FETUS,
        FETUSJIB,
        FETUSBROKE,
        HOTMEAT,
        FOODOBJECT16,
        DOLPHIN1,
        DOLPHIN2,
        TOUGHGAL,
        TAMPON,
        XXXSTACY,
        4946,
        4947,
        693,
        2254,
        4560,
        4561,
        4562,
        4498,
        4957 });

    setflag(SFLAG_SHRINKAUTOAIM, {
        GREENSLIME,
        GREENSLIME + 1,
        GREENSLIME + 2,
        GREENSLIME + 3,
        GREENSLIME + 4,
        GREENSLIME + 5,
        GREENSLIME + 6,
        GREENSLIME + 7,
        });

    actorinfo[ORGANTIC].aimoffset = 32 << 8;
    actorinfo[ROTATEGUN].aimoffset = 32 << 8;

    weaponsandammosprites[0] = RPGSPRITE;
    weaponsandammosprites[1] = CHAINGUNSPRITE;
    weaponsandammosprites[2] = DEVISTATORAMMO;
    weaponsandammosprites[3] = RPGAMMO;
    weaponsandammosprites[4] = RPGAMMO;
    weaponsandammosprites[5] = JETPACK;
    weaponsandammosprites[6] = SHIELD;
    weaponsandammosprites[7] = FIRSTAID;
    weaponsandammosprites[8] = STEROIDS;
    weaponsandammosprites[9] = RPGAMMO;
    weaponsandammosprites[10] = RPGAMMO;
    weaponsandammosprites[11] = RPGSPRITE;
    weaponsandammosprites[12] = RPGAMMO;
    weaponsandammosprites[13] = FREEZESPRITE;
    weaponsandammosprites[14] = FREEZEAMMO;

    TILE_BOX = BOX;
    TILE_TREE1 = TREE1;
    TILE_TREE2 = TREE2;
    TILE_TIRE = TIRE;
    TILE_CONE = CONE;
    TILE_W_FORCEFIELD = W_FORCEFIELD;
    TILE_CAMERA1 = CAMERA1;
    TILE_SCRAP6 = SCRAP6;
    TILE_APLAYER = APLAYER;
    TILE_DRONE = DRONE;
    TILE_MENUSCREEN = MENUSCREEN;
    TILE_SCREENBORDER = BIGHOLE;
    TILE_VIEWBORDER = VIEWBORDER;
    TILE_LASERLINE = LASERLINE;
    TILE_APLAYERTOP = APLAYERTOP;
    TILE_CAMCORNER = CAMCORNER;
    TILE_CAMLIGHT = CAMLIGHT;
    TILE_STATIC = STATIC;
    TILE_BOTTOMSTATUSBAR = BOTTOMSTATUSBAR;
    TILE_SPINNINGNUKEICON = SPINNINGNUKEICON;
    TILE_THREEDEE = THREEDEE;
    TILE_INGAMEDUKETHREEDEE = INGAMEDUKETHREEDEE;
    TILE_PLUTOPAKSPRITE = PLUTOPAKSPRITE;
    TILE_MENUBAR = MENUBAR;
    TILE_ATOMICHEALTH = ATOMICHEALTH;
    TILE_FLOORSLIME = FLOORSLIME;
    TILE_JIBS6 = JIBS6;
    TILE_WATERBUBBLE = WATERBUBBLE;
    TILE_SMALLSMOKE = SMALLSMOKE;
    TILE_BLOODPOOL = BLOODPOOL;
    TILE_FOOTPRINTS = FOOTPRINTS;
    TILE_FOOTPRINTS2 = FOOTPRINTS2;
    TILE_FOOTPRINTS3 = FOOTPRINTS3;
    TILE_FOOTPRINTS4 = FOOTPRINTS4;
    TILE_CLOUDYSKIES = CLOUDYSKIES;
    TILE_ARROW = ARROW;
    TILE_ACCESSSWITCH = ACCESSSWITCH;
    TILE_ACCESSSWITCH2 = ACCESSSWITCH2;
    TILE_GLASSPIECES = GLASSPIECES;
    TILE_BETAVERSION = BETAVERSION;
    TILE_MIRROR = MIRROR;
    TILE_CLOUDYOCEAN = CLOUDYOCEAN;
    TILE_MOONSKY1 = MOONSKY1;
    TILE_LA = LA;
    TILE_LOADSCREEN = LOADSCREEN;
    TILE_CROSSHAIR = CROSSHAIR;
    TILE_BIGORBIT1 = BIGORBIT1;
    TILE_HURTRAIL = HURTRAIL;
    TILE_FLOORPLASMA = FLOORPLASMA;

}
	
	
END_DUKE_NS
