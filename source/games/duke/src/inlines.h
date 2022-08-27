#pragma once

#include "mathutil.h"
#include "gamehud.h"
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
	return ((gs.actorinfo[tileNum].flags & (SFLAG_INTERNAL_BADGUY | SFLAG_BADGUY)) != 0);
}

inline int bossguypic(int const tileNum)
{
	return ((gs.actorinfo[tileNum].flags & (SFLAG_BOSS)) != 0);
}

inline int actorflag(DDukeActor * actor, EDukeFlags1 mask)
{
	return (((gs.actorinfo[actor->spr.picnum].flags) & mask) != 0);
}

inline int actorflag(DDukeActor* actor, EDukeFlags2 mask)
{
	return (((gs.actorinfo[actor->spr.picnum].flags2) & mask) != 0);
}

inline int attackerflag(DDukeActor* actor, EDukeFlags1 mask)
{
	return (((gs.actorinfo[actor->attackertype].flags) & mask) != 0);
}

inline int attackerflag(DDukeActor* actor, EDukeFlags2 mask)
{
	return (((gs.actorinfo[actor->attackertype].flags2) & mask) != 0);
}

inline int actorfella(DDukeActor* actor)
{
	return actorflag(actor, SFLAG_KILLCOUNT);
}

inline void setflag(EDukeFlags1 flag, const std::initializer_list<short>& types)
{
	for (auto val : types)
	{
		gs.actorinfo[val].flags |= flag;
	}
}

inline void setflag(EDukeFlags2 flag, const std::initializer_list<short>& types)
{
	for (auto val : types)
	{
		gs.actorinfo[val].flags2 |= flag;
	}
}

inline bool inventory(DDukeActor* S)
{
	return actorflag(S, SFLAG_INVENTORY);
}

inline void settileflag(int flag, const std::initializer_list<short>& types)
{
	for (auto val : types)
	{
		gs.tileinfo[val].flags |= flag;
	}
}

inline bool wallswitchcheck(DDukeActor* s)
{
	return !!(gs.tileinfo[s->spr.picnum].flags & TFLAG_WALLSWITCH);
}

inline int checkcursectnums(sectortype* se)
{
	int i;
	for(i=connecthead;i>=0;i=connectpoint2[i])
		if(ps[i].GetActor() && ps[i].GetActor()->sector() == se ) return i;
	return -1;
}

inline int ldist(DDukeActor* s1, DDukeActor* s2)
{
	int vx, vy;
	vx = s1->int_pos().X - s2->int_pos().X;
	vy = s1->int_pos().Y - s2->int_pos().Y;
	return(FindDistance2D(vx, vy) + 1);
}

inline int ldist(const DDukeActor* s1, const tspritetype* s2)
{
	int vx, vy;
	vx = s1->int_pos().X - s2->int_pos().X;
	vy = s1->int_pos().Y - s2->int_pos().Y;
	return(FindDistance2D(vx, vy) + 1);
}
inline int dist(DDukeActor* s1, DDukeActor* s2)
{
	int vx, vy, vz;
	vx = s1->int_pos().X - s2->int_pos().X;
	vy = s1->int_pos().Y - s2->int_pos().Y;
	vz = s1->int_pos().Z - s2->int_pos().Z;
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

inline float GetPlayerHorizon(int pl)
{
	return ps[pl].sync.horz;
}

inline void clearfriction()
{
	for (int i = 0; i != -1; i = connectpoint2[i])
	{
		ps[i].fric.X = ps[i].fric.Y = 0;
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

inline void doslopetilting(player_struct* p, double const scaleAdjust = 1)
{
	bool const canslopetilt = p->on_ground && p->insector() && p->cursector->lotag != ST_2_UNDERWATER && (p->cursector->floorstat & CSTAT_SECTOR_SLOPE);
	p->horizon.calcviewpitch(p->pos.XY(), p->angle.ang, p->aim_mode == 0, canslopetilt, p->cursector, scaleAdjust);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

inline void hud_draw(double x, double y, int tilenum, int shade, int orientation)
{
	int p = ps[screenpeek].cursector->floorpal;
	hud_drawsprite(x, y, 65536, 0, tilenum, shade, p, 2 | orientation);
}

inline void animateshrunken(player_struct* p, double weapon_xoffset, double looking_arc, double look_anghalf, int tilenum, int8_t shade, int o, double smoothratio)
{
	const double fistsign = bsinf(interpolatedvalue(p->ofistsign, p->fistsign, smoothratio), -10);
	if (p->jetpack_on == 0)	looking_arc += 32 - (p->GetActor()->spr.xvel >> 1);
	hud_draw(weapon_xoffset + fistsign + 250 - look_anghalf, looking_arc + 258 - fabs(fistsign * 4), tilenum, shade, o);
	hud_draw(weapon_xoffset - fistsign + 40 - look_anghalf, looking_arc + 200 + fabs(fistsign * 4), tilenum, shade, o | 4);
}

inline ESpriteFlags randomFlip()
{
	int r = krand() & 12;
	if (r == 0) return 0;
	if (r == 4) return CSTAT_SPRITE_XFLIP;
	if (r == 8) return CSTAT_SPRITE_YFLIP;
	return CSTAT_SPRITE_XFLIP | CSTAT_SPRITE_YFLIP;
}

inline ESpriteFlags randomXFlip()
{
	int r = krand() & 4;
	if (r == 0) return 0;
	return CSTAT_SPRITE_XFLIP;
}

END_DUKE_NS
