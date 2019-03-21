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

//#define MAIN
//#define QUIET
#include "build.h"
#include "cache1d.h"

#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "game.h"
#include "network.h"

#include "mytypes.h"
#include "control.h"
#include "function.h"
#include "demo.h"

#include "player.h"
#include "menus.h"


DFILE DemoFileIn = DF_ERR;
FILE *DemoFileOut;
SWBOOL DemoPlaying = FALSE;
SWBOOL DemoRecording = FALSE;
SWBOOL DemoEdit = FALSE;
SWBOOL DemoMode = FALSE;
SWBOOL DemoModeMenuState = FALSE;
SWBOOL DemoOverride = FALSE;
char DemoFileName[16] = "demo.dmo";
char DemoLevelName[16] = "";
extern SWBOOL NewGame;

// Demo sync stuff
FILE *DemoSyncFile;
SWBOOL DemoSyncTest = FALSE, DemoSyncRecord = FALSE;
char DemoTmpName[16] = "";

SW_PACKET DemoBuffer[DEMO_BUFFER_MAX];
int DemoRecCnt = 0;                    // Can only record 1-player game

SWBOOL DemoDone;

void DemoWriteHeader(void);
void DemoReadHeader(void);
void DemoReadBuffer(void);


//
// DemoDebug Vars
//

// DemoDebugMode will close the file after every write
SWBOOL DemoDebugMode = FALSE;
//SWBOOL DemoDebugMode = TRUE;
SWBOOL DemoInitOnce = FALSE;
short DemoDebugBufferMax = 1;

extern char LevelName[];
extern char LevelSong[16];
extern uint8_t FakeMultiNumPlayers;
extern SWBOOL QuitFlag;

///////////////////////////////////////////
//
// Demo File Manipulation
//
///////////////////////////////////////////

char *DemoSyncFileName(void)
{
    static char file_name[32];
    char *ptr;

    strcpy(file_name, DemoFileName);

    if ((ptr = strchr(file_name, '.')) == 0)
        strcat(file_name, ".dms");
    else
    {
        *ptr = '\0';
        strcat(file_name, ".dms");
    }

    return file_name;
}

void
DemoSetup(void)
{
    if (DemoRecording)
    {
        if (DemoSyncRecord)
            DemoSyncFile = fopen(DemoSyncFileName(),"wb");

        DemoWriteHeader();
        memset(&DemoBuffer, -1, sizeof(DemoBuffer));
    }

    if (DemoPlaying)
    {
        if (DemoSyncRecord)
            DemoSyncFile = fopen(DemoSyncFileName(),"wb");
        if (DemoSyncTest)
            DemoSyncFile = fopen(DemoSyncFileName(),"rb");

        DemoReadHeader();
        memset(&DemoBuffer, -1, sizeof(DemoBuffer));
        DemoReadBuffer();
    }
}

void
DemoRecordSetup(void)
{
    if (DemoRecording)
    {
        if (DemoSyncRecord)
            DemoSyncFile = fopen(DemoSyncFileName(),"wb");

        DemoWriteHeader();
        memset(&DemoBuffer, -1, sizeof(DemoBuffer));
    }
}

void
DemoPlaySetup(void)
{
    if (DemoPlaying)
    {
        if (DemoSyncRecord)
            DemoSyncFile = fopen(DemoSyncFileName(),"wb");
        if (DemoSyncTest)
            DemoSyncFile = fopen(DemoSyncFileName(),"rb");

        DemoReadHeader();
        memset(&DemoBuffer, -1, sizeof(DemoBuffer));
        DemoReadBuffer();
    }
}

void
DemoWriteHeader(void)
{
    DEMO_HEADER dh;
    DEMO_START_POS dsp;
    PLAYERp pp;

    DemoFileOut = fopen(DemoFileName, "wb");

    if (!DemoFileOut)
        return;

    strcpy(dh.map_name, LevelName);
    strcpy(dh.LevelSong, LevelSong);
    dh.Level = Level;

    if (FakeMultiNumPlayers)
        dh.numplayers = FakeMultiNumPlayers;
    else
        dh.numplayers = numplayers;

    fwrite(&dh, sizeof(dh), 1, DemoFileOut);

    for (pp = Player; pp < Player + dh.numplayers; pp++)
    {
        dsp.x = pp->posx;
        dsp.y = pp->posy;
        dsp.z = pp->posz;
        fwrite(&dsp, sizeof(dsp), 1, DemoFileOut);
        fwrite(&pp->Flags, sizeof(pp->Flags), 1, DemoFileOut);
        fwrite(&pp->pang, sizeof(pp->pang), 1, DemoFileOut);
    }

    fwrite(&Skill, sizeof(Skill), 1, DemoFileOut);
    fwrite(&gNet, sizeof(gNet), 1, DemoFileOut);

    if (DemoDebugMode)
    {
        DemoDebugBufferMax = numplayers;
        fclose(DemoFileOut);
    }
}

void
DemoReadHeader(void)
{
    DEMO_HEADER dh;
    DEMO_START_POS dsp;
    PLAYERp pp;

#if DEMO_FILE_TYPE != DEMO_FILE_GROUP
    if (DemoEdit)
    {
        DemoFileIn = fopen(DemoFileName, "rb+");
    }
    else
#endif
    {
        //DemoFileIn = fopen(DemoFileName, "rb");
        DemoFileIn = DOPEN_READ(DemoFileName);
    }

    if (DemoFileIn == DF_ERR)
    {
        TerminateGame();
        printf("File %s is not a valid demo file.",DemoFileName);
        exit(0);
    }

    DREAD(&dh, sizeof(dh), 1, DemoFileIn);

    strcpy(DemoLevelName, dh.map_name);
    strcpy(LevelSong, dh.LevelSong);
    Level = dh.Level;
    if (dh.numplayers > 1)
    {
        FakeMultiNumPlayers = dh.numplayers;
    }
    else
        numplayers = dh.numplayers;

    for (pp = Player; pp < Player + dh.numplayers; pp++)
    {
        DREAD(&dsp, sizeof(dsp), 1, DemoFileIn);
        pp->posx = dsp.x;
        pp->posy = dsp.y;
        pp->posz = dsp.z;
        COVERupdatesector(pp->posx, pp->posy, &pp->cursectnum);
        //pp->cursectnum = 0;
        //updatesectorz(pp->posx, pp->posy, pp->posz, &pp->cursectnum);
        DREAD(&pp->Flags, sizeof(pp->Flags), 1, DemoFileIn);
        DREAD(&pp->pang, sizeof(pp->pang), 1, DemoFileIn);
    }

    DREAD(&Skill, sizeof(Skill), 1, DemoFileIn);
    DREAD(&gNet, sizeof(gNet), 1, DemoFileIn);
}

void
DemoDebugWrite(void)
{
    int size;

    DemoFileOut = fopen(DemoFileName, "ab");

    ASSERT(DemoFileOut);

    size = sizeof(SW_PACKET) * DemoDebugBufferMax;
    fwrite(&DemoBuffer, size, 1, DemoFileOut);
    memset(&DemoBuffer, -1, size);

    fclose(DemoFileOut);
}

void
DemoWriteBuffer(void)
{
    fwrite(&DemoBuffer, sizeof(DemoBuffer), 1, DemoFileOut);
    memset(&DemoBuffer, -1, sizeof(DemoBuffer));
}

void
DemoReadBuffer(void)
{
    memset(&DemoBuffer, -1, sizeof(DemoBuffer));
    DREAD(&DemoBuffer, sizeof(DemoBuffer), 1, DemoFileIn);
}

void
DemoBackupBuffer(void)
{
#if DEMO_FILE_TYPE != DEMO_FILE_GROUP
    FILE *NewDemoFile;
    FILE *OldDemoFile = DemoFileIn;
    int pos,i;
    char copy_buffer;
    char NewDemoFileName[16] = "!";

    // seek backwards to beginning of last buffer
    fseek(OldDemoFile, -sizeof(DemoBuffer), SEEK_CUR);
    pos = ftell(OldDemoFile);

    // open a new edit file
    strcat(NewDemoFileName, DemoFileName);
    NewDemoFile = fopen(NewDemoFileName, "wb");

    rewind(OldDemoFile);

    // copy old demo to new demo
    for (i = 0; i < pos; i++)
    {
        fread(&copy_buffer, sizeof(copy_buffer), 1, OldDemoFile);
        fwrite(&copy_buffer,sizeof(copy_buffer), 1, NewDemoFile);
    }

    DemoFileOut = NewDemoFile;
    fclose(OldDemoFile);
#endif
}

void
DemoTerm(void)
{
    if (DemoRecording)
    {
        // if already closed
        if (DemoFileOut == NULL)
            return;

        if (DemoDebugMode)
        {
            DemoFileOut = fopen(DemoFileName, "ab");
            ASSERT(DemoFileOut);
        }
        else
        {
            // paste on a -1 record to the current buffer
            if (DemoRecCnt < DEMO_BUFFER_MAX)
                memset(&DemoBuffer[DemoRecCnt], -1, sizeof(DemoBuffer[DemoRecCnt]));

            DemoWriteBuffer();
        }

        // write at least 1 record at the end filled with -1
        // just for good measure
        memset(&DemoBuffer[0], -1, sizeof(DemoBuffer[0]));
        fwrite(&DemoBuffer[0], sizeof(DemoBuffer[0]), 1, DemoFileOut);

        fclose(DemoFileOut);
        DemoFileOut = NULL;
    }

    if (DemoPlaying)
    {
        if (DemoFileIn == DF_ERR)
            return;

        DCLOSE(DemoFileIn);
        DemoFileIn = DF_ERR;
    }

    if (DemoSyncTest||DemoSyncRecord)
    {
        fclose(DemoSyncFile);
        DemoSyncFile = NULL;
    }
}

///////////////////////////////////////////
//
// Demo Play Back
//
///////////////////////////////////////////


void
DemoPlayBack(void)
{
    int pnum, cnt;
    static int buf_ndx;
    PLAYERp pp;
    ControlInfo info;
    int Xdim, Ydim, ScreenSize;

    if (SW_SHAREWARE)
    {
        // code here needs to be similar to RunLevel startup code
        PlaySong(LevelSong, -1, TRUE, TRUE);
    }


    // Initialize Game part of network code (When ready2send != 0)
    InitNetVars();

    // IMPORTANT - MUST be right before game loop
    InitTimingVars();

    // THIS STUFF DEPENDS ON MYCONNECTINDEX BEING SET RIGHT
    pp = Player + myconnectindex;
    SetRedrawScreen(pp);

    if (!DemoInitOnce)
        buf_ndx = 0;

    // everything has been inited at least once for PLAYBACK
    DemoInitOnce = TRUE;

    cnt = 0;
    ready2send = 0;
    DemoDone = FALSE;

    while (TRUE)
    {
        // makes code run at the same rate
        while (totalclock > totalsynctics)
        {
            handleevents();

            TRAVERSE_CONNECT(pnum)
            {
                pp = Player + pnum;
                pp->inputfifo[pp->movefifoend & (MOVEFIFOSIZ-1)] = DemoBuffer[buf_ndx];
                pp->movefifoend++;
                buf_ndx++;

                if (pp->inputfifo[(pp->movefifoend - 1) & (MOVEFIFOSIZ-1)].bits == -1)
                {
                    DemoDone = TRUE;
                    break;
                }

                if (buf_ndx > DEMO_BUFFER_MAX - 1)
                {
                    DemoReadBuffer();
                    buf_ndx = 0;
                }
            }

            if (DemoDone)
                break;

            cnt++;

            CONTROL_GetInput(&info);

            domovethings();

            MNU_CheckForMenus();

            // fast forward and slow mo
            if (DemoEdit)
            {
                if (KEY_PRESSED(KEYSC_F))
                {
                    if (KEY_PRESSED(KEYSC_LSHIFT) || KEY_PRESSED(KEYSC_RSHIFT))
                        totalclock += synctics;
                    else
                        totalclock += synctics-1;
                }

                if (KEY_PRESSED(KEYSC_S))
                    totalclock += 1-synctics;
            }
            else
            {
#if DEBUG
                if (KEY_PRESSED(KEYSC_ALT) && KEY_PRESSED(KEYSC_CTRL) && KEY_PRESSED(KEYSC_S))
                {
                    KEY_PRESSED(KEYSC_ALT) = KEY_PRESSED(KEYSC_CTRL) = KEY_PRESSED(KEYSC_S) = 0;
                    saveboard("demosave.map", (vec3_t *)Player, &Player->pang, &Player->cursectnum);
                }
#endif

                if (BUTTON(gamefunc_See_Co_Op_View))
                {
                    CONTROL_ClearButton(gamefunc_See_Co_Op_View);

                    screenpeek = connectpoint2[screenpeek];

                    if (screenpeek < 0)
                        screenpeek = connecthead;
                }

#if DEBUG
                if (KEY_PRESSED(KEYSC_RIGHT) || KEY_PRESSED(KEYSC_UP))
                {
                    if (KEY_PRESSED(KEYSC_LSHIFT) || KEY_PRESSED(KEYSC_RSHIFT))
                        totalclock += synctics;
                    else
                        totalclock += synctics-1;
                }

                if (KEY_PRESSED(KEYSC_LEFT) || KEY_PRESSED(KEYSC_DOWN))
                    totalclock += 1-synctics;
#endif
            }


            if (DemoSyncRecord)
                demosync_record();
            if (DemoSyncTest)
                demosync_test(cnt);
        }

        // Put this back in later when keyboard stuff is stable
        if (DemoEdit)
        {
            //CONTROL_GetButtonInput();
            CONTROL_GetInput(&info);

            // if a key is pressed, start recording from the point the key
            // was pressed
            if (BUTTON(gamefunc_Move_Forward) ||
                BUTTON(gamefunc_Move_Backward) ||
                BUTTON(gamefunc_Turn_Left) ||
                BUTTON(gamefunc_Turn_Right) ||
                BUTTON(gamefunc_Fire) ||
                BUTTON(gamefunc_Open) ||
                BUTTON(gamefunc_Jump) ||
                BUTTON(gamefunc_Crouch) ||
                BUTTON(gamefunc_Look_Up) ||
                BUTTON(gamefunc_Look_Down))
            {
                DemoBackupBuffer();

                DemoRecCnt = buf_ndx;
                DemoPlaying = FALSE;
                DemoRecording = TRUE;
                return;
            }
        }

        if (BUTTON(gamefunc_See_Co_Op_View))
        {
            screenpeek += 1;
            if (screenpeek > numplayers-1)
                screenpeek = 0;
        }

        // demo is over
        if (DemoDone)
            break;

        if (QuitFlag)
        {
            DemoMode = FALSE;
            break;
        }

        if (ExitLevel)
        {
            // Quiting Demo
            ExitLevel = FALSE;
            if (DemoMode)
            {
                DemoPlaying = FALSE;
                DemoMode = FALSE;
            }
            break;
        }

        drawscreen(Player + screenpeek);
    }

    // only exit if conditions are write
    if (DemoDone && !DemoMode && !NewGame)
    {
        TerminateLevel();
        TerminateGame();
        exit(0);
    }

}

//
// Still using old method of playback - this was for opening demo
//

void
ScenePlayBack(void)
{
    int buf_ndx, pnum, cnt;
    PLAYERp pp;

    if (SW_SHAREWARE)
    {
        // code here needs to be similar to RunLevel startup code
        strcpy(LevelSong,"yokoha03.mid");
        PlaySong(LevelSong, -1, TRUE, TRUE);
    }

    // IMPORTANT - MUST be right before game loop
    InitTimingVars();

    buf_ndx = 0;
    cnt = 0;
    ready2send = 0;
    DemoDone = FALSE;

    ResetKeys();

    while (TRUE)
    {
        // makes code run at the same rate
        while ((totalclock > totalsynctics))
        {
            TRAVERSE_CONNECT(pnum)
            {
                pp = Player + pnum;
                pp->inputfifo[pp->movefifoend & (MOVEFIFOSIZ - 1)] = DemoBuffer[buf_ndx];
                pp->movefifoend++;
                buf_ndx++;

                if (pp->inputfifo[(pp->movefifoend - 1) & (MOVEFIFOSIZ - 1)].bits == -1)
                {
                    DemoDone = TRUE;
                    break;
                }

                if (buf_ndx > DEMO_BUFFER_MAX - 1)
                {
                    DemoReadBuffer();
                    buf_ndx = 0;
                }
            }

            if (KeyPressed())
                DemoDone = TRUE;

            if (DemoDone)
                break;

            cnt++;

            //movethings();
            domovethings();

            MNU_CheckForMenus();
        }

        // demo is over
        if (DemoDone)
            break;

        drawscreen(Player + screenpeek);
    }
}



