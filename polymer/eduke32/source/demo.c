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

#include "duke3d.h"
#include "demo.h"
//#include "premap.h"  // G_UpdateScreenArea()
#include "menus.h"
#include "savegame.h"
#include "input.h"

char g_firstDemoFile[BMAX_PATH];

FILE *g_demo_filePtr = (FILE *)NULL;  // write
int32_t g_demo_recFilePtr = -1;  // read

int32_t g_demo_cnt;
int32_t g_demo_goalCnt=0;
int32_t g_demo_totalCnt;
int32_t g_demo_paused=0;
int32_t g_demo_rewind=0;
int32_t g_demo_showStats=1;
static int32_t g_demo_soundToggle;

static int32_t demo_hasdiffs, demorec_diffs=1, demorec_difftics = 2*(TICRATE/TICSPERFRAME);
int32_t demoplay_diffs=1;
int32_t demorec_diffs_cvar=1;
int32_t demorec_force_cvar=0;
int32_t demorec_difftics_cvar = 2*(TICRATE/TICSPERFRAME);
int32_t demorec_diffcompress_cvar=1;
int32_t demorec_synccompress_cvar=1;
int32_t demorec_seeds_cvar=1;
int32_t demoplay_showsync=1;

static int32_t demo_synccompress=1, demorec_seeds=1, demo_hasseeds;

static void Demo_RestoreModes(int32_t menu)
{
    if (menu)
        g_player[myconnectindex].ps->gm |= MODE_MENU;
    else
        g_player[myconnectindex].ps->gm &= ~MODE_MENU;

    g_player[myconnectindex].ps->gm &= ~MODE_GAME;
    g_player[myconnectindex].ps->gm |= MODE_DEMO;
}

void Demo_PrepareWarp(void)
{
    if (!g_demo_paused)
    {
        g_demo_soundToggle = ud.config.SoundToggle;
        ud.config.SoundToggle = 0;
    }

    FX_StopAllSounds();
    S_ClearSoundLocks();
}


static int32_t G_OpenDemoRead(int32_t g_whichDemo) // 0 = mine
{
    char demofn[14];
    int32_t i;

    savehead_t saveh;

    Bstrcpy(demofn, "edemo_.edm");

    if (g_whichDemo == 10)
        demofn[5] = 'x';
    else
        demofn[5] = '0' + g_whichDemo;

    {
        char *demofnptr = (g_whichDemo == 1 && g_firstDemoFile[0]) ? g_firstDemoFile : demofn;

        g_demo_recFilePtr = kopen4loadfrommod(demofnptr, g_loadFromGroupOnly);
        if (g_demo_recFilePtr == -1)
            return 0;
    }

    Bassert(g_whichDemo >= 1);
    i = sv_loadsnapshot(g_demo_recFilePtr, -g_whichDemo, &saveh);
    if (i)
    {
        OSD_Printf(OSD_ERROR "There were errors opening demo %d (code: %d).\n", g_whichDemo, i);
        kclose(g_demo_recFilePtr); g_demo_recFilePtr = -1;
        return 0;
    }

    demo_hasdiffs = saveh.recdiffsp;
    g_demo_totalCnt = saveh.reccnt;
    demo_synccompress = saveh.synccompress;

    demo_hasseeds = demo_synccompress&2;
    demo_synccompress &= 1;

    i = g_demo_totalCnt/(TICRATE/TICSPERFRAME);
    OSD_Printf("demo %d duration: %d min %d sec\n", g_whichDemo, i/60, i%60);

    g_demo_cnt = 1;
    ud.reccnt = 0;

    ud.god = ud.cashman = ud.eog = ud.showallmap = 0;
    ud.noclip = ud.scrollmode = ud.overhead_on = 0; //= ud.pause_on = 0;

    //        G_NewGame(ud.volume_number,ud.level_number,ud.player_skill);
    //        G_ResetTimers();
    totalclock = ototalclock = lockclock = 0;

    return 1;
}

#if KRANDDEBUG
extern void krd_enable(int32_t which);
extern int32_t krd_print(const char *filename);
#endif

void G_OpenDemoWrite(void)
{
    char demofn[BMAX_PATH];
    int32_t i, demonum=1;

    if (ud.recstat == 2)
    {
        kclose(g_demo_recFilePtr);
        g_demo_recFilePtr = -1;
    }

    if ((g_player[myconnectindex].ps->gm&MODE_GAME) && g_player[myconnectindex].ps->dead_flag)
    {
        Bstrcpy(ScriptQuotes[QUOTE_RESERVED4], "CANNOT START DEMO RECORDING WHEN DEAD!");
        P_DoQuote(QUOTE_RESERVED4, g_player[myconnectindex].ps);
        ud.recstat = ud.m_recstat = 0;
        return;
    }

    if (demorec_diffs_cvar && !demorec_force_cvar)
        for (i=1; i<g_scriptSize-2; i++)
        {
            intptr_t w=script[i];
            if ((w&0x0fff)==CON_RESIZEARRAY && (w>>12) && script[i+1]>=0 && script[i+1]<g_gameArrayCount)
            {
                OSD_Printf("\nThe CON code possibly contains a RESIZEARRAY command.\n");
                OSD_Printf("Gamearrays that change their size during the game are unsupported by\n");
                OSD_Printf("the demo recording system. If you are sure that the code doesn't\n");
                OSD_Printf("contain a RESIZEARRAY command, you can force recording with the\n");
                OSD_Printf("`demorec_force' cvar. Alternatively, you can disable diff recording\n");
                OSD_Printf("with the `demorec_diffs' cvar.\n\n");
                Bstrcpy(ScriptQuotes[QUOTE_RESERVED4], "FAILED STARTING DEMO RECORDING. SEE OSD FOR DETAILS.");
                P_DoQuote(QUOTE_RESERVED4, g_player[myconnectindex].ps);
                ud.recstat = ud.m_recstat = 0;
                return;
            }
        }

        do
        {
            if (demonum == 10000)
                return;

            if (G_ModDirSnprintf(demofn, sizeof(demofn), "edemo%d.edm", demonum))
            {
                initprintf("Couldn't start demo writing: INTERNAL ERROR: file name too long\n");
                goto error_wopen_demo;
            }

            demonum++;

            g_demo_filePtr = Bfopen(demofn, "rb");
            if (g_demo_filePtr == NULL)
                break;

            MAYBE_FCLOSE_AND_NULL(g_demo_filePtr);
        }
        while (1);

        g_demo_filePtr = Bfopen(demofn,"wb");
        if (g_demo_filePtr == NULL)
            return;

        i=sv_saveandmakesnapshot(g_demo_filePtr, -1, demorec_diffs_cvar, demorec_diffcompress_cvar,
            demorec_synccompress_cvar|(demorec_seeds_cvar<<1));
        if (i)
        {
            MAYBE_FCLOSE_AND_NULL(g_demo_filePtr);
error_wopen_demo:
            Bstrcpy(ScriptQuotes[QUOTE_RESERVED4], "FAILED STARTING DEMO RECORDING. SEE OSD FOR DETAILS.");
            P_DoQuote(QUOTE_RESERVED4, g_player[myconnectindex].ps);
            ud.recstat = ud.m_recstat = 0;
            return;
        }

        demorec_seeds = demorec_seeds_cvar;
        demorec_diffs = demorec_diffs_cvar;
        demo_synccompress = demorec_synccompress_cvar;
        demorec_difftics = demorec_difftics_cvar;

        Bstrcpy(ScriptQuotes[QUOTE_RESERVED4], "DEMO RECORDING STARTED");
        P_DoQuote(QUOTE_RESERVED4, g_player[myconnectindex].ps);

        ud.reccnt = 0;
        ud.recstat = ud.m_recstat = 1;  //

#if KRANDDEBUG
        krd_enable(1);
#endif
        g_demo_cnt = 1;
}


static uint8_t g_demo_seedbuf[RECSYNCBUFSIZ];

static void Demo_WriteSync()
{
    int16_t tmpreccnt;

    fwrite("sYnC", 4, 1, g_demo_filePtr);
    tmpreccnt = (int16_t)ud.reccnt;
    fwrite(&tmpreccnt, sizeof(int16_t), 1, g_demo_filePtr);
    if (demorec_seeds)
        fwrite(g_demo_seedbuf, 1, ud.reccnt, g_demo_filePtr);

    if (demo_synccompress)
        dfwrite(recsync, sizeof(input_t), ud.reccnt, g_demo_filePtr);
    else //if (demo_synccompress==0)
        fwrite(recsync, sizeof(input_t), ud.reccnt, g_demo_filePtr);

    ud.reccnt = 0;
}

void G_DemoRecord(void)
{
    int16_t i;

    g_demo_cnt++;

    if (demorec_diffs && (g_demo_cnt%demorec_difftics == 1))
    {
        sv_writediff(g_demo_filePtr);
        demorec_difftics = demorec_difftics_cvar;
    }

    if (demorec_seeds)
        g_demo_seedbuf[ud.reccnt] = (uint8_t)(randomseed>>24);

    for (TRAVERSE_CONNECT(i))
    {
        Bmemcpy(&recsync[ud.reccnt], g_player[i].sync, sizeof(input_t));
        ud.reccnt++;
    }

    if (ud.reccnt > RECSYNCBUFSIZ-MAXPLAYERS || (demorec_diffs && (g_demo_cnt%demorec_difftics == 0)))
        Demo_WriteSync();
}

void G_CloseDemoWrite(void)
{
    if (ud.recstat == 1)
    {
        if (ud.reccnt > 0)
            Demo_WriteSync();

        fwrite("EnD!", 4, 1, g_demo_filePtr);

        // lastly, we need to write the number of written recsyncs to the demo file
        if (fseek(g_demo_filePtr, offsetof(savehead_t, reccnt), SEEK_SET))
            perror("G_CloseDemoWrite: final fseek");
        else
            fwrite(&g_demo_cnt, sizeof(g_demo_cnt), 1, g_demo_filePtr);

        ud.recstat = ud.m_recstat = 0;
        MAYBE_FCLOSE_AND_NULL(g_demo_filePtr);

        sv_freemem();

        Bstrcpy(ScriptQuotes[QUOTE_RESERVED4], "DEMO RECORDING STOPPED");
        P_DoQuote(QUOTE_RESERVED4, g_player[myconnectindex].ps);
    }
#if KRANDDEBUG
    krd_print("krandrec.log");
#endif
}

static int32_t g_whichDemo = 1;

static int32_t Demo_UpdateState(int32_t frominit)
{
    int32_t j = g_player[myconnectindex].ps->gm&MODE_MENU;
    int32_t k = sv_updatestate(frominit);
    //                            tmpdifftime = g_demo_cnt+12;
    Demo_RestoreModes(j);

    if (k)
        OSD_Printf("sv_updatestate() returned %d.\n", k);
    return k;
}

#define CORRUPT(code) do { corruptcode=code; goto corrupt; } while(0)

static int32_t Demo_ReadSync(int32_t errcode)
{
    uint16_t si;
    int32_t i;

    if (kread(g_demo_recFilePtr, &si, sizeof(uint16_t)) != sizeof(uint16_t))
        return errcode;

    i = si;
    if (demo_hasseeds)
    {
        if (kread(g_demo_recFilePtr, g_demo_seedbuf, i) != i)
            return errcode;
    }

    if (demo_synccompress)
    {
        if (kdfread(recsync, sizeof(input_t), i, g_demo_recFilePtr) != i)
            return errcode+1;
    }
    else
    {
        int32_t bytes = sizeof(input_t)*i;

        if (kread(g_demo_recFilePtr, recsync, bytes) != bytes)
            return errcode+2;
    }

    ud.reccnt = i;
    return 0;
}


int32_t G_PlaybackDemo(void)
{
    int32_t bigi, j, initsyncofs = 0, lastsyncofs = 0, lastsynctic = 0, lastsyncclock = 0;
    int32_t foundemo = 0, corruptcode, outofsync=0;
    static int32_t in_menu = 0;
    //    static int32_t tmpdifftime=0;

    if (ready2send)
        return 0;

RECHECK:

#if KRANDDEBUG
    if (foundemo)
        krd_print("krandplay.log");
#endif

    in_menu = g_player[myconnectindex].ps->gm&MODE_MENU;

    pub = NUMPAGES;
    pus = NUMPAGES;

    flushperms();

    if (!g_netServer && ud.multimode < 2)
        foundemo = G_OpenDemoRead(g_whichDemo);

    if (foundemo == 0)
    {
        if (g_whichDemo > 1)
        {
            g_whichDemo = 1;
            goto RECHECK;
        }

        fadepal(0,0,0, 0,63,7);
        P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0 /*1*/);    // JBF 20040308
        G_DrawBackground();
        M_DisplayMenus();
        //g_player[myconnectindex].ps->palette = palette;
        nextpage();
        fadepal(0,0,0, 63,0,-7);
        ud.reccnt = 0;
    }
    else
    {
        ud.recstat = 2;
        g_whichDemo++;
        if (g_whichDemo == 10)
            g_whichDemo = 1;

        g_player[myconnectindex].ps->gm &= ~MODE_GAME;
        g_player[myconnectindex].ps->gm |= MODE_DEMO;

        lastsyncofs = ktell(g_demo_recFilePtr);
        initsyncofs = lastsyncofs;
        lastsynctic = g_demo_cnt;
        lastsyncclock = totalclock;
        outofsync = 0;
#if KRANDDEBUG
        krd_enable(2);
#endif
    }

    if (foundemo == 0 || in_menu || I_CheckAllInput() || numplayers > 1)
    {
        FX_StopAllSounds();
        S_ClearSoundLocks();
        g_player[myconnectindex].ps->gm |= MODE_MENU;
    }

    ready2send = 0;
    bigi = 0;

    I_ClearAllInput();

    //    OSD_Printf("ticcnt=%d, total=%d\n", g_demo_cnt, g_demo_totalCnt);
    while (g_demo_cnt < g_demo_totalCnt || foundemo==0)
    {
        // Main loop here. It also runs when there's no demo to show,
        // so maybe a better name for this function would be
        // G_MainLoopWhenNotInGame()?

        if (foundemo && (!g_demo_paused || g_demo_goalCnt))
        {
            if (g_demo_goalCnt>0 && g_demo_goalCnt < g_demo_cnt)
            {
                // initialize rewind or fast-forward

                int32_t menu = g_player[myconnectindex].ps->gm&MODE_MENU;

                if (g_demo_goalCnt > lastsynctic)
                {
                    // fast-forward
                    if (Demo_UpdateState(0)==0)
                    {
                        g_demo_cnt = lastsynctic;
                        klseek(g_demo_recFilePtr, lastsyncofs, SEEK_SET);
                        ud.reccnt = 0;

                        totalclock = ototalclock = lockclock = lastsyncclock;
                    }
                    else CORRUPT(-1);
                }
                else
                {
                    // update to initial state
                    if (Demo_UpdateState(1) == 0)
                    {
                        klseek(g_demo_recFilePtr, initsyncofs, SEEK_SET);
                        g_levelTextTime = 0;

                        g_demo_cnt = 1;
                        ud.reccnt = 0;

                        //                        ud.god = ud.cashman = ud.eog = ud.showallmap = 0;
                        //                        ud.noclip = ud.scrollmode = ud.overhead_on = ud.pause_on = 0;

                        totalclock = ototalclock = lockclock = 0;
                    }
                    else CORRUPT(0);
                }

                Demo_RestoreModes(menu);
            }

            while (totalclock >= (lockclock+TICSPERFRAME)
                //                   || (ud.reccnt > (TICRATE/TICSPERFRAME)*2 && ud.pause_on)
                || (g_demo_goalCnt>0 && g_demo_cnt<g_demo_goalCnt))
            {
                if (ud.reccnt<=0)
                {
                    // Record count reached zero (or <0, corrupted), need
                    // reading another chunk.

                    char tmpbuf[4];

                    if (ud.reccnt<0)
                    {
                        OSD_Printf("G_PlaybackDemo: ud.reccnt<0!\n");
                        CORRUPT(1);
                    }

                    bigi = 0;
                    //reread:
                    if (kread(g_demo_recFilePtr, tmpbuf, 4) != 4)
                        CORRUPT(2);

                    if (Bmemcmp(tmpbuf, "sYnC", 4)==0)
                    {
                        int32_t err = Demo_ReadSync(3);
                        if (err)
                            CORRUPT(err);
                    }

                    else if (demo_hasdiffs && Bmemcmp(tmpbuf, "dIfF", 4)==0)
                    {
                        int32_t k = sv_readdiff(g_demo_recFilePtr);

                        if (k)
                        {
                            OSD_Printf("sv_readdiff() returned %d.\n", k);
                            CORRUPT(6);
                        }
                        else
                        {
                            lastsyncofs = ktell(g_demo_recFilePtr);
                            lastsynctic = g_demo_cnt;
                            lastsyncclock = totalclock;

                            if (kread(g_demo_recFilePtr, tmpbuf, 4) != 4)
                                CORRUPT(7);
                            if (Bmemcmp(tmpbuf, "sYnC", 4))
                                CORRUPT(8);

                            {
                                int32_t err = Demo_ReadSync(9);
                                if (err)
                                    CORRUPT(err);
                            }

                            if ((g_demo_goalCnt==0 && demoplay_diffs) ||
                                (g_demo_goalCnt>0 && ud.reccnt/ud.multimode >= g_demo_goalCnt-g_demo_cnt))
                            {
                                Demo_UpdateState(0);
                            }
                        }
                    }
                    else if (Bmemcmp(tmpbuf, "EnD!", 4)==0)
                        goto nextdemo;
                    else CORRUPT(12);

                    if (0)
                    {
corrupt:
                        OSD_Printf(OSD_ERROR "Demo %d is corrupt (code %d).\n", g_whichDemo-1, corruptcode);
nextdemo:
                        foundemo = 0;
                        ud.reccnt = 0;
                        kclose(g_demo_recFilePtr); g_demo_recFilePtr = -1;
                        g_player[myconnectindex].ps->gm |= MODE_MENU;

                        if (g_demo_goalCnt>0)
                        {
                            g_demo_goalCnt=0;
                            ud.config.SoundToggle = g_demo_soundToggle;
                        }

                        goto RECHECK;
                    }
                }

                if (demo_hasseeds)
                    outofsync = ((uint8_t)(randomseed>>24) != g_demo_seedbuf[bigi]);

                for (TRAVERSE_CONNECT(j))
                {
                    Bmemcpy(&inputfifo[0][j], &recsync[bigi], sizeof(input_t));
                    bigi++;
                    ud.reccnt--;
                }

                g_demo_cnt++;

                if (!g_demo_paused)
                {
                    // assumption that ud.multimode doesn't change in a demo may not be true
                    // sometime in the future                    v v v v v v v v v
                    if (g_demo_goalCnt==0 || !demo_hasdiffs || ud.reccnt/ud.multimode>=g_demo_goalCnt-g_demo_cnt)
                        G_DoMoveThings();  // increases lockclock by TICSPERFRAME
                    else
                        lockclock += TICSPERFRAME;
                }
                else
                {
                    int32_t k = ud.config.SoundToggle;
                    ud.config.SoundToggle = 0;
                    G_DoMoveThings();
                    ud.config.SoundToggle = k;
                }

                ototalclock += TICSPERFRAME;

                if (g_demo_goalCnt > 0)
                {
                    // if fast-forwarding, we must update totalclock
                    totalclock += TICSPERFRAME;

//                    OSD_Printf("t:%d, l+T:%d; cnt:%d, goal:%d%s", totalclock, (lockclock+TICSPERFRAME),
//                               g_demo_cnt, g_demo_goalCnt, g_demo_cnt>=g_demo_goalCnt?" ":"\n");
                    if (g_demo_cnt>=g_demo_goalCnt)
                    {
                        g_demo_goalCnt = 0;
                        ud.config.SoundToggle = g_demo_soundToggle;
                    }
                }
            }
        }
        else if (foundemo && g_demo_paused)
        {
            totalclock = lockclock;
        }

        if (foundemo == 0)
        {
            G_DrawBackground();
        }
        else
        {
            static uint32_t nextrender = 0, framewaiting = 0;
            uint32_t tt;

            G_HandleLocalKeys();

            //            j = min(max((totalclock-lockclock)*(65536/TICSPERFRAME),0),65536);

            if (framewaiting)
            {
                framewaiting--;
#if 0
                if (ud.statusbarmode == 1 && (ud.statusbarscale == 100 || !getrendermode()))
                {
                    ud.statusbarmode = 0;
                    G_UpdateScreenArea();
                }
#endif
                nextpage();
            }

            tt = getticks();

            if (r_maxfps == 0 || tt >= nextrender)
            {
                if (tt > nextrender+g_frameDelay)
                    nextrender = tt;

                nextrender += g_frameDelay;

                j = min(max((totalclock - ototalclock) * (65536 / 4),0),65536);
                if (g_demo_paused && g_demo_rewind)
                    j = 65536-j;

                G_DrawRooms(screenpeek,j);
                G_DisplayRest(j);

                framewaiting++;
            }

            if ((g_player[myconnectindex].ps->gm&MODE_MENU) == 0)
            {
                if (demoplay_showsync && outofsync)
                    gametext(160,100,"OUT OF SYNC",0,2+8+16);

                if (g_demo_showStats)
                {
#if 0
                    if (g_demo_cnt<tmpdifftime)
                        gametext(160,100,"DIFF",0,2+8+16);

                    {
                        char buf[32];
                        Bsprintf(buf, "RC:%4d  TC:%5d", ud.reccnt, g_demo_cnt);
                        gametext(160,100,buf,0,2+8+16);
                    }
#endif
                    j=g_demo_cnt/(TICRATE/TICSPERFRAME);
                    Bsprintf(buf, "%02d:%02d", j/60, j%60);
                    gametext(18,16,buf,0,2+8+16+1024);

                    rotatesprite(60<<16,16<<16,32768,0,SLIDEBAR,0,0,2+8+16+1024,0,0,(xdim*95)/320,ydim-1);
                    rotatesprite(90<<16,16<<16,32768,0,SLIDEBAR,0,0,2+8+16+1024,(xdim*95)/320,0,(xdim*125)/320,ydim-1);
                    rotatesprite(120<<16,16<<16,32768,0,SLIDEBAR,0,0,2+8+16+1024,(xdim*125)/320,0,(xdim*155)/320,ydim-1);
                    rotatesprite(150<<16,16<<16,32768,0,SLIDEBAR,0,0,2+8+16+1024,(xdim*155)/320,0,xdim-1,ydim-1);

                    j = (182<<16) - ((((120*(g_demo_totalCnt-g_demo_cnt))<<4)/g_demo_totalCnt)<<12);
                    rotatesprite_fs(j,(16<<16)+(1<<15),32768,0,SLIDEBAR+1,0,0,2+8+16+1024);

                    j=(g_demo_totalCnt-g_demo_cnt)/(TICRATE/TICSPERFRAME);
                    Bsprintf(buf, "-%02d:%02d%s", j/60, j%60, g_demo_paused?"   ^15PAUSED":"");
                    gametext(194,16,buf,0,2+8+16+1024);
                }
            }

            if ((g_netServer || ud.multimode > 1) && g_player[myconnectindex].ps->gm)
                Net_GetPackets();

            if (g_player[myconnectindex].gotvote == 0 && voting != -1 && voting != myconnectindex)
                gametext(160,60,"Press F1 to Accept, F2 to Decline",0,2+8+16);
        }

        if ((g_player[myconnectindex].ps->gm&MODE_MENU) && (g_player[myconnectindex].ps->gm&MODE_EOL))
            goto RECHECK;

        if (I_EscapeTrigger() && (g_player[myconnectindex].ps->gm&MODE_MENU) == 0 && (g_player[myconnectindex].ps->gm&MODE_TYPE) == 0)
        {
            I_EscapeTriggerClear();
            FX_StopAllSounds();
            S_ClearSoundLocks();
            g_player[myconnectindex].ps->gm |= MODE_MENU;
            ChangeToMenu(0);
            S_MenuSound();
        }

        if (g_player[myconnectindex].ps->gm&MODE_TYPE)
        {
            Net_EnterMessage();
            if ((g_player[myconnectindex].ps->gm&MODE_TYPE) != MODE_TYPE)
                g_player[myconnectindex].ps->gm = MODE_MENU;
        }
        else
        {
            if (ud.recstat != 2)
                M_DisplayMenus();
            if ((g_netServer || ud.multimode > 1)  && g_currentMenu != 20003 && g_currentMenu != 20005 && g_currentMenu != 210)
            {
                ControlInfo noshareinfo;
                CONTROL_GetInput(&noshareinfo);
                if (BUTTON(gamefunc_SendMessage))
                {
                    KB_FlushKeyboardQueue();
                    CONTROL_ClearButton(gamefunc_SendMessage);
                    g_player[myconnectindex].ps->gm = MODE_TYPE;
                    typebuf[0] = 0;
                    inputloc = 0;
                }
            }
        }

        G_PrintGameQuotes(screenpeek);

        if (ud.last_camsprite != ud.camerasprite)
        {
            ud.last_camsprite = ud.camerasprite;
            ud.camera_time = totalclock+(TICRATE*2);
        }

        if (VOLUMEONE)
        {
            if (ud.show_help == 0 && (g_player[myconnectindex].ps->gm&MODE_MENU) == 0)
                rotatesprite_fs((320-50)<<16,9<<16,65536L,0,BETAVERSION,0,0,2+8+16+128);
        }

        G_HandleAsync();

        if (!ud.recstat)
            nextpage();

        if (g_player[myconnectindex].ps->gm == MODE_GAME)
        {
            // user wants to play a game, quit showing demo!

            if (foundemo)
            {
#if KRANDDEBUG
                krd_print("krandplay.log");
#endif
                kclose(g_demo_recFilePtr); g_demo_recFilePtr = -1;
            }

            return 0;
        }
    }

    ud.multimode = numplayers;  // fixes 2 infinite loops after watching demo
    kclose(g_demo_recFilePtr); g_demo_recFilePtr = -1;

    // if we're in the menu, try next demo immediately
    if (g_player[myconnectindex].ps->gm&MODE_MENU)
        goto RECHECK;

#if KRANDDEBUG
    if (foundemo)
        krd_print("krandplay.log");
#endif

    // finished playing a demo and not in menu:
    // return so that e.g. the title can be shown
    return 1;
}
