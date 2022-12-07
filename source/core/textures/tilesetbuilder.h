#pragma once

#include "texinfo.h"

struct TileBuildDesc
{
	// persistent data that will be present for the entire run of the game.
	TexExtInfo extinfo;				

	// work data
	FImageSource* orgimage;				// this is the original tile image, not connected to a FTexture yet.
	FImageSource* tileimage;			// this is the current tile image, it may be connected to 'imported'. Used for convenient access.
	FGameTexture* imported;				// imported replacement from the texture manager;

	// info that will be copied into the final texture object
	float alphathreshold;
	int leftOffset, topOffset;			// overrides for imported textures.
};

struct TilesetBuildInfo
{
	TArray<TileBuildDesc> tile;
	unsigned maxtileofart;
	TArray <std::pair<FName, int>> aliases;
	void addName(const char* name, int index)
	{
		aliases.Push(std::make_pair(name, index));
	}

	void Delete(int tileno)
	{
		tile[tileno].tileimage = nullptr;
		tile[tileno].imported = nullptr;
	}

	void MakeWritable(int tile);
	void CreateWritable(int tile, int w, int h);
	void MakeCanvas(int tilenum, int width, int height);

};

void ConstructTileset();
int32_t tileGetCRC32(FImageSource* image);

