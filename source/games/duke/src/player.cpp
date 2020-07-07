//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2020 - Christoph Oelckers

This file is part of Enhanced Duke Nukem 3D version 1.5 - Atomic Edition

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

EDuke enhancements integrated: 04/13/2003 - Matt Saettler

Note: EDuke source was in transition.  Changes are in-progress in the
source as it is released.

*/
//-------------------------------------------------------------------------


#include "ns.h"
#include "global.h"
#include "mapinfo.h"

BEGIN_DUKE_NS 

//---------------------------------------------------------------------------
//
// why is this such a mess?
//
//---------------------------------------------------------------------------

void setpal(struct player_struct* p)
{
	int palette;
	if (p->DrugMode) palette = DRUGPAL;
	else if (p->heat_on) palette = SLIMEPAL;
	else if (p->cursectnum < 0) palette = BASEPAL; // don't crash if out of range.
	else if (sector[p->cursectnum].ceilingpicnum >= TILE_FLOORSLIME && sector[p->cursectnum].ceilingpicnum <= TILE_FLOORSLIME + 2) palette = SLIMEPAL;
	else if (sector[p->cursectnum].lotag == ST_2_UNDERWATER) palette = WATERPAL;
	else palette = BASEPAL;
	p->palette = palette;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void quickkill(struct player_struct* p)
{
	SetPlayerPal(p, PalEntry(48, 48, 48, 48));

	sprite[p->i].extra = 0;
	sprite[p->i].cstat |= 32768;
	if (ud.god == 0) fi.guts(&sprite[p->i], TILE_JIBS6, 8, myconnectindex);
	return;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void forceplayerangle(struct player_struct* p)
{
	int n;

	n = 128 - (krand() & 255);

	p->addhoriz(64);
	p->return_to_center = 9;
	p->setlookang(n >> 1);
	p->setrotscrnang(n >> 1);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void tracers(int x1, int y1, int z1, int x2, int y2, int z2, int n)
{
	int i, xv, yv, zv;
	short sect = -1;

	i = n + 1;
	xv = (x2 - x1) / i;
	yv = (y2 - y1) / i;
	zv = (z2 - z1) / i;

	if ((abs(x1 - x2) + abs(y1 - y2)) < 3084)
		return;

	for (i = n; i > 0; i--)
	{
		x1 += xv;
		y1 += yv;
		z1 += zv;
		updatesector(x1, y1, &sect);
		if (sect >= 0)
		{
			if (sector[sect].lotag == 2)
				EGS(sect, x1, y1, z1, TILE_WATERBUBBLE, -32, 4 + (krand() & 3), 4 + (krand() & 3), krand() & 2047, 0, 0, ps[0].i, 5);
			else
				EGS(sect, x1, y1, z1, TILE_SMALLSMOKE, -32, 14, 14, 0, 0, 0, ps[0].i, 5);
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int hits(int i)
{
	auto sp = &sprite[i];
	int sx, sy, sz;
	short sect;
	short hw, hs;
	int zoff;

	if (sp->picnum == TILE_APLAYER) zoff = (40 << 8);
	else zoff = 0;

	hitscan(sp->x, sp->y, sp->z - zoff, sp->sectnum, sintable[(sp->ang + 512) & 2047], sintable[sp->ang & 2047], 0, &sect, &hw, &hs, &sx, &sy, &sz, CLIPMASK1);

	return (FindDistance2D(sx - sp->x, sy - sp->y));
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int hitasprite(int i, short* hitsp)
{
	auto sp = &sprite[i];
	int sx, sy, sz, zoff;
	short sect, hw;

	if (badguy(&sprite[i]))
		zoff = (42 << 8);
	else if (sp->picnum == TILE_APLAYER) zoff = (39 << 8);
	else zoff = 0;

	hitscan(sp->x, sp->y, sp->z - zoff, sp->sectnum, sintable[(sp->ang + 512) & 2047], sintable[sp->ang & 2047], 0, &sect, &hw, hitsp, &sx, &sy, &sz, CLIPMASK1);

	if (hw >= 0 && (wall[hw].cstat & 16) && badguy(&sprite[i]))
		return((1 << 30));

	return (FindDistance2D(sx - sp->x, sy - sp->y));
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int hitawall(struct player_struct* p, int* hitw)
{
	int sx, sy, sz;
	short sect, hs, hitw1;

	hitscan(p->posx, p->posy, p->posz, p->cursectnum,
		sintable[(p->getang() + 512) & 2047], sintable[p->getang() & 2047], 0, &sect, &hitw1, &hs, &sx, &sy, &sz, CLIPMASK0);
	*hitw = hitw1;

	return (FindDistance2D(sx - p->posx, sy - p->posy));
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int aim(spritetype* s, int aang)
{
	char gotshrinker, gotfreezer;
	int i, j, a, k, cans;
	int aimstats[] = { 10,13,1,2 };
	int dx1, dy1, dx2, dy2, dx3, dy3, smax, sdist;
	int xv, yv;

	a = s->ang;

	// Autoaim from DukeGDX.
	if (s->picnum == TILE_APLAYER && ps[s->yvel].auto_aim == 0)
		return -1;

	j = -1;
	//	  if(s->picnum == TILE_APLAYER && ps[s->yvel].aim_mode) return -1;

	if (isRR())
	{
		gotshrinker = 0;
		gotfreezer = 0;
	}
	else if (isWW2GI())
	{
		gotshrinker = s->picnum == TILE_APLAYER && aplWeaponWorksLike[ps[s->yvel].curr_weapon][s->yvel] == SHRINKER_WEAPON;
		gotfreezer = s->picnum == TILE_APLAYER && aplWeaponWorksLike[ps[s->yvel].curr_weapon][s->yvel] == FREEZE_WEAPON;
	}
	else
	{
		gotshrinker = s->picnum == TILE_APLAYER && ps[s->yvel].curr_weapon == SHRINKER_WEAPON;
		gotfreezer = s->picnum == TILE_APLAYER && ps[s->yvel].curr_weapon == FREEZE_WEAPON;
	}

	smax = 0x7fffffff;

	dx1 = sintable[(a + 512 - aang) & 2047];
	dy1 = sintable[(a - aang) & 2047];
	dx2 = sintable[(a + 512 + aang) & 2047];
	dy2 = sintable[(a + aang) & 2047];

	dx3 = sintable[(a + 512) & 2047];
	dy3 = sintable[a & 2047];

	for (k = 0; k < 4; k++)
	{
		if (j >= 0)
			break;
		for (i = headspritestat[aimstats[k]]; i >= 0; i = nextspritestat[i])
			if (sprite[i].xrepeat > 0 && sprite[i].extra >= 0 && (sprite[i].cstat & (257 + 32768)) == 257)
				if (badguy(&sprite[i]) || k < 2)
				{
					auto sp = &sprite[i];
					if (badguy(&sprite[i]) || sp->picnum == TILE_APLAYER)
					{
						if (sp->picnum == TILE_APLAYER &&
							(isRR() && ud.ffire == 0) &&
							ud.coop == 1 &&
							s->picnum == TILE_APLAYER &&
							s != &sprite[i])
							continue;

						if (gotshrinker && sprite[i].xrepeat < 30 && !(actorinfo[sp->picnum].flags & SFLAG_SHRINKAUTOAIM)) continue;
						if (gotfreezer && sprite[i].pal == 1) continue;
					}

					xv = (sp->x - s->x);
					yv = (sp->y - s->y);

					if ((dy1 * xv) <= (dx1 * yv))
						if ((dy2 * xv) >= (dx2 * yv))
						{
							sdist = mulscale(dx3, xv, 14) + mulscale(dy3, yv, 14);
							if (sdist > 512 && sdist < smax)
							{
								if (s->picnum == TILE_APLAYER)
									a = (abs(scale(sp->z - s->z, 10, sdist) - (ps[s->yvel].gethorizsum() - 100)) < 100);
								else a = 1;

								cans = cansee(sp->x, sp->y, sp->z - (32 << 8) + actorinfo[sp->picnum].aimoffset, sp->sectnum, s->x, s->y, s->z - (32 << 8), s->sectnum);

								if (a && cans)
								{
									smax = sdist;
									j = i;
								}
							}
						}
				}
	}

	return j;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void dokneeattack(int snum, int pi, const std::initializer_list<int> & respawnlist)
{
	auto p = &ps[snum];

	if (p->knee_incs > 0)
	{
		p->knee_incs++;
		p->addhoriz(-48);
		p->return_to_center = 9;
		if (p->knee_incs > 15)
		{
			p->knee_incs = 0;
			p->holster_weapon = 0;
			if (p->weapon_pos < 0)
				p->weapon_pos = -p->weapon_pos;
			if (p->actorsqu >= 0 && dist(&sprite[pi], &sprite[p->actorsqu]) < 1400)
			{
				fi.guts(&sprite[p->actorsqu], TILE_JIBS6, 7, myconnectindex);
				fi.spawn(p->actorsqu, TILE_BLOODPOOL);
				spritesound(SQUISHED, p->actorsqu);
				if (isIn(sprite[p->actorsqu].picnum, respawnlist))
				{
					if (sprite[p->actorsqu].yvel)
						fi.operaterespawns(sprite[p->actorsqu].yvel);
				}

				if (sprite[p->actorsqu].picnum == TILE_APLAYER)
				{
					quickkill(&ps[sprite[p->actorsqu].yvel]);
					ps[sprite[p->actorsqu].yvel].frag_ps = snum;
				}
				else if (badguy(&sprite[p->actorsqu]))
				{
					deletesprite(p->actorsqu);
					p->actors_killed++;
				}
				else deletesprite(p->actorsqu);
			}
			p->actorsqu = -1;
		}
		else if (p->actorsqu >= 0)
			p->addang(getincangle(p->getang(), getangle(sprite[p->actorsqu].x - p->posx, sprite[p->actorsqu].y - p->posy)) >> 2);
	}

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int makepainsounds(int snum, int type)
{
	auto p = &ps[snum];
	auto pi = p->i;
	auto s = &sprite[pi];
	int k = 0;

	switch (type)
	{
	case 0:
		if (rnd(32))
		{
			if (p->boot_amount > 0)
				k = 1;
			else
			{
				if (!A_CheckSoundPlaying(pi, DUKE_LONGTERM_PAIN))
					spritesound(DUKE_LONGTERM_PAIN, pi);
				SetPlayerPal(p, PalEntry(32, 64, 64, 64));
				s->extra -= 1 + (krand() & 3);
				if (!A_CheckSoundPlaying(pi, SHORT_CIRCUIT))
					spritesound(SHORT_CIRCUIT, pi);
			}
		}
		break;
	case 1:
		if (rnd(16))
		{
			if (p->boot_amount > 0)
				k = 1;
			else
			{
				if (!A_CheckSoundPlaying(pi, DUKE_LONGTERM_PAIN))
					spritesound(DUKE_LONGTERM_PAIN, pi);
				SetPlayerPal(p, PalEntry(32, 0, 8, 0));
				s->extra -= 1 + (krand() & 3);
			}
		}
		break;
	case 2:
		if (rnd(32))
		{
			if (p->boot_amount > 0)
				k = 1;
			else
			{
				if (!A_CheckSoundPlaying(pi, DUKE_LONGTERM_PAIN))
					spritesound(DUKE_LONGTERM_PAIN, pi);
				SetPlayerPal(p, PalEntry(32, 8, 0, 0));
				s->extra -= 1 + (krand() & 3);
			}
		}
		break;
	case 3:
		if ((krand() & 3) == 1)
			if (p->on_ground)
			{
				if (p->OnMotorcycle)
					s->extra -= 2;
				else
					s->extra -= 4;
				spritesound(DUKE_LONGTERM_PAIN, pi);
			}
		break;
	}
	return k;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void footprints(int snum)
{
	auto p = &ps[snum];
	auto pi = p->i;
	auto s = &sprite[pi];
	auto psect = s->sectnum;

	if (p->footprintcount > 0 && p->on_ground)
		if ((sector[p->cursectnum].floorstat & 2) != 2)
		{
			int j;
			for (j = headspritesect[psect]; j >= 0; j = nextspritesect[j])
				if (sprite[j].picnum == TILE_FOOTPRINTS || sprite[j].picnum == TILE_FOOTPRINTS2 || sprite[j].picnum == TILE_FOOTPRINTS3 || sprite[j].picnum == TILE_FOOTPRINTS4)
					if (abs(sprite[j].x - p->posx) < 384)
						if (abs(sprite[j].y - p->posy) < 384)
							break;
			if (j < 0)
			{
				p->footprintcount--;
				if (sector[p->cursectnum].lotag == 0 && sector[p->cursectnum].hitag == 0)
				{
					switch (krand() & 3)
					{
					case 0:	 j = fi.spawn(pi, TILE_FOOTPRINTS); break;
					case 1:	 j = fi.spawn(pi, TILE_FOOTPRINTS2); break;
					case 2:	 j = fi.spawn(pi, TILE_FOOTPRINTS3); break;
					default: j = fi.spawn(pi, TILE_FOOTPRINTS4); break;
					}
					sprite[j].pal = p->footprintpal;
					sprite[j].shade = p->footprintshade;
				}
			}
		}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void playerisdead(int snum, int psectlotag, int fz, int cz)
{
	auto p = &ps[snum];
	auto pi = p->i;
	auto s = &sprite[pi];

	if (p->dead_flag == 0)
	{
		if (s->pal != 1)
		{
			SetPlayerPal(p, PalEntry(63, 63, 0, 0));
			p->posz -= (16 << 8);
			s->z -= (16 << 8);
		}
#if 0
		if (ud.recstat == 1 && ud.multimode < 2)
			closedemowrite();
#endif

		if (s->pal != 1)
			p->dead_flag = (512 - ((krand() & 1) << 10) + (krand() & 255) - 512) & 2047;

		p->jetpack_on = 0;
		p->holoduke_on = -1;

		if (!isRR())S_StopEnvSound(DUKE_JETPACK_IDLE, pi);
		S_StopEnvSound(-1, pi, CHAN_VOICE);


		if (s->pal != 1 && (s->cstat & 32768) == 0) s->cstat = 0;

		if (ud.multimode > 1 && (s->pal != 1 || (s->cstat & 32768)))
		{
			if (p->frag_ps != snum)
			{
				ps[p->frag_ps].frag++;
				//frags[p->frag_ps][snum]++;
				g_player[p->frag_ps].frags[snum]++;	 // TRANSITIONAL
				g_player[snum].frags[snum]++;  // deaths

				auto pname = &g_player[p->frag_ps].user_name[0];	 // TRANSITIONAL
				//&ud.user_name[p->frag_ps][0]);
				if (snum == screenpeek)
				{
					quoteMgr.InitializeQuote(QUOTE_RESERVED, "Killed by %s", pname);
					FTA(QUOTE_RESERVED, p);
				}
				else
				{
					quoteMgr.InitializeQuote(QUOTE_RESERVED2, "Killed %s", pname);
					FTA(QUOTE_RESERVED2, p);
				}

			}
			else p->fraggedself++;

#if 0
			if (myconnectindex == connecthead)
			{
				sprintf(tempbuf, "frag %d killed %d\n", p->frag_ps + 1, snum + 1);
				sendscore(tempbuf);
				//					  printf(tempbuf);
			}
#endif

			p->frag_ps = snum;
		}
	}

	if (psectlotag == ST_2_UNDERWATER)
	{
		if (p->on_warping_sector == 0)
		{
			if (abs(p->posz - fz) > (PHEIGHT >> 1))
				p->posz += 348;
		}
		else
		{
			s->z -= 512;
			s->zvel = -348;
		}

		clipmove(&p->posx, &p->posy,
			&p->posz, &p->cursectnum,
			0, 0, 164L, (4L << 8), (4L << 8), CLIPMASK0);
		//			  p->bobcounter += 32;
	}

	p->oposx = p->posx;
	p->oposy = p->posy;
	p->oposz = p->posz;
	p->oq16ang = p->q16ang;
	p->opyoff = p->pyoff;

	p->sethoriz(100);
	p->q16horizoff = 0;

	updatesector(p->posx, p->posy, &p->cursectnum);

	pushmove(&p->posx, &p->posy, &p->posz, &p->cursectnum, 128L, (4L << 8), (20L << 8), CLIPMASK0);

	if (fz > cz + (16 << 8) && s->pal != 1)
		p->setrotscrnang((p->dead_flag + ((fz + p->posz) >> 7)) & 2047);

	p->on_warping_sector = 0;

}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int endoflevel(int snum)
{
	auto p = &ps[snum];

	// the fist puching the end-of-level thing...
	p->fist_incs++;
	if (p->fist_incs == 28)
	{
#if 0
		if (ud.recstat == 1) closedemowrite();
#endif
		sound(PIPEBOMB_EXPLODE);
		SetPlayerPal(p, PalEntry(48, 64, 64, 64));
	}
	if (p->fist_incs > 42)
	{
		setnextmap(!!p->buttonpalette);
		return 1;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int timedexit(int snum)
{
	auto p = &ps[snum];
	p->timebeforeexit--;
	if (p->timebeforeexit == 26 * 5)
	{
		FX_StopAllSounds();
		if (p->customexitsound >= 0)
		{
			sound(p->customexitsound);
			FTA(102, p);
		}
	}
	else if (p->timebeforeexit == 1)
	{
		setnextmap(false);
		return true;
	}
	return false;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void playerCrouch(int snum)
{
	auto p = &ps[snum];
	// crouching
	SetGameVarID(g_iReturnVarID, 0, p->i, snum);
	OnEvent(EVENT_CROUCH, p->i, snum, -1);
	if (GetGameVarID(g_iReturnVarID, p->i, snum) == 0)
	{
		p->posz += (2048 + 768);
		p->crack_time = 777;
	}
}

void playerJump(int snum, int fz, int cz)
{
	auto p = &ps[snum];
	if (p->jumping_toggle == 0 && p->jumping_counter == 0)
	{
		if ((fz - cz) > (56 << 8))
		{
			SetGameVarID(g_iReturnVarID, 0, p->i, snum);
			OnEvent(EVENT_JUMP, p->i, snum, -1);
			if (GetGameVarID(g_iReturnVarID, p->i, snum) == 0)
			{
				p->jumping_counter = 1;
				p->jumping_toggle = 1;
			}
		}
	}
}

void playerLookLeft(int snum)
{
	auto p = &ps[snum];
	SetGameVarID(g_iReturnVarID, 0, p->i, snum);
	OnEvent(EVENT_LOOKLEFT, p->i, snum, -1);
	if (GetGameVarID(g_iReturnVarID, p->i, snum) == 0)
	{
		p->addlookang(-152);
		p->addrotscrnang(24);
	}
}

void playerLookRight(int snum)
{
	auto p = &ps[snum];
	SetGameVarID(g_iReturnVarID, 0, p->i, snum);
	OnEvent(EVENT_LOOKRIGHT, p->i, snum, -1);
	if (GetGameVarID(g_iReturnVarID, p->i, snum) == 0)
	{
		p->addlookang(152);
		p->addrotscrnang(24);
	}
}

void playerCenterView(int snum)
{
	auto p = &ps[snum];
	SetGameVarID(g_iReturnVarID, 0, p->i, snum);
	OnEvent(EVENT_RETURNTOCENTER, p->i, snum, -1);
	if (GetGameVarID(g_iReturnVarID, p->i, snum) == 0)
	{
		p->return_to_center = 9;
	}
}

#pragma message("input stuff begins here")
void horizAngleAdjust(int snum, int delta)
{
#if 1 // for per-frame input
	g_player[snum].horizAngleAdjust = delta;
#else // for synchronous input
	ps[snum].addhoriz(delta);
#endif
}

void playerLookUp(int snum, ESyncBits sb_snum)
{
	auto p = &ps[snum];
	SetGameVarID(g_iReturnVarID, 0, p->i, snum);
	OnEvent(EVENT_LOOKUP, p->i, snum, -1);
	if (GetGameVarID(g_iReturnVarID, p->i, snum) == 0)
	{
		p->return_to_center = 9;
		horizAngleAdjust(snum, (sb_snum & SKB_RUN) ? 12 : 24);
	}
}

void playerLookDown(int snum, ESyncBits sb_snum)
{
	auto p = &ps[snum];
	SetGameVarID(g_iReturnVarID, 0, p->i, snum);
	OnEvent(EVENT_LOOKDOWN, p->i, snum, -1);
	if (GetGameVarID(g_iReturnVarID, p->i, snum) == 0)
	{
		p->return_to_center = 9;
		horizAngleAdjust(snum, (sb_snum & SKB_RUN) ? -12 : -24);
	}
}

void playerAimUp(int snum, ESyncBits sb_snum)
{
	auto p = &ps[snum];
	SetGameVarID(g_iReturnVarID, 0, p->i, snum);
	OnEvent(EVENT_AIMUP, p->i, snum, -1);
	if (GetGameVarID(g_iReturnVarID, p->i, snum) == 0)
	{
		horizAngleAdjust(snum, (sb_snum & SKB_RUN) ? 6 : 12);
	}
}

void playerAimDown(int snum, ESyncBits sb_snum)
{
	auto p = &ps[snum];
	SetGameVarID(g_iReturnVarID, 0, p->i, snum);
	OnEvent(EVENT_AIMDOWN, p->i, snum, -1);
	if (GetGameVarID(g_iReturnVarID, p->i, snum) == 0)
	{
		horizAngleAdjust(snum, (sb_snum & SKB_RUN) ? -6 : -12);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int haskey(int sect, int snum)
{
	auto p = &ps[snum];
	if (!g_sectorExtra[sect])
		return 1;
	if (g_sectorExtra[sect] > 6)
		return 1;
	int wk = g_sectorExtra[sect];
	if (wk > 3)
		wk -= 3;

	if (p->keys[wk] == 1)
	{
		g_sectorExtra[sect] = 0;
		return 1;
	}

	return 0;
}

//---------------------------------------------------------------------------
//
// view - as in third person view (stupid name for this function)
//
//---------------------------------------------------------------------------

bool view(struct player_struct* pp, int* vx, int* vy, int* vz, short* vsectnum, int ang, int horiz)
{
	spritetype* sp;
	int i, nx, ny, nz, hx, hy, hitx, hity, hitz;
	short bakcstat, hitsect, hitwall, hitsprite, daang;

	nx = (sintable[(ang + 1536) & 2047] >> 4);
	ny = (sintable[(ang + 1024) & 2047] >> 4);
	nz = (horiz - 100) * 128;

	sp = &sprite[pp->i];

	bakcstat = sp->cstat;
	sp->cstat &= (short)~0x101;

	updatesectorz(*vx, *vy, *vz, vsectnum);
	hitscan(*vx, *vy, *vz, *vsectnum, nx, ny, nz, &hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz, CLIPMASK1);

	if (*vsectnum < 0)
	{
		sp->cstat = bakcstat;
		return false;
	}

	hx = hitx - (*vx); hy = hity - (*vy);
	if (abs(nx) + abs(ny) > abs(hx) + abs(hy))
	{
		*vsectnum = hitsect;
		if (hitwall >= 0)
		{
			daang = getangle(wall[wall[hitwall].point2].x - wall[hitwall].x,
				wall[wall[hitwall].point2].y - wall[hitwall].y);

			i = nx * sintable[daang] + ny * sintable[(daang + 1536) & 2047];
			if (abs(nx) > abs(ny)) hx -= mulscale28(nx, i);
			else hy -= mulscale28(ny, i);
		}
		else if (hitsprite < 0)
		{
			if (abs(nx) > abs(ny)) hx -= (nx >> 5);
			else hy -= (ny >> 5);
		}
		if (abs(nx) > abs(ny)) i = divscale16(hx, nx);
		else i = divscale16(hy, ny);
		if (i < cameradist) cameradist = i;
	}
	*vx = (*vx) + mulscale16(nx, cameradist);
	*vy = (*vy) + mulscale16(ny, cameradist);
	*vz = (*vz) + mulscale16(nz, cameradist);

	cameradist = min(cameradist + (((int)totalclock - cameraclock) << 10), 65536);
	cameraclock = (int)totalclock;

	updatesectorz(*vx, *vy, *vz, vsectnum);

	sp->cstat = bakcstat;
	return true;
}
	 

END_DUKE_NS
