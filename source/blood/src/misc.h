//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------
#pragma once

#include "common.h"
#include "filesystem.h"

using Resource = FileSystem;
// Map NBlood's resource system to our own.
using DICTNODE = FResourceLump;

BEGIN_BLD_NS

void *ResReadLine(char *buffer, unsigned int nBytes, void **pRes);
unsigned int qrand(void);
int wrand(void);
void wsrand(int);
void ChangeExtension(char *pzFile, const char *pzExt);
void SplitPath(const char *pzPath, char *pzDirectory, char *pzFile, char *pzType);
void ConcatPath(const char* pzPath1, const char* pzPath2, char* pzConcatPath);
void FireInit(void);
void FireProcess(void);
void drawLoadingScreen(void);
void UpdateNetworkMenus(void); 
void InitMirrors(void);
void sub_5571C(char mode);
void sub_557C4(int x, int y, int interpolation);
void DrawMirrors(int x, int y, int z, fix16_t a, fix16_t horiz, int smooth, int viewPlayer);
int32_t registerosdcommands(void); 
int qanimateoffs(int a1, int a2);
void qloadpalette();
int32_t qgetpalookup(int32_t a1, int32_t a2);
void HookReplaceFunctions();

END_BLD_NS
