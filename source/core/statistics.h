#pragma once
#include "files.h"

void STAT_StartNewGame(const char *episode, int skill);
void STAT_NewLevel(const char* mapname);
void STAT_Update(bool endofgame);
void STAT_Cancel();

class FSerializer;
void InitStatistics();
void SerializeStatistics(FSerializer &);
