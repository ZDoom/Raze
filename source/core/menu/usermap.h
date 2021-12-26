#pragma once

#include "maptypes.h"

struct FUsermapEntry
{
	FString displayname;
	FString container;
	FString filename;
	FString info;
	int size = 0;
	bool wallsread  = false;
	TArray<walltype> walls;	// for rendering a preview of the map
};

struct FUsermapDirectory
{
	FString dirname;
	FUsermapDirectory* parent = nullptr;
	TArray<FUsermapDirectory> subdirectories;
	TArray<FUsermapEntry> entries;
};

