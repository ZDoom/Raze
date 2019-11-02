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

#ifndef grpscan_h_
#define grpscan_h_

#ifdef __cplusplus
extern "C" {
#endif

#define MAXLISTNAMELEN 32

// List of internally-known GRP files
#define POWERSLAVE_CRC  (int32_t)0x303CBD89
// TODO:
#define EXHUMED_CRC     (int32_t)0xE3B172F1

#define GAMEFLAG_POWERSLAVE 0x00000001
#define GAMEFLAG_EXHUMED    0x00000002
//#define GAMEFLAG_ADDON      0x00000010
//#define GAMEFLAG_SHAREWARE  0x00000020
#define GAMEFLAGMASK        0x000000FF // flags allowed from grpinfo

typedef struct internalgrpinfo_t {
    char const *name;
    int32_t const crcval;
    int32_t const size;
    int32_t const game;
    int32_t const dependency;
    // char const *scriptname;
    // void(*postprocessing)(int32_t);
} internalgrpinfo_t;

typedef struct grpinfo_t {
	char *name;
	int32_t crcval;
	int32_t size;
	int32_t game;
    int32_t dependency;
    // char *scriptname;
    char *defname;
    // char *rtsname;
    // void (*postprocessing)(int32_t);
	struct grpinfo_t *next;
} grpinfo_t;

typedef struct grpfile_t {
	char *filename;
	struct grpinfo_t const *type;
	struct grpfile_t *next;
} grpfile_t;

extern grpfile_t *foundgrps;
extern grpinfo_t *listgrps;

extern grpfile_t * FindGroup(int32_t crcval);

int32_t ScanGroups(void);
void FreeGroups(void);

#ifdef __cplusplus
}
#endif

#endif
