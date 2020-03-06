//-------------------------------------------------------------------------
/*
Copyright (C) 2013 EDuke32 developers and contributors

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

#ifndef EDUKE32_SOUNDS_COMMON_H
#define EDUKE32_SOUNDS_COMMON_H

// Sound flags
enum {
    SF_LOOP = 1,
    SF_MSFX = 2,
    SF_TALK = 4,
    SF_ADULT = 8,
    SF_GLOBAL = 16,
    SF_ONEINST_INTERNAL = 32,

    SF_DTAG = 128,
};

#endif
