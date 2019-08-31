
#ifndef __map_h__
#define __map_h__

#include "compat.h"

extern short bShowTowers;
extern int ldMapZoom;
extern int lMapZoom;

void InitMap();
void GrabMap();
void UpdateMap();
void DrawMap();

#endif
