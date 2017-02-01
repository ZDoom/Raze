//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

#define TEXT_INFO_TIME (3)
#define TEXT_INFO_X (16)
//#define TEXT_INFO_Y (20)
#define TEXT_INFO_Y (40)
#define TEXT_INFO_YOFF (10)
#define TEXT_INFO_LINE(line) (TEXT_INFO_Y + ((line) * TEXT_INFO_YOFF))
//#define TEXT_INFO_LINE(line) (TEXT_INFO_Y + ((line) * TEXT_INFO_YOFF) + GlobalInfoLineOffset)

void DisplayFragNames(PLAYERp pp);
void DisplayMiniBarSmString(PLAYERp pp,short xs,short ys, short pal, const char *buffer);
void DisplaySmString(PLAYERp pp, short xs, short ys, short pal, const char *buffer);
void DisplayMiniBarNumber(PLAYERp pp,short xs,short ys,int number);
void DisplaySummaryString(PLAYERp pp,short xs,short ys,short color,short shade,const char *buffer);
void DisplayPanelNumber(PLAYERp pp,short xs,short ys,int number);
void PutStringInfo(PLAYERp pp, const char *string);
void PutStringInfoLine(PLAYERp pp, const char *string);
void PutStringInfoLine2(PLAYERp pp, const char *string);
void pClearTextLine(PLAYERp pp,long y);
void pMenuClearTextLine(PLAYERp pp);
