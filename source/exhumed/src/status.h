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

#ifndef __status_h__
#define __status_h__

BEGIN_PS_NS


extern short nMaskY;
extern short nCounterBullet;
extern short airpages;

void RefreshStatus();
void InitStatus();
void SetPlayerItem(short nPlayer, short nItem);
void SetMagicFrame();
void SetHealthFrame(short nVal);
void SetAirFrame();
void MoveStatus();
void DrawSnakeCamStatus();
void DrawStatus();
int BuildStatusAnim(int val, int nFlags);
void SetCounter(short nVal);
void SetCounterImmediate(short nVal);

END_PS_NS

#endif
