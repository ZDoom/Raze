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

#include "ns.h"
#include "duke3d.h"
#include "build.h"
#include "v_video.h"
#include "prediction.h"


BEGIN_DUKE_NS

//---------------------------------------------------------------------------
//
// Floor Over Floor

// If standing in sector with SE42
// then draw viewing to SE41 and raise all =hi SE43 cielings.

// If standing in sector with SE43
// then draw viewing to SE40 and lower all =hi SE42 floors.

// If standing in sector with SE44
// then draw viewing to SE40.

// If standing in sector with SE45
// then draw viewing to SE41.
//
//---------------------------------------------------------------------------

static int tempsectorz[MAXSECTORS];
static int tempsectorpicnum[MAXSECTORS];
//short tempcursectnum;

void SE40_Draw(int tag, int spnum, int x, int y, int z, binangle a, fixedhoriz h, int smoothratio)
{
	int i, j = 0, k = 0;
	int floor1, floor2 = 0, ok = 0, fofmode = 0;
	int offx, offy;

	if (sprite[spnum].ang != 512) return;

	i = FOF;    //Effect TILE
	tileDelete(FOF);
	if (!(gotpic[i >> 3] & (1 << (i & 7)))) return;
	gotpic[i >> 3] &= ~(1 << (i & 7));

	floor1 = spnum;

	if (sprite[spnum].lotag == tag + 2) fofmode = tag + 0;
	if (sprite[spnum].lotag == tag + 3) fofmode = tag + 1;
	if (sprite[spnum].lotag == tag + 4) fofmode = tag + 0;
	if (sprite[spnum].lotag == tag + 5) fofmode = tag + 1;

	ok++;

	for (j = 0; j < MAXSPRITES; j++)
	{
		if (
			sprite[j].picnum == 1 &&
			sprite[j].lotag == fofmode &&
			sprite[j].hitag == sprite[floor1].hitag
			) {
			floor1 = j; fofmode = sprite[j].lotag; ok++; break;
		}
	}
	// if(ok==1) { Message("no floor1",RED); return; }

	if (fofmode == tag + 0) k = tag + 1; else k = tag + 0;

	for (j = 0; j < MAXSPRITES; j++)
	{
		if (
			sprite[j].picnum == 1 &&
			sprite[j].lotag == k &&
			sprite[j].hitag == sprite[floor1].hitag
			) {
			floor2 = j; ok++; break;
		}
	}

	// if(ok==2) { Message("no floor2",RED); return; }

	for (j = 0; j < MAXSPRITES; j++)  // raise ceiling or floor
	{
		if (sprite[j].picnum == 1 &&
			sprite[j].lotag == k + 2 &&
			sprite[j].hitag == sprite[floor1].hitag
			)
		{
			if (k == tag + 0)
			{
				tempsectorz[sprite[j].sectnum] = sector[sprite[j].sectnum].floorz;
				sector[sprite[j].sectnum].floorz += (((z - sector[sprite[j].sectnum].floorz) / 32768) + 1) * 32768;
				tempsectorpicnum[sprite[j].sectnum] = sector[sprite[j].sectnum].floorpicnum;
				sector[sprite[j].sectnum].floorpicnum = 13;
			}
			if (k == tag + 1)
			{
				tempsectorz[sprite[j].sectnum] = sector[sprite[j].sectnum].ceilingz;
				sector[sprite[j].sectnum].ceilingz += (((z - sector[sprite[j].sectnum].ceilingz) / 32768) - 1) * 32768;
				tempsectorpicnum[sprite[j].sectnum] = sector[sprite[j].sectnum].ceilingpicnum;
				sector[sprite[j].sectnum].ceilingpicnum = 13;
			}
		}
	}

	i = floor1;
	offx = x - sprite[i].x;
	offy = y - sprite[i].y;
	i = floor2;

	renderDrawRoomsQ16(sprite[i].x + offx, sprite[i].y + offy, z, a.asq16(), h.asq16(), sprite[i].sectnum);
	fi.animatesprites(offx + sprite[i].x, offy + sprite[i].y, a.asbuild(), smoothratio);
	renderDrawMasks();

	for (j = 0; j < MAXSPRITES; j++)  // restore ceiling or floor
	{
		if (sprite[j].picnum == 1 &&
			sprite[j].lotag == k + 2 &&
			sprite[j].hitag == sprite[floor1].hitag
			)
		{
			if (k == tag + 0)
			{
				sector[sprite[j].sectnum].floorz = tempsectorz[sprite[j].sectnum];
				sector[sprite[j].sectnum].floorpicnum = tempsectorpicnum[sprite[j].sectnum];
			}
			if (k == tag + 1)
			{
				sector[sprite[j].sectnum].ceilingz = tempsectorz[sprite[j].sectnum];
				sector[sprite[j].sectnum].ceilingpicnum = tempsectorpicnum[sprite[j].sectnum];
			}
		}// end if
	}// end for

} // end SE40


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void se40code(int x, int y, int z, binangle a, fixedhoriz h, int smoothratio)
{
	int i, tag;
	if (!isRR()) tag = 40;
	else if (isRRRA()) tag = 150;
	else return;

	i = headspritestat[STAT_RAROR];
	while (i >= 0)
	{
		switch (sprite[i].lotag - tag + 40)
		{
			//            case 40:
			//            case 41:
			//                SE40_Draw(i,x,y,a,smoothratio);
			//                break;
		case 42:
		case 43:
		case 44:
		case 45:
			if (ps[screenpeek].cursectnum == sprite[i].sectnum)
				SE40_Draw(tag, i, x, y, z, a, h, smoothratio);
			break;
		}
		i = nextspritestat[i];
	}
}


//---------------------------------------------------------------------------
//
// split out so it can also be applied to camera views
//
//---------------------------------------------------------------------------

void renderMirror(int cposx, int cposy, int cposz, binangle cang, fixedhoriz choriz, int smoothratio)
{
	if ((gotpic[TILE_MIRROR >> 3] & (1 << (TILE_MIRROR & 7))) > 0)
	{
		int dst = 0x7fffffff, i = 0;
		for (int k = 0; k < mirrorcnt; k++)
		{
			int j = abs(wall[mirrorwall[k]].x - cposx) + abs(wall[mirrorwall[k]].y - cposy);
			if (j < dst) dst = j, i = k;
		}

		if (wall[mirrorwall[i]].overpicnum == TILE_MIRROR)
		{
			int tposx, tposy;
			fixed_t tang;

			renderPrepareMirror(cposx, cposy, cposz, cang.asq16(), choriz.asq16(), mirrorwall[i], &tposx, &tposy, &tang);

			int j = g_visibility;
			g_visibility = (j >> 1) + (j >> 2);

			renderDrawRoomsQ16(tposx, tposy, cposz, tang, choriz.asq16(), mirrorsector[i] + MAXSECTORS);

			display_mirror = 1;
			fi.animatesprites(tposx, tposy, tang, smoothratio);
			display_mirror = 0;

			renderDrawMasks();
			renderCompleteMirror();   //Reverse screen x-wise in this function
			g_visibility = j;
		}
		gotpic[TILE_MIRROR >> 3] &= ~(1 << (TILE_MIRROR & 7));
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void animatecamsprite(int smoothRatio)
{
	const int VIEWSCREEN_ACTIVE_DISTANCE = 8192;

	if (camsprite < 0)
		return;

	int spriteNum = camsprite;

	auto p = &ps[screenpeek];
	auto sp = &sprite[spriteNum];

	if (p->newowner >= 0) sp->owner = p->newowner;

	if (sp->owner >= 0 && dist(&sprite[p->i], sp) < VIEWSCREEN_ACTIVE_DISTANCE)
	{
		auto tex = tileGetTexture(sp->picnum);
		TileFiles.MakeCanvas(TILE_VIEWSCR, tex->GetDisplayWidth(), tex->GetDisplayHeight());

		auto canvas = renderSetTarget(TILE_VIEWSCR);
		if (!canvas) return;

		screen->RenderTextureView(canvas, [=](IntRect& rect)
			{
				// fixme: This needs to interpolate the camera's angle. Position is not relevant because cameras do not move.
				auto camera = &sprite[sp->owner];
				// Note: no ROR or camera here for now - the current setup has no means to detect these things before rendering the scene itself.
				drawrooms(camera->x, camera->y, camera->z, camera->ang, 100 + camera->shade, camera->sectnum); // why 'shade'...?
				display_mirror = 1; // should really be 'display external view'.
				fi.animatesprites(camera->x, camera->y, camera->ang, smoothRatio);
				display_mirror = 0;
				renderDrawMasks();
			});
		renderRestoreTarget();
	}
}

//---------------------------------------------------------------------------
//
// RRRA's drug distortion effect
//
//---------------------------------------------------------------------------

void setdrugmode(player_struct *p, int oyrepeat)
{
	if (playrunning())
	{
		if (p->DrugMode > 0)
		{
			int var_8c;
			if (p->drug_stat[0] == 0)
			{
				p->drug_stat[1]++;
				var_8c = oyrepeat + p->drug_stat[1] * 5000;
				if (oyrepeat * 3 < var_8c)
				{
					renderSetAspect(oyrepeat * 3, yxaspect);
					p->drug_aspect = oyrepeat * 3;
					p->drug_stat[0] = 2;
				}
				else
				{
					renderSetAspect(var_8c, yxaspect);
					p->drug_aspect = var_8c;
				}
				setpal(p);
			}
			else if (p->drug_stat[0] == 3)
			{
				p->drug_stat[1]--;
				var_8c = oyrepeat + p->drug_stat[1] * 5000;
				if (var_8c < oyrepeat)
				{
					renderSetAspect(oyrepeat, yxaspect);
					p->DrugMode = 0;
					p->drug_stat[0] = 0;
					p->drug_stat[2] = 0;
					p->drug_stat[1] = 0;
				}
				else
				{
					renderSetAspect(var_8c, yxaspect);
					p->drug_aspect = var_8c;
				}
				setpal(p);
			}
			else if (p->drug_stat[0] == 2)
			{
				if (p->drug_stat[2] > 30)
				{
					p->drug_stat[0] = 1;
				}
				else
				{
					p->drug_stat[2]++;
					renderSetAspect(p->drug_stat[2] * 500 + oyrepeat * 3, yxaspect);
					p->drug_aspect = oyrepeat * 3 + p->drug_stat[2] * 500;
					setpal(p);
				}
			}
			else
			{
				if (p->drug_stat[2] < 1)
				{
					p->drug_stat[0] = 2;
					p->DrugMode--;
					if (p->DrugMode == 1)
						p->drug_stat[0] = 3;
				}
				else
				{
					p->drug_stat[2]--;
					renderSetAspect(p->drug_stat[2] * 500 + oyrepeat * 3, yxaspect);
					p->drug_aspect = oyrepeat * 3 + p->drug_stat[2] * 500;
					setpal(p);
				}
			}
		}
	}
	else if (p->DrugMode > 0)
	{
		renderSetAspect(p->drug_aspect, yxaspect);
		setpal(p);
	}
}

//---------------------------------------------------------------------------
//
// used by RR to inject some external geometry into a scene. 
//
//---------------------------------------------------------------------------

static void geometryEffect(int cposx, int cposy, int cposz, binangle cang, fixedhoriz choriz, int sect, int smoothratio)
{
	short gs, tgsect, nextspr, geosect, geoid = 0;
	int spr;
	renderDrawRoomsQ16(cposx, cposy, cposz, cang.asq16(), choriz.asq16(), sect);
	fi.animatesprites(cposx, cposy, cang.asbuild(), smoothratio);
	renderDrawMasks();
	for (gs = 0; gs < geocnt; gs++)
	{
		tgsect = geosector[gs];
		spr = headspritesect[tgsect];
		while (spr != -1)
		{
			nextspr = nextspritesect[spr];
			changespritesect((short)spr, geosectorwarp[gs]);
			setsprite((short)spr, sprite[spr].x -= geox[gs], sprite[spr].y -= geoy[gs], sprite[spr].z);
			spr = nextspr;
		}
		if (geosector[gs] == sect)
		{
			geosect = geosectorwarp[gs];
			geoid = gs;
		}
	}
	cposx -= geox[geoid];
	cposy -= geoy[geoid];
	renderDrawRoomsQ16(cposx, cposy, cposz, cang.asq16(), choriz.asq16(), sect);
	cposx += geox[geoid];
	cposy += geoy[geoid];
	for (gs = 0; gs < geocnt; gs++)
	{
		tgsect = geosectorwarp[gs];
		spr = headspritesect[tgsect];
		while (spr != -1)
		{
			nextspr = nextspritesect[spr];
			changespritesect((short)spr, geosector[gs]);
			setsprite((short)spr, sprite[spr].x += geox[gs], sprite[spr].y += geoy[gs], sprite[spr].z);
			spr = nextspr;
		}
	}
	fi.animatesprites(cposx, cposy, cang.asbuild(), smoothratio);
	renderDrawMasks();
	for (gs = 0; gs < geocnt; gs++)
	{
		tgsect = geosector[gs];
		spr = headspritesect[tgsect];
		while (spr != -1)
		{
			nextspr = nextspritesect[spr];
			changespritesect((short)spr, geosectorwarp2[gs]);
			setsprite((short)spr, sprite[spr].x -= geox2[gs], sprite[spr].y -= geoy2[gs], sprite[spr].z);
			spr = nextspr;
		}
		if (geosector[gs] == sect)
		{
			geosect = geosectorwarp2[gs];
			geoid = gs;
		}
	}
	cposx -= geox2[geoid];
	cposy -= geoy2[geoid];
	renderDrawRoomsQ16(cposx, cposy, cposz, cang.asq16(), choriz.asq16(), sect);
	cposx += geox2[geoid];
	cposy += geoy2[geoid];
	for (gs = 0; gs < geocnt; gs++)
	{
		tgsect = geosectorwarp2[gs];
		spr = headspritesect[tgsect];
		while (spr != -1)
		{
			nextspr = nextspritesect[spr];
			changespritesect((short)spr, geosector[gs]);
			setsprite((short)spr, sprite[spr].x += geox2[gs], sprite[spr].y += geoy2[gs], sprite[spr].z);
			spr = nextspr;
		}
	}
	fi.animatesprites(cposx, cposy, cang.asbuild(), smoothratio);
	renderDrawMasks();
}
//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void displayrooms(int snum, double smoothratio)
{
	int cposx, cposy, cposz, fz, cz;
	short sect;
	binangle cang;
	fixedhoriz choriz;
	struct player_struct* p;
	int tiltcs = 0; // JBF 20030807

	p = &ps[snum];

	if (automapMode == am_full || p->cursectnum == -1)
		return;

	// Do not light up the fog in RRRA's E2L1. Ideally this should apply to all foggy levels but all others use lookup table hacks for their fog.
	if (isRRRA() && fogactive)
	{
		p->visibility = ud.const_visibility;
	}

	g_visibility = p->visibility;

	videoSetCorrectedAspect();

	sect = p->cursectnum;
	if (sect < 0 || sect >= MAXSECTORS) return;

	GLInterface.SetMapFog(fogactive != 0);
	dointerpolations(smoothratio);

	setgamepalette(BASEPAL);
	animatecamsprite(smoothratio);

	// The camera texture must be rendered with the base palette, so this is the only place where the current global palette can be set.
	// The setting here will be carried over to the rendering of the weapon sprites, but other 2D content will always default to the main palette.
	setgamepalette(p->palette);
	if (ud.camerasprite >= 0)
	{
		spritetype* s;

		s = &sprite[ud.camerasprite];

		if (s->yvel < 0) s->yvel = -100;
		else if (s->yvel > 199) s->yvel = 300;

		cang = buildang(hittype[ud.camerasprite].tempang + xs_CRoundToInt(fmulscale16(((s->ang + 1024 - hittype[ud.camerasprite].tempang) & 2047) - 1024, smoothratio)));

		auto bh = buildhoriz(s->yvel);
		se40code(s->x, s->y, s->z, cang, bh, smoothratio);
		renderMirror(s->x, s->y, s->z, cang, bh, smoothratio);
		renderDrawRoomsQ16(s->x, s->y, s->z - (4 << 8), cang.asq16(), bh.asq16(), s->sectnum);
		fi.animatesprites(s->x, s->y, cang.asbuild(), smoothratio);
		renderDrawMasks();
	}
	else
	{
		int i = divscale22(1, isRR() ? 64 : sprite[p->i].yrepeat + 28);
		fixed_t dang = IntToFixed(1024);
		if (!isRRRA() || !p->DrugMode)
		{
			// Fixme: This should get the aspect ratio from the backend, not the current viewport size.
			int viewingRange = xs_CRoundToInt(double(i) * tan(r_fov * (pi::pi() / 360.)));
			renderSetAspect(mulscale16(viewingRange, viewingrange), yxaspect);
		}
		else
		{
			setdrugmode(p, i);
		}

		if (!cl_syncinput)
			renderSetRollAngle(FixedToFloat(p->q16rotscrnang));
		else
			renderSetRollAngle(FixedToFloat(p->oq16rotscrnang + fmulscale16(((p->q16rotscrnang - p->oq16rotscrnang + dang) & 0x7FFFFFF) - dang, smoothratio)));

		if ((snum == myconnectindex) && (numplayers > 1))
		{
			cposx = omyx + xs_CRoundToInt(fmulscale16(myx - omyx, smoothratio));
			cposy = omyy + xs_CRoundToInt(fmulscale16(myy - omyy, smoothratio));
			cposz = omyz + xs_CRoundToInt(fmulscale16(myz - omyz, smoothratio));
			if (cl_syncinput)
			{
				fixed_t ohorz = (oq16myhoriz + oq16myhorizoff);
				fixed_t horz = (q16myhoriz + q16myhorizoff);
				choriz = q16horiz(ohorz + xs_CRoundToInt(fmulscale16(horz - ohorz, smoothratio)));
				cang = q16ang(oq16myang + xs_CRoundToInt(fmulscale16(((q16myang + dang - oq16myang) & 0x7FFFFFF) - dang, smoothratio)));
			}
			else
			{
				cang = q16ang(q16myang);
				choriz = q16horiz(q16myhoriz + q16myhorizoff);
			}
			sect = mycursectnum;
		}
		else
		{
			cposx = p->oposx + xs_CRoundToInt(fmulscale16(p->posx - p->oposx, smoothratio));
			cposy = p->oposy + xs_CRoundToInt(fmulscale16(p->posy - p->oposy, smoothratio));
			cposz = p->oposz + xs_CRoundToInt(fmulscale16(p->posz - p->oposz, smoothratio));
			if (cl_syncinput)
			{
				// Original code for when the values are passed through the sync struct
				fixed_t ohorz = (p->oq16horiz + p->oq16horizoff);
				fixed_t horz = (p->q16horiz + p->q16horizoff);
				choriz = q16horiz(ohorz + xs_CRoundToInt(fmulscale16(horz - ohorz, smoothratio)));

				fixed_t oang = (p->oq16ang + p->oq16look_ang);
				fixed_t ang = (p->q16ang + p->q16look_ang);
				cang = q16ang(oang + xs_CRoundToInt(fmulscale16(((ang + dang - oang) & 0x7FFFFFF) - dang, smoothratio)));
			}
			else
			{
				// This is for real time updating of the view direction.
				cang = q16ang(p->q16ang + p->q16look_ang);
				choriz = q16horiz(p->q16horiz + p->q16horizoff);
			}
		}

		if (p->newowner >= 0)
		{
			fixed_t oang = hittype[p->newowner].oq16ang;
			cang = q16ang(oang + xs_CRoundToInt(fmulscale16(((p->q16ang + dang - oang) & 0x7FFFFFF) - dang, smoothratio)));
			choriz = q16horiz(p->q16horiz + p->q16horizoff);
			cposx = p->posx;
			cposy = p->posy;
			cposz = p->posz;
			sect = sprite[p->newowner].sectnum;
			smoothratio = MaxSmoothRatio;
		}
		else if (p->over_shoulder_on == 0)
		{
			if (cl_viewbob) cposz += p->opyoff + xs_CRoundToInt(fmulscale16(p->pyoff - p->opyoff, smoothratio));
		}
		else view(p, &cposx, &cposy, &cposz, &sect, cang.asbuild(), choriz.asbuild(), smoothratio);

		cz = hittype[p->i].ceilingz;
		fz = hittype[p->i].floorz;

		if (earthquaketime > 0 && p->on_ground == 1)
		{
			cposz += 256 - (((earthquaketime) & 1) << 9);
			cang += buildang((2 - ((earthquaketime) & 2)) << 2);
		}

		if (sprite[p->i].pal == 1) cposz -= (18 << 8);

		if (p->newowner >= 0)
			choriz = buildhoriz(100 + sprite[p->newowner].shade);

		else if (p->spritebridge == 0)
		{
			if (cposz < (p->truecz + (4 << 8))) cposz = cz + (4 << 8);
			else if (cposz > (p->truefz - (4 << 8))) cposz = fz - (4 << 8);
		}

		if (sect >= 0)
		{
			getzsofslope(sect, cposx, cposy, &cz, &fz);
			if (cposz < cz + (4 << 8)) cposz = cz + (4 << 8);
			if (cposz > fz - (4 << 8)) cposz = fz - (4 << 8);
		}

		choriz = clamp(choriz, buildhoriz(HORIZ_MIN), buildhoriz(HORIZ_MAX));

		if (isRR() && sector[sect].lotag == 848)
		{
			geometryEffect(cposx, cposy, cposz, cang, choriz, sect, smoothratio);
		}
		else
		{
			se40code(cposx, cposy, cposz, cang, choriz, smoothratio);
			renderMirror(cposx, cposy, cposz, cang, choriz, smoothratio);
			renderDrawRoomsQ16(cposx, cposy, cposz, cang.asq16(), choriz.asq16(), sect);
			fi.animatesprites(cposx, cposy, cang.asbuild(), smoothratio);
			renderDrawMasks();
		}
	}
	//GLInterface.SetMapFog(false);
	restoreinterpolations();

	if (!isRRRA() || !fogactive)
	{
		if (ud.levelclock < lastvisinc)
		{
			if (abs(p->visibility - ud.const_visibility) > 8)
				p->visibility += (ud.const_visibility - p->visibility) >> 2;
		}
		else p->visibility = ud.const_visibility;
	}
}

bool GameInterface::GenerateSavePic()
{
	displayrooms(myconnectindex, MaxSmoothRatio);
	return true;
}



END_DUKE_NS
