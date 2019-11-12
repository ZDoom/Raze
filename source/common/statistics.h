#pragma once
#include "files.h"

void STAT_StartNewGame(const char *episode, int skill);
void STAT_NewLevel(const char* mapname);
void STAT_Update(bool endofgame);
void STAT_Cancel();

void SaveStatistics(FileWriter& fil);
bool ReadStatistics(FileReader& fil);
