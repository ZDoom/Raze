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

#ifndef osdcmds_h_
#define osdcmds_h_

#ifdef __cplusplus
extern "C" {
#endif

struct osdcmd_cheatsinfo {
	int32_t cheatnum;	// -1 = none, else = see DoCheats()
	int32_t volume,level;
};

extern struct osdcmd_cheatsinfo osdcmd_cheatsinfo_stat;

int32_t registerosdcommands(void);
void onvideomodechange(int32_t newmode);

extern float r_ambientlight,r_ambientlightrecip;

extern const char *const ConsoleButtons[];

extern uint32_t cl_cheatmask;

#ifdef __cplusplus
}
#endif

#endif	// osdcmds_h_

