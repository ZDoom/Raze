#pragma once
#include "ns.h"
#include "textureid.h"


BEGIN_PS_NS
#define x(a) kTex##a,

enum ETextureIDs
{
	#include "texidsdef.h"
	TEXID_COUNT
};
#undef x

inline FTextureID aTexIds[TEXID_COUNT];

#define x(a) #a,

inline const char* const texlistnames[] = 
{
	#include "texidsdef.h"
};
#undef x


inline void InitTextureIDs()
{
	for(int i = 0; i < TEXID_COUNT; i++)
	{
		aTexIds[i] = TexMan.CheckForTexture(texlistnames[i], ETextureType::Any);
	}
}

END_PS_NS
