#pragma once
#include "dobject.h"
#include "build.h"

BEGIN_DUKE_NS


// Iterator wrappers that return an actor pointer, not an index.
class DukeStatIterator : public StatIterator
{
public:
	DukeStatIterator(int stat) : StatIterator(stat)
	{
	}
	
	DDukeActor *Next()
	{
		int n = NextIndex();
		return n >= 0? &hittype[n] : nullptr;
	}

	DDukeActor *Peek()
	{
		int n = PeekIndex();
		return n >= 0? &hittype[n] : nullptr;
	}
};

class DukeSectIterator : public SectIterator
{
public:
	DukeSectIterator(int stat) : SectIterator(stat)
	{
	}
	
	DDukeActor *Next()
	{
		int n = NextIndex();
		return n >= 0? &hittype[n] : nullptr;
	}

	DDukeActor *Peek()
	{
		int n = PeekIndex();
		return n >= 0? &hittype[n] : nullptr;
	}
};

// An interator to iterate over all sprites.
class DukeSpriteIterator
{
	DukeStatIterator it;
	int stat = STAT_DEFAULT;

public:
	DukeSpriteIterator() : it(STAT_DEFAULT) {}

	DDukeActor* Next()
	{
		while (stat < MAXSTATUS)
		{
			auto ac = it.Next();
			if (ac) return ac;
			stat++;
			if (stat < MAXSTATUS) it.Reset(stat);
		}
		return nullptr;
	}
};

inline DDukeActor* player_struct::GetActor()
{
	return &hittype[i];
}

inline int player_struct::GetPlayerNum()
{
	return GetActor()->s.yvel;
}

// Refactoring helpers/intermediates
inline void changespritestat(DDukeActor* a, int newstat)
{
	::changespritestat(a->GetIndex(), newstat);
}

// The int version also needs to be wrapped due to namespacing issues.
inline void changespritestat(int i, int newstat)
{
	::changespritestat(i, newstat);
}

inline void changespritesect(DDukeActor* a, int newsect)
{
	::changespritesect(a->GetIndex(), newsect);
}

inline void changespritesect(int i, int newsect)
{
	::changespritesect(i, newsect);
}

inline int setsprite(DDukeActor* a, int x, int y, int z)
{
	return ::setsprite(a->GetIndex(), x, y, z);
}

inline int setsprite(DDukeActor* a, const vec3_t& pos)
{
	return ::setsprite(a->GetIndex(), pos.x, pos.y, pos.z);
}

// see comment for changespritestat.
inline int setsprite(int i, int x, int y, int z)
{
	return ::setsprite(i, x, y, z);
}

inline int S_PlayActorSound(int soundNum, DDukeActor* spriteNum, int channel = CHAN_AUTO, EChanFlags flags = 0)
{
	return S_PlayActorSound(soundNum, spriteNum ? spriteNum->GetIndex() : -1, channel, flags);
}

inline void S_StopSound(int sndNum, DDukeActor* actor, int flags = -1)
{
	S_StopSound(sndNum, actor ? actor->GetIndex() : -1, flags);
}

inline void S_RelinkActorSound(DDukeActor* from, DDukeActor* to)
{
	S_RelinkActorSound(from ? from->GetIndex() : -1, to ? to->GetIndex() : -1);
}

inline int S_CheckActorSoundPlaying(DDukeActor* a, int soundNum, int channel = 0)
{
	return S_CheckActorSoundPlaying(a->GetIndex(), soundNum, channel);
}

inline DDukeActor* EGS(short whatsect, int s_x, int s_y, int s_z, short s_pn, signed char s_s, signed char s_xr, signed char s_yr, short s_a, short s_ve, int s_zv, DDukeActor* Owner, signed char s_ss)
{
	return &hittype[EGS(whatsect, s_x, s_y, s_z, s_pn, s_s, s_xr, s_yr, s_a, s_ve, s_zv, Owner ? Owner->GetIndex() : -1, s_ss)];
}

inline int ActorToScriptIndex(DDukeActor* a)
{
	if (!a) return -1;
	return a->GetIndex();
}

inline DDukeActor* ScriptIndexToActor(int index)
{
	// only allow valid actors to get through here. Everything else gets null'ed.
	if (index < 0 || index >= MAXSPRITES || hittype[index].s.statnum == MAXSTATUS) return nullptr;
	return &hittype[index];
}

inline bool wallswitchcheck(DDukeActor* s)
{
	return !!(tileinfo[s->s.picnum].flags & TFLAG_WALLSWITCH);
}

inline DDukeActor* spawn(DDukeActor* spawner, int type)
{
	int i = fi.spawn(spawner ? spawner->GetIndex() : -1, type);
	return i == -1 ? nullptr : &hittype[i];
}

inline int ldist(DDukeActor* s1, DDukeActor* s2)
{
	return ldist(&s1->s, &s2->s);
}

inline int dist(DDukeActor* s1, DDukeActor* s2)
{
	return dist(&s1->s, &s2->s);
}

inline int badguy(DDukeActor* pSprite)
{
	return badguypic(pSprite->s.picnum);
}

inline int bossguy(DDukeActor* pSprite)
{
	return bossguypic(pSprite->s.picnum);
}

inline int GetGameVarID(int id, DDukeActor* sActor, int sPlayer)
{
	return GetGameVarID(id, sActor->GetIndex(), sPlayer);
}

inline void SetGameVarID(int id, int lValue, DDukeActor* sActor, int sPlayer)
{
	SetGameVarID(id, lValue, sActor->GetIndex(), sPlayer);
}

// old interface versions of already changed functions

inline void RANDOMSCRAP(spritetype* s, int i)
{
	return RANDOMSCRAP(&hittype[s - sprite]);
}

inline void deletesprite(int num)
{
	deletesprite(&hittype[num]);
}

inline int ssp(int i, unsigned int cliptype) //The set sprite function
{
	return ssp(&hittype[i], cliptype);
}

int movesprite_ex_d(DDukeActor* actor, int xchange, int ychange, int zchange, unsigned int cliptype, Collision& result);
int movesprite_ex_r(DDukeActor* actor, int xchange, int ychange, int zchange, unsigned int cliptype, Collision& result);

inline int movesprite_d(int actor, int xchange, int ychange, int zchange, unsigned int cliptype)
{
	Collision c;
	movesprite_ex_d(&hittype[actor], xchange, ychange, zchange, cliptype, c);
	return c.legacyVal;
}

inline int movesprite_r(int actor, int xchange, int ychange, int zchange, unsigned int cliptype)
{
	Collision c;
	movesprite_ex_r(&hittype[actor], xchange, ychange, zchange, cliptype, c);
	return c.legacyVal;
}

inline int movesprite_ex(DDukeActor* actor, int xchange, int ychange, int zchange, unsigned int cliptype, Collision& result)
{
	auto f = isRR() ? movesprite_ex_r : movesprite_ex_d;
	return f(actor, xchange, ychange, zchange, cliptype, result);
}

inline void ms(short i)
{
	ms(&hittype[i]);
}

inline void execute(DDukeActor* act, int a, int b)
{
	execute(act->GetIndex(), a, b);
}

inline void makeitfall(DDukeActor* act)
{
	makeitfall(act->GetIndex());
}

END_DUKE_NS
