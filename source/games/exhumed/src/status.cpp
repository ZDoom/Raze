//-------------------------------------------------------------------------
/*
Copyright (C) 2020-2021 Christoph Oelckers

This file is part of Raze.

This is free software; you can redistribute it and/or
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
*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "engine.h"
#include "player.h"
#include "status.h"
#include "exhumed.h"
#include "sequence.h"
#include "names.h"
#include "view.h"
#include "v_2ddrawer.h"
#include "statusbar.h"
#include "v_draw.h"
#include "automap.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

BEGIN_PS_NS



// All this must be moved into the status bar once it is made persistent!
int nStatusSeqOffset;

void InitStatus()
{
    nStatusSeqOffset = SeqOffsets[kSeqStatus];
}


//---------------------------------------------------------------------------
//
// This is to hide the dirt from the script code.
// These sequence arrays later need to be refactored 
// if this is ever supposed to become a useful feature, 
// so hide the dirty internals behind a handful of functions.
//
//---------------------------------------------------------------------------

struct ChunkFrame
{
    FTextureID tex;
    int x, y;
    int flags;

    void GetChunkFrame(int nFrameBase)
    {
        x = ChunkXpos[nFrameBase];
        y = ChunkYpos[nFrameBase];
        auto ttex = tileGetTexture(ChunkPict[nFrameBase]);
        if (ttex) tex = ttex->GetID();
        else tex.SetInvalid();
        flags = ChunkFlag[nFrameBase];
    }
};

DEFINE_ACTION_FUNCTION(_ChunkFrame, GetChunkFrame)
{
    PARAM_SELF_STRUCT_PROLOGUE(ChunkFrame);
    PARAM_INT(index);
    self->GetChunkFrame(index);
    return 0;
}

DEFINE_ACTION_FUNCTION(_Exhumed, GetStatusSequence)
{
    PARAM_PROLOGUE;
    PARAM_INT(nSequence);
    PARAM_INT(frameindex);

    frameindex += SeqBase[nStatusSeqOffset + nSequence];
    if (numret > 0) ret[0].SetInt(FrameBase[frameindex]);
    if (numret > 1) ret[1].SetInt(FrameSize[frameindex]);
    return min(numret, 2);
}

DEFINE_ACTION_FUNCTION(_Exhumed, MoveStatusSequence)
{
    PARAM_PROLOGUE;
    PARAM_INT(s1);
    PARAM_INT(s2);
    seq_MoveSequence(nullptr, nStatusSeqOffset + s1, s2);
    ACTION_RETURN_INT(SeqSize[nStatusSeqOffset + s1]);
}

int SizeOfStatusSequence(int s1)
{
    return SeqSize[nStatusSeqOffset + s1];
}

DEFINE_ACTION_FUNCTION_NATIVE(_Exhumed, SizeOfStatusSequence, SizeOfStatusSequence)
{
    PARAM_PROLOGUE;
    PARAM_INT(s1);
    ACTION_RETURN_INT(SeqSize[nStatusSeqOffset + s1]);
}

void UpdateFrame()
{
    auto tex = tileGetTexture(nBackgroundPic);

    twod->AddFlatFill(0, 0, xdim, windowxy1.Y - 3, tex);
    twod->AddFlatFill(0, windowxy2.Y + 4, xdim, ydim, tex);
    twod->AddFlatFill(0, windowxy1.Y - 3, windowxy1.X - 3, windowxy2.Y + 4, tex);
    twod->AddFlatFill(windowxy2.X + 4, windowxy1.Y - 3, xdim, windowxy2.Y + 4, tex);

    twod->AddFlatFill(windowxy1.X - 3, windowxy1.Y - 3, windowxy1.X, windowxy2.Y + 1, tex, 0, 1, 0xff545454);
    twod->AddFlatFill(windowxy1.X, windowxy1.Y - 3, windowxy2.X + 4, windowxy1.Y, tex, 0, 1, 0xff545454);
    twod->AddFlatFill(windowxy2.X + 1, windowxy1.Y, windowxy2.X + 4, windowxy2.Y + 4, tex, 0, 1, 0xff2a2a2a);
    twod->AddFlatFill(windowxy1.X - 3, windowxy2.Y + 1, windowxy2.X + 1, windowxy2.Y + 4, tex, 0, 1, 0xff2a2a2a);
}

void StatusMessage(int messageTime, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    VPrintf(PRINT_NOTIFY, fmt, ap);
    Printf(PRINT_NOTIFY, "\n");
    va_end(ap);
}

void DrawStatusBar()
{
    if (hud_size <= Hud_Stbar)
    {
        UpdateFrame();
    }
    SummaryInfo info{};
    info.kills = nCreaturesKilled;
    info.maxkills = nCreaturesTotal;
    // got no secrets in the game
    info.time = Scale(PlayClock, 1000, 120);
    UpdateStatusBar(&info);

    if (nSnakeCam >= 0)
    {
        const char* text = "S E R P E N T   C A M";
        int width = SmallFont->StringWidth(text);
        DrawText(twod, SmallFont, CR_UNTRANSLATED, 160 - width / 2, 1, text, DTA_FullscreenScale, FSMode_Fit320x200, TAG_DONE);
    }

}

END_PS_NS
