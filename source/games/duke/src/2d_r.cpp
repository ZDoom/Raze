//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2017-2019 Nuke.YKT

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

// this file collects all 2D content of the game that was scattered across multiple sources originally.
// All this should transition to a more modern, preferably localization friendly, approach later.

#include "ns.h"
#include "duke3d.h"
#include "names_rr.h"
#include "animtexture.h"
#include "animlib.h"
#include "raze_music.h"
#include "mapinfo.h"
#include "screenjob.h"
#include "texturemanager.h"
//#include "zz_text.h"

#undef gametext
//#undef menutext

BEGIN_DUKE_NS


//==========================================================================
//
// Sets up the game fonts.
// This is a duplicate of the _d function but needed since the tile numbers differ.
//
//==========================================================================

void InitFonts_r()
{
    GlyphSet fontdata;

    // Small font
    for (int i = 0; i < 95; i++)
    {
        auto tile = tileGetTexture(STARTALPHANUM + i);
        if (tile && tile->isValid() && tile->GetTexelWidth() > 0 && tile->GetTexelHeight() > 0)
            fontdata.Insert('!' + i, tile);
    }
    SmallFont = new ::FFont("SmallFont", nullptr, "defsmallfont", 0, 0, 0, -1, 10, false, false, false, &fontdata);
    SmallFont->SetKerning(2);
    fontdata.Clear();

    // Big font

    // This font is VERY messy...
    fontdata.Insert('_', tileGetTexture(BIGALPHANUM - 11));
    fontdata.Insert('-', tileGetTexture(BIGALPHANUM - 11));
    for (int i = 0; i < 10; i++) fontdata.Insert('0' + i, tileGetTexture(BIGALPHANUM - 10 + i));
    for (int i = 0; i < 26; i++) fontdata.Insert('A' + i, tileGetTexture(BIGALPHANUM + i));
    fontdata.Insert('.', tileGetTexture(BIGPERIOD));
    fontdata.Insert(',', tileGetTexture(BIGCOMMA));
    fontdata.Insert('!', tileGetTexture(BIGX));
    fontdata.Insert('?', tileGetTexture(BIGQ));
    fontdata.Insert(';', tileGetTexture(BIGSEMI));
    fontdata.Insert(':', tileGetTexture(BIGCOLIN));
    fontdata.Insert('\\', tileGetTexture(BIGALPHANUM + 68));
    fontdata.Insert('/', tileGetTexture(BIGALPHANUM + 68));
    fontdata.Insert('%', tileGetTexture(BIGALPHANUM + 69));
    fontdata.Insert('`', tileGetTexture(BIGAPPOS));
    fontdata.Insert('"', tileGetTexture(BIGAPPOS));
    fontdata.Insert('\'', tileGetTexture(BIGAPPOS));
    GlyphSet::Iterator it(fontdata);
    GlyphSet::Pair* pair;
    while (it.NextPair(pair)) pair->Value->SetOffsetsNotForFont();
    BigFont = new ::FFont("BigFont", nullptr, "defbigfont", 0, 0, 0, -1, 10, false, false, false, &fontdata);
    fontdata.Clear();

    // Tiny font
    for (int i = 0; i < 95; i++)
    {
        auto tile = tileGetTexture(MINIFONT + i);
        if (tile && tile->isValid() && tile->GetTexelWidth() > 0 && tile->GetTexelHeight() > 0)
            fontdata.Insert('!' + i, tile);
    }
    fontdata.Insert(1, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.
    SmallFont2 = new ::FFont("SmallFont2", nullptr, "defsmallfont2", 0, 0, 0, -1, 6, false, false, false, &fontdata);
    SmallFont2->SetKerning(2);
    fontdata.Clear();

    // SBAR index font
    for (int i = 0; i < 10; i++) fontdata.Insert('0' + i, tileGetTexture(THREEBYFIVE + i));
    fontdata.Insert(':', tileGetTexture(THREEBYFIVE + 10));
    fontdata.Insert('/', tileGetTexture(THREEBYFIVE + 11));
    fontdata.Insert('%', tileGetTexture(MINIFONT + '%' - '!'));
    fontdata.Insert(1, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.
    IndexFont = new ::FFont("IndexFont", nullptr, nullptr, 0, 0, 0, -1, -1, false, false, false, &fontdata);

    fontdata.Clear();

    // digital font
    for (int i = 0; i < 10; i++) fontdata.Insert('0' + i, tileGetTexture(DIGITALNUM + i));
    fontdata.Insert(1, TexMan.FindGameTexture("TINYBLAK")); // this is only here to widen the color range of the font to produce a better translation.
    DigiFont = new ::FFont("DigiFont", nullptr, nullptr, 0, 0, 0, -1, -1, false, false, false, &fontdata);

}

//==========================================================================
//
// wrappers around DrawText to allow easier reuse of the old code.
// The vertical displacements are to have the same positioning as with the original code.
//
//==========================================================================

static void BigText(double x, double y, const char* text)
{
    x *= 2; y *= 2;
    auto width = BigFont->StringWidth(text);
    DrawText(twod, BigFont, CR_UNTRANSLATED, x - width / 2, y - 24, text, DTA_FullscreenScale, 3, DTA_VirtualWidth, 640, DTA_VirtualHeight, 400, TAG_DONE);
}

static void GameText(double x, double y, const char* t, int shade, int align = -1, int trans = 0)
{
    x *= 2; y *= 2;
    if (align != -1)
        x -= SmallFont->StringWidth(t) * (align == 0 ? 0.5 : 1);
    int light = Scale(numshades - shade, 255, numshades);
    PalEntry pe(255, light, light, light);
    DrawText(twod, SmallFont, CR_UNDEFINED, x, y + 2, t, DTA_FullscreenScale, 3, DTA_VirtualWidth, 640, DTA_VirtualHeight, 400, DTA_TranslationIndex, TRANSLATION(Translation_Remap, trans), DTA_Color, pe, TAG_DONE);
}

static void MiniText(double x, double y, const char* t, int shade, int align = -1, int trans = 0)
{
    x *= 2; y *= 2;
    if (align != -1)
        x -= SmallFont2->StringWidth(t) * (align == 0 ? 0.5 : 1);
    int light = Scale(numshades - shade, 255, numshades);
    PalEntry pe(255, light, light, light);
    DrawText(twod, SmallFont2, CR_UNDEFINED, x, y, t, DTA_FullscreenScale, 3, DTA_VirtualWidth, 640, DTA_VirtualHeight, 400, DTA_TranslationIndex, TRANSLATION(Translation_Remap, trans), DTA_Color, pe, TAG_DONE);
}



#if 0
void ShowMapFrame(void)
{
    short t = -1, i;
    ps[myconnectindex].palette = palette;
    if (ud.volume_number == 0)
    {
        switch (ud.level_number)
        {
            case 1:
                t = 0;
                break;
            case 2:
                t = 1;
                break;
            case 3:
                t = 2;
                break;
            case 4:
                t = 3;
                break;
            case 5:
                t = 4;
                break;
            case 6:
                t = 5;
                break;
            default:
                t = 6;
                break;
        }
    }
    else
    {
        switch (ud.level_number)
        {
            case 1:
                t = 7;
                break;
            case 2:
                t = 8;
                break;
            case 3:
                t = 9;
                break;
            case 4:
                t = 10;
                break;
            case 5:
                t = 11;
                break;
            case 6:
                t = 12;
                break;
            default:
                t = -1;
                break;
        }
    }
    rotatesprite(0,0,65536,0,8624+t,0,0,10+16+64+128,0,0,xdim-1,ydim-1);
    for (i = 0; i < 64; i++)
        palto(0,0,0,63-i);
    ps[myconnectindex].palette = palette;
}
#endif

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void Logo_r(CompletionFunc completion)
{
    Mus_Stop();
    FX_StopAllSounds(); // JBF 20031228

    static const AnimSound introsound[] =
    {
        { 1, 29+1 },
        { -1, -1 }
    };

    static const AnimSound rednecksound[] =
    {
        { 1, 478+1 },
        { -1, -1 }
    };

    static const AnimSound  xatrixsound[] =
    {
        { 1, 479+1 },
        { -1, -1 }
    };

    static const int framespeed[] = { 9, 9, 9 }; // same for all 3 anims

    JobDesc jobs[3];
    int job = 0;

    if (!isRRRA())
    {
        jobs[job++] = { PlayVideo("rr_intro.anm", introsound, framespeed), nullptr };
        jobs[job++] = { PlayVideo("redneck.anm", rednecksound, framespeed), nullptr };
        jobs[job++] = { PlayVideo("xatlogo.anm", xatrixsound, framespeed), nullptr };
    }
    else
    {
        jobs[job++] = { PlayVideo("redint.mve"), nullptr };
    }
    RunScreenJob(jobs, job, completion);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void bonussequence_r(int num, CompletionFunc completion)
{
    static const AnimSound  turdmov[] =
    {
        { 1, 82 + 1 },
        { -1, -1 }
    };

    static const AnimSound  rr_outro[] =
    {
        { 1, 35 + 1 },
        { -1, -1 }
    };

    static const int framespeed[] = { 9, 9, 9 }; // same for all 3 anims

    Mus_Stop();
    FX_StopAllSounds();

    JobDesc jobs[2];
    int job = 0;

    switch (num)
    {
    case 0:
        jobs[job++] = { PlayVideo("turdmov.anm", turdmov, framespeed), nullptr };
        jobs[job++] = { Create<DImageScreen>(TENSCREEN), nullptr };
        break;

    case 1:
        jobs[job++] = { PlayVideo("rr_outro.anm", rr_outro, framespeed), nullptr };
        jobs[job++] = { Create<DImageScreen>(TENSCREEN), nullptr };
        break;

    default:
        break;
    }
    RunScreenJob(jobs, job, completion);
}

//---------------------------------------------------------------------------
//
// RRRA only
//
//---------------------------------------------------------------------------

void PlayMapAnim(CompletionFunc completion)
{
    char fn[20];

    int lev = ud.level_number + 7 * ud.volume_number;
    if (lev >= 1 && lev <= 13)
    {
        mysnprintf(fn, 20, "lvl%d.anm", lev);

        static const int framespeed[] = { 20, 20, 7200 };   // wait for one minute on the final frame
        JobDesc job = { PlayVideo(fn, nullptr, framespeed) };
        RunScreenJob(&job, 1, completion);
    }
    else completion(false);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DRRMultiplayerBonusScreen : public DScreenJob
{
    int playerswhenstarted;

public:
    DRRMultiplayerBonusScreen(int pws)
    {
        playerswhenstarted = pws;
    }

    int Frame(uint64_t clock, bool skiprequest)
    {
        char tempbuf[32];
        twod->ClearScreen();
        DrawTexture(twod, tileGetTexture(MENUSCREEN), 0, 0, DTA_FullscreenEx, 3, DTA_Color, 0xff808080, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);
        double scale = 0.36;
        DrawTexture(twod, tileGetTexture(INGAMEDUKETHREEDEE, true), 160, 34, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, 
            DTA_CenterOffset, true, DTA_ScaleX, scale, DTA_ScaleY, 0.36, TAG_DONE);

        GameText(160, 58, GStrings("Multiplayer Totals"), 0, 0);
        GameText(160, 58 + 10, currentLevel->DisplayName(), 0, 0);
        GameText(160, 165, GStrings("Presskey"), 0, 0);

        int t = 0;

        MiniText(38, 80, GStrings("Name"), 0);
        MiniText(269 + 20, 80, GStrings("Kills"), 0, 1);

        for (int i = 0; i < playerswhenstarted; i++)
        {
            mysnprintf(tempbuf, 32, "%-4ld", i + 1);
            MiniText(92 + (i * 23), 80, tempbuf, 0);
        }

        for (int i = 0; i < playerswhenstarted; i++)
        {
            int xfragtotal = 0;
            mysnprintf(tempbuf, 32, "%ld", i + 1);

            MiniText(30, 90 + t, tempbuf, 0);
            MiniText(38, 90 + t, g_player[i].user_name, 0, -1, ps[i].palookup);

            for (int y = 0; y < playerswhenstarted; y++)
            {
                int frag = g_player[i].frags[y];// frags[i][y]);
                if (i == y)
                {
                    mysnprintf(tempbuf, 32, "%-4ld", ps[y].fraggedself);
                    MiniText(92 + (y * 23), 90 + t, tempbuf, 0);
                    xfragtotal -= ps[y].fraggedself;
                }
                else
                {
                    mysnprintf(tempbuf, 32, "%-4ld", frag);
                    MiniText(92 + (y * 23), 90 + t, tempbuf, 0);
                    xfragtotal += frag;
                }
                /*
                if (myconnectindex == connecthead)
                {
                    mysnprintf(tempbuf, 32, "stats %ld killed %ld %ld\n", i + 1, y + 1, frag);
                    sendscore(tempbuf);
                }
                */
            }

            mysnprintf(tempbuf, 32, "%-4ld", xfragtotal);
            MiniText(101 + (8 * 23), 90 + t, tempbuf, 0);

            t += 7;
        }

        for (int y = 0; y < playerswhenstarted; y++)
        {
            int yfragtotal = 0;
            for (int i = 0; i < playerswhenstarted; i++)
            {
                if (i == y)
                    yfragtotal += ps[i].fraggedself;
                int frag = g_player[i].frags[y];// frags[i][y]);
                yfragtotal += frag;
            }
            mysnprintf(tempbuf, 32, "%-4ld", yfragtotal);
            MiniText(92 + (y * 23), 96 + (8 * 7), tempbuf, 0);
        }

        MiniText(45, 96 + (8 * 7), GStrings("Deaths"), 0);
        return skiprequest ? -1 : 1;
    }
};

void ShowMPBonusScreen_r(int pws, CompletionFunc completion)
{
    JobDesc job = { Create<DRRMultiplayerBonusScreen>(pws) };
    RunScreenJob(&job, 1, completion);
}


#if 1
CCMD(testrbonus)
{
    if (argv.argc() > 1)
    {
        //bonussequence_d(strtol(argv[1], nullptr, 0), nullptr);
        ShowMPBonusScreen_r(strtol(argv[1], nullptr, 0), nullptr);
    }
}
#endif


#if 0

#ifdef RRRA
void dobonus2(char bonusonly)
{
    short t, r, tinc,gfx_offset,bg_tile;
    long i, y,xfragtotal,yfragtotal,var24;
    short bonuscnt;

    var24 = 0;
    bonuscnt = 0;

    for(t=0;t<64;t++) palto(0,0,0,t);
    setview(0,0,xdim-1,ydim-1);
    clearview(0L);
    nextpage();
    flushperms();

    FX_StopAllSounds();
    clearsoundlocks();
    FX_SetReverb(0L);

    if (boardfilename[0] == 0 && numplayers < 2)
    {
        if ((ud.eog == 0 || ud.volume_number != 1) && ud.volume_number <= 1)
        {
            var24 = 1;
            MUSIC_StopSong();
            KB_FlushKeyboardQueue();
            ShowMapFrame();
        }
    }

    if(bonusonly) goto FRAGBONUS;

    FRAGBONUS:

    ps[myconnectindex].palette = palette;
    KB_FlushKeyboardQueue();
    totalclock = 0; tinc = 0;
    bonuscnt = 0;

    MUSIC_StopSong();

    if(playerswhenstarted > 1 && ud.coop != 1 )
    {
        if(!(MusicToggle == 0 || MusicDevice == NumSoundCards))
            sound(249);

        rotatesprite(0,0,65536L,0,MENUSCREEN,16,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
        rotatesprite(160<<16,57<<16,16592L,0,THREEDEE,0,0,2+8,0,0,xdim-1,ydim-1);
        gametext(160,58,"MULTIPLAYER TOTALS",0);
        gametext(160,58+10,level_names[(ud.volume_number*7)+ud.last_level-1],0);

        gametext(160,175,"PRESS ANY KEY TO CONTINUE",0);


        t = 0;
        minitext(23,80,"   NAME                                           KILLS",8,2+8+16+128);
        for(i=0;i<playerswhenstarted;i++)
        {
            sprintf(tempbuf,"%-4ld",i+1);
            minitext(92+(i*23),80,tempbuf,0,2+8+16+128);
        }

        for(i=0;i<playerswhenstarted;i++)
        {
            xfragtotal = 0;
            sprintf(tempbuf,"%ld",i+1);

            minitext(30,90+t,tempbuf,0,2+8+16+128);
            minitext(38,90+t,ud.user_name[i],ps[i].palookup,2+8+16+128);

            for(y=0;y<playerswhenstarted;y++)
            {
                if(i == y)
                {
                    sprintf(tempbuf,"%-4ld",ps[y].fraggedself);
                    minitext(92+(y*23),90+t,tempbuf,0,2+8+16+128);
                    xfragtotal -= ps[y].fraggedself;
                }
                else
                {
                    sprintf(tempbuf,"%-4ld",frags[i][y]);
                    minitext(92+(y*23),90+t,tempbuf,0,2+8+16+128);
                    xfragtotal += frags[i][y];
                }

                if(myconnectindex == connecthead)
                {
                    sprintf(tempbuf,"stats %ld killed %ld %ld\n",i+1,y+1,frags[i][y]);
                    sendscore(tempbuf);
                }
            }

            sprintf(tempbuf,"%-4ld",xfragtotal);
            minitext(101+(8*23),90+t,tempbuf,0,2+8+16+128);

            t += 7;
        }

        for(y=0;y<playerswhenstarted;y++)
        {
            yfragtotal = 0;
            for(i=0;i<playerswhenstarted;i++)
            {
                if(i == y)
                    yfragtotal += ps[i].fraggedself;
                yfragtotal += frags[i][y];
            }
            sprintf(tempbuf,"%-4ld",yfragtotal);
            minitext(92+(y*23),96+(8*7),tempbuf,0,2+8+16+128);
        }

        minitext(45,96+(8*7),"DEATHS",0,2+8+16+128);
        nextpage();

        for(t=0;t<64;t++)
            palto(0,0,0,63-t);

        KB_FlushKeyboardQueue();
        while(KB_KeyWaiting()==0) getpackets();

        if( KB_KeyPressed( sc_F12 ) )
        {
            KB_ClearKeyDown( sc_F12 );
            screencapture("rdnk0000.pcx",0);
        }

        if(bonusonly || ud.multimode > 1) return;

        for(t=0;t<64;t++) palto(0,0,0,t);
    }

    if(bonusonly || ud.multimode > 1) return;

    if(!(MusicToggle == 0 || MusicDevice == NumSoundCards))
        sound(249);

    
    gfx_offset = (ud.volume_number&1)*5;
    bg_tile = RRTILE403;
    if (ud.volume_number == 0)
        bg_tile = ud.level_number+RRTILE403-1;
    else
        bg_tile = ud.level_number+RRTILE409-1;

    if (lastlevel || vixenlevel)
        bg_tile = RRTILE409+7;

    if (boardfilename[0])
    {
        if (!var24)
        {
            rotatesprite(0,0,65536L,0,403,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
            endlvlmenutext(80,16,0,0,boardfilename);
        }
    }
    else if (!var24)
    {
        rotatesprite(0,0,65536L,0,bg_tile,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
        if ((lastlevel && ud.volume_number == 2) || vixenlevel)
            endlvlmenutext(80,16,0,0,"CLOSE ENCOUNTERS");
        else if (turdlevel)
            endlvlmenutext(80,16,0,0,"SMELTING PLANT");
        else
            endlvlmenutext(80,16,0,0,level_names[(ud.volume_number*7)+ud.last_level-1]);
    }
    else
    {
        endlvlmenutext(80,16,0,0,level_names[(ud.volume_number*7)+ud.last_level-1]);
    }

    endlvlmenutext(15,192,0,0,"PRESS ANY KEY TO CONTINUE");
    KB_FlushKeyboardQueue();
    if (!var24)
    {
        nextpage();
        for(t=0;t<64;t++) palto(0,0,0,63-t);
    }
    bonuscnt = 0;
    totalclock = 0; tinc = 0;

    while( 1 )
    {
        if(ps[myconnectindex].gm&MODE_EOL)
        {
            if (boardfilename[0])
            {
                if (!var24)
                    rotatesprite(0,0,65536L,0,403,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
            }
            else if (!var24)
                rotatesprite(0,0,65536L,0,bg_tile,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);

            if( totalclock > (1000000000L) && totalclock < (1000000320L) )
            {
                switch( ((unsigned long)totalclock>>4)%15 )
                {
                    case 0:
                        if(bonuscnt == 6)
                        {
                            bonuscnt++;
                            sound(425);
                            switch(rand()&3)
                            {
                                case 0:
                                    sound(195);
                                    break;
                                case 1:
                                    sound(196);
                                    break;
                                case 2:
                                    sound(197);
                                    break;
                                case 3:
                                    sound(199);
                                    break;
                            }
                        }
                    case 1:
                    case 4:
                    case 5:
                    case 2:
                    case 3:
                       break;
                }
            }
            else if( totalclock > (10240+120L) ) break;
            else
            {
                switch( (totalclock>>5)&3 )
                {
                    case 1:
                    case 3:
                        break;
                    case 2:
                        break;
                }
            }

            if (boardfilename[0])
            {
                if (!var24)
                {
                    rotatesprite(0,0,65536L,0,403,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                    endlvlmenutext(80,16,0,0,boardfilename);
                }
            }
            else if (!var24)
            {
                rotatesprite(0,0,65536L,0,bg_tile,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                if ((lastlevel && ud.volume_number == 2) || vixenlevel)
                    endlvlmenutext(80,16,0,0,"CLOSE ENCOUNTERS");
                else if (turdlevel)
                    endlvlmenutext(80,16,0,0,"SMELTING PLANT");
                else
                    endlvlmenutext(80,16,0,0,level_names[(ud.volume_number*7)+ud.last_level-1]);
            }
            else
            {
                endlvlmenutext(80,16,0,0,level_names[(ud.volume_number*7)+ud.last_level-1]);
            }

            endlvlmenutext(15,192,0,0,"PRESS ANY KEY TO CONTINUE");

            if( totalclock > (60*3) )
            {
                endlvlmenutext(30,48,0,0,"Yer Time:");
                endlvlmenutext(30,64,0,0,"Par time:");
                endlvlmenutext(30,80,0,0,"Xatrix Time:");
                if(bonuscnt == 0)
                    bonuscnt++;

                if( totalclock > (60*4) )
                {
                    if(bonuscnt == 1)
                    {
                        bonuscnt++;
                        sound(404);
                    }
                    sprintf(tempbuf,"%02ld : %02ld",
                        (ps[myconnectindex].player_par/(26*60))%60,
                        (ps[myconnectindex].player_par/26)%60);
                    endlvlmenutext(191,48,0,0,tempbuf);

                    if(!boardfilename[0])
                    {
                        sprintf(tempbuf,"%02ld : %02ld",
                            (partime[ud.volume_number*7+ud.last_level-1]/(26*60))%60,
                            (partime[ud.volume_number*7+ud.last_level-1]/26)%60);
                        endlvlmenutext(191,64,0,0,tempbuf);

                        sprintf(tempbuf,"%02ld : %02ld",
                            (designertime[ud.volume_number*7+ud.last_level-1]/(26*60))%60,
                            (designertime[ud.volume_number*7+ud.last_level-1]/26)%60);
                        endlvlmenutext(191,80,0,0,tempbuf);
                    }

                }
            }
            if( totalclock > (60*6) )
            {
                endlvlmenutext(30,112,0,0,"Varmints Killed:");
                endlvlmenutext(30,128,0,0,"Varmints Left:");

                if(bonuscnt == 2)
                    bonuscnt++;

                if( totalclock > (60*7) )
                {
                    if(bonuscnt == 3)
                    {
                        bonuscnt++;
                        sound(422);
                    }
                    sprintf(tempbuf,"%-3ld",ps[myconnectindex].actors_killed);
                    endlvlmenutext(231,112,0,0,tempbuf);
                    if(ud.player_skill > 3 )
                    {
                        sprintf(tempbuf,"N/A");
                        endlvlmenutext(231,128,0,0,tempbuf);
                    }
                    else
                    {
                        if( (ps[myconnectindex].max_actors_killed-ps[myconnectindex].actors_killed) < 0 )
                            sprintf(tempbuf,"%-3ld",0);
                        else sprintf(tempbuf,"%-3ld",ps[myconnectindex].max_actors_killed-ps[myconnectindex].actors_killed);
                        endlvlmenutext(231,128,0,0,tempbuf);
                    }
                }
            }
            if( totalclock > (60*9) )
            {
                endlvlmenutext(30,144,0,0,"Secrets Found:");
                endlvlmenutext(30,160,0,0,"Secrets Missed:");
                if(bonuscnt == 4) bonuscnt++;

                if( totalclock > (60*10) )
                {
                    if(bonuscnt == 5)
                    {
                        bonuscnt++;
                        sound(404);
                    }
                    sprintf(tempbuf,"%-3ld",ps[myconnectindex].secret_rooms);
                    endlvlmenutext(231,144,0,0,tempbuf);
                    if( ps[myconnectindex].secret_rooms > 0 )
                        sprintf(tempbuf,"%-3ld%",(100*ps[myconnectindex].secret_rooms/ps[myconnectindex].max_secret_rooms));
                    sprintf(tempbuf,"%-3ld",ps[myconnectindex].max_secret_rooms-ps[myconnectindex].secret_rooms);
                    endlvlmenutext(231,160,0,0,tempbuf);
                }
            }

            if(totalclock > 10240 && totalclock < 10240+10240)
                totalclock = 1024;

            if( KB_KeyWaiting() && totalclock > (60*2) )
            {
                if (var24)
                {
                    if (bonuscnt == 7)
                    {
                        bonuscnt++;
                        MUSIC_StopSong();
                        KB_FlushKeyboardQueue();
                        PlayMapAnim();
                    }
                    else if (bonuscnt == 8)
                    {
                        KB_FlushKeyboardQueue();
                        totalclock = 10361;
                        break;
                    }

                }
                if( KB_KeyPressed( sc_F12 ) )
                {
                    KB_ClearKeyDown( sc_F12 );
                    screencapture("rdnk0000.pcx",0);
                }

                if (var24)
                {
                    if( totalclock < (60*13) )
                    {
                        KB_FlushKeyboardQueue();
                        totalclock = (60*13);
                    }
                    else if( totalclock < (1000000000L))
                       totalclock = (1000000000L);
                }
                else
                {
                    if( totalclock < (60*13) )
                    {
                        KB_FlushKeyboardQueue();
                        totalclock = (60*13);
                    }
                    else if( totalclock < (1000000000L))
                       totalclock = (1000000000L);
                }
            }
        }
        else break;
        if (!var24 || bonuscnt)
            nextpage();
    }
    if (ud.eog)
    {
        for (t = 0; t < 64; t++) palto(0, 0, 0, t);
        clearview(0);
        nextpage();
        spritesound(35,ps[0].i);
        palto(0, 0, 0, 0);
        ps[myconnectindex].palette = palette;
        while (1)
        {
            int var40;
            switch ((totalclock >> 4) & 1)
            {
            case 0:
                rotatesprite(0,0,65536,0,0,RRTILE8677,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                nextpage();
                palto(0, 0, 0, 0);
                ps[myconnectindex].palette = palette;
                getpackets();
                break;
            case 1:
                rotatesprite(0,0,65536,0,0,RRTILE8677+1,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                nextpage();
                palto(0, 0, 0, 0);
                ps[myconnectindex].palette = palette;
                getpackets();
                break;
            }
            if (Sound[35].num == 0) break;
        }
    }
    if (word_119BE4)
    {
        word_119BE4 = 0;
        ud.m_volume_number = ud.volume_number = 1;
        ud.m_level_number = ud.level_number = 0;
        ud.eog = 0;
    }
    if (turdlevel)
        turdlevel = 0;
    if (vixenlevel)
        vixenlevel = 0;
}
#else
void dobonus(char bonusonly)
{
    short t, r, tinc,gfx_offset;
    long i, y,xfragtotal,yfragtotal;
    short bonuscnt;
    short bg_tile;

    bonuscnt = 0;

    for(t=0;t<64;t++) palto(0,0,0,t);
    setview(0,0,xdim-1,ydim-1);
    clearview(0L);
    nextpage();
    flushperms();

    FX_StopAllSounds();
    clearsoundlocks();
    FX_SetReverb(0L);

    if(bonusonly) goto FRAGBONUS;

    if(numplayers < 2 && ud.eog && ud.from_bonus == 0)
        switch(ud.volume_number)
    {
        case 0:
            MUSIC_StopSong();
            clearview(0L);
            nextpage();
            if(ud.lockout == 0)
            {
                playanm("turdmov.anm",5,5);
                KB_FlushKeyboardQueue();
                clearview(0L);
                nextpage();
            }
            ud.level_number = 0;
            ud.volume_number = 1;
            ud.eog = 0;

            for(t=0;t<64;t++) palto(0,0,0,t);

            KB_FlushKeyboardQueue();
            ps[myconnectindex].palette = palette;

            rotatesprite(0,0,65536L,0,1685,0,0,2+8+16+64,0,0,xdim-1,ydim-1);
            nextpage(); for(t=63;t>0;t--) palto(0,0,0,t);
            while( !KB_KeyWaiting() ) getpackets();
            for(t=0;t<64;t++) palto(0,0,0,t);
            MUSIC_StopSong();
            FX_StopAllSounds();
            clearsoundlocks();
            break;
        case 1:
            MUSIC_StopSong();
            clearview(0L);
            nextpage();

            if(ud.lockout == 0)
            {
                playanm("rr_outro.anm",5,4);
                KB_FlushKeyboardQueue();
                clearview(0L);
                nextpage();
            }
            lastlevel = 0;
            vixenlevel = 1;
            ud.level_number = 0;
            ud.volume_number = 0;

            for(t=0;t<64;t++) palto(0,0,0,t);
            setview(0,0,xdim-1,ydim-1);
            KB_FlushKeyboardQueue();
            ps[myconnectindex].palette = palette;
            rotatesprite(0,0,65536L,0,1685,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
            nextpage(); for(t=63;t>0;t--) palto(0,0,0,t);
            while( !KB_KeyWaiting() ) getpackets();
            for(t=0;t<64;t++) palto(0,0,0,t);

            break;

        case 2:
            KB_FlushKeyboardQueue();
            while( !KB_KeyWaiting() ) getpackets();

            FX_StopAllSounds();
            clearsoundlocks();
            KB_FlushKeyboardQueue();

            clearview(0L);
            nextpage();
            playanm("LNRDTEAM.ANM",4,3);
            KB_FlushKeyboardQueue();

            while( !KB_KeyWaiting() ) getpackets();

            FX_StopAllSounds();
            clearsoundlocks();

            KB_FlushKeyboardQueue();

            break;
    }

    FRAGBONUS:

    ps[myconnectindex].palette = palette;
    KB_FlushKeyboardQueue();
    totalclock = 0; tinc = 0;
    bonuscnt = 0;

    MUSIC_StopSong();
    FX_StopAllSounds();
    clearsoundlocks();

    if(playerswhenstarted > 1 && ud.coop != 1 )
    {
        if(!(MusicToggle == 0 || MusicDevice == NumSoundCards))
            sound(249);

        rotatesprite(0,0,65536L,0,MENUSCREEN,16,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
        rotatesprite(160<<16,24<<16,23592L,0,INGAMEDUKETHREEDEE,0,0,2+8,0,0,xdim-1,ydim-1);
        gametext(160,58,"MULTIPLAYER TOTALS",0);
        gametext(160,58+10,level_names[(ud.volume_number*7)+ud.last_level-1],0);

        gametext(160,175,"PRESS ANY KEY TO CONTINUE",0);


        t = 0;
        minitext(23,80,"   NAME                                           KILLS",8,2+8+16+128);
        for(i=0;i<playerswhenstarted;i++)
        {
            sprintf(tempbuf,"%-4ld",i+1);
            minitext(92+(i*23),80,tempbuf,0,2+8+16+128);
        }

        for(i=0;i<playerswhenstarted;i++)
        {
            xfragtotal = 0;
            sprintf(tempbuf,"%ld",i+1);

            minitext(30,90+t,tempbuf,0,2+8+16+128);
            minitext(38,90+t,ud.user_name[i],ps[i].palookup,2+8+16+128);

            for(y=0;y<playerswhenstarted;y++)
            {
                if(i == y)
                {
                    sprintf(tempbuf,"%-4ld",ps[y].fraggedself);
                    minitext(92+(y*23),90+t,tempbuf,0,2+8+16+128);
                    xfragtotal -= ps[y].fraggedself;
                }
                else
                {
                    sprintf(tempbuf,"%-4ld",frags[i][y]);
                    minitext(92+(y*23),90+t,tempbuf,0,2+8+16+128);
                    xfragtotal += frags[i][y];
                }

                if(myconnectindex == connecthead)
                {
                    sprintf(tempbuf,"stats %ld killed %ld %ld\n",i+1,y+1,frags[i][y]);
                    sendscore(tempbuf);
                }
            }

            sprintf(tempbuf,"%-4ld",xfragtotal);
            minitext(101+(8*23),90+t,tempbuf,0,2+8+16+128);

            t += 7;
        }

        for(y=0;y<playerswhenstarted;y++)
        {
            yfragtotal = 0;
            for(i=0;i<playerswhenstarted;i++)
            {
                if(i == y)
                    yfragtotal += ps[i].fraggedself;
                yfragtotal += frags[i][y];
            }
            sprintf(tempbuf,"%-4ld",yfragtotal);
            minitext(92+(y*23),96+(8*7),tempbuf,0,2+8+16+128);
        }

        minitext(45,96+(8*7),"DEATHS",0,2+8+16+128);
        nextpage();

        for(t=0;t<64;t++)
            palto(0,0,0,63-t);

        KB_FlushKeyboardQueue();
        while(KB_KeyWaiting()==0) getpackets();

        if( KB_KeyPressed( sc_F12 ) )
        {
            KB_ClearKeyDown( sc_F12 );
            screencapture("rdnk0000.pcx",0);
        }

        if(bonusonly || ud.multimode > 1) return;

        for(t=0;t<64;t++) palto(0,0,0,t);
    }

    if(bonusonly || ud.multimode > 1) return;

    if(!(MusicToggle == 0 || MusicDevice == NumSoundCards))
        sound(249);

    
    gfx_offset = (ud.volume_number&1)*5;
    bg_tile = RRTILE403;
    if (ud.volume_number == 0)
        bg_tile = ud.level_number+RRTILE403-1;
    else
        bg_tile = ud.level_number+RRTILE409-1;

    if (lastlevel || vixenlevel)
        bg_tile = RRTILE409+7;

    if (boardfilename[0])
    {
        rotatesprite(0,0,65536L,0,403,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
        endlvlmenutext(80,16,0,0,boardfilename);
    }
    else
    {
        rotatesprite(0,0,65536L,0,bg_tile,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
        if ((lastlevel && ud.volume_number == 2) || vixenlevel)
            endlvlmenutext(80,16,0,0,"CLOSE ENCOUNTERS");
        else if (turdlevel)
            endlvlmenutext(80,16,0,0,"SMELTING PLANT");
        else
            endlvlmenutext(80,16,0,0,level_names[(ud.volume_number*7)+ud.last_level-1]);
    }

    endlvlmenutext(15,192,0,0,"PRESS ANY KEY TO CONTINUE");
    nextpage();
    KB_FlushKeyboardQueue();
    for(t=0;t<64;t++) palto(0,0,0,63-t);
    bonuscnt = 0;
    totalclock = 0; tinc = 0;

    while( 1 )
    {
        if(ps[myconnectindex].gm&MODE_EOL)
        {
            if (boardfilename[0])
                rotatesprite(0,0,65536L,0,403,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
            else
                rotatesprite(0,0,65536L,0,bg_tile,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);

            if( totalclock > (1000000000L) && totalclock < (1000000320L) )
            {
                switch( ((unsigned long)totalclock>>4)%15 )
                {
                    case 0:
                        if(bonuscnt == 6)
                        {
                            bonuscnt++;
                            sound(425);
                            switch(rand()&3)
                            {
                                case 0:
                                    sound(195);
                                    break;
                                case 1:
                                    sound(196);
                                    break;
                                case 2:
                                    sound(197);
                                    break;
                                case 3:
                                    sound(199);
                                    break;
                            }
                        }
                    case 1:
                    case 4:
                    case 5:
                    case 2:
                    case 3:
                       break;
                }
            }
            else if( totalclock > (10240+120L) ) break;
            else
            {
                switch( (totalclock>>5)&3 )
                {
                    case 1:
                    case 3:
                        break;
                    case 2:
                        break;
                }
            }

            if (boardfilename[0])
            {
                rotatesprite(0,0,65536L,0,403,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                endlvlmenutext(80,16,0,0,boardfilename);
            }
            else
            {
                rotatesprite(0,0,65536L,0,bg_tile,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                if ((lastlevel && ud.volume_number == 2) || vixenlevel)
                    endlvlmenutext(80,16,0,0,"CLOSE ENCOUNTERS");
                else if (turdlevel)
                    endlvlmenutext(80,16,0,0,"SMELTING PLANT");
                else
                    endlvlmenutext(80,16,0,0,level_names[(ud.volume_number*7)+ud.last_level-1]);
            }

            endlvlmenutext(15,192,0,0,"PRESS ANY KEY TO CONTINUE");

            if( totalclock > (60*3) )
            {
                endlvlmenutext(30,48,0,0,"Yer Time:");
                endlvlmenutext(30,64,0,0,"Par time:");
                endlvlmenutext(30,80,0,0,"Xatrix Time:");
                if(bonuscnt == 0)
                    bonuscnt++;

                if( totalclock > (60*4) )
                {
                    if(bonuscnt == 1)
                    {
                        bonuscnt++;
                        sound(404);
                    }
                    sprintf(tempbuf,"%02ld : %02ld",
                        (ps[myconnectindex].player_par/(26*60))%60,
                        (ps[myconnectindex].player_par/26)%60);
                    endlvlmenutext(191,48,0,0,tempbuf);

                    if(!boardfilename[0])
                    {
                        sprintf(tempbuf,"%02ld : %02ld",
                            (partime[ud.volume_number*7+ud.last_level-1]/(26*60))%60,
                            (partime[ud.volume_number*7+ud.last_level-1]/26)%60);
                        endlvlmenutext(191,64,0,0,tempbuf);

                        sprintf(tempbuf,"%02ld : %02ld",
                            (designertime[ud.volume_number*7+ud.last_level-1]/(26*60))%60,
                            (designertime[ud.volume_number*7+ud.last_level-1]/26)%60);
                        endlvlmenutext(191,80,0,0,tempbuf);
                    }

                }
            }
            if( totalclock > (60*6) )
            {
                endlvlmenutext(30,112,0,0,"Varmints Killed:");
                endlvlmenutext(30,128,0,0,"Varmints Left:");

                if(bonuscnt == 2)
                    bonuscnt++;

                if( totalclock > (60*7) )
                {
                    if(bonuscnt == 3)
                    {
                        bonuscnt++;
                        sound(422);
                    }
                    sprintf(tempbuf,"%-3ld",ps[myconnectindex].actors_killed);
                    endlvlmenutext(231,112,0,0,tempbuf);
                    if(ud.player_skill > 3 )
                    {
                        sprintf(tempbuf,"N/A");
                        endlvlmenutext(231,128,0,0,tempbuf);
                    }
                    else
                    {
                        if( (ps[myconnectindex].max_actors_killed-ps[myconnectindex].actors_killed) < 0 )
                            sprintf(tempbuf,"%-3ld",0);
                        else sprintf(tempbuf,"%-3ld",ps[myconnectindex].max_actors_killed-ps[myconnectindex].actors_killed);
                        endlvlmenutext(231,128,0,0,tempbuf);
                    }
                }
            }
            if( totalclock > (60*9) )
            {
                endlvlmenutext(30,144,0,0,"Secrets Found:");
                endlvlmenutext(30,160,0,0,"Secrets Missed:");
                if(bonuscnt == 4) bonuscnt++;

                if( totalclock > (60*10) )
                {
                    if(bonuscnt == 5)
                    {
                        bonuscnt++;
                        sound(404);
                    }
                    sprintf(tempbuf,"%-3ld",ps[myconnectindex].secret_rooms);
                    endlvlmenutext(231,144,0,0,tempbuf);
                    if( ps[myconnectindex].secret_rooms > 0 )
                        sprintf(tempbuf,"%-3ld%",(100*ps[myconnectindex].secret_rooms/ps[myconnectindex].max_secret_rooms));
                    sprintf(tempbuf,"%-3ld",ps[myconnectindex].max_secret_rooms-ps[myconnectindex].secret_rooms);
                    endlvlmenutext(231,160,0,0,tempbuf);
                }
            }

            if(totalclock > 10240 && totalclock < 10240+10240)
                totalclock = 1024;

            if( KB_KeyWaiting() && totalclock > (60*2) )
            {
                if( KB_KeyPressed( sc_F12 ) )
                {
                    KB_ClearKeyDown( sc_F12 );
                    screencapture("rdnk0000.pcx",0);
                }

                if( totalclock < (60*13) )
                {
                    KB_FlushKeyboardQueue();
                    totalclock = (60*13);
                }
                else if( totalclock < (1000000000L))
                   totalclock = (1000000000L);
            }
        }
        else break;
        nextpage();
    }
    if (turdlevel)
        turdlevel = 0;
    if (vixenlevel)
        vixenlevel = 0;
}
#endif

#ifdef RRRA

void dobonus(char bonusonly)
{
    short t, r, tinc,gfx_offset,bg_tile;
    long i, y,xfragtotal,yfragtotal;
    short bonuscnt;

    bonuscnt = 0;

    for(t=0;t<64;t++) palto(0,0,0,t);
    setview(0,0,xdim-1,ydim-1);
    clearview(0L);
    nextpage();
    flushperms();

    FX_StopAllSounds();
    clearsoundlocks();
    FX_SetReverb(0L);

    if(bonusonly) goto FRAGBONUS;

    FRAGBONUS:

    ps[myconnectindex].palette = palette;
    KB_FlushKeyboardQueue();
    totalclock = 0; tinc = 0;
    bonuscnt = 0;

    MUSIC_StopSong();
    FX_StopAllSounds();
    clearsoundlocks();

    if(playerswhenstarted > 1 && ud.coop != 1 )
    {
        if(!(MusicToggle == 0 || MusicDevice == NumSoundCards))
            sound(249);

        rotatesprite(0,0,65536L,0,MENUSCREEN,16,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
        rotatesprite(160<<16,24<<16,23592L,0,INGAMEDUKETHREEDEE,0,0,2+8,0,0,xdim-1,ydim-1);
        gametext(160,58,"MULTIPLAYER TOTALS",0);
        gametext(160,58+10,level_names[(ud.volume_number*7)+ud.last_level-1],0);

        gametext(160,175,"PRESS ANY KEY TO CONTINUE",0);


        t = 0;
        minitext(23,80,"   NAME                                           KILLS",8,2+8+16+128);
        for(i=0;i<playerswhenstarted;i++)
        {
            sprintf(tempbuf,"%-4ld",i+1);
            minitext(92+(i*23),80,tempbuf,0,2+8+16+128);
        }

        for(i=0;i<playerswhenstarted;i++)
        {
            xfragtotal = 0;
            sprintf(tempbuf,"%ld",i+1);

            minitext(30,90+t,tempbuf,0,2+8+16+128);
            minitext(38,90+t,ud.user_name[i],ps[i].palookup,2+8+16+128);

            for(y=0;y<playerswhenstarted;y++)
            {
                if(i == y)
                {
                    sprintf(tempbuf,"%-4ld",ps[y].fraggedself);
                    minitext(92+(y*23),90+t,tempbuf,0,2+8+16+128);
                    xfragtotal -= ps[y].fraggedself;
                }
                else
                {
                    sprintf(tempbuf,"%-4ld",frags[i][y]);
                    minitext(92+(y*23),90+t,tempbuf,0,2+8+16+128);
                    xfragtotal += frags[i][y];
                }

                if(myconnectindex == connecthead)
                {
                    sprintf(tempbuf,"stats %ld killed %ld %ld\n",i+1,y+1,frags[i][y]);
                    sendscore(tempbuf);
                }
            }

            sprintf(tempbuf,"%-4ld",xfragtotal);
            minitext(101+(8*23),90+t,tempbuf,0,2+8+16+128);

            t += 7;
        }

        for(y=0;y<playerswhenstarted;y++)
        {
            yfragtotal = 0;
            for(i=0;i<playerswhenstarted;i++)
            {
                if(i == y)
                    yfragtotal += ps[i].fraggedself;
                yfragtotal += frags[i][y];
            }
            sprintf(tempbuf,"%-4ld",yfragtotal);
            minitext(92+(y*23),96+(8*7),tempbuf,0,2+8+16+128);
        }

        minitext(45,96+(8*7),"DEATHS",0,2+8+16+128);
        nextpage();

        for(t=0;t<64;t++)
            palto(0,0,0,63-t);

        KB_FlushKeyboardQueue();
        while(KB_KeyWaiting()==0) getpackets();

        if( KB_KeyPressed( sc_F12 ) )
        {
            KB_ClearKeyDown( sc_F12 );
            screencapture("rdnk0000.pcx",0);
        }

        if(bonusonly || ud.multimode > 1) return;

        for(t=0;t<64;t++) palto(0,0,0,t);
    }

    if(bonusonly || ud.multimode > 1) return;

    if(!(MusicToggle == 0 || MusicDevice == NumSoundCards))
        sound(249);

    
    gfx_offset = (ud.volume_number&1)*5;
    bg_tile = RRTILE403;
    if (ud.volume_number == 0)
        bg_tile = ud.level_number+RRTILE403-1;
    else
        bg_tile = ud.level_number+RRTILE409-1;

    if (lastlevel || vixenlevel)
        bg_tile = RRTILE409+7;

    if (boardfilename[0])
    {
        rotatesprite(0,0,65536L,0,403,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
        endlvlmenutext(80,16,0,0,boardfilename);
    }
    else
    {
        rotatesprite(0,0,65536L,0,bg_tile,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
        if ((lastlevel && ud.volume_number == 2) || vixenlevel)
            endlvlmenutext(80,16,0,0,"CLOSE ENCOUNTERS");
        else if (turdlevel)
            endlvlmenutext(80,16,0,0,"SMELTING PLANT");
        else
            endlvlmenutext(80,16,0,0,level_names[(ud.volume_number*7)+ud.last_level-1]);
    }

    endlvlmenutext(15,192,0,0,"PRESS ANY KEY TO CONTINUE");
    nextpage();
    KB_FlushKeyboardQueue();
    for(t=0;t<64;t++) palto(0,0,0,63-t);
    bonuscnt = 0;
    totalclock = 0; tinc = 0;

    while( 1 )
    {
        if(ps[myconnectindex].gm&MODE_EOL)
        {
            if (boardfilename[0])
                rotatesprite(0,0,65536L,0,403,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
            else
                rotatesprite(0,0,65536L,0,bg_tile,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);

            if( totalclock > (1000000000L) && totalclock < (1000000320L) )
            {
                switch( ((unsigned long)totalclock>>4)%15 )
                {
                    case 0:
                        if(bonuscnt == 6)
                        {
                            bonuscnt++;
                            sound(425);
                            switch(rand()&3)
                            {
                                case 0:
                                    sound(195);
                                    break;
                                case 1:
                                    sound(196);
                                    break;
                                case 2:
                                    sound(197);
                                    break;
                                case 3:
                                    sound(199);
                                    break;
                            }
                        }
                    case 1:
                    case 4:
                    case 5:
                    case 2:
                    case 3:
                       break;
                }
            }
            else if( totalclock > (10240+120L) ) break;
            else
            {
                switch( (totalclock>>5)&3 )
                {
                    case 1:
                    case 3:
                        break;
                    case 2:
                        break;
                }
            }

            if (boardfilename[0])
            {
                rotatesprite(0,0,65536L,0,403,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                endlvlmenutext(80,16,0,0,boardfilename);
            }
            else
            {
                rotatesprite(0,0,65536L,0,bg_tile,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                if ((lastlevel && ud.volume_number == 2) || vixenlevel)
                    endlvlmenutext(80,16,0,0,"CLOSE ENCOUNTERS");
                else if (turdlevel)
                    endlvlmenutext(80,16,0,0,"SMELTING PLANT");
                else
                    endlvlmenutext(80,16,0,0,level_names[(ud.volume_number*7)+ud.last_level-1]);
            }

            endlvlmenutext(15,192,0,0,"PRESS ANY KEY TO CONTINUE");

            if( totalclock > (60*3) )
            {
                endlvlmenutext(30,48,0,0,"Yer Time:");
                endlvlmenutext(30,64,0,0,"Par time:");
                endlvlmenutext(30,80,0,0,"Xatrix Time:");
                if(bonuscnt == 0)
                    bonuscnt++;

                if( totalclock > (60*4) )
                {
                    if(bonuscnt == 1)
                    {
                        bonuscnt++;
                        sound(404);
                    }
                    sprintf(tempbuf,"%02ld : %02ld",
                        (ps[myconnectindex].player_par/(26*60))%60,
                        (ps[myconnectindex].player_par/26)%60);
                    endlvlmenutext(191,48,0,0,tempbuf);

                    if(!boardfilename[0])
                    {
                        sprintf(tempbuf,"%02ld : %02ld",
                            (partime[ud.volume_number*7+ud.last_level-1]/(26*60))%60,
                            (partime[ud.volume_number*7+ud.last_level-1]/26)%60);
                        endlvlmenutext(191,64,0,0,tempbuf);

                        sprintf(tempbuf,"%02ld : %02ld",
                            (designertime[ud.volume_number*7+ud.last_level-1]/(26*60))%60,
                            (designertime[ud.volume_number*7+ud.last_level-1]/26)%60);
                        endlvlmenutext(191,80,0,0,tempbuf);
                    }

                }
            }
            if( totalclock > (60*6) )
            {
                endlvlmenutext(30,112,0,0,"Varmints Killed:");
                endlvlmenutext(30,128,0,0,"Varmints Left:");

                if(bonuscnt == 2)
                    bonuscnt++;

                if( totalclock > (60*7) )
                {
                    if(bonuscnt == 3)
                    {
                        bonuscnt++;
                        sound(422);
                    }
                    sprintf(tempbuf,"%-3ld",ps[myconnectindex].actors_killed);
                    endlvlmenutext(231,112,0,0,tempbuf);
                    if(ud.player_skill > 3 )
                    {
                        sprintf(tempbuf,"N/A");
                        endlvlmenutext(231,128,0,0,tempbuf);
                    }
                    else
                    {
                        if( (ps[myconnectindex].max_actors_killed-ps[myconnectindex].actors_killed) < 0 )
                            sprintf(tempbuf,"%-3ld",0);
                        else sprintf(tempbuf,"%-3ld",ps[myconnectindex].max_actors_killed-ps[myconnectindex].actors_killed);
                        endlvlmenutext(231,128,0,0,tempbuf);
                    }
                }
            }
            if( totalclock > (60*9) )
            {
                endlvlmenutext(30,144,0,0,"Secrets Found:");
                endlvlmenutext(30,160,0,0,"Secrets Missed:");
                if(bonuscnt == 4) bonuscnt++;

                if( totalclock > (60*10) )
                {
                    if(bonuscnt == 5)
                    {
                        bonuscnt++;
                        sound(404);
                    }
                    sprintf(tempbuf,"%-3ld",ps[myconnectindex].secret_rooms);
                    endlvlmenutext(231,144,0,0,tempbuf);
                    if( ps[myconnectindex].secret_rooms > 0 )
                        sprintf(tempbuf,"%-3ld%",(100*ps[myconnectindex].secret_rooms/ps[myconnectindex].max_secret_rooms));
                    sprintf(tempbuf,"%-3ld",ps[myconnectindex].max_secret_rooms-ps[myconnectindex].secret_rooms);
                    endlvlmenutext(231,160,0,0,tempbuf);
                }
            }

            if(totalclock > 10240 && totalclock < 10240+10240)
                totalclock = 1024;

            if( KB_KeyWaiting() && totalclock > (60*2) )
            {
                if( KB_KeyPressed( sc_F12 ) )
                {
                    KB_ClearKeyDown( sc_F12 );
                    screencapture("rdnk0000.pcx",0);
                }

                if( totalclock < (60*13) )
                {
                    KB_FlushKeyboardQueue();
                    totalclock = (60*13);
                }
                else if( totalclock < (1000000000L))
                   totalclock = (1000000000L);
            }
        }
        else break;
        nextpage();
    }
    if (turdlevel)
        turdlevel = 0;
    if (vixenlevel)
        vixenlevel = 0;
}
#endif

#endif

END_DUKE_NS
