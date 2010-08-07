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

//***************************************************************************
//
//    RTS.H
//
//***************************************************************************

#ifndef __rts_public__
#define __rts_public__

extern char rts_lumplockbyte[11];
extern int32_t rts_numlumps;

/*
====================
=
= RTS_Init
=
= Files with a .rts extension are idlink files with multiple lumps
=
====================
*/

void RTS_Init (const char *filename);
/*
====================
=
= RTS_NumSounds
=
====================
*/

int32_t RTS_NumSounds (void);
/*
====================
=
= RTS_SoundLength
=
= Returns the buffer size needed to load the given lump
=
====================
*/

int32_t RTS_SoundLength (int32_t lump);
/*
====================
=
= RTS_GetSoundName
=
====================
*/

const char * RTS_GetSoundName (int32_t i);
/*
====================
=
= RTS_GetSound
=
====================
*/
void *RTS_GetSound (int32_t lump);
#endif
