//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#ifndef __grpscan_h__
#define __grpscan_h__

// List of internally-known GRP files
#define numgrpfiles 8
struct grpfile {
	const char *name;
	int32_t crcval;
	int32_t size;
	int32_t game;
	struct grpfile *next;
} grpfiles[numgrpfiles], *foundgrps;

int32_t ScanGroups(void);
void FreeGroups(void);

#endif
