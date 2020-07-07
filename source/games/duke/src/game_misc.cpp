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

#include "duke3d.h"
#include "baselayer.h"
#include "m_argv.h"
#include "mapinfo.h"
#include "texturemanager.h"
#include "statusbar.h"
#include "st_start.h"
#include "i_interface.h"
#include "prediction.h"

BEGIN_DUKE_NS

FFont* IndexFont;
FFont* DigiFont;
int rtsplaying;

//---------------------------------------------------------------------------
//
// debug output
//
//---------------------------------------------------------------------------

FString GameInterface::GetCoordString()
{
	int snum = screenpeek;
	FString out;

	out.Format("pos= %d, %d, %d - angle = %2.3f - sector = %d, lotag = %d, hitag = %d",
		ps[snum].posx, ps[snum].posy, ps[snum].posz, ps[snum].q16ang / 65536., ps[snum].cursectnum,
		sector[ps[snum].cursectnum].lotag, sector[ps[snum].cursectnum].hitag);

	return out;
}


GameStats GameInterface::getStats()
{
	struct player_struct* p = &ps[myconnectindex];
	return { p->actors_killed, p->max_actors_killed, p->secret_rooms, p->max_secret_rooms, p->player_par / REALGAMETICSPERSEC, p->frag };
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

FString GameInterface::statFPS()
{
	FString output;

	output.AppendFormat("Actor think time: %.3f ms\n", actortime.TimeMS());
	output.AppendFormat("Total think time: %.3f ms\n", thinktime.TimeMS());
	output.AppendFormat("Game Update: %.3f ms\n", gameupdatetime.TimeMS());
	output.AppendFormat("Draw time: %.3f ms\n", drawtime.TimeMS());

	return output;
}


//---------------------------------------------------------------------------
//
// game specific command line args go here. 
//
//---------------------------------------------------------------------------

void checkcommandline()
{
	auto val = Args->CheckValue("-skill");
	if (!val) val = Args->CheckValue("-s");
	if (val)
	{
		ud.m_player_skill = ud.player_skill = clamp((int)strtol(val, nullptr, 0), 0, 5);
		if (ud.m_player_skill == 4) ud.m_respawn_monsters = ud.respawn_monsters = 1;
	}
	val = Args->CheckValue("-respawn");
	if (!val) val = Args->CheckValue("-t");
	if (val)
	{
		if (*val == '1') ud.m_respawn_monsters = 1;
		else if (*val == '2') ud.m_respawn_items = 1;
		else if (*val == '3') ud.m_respawn_inventory = 1;
		else
		{
			ud.m_respawn_monsters = 1;
			ud.m_respawn_items = 1;
			ud.m_respawn_inventory = 1;
		}
		Printf("Respawn on.\n");
	}
}

//---------------------------------------------------------------------------
//
// fixme: Menu does not call this
//
//---------------------------------------------------------------------------

void gameexitfrommenu()
{
	// MP scoreboard
	if (playerswhenstarted > 1 && g_player[myconnectindex].ps->gm & MODE_GAME && !ud.coop)
	{
		dobonus(1);
	}

	// shareware and TEN screens
	if (!VOLUMEALL && !isRR())
		showtwoscreens([](bool) {});

	endoomName = isRR() ? "redneck.bin" : VOLUMEALL ? "duke3d.bin" : "dukesw.bin";
	ST_Endoom();
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void genspriteremaps(void)
{
	int j;

	auto fr = fileSystem.OpenFileReader("lookup.dat");
	if (!fr.isOpen())
		   return;

	j = lookups.loadTable(fr);

	if (j < 0)
	{
		if (j == -1)
			Printf("ERROR loading \"lookup.dat\": failed reading enough data.\n");

		return;
	}

	uint8_t paldata[768];

	for (j=1; j<=5; j++)
	{
		if (fr.Read(paldata, 768) != 768)
			return;

		for (int k = 0; k < 768; k++) // Build uses 6 bit VGA palettes.
			paldata[k] = (paldata[k] << 2) | (paldata[k] >> 6);

		paletteSetColorTable(j, paldata, j == DREALMSPAL || j == ENDINGPAL, j < DREALMSPAL);
	}

	for (int i = 0; i < 256; i++)
	{
		// swap red and blue channels.
		paldata[i * 3] = GPalette.BaseColors[i].b;
		paldata[i * 3+1] = GPalette.BaseColors[i].g;
		paldata[i * 3+2] = GPalette.BaseColors[i].r;
	}
	paletteSetColorTable(DRUGPAL, paldata, false, false); // todo: implement this as a shader effect (swap R and B in postprocessing.)

	if (isRR())
	{
		uint8_t table[256];
		for (j = 0; j < 256; j++)
			table[j] = j;
		for (j = 0; j < 32; j++)
			table[j] = j + 32;

		lookups.makeTable(7, table, 0, 0, 0, 0);

		for (j = 0; j < 256; j++)
			table[j] = j;
		lookups.makeTable(30, table, 0, 0, 0, 0);
		lookups.makeTable(31, table, 0, 0, 0, 0);
		lookups.makeTable(32, table, 0, 0, 0, 0);
		lookups.makeTable(33, table, 0, 0, 0, 0);
		if (isRRRA())
			lookups.makeTable(105, table, 0, 0, 0, 0);

		int unk = 63;
		for (j = 64; j < 80; j++)
		{
			unk--;
			table[j] = unk;
			table[j + 16] = j - 24;
		}
		table[80] = 80;
		table[81] = 81;
		for (j = 0; j < 32; j++)
		{
			table[j] = j + 32;
		}
		lookups.makeTable(34, table, 0, 0, 0, 0);
		for (j = 0; j < 256; j++)
			table[j] = j;
		for (j = 0; j < 16; j++)
			table[j] = j + 129;
		for (j = 16; j < 32; j++)
			table[j] = j + 192;
		lookups.makeTable(35, table, 0, 0, 0, 0);
		if (isRRRA())
		{
			lookups.makeTable(50, nullptr, 12 * 4, 12 * 4, 12 * 4, 0);
			lookups.makeTable(51, nullptr, 12 * 4, 12 * 4, 12 * 4, 0);
			lookups.makeTable(54, lookups.getTable(8), 32 * 4, 32 * 4, 32 * 4, 0);
		}
	}
}

//---------------------------------------------------------------------------
//
// This now redirects the messagew to the console's notification display
// which has all the features to reasonably do this in Duke style.
//
//---------------------------------------------------------------------------

void FTA(int q, struct player_struct* p)
{
	if (hud_messages == 0 || q < 0 || !(p->gm & MODE_GAME))
		return;

	if (p->ftq != q)
	{
		if (q == 13) p->ftq = q;
		auto qu = quoteMgr.GetQuote(q);
		if (p == g_player[screenpeek].ps && qu[0] != '\0')
		{
			if (q >= 70 && q <= 72 && hud_messages == 2)
			{
				// Todo: redirect this to a centered message (these are "need a key" messages)
			}
			else
			{
				int printlevel = hud_messages == 1 ? PRINT_MEDIUM : PRINT_MEDIUM | PRINT_NOTIFY;
				Printf(printlevel, "%s\n", qu);
			}
		}
	}
}

//==========================================================================
//
// Draws the background
// todo: split up to have dedicated functions for both cases.
//
//==========================================================================

void drawbackground(void)
{
	if ((g_player[myconnectindex].ps->gm & MODE_GAME) == 0 && ud.recstat != 2)
	{
		twod->ClearScreen();
		auto tex = tileGetTexture(TILE_MENUSCREEN);
		PalEntry color = 0xff808080;
		if (!hud_bgstretch)
			DrawTexture(twod, tex, 0, 0, DTA_FullscreenEx, 3, DTA_Color, color, TAG_DONE);
		else
			DrawTexture(twod, tex, 0, 0, DTA_VirtualWidth, twod->GetWidth(), DTA_VirtualHeight, twod->GetHeight(), DTA_KeepRatio, true, DTA_Color, color, TAG_DONE);
		return;
	}
}

//---------------------------------------------------------------------------
//
// this is from ZDoom
//
//---------------------------------------------------------------------------

void V_AddBlend (float r, float g, float b, float a, float v_blend[4])
{
	r = clamp(r/255.f, 0.f, 0.25f);
	g = clamp(r/255.f, 0.f, 0.25f);
	b = clamp(r/255.f, 0.f, 0.25f);
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
// 'rest' in this case means everything not part of the 3D scene and its weapon sprite.
//
//---------------------------------------------------------------------------

void displayrest(int smoothratio)
{
	int i, j;
	unsigned char fader = 0, fadeg = 0, fadeb = 0, fadef = 0, tintr = 0, tintg = 0, tintb = 0, tintf = 0, dotint = 0;

	struct player_struct* pp;
	walltype* wal;
	int cposx, cposy, cang;

	pp = &ps[screenpeek];

	float blend[4] = {};

	// this does pain tinting etc from the CON
	V_AddBlend(pp->pals.r, pp->pals.g, pp->pals.b, pp->pals.a, blend);
	// loogies courtesy of being snotted on
	if (pp->loogcnt > 0)
	{
		V_AddBlend(0, 63, 0, (pp->loogcnt >> 1), blend);
	}
	if (blend[3])
	{
		// result must be multiplied by 4 and normalised to 255. (4*255 = 1020)
		auto comp = [&](int i) { return clamp(int(blend[i] * 1020), 0, 255); };
		videoFadePalette(comp(0), comp(1), comp(2), comp(3)); 
	}
	else
		videoclearFade();

	i = pp->cursectnum;

	if (i >= 0) show2dsector.Set(i);
	wal = &wall[sector[i].wallptr];
	for (j = sector[i].wallnum; j > 0; j--, wal++)
	{
		i = wal->nextsector;
		if (i < 0) continue;
		if (wal->cstat & 0x0071) continue;
		if (wall[wal->nextwall].cstat & 0x0071) continue;
		if (sector[i].lotag == 32767) continue;
		if (sector[i].ceilingz >= sector[i].floorz) continue;
		show2dsector.Set(i);
	}

	if (ud.camerasprite == -1)
	{
		if (ud.overhead_on != 2)
		{
			if (!isRR() && pp->newowner >= 0)
				cameratext(pp->newowner);
			else
			{
				fi.displayweapon(screenpeek);
				if (pp->over_shoulder_on == 0)
					fi.displaymasks(screenpeek);
			}
			if (!isRR())
				moveclouds();
		}

		if (ud.overhead_on > 0)
		{
			// smoothratio = min(max(smoothratio,0),65536);
			smoothratio = calc_smoothratio(totalclock, ototalclock);
			dointerpolations(smoothratio);

			if (ud.scrollmode == 0)
			{
				if (pp->newowner == -1 && !ud.pause_on)
				{
					if (screenpeek == myconnectindex && numplayers > 1)
					{
						cposx = omyx + mulscale16(myx - omyx, smoothratio);
						cposy = omyy + mulscale16(myy - omyy, smoothratio);
						cang = fix16_to_int(omyang) + mulscale16((fix16_to_int(myang + F16(1024) - omyang) & 2047) - 1024, smoothratio);
					}
					else
					{
						cposx = pp->oposx + mulscale16(pp->posx - pp->oposx, smoothratio);
						cposy = pp->oposy + mulscale16(pp->posy - pp->oposy, smoothratio);
						cang = pp->getang();// + mulscale16((fix16_to_int(pp->q16ang+F16(1024)-pp->oq16ang)&2047)-1024, smoothratio);
					}
				}
				else
				{
					cposx = pp->oposx;
					cposy = pp->oposy;
					cang = fix16_to_int(pp->oq16ang);
				}
			}
			else
			{
				if (!ud.pause_on)
				{
					ud.fola += ud.folavel >> 3;
					ud.folx += (ud.folfvel * sintable[(512 + 2048 - ud.fola) & 2047]) >> 14;
					ud.foly += (ud.folfvel * sintable[(512 + 1024 - 512 - ud.fola) & 2047]) >> 14;
				}
				cposx = ud.folx;
				cposy = ud.foly;
				cang = ud.fola;
			}

			if (ud.overhead_on == 2)
			{
				twod->ClearScreen();
				renderDrawMapView(cposx, cposy, pp->zoom, cang);
			}
			drawoverheadmap(cposx, cposy, pp->zoom, cang);

			restoreinterpolations();
		}
	}

	if (isRR()) drawstatusbar_r(screenpeek);
	else drawstatusbar_d(screenpeek);

	if (ps[myconnectindex].newowner == -1 && ud.overhead_on == 0 && ud.crosshair && ud.camerasprite == -1)
	{
		int32_t a = TILE_CROSSHAIR;

		if ((unsigned)a < MAXTILES)
		{
			vec2_t crosshairpos = {  };
			//vec2_t crosshairpos = { ud.returnvar[0], ud.returnvar[1] };
			uint32_t crosshair_o = 1 | 2;
			double crosshair_scale = cl_crosshairscale * .001;
			if (isRR()) crosshair_scale *= .5;

			DrawTexture(twod, tileGetTexture(a), 160 - (ps[myconnectindex].look_ang >> 1), 100,
				DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_ScaleX, crosshair_scale, DTA_ScaleY, crosshair_scale,
				DTA_ViewportX, windowxy1.x, DTA_ViewportY, windowxy1.y, DTA_ViewportWidth, windowxy2.x - windowxy1.x, DTA_ViewportY, windowxy2.y - windowxy1.y, TAG_DONE);
		}
	}

	if (ud.pause_on == 1 && (ps[myconnectindex].gm & MODE_MENU) == 0)
		fi.PrintPaused();
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void drawoverheadmap(int cposx, int cposy, int czoom, int cang)
{
	int i, j, k, l, x1, y1, x2, y2, x3, y3, x4, y4, ox, oy, xoff, yoff;
	int dax, day, cosang, sinang, xspan, yspan, sprx, spry;
	int xrepeat, yrepeat, z1, z2, startwall, endwall, tilenum, daang;
	int xvect, yvect, xvect2, yvect2;
	int p;
	PalEntry col;
	walltype* wal, * wal2;
	spritetype* spr;

	renderSetAspect(65536, 65536);

	xvect = sintable[(-cang) & 2047] * czoom;
	yvect = sintable[(1536 - cang) & 2047] * czoom;
	xvect2 = mulscale16(xvect, yxaspect);
	yvect2 = mulscale16(yvect, yxaspect);

	//Draw red lines
	for (i = 0; i < numsectors; i++)
	{
		if (!gFullMap && !show2dsector[i]) continue;

		startwall = sector[i].wallptr;
		endwall = sector[i].wallptr + sector[i].wallnum;

		z1 = sector[i].ceilingz;
		z2 = sector[i].floorz;

		for (j = startwall, wal = &wall[startwall]; j < endwall; j++, wal++)
		{
			k = wal->nextwall;
			if (k < 0) continue;

			if (sector[wal->nextsector].ceilingz == z1 && sector[wal->nextsector].floorz == z2)
				if (((wal->cstat | wall[wal->nextwall].cstat) & (16 + 32)) == 0) continue;

			if (!gFullMap && !show2dsector[wal->nextsector])
			{
				col = PalEntry(170, 170, 170);
				ox = wal->x - cposx;
				oy = wal->y - cposy;
				x1 = dmulscale16(ox, xvect, -oy, yvect) + (xdim << 11);
				y1 = dmulscale16(oy, xvect2, ox, yvect2) + (ydim << 11);

				wal2 = &wall[wal->point2];
				ox = wal2->x - cposx;
				oy = wal2->y - cposy;
				x2 = dmulscale16(ox, xvect, -oy, yvect) + (xdim << 11);
				y2 = dmulscale16(oy, xvect2, ox, yvect2) + (ydim << 11);

				drawlinergb(x1, y1, x2, y2, col);
			}
		}
	}

	//Draw sprites
	k = ps[screenpeek].i;
	for (i = 0; i < numsectors; i++)
	{
		if (!gFullMap || !show2dsector[i]) continue;
		for (j = headspritesect[i]; j >= 0; j = nextspritesect[j])
		{
			spr = &sprite[j];

			if (j == k || (spr->cstat & 0x8000) || spr->cstat == 257 || spr->xrepeat == 0) continue;

			col = PalEntry(0, 170, 170);
			if (spr->cstat & 1) col = PalEntry(170, 0, 170);

			sprx = spr->x;
			spry = spr->y;

			if ((spr->cstat & 257) != 0) switch (spr->cstat & 48)
			{
			case 0:
				//                    break;

				ox = sprx - cposx;
				oy = spry - cposy;
				x1 = dmulscale16(ox, xvect, -oy, yvect);
				y1 = dmulscale16(oy, xvect2, ox, yvect2);

				ox = (sintable[(spr->ang + 512) & 2047] >> 7);
				oy = (sintable[(spr->ang) & 2047] >> 7);
				x2 = dmulscale16(ox, xvect, -oy, yvect);
				y2 = dmulscale16(oy, xvect, ox, yvect);

				x3 = mulscale16(x2, yxaspect);
				y3 = mulscale16(y2, yxaspect);

				drawlinergb(x1 - x2 + (xdim << 11), y1 - y3 + (ydim << 11),
					x1 + x2 + (xdim << 11), y1 + y3 + (ydim << 11), col);
				drawlinergb(x1 - y2 + (xdim << 11), y1 + x3 + (ydim << 11),
					x1 + x2 + (xdim << 11), y1 + y3 + (ydim << 11), col);
				drawlinergb(x1 + y2 + (xdim << 11), y1 - x3 + (ydim << 11),
					x1 + x2 + (xdim << 11), y1 + y3 + (ydim << 11), col);
				break;

			case 16:
				if (spr->picnum == TILE_LASERLINE)
				{
					x1 = sprx;
					y1 = spry;
					tilenum = spr->picnum;
					xoff = tileLeftOffset(tilenum) + spr->xoffset;
					if ((spr->cstat & 4) > 0) xoff = -xoff;
					k = spr->ang;
					l = spr->xrepeat;
					dax = sintable[k & 2047] * l;
					day = sintable[(k + 1536) & 2047] * l;
					l = tilesiz[tilenum].x;
					k = (l >> 1) + xoff;
					x1 -= mulscale16(dax, k);
					x2 = x1 + mulscale16(dax, l);
					y1 -= mulscale16(day, k);
					y2 = y1 + mulscale16(day, l);

					ox = x1 - cposx;
					oy = y1 - cposy;
					x1 = dmulscale16(ox, xvect, -oy, yvect);
					y1 = dmulscale16(oy, xvect2, ox, yvect2);

					ox = x2 - cposx;
					oy = y2 - cposy;
					x2 = dmulscale16(ox, xvect, -oy, yvect);
					y2 = dmulscale16(oy, xvect2, ox, yvect2);

					drawlinergb(x1 + (xdim << 11), y1 + (ydim << 11),
						x2 + (xdim << 11), y2 + (ydim << 11), col);
				}

				break;

			case 32:
				tilenum = spr->picnum;
				xoff = tileLeftOffset(tilenum) + spr->xoffset;
				yoff = tileTopOffset(tilenum) + spr->yoffset;
				if ((spr->cstat & 4) > 0) xoff = -xoff;
				if ((spr->cstat & 8) > 0) yoff = -yoff;

				k = spr->ang;
				cosang = sintable[(k + 512) & 2047];
				sinang = sintable[k & 2047];
				xspan = tilesiz[tilenum].x;
				xrepeat = spr->xrepeat;
				yspan = tilesiz[tilenum].y;
				yrepeat = spr->yrepeat;

				dax = ((xspan >> 1) + xoff) * xrepeat;
				day = ((yspan >> 1) + yoff) * yrepeat;
				x1 = sprx + dmulscale16(sinang, dax, cosang, day);
				y1 = spry + dmulscale16(sinang, day, -cosang, dax);
				l = xspan * xrepeat;
				x2 = x1 - mulscale16(sinang, l);
				y2 = y1 + mulscale16(cosang, l);
				l = yspan * yrepeat;
				k = -mulscale16(cosang, l);
				x3 = x2 + k;
				x4 = x1 + k;
				k = -mulscale16(sinang, l);
				y3 = y2 + k;
				y4 = y1 + k;

				ox = x1 - cposx;
				oy = y1 - cposy;
				x1 = dmulscale16(ox, xvect, -oy, yvect);
				y1 = dmulscale16(oy, xvect2, ox, yvect2);

				ox = x2 - cposx;
				oy = y2 - cposy;
				x2 = dmulscale16(ox, xvect, -oy, yvect);
				y2 = dmulscale16(oy, xvect2, ox, yvect2);

				ox = x3 - cposx;
				oy = y3 - cposy;
				x3 = dmulscale16(ox, xvect, -oy, yvect);
				y3 = dmulscale16(oy, xvect2, ox, yvect2);

				ox = x4 - cposx;
				oy = y4 - cposy;
				x4 = dmulscale16(ox, xvect, -oy, yvect);
				y4 = dmulscale16(oy, xvect2, ox, yvect2);

				drawlinergb(x1 + (xdim << 11), y1 + (ydim << 11),
					x2 + (xdim << 11), y2 + (ydim << 11), col);

				drawlinergb(x2 + (xdim << 11), y2 + (ydim << 11),
					x3 + (xdim << 11), y3 + (ydim << 11), col);

				drawlinergb(x3 + (xdim << 11), y3 + (ydim << 11),
					x4 + (xdim << 11), y4 + (ydim << 11), col);

				drawlinergb(x4 + (xdim << 11), y4 + (ydim << 11),
					x1 + (xdim << 11), y1 + (ydim << 11), col);

				break;
			}
		}
	}

	//Draw white lines
	for (i = numsectors - 1; i >= 0; i--)
	{
		if (!gFullMap && !show2dsector[i]) continue;

		startwall = sector[i].wallptr;
		endwall = sector[i].wallptr + sector[i].wallnum;

		k = -1;
		for (j = startwall, wal = &wall[startwall]; j < endwall; j++, wal++)
		{
			if (wal->nextwall >= 0) continue;

			if (tilesiz[wal->picnum].x == 0) continue;
			if (tilesiz[wal->picnum].y == 0) continue;

			if (j == k)
			{
				x1 = x2;
				y1 = y2;
			}
			else
			{
				ox = wal->x - cposx;
				oy = wal->y - cposy;
				x1 = dmulscale16(ox, xvect, -oy, yvect) + (xdim << 11);
				y1 = dmulscale16(oy, xvect2, ox, yvect2) + (ydim << 11);
			}

			k = wal->point2;
			wal2 = &wall[k];
			ox = wal2->x - cposx;
			oy = wal2->y - cposy;
			x2 = dmulscale16(ox, xvect, -oy, yvect) + (xdim << 11);
			y2 = dmulscale16(oy, xvect2, ox, yvect2) + (ydim << 11);

			drawlinergb(x1, y1, x2, y2, PalEntry(170, 170, 170));
		}
	}

	videoSetCorrectedAspect();

	for (p = connecthead; p >= 0; p = connectpoint2[p])
	{
		if (ud.scrollmode && p == screenpeek) continue;

		ox = sprite[ps[p].i].x - cposx;
		oy = sprite[ps[p].i].y - cposy;
		daang = (sprite[ps[p].i].ang - cang) & 2047;
		if (p == screenpeek)
		{
			ox = 0;
			oy = 0;
			daang = 0;
		}
		x1 = mulscale(ox, xvect, 16) - mulscale(oy, yvect, 16);
		y1 = mulscale(oy, xvect2, 16) + mulscale(ox, yvect2, 16);

		if (p == screenpeek || ud.coop == 1)
		{
			auto& pp = ps[p];
			if (sprite[pp.i].xvel > 16 && pp.on_ground)
				i = TILE_APLAYERTOP + (((int)totalclock >> 4) & 3);
			else
				i = TILE_APLAYERTOP;

			j = klabs(pp.truefz - pp.posz) >> 8;
			j = mulscale(czoom * (sprite[pp.i].yrepeat + j), yxaspect, 16);

			if (j < 22000) j = 22000;
			else if (j > (65536 << 1)) j = (65536 << 1);

			int light = Scale(numshades - sprite[pp.i].shade, 255, numshades);
			PalEntry pe(255, light, light, light);
			DrawTexture(twod, tileGetTexture(i), xdim / 2. + x1 / 4096., ydim / 2. + y1 / 4096., DTA_TranslationIndex, TRANSLATION(Translation_Remap + pp.palette, sprite[pp.i].pal),
				DTA_Color, pe, DTA_ScaleX, j / 65536., DTA_ScaleY, j/65536., TAG_DONE);
		}
	}

	if (/*textret == 0 &&*/ ud.overhead_on == 2)
	{
		double scale = isRR() ? 0.5 : 1.;
		int top = isRR() ? 0 : ((ud.screen_size > 0) ? 147 : 179);
		if (!G_HaveUserMap())
		DrawText(twod, SmallFont2, CR_UNDEFINED, 5, top+6, GStrings.localize(gVolumeNames[ud.volume_number]), 
			DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_ScaleX, scale, DTA_ScaleY, scale, DTA_KeepRatio, true, TAG_DONE);
		DrawText(twod, SmallFont2, CR_UNDEFINED, 5, top + 12, currentLevel->DisplayName(),
			DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, DTA_ScaleX, scale, DTA_ScaleY, scale, DTA_KeepRatio, true, TAG_DONE);
	}

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void cameratext(int i)
{
	auto drawitem = [=](int tile, double x, double y, bool flipx, bool flipy)
	{
		DrawTexture(twod, tileGetTexture(tile), x, y, DTA_ViewportX, windowxy1.x, DTA_ViewportY, windowxy1.y, DTA_ViewportWidth, windowxy2.x - windowxy1.x + 1, DTA_CenterOffset, true,
			DTA_ViewportHeight, windowxy2.y - windowxy1.y + 1, DTA_FlipX, flipx, DTA_FlipY, flipy, DTA_FullscreenScale, 3, DTA_VirtualWidth, 320, DTA_VirtualHeight, 200, TAG_DONE);
	};
	if (!hittype[i].temp_data[0])
	{
		drawitem(TILE_CAMCORNER, 24, 33, false, false);
		drawitem(TILE_CAMCORNER + 1, 320 - 26, 33, false, false);
		drawitem(TILE_CAMCORNER + 1, 24, 163, true, true);
		drawitem(TILE_CAMCORNER + 1, 320 - 26, 163, false, true);

		if ((int)totalclock & 16)
			drawitem(TILE_CAMLIGHT, 46, 32, false, false);
	}
	else
	{
		int flipbits = ((int)totalclock << 1) & 48;

		for (int x = -64; x < 394; x += 64)
			for (int y = 0; y < 200; y += 64)
				drawitem(TILE_STATIC, x, y, !!((int)totalclock & 8), !!((int)totalclock & 16));
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void dobonus(int bonusonly)
{
    if (isRRRA());
    else if (isRR()) dobonus_r(bonusonly, nullptr);
    else dobonus_d(bonusonly, nullptr);

    // This hack needs to go away!
    if (RRRA_EndEpisode)
    {
        RRRA_EndEpisode = 0;
        ud.volume_number = 1;
        ud.level_number = 0;
        ud.eog = 0;
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


END_DUKE_NS

