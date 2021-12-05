#pragma once
#include "dobject.h"
#include "build.h"

BEGIN_DUKE_NS


using DukeStatIterator = TStatIterator<DDukeActor>;
using DukeSectIterator = TSectIterator<DDukeActor>;
using DukeSpriteIterator = TSpriteIterator<DDukeActor>;

inline DDukeActor* player_struct::GetActor()
{
	return actor;
}

inline int player_struct::GetPlayerNum()
{
	return actor->s->yvel;
}

DDukeActor* spawn(DDukeActor* spawner, int type);

inline int ldist(DDukeActor* s1, DDukeActor* s2)
{
	return ldist(s1->s, s2->s);
}

inline int dist(DDukeActor* s1, DDukeActor* s2)
{
	return dist(s1->s, s2->s);
}

inline int badguy(DDukeActor* pSprite)
{
	return badguypic(pSprite->s->picnum);
}

inline int bossguy(DDukeActor* pSprite)
{
	return bossguypic(pSprite->s->picnum);
}

// old interface versions of already changed functions

int movesprite_ex_d(DDukeActor* actor, int xchange, int ychange, int zchange, unsigned int cliptype, Collision& result);
int movesprite_ex_r(DDukeActor* actor, int xchange, int ychange, int zchange, unsigned int cliptype, Collision& result);

inline int movesprite_ex(DDukeActor* actor, int xchange, int ychange, int zchange, unsigned int cliptype, Collision& result)
{
	auto f = isRR() ? movesprite_ex_r : movesprite_ex_d;
	return f(actor, xchange, ychange, zchange, cliptype, result);
}



END_DUKE_NS
