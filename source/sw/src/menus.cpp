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
#include "ns.h"

#include "build.h"

#include "names2.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "sector.h"
#include "sprite.h"
#include "weapon.h"
#include "player.h"
#include "jsector.h"
#include "menus.h"
#include "pal.h"
#include "demo.h"

#include "keydef.h"

#include "gamecontrol.h"
#include "gamedefs.h"
#include "network.h"
#include "version.h"
#include "network.h"

#include "misc.h"
#include "palettecontainer.h"

BEGIN_SW_NS


//#define PLOCK_VERSION TRUE

extern SWBOOL ExitLevel, NewGame;


short TimeLimitTable[9] = {0,3,5,10,15,20,30,45,60};

SWBOOL
MNU_StartNetGame(void)
{
    extern SWBOOL ExitLevel, ShortGameMode, DemoInitOnce, FirstTimeIntoGame;
    extern short Level, Skill;
    // CTW REMOVED
    //extern int gTenActivated;
    // CTW REMOVED END
    int pnum;

    // always assumed that a demo is playing

    ready2send = 0;
    // Skill can go negative here
	Skill = gs.NetMonsters - 1;
    Level = gs.NetLevel + 1;
    DemoPlaying = FALSE;
    ExitLevel = TRUE;
    NewGame = TRUE;
    // restart demo for multi-play mode
    DemoInitOnce = FALSE;

    // TENSW: return if a joiner
    if (/* CTW REMOVED gTenActivated && */ !AutoNet && FirstTimeIntoGame)
        return TRUE;

    // need to set gNet vars for self
    // everone else gets a packet to set them
    gNet.AutoAim            = cl_autoaim;
    gNet.SpawnMarkers       = gs.NetSpawnMarkers;
    gNet.HurtTeammate       = gs.NetHurtTeammate;
    gNet.Nuke               = gs.NetNuke;
	gNet.KillLimit = gs.NetKillLimit * 10;
	gNet.TimeLimit = TimeLimitTable[gs.NetTimeLimit] * 60 * 120;

    if (ShortGameMode)
    {
        gNet.KillLimit /= 10;
        gNet.TimeLimit /= 2;
    }

    gNet.TimeLimitClock     = gNet.TimeLimit;
    gNet.TeamPlay           = gs.NetTeamPlay;
	gNet.MultiGameType = gs.NetGameType + 1;

    if (gNet.MultiGameType == MULTI_GAME_COMMBAT_NO_RESPAWN)
    {
        gNet.MultiGameType = MULTI_GAME_COMMBAT;
        gNet.NoRespawn = TRUE;
    }
    else
    {
        gNet.NoRespawn = FALSE;
    }

    if (CommEnabled)
    {
        PACKET_NEW_GAME p;

        p.PacketType = PACKET_TYPE_NEW_GAME;
        p.Level = Level;
        p.Skill = Skill;
        p.GameType = gs.NetGameType;
        p.AutoAim = cl_autoaim;
        p.HurtTeammate = gs.NetHurtTeammate;
        p.TeamPlay = gs.NetTeamPlay;
        p.SpawnMarkers = gs.NetSpawnMarkers;
        p.KillLimit = gs.NetKillLimit;
        p.TimeLimit = gs.NetTimeLimit;
        p.Nuke = gs.NetNuke;

        netbroadcastpacket((uint8_t*)(&p), sizeof(p));            // TENSW
    }


    return TRUE;
}


////////////////////////////////////////////////
// Measure the pixel width of a graphic string
////////////////////////////////////////////////
static char lg_xlat_num[] = { 0,1,2,3,4,5,6,7,8,9 };
#define FONT_LARGE_ALPHA 3706
#define FONT_LARGE_DIGIT 3732
#define MenuDrawFlags (ROTATE_SPRITE_SCREEN_CLIP)
#define MZ              65536
#define MENU_SHADE_DEFAULT 0
#define MENU_SHADE_INACTIVE 20

void MNU_MeasureStringLarge(const char *string, short *w, short *h)
{
    short ndx, width, height;
    char c;
    short pic;

    width = 0;
    height = *h;

    for (ndx = 0; (c = string[ndx]) != 0; ndx++)
    {
        if (isalpha(c))
        {
            c = toupper(c);
            pic = FONT_LARGE_ALPHA + (c - 'A');
        }
        else if (isdigit(c))
        {
            pic = FONT_LARGE_DIGIT + lg_xlat_num[(c - '0')];
        }
        else if (c == ' ')
        {
            width += 10;                 // Special case for space char
            continue;
        }
        else
        {
            continue;
        }

        width += tilesiz[pic].x+1;
        if (height < tilesiz[pic].y)
            height = tilesiz[pic].y;
    }

    *w = width;
    *h = height;
}

////////////////////////////////////////////////
// Draw a string using a graphic font
////////////////////////////////////////////////
void MNU_DrawStringLarge(short x, short y, const char *string, int shade)
{
    int ndx, offset;
    char c;
    short pic;

    offset = x;

    for (ndx = 0; (c = string[ndx]) != 0; ndx++)
    {
        if (isalpha(c))
        {
            c = toupper(c);
            pic = FONT_LARGE_ALPHA + (c - 'A');
        }
        else if (isdigit(c))
        {
            pic = FONT_LARGE_DIGIT + lg_xlat_num[(c - '0')];
        }
        else if (c == ' ')
        {
            offset += 10;
            continue;
        }
        else
        {
            continue;
        }

        rotatesprite(offset << 16, y << 16, MZ, 0, pic, shade, 0, MenuDrawFlags|ROTATE_SPRITE_CORNER, 0, 0, xdim - 1, ydim - 1);
        offset += tilesiz[pic].x + 1;
    }

}


////////////////////////////////////////////////
// Measure the pixel width of a graphic string
////////////////////////////////////////////////
void MNU_MeasureString(const char *string, short *w, short *h)
{
    short ndx, width, height;
    char c;
    short ac;

    if (string[0] == '^')
    {
        MNU_MeasureStringLarge(&string[1], w, h);
        return;
    }

    width = 0;
    height = *h;

    for (ndx = 0; (c = string[ndx]) != 0; ndx++)
    {
        ac = c - '!' + STARTALPHANUM;
        if ((ac < STARTALPHANUM || ac > ENDALPHANUM)  && c != asc_Space)
            break;

        if (c > asc_Space && c < 127)
        {
            width += tilesiz[ac].x;
            if (height < tilesiz[ac].y)
                height = tilesiz[ac].y;
        }
        else if (c == asc_Space)
            width += 4;                 // Special case for space char
    }

    *w = width;
    *h = height;
}

////////////////////////////////////////////////
// Draw a string using a graphic font
//
// MenuTextShade and MenuDrawFlags
////////////////////////////////////////////////
void MNU_DrawString(short x, short y, const char *string, short shade, short pal)
{
    int ndx, offset;
    char c;
    short ac;

    if (string[0] == '^')
    {
        MNU_DrawStringLarge(x,y, &string[1]);
        return;
    }

    offset = x;

    for (ndx = 0; (c = string[ndx]) != 0; ndx++)
    {
        ac = c - '!' + STARTALPHANUM;
        if ((ac < STARTALPHANUM || ac > ENDALPHANUM)  && c != asc_Space)
            break;

        if (c > asc_Space && c < 127)
        {
            rotatesprite(offset<<16,y<<16,MZ,0,ac, shade, pal, MenuDrawFlags, 0, 0, xdim - 1, ydim - 1);
            offset += tilesiz[ac].x;
        }
        else if (c == asc_Space)
            offset += 4;                // Special case for space char
    }

}

////////////////////////////////////////////////
// Measure the pixel width of a small font string
////////////////////////////////////////////////
void MNU_MeasureSmallString(const char *string, short *w, short *h)
{
    short ndx, width, height;
    char c;
    short ac;

    width = 0;
    height = *h;

    for (ndx = 0; (c = string[ndx]) != 0; ndx++)
    {
        ac = (c - '!') + 2930;
        if ((ac < 2930 || ac > 3023) && c != asc_Space)
            break;

        if (c > asc_Space && c < 127)
        {
            width += tilesiz[ac].x;
            if (height < tilesiz[ac].y)
                height = tilesiz[ac].y;
        }
        else if (c == asc_Space)
            width += 4;                 // Special case for space char
    }

    *w = width;
    *h = height;
}

////////////////////////////////////////////////
// Draw a string using a small graphic font
////////////////////////////////////////////////
void MNU_DrawSmallString(short x, short y, const char *string, short shade, short pal)
{
    int ndx;
    char c;
    short ac,offset;


    offset = x;

    for (ndx = 0; (c = string[ndx]) != 0; ndx++)
    {
        ac = c - '!' + 2930;
        if ((ac < 2930 || ac > 3023)  && c != asc_Space)
            break;

        if (c > asc_Space && c < 127)
        {
            rotatesprite(offset<<16,y<<16,MZ,0,ac, shade, pal, MenuDrawFlags, 0, 0, xdim - 1, ydim - 1);

            offset += tilesiz[ac].x;

        }
        else if (c == asc_Space)
        {
            offset += 4;                // Special case for space char
        }
    }

}


//////////////////////////////////////////////////////////////////////////////
#define FADE_DAMAGE_FACTOR  3   // 100 health / 32 shade cycles = 3.125

// Fades from 100% to 62.5% somewhat quickly,
//  then from 62.5% to 37.5% slowly,
//  then from 37.5% to 0% quickly.
// This seems to capture the pain caused by enemy shots, plus the extreme
//  fade caused by being blinded or intense pain.
// Perhaps the next step would be to apply a gentle smoothing to the
//  intersections of these lines.
static int faderamp[32] =
{
    // y=64-4x
    252,240,224,208,192,176,

    // y=44.8-(16/20)x
    160,156,152,152,148,
    144,140,136,136,132,
    128,124,120,120,116,
    112,108,104,104,100,

    // y=128-4x
    96,80,64,48,32,16
};


typedef struct RGB_color_typ
{
	unsigned char red;
	unsigned char green;
	unsigned char blue;
} RGB_color, * RGB_color_ptr;


//////////////////////////////////////////
// Set the amount of redness for damage
// the player just took
//////////////////////////////////////////
void SetFadeAmt(PLAYERp pp, short damage, unsigned char startcolor)
{
	short fadedamage = 0;
    RGB_color color;

	//Printf("SetAmt: fadeamt = %d, startcolor = %d, pp = %d",pp->FadeAmt,startcolor,pp->StartColor);

    if (abs(pp->FadeAmt) > 0 && startcolor == pp->StartColor)
        return;

    // Don't ever over ride flash bomb
    if (pp->StartColor == 1 && abs(pp->FadeAmt) > 0)
        return;

    // Reset the palette
    if (pp == Player + screenpeek)
    {
		videoFadePalette(0, 0, 0, 0);
    }

    if (damage < -150 && damage > -1000) fadedamage = 150;
    else if (damage < -1000) // Underwater
		fadedamage = abs(damage + 1000);
    else
        fadedamage = abs(damage);

    if (damage >= -5 && damage < 0)
        fadedamage += 10;

    // Don't let red to TOO red
    if (startcolor == COLOR_PAIN && fadedamage > 100) fadedamage = 100;

    pp->FadeAmt = fadedamage / FADE_DAMAGE_FACTOR;

    if (pp->FadeAmt <= 0)
    {
        pp->FadeAmt = 0;
        return;
    }

    // It's a health item, just do a preset flash amount
    if (damage > 0)
        pp->FadeAmt = 3;

    pp->StartColor = startcolor;

    pp->FadeTics = 0;

    color.red = GPalette.BaseColors[pp->StartColor].r;
    color.green = GPalette.BaseColors[pp->StartColor].g;
    color.blue = GPalette.BaseColors[pp->StartColor].b;

    // Do initial palette set
    if (pp == Player + screenpeek)
    {
		videoFadePalette(color.red, color.green, color.blue, faderamp[min(31, max(0, 32 - abs(pp->FadeAmt)))]);
        if (damage < -1000)
            pp->FadeAmt = 1000;  // Don't call DoPaletteFlash for underwater stuff
    }
}

//////////////////////////////////////////
// Do the screen reddness based on damage
//////////////////////////////////////////
#define MAXFADETICS     5
void DoPaletteFlash(PLAYERp pp)
{
    if (pp->FadeAmt <= 1)
    {
        pp->FadeAmt = 0;
        pp->StartColor = 0;
        if (pp == Player + screenpeek)
        {
			videoFadePalette(0, 0, 0, 0);
            DoPlayerDivePalette(pp);  // Check Dive again
            DoPlayerNightVisionPalette(pp);  // Check Night Vision again
        }

        return;
    }


    pp->FadeTics += synctics;           // Add this frame's tic amount to
    // counter

    if (pp->FadeTics >= MAXFADETICS)
    {
        while (pp->FadeTics >= MAXFADETICS)
        {
            pp->FadeTics -= MAXFADETICS;

            pp->FadeAmt--;              // Decrement FadeAmt till it gets to
            // 0 again.
        }
    }
    else
        return;                         // Return if they were not >
    // MAXFADETICS

    if (pp->FadeAmt > 32)
        return;

    if (pp->FadeAmt <= 1)
    {
        pp->FadeAmt = 0;
        pp->StartColor = 0;
        if (pp == Player + screenpeek)
        {
			videoFadePalette(0, 0, 0, 0);
            DoPlayerDivePalette(pp);  // Check Dive again
            DoPlayerNightVisionPalette(pp);  // Check Night Vision again
        }
        return;
    }
    else
    {
        // Only hard set the palette if this is currently the player's view
        if (pp == Player + screenpeek)
        {
            videoFadePalette(
                GPalette.BaseColors[pp->StartColor].r,
                GPalette.BaseColors[pp->StartColor].g,
                GPalette.BaseColors[pp->StartColor].b,
                faderamp[min(31, max(0, 32 - abs(pp->FadeAmt)))]
                );
        }

    }

}


SWBOOL MNU_ShareWareMessage()
{
	const char* extra_text;
	short w, h;

	if (SW_SHAREWARE)
	{
		extra_text = "Be sure to call 800-3DREALMS today";
		MNU_MeasureString(extra_text, &w, &h);
		MNU_DrawString(TEXT_XCENTER(w), 110, extra_text, 1, 16);
		extra_text = "and order the game.";
		MNU_MeasureString(extra_text, &w, &h);
		MNU_DrawString(TEXT_XCENTER(w), 120, extra_text, 1, 16);
		extra_text = "You are only playing the first ";
		MNU_MeasureString(extra_text, &w, &h);
		MNU_DrawString(TEXT_XCENTER(w), 130, extra_text, 1, 16);
		extra_text = "four levels, and are missing most";
		MNU_MeasureString(extra_text, &w, &h);
		MNU_DrawString(TEXT_XCENTER(w), 140, extra_text, 1, 16);
		extra_text = "of the game, weapons and monsters.";
		MNU_MeasureString(extra_text, &w, &h);
		MNU_DrawString(TEXT_XCENTER(w), 150, extra_text, 1, 16);
		extra_text = "See the ordering information.";
		MNU_MeasureString(extra_text, &w, &h);
		MNU_DrawString(TEXT_XCENTER(w), 160, extra_text, 1, 16);
		//SET(item->flags, mf_disabled);
	}
	return TRUE;
}

#if 0

#endif

END_SW_NS
