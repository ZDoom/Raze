#pragma once

#include "gamehud.h"
#include "global.h"
#include "models/modeldata.h"
#include "texinfo.h"
#include "funct.h"
#include "mapinfo.h"

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
	return actor->GetClass() == DukeMasterSwitchClass && actor->spr.statnum != STAT_REMOVED;
}

inline int issoundcontroller(DDukeActor* actor)
{
	return actor->GetClass() == DukeSoundControllerClass;
}

inline int isrespawncontroller(DDukeActor* actor)
{
	return actor->GetClass() == DukeRespawnControllerClass;
}

inline int isactivator(DDukeActor* actor)
{
	return actor->GetClass() == DukeActivatorClass;
}

inline int islockedactivator(DDukeActor* actor)
{
	return actor->GetClass() == DukeActivatorLockedClass;
}

inline int islocator(DDukeActor* actor)
{
	return actor->GetClass() == DukeLocatorClass;
}

inline int iseffector(DDukeActor* actor)
{
	return actor->GetClass() == DukeSectorEffectorClass;
}


inline bool inventory(DDukeActor* S)
{
	return S->flags1 & SFLAG_INVENTORY;
}

inline bool wallswitchcheck(DDukeActor* s)
{
	return !!(tileflags(s->spr.spritetexture()) & TFLAG_WALLSWITCH);
}

inline bool isadoorwall(FTextureID dapic)
{
	return tileflags(dapic) & TFLAG_DOORWALL;
}

inline bool isablockdoor(FTextureID dapic)
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
	return (!!((ps[pl].input.actions) & bit));
}

inline ESyncBits PlayerInputBits(int pl, ESyncBits bits)
{
	return (ps[pl].input.actions & bits);
}

inline void PlayerSetInput(int pl, ESyncBits bit)
{
	ps[pl].input.actions |= bit;
}


inline int PlayerNewWeapon(int pl)
{
	return ps[pl].input.getNewWeapon();
}

inline void PlayerSetItemUsed(int pl, int num)
{
	ps[pl].input.setItemUsed(num - 1);
}

inline bool PlayerUseItem(int pl, int num)
{
	return ps[pl].input.isItemUsed(num - 1);
}

inline float PlayerInputSideVel(int pl)
{
	return ps[pl].input.svel;
}

inline float PlayerInputForwardVel(int pl)
{
	return ps[pl].input.fvel;
}

inline float PlayerInputAngVel(int pl)
{
	return ps[pl].input.avel;
}

inline DAngle GetPlayerHorizon(int pl)
{
	return DAngle::fromDeg(ps[pl].input.horz);
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
	p->Angles.doViewPitch(p->aim_mode == 0 && p->on_ground && p->cursector->lotag != ST_2_UNDERWATER);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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
inline void applyRotation1(DDukeActor* h, tspritetype* t, DAngle viewang, FTextureID base = FNullTextureID())
{
	if (tilehasmodelorvoxel(h->spr.spritetexture(), h->spr.pal))
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
	if (base.isNull()) base = h->spr.spritetexture();
	t->setspritetexture(base + k);
}

// 6 (12) frame rotation.
inline void applyRotation2(DDukeActor* h, tspritetype* t, DAngle viewang, FTextureID base = FNullTextureID())
{
	if (tilehasmodelorvoxel(h->spr.spritetexture(), h->spr.pal))
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
	if (base.isNull()) base = h->spr.spritetexture();
	t->setspritetexture(base + k);
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

inline void processinputvel(int snum)
{
	const auto p = &ps[snum];
	const auto velvect = DVector2(p->input.fvel, p->input.svel).Rotated(p->GetActor()->spr.Angles.Yaw) + p->fric;
	p->input.fvel = (float)velvect.X;
	p->input.svel = (float)velvect.Y;
}


inline const ActorInfo* DDukeActor::conInfo() const
{
	auto tn = static_cast<PClassActor*>(GetClass())->ActorInfo()->TypeNum;
	return tn < 0 || gs.actorinfo[tn].scriptaddress == 0 ? nullptr : &gs.actorinfo[tn];
}

inline bool DDukeActor::isPlayer() const
{
	return IsKindOf(DukePlayerBaseClass);
}

inline bool shrinkersizecheck(PClass* type, DDukeActor* act2)
{
	// this should be a damage type check, not a projectile type check.
	// It's also quite broken because it doesn't check for being shrunk but tries to guess it from the size.
	// Unfortunately, with CON there is no way to retrieve proper shrunk state in any way.
	return type != DukeShrinkSparkClass || (act2->spr.scale.X >= 0.375);
}

inline void setPlayerActorViewZOffset(DDukeActor* const pact)
{
	if (!PlayClock)
	{
		pact->spr.pos.Z += gs.playerheight;
		pact->opos.Z += gs.playerheight;
		pact->oviewzoffset = pact->viewzoffset = -gs.playerheight;
	}
}

// flag mess to avoid double counting of kills.
// this is still not foolproof because CON requires manually recording the kills.
inline void addtokills(DDukeActor* actor)
{
	if (actor->flags1 & SFLAG_KILLCOUNT)
	{
		Level.addKillCount(1);
		actor->spr.cstat2 |= CSTAT2_SPRITE_COUNTKILL;
	}
}

inline void addkill(DDukeActor* actor)
{
	if ((actor->flags1 & SFLAG_KILLCOUNT) && (actor->spr.cstat2 & CSTAT2_SPRITE_COUNTKILL))
	{
		Level.addKill(myconnectindex);
		actor->spr.cstat2 &= ~CSTAT2_SPRITE_COUNTKILL;
	}
}

inline void subkill(DDukeActor* actor)
{
	if ((actor->flags1 & SFLAG_KILLCOUNT) && !(actor->spr.cstat2 & CSTAT2_SPRITE_COUNTKILL))
	{
		Level.addKill(-1, -1); // only deduct from global, not from player kills
		actor->spr.cstat2 |= CSTAT2_SPRITE_COUNTKILL;
	}
}

inline void dokill(player_struct* p, DDukeActor* g_ac, int amount)
{
	if (g_ac->spriteextra < 1 || g_ac->spriteextra == 128 || !isRR())
	{
		if (amount > 0) addkill(g_ac);
		else if (amount < 0) subkill(g_ac);
	}
	g_ac->actorstayput = nullptr;
}


END_DUKE_NS
