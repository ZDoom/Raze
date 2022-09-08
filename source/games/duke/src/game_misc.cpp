//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2020 - Christoph Oelckers

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

// This file collects several functions from the original game.c
// that do not fit any particular category.

#include "ns.h"	// Must come before everything else!

#include "automap.h"
#include "duke3d.h"
#include "m_argv.h"
#include "mapinfo.h"
#include "texturemanager.h"
#include "statusbar.h"
#include "st_start.h"
#include "i_interface.h"
#include "prediction.h"
#include "gamestate.h"
#include "dukeactor.h"
#include "interpolate.h"
#include "razefont.h"
#include "startscreen.h"

BEGIN_DUKE_NS


//---------------------------------------------------------------------------
//
// debug output
//
//---------------------------------------------------------------------------

std::pair<DVector3, DAngle> GameInterface::GetCoordinates()
{
	return std::make_pair(ps[screenpeek].pos, ps[screenpeek].angle.ang);
}

GameStats GameInterface::getStats()
{
	player_struct* p = &ps[myconnectindex];
	return { p->actors_killed, p->max_actors_killed, p->secret_rooms, p->max_secret_rooms, p->player_par / REALGAMETICSPERSEC, p->frag };
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void endthegame(bool)
{
	endoomName = isRR() ? "redneck.bin" : !isShareware() ? "duke3d.bin" : "dukesw.bin";
	ST_Endoom();
}


void GameInterface::ExitFromMenu() 
{ 
#if 0
	// do we really need this scoreboard stuff here?
	auto runbonus = [=](auto completion)
	{
	// MP scoreboard
		if (playerswhenstarted > 1 && !ud.coop) ShowScoreboard(playerswhenstarted);
	else completion(false);
	};

	auto runtwoscreens = [](auto completion)
	{
	// shareware and TEN screens
	if (isShareware() && !isRR())
			StartCutscene("DukeCutscenes.BuildSharewareExit", 0, completion);
	else completion(false);
	};

	runbonus([=](bool aborted) { runtwoscreens(endthegame); });
#else
	if (isShareware() && !isRR())
		StartCutscene("DukeCutscenes.BuildSharewareExit", 0, endthegame);
	else endthegame(false);
#endif
}

//---------------------------------------------------------------------------
//
// This now redirects the messages to the console's notification display
// which has all the features to reasonably do this in Duke style.
//
//---------------------------------------------------------------------------

void FTA(int q, player_struct* p)
{
	if (q < 0 || gamestate != GS_LEVEL)
		return;

	if (p->ftq != q || (PlayClock - p->ftt > TICRATE && q != QUOTE_DEAD))
	{
		p->ftq = q;
		auto qu = quoteMgr.GetQuote(q);
		if (p == &ps[screenpeek] && qu[0] != '\0')
		{
#if 0
			if (q >= 70 && q <= 72)
			{
				// Todo: redirect this to a centered message (these are "need a key" messages)
			}
			else
#endif
			{
				Printf(PRINT_MEDIUM | PRINT_NOTIFY, "%s\n", qu);
			}
		}
	}
	p->ftt = PlayClock;
}

//==========================================================================
//
// Draws the background
//
//==========================================================================

void GameInterface::DrawBackground()
{
	twod->ClearScreen();
	auto tex = tileGetTexture(TILE_MENUSCREEN);
	PalEntry color = 0xff808080;
	if (!hud_bgstretch)
		DrawTexture(twod, tex, 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_Color, color, TAG_DONE);
	else
		DrawTexture(twod, tex, 0, 0, DTA_VirtualWidth, twod->GetWidth(), DTA_VirtualHeight, twod->GetHeight(), DTA_KeepRatio, true, DTA_Color, color, TAG_DONE);
}

//---------------------------------------------------------------------------
//
// this is from ZDoom
//
//---------------------------------------------------------------------------

void V_AddBlend (float r, float g, float b, float a, float v_blend[4])
{
	r = clamp(r/255.f, 0.f, 0.25f);
	g = clamp(g/255.f, 0.f, 0.25f);
	b = clamp(b/255.f, 0.f, 0.25f);
	a = clamp(a/255.f, 0.f, 0.25f);


	float a2, a3;

	if (a <= 0)
		return;
	a2 = v_blend[3] + (1-v_blend[3])*a;	// new total alpha
	a3 = v_blend[3]/a2;		// fraction of color from old

	v_blend[0] = v_blend[0]*a3 + r*(1-a3);
	v_blend[1] = v_blend[1]*a3 + g*(1-a3);
	v_blend[2] = v_blend[2]*a3 + b*(1-a3);
	v_blend[3] = a2;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

 void setgamepalette(int palid)
{
	if (palid >= MAXBASEPALS || palid < 0) palid = 0;
	auto& fstint = lookups.tables[MAXPALOOKUPS - 1];
	if (palid == WATERPAL) fstint.tintColor = PalEntry(224, 192, 255);
	else if (palid == SLIMEPAL) fstint.tintColor = PalEntry(208, 255, 192);
	else fstint.tintColor = 0xffffff;
	videoSetPalette(palid);
}

 //---------------------------------------------------------------------------
 //
 // draws the weapon sprite and other 2D content that's part of the scene.
 //
 //---------------------------------------------------------------------------

 void drawweapon(double interpfrac)
 {
	 auto pp = &ps[screenpeek];
	 if (!isRR() && pp->newOwner != nullptr)
		 cameratext(pp->newOwner);
	 else
	 {
		 fi.displayweapon(screenpeek, interpfrac);
		 if (pp->over_shoulder_on == 0)
			 fi.displaymasks(screenpeek, pp->GetActor()->spr.pal == 1 || !pp->insector() ? 1 : pp->cursector->floorpal, interpfrac);
	 }

 }

//---------------------------------------------------------------------------
//
// draws everything not part of the 3D scene and its weapon sprite.
//
//---------------------------------------------------------------------------

void drawoverlays(double interpfrac)
{
	player_struct* pp;
	DVector2 cposxy;
	DAngle cang;

	pp = &ps[screenpeek];
	// set palette here, in case the 3D view is off.
	setgamepalette(setpal(pp));

	float blend[4] = {};

	// this does pain tinting etc from the CON
	V_AddBlend(pp->pals.r, pp->pals.g, pp->pals.b, pp->pals.a, blend);
	// loogies courtesy of being snotted on
	if (pp->loogcnt > 0 && !isRR())
	{
		V_AddBlend(0, 63, 0, float(pp->loogcnt >> 1), blend);
	}
	if (blend[3])
	{
		// result must be multiplied by 4 and normalised to 255. (4*255 = 1020)
		auto comp = [&](int i, int maxv=255) { return clamp(int(blend[i] * 1020), 0, maxv); };
		videoFadePalette(comp(0), comp(1), comp(2), comp(3, 192)); // Never fully saturate the alpha channel
	}
	else
		videoclearFade();

	MarkSectorSeen(pp->cursector);

	if (ud.cameraactor == nullptr)
	{
		if (automapMode != am_off)
		{
			DoInterpolations(interpfrac);

			if (pp->newOwner == nullptr && playrunning())
			{
				if (screenpeek == myconnectindex && numplayers > 1)
				{
					cposxy = interpolatedvalue(omypos, mypos, interpfrac).XY();
					cang = !SyncInput() ? myang : interpolatedvalue(omyang, myang, interpfrac);
				}
				else
				{
					cposxy = interpolatedvalue(pp->opos, pp->pos, interpfrac).XY();
					cang = !SyncInput() ? pp->angle.ang : interpolatedvalue(pp->angle.oang, pp->angle.ang, interpfrac);
				}
			}
			else
			{
				cposxy = pp->opos.XY();
				cang = pp->angle.oang;
			}
			DrawOverheadMap(cposxy, cang, interpfrac);
			RestoreInterpolations();
		}
	}

	DrawStatusBar();

	if (ps[myconnectindex].newOwner == nullptr && ud.cameraactor == nullptr)
	{
		DrawCrosshair(TILE_CROSSHAIR, ps[screenpeek].last_extra, -pp->angle.look_anghalf(interpfrac), pp->over_shoulder_on ? 2.5 : 0, isRR() ? 0.5 : 1);
	}

	if (paused == 2)
	{
		double x = 160, y = 100;
		double scale = isRR() ? 0.4 : 1.;
		const char* text = GStrings("Game Paused");
		auto myfont = PickBigFont(text);
		x -= myfont->StringWidth(text) * 0.5 * scale;
		DrawText(twod, myfont, CR_UNTRANSLATED, x, y - 12, text, DTA_FullscreenScale, FSMode_Fit320x200, DTA_ScaleX, scale, DTA_ScaleY, scale, TAG_DONE);
	}
}




//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void cameratext(DDukeActor *cam)
{
	auto drawitem = [=](int tile, double x, double y, bool flipx, bool flipy)
	{
		DrawTexture(twod, tileGetTexture(tile), x, y, DTA_ViewportX, viewport3d.Left(), DTA_ViewportY, viewport3d.Top(), DTA_ViewportWidth, viewport3d.Width(), 
			DTA_ViewportHeight, viewport3d.Height(), DTA_FlipX, flipx, DTA_FlipY, flipy, DTA_CenterOffsetRel, 2, DTA_FullscreenScale, FSMode_Fit320x200, TAG_DONE);
	};
	if (!cam->temp_data[0])
	{
		drawitem(TILE_CAMCORNER, 24, 33, false, false);
		drawitem(TILE_CAMCORNER + 1, 320 - 26, 33, false, false);
		drawitem(TILE_CAMCORNER + 1, 24, 163, true, true);
		drawitem(TILE_CAMCORNER + 1, 320 - 26, 163, false, true);

		if (PlayClock & 16)
			drawitem(TILE_CAMLIGHT, 46, 32, false, false);
	}
	else
	{
		for (int x = -64; x < 394; x += 64)
			for (int y = 0; y < 200; y += 64)
				drawitem(TILE_STATIC, x, y, !!(PlayClock & 8), !!(PlayClock & 16));
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int startrts(int lumpNum, int localPlayer)
{
	if (SoundEnabled() &&
		RTS_IsInitialized() && rtsplaying == 0 && (snd_speech & (localPlayer ? 1 : 4)))
	{
		auto sid = RTS_GetSoundID(lumpNum - 1);
		if (sid != -1)
		{
			S_PlaySound(sid, CHAN_AUTO, CHANF_UI);
			rtsplaying = 7;
			return 1;
		}
	}

	return 0;
}

ReservedSpace GameInterface::GetReservedScreenSpace(int viewsize)
{
	// todo: factor in the frag bar: tileHeight(TILE_FRAGBAR)
	int sbar = tileHeight(TILE_BOTTOMSTATUSBAR);
	if (isRR())
	{
		sbar >>= 1;
	}
	return { 0, sbar };
}

::GameInterface* CreateInterface()
{
	return new GameInterface;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool GameInterface::DrawAutomapPlayer(const DVector2& mxy, const DVector2& cpos, const DAngle cang, const DVector2& xydim, const double czoom, double const interpfrac)
{
	// Pre-caculate incoming angle vector.
	auto cangvect = cang.ToVector();

	// Draw sprites
	if (gFullMap)
	{
		for (unsigned ii = 0; ii < sector.Size(); ii++)
		{
			if (show2dsector[ii]) continue;
			DukeSectIterator it(ii);
			while (auto act = it.Next())
			{
				if (act == ps[screenpeek].actor || (act->spr.cstat & CSTAT_SPRITE_INVISIBLE) || act->spr.cstat == CSTAT_SPRITE_BLOCK_ALL || act->spr.xrepeat == 0) continue;

				if ((act->spr.cstat & CSTAT_SPRITE_BLOCK_ALL) != 0)
				{
					PalEntry col = act->spr.cstat & CSTAT_SPRITE_BLOCK ? PalEntry(170, 0, 170) : PalEntry(0, 170, 170);
					auto sprpos = act->spr.pos.XY() - cpos;

					switch (act->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK)
					{
					case CSTAT_SPRITE_ALIGNMENT_FACING:
						DrawAutomapAlignmentFacing(act->spr, sprpos, cangvect, czoom, xydim, col);
						break;

					case CSTAT_SPRITE_ALIGNMENT_WALL:
						if (actorflag(act, SFLAG2_SHOWWALLSPRITEONMAP)) DrawAutomapAlignmentWall(act->spr, sprpos, cangvect, czoom, xydim, col);
						break;

					case CSTAT_SPRITE_ALIGNMENT_FLOOR:
					case CSTAT_SPRITE_ALIGNMENT_SLOPE:
						DrawAutomapAlignmentFloor(act->spr, sprpos, cangvect, czoom, xydim, col);
						break;
					}
				}
			}
		}
	}

	for (int p = connecthead; p >= 0; p = connectpoint2[p])
	{
		if (p == screenpeek || ud.coop == 1)
		{
			auto& pp = ps[p];
			auto act = pp.GetActor();
			int i = TILE_APLAYERTOP + (act->int_xvel() > 16 && pp.on_ground ? (PlayClock >> 4) & 3 : 0);
			double j = clamp(czoom * act->spr.yrepeat + abs(pp.truefz - pp.pos.Z), 21.5, 128.) * REPEAT_SCALE;

			auto const vec = OutAutomapVector(mxy - cpos, cangvect, czoom, xydim);
			auto const daang = -((!SyncInput() ? act->spr.angle : act->interpolatedangle(interpfrac)) - cang).Normalized360().Degrees();

			DrawTexture(twod, tileGetTexture(i), vec.X, vec.Y, DTA_TranslationIndex, TRANSLATION(Translation_Remap + setpal(&pp), act->spr.pal), DTA_CenterOffset, true,
				DTA_Rotate, daang, DTA_Color, shadeToLight(act->spr.shade), DTA_ScaleX, j, DTA_ScaleY, j, TAG_DONE);
		}
	}
	return true;
}




END_DUKE_NS

