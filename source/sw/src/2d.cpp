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
#include "screenjob.h"
#include "game.h"
#include "sounds.h"
#include "v_draw.h"
#include "network.h"
#include "gamecontrol.h"


BEGIN_SW_NS

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DSWDRealmsScreen : public DScreenJob
{
public:
    DSWDRealmsScreen() : DScreenJob(fadein | fadeout) {}

    int Frame(uint64_t clock, bool skiprequest) override
    {
        const uint64_t duration = 5'000'000'000;
        const auto tex = tileGetTexture(THREED_REALMS_PIC, true);
        const int translation = TRANSLATION(Translation_BasePalettes, DREALMSPAL);

        twod->ClearScreen();
        DrawTexture(twod, tex, 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_TranslationIndex, translation, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);
        return skiprequest ? -1 : clock < duration ? 1 : 0;
    }
};


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void Logo(const CompletionFunc& completion)
{
    StopSound();
    PlayTheme();

    static const AnimSound logosound[] =
    {
        { 1, DIGI_NOMESSWITHWANG },
        { 5, DIGI_INTRO_SLASH },
        { 15, DIGI_INTRO_WHIRL },
        { -1, -1 }
    };
    static const int logoframetimes[] = { 360, 8, 128 };

    if (!AutoNet && !userConfig.nologo)
	{
		JobDesc jobs[3];
		int job = 0;
		jobs[job++] = { Create<DSWDRealmsScreen>() };
		jobs[job++] = { PlayVideo("sw.anm", logosound, logoframetimes)};
		RunScreenJob(jobs, job, completion, true, true);
	}
	else completion(false);
}



END_SW_NS
