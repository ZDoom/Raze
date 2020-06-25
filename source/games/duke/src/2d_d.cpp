//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment

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
Modifications for JonoF's port by Jonathon Fowler (jf@jonof.id.au)
*/
//-------------------------------------------------------------------------

// this file collects all 2D content of the game that was scattered across multiple sources originally.
// All this should transition to a more modern, preferably localization friendly, approach later.



// Text output - needs to transition to the actual font routines once everything is set up.

int gametext(int x,int y,const char *t,char s,short dabits)
{
    short ac,newx;
    char centre;
    const char *oldt;

    centre = ( x == (320>>1) );
    newx = 0;
    oldt = t;

    if(centre)
    {
        while(*t)
        {
            if(*t == 32) {newx+=5;t++;continue;}
            else ac = *t - '!' + STARTALPHANUM;

            if( ac < STARTALPHANUM || ac > ENDALPHANUM ) break;

            if(*t >= '0' && *t <= '9')
                newx += 8;
            else newx += tilesizx[ac];
            t++;
        }

        t = oldt;
        x = (320>>1)-(newx>>1);
    }

    while(*t)
    {
        if(*t == 32) {x+=5;t++;continue;}
        else ac = *t - '!' + STARTALPHANUM;

        if( ac < STARTALPHANUM || ac > ENDALPHANUM )
            break;

        rotatesprite(x<<16,y<<16,65536L,0,ac,s,0,dabits,0,0,xdim-1,ydim-1);

        if(*t >= '0' && *t <= '9')
            x += 8;
        else x += tilesizx[ac];

        t++;
    }

    return (x);
}

int gametextpal(int x,int y,const char *t,char s,unsigned char p)
{
    short ac,newx;
    char centre;
    const char *oldt;

    centre = ( x == (320>>1) );
    newx = 0;
    oldt = t;

    if(centre)
    {
        while(*t)
        {
            if(*t == 32) {newx+=5;t++;continue;}
            else ac = *t - '!' + STARTALPHANUM;

            if( ac < STARTALPHANUM || ac > ENDALPHANUM ) break;

            if(*t >= '0' && *t <= '9')
                newx += 8;
            else newx += tilesizx[ac];
            t++;
        }

        t = oldt;
        x = (320>>1)-(newx>>1);
    }

    while(*t)
    {
        if(*t == 32) {x+=5;t++;continue;}
        else ac = *t - '!' + STARTALPHANUM;

        if( ac < STARTALPHANUM || ac > ENDALPHANUM )
            break;

        rotatesprite(x<<16,y<<16,65536L,0,ac,s,p,2+8+16,0,0,xdim-1,ydim-1);
        if(*t >= '0' && *t <= '9')
            x += 8;
        else x += tilesizx[ac];

        t++;
    }

    return (x);
}

int gametextpart(int x,int y,const char *t,char s,short p)
{
    short ac,newx, cnt;
    char centre;
    const char *oldt;

    centre = ( x == (320>>1) );
    newx = 0;
    oldt = t;
    cnt = 0;

    if(centre)
    {
        while(*t)
        {
            if(cnt == p) break;

            if(*t == 32) {newx+=5;t++;continue;}
            else ac = *t - '!' + STARTALPHANUM;

            if( ac < STARTALPHANUM || ac > ENDALPHANUM ) break;

            newx += tilesizx[ac];
            t++;
            cnt++;

        }

        t = oldt;
        x = (320>>1)-(newx>>1);
    }

    cnt = 0;
    while(*t)
    {
        if(*t == 32) {x+=5;t++;continue;}
        else ac = *t - '!' + STARTALPHANUM;

        if( ac < STARTALPHANUM || ac > ENDALPHANUM ) break;

        if(cnt == p)
        {
            rotatesprite(x<<16,y<<16,65536L,0,ac,s,1,2+8+16,0,0,xdim-1,ydim-1);
            break;
        }
        else
            rotatesprite(x<<16,y<<16,65536L,0,ac,s,0,2+8+16,0,0,xdim-1,ydim-1);

        x += tilesizx[ac];

        t++;
        cnt++;
    }

    return (x);
}

void gamenumber(int x,int y,int n,char s)
{
    char b[10];
    //ltoa(n,b,10);
    Bsnprintf(b,10,"%d",n);
    gametext(x,y,b,s,2+8+16);
}

// ANM player - catastrophically shitty implementation. Todo: Move the sound and fps data to a control file per movie.

void endanimsounds(int fr)
{
    switch(ud.volume_number)
    {
        case 0:break;
        case 1:
            switch(fr)
            {
                case 1:
                    sound(WIND_AMBIENCE);
                    break;
                case 26:
                    sound(ENDSEQVOL2SND1);
                    break;
                case 36:
                    sound(ENDSEQVOL2SND2);
                    break;
                case 54:
                    sound(THUD);
                    break;
                case 62:
                    sound(ENDSEQVOL2SND3);
                    break;
                case 75:
                    sound(ENDSEQVOL2SND4);
                    break;
                case 81:
                    sound(ENDSEQVOL2SND5);
                    break;
                case 115:
                    sound(ENDSEQVOL2SND6);
                    break;
                case 124:
                    sound(ENDSEQVOL2SND7);
                    break;
            }
            break;
        case 2:
            switch(fr)
            {
                case 1:
                    sound(WIND_REPEAT);
                    break;
                case 98:
                    sound(DUKE_GRUNT);
                    break;
                case 82+20:
                    sound(THUD);
                    sound(SQUISHED);
                    break;
                case 104+20:
                    sound(ENDSEQVOL3SND3);
                    break;
                case 114+20:
                    sound(ENDSEQVOL3SND2);
                    break;
                case 158:
                    sound(PIPEBOMB_EXPLODE);
                    break;
            }
            break;
    }
}

void logoanimsounds(int fr)
{
    switch(fr)
    {
        case 1:
            sound(FLY_BY);
            break;
        case 19:
            sound(PIPEBOMB_EXPLODE);
            break;
    }
}

void intro4animsounds(int fr)
{
    switch(fr)
    {
        case 1:
            sound(INTRO4_B);
            break;
        case 12:
        case 34:
            sound(SHORT_CIRCUIT);
            break;
        case 18:
            sound(INTRO4_5);
            break;
    }
}

void first4animsounds(int fr)
{
    switch(fr)
    {
        case 1:
            sound(INTRO4_1);
            break;
        case 12:
            sound(INTRO4_2);
            break;
        case 7:
            sound(INTRO4_3);
            break;
        case 26:
            sound(INTRO4_4);
            break;
    }
}

void intro42animsounds(int fr)
{
    switch(fr)
    {
        case 10:
            sound(INTRO4_6);
            break;
    }
}




void endanimvol41(int fr)
{
    switch(fr)
    {
        case 3:
            sound(DUKE_UNDERWATER);
            break;
        case 35:
            sound(VOL4ENDSND1);
            break;
    }
}

void endanimvol42(int fr)
{
    switch(fr)
    {
        case 11:
            sound(DUKE_UNDERWATER);
            break;
        case 20:
            sound(VOL4ENDSND1);
            break;
        case 39:
            sound(VOL4ENDSND2);
            break;
        case 50:
            FX_StopAllSounds();
            break;
    }
}

void endanimvol43(int fr)
{
    switch(fr)
    {
        case 1:
            sound(BOSS4_DEADSPEECH);
            break;
        case 40:
            sound(VOL4ENDSND1);
            sound(DUKE_UNDERWATER);
            break;
        case 50:
            sound(BIGBANG);
            break;
    }
}


void playanm(const char *fn,char t)
{
    unsigned char *animbuf, *palptr, palbuf[768];
    int i, j, k, length=0, numframes=0;
    int32 handle=-1;
    UserInput uinfo;

//    return;
    AnimTextures animtex;

    inputState.ClearAllInput();

    uinfo.dir = dir_None;
    uinfo.button0 = uinfo.button1 = FALSE;
	
	auto fr = fileSystem.OpenFileReader(fn);

	if (!fr.isOpen())
		goto ENDOFANIMLOOP;

	buffer = fr.ReadPadded(1);
	fr.Close();

    anim->animbuf = buffer.Data();

    if (memcmp(anim->animbuf, "LPF ", 4) ||
        ANIM_LoadAnim(anim->animbuf, buffer.Size()-1) < 0 ||
        (numframes = ANIM_NumFrames()) <= 0)
    {
        Printf("%s: Invalid ANM file\n", fn);
        goto ENDOFANIMLOOP;
    }
	
    animtex.SetSize(AnimTexture::Paletted, 320, 200);
    palptr = ANIM_GetPalette();

    ototalclock = totalclock + 10;

    for(i=1;i<numframes;i++)
    {
        while(totalclock < ototalclock)
        {
            handleevents();
            getpackets();
			if (inputState.CheckAllInput())
                goto ENDOFANIMLOOP;
        }
        animtex.SetFrame(ANIM_GetPalette(), ANIM_DrawFrame(i));

		// ouch!
        if(t == 10) ototalclock += 14;
        else if(t == 9) ototalclock += 10;
        else if(t == 7) ototalclock += 18;
        else if(t == 6) ototalclock += 14;
        else if(t == 5) ototalclock += 9;
        else if(ud.volume_number == 3) ototalclock += 10;
        else if(ud.volume_number == 2) ototalclock += 10;
        else if(ud.volume_number == 1) ototalclock += 18;
        else                           ototalclock += 10;

        twod->ClearScreen();
		DrawTexture(twod, animtex.GetFrame(), 0, 0, DTA_FullscreenEx, 3, DTA_MASKED, false, TAG_DONE);
		
        clearview(0);
        rotatesprite(0<<16,0<<16,65536L,512,TILE_ANIM,0,0,2+4+8+16+64, 0,0,xdim-1,ydim-1);
        videoNextPage();
        inputState.ClearAllInput();

        if(t == 8) endanimvol41(i);
        else if(t == 10) endanimvol42(i);
        else if(t == 11) endanimvol43(i);
        else if(t == 9) intro42animsounds(i);
        else if(t == 7) intro4animsounds(i);
        else if(t == 6) first4animsounds(i);
        else if(t == 5) logoanimsounds(i);
        else if(t < 4) endanimsounds(i);
    }

    ENDOFANIMLOOP:

    inputState.ClearAllInput();
    ANIM_FreeAnim ();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------
 
 void Logo(void)
{
    short i,j,soundanm;
    UserInput uinfo;

    soundanm = 0;

    ready2send = 0;

    KB_FlushKeyboardQueue();
    KB_ClearKeysDown(); // JBF

    setview(0,0,xdim-1,ydim-1);
    clearview(0L);
    IFISSOFTMODE palto(0,0,0,63);

    flushperms();
    nextpage();

    stopmusic();
    FX_StopAllSounds(); // JBF 20031228
    clearsoundlocks();  // JBF 20031228

if (VOLUMEALL) {

    if(!KB_KeyWaiting() && nomorelogohack == 0)
    {
        getpackets();
        playanm("logo.anm",5);
        IFISSOFTMODE palto(0,0,0,63);
        KB_FlushKeyboardQueue();
        KB_ClearKeysDown(); // JBF
    }

    clearview(0L);
    nextpage();
}

    playmusic(&env_music_fn[0][0]);
    if (!NAM) {
        fadepal(0,0,0, 0,64,7);
        //ps[myconnectindex].palette = drealms;
        //palto(0,0,0,63);
        setgamepalette(&ps[myconnectindex], drealms, 3);    // JBF 20040308
        rotatesprite(0,0,65536L,0,DREALMS,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
        nextpage();
        fadepal(0,0,0, 63,0,-7);
        totalclock = 0;

        uinfo.dir = dir_None;
        uinfo.button0 = uinfo.button1 = FALSE;
        KB_FlushKeyboardQueue();
        do {
            handleevents();
            getpackets();
            CONTROL_GetUserInput(&uinfo);
        } while (totalclock < (120*7) && !KB_KeyWaiting() && !uinfo.button0 && !uinfo.button1 );
        CONTROL_ClearUserInput(&uinfo);

        KB_ClearKeysDown(); // JBF
    }

    fadepal(0,0,0, 0,64,7);
    clearview(0L);
    nextpage();

    //ps[myconnectindex].palette = titlepal;
    setgamepalette(&ps[myconnectindex], titlepal, 3);   // JBF 20040308
    flushperms();
    rotatesprite(0,0,65536L,0,BETASCREEN,0,0,2+8+16+64,0,0,xdim-1,ydim-1);
    fadepal(0,0,0, 63,0,-7);
    totalclock = 0;

    uinfo.dir = dir_None;
    uinfo.button0 = uinfo.button1 = FALSE;
    KB_FlushKeyboardQueue();
    do {
        clearview(0);
        rotatesprite(0,0,65536L,0,BETASCREEN,0,0,2+8+16+64,0,0,xdim-1,ydim-1);

        if( totalclock > 120 && totalclock < (120+60) )
        {
            if(soundanm == 0)
            {
                soundanm = 1;
                sound(PIPEBOMB_EXPLODE);
            }
            rotatesprite(160<<16,104<<16,(totalclock-120)<<10,0,DUKENUKEM,0,0,2+8,0,0,xdim-1,ydim-1);
        }
        else if( totalclock >= (120+60) )
            rotatesprite(160<<16,(104)<<16,60<<10,0,DUKENUKEM,0,0,2+8,0,0,xdim-1,ydim-1);

        if( totalclock > 220 && totalclock < (220+30) )
        {
            if( soundanm == 1)
            {
                soundanm = 2;
                sound(PIPEBOMB_EXPLODE);
            }

            rotatesprite(160<<16,(104)<<16,60<<10,0,DUKENUKEM,0,0,2+8,0,0,xdim-1,ydim-1);
            rotatesprite(160<<16,(129)<<16,(totalclock - 220 )<<11,0,THREEDEE,0,0,2+8,0,0,xdim-1,ydim-1);
        }
        else if( totalclock >= (220+30) )
            rotatesprite(160<<16,(129)<<16,30<<11,0,THREEDEE,0,0,2+8,0,0,xdim-1,ydim-1);

        if (PLUTOPAK) { // JBF 20030804
            if( totalclock >= 280 && totalclock < 395 )
            {
                rotatesprite(160<<16,(151)<<16,(410-totalclock)<<12,0,PLUTOPAKSPRITE+1,0,0,2+8,0,0,xdim-1,ydim-1);
                if(soundanm == 2)
                {
                    soundanm = 3;
                    sound(FLY_BY);
                }
            }
            else if( totalclock >= 395 )
            {
                if(soundanm == 3)
                {
                    soundanm = 4;
                    sound(PIPEBOMB_EXPLODE);
                }
                rotatesprite(160<<16,(151)<<16,30<<11,0,PLUTOPAKSPRITE+1,0,0,2+8,0,0,xdim-1,ydim-1);
            }
        }

        nextpage();
        handleevents();
        getpackets();
        CONTROL_GetUserInput(&uinfo);
    } while(totalclock < (860+120) && !KB_KeyWaiting() && !uinfo.button0 && !uinfo.button1);
    CONTROL_ClearUserInput(&uinfo);
    KB_ClearKeysDown(); // JBF

    if(ud.multimode > 1)
    {
        rotatesprite(0,0,65536L,0,BETASCREEN,0,0,2+8+16+64,0,0,xdim-1,ydim-1);

        rotatesprite(160<<16,(104)<<16,60<<10,0,DUKENUKEM,0,0,2+8,0,0,xdim-1,ydim-1);
        rotatesprite(160<<16,(129)<<16,30<<11,0,THREEDEE,0,0,2+8,0,0,xdim-1,ydim-1);
        if (PLUTOPAK)   // JBF 20030804
            rotatesprite(160<<16,(151)<<16,30<<11,0,PLUTOPAKSPRITE+1,0,0,2+8,0,0,xdim-1,ydim-1);

        gametext(160,190,"WAITING FOR PLAYERS",14,2);
        nextpage();
    }

    waitforeverybody();

    flushperms();
    clearview(0L);
    nextpage();

    //ps[myconnectindex].palette = palette;
    setgamepalette(&ps[myconnectindex], palette, 0);    // JBF 20040308
    sound(NITEVISION_ONOFF);

    //palto(0,0,0,0);
    clearview(0L);
}

void dobonus(char bonusonly)
{
    short t, r, tinc,gfx_offset;
    long i, y,xfragtotal,yfragtotal;
    short bonuscnt;

    long breathe[] =
    {
         0,  30,VICTORY1+1,176,59,
        30,  60,VICTORY1+2,176,59,
        60,  90,VICTORY1+1,176,59,
        90, 120,0         ,176,59
    };

    long bossmove[] =
    {
         0, 120,VICTORY1+3,86,59,
       220, 260,VICTORY1+4,86,59,
       260, 290,VICTORY1+5,86,59,
       290, 320,VICTORY1+6,86,59,
       320, 350,VICTORY1+7,86,59,
       350, 380,VICTORY1+8,86,59
    };

    bonuscnt = 0;

    for(t=0;t<64;t+=7) palto(0,0,0,t);
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
            if(ud.lockout == 0)
            {
                clearview(0L);
                rotatesprite(0,50<<16,65536L,0,VICTORY1,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                nextpage();
                ps[myconnectindex].palette = endingpal;
                for(t=63;t>=0;t--) palto(0,0,0,t);

                KB_FlushKeyboardQueue();
                totalclock = 0; tinc = 0;
                while( 1 )
                {
                    clearview(0L);
                    rotatesprite(0,50<<16,65536L,0,VICTORY1,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);

                    // boss
                    if( totalclock > 390 && totalclock < 780 )
                        for(t=0;t<35;t+=5) if( bossmove[t+2] && (totalclock%390) > bossmove[t] && (totalclock%390) <= bossmove[t+1] )
                    {
                        if(t==10 && bonuscnt == 1) { sound(SHOTGUN_FIRE);sound(SQUISHED); bonuscnt++; }
                        rotatesprite(bossmove[t+3]<<16,bossmove[t+4]<<16,65536L,0,bossmove[t+2],0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                    }

                    // Breathe
                    if( totalclock < 450 || totalclock >= 750 )
                    {
                        if(totalclock >= 750)
                        {
                            rotatesprite(86<<16,59<<16,65536L,0,VICTORY1+8,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                            if(totalclock >= 750 && bonuscnt == 2) { sound(DUKETALKTOBOSS); bonuscnt++; }
                        }
                        for(t=0;t<20;t+=5)
                            if( breathe[t+2] && (totalclock%120) > breathe[t] && (totalclock%120) <= breathe[t+1] )
                        {
                                if(t==5 && bonuscnt == 0)
                                {
                                    sound(BOSSTALKTODUKE);
                                    bonuscnt++;
                                }
                                rotatesprite(breathe[t+3]<<16,breathe[t+4]<<16,65536L,0,breathe[t+2],0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                        }
                    }

                    getpackets();
                    nextpage();
                    if( KB_KeyWaiting() ) break;
                }
            }

            for(t=0;t<64;t++) palto(0,0,0,t);

            KB_FlushKeyboardQueue();
            ps[myconnectindex].palette = palette;

            rotatesprite(0,0,65536L,0,3292,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
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
                playanm("cineov2.anm",1);
                KB_FlushKeyBoardQueue();
                clearview(0L);
                nextpage();
            }

            sound(PIPEBOMB_EXPLODE);

            for(t=0;t<64;t++) palto(0,0,0,t);
            setview(0,0,xdim-1,ydim-1);
            KB_FlushKeyboardQueue();
            ps[myconnectindex].palette = palette;
            rotatesprite(0,0,65536L,0,3293,0,0,2+8+16+64, 0,0,xdim-1,ydim-1);
            nextpage(); for(t=63;t>0;t--) palto(0,0,0,t);
            while( !KB_KeyWaiting() ) getpackets();
            for(t=0;t<64;t++) palto(0,0,0,t);

            break;

        case 3:

            setview(0,0,xdim-1,ydim-1);

            MUSIC_StopSong();
            clearview(0L);
            nextpage();

            if(ud.lockout == 0)
            {
                KB_FlushKeyboardQueue();
                playanm("vol4e1.anm",8);
                clearview(0L);
                nextpage();
                playanm("vol4e2.anm",10);
                clearview(0L);
                nextpage();
                playanm("vol4e3.anm",11);
                clearview(0L);
                nextpage();
            }

            FX_StopAllSounds();
            clearsoundlocks();
            sound(ENDSEQVOL3SND4);
            KB_FlushKeyBoardQueue();

            ps[myconnectindex].palette = palette;
            palto(0,0,0,63);
            clearview(0L);
            menutext(160,60,0,0,"THANKS TO ALL OUR");
            menutext(160,60+16,0,0,"FANS FOR GIVING");
            menutext(160,60+16+16,0,0,"US BIG HEADS.");
            menutext(160,70+16+16+16,0,0,"LOOK FOR A DUKE NUKEM 3D");
            menutext(160,70+16+16+16+16,0,0,"SEQUEL SOON.");
            nextpage();

            for(t=63;t>0;t-=3) palto(0,0,0,t);
            KB_FlushKeyboardQueue();
            while(!KB_KeyWaiting()) getpackets();
            for(t=0;t<64;t+=3) palto(0,0,0,t);

            clearview(0L);
            nextpage();

            playanm("DUKETEAM.ANM",4);

            KB_FlushKeyBoardQueue();
            while(!KB_KeyWaiting()) getpackets();

            clearview(0L);
            nextpage();
            palto(0,0,0,63);

            FX_StopAllSounds();
            clearsoundlocks();
            KB_FlushKeyBoardQueue();

            break;

        case 2:

            MUSIC_StopSong();
            clearview(0L);
            nextpage();
            if(ud.lockout == 0)
            {
                for(t=63;t>=0;t--) palto(0,0,0,t);
                playanm("cineov3.anm",2);
                KB_FlushKeyBoardQueue();
                ototalclock = totalclock+200;
                while(totalclock < ototalclock) getpackets();
                clearview(0L);
                nextpage();

                FX_StopAllSounds();
                clearsoundlocks();
            }

            playanm("RADLOGO.ANM",3);

            if( ud.lockout == 0 && !KB_KeyWaiting() )
            {
                sound(ENDSEQVOL3SND5);
                while(Sound[ENDSEQVOL3SND5].lock>=200) getpackets();
                if(KB_KeyWaiting()) goto ENDANM;
                sound(ENDSEQVOL3SND6);
                while(Sound[ENDSEQVOL3SND6].lock>=200) getpackets();
                if(KB_KeyWaiting()) goto ENDANM;
                sound(ENDSEQVOL3SND7);
                while(Sound[ENDSEQVOL3SND7].lock>=200) getpackets();
                if(KB_KeyWaiting()) goto ENDANM;
                sound(ENDSEQVOL3SND8);
                while(Sound[ENDSEQVOL3SND8].lock>=200) getpackets();
                if(KB_KeyWaiting()) goto ENDANM;
                sound(ENDSEQVOL3SND9);
                while(Sound[ENDSEQVOL3SND9].lock>=200) getpackets();
            }

            KB_FlushKeyBoardQueue();
            totalclock = 0;
            while(!KB_KeyWaiting() && totalclock < 120) getpackets();

            ENDANM:

            FX_StopAllSounds();
            clearsoundlocks();

            KB_FlushKeyBoardQueue();

            clearview(0L);

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
            sound(BONUSMUSIC);

        rotatesprite(0,0,65536L,0,MENUSCREEN,16,0,2+8+16+64,0,0,xdim-1,ydim-1);
        rotatesprite(160<<16,34<<16,65536L,0,INGAMEDUKETHREEDEE,0,0,10,0,0,xdim-1,ydim-1);

#ifndef UK
        rotatesprite((260)<<16,36<<16,65536L,0,PLUTOPAKSPRITE+2,0,0,2+8,0,0,xdim-1,ydim-1);
#endif

        gametext(160,58+2,"MULTIPLAYER TOTALS",0,2+8+16);
        gametext(160,58+10,level_names[(ud.volume_number*11)+ud.last_level-1],0,2+8+16);

        gametext(160,165,"PRESS ANY KEY TO CONTINUE",0,2+8+16);


        t = 0;
        minitext(23,80,"   NAME                                           KILLS",8,2+8+16+128);
        for(i=0;i<playerswhenstarted;i++)
        {
            sprintf(tempbuf,"%-4ld",i+1);
            minitext(92+(i*23),80,tempbuf,3,2+8+16+128);
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
                    minitext(92+(y*23),90+t,tempbuf,2,2+8+16+128);
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
            minitext(101+(8*23),90+t,tempbuf,2,2+8+16+128);

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
            minitext(92+(y*23),96+(8*7),tempbuf,2,2+8+16+128);
        }

        minitext(45,96+(8*7),"DEATHS",8,2+8+16+128);
        nextpage();

        for(t=0;t<64;t+=7)
            palto(0,0,0,63-t);

        KB_FlushKeyboardQueue();
        while(KB_KeyWaiting()==0) getpackets();

        if( KB_KeyPressed( sc_F12 ) )
        {
            KB_ClearKeyDown( sc_F12 );
#ifdef NAM			
            screencapture("WW20000.pcx",0);
#else
#ifdef NAM			
            screencapture("nam0000.pcx",0);
#else
            screencapture("duke0000.pcx",0);
#endif
#endif
        }

        if(bonusonly || ud.multimode > 1) return;

        for(t=0;t<64;t+=7) palto(0,0,0,t);
    }

    if(bonusonly || ud.multimode > 1) return;

    switch(ud.volume_number)
    {
        case 1:
            gfx_offset = 5;
            break;
        default:
            gfx_offset = 0;
            break;
    }

    rotatesprite(0,0,65536L,0,BONUSSCREEN+gfx_offset,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);

    menutext(160,20-6,0,0,&level_names[(ud.volume_number*11)+ud.last_level-1][0]);
    menutext(160,36-6,0,0,"COMPLETED");

    gametext(160,192,"PRESS ANY KEY TO CONTINUE",16,2+8+16);

    if(!(MusicToggle == 0 || MusicDevice == NumSoundCards))
        sound(BONUSMUSIC);

    nextpage();
    KB_FlushKeyboardQueue();
    for(t=0;t<64;t++) palto(0,0,0,63-t);
    bonuscnt = 0;
    totalclock = 0; tinc = 0;

    while( 1 )
    {
        if(ps[myconnectindex].gm&MODE_EOL)
        {
            rotatesprite(0,0,65536L,0,BONUSSCREEN+gfx_offset,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);

            if( totalclock > (1000000000L) && totalclock < (1000000320L) )
            {
                switch( (totalclock>>4)%15 )
                {
                    case 0:
                        if(bonuscnt == 6)
                        {
                            bonuscnt++;
                            sound(SHOTGUN_COCK);
                            switch(rand()&3)
                            {
                                case 0:
                                    sound(BONUS_SPEECH1);
                                    break;
                                case 1:
                                    sound(BONUS_SPEECH2);
                                    break;
                                case 2:
                                    sound(BONUS_SPEECH3);
                                    break;
                                case 3:
                                    sound(BONUS_SPEECH4);
                                    break;
                            }
                        }
                    case 1:
                    case 4:
                    case 5:
                        rotatesprite(199<<16,31<<16,65536L,0,BONUSSCREEN+3+gfx_offset,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                        break;
                    case 2:
                    case 3:
                       rotatesprite(199<<16,31<<16,65536L,0,BONUSSCREEN+4+gfx_offset,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
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
                        rotatesprite(199<<16,31<<16,65536L,0,BONUSSCREEN+1+gfx_offset,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                        break;
                    case 2:
                        rotatesprite(199<<16,31<<16,65536L,0,BONUSSCREEN+2+gfx_offset,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                        break;
                }
            }

            menutext(160,20-6,0,0,&level_names[(ud.volume_number*11)+ud.last_level-1][0]);
            menutext(160,36-6,0,0,"COMPLETED");

            gametext(160,192,"PRESS ANY KEY TO CONTINUE",16,2+8+16);

            if( totalclock > (60*3) )
            {
                gametext(10,59+9,"Your Time:",0,2+8+16);
                gametext(10,69+9,"Par time:",0,2+8+16);
#ifdef NAM
//				gametext(10,78+9,"Green Beret's Time:",0,2+8+16);
#else
				gametext(10,78+9,"3D Realms' Time:",0,2+8+16);
#endif
                if(bonuscnt == 0)
                    bonuscnt++;

                if( totalclock > (60*4) )
                {
                    if(bonuscnt == 1)
                    {
                        bonuscnt++;
                        sound(PIPEBOMB_EXPLODE);
                    }
                    sprintf(tempbuf,"%02ld:%02ld",
                        (ps[myconnectindex].player_par/(26*60))%60,
                        (ps[myconnectindex].player_par/26)%60);
                    gametext((320>>2)+71,60+9,tempbuf,0,2+8+16);

                    sprintf(tempbuf,"%02ld:%02ld",
                        (partime[ud.volume_number*11+ud.last_level-1]/(26*60))%60,
                        (partime[ud.volume_number*11+ud.last_level-1]/26)%60);
                    gametext((320>>2)+71,69+9,tempbuf,0,2+8+16);

#ifndef NAM					
                    sprintf(tempbuf,"%02ld:%02ld",
                        (designertime[ud.volume_number*11+ud.last_level-1]/(26*60))%60,
                        (designertime[ud.volume_number*11+ud.last_level-1]/26)%60);
                    gametext((320>>2)+71,78+9,tempbuf,0,2+8+16);
#endif					

                }
            }
            if( totalclock > (60*6) )
            {
                gametext(10,94+9,"Enemies Killed:",0,2+8+16);
                gametext(10,99+4+9,"Enemies Left:",0,2+8+16);

                if(bonuscnt == 2)
                {
                    bonuscnt++;
                    sound(FLY_BY);
                }

                if( totalclock > (60*7) )
                {
                    if(bonuscnt == 3)
                    {
                        bonuscnt++;
                        sound(PIPEBOMB_EXPLODE);
                    }
                    sprintf(tempbuf,"%-3ld",ps[myconnectindex].actors_killed);
                    gametext((320>>2)+70,93+9,tempbuf,0,2+8+16);
                    if(ud.player_skill > 3 )
                    {
                        sprintf(tempbuf,"N/A");
                        gametext((320>>2)+70,99+4+9,tempbuf,0,2+8+16);
                    }
                    else
                    {
                        if( (ps[myconnectindex].max_actors_killed-ps[myconnectindex].actors_killed) < 0 )
                            sprintf(tempbuf,"%-3ld",0);
                        else sprintf(tempbuf,"%-3ld",ps[myconnectindex].max_actors_killed-ps[myconnectindex].actors_killed);
                        gametext((320>>2)+70,99+4+9,tempbuf,0,2+8+16);
                    }
                }
            }
            if( totalclock > (60*9) )
            {
                gametext(10,120+9,"Secrets Found:",0,2+8+16);
                gametext(10,130+9,"Secrets Missed:",0,2+8+16);
                if(bonuscnt == 4) bonuscnt++;

                if( totalclock > (60*10) )
                {
                    if(bonuscnt == 5)
                    {
                        bonuscnt++;
                        sound(PIPEBOMB_EXPLODE);
                    }
                    sprintf(tempbuf,"%-3ld",ps[myconnectindex].secret_rooms);
                    gametext((320>>2)+70,120+9,tempbuf,0,2+8+16);
                    if( ps[myconnectindex].secret_rooms > 0 )
                        sprintf(tempbuf,"%-3ld%",(100*ps[myconnectindex].secret_rooms/ps[myconnectindex].max_secret_rooms));
                    sprintf(tempbuf,"%-3ld",ps[myconnectindex].max_secret_rooms-ps[myconnectindex].secret_rooms);
                    gametext((320>>2)+70,130+9,tempbuf,0,2+8+16);
                }
            }

            if(totalclock > 10240 && totalclock < 10240+10240)
                totalclock = 1024;

            if( KB_KeyWaiting() && totalclock > (60*2) )
            {
                if( KB_KeyPressed( sc_F12 ) )
                {
                    KB_ClearKeyDown( sc_F12 );
#ifdef WW2
                    screencapture("WW20000.pcx",0);
#else
#ifdef NAM
                    screencapture("nam0000.pcx",0);
#else
                    screencapture("duke0000.pcx",0);
#endif
#endif
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
}


