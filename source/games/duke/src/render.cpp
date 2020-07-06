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

void DrawBorder();

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

void SE40_Draw(int tag, int spnum, int x, int y, int z, int a, int h, int smoothratio)
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
#if 0
	drawrooms(offx + sprite[i].x, offy + sprite[i].y, z, a, h, sprite[i].sectnum);
#else
	renderDrawRoomsQ16(sprite[i].x + offx, sprite[i].y + offy, z, a, h, sprite[i].sectnum);
#endif

	fi.animatesprites(offx + sprite[i].x, offy + sprite[i].y, fix16_to_int(a), smoothratio);
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

void se40code(int x, int y, int z, int a, int h, int smoothratio)
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

void renderMirror(int cposx, int cposy, int cposz, int cang, int choriz, int smoothratio)
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
			int tposx, tposy, tang;

			renderPrepareMirror(cposx, cposy, cposz, cang << FRACBITS, choriz<<FRACBITS, mirrorwall[i], &tposx, &tposy, &tang);

			int j = g_visibility;
			g_visibility = (j >> 1) + (j >> 2);

			renderDrawRoomsQ16(tposx, tposy, cposz, tang, choriz << FRACBITS, mirrorsector[i] + MAXSECTORS);

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
	if (!paused)
	{
		if (p->DrugMode > 0 && !(p->gm & MODE_TYPE) && !ud.pause_on)
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

static void geometryEffect(int cposx, int cposy, int cposz, int cang, int choriz, int sect, int smoothratio)
{
	short gs, tgsect, nextspr, geosect, geoid = 0;
	int spr;
	drawrooms(cposx, cposy, cposz, cang, choriz, sect);
	fi.animatesprites(cposx, cposy, cang, smoothratio);
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
	drawrooms(cposx, cposy, cposz, cang, choriz, sect);
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
	fi.animatesprites(cposx, cposy, cang, smoothratio);
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
	drawrooms(cposx, cposy, cposz, cang, choriz, sect);
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
	fi.animatesprites(cposx, cposy, cang, smoothratio);
	renderDrawMasks();
}
//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void displayrooms(int snum, int smoothratio)
{
	int cposx, cposy, cposz, fz, cz;
	short sect, cang, choriz;
	struct player_struct* p;
	int tiltcs = 0; // JBF 20030807

	p = &ps[snum];
	DrawBorder();

	if (ud.overhead_on == 2 || p->cursectnum == -1)
		return;

	// Do not light up the fog in RRRA's E2L1. Ideally this should apply to all foggy levels but all others use lookup table hacks for their fog.
	if (isRRRA() && fogactive)
	{
		p->visibility = ud.const_visibility;
	}

	g_visibility = p->visibility;

	newaspect_enable = 1;
	videoSetCorrectedAspect();

	smoothratio = min(max(smoothratio, 0), 65536);
	if (ud.pause_on || ps[snum].on_crane > -1) smoothratio = 65536;

	sect = p->cursectnum;
	if (sect < 0 || sect >= MAXSECTORS) return;

	dointerpolations(smoothratio);

	animatecamsprite(smoothratio);

	if (ud.camerasprite >= 0)
	{
		spritetype* s;

		s = &sprite[ud.camerasprite];

		if (s->yvel < 0) s->yvel = -100;
		else if (s->yvel > 199) s->yvel = 300;

		cang = hittype[ud.camerasprite].tempang + mulscale16((int)(((s->ang + 1024 - hittype[ud.camerasprite].tempang) & 2047) - 1024), smoothratio);

		se40code(s->x, s->y, s->z, cang, s->yvel, smoothratio);
		renderMirror(s->x, s->y, s->z, cang, s->yvel, smoothratio);
		drawrooms(s->x, s->y, s->z - (4 << 8), cang, s->yvel, s->sectnum);
		fi.animatesprites(s->x, s->y, cang, smoothratio);
		renderDrawMasks();
	}
	else
	{
		int i = divscale22(1, isRR() ? 64 : sprite[p->i].yrepeat + 28);
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

		renderSetRollAngle(p->orotscrnang + mulscale16(((p->rotscrnang - p->orotscrnang + 1024) & 2047) - 1024, smoothratio));
		p->orotscrnang = p->rotscrnang; // JBF: save it for next time

		if ((snum == myconnectindex) && (numplayers > 1))
		{
			cposx = omyx + mulscale16((int)(myx - omyx), smoothratio);
			cposy = omyy + mulscale16((int)(myy - omyy), smoothratio);
			cposz = omyz + mulscale16((int)(myz - omyz), smoothratio);
#if 0
			cang = omyang + mulscale16((int)(((myang + 1024 - omyang) & 2047) - 1024), smoothratio);
			choriz = omyhoriz + omyhorizoff + mulscale16((int)(myhoriz + myhorizoff - omyhoriz - omyhorizoff), smoothratio);
#else
			cang = myang;
			choriz = (myhoriz + myhorizoff);
#endif
			sect = mycursectnum;
		}
		else
		{
			cposx = p->oposx + mulscale16((int)(p->posx - p->oposx), smoothratio);
			cposy = p->oposy + mulscale16((int)(p->posy - p->oposy), smoothratio);
			cposz = p->oposz + mulscale16((int)(p->posz - p->oposz), smoothratio);
#if 0
			// Original code for when the values are passed through the sync struct
			cang = p->getoang() + mulscale16((int)(((p->getang() + 1024 - p->getoang()) & 2047) - 1024), smoothratio);
			choriz = p->ohoriz+p->ohorizoff+mulscale16((int)(p->gethorizsum()-p->ohoriz-p->ohorizoff),smoothratio);
#else
			// This is for real time updating of the view direction.
			cang = p->getang();
			choriz = p->gethorizsum();
#endif
		}
		cang += p->look_ang;

		if (p->newowner >= 0)
		{
			cang = p->getang() + p->getlookang();
			choriz = p->gethorizsum();
			cposx = p->posx;
			cposy = p->posy;
			cposz = p->posz;
			sect = sprite[p->newowner].sectnum;
			smoothratio = 65536L;
		}
		else if (p->over_shoulder_on == 0)
		{
			if (cl_viewbob) cposz += p->opyoff + mulscale16((int)(p->pyoff - p->opyoff), smoothratio);
		}
		else view(p, &cposx, &cposy, &cposz, &sect, cang, choriz);

		cz = hittype[p->i].ceilingz;
		fz = hittype[p->i].floorz;

		if (earthquaketime > 0 && p->on_ground == 1)
		{
			cposz += 256 - (((earthquaketime) & 1) << 9);
			cang += (2 - ((earthquaketime) & 2)) << 2;
		}

		if (sprite[p->i].pal == 1) cposz -= (18 << 8);

		if (p->newowner >= 0)
			choriz = 100 + sprite[p->newowner].shade;

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

		if (choriz > 299) choriz = 299;
		else if (choriz < -99) choriz = -99;

		if (isRR() && sector[sect].lotag == 848)
		{
			geometryEffect(cposx, cposy, cposz, cang, choriz, sect, smoothratio);
		}
		else
		{
			se40code(cposx, cposy, cposz, cang, choriz, smoothratio);
			renderMirror(cposx, cposy, cposz, cang, choriz, smoothratio);
			drawrooms(cposx, cposy, cposz, cang, choriz, sect);
			fi.animatesprites(cposx, cposy, cang, smoothratio);
			renderDrawMasks();
		}
	}

	restoreinterpolations();

	if (!isRRRA() || !fogactive)
	{
		if (totalclock < lastvisinc)
		{
			if (abs(p->visibility - ud.const_visibility) > 8)
				p->visibility += (ud.const_visibility - p->visibility) >> 2;
		}
		else p->visibility = ud.const_visibility;
	}
}

bool GameInterface::GenerateSavePic()
{
	displayrooms(myconnectindex, 65536);
	return true;
}



END_DUKE_NS
