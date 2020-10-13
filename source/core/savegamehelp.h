#pragma once

#include "resourcefile.h"

bool OpenSaveGameForWrite(const char *fname, const char *name);
bool OpenSaveGameForRead(const char *name);

FileWriter *WriteSavegameChunk(const char *name);
FileReader ReadSavegameChunk(const char *name);

bool FinishSavegameWrite();
void FinishSavegameRead();

// Savegame utilities
class FileReader;

FString G_BuildSaveName (const char *prefix);
int G_ValidateSavegame(FileReader &fr, FString *savetitle, bool formenu);

void G_LoadGame(const char* filename);
void G_SaveGame(const char* fn, const char* desc, bool ok4q, bool forceq);

void M_Autosave();

#define SAVEGAME_EXT ".dsave"

