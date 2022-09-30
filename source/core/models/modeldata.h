#pragma once


#include <stdint.h>
#include "vectors.h"

enum {
	HUDFLAG_HIDE = 1,
	HUDFLAG_NOBOB = 2,
	HUDFLAG_FLIPPED = 4,
	HUDFLAG_NODEPTH = 8,
};

struct ModelAnimation
{
	int startframe, endframe;
	int fpssc, flags;
};

struct ModelSkinDef
{
	uint8_t palette, flags;
	int32_t skinnum, surfnum;   // Skin identifier, surface number
	FTextureID texture;
	float param, specpower, specfactor;
};


struct ModelDescriptor
{
	unsigned modelID;

	// stuff that can get set from .DEF so we need to keep it.
	int shadeoff;
	int flags;
	float scale, bscale, zadd, yoffset;
	bool deleted;
	FGameTexture* texture;
	TArray<ModelAnimation> anims;
	TArray<ModelSkinDef> skins;
};

struct ModelTileFrame
{
	// maps build tiles to particular animation frames of a model
	int	modelid;
	int framenum;
	float smoothduration;
	int skinnum;
};



struct ModelManager
{
	TArray<ModelDescriptor> modelDescs;
	TMap<unsigned, ModelTileFrame> frameMap;

	unsigned FrameMapKey(unsigned tilenum, unsigned palette)
	{
		return tilenum + (palette < 16);	// good enough for now - should later be redirected to the underlying texture ID.
	}

	// Interface for the .def parser
	int LoadModel(const char* fn);
	int SetMisc(int modelid, float scale, int shadeoff, float zadd, float yoffset, int flags);
	int DefineFrame(int modelid, const char* framename, int tilenum, int skinnum, float smoothduration, int pal);
	int DefineAnimation(int modelid, const char* framestart, const char* frameend, int fpssc, int flags);
	int DefineSkin(int modelid, const char* skinfn, int palnum, int skinnum, int surfnum, float param, float specpower, float specfactor, int flags);
	int DefineHud(int modelid, int tilex, FVector3 add, int angadd, int flags, int fov);
	int UndefineTile(int tile);
	int UndefineModel(int modelid);
	bool CheckModel(int tilenum, int pal)
	{
		return frameMap.CheckKey(FrameMapKey(tilenum, pal)) != nullptr;
	}
	ModelTileFrame* GetModel(int tilenum, int pal)
	{
		return frameMap.CheckKey(FrameMapKey(tilenum, pal));
	}
};


int tilehasmodelorvoxel(int const tilenume, int pal);
inline ModelManager modelManager;
