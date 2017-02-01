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

extern FILE *DemoFile;
extern SWBOOL DemoPlaying;
extern SWBOOL DemoRecording;
extern SWBOOL DemoEdit;
extern SWBOOL DemoMode;
extern SWBOOL DemoOverride;
extern char DemoFileName[16];
extern char DemoLevelName[16];

extern FILE *DemoSyncFile;
extern SWBOOL DemoSyncTest;
extern SWBOOL DemoSyncRecord;
extern char DemoTmpName[16];

extern SWBOOL DemoDebugMode;
extern SWBOOL DemoInitOnce;
extern short DemoDebugBufferMax;

#define DEMO_BUFFER_MAX 2048
extern SW_PACKET DemoBuffer[DEMO_BUFFER_MAX];
extern int DemoRecCnt;                    // Can only record 1-player game

#define DEMO_FILE_GROUP 0
#define DEMO_FILE_STD   1
#define DEMO_FILE_TYPE DEMO_FILE_GROUP

// Demo File - reading from group
#if DEMO_FILE_TYPE == DEMO_FILE_GROUP
typedef long DFILE;
#define DREAD(ptr, size, num, handle) kread((handle),(ptr),(size)*(num))
#define DOPEN_READ(name) kopen4load(name,0)
#define DCLOSE(handle) kclose(handle)
#define DF_ERR -1
#else
typedef FILE *DFILE;
#define DREAD(ptr, size, num,handle) fread((ptr),(size),(num),(handle))
#define DWRITE(ptr, size, num,handle) fwrite((ptr),(size),(num),(handle))
#define DOPEN_WRITE(name) fopen(name,"wb")
#define DOPEN_READ(name) fopen(name,"rb")
#define DCLOSE(handle) fclose(handle)
#define DF_ERR NULL
#endif

void DemoTerm(void);
void DemoPlaySetup(void);
void DemoPlayBack(void);
void ScenePlayBack(void);
void DemoRecordSetup(void);
void DemoDebugWrite(void);
void DemoWriteBuffer(void);
