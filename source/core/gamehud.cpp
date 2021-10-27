/*
** gamehud.cpp
**
** Management of HUD elements
**
**---------------------------------------------------------------------------
** Copyright 2019-2021 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include <memory>
#include <assert.h>
#include "gamehud.h"
#include "textures.h"
#include "palette.h"
#include "gamecontrol.h"
#include "v_2ddrawer.h"
#include "v_video.h"
#include "build.h"
#include "v_draw.h"
#include "v_font.h"
#include "gamestruct.h"
#include "gamefuncs.h"

F2DDrawer twodpsp;


void hud_drawsprite(double sx, double sy, double sz, double a, int picnum, int dashade, int dapalnum, int dastat, double alpha)
{
	sz *= 1. / 65536.;
	alpha *= (dastat & RS_TRANS1)? glblend[0].def[!!(dastat & RS_TRANS2)].alpha : 1.;
	int palid = TRANSLATION(Translation_Remap + curbasepal, dapalnum);

	if (picanm[picnum].sf & PICANM_ANIMTYPE_MASK)
		picnum += animateoffs(picnum, 0);

	auto tex = tileGetTexture(picnum);

	DrawTexture(&twodpsp, tex, sx, sy,
		DTA_ScaleX, sz, DTA_ScaleY, sz,
		DTA_Color, shadeToLight(dashade),
		DTA_TranslationIndex, palid,
		DTA_ViewportX, windowxy1.x, DTA_ViewportY, windowxy1.y,
		DTA_ViewportWidth, windowxy2.x - windowxy1.x + 1, DTA_ViewportHeight, windowxy2.y - windowxy1.y + 1,
		DTA_FullscreenScale, (dastat & RS_STRETCH)? FSMode_ScaleToScreen: FSMode_ScaleToHeight, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200,
		DTA_CenterOffsetRel, !(dastat & (RS_TOPLEFT | RS_CENTER)),
		DTA_TopLeft, !!(dastat & RS_TOPLEFT),
		DTA_CenterOffset, !!(dastat & RS_CENTER),
		DTA_FlipX, !!(dastat & RS_XFLIPHUD),
		DTA_FlipY, !!(dastat & RS_YFLIPHUD),
		DTA_Pin, (dastat & RS_ALIGN_R) ? 1 : (dastat & RS_ALIGN_L) ? -1 : 0,
		DTA_Rotate, a * -BAngToDegree,
		DTA_FlipOffsets, !(dastat & (/*RS_TOPLEFT |*/ RS_CENTER)),
		DTA_Alpha, alpha,
		TAG_DONE);
}


//==========================================================================
//
// DFrameBuffer :: DrawRateStuff
//
// Draws the fps counter, dot ticker, and palette debug.
//
//==========================================================================
CVAR(Bool, vid_fps, false, 0)


static FString statFPS()
{
	static int32_t frameCount;
	static double lastFrameTime;
	static double cumulativeFrameDelay;
	static double lastFPS;

	FString output;

	double frameTime = I_msTimeF();
	double frameDelay = frameTime - lastFrameTime;
	cumulativeFrameDelay += frameDelay;

	frameCount++;
	if (frameDelay >= 0)
	{
		output.AppendFormat("%5.1f fps", lastFPS);
		if (frameDelay < 10) output.AppendFormat(" ");
		output.AppendFormat(" (%.1f ms)\n", frameDelay);

		if (cumulativeFrameDelay >= 1000.0)
		{
			lastFPS = 1000. * frameCount / cumulativeFrameDelay;
			frameCount = 0;
			cumulativeFrameDelay = 0.0;
		}
	}
	lastFrameTime = frameTime;
	return output;
}

void DrawRateStuff()
{
	// Draws frame time and cumulative fps
	if (vid_fps)
	{
		FString fpsbuff = statFPS();

		int textScale = active_con_scale(twod);
		int rate_x = screen->GetWidth() / textScale - NewConsoleFont->StringWidth(&fpsbuff[0]);
		twod->AddColorOnlyQuad(rate_x * textScale, 0, screen->GetWidth(), NewConsoleFont->GetHeight() * textScale, MAKEARGB(255, 0, 0, 0));
		DrawText(twod, NewConsoleFont, CR_WHITE, rate_x, 0, (char*)&fpsbuff[0],
			DTA_VirtualWidth, screen->GetWidth() / textScale,
			DTA_VirtualHeight, screen->GetHeight() / textScale,
			DTA_KeepRatio, true, TAG_DONE);

	}
}

