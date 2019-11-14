#pragma once

#include "filesystem/resourcefile.h"

void OpenSaveGameForWrite(const char *name);
bool OpenSaveGameForRead(const char *name);

FileWriter *WriteSavegameChunk(const char *name);
FileReader ReadSavegameChunk(const char *name);

bool FinishSavegameWrite();
void FinishSavegameRead();
