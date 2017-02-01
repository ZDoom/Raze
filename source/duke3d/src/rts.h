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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

//***************************************************************************
//
//    RTS.H
//
//***************************************************************************

#ifndef rts_public_
#define rts_public_

extern char rts_lumplockbyte[11];

/* Files with a .rts extension are idlink files with multiple lumps */
void RTS_Init(const char *filename);

int32_t RTS_IsInitialized(void);

/* Returns the buffer size needed to load the given lump */
int32_t RTS_SoundLength(int32_t lump);

void *RTS_GetSound(int32_t lump);

#endif
