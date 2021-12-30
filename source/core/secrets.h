#pragma once
#include "files.h"

class FSerializer;
void SECRET_Serialize(FSerializer &arc);
void SECRET_SetMapName(const char *filename, const char *maptitle);
bool SECRET_Trigger(int num);


