#pragma once

#include "gamehud.h"
#include "global.h"
#include "models/modeldata.h"

// all inline functions.
BEGIN_DUKE_NS

inline int rnd(int X)
{
	return ((krand() >> 8) >= (255 - (X)));
}

// internal controller classes can be directly checked for by type
inline int ismasterswitch(DDukeActor* actor)
{
	// The STAT_REMOVED check here is important!
	return actor->GetClass()->TypeName == NAME_DukeMasterSwitch && actor->spr.statnum != STAT_REMOVED;
}

inline int issoundcontroller(DDukeActor* actor)
{
	return actor->GetClass()->TypeName == NAME_DukeSoundController;
}

inline int isrespawncontroller(DDukeActor* actor)
{
	return actor->GetClass()->TypeName == NAME_DukeRespawnController;
}

inline int isactivator(DDukeActor* actor)
{
	return actor->GetClass()->TypeName == NAME_DukeActivator;
}

inline int islockedactivator(DDukeActor* actor)
{
	return actor->GetClass()->TypeName == NAME_DukeActivatorLocked;
}

inline int islocator(DDukeActor* actor)
{
	return actor->GetClass()->TypeName == NAME_DukeLocator;
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

inline int& tileflags(unsigned int tilenum)
{
	static int sink = 0;
	if (tilenum >= MAXTILES) return sink;
	return TileFiles.tiledata[tilenum].tileflags;
}

inline bool wallswitchcheck(DDukeActor* s)
{
	return !!(tileflags(s->spr.picnum) & TFLAG_WALLSWITCH);
}

inline bool isadoorwall(int dapic)
{
	return tileflags(dapic) & TFLAG_DOORWALL;
}

inline bool isablockdoor(int dapic)
{
	return tileflags(dapic) & TFLAG_BLOCKDOOR;
}



inline int checkcursectnums(sectortype* se)
{
	int i;
	for(i=connecthead;i>=0;i=connectpoint2[i])
		if(ps[i].GetActor() && ps[i].GetActor()->sector() == se ) return i;
	return -1;
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

inline float PlayerInputSideVel(int pl)
{
	return ps[pl].sync.svel;
}

inline float PlayerInputForwardVel(int pl)
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

inline void doslopetilting(player_struct* p)
{
	bool const canslopetilt = p->on_ground && p->insector() && p->cursector->lotag != ST_2_UNDERWATER && (p->cursector->floorstat & CSTAT_SECTOR_SLOPE);
	p->Angles.doViewPitch(p->GetActor()->spr.pos.XY(), p->GetActor()->spr.Angles.Yaw, p->aim_mode == 0, canslopetilt, p->cursector);
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

inline void animateshrunken(player_struct* p, double weapon_xoffset, double looking_arc, double look_anghalf, int tilenum, int8_t shade, int o, double interpfrac)
{
	const double fistsign = BobVal(interpolatedvalue<double>(p->ofistsign, p->fistsign, interpfrac)) * 16;
	if (p->jetpack_on == 0)	looking_arc += 32 - (p->GetActor()->vel.X * 8);
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

inline DAngle randomAngle(double span = 360.)
{
	return DAngle::fromDeg(krandf(span));
}

inline DAngle randomAngle(DAngle span)
{
	return DAngle::fromDeg(krandf(span.Degrees()));
}

inline int angletorotation1(DAngle sprang, DAngle viewang, int shiftv = 8, int andv = 7)
{
	return (((sprang.Buildang() + 3072 + 128 - viewang.Buildang()) & 2047) >> shiftv) & andv;
}

inline int angletorotation2(DAngle sprang, DAngle viewang)
{
	return ((sprang.Buildang() + 3072 + 128 - viewang.Buildang()) & 2047) / 170;
}

// 4 (8) frame rotation.
inline void applyRotation1(DDukeActor* h, tspritetype* t, DAngle viewang)
{
	if (hw_models && modelManager.CheckModel(h->spr.picnum, h->spr.pal))
	{
		t->cstat &= ~CSTAT_SPRITE_XFLIP;
		return;
	}

	int k = angletorotation1(t->Angles.Yaw, viewang);
	if (k > 4)
	{
		k = 8 - k;
		t->cstat |= CSTAT_SPRITE_XFLIP;
	}
	else t->cstat &= ~CSTAT_SPRITE_XFLIP;
	t->picnum = h->spr.picnum + k;
}

// 6 (12) frame rotation.
inline void applyRotation2(DDukeActor* h, tspritetype* t, DAngle viewang)
{
	if (hw_models && modelManager.CheckModel(h->spr.picnum, h->spr.pal))
	{
		t->cstat &= ~CSTAT_SPRITE_XFLIP;
		return;
	}

	int k = angletorotation2(t->Angles.Yaw, viewang);
	if (k > 6)
	{
		k = 12 - k;
		t->cstat |= CSTAT_SPRITE_XFLIP;
	}
	else t->cstat &= ~CSTAT_SPRITE_XFLIP;
	t->picnum = h->spr.picnum + k;
}

inline int monsterCheatCheck(DDukeActor* self)
{
	if (ud.multimode < 2)
	{
		if (actor_tog == 1)
		{
			self->spr.cstat = CSTAT_SPRITE_INVISIBLE;
			return true;
		}
		else if (actor_tog == 2) self->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
	}
	return false;
}

END_DUKE_NS
