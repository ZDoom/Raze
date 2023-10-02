#pragma once
#include "dobject.h"
#include "build.h"

BEGIN_DUKE_NS


using DukeStatIterator = TStatIterator<DDukeActor>;
using DukeSectIterator = TSectIterator<DDukeActor>;
using DukeSpriteIterator = TSpriteIterator<DDukeActor>;

inline DDukeActor* DukePlayer::GetActor()
{
	return static_cast<DDukeActor*>(actor);
}

inline int DukePlayer::GetPlayerNum()
{
	return GetActor()->PlayerIndex();
}

DDukeActor* spawn(DDukeActor* spawner, PClassActor* pname);
DDukeActor* spawnsprite(DDukeActor* origin, int typeId);

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
