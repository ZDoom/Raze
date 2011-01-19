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

#ifndef __demo_h__
#define __demo_h__

extern FILE *g_demo_filePtr;
extern char firstdemofile[80];

extern int32_t demoplay_diffs;
extern int32_t demoplay_showsync;
extern int32_t demorec_diffcompress_cvar;
extern int32_t demorec_diffs_cvar;
extern int32_t demorec_difftics_cvar;
extern int32_t demorec_force_cvar;
extern int32_t demorec_seeds_cvar;
extern int32_t demorec_synccompress_cvar;
extern int32_t g_demo_cnt;
extern int32_t g_demo_goalCnt;
extern int32_t g_demo_paused;
extern int32_t g_demo_recFilePtr;
extern int32_t g_demo_rewind;
extern int32_t g_demo_showStats;
extern int32_t g_demo_soundToggle;
extern int32_t g_demo_totalCnt;

int32_t G_OpenDemoRead(int32_t g_whichDemo);
int32_t G_PlaybackDemo(void);
int32_t sv_loadsnapshot(int32_t fil,int32_t *ret_hasdiffs,int32_t *ret_demoticcnt,int32_t *ret_synccompress);
int32_t sv_updatestate(int32_t frominit);
void demo_preparewarp(void);
void G_CloseDemoWrite(void);
void G_DemoRecord(void);
void G_OpenDemoWrite(void);

#if KRANDDEBUG
int32_t krd_print(const char *filename);
void krd_enable(int32_t which);
#endif

#endif
