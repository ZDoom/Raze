#pragma once
#include "files.h"

void SECRET_Save(FileWriter &fil);
bool SECRET_Load(FileReader &fil);
void SECRET_SetMapName(const char *filename, const char *maptitle);
void SECRET_Trigger(int num);
 
