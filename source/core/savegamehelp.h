#pragma once

#include "resourcefile.h"
#include "build.h"
#include "gamefuncs.h"
#include "coreactor.h"

// Savegame utilities
class FileReader;
extern int SaveVersion;

FString G_BuildSaveName (const char *prefix);
int G_ValidateSavegame(FileReader &fr, FString *savetitle, bool formenu);

void G_LoadGame(const char* filename, bool hidecon = false);
void G_SaveGame(const char* fn, const char* desc);
void G_DoSaveGame(bool okForQuicksave, bool forceQuicksave, const char* filename, const char* description);
void G_DoLoadGame();

void M_Autosave();

#define SAVEGAME_EXT ".dsave"


template<> inline FSerializer& Serialize(FSerializer& arc, const char* keyname, sectortype*& w, sectortype** def)
{
	assert(arc.isReading() || w == nullptr || (w >= &sector[0] && w <= &sector.Last()));
	int ndx = w ? sectnum(w) : -1;
	arc(keyname, ndx);
	w = !validSectorIndex(ndx) ? nullptr : &sector[ndx];
	return arc;
}

template<> inline FSerializer& Serialize(FSerializer& arc, const char* keyname, walltype*& w, walltype** def)
{
	int ndx = w ? wallnum(w) : -1;
	arc(keyname, ndx);
	w = !validWallIndex(ndx) ? nullptr : &wall[ndx];
	return arc;
}

template<class T>
inline FSerializer& Serialize(FSerializer& arc, const char* keyname, THitInfo<T>& w, THitInfo<T>* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("sect", w.hitSector)
			("sprite", w.hitActor)
			("wall", w.hitWall)
			("x", w.hitpos.X)
			("y", w.hitpos.Y)
			("z", w.hitpos.Z)
			.EndObject();
	}
	return arc;
}

template<class T>
inline FSerializer& Serialize(FSerializer& arc, const char* keyname, TCollision<T>& w, TCollision<T>* def)
{
	if (arc.BeginObject(keyname))
	{
		arc("type", w.type);
		if (w.type == kHitWall) arc("index", w.hitWall);
		else if (w.type == kHitSprite) arc("index", w.hitActor);
		else if (w.type == kHitSector) arc("index", w.hitSector);
		else if (arc.isReading()) w.hitSector = nullptr;
		arc.EndObject();
	}
	return arc;
}
