#pragma once

#include "serializer.h"

class PClassActor;
struct FState;

class FRazeSerializer : public FSerializer
{
	
public:
	FSerializer &Sprite(const char *key, int32_t &spritenum, int32_t *def) override;
	FSerializer& StatePointer(const char* key, void* ptraddr, bool *res) override;

};

template<> FSerializer &Serialize(FSerializer &arc, const char *key, PClassActor *&clst, PClassActor **def);
FSerializer &Serialize(FSerializer &arc, const char *key, FState *&state, FState **def, bool *retcode);
template<> inline FSerializer &Serialize(FSerializer &arc, const char *key, FState *&state, FState **def)
{
	return Serialize(arc, key, state, def, nullptr);
}
