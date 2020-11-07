#pragma once

#include "build.h"
#include "common_game.h"
#include "db.h"
#include "ai.h"
#include "nnexts.h"
#include "seq.h"
#include "aiunicult.h"

BEGIN_BLD_NS

extern int cumulDamage[kMaxXSprites];

// Due to the messed up array storage of all the game data we cannot do any direct references here yet. We have to access everything via wrapper functions for now.
// Note that the indexing is very inconsistent - partially by sprite index, partially by xsprite index.
class DBloodActor
{
	int index;
    DBloodActor* base();

public:
    int dudeSlope;

    DBloodActor() :index(int(this - base())) { /*assert(index >= 0 && index < kMaxSprites);*/ }
    DBloodActor& operator=(const DBloodActor& other) = default;
	
	bool hasX() { return sprite[index].extra > 0; }
	spritetype& s() { return sprite[index]; }
	XSPRITE& x() { return xsprite[sprite[index].extra]; }	// calling this does not validate the xsprite!
    SPRITEHIT& hit() { return gSpriteHit[sprite[index].extra]; }
    int& xvel() { return Blood::xvel[index]; }
    int& yvel() { return Blood::yvel[index]; }
    int& zvel() { return Blood::zvel[index]; }

    int& cumulDamage() { return Blood::cumulDamage[sprite[index].extra]; }
    DUDEEXTRA& dudeExtra() { return gDudeExtra[sprite[index].extra]; }
    SPRITEMASS& spriteMass() { return gSpriteMass[sprite[index].extra]; }
    GENDUDEEXTRA& genDudeExtra() { return Blood::gGenDudeExtra[index]; }
    POINT3D& basePoint() { return Blood::baseSprite[index]; }
};

extern DBloodActor bloodActors[kMaxSprites];

inline DBloodActor* DBloodActor::base() { return bloodActors; }

END_BLD_NS
