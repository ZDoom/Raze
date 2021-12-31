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

#include "keydef.h"

#include "gamecontrol.h"
#include "network.h"
#include "version.h"
#include "network.h"

#include "misc.h"
#include "palettecontainer.h"

BEGIN_SW_NS

//////////////////////////////////////////////////////////////////////////////

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


//////////////////////////////////////////
// Set the amount of redness for damage
// the player just took
//////////////////////////////////////////
void SetFadeAmt(PLAYER* pp, short damage, uint8_t startcolor)
{
    const int FADE_DAMAGE_FACTOR = 3;   // 100 health / 32 shade cycles = 3.125

	short fadedamage = 0;

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

    auto color = GPalette.BaseColors[pp->StartColor];

    // Do initial palette set
    if (pp == Player + screenpeek)
    {
		videoFadePalette(color.r, color.g, color.b, faderamp[min(31, max(0, 32 - abs(pp->FadeAmt)))]);
        if (damage < -1000)
            pp->FadeAmt = 1000;  // Don't call DoPaletteFlash for underwater stuff
    }
}

//////////////////////////////////////////
// Do the screen reddness based on damage
//////////////////////////////////////////
void DoPaletteFlash(PLAYER* pp)
{
    const int MAXFADETICS = 5;

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

END_SW_NS
