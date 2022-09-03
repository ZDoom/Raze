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
	return actor->PlayerIndex();
}

DDukeActor* spawn(DDukeActor* spawner, int type);

inline int badguy(DDukeActor* pSprite)
{
	return badguypic(pSprite->spr.picnum);
}

inline int bossguy(DDukeActor* pSprite)
{
	return bossguypic(pSprite->spr.picnum);
}

// old interface versions of already changed functions

int movesprite_ex_d(DDukeActor* actor, int xchange, int ychange, int zchange, unsigned int cliptype, Collision& result);
int movesprite_ex_r(DDukeActor* actor, int xchange, int ychange, int zchange, unsigned int cliptype, Collision& result);

inline int movesprite_ex(DDukeActor* actor, int xchange, int ychange, int zchange, unsigned int cliptype, Collision& result)
{
	auto f = isRR() ? movesprite_ex_r : movesprite_ex_d;
	return f(actor, xchange, ychange, zchange, cliptype, result);
}

inline int movesprite_ex(DDukeActor* actor, const DVector3& change, unsigned int cliptype, Collision& result)
{
	auto f = isRR() ? movesprite_ex_r : movesprite_ex_d;
	return f(actor, change.X * worldtoint, change.Y * worldtoint, change.Z * zworldtoint, cliptype, result);
}


END_DUKE_NS
