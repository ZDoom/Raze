#pragma once

BEGIN_DUKE_NS

// These are all globally accessed tiles.
extern int TILE_BOX;
extern int TILE_TREE1;
extern int TILE_TREE2;
extern int TILE_TIRE;
extern int TILE_CONE;
extern int TILE_W_FORCEFIELD;
extern int TILE_CAMERA1;
extern int TILE_SCRAP6;
extern int TILE_APLAYER;
extern int TILE_DRONE;
extern int TILE_MENUSCREEN;
extern int TILE_SCREENBORDER;
extern int TILE_VIEWBORDER;
extern int TILE_LASERLINE;
extern int TILE_APLAYERTOP;
extern int TILE_CAMCORNER;
extern int TILE_CAMLIGHT;
extern int TILE_STATIC;
extern int TILE_BOTTOMSTATUSBAR;
extern int TILE_ATOMICHEALTH;
extern int TILE_FLOORSLIME;
extern int TILE_JIBS6;
extern int TILE_FIRE;
extern int TILE_WATERBUBBLE;
extern int TILE_SMALLSMOKE;
extern int TILE_BLOODPOOL;
extern int TILE_FOOTPRINTS;
extern int TILE_FOOTPRINTS2;
extern int TILE_FOOTPRINTS3;
extern int TILE_FOOTPRINTS4;
extern int TILE_CLOUDYSKIES;
extern int TILE_ARROW;
extern int TILE_ACCESSSWITCH;
extern int TILE_ACCESSSWITCH2;
extern int TILE_GLASSPIECES;
extern int TILE_HEN;
extern int TILE_BETAVERSION;
extern int TILE_MIRROR;
extern int TILE_CLOUDYOCEAN;
extern int TILE_MOONSKY1;
extern int TILE_BIGORBIT;
extern int TILE_LA;
extern int TILE_LOADSCREEN;
extern int TILE_CROSSHAIR;
extern int TILE_BIGORBIT1;
extern int TILE_HURTRAIL;
extern int TILE_FLOORPLASMA;
extern int TILE_EGG;



enum dukeweapon_t
{
	KNEE_WEAPON,  // 0
	PISTOL_WEAPON,
	SHOTGUN_WEAPON,
	CHAINGUN_WEAPON,
	RPG_WEAPON,
	HANDBOMB_WEAPON,  // 5
	SHRINKER_WEAPON,
	DEVISTATOR_WEAPON,
	TRIPBOMB_WEAPON,
	FREEZE_WEAPON,
	HANDREMOTE_WEAPON,  // 10
	GROW_WEAPON,
	FLAMETHROWER_WEAPON,    // World Tour

	MIN_WEAPON = 0,
	MAX_WEAPON = 9,

	// These names have been pieced together from RedneckGDX and RedNukem because the reconstructed source recycled Duke's names for the first 11 weapons.
	// Names for 0-2 are the same
	RIFLEGUN_WEAPON = 3,
	DYNAMITE_WEAPON = 4,
	CROSSBOW_WEAPON = 5,
	THROWSAW_WEAPON = 6,
	ALIENBLASTER_WEAPON = 7,
	POWDERKEG_WEAPON = 8,
	TIT_WEAPON = 9,
	THROWINGDYNAMITE_WEAPON = 10,
	BUZZSAW_WEAPON = 11,
	BOWLING_WEAPON = 12,
	MOTORCYCLE_WEAPON = 13,
	BOAT_WEAPON = 14,
	SLINGBLADE_WEAPON = 15,
	CHICKEN_WEAPON = 16,
	MAX_WEAPONS
};

END_DUKE_NS
