#pragma once
#include "textureid.h"

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