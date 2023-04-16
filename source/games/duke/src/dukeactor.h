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
DDukeActor* spawn(DDukeActor* actj, PClassActor* pname);

// return type is int for scripting - the value must still be true or false!
inline int badguy(const DDukeActor* pSprite)
{
	return !!(pSprite->flags1 & (SFLAG_BADGUY | SFLAG_INTERNAL_BADGUY));
}

inline int bossguy(const DDukeActor* pSprite)
{
	return !!(pSprite->flags1 & SFLAG_BOSS);
}

// old interface versions of already changed functions

int movesprite_ex_d(DDukeActor* actor, const DVector3& change, unsigned int cliptype, Collision& result);
int movesprite_ex_r(DDukeActor* actor, const DVector3& change, unsigned int cliptype, Collision& result);

inline int movesprite_ex(DDukeActor* actor, const DVector3& change, unsigned int cliptype, Collision& result)
{
	auto f = isRR() ? movesprite_ex_r : movesprite_ex_d;
	return f(actor, change, cliptype, result);
}


END_DUKE_NS
