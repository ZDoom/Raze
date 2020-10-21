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

This file contains parts of DukeGDX by Alexander Makarov-[M210] (m210-2007@mail.ru)

*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "global.h"
#include "names_d.h"
#include "serializer.h"
#include "dukeactor.h"

BEGIN_DUKE_NS

struct FireProj
{
	int x, y, z;
	int xv, yv, zv;
};

static TMap<int, FireProj> fire;

static FSerializer& Serialize(FSerializer& arc, const char* key, FireProj& p, FireProj* def)
{
	if (arc.BeginObject(key))
	{
		arc("x", p.x)
			("y", p.y)
			("z", p.z)
			("xv", p.xv)
			("yv", p.yv)
			("zv", p.zv)
			.EndObject();
	}
	return arc;
}

void SerializeActorGlobals(FSerializer& arc)
{
	if (arc.isWriting() && fire.CountUsed() == 0) return;
	bool res = arc.BeginArray("FireProj");
	if (arc.isReading())
	{
		fire.Clear();
		if (!res) return;
		auto length = arc.ArraySize() / 2;
		int key;
		FireProj value;

		for (int i = 0; i < length; i++)
		{
			Serialize(arc, nullptr, key, nullptr);
			Serialize(arc, nullptr, value, nullptr);
			fire.Insert(key, value);
		}
	}
	else
	{
		TMap<int, FireProj>::Iterator it(fire);
		TMap<int, FireProj>::Pair* pair;
		while (it.NextPair(pair))
		{
			int k = pair->Key;
			Serialize(arc, nullptr, k, nullptr);
			Serialize(arc, nullptr, pair->Value, nullptr);
		}
	}
	arc.EndArray();
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool ceilingspace_d(int sectnum)
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

bool floorspace_d(int sectnum)
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

void check_fta_sounds_d(int i)
{
	auto spri = &sprite[i];
	if (spri->extra > 0) switch (spri->picnum)
	{
	case LIZTROOPONTOILET:
	case LIZTROOPJUSTSIT:
	case LIZTROOPSHOOT:
	case LIZTROOPJETPACK:
	case LIZTROOPDUCKING:
	case LIZTROOPRUNNING:
	case LIZTROOP:
		S_PlayActorSound(PRED_RECOG, i);
		break;
	case LIZMAN:
	case LIZMANSPITTING:
	case LIZMANFEEDING:
	case LIZMANJUMP:
		S_PlayActorSound(CAPT_RECOG, i);
		break;
	case PIGCOP:
	case PIGCOPDIVE:
		S_PlayActorSound(PIG_RECOG, i);
		break;
	case RECON:
		S_PlayActorSound(RECO_RECOG, i);
		break;
	case DRONE:
		S_PlayActorSound(DRON_RECOG, i);
		break;
	case COMMANDER:
	case COMMANDERSTAYPUT:
		S_PlayActorSound(COMM_RECOG, i);
		break;
	case ORGANTIC:
		S_PlayActorSound(TURR_RECOG, i);
		break;
	case OCTABRAIN:
	case OCTABRAINSTAYPUT:
		S_PlayActorSound(OCTA_RECOG, i);
		break;
	case BOSS1:
		S_PlaySound(BOS1_RECOG);
		break;
	case BOSS2:
		if (spri->pal == 1)
			S_PlaySound(BOS2_RECOG);
		else S_PlaySound(WHIPYOURASS);
		break;
	case BOSS3:
		if (spri->pal == 1)
			S_PlaySound(BOS3_RECOG);
		else S_PlaySound(RIPHEADNECK);
		break;
	case BOSS4:
	case BOSS4STAYPUT:
		if (spri->pal == 1)
			S_PlaySound(BOS4_RECOG);
		S_PlaySound(BOSS4_FIRSTSEE);
		break;
	case GREENSLIME:
		S_PlayActorSound(SLIM_RECOG, i);
		break;
	}
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void addweapon_d(struct player_struct *p, int weapon)
{
	if ( p->gotweapon[weapon] == 0 )
	{
		p->gotweapon.Set(weapon);
		if(weapon == SHRINKER_WEAPON)
			p->gotweapon.Set(GROW_WEAPON);
	}

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
#ifdef EDUKE
	if(p->curr_weapon != weapon)
	{
		int snum;
		snum = sprite[p->i].yvel;

		SetGameVarID(g_iWeaponVarID,weapon, snum, p->i);
		if (p->curr_weapon >= 0)
		{
			SetGameVarID(g_iWorksLikeVarID, aplWeaponWorksLike[weapon][snum], snum, p->i);
		}
		else
		{
			SetGameVarID(g_iWorksLikeVarID, -1, snum, p->i);
		}
		SetGameVarID(g_iReturnVarID, 0, snum, -1);
		OnEvent(EVENT_CHANGEWEAPON, p->i, snum, -1);
		if (GetGameVarID(g_iReturnVarID, -1, snum) == 0)
		{
			p->curr_weapon = weapon;
		}
	}
#else
	p->curr_weapon = weapon;
#endif

	switch (weapon)
	{
	case KNEE_WEAPON:
	case TRIPBOMB_WEAPON:
	case HANDREMOTE_WEAPON:
	case HANDBOMB_WEAPON:	 
		break;
	case SHOTGUN_WEAPON:	  
		S_PlayActorSound(SHOTGUN_COCK, p->i); 
		break;
	case PISTOL_WEAPON:	   
		S_PlayActorSound(INSERT_CLIP, p->i); 
		break;
	default:	  
		S_PlayActorSound(SELECT_WEAPON, p->i); 
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool ifsquished(int i, int p)
{
	if (isRR()) return false;	// this function is a no-op in RR's source.

	auto spri = &sprite[i];
	auto ht = &hittype[i];
	bool squishme = false;
	if (spri->picnum == APLAYER && ud.clipping)
		return false;

	auto& sc = sector[spri->sectnum];
	int floorceildist = sc.floorz - sc.ceilingz;

	if (sc.lotag != ST_23_SWINGING_DOOR)
	{
		if (spri->pal == 1)
			squishme = floorceildist < (32 << 8) && (sc.lotag & 32768) == 0;
		else
			squishme = floorceildist < (12 << 8);
	}

	if (squishme)
	{
		FTA(QUOTE_SQUISHED, &ps[p]);

		if (badguy(&sprite[i]))
			spri->xvel = 0;

		if (spri->pal == 1)
		{
			ht->picnum = SHOTSPARK1;
			ht->extra = 1;
			return false;
		}

		return true;
	}
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void hitradius_d(DDukeActor* actor, int  r, int  hp1, int  hp2, int  hp3, int  hp4)
{
	walltype* wal;
	int d, q, x1, y1;
	int sectcnt, sectend, dasect, startwall, endwall, nextsect;
	short p, x, sect;
	static const uint8_t statlist[] = { STAT_DEFAULT, STAT_ACTOR, STAT_STANDABLE, STAT_PLAYER, STAT_FALLER, STAT_ZOMBIEACTOR, STAT_MISC };
	short tempshort[MAXSECTORS];	// originally hijacked a global buffer which is bad. Q: How many do we really need? RedNukem says 64.
	
	auto spri = &actor->s;
	
	if(spri->picnum == RPG && spri->xrepeat < 11) goto SKIPWALLCHECK;
	
	if(spri->picnum != SHRINKSPARK)
	{
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
	}
	
SKIPWALLCHECK:
	
	q = -(16 << 8) + (krand() & ((32 << 8) - 1));
	
	auto Owner = actor->GetOwner();
	for (x = 0; x < 7; x++)
	{
		DukeStatIterator itj(statlist[x]);
		while (auto act2 = itj.Next())
		{
			auto spri2 = &act2->s;
			if (isWorldTour() && Owner)
			{
				if (Owner->s.picnum == APLAYER && spri2->picnum == APLAYER && ud.coop != 0 && ud.ffire == 0 && Owner != act2)
				{
					continue;
				}
				
				if (spri->picnum == FLAMETHROWERFLAME && ((Owner->s.picnum == FIREFLY && spri2->picnum == FIREFLY) || (Owner->s.picnum == BOSS5 && spri2->picnum == BOSS5)))
				{
					continue;
				}
			}
			
			if (x == 0 || x >= 5 || AFLAMABLE(spri2->picnum))
			{
				if (spri->picnum != SHRINKSPARK || (spri2->cstat & 257))
					if (dist(actor, act2) < r)
					{
						if (badguy(act2) && !cansee(spri2->x, spri2->y, spri2->z + q, spri2->sectnum, spri->x, spri->y, spri->z + q, spri->sectnum))
							continue;
						fi.checkhitsprite(act2->GetIndex(), actor->GetIndex());
					}
			}
			else if (spri2->extra >= 0 && act2 != actor && (spri2->picnum == TRIPBOMB || badguy(act2) || spri2->picnum == QUEBALL || spri2->picnum == STRIPEBALL || (spri2->cstat & 257) || spri2->picnum == DUKELYINGDEAD))
			{
				if (spri->picnum == SHRINKSPARK && spri2->picnum != SHARK && (act2 == Owner || spri2->xrepeat < 24))
				{
					continue;
				}
				if (spri->picnum == MORTER && act2 == Owner)
				{
					continue;
				}
				
				if (spri2->picnum == APLAYER) spri2->z -= PHEIGHT;
				d = dist(actor, act2);
				if (spri2->picnum == APLAYER) spri2->z += PHEIGHT;
				
				if (d < r && cansee(spri2->x, spri2->y, spri2->z - (8 << 8), spri2->sectnum, spri->x, spri->y, spri->z - (12 << 8), spri->sectnum))
				{
					act2->ang = getangle(spri2->x - spri->x, spri2->y - spri->y);
					
					if (spri->picnum == RPG && spri2->extra > 0)
						act2->picnum = RPG;
					else if (!isWorldTour())
					{
						if (spri->picnum == SHRINKSPARK)
							act2->picnum = SHRINKSPARK;
						else act2->picnum = RADIUSEXPLOSION;
					}
					else
					{
						if (spri->picnum == SHRINKSPARK || spri->picnum == FLAMETHROWERFLAME)
							act2->picnum = spri->picnum;
						else if (spri->picnum != FIREBALL || !Owner || Owner->s.picnum != APLAYER)
						{
							if (spri->picnum == LAVAPOOL)
								act2->picnum = FLAMETHROWERFLAME;
							else
								act2->picnum = RADIUSEXPLOSION;
						}
						else
							act2->picnum = FLAMETHROWERFLAME;
					}
					
					if (spri->picnum != SHRINKSPARK && (!isWorldTour() || spri->picnum != LAVAPOOL))
					{
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
						
						if (spri2->picnum != TANK && spri2->picnum != ROTATEGUN && spri2->picnum != RECON && !bossguy(act2))
						{
							if (spri2->xvel < 0) spri2->xvel = 0;
							spri2->xvel += (spri->extra << 2);
						}
						
						if (spri2->picnum == PODFEM1 || spri2->picnum == FEM1 ||
							spri2->picnum == FEM2 || spri2->picnum == FEM3 ||
							spri2->picnum == FEM4 || spri2->picnum == FEM5 ||
							spri2->picnum == FEM6 || spri2->picnum == FEM7 ||
							spri2->picnum == FEM8 || spri2->picnum == FEM9 ||
							spri2->picnum == FEM10 || spri2->picnum == STATUE ||
							spri2->picnum == STATUEFLASH || spri2->picnum == SPACEMARINE || spri2->picnum == QUEBALL || spri2->picnum == STRIPEBALL)
							fi.checkhitsprite(act2->GetIndex(), actor->GetIndex());
					}
					else if (spri->extra == 0) act2->extra = 0;
					
					if (spri2->picnum != RADIUSEXPLOSION && Owner && Owner->s.statnum < MAXSTATUS)
					{
						if (spri2->picnum == APLAYER)
						{
							p = spri2->yvel;
							
							if (isWorldTour() && act2->picnum == FLAMETHROWERFLAME && Owner->s.picnum == APLAYER)
							{
								ps[p].numloogs = -1 - spri->yvel;
							}
							
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


int movesprite_ex_d(DDukeActor* actor, int xchange, int ychange, int zchange, unsigned int cliptype, Collision &result)
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
			if (spri->picnum == LIZMAN)
				cd = 292;
			else if (actorflag(actor->GetIndex(), SFLAG_BADGUY))
				cd = spri->clipdist << 2;
			else
				cd = 192;

			retval = clipmove(&spri->x, &spri->y, &daz, &dasectnum, ((xchange * TICSPERFRAME) << 11), ((ychange * TICSPERFRAME) << 11), cd, (4 << 8), (4 << 8), cliptype);
		}

		// conditional code from hell...
		if (dasectnum < 0 || (dasectnum >= 0 &&
			((actor->actorstayput >= 0 && actor->actorstayput != dasectnum) ||
			 ((spri->picnum == BOSS2) && spri->pal == 0 && sector[dasectnum].lotag != 3) ||
			 ((spri->picnum == BOSS1 || spri->picnum == BOSS2) && sector[dasectnum].lotag == ST_1_ABOVE_WATER) ||
			 (sector[dasectnum].lotag == ST_1_ABOVE_WATER && (spri->picnum == LIZMAN || (spri->picnum == LIZTROOP && spri->zvel == 0)))
			))
		 )
		{
			spri->x = oldx;
			spri->y = oldy;
			if (sector[dasectnum].lotag == ST_1_ABOVE_WATER && spri->picnum == LIZMAN)
				spri->ang = (krand()&2047);
			else if ((actor->temp_data[0]&3) == 1 && spri->picnum != COMMANDER)
				spri->ang = (krand()&2047);
			setsprite(actor,oldx,oldy,spri->z);
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
			clipmove(&spri->x, &spri->y, &daz, &dasectnum, ((xchange * TICSPERFRAME) << 11), ((ychange * TICSPERFRAME) << 11), (int)(spri->clipdist << 2), (4 << 8), (4 << 8), cliptype);
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

void lotsofmoney_d(DDukeActor *actor, short n)
{
	lotsofstuff(actor, n, MONEY);
}

void lotsofmail_d(DDukeActor *actor, short n)
{
	lotsofstuff(actor, n, MAIL);
}

void lotsofpaper_d(DDukeActor *actor, short n)
{
	lotsofstuff(actor, n, PAPER);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void guts_d(spritetype* s, short gtype, short n, short p)
{
	int gutz, floorz;
	int i=0, j;
	int sx, sy;
	uint8_t pal;

	if (badguy(s) && s->xrepeat < 16)
		sx = sy = 8;
	else sx = sy = 32;

	gutz = s->z - (8 << 8);
	floorz = getflorzofslope(s->sectnum, s->x, s->y);

	if (gutz > (floorz - (8 << 8)))
		gutz = floorz - (8 << 8);

	if (s->picnum == COMMANDER)
		gutz -= (24 << 8);

	if (badguy(s) && s->pal == 6)
		pal = 6;
	else if (s->picnum != LIZTROOP) // EDuke32 transfers the palette unconditionally, I'm not sure that's such a good idea.
		pal = 0;
	else
		pal = s->pal;

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
		i = EGS(s->sectnum, s->x + (r5 & 255) - 128, s->y + (r4 & 255) - 128, gutz - (r3 & 8191), gtype, -32, sx, sy, a, 48 + (r2 & 31), -512 - (r1 & 2047), ps[p].i, 5);
		auto si = &sprite[i];
		if (si->picnum == JIBS2)
		{
			si->xrepeat >>= 2;
			si->yrepeat >>= 2;
		}
		if (pal != 0)
			si->pal = pal;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void gutsdir_d(spritetype* s, short gtype, short n, short p)
{
	int sx, sy;

	if (badguy(s) && s->xrepeat < 16)
		sx = sy = 8;
	else sx = sy = 32;

	int gutz = s->z - (8 << 8);
	int floorz = getflorzofslope(s->sectnum, s->x, s->y);

	if (gutz > (floorz - (8 << 8)))
		gutz = floorz - (8 << 8);

	if (s->picnum == COMMANDER)
		gutz -= (24 << 8);

	for (int j = 0; j < n; j++)
	{
		int a = krand() & 2047;
		int r1 = krand();
		int r2 = krand();
		// TRANSITIONAL: owned by a player???
		EGS(s->sectnum, s->x, s->y, gutz, gtype, -32, sx, sy, a, 256 + (r2 & 127), -512 - (r1 & 2047), ps[p].i, 5);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void movefta_d(void)
{
	int x, px, py, sx, sy;
	short p, psect, ssect;
	int i, j;

	StatIterator iti(STAT_ZOMBIEACTOR);

	while ((i = iti.NextIndex()) >= 0)
	{
		auto s = &sprite[i];
		auto ht = &hittype[i];
		p = findplayer(s, &x);

		ssect = psect = s->sectnum;

		if (sprite[ps[p].i].extra > 0)
		{
			if (x < 30000)
			{
				ht->timetosleep++;
				if (ht->timetosleep >= (x >> 8))
				{
					if (badguy(s))
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

						int r1 = krand();
						int r2 = krand();
						j = cansee(sx, sy, s->z - (r2 % (52 << 8)), s->sectnum, px, py, ps[p].oposz - (r1 % (32 << 8)), ps[p].cursectnum);
					}
					else
					{
						int r1 = krand();
						int r2 = krand();
						j = cansee(s->x, s->y, s->z - ((r2 & 31) << 8), s->sectnum, ps[p].oposx, ps[p].oposy, ps[p].oposz - ((r1 & 31) << 8), ps[p].cursectnum);
					}


					if (j) switch(s->picnum)
					{
						case RUBBERCAN:
						case EXPLODINGBARREL:
						case WOODENHORSE:
						case HORSEONSIDE:
						case CANWITHSOMETHING:
						case CANWITHSOMETHING2:
						case CANWITHSOMETHING3:
						case CANWITHSOMETHING4:
						case FIREBARREL:
						case FIREVASE:
						case NUKEBARREL:
						case NUKEBARRELDENTED:
						case NUKEBARRELLEAKED:
						case TRIPBOMB:
							if (sector[s->sectnum].ceilingstat&1)
								s->shade = sector[s->sectnum].ceilingshade;
							else s->shade = sector[s->sectnum].floorshade;

							ht->timetosleep = 0;
							changespritestat(i, STAT_STANDABLE);
							break;

						default:
							ht->timetosleep = 0;
							fi.check_fta_sounds(i);
							changespritestat(i, STAT_ACTOR);
							break;
					}
					else ht->timetosleep = 0;
				}
			}
			if (badguy(s))
			{
				if (sector[s->sectnum].ceilingstat & 1)
					s->shade = sector[s->sectnum].ceilingshade;
				else s->shade = sector[s->sectnum].floorshade;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int ifhitsectors_d(int sectnum)
{
	StatIterator it(STAT_MISC);
	int i;
	while((i = it.NextIndex()) >= 0)
	{
		auto s = &sprite[i];
		if (s->picnum == EXPLOSION2 && sectnum == s->sectnum)
			return i;
	}
	return -1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int ifhitbyweapon_d(DDukeActor *actor)
{
	short p;
	auto spri = &actor->s;
	auto hitowner = actor->GetHitOwner();

	if (actor->extra >= 0)
	{
		if (spri->extra >= 0)
		{
			if (spri->picnum == APLAYER)
			{
				if (ud.god && actor->picnum != SHRINKSPARK) return -1;

				p = spri->yvel;
				
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

				switch(actor->picnum)
				{
					case RADIUSEXPLOSION:
					case RPG:
					case HYDRENT:
					case HEAVYHBOMB:
					case SEENINE:
					case OOZFILTER:
					case EXPLODINGBARREL:
						ps[p].posxv +=
							actor->extra*(sintable[(actor->ang+512)&2047]) << 2;
						ps[p].posyv +=
							actor->extra*(sintable[actor->ang&2047]) << 2;
						break;
					default:
						ps[p].posxv +=
							actor->extra*(sintable[(actor->ang+512)&2047]) << 1;
						ps[p].posyv +=
							actor->extra*(sintable[actor->ang&2047]) << 1;
						break;
				}
			}
			else
			{
				if (actor->extra == 0)
					if (actor->picnum == SHRINKSPARK && spri->xrepeat < 24)
						return -1;

				if (isWorldTour() && actor->picnum == FIREFLY && spri->xrepeat < 48)
				{
					if (actor->picnum != RADIUSEXPLOSION && actor->picnum != RPG)
						return -1;
				}

				spri->extra -= actor->extra;
				auto Owner = actor->GetOwner();
				if (spri->picnum != RECON && Owner && Owner->s.statnum < MAXSTATUS)
					actor->SetOwner(hitowner);
			}

			actor->extra = -1;
			return actor->picnum;
		}
	}


	if (ud.multimode < 2 || !isWorldTour()
		|| actor->picnum != FLAMETHROWERFLAME
		|| actor->extra >= 0
		|| spri->extra > 0
		|| spri->picnum != APLAYER
		|| ps[actor->PlayerIndex()].numloogs > 0
		|| hitowner == nullptr)
	{
		actor->extra = -1;
		return -1;
	}
	else
	{
		p = actor->PlayerIndex();
		spri->extra = 0;
		ps[p].wackedbyactor = hitowner;

		if (hitowner->s.picnum == APLAYER && hitowner != ps[p].GetActor())
			ps[p].frag_ps = hitowner->PlayerIndex(); // set the proper player index here - this previously set the sprite index...

		actor->SetHitOwner(ps[p].GetActor());
		actor->extra = -1;

		return FLAMETHROWERFLAME;
	}
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movefallers_d(void)
{
	short sect;
	int i, j, x;

	StatIterator iti(STAT_FALLER);
	while ((i = iti.NextIndex()) >= 0)
	{
		auto s = &sprite[i];
		auto ht = &hittype[i];

		sect = s->sectnum;

		if (ht->temp_data[0] == 0)
		{
			s->z -= (16 << 8);
			ht->temp_data[1] = s->ang;
			x = s->extra;
			j = fi.ifhitbyweapon(&hittype[i]);
			if (j >= 0)
			{
				if (j == FIREEXT || j == RPG || j == RADIUSEXPLOSION || j == SEENINE || j == OOZFILTER)
				{
					if (s->extra <= 0)
					{
						ht->temp_data[0] = 1;
						StatIterator itj(STAT_FALLER);
						while ((j = itj.NextIndex()) >= 0)
						{
							auto sj = &sprite[j];
							if (sj->hitag == s->hitag)
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
					ht->extra = 0;
					s->extra = x;
				}
			}
			s->ang = ht->temp_data[1];
			s->z += (16 << 8);
		}
		else if (ht->temp_data[0] == 1)
		{
			if (s->lotag > 0)
			{
				s->lotag-=3;
				if (s->lotag <= 0)
				{
					s->xvel = (32 + (krand() & 63));
					s->zvel = -(1024 + (krand() & 1023));
				}
			}
			else
			{
				if (s->xvel > 0)
				{
					s->xvel -= 8;
					ssp(i,CLIPMASK0);
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
// Duke only
//
//---------------------------------------------------------------------------

static void movetripbomb(int i)
{
	auto s = &sprite[i];
	int j, x;
	int lTripBombControl = GetGameVar("TRIPBOMB_CONTROL", TRIPBOMB_TRIPWIRE, -1, -1);
	if (lTripBombControl & TRIPBOMB_TIMER)
	{
		// we're on a timer....
		if (s->extra >= 0)
		{
			s->extra--;
			if (s->extra == 0)
			{
				hittype[i].temp_data[2] = 16;
				S_PlayActorSound(LASERTRIP_ARMING, i);
			}
		}
	}
	if (hittype[i].temp_data[2] > 0)
	{
		hittype[i].temp_data[2]--;
		if (hittype[i].temp_data[2] == 8)
		{
			S_PlayActorSound(LASERTRIP_EXPLODE, i);
			for (j = 0; j < 5; j++) RANDOMSCRAP(s, i);
			x = s->extra;
			fi.hitradius(&hittype[i], tripbombblastradius, x >> 2, x >> 1, x - (x >> 2), x);

			j = fi.spawn(i, EXPLOSION2);
			sprite[j].ang = s->ang;
			sprite[j].xvel = 348;
			ssp(j, CLIPMASK0);

			StatIterator it(STAT_MISC);
			while ((j = it.NextIndex()) >= 0)
			{
				if (sprite[j].picnum == LASERLINE && s->hitag == sprite[j].hitag)
					sprite[j].xrepeat = sprite[j].yrepeat = 0;
			}
			deletesprite(i);
		}
		return;
	}
	else
	{
		x = s->extra;
		s->extra = 1;
		int16_t l = s->ang;
		j = fi.ifhitbyweapon(&hittype[i]);
		if (j >= 0)
		{ 
			hittype[i].temp_data[2] = 16; 
		}
		s->extra = x;
		s->ang = l;
	}

	if (hittype[i].temp_data[0] < 32)
	{
		findplayer(s, &x);
		if (x > 768) hittype[i].temp_data[0]++;
		else if (hittype[i].temp_data[0] > 16) hittype[i].temp_data[0]++;
	}
	if (hittype[i].temp_data[0] == 32)
	{
		int16_t l = s->ang;
		s->ang = hittype[i].temp_data[5];

		hittype[i].temp_data[3] = s->x; hittype[i].temp_data[4] = s->y;
		s->x += sintable[(hittype[i].temp_data[5] + 512) & 2047] >> 9;
		s->y += sintable[(hittype[i].temp_data[5]) & 2047] >> 9;
		s->z -= (3 << 8);

		// Laser fix from EDuke32.
		int16_t const oldSectNum = s->sectnum;
		int16_t       curSectNum = s->sectnum;

		updatesectorneighbor(s->x, s->y, &curSectNum, 1024, 2048);
		changespritesect(i, curSectNum);

		int16_t m;
		x = hitasprite(i, &m);

		hittype[i].lastvx = x;

		s->ang = l;

		int k = 0;

		if (lTripBombControl & TRIPBOMB_TRIPWIRE)
		{
			// we're on a trip wire
			while (x > 0)
			{
				j = fi.spawn(i, LASERLINE);
				setsprite(j, sprite[j].x, sprite[j].y, sprite[j].z);
				sprite[j].hitag = s->hitag;
				hittype[j].temp_data[1] = sprite[j].z;

				if (x < 1024)
				{
					sprite[j].xrepeat = x >> 5;
					break;
				}
				x -= 1024;

				s->x += sintable[(hittype[i].temp_data[5] + 512) & 2047] >> 4;
				s->y += sintable[(hittype[i].temp_data[5]) & 2047] >> 4;
				updatesectorneighbor(s->x, s->y, &curSectNum, 1024, 2048);

				if (curSectNum == -1)
					break;

				changespritesect(i, curSectNum);

				// this is a hack to work around the LASERLINE sprite's art tile offset
				changespritesect(j, curSectNum);

			}
		}

		hittype[i].temp_data[0]++;
		s->x = hittype[i].temp_data[3]; s->y = hittype[i].temp_data[4];
		s->z += (3 << 8);
		changespritesect(i, oldSectNum);
		hittype[i].temp_data[3] = 0;
		if (m >= 0 && lTripBombControl & TRIPBOMB_TRIPWIRE)
		{
			hittype[i].temp_data[2] = 13;
			S_PlayActorSound(LASERTRIP_ARMING, i);
		}
		else hittype[i].temp_data[2] = 0;
	}
	if (hittype[i].temp_data[0] == 33)
	{
		hittype[i].temp_data[1]++;


		hittype[i].temp_data[3] = s->x; hittype[i].temp_data[4] = s->y;
		s->x += sintable[(hittype[i].temp_data[5] + 512) & 2047] >> 9;
		s->y += sintable[(hittype[i].temp_data[5]) & 2047] >> 9;
		s->z -= (3 << 8);
		setsprite(i, s->x, s->y, s->z);

		int16_t m;
		x = hitasprite(i, &m);

		s->x = hittype[i].temp_data[3]; s->y = hittype[i].temp_data[4];
		s->z += (3 << 8);
		setsprite(i, s->x, s->y, s->z);

		if (hittype[i].lastvx != x && lTripBombControl & TRIPBOMB_TRIPWIRE)
		{
			hittype[i].temp_data[2] = 13;
			S_PlayActorSound(LASERTRIP_ARMING, i);
		}
	}
}

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
		if (j == FIREEXT || j == RPG || j == RADIUSEXPLOSION || j == SEENINE || j == OOZFILTER)
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
// Duke only
//
//---------------------------------------------------------------------------

static void movefireext(int i)
{
	int j = fi.ifhitbyweapon(&hittype[i]);
	if (j == -1) return;

	auto s = &sprite[i];
	auto t = &hittype[i].temp_data[0];

	for (int k = 0; k < 16; k++)
	{
		j = EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, sprite[i].z - (krand() % (48 << 8)), SCRAP3 + (krand() & 3), -8, 48, 48, krand() & 2047, (krand() & 63) + 64, -(krand() & 4095) - (sprite[i].zvel >> 2), i, 5);
		sprite[j].pal = 2;
	}

	fi.spawn(i, EXPLOSION2);
	S_PlayActorSound(PIPEBOMB_EXPLODE, i);
	S_PlayActorSound(GLASS_HEAVYBREAK, i);

	if (s->hitag > 0)
	{
		StatIterator it(STAT_STANDABLE);
		int j;
		while ((j = it.NextIndex()) >= 0)
		{
			auto sj = &sprite[j];
			if (s->hitag == sj->hitag && (sj->picnum == OOZFILTER || sj->picnum == SEENINE))
				if (sj->shade != -32)
					sj->shade = -32;
		}

		int x = s->extra;
		fi.spawn(i, EXPLOSION2);
		fi.hitradius(&hittype[i], pipebombblastradius, x >> 2, x - (x >> 1), x - (x >> 2), x);
		S_PlayActorSound(PIPEBOMB_EXPLODE, i);
		detonate(i, EXPLOSION2);
	}
	else
	{
		fi.hitradius(&hittype[i], seenineblastradius, 10, 15, 20, 25);
		deletesprite(i);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void moveviewscreen(int i)
{
	auto s = &sprite[i];
	if (s->xrepeat == 0) deletesprite(i);
	else
	{
		int x;
		findplayer(s, &x);

		if (x < 2048)
		{
#if 0
			if (sprite[i].yvel  == 1)
				camsprite = i;
#endif
		}
		else if (camsprite != -1 && hittype[i].temp_data[0] == 1)
		{
			camsprite = -1;
			s->yvel = 0;
			hittype[i].temp_data[0] = 0;
		}
	}
}

//---------------------------------------------------------------------------
//
// Duke only
//
//---------------------------------------------------------------------------

static void movesidebolt(int i)
{
	auto s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int x;
	int sect = s->sectnum;

	auto p = findplayer(s, &x);
	if (x > 20480) return;

CLEAR_THE_BOLT2:
	if (t[2])
	{
		t[2]--;
		return;
	}
	if ((s->xrepeat | s->yrepeat) == 0)
	{
		s->xrepeat = t[0];
		s->yrepeat = t[1];
	}
	if ((krand() & 8) == 0)
	{
		t[0] = s->xrepeat;
		t[1] = s->yrepeat;
		t[2] = global_random & 4;
		s->xrepeat = s->yrepeat = 0;
		goto CLEAR_THE_BOLT2;
	}
	s->picnum++;

#if 0
	// content of l was undefined.
	if (l & 1) s->cstat ^= 2;
#endif

	if ((krand() & 1) && sector[sect].floorpicnum == HURTRAIL)
		S_PlayActorSound(SHORT_CIRCUIT, i);

	if (s->picnum == SIDEBOLT1 + 4) s->picnum = SIDEBOLT1;
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

	if (s->picnum == (BOLT1+1) && (krand()&7) == 0 && sector[sect].floorpicnum == HURTRAIL)
		S_PlayActorSound(SHORT_CIRCUIT,i);

	if (s->picnum==BOLT1+4) s->picnum=BOLT1;

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

void movestandables_d(void)
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

		else if (picnum == TRIPBOMB)
		{
			movetripbomb(i);
		}

		else if (picnum >= CRACK1 && picnum <= CRACK1 + 3)
		{
			movecrack(i);
		}

		else if (picnum == FIREEXT)
		{
			movefireext(i);
		}

		else if (picnum == OOZFILTER || picnum == SEENINE || picnum == SEENINEDEAD || picnum == (SEENINEDEAD + 1))
		{
			moveooz(&hittype[i], SEENINE, SEENINEDEAD, OOZFILTER, EXPLOSION2);
		}

		else if (picnum == MASTERSWITCH)
		{
			movemasterswitch(&hittype[i], SEENINE, OOZFILTER);
		}

		else if (picnum == VIEWSCREEN || picnum == VIEWSCREEN2)
		{
			moveviewscreen(i);
		}

		else if (picnum == TRASH)
		{
			movetrash(&hittype[i]);
		}

		else if (picnum >= SIDEBOLT1 && picnum <= SIDEBOLT1 + 3)
		{
			movesidebolt(i);
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

		else if (isIn(picnum, CANWITHSOMETHING, CANWITHSOMETHING2, CANWITHSOMETHING3, CANWITHSOMETHING4))
		{
			movecanwithsomething(&hittype[i]);
		}

		else if (isIn(picnum,
				EXPLODINGBARREL,
				WOODENHORSE,
				HORSEONSIDE,
				FLOORFLAME,
				FIREBARREL,
				FIREVASE,
				NUKEBARREL,
				NUKEBARRELDENTED,
				NUKEBARRELLEAKED,
				TOILETWATER,
				RUBBERCAN,
				STEAM,
				CEILINGSTEAM,
				WATERBUBBLEMAKER))
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

static bool movefireball(int i)
{
	auto s = &sprite[i];
	auto ht = &hittype[i];

	if (sector[s->sectnum].lotag == 2)
	{
		deletesprite(i);
		return true;
	}

	if (sprite[s->owner].picnum != FIREBALL)
	{
		if (ht->temp_data[0] >= 1 && ht->temp_data[0] < 6)
		{
			float siz = 1.0f - (ht->temp_data[0] * 0.2f);
			int trail = ht->temp_data[1];
			int j = ht->temp_data[1] = fi.spawn(i, FIREBALL);

			auto spr = &sprite[j];
			spr->xvel = s->xvel;
			spr->yvel = s->yvel;
			spr->zvel = s->zvel;
			if (ht->temp_data[0] > 1)
			{
				FireProj* proj = fire.CheckKey(trail);
				if (proj != nullptr)
				{
					spr->x = proj->x;
					spr->y = proj->y;
					spr->z = proj->z;
					spr->xvel = proj->xv;
					spr->yvel = proj->yv;
					spr->zvel = proj->zv;
				}
			}
			spr->yrepeat = spr->xrepeat = (short)(sprite[i].xrepeat * siz);
			spr->cstat = sprite[i].cstat;
			spr->extra = 0;

			FireProj proj = { spr->x, spr->y, spr->z, spr->xvel, spr->yvel, spr->zvel };
			fire.Insert(j, proj);
			changespritestat((short)j, (short)4);
		}
		ht->temp_data[0]++;
	}
	if (s->zvel < 15000)
		s->zvel += 200;
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static bool weaponhitsprite(int i, int j, bool fireball)
{
	auto s = &sprite[i];

	if (s->picnum == FREEZEBLAST && sprite[j].pal == 1)
		if (badguy(&sprite[j]) || sprite[j].picnum == APLAYER)
		{
			j = fi.spawn(i, TRANSPORTERSTAR);
			sprite[j].pal = 1;
			sprite[j].xrepeat = 32;
			sprite[j].yrepeat = 32;

			deletesprite(i);
			return true;
		}

	if (!isWorldTour() || s->picnum != FIREBALL || fireball)
		fi.checkhitsprite(j, i);

	if (sprite[j].picnum == APLAYER)
	{
		int p = sprite[j].yvel;

		if (ud.multimode >= 2 && fireball && sprite[s->owner].picnum == APLAYER)
		{
			ps[p].numloogs = -1 - sprite[i].yvel;
		}

		S_PlayActorSound(PISTOL_BODYHIT, j);

		if (s->picnum == SPIT)
		{
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

static bool weaponhitwall(int i, int j, const vec3_t &oldpos)
{
	auto s = &sprite[i];

	if (s->picnum != RPG && s->picnum != FREEZEBLAST && s->picnum != SPIT &&
		(!isWorldTour() || s->picnum != FIREBALL) &&
		(wall[j].overpicnum == MIRROR || wall[j].picnum == MIRROR))
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

		if (s->picnum == FREEZEBLAST)
		{
			if (wall[j].overpicnum != MIRROR && wall[j].picnum != MIRROR)
			{
				s->extra >>= 1;
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

static bool weaponhitsector(int i, const vec3_t& oldpos, bool fireball)
{
	auto s = &sprite[i];

	setsprite(i, &oldpos);

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
	else if (fireball)
	{
		int j = fi.spawn(i, LAVAPOOL);
		sprite[j].owner = sprite[i].owner;
		sprite[j].yvel = sprite[i].yvel;
		hittype[j].owner = sprite[i].owner;
		deletesprite(i);
		return true;
	}

	if (s->picnum == FREEZEBLAST)
	{
		bounce(i);
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

static void weaponcommon_d(int i)
{
	auto s = &sprite[i];

	if (s->picnum == COOLEXPLOSION1)
		if (!S_CheckSoundPlaying(i, WIERDSHOT_FLY))
			S_PlayActorSound(WIERDSHOT_FLY, i);

	int p = -1;
	int k, ll;
	auto oldpos = s->pos;

	if (s->picnum == RPG && sector[s->sectnum].lotag == 2)
	{
		k = s->xvel >> 1;
		ll = s->zvel >> 1;
	}
	else
	{
		k = s->xvel;
		ll = s->zvel;
	}

	getglobalz(i);

	switch (s->picnum)
	{
	case RPG:
		if (hittype[i].picnum != BOSS2 && s->xrepeat >= 10 && sector[s->sectnum].lotag != 2)
		{
			int j = fi.spawn(i, SMALLSMOKE);
			sprite[j].z += (1 << 8);
		}
		break;

	case FIREBALL:
		if (movefireball(i)) return;
		break;
	}

	int j = fi.movesprite(i,
		(k * (sintable[(s->ang + 512) & 2047])) >> 14,
		(k * (sintable[s->ang & 2047])) >> 14, ll, CLIPMASK1);

	if (s->picnum == RPG && s->yvel >= 0)
		if (FindDistance2D(s->x - sprite[s->yvel].x, s->y - sprite[s->yvel].y) < 256)
			j = kHitSprite | s->yvel;

	if (s->sectnum < 0)
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
			if ((s->z > hittype[i].floorz && sector[s->sectnum].lotag != 1) ||
				(s->z > hittype[i].floorz + (16 << 8) && sector[s->sectnum].lotag == 1))
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
			int x = EGS(s->sectnum,
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
		if (s->picnum == COOLEXPLOSION1)
		{
			if ((j & kHitTypeMask) == kHitSprite && sprite[j & kHitIndexMask].picnum != APLAYER)
			{
				return;
			}
			s->xvel = 0;
			s->zvel = 0;
		}

		bool fireball = (isWorldTour() && s->picnum == FIREBALL && sprite[s->owner].picnum != FIREBALL);

		if ((j & kHitTypeMask) == kHitSprite)
		{
			j &= kHitIndexMask;
			if (weaponhitsprite(i, j, fireball)) return;
		}
		else if ((j & kHitTypeMask) == kHitWall)
		{
			j &= kHitIndexMask;
			if (weaponhitwall(i, j, oldpos)) return;
		}
		else if ((j & kHitTypeMask) == kHitSector)
		{
			if (weaponhitsector(i, oldpos, fireball)) return;
		}

		if (s->picnum != SPIT)
		{
			if (s->picnum == RPG)
			{
				// j is only needed for the hit type mask.
				rpgexplode(i, j, oldpos, EXPLOSION2, EXPLOSION2BOT, -1, RPG_EXPLODE);
			}
			else if (s->picnum == SHRINKSPARK)
			{
				fi.spawn(i, SHRINKEREXPLOSION);
				S_PlayActorSound(SHRINKER_HIT, i);
				fi.hitradius(&hittype[i], shrinkerblastradius, 0, 0, 0, 0);
			}
			else if (s->picnum != COOLEXPLOSION1 && s->picnum != FREEZEBLAST && s->picnum != FIRELASER && (!isWorldTour() || s->picnum != FIREBALL))
			{
				k = fi.spawn(i, EXPLOSION2);
				sprite[k].xrepeat = sprite[k].yrepeat = s->xrepeat >> 1;
				if ((j & kHitTypeMask) == kHitSector)
				{
					if (s->zvel < 0)
					{
						sprite[k].cstat |= 8; sprite[k].z += (72 << 8);
					}
				}
			}
			if (fireball)
			{
				j = fi.spawn(i, EXPLOSION2);
				sprite[j].xrepeat = sprite[j].yrepeat = (short)(s->xrepeat >> 1);
			}
		}
		if (s->picnum != COOLEXPLOSION1)
		{
			deletesprite(i);
			return;
		}
	}
	if (s->picnum == COOLEXPLOSION1)
	{
		s->shade++;
		if (s->shade >= 40)
		{
			deletesprite(i);
			return;
		}
	}
	else if (s->picnum == RPG && sector[s->sectnum].lotag == 2 && s->xrepeat >= 10 && rnd(140))
		fi.spawn(i, WATERBUBBLE);

}
//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveweapons_d(void)
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

		switch(s->picnum)
		{
		case RADIUSEXPLOSION:
		case KNEE:
			deletesprite(i);
			continue;
		case TONGUE:
			movetongue(i, TONGUE, INNERJAW);
			continue;

		case FREEZEBLAST:
			if (s->yvel < 1 || s->extra < 2 || (s->xvel|s->zvel) == 0)
			{
				int j = fi.spawn(i,TRANSPORTERSTAR);
				sprite[j].pal = 1;
				sprite[j].xrepeat = 32;
				sprite[j].yrepeat = 32;
				deletesprite(i);
				continue;
			}
		case FIREBALL:
			// Twentieth Anniversary World Tour
			if (s->picnum == FIREBALL && !isWorldTour()) break;
		case SHRINKSPARK:
		case RPG:
		case FIRELASER:
		case SPIT:
		case COOLEXPLOSION1:
			weaponcommon_d(i);
			break;

		case SHOTSPARK1:
		{
			int x;
			int p = findplayer(s, &x);
			execute(i, p, x);
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

void movetransports_d(void)
{
	int warpspriteto;
	int ll, onfloorz, q;
	int i, j;
	
	StatIterator iti(STAT_TRANSPORT);
	while ((i = iti.NextIndex()) >= 0)
	{
		auto spri = &sprite[i];
		auto hiti = &hittype[i];
		auto spriowner = spri->owner < 0? nullptr : &sprite[spri->owner];
		
		int sect = spri->sectnum;
		int sectlotag = sector[sect].lotag;
		
		
		if (spri->owner == i)
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
			case STAT_PLAYER:    // Player
				
				if (sprj->owner != -1)
				{
					int p = sprj->yvel;
					
					ps[p].on_warping_sector = 1;
					
					if (ps[p].transporter_hold == 0 && ps[p].jumping_counter == 0)
					{
						if (ps[p].on_ground && sectlotag == 0 && onfloorz && ps[p].jetpack_on == 0)
						{
							if (spri->pal == 0)
							{
								fi.spawn(i, TRANSPORTERBEAM);
								S_PlayActorSound(TELEPORTER, i);
							}
							
							for (int k = connecthead; k >= 0; k = connectpoint2[k])
							if (ps[k].cursectnum == spriowner->sectnum)
							{
								ps[k].frag_ps = p;
								sprite[ps[k].i].extra = 0;
							}
							
							ps[p].angle.ang = buildang(spriowner->ang);
							
							if (spriowner->owner != spri->owner)
							{
								hiti->temp_data[0] = 13;
								hittype[spri->owner].temp_data[0] = 13;
								ps[p].transporter_hold = 13;
							}
							
							ps[p].bobposx = ps[p].oposx = ps[p].posx = spriowner->x;
							ps[p].bobposy = ps[p].oposy = ps[p].posy = spriowner->y;
							ps[p].oposz = ps[p].posz = spriowner->z - PHEIGHT;
							
							changespritesect(j, spriowner->sectnum);
							ps[p].cursectnum = sprj->sectnum;
							
							if (spri->pal == 0)
							{
								int k = fi.spawn(spri->owner, TRANSPORTERBEAM);
								S_PlayActorSound(TELEPORTER, k);
							}
							
							break;
						}
					}
					else if (!(sectlotag == 1 && ps[p].on_ground == 1)) break;
					
					if (onfloorz == 0 && abs(spri->z - ps[p].posz) < 6144)
						if ((ps[p].jetpack_on == 0) || (ps[p].jetpack_on && (PlayerInput(p, SB_JUMP))) ||
							(ps[p].jetpack_on && (PlayerInput(p, SB_CROUCH) ^ !!ps[p].crouch_toggle)))
						{
							ps[p].oposx = ps[p].posx += spriowner->x - spri->x;
							ps[p].oposy = ps[p].posy += spriowner->y - spri->y;
							
							if (ps[p].jetpack_on && (PlayerInput(p, SB_JUMP) || ps[p].jetpack_on < 11))
								ps[p].posz = spriowner->z - 6144;
							else ps[p].posz = spriowner->z + 6144;
							ps[p].oposz = ps[p].posz;
							
							hittype[ps[p].i].bposx = ps[p].posx;
							hittype[ps[p].i].bposy = ps[p].posy;
							hittype[ps[p].i].bposz = ps[p].posz;
							
							changespritesect(j, spriowner->sectnum);
							ps[p].cursectnum = spriowner->sectnum;
							
							break;
						}
					
					int k = 0;
					
					if (onfloorz && sectlotag == ST_1_ABOVE_WATER && ps[p].on_ground && ps[p].posz > (sector[sect].floorz - (16 << 8)) && (PlayerInput(p, SB_CROUCH) || ps[p].poszv > 2048))
						//                        if( onfloorz && sectlotag == 1 && ps[p].posz > (sector[sect].floorz-(6<<8)) )
					{
						k = 1;
						if (screenpeek == p)
						{
							FX_StopAllSounds();
						}
						if (sprite[ps[p].i].extra > 0)
							S_PlayActorSound(DUKE_UNDERWATER, j);
						ps[p].oposz = ps[p].posz =
						sector[spriowner->sectnum].ceilingz + (7 << 8);
						
						ps[p].posxv = 4096 - (krand() & 8192);
						ps[p].posyv = 4096 - (krand() & 8192);
						
					}
					
					if (onfloorz && sectlotag == ST_2_UNDERWATER && ps[p].posz < (sector[sect].ceilingz + (6 << 8)))
					{
						k = 1;
						//                            if( sprj->extra <= 0) break;
						if (screenpeek == p)
						{
							FX_StopAllSounds();
						}
						S_PlayActorSound(DUKE_GASP, j);
						
						ps[p].oposz = ps[p].posz =
						sector[spriowner->sectnum].floorz - (7 << 8);
						
						ps[p].jumping_toggle = 1;
						ps[p].jumping_counter = 0;
					}
					
					if (k == 1)
					{
						ps[p].oposx = ps[p].posx += spriowner->x - spri->x;
						ps[p].oposy = ps[p].posy += spriowner->y - spri->y;
						
						if (spriowner->owner != spri->owner)
							ps[p].transporter_hold = -2;
						ps[p].cursectnum = spriowner->sectnum;
						
						changespritesect(j, spriowner->sectnum);
						setsprite(ps[p].i, ps[p].posx, ps[p].posy, ps[p].posz + PHEIGHT);
						
						setpal(&ps[p]);
						
						if ((krand() & 255) < 32)
							fi.spawn(j, WATERSPLASH2);
						
						if (sectlotag == 1)
							for (int l = 0; l < 9; l++)
						{
							q = fi.spawn(ps[p].i, WATERBUBBLE);
							sprite[q].z += krand() & 16383;
						}
					}
				}
				break;
				
			case STAT_ACTOR:
				switch (sprj->picnum)
				{
				case SHARK:
				case COMMANDER:
				case OCTABRAIN:
				case GREENSLIME:
				case GREENSLIME + 1:
				case GREENSLIME + 2:
				case GREENSLIME + 3:
				case GREENSLIME + 4:
				case GREENSLIME + 5:
				case GREENSLIME + 6:
				case GREENSLIME + 7:
					if (sprj->extra > 0)
						continue;
				}
			case STAT_PROJECTILE:
			case STAT_MISC:
			case STAT_FALLER:
			case STAT_DUMMYPLAYER:
				
				ll = abs(sprj->zvel);
				
				{
					warpspriteto = 0;
					if (ll && sectlotag == 2 && sprj->z < (sector[sect].ceilingz + ll))
						warpspriteto = 1;
					
					if (ll && sectlotag == 1 && sprj->z > (sector[sect].floorz - ll))
						warpspriteto = 1;
					
					if (sectlotag == 0 && (onfloorz || abs(sprj->z - spri->z) < 4096))
					{
						if (spriowner->owner != spri->owner && onfloorz && hiti->temp_data[0] > 0 && sprj->statnum != 5)
						{
							hiti->temp_data[0]++;
							goto BOLT;
						}
						warpspriteto = 1;
					}
					
					if (warpspriteto) switch (sprj->picnum)
					{
					case TRANSPORTERSTAR:
					case TRANSPORTERBEAM:
					case TRIPBOMB:
					case BULLETHOLE:
					case WATERSPLASH2:
					case BURNING:
					case BURNING2:
					case FIRE:
					case FIRE2:
					case TOILETWATER:
					case LASERLINE:
						continue;
					case PLAYERONWATER:
						if (sectlotag == 2)
						{
							sprj->cstat &= 32767;
							break;
						}
					default:
						if (sprj->statnum == 5 && !(sectlotag == 1 || sectlotag == 2))
							break;
						
					case WATERBUBBLE:
						//if( rnd(192) && sprj->picnum == WATERBUBBLE)
						// break;
						
						if (sectlotag > 0)
						{
							int k = fi.spawn(j, WATERSPLASH2);
							if (sectlotag == 1 && sprj->statnum == 4)
							{
								sprite[k].xvel = sprj->xvel >> 1;
								sprite[k].ang = sprj->ang;
								ssp(k, CLIPMASK0);
							}
						}
						
						switch (sectlotag)
						{
						case 0:
							if (onfloorz)
							{
								if (sprj->statnum == 4 || (checkcursectnums(sect) == -1 && checkcursectnums(spriowner->sectnum) == -1))
								{
									sprj->x += (spriowner->x - spri->x);
									sprj->y += (spriowner->y - spri->y);
									sprj->z -= spri->z - sector[spriowner->sectnum].floorz;
									sprj->ang = spriowner->ang;
									
									hitj->bposx = sprj->x;
									hitj->bposy = sprj->y;
									hitj->bposz = sprj->z;
									
									if (spri->pal == 0)
									{
										int k = fi.spawn(i, TRANSPORTERBEAM);
										S_PlayActorSound(TELEPORTER, k);
										
										k = fi.spawn(spri->owner, TRANSPORTERBEAM);
										S_PlayActorSound(TELEPORTER, k);
									}
									
									if (spriowner->owner != spri->owner)
									{
										hiti->temp_data[0] = 13;
										hittype[spri->owner].temp_data[0] = 13;
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
						case 1:
							sprj->x += (spriowner->x - spri->x);
							sprj->y += (spriowner->y - spri->y);
							sprj->z = sector[spriowner->sectnum].ceilingz + ll;
							
							hitj->bposx = sprj->x;
							hitj->bposy = sprj->y;
							hitj->bposz = sprj->z;
							
							changespritesect(j, spriowner->sectnum);
							
							break;
						case 2:
							sprj->x += (spriowner->x - spri->x);
							sprj->y += (spriowner->y - spri->y);
							sprj->z = sector[spriowner->sectnum].floorz - ll;
							
							hitj->bposx = sprj->x;
							hitj->bposy = sprj->y;
							hitj->bposz = sprj->z;
							
							changespritesect(j, spriowner->sectnum);
							
							break;
						}
						
						break;
					}
				}
				break;
				
			}
		}
	BOLT:;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void greenslime(int i)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int sect = s->sectnum;
	int j;

	// #ifndef isShareware()
	if (ud.multimode < 2)
	{
		if (actor_tog == 1)
		{
			s->cstat = (short)32768;
			return;
		}
		else if (actor_tog == 2) s->cstat = 257;
	}
	// #endif

	t[1] += 128;

	if (sector[sect].floorstat & 1)
	{
		deletesprite(i);
		return;
	}

	int x;
	int p = findplayer(s, &x);

	if (x > 20480)
	{
		hittype[i].timetosleep++;
		if (hittype[i].timetosleep > SLEEPTIME)
		{
			hittype[i].timetosleep = 0;
			changespritestat(i, 2);
			return;
		}
	}

	if (t[0] == -5) // FROZEN
	{
		t[3]++;
		if (t[3] > 280)
		{
			s->pal = 0;
			t[0] = 0;
			return;
		}
		makeitfall(i);
		s->cstat = 257;
		s->picnum = GREENSLIME + 2;
		s->extra = 1;
		s->pal = 1;
		j = fi.ifhitbyweapon(&hittype[i]); if (j >= 0)
		{
			if (j == FREEZEBLAST)
				return;
			for (j = 16; j >= 0; j--)
			{
				int k = EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, sprite[i].z, GLASSPIECES + (j % 3), -32, 36, 36, krand() & 2047, 32 + (krand() & 63), 1024 - (krand() & 1023), i, 5);
				sprite[k].pal = 1;
			}
			ps[p].actors_killed++;
			S_PlayActorSound(GLASS_BREAKING, i);
			deletesprite(i);
		}
		else if (x < 1024 && ps[p].quick_kick == 0)
		{
			j = getincangle(ps[p].angle.ang.asbuild(), getangle(sprite[i].x - ps[p].posx, sprite[i].y - ps[p].posy));
			if (j > -128 && j < 128)
				ps[p].quick_kick = 14;
		}

		return;
	}

	if (x < 1596)
		s->cstat = 0;
	else s->cstat = 257;

	if (t[0] == -4) //On the player
	{
		if (sprite[ps[p].i].extra < 1)
		{
			t[0] = 0;
			return;
		}

		setsprite(i, s->x, s->y, s->z);

		s->ang = ps[p].angle.ang.asbuild();

		if ((PlayerInput(p, SB_FIRE) || (ps[p].quick_kick > 0)) && sprite[ps[p].i].extra > 0)
			if (ps[p].quick_kick > 0 || (ps[p].curr_weapon != HANDREMOTE_WEAPON && ps[p].curr_weapon != HANDBOMB_WEAPON && ps[p].curr_weapon != TRIPBOMB_WEAPON && ps[p].ammo_amount[ps[p].curr_weapon] >= 0))
			{
				for (x = 0; x < 8; x++)
				{
					j = EGS(sect, s->x, s->y, s->z - (8 << 8), SCRAP3 + (krand() & 3), -8, 48, 48, krand() & 2047, (krand() & 63) + 64, -(krand() & 4095) - (s->zvel >> 2), i, 5);
					sprite[j].pal = 6;
				}

				S_PlayActorSound(SLIM_DYING, i);
				S_PlayActorSound(SQUISHED, i);
				if ((krand() & 255) < 32)
				{
					j = fi.spawn(i, BLOODPOOL);
					sprite[j].pal = 0;
				}
				ps[p].actors_killed++;
				t[0] = -3;
				if (ps[p].somethingonplayer == &hittype[i])
					ps[p].somethingonplayer = nullptr;
				deletesprite(i);
				return;
			}

		s->z = ps[p].posz + ps[p].pyoff - t[2] + (8 << 8);

		s->z += -ps[p].horizon.horiz.asq16() >> 12;

		if (t[2] > 512)
			t[2] -= 128;

		if (t[2] < 348)
			t[2] += 128;

		if (ps[p].newowner >= 0)
		{
			ps[p].newowner = -1;
			ps[p].posx = ps[p].oposx;
			ps[p].posy = ps[p].oposy;
			ps[p].posz = ps[p].oposz;
			ps[p].angle.restore();

			updatesector(ps[p].posx, ps[p].posy, &ps[p].cursectnum);
			setpal(&ps[p]);

			StatIterator it(STAT_ACTOR);
			while ((j = it.NextIndex()) >= 0)
			{
				if (sprite[j].picnum == CAMERA1) sprite[j].yvel = 0;
			}
		}

		if (t[3] > 0)
		{
			short frames[] = { 5,5,6,6,7,7,6,5 };

			s->picnum = GREENSLIME + frames[t[3]];

			if (t[3] == 5)
			{
				sprite[ps[p].i].extra += -(5 + (krand() & 3));
				S_PlayActorSound(SLIM_ATTACK, i);
			}

			if (t[3] < 7) t[3]++;
			else t[3] = 0;

		}
		else
		{
			s->picnum = GREENSLIME + 5;
			if (rnd(32))
				t[3] = 1;
		}

		s->xrepeat = 20 + (sintable[t[1] & 2047] >> 13);
		s->yrepeat = 15 + (sintable[t[1] & 2047] >> 13);

		s->x = ps[p].posx + (sintable[(ps[p].angle.ang.asbuild() + 512) & 2047] >> 7);
		s->y = ps[p].posy + (sintable[ps[p].angle.ang.asbuild() & 2047] >> 7);

		return;
	}

	else if (s->xvel < 64 && x < 768)
	{
		if (ps[p].somethingonplayer == nullptr)
		{
			ps[p].somethingonplayer = &hittype[i];
			if (t[0] == 3 || t[0] == 2) //Falling downward
				t[2] = (12 << 8);
			else t[2] = -(13 << 8); //Climbing up duke
			t[0] = -4;
		}
	}

	j = fi.ifhitbyweapon(&hittype[i]); if (j >= 0)
	{
		S_PlayActorSound(SLIM_DYING, i);

		if (ps[p].somethingonplayer == &hittype[i])
			ps[p].somethingonplayer = nullptr;

		if (j == FREEZEBLAST)
		{
			S_PlayActorSound(SOMETHINGFROZE, i); t[0] = -5; t[3] = 0;
			return;
		}
		ps[p].actors_killed++;

		if ((krand() & 255) < 32)
		{
			j = fi.spawn(i, BLOODPOOL);
			sprite[j].pal = 0;
		}

		for (x = 0; x < 8; x++)
		{
			j = EGS(sect, s->x, s->y, s->z - (8 << 8), SCRAP3 + (krand() & 3), -8, 48, 48, krand() & 2047, (krand() & 63) + 64, -(krand() & 4095) - (s->zvel >> 2), i, 5);
			sprite[j].pal = 6;
		}
		t[0] = -3;
		deletesprite(i);
		return;
	}
	// All weap
	if (t[0] == -1) //Shrinking down
	{
		makeitfall(i);

		s->cstat &= 65535 - 8;
		s->picnum = GREENSLIME + 4;

		//					if(s->yrepeat > 62)
		  //					  fi.guts(s,JIBS6,5,myconnectindex);

		if (s->xrepeat > 32) s->xrepeat -= krand() & 7;
		if (s->yrepeat > 16) s->yrepeat -= krand() & 7;
		else
		{
			s->xrepeat = 40;
			s->yrepeat = 16;
			t[5] = -1;
			t[0] = 0;
		}

		return;
	}
	else if (t[0] != -2) getglobalz(i);

	if (t[0] == -2) //On top of somebody (an enemy)
	{
		makeitfall(i);
		sprite[t[5]].xvel = 0;

		int l = sprite[t[5]].ang;

		s->z = sprite[t[5]].z;
		s->x = sprite[t[5]].x + (sintable[(l + 512) & 2047] >> 11);
		s->y = sprite[t[5]].y + (sintable[l & 2047] >> 11);

		s->picnum = GREENSLIME + 2 + (global_random & 1);

		if (s->yrepeat < 64) s->yrepeat += 2;
		else
		{
			if (s->xrepeat < 32) s->xrepeat += 4;
			else
			{
				t[0] = -1;
				x = ldist(s, &sprite[t[5]]);
				if (x < 768) {
					sprite[t[5]].xrepeat = 0;
				}
			}
		}
		return;
	}

	//Check randomly to see of there is an actor near
	if (rnd(32))
	{
		SectIterator it(sect);
		while ((j = it.NextIndex()) >= 0)
		{
			switch (sprite[j].picnum)
			{
			case LIZTROOP:
			case LIZMAN:
			case PIGCOP:
			case NEWBEAST:
				if (ldist(s, &sprite[j]) < 768 && (abs(s->z - sprite[j].z) < 8192)) //Gulp them
				{
					t[5] = j;
					t[0] = -2;
					t[1] = 0;
					return;
				}
			}
		}
	}

	//Moving on the ground or ceiling

	if (t[0] == 0 || t[0] == 2)
	{
		s->picnum = GREENSLIME;

		if ((krand() & 511) == 0)
			S_PlayActorSound(SLIM_ROAM, i);

		if (t[0] == 2)
		{
			s->zvel = 0;
			s->cstat &= (65535 - 8);

			if ((sector[sect].ceilingstat & 1) || (hittype[i].ceilingz + 6144) < s->z)
			{
				s->z += 2048;
				t[0] = 3;
				return;
			}
		}
		else
		{
			s->cstat |= 8;
			makeitfall(i);
		}

		if (everyothertime & 1) ssp(i, CLIPMASK0);

		if (s->xvel > 96)
		{
			s->xvel -= 2;
			return;
		}
		else
		{
			if (s->xvel < 32) s->xvel += 4;
			s->xvel = 64 - (sintable[(t[1] + 512) & 2047] >> 9);

			s->ang += getincangle(s->ang,
				getangle(ps[p].posx - s->x, ps[p].posy - s->y)) >> 3;
			// TJR
		}

		s->xrepeat = 36 + (sintable[(t[1] + 512) & 2047] >> 11);
		s->yrepeat = 16 + (sintable[t[1] & 2047] >> 13);

		if (rnd(4) && (sector[sect].ceilingstat & 1) == 0 &&
			abs(hittype[i].floorz - hittype[i].ceilingz)
			< (192 << 8))
		{
			s->zvel = 0;
			t[0]++;
		}

	}

	if (t[0] == 1)
	{
		s->picnum = GREENSLIME;
		if (s->yrepeat < 40) s->yrepeat += 8;
		if (s->xrepeat > 8) s->xrepeat -= 4;
		if (s->zvel > -(2048 + 1024))
			s->zvel -= 348;
		s->z += s->zvel;
		if (s->z < hittype[i].ceilingz + 4096)
		{
			s->z = hittype[i].ceilingz + 4096;
			s->xvel = 0;
			t[0] = 2;
		}
	}

	if (t[0] == 3)
	{
		s->picnum = GREENSLIME + 1;

		makeitfall(i);

		if (s->z > hittype[i].floorz - (8 << 8))
		{
			s->yrepeat -= 4;
			s->xrepeat += 2;
		}
		else
		{
			if (s->yrepeat < (40 - 4)) s->yrepeat += 8;
			if (s->xrepeat > 8) s->xrepeat -= 4;
		}

		if (s->z > hittype[i].floorz - 2048)
		{
			s->z = hittype[i].floorz - 2048;
			t[0] = 0;
			s->xvel = 0;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void flamethrowerflame(int i)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int sect = s->sectnum;
	int x, j;
	int p = findplayer(s, &x);
	execute(i, p, x);
	t[0]++;
	if (sector[sect].lotag == 2)
	{
		sprite[fi.spawn(i, EXPLOSION2)].shade = 127;
		deletesprite(i);
		return;
	}

	int dax = s->x;
	int day = s->y;
	int daz = s->z;
	int xvel = s->xvel;
	int zvel = s->zvel;

	getglobalz(i);

	int ds = t[0] / 6;
	if (s->xrepeat < 80)
		s->yrepeat = s->xrepeat += ds;
	s->clipdist += ds;
	if (t[0] <= 2)
		t[3] = krand() % 10;
	if (t[0] > 30) 
	{
		sprite[fi.spawn(i, EXPLOSION2)].shade = 127;
		deletesprite(i);
		return;
	}

	j = fi.movesprite(i, (xvel * (sintable[(s->ang + 512) & 2047])) >> 14,
		(xvel * (sintable[s->ang & 2047])) >> 14, s->zvel, CLIPMASK1);

	if (s->sectnum < 0)
	{
		deletesprite(i);
		return;
	}

	if ((j & kHitTypeMask) != kHitSprite)
	{
		if (s->z < hittype[i].ceilingz)
		{
			j = kHitSector | (s->sectnum);
			s->zvel = -1;
		}
		else if ((s->z > hittype[i].floorz && sector[s->sectnum].lotag != 1)
			|| (s->z > hittype[i].floorz + (16 << 8) && sector[s->sectnum].lotag == 1))
		{
			j = kHitSector | (s->sectnum);
			if (sector[s->sectnum].lotag != 1)
				s->zvel = 1;
		}
	}

	if (j != 0) {
		s->xvel = s->yvel = s->zvel = 0;
		if ((j & kHitTypeMask) == kHitSprite)
		{
			j &= (MAXSPRITES - 1);
			fi.checkhitsprite((short)j, i);
			if (sprite[j].picnum == APLAYER)
				S_PlayActorSound(j, PISTOL_BODYHIT);
		}
		else if ((j & kHitTypeMask) == kHitWall)
		{
			j &= (MAXWALLS - 1);
			setsprite(i, dax, day, daz);
			fi.checkhitwall(i, j, s->x, s->y, s->z, s->picnum);
		}
		else if ((j & kHitTypeMask) == kHitSector)
		{
			setsprite(i, dax, day, daz);
			if (s->zvel < 0)
				fi.checkhitceiling(s->sectnum);
		}

		if (s->xrepeat >= 10)
		{
			x = s->extra;
			fi.hitradius(&hittype[i], rpgblastradius, x >> 2, x >> 1, x - (x >> 2), x);
		}
		else
		{
			x = s->extra + (global_random & 3);
			fi.hitradius(&hittype[i], (rpgblastradius >> 1), x >> 2, x >> 1, x - (x >> 2), x);
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

	if (s->picnum != BOUNCEMINE)
	{
		makeitfall(i);

		if (sector[sect].lotag != 1 && s->z >= hittype[i].floorz - (FOURSLEIGHT) && s->yvel < 3)
		{
			if (s->yvel > 0 || (s->yvel == 0 && hittype[i].floorz == sector[sect].floorz))
				S_PlayActorSound(PIPEBOMB_BOUNCE, i);
			s->zvel = -((4 - s->yvel) << 8);
			if (sector[s->sectnum].lotag == 2)
				s->zvel >>= 2;
			s->yvel++;
		}
		if (s->z < hittype[i].ceilingz) // && sector[sect].lotag != 2 )
		{
			s->z = hittype[i].ceilingz + (3 << 8);
			s->zvel = 0;
		}
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
		}
	}
	else t[5] = 0;

	if (t[3] == 0 && (s->picnum == BOUNCEMINE || s->picnum == MORTER) && (j || x < 844))
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

		s->ang = ((k << 1) - s->ang) & 2047;
		s->xvel >>= 1;
	}

DETONATEB:

	bool bBoom = false;
	if ((l >= 0 && ps[l].hbomb_on == 0) || t[3] == 1)
		bBoom = true;
	if (isNamWW2GI() && s->picnum == HEAVYHBOMB)
	{
		s->extra--;
		if (s->extra <= 0)
			bBoom = true;
	}
	if (bBoom)
	{
		t[4]++;

		if (t[4] == 2)
		{
			x = s->extra;
			int m = 0;
			switch (s->picnum)
			{
			case HEAVYHBOMB: m = pipebombblastradius; break;
			case MORTER: m = morterblastradius; break;
			case BOUNCEMINE: m = bouncemineblastradius; break;
			}

			fi.hitradius(&hittype[i], m, x >> 2, x >> 1, x - (x >> 2), x);
			fi.spawn(i, EXPLOSION2);
			if (s->zvel == 0)
				fi.spawn(i, EXPLOSION2BOT);
			S_PlayActorSound(PIPEBOMB_EXPLODE, i);
			for (x = 0; x < 8; x++)
				RANDOMSCRAP(s, i);
		}

		if (s->yrepeat)
		{
			s->yrepeat = 0;
			return;
		}

		if (t[4] > 20)
		{
			if (s->owner != i || ud.respawn_items == 0)
			{
				deletesprite(i);
				return;
			}
			else
			{
				t[2] = respawnitemtime;
				fi.spawn(i, RESPAWNMARKERRED);
				s->cstat = (short)32768;
				s->yrepeat = 9;
				return;
			}
		}
	}
	else if (s->picnum == HEAVYHBOMB && x < 788 && t[0] > 7 && s->xvel == 0)
		if (cansee(s->x, s->y, s->z - (8 << 8), s->sectnum, ps[p].posx, ps[p].posy, ps[p].posz, ps[p].cursectnum))
			if (ps[p].ammo_amount[HANDBOMB_WEAPON] < max_ammo_amount[HANDBOMB_WEAPON])
			{
				if (ud.coop >= 1 && s->owner == i)
				{
					for (j = 0; j < ps[p].weapreccnt; j++)
						if (ps[p].weaprecs[j] == s->picnum)
							continue;

					if (ps[p].weapreccnt < 255) // DukeGDX has 16 here.
						ps[p].weaprecs[ps[p].weapreccnt++] = s->picnum;
				}

				addammo(HANDBOMB_WEAPON, &ps[p], 1);
				S_PlayActorSound(DUKE_GET, ps[p].i);

				if (ps[p].gotweapon[HANDBOMB_WEAPON] == 0 || s->owner == ps[p].i)
					fi.addweapon(&ps[p], HANDBOMB_WEAPON);

				if (sprite[s->owner].picnum != APLAYER)
				{
					SetPlayerPal(&ps[p], PalEntry(32, 0, 32, 0));
				}

				if (s->owner != i || ud.respawn_items == 0)
				{
					if (s->owner == i && ud.coop >= 1)
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

void moveactors_d(void)
{
	int x, * t;
	short j, sect, p;
	spritetype* s;
	unsigned short k;
	
	StatIterator it(STAT_ACTOR);
	int i;
	while ((i = it.NextIndex()) >= 0)
	{
		s = &sprite[i];

		sect = s->sectnum;

		if (s->xrepeat == 0 || sect < 0 || sect >= MAXSECTORS)
		{ 
			deletesprite(i);
			continue;
		}

		t = &hittype[i].temp_data[0];

		hittype[i].bposx = s->x;
		hittype[i].bposy = s->y;
		hittype[i].bposz = s->z;


		switch (s->picnum)
		{
		case FLAMETHROWERFLAME:
			if (isWorldTour()) flamethrowerflame(i);
			continue;

		case DUCK:
		case TARGET:
			if (s->cstat & 32)
			{
				t[0]++;
				if (t[0] > 60)
				{
					t[0] = 0;
					s->cstat = 128 + 257 + 16;
					s->extra = 1;
				}
			}
			else
			{
				j = fi.ifhitbyweapon(&hittype[i]);
				if (j >= 0)
				{
					s->cstat = 32 + 128;
					k = 1;

					StatIterator it(STAT_ACTOR);
					int j;
					while ((j = it.NextIndex()) >= 0)
					{
						auto sj = &sprite[j];
						if (sj->lotag == s->lotag &&
							sj->picnum == s->picnum)
						{
							if ((sj->hitag && !(sj->cstat & 32)) ||
								(!sj->hitag && (sj->cstat & 32))
								)
							{
								k = 0;
								break;
							}
						}
					}

					if (k == 1)
					{
						operateactivators(s->lotag, -1);
						fi.operateforcefields(i, s->lotag);
						operatemasterswitches(s->lotag);
					}
				}
			}
			continue;

		case RESPAWNMARKERRED:
		case RESPAWNMARKERYELLOW:
		case RESPAWNMARKERGREEN:
			if (!respawnmarker(i, RESPAWNMARKERYELLOW, RESPAWNMARKERGREEN)) continue;
			break;

		case HELECOPT:
		case DUKECAR:

			s->z += s->zvel;
			t[0]++;

			if (t[0] == 4) S_PlayActorSound(WAR_AMBIENCE2, i);

			if (t[0] > (26 * 8))
			{
				S_PlaySound(RPG_EXPLODE);
				for (j = 0; j < 32; j++) 
						RANDOMSCRAP(s, i);
				earthquaketime = 16;
				deletesprite(i);
				continue;
			} 
			else if ((t[0] & 3) == 0)
				fi.spawn(i, EXPLOSION2);
			ssp(i, CLIPMASK0);
			break;
		case RAT:
			if (!rat(i, true)) continue;
			break;
		case QUEBALL:
		case STRIPEBALL:
			if (!queball(i, POCKET, QUEBALL, STRIPEBALL)) continue;
			break;
		case FORCESPHERE:
			forcesphere(i, FORCESPHERE);
			continue;

		case RECON:
			recon(i, EXPLOSION2, FIRELASER, RECO_ATTACK, RECO_PAIN, RECO_ROAM, 10, [](int i)->int { return PIGCOP; });
			continue;

		case OOZ:
		case OOZ2:
			ooz(i);
			continue;

		case GREENSLIME:
		case GREENSLIME + 1:
		case GREENSLIME + 2:
		case GREENSLIME + 3:
		case GREENSLIME + 4:
		case GREENSLIME + 5:
		case GREENSLIME + 6:
		case GREENSLIME + 7:
			greenslime(i);
			continue;

		case BOUNCEMINE:
		case MORTER:
			j = fi.spawn(i, FRAMEEFFECT1);
			hittype[j].temp_data[0] = 3;

		case HEAVYHBOMB:
			heavyhbomb(i);
			continue;

		case REACTORBURNT:
		case REACTOR2BURNT:
			continue;

		case REACTOR:
		case REACTOR2:
			reactor(i, REACTOR, REACTOR2, REACTORBURNT, REACTOR2BURNT, REACTORSPARK, REACTOR2SPARK);
			continue;

		case CAMERA1:
			camera(i);
			continue;
		}


		// #ifndef VOLOMEONE
		if (ud.multimode < 2 && badguy(s))
		{
			if (actor_tog == 1)
			{
				s->cstat = (short)32768;
				continue;
			}
			else if (actor_tog == 2) s->cstat = 257;
		}
		// #endif

		p = findplayer(s, &x);

		execute(i, p, x);
	}

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void fireflyflyingeffect(int i)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int x, p = findplayer(s, &x);
	execute(i, p, x);

	auto owner = &sprite[s->owner];
	if (owner->picnum != FIREFLY) 
	{
		deletesprite(i);
		return;
	}

	if (owner->xrepeat >= 24 || owner->pal == 1)
		s->cstat |= 0x8000;
	else
		s->cstat &= ~0x8000;

	double dx = owner->x - sprite[ps[p].i].x;
	double dy = owner->y - sprite[ps[p].i].y;
	double dist = sqrt(dx * dx + dy * dy);
	if (dist != 0.0) 
	{
		dx /= dist;
		dy /= dist;
	}

	s->x = (int) (owner->x - (dx * -10.0));
	s->y = (int) (owner->y - (dy * -10.0));
	s->z = owner->z + 2048;

	if (owner->extra <= 0) 
	{
		deletesprite(i);
	}

}
//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveexplosions_d(void)  // STATNUM 5
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
		case FIREFLYFLYINGEFFECT:
			if (isWorldTour()) fireflyflyingeffect(i);
			continue;
			
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

		case NUKEBUTTON:
		case NUKEBUTTON + 1:
		case NUKEBUTTON + 2:
		case NUKEBUTTON + 3:

			if (t[0])
			{
				t[0]++;
				if (t[0] == 8) s->picnum = NUKEBUTTON + 1;
				else if (t[0] == 16)
				{
					s->picnum = NUKEBUTTON + 2;
					ps[sprite[s->owner].yvel].fist_incs = 1;
				}
				if (ps[sprite[s->owner].yvel].fist_incs == 26)
					s->picnum = NUKEBUTTON + 3;
			}
			continue;

		case FORCESPHERE:
			forcesphere(i);
			continue;
		case WATERSPLASH2:
			watersplash2(i);
			continue;

		case FRAMEEFFECT1:
			frameeffect1(i);
			continue;
		case INNERJAW:
		case INNERJAW + 1:

			p = findplayer(s, &x);
			if (x < 512)
			{
				SetPlayerPal(&ps[p], PalEntry(32, 32, 0, 0));
				sprite[ps[p].i].extra -= 4;
			}

		case FIRELASER:
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
		case MAIL + 1:
		case PAPER + 1:
			hittype[i].floorz = s->z = getflorzofslope(s->sectnum, s->x, s->y);
			break;
		case MONEY:
		case MAIL:
		case PAPER:
			money(i, BLOODPOOL);

			break;

		case JIBS1:
		case JIBS2:
		case JIBS3:
		case JIBS4:
		case JIBS5:
		case JIBS6:
		case HEADJIB1:
		case ARMJIB1:
		case LEGJIB1:
		case LIZMANHEAD1:
		case LIZMANARM1:
		case LIZMANLEG1:
		case DUKETORSO:
		case DUKEGUN:
		case DUKELEG:
			jibs(i, JIBS6, true, false, false, s->picnum == DUKELEG || s->picnum == DUKETORSO || s->picnum == DUKEGUN, false);

			continue;
		case BLOODPOOL:
		case PUKE:
			bloodpool(i, s->picnum == PUKE, TIRE);

			continue;

		case LAVAPOOL:
		case ONFIRE:
		case ONFIRESMOKE:
		case BURNEDCORPSE:
		case LAVAPOOLBUBBLE:
		case WHISPYSMOKE:
			if (!isWorldTour())
				continue;

		case BURNING:
		case BURNING2:
		case FECES:
		case WATERBUBBLE:
		case SMALLSMOKE:
		case EXPLOSION2:
		case SHRINKEREXPLOSION:
		case EXPLOSION2BOT:
		case BLOOD:
		case LASERSITE:
		case FORCERIPPLE:
		case TRANSPORTERSTAR:
		case TRANSPORTERBEAM:
			p = findplayer(s, &x);
			execute(i, p, x);
			continue;

		case SHELL:
		case SHOTGUNSHELL:
			shell(i, (sector[sect].floorz + (24 << 8)) < s->z);
			continue;

		case GLASSPIECES:
		case GLASSPIECES + 1:
		case GLASSPIECES + 2:
			glasspieces(i);
			continue;
		}

		if (s->picnum >= SCRAP6 && s->picnum <= SCRAP5 + 3)
		{
			scrap(i, SCRAP1, SCRAP6);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveeffectors_d(void)   //STATNUM 3
{
	int q = 0, l, x, st, j, * t;
	int p, sh;
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
			handle_se00(i, LASERLINE);
			break;
			
		case SE_1_PIVOT: //Nothing for now used as the pivot
			handle_se01(i);
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
				}
			}
			else s->xvel = k;

			StatIterator it(STAT_EFFECTOR);
			int j;
			while ((j = it.NextIndex()) >= 0)
			{
				auto sj = &sprite[j];
				if ((sj->lotag == 14) && (sh == sj->hitag) && (hittype[j].temp_data[0] == t[0]))
				{
					sj->xvel = s->xvel;
					//						if( t[4] == 1 )
					{
						if (hittype[j].temp_data[5] == 0)
							hittype[j].temp_data[5] = dist(sj, s);
						x = sgn(dist(sj, s) - hittype[j].temp_data[5]);
						if (sj->extra)
							x = -x;
						s->xvel += x;
					}
					hittype[j].temp_data[4] = t[4];
				}
			}
			x = 0;
		}
		case SE_14_SUBWAY_CAR:
			handle_se14(i, true, RPG, JIBS6);
			break;

		case SE_30_TWO_WAY_TRAIN:
			handle_se30(i, JIBS6);
			break;


		case SE_2_EARTHQUAKE:
			handle_se02(i);
			break;

			//Flashing sector lights after reactor EXPLOSION2
		case SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT:
			handle_se03(i);
			break;

		case SE_4_RANDOM_LIGHTS:
			handle_se04(i);
			break;

			//BOSS
		case SE_5_BOSS:
			handle_se05(i, FIRELASER);
			break;

		case SE_8_UP_OPEN_DOOR_LIGHTS:
		case SE_9_DOWN_OPEN_DOOR_LIGHTS:
			handle_se08(i, false);
			break;

		case SE_10_DOOR_AUTO_CLOSE:
		{
			static const int tags[] = { 20, 21, 22, 26, 0};
			handle_se10(i, tags);
			break;
		}
		case SE_11_SWINGING_DOOR:
			handle_se11(i);
			break;
			
		case SE_12_LIGHT_SWITCH:
			handle_se12(i);
			break;

		case SE_13_EXPLOSIVE:
			handle_se13(i);
			break;

		case SE_15_SLIDING_DOOR:
			handle_se15(i);
			break;

		case SE_16_REACTOR:
			handle_se16(i, REACTOR, REACTOR2);
			break;

		case SE_17_WARP_ELEVATOR:
			handle_se17(i);
			break;

		case SE_18_INCREMENTAL_SECTOR_RISE_FALL:
			handle_se18(i, true);
			break;

		case SE_19_EXPLOSION_LOWERS_CEILING:
			handle_se19(i, BIGFORCE);
			break;

		case SE_20_STRETCH_BRIDGE:
			handle_se20(i);
			break;

		case SE_21_DROP_FLOOR:
			handle_se21(i);
			break;

		case SE_22_TEETH_DOOR:
			handle_se22(i);

			break;

		case SE_24_CONVEYOR:
		case 34:
		{
			if (t[4]) break;

			x = (sprite[i].yvel * sintable[(s->ang + 512) & 2047]) >> 18;
			l = (sprite[i].yvel * sintable[s->ang & 2047]) >> 18;

			k = 0;

			SectIterator it(s->sectnum);
			while ((j = it.NextIndex()) >= 0)
			{
				auto sprj = &sprite[j];
				if (sprj->zvel >= 0)
					switch (sprj->statnum)
					{
					case 5:
						switch (sprj->picnum)
						{
						case BLOODPOOL:
						case PUKE:
						case FOOTPRINTS:
						case FOOTPRINTS2:
						case FOOTPRINTS3:
						case FOOTPRINTS4:
						case BULLETHOLE:
						case BLOODSPLAT1:
						case BLOODSPLAT2:
						case BLOODSPLAT3:
						case BLOODSPLAT4:
							sprj->xrepeat = sprj->yrepeat = 0;
							continue;
						case LASERLINE:
							continue;
						}
					case 6:
						if (sprj->picnum == TRIPBOMB) break;
					case 1:
					case 0:
						if (
							sprj->picnum == BOLT1 ||
							sprj->picnum == BOLT1 + 1 ||
							sprj->picnum == BOLT1 + 2 ||
							sprj->picnum == BOLT1 + 3 ||
							sprj->picnum == SIDEBOLT1 ||
							sprj->picnum == SIDEBOLT1 + 1 ||
							sprj->picnum == SIDEBOLT1 + 2 ||
							sprj->picnum == SIDEBOLT1 + 3 ||
							wallswitchcheck(j)
							)
							break;

						if (!(sprj->picnum >= CRANE && sprj->picnum <= (CRANE + 3)))
						{
							if (sprj->z > (hittype[j].floorz - (16 << 8)))
							{
								hittype[j].bposx = sprj->x;
								hittype[j].bposy = sprj->y;

								sprj->x += x >> 2;
								sprj->y += l >> 2;

								setsprite(j, sprj->x, sprj->y, sprj->z);

								if (sector[sprj->sectnum].floorstat & 2)
									if (sprj->statnum == 2)
										makeitfall(j);
							}
						}
						break;
					}
			}

			for (p = connecthead; p >= 0; p = connectpoint2[p])
			{
				if (ps[p].cursectnum == s->sectnum && ps[p].on_ground)
				{
					if (abs(ps[p].pos.z - ps[p].truefz) < PHEIGHT + (9 << 8))
					{
						ps[p].fric.x += x << 3;
						ps[p].fric.y += l << 3;
					}
				}
			}

			sc->floorxpanning += sprite[i].yvel >> 7;

			break;
		}
		case 35:
			handle_se35(i, SMALLSMOKE, EXPLOSION2);
			break;

		case 25: //PISTONS

			if (t[4] == 0) break;

			if (sc->floorz <= sc->ceilingz)
				s->shade = 0;
			else if (sc->ceilingz <= t[3])
				s->shade = 1;

			if (s->shade)
			{
				sc->ceilingz += sprite[i].yvel  << 4;
				if (sc->ceilingz > sc->floorz)
					sc->ceilingz = sc->floorz;
			}
			else
			{
				sc->ceilingz -= sprite[i].yvel  << 4;
				if (sc->ceilingz < t[3])
					sc->ceilingz = t[3];
			}

			break;

		case 26:
			handle_se26(i);
			break;

		case SE_27_DEMO_CAM:
			handle_se27(i);
			break;
		case 28:
			if (t[5] > 0)
			{
				t[5]--;
				break;
			}

			if (hittype[i].temp_data[0] == 0)
			{
				p = findplayer(s, &x);
				if (x > 15500)
					break;
				hittype[i].temp_data[0] = 1;
				hittype[i].temp_data[1] = 64 + (krand() & 511);
				hittype[i].temp_data[2] = 0;
			}
			else
			{
				hittype[i].temp_data[2]++;
				if (hittype[i].temp_data[2] > hittype[i].temp_data[1])
				{
					hittype[i].temp_data[0] = 0;
					ps[screenpeek].visibility = ud.const_visibility;
					break;
				}
				else if (hittype[i].temp_data[2] == (hittype[i].temp_data[1] >> 1))
					S_PlayActorSound(THUNDER, i);
				else if (hittype[i].temp_data[2] == (hittype[i].temp_data[1] >> 3))
					S_PlayActorSound(LIGHTNING_SLAP, i);
				else if (hittype[i].temp_data[2] == (hittype[i].temp_data[1] >> 2))
				{
					StatIterator it(STAT_DEFAULT);
					while ((j = it.NextIndex()) >= 0)
					{
						if (sprite[j].picnum == NATURALLIGHTNING && sprite[j].hitag == s->hitag)
							sprite[j].cstat |= 32768;
					}
				}
				else if (hittype[i].temp_data[2] > (hittype[i].temp_data[1] >> 3) && hittype[i].temp_data[2] < (hittype[i].temp_data[1] >> 2))
				{
					if (cansee(s->x, s->y, s->z, s->sectnum, ps[screenpeek].posx, ps[screenpeek].posy, ps[screenpeek].posz, ps[screenpeek].cursectnum))
						j = 1;
					else j = 0;

					if (rnd(192) && (hittype[i].temp_data[2] & 1))
					{
						if (j)
							ps[screenpeek].visibility = 0;
					}
					else if (j)
						ps[screenpeek].visibility = ud.const_visibility;

					StatIterator it(STAT_DEFAULT);
					while ((j = it.NextIndex()) >= 0)
					{
						auto sj = &sprite[j];
						if (sj->picnum == NATURALLIGHTNING && sj->hitag == s->hitag)
						{
							if (rnd(32) && (hittype[i].temp_data[2] & 1))
							{
								sj->cstat &= 32767;
								fi.spawn(j, SMALLSMOKE);

								p = findplayer(s, &x);
								x = ldist(&sprite[ps[p].i], sj);
								if (x < 768)
								{
									if (S_CheckSoundPlaying(ps[p].i, DUKE_LONGTERM_PAIN) < 1)
										S_PlayActorSound(DUKE_LONGTERM_PAIN, ps[p].i);
									S_PlayActorSound(SHORT_CIRCUIT, ps[p].i);
									sprite[ps[p].i].extra -= 8 + (krand() & 7);
									SetPlayerPal(&ps[p], PalEntry(32, 16, 0, 0));
								}
								break;
							}
							else sj->cstat |= 32768;
						}
					}
				}
			}
			break;
		case 29:
			s->hitag += 64;
			l = mulscale12((int)s->yvel, sintable[s->hitag & 2047]);
			sc->floorz = s->z + l;
			break;
		case 31: // True Drop Floor
			if (t[0] == 1)
			{
				// Choose dir

				if (t[3] > 0)
				{
					t[3]--;
					break;
				}

				if (t[2] == 1) // Retract
				{
					if (sprite[i].ang != 1536)
					{
						if (abs(sc->floorz - s->z) < sprite[i].yvel )
						{
							sc->floorz = s->z;
							t[2] = 0;
							t[0] = 0;
							t[3] = s->hitag;
							callsound(s->sectnum, i);
						}
						else
						{
							l = sgn(s->z - sc->floorz) * sprite[i].yvel ;
							sc->floorz += l;

							SectIterator it(s->sectnum);
							while ((j = it.NextIndex()) >= 0)
							{
								auto sj = &sprite[j];
								if (sj->picnum == APLAYER && sj->owner >= 0)
									if (ps[sj->yvel].on_ground == 1)
										ps[sj->yvel].posz += l;
								if (sj->zvel == 0 && sj->statnum != 3 && sj->statnum != 4)
								{
									hittype[j].bposz = sj->z += l;
									hittype[j].floorz = sc->floorz;
								}
							}
						}
					}
					else
					{
						if (abs(sc->floorz - t[1]) < sprite[i].yvel )
						{
							sc->floorz = t[1];
							callsound(s->sectnum, i);
							t[2] = 0;
							t[0] = 0;
							t[3] = s->hitag;
						}
						else
						{
							l = sgn(t[1] - sc->floorz) * sprite[i].yvel ;
							sc->floorz += l;

							SectIterator it(s->sectnum);
							while ((j = it.NextIndex()) >= 0)
							{
								auto sj = &sprite[j];
								if (sj->picnum == APLAYER && sj->owner >= 0)
									if (ps[sj->yvel].on_ground == 1)
										ps[sj->yvel].posz += l;
								if (sj->zvel == 0 && sj->statnum != 3 && sj->statnum != 4)
								{
									hittype[j].bposz = sj->z += l;
									hittype[j].floorz = sc->floorz;
								}
							}
						}
					}
					break;
				}

				if ((s->ang & 2047) == 1536)
				{
					if (abs(s->z - sc->floorz) < sprite[i].yvel )
					{
						callsound(s->sectnum, i);
						t[0] = 0;
						t[2] = 1;
						t[3] = s->hitag;
					}
					else
					{
						l = sgn(s->z - sc->floorz) * sprite[i].yvel ;
						sc->floorz += l;

						SectIterator it(s->sectnum);
						while ((j = it.NextIndex()) >= 0)
						{
							auto sj = &sprite[j];
							if (sj->picnum == APLAYER && sj->owner >= 0)
								if (ps[sj->yvel].on_ground == 1)
									ps[sj->yvel].posz += l;
							if (sj->zvel == 0 && sj->statnum != 3 && sj->statnum != 4)
							{
								hittype[j].bposz = sj->z += l;
								hittype[j].floorz = sc->floorz;
							}
						}
					}
				}
				else
				{
					if (abs(sc->floorz - t[1]) < sprite[i].yvel )
					{
						t[0] = 0;
						callsound(s->sectnum, i);
						t[2] = 1;
						t[3] = s->hitag;
					}
					else
					{
						l = sgn(s->z - t[1]) * sprite[i].yvel ;
						sc->floorz -= l;

						SectIterator it(s->sectnum);
						while ((j = it.NextIndex()) >= 0)
						{
							auto sj = &sprite[j];
							if (sj->picnum == APLAYER && sj->owner >= 0)
								if (ps[sj->yvel].on_ground == 1)
									ps[sj->yvel].posz -= l;
							if (sj->zvel == 0 && sj->statnum != 3 && sj->statnum != 4)
							{
								hittype[j].bposz = sj->z -= l;
								hittype[j].floorz = sc->floorz;
							}
						}
					}
				}
			}
			break;

		case 32: // True Drop Ceiling
			handle_se32(i);
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
			handle_se128(i);
			break;

		case 130:
			handle_se130(i, 80, EXPLOSION2);
			break;
		case 131:
			handle_se130(i, 40, EXPLOSION2);
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
// 
//
//---------------------------------------------------------------------------

void move_d(int g_i, int g_p, int g_x)
{
	auto g_sp = &sprite[g_i];
	auto g_t = hittype[g_i].temp_data;
	int l;
	short goalang, angdif;
	int daxvel;

	int a = g_sp->hitag;

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


	if ((a & jumptoplayer) == jumptoplayer)
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
		return;
	}

	auto moveptr = &ScriptCode[g_t[1]];

	if (a & geth) g_sp->xvel += (*moveptr - g_sp->xvel) >> 1;
	if (a & getv) g_sp->zvel += ((*(moveptr + 1) << 4) - g_sp->zvel) >> 1;

	if (a & dodgebullet)
		dodge(g_sp);

	if (g_sp->picnum != APLAYER)
		alterang(a, g_i, g_p);

	if (g_sp->xvel > -6 && g_sp->xvel < 6) g_sp->xvel = 0;

	a = badguy(g_sp);

	if (g_sp->xvel || g_sp->zvel)
	{
		if (a && g_sp->picnum != ROTATEGUN)
		{
			if ((g_sp->picnum == DRONE || g_sp->picnum == COMMANDER) && g_sp->extra > 0)
			{
				if (g_sp->picnum == COMMANDER)
				{
					hittype[g_i].floorz = l = getflorzofslope(g_sp->sectnum, g_sp->x, g_sp->y);
					if (g_sp->z > (l - (8 << 8)))
					{
						if (g_sp->z > (l - (8 << 8))) g_sp->z = l - (8 << 8);
						g_sp->zvel = 0;
					}

					hittype[g_i].ceilingz = l = getceilzofslope(g_sp->sectnum, g_sp->x, g_sp->y);
					if ((g_sp->z - l) < (80 << 8))
					{
						g_sp->z = l + (80 << 8);
						g_sp->zvel = 0;
					}
				}
				else
				{
					if (g_sp->zvel > 0)
					{
						hittype[g_i].floorz = l = getflorzofslope(g_sp->sectnum, g_sp->x, g_sp->y);
						if (g_sp->z > (l - (30 << 8)))
							g_sp->z = l - (30 << 8);
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
			}
			else if (g_sp->picnum != ORGANTIC)
			{
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
		}
		else if (g_sp->picnum == APLAYER)
			if ((g_sp->z - hittype[g_i].ceilingz) < (32 << 8))
				g_sp->z = hittype[g_i].ceilingz + (32 << 8);

		daxvel = g_sp->xvel;
		angdif = g_sp->ang;

		if (a && g_sp->picnum != ROTATEGUN)
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
			else if (g_sp->picnum != DRONE && g_sp->picnum != SHARK && g_sp->picnum != COMMANDER)
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

		hittype[g_i].movflag = fi.movesprite(g_i,
			(daxvel * (sintable[(angdif + 512) & 2047])) >> 14,
			(daxvel * (sintable[angdif & 2047])) >> 14, g_sp->zvel, CLIPMASK0);
	}

	if (a)
	{
		if (sector[g_sp->sectnum].ceilingstat & 1)
			g_sp->shade += (sector[g_sp->sectnum].ceilingshade - g_sp->shade) >> 1;
		else g_sp->shade += (sector[g_sp->sectnum].floorshade - g_sp->shade) >> 1;

		if (sector[g_sp->sectnum].floorpicnum == MIRROR)
			deletesprite(g_i);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void falladjustz(spritetype* g_sp)
{
	switch (g_sp->picnum)
	{
	case OCTABRAIN:
	case COMMANDER:
	case DRONE:
		break;
	default:
		g_sp->z += (24 << 8);
		break;
	}
}

void fall_d(int g_i, int g_p)
{
	fall_common(g_i, g_p, JIBS6, DRONE, BLOODPOOL, SHOTSPARK1, SQUISHED, THUD, nullptr, falladjustz);
}

bool spawnweapondebris_d(int picnum, int dnum)
{
	return picnum == BLIMP && dnum == SCRAP1;
}

void respawnhitag_d(spritetype* g_sp)
{
	switch (g_sp->picnum)
	{
	case FEM1:
	case FEM2:
	case FEM3:
	case FEM4:
	case FEM5:
	case FEM6:
	case FEM7:
	case FEM8:
	case FEM9:
	case FEM10:
	case PODFEM1:
	case NAKED1:
	case STATUE:
		if (g_sp->yvel) fi.operaterespawns(g_sp->yvel);
		break;
	default:
		if (g_sp->hitag >= 0) fi.operaterespawns(g_sp->hitag);
		break;
	}
}

void checktimetosleep_d(int g_i)
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

void think_d(void)
{
	thinktime.Reset();
	thinktime.Clock();
	recordoldspritepos();

	movefta_d();			//ST 2
	moveweapons_d();		//ST 4
	movetransports_d();		//ST 9
	moveplayers();			//ST 10
	movefallers_d();		//ST 12
	moveexplosions_d();		//ST 5

	actortime.Reset();
	actortime.Clock();
	moveactors_d();			//ST 1
	actortime.Unclock();

	moveeffectors_d();		//ST 3
	movestandables_d();		//ST 6
	doanimations();
	movefx();				//ST 11

	thinktime.Unclock();
}


END_DUKE_NS
