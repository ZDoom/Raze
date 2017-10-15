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
#define DUKE13_CRC  (int32_t)0xBBC9CE44
#define DUKEKR_CRC  (int32_t)0xAA4F6A40
#define DUKE15_CRC  (int32_t)0xFD3DCFF1
#define DUKEPP_CRC  (int32_t)0xF514A6AC
#define DUKEWT_CRC  (int32_t)0x982AFE4A
#define DUKE099_CRC (int32_t)0x02F18900
#define DUKE10_CRC  (int32_t)0xA28AA589
#define DUKE11_CRC  (int32_t)0x912E1E8D
#define DUKESW_CRC  (int32_t)0x983AD923
#define DUKEMD_CRC  (int32_t)0xC5F71561
#define DUKEMD2_CRC  (int32_t)0x73A15EE7
#define DUKEDC13_CRC (int32_t)0xA9242158
#define DUKEDCPP_CRC (int32_t)0xB79D997F
#define DUKEDC_CRC  (int32_t)0xA8CF80DA
#define VACA13_CRC  (int32_t)0x4A2DBB62
#define VACAPP_CRC  (int32_t)0x2F4FCCEE
#define VACA15_CRC  (int32_t)0xB62B42FD
#define DUKECB_CRC  (int32_t)0x18F01C5B
#define DUKENW_CRC  (int32_t)0xF1CAE8E4
#define DZ2_13_CRC  (int32_t)0x82C1B47F
#define DZ2_PP_CRC  (int32_t)0x7FB6117C
#define NAM_CRC     (int32_t)0x75C1F07B
#define NAPALM_CRC  (int32_t)0x3DE1589A
#define WW2GI_CRC   (int32_t)0x907B82BF
#define PLATOONL_CRC (int32_t)0xD1ED8C0C

enum addon_t {
    ADDON_NONE,
    ADDON_DUKEDC,
    ADDON_NWINTER,
    ADDON_CARIBBEAN,
    NUMADDONS
};

typedef struct internalgrpinfo_t {
    char const *name;
    int32_t const crcval;
    int32_t const size;
    int32_t const game;
    int32_t const dependency;
    char const *scriptname;
    void(*postprocessing)(int32_t);
} internalgrpinfo_t;

typedef struct grpinfo_t {
	char *name;
	int32_t crcval;
	int32_t size;
	int32_t game;
    int32_t dependency;
    char *scriptname;
    char *defname;
    void (*postprocessing)(int32_t);
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
