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
#include "mmulti.h"
#include "mapinfo.h"
#include "dukeactor.h"

BEGIN_DUKE_NS

void dojaildoor();
void moveminecart();

void ballreturn(short spr);
short pinsectorresetdown(short sect);
short pinsectorresetup(short sect);
short checkpins(short sect);
void resetpins(short sect);
void resetlanepics(void);


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool ceilingspace_r(int sectnum)
{
	if( (sector[sectnum].ceilingstat&1) && sector[sectnum].ceilingpal == 0 )
	{
		switch(sector[sectnum].ceilingpicnum)
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

bool floorspace_r(int sectnum)
{
	if( (sector[sectnum].floorstat&1) && sector[sectnum].ceilingpal == 0 )
	{
		switch(sector[sectnum].floorpicnum)
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
	if (actor->s.extra > 0) switch (actor->s.picnum)
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
	short cw = p->curr_weapon;
	if (p->OnMotorcycle || p->OnBoat)
	{
		p->gotweapon.Set(weapon);
		if (weapon == THROWSAW_WEAPON)
		{
			p->gotweapon.Set(BUZZSAW_WEAPON);
			p->ammo_amount[BUZZSAW_WEAPON] = 1;
		}
		else if (weapon == CROSSBOW_WEAPON)
		{
			p->gotweapon.Set(CHICKEN_WEAPON);
			p->gotweapon.Set(DYNAMITE_WEAPON);
		}
		else if (weapon == SLINGBLADE_WEAPON)
		{
			p->ammo_amount[SLINGBLADE_WEAPON] = 1;
		}
		return;
	}

	if (p->gotweapon[weapon] == 0)
	{
		p->gotweapon.Set(weapon);
		if (weapon == THROWSAW_WEAPON)
		{
			p->gotweapon.Set(BUZZSAW_WEAPON);
			if (isRRRA()) p->ammo_amount[BUZZSAW_WEAPON] = 1;
		}
		if (isRRRA())
		{
			if (weapon == CROSSBOW_WEAPON)
			{
				p->gotweapon.Set(CHICKEN_WEAPON);
			}
			if (weapon == SLINGBLADE_WEAPON)
			{
				p->ammo_amount[SLINGBLADE_WEAPON] = 50;
			}
		}
		if (weapon == CROSSBOW_WEAPON)
		{
			p->gotweapon.Set(DYNAMITE_WEAPON);
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
		S_PlayActorSound(SHOTGUN_COCK, p->i); 
		break;
	case PISTOL_WEAPON:	   
		S_PlayActorSound(INSERT_CLIP, p->i); 
		break;
	default:	  
		S_PlayActorSound(EJECT_CLIP, p->i); 
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
	int d, q, x1, y1;
	int sectcnt, sectend, dasect, startwall, endwall, nextsect;
	short p, x, sect;
	static const uint8_t statlist[] = { STAT_DEFAULT, STAT_ACTOR, STAT_STANDABLE, STAT_PLAYER, STAT_FALLER, STAT_ZOMBIEACTOR, STAT_MISC };
	short tempshort[MAXSECTORS];	// originally hijacked a global buffer which is bad. Q: How many do we really need? RedNukem says 64.

	auto spri = &actor->s;

	if (spri->xrepeat < 11)
	{
		if (spri->picnum == RPG || ((isRRRA()) && spri->picnum == RPG2)) goto SKIPWALLCHECK;
	}

	tempshort[0] = spri->sectnum;
	dasect = spri->sectnum;
	sectcnt = 0; sectend = 1;

	do
	{
		dasect = tempshort[sectcnt++];
		if (((sector[dasect].ceilingz - spri->z) >> 8) < r)
		{
			d = abs(wall[sector[dasect].wallptr].x - spri->x) + abs(wall[sector[dasect].wallptr].y - spri->y);
			if (d < r)
				fi.checkhitceiling(dasect);
			else
			{
				// ouch...
				d = abs(wall[wall[wall[sector[dasect].wallptr].point2].point2].x - spri->x) + abs(wall[wall[wall[sector[dasect].wallptr].point2].point2].y - spri->y);
				if (d < r)
					fi.checkhitceiling(dasect);
			}
		}

		startwall = sector[dasect].wallptr;
		endwall = startwall + sector[dasect].wallnum;
		for (x = startwall, wal = &wall[startwall]; x < endwall; x++, wal++)
			if ((abs(wal->x - spri->x) + abs(wal->y - spri->y)) < r)
			{
				nextsect = wal->nextsector;
				if (nextsect >= 0)
				{
					for (dasect = sectend - 1; dasect >= 0; dasect--)
						if (tempshort[dasect] == nextsect) break;
					if (dasect < 0) tempshort[sectend++] = nextsect;
				}
				x1 = (((wal->x + wall[wal->point2].x) >> 1) + spri->x) >> 1;
				y1 = (((wal->y + wall[wal->point2].y) >> 1) + spri->y) >> 1;
				updatesector(x1, y1, &sect);
				if (sect >= 0 && cansee(x1, y1, spri->z, sect, spri->x, spri->y, spri->z, spri->sectnum))
					fi.checkhitwall(actor->GetIndex(), x, wal->x, wal->y, spri->z, spri->picnum);
			}
	} while (sectcnt < sectend);

SKIPWALLCHECK:

	q = -(24 << 8) + (krand() & ((32 << 8) - 1));

	auto Owner = actor->GetOwner();
	for (x = 0; x < 7; x++)
	{
		DukeStatIterator it1(statlist[x]);
		while (auto act2 = it1.Next())
		{
			auto spri2 = &act2->s;
			if (x == 0 || x >= 5 || AFLAMABLE(spri2->picnum))
			{
				if (spri2->cstat & 257)
					if (dist(actor, act2) < r)
					{
						if (badguy(act2) && !cansee(spri2->x, spri2->y, spri2->z + q, spri2->sectnum, spri->x, spri->y, spri->z + q, spri->sectnum))
						{
							continue;
						}
						fi.checkhitsprite(act2->GetIndex(), actor->GetIndex());
					}
			}
			else if (spri2->extra >= 0 && act2 != actor && (badguy(act2) || spri2->picnum == QUEBALL || spri2->picnum == RRTILE3440 || spri2->picnum == STRIPEBALL || (spri2->cstat & 257) || spri2->picnum == DUKELYINGDEAD))
			{
				if (spri->picnum == MORTER && act2 == Owner)
				{
					continue;
				}
				if ((isRRRA()) && spri->picnum == CHEERBOMB && act2 == Owner)
				{
					continue;
				}

				if (spri2->picnum == APLAYER) spri2->z -= PHEIGHT;
				d = dist(actor, act2);
				if (spri2->picnum == APLAYER) spri2->z += PHEIGHT;

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
						spri2->picnum == STRIPEBALL || spri2->picnum == RRTILE3440)
						fi.checkhitsprite(act2->GetIndex(), actor->GetIndex());

					if (spri2->picnum != RADIUSEXPLOSION &&
						Owner && Owner->s.statnum < MAXSTATUS)
					{
						if (spri2->picnum == APLAYER)
						{
							p = act2->PlayerIndex();
							if (ps[p].newowner >= 0)
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
	int daz, h, oldx, oldy;
	short retval, dasectnum, cd;
	auto spri = &actor->s;
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

	daz = spri->z;
	h = ((tileHeight(spri->picnum) * spri->yrepeat) << 1);
	daz -= h;

	if (bg)
	{
		oldx = spri->x;
		oldy = spri->y;

		if (spri->xrepeat > 60)
			retval = clipmove(&spri->x, &spri->y, &daz, &dasectnum, ((xchange * TICSPERFRAME) << 11), ((ychange * TICSPERFRAME) << 11), 1024L, (4 << 8), (4 << 8), cliptype);
		else
		{
			cd = 192;
			retval = clipmove(&spri->x, &spri->y, &daz, &dasectnum, ((xchange * TICSPERFRAME) << 11), ((ychange * TICSPERFRAME) << 11), cd, (4 << 8), (4 << 8), cliptype);
		}

		if (dasectnum < 0 || (dasectnum >= 0 && actor->actorstayput >= 0 && actor->actorstayput != dasectnum))
		{
			spri->x = oldx;
			spri->y = oldy;
			if (sector[dasectnum].lotag == ST_1_ABOVE_WATER)
				spri->ang = (krand() & 2047);
			else if ((actor->temp_data[0] & 3) == 1)
				spri->ang = (krand() & 2047);
			setsprite(actor, oldx, oldy, spri->z);
			if (dasectnum < 0) dasectnum = 0;
			return result.setSector(dasectnum);
		}
		if ((retval & kHitTypeMask) > kHitSector && (actor->cgg == 0)) spri->ang += 768;
	}
	else
	{
		if (spri->statnum == STAT_PROJECTILE)
			retval =
			clipmove(&spri->x, &spri->y, &daz, &dasectnum, ((xchange * TICSPERFRAME) << 11), ((ychange * TICSPERFRAME) << 11), 8L, (4 << 8), (4 << 8), cliptype);
		else
			retval =
			clipmove(&spri->x, &spri->y, &daz, &dasectnum, ((xchange * TICSPERFRAME) << 11), ((ychange * TICSPERFRAME) << 11), 128, (4 << 8), (4 << 8), cliptype);
	}

	if (dasectnum >= 0)
		if ((dasectnum != spri->sectnum))
			changespritesect(actor, dasectnum);
	daz = spri->z + ((zchange * TICSPERFRAME) >> 3);
	if ((daz > actor->ceilingz) && (daz <= actor->floorz))
		spri->z = daz;
	else if (retval == 0)
		return result.setSector(dasectnum);

	return result.setFromEngine(retval);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void lotsoffeathers_r(DDukeActor *actor, short n)
{
	lotsofstuff(actor, n, MONEY);
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void guts_r(DDukeActor* actor, short gtype, short n, short p)
{
	auto s = &actor->s;
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

	gutz += actorinfo[s->picnum].gutsoffset;

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
			spawned->s.pal = pal;
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
	short j, p, psect, ssect;

	DukeStatIterator it(STAT_ZOMBIEACTOR);
	while(auto act = it.Next())
	{
		auto s = &act->s;
		p = findplayer(act, &x);
		j = 0;

		ssect = psect = s->sectnum;

		if (ps[p].GetActor()->s.extra > 0)
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
							 || (sintable[(s->ang + 512) & 2047] * (px - sx) + sintable[s->ang & 2047] * (py - sy) >= 0))
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
						if (sector[s->sectnum].ceilingstat & 1)
							s->shade = sector[s->sectnum].ceilingshade;
						else s->shade = sector[s->sectnum].floorshade;

						act->timetosleep = 0;
						changespritestat(act, STAT_STANDABLE);
						break;
					default:
#if 0
						// TRANSITIONAL: RedNukem has this here. Needed?
						if (actorflag(spriteNum, SFLAG_USEACTIVATOR) && sector[s prite[spriteNum].sectnum].lotag & 16384) break;
#endif
						act->timetosleep = 0;
						check_fta_sounds_r(act);
						changespritestat(act, STAT_ACTOR);
						break;
					}
					else act->timetosleep = 0;
				}
			}
			if (/*!j &&*/ badguy(act)) // this is like RedneckGDX. j is uninitialized here, i.e. most likely not 0.
			{
				if (sector[s->sectnum].ceilingstat & 1)
					s->shade = sector[s->sectnum].ceilingshade;
				else s->shade = sector[s->sectnum].floorshade;

				if (s->picnum != HEN || s->picnum != COW || s->picnum != PIG || s->picnum != DOGRUN || ((isRRRA()) && s->picnum != RABBIT))
				{
					if (wakeup(act, p))
					{
						act->timetosleep = 0;
						check_fta_sounds_r(act);
						changespritestat(act, STAT_ACTOR);
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
		if (a1->s.picnum == EXPLOSION2 || (a1->s.picnum == EXPLOSION3 && sectnum == a1->s.sectnum))
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
	auto spri = &actor->s;

	if (actor->extra >= 0)
	{
		if (spri->extra >= 0)
		{
			if (spri->picnum == APLAYER)
			{
				if (ud.god) return -1;

				p = actor->PlayerIndex();

				if (hitowner &&
					hitowner->s.picnum == APLAYER &&
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

						if (hitowner->s.picnum == APLAYER && p != hitowner->PlayerIndex())
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
					ps[p].posxv +=
						actor->extra * (sintable[(actor->ang + 512) & 2047]) << 2;
					ps[p].posyv +=
						actor->extra * (sintable[actor->ang & 2047]) << 2;
					break;
				default:
					ps[p].posxv +=
						actor->extra * (sintable[(actor->ang + 512) & 2047]) << 1;
					ps[p].posyv +=
						actor->extra * (sintable[actor->ang & 2047]) << 1;
					break;
				}
			}
			else
			{
				if (actor->extra == 0)
					if (spri->xrepeat < 24)
						return -1;

				spri->extra -= actor->extra;
				if (spri->picnum != RECON && actor->GetOwner() && actor->GetOwner()->s.statnum < MAXSTATUS)
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
	auto newspr = &newact->s;
	newspr->pal = oldact->s.pal;
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
	oldact->s.extra = (66 - 13);
	newspr->pal = 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movefallers_r(void)
{
	short sect, j;
	spritetype* s;
	int x;

	StatIterator it(STAT_FALLER);
	int i;
	while ((i = it.NextIndex()) >= 0)
	{
		s = &sprite[i];

		sect = s->sectnum;

		if (hittype[i].temp_data[0] == 0)
		{
			s->z -= (16 << 8);
			hittype[i].temp_data[1] = s->ang;
			x = s->extra;
			j = fi.ifhitbyweapon(&hittype[i]);
			if (j >= 0) 
			{
				if (j == RPG || (isRRRA() && j == RPG2) || j == RADIUSEXPLOSION || j == SEENINE || j == OOZFILTER)
				{
					if (s->extra <= 0)
					{
						hittype[i].temp_data[0] = 1;
						StatIterator it(STAT_FALLER);
						while ((j = it.NextIndex()) >= 0)
						{
							auto sj = &sprite[j];
							if (sj->hitag == sprite[i].hitag)
							{
								hittype[j].temp_data[0] = 1;
								sj->cstat &= (65535 - 64);
								if (sj->picnum == CEILINGSTEAM || sj->picnum == STEAM)
									sj->cstat |= 32768;
							}
						}
					}
				}
				else
				{
					hittype[i].extra = 0;
					s->extra = x;
				}
			}
			s->ang = hittype[i].temp_data[1];
			s->z += (16 << 8);
		}
		else if (hittype[i].temp_data[0] == 1)
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
					ssp(i, CLIPMASK0);
				}

				if (fi.floorspace(s->sectnum)) x = 0;
				else
				{
					if (fi.ceilingspace(s->sectnum))
						x = gc / 6;
					else
						x = gc;
				}

				if (s->z < (sector[sect].floorz - FOURSLEIGHT))
				{
					s->zvel += x;
					if (s->zvel > 6144)
						s->zvel = 6144;
					s->z += s->zvel;
				}
				if ((sector[sect].floorz - s->z) < (16 << 8))
				{
					j = 1 + (krand() & 7);
					for (x = 0; x < j; x++) RANDOMSCRAP(s, i);
					deletesprite(i);
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

static void movecrack(int i)
{
	auto s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	if (s->hitag > 0)
	{
		t[0] = s->cstat;
		t[1] = s->ang;
		int j = fi.ifhitbyweapon(&hittype[i]);
		if (j == RPG || (isRRRA() && j == RPG2) || j == RADIUSEXPLOSION || j == SEENINE || j == OOZFILTER)
		{
			StatIterator it(STAT_STANDABLE);
			while ((j = it.NextIndex()) >= 0)
			{
				auto sj = &sprite[j];
				if (s->hitag == sj->hitag && (sj->picnum == OOZFILTER || sj->picnum == SEENINE))
					if (sj->shade != -32)
						sj->shade = -32;
			}
			detonate(i, EXPLOSION2);
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

static void movebolt(int i)
{
	auto s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int x;
	int sect = s->sectnum;

	auto p = findplayer(s, &x);
	if (x > 20480) return;

	if (t[3] == 0)
		t[3] = sector[sect].floorshade;

CLEAR_THE_BOLT:
	if (t[2])
	{
		t[2]--;
		sector[sect].floorshade = 20;
		sector[sect].ceilingshade = 20;
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

	if (s->picnum == (BOLT1 + 1) && (krand() & 1) && sector[sect].floorpicnum == HURTRAIL)
		S_PlayActorSound(SHORT_CIRCUIT, i);

	if (s->picnum == BOLT1 + 4) s->picnum = BOLT1;

	if (s->picnum & 1)
	{
		sector[sect].floorshade = 0;
		sector[sect].ceilingshade = 0;
	}
	else
	{
		sector[sect].floorshade = 20;
		sector[sect].ceilingshade = 20;
	}
}

//---------------------------------------------------------------------------
//
// this has been broken up into lots of smaller subfunctions
//
//---------------------------------------------------------------------------

void movestandables_r(void)
{
	StatIterator it(STAT_STANDABLE);
	int i;
	while ((i = it.NextIndex()) >= 0)
	{
		auto s = &sprite[i];
		int picnum = s->picnum;

		if (s->sectnum < 0)
		{
			deletesprite(i);
			continue;
		}

		hittype[i].bposx = s->x;
		hittype[i].bposy = s->y;
		hittype[i].bposz = s->z;


		if (picnum >= CRANE && picnum <= CRANE +3)
		{
			movecrane(&hittype[i], CRANE);
		}

		else if (picnum >= WATERFOUNTAIN && picnum <= WATERFOUNTAIN + 3)
		{
			movefountain(&hittype[i], WATERFOUNTAIN);
		}

		else if (AFLAMABLE(picnum))
		{
			moveflammable(&hittype[i], TIRE, BOX, BLOODPOOL);
		}


		else if (picnum >= CRACK1 && picnum <= CRACK1 + 3)
		{
			movecrack(i);
		}

		else if (picnum == OOZFILTER || picnum == SEENINE || picnum == SEENINEDEAD || picnum == (SEENINEDEAD + 1))
		{
			moveooz(&hittype[i], SEENINE, SEENINEDEAD, OOZFILTER, EXPLOSION2);
		}

		else if (picnum == MASTERSWITCH)
		{
			movemasterswitch(&hittype[i], SEENINE, OOZFILTER);
		}

		else if (picnum == TRASH)
		{
			movetrash(&hittype[i]);
		}

		else if (picnum >= BOLT1 && picnum <= BOLT1 + 3)
		{
			movebolt(i);
		}

		else if (picnum == WATERDRIP)
		{
			movewaterdrip(&hittype[i], WATERDRIP);
		}

		else if (picnum == DOORSHOCK)
		{
			movedoorshock(&hittype[i]);
		}

		else if (picnum == TOUCHPLATE)
		{
			movetouchplate(&hittype[i], TOUCHPLATE);
		}

		else if (picnum == CANWITHSOMETHING)
		{
			movecanwithsomething(&hittype[i]);
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
			int p = findplayer(s, &x);
			execute(i, p, x);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void chickenarrow(int i)
{
	auto s = &sprite[i];
	s->hitag++;
	if (hittype[i].picnum != BOSS2 && s->xrepeat >= 10 && sector[s->sectnum].lotag != 2)
	{
		int j = fi.spawn(i, SMALLSMOKE);
		sprite[j].z += (1 << 8);
		if ((krand() & 15) == 2)
		{
			j = fi.spawn(i, 1310);
		}
	}
	if (sprite[s->lotag].extra <= 0)
		s->lotag = 0;
	if (s->lotag != 0 && s->hitag > 5)
	{
		spritetype* ts;
		int ang, ang2, ang3;
		ts = &sprite[s->lotag];
		ang = getangle(ts->x - s->x, ts->y - s->y);
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

static bool weaponhitsprite(int i, int j, const vec3_t &oldpos)
{
	auto s = &sprite[i];

	if (isRRRA())
	{
		if (sprite[j].picnum == MINION
			&& (s->picnum == RPG || s->picnum == RPG2)
			&& sprite[j].pal == 19)
		{
			S_PlayActorSound(RPG_EXPLODE, i);
			int k = fi.spawn(i, EXPLOSION2);
			sprite[k].pos = oldpos;
			return true;
		}
	}
	else if (s->picnum == FREEZEBLAST && sprite[j].pal == 1)
		if (badguy(&sprite[j]) || sprite[j].picnum == APLAYER)
		{
			j = fi.spawn(i, TRANSPORTERSTAR);
			sprite[j].pal = 1;
			sprite[j].xrepeat = 32;
			sprite[j].yrepeat = 32;

			deletesprite(i);
			return true;
		}

	fi.checkhitsprite(j, i);

	if (sprite[j].picnum == APLAYER)
	{
		int p = sprite[j].yvel;
		S_PlayActorSound(PISTOL_BODYHIT, j);

		if (s->picnum == SPIT)
		{
			if (isRRRA() && sprite[s->owner].picnum == MAMA)
			{
				guts_r(&hittype[i], RABBITJIBA, 2, myconnectindex);
				guts_r(&hittype[i], RABBITJIBB, 2, myconnectindex);
				guts_r(&hittype[i], RABBITJIBC, 2, myconnectindex);
			}

			ps[p].horizon.addadjustment(32);
			ps[p].sync.actions |= SB_CENTERVIEW;

			if (ps[p].loogcnt == 0)
			{
				if (!S_CheckActorSoundPlaying(ps[p].i, DUKE_LONGTERM_PAIN))
					S_PlayActorSound(DUKE_LONGTERM_PAIN, ps[p].i);

				j = 3 + (krand() & 3);
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

static bool weaponhitwall(int i, int j, const vec3_t& oldpos)
{
	auto act = &hittype[i];
	auto s = &act->s;

	if (isRRRA() && sprite[s->owner].picnum == MAMA)
	{
		guts_r(&hittype[i], RABBITJIBA, 2, myconnectindex);
		guts_r(&hittype[i], RABBITJIBB, 2, myconnectindex);
		guts_r(&hittype[i], RABBITJIBC, 2, myconnectindex);
	}

	if (s->picnum != RPG && (!isRRRA() || s->picnum != RPG2) && s->picnum != FREEZEBLAST && s->picnum != SPIT && s->picnum != SHRINKSPARK && (wall[j].overpicnum == MIRROR || wall[j].picnum == MIRROR))
	{
		int k = getangle(
			wall[wall[j].point2].x - wall[j].x,
			wall[wall[j].point2].y - wall[j].y);
		s->ang = ((k << 1) - s->ang) & 2047;
		s->owner = i;
		fi.spawn(i, TRANSPORTERSTAR);
		return true;
	}
	else
	{
		setsprite(i, &oldpos);
		fi.checkhitwall(i, j, s->x, s->y, s->z, s->picnum);

		if (!isRRRA() && s->picnum == FREEZEBLAST)
		{
			if (wall[j].overpicnum != MIRROR && wall[j].picnum != MIRROR)
			{
				s->extra >>= 1;
				if (s->xrepeat > 8)
					s->xrepeat -= 2;
				if (s->yrepeat > 8)
					s->yrepeat -= 2;
				s->yvel--;
			}

			int k = getangle(
				wall[wall[j].point2].x - wall[j].x,
				wall[wall[j].point2].y - wall[j].y);
			s->ang = ((k << 1) - s->ang) & 2047;
			return true;
		}
		if (s->picnum == SHRINKSPARK)
		{
			if (wall[j].picnum >= RRTILE3643 && wall[j].picnum < RRTILE3643 + 3)
			{
				deletesprite(i);
			}
			if (s->extra <= 0)
			{
				s->x += sintable[(s->ang + 512) & 2047] >> 7;
				s->y += sintable[s->ang & 2047] >> 7;
				if (!isRRRA() || (sprite[s->owner].picnum != CHEER && sprite[s->owner].picnum != CHEERSTAYPUT))
				{
					auto j = spawn(act, CIRCLESTUCK);
					j->s.xrepeat = 8;
					j->s.yrepeat = 8;
					j->s.cstat = 16;
					j->s.ang = (j->s.ang + 512) & 2047;
					j->s.clipdist = mulscale7(s->xrepeat, tilesiz[s->picnum].x);
				}
				deletesprite(i);
				return true;
			}
			if (wall[j].overpicnum != MIRROR && wall[j].picnum != MIRROR)
			{
				s->extra -= 20;
				s->yvel--;
			}

			int k = getangle(
				wall[wall[j].point2].x - wall[j].x,
				wall[wall[j].point2].y - wall[j].y);
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

bool weaponhitsector(int i, const vec3_t& oldpos)
{
	auto s = &sprite[i];

	setsprite(i, &oldpos);

	if (isRRRA() && sprite[s->owner].picnum == MAMA)
	{
		guts_r(&hittype[i], RABBITJIBA, 2, myconnectindex);
		guts_r(&hittype[i], RABBITJIBB, 2, myconnectindex);
		guts_r(&hittype[i], RABBITJIBC, 2, myconnectindex);
	}

	if (s->zvel < 0)
	{
		if (sector[s->sectnum].ceilingstat & 1)
			if (sector[s->sectnum].ceilingpal == 0)
			{
				deletesprite(i);
				return true;
			}

		fi.checkhitceiling(s->sectnum);
	}

	if (!isRRRA() && s->picnum == FREEZEBLAST)
	{
		bounce(&hittype[i]);
		ssp(i, CLIPMASK1);
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

static void weaponcommon_r(int i)
{
	int j, k, p;
	int x, ll;

	auto s = &sprite[i];
	p = -1;

	if (s->picnum == RPG && sector[s->sectnum].lotag == 2)
	{
		k = s->xvel >> 1;
		ll = s->zvel >> 1;
	}
	else if (isRRRA() && s->picnum == RPG2 && sector[s->sectnum].lotag == 2)
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

	getglobalz(i);

	switch (s->picnum)
	{
	case RPG:
		if (hittype[i].picnum != BOSS2 && s->xrepeat >= 10 && sector[s->sectnum].lotag != 2)
		{
			j = fi.spawn(i, SMALLSMOKE);
			sprite[j].z += (1 << 8);
		}
		break;
	case RPG2:
		if (!isRRRA()) break;
		chickenarrow(i);
		break;

	case RRTILE1790:
		if (!isRRRA()) break;
		if (s->extra)
		{
			s->zvel = -(s->extra * 250);
			s->extra--;
		}
		else
			makeitfall(i);
		if (s->xrepeat >= 10 && sector[s->sectnum].lotag != 2)
		{
			j = fi.spawn(i, SMALLSMOKE);
			sprite[j].z += (1 << 8);
		}
		break;
	}

	j = movesprite_r(i,
		(k * (sintable[(s->ang + 512) & 2047])) >> 14,
		(k * (sintable[s->ang & 2047])) >> 14, ll, CLIPMASK1);

	if ((s->picnum == RPG || (isRRRA() && isIn(s->picnum, RPG2, RRTILE1790))) && s->yvel >= 0)
		if (FindDistance2D(s->x - sprite[s->yvel].x, s->y - sprite[s->yvel].y) < 256)
			j = 49152 | s->yvel;

	if (s->sectnum < 0) // || (isRR() && sector[s->sectnum].filler == 800))
	{
		deletesprite(i);
		return;
	}

	if ((j & kHitTypeMask) != kHitSprite && s->picnum != FREEZEBLAST)
	{
		if (s->z < hittype[i].ceilingz)
		{
			j = kHitSector | (s->sectnum);
			s->zvel = -1;
		}
		else
			if (s->z > hittype[i].floorz)
			{
				j = kHitSector | (s->sectnum);
				if (sector[s->sectnum].lotag != 1)
					s->zvel = 1;
			}
	}

	if (s->picnum == FIRELASER)
	{
		for (k = -3; k < 2; k++)
		{
			x = EGS(s->sectnum,
				s->x + ((k * sintable[(s->ang + 512) & 2047]) >> 9),
				s->y + ((k * sintable[s->ang & 2047]) >> 9),
				s->z + ((k * ksgn(s->zvel)) * abs(s->zvel / 24)), FIRELASER, -40 + (k << 2),
				s->xrepeat, s->yrepeat, 0, 0, 0, s->owner, 5);

			sprite[x].cstat = 128;
			sprite[x].pal = s->pal;
		}
	}
	else if (s->picnum == SPIT) if (s->zvel < 6144)
		s->zvel += gc - 112;

	if (j != 0)
	{
		if ((j & kHitTypeMask) == kHitSprite)
		{
			j &= kHitIndexMask;
			if (weaponhitsprite(i, j, oldpos)) return;
		}
		else if ((j & kHitTypeMask) == kHitWall)
		{
			j &= kHitIndexMask;
			if (weaponhitwall(i, j, oldpos)) return;
		}
		else if ((j & 49152) == 16384)
		{
			if (weaponhitsector(i, oldpos)) return;
		}

		if (s->picnum != SPIT)
		{
			if (s->picnum == RPG) rpgexplode(&hittype[i], j, oldpos, EXPLOSION2, -1, -1, RPG_EXPLODE);
			else if (isRRRA() && s->picnum == RPG2) rpgexplode(&hittype[i], j, oldpos, EXPLOSION2, -1, 150, 247);
			else if (isRRRA() && s->picnum == RRTILE1790) rpgexplode(&hittype[i], j, oldpos, EXPLOSION2, -1, 160, RPG_EXPLODE);
			else if (s->picnum != FREEZEBLAST && s->picnum != FIRELASER && s->picnum != SHRINKSPARK)
			{
				k = fi.spawn(i, 1441);
				sprite[k].xrepeat = sprite[k].yrepeat = s->xrepeat >> 1;
				if ((j & kHitTypeMask) == kHitSector)
				{
					if (s->zvel < 0)
					{
						sprite[k].cstat |= 8; sprite[k].z += (72 << 8);
					}
				}
			}
		}
		deletesprite(i);
		return;
	}
	if ((s->picnum == RPG || (isRRRA() && s->picnum == RPG2)) && sector[s->sectnum].lotag == 2 && s->xrepeat >= 10 && rnd(184))
		fi.spawn(i, WATERBUBBLE);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveweapons_r(void)
{
	StatIterator it(STAT_PROJECTILE);
	int i;
	while ((i = it.NextIndex()) >= 0)
	{
		auto s = &sprite[i];

		if (s->sectnum < 0)
		{
			deletesprite(i);
			continue;
		}

		hittype[i].bposx = s->x;
		hittype[i].bposy = s->y;
		hittype[i].bposz = s->z;

		switch (s->picnum)
		{
		case RADIUSEXPLOSION:
			deletesprite(i);
			continue;
		case TONGUE:
			movetongue(&hittype[i], TONGUE, INNERJAW);
			continue;

		case FREEZEBLAST:
			if (s->yvel < 1 || s->extra < 2 || (s->xvel | s->zvel) == 0)
			{
				int j = fi.spawn(i, TRANSPORTERSTAR);
				sprite[j].pal = 1;
				sprite[j].xrepeat = 32;
				sprite[j].yrepeat = 32;
				deletesprite(i);
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
			weaponcommon_r(i);
			continue;


		case SHOTSPARK1:
		{
			int x;
			int p = findplayer(s, &x);
			execute(i, p, x);
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
	char warpdir, warpspriteto;
	short k, p, sect, sectlotag;
	int i, j, ll2, ll, onfloorz;

	 //Transporters

	StatIterator iti(STAT_TRANSPORT);
	while ((i = iti.NextIndex()) >= 0)
	{
		auto spri = &sprite[i];
		auto hiti = &hittype[i];
		auto spriowner = spri->owner < 0? nullptr : &sprite[spri->owner];

		sect = spri->sectnum;
		sectlotag = sector[sect].lotag;

		auto& OW = spri->owner;
		auto PN = spri->picnum;
		if (OW == i)
		{
			continue;
		}

		onfloorz = hiti->temp_data[4];

		if (hiti->temp_data[0] > 0) hiti->temp_data[0]--;

		SectIterator itj(sect);
		while ((j = itj.NextIndex()) >= 0)
		{
			auto sprj = &sprite[j];
			auto hitj = &hittype[j];

			switch (sprj->statnum)
			{
			case STAT_PLAYER:	// Player

				if (sprj->owner != -1)
				{
					p = sprj->yvel;

					ps[p].on_warping_sector = 1;

					if (ps[p].transporter_hold == 0 && ps[p].jumping_counter == 0)
					{
						if (ps[p].on_ground && sectlotag == 0 && onfloorz && ps[p].jetpack_on == 0)
						{
							fi.spawn(i, TRANSPORTERBEAM);
							S_PlayActorSound(TELEPORTER, i);

							for (k = connecthead; k >= 0; k = connectpoint2[k])// connectpoinhittype[i].temp_data[1][k])
								if (ps[k].cursectnum == spriowner->sectnum)
								{
									ps[k].frag_ps = p;
									sprite[ps[k].i].extra = 0;
								}

							ps[p].angle.ang = buildang(spriowner->ang);

							if (spriowner->owner != OW)
							{
								hiti->temp_data[0] = 13;
								hittype[OW].temp_data[0] = 13;
								ps[p].transporter_hold = 13;
							}

							ps[p].bobposx = ps[p].oposx = ps[p].posx = spriowner->x;
							ps[p].bobposy = ps[p].oposy = ps[p].posy = spriowner->y;
							ps[p].oposz = ps[p].posz = spriowner->z - (PHEIGHT - (4 << 8));

							changespritesect(j, spriowner->sectnum);
							ps[p].cursectnum = sprj->sectnum;

							k = fi.spawn(OW, TRANSPORTERBEAM);
							S_PlayActorSound(TELEPORTER, k);

							break;
						}
					}
					else break;

					if (onfloorz == 0 && abs(spri->z - ps[p].posz) < 6144)
						if ((ps[p].jetpack_on == 0) || (ps[p].jetpack_on && PlayerInput(p, SB_JUMP)) ||
							(ps[p].jetpack_on && PlayerInput(p, SB_CROUCH)))
						{
							ps[p].oposx = ps[p].posx += spriowner->x - spri->x;
							ps[p].oposy = ps[p].posy += spriowner->y - spri->y;

							if (ps[p].jetpack_on && (PlayerInput(p, SB_JUMP) || ps[p].jetpack_on < 11))
								ps[p].posz = spriowner->z - 6144;
							else ps[p].posz = spriowner->z + 6144;
							ps[p].oposz = ps[p].posz;

							changespritesect(j, spriowner->sectnum);
							ps[p].cursectnum = spriowner->sectnum;

							break;
						}

					k = 0;

					if (isRRRA())
					{
						if (onfloorz && sectlotag == 160 && ps[p].posz > (sector[sect].floorz - (48 << 8)))
						{
							k = 2;
							ps[p].oposz = ps[p].posz =
								sector[spriowner->sectnum].ceilingz + (7 << 8);
						}

						if (onfloorz && sectlotag == 161 && ps[p].posz < (sector[sect].ceilingz + (6 << 8)))
						{
							k = 2;
							if (sprite[ps[p].i].extra <= 0) break;
							ps[p].oposz = ps[p].posz =
								sector[spriowner->sectnum].floorz - (49 << 8);
						}
					}

					if ((onfloorz && sectlotag == ST_1_ABOVE_WATER && ps[p].posz > (sector[sect].floorz - (6 << 8))) ||
						(onfloorz && sectlotag == ST_1_ABOVE_WATER && ps[p].OnMotorcycle))
					{
						if (ps[p].OnBoat) break;
						k = 1;
						if (screenpeek == p)
						{
							FX_StopAllSounds();
						}
						S_PlayActorSound(DUKE_UNDERWATER, ps[p].i);
						ps[p].oposz = ps[p].posz =
							sector[spriowner->sectnum].ceilingz + (7 << 8);
						if (ps[p].OnMotorcycle)
							ps[p].moto_underwater = 1;
					}

					if (onfloorz && sectlotag == ST_2_UNDERWATER && ps[p].posz < (sector[sect].ceilingz + (6 << 8)))
					{
						k = 1;
						if (sprite[ps[p].i].extra <= 0) break;
						if (screenpeek == p)
						{
							FX_StopAllSounds();
						}
						S_PlayActorSound(DUKE_GASP, ps[p].i);

						ps[p].oposz = ps[p].posz =
							sector[spriowner->sectnum].floorz - (7 << 8);
					}

					if (k == 1)
					{
						ps[p].oposx = ps[p].posx += spriowner->x - spri->x;
						ps[p].oposy = ps[p].posy += spriowner->y - spri->y;

						if (spriowner->owner != OW)
							ps[p].transporter_hold = -2;
						ps[p].cursectnum = spriowner->sectnum;

						changespritesect(j, spriowner->sectnum);

						setpal(&ps[p]);

						if ((krand() & 255) < 32)
							fi.spawn(ps[p].i, WATERSPLASH2);
					}
					else if (isRRRA() && k == 2)
					{
						ps[p].oposx = ps[p].posx += spriowner->x - spri->x;
						ps[p].oposy = ps[p].posy += spriowner->y - spri->y;

						if (spriowner->owner != OW)
							ps[p].transporter_hold = -2;
						ps[p].cursectnum = spriowner->sectnum;

						changespritesect(j, spriowner->sectnum);
					}
				}
				break;

			case STAT_ACTOR:
				if (PN == SHARK ||
					(isRRRA() && (PN == CHEERBOAT || PN == HULKBOAT || PN == MINIONBOAT || PN == UFO1_RRRA)) ||
					(!isRRRA() && (PN == UFO1_RR || PN == UFO2 || PN == UFO3 || PN == UFO4 || PN == UFO5))) continue;
			case STAT_PROJECTILE:
			case STAT_MISC:
			case STAT_DUMMYPLAYER:

				ll = abs(sprj->zvel);
				if (isRRRA())
				{
					if (sprj->zvel >= 0)
						warpdir = 2;
					else
						warpdir = 1;
				}

				{
					warpspriteto = 0;
					if (ll && sectlotag == ST_2_UNDERWATER && sprj->z < (sector[sect].ceilingz + ll))
						warpspriteto = 1;

					if (ll && sectlotag == ST_1_ABOVE_WATER && sprj->z > (sector[sect].floorz - ll))
						if (!isRRRA() || (sprj->picnum != CHEERBOAT && sprj->picnum != HULKBOAT && sprj->picnum != MINIONBOAT))
							warpspriteto = 1;

					if (isRRRA())
					{
						if (ll && sectlotag == 161 && sprj->z < (sector[sect].ceilingz + ll) && warpdir == 1)
						{
							warpspriteto = 1;
							ll2 = ll - abs(sprj->z - sector[sect].ceilingz);
						}
						else if (sectlotag == 161 && sprj->z < (sector[sect].ceilingz + 1000) && warpdir == 1)
						{
							warpspriteto = 1;
							ll2 = 1;
						}
						if (ll && sectlotag == 160 && sprj->z > (sector[sect].floorz - ll) && warpdir == 2)
						{
							warpspriteto = 1;
							ll2 = ll - abs(sector[sect].floorz - sprj->z);
						}
						else if (sectlotag == 160 && sprj->z > (sector[sect].floorz - 1000) && warpdir == 2)
						{
							warpspriteto = 1;
							ll2 = 1;
						}
					}

					if (sectlotag == 0 && (onfloorz || abs(sprj->z - spri->z) < 4096))
					{
						if (spriowner->owner != OW && onfloorz && hiti->temp_data[0] > 0 && sprj->statnum != 5)
						{
							hiti->temp_data[0]++;
							continue;
						}
						warpspriteto = 1;
					}

					if (warpspriteto) switch (sprj->picnum)
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
							sprj->cstat &= 32767;
							break;
						}
					default:
						if (sprj->statnum == 5 && !(sectlotag == ST_1_ABOVE_WATER || sectlotag == ST_2_UNDERWATER || (isRRRA() && (sectlotag == 160 || sectlotag == 161))))
							break;

					case WATERBUBBLE:
						if (rnd(192) && sprj->picnum == WATERBUBBLE)
							break;

						if (sectlotag > 0)
						{
							k = fi.spawn(j, WATERSPLASH2);
							if (sectlotag == 1 && sprj->statnum == 4)
							{
								sprite[k].xvel = sprj->xvel >> 1;
								sprite[k].ang = sprj->ang;
								ssp(k, CLIPMASK0);
							}
						}

						switch (sectlotag)
						{
						case ST_0_NO_EFFECT:
							if (onfloorz)
							{
								if (checkcursectnums(sect) == -1 && checkcursectnums(spriowner->sectnum) == -1)
								{
									sprj->x += (spriowner->x - spri->x);
									sprj->y += (spriowner->y - spri->y);
									sprj->z -= spri->z - sector[spriowner->sectnum].floorz;
									sprj->ang = spriowner->ang;

									hitj->bposx = sprj->x;
									hitj->bposy = sprj->y;
									hitj->bposz = sprj->z;

									k = fi.spawn(i, TRANSPORTERBEAM);
									S_PlayActorSound(TELEPORTER, k);

									k = fi.spawn(OW, TRANSPORTERBEAM);
									S_PlayActorSound(TELEPORTER, k);

									if (spriowner->owner != OW)
									{
										hiti->temp_data[0] = 13;
										hittype[OW].temp_data[0] = 13;
									}

									changespritesect(j, spriowner->sectnum);
								}
							}
							else
							{
								sprj->x += (spriowner->x - spri->x);
								sprj->y += (spriowner->y - spri->y);
								sprj->z = spriowner->z + 4096;

								hitj->bposx = sprj->x;
								hitj->bposy = sprj->y;
								hitj->bposz = sprj->z;

								changespritesect(j, spriowner->sectnum);
							}
							break;
						case ST_1_ABOVE_WATER:
							sprj->x += (spriowner->x - spri->x);
							sprj->y += (spriowner->y - spri->y);
							sprj->z = sector[spriowner->sectnum].ceilingz + ll;

							hitj->bposx = sprj->x;
							hitj->bposy = sprj->y;
							hitj->bposz = sprj->z;

							changespritesect(j, spriowner->sectnum);

							break;
						case ST_2_UNDERWATER:
							sprj->x += (spriowner->x - spri->x);
							sprj->y += (spriowner->y - spri->y);
							sprj->z = sector[spriowner->sectnum].floorz - ll;

							hitj->bposx = sprj->x;
							hitj->bposy = sprj->y;
							hitj->bposz = sprj->z;

							changespritesect(j, spriowner->sectnum);

							break;

						case 160:
							if (!isRRRA()) break;
							sprj->x += (spriowner->x - spri->x);
							sprj->y += (spriowner->y - spri->y);
							sprj->z = sector[spriowner->sectnum].ceilingz + ll2;

							hitj->bposx = sprj->x;
							hitj->bposy = sprj->y;
							hitj->bposz = sprj->z;

							changespritesect(j, spriowner->sectnum);

							fi.movesprite(j, (sprj->xvel * sintable[(sprj->ang + 512) & 2047]) >> 14,
								(sprj->xvel * sintable[sprj->ang & 2047]) >> 14, 0, CLIPMASK1);

							break;
						case 161:
							if (!isRRRA()) break;
							sprj->x += (spriowner->x - spri->x);
							sprj->y += (spriowner->y - spri->y);
							sprj->z = sector[spriowner->sectnum].floorz - ll2;

							hitj->bposx = sprj->x;
							hitj->bposy = sprj->y;
							hitj->bposz = sprj->z;

							changespritesect(j, spriowner->sectnum);

							fi.movesprite(j, (sprj->xvel * sintable[(sprj->ang + 512) & 2047]) >> 14,
								(sprj->xvel * sintable[sprj->ang & 2047]) >> 14, 0, CLIPMASK1);

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
	int i, j;

	StatIterator it(117);
	while ((i = it.NextIndex()) >= 0)
	{
		auto s = &sprite[i];
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
				fi.spawn(i, s->lotag);
				deletesprite(i);
			}
		}
		j = fi.movesprite(i, 0, 0, s->extra * 2, CLIPMASK0);
	}

	it.Reset(118);
	while ((i = it.NextIndex()) >= 0)
	{
		auto s = &sprite[i];
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
		j = fi.movesprite(i, 0, 0, s->extra, CLIPMASK0);
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
		short ti;
		for (ti = 0; ti < MAXSPRITES; ti++)
		{
			auto tispr = &sprite[ti];
			switch (tispr->picnum)
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
					tispr->xrepeat = tispr->xrepeat << 1;
					tispr->yrepeat = tispr->yrepeat << 1;
					tispr->clipdist = mulscale7(tispr->xrepeat, tilesiz[tispr->picnum].x);
				}
				else if (enemysizecheat == 2)
				{
					tispr->xrepeat = tispr->xrepeat >> 1;
					tispr->yrepeat = tispr->yrepeat >> 1;
					tispr->clipdist = mulscale7(tispr->xrepeat, tilesiz[tispr->picnum].y);
				}
				break;
			}

		}
		enemysizecheat = 0;
	}

	it.Reset(121);
	while ((i = it.NextIndex()) >= 0)
	{
		auto s = &sprite[i];
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
			fi.movesprite(i, 0, 0, -300, CLIPMASK0);
			if (sector[s->sectnum].ceilingz + (4 << 8) > s->z)
			{
				s->picnum = 0;
				s->extra = 100;
			}
		}
		else if (s->extra == 200)
		{
			setsprite(i, s->x, s->y, sector[s->sectnum].floorz - 10);
			s->extra = 1;
			s->picnum = PIG + 11;
			fi.spawn(i, TRANSPORTERSTAR);
		}
	}

	it.Reset(119);
	while ((i = it.NextIndex()) >= 0)
	{
		auto s = &sprite[i];
		if (s->hitag > 0)
		{
			if (s->extra == 0)
			{
				s->hitag--;
				s->extra = 150;
				fi.spawn(i, RABBIT);
			}
			else
				s->extra--;
		}
	}
	it.Reset(116);
	while ((i = it.NextIndex()) >= 0)
	{
		auto s = &sprite[i];
		if (s->extra)
		{
			if (s->extra == s->lotag)
				S_PlaySound(183);
			s->extra--;
			j = fi.movesprite(i,
				(s->hitag * sintable[(s->ang + 512) & 2047]) >> 14,
				(s->hitag * sintable[s->ang & 2047]) >> 14,
				s->hitag << 1, CLIPMASK0);
			if (j > 0)
			{
				S_PlayActorSound(PIPEBOMB_EXPLODE, i);
				deletesprite(i);
			}
			if (s->extra == 0)
			{
				S_PlaySound(215);
				deletesprite(i);
				earthquaketime = 32;
				SetPlayerPal(&ps[myconnectindex], PalEntry(32, 32, 32, 48));
			}
		}
	}

	it.Reset(115);
	while ((i = it.NextIndex()) >= 0)
	{
		auto s = &sprite[i];
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
						fi.spawn(i, BATTERYAMMO);
						ps[screenpeek].SlotWin |= 1;
						S_PlayActorSound(52, i);
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
						fi.spawn(i, HEAVYHBOMB);
						ps[screenpeek].SlotWin |= 2;
						S_PlayActorSound(52, i);
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
						fi.spawn(i, SIXPAK);
						ps[screenpeek].SlotWin |= 4;
						S_PlayActorSound(52, i);
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
						fi.spawn(i, ATOMICHEALTH);
						ps[screenpeek].SlotWin |= 8;
						S_PlayActorSound(52, i);
					}
				}
			}
		}
	}

	it.Reset(122);
	while ((i = it.NextIndex()) >= 0)
	{
		auto s = &sprite[i];
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
						fi.spawn(i, BATTERYAMMO);
						ps[screenpeek].SlotWin |= 1;
						S_PlayActorSound(342, i);
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
						fi.spawn(i, HEAVYHBOMB);
						ps[screenpeek].SlotWin |= 2;
						S_PlayActorSound(342, i);
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
						fi.spawn(i, SIXPAK);
						ps[screenpeek].SlotWin |= 4;
						S_PlayActorSound(342, i);
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
						fi.spawn(i, ATOMICHEALTH);
						ps[screenpeek].SlotWin |= 8;
						S_PlayActorSound(342, i);
					}
				}
			}
		}
	}

	it.Reset(123);
	while ((i = it.NextIndex()) >= 0)
	{
		auto s = &sprite[i];
		if (s->lotag == 5)
			if (!S_CheckSoundPlaying(330))
				S_PlayActorSound(330, i);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void rr_specialstats()
{
	int x;
	int i, j, p, pi;
	unsigned short pst;

	StatIterator it(107);
	while ((i = it.NextIndex()) >= 0)
	{
		auto s = &sprite[i];

		if (s->hitag == 100)
		{
			s->z += (4 << 8);
			if (s->z >= sector[s->sectnum].floorz + 15168)
				s->z = sector[s->sectnum].floorz + 15168;
		}

		if (s->picnum == LUMBERBLADE)
		{
			s->extra++;
			if (s->extra == 192)
			{
				s->hitag = 0;
				s->z = sector[s->sectnum].floorz - 15168;
				s->extra = 0;
				s->picnum = RRTILE3410;
				StatIterator itj(STAT_DEFAULT);
				while ((j = itj.NextIndex()) >= 0)
				{
					auto sprj = &sprite[j];
					if (sprj->picnum == 128)
						if (sprj->hitag == 999)
							sprj->picnum = 127;
				}
			}
		}
	}

	if (chickenplant)
	{
		it.Reset(106);
		while ((i = it.NextIndex()) >= 0)
		{
			auto s = &sprite[i];
			switch (s->picnum)
			{
			case RRTILE285:
				s->lotag--;
				if (s->lotag < 0)
				{
					j = fi.spawn(i, RRTILE3190);
					sprite[j].ang = s->ang;
					s->lotag = 128;
				}
				break;
			case RRTILE286:
				s->lotag--;
				if (s->lotag < 0)
				{
					j = fi.spawn(i, RRTILE3192);
					sprite[j].ang = s->ang;
					s->lotag = 256;
				}
				break;
			case RRTILE287:
				s->lotag--;
				if (s->lotag < 0)
				{
					lotsoffeathers_r(&hittype[i], (krand() & 3) + 4);
					s->lotag = 84;
				}
				break;
			case RRTILE288:
				s->lotag--;
				if (s->lotag < 0)
				{
					j = fi.spawn(i, RRTILE3132);
					s->lotag = 96;
					if (!isRRRA()) S_PlayActorSound(472, j);
				}
				break;
			case RRTILE289:
				s->lotag--;
				if (s->lotag < 0)
				{
					j = fi.spawn(i, RRTILE3120);
					sprite[j].ang = s->ang;
					s->lotag = 448;
				}
				break;
			case RRTILE290:
				s->lotag--;
				if (s->lotag < 0)
				{
					j = fi.spawn(i, RRTILE3122);
					sprite[j].ang = s->ang;
					s->lotag = 64;
				}
				break;
			case RRTILE291:
				s->lotag--;
				if (s->lotag < 0)
				{
					j = fi.spawn(i, RRTILE3123);
					sprite[j].ang = s->ang;
					s->lotag = 512;
				}
				break;
			case RRTILE292:
				s->lotag--;
				if (s->lotag < 0)
				{
					j = fi.spawn(i, RRTILE3124);
					sprite[j].ang = s->ang;
					s->lotag = 224;
				}
				break;
			case RRTILE293:
				s->lotag--;
				if (s->lotag < 0)
				{
					guts_r(&hittype[i], JIBS1, 1, myconnectindex);
					guts_r(&hittype[i], JIBS2, 1, myconnectindex);
					guts_r(&hittype[i], JIBS3, 1, myconnectindex);
					guts_r(&hittype[i], JIBS4, 1, myconnectindex);
					s->lotag = 256;
				}
				break;
			}
		}
	}

	it.Reset(STAT_BOWLING);
	while ((i = it.NextIndex()) >= 0)
	{
		auto s = &sprite[i];
		if (s->picnum == RRTILE280)
			if (s->lotag == 100)
			{
				pst = pinsectorresetup(s->sectnum);
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
	while ((i = it.NextIndex()) >= 0)
	{
		auto s = &sprite[i];
		if (s->picnum == RRTILE296)
		{
			p = findplayer(s, &x);
			if (x < 2047)
			{
				StatIterator itj(108);
				while ((j = itj.NextIndex()) >= 0)
				{
					auto sprj = &sprite[j];
					if (sprj->picnum == RRTILE297)
					{
						ps[p].angle.ang = buildang(sprj->ang);
						ps[p].bobposx = ps[p].oposx = ps[p].posx = sprj->x;
						ps[p].bobposy = ps[p].oposy = ps[p].posy = sprj->y;
						ps[p].oposz = ps[p].posz = sprj->z - (36 << 8);
						pi = ps[p].i;
						changespritesect(pi, sprj->sectnum);
						ps[p].cursectnum = sprite[pi].sectnum;
						S_PlayActorSound(70, j);
						deletesprite(j);
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

static void heavyhbomb(int i)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int sect = s->sectnum;
	int x, j, l;

	if ((s->cstat & 32768))
	{
		t[2]--;
		if (t[2] <= 0)
		{
			S_PlayActorSound(TELEPORTER, i);
			fi.spawn(i, TRANSPORTERSTAR);
			s->cstat = 257;
		}
		return;
	}

	int p = findplayer(s, &x);

	if (x < 1220) s->cstat &= ~257;
	else s->cstat |= 257;

	if (t[3] == 0)
	{
		j = fi.ifhitbyweapon(&hittype[i]);
		if (j >= 0)
		{
			t[3] = 1;
			t[4] = 0;
			l = 0;
			s->xvel = 0;
			goto DETONATEB;
		}
	}

	makeitfall(i);

	if (sector[sect].lotag != 1 && (!isRRRA() || sector[sect].lotag != 160) && s->z >= hittype[i].floorz - (FOURSLEIGHT) && s->yvel < 3)
	{
		if (s->yvel > 0 || (s->yvel == 0 && hittype[i].floorz == sector[sect].floorz))
		{
			if (s->picnum != CHEERBOMB)
				S_PlayActorSound(PIPEBOMB_BOUNCE, i);
			else
			{
				t[3] = 1;
				t[4] = 1;
				l = 0;
				goto DETONATEB;
			}
		}
		s->zvel = -((4 - s->yvel) << 8);
		if (sector[s->sectnum].lotag == 2)
			s->zvel >>= 2;
		s->yvel++;
	}
	if (s->picnum != CHEERBOMB && s->z < hittype[i].ceilingz + (16 << 8) && sector[sect].lotag != 2)
	{
		s->z = hittype[i].ceilingz + (16 << 8);
		s->zvel = 0;
	}

	j = fi.movesprite(i,
		(s->xvel * (sintable[(s->ang + 512) & 2047])) >> 14,
		(s->xvel * (sintable[s->ang & 2047])) >> 14,
		s->zvel, CLIPMASK0);

	if (sector[sprite[i].sectnum].lotag == 1 && s->zvel == 0)
	{
		s->z += (32 << 8);
		if (t[5] == 0)
		{
			t[5] = 1;
			fi.spawn(i, WATERSPLASH2);
			if (isRRRA() && s->picnum == MORTER)
				s->xvel = 0;
		}
	}
	else t[5] = 0;

	if (t[3] == 0 && s->picnum == MORTER && (j || x < 844))
	{
		t[3] = 1;
		t[4] = 0;
		l = 0;
		s->xvel = 0;
		goto DETONATEB;
	}

	if (t[3] == 0 && s->picnum == CHEERBOMB && (j || x < 844))
	{
		t[3] = 1;
		t[4] = 0;
		l = 0;
		s->xvel = 0;
		goto DETONATEB;
	}

	if (sprite[s->owner].picnum == APLAYER)
		l = sprite[s->owner].yvel;
	else l = -1;

	if (s->xvel > 0)
	{
		s->xvel -= 5;
		if (sector[sect].lotag == 2)
			s->xvel -= 10;

		if (s->xvel < 0)
			s->xvel = 0;
		if (s->xvel & 8) s->cstat ^= 4;
	}

	if ((j & 49152) == 32768)
	{
		j &= (MAXWALLS - 1);

		fi.checkhitwall(i, j, s->x, s->y, s->z, s->picnum);

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
			case TRIPBOMBSPRITE: m = tripbombblastradius; break;	// powder keg
			case HEAVYHBOMB: m = pipebombblastradius; break;
			case HBOMBAMMO: m = pipebombblastradius; break;
			case MORTER: m = morterblastradius; break;
			case CHEERBOMB: m = morterblastradius; break;
			}

			if (sector[s->sectnum].lotag != 800)
			{
				fi.hitradius(&hittype[i], m, x >> 2, x >> 1, x - (x >> 2), x);
				fi.spawn(i, EXPLOSION2);
				if (s->picnum == CHEERBOMB)
					fi.spawn(i, BURNING);
				S_PlayActorSound(PIPEBOMB_EXPLODE, i);
				for (x = 0; x < 8; x++)
					RANDOMSCRAP(s, i);
			}
		}

		if (s->yrepeat)
		{
			s->yrepeat = 0;
			return;
		}

		if (t[4] > 20)
		{
			deletesprite(i);
			return;
		}
		if (s->picnum == CHEERBOMB)
		{
			fi.spawn(i, BURNING);
			deletesprite(i);
			return;
		}
	}
	else if (s->picnum == HEAVYHBOMB && x < 788 && t[0] > 7 && s->xvel == 0)
		if (cansee(s->x, s->y, s->z - (8 << 8), s->sectnum, ps[p].posx, ps[p].posy, ps[p].posz, ps[p].cursectnum))
			if (ps[p].ammo_amount[DYNAMITE_WEAPON] < max_ammo_amount[DYNAMITE_WEAPON])
				if (s->pal == 0)
				{
					if (ud.coop >= 1)
					{
						for (j = 0; j < ps[p].weapreccnt; j++)
							if (ps[p].weaprecs[j] == i)
								return;

						if (ps[p].weapreccnt < 255)
							ps[p].weaprecs[ps[p].weapreccnt++] = i;
					}

					addammo(DYNAMITE_WEAPON, &ps[p], 1);
					addammo(CROSSBOW_WEAPON, &ps[p], 1);
					S_PlayActorSound(DUKE_GET, ps[p].i);

					if (ps[p].gotweapon[DYNAMITE_WEAPON] == 0 || s->owner == ps[p].i)
						fi.addweapon(&ps[p], DYNAMITE_WEAPON);

					if (sprite[s->owner].picnum != APLAYER)
					{
						SetPlayerPal(&ps[p], PalEntry(32, 0, 32, 0));
					}

					if (hittype[s->owner].picnum != HEAVYHBOMB || ud.respawn_items == 0 || sprite[s->owner].picnum == APLAYER)
					{
						if (s->picnum == HEAVYHBOMB &&
							sprite[s->owner].picnum != APLAYER && ud.coop)
							return;
						deletesprite(i);
						return;
					}
					else
					{
						t[2] = respawnitemtime;
						fi.spawn(i, RESPAWNMARKERRED);
						s->cstat = (short)32768;
					}
				}

	if (t[0] < 8) t[0]++;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static int henstand(int i)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int sect = s->sectnum;
	int j;

	if (s->picnum == HENSTAND || s->picnum == HENSTAND + 1)
	{
		s->lotag--;
		if (s->lotag == 0)
		{
			fi.spawn(i, HEN);
			deletesprite(i);
			return 1;
		}
	}
	if (sector[s->sectnum].lotag == 900)
		s->xvel = 0;
	if (s->xvel)
	{
		makeitfall(i);
		j = fi.movesprite(i,
			(sintable[(s->ang + 512) & 2047] * s->xvel) >> 14,
			(sintable[s->ang & 2047] * s->xvel) >> 14,
			s->zvel, CLIPMASK0);
		if (j & 49152)
		{
			if ((j & 49152) == 32768)
			{
				j &= (MAXWALLS - 1);
				int k = getangle(
					wall[wall[j].point2].x - wall[j].x,
					wall[wall[j].point2].y - wall[j].y);
				s->ang = ((k << 1) - s->ang) & 2047;
			}
			else if ((j & 49152) == 49152)
			{
				j &= (MAXSPRITES - 1);
				fi.checkhitsprite(i, j);
				if (sprite[j].picnum == HEN)
				{
					int ns = fi.spawn(j, HENSTAND);
					deletesprite(j);
					sprite[ns].xvel = 32;
					sprite[ns].lotag = 40;
					sprite[ns].ang = s->ang;
				}
			}
		}
		s->xvel--;
		if (s->xvel < 0) s->xvel = 0;
		s->cstat = 257;
		if (s->picnum == RRTILE3440)
		{
			s->cstat |= 4 & s->xvel;
			s->cstat |= 8 & s->xvel;
			if (krand() & 1)
				s->picnum = RRTILE3440 + 1;
		}
		else if (s->picnum == HENSTAND)
		{
			s->cstat |= 4 & s->xvel;
			s->cstat |= 8 & s->xvel;
			if (krand() & 1)
				s->picnum = HENSTAND + 1;
			if (!s->xvel)
				return 2;//deletesprite(i); still needs to run a script but should not do on a deleted object
		}
		if (s->picnum == RRTILE3440 || (s->picnum == RRTILE3440 + 1 && !s->xvel))
		{
			return 2;//deletesprite(i); still needs to run a script but should not do on a deleted object
		}
	}
	else if (sector[s->sectnum].lotag == 900)
	{
		if (s->picnum == BOWLINGBALL)
			ballreturn(i);
		deletesprite(i);
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
	int j, sect, p;
	spritetype *s;
	
	dojaildoor();
	moveminecart();

	if (isRRRA())
	{
		rrra_specialstats();
	}
	rr_specialstats();

	StatIterator it(STAT_ACTOR);
	int i;
	while ((i = it.NextIndex()) >= 0)
	{
		bool deleteafterexecute = false;	// taking a cue here from RedNukem to not run scripts on deleted sprites.

		s = &sprite[i];

		sect = s->sectnum;

		if( s->xrepeat == 0 || sect < 0 || sect >= MAXSECTORS)
		{
			deletesprite(i);
			continue;
		}

		auto t = &hittype[i].temp_data[0];

		hittype[i].bposx = s->x;
		hittype[i].bposy = s->y;
		hittype[i].bposz = s->z;


		switch(s->picnum)
		{
			case RESPAWNMARKERRED:
			case RESPAWNMARKERYELLOW:
			case RESPAWNMARKERGREEN:
				if (!respawnmarker(&hittype[i], RESPAWNMARKERYELLOW, RESPAWNMARKERGREEN)) continue;
				break;
			case RAT:
				if (!rat(&hittype[i], !isRRRA())) continue;
				break;
			case RRTILE3190:
			case RRTILE3191:
			case RRTILE3192:
				if (!chickenplant) 
				{
					deletesprite(i);
					continue;
				}
				if (sector[sprite[i].sectnum].lotag == 903)
					makeitfall(i);
				j = fi.movesprite(i,
					(s->xvel*sintable[(s->ang+512)&2047])>>14,
					(s->xvel*sintable[s->ang&2047])>>14,
					s->zvel,CLIPMASK0);
				switch (sector[sprite[i].sectnum].lotag)
				{
					case 901:
						sprite[i].picnum = RRTILE3191;
						break;
					case 902:
						sprite[i].picnum = RRTILE3192;
						break;
					case 903:
						if (sprite[i].z >= sector[sprite[i].sectnum].floorz - (8<<8)) 
						{
							deletesprite(i);
							continue;
						}
						break;
					case 904:
						deletesprite(i);
						continue;
						break;
				}
				if ((j & 32768) == 32768) 
				{
					deletesprite(i);
					continue;
				}
				break;

			case RRTILE3120:
			case RRTILE3122:
			case RRTILE3123:
			case RRTILE3124:
				if (!chickenplant) 
				{
					deletesprite(i);
					continue;
				}
				makeitfall(i);
				j = fi.movesprite(i,
					(s->xvel*(sintable[(s->ang+512)&2047]))>>14,
					(s->xvel*(sintable[s->ang&2047]))>>14,
					s->zvel,CLIPMASK0);
				if ((j & 32768) == 32768) 
				{
					deletesprite(i);
					continue;
				}
				if (sector[s->sectnum].lotag == 903)
				{
					if (sprite[i].z >= sector[sprite[i].sectnum].floorz - (4<<8))
					{
						deletesprite(i);
						continue;
					}
				}
				else if (sector[s->sectnum].lotag == 904)
				{
					deletesprite(i);
					continue;
				}
				break;

			case RRTILE3132:
				if (!chickenplant) 
				{
					deletesprite(i);
					continue;
				}
				makeitfall(i);
				j = fi.movesprite(i,
					(s->xvel*sintable[(s->ang+512)&2047])>>14,
					(s->xvel*sintable[s->ang&2047])>>14,
					s->zvel,CLIPMASK0);
				if (s->z >= sector[s->sectnum].floorz - (8<<8))
				{
					if (sector[s->sectnum].lotag == 1)
					{
						j = fi.spawn(i,WATERSPLASH2);
						sprite[j].z = sector[sprite[j].sectnum].floorz;
					}
					deletesprite(i);
					continue;
				}
				break;
			case BOWLINGBALL:
				if (s->xvel)
				{
					if(!S_CheckSoundPlaying(356))
						S_PlayActorSound(356,i);
				}
				else
				{
					fi.spawn(i,BOWLINGBALLSPRITE);
					deletesprite(i);
					continue;
				}
				if (sector[s->sectnum].lotag == 900)
				{
					S_StopSound(356, -1);
				}
			case RRTILE3440:
			case RRTILE3440+1:
			case HENSTAND:
			case HENSTAND+1:
			{
				int todo = henstand(i);
				if (todo == 2) deleteafterexecute = true;
				if (todo == 1) continue;
				break;
			}

			case QUEBALL:
			case STRIPEBALL:
				if (!queball(&hittype[i], POCKET, QUEBALL, STRIPEBALL)) continue;
				break;
			case FORCESPHERE:
				forcesphere(&hittype[i], FORCESPHERE);
				continue;

			case RECON:
			case UFO1_RR:
			case UFO2:
			case UFO3:
			case UFO4:
			case UFO5:
				recon(&hittype[i], EXPLOSION2, FIRELASER, -1, -1, 457, 8, [](DDukeActor* i) ->int
					{
						auto s = &i->s;
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
				ooz(&hittype[i]);
				continue;

			case EMPTYBIKE:
				if (!isRRRA()) break;
				makeitfall(i);
				getglobalz(i);
				if (sector[sect].lotag == 1)
				{
					setsprite(i,s->x,s->y,hittype[i].floorz+(16<<8));
				}
				break;

			case EMPTYBOAT:
				if (!isRRRA()) break;
				makeitfall(i);
				getglobalz(i);
				break;

			case TRIPBOMBSPRITE:
				if (!isRRRA() || (sector[sect].lotag != 1 && sector[sect].lotag != 160))
					if (s->xvel)
					{
						j = fi.movesprite(i,
							(s->xvel*sintable[(s->ang+512)&2047])>>14,
							(s->xvel*sintable[s->ang&2047])>>14,
							s->zvel,CLIPMASK0);
						s->xvel--;
					}
				break;

			case CHEERBOMB:
				if (!isRRRA()) break;
			case MORTER:
			case HEAVYHBOMB:
				heavyhbomb(i);
				continue;
				
			case REACTORBURNT:
			case REACTOR2BURNT:
				continue;

			case REACTOR:
			case REACTOR2:
				reactor(&hittype[i], REACTOR, REACTOR2, REACTORBURNT, REACTOR2BURNT, REACTORSPARK, REACTOR2SPARK);
				continue;

			case CAMERA1:
				camera(&hittype[i]);
				continue;
		}


// #ifndef VOLOMEONE
		if( ud.multimode < 2 && badguy(s) )
		{
			if( actor_tog == 1)
			{
				s->cstat = (short)32768;
				continue;
			}
			else if(actor_tog == 2) s->cstat = 257;
		}
// #endif

		p = findplayer(s,&x);

		execute(i,p,x);
		if (deleteafterexecute) deletesprite(i);
	}

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveexplosions_r(void)  // STATNUM 5
{
	int sect, p;
	int x, * t;
	spritetype* s;

	
	StatIterator it(STAT_MISC);
	int i;
	while ((i = it.NextIndex()) >= 0)
	{
		t = &hittype[i].temp_data[0];
		s = &sprite[i];
		sect = s->sectnum;

		if (sect < 0 || s->xrepeat == 0) 
		{
			deletesprite(i);
			continue;
		}

		hittype[i].bposx = s->x;
		hittype[i].bposy = s->y;
		hittype[i].bposz = s->z;

		switch (s->picnum)
		{
		case SHOTGUNSPRITE:
			if (sector[s->sectnum].lotag == 800)
				if (s->z >= sector[s->sectnum].floorz - (8 << 8))
				{
					deletesprite(i);
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
			forcesphereexplode(&hittype[i]);
			continue;

		case MUD:

			t[0]++;
			if (t[0] == 1)
			{
				if (sector[sect].floorpicnum != 3073)
				{
					deletesprite(i);
					continue;
				}
				if (S_CheckSoundPlaying(22))
					S_PlayActorSound(22, i);
			}
			if (t[0] == 3)
			{
				t[0] = 0;
				t[1]++;
			}
			if (t[1] == 5)
				deletesprite(i);
			continue;

		case WATERSPLASH2:
			watersplash2(&hittype[i]);
			continue;

		case FRAMEEFFECT1:
			frameeffect1(&hittype[i]);
			continue;
		case INNERJAW:
		case INNERJAW + 1:

			p = findplayer(s, &x);
			if (x < 512)
			{
				SetPlayerPal(&ps[p], PalEntry(32, 32, 0, 0));
				sprite[ps[p].i].extra -= 4;
			}

		case COOLEXPLOSION1:
		case FIRELASER:
		case OWHIP:
		case UWHIP:
			if (s->extra != 999)
				s->extra = 999;
			else
			{
				deletesprite(i);
				continue;
			}
			break;
		case TONGUE:
			deletesprite(i);
			continue;
		case MONEY + 1:
			hittype[i].floorz = s->z = getflorzofslope(s->sectnum, s->x, s->y);
			if (sector[s->sectnum].lotag == 800)
			{
				deletesprite(i);
				continue;
			}
			break;
		case MONEY:
			if (!money(&hittype[i], BLOODPOOL)) continue;

			if (sector[s->sectnum].lotag == 800)
				if (s->z >= sector[s->sectnum].floorz - (8 << 8))
				{
					deletesprite(i);
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
			if (!jibs(&hittype[i], JIBS6, false, true, true, s->picnum == DUKELEG || s->picnum == DUKETORSO || s->picnum == DUKEGUN,
				isRRRA() && (s->picnum == RRTILE2465 || s->picnum == RRTILE2560))) continue;
			
			if (sector[s->sectnum].lotag == 800)
				if (s->z >= sector[s->sectnum].floorz - (8 << 8))
				{
					deletesprite(i);
					continue;
				}

			continue;

		case BLOODPOOL:
			if (!bloodpool(&hittype[i], false, TIRE)) continue;

			if (sector[s->sectnum].lotag == 800)
				if (s->z >= sector[s->sectnum].floorz - (8 << 8))
				{
					deletesprite(i);
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
			p = findplayer(s, &x);
			execute(i, p, x);
			continue;

		case SHELL:
		case SHOTGUNSHELL:
			shell(&hittype[i], false);
			continue;

		case GLASSPIECES:
		case GLASSPIECES + 1:
		case GLASSPIECES + 2:
		case POPCORN:
			glasspieces(&hittype[i]);
			continue;
		}

		if (s->picnum >= SCRAP6 && s->picnum <= SCRAP5 + 3)
		{
			scrap(&hittype[i], SCRAP1, SCRAP6);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveeffectors_r(void)   //STATNUM 3
{
	int l, x, st, j, * t;
	int sh, ns, pn;
	short k;
	spritetype* s;
	sectortype* sc;
	walltype* wal;

	clearfriction();

	
	StatIterator it(STAT_EFFECTOR);
	int i;
	while ((i = it.NextIndex()) >= 0)
	{
		s = &sprite[i];

		sc = &sector[s->sectnum];
		st = s->lotag;
		sh = s->hitag;

		t = &hittype[i].temp_data[0];

		switch (st)
		{
		case SE_0_ROTATING_SECTOR:
			handle_se00(&hittype[i], -1);
			break;
			
		case SE_1_PIVOT: //Nothing for now used as the pivot
			handle_se01(&hittype[i]);
			break;
			
		case SE_6_SUBWAY:
		{
			k = sc->extra;

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
						ns = fi.spawn(i, HULK);
						sprite[ns].z = sector[sprite[ns].sectnum].ceilingz;
						sprite[ns].pal = 33;
						if (!hulkspawn)
						{
							ns = EGS(s->sectnum, s->x, s->y, sector[s->sectnum].ceilingz + 119428, 3677, -8, 16, 16, 0, 0, 0, i, 5);
							sprite[ns].cstat = 514;
							sprite[ns].pal = 7;
							sprite[ns].xrepeat = 80;
							sprite[ns].yrepeat = 255;
							ns = fi.spawn(i, 296);
							sprite[ns].cstat = 0;
							sprite[ns].cstat |= 32768;
							sprite[ns].z = sector[s->sectnum].floorz - 6144;
							deletesprite(i);
							break;
						}
					}
				}
			}
			else
			{
				s->xvel = k;
				SectIterator it(s->sectnum);
				while ((j = it.NextIndex()) >= 0)
				{
					if (sprite[j].picnum == UFOBEAM)
						if (ufospawn)
							if (++ufocnt == 64)
							{
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
								ns = fi.spawn(i, pn);
								sprite[ns].z = sector[sprite[ns].sectnum].ceilingz;
							}
				}
			}

			StatIterator it(STAT_EFFECTOR);
			while ((j = it.NextIndex()) >= 0)
			{
				auto sj = &sprite[j];
				auto htj = &hittype[j];
				if ((sj->lotag == 14) && (sh == sj->hitag) && (htj->temp_data[0] == t[0]))
				{
					sj->xvel = s->xvel;
					//						if( t[4] == 1 )
					{
						if (htj->temp_data[5] == 0)
							htj->temp_data[5] = dist(&sprite[j], s);
						x = sgn(dist(&sprite[j], s) - htj->temp_data[5]);
						if (sj->extra)
							x = -x;
						s->xvel += x;
					}
					htj->temp_data[4] = t[4];
				}
			}
			x = 0;
		}

		case SE_14_SUBWAY_CAR:
			handle_se14(&hittype[i], false, RPG, JIBS6);
			break;

		case SE_30_TWO_WAY_TRAIN:
			handle_se30(&hittype[i], JIBS6);
			break;


		case SE_2_EARTHQUAKE:
			handle_se02(&hittype[i]);
			break;

			//Flashing sector lights after reactor EXPLOSION2
		case SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT:
			handle_se03(&hittype[i]);
			break;

		case SE_4_RANDOM_LIGHTS:
			handle_se04(&hittype[i]);
			break;

			//BOSS
		case SE_5_BOSS:
			handle_se05(&hittype[i], FIRELASER);
			break;

		case SE_8_UP_OPEN_DOOR_LIGHTS:
		case SE_9_DOWN_OPEN_DOOR_LIGHTS:
			handle_se08(&hittype[i], true);
			break;

		case SE_10_DOOR_AUTO_CLOSE:

			handle_se10(&hittype[i], nullptr);
			break;
		case SE_11_SWINGING_DOOR:
			handle_se11(&hittype[i]);
			break;
			
		case SE_12_LIGHT_SWITCH:
			handle_se12(&hittype[i]);
			break;

		case SE_47_LIGHT_SWITCH:
			if (isRRRA()) handle_se12(&hittype[i], 1);
			break;
			
		case SE_48_LIGHT_SWITCH:
			if (isRRRA()) handle_se12(&hittype[i], 2);
			break;
			

		case SE_13_EXPLOSIVE:
			handle_se13(&hittype[i]);
			break;

		case SE_15_SLIDING_DOOR:
			handle_se15(&hittype[i]);
			break;

		case SE_16_REACTOR:
			handle_se16(&hittype[i], REACTOR, REACTOR2);
			break;

		case SE_17_WARP_ELEVATOR:
			handle_se17(&hittype[i]);
			break;

		case SE_18_INCREMENTAL_SECTOR_RISE_FALL:
			handle_se18(&hittype[i], true);
			break;

		case SE_19_EXPLOSION_LOWERS_CEILING:
			handle_se19(&hittype[i], BIGFORCE);
			break;

		case SE_20_STRETCH_BRIDGE:
			handle_se20(&hittype[i]);
			break;

		case SE_21_DROP_FLOOR:
			handle_se21(&hittype[i]);
			break;

		case SE_22_TEETH_DOOR:
			handle_se22(&hittype[i]);

			break;

		case 156:
			if (!isRRRA()) break;
		case SE_24_CONVEYOR:
		case 34:
		{
			static int16_t list1[] = { BLOODPOOL, PUKE, FOOTPRINTS, FOOTPRINTS2, FOOTPRINTS3, -1 };
			static int16_t list2[] = { BOLT1, BOLT1 + 1,BOLT1 + 2, BOLT1 + 3, -1 };
			handle_se24(&hittype[i], list1, list2, BULLETHOLE, -1, CRANE, 1);
			break;
		}

		case 35:
			handle_se35(&hittype[i], SMALLSMOKE, EXPLOSION2);
			break;

		case 25: //PISTONS
			if (t[4] == 0) break;
			handle_se25(&hittype[i], 4, isRRRA() ? 371 : -1, isRRRA() ? 167 : -1);
			break;

		case 26:
			handle_se26(&hittype[i]);
			break;

		case SE_27_DEMO_CAM:
			handle_se27(&hittype[i]);
			break;

		case 29:
			s->hitag += 64;
			l = mulscale12((int)s->yvel, sintable[s->hitag & 2047]);
			sc->floorz = s->z + l;
			break;

		case 31: // True Drop Floor
			handle_se31(&hittype[i], false);
			break;

		case 32: // True Drop Ceiling
			handle_se32(&hittype[i]);
			break;

		case 33:
			if (earthquaketime > 0 && (krand() & 7) == 0)
				RANDOMSCRAP(s, i);
			break;
		case 36:

			if (t[0])
			{
				if (t[0] == 1)
					fi.shoot(i, sc->extra);
				else if (t[0] == 26 * 5)
					t[0] = 0;
				t[0]++;
			}
			break;

		case 128: //SE to control glass breakage
			handle_se128(&hittype[i]);
			break;

		case 130:
			handle_se130(&hittype[i], 80, EXPLOSION2);
			break;
		case 131:
			handle_se130(&hittype[i], 40, EXPLOSION2);
			break;
		}
	}

	//Sloped sin-wave floors!
	it.Reset(STAT_EFFECTOR);
	while ((i = it.NextIndex()) >= 0)
	{
		s = &sprite[i];
		if (s->lotag != 29) continue;
		sc = &sector[s->sectnum];
		if (sc->wallnum != 4) continue;
		wal = &wall[sc->wallptr + 2];
		alignflorslope(s->sectnum, wal->x, wal->y, sector[wal->nextsector].floorz);
	}
}


//---------------------------------------------------------------------------
//
// game specific part of makeitfall.
//
//---------------------------------------------------------------------------

int adjustfall(DDukeActor *actor, int c)
{
	if ((actor->s.picnum == BIKERB || actor->s.picnum == CHEERB) && c == gc)
		c = gc>>2;
	else if (actor->s.picnum == BIKERBV2 && c == gc)
		c = gc>>3;
	return c;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void move_r(int g_i, int g_p, int g_x)
{
	auto g_sp = &sprite[g_i];
	auto g_t = hittype[g_i].temp_data;
	int l;
	short a, goalang, angdif;
	int daxvel;

	a = g_sp->hitag;

	if (a == -1) a = 0;

	g_t[0]++;

	if (a & face_player)
	{
		if (ps[g_p].newowner >= 0)
			goalang = getangle(ps[g_p].oposx - g_sp->x, ps[g_p].oposy - g_sp->y);
		else goalang = getangle(ps[g_p].posx - g_sp->x, ps[g_p].posy - g_sp->y);
		angdif = getincangle(g_sp->ang, goalang) >> 2;
		if (angdif > -8 && angdif < 0) angdif = 0;
		g_sp->ang += angdif;
	}

	if (a & spin)
		g_sp->ang += sintable[((g_t[0] << 3) & 2047)] >> 6;

	if (a & face_player_slow)
	{
		if (ps[g_p].newowner >= 0)
			goalang = getangle(ps[g_p].oposx - g_sp->x, ps[g_p].oposy - g_sp->y);
		else goalang = getangle(ps[g_p].posx - g_sp->x, ps[g_p].posy - g_sp->y);
		angdif = ksgn(getincangle(g_sp->ang, goalang)) << 5;
		if (angdif > -32 && angdif < 0)
		{
			angdif = 0;
			g_sp->ang = goalang;
		}
		g_sp->ang += angdif;
	}

	if (isRRRA())
	{
		if (a & antifaceplayerslow)
		{
			if (ps[g_p].newowner >= 0)
				goalang = (getangle(ps[g_p].oposx - g_sp->x, ps[g_p].oposy - g_sp->y) + 1024) & 2047;
			else goalang = (getangle(ps[g_p].posx - g_sp->x, ps[g_p].posy - g_sp->y) + 1024) & 2047;
			angdif = ksgn(getincangle(g_sp->ang, goalang)) << 5;
			if (angdif > -32 && angdif < 0)
			{
				angdif = 0;
				g_sp->ang = goalang;
			}
			g_sp->ang += angdif;
		}

		if ((a & jumptoplayer) == jumptoplayer)
		{
			if (g_sp->picnum == CHEER)
			{
				if (g_t[0] < 16)
					g_sp->zvel -= (sintable[(512 + (g_t[0] << 4)) & 2047] / 40);
			}
			else
			{
				if (g_t[0] < 16)
					g_sp->zvel -= (sintable[(512 + (g_t[0] << 4)) & 2047] >> 5);
			}
		}
		if (a & justjump1)
		{
			if (g_sp->picnum == RABBIT)
			{
				if (g_t[0] < 8)
					g_sp->zvel -= (sintable[(512 + (g_t[0] << 4)) & 2047] / 30);
			}
			else if (g_sp->picnum == MAMA)
			{
				if (g_t[0] < 8)
					g_sp->zvel -= (sintable[(512 + (g_t[0] << 4)) & 2047] / 35);
			}
		}
		if (a & justjump2)
		{
			if (g_sp->picnum == RABBIT)
			{
				if (g_t[0] < 8)
					g_sp->zvel -= (sintable[(512 + (g_t[0] << 4)) & 2047] / 24);
			}
			else if (g_sp->picnum == MAMA)
			{
				if (g_t[0] < 8)
					g_sp->zvel -= (sintable[(512 + (g_t[0] << 4)) & 2047] / 28);
			}
		}
		if (a & windang)
		{
			if (g_t[0] < 8)
				g_sp->zvel -= (sintable[(512 + (g_t[0] << 4)) & 2047] / 24);
		}
	}
	else if ((a & jumptoplayer) == jumptoplayer)
	{
		if (g_t[0] < 16)
			g_sp->zvel -= (sintable[(512 + (g_t[0] << 4)) & 2047] >> 5);
	}


	if (a & face_player_smart)
	{
		int newx, newy;

		newx = ps[g_p].posx + (ps[g_p].posxv / 768);
		newy = ps[g_p].posy + (ps[g_p].posyv / 768);
		goalang = getangle(newx - g_sp->x, newy - g_sp->y);
		angdif = getincangle(g_sp->ang, goalang) >> 2;
		if (angdif > -8 && angdif < 0) angdif = 0;
		g_sp->ang += angdif;
	}

	if (g_t[1] == 0 || a == 0)
	{
		if ((badguy(g_sp) && g_sp->extra <= 0) || (hittype[g_i].bposx != g_sp->x) || (hittype[g_i].bposy != g_sp->y))
		{
			hittype[g_i].bposx = g_sp->x;
			hittype[g_i].bposy = g_sp->y;
			setsprite(g_i, g_sp->x, g_sp->y, g_sp->z);
		}
		if (badguy(g_sp) && g_sp->extra <= 0)
		{
			if (sector[g_sp->sectnum].ceilingstat & 1)
			{
				if (shadedsector[g_sp->sectnum] == 1)
				{
					g_sp->shade += (16 - g_sp->shade) >> 1;
				}
				else
				{
					g_sp->shade += (sector[g_sp->sectnum].ceilingshade - g_sp->shade) >> 1;
				}
			}
			else
			{
				g_sp->shade += (sector[g_sp->sectnum].floorshade - g_sp->shade) >> 1;
			}
		}
		return;
	}

	auto moveptr = &ScriptCode[g_t[1]];

	if (a & geth) g_sp->xvel += (*moveptr - g_sp->xvel) >> 1;
	if (a & getv) g_sp->zvel += ((*(moveptr + 1) << 4) - g_sp->zvel) >> 1;

	if (a & dodgebullet)
		dodge(&hittype[g_i]);

	if (g_sp->picnum != APLAYER)
		alterang(a, &hittype[g_i], g_p);

	if (g_sp->xvel > -6 && g_sp->xvel < 6) g_sp->xvel = 0;

	a = badguy(g_sp);

	if (g_sp->xvel || g_sp->zvel)
	{
		if (a)
		{
			if (g_sp->picnum == DRONE && g_sp->extra > 0)
			{
				if (g_sp->zvel > 0)
				{
					hittype[g_i].floorz = l = getflorzofslope(g_sp->sectnum, g_sp->x, g_sp->y);
					if (isRRRA())
					{
						if (g_sp->z > (l - (28 << 8)))
							g_sp->z = l - (28 << 8);
					}
					else
					{
						if (g_sp->z > (l - (30 << 8)))
							g_sp->z = l - (30 << 8);
					}
				}
				else
				{
					hittype[g_i].ceilingz = l = getceilzofslope(g_sp->sectnum, g_sp->x, g_sp->y);
					if ((g_sp->z - l) < (50 << 8))
					{
						g_sp->z = l + (50 << 8);
						g_sp->zvel = 0;
					}
				}
			}
			if (g_sp->zvel > 0 && hittype[g_i].floorz < g_sp->z)
				g_sp->z = hittype[g_i].floorz;
			if (g_sp->zvel < 0)
			{
				l = getceilzofslope(g_sp->sectnum, g_sp->x, g_sp->y);
				if ((g_sp->z - l) < (66 << 8))
				{
					g_sp->z = l + (66 << 8);
					g_sp->zvel >>= 1;
				}
			}
		}
		else if (g_sp->picnum == APLAYER)
			if ((g_sp->z - hittype[g_i].ceilingz) < (32 << 8))
				g_sp->z = hittype[g_i].ceilingz + (32 << 8);

		daxvel = g_sp->xvel;
		angdif = g_sp->ang;

		if (a)
		{
			if (g_x < 960 && g_sp->xrepeat > 16)
			{

				daxvel = -(1024 - g_x);
				angdif = getangle(ps[g_p].posx - g_sp->x, ps[g_p].posy - g_sp->y);

				if (g_x < 512)
				{
					ps[g_p].posxv = 0;
					ps[g_p].posyv = 0;
				}
				else
				{
					ps[g_p].posxv = mulscale(ps[g_p].posxv, dukefriction - 0x2000, 16);
					ps[g_p].posyv = mulscale(ps[g_p].posyv, dukefriction - 0x2000, 16);
				}
			}
			else if ((isRRRA() && g_sp->picnum != DRONE && g_sp->picnum != SHARK && g_sp->picnum != UFO1_RRRA) ||
					(!isRRRA() && g_sp->picnum != DRONE && g_sp->picnum != SHARK && g_sp->picnum != UFO1_RR
							&& g_sp->picnum != UFO2 && g_sp->picnum != UFO3 && g_sp->picnum != UFO4 && g_sp->picnum != UFO5))
			{
				if (hittype[g_i].bposz != g_sp->z || (ud.multimode < 2 && ud.player_skill < 2))
				{
					if ((g_t[0] & 1) || ps[g_p].actorsqu == &hittype[g_i]) return;
					else daxvel <<= 1;
				}
				else
				{
					if ((g_t[0] & 3) || ps[g_p].actorsqu == &hittype[g_i]) return;
					else daxvel <<= 2;
				}
			}
		}
		if (isRRRA())
		{
			if (sector[g_sp->sectnum].lotag != 1)
			{
				switch (g_sp->picnum)
				{
				case MINIONBOAT:
				case HULKBOAT:
				case CHEERBOAT:
					daxvel >>= 1;
					break;
				}
			}
			else if (sector[g_sp->sectnum].lotag == 1)
			{
				switch (g_sp->picnum)
				{
				case BIKERB:
				case BIKERBV2:
				case CHEERB:
					daxvel >>= 1;
					break;
				}
			}
		}

		hittype[g_i].movflag = fi.movesprite(g_i,
			(daxvel * (sintable[(angdif + 512) & 2047])) >> 14,
			(daxvel * (sintable[angdif & 2047])) >> 14, g_sp->zvel, CLIPMASK0);
	}

	if (a)
	{
		if (sector[g_sp->sectnum].ceilingstat & 1)
		{
			if (shadedsector[g_sp->sectnum] == 1)
			{
				g_sp->shade += (16 - g_sp->shade) >> 1;
			}
			else
			{
				g_sp->shade += (sector[g_sp->sectnum].ceilingshade - g_sp->shade) >> 1;
			}
		}
		else g_sp->shade += (sector[g_sp->sectnum].floorshade - g_sp->shade) >> 1;

		if (sector[g_sp->sectnum].floorpicnum == MIRROR)
			deletesprite(g_i);
	}
}

void fakebubbaspawn(int g_i, int g_p)
{
	fakebubba_spawn++;
	switch (fakebubba_spawn)
	{
	default:
		break;
	case 1:
		fi.spawn(g_i, PIG);
		break;
	case 2:
		fi.spawn(g_i, MINION);
		break;
	case 3:
		fi.spawn(g_i, CHEER);
		break;
	case 4:
		fi.spawn(g_i, VIXEN);
		operateactivators(666, ps[g_p].i);
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
	auto s = &actor->s;
	int sphit = 0;
	if (isRRRA())
	{
		if (sector[s->sectnum].lotag == 801)
		{
			if (s->picnum == ROCK)
			{
				spawn(actor, ROCK2);
				spawn(actor, ROCK2);
				addspritetodelete();
			}
			return 0;
		}
		else if (sector[s->sectnum].lotag == 802)
		{
			if (s->picnum != APLAYER && badguy(actor) && s->z == actor->floorz - FOURSLEIGHT)
			{
				fi.guts(actor, JIBS6, 5, playernum);
				S_PlayActorSound(SQUISHED, actor);
				addspritetodelete();
			}
			return 0;
		}
		else if (sector[s->sectnum].lotag == 803)
		{
			if (s->picnum == ROCK2)
				addspritetodelete();
			return 0;
		}
	}
	if (sector[s->sectnum].lotag == 800)
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
	else if (isRRRA() && (sector[s->sectnum].floorpicnum == RRTILE7820 || sector[s->sectnum].floorpicnum == RRTILE7768))
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

void fall_r(int g_i, int g_p)
{
	fall_common(&hittype[g_i], g_p, JIBS6, DRONE, BLOODPOOL, SHOTSPARK1, 69, 158, fallspecial);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void destroyit(int g_i)
{
	auto g_sp = &sprite[g_i];
	spritetype* js;
	int lotag, hitag;
	int k, jj;
	int wi, wj;
	int spr;
	int wallstart2, wallend2;
	int sectnum;
	int wallstart, wallend;

	hitag = 0;
	SectIterator it1(g_sp->sectnum);
	while ((k = it1.NextIndex()) >= 0)
	{
		auto sj = &sprite[k];
		if (sj->picnum == RRTILE63)
		{
			lotag = sj->lotag;
			spr = k;
			if (sj->hitag)
				hitag = sj->hitag;
		}
	}
	StatIterator it(STAT_DESTRUCT);
	while ((jj = it.NextIndex()) >= 0)
	{
		js = &sprite[jj];
		if (hitag)
			if (hitag == js->hitag)
			{
				SectIterator it(js->sectnum);
				while ((k = it.NextIndex()) >= 0)
				{
					if (sprite[k].picnum == DESTRUCTO)
					{
						hittype[k].picnum = SHOTSPARK1;
						hittype[k].extra = 1;
					}
				}
			}
		if (sprite[spr].sectnum != js->sectnum)
			if (lotag == js->lotag)
			{
				sectnum = sprite[spr].sectnum;
				wallstart = sector[sectnum].wallptr;
				wallend = wallstart + sector[sectnum].wallnum;
				wallstart2 = sector[js->sectnum].wallptr;
				wallend2 = wallstart2 + sector[js->sectnum].wallnum;
				for (wi = wallstart, wj = wallstart2; wi < wallend; wi++, wj++)
				{
					wall[wi].picnum = wall[wj].picnum;
					wall[wi].overpicnum = wall[wj].overpicnum;
					wall[wi].shade = wall[wj].shade;
					wall[wi].xrepeat = wall[wj].xrepeat;
					wall[wi].yrepeat = wall[wj].yrepeat;
					wall[wi].xpanning = wall[wj].xpanning;
					wall[wi].ypanning = wall[wj].ypanning;
					if (isRRRA() && wall[wi].nextwall != -1)
					{
						wall[wi].cstat = 0;
						wall[wall[wi].nextwall].cstat = 0;
					}
				}
				sector[sectnum].floorz = sector[js->sectnum].floorz;
				sector[sectnum].ceilingz = sector[js->sectnum].ceilingz;
				sector[sectnum].ceilingstat = sector[js->sectnum].ceilingstat;
				sector[sectnum].floorstat = sector[js->sectnum].floorstat;
				sector[sectnum].ceilingpicnum = sector[js->sectnum].ceilingpicnum;
				sector[sectnum].ceilingheinum = sector[js->sectnum].ceilingheinum;
				sector[sectnum].ceilingshade = sector[js->sectnum].ceilingshade;
				sector[sectnum].ceilingpal = sector[js->sectnum].ceilingpal;
				sector[sectnum].ceilingxpanning = sector[js->sectnum].ceilingxpanning;
				sector[sectnum].ceilingypanning = sector[js->sectnum].ceilingypanning;
				sector[sectnum].floorpicnum = sector[js->sectnum].floorpicnum;
				sector[sectnum].floorheinum = sector[js->sectnum].floorheinum;
				sector[sectnum].floorshade = sector[js->sectnum].floorshade;
				sector[sectnum].floorpal = sector[js->sectnum].floorpal;
				sector[sectnum].floorxpanning = sector[js->sectnum].floorxpanning;
				sector[sectnum].floorypanning = sector[js->sectnum].floorypanning;
				sector[sectnum].visibility = sector[js->sectnum].visibility;
				sectorextra[sectnum] = sectorextra[js->sectnum]; // TRANSITIONAL: at least rename this.
				sector[sectnum].lotag = sector[js->sectnum].lotag;
				sector[sectnum].hitag = sector[js->sectnum].hitag;
				sector[sectnum].extra = sector[js->sectnum].extra;
			}
	}
	it1.Reset(g_sp->sectnum);
	while ((k = it.NextIndex()) >= 0)
	{
		switch (sprite[k].picnum)
		{
		case DESTRUCTO:
		case RRTILE63:
		case TORNADO:
		case APLAYER:
		case COOT:
			break;
		default:
			deletesprite(k);
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void mamaspawn(int g_i)
{
	if (mamaspawn_count)
	{
		mamaspawn_count--;
		fi.spawn(g_i, RABBIT);
	}
}

bool spawnweapondebris_r(int picnum, int dnum)
{
	return dnum == SCRAP1;
}

void respawnhitag_r(spritetype* g_sp)
{
	switch (g_sp->picnum)
	{
	case FEM10:
	case NAKED1:
	case STATUE:
		if (g_sp->yvel) fi.operaterespawns(g_sp->yvel);
		break;
	default:
		if (g_sp->hitag >= 0) fi.operaterespawns(g_sp->hitag);
		break;
	}
}

void checktimetosleep_r(int g_i)
{
	auto g_sp = &sprite[g_i];
	if (g_sp->statnum == 6)
	{
		switch (g_sp->picnum)
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
			if (hittype[g_i].timetosleep > 1)
				hittype[g_i].timetosleep--;
			else if (hittype[g_i].timetosleep == 1)
				changespritestat(g_i, 2);
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
