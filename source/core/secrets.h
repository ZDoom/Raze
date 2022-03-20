#pragma once
#include "files.h"

enum ESecretType
{
	Secret_Sector = 0,
	Secret_Sprite,
	Secret_Wall
};

class FSerializer;
void SECRET_Serialize(FSerializer &arc);
void SECRET_SetMapName(const char *filename, const char *maptitle);
bool SECRET_Trigger(int num, int type = Secret_Sector);


