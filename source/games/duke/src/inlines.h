#pragma once

#include "mathutil.h"
#include "glbackend/glbackend.h"
#include "global.h"

// all inline functions.
BEGIN_DUKE_NS

inline int rnd(int X)
{
	return ((krand() >> 8) >= (255 - (X)));
}

inline bool AFLAMABLE(int X)
{
	return (X == TILE_BOX || X == TILE_TREE1 || X == TILE_TREE2 || X == TILE_TIRE || X == TILE_CONE);
}

inline int badguypic(int const tileNum)
{
	return ((actorinfo[tileNum].flags & (SFLAG_INTERNAL_BADGUY | SFLAG_BADGUY)) != 0);
}

inline int badguy(void const * const pSprite)
{
	return badguypic(((uspritetype const *) pSprite)->picnum);
}

inline int actorflag(int spritenum, int mask)
{
	return (((actorinfo[sprite[spritenum].picnum].flags/* ^ hittype[spritenum].flags*/) & mask) != 0);
}

inline int actorfella(int spnum)
{
	return actorflag(spnum, SFLAG_KILLCOUNT);
}

inline void setflag(int flag, const std::initializer_list<short>& types)
{
	for (auto val : types)
	{
		actorinfo[val].flags |= flag;
	}
}

inline bool inventory(spritetype* S)
{
	return !!(actorinfo[S->picnum].flags & SFLAG_INVENTORY);
}

inline void settileflag(int flag, const std::initializer_list<short>& types)
{
	for (auto val : types)
	{
		tileinfo[val].flags |= flag;
	}
}

inline bool wallswitchcheck(int s)
{
	return !!(tileinfo[sprite[s].picnum].flags & TFLAG_WALLSWITCH);
}

inline int checkcursectnums(int se)
{
	int i;
	for(i=connecthead;i>=0;i=connectpoint2[i])
		if((unsigned)ps[i].i < MAXSPRITES && sprite[ps[i].i].sectnum == se ) return i;
	return -1;
}

// These are from duke's sector.c
inline int ldist(const spritetype* s1, const spritetype* s2)
{
	int vx, vy;
	vx = s1->x - s2->x;
	vy = s1->y - s2->y;
	return(FindDistance2D(vx, vy) + 1);
}

inline int ldist(const spritetype* s1, const tspritetype* s2)
{
	int vx, vy;
	vx = s1->x - s2->x;
	vy = s1->y - s2->y;
	return(FindDistance2D(vx, vy) + 1);
}

inline int dist(const spritetype* s1, const spritetype* s2)
{
	int vx, vy, vz;
	vx = s1->x - s2->x;
	vy = s1->y - s2->y;
	vz = s1->z - s2->z;
	return(FindDistance3D(vx, vy, vz));
}

inline bool isIn(int value, int first)
{
	return value == first;
}

template<typename... Args>
bool isIn(int value, int first, Args... args)
{
	return value == first || isIn(value, args...);
}

inline bool isIn(int value, const std::initializer_list<int>& list)
{
	for (auto v : list) if (v == value) return true;
	return false;
}


// these are mainly here to avoid directly accessing the input data so that it can be more easily refactored later.
inline bool PlayerInput(int pl, ESyncBits bit)
{
	return (!!((sync[pl].actions) & bit));
}

inline ESyncBits PlayerInputBits(int pl, ESyncBits bits)
{
	return (sync[pl].actions & bits);
}

inline void PlayerSetInput(int pl, ESyncBits bit)
{
	sync[pl].actions |= bit;
}


inline int PlayerNewWeapon(int pl)
{
	return sync[pl].getNewWeapon();
}

inline void PlayerSetItemUsed(int pl, int num)
{
	sync[pl].setItemUsed(num - 1);
}

inline bool PlayerUseItem(int pl, int num)
{
	return sync[pl].isItemUsed(num - 1);
}

inline int PlayerInputSideVel(int pl)
{
	return sync[pl].svel;
}

inline int PlayerInputForwardVel(int pl)
{
	return sync[pl].fvel;
}

inline fixed_t PlayerInputAngVel(int pl)
{
	return sync[pl].q16avel;
}

inline void clearfriction()
{
	for (int i = 0; i != -1; i = connectpoint2[i])
	{
		ps[i].fric.x = ps[i].fric.y = 0;
	}
}

inline void SetPlayerPal(player_struct* p, PalEntry pe)
{
	p->pals = pe;
}

inline bool playrunning()
{
	return (paused == 0 || (paused == 1 && (ud.recstat == 2 || ud.multimode > 1)));
}

inline void backupplayer(player_struct* p)
{
	backuppos(p);
	backuplook(p);
	backupview(p);
}

// the weapon display code uses these.
inline double getHalfLookAng(int snum, bool interpolate, double smoothratio)
{
	struct player_struct *p = &ps[snum];
	return (!interpolate ? p->q16look_ang : p->oq16look_ang + fmulscale16(p->q16look_ang - p->oq16look_ang, smoothratio)) * (0.5 / FRACUNIT);
}

inline double get16thOfHoriz(int snum, bool interpolate, double smoothratio)
{
	struct player_struct *p = &ps[snum];
	fixed_t ohorz = p->oq16horiz - p->oq16horizoff;
	fixed_t horz = p->q16horiz - p->q16horizoff;
	return (!interpolate ? horz : ohorz + fmulscale16(horz - ohorz, smoothratio)) * (0.0625 / FRACUNIT);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

inline void hud_draw(double x, double y, int tilenum, int shade, int orientation)
{
	int p = sector[ps[screenpeek].cursectnum].floorpal;
	hud_drawsprite(x, y, 65536, 0, tilenum, shade, p, 2 | orientation);
}

END_DUKE_NS
