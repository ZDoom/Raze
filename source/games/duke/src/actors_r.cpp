//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2017-2019 Nuke.YKT
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
*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "global.h"
#include "names_r.h"
#include "mapinfo.h"
#include "dukeactor.h"

BEGIN_DUKE_NS

void dojaildoor();
void moveminecart();

void ballreturn(DDukeActor* spr);
void pinsectorresetdown(int sect);
int pinsectorresetup(int sect);
int checkpins(int sect);
void resetpins(int sect);
void resetlanepics(void);


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool ceilingspace_r(sectortype* sectp)
{
	if( (sectp->ceilingstat&1) && sectp->ceilingpal == 0 )
	{
		switch(sectp->ceilingpicnum)
		{
			case MOONSKY1:
			case BIGORBIT1:
				return 1;
		}
	}
	return 0;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool floorspace_r(sectortype* sectp)
{
	if( (sectp->floorstat&1) && sectp->ceilingpal == 0 )
	{
		switch(sectp->floorpicnum)
		{
			case MOONSKY1:
			case BIGORBIT1:
				return 1;
		}
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void check_fta_sounds_r(DDukeActor* actor)
{
	if (actor->s->extra > 0) switch (actor->s->picnum)
	{
	case COOT: // LIZTROOP
		if (!isRRRA() && (krand() & 3) == 2)
			S_PlayActorSound(PRED_RECOG, actor);
		break;
	case BILLYCOCK:
	case BILLYRAY:
	case BRAYSNIPER: // PIGCOP
		S_PlayActorSound(PIG_RECOG, actor);
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void addweapon_r(struct player_struct* p, int weapon)
{
	int cw = p->curr_weapon;
	if (p->OnMotorcycle || p->OnBoat)
	{
		p->gotweapon[weapon] = true;;
		if (weapon == THROWSAW_WEAPON)
		{
			p->gotweapon[BUZZSAW_WEAPON] = true;
			p->ammo_amount[BUZZSAW_WEAPON] = 1;
		}
		else if (weapon == CROSSBOW_WEAPON)
		{
			p->gotweapon[CHICKEN_WEAPON] = true;
			p->gotweapon[DYNAMITE_WEAPON] = true;
		}
		else if (weapon == SLINGBLADE_WEAPON)
		{
			p->ammo_amount[SLINGBLADE_WEAPON] = 1;
		}
		return;
	}

	if (p->gotweapon[weapon] == 0)
	{
		p->gotweapon[weapon] = true;;
		if (weapon == THROWSAW_WEAPON)
		{
			p->gotweapon[BUZZSAW_WEAPON] = true;
			p->ammo_amount[BUZZSAW_WEAPON] = 1;
		}
		if (isRRRA())
		{
			if (weapon == CROSSBOW_WEAPON)
			{
				p->gotweapon[CHICKEN_WEAPON] = true;
			}
			if (weapon == SLINGBLADE_WEAPON)
			{
				p->ammo_amount[SLINGBLADE_WEAPON] = 50;
			}
		}
		if (weapon == CROSSBOW_WEAPON)
		{
			p->gotweapon[DYNAMITE_WEAPON] = true;
		}

		if (weapon != DYNAMITE_WEAPON)
			cw = weapon;
	}
	else
		cw = weapon;

	if (weapon == DYNAMITE_WEAPON)
		p->last_weapon = -1;

	p->random_club_frame = 0;

	if (p->holster_weapon == 0)
	{
		p->weapon_pos = -1;
		p->last_weapon = p->curr_weapon;
	}
	else
	{
		p->weapon_pos = 10;
		p->holster_weapon = 0;
		p->last_weapon = -1;
	}

	p->okickback_pic = p->kickback_pic = 0;
	p->curr_weapon = cw;
	p->wantweaponfire = -1;

	switch (weapon)
	{
	case SLINGBLADE_WEAPON:
		if (!isRRRA()) break;
	case KNEE_WEAPON:
	case DYNAMITE_WEAPON:	 
	case TRIPBOMB_WEAPON:
	case THROWINGDYNAMITE_WEAPON:
		break;
	case SHOTGUN_WEAPON:	  
		S_PlayActorSound(SHOTGUN_COCK, p->GetActor()); 
		break;
	case PISTOL_WEAPON:	   
		S_PlayActorSound(INSERT_CLIP, p->GetActor());
		break;
	default:	  
		S_PlayActorSound(EJECT_CLIP, p->GetActor());
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void hitradius_r(DDukeActor* actor, int  r, int  hp1, int  hp2, int  hp3, int  hp4)
{
	walltype* wal;
	sectortype* dasectp;
	int d, q, x1, y1;
	int sectcnt, sectend, dasect, startwall, endwall, nextsect;
	int p, x;
	int sect;
	static const uint8_t statlist[] = { STAT_DEFAULT, STAT_ACTOR, STAT_STANDABLE, STAT_PLAYER, STAT_FALLER, STAT_ZOMBIEACTOR, STAT_MISC };
	int tempsect[128];	// originally hijacked a global buffer which is bad. Q: How many do we really need? RedNukem says 64.

	auto spri = actor->s;

	if (spri->xrepeat < 11)
	{
		if (spri->picnum == RPG || ((isRRRA()) && spri->picnum == RPG2)) goto SKIPWALLCHECK;
	}

	tempsect[0] = spri->sectnum;
	dasect = spri->sectnum;
	dasectp = spri->sector();
	sectcnt = 0; sectend = 1;

	do
	{
		dasect = tempsect[sectcnt++];
		dasectp = &sector[dasect];
		if (((dasectp->ceilingz - spri->z) >> 8) < r)
		{
			d = abs(wall[dasectp->wallptr].x - spri->x) + abs(wall[dasectp->wallptr].y - spri->y);
			if (d < r)
				fi.checkhitceiling(dasect);
			else
			{
				// ouch...
				d = abs(wall[wall[wall[dasectp->wallptr].point2].point2].x - spri->x) + abs(wall[wall[wall[dasectp->wallptr].point2].point2].y - spri->y);
				if (d < r)
					fi.checkhitceiling(dasect);
			}
		}

		startwall = dasectp->wallptr;
		endwall = startwall + dasectp->wallnum;
		for (x = startwall, wal = &wall[startwall]; x < endwall; x++, wal++)
			if ((abs(wal->x - spri->x) + abs(wal->y - spri->y)) < r)
			{
				nextsect = wal->nextsector;
				if (nextsect >= 0)
				{
					for (dasect = sectend - 1; dasect >= 0; dasect--)
						if (tempsect[dasect] == nextsect) break;
					if (dasect < 0) tempsect[sectend++] = nextsect;
				}
				x1 = (((wal->x + wall[wal->point2].x) >> 1) + spri->x) >> 1;
				y1 = (((wal->y + wall[wal->point2].y) >> 1) + spri->y) >> 1;
				updatesector(x1, y1, &sect);
				if (sect >= 0 && cansee(x1, y1, spri->z, sect, spri->x, spri->y, spri->z, spri->sectnum))
					fi.checkhitwall(actor, x, wal->x, wal->y, spri->z, spri->picnum);
			}
	} while (sectcnt < sectend && sectcnt < (int)countof(tempsect));

SKIPWALLCHECK:

	q = -(24 << 8) + (krand() & ((32 << 8) - 1));

	auto Owner = actor->GetOwner();
	for (x = 0; x < 7; x++)
	{
		DukeStatIterator it1(statlist[x]);
		while (auto act2 = it1.Next())
		{
			auto spri2 = act2->s;
			if (x == 0 || x >= 5 || AFLAMABLE(spri2->picnum))
			{
				if (spri2->cstat & 257)
					if (dist(actor, act2) < r)
					{
						if (badguy(act2) && !cansee(spri2->x, spri2->y, spri2->z + q, spri2->sectnum, spri->x, spri->y, spri->z + q, spri->sectnum))
						{
							continue;
						}
						fi.checkhitsprite(act2, actor);
					}
			}
			else if (spri2->extra >= 0 && act2 != actor && (badguy(act2) || spri2->picnum == QUEBALL || spri2->picnum == BOWLINGPIN || spri2->picnum == STRIPEBALL || (spri2->cstat & 257) || spri2->picnum == DUKELYINGDEAD))
			{
				if (spri->picnum == MORTER && act2 == Owner)
				{
					continue;
				}
				if ((isRRRA()) && spri->picnum == CHEERBOMB && act2 == Owner)
				{
					continue;
				}

				if (spri2->picnum == APLAYER) spri2->z -= gs.playerheight;
				d = dist(actor, act2);
				if (spri2->picnum == APLAYER) spri2->z += gs.playerheight;

				if (d < r && cansee(spri2->x, spri2->y, spri2->z - (8 << 8), spri2->sectnum, spri->x, spri->y, spri->z - (12 << 8), spri->sectnum))
				{
					if ((isRRRA()) && spri2->picnum == MINION && spri2->pal == 19)
					{
						continue;
					}

					act2->ang = getangle(spri2->x - spri->x, spri2->y - spri->y);

					if (spri->picnum == RPG && spri2->extra > 0)
						act2->picnum = RPG;
					else if ((isRRRA()) && spri->picnum == RPG2 && spri2->extra > 0)
						act2->picnum = RPG;
					else
						act2->picnum = RADIUSEXPLOSION;

					if (d < r / 3)
					{
						if (hp4 == hp3) hp4++;
						act2->extra = hp3 + (krand() % (hp4 - hp3));
					}
					else if (d < 2 * r / 3)
					{
						if (hp3 == hp2) hp3++;
						act2->extra = hp2 + (krand() % (hp3 - hp2));
					}
					else if (d < r)
					{
						if (hp2 == hp1) hp2++;
						act2->extra = hp1 + (krand() % (hp2 - hp1));
					}

					int pic = spri2->picnum;
					if ((isRRRA())? 
						(pic != HULK && pic != MAMA && pic != BILLYPLAY && pic != COOTPLAY && pic != MAMACLOUD) :
						(pic != HULK && pic != SBMOVE))
					{
						if (spri2->xvel < 0) spri2->xvel = 0;
						spri2->xvel += (spri2->extra << 2);
					}

					if (spri2->picnum == STATUEFLASH || spri2->picnum == QUEBALL ||
						spri2->picnum == STRIPEBALL || spri2->picnum == BOWLINGPIN)
						fi.checkhitsprite(act2, actor);

					if (spri2->picnum != RADIUSEXPLOSION &&
						Owner && Owner->s->statnum < MAXSTATUS)
					{
						if (spri2->picnum == APLAYER)
						{
							p = act2->PlayerIndex();
							if (ps[p].newOwner != nullptr)
							{
								clearcamera(&ps[p]);
							}
						}
						act2->SetHitOwner(actor->GetOwner());
					}
				}
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int movesprite_ex_r(DDukeActor* actor, int xchange, int ychange, int zchange, unsigned int cliptype, Collision &result)
{
	int dasectnum;
	int clipdist;
	auto spri = actor->s;
	int bg = badguy(actor);

	if (spri->statnum == 5 || (bg && spri->xrepeat < 4))
	{
		spri->x += (xchange * TICSPERFRAME) >> 2;
		spri->y += (ychange * TICSPERFRAME) >> 2;
		spri->z += (zchange * TICSPERFRAME) >> 2;
		if (bg)
			setsprite(actor, spri->x, spri->y, spri->z);
		return result.setNone();
	}

	dasectnum = spri->sectnum;
	auto dasectp = spri->sector();

	vec3_t pos = spri->pos;
	pos.z -= ((tileHeight(spri->picnum) * spri->yrepeat) << 1);

	if (bg)
	{
		if (spri->xrepeat > 60)
			clipmove_ex(&pos, &dasectnum, ((xchange * TICSPERFRAME) << 11), ((ychange * TICSPERFRAME) << 11), 1024, (4 << 8), (4 << 8), cliptype, result);
		else
		{
			clipdist = 192;
			clipmove_ex(&pos, &dasectnum, ((xchange * TICSPERFRAME) << 11), ((ychange * TICSPERFRAME) << 11), clipdist, (4 << 8), (4 << 8), cliptype, result);
		}

		if (dasectnum < 0 || (dasectnum >= 0 && actor->actorstayput >= 0 && actor->actorstayput != dasectnum))
		{
			if (dasectp->lotag == ST_1_ABOVE_WATER)
				spri->ang = (krand() & 2047);
			else if ((actor->temp_data[0] & 3) == 1)
				spri->ang = (krand() & 2047);
			setsprite(actor, spri->pos);
			if (dasectnum < 0) dasectnum = 0;
			return result.setSector(dasectnum);
		}
		if ((result.type == kHitSector || result.type == kHitSprite) && (actor->cgg == 0)) spri->ang += 768;
	}
	else
	{
		if (spri->statnum == STAT_PROJECTILE)
			clipmove_ex(&pos, &dasectnum, ((xchange * TICSPERFRAME) << 11), ((ychange * TICSPERFRAME) << 11), 8, (4 << 8), (4 << 8), cliptype, result);
		else
			clipmove_ex(&pos, &dasectnum, ((xchange * TICSPERFRAME) << 11), ((ychange * TICSPERFRAME) << 11), 128, (4 << 8), (4 << 8), cliptype, result);
	}
	spri->x = pos.x;
	spri->y = pos.y;

	if (dasectnum >= 0)
		if ((dasectnum != spri->sectnum))
			changeactorsect(actor, dasectnum);
	int daz = spri->z + ((zchange * TICSPERFRAME) >> 3);
	if ((daz > actor->ceilingz) && (daz <= actor->floorz))
		spri->z = daz;
	else if (result.type == kHitNone)
		return result.setSector(dasectnum);

	return result.type;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void lotsoffeathers_r(DDukeActor *actor, int n)
{
	lotsofstuff(actor, n, MONEY);
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void guts_r(DDukeActor* actor, int gtype, int n, int p)
{
	auto s = actor->s;
	int gutz, floorz;
	int i=0, j;
	int sx, sy;
	uint8_t pal;

	if (badguy(actor) && s->xrepeat < 16)
		sx = sy = 8;
	else sx = sy = 32;

	gutz = s->z - (8 << 8);
	floorz = getflorzofslope(s->sectnum, s->x, s->y);

	if (gutz > (floorz - (8 << 8)))
		gutz = floorz - (8 << 8);

	gutz += gs.actorinfo[s->picnum].gutsoffset;

	if (badguy(actor) && s->pal == 6)
		pal = 6;
	else
	{
		pal = 0;
		if (isRRRA())
		{
			if (s->picnum == MINION && (s->pal == 8 || s->pal == 19)) pal = s->pal;
		}
	}

	for (j = 0; j < n; j++)
	{
		// RANDCORRECT version from RR.
		int a = krand() & 2047;
		int r1 = krand();
		int r2 = krand();
		int r3 = krand();
		int r4 = krand();
		int r5 = krand();
		// TRANSITIONAL: owned by a player???
		auto spawned = EGS(s->sectnum, s->x + (r5 & 255) - 128, s->y + (r4 & 255) - 128, gutz - (r3 & 8191), gtype, -32, sx >> 1, sy >> 1, a, 48 + (r2 & 31), -512 - (r1 & 2047), ps[p].GetActor(), 5);
		if (pal != 0)
			spawned->s->pal = pal;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------


void movefta_r(void)
{
	int x, px, py, sx, sy;
	int j, p;
	int psect, ssect;

	DukeStatIterator it(STAT_ZOMBIEACTOR);
	while(auto act = it.Next())
	{
		auto s = act->s;
		p = findplayer(act, &x);
		j = 0;

		ssect = psect = s->sectnum;

		if (ps[p].GetActor()->s->extra > 0)
		{
			if (x < 30000)
			{
				act->timetosleep++;
				if (act->timetosleep >= (x >> 8))
				{
					if (badguy(act))
					{
						px = ps[p].oposx + 64 - (krand() & 127);
						py = ps[p].oposy + 64 - (krand() & 127);
						updatesector(px, py, &psect);
						if (psect == -1)
						{
							continue;
						}
						sx = s->x + 64 - (krand() & 127);
						sy = s->y + 64 - (krand() & 127);
						updatesector(px, py, &ssect);
						if (ssect == -1)
						{
							continue;
						}

						if (s->pal == 33 || s->picnum == VIXEN ||  
							(isRRRA() && (isIn(s->picnum, COOT, COOTSTAYPUT, BIKER, BIKERB, BIKERBV2, CHEER, CHEERB,
								CHEERSTAYPUT, MINIONBOAT, HULKBOAT, CHEERBOAT, RABBIT, COOTPLAY, BILLYPLAY, MAKEOUT, MAMA)
								|| (s->picnum == MINION && s->pal == 8)))
							 || (bcos(s->ang) * (px - sx) + bsin(s->ang) * (py - sy) >= 0))
						{
							int r1 = krand();
							int r2 = krand();
							j = cansee(sx, sy, s->z - (r2 % (52 << 8)), s->sectnum, px, py, ps[p].oposz - (r1 % (32 << 8)), ps[p].cursectnum);
						}
					}
					else
					{
						int r1 = krand();
						int r2 = krand();
						j = cansee(s->x, s->y, s->z - ((r2 & 31) << 8), s->sectnum, ps[p].oposx, ps[p].oposy, ps[p].oposz - ((r1 & 31) << 8), ps[p].cursectnum);
					}


					if (j) switch (s->picnum)
					{
					case RUBBERCAN:
					case EXPLODINGBARREL:
					case WOODENHORSE:
					case HORSEONSIDE:
					case CANWITHSOMETHING:
					case FIREBARREL:
					case FIREVASE:
					case NUKEBARREL:
					case NUKEBARRELDENTED:
					case NUKEBARRELLEAKED:
						if (s->sector()->ceilingstat & 1)
							s->shade = s->sector()->ceilingshade;
						else s->shade = s->sector()->floorshade;

						act->timetosleep = 0;
						changeactorstat(act, STAT_STANDABLE);
						break;
					default:
#if 0
						// TRANSITIONAL: RedNukem has this here. Needed?
						if (actorflag(act, SFLAG_USEACTIVATOR) && sector [act->s.lotag & 16384) break;
#endif
						act->timetosleep = 0;
						check_fta_sounds_r(act);
						changeactorstat(act, STAT_ACTOR);
						break;
					}
					else act->timetosleep = 0;
				}
			}
			if (/*!j &&*/ badguy(act)) // this is like RedneckGDX. j is uninitialized here, i.e. most likely not 0.
			{
				if (s->sector()->ceilingstat & 1)
					s->shade = s->sector()->ceilingshade;
				else s->shade = s->sector()->floorshade;

				if (s->picnum == HEN || s->picnum == COW || s->picnum == PIG || s->picnum == DOGRUN || ((isRRRA()) && s->picnum == RABBIT))
				{
					if (wakeup(act, p))
					{
						act->timetosleep = 0;
						check_fta_sounds_r(act);
						changeactorstat(act, STAT_ACTOR);
					}
				}
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

DDukeActor* ifhitsectors_r(int sectnum)
{
	DukeStatIterator it(STAT_MISC);
	while (auto a1 = it.Next())
	{
		if (a1->s->picnum == EXPLOSION2 || (a1->s->picnum == EXPLOSION3 && sectnum == a1->s->sectnum))
			return a1;
	}
	return nullptr;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int ifhitbyweapon_r(DDukeActor *actor)
{
	int p;
	auto hitowner = actor->GetHitOwner();
	auto spri = actor->s;

	if (actor->extra >= 0)
	{
		if (spri->extra >= 0)
		{
			if (spri->picnum == APLAYER)
			{
				if (ud.god) return -1;

				p = actor->PlayerIndex();

				if (hitowner &&
					hitowner->s->picnum == APLAYER &&
					ud.coop == 1 &&
					ud.ffire == 0)
					return -1;

				spri->extra -= actor->extra;

				if (hitowner)
				{
					if (spri->extra <= 0 && actor->picnum != FREEZEBLAST)
					{
						spri->extra = 0;

						ps[p].wackedbyactor = hitowner;

						if (hitowner->s->picnum == APLAYER && p != hitowner->PlayerIndex())
						{
							ps[p].frag_ps = hitowner->PlayerIndex();
						}
						actor->SetHitOwner(ps[p].GetActor());
					}
				}

				int pn = actor->picnum;
				if (pn == RPG2 && !isRRRA()) pn = 0; // avoid messing around with gotos.
				switch (pn)
				{
				case RADIUSEXPLOSION:
				case RPG:
				case HYDRENT:
				case HEAVYHBOMB:
				case SEENINE:
				case OOZFILTER:
				case EXPLODINGBARREL:
				case TRIPBOMBSPRITE:
				case RPG2:
					ps[p].posxv += actor->extra * bcos(actor->ang, 2);
					ps[p].posyv += actor->extra * bsin(actor->ang, 2);
					break;
				default:
					ps[p].posxv += actor->extra * bcos(actor->ang, 1);
					ps[p].posyv += actor->extra * bsin(actor->ang, 1);
					break;
				}
			}
			else
			{
				if (actor->extra == 0)
					if (spri->xrepeat < 24)
						return -1;

				spri->extra -= actor->extra;
				if (spri->picnum != RECON && actor->GetOwner() && actor->GetOwner()->s->statnum < MAXSTATUS)
					actor->SetOwner(hitowner);
			}

			actor->extra = -1;
			return actor->picnum;
		}
	}

	actor->extra = -1;
	return -1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void respawn_rrra(DDukeActor* oldact, DDukeActor* newact)
{
	auto newspr = newact->s;
	newspr->pal = oldact->s->pal;
	if (newspr->picnum == MAMA)
	{
		if (newspr->pal == 30)
		{
			newspr->xrepeat = 26;
			newspr->yrepeat = 26;
			newspr->clipdist = 75;
		}
		else if (newspr->pal == 31)
		{
			newspr->xrepeat = 36;
			newspr->yrepeat = 36;
			newspr->clipdist = 100;
		}
		else if (newspr->pal == 32)
		{
			newspr->xrepeat = 50;
			newspr->yrepeat = 50;
			newspr->clipdist = 100;
		}
		else
		{
			newspr->xrepeat = 50;
			newspr->yrepeat = 50;
			newspr->clipdist = 100;
		}
	}

	if (newspr->pal == 8)
	{
		newspr->cstat |= 2;
	}

	if (newspr->pal != 6)
	{
		deletesprite(oldact);
		return;
	}
	oldact->s->extra = (66 - 13);
	newspr->pal = 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movefallers_r(void)
{
	DukeStatIterator it(STAT_FALLER);
	while (auto act = it.Next())
	{
		auto s = act->s;
		auto sectp = s->sector();

		if (act->temp_data[0] == 0)
		{
			s->z -= (16 << 8);
			act->temp_data[1] = s->ang;
			int x = s->extra;
			int j = fi.ifhitbyweapon(act);
			if (j >= 0)
			{
				if (j == RPG || (isRRRA() && j == RPG2) || j == RADIUSEXPLOSION || j == SEENINE || j == OOZFILTER)
				{
					if (s->extra <= 0)
					{
						act->temp_data[0] = 1;
						DukeStatIterator it(STAT_FALLER);
						while (auto ac2 = it.Next())
						{
							if (ac2->s->hitag == s->hitag)
							{
								ac2->temp_data[0] = 1;
								ac2->s->cstat &= (65535 - 64);
								if (ac2->s->picnum == CEILINGSTEAM || ac2->s->picnum == STEAM)
									ac2->s->cstat |= 32768;
							}
						}
					}
				}
				else
				{
					act->extra = 0;
					s->extra = x;
				}
			}
			s->ang = act->temp_data[1];
			s->z += (16 << 8);
		}
		else if (act->temp_data[0] == 1)
		{
			if (s->lotag > 0)
			{
				s->lotag -= 3;
				s->xvel = ((64 + krand()) & 127);
				s->zvel = -(1024 + (krand() & 1023));
			}
			else
			{
				if (s->xvel > 0)
				{
					s->xvel -= 2;
					ssp(act, CLIPMASK0);
				}

				int x;
				if (fi.floorspace(s->sector())) x = 0;
				else
				{
					if (fi.ceilingspace(s->sector()))
						x = gs.gravity / 6;
					else
						x = gs.gravity;
				}

				if (s->z < (sectp->floorz - FOURSLEIGHT))
				{
					s->zvel += x;
					if (s->zvel > 6144)
						s->zvel = 6144;
					s->z += s->zvel;
				}
				if ((sectp->floorz - s->z) < (16 << 8))
				{
					int j = 1 + (krand() & 7);
					for (x = 0; x < j; x++) RANDOMSCRAP(act);
					deletesprite(act);
				}
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// split out of movestandables
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void movecrack(DDukeActor* actor)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	if (s->hitag > 0)
	{
		t[0] = s->cstat;
		t[1] = s->ang;
		int j = fi.ifhitbyweapon(actor);
		if (j == RPG || (isRRRA() && j == RPG2) || j == RADIUSEXPLOSION || j == SEENINE || j == OOZFILTER)
		{
			DukeStatIterator it(STAT_STANDABLE);
			while (auto a1 = it.Next())
			{
				if (s->hitag == a1->s->hitag && (a1->s->picnum == OOZFILTER || a1->s->picnum == SEENINE))
					if (a1->s->shade != -32)
						a1->s->shade = -32;
			}
			detonate(actor, EXPLOSION2);
		}
		else
		{
			s->cstat = t[0];
			s->ang = t[1];
			s->extra = 0;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void movebolt(DDukeActor* actor)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	int x;
	auto sectp = s->sector();

	auto p = findplayer(actor, &x);
	if (x > 20480) return;

	if (t[3] == 0)
		t[3] = sectp->floorshade;

CLEAR_THE_BOLT:
	if (t[2])
	{
		t[2]--;
		sectp->floorshade = 20;
		sectp->ceilingshade = 20;
		return;
	}
	if ((s->xrepeat | s->yrepeat) == 0)
	{
		s->xrepeat = t[0];
		s->yrepeat = t[1];
	}
	else if ((krand() & 8) == 0)
	{
		t[0] = s->xrepeat;
		t[1] = s->yrepeat;
		t[2] = global_random & 4;
		s->xrepeat = s->yrepeat = 0;
		goto CLEAR_THE_BOLT;
	}
	s->picnum++;

	int l = global_random & 7;
	s->xrepeat = l + 8;

	if (l & 1) s->cstat ^= 2;

	if (s->picnum == (BOLT1 + 1) && (krand() & 1) && sectp->floorpicnum == HURTRAIL)
		S_PlayActorSound(SHORT_CIRCUIT, actor);

	if (s->picnum == BOLT1 + 4) s->picnum = BOLT1;

	if (s->picnum & 1)
	{
		sectp->floorshade = 0;
		sectp->ceilingshade = 0;
	}
	else
	{
		sectp->floorshade = 20;
		sectp->ceilingshade = 20;
	}
}

//---------------------------------------------------------------------------
//
// this has been broken up into lots of smaller subfunctions
//
//---------------------------------------------------------------------------

void movestandables_r(void)
{
	DukeStatIterator it(STAT_STANDABLE);
	while (auto act = it.Next())
	{
		int picnum = act->s->picnum;

		if (act->s->sectnum < 0)
		{
			deletesprite(act);
			continue;
		}

		if (picnum >= CRANE && picnum <= CRANE +3)
		{
			movecrane(act, CRANE);
		}

		else if (picnum >= WATERFOUNTAIN && picnum <= WATERFOUNTAIN + 3)
		{
			movefountain(act, WATERFOUNTAIN);
		}

		else if (AFLAMABLE(picnum))
		{
			moveflammable(act, TIRE, BOX, BLOODPOOL);
		}


		else if (picnum >= CRACK1 && picnum <= CRACK1 + 3)
		{
			movecrack(act);
		}

		else if (picnum == OOZFILTER || picnum == SEENINE || picnum == SEENINEDEAD || picnum == (SEENINEDEAD + 1))
		{
			moveooz(act, SEENINE, SEENINEDEAD, OOZFILTER, EXPLOSION2);
		}

		else if (picnum == MASTERSWITCH)
		{
			movemasterswitch(act, SEENINE, OOZFILTER);
		}

		else if (picnum == TRASH)
		{
			movetrash(act);
		}

		else if (picnum >= BOLT1 && picnum <= BOLT1 + 3)
		{
			movebolt(act);
		}

		else if (picnum == WATERDRIP)
		{
			movewaterdrip(act, WATERDRIP);
		}

		else if (picnum == DOORSHOCK)
		{
			movedoorshock(act);
		}

		else if (picnum == TOUCHPLATE)
		{
			movetouchplate(act, TOUCHPLATE);
		}

		else if (picnum == CANWITHSOMETHING)
		{
			movecanwithsomething(act);
		}

		else if (isIn(picnum,
				EXPLODINGBARREL,
				WOODENHORSE,
				HORSEONSIDE,
				FIREBARREL,
				FIREVASE,
				NUKEBARREL,
				NUKEBARRELDENTED,
				NUKEBARRELLEAKED,
				TOILETWATER,
				RUBBERCAN,
				STEAM,
				CEILINGSTEAM))
		{
			int x;
			int p = findplayer(act, &x);
			execute(act, p, x);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void chickenarrow(DDukeActor* actor)
{
	auto s = actor->s;
	s->hitag++;
	if (actor->picnum != BOSS2 && s->xrepeat >= 10 && s->sector()->lotag != 2)
	{
		auto spawned = spawn(actor, SMALLSMOKE);
		spawned->s->z += (1 << 8);
		if ((krand() & 15) == 2)
		{
			spawn(actor, MONEY);
		}
	}
	auto ts = actor->seek_actor;
	if (!ts) return;

	if (ts->s->extra <= 0)
		actor->seek_actor = nullptr;
	if (actor->seek_actor && s->hitag > 5)
	{
		int ang, ang2, ang3;
		ang = getangle(ts->s->x - s->x, ts->s->y - s->y);
		ang2 = ang - s->ang;
		ang3 = abs(ang2);
		if (ang2 < 100)
		{
			if (ang3 > 1023)
				s->ang += 51;
			else
				s->ang -= 51;
		}
		else if (ang2 > 100)
		{
			if (ang3 > 1023)
				s->ang -= 51;
			else
				s->ang += 51;
		}
		else
			s->ang = ang;

		if (s->hitag > 180)
			if (s->zvel <= 0)
				s->zvel += 200;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static bool weaponhitsprite(DDukeActor *proj, DDukeActor *targ, const vec3_t &oldpos)
{
	auto s = proj->s;
	if (isRRRA())
	{
		if (targ->s->picnum == MINION
			&& (s->picnum == RPG || s->picnum == RPG2)
			&& targ->s->pal == 19)
		{
			S_PlayActorSound(RPG_EXPLODE, proj);
			spawn(proj, EXPLOSION2)->s->pos = oldpos;
			return true;
		}
	}
	else if (s->picnum == FREEZEBLAST && targ->s->pal == 1)
		if (badguy(targ) || targ->s->picnum == APLAYER)
		{
			auto star = spawn(proj, TRANSPORTERSTAR);
			star->s->pal = 1;
			star->s->xrepeat = 32;
			star->s->yrepeat = 32;

			deletesprite(proj);
			return true;
		}

	fi.checkhitsprite(targ, proj);

	if (targ->s->picnum == APLAYER)
	{
		int p = targ->s->yvel;
		S_PlayActorSound(PISTOL_BODYHIT, targ);

		if (s->picnum == SPIT)
		{
			if (isRRRA() && proj->GetOwner() && proj->GetOwner()->s->picnum == MAMA)
			{
				guts_r(proj, RABBITJIBA, 2, myconnectindex);
				guts_r(proj, RABBITJIBB, 2, myconnectindex);
				guts_r(proj, RABBITJIBC, 2, myconnectindex);
			}

			ps[p].horizon.addadjustment(32);
			ps[p].sync.actions |= SB_CENTERVIEW;

			if (ps[p].loogcnt == 0)
			{
				if (!S_CheckActorSoundPlaying(ps[p].GetActor(), DUKE_LONGTERM_PAIN))
					S_PlayActorSound(DUKE_LONGTERM_PAIN, ps[p].GetActor());

				int j = 3 + (krand() & 3);
				ps[p].numloogs = j;
				ps[p].loogcnt = 24 * 4;
				for (int x = 0; x < j; x++)
				{
					ps[p].loogiex[x] = krand() % 320;
					ps[p].loogiey[x] = krand() % 200;
				}
			}
		}
	}
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static bool weaponhitwall(DDukeActor *proj, int wal, const vec3_t& oldpos)
{
	auto s = proj->s;
	if (isRRRA() && proj->GetOwner() && proj->GetOwner()->s->picnum == MAMA)
	{
		guts_r(proj, RABBITJIBA, 2, myconnectindex);
		guts_r(proj, RABBITJIBB, 2, myconnectindex);
		guts_r(proj, RABBITJIBC, 2, myconnectindex);
	}

	if (s->picnum != RPG && (!isRRRA() || s->picnum != RPG2) && s->picnum != FREEZEBLAST && s->picnum != SPIT && s->picnum != SHRINKSPARK && (wall[wal].overpicnum == MIRROR || wall[wal].picnum == MIRROR))
	{
		int k = getangle(
			wall[wall[wal].point2].x - wall[wal].x,
			wall[wall[wal].point2].y - wall[wal].y);
		s->ang = ((k << 1) - s->ang) & 2047;
		proj->SetOwner(proj);
		spawn(proj, TRANSPORTERSTAR);
		return true;
	}
	else
	{
		setsprite(proj, oldpos);
		fi.checkhitwall(proj, wal, s->x, s->y, s->z, s->picnum);

		if (!isRRRA() && s->picnum == FREEZEBLAST)
		{
			if (wall[wal].overpicnum != MIRROR && wall[wal].picnum != MIRROR)
			{
				s->extra >>= 1;
				if (s->xrepeat > 8)
					s->xrepeat -= 2;
				if (s->yrepeat > 8)
					s->yrepeat -= 2;
				s->yvel--;
			}

			int k = getangle(
				wall[wall[wal].point2].x - wall[wal].x,
				wall[wall[wal].point2].y - wall[wal].y);
			s->ang = ((k << 1) - s->ang) & 2047;
			return true;
		}
		if (s->picnum == SHRINKSPARK)
		{
			if (wall[wal].picnum >= RRTILE3643 && wall[wal].picnum < RRTILE3643 + 3)
			{
				deletesprite(proj);
			}
			if (s->extra <= 0)
			{
				s->x += bcos(s->ang, -7);
				s->y += bsin(s->ang, -7);
				auto Owner = proj->GetOwner();
				if (!isRRRA() || !Owner || (Owner->s->picnum != CHEER && Owner->s->picnum != CHEERSTAYPUT))
				{
					auto j = spawn(proj, CIRCLESTUCK);
					j->s->xrepeat = 8;
					j->s->yrepeat = 8;
					j->s->cstat = 16;
					j->s->ang = (j->s->ang + 512) & 2047;
					j->s->clipdist = MulScale(s->xrepeat, tileWidth(s->picnum), 7);
				}
				deletesprite(proj);
				return true;
			}
			if (wall[wal].overpicnum != MIRROR && wall[wal].picnum != MIRROR)
			{
				s->extra -= 20;
				s->yvel--;
			}

			int k = getangle(
				wall[wall[wal].point2].x - wall[wal].x,
				wall[wall[wal].point2].y - wall[wal].y);
			s->ang = ((k << 1) - s->ang) & 2047;
			return true;
		}
	}
	return false;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool weaponhitsector(DDukeActor *proj, const vec3_t& oldpos)
{
	auto s = proj->s;
	setsprite(proj, oldpos);

	if (isRRRA() && proj->GetOwner() && proj->GetOwner()->s->picnum == MAMA)
	{
		guts_r(proj, RABBITJIBA, 2, myconnectindex);
		guts_r(proj, RABBITJIBB, 2, myconnectindex);
		guts_r(proj, RABBITJIBC, 2, myconnectindex);
	}

	if (s->zvel < 0)
	{
		if (s->sector()->ceilingstat & 1)
			if (s->sector()->ceilingpal == 0)
			{
				deletesprite(proj);
				return true;
			}

		fi.checkhitceiling(s->sectnum);
	}

	if (!isRRRA() && s->picnum == FREEZEBLAST)
	{
		bounce(proj);
		ssp(proj, CLIPMASK1);
		s->extra >>= 1;
		if (s->xrepeat > 8)
			s->xrepeat -= 2;
		if (s->yrepeat > 8)
			s->yrepeat -= 2;
		s->yvel--;
		return true;
	}
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void weaponcommon_r(DDukeActor *proj)
{
	auto s = proj->s;
	int k, p;
	int ll;

	p = -1;

	if (s->picnum == RPG && s->sector()->lotag == 2)
	{
		k = s->xvel >> 1;
		ll = s->zvel >> 1;
	}
	else if (isRRRA() && s->picnum == RPG2 && s->sector()->lotag == 2)
	{
		k = s->xvel >> 1;
		ll = s->zvel >> 1;
	}
	else
	{
		k = s->xvel;
		ll = s->zvel;
	}

	auto oldpos = s->pos;

	getglobalz(proj);

	switch (s->picnum)
	{
	case RPG:
		if (proj->picnum != BOSS2 && s->xrepeat >= 10 && s->sector()->lotag != 2)
		{
			spawn(proj, SMALLSMOKE)->s->z += (1 << 8);
		}
		break;
	case RPG2:
		if (!isRRRA()) break;
		chickenarrow(proj);
		break;

	case RRTILE1790:
		if (!isRRRA()) break;
		if (s->extra)
		{
			s->zvel = -(s->extra * 250);
			s->extra--;
		}
		else
			makeitfall(proj);
		if (s->xrepeat >= 10 && s->sector()->lotag != 2)
		{
			spawn(proj, SMALLSMOKE)->s->z += (1 << 8);
		}
		break;
	}

	Collision coll;
	movesprite_ex(proj,
		MulScale(k, bcos(s->ang), 14),
		MulScale(k, bsin(s->ang), 14), ll, CLIPMASK1, coll);

	if ((s->picnum == RPG || (isRRRA() && isIn(s->picnum, RPG2, RRTILE1790))) && proj->temp_actor != nullptr)
		if (FindDistance2D(s->x - proj->temp_actor->s->x, s->y - proj->temp_actor->s->y) < 256)
			coll.setSprite(proj->temp_actor);

	if (s->sectnum < 0) // || (isRR() && s->sector()->filler == 800))
	{
		deletesprite(proj);
		return;
	}

	if (coll.type != kHitSprite && s->picnum != FREEZEBLAST)
	{
		if (s->z < proj->ceilingz)
		{
			coll.setSector(s->sectnum);
			s->zvel = -1;
		}
		else
			if (s->z > proj->floorz)
			{
				coll.setSector(s->sectnum);
				if (s->sector()->lotag != 1)
					s->zvel = 1;
			}
	}

	if (s->picnum == FIRELASER)
	{
		for (k = -3; k < 2; k++)
		{
			auto x = EGS(s->sectnum,
				s->x + MulScale(k, bcos(s->ang), 9),
				s->y + MulScale(k, bsin(s->ang), 9),
				s->z + ((k * Sgn(s->zvel)) * abs(s->zvel / 24)), FIRELASER, -40 + (k << 2),
				s->xrepeat, s->yrepeat, 0, 0, 0, proj->GetOwner(), 5);

			x->s->cstat = 128;
			x->s->pal = s->pal;
		}
	}
	else if (s->picnum == SPIT) if (s->zvel < 6144)
		s->zvel += gs.gravity - 112;

	if (coll.type != 0)
	{
		if (coll.type == kHitSprite)
		{
			if (weaponhitsprite(proj, coll.actor, oldpos)) return;
		}
		else if (coll.type == kHitWall)
		{
			if (weaponhitwall(proj, coll.index, oldpos)) return;
		}
		else if (coll.type == kHitSector)
		{
			if (weaponhitsector(proj, oldpos)) return;
		}

		if (s->picnum != SPIT)
		{
			if (s->picnum == RPG) rpgexplode(proj, coll.type, oldpos, EXPLOSION2, -1, -1, RPG_EXPLODE);
			else if (isRRRA() && s->picnum == RPG2) rpgexplode(proj, coll.type, oldpos, EXPLOSION2, -1,  150, 247);
			else if (isRRRA() && s->picnum == RRTILE1790) rpgexplode(proj, coll.type, oldpos, EXPLOSION2, -1,  160, RPG_EXPLODE);
			else if (s->picnum != FREEZEBLAST && s->picnum != FIRELASER && s->picnum != SHRINKSPARK)
			{
				auto k = spawn(proj, 1441);
				k->s->xrepeat = k->s->yrepeat = s->xrepeat >> 1;
				if (coll.type == kHitSector)
				{
					if (s->zvel < 0)
					{
						k->s->cstat |= 8;
						k->s->z += (72 << 8);
					}
				}
			}
		}
		deletesprite(proj);
		return;
	}
	if ((s->picnum == RPG || (isRRRA() && s->picnum == RPG2)) && s->sector()->lotag == 2 && s->xrepeat >= 10 && rnd(184))
		spawn(proj, WATERBUBBLE);

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveweapons_r(void)
{
	DukeStatIterator it(STAT_PROJECTILE);
	while (auto proj = it.Next())
	{
		if (proj->s->sectnum < 0)
		{
			deletesprite(proj);
			continue;
		}

		switch (proj->s->picnum)
		{
		case RADIUSEXPLOSION:
			deletesprite(proj);
			continue;
		case TONGUE:
			movetongue(proj, TONGUE, INNERJAW);
			continue;

		case FREEZEBLAST:
			if (proj->s->yvel < 1 || proj->s->extra < 2 || (proj->s->xvel | proj->s->zvel) == 0)
			{
				auto star = spawn(proj, TRANSPORTERSTAR);
				star->s->pal = 1;
				star->s->xrepeat = 32;
				star->s->yrepeat = 32;
				deletesprite(proj);
				continue;
			}
		case RPG2:
		case RRTILE1790:
			if (!isRRRA()) continue;
		case SHRINKSPARK:
		case RPG:
		case FIRELASER:
		case SPIT:
		case COOLEXPLOSION1:
		case OWHIP:
		case UWHIP:
			weaponcommon_r(proj);
			continue;


		case SHOTSPARK1:
		{
			int x;
			int p = findplayer(proj, &x);
			execute(proj, p, x);
			continue;
		}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movetransports_r(void)
{
	uint8_t warpdir, warpspriteto;
	int k, p, sectlotag;
	int ll2, ll, onfloorz;
	Collision coll;

	 //Transporters

	DukeStatIterator iti(STAT_TRANSPORT);
	while (auto act = iti.Next())
	{
		auto spr = act->s;
		auto sectp = spr->sector();
		sectlotag = sectp->lotag;

		auto Owner = act->GetOwner();
		if (Owner == act || Owner == nullptr)
		{
			continue;
		}

		onfloorz = act->temp_data[4];

		if (act->temp_data[0] > 0) act->temp_data[0]--;

		DukeSectIterator itj(spr->sectnum);
		while (auto act2 = itj.Next())
		{
			auto spr2 = act2->s;
			switch (spr2->statnum)
			{
			case STAT_PLAYER:	// Player

				if (act2->GetOwner())
				{
					p = spr2->yvel;

					ps[p].on_warping_sector = 1;

					if (ps[p].transporter_hold == 0 && ps[p].jumping_counter == 0)
					{
						if (ps[p].on_ground && sectlotag == 0 && onfloorz && ps[p].jetpack_on == 0)
						{
							spawn(act,  TRANSPORTERBEAM);
							S_PlayActorSound(TELEPORTER, act);

							for (k = connecthead; k >= 0; k = connectpoint2[k])
								if (ps[k].cursectnum == Owner->s->sectnum)
								{
									ps[k].frag_ps = p;
									ps[k].GetActor()->s->extra = 0;
								}

							ps[p].angle.ang = buildang(Owner->s->ang);

							if (Owner->GetOwner() != Owner)
							{
								act->temp_data[0] = 13;
								Owner->temp_data[0] = 13;
								ps[p].transporter_hold = 13;
							}

							ps[p].bobposx = ps[p].oposx = ps[p].pos.x = Owner->s->x;
							ps[p].bobposy = ps[p].oposy = ps[p].pos.y = Owner->s->y;
							ps[p].oposz = ps[p].pos.z = Owner->s->z - (gs.playerheight - (4 << 8));

							changeactorsect(act2, Owner->s->sectnum);
							ps[p].cursectnum = spr2->sectnum;

							auto beam = spawn(Owner, TRANSPORTERBEAM);
							S_PlayActorSound(TELEPORTER, beam);

							break;
						}
					}
					else break;

					if (onfloorz == 0 && abs(spr->z - ps[p].pos.z) < 6144)
						if ((ps[p].jetpack_on == 0) || (ps[p].jetpack_on && PlayerInput(p, SB_JUMP)) ||
							(ps[p].jetpack_on && PlayerInput(p, SB_CROUCH)))
						{
							ps[p].oposx = ps[p].pos.x += Owner->s->x - spr->x;
							ps[p].oposy = ps[p].pos.y += Owner->s->y - spr->y;

							if (ps[p].jetpack_on && (PlayerInput(p, SB_JUMP) || ps[p].jetpack_on < 11))
								ps[p].pos.z = Owner->s->z - 6144;
							else ps[p].pos.z = Owner->s->z + 6144;
							ps[p].oposz = ps[p].pos.z;

							changeactorsect(act2, Owner->s->sectnum);
							ps[p].cursectnum = Owner->s->sectnum;

							break;
						}

					k = 0;

					if (isRRRA())
					{
						if (onfloorz && sectlotag == 160 && ps[p].pos.z > (sectp->floorz - (48 << 8)))
						{
							k = 2;
							ps[p].oposz = ps[p].pos.z =
								Owner->getSector()->ceilingz + (7 << 8);
						}

						if (onfloorz && sectlotag == 161 && ps[p].pos.z < (sectp->ceilingz + (6 << 8)))
						{
							k = 2;
							if (ps[p].GetActor()->s->extra <= 0) break;
							ps[p].oposz = ps[p].pos.z =
								Owner->getSector()->floorz - (49 << 8);
						}
					}

					if ((onfloorz && sectlotag == ST_1_ABOVE_WATER && ps[p].pos.z > (sectp->floorz - (6 << 8))) ||
						(onfloorz && sectlotag == ST_1_ABOVE_WATER && ps[p].OnMotorcycle))
					{
						if (ps[p].OnBoat) break;
						k = 1;
						if (screenpeek == p)
						{
							FX_StopAllSounds();
						}
						S_PlayActorSound(DUKE_UNDERWATER, ps[p].GetActor());
						ps[p].oposz = ps[p].pos.z =
							Owner->getSector()->ceilingz + (7 << 8);
						if (ps[p].OnMotorcycle)
							ps[p].moto_underwater = 1;
					}

					if (onfloorz && sectlotag == ST_2_UNDERWATER && ps[p].pos.z < (sectp->ceilingz + (6 << 8)))
					{
						k = 1;
						if (ps[p].GetActor()->s->extra <= 0) break;
						if (screenpeek == p)
						{
							FX_StopAllSounds();
						}
						S_PlayActorSound(DUKE_GASP, ps[p].GetActor());

						ps[p].oposz = ps[p].pos.z =
							Owner->getSector()->floorz - (7 << 8);
					}

					if (k == 1)
					{
						ps[p].oposx = ps[p].pos.x += Owner->s->x - spr->x;
						ps[p].oposy = ps[p].pos.y += Owner->s->y - spr->y;

						if (Owner->GetOwner() != Owner)
							ps[p].transporter_hold = -2;
						ps[p].cursectnum = Owner->s->sectnum;

						changeactorsect(act2, Owner->s->sectnum);

						if ((krand() & 255) < 32)
							spawn(ps[p].GetActor(), WATERSPLASH2);
					}
					else if (isRRRA() && k == 2)
					{
						ps[p].oposx = ps[p].pos.x += Owner->s->x - spr->x;
						ps[p].oposy = ps[p].pos.y += Owner->s->y - spr->y;

						if (Owner->GetOwner() != Owner)
							ps[p].transporter_hold = -2;
						ps[p].cursectnum = Owner->s->sectnum;

						changeactorsect(act2, Owner->s->sectnum);
					}
				}
				break;

			case STAT_ACTOR:
				if (spr->picnum == SHARK ||
					(isRRRA() && (spr->picnum == CHEERBOAT || spr->picnum == HULKBOAT || spr->picnum == MINIONBOAT || spr->picnum == UFO1_RRRA)) ||
					(!isRRRA() && (spr->picnum == UFO1_RR || spr->picnum == UFO2 || spr->picnum == UFO3 || spr->picnum == UFO4 || spr->picnum == UFO5))) continue;
			case STAT_PROJECTILE:
			case STAT_MISC:
			case STAT_DUMMYPLAYER:

				ll = abs(spr2->zvel);
				if (isRRRA())
				{
					if (spr2->zvel >= 0)
						warpdir = 2;
					else
						warpdir = 1;
				}

				{
					warpspriteto = 0;
					if (ll && sectlotag == ST_2_UNDERWATER && spr2->z < (sectp->ceilingz + ll))
						warpspriteto = 1;

					if (ll && sectlotag == ST_1_ABOVE_WATER && spr2->z > (sectp->floorz - ll))
						if (!isRRRA() || (spr2->picnum != CHEERBOAT && spr2->picnum != HULKBOAT && spr2->picnum != MINIONBOAT))
							warpspriteto = 1;

					if (isRRRA())
					{
						if (ll && sectlotag == 161 && spr2->z < (sectp->ceilingz + ll) && warpdir == 1)
						{
							warpspriteto = 1;
							ll2 = ll - abs(spr2->z - sectp->ceilingz);
						}
						else if (sectlotag == 161 && spr2->z < (sectp->ceilingz + 1000) && warpdir == 1)
						{
							warpspriteto = 1;
							ll2 = 1;
						}
						if (ll && sectlotag == 160 && spr2->z > (sectp->floorz - ll) && warpdir == 2)
						{
							warpspriteto = 1;
							ll2 = ll - abs(sectp->floorz - spr2->z);
						}
						else if (sectlotag == 160 && spr2->z > (sectp->floorz - 1000) && warpdir == 2)
						{
							warpspriteto = 1;
							ll2 = 1;
						}
					}

					if (sectlotag == 0 && (onfloorz || abs(spr2->z - spr->z) < 4096))
					{
						if (Owner->GetOwner() != Owner && onfloorz && act->temp_data[0] > 0 && spr2->statnum != 5)
						{
							act->temp_data[0]++;
							continue;
						}
						warpspriteto = 1;
					}

					if (warpspriteto) switch (spr2->picnum)
					{
					case TRANSPORTERSTAR:
					case TRANSPORTERBEAM:
					case BULLETHOLE:
					case WATERSPLASH2:
					case BURNING:
					case FIRE:
					case MUD:
						continue;
					case PLAYERONWATER:
						if (sectlotag == ST_2_UNDERWATER)
						{
							spr2->cstat &= 32767;
							break;
						}
					default:
						if (spr2->statnum == 5 && !(sectlotag == ST_1_ABOVE_WATER || sectlotag == ST_2_UNDERWATER || (isRRRA() && (sectlotag == 160 || sectlotag == 161))))
							break;

					case WATERBUBBLE:
						if (rnd(192) && spr2->picnum == WATERBUBBLE)
							break;

						if (sectlotag > 0)
						{
							auto k = spawn(act2, WATERSPLASH2);
							if (sectlotag == 1 && spr2->statnum == 4)
							{
								k->s->xvel = spr2->xvel >> 1;
								k->s->ang = spr2->ang;
								ssp(k, CLIPMASK0);
							}
						}

						switch (sectlotag)
						{
						case ST_0_NO_EFFECT:
							if (onfloorz)
							{
								if (checkcursectnums(spr->sectnum) == -1 && checkcursectnums(Owner->s->sectnum) == -1)
								{
									spr2->x += (Owner->s->x - spr->x);
									spr2->y += (Owner->s->y - spr->y);
									spr2->z -= spr->z - Owner->getSector()->floorz;
									spr2->ang = Owner->s->ang;

									spr2->backupang();

									auto beam = spawn(act, TRANSPORTERBEAM);
									S_PlayActorSound(TELEPORTER, beam);

									beam = spawn(Owner, TRANSPORTERBEAM);
									S_PlayActorSound(TELEPORTER, beam);

									if (Owner->GetOwner() != Owner)
									{
										act->temp_data[0] = 13;
										Owner->temp_data[0] = 13;
									}

									changeactorsect(act2, Owner->s->sectnum);
								}
							}
							else
							{
								spr2->x += (Owner->s->x - spr->x);
								spr2->y += (Owner->s->y - spr->y);
								spr2->z = Owner->s->z + 4096;

								spr2->backupz();

								changeactorsect(act2, Owner->s->sectnum);
							}
							break;
						case ST_1_ABOVE_WATER:
							spr2->x += (Owner->s->x - spr->x);
							spr2->y += (Owner->s->y - spr->y);
							spr2->z = Owner->getSector()->ceilingz + ll;

							spr2->backupz();

							changeactorsect(act2, Owner->s->sectnum);

							break;
						case ST_2_UNDERWATER:
							spr2->x += (Owner->s->x - spr->x);
							spr2->y += (Owner->s->y - spr->y);
							spr2->z = Owner->getSector()->floorz - ll;

							spr2->backupz();

							changeactorsect(act2, Owner->s->sectnum);

							break;

						case 160:
							if (!isRRRA()) break;
							spr2->x += (Owner->s->x - spr->x);
							spr2->y += (Owner->s->y - spr->y);
							spr2->z = Owner->getSector()->ceilingz + ll2;

							spr2->backupz();

							changeactorsect(act2, Owner->s->sectnum);

							movesprite_ex(act2, MulScale(spr2->xvel, bcos(spr2->ang), 14),
								MulScale(spr2->xvel, bsin(spr2->ang), 14), 0, CLIPMASK1, coll);

							break;
						case 161:
							if (!isRRRA()) break;
							spr2->x += (Owner->s->x - spr->x);
							spr2->y += (Owner->s->y - spr->y);
							spr2->z = Owner->getSector()->floorz - ll2;

							spr2->backupz();

							changeactorsect(act2, Owner->s->sectnum);

							movesprite_ex(act2, MulScale(spr2->xvel, bcos(spr2->ang), 14),
								MulScale(spr2->xvel, bsin(spr2->ang), 14), 0, CLIPMASK1, coll);

							break;
						}

						break;
					}
				}
				break;

			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void rrra_specialstats()
{
	Collision coll;
	DukeStatIterator it(117);
	while (auto act = it.Next())
	{
		auto s = act->s;
		if (s->hitag > 2)
			s->hitag = 0;
		if ((s->picnum == RRTILE8488 || s->picnum == RRTILE8490) && s->hitag != 2)
		{
			s->hitag = 2;
			s->extra = -100;
		}
		if (s->hitag == 0)
		{
			s->extra++;
			if (s->extra >= 30)
				s->hitag = 1;
		}
		else if (s->hitag == 1)
		{
			s->extra--;
			if (s->extra <= -30)
				s->hitag = 0;
		}
		else if (s->hitag == 2)
		{
			s->extra--;
			if (s->extra <= -104)
			{
				spawn(act, s->lotag);
				deletesprite(act);
			}
		}
		movesprite_ex(act, 0, 0, s->extra * 2, CLIPMASK0, coll);
	}

	it.Reset(118);
	while (auto act = it.Next())
	{
		auto s = act->s;
		if (s->hitag > 1)
			s->hitag = 0;
		if (s->hitag == 0)
		{
			s->extra++;
			if (s->extra >= 20)
				s->hitag = 1;
		}
		else if (s->hitag == 1)
		{
			s->extra--;
			if (s->extra <= -20)
				s->hitag = 0;
		}
		movesprite_ex(act, 0, 0, s->extra, CLIPMASK0, coll);
	}

	if (ps[screenpeek].MamaEnd > 0)
	{
		ps[screenpeek].MamaEnd--;
		if (ps[screenpeek].MamaEnd == 0)
		{
			CompleteLevel(nullptr);
		}
	}

	if (enemysizecheat > 0)
	{
		DukeSpriteIterator it;
		while (auto act = it.Next())
		{
			auto s = act->s;
			switch (s->picnum)
			{
				//case 4049:
				//case 4050:
			case BILLYCOCK:
			case BILLYRAY:
			case BILLYRAYSTAYPUT:
			case BRAYSNIPER:
			case DOGRUN:
			case LTH:
			case HULKJUMP:
			case HULK:
			case HULKSTAYPUT:
			case HEN:
			case DRONE:
			case PIG:
			case MINION:
			case MINIONSTAYPUT:
			case UFO1_RRRA:
			case UFO2:
			case UFO3:
			case UFO4:
			case UFO5:
			case COOT:
			case COOTSTAYPUT:
			case VIXEN:
			case BIKERB:
			case BIKERBV2:
			case BIKER:
			case MAKEOUT:
			case CHEERB:
			case CHEER:
			case CHEERSTAYPUT:
			case COOTPLAY:
			case BILLYPLAY:
			case MINIONBOAT:
			case HULKBOAT:
			case CHEERBOAT:
			case RABBIT:
			case MAMA:
				if (enemysizecheat == 3)
				{
					s->xrepeat <<= 1;
					s->yrepeat <<= 1;
					s->clipdist = MulScale(s->xrepeat, tileWidth(s->picnum), 7);
				}
				else if (enemysizecheat == 2)
				{
					s->xrepeat >>= 1;
					s->yrepeat >>= 1;
					s->clipdist = MulScale(s->xrepeat, tileHeight(s->picnum), 7);
				}
				break;
			}

		}
		enemysizecheat = 0;
	}

	it.Reset(121);
	while (auto act = it.Next())
	{
		auto s = act->s;
		s->extra++;
		if (s->extra < 100)
		{
			if (s->extra == 90)
			{
				s->picnum--;
				if (s->picnum < PIG + 7)
					s->picnum = PIG + 7;
				s->extra = 1;
			}
			movesprite_ex(act, 0, 0, -300, CLIPMASK0, coll);
			if (s->sector()->ceilingz + (4 << 8) > s->z)
			{
				s->picnum = 0;
				s->extra = 100;
			}
		}
		else if (s->extra == 200)
		{
			setsprite(act, s->x, s->y, s->sector()->floorz - 10);
			s->extra = 1;
			s->picnum = PIG + 11;
			spawn(act, TRANSPORTERSTAR);
		}
	}

	it.Reset(119);
	while (auto act = it.Next())
	{
		auto s = act->s;
		if (s->hitag > 0)
		{
			if (s->extra == 0)
			{
				s->hitag--;
				s->extra = 150;
				spawn(act, RABBIT);
			}
			else
				s->extra--;
		}
	}
	it.Reset(116);
	while (auto act = it.Next())
	{
		auto s = act->s;
		if (s->extra)
		{
			if (s->extra == s->lotag)
				S_PlaySound(183);
			s->extra--;
			int j = movesprite_ex(act,
				MulScale(s->hitag, bcos(s->ang), 14),
				MulScale(s->hitag, bsin(s->ang), 14),
				s->hitag << 1, CLIPMASK0, coll);
			if (j > 0)
			{
				S_PlayActorSound(PIPEBOMB_EXPLODE, act);
				deletesprite(act);
			}
			if (s->extra == 0)
			{
				S_PlaySound(215);
				deletesprite(act);
				earthquaketime = 32;
				SetPlayerPal(&ps[myconnectindex], PalEntry(32, 32, 32, 48));
			}
		}
	}

	it.Reset(115);
	while (auto act = it.Next())
	{
		auto s = act->s;
		if (s->extra)
		{
			if (s->picnum != RRTILE8162)
				s->picnum = RRTILE8162;
			s->extra--;
			if (s->extra == 0)
			{
				int rvar;
				rvar = krand() & 127;
				if (rvar < 96)
				{
					s->picnum = RRTILE8162 + 3;
				}
				else if (rvar < 112)
				{
					if (ps[screenpeek].SlotWin & 1)
					{
						s->picnum = RRTILE8162 + 3;
					}
					else
					{
						s->picnum = RRTILE8162 + 2;
						spawn(act, BATTERYAMMO);
						ps[screenpeek].SlotWin |= 1;
						S_PlayActorSound(52, act);
					}
				}
				else if (rvar < 120)
				{
					if (ps[screenpeek].SlotWin & 2)
					{
						s->picnum = RRTILE8162 + 3;
					}
					else
					{
						s->picnum = RRTILE8162 + 6;
						spawn(act, HEAVYHBOMB);
						ps[screenpeek].SlotWin |= 2;
						S_PlayActorSound(52, act);
					}
				}
				else if (rvar < 126)
				{
					if (ps[screenpeek].SlotWin & 4)
					{
						s->picnum = RRTILE8162 + 3;
					}
					else
					{
						s->picnum = RRTILE8162 + 5;
						spawn(act, SIXPAK);
						ps[screenpeek].SlotWin |= 4;
						S_PlayActorSound(52, act);
					}
				}
				else
				{
					if (ps[screenpeek].SlotWin & 8)
					{
						s->picnum = RRTILE8162 + 3;
					}
					else
					{
						s->picnum = RRTILE8162 + 4;
						spawn(act, ATOMICHEALTH);
						ps[screenpeek].SlotWin |= 8;
						S_PlayActorSound(52, act);
					}
				}
			}
		}
	}

	it.Reset(122);
	while (auto act = it.Next())
	{
		auto s = act->s;
		if (s->extra)
		{
			if (s->picnum != RRTILE8589)
				s->picnum = RRTILE8589;
			s->extra--;
			if (s->extra == 0)
			{
				int rvar;
				rvar = krand() & 127;
				if (rvar < 96)
				{
					s->picnum = RRTILE8589 + 4;
				}
				else if (rvar < 112)
				{
					if (ps[screenpeek].SlotWin & 1)
					{
						s->picnum = RRTILE8589 + 4;
					}
					else
					{
						s->picnum = RRTILE8589 + 5;
						spawn(act, BATTERYAMMO);
						ps[screenpeek].SlotWin |= 1;
						S_PlayActorSound(342, act);
					}
				}
				else if (rvar < 120)
				{
					if (ps[screenpeek].SlotWin & 2)
					{
						s->picnum = RRTILE8589 + 4;
					}
					else
					{
						s->picnum = RRTILE8589 + 6;
						spawn(act, HEAVYHBOMB);
						ps[screenpeek].SlotWin |= 2;
						S_PlayActorSound(342, act);
					}
				}
				else if (rvar < 126)
				{
					if (ps[screenpeek].SlotWin & 4)
					{
						s->picnum = RRTILE8589 + 4;
					}
					else
					{
						s->picnum = RRTILE8589 + 2;
						spawn(act, SIXPAK);
						ps[screenpeek].SlotWin |= 4;
						S_PlayActorSound(342, act);
					}
				}
				else
				{
					if (ps[screenpeek].SlotWin & 8)
					{
						s->picnum = RRTILE8589 + 4;
					}
					else
					{
						s->picnum = RRTILE8589 + 3;
						spawn(act, ATOMICHEALTH);
						ps[screenpeek].SlotWin |= 8;
						S_PlayActorSound(342, act);
					}
				}
			}
		}
	}

	it.Reset(123);
	while (auto act = it.Next())
	{
		if (act->s->lotag == 5)
			if (!S_CheckSoundPlaying(330))
				S_PlayActorSound(330, act);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void rr_specialstats()
{
	DukeStatIterator it(107);
	while (auto act = it.Next())
	{
		auto s = act->s;
		if (s->hitag == 100)
		{
			s->z += (4 << 8);
			if (s->z >= s->sector()->floorz + 15168)
				s->z = s->sector()->floorz + 15168;
		}

		if (s->picnum == LUMBERBLADE)
		{
			s->extra++;
			if (s->extra == 192)
			{
				s->hitag = 0;
				s->z = s->sector()->floorz - 15168;
				s->extra = 0;
				s->picnum = RRTILE3410;
				DukeStatIterator it2(STAT_DEFAULT);
				while (auto act2 = it2.Next())
				{
					if (act2->s->picnum == 128)
						if (act2->s->hitag == 999)
							act2->s->picnum = 127;
				}
			}
		}
	}

	if (chickenplant)
	{
		it.Reset(106);
		while (auto act = it.Next())
		{
			auto s = act->s;
			switch (s->picnum)
			{
			case RRTILE285:
				s->lotag--;
				if (s->lotag < 0)
				{
					spawn(act, RRTILE3190)->s->ang = s->ang;
					s->lotag = 128;
				}
				break;
			case RRTILE286:
				s->lotag--;
				if (s->lotag < 0)
				{
					spawn(act, RRTILE3192)->s->ang = s->ang;
					s->lotag = 256;
				}
				break;
			case RRTILE287:
				s->lotag--;
				if (s->lotag < 0)
				{
					lotsoffeathers_r(act, (krand() & 3) + 4);
					s->lotag = 84;
				}
				break;
			case RRTILE288:
				s->lotag--;
				if (s->lotag < 0)
				{
					auto j = spawn(act, RRTILE3132);
					s->lotag = 96;
					if (!isRRRA()) S_PlayActorSound(472, j);
				}
				break;
			case RRTILE289:
				s->lotag--;
				if (s->lotag < 0)
				{
					spawn(act, RRTILE3120)->s->ang = s->ang;
					s->lotag = 448;
				}
				break;
			case RRTILE290:
				s->lotag--;
				if (s->lotag < 0)
				{
					spawn(act, RRTILE3122)->s->ang = s->ang;
					s->lotag = 64;
				}
				break;
			case RRTILE291:
				s->lotag--;
				if (s->lotag < 0)
				{
					spawn(act, RRTILE3123)->s->ang = s->ang;
					s->lotag = 512;
				}
				break;
			case RRTILE292:
				s->lotag--;
				if (s->lotag < 0)
				{
					spawn(act, RRTILE3124)->s->ang = s->ang;
					s->lotag = 224;
				}
				break;
			case RRTILE293:
				s->lotag--;
				if (s->lotag < 0)
				{
					fi.guts(act, JIBS1, 1, myconnectindex);
					fi.guts(act, JIBS2, 1, myconnectindex);
					fi.guts(act, JIBS3, 1, myconnectindex);
					fi.guts(act, JIBS4, 1, myconnectindex);
					s->lotag = 256;
				}
				break;
			}
		}
	}

	it.Reset(STAT_BOWLING);
	while (auto act = it.Next())
	{
		auto s = act->s;
		if (s->picnum == BOWLINGPINSPOT)
			if (s->lotag == 100)
			{
				auto pst = pinsectorresetup(s->sectnum);
				if (pst)
				{
					s->lotag = 0;
					if (s->extra == 1)
					{
						pst = checkpins(s->sectnum);
						if (!pst)
						{
							s->extra = 2;
						}
					}
					if (s->extra == 2)
					{
						s->extra = 0;
						resetpins(s->sectnum);
					}
				}
			}
	}

	it.Reset(108);
	while (auto act = it.Next())
	{
		auto s = act->s;
		if (s->picnum == RRTILE296)
		{
			int x;
			int p = findplayer(act, &x);
			if (x < 2047)
			{
				DukeStatIterator it2(108);
				while (auto act2 = it2.Next())
				{
					if (act2->s->picnum == RRTILE297)
					{
						ps[p].angle.ang = buildang(act2->s->ang);
						ps[p].bobposx = ps[p].oposx = ps[p].pos.x = act2->s->x;
						ps[p].bobposy = ps[p].oposy = ps[p].pos.y = act2->s->y;
						ps[p].oposz = ps[p].pos.z = act2->s->z - (36 << 8);
						auto pact = ps[p].GetActor();
						changeactorsect(pact, act2->s->sectnum);
						ps[p].cursectnum = pact->s->sectnum;
						S_PlayActorSound(70, act2);
						deletesprite(act2);
					}
				}
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void heavyhbomb(DDukeActor *actor)
{
	auto s = actor->s;
	auto t = &actor->temp_data[0];
	auto sectp = s->sector();
	int x, l;
	auto Owner = actor->GetOwner();

	if ((s->cstat & 32768))
	{
		t[2]--;
		if (t[2] <= 0)
		{
			S_PlayActorSound(TELEPORTER, actor);
			spawn(actor, TRANSPORTERSTAR);
			s->cstat = 257;
		}
		return;
	}

	int p = findplayer(actor, &x);

	if (x < 1220) s->cstat &= ~257;
	else s->cstat |= 257;

	if (t[3] == 0)
	{
		int j = fi.ifhitbyweapon(actor);
		if (j >= 0)
		{
			t[3] = 1;
			t[4] = 0;
			l = 0;
			s->xvel = 0;
			goto DETONATEB;
		}
	}

	makeitfall(actor);

	if (sectp->lotag != 1 && (!isRRRA() || sectp->lotag != 160) && s->z >= actor->floorz - (FOURSLEIGHT) && s->yvel < 3)
	{
		if (s->yvel > 0 || (s->yvel == 0 && actor->floorz == sectp->floorz))
		{
			if (s->picnum != CHEERBOMB)
				S_PlayActorSound(PIPEBOMB_BOUNCE, actor);
			else
			{
				t[3] = 1;
				t[4] = 1;
				l = 0;
				goto DETONATEB;
			}
		}
		s->zvel = -((4 - s->yvel) << 8);
		if (s->sector()->lotag == 2)
			s->zvel >>= 2;
		s->yvel++;
	}
	if (s->picnum != CHEERBOMB && s->z < actor->ceilingz + (16 << 8) && sectp->lotag != 2)
	{
		s->z = actor->ceilingz + (16 << 8);
		s->zvel = 0;
	}

	Collision coll;
	movesprite_ex(actor,
		MulScale(s->xvel, bcos(s->ang), 14),
		MulScale(s->xvel, bsin(s->ang), 14),
		s->zvel, CLIPMASK0, coll);

	if (s->sector()->lotag == 1 && s->zvel == 0)
	{
		s->z += (32 << 8);
		if (t[5] == 0)
		{
			t[5] = 1;
			spawn(actor, WATERSPLASH2);
			if (isRRRA() && s->picnum == MORTER)
				s->xvel = 0;
		}
	}
	else t[5] = 0;

	if (t[3] == 0 && s->picnum == MORTER && (coll.type || x < 844))
	{
		t[3] = 1;
		t[4] = 0;
		l = 0;
		s->xvel = 0;
		goto DETONATEB;
	}

	if (t[3] == 0 && s->picnum == CHEERBOMB && (coll.type || x < 844))
	{
		t[3] = 1;
		t[4] = 0;
		l = 0;
		s->xvel = 0;
		goto DETONATEB;
	}

	if (Owner && Owner->s->picnum == APLAYER)
		l = Owner->PlayerIndex();
	else l = -1;

	if (s->xvel > 0)
	{
		s->xvel -= 5;
		if (sectp->lotag == 2)
			s->xvel -= 10;

		if (s->xvel < 0)
			s->xvel = 0;
		if (s->xvel & 8) s->cstat ^= 4;
	}

	if (coll.type == kHitWall)
	{
		int j = coll.index;
		fi.checkhitwall(actor, j, s->x, s->y, s->z, s->picnum);

		int k = getangle(
			wall[wall[j].point2].x - wall[j].x,
			wall[wall[j].point2].y - wall[j].y);

		if (s->picnum == CHEERBOMB)
		{
			t[3] = 1;
			t[4] = 0;
			l = 0;
			s->xvel = 0;
			goto DETONATEB;
		}
		s->ang = ((k << 1) - s->ang) & 2047;
		s->xvel >>= 1;
	}

DETONATEB:

	if ((l >= 0 && ps[l].hbomb_on == 0) || t[3] == 1)
	{
		t[4]++;

		if (t[4] == 2)
		{
			x = s->extra;
			int m = 0;
			switch (s->picnum)
			{
			case TRIPBOMBSPRITE: m = gs.tripbombblastradius; break;	// powder keg
			case HEAVYHBOMB: m = gs.pipebombblastradius; break;
			case HBOMBAMMO: m = gs.pipebombblastradius; break;
			case MORTER: m = gs.morterblastradius; break;
			case CHEERBOMB: m = gs.morterblastradius; break;
			}

			if (s->sector()->lotag != 800)
			{
				fi.hitradius(actor, m, x >> 2, x >> 1, x - (x >> 2), x);
				spawn(actor, EXPLOSION2);
				if (s->picnum == CHEERBOMB)
					spawn(actor, BURNING);
				S_PlayActorSound(PIPEBOMB_EXPLODE, actor);
				for (x = 0; x < 8; x++)
					RANDOMSCRAP(actor);
			}
		}

		if (s->yrepeat)
		{
			s->yrepeat = 0;
			return;
		}

		if (t[4] > 20)
		{
			deletesprite(actor);
			return;
		}
		if (s->picnum == CHEERBOMB)
		{
			spawn(actor, BURNING);
			deletesprite(actor);
			return;
		}
	}
	else if (s->picnum == HEAVYHBOMB && x < 788 && t[0] > 7 && s->xvel == 0)
		if (cansee(s->x, s->y, s->z - (8 << 8), s->sectnum, ps[p].pos.x, ps[p].pos.y, ps[p].pos.z, ps[p].cursectnum))
			if (ps[p].ammo_amount[DYNAMITE_WEAPON] < gs.max_ammo_amount[DYNAMITE_WEAPON])
				if (s->pal == 0)
				{
					if (ud.coop >= 1)
					{
						for (int j = 0; j < ps[p].weapreccnt; j++)
							if (ps[p].weaprecs[j] == s->picnum)
								return;

						if (ps[p].weapreccnt < 255)
							ps[p].weaprecs[ps[p].weapreccnt++] = s->picnum;
					}

					addammo(DYNAMITE_WEAPON, &ps[p], 1);
					addammo(CROSSBOW_WEAPON, &ps[p], 1);
					S_PlayActorSound(DUKE_GET, ps[p].GetActor());

					if (ps[p].gotweapon[DYNAMITE_WEAPON] == 0 || Owner == ps[p].GetActor())
						fi.addweapon(&ps[p], DYNAMITE_WEAPON);

					if (!Owner || Owner->s->picnum != APLAYER)
					{
						SetPlayerPal(&ps[p], PalEntry(32, 0, 32, 0));
					}

					if (Owner && (Owner->picnum != HEAVYHBOMB || ud.respawn_items == 0 || Owner->s->picnum == APLAYER))
					{
						if (s->picnum == HEAVYHBOMB && Owner->s->picnum != APLAYER && ud.coop)
							return;
						deletesprite(actor);
						return;
					}
					else
					{
						t[2] = gs.respawnitemtime;
						spawn(actor, RESPAWNMARKERRED);
						s->cstat = 32768;
					}
				}

	if (t[0] < 8) t[0]++;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static int henstand(DDukeActor *actor)
{
	auto s = actor->s;
	auto t = &actor->temp_data[0];

	if (s->picnum == HENSTAND || s->picnum == HENSTAND + 1)
	{
		s->lotag--;
		if (s->lotag == 0)
		{
			spawn(actor, HEN);
			deletesprite(actor);
			return 1;
		}
	}
	if (s->sector()->lotag == 900)
		s->xvel = 0;
	if (s->xvel)
	{
		makeitfall(actor);
		Collision coll;
		movesprite_ex(actor,
			MulScale(bcos(s->ang), s->xvel, 14),
			MulScale(bsin(s->ang), s->xvel, 14),
			s->zvel, CLIPMASK0, coll);
		if (coll.type)
		{
			if (coll.type == kHitWall)
			{
				int j = coll.index;
				int k = getangle(
					wall[wall[j].point2].x - wall[j].x,
					wall[wall[j].point2].y - wall[j].y);
				s->ang = ((k << 1) - s->ang) & 2047;
			}
			else if (coll.type == kHitSprite)
			{
				auto hitact = coll.actor;
				fi.checkhitsprite(actor, hitact);
				if (hitact->s->picnum == HEN)
				{
					auto ns = spawn(hitact, HENSTAND);
					deletesprite(hitact);
					ns->s->xvel = 32;
					ns->s->lotag = 40;
					ns->s->ang = s->ang;
				}
			}
		}
		s->xvel--;
		if (s->xvel < 0) s->xvel = 0;
		s->cstat = 257;
		if (s->picnum == BOWLINGPIN)
		{
			s->cstat |= 4 & s->xvel;
			s->cstat |= 8 & s->xvel;
			if (krand() & 1)
				s->picnum = BOWLINGPIN + 1;
		}
		else if (s->picnum == HENSTAND)
		{
			s->cstat |= 4 & s->xvel;
			s->cstat |= 8 & s->xvel;
			if (krand() & 1)
				s->picnum = HENSTAND + 1;
			if (!s->xvel)
				return 2;//deletesprite(actor); still needs to run a script but should not do on a deleted object
		}
		if (s->picnum == BOWLINGPIN || (s->picnum == BOWLINGPIN + 1 && !s->xvel))
		{
			return 2;//deletesprite(actor); still needs to run a script but should not do on a deleted object
		}
	}
	else if (s->sector()->lotag == 900)
	{
		if (s->picnum == BOWLINGBALL)
			ballreturn(actor);
		deletesprite(actor);
		return 1;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveactors_r(void)
{
	int x;
	int p;
	Collision coll;
	
	dojaildoor();
	moveminecart();

	if (isRRRA())
	{
		rrra_specialstats();
	}
	rr_specialstats();

	DukeStatIterator it(STAT_ACTOR);
	while (auto act = it.Next())
	{
		auto s = act->s;
		bool deleteafterexecute = false;	// taking a cue here from RedNukem to not run scripts on deleted sprites.

		if( s->xrepeat == 0 || s->sectnum < 0 || s->sectnum >= MAXSECTORS)
		{
			deletesprite(act);
			continue;
		}

		auto sectp = s->sector();
		auto t = &act->temp_data[0];

		switch(s->picnum)
		{
			case RESPAWNMARKERRED:
			case RESPAWNMARKERYELLOW:
			case RESPAWNMARKERGREEN:
				if (!respawnmarker(act, RESPAWNMARKERYELLOW, RESPAWNMARKERGREEN)) continue;
				break;
			case RAT:
				if (!rat(act, !isRRRA())) continue;
				break;
			case RRTILE3190:
			case RRTILE3191:
			case RRTILE3192:
				if (!chickenplant)
				{
					deletesprite(act);
					continue;
				}
				if (sectp->lotag == 903)
					makeitfall(act);
				movesprite_ex(act,
					MulScale(s->xvel, bcos(s->ang), 14),
					MulScale(s->xvel, bsin(s->ang), 14),
					s->zvel,CLIPMASK0, coll);
				switch (sectp->lotag)
				{
					case 901:
						s->picnum = RRTILE3191;
						break;
					case 902:
						s->picnum = RRTILE3192;
						break;
					case 903:
						if (s->z >= sectp->floorz - (8<<8))
						{
							deletesprite(act);
							continue;
						}
						break;
					case 904:
						deletesprite(act);
						continue;
						break;
				}
				if (coll.type > kHitSector)
				{
					deletesprite(act);
					continue;
				}
				break;

			case RRTILE3120:
			case RRTILE3122:
			case RRTILE3123:
			case RRTILE3124:
				if (!chickenplant)
				{
					deletesprite(act);
					continue;
				}
				makeitfall(act);
				movesprite_ex(act,
					MulScale(s->xvel, bcos(s->ang), 14),
					MulScale(s->xvel, bsin(s->ang), 14),
					s->zvel,CLIPMASK0, coll);
				if (coll.type > kHitSector)
				{
					deletesprite(act);
					continue;
				}
				if (sectp->lotag == 903)
				{
					if (s->z >= sectp->floorz - (4<<8))
					{
						deletesprite(act);
						continue;
					}
				}
				else if (sectp->lotag == 904)
				{
					deletesprite(act);
					continue;
				}
				break;

			case RRTILE3132:
				if (!chickenplant)
				{
					deletesprite(act);
					continue;
				}
				makeitfall(act);
				movesprite_ex(act,
					MulScale(s->xvel, bcos(s->ang), 14),
					MulScale(s->xvel, bsin(s->ang), 14),
					s->zvel,CLIPMASK0, coll);
				if (s->z >= sectp->floorz - (8<<8))
				{
					if (sectp->lotag == 1)
					{
						auto j = spawn(act, WATERSPLASH2);
						j->s->z = j->getSector()->floorz;
					}
					deletesprite(act);
					continue;
				}
				break;
			case BOWLINGBALL:
				if (s->xvel)
				{
					if(!S_CheckSoundPlaying(356))
						S_PlayActorSound(356,act);
				}
				else
				{
					spawn(act,BOWLINGBALLSPRITE);
					deletesprite(act);
					continue;
				}
				if (s->sector()->lotag == 900)
				{
					S_StopSound(356, nullptr);
				}
			case BOWLINGPIN:
			case BOWLINGPIN+1:
			case HENSTAND:
			case HENSTAND+1:
			{
				int todo = henstand(act);
				if (todo == 2) deleteafterexecute = true;
				if (todo == 1) continue;
				break;
			}

			case QUEBALL:
			case STRIPEBALL:
				if (!queball(act, POCKET, QUEBALL, STRIPEBALL)) continue;
				break;
			case FORCESPHERE:
				forcesphere(act, FORCESPHERE);
				continue;

			case RECON:
			case UFO1_RR:
			case UFO2:
			case UFO3:
			case UFO4:
			case UFO5:
				recon(act, EXPLOSION2, FIRELASER, -1, -1, 457, 8, [](DDukeActor* act) ->int
					{
						auto s = act->s;
						if (isRRRA() && ufospawnsminion)
							return MINION;
						else if (s->picnum == UFO1_RR)
							return HEN;
						else if (s->picnum == UFO2)
							return COOT;
						else if (s->picnum == UFO3)
							return COW;
						else if (s->picnum == UFO4)
							return PIG;
						else if (s->picnum == UFO5)
							return BILLYRAY;
						else return -1;
					});
				continue;

			case OOZ:
				ooz(act);
				continue;

			case EMPTYBIKE:
				if (!isRRRA()) break;
				makeitfall(act);
				getglobalz(act);
				if (sectp->lotag == 1)
				{
					setsprite(act,s->x,s->y,act->floorz+(16<<8));
				}
				break;

			case EMPTYBOAT:
				if (!isRRRA()) break;
				makeitfall(act);
				getglobalz(act);
				break;

			case TRIPBOMBSPRITE:
				if (!isRRRA() || (sectp->lotag != 1 && sectp->lotag != 160))
					if (s->xvel)
					{
						movesprite_ex(act,
							MulScale(s->xvel, bcos(s->ang), 14),
							MulScale(s->xvel, bsin(s->ang), 14),
							s->zvel,CLIPMASK0, coll);
						s->xvel--;
					}
				break;

			case CHEERBOMB:
				if (!isRRRA()) break;
			case MORTER:
			case HEAVYHBOMB:
				heavyhbomb(act);
				continue;
				
			case REACTORBURNT:
			case REACTOR2BURNT:
				continue;

			case REACTOR:
			case REACTOR2:
				reactor(act, REACTOR, REACTOR2, REACTORBURNT, REACTOR2BURNT, REACTORSPARK, REACTOR2SPARK);
				continue;

			case CAMERA1:
				camera(act);
				continue;
		}


// #ifndef VOLOMEONE
		if( ud.multimode < 2 && badguy(act) )
		{
			if( actor_tog == 1)
			{
				s->cstat = 32768;
				continue;
			}
			else if(actor_tog == 2) s->cstat = 257;
		}
// #endif

		p = findplayer(act,&x);

		execute(act,p,x);
		if (deleteafterexecute) deletesprite(act);
	}

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveexplosions_r(void)  // STATNUM 5
{
	int p;
	int x, * t;

	
	DukeStatIterator it(STAT_MISC);
	while (auto act = it.Next())
	{
		auto s = act->s;
		t = &act->temp_data[0];
		auto sectp = s->sector();

		if (s->sectnum < 0 || s->xrepeat == 0) 
		{
			deletesprite(act);
			continue;
		}

		switch (s->picnum)
		{
		case SHOTGUNSPRITE:
			if (s->sector()->lotag == 800)
				if (s->z >= s->sector()->floorz - (8 << 8))
				{
					deletesprite(act);
					continue;
				}
			break;
		case NEON1:
		case NEON2:
		case NEON3:
		case NEON4:
		case NEON5:
		case NEON6:

			if ((global_random / (s->lotag + 1) & 31) > 4) s->shade = -127;
			else s->shade = 127;
			continue;

		case BLOODSPLAT1:
		case BLOODSPLAT2:
		case BLOODSPLAT3:
		case BLOODSPLAT4:

			if (t[0] == 7 * 26) continue;
			s->z += 16 + (krand() & 15);
			t[0]++;
			if ((t[0] % 9) == 0) s->yrepeat++;
			continue;


		case FORCESPHERE:
			forcesphereexplode(act);
			continue;

		case MUD:

			t[0]++;
			if (t[0] == 1)
			{
				if (sectp->floorpicnum != 3073)
				{
					deletesprite(act);
					continue;
				}
				if (S_CheckSoundPlaying(22))
					S_PlayActorSound(22, act);
			}
			if (t[0] == 3)
			{
				t[0] = 0;
				t[1]++;
			}
			if (t[1] == 5)
				deletesprite(act);
			continue;

		case WATERSPLASH2:
			watersplash2(act);
			continue;

		case FRAMEEFFECT1:
			frameeffect1(act);
			continue;
		case INNERJAW:
		case INNERJAW + 1:

			p = findplayer(act, &x);
			if (x < 512)
			{
				SetPlayerPal(&ps[p], PalEntry(32, 32, 0, 0));
				ps[p].GetActor()->s->extra -= 4;
			}

		case COOLEXPLOSION1:
		case FIRELASER:
		case OWHIP:
		case UWHIP:
			if (s->extra != 999)
				s->extra = 999;
			else
			{
				deletesprite(act);
				continue;
			}
			break;
		case TONGUE:
			deletesprite(act);
			continue;
		case FEATHER + 1: // feather
			act->floorz = s->z = getflorzofslope(s->sectnum, s->x, s->y);
			if (s->sector()->lotag == 800)
			{
				deletesprite(act);
				continue;
			}
			break;
		case FEATHER:
			if (!money(act, BLOODPOOL)) continue;

			if (s->sector()->lotag == 800)
				if (s->z >= s->sector()->floorz - (8 << 8))
				{
					deletesprite(act);
					continue;
				}

			break;

		case RRTILE2460:
		case RRTILE2465:
		case BIKEJIBA:
		case BIKEJIBB:
		case BIKEJIBC:
		case BIKERJIBA:
		case BIKERJIBB:
		case BIKERJIBC:
		case BIKERJIBD:
		case CHEERJIBA:
		case CHEERJIBB:
		case CHEERJIBC:
		case CHEERJIBD:
		case FBOATJIBA:
		case FBOATJIBB:
		case RABBITJIBA:
		case RABBITJIBB:
		case RABBITJIBC:
		case MAMAJIBA:
		case MAMAJIBB:
			if (!isRRRA()) break;

		case BILLYJIBA:
		case BILLYJIBB:
		case HULKJIBA:
		case HULKJIBB:
		case HULKJIBC:
		case MINJIBA:
		case MINJIBB:
		case MINJIBC:
		case COOTJIBA:
		case COOTJIBB:
		case COOTJIBC:
		case JIBS1:
		case JIBS2:
		case JIBS3:
		case JIBS4:
		case JIBS5:
		case JIBS6:
		case DUKETORSO:
		case DUKEGUN:
		case DUKELEG:
			if (!jibs(act, JIBS6, false, true, true, s->picnum == DUKELEG || s->picnum == DUKETORSO || s->picnum == DUKEGUN,
				isRRRA() && (s->picnum == RRTILE2465 || s->picnum == RRTILE2560))) continue;
			
			if (s->sector()->lotag == 800)
				if (s->z >= s->sector()->floorz - (8 << 8))
				{
					deletesprite(act);
					continue;
				}

			continue;

		case BLOODPOOL:
			if (!bloodpool(act, false, TIRE)) continue;

			if (s->sector()->lotag == 800)
				if (s->z >= s->sector()->floorz - (8 << 8))
				{
					deletesprite(act);
				}
			continue;

		case BURNING:
		case WATERBUBBLE:
		case SMALLSMOKE:
		case EXPLOSION2:
		case EXPLOSION3:
		case BLOOD:
		case FORCERIPPLE:
		case TRANSPORTERSTAR:
		case TRANSPORTERBEAM:
			p = findplayer(act, &x);
			execute(act, p, x);
			continue;

		case SHELL:
		case SHOTGUNSHELL:
			shell(act, false);
			continue;

		case GLASSPIECES:
		case GLASSPIECES + 1:
		case GLASSPIECES + 2:
		case POPCORN:
			glasspieces(act);
			continue;
		}

		if (s->picnum >= SCRAP6 && s->picnum <= SCRAP5 + 3)
		{
			scrap(act, SCRAP1, SCRAP6);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void handle_se06_r(DDukeActor *actor)
{
	auto s = actor->s;
	auto t = &actor->temp_data[0];

	auto sc = actor->getSector();
	int st = s->lotag;
	int sh = s->hitag;

	int k = sc->extra;

	if (t[4] > 0)
	{
		t[4]--;
		if (t[4] >= (k - (k >> 3)))
			s->xvel -= (k >> 5);
		if (t[4] > ((k >> 1) - 1) && t[4] < (k - (k >> 3)))
			s->xvel = 0;
		if (t[4] < (k >> 1))
			s->xvel += (k >> 5);
		if (t[4] < ((k >> 1) - (k >> 3)))
		{
			t[4] = 0;
			s->xvel = k;
			if ((!isRRRA() || lastlevel) && hulkspawn)
			{
				hulkspawn--;
				auto ns = spawn(actor, HULK);
				ns->s->z = ns->getSector()->ceilingz;
				ns->s->pal = 33;
				if (!hulkspawn)
				{
					ns = EGS(s->sectnum, s->x, s->y, s->sector()->ceilingz + 119428, 3677, -8, 16, 16, 0, 0, 0, actor, 5);
					ns->s->cstat = 514;
					ns->s->pal = 7;
					ns->s->xrepeat = 80;
					ns->s->yrepeat = 255;
					ns = spawn(actor, 296);
					ns->s->cstat = 0;
					ns->s->cstat |= 32768;
					ns->s->z = s->sector()->floorz - 6144;
					deletesprite(actor);
					return;
				}
			}
		}
	}
	else
	{
		s->xvel = k;
		DukeSectIterator it(s->sectnum);
		while (auto a2 = it.Next())
		{
			if (a2->s->picnum == UFOBEAM && ufospawn && ++ufocnt == 64)
			{
				int pn;
				ufocnt = 0;
				ufospawn--;
				if (!isRRRA())
				{
					switch (krand() & 3)
					{
					default:
					case 0:
						pn = UFO1_RR;
						break;
					case 1:
						pn = UFO2;
						break;
					case 2:
						pn = UFO3;
						break;
					case 3:
						pn = UFO4;
						break;
					}
				}
				else pn = UFO1_RRRA;
				auto ns = spawn(actor, pn);
				ns->s->z = ns->getSector()->ceilingz;
			}
		}
	}

	DukeStatIterator it(STAT_EFFECTOR);
	while (auto act2 = it.Next())
	{
		if ((act2->s->lotag == 14) && (sh == act2->s->hitag) && (act2->temp_data[0] == t[0]))
		{
			act2->s->xvel = s->xvel;
			//						if( t[4] == 1 )
			{
				if (act2->temp_data[5] == 0)
					act2->temp_data[5] = dist(act2, actor);
				int x = Sgn(dist(act2, actor) - act2->temp_data[5]);
				if (act2->s->extra) x = -x;
				s->xvel += x;
			}
			act2->temp_data[4] = t[4];
		}
	}
	handle_se14(actor, false, RPG, JIBS6);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveeffectors_r(void)   //STATNUM 3
{
	int l;

	clearfriction();

	DukeStatIterator it(STAT_EFFECTOR);
	while (auto act = it.Next())
	{
		auto sc = act->getSector();
		int st = act->s->lotag;
		int sh = act->s->hitag;

		auto t = &act->temp_data[0];

		switch (st)
		{
		case SE_0_ROTATING_SECTOR:
			handle_se00(act, -1);
			break;
			
		case SE_1_PIVOT: //Nothing for now used as the pivot
			handle_se01(act);
			break;
			
		case SE_6_SUBWAY:
			handle_se06_r(act);
			break;

		case SE_14_SUBWAY_CAR:
			handle_se14(act, false, RPG, JIBS6);
			break;

		case SE_30_TWO_WAY_TRAIN:
			handle_se30(act, JIBS6);
			break;


		case SE_2_EARTHQUAKE:
			handle_se02(act);
			break;

			//Flashing sector lights after reactor EXPLOSION2
		case SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT:
			handle_se03(act);
			break;

		case SE_4_RANDOM_LIGHTS:
			handle_se04(act);
			break;

			//BOSS
		case SE_5_BOSS:
			handle_se05(act, FIRELASER);
			break;

		case SE_8_UP_OPEN_DOOR_LIGHTS:
		case SE_9_DOWN_OPEN_DOOR_LIGHTS:
			handle_se08(act, true);
			break;

		case SE_10_DOOR_AUTO_CLOSE:
			handle_se10(act, nullptr);
			break;

		case SE_11_SWINGING_DOOR:
			handle_se11(act);
			break;
			
		case SE_12_LIGHT_SWITCH:
			handle_se12(act);
			break;

		case SE_47_LIGHT_SWITCH:
			if (isRRRA()) handle_se12(act, 1);
			break;
			
		case SE_48_LIGHT_SWITCH:
			if (isRRRA()) handle_se12(act, 2);
			break;
			

		case SE_13_EXPLOSIVE:
			handle_se13(act);
			break;

		case SE_15_SLIDING_DOOR:
			handle_se15(act);
			break;

		case SE_16_REACTOR:
			handle_se16(act, REACTOR, REACTOR2);
			break;

		case SE_17_WARP_ELEVATOR:
			handle_se17(act);
			break;

		case SE_18_INCREMENTAL_SECTOR_RISE_FALL:
			handle_se18(act, true);
			break;

		case SE_19_EXPLOSION_LOWERS_CEILING:
			handle_se19(act, BIGFORCE);
			break;

		case SE_20_STRETCH_BRIDGE:
			handle_se20(act);
			break;

		case SE_21_DROP_FLOOR:
			handle_se21(act);
			break;

		case SE_22_TEETH_DOOR:
			handle_se22(act);
			break;

		case 156:
			if (!isRRRA()) break;
		case SE_24_CONVEYOR:
		case SE_34:
		{
			static const int16_t list1[] = { BLOODPOOL, PUKE, FOOTPRINTS, FOOTPRINTS2, FOOTPRINTS3, -1 };
			static const int16_t list2[] = { BOLT1, BOLT1 + 1,BOLT1 + 2, BOLT1 + 3, -1 };
			handle_se24(act, list1, list2, st != 156, BULLETHOLE, -1, CRANE, 1);
			break;
		}

		case SE_35:
			handle_se35(act, SMALLSMOKE, EXPLOSION2);
			break;

		case SE_25_PISTON: //PISTONS
			if (t[4] == 0) break;
			handle_se25(act, 4, isRRRA() ? 371 : -1, isRRRA() ? 167 : -1);
			break;

		case SE_26:
			handle_se26(act);
			break;

		case SE_27_DEMO_CAM:
			handle_se27(act);
			break;

		case SE_29_WAVES:
			act->s->hitag += 64;
			l = MulScale(act->s->yvel, bsin(act->s->hitag), 12);
			sc->floorz = act->s->z + l;
			break;

		case SE_31_FLOOR_RISE_FALL: // True Drop Floor
			handle_se31(act, false);
			break;

		case SE_32_CEILING_RISE_FALL: // True Drop Ceiling
			handle_se32(act);
			break;

		case SE_33_QUAKE_DEBRIS:
			if (earthquaketime > 0 && (krand() & 7) == 0)
				RANDOMSCRAP(act);
			break;
		case SE_36_PROJ_SHOOTER:

			if (t[0])
			{
				if (t[0] == 1)
					fi.shoot(act, sc->extra);
				else if (t[0] == 26 * 5)
					t[0] = 0;
				t[0]++;
			}
			break;

		case 128: //SE to control glass breakage
			handle_se128(act);
			break;

		case SE_130:
			handle_se130(act, 80, EXPLOSION2);
			break;
		case SE_131:
			handle_se130(act, 40, EXPLOSION2);
			break;
		}
	}

	//Sloped sin-wave floors!
	it.Reset(STAT_EFFECTOR);
	while (auto act = it.Next())
	{
		auto s = act->s;
		if (act->s->lotag != SE_29_WAVES) continue;
		auto sc = act->getSector();
		if (sc->wallnum != 4) continue;
		auto wal = &wall[sc->wallptr + 2];
		alignflorslope(act->s->sectnum, wal->x, wal->y, wal->nextSector()->floorz);
	}
}


//---------------------------------------------------------------------------
//
// game specific part of makeitfall.
//
//---------------------------------------------------------------------------

int adjustfall(DDukeActor *actor, int c)
{
	if ((actor->s->picnum == BIKERB || actor->s->picnum == CHEERB) && c == gs.gravity)
		c = gs.gravity>>2;
	else if (actor->s->picnum == BIKERBV2 && c == gs.gravity)
		c = gs.gravity>>3;
	return c;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void move_r(DDukeActor *actor, int pnum, int xvel)
{
	auto spr = actor->s;
	auto t = actor->temp_data;
	int l;
	int goalang, angdif;
	int daxvel;

	int a = spr->hitag;

	if (a == -1) a = 0;

	t[0]++;

	if (a & face_player)
	{
		if (ps[pnum].newOwner != nullptr)
			goalang = getangle(ps[pnum].oposx - spr->x, ps[pnum].oposy - spr->y);
		else goalang = getangle(ps[pnum].pos.x - spr->x, ps[pnum].pos.y - spr->y);
		angdif = getincangle(spr->ang, goalang) >> 2;
		if (angdif > -8 && angdif < 0) angdif = 0;
		spr->ang += angdif;
	}

	if (a & spin)
		spr->ang += bsin(t[0] << 3, -6);

	if (a & face_player_slow)
	{
		if (ps[pnum].newOwner != nullptr)
			goalang = getangle(ps[pnum].oposx - spr->x, ps[pnum].oposy - spr->y);
		else goalang = getangle(ps[pnum].pos.x - spr->x, ps[pnum].pos.y - spr->y);
		angdif = Sgn(getincangle(spr->ang, goalang)) << 5;
		if (angdif > -32 && angdif < 0)
		{
			angdif = 0;
			spr->ang = goalang;
		}
		spr->ang += angdif;
	}

	if (isRRRA())
	{
		if (a & antifaceplayerslow)
		{
			if (ps[pnum].newOwner != nullptr)
				goalang = (getangle(ps[pnum].oposx - spr->x, ps[pnum].oposy - spr->y) + 1024) & 2047;
			else goalang = (getangle(ps[pnum].pos.x - spr->x, ps[pnum].pos.y - spr->y) + 1024) & 2047;
			angdif = Sgn(getincangle(spr->ang, goalang)) << 5;
			if (angdif > -32 && angdif < 0)
			{
				angdif = 0;
				spr->ang = goalang;
			}
			spr->ang += angdif;
		}

		if ((a & jumptoplayer) == jumptoplayer)
		{
			if (spr->picnum == CHEER)
			{
				if (t[0] < 16)
					spr->zvel -= bcos(t[0] << 4) / 40;
			}
			else
			{
				if (t[0] < 16)
					spr->zvel -= bcos(t[0] << 4, -5);
			}
		}
		if (a & justjump1)
		{
			if (spr->picnum == RABBIT)
			{
				if (t[0] < 8)
					spr->zvel -= bcos(t[0] << 4) / 30;
			}
			else if (spr->picnum == MAMA)
			{
				if (t[0] < 8)
					spr->zvel -= bcos(t[0] << 4) / 35;
			}
		}
		if (a & justjump2)
		{
			if (spr->picnum == RABBIT)
			{
				if (t[0] < 8)
					spr->zvel -= bcos(t[0] << 4) / 24;
			}
			else if (spr->picnum == MAMA)
			{
				if (t[0] < 8)
					spr->zvel -= bcos(t[0] << 4) / 28;
			}
		}
		if (a & windang)
		{
			if (t[0] < 8)
				spr->zvel -= bcos(t[0] << 4) / 24;
		}
	}
	else if ((a & jumptoplayer) == jumptoplayer)
	{
		if (t[0] < 16)
			spr->zvel -= bcos(t[0] << 4, -5);
	}


	if (a & face_player_smart)
	{
		int newx, newy;

		newx = ps[pnum].pos.x + (ps[pnum].posxv / 768);
		newy = ps[pnum].pos.y + (ps[pnum].posyv / 768);
		goalang = getangle(newx - spr->x, newy - spr->y);
		angdif = getincangle(spr->ang, goalang) >> 2;
		if (angdif > -8 && angdif < 0) angdif = 0;
		spr->ang += angdif;
	}

	if (t[1] == 0 || a == 0)
	{
		if ((badguy(actor) && spr->extra <= 0) || (spr->ox != spr->x) || (spr->oy != spr->y))
		{
			spr->backupvec2();
			setsprite(actor, spr->pos);
		}
		if (badguy(actor) && spr->extra <= 0)
		{
			if (spr->sector()->ceilingstat & 1)
			{
				if (shadedsector[spr->sectnum] == 1)
				{
					spr->shade += (16 - spr->shade) >> 1;
				}
				else
				{
					spr->shade += (spr->sector()->ceilingshade - spr->shade) >> 1;
				}
			}
			else
			{
				spr->shade += (spr->sector()->floorshade - spr->shade) >> 1;
			}
		}
		return;
	}

	auto moveptr = &ScriptCode[t[1]];

	if (a & geth) spr->xvel += (*moveptr - spr->xvel) >> 1;
	if (a & getv) spr->zvel += ((*(moveptr + 1) << 4) - spr->zvel) >> 1;

	if (a & dodgebullet)
		dodge(actor);

	if (spr->picnum != APLAYER)
		alterang(a, actor, pnum);

	if (spr->xvel > -6 && spr->xvel < 6) spr->xvel = 0;

	a = badguy(actor);

	if (spr->xvel || spr->zvel)
	{
		if (a)
		{
			if (spr->picnum == DRONE && spr->extra > 0)
			{
				if (spr->zvel > 0)
				{
					actor->floorz = l = getflorzofslope(spr->sectnum, spr->x, spr->y);
					if (isRRRA())
					{
						if (spr->z > (l - (28 << 8)))
							spr->z = l - (28 << 8);
					}
					else
					{
						if (spr->z > (l - (30 << 8)))
							spr->z = l - (30 << 8);
					}
				}
				else
				{
					actor->ceilingz = l = getceilzofslope(spr->sectnum, spr->x, spr->y);
					if ((spr->z - l) < (50 << 8))
					{
						spr->z = l + (50 << 8);
						spr->zvel = 0;
					}
				}
			}
			if (spr->zvel > 0 && actor->floorz < spr->z)
				spr->z = actor->floorz;
			if (spr->zvel < 0)
			{
				l = getceilzofslope(spr->sectnum, spr->x, spr->y);
				if ((spr->z - l) < (66 << 8))
				{
					spr->z = l + (66 << 8);
					spr->zvel >>= 1;
				}
			}
		}
		else if (spr->picnum == APLAYER)
			if ((spr->z - actor->ceilingz) < (32 << 8))
				spr->z = actor->ceilingz + (32 << 8);

		daxvel = spr->xvel;
		angdif = spr->ang;

		if (a)
		{
			if (xvel < 960 && spr->xrepeat > 16)
			{

				daxvel = -(1024 - xvel);
				angdif = getangle(ps[pnum].pos.x - spr->x, ps[pnum].pos.y - spr->y);

				if (xvel < 512)
				{
					ps[pnum].posxv = 0;
					ps[pnum].posyv = 0;
				}
				else
				{
					ps[pnum].posxv = MulScale(ps[pnum].posxv, gs.playerfriction - 0x2000, 16);
					ps[pnum].posyv = MulScale(ps[pnum].posyv, gs.playerfriction - 0x2000, 16);
				}
			}
			else if ((isRRRA() && spr->picnum != DRONE && spr->picnum != SHARK && spr->picnum != UFO1_RRRA) ||
					(!isRRRA() && spr->picnum != DRONE && spr->picnum != SHARK && spr->picnum != UFO1_RR
							&& spr->picnum != UFO2 && spr->picnum != UFO3 && spr->picnum != UFO4 && spr->picnum != UFO5))
			{
				if (spr->oz != spr->z || (ud.multimode < 2 && ud.player_skill < 2))
				{
					if ((t[0] & 1) || ps[pnum].actorsqu == actor) return;
					else daxvel <<= 1;
				}
				else
				{
					if ((t[0] & 3) || ps[pnum].actorsqu == actor) return;
					else daxvel <<= 2;
				}
			}
		}
		if (isRRRA())
		{
			if (spr->sector()->lotag != 1)
			{
				switch (spr->picnum)
				{
				case MINIONBOAT:
				case HULKBOAT:
				case CHEERBOAT:
					daxvel >>= 1;
					break;
				}
			}
			else if (spr->sector()->lotag == 1)
			{
				switch (spr->picnum)
				{
				case BIKERB:
				case BIKERBV2:
				case CHEERB:
					daxvel >>= 1;
					break;
				}
			}
		}

		Collision coll;
		actor->movflag = movesprite_ex(actor,
			MulScale(daxvel, bcos(angdif), 14),
			MulScale(daxvel, bsin(angdif), 14), spr->zvel, CLIPMASK0, coll);
	}

	if (a)
	{
		if (spr->sector()->ceilingstat & 1)
		{
			if (shadedsector[spr->sectnum] == 1)
			{
				spr->shade += (16 - spr->shade) >> 1;
			}
			else
			{
				spr->shade += (spr->sector()->ceilingshade - spr->shade) >> 1;
			}
		}
		else spr->shade += (spr->sector()->floorshade - spr->shade) >> 1;

		if (spr->sector()->floorpicnum == MIRROR)
			deletesprite(actor);
	}
}

void fakebubbaspawn(DDukeActor *actor, int g_p)
{
	fakebubba_spawn++;
	switch (fakebubba_spawn)
	{
	default:
		break;
	case 1:
		spawn(actor, PIG);
		break;
	case 2:
		spawn(actor, MINION);
		break;
	case 3:
		spawn(actor, CHEER);
		break;
	case 4:
		spawn(actor, VIXEN);
		operateactivators(666, g_p);
		break;
	}
}

//---------------------------------------------------------------------------
//
// special checks in fall that only apply to RR.
//
//---------------------------------------------------------------------------

static int fallspecial(DDukeActor *actor, int playernum)
{
	auto s = actor->s;
	int sphit = 0;
	if (isRRRA())
	{
		if (s->sector()->lotag == 801)
		{
			if (s->picnum == ROCK)
			{
				spawn(actor, ROCK2);
				spawn(actor, ROCK2);
				addspritetodelete();
			}
			return 0;
		}
		else if (s->sector()->lotag == 802)
		{
			if (s->picnum != APLAYER && badguy(actor) && s->z == actor->floorz - FOURSLEIGHT)
			{
				fi.guts(actor, JIBS6, 5, playernum);
				S_PlayActorSound(SQUISHED, actor);
				addspritetodelete();
			}
			return 0;
		}
		else if (s->sector()->lotag == 803)
		{
			if (s->picnum == ROCK2)
				addspritetodelete();
			return 0;
		}
	}
	if (s->sector()->lotag == 800)
	{
		if (s->picnum == 40)
		{
			addspritetodelete();
			return 0;
		}
		if (s->picnum != APLAYER && (badguy(actor) || s->picnum == HEN || s->picnum == COW || s->picnum == PIG || s->picnum == DOGRUN || s->picnum == RABBIT) && (!isRRRA() || actor->spriteextra < 128))
		{
			s->z = actor->floorz - FOURSLEIGHT;
			s->zvel = 8000;
			s->extra = 0;
			actor->spriteextra++;
			sphit = 1;
		}
		else if (s->picnum != APLAYER)
		{
			if (!actor->spriteextra)
				addspritetodelete();
			return 0;
		}
		actor->picnum = SHOTSPARK1;
		actor->extra = 1;
	}
	else if (isRRRA() && (s->sector()->floorpicnum == RRTILE7820 || s->sector()->floorpicnum == RRTILE7768))
	{
		if (s->picnum != MINION && s->pal != 19)
		{
			if ((krand() & 3) == 1)
			{
				actor->picnum = SHOTSPARK1;
				actor->extra = 5;
			}
		}
	}	
	return sphit;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void fall_r(DDukeActor* ac, int g_p)
{
	fall_common(ac, g_p, JIBS6, DRONE, BLOODPOOL, SHOTSPARK1, 69, 158, fallspecial);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void destroyit(DDukeActor *actor)
{
	int lotag, hitag;
	int wi, wj;
	int wallstart2, wallend2;
	int sectnum;
	int wallstart, wallend;
	DDukeActor* spr = nullptr;

	hitag = 0;
	DukeSectIterator it1(actor->s->sectnum);
	while (auto a2 = it1.Next())
	{
		if (a2->s->picnum == RRTILE63)
		{
			lotag = a2->s->lotag;
			spr = a2;
			if (a2->s->hitag)
				hitag = a2->s->hitag;
		}
	}
	DukeStatIterator it(STAT_DESTRUCT);
	while (auto a2 = it.Next())
	{
		int it_sect = a2->s->sectnum;
		if (hitag && hitag == a2->s->hitag)
		{
			DukeSectIterator its(it_sect);
			while (auto a3 = its.Next())
			{
				if (a3->s->picnum == DESTRUCTO)
				{
					a3->picnum = SHOTSPARK1;
					a3->extra = 1;
				}
			}
		}
		if (spr && spr->s->sectnum != it_sect)
			if (lotag == a2->s->lotag)
			{
				sectnum = spr->s->sectnum;

				auto destsect = spr->getSector();
				auto srcsect = &sector[it_sect];

				wallstart = destsect->wallptr;
				wallend = wallstart + destsect->wallnum;
				wallstart2 = srcsect->wallptr;
				wallend2 = wallstart2 + srcsect->wallnum;
				for (wi = wallstart, wj = wallstart2; wi < wallend; wi++, wj++)
				{
					wall[wi].picnum = wall[wj].picnum;
					wall[wi].overpicnum = wall[wj].overpicnum;
					wall[wi].shade = wall[wj].shade;
					wall[wi].xrepeat = wall[wj].xrepeat;
					wall[wi].yrepeat = wall[wj].yrepeat;
					wall[wi].xpan_ = wall[wj].xpan_;
					wall[wi].ypan_ = wall[wj].ypan_;
					if (isRRRA() && wall[wi].nextwall != -1)
					{
						wall[wi].cstat = 0;
						wall[wall[wi].nextwall].cstat = 0;
					}
				}
				destsect->floorz = srcsect->floorz;
				destsect->ceilingz = srcsect->ceilingz;
				destsect->ceilingstat = srcsect->ceilingstat;
				destsect->floorstat = srcsect->floorstat;
				destsect->ceilingpicnum = srcsect->ceilingpicnum;
				destsect->ceilingheinum = srcsect->ceilingheinum;
				destsect->ceilingshade = srcsect->ceilingshade;
				destsect->ceilingpal = srcsect->ceilingpal;
				destsect->ceilingxpan_ = srcsect->ceilingxpan_;
				destsect->ceilingypan_ = srcsect->ceilingypan_;
				destsect->floorpicnum = srcsect->floorpicnum;
				destsect->floorheinum = srcsect->floorheinum;
				destsect->floorshade = srcsect->floorshade;
				destsect->floorpal = srcsect->floorpal;
				destsect->floorxpan_ = srcsect->floorxpan_;
				destsect->floorypan_ = srcsect->floorypan_;
				destsect->visibility = srcsect->visibility;
				sectorextra[sectnum] = sectorextra[it_sect]; // TRANSITIONAL: at least rename this.
				destsect->lotag = srcsect->lotag;
				destsect->hitag = srcsect->hitag;
				destsect->extra = srcsect->extra;
			}
	}
	it1.Reset(actor->s->sectnum);
	while (auto a2 = it1.Next())
	{
		switch (a2->s->picnum)
		{
		case DESTRUCTO:
		case RRTILE63:
		case TORNADO:
		case APLAYER:
		case COOT:
			break;
		default:
			deletesprite(a2);
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void mamaspawn(DDukeActor *actor)
{
	if (mamaspawn_count)
	{
		mamaspawn_count--;
		spawn(actor, RABBIT);
	}
}

bool spawnweapondebris_r(int picnum, int dnum)
{
	return dnum == SCRAP1;
}

void respawnhitag_r(DDukeActor *actor)
{
	switch (actor->s->picnum)
	{
	case FEM10:
	case NAKED1:
	case STATUE:
		if (actor->s->yvel) fi.operaterespawns(actor->s->yvel);
		break;
	default:
		if (actor->s->hitag >= 0) fi.operaterespawns(actor->s->hitag);
		break;
	}
}

void checktimetosleep_r(DDukeActor *actor)
{
	if (actor->s->statnum == STAT_STANDABLE)
	{
		switch (actor->s->picnum)
		{
		case RUBBERCAN:
		case EXPLODINGBARREL:
		case WOODENHORSE:
		case HORSEONSIDE:
		case CANWITHSOMETHING:
		case FIREBARREL:
		case NUKEBARREL:
		case NUKEBARRELDENTED:
		case NUKEBARRELLEAKED:
		case TRIPBOMB:
		case EGG:
			if (actor->timetosleep > 1)
				actor->timetosleep--;
			else if (actor->timetosleep == 1)
				changeactorstat(actor, STAT_ZOMBIEACTOR);
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void thunder(void);

void think_r(void)
{
	thinktime.Reset();
	thinktime.Clock();
	recordoldspritepos();

	movefta_r();			//ST 2
	moveweapons_r();		//ST 4
	movetransports_r();		//ST 9
	moveplayers();			//ST 10
	movefallers_r();		//ST 12
	moveexplosions_r();		//ST 5

	actortime.Reset();
	actortime.Clock();
	moveactors_r();			//ST 1
	actortime.Unclock();

	moveeffectors_r();		//ST 3
	movestandables_r();		//ST 6
	doanimations();
	movefx();				//ST 11

	if (numplayers < 2 && thunderon)
		thunder();

	thinktime.Unclock();
}


END_DUKE_NS
