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

DDukeActor* spawn(DDukeActor* spawner, PClassActor* pname);

// return type is int for scripting - the value must still be true or false!
inline int badguy(const DDukeActor* pSprite)
{
	return !!(pSprite->flags1 & (SFLAG_BADGUY | SFLAG_INTERNAL_BADGUY));
}

inline int bossguy(const DDukeActor* pSprite)
{
	return !!(pSprite->flags1 & SFLAG_BOSS);
}

END_DUKE_NS
