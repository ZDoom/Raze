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

inline int badguy(spritetype const * const pSprite)
{
	return badguypic(pSprite->picnum);
}

inline int bossguypic(int const tileNum)
{
	return ((actorinfo[tileNum].flags & (SFLAG_BOSS)) != 0);
}

inline int bossguy(spritetype const* const pSprite)
{
	return bossguypic(pSprite->picnum);
}

inline int actorflag(DDukeActor * actor, int mask)
{
	return (((actorinfo[actor->s.picnum].flags/* ^ hittype[spritenum].flags*/) & mask) != 0);
}

inline int actorfella(DDukeActor* actor)
{
	return actorflag(actor, SFLAG_KILLCOUNT);
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
		if(ps[i].GetActor() && ps[i].GetActor()->s.sectnum == se ) return i;
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
	return (!!((ps[pl].sync.actions) & bit));
}

inline ESyncBits PlayerInputBits(int pl, ESyncBits bits)
{
	return (ps[pl].sync.actions & bits);
}

inline void PlayerSetInput(int pl, ESyncBits bit)
{
	ps[pl].sync.actions |= bit;
}


inline int PlayerNewWeapon(int pl)
{
	return ps[pl].sync.getNewWeapon();
}

inline void PlayerSetItemUsed(int pl, int num)
{
	ps[pl].sync.setItemUsed(num - 1);
}

inline bool PlayerUseItem(int pl, int num)
{
	return ps[pl].sync.isItemUsed(num - 1);
}

inline int PlayerInputSideVel(int pl)
{
	return ps[pl].sync.svel;
}

inline int PlayerInputForwardVel(int pl)
{
	return ps[pl].sync.fvel;
}

inline float PlayerInputAngVel(int pl)
{
	return ps[pl].sync.avel;
}

inline float PlayerHorizon(int pl)
{
	return ps[pl].sync.horz;
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

// the weapon display code uses this.
inline double get16thOfHoriz(int const snum, bool const interpolate, double const smoothratio)
{
	return (!interpolate ? ps[snum].horizon.sum() : ps[snum].horizon.interpolatedsum(smoothratio)).asq16() * (0.0625 / FRACUNIT);
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
