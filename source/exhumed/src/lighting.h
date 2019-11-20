//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
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

#ifndef __lighting_h__
#define __lighting_h__

extern short nFlashDepth;

void InitLights();
void AddFlash(short nSector, int x, int y, int z, int val);
void SetTorch(int nPlayer, int bTorchOnOff);
void UndoFlashes();
void DoLights();
void AddFlow(int nSprite, int a, int b);
void BuildFlash(short nPlayer, short nSector, int nVal);
void AddGlow(short nSector, int nVal);
void AddFlicker(short nSector, int nVal);

extern short bTorch;

#endif