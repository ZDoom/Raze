#pragma once

#include "filesystem/resourcefile.h"

void OpenSaveGameForWrite(const char *name);
bool OpenSaveGameForRead(const char *name);

FileWriter *WriteSavegameChunk(const char *name);
FileReader ReadSavegameChunk(const char *name);

bool FinishSavegameWrite();
void FinishSavegameRead();

// Savegame utilities
class FileReader;

FString G_BuildSaveName (const char *prefix);
int G_ValidateSavegame(FileReader &fr, FString *savetitle);
void G_WriteSaveHeader(const char *name, const char*mapname, const char *title);

#define SAVEGAME_EXT ".dsave"

