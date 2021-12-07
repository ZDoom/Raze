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
		int length = arc.ArraySize() / 2;
		int key;
		FireProj value;

		for (int i = 0; i < length; i++)
		{
			arc(nullptr, key);
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

bool ceilingspace_d(sectortype* sectp)
{
	if (sectp && (sectp->ceilingstat&1) && sectp->ceilingpal == 0)
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

bool floorspace_d(sectortype* sectp)
{
	if (sectp && (sectp->floorstat&1) && sectp->ceilingpal == 0)
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

void check_fta_sounds_d(DDukeActor* actor)
{
	if (actor->s->extra > 0) switch (actor->s->picnum)
	{
	case LIZTROOPONTOILET:
	case LIZTROOPJUSTSIT:
	case LIZTROOPSHOOT:
	case LIZTROOPJETPACK:
	case LIZTROOPDUCKING:
	case LIZTROOPRUNNING:
	case LIZTROOP:
		S_PlayActorSound(PRED_RECOG, actor);
		break;
	case LIZMAN:
	case LIZMANSPITTING:
	case LIZMANFEEDING:
	case LIZMANJUMP:
		S_PlayActorSound(CAPT_RECOG, actor);
		break;
	case PIGCOP:
	case PIGCOPDIVE:
		S_PlayActorSound(PIG_RECOG, actor);
		break;
	case RECON:
		S_PlayActorSound(RECO_RECOG, actor);
		break;
	case DRONE:
		S_PlayActorSound(DRON_RECOG, actor);
		break;
	case COMMANDER:
	case COMMANDERSTAYPUT:
		S_PlayActorSound(COMM_RECOG, actor);
		break;
	case ORGANTIC:
		S_PlayActorSound(TURR_RECOG, actor);
		break;
	case OCTABRAIN:
	case OCTABRAINSTAYPUT:
		S_PlayActorSound(OCTA_RECOG, actor);
		break;
	case BOSS1:
		S_PlaySound(BOS1_RECOG);
		break;
	case BOSS2:
		if (actor->s->pal == 1)
			S_PlaySound(BOS2_RECOG);
		else S_PlaySound(WHIPYOURASS);
		break;
	case BOSS3:
		if (actor->s->pal == 1)
			S_PlaySound(BOS3_RECOG);
		else S_PlaySound(RIPHEADNECK);
		break;
	case BOSS4:
	case BOSS4STAYPUT:
		if (actor->s->pal == 1)
			S_PlaySound(BOS4_RECOG);
		S_PlaySound(BOSS4_FIRSTSEE);
		break;
	case GREENSLIME:
		S_PlayActorSound(SLIM_RECOG, actor);
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
		p->gotweapon[weapon] = true;
		if (weapon == SHRINKER_WEAPON)
			p->gotweapon[GROW_WEAPON] = true;
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
		snum = p->GetPlayerNum();

		SetGameVarID(g_iWeaponVarID,weapon, snum, p->GetActor());
		if (p->curr_weapon >= 0)
		{
			SetGameVarID(g_iWorksLikeVarID, aplWeaponWorksLike[weapon][snum], snum, p->GetActor());
		}
		else
		{
			SetGameVarID(g_iWorksLikeVarID, -1, snum, p->GetActor());
		}
		SetGameVarID(g_iReturnVarID, 0, snum, -1);
		OnEvent(EVENT_CHANGEWEAPON, snum, p->GetActor(), -1);
		if (GetGameVarID(g_iReturnVarID, nullptr, snum) == 0)
		{
			p->curr_weapon = weapon;
		}
	}
#else
	p->curr_weapon = weapon;
#endif
	p->wantweaponfire = -1;

	switch (weapon)
	{
	case KNEE_WEAPON:
	case TRIPBOMB_WEAPON:
	case HANDREMOTE_WEAPON:
	case HANDBOMB_WEAPON:	 
		break;
	case SHOTGUN_WEAPON:	  
		S_PlayActorSound(SHOTGUN_COCK, p->GetActor()); 
		break;
	case PISTOL_WEAPON:	   
		S_PlayActorSound(INSERT_CLIP, p->GetActor());
		break;
	default:	  
		S_PlayActorSound(SELECT_WEAPON, p->GetActor());
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool ifsquished(DDukeActor* actor, int p)
{
	if (isRR()) return false;	// this function is a no-op in RR's source.

	auto spri = actor->s;
	bool squishme = false;
	if (spri->picnum == APLAYER && ud.clipping)
		return false;

	auto sectp = spri->sector();
	int floorceildist = sectp->floorz - sectp->ceilingz;

	if (sectp->lotag != ST_23_SWINGING_DOOR)
	{
		if (spri->pal == 1)
			squishme = floorceildist < (32 << 8) && (sectp->lotag & 32768) == 0;
		else
			squishme = floorceildist < (12 << 8);
	}

	if (squishme)
	{
		FTA(QUOTE_SQUISHED, &ps[p]);

		if (badguy(actor))
			spri->xvel = 0;

		if (spri->pal == 1)
		{
			actor->picnum = SHOTSPARK1;
			actor->extra = 1;
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
	static const uint8_t statlist[] = { STAT_DEFAULT, STAT_ACTOR, STAT_STANDABLE, STAT_PLAYER, STAT_FALLER, STAT_ZOMBIEACTOR, STAT_MISC };

	auto spri = actor->s;
	
	if(spri->picnum != SHRINKSPARK && !(spri->picnum == RPG && spri->xrepeat < 11))
	{
		BFSSearch search(numsectors, spri->sectnum);
	
		for(unsigned dasect; (dasect = search.GetNext()) != BFSSearch::EOL;)
		{
			auto dasectp = &sector[dasect];
			if (((dasectp->ceilingz - spri->z) >> 8) < r)
			{
				auto wal = dasectp->firstWall();
				int d = abs(wal->x - spri->x) + abs(wal->y - spri->y);
				if (d < r)
					fi.checkhitceiling(dasectp);
				else
				{
					auto thirdpoint = wal->point2Wall()->point2Wall();
					d = abs(thirdpoint->x - spri->x) + abs(thirdpoint->y - spri->y);
					if (d < r)
						fi.checkhitceiling(dasectp);
				}
			}

			for (auto& wal : wallsofsector(dasectp))
			{
				if ((abs(wal.x - spri->x) + abs(wal.y - spri->y)) < r)
				{
					int nextsect = wal.nextsector;
					if (nextsect >= 0)
					{
						search.Add(nextsect);
					}
					int x1 = (((wal.x + wal.point2Wall()->x) >> 1) + spri->x) >> 1;
					int y1 = (((wal.y + wal.point2Wall()->y) >> 1) + spri->y) >> 1;
					int sect;
					updatesector(x1, y1, &sect);
					if (sect >= 0 && cansee(x1, y1, spri->z, sect, spri->x, spri->y, spri->z, spri->sectnum))
						fi.checkhitwall(actor, &wal, wal.x, wal.y, spri->z, spri->picnum);
				}
			}
		}
	}
	
	int q = -(16 << 8) + (krand() & ((32 << 8) - 1));
	
	auto Owner = actor->GetOwner();
	for (int x = 0; x < 7; x++)
	{
		DukeStatIterator itj(statlist[x]);
		while (auto act2 = itj.Next())
		{
			auto spri2 = act2->s;
			if (isWorldTour() && Owner)
			{
				if (Owner->s->picnum == APLAYER && spri2->picnum == APLAYER && ud.coop != 0 && ud.ffire == 0 && Owner != act2)
				{
					continue;
				}
				
				if (spri->picnum == FLAMETHROWERFLAME && ((Owner->s->picnum == FIREFLY && spri2->picnum == FIREFLY) || (Owner->s->picnum == BOSS5 && spri2->picnum == BOSS5)))
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
						fi.checkhitsprite(act2, actor);
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
				
				if (spri2->picnum == APLAYER) spri2->z -= gs.playerheight;
				int d = dist(actor, act2);
				if (spri2->picnum == APLAYER) spri2->z += gs.playerheight;
				
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
						else if (spri->picnum != FIREBALL || !Owner || Owner->s->picnum != APLAYER)
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
							fi.checkhitsprite(act2, actor);
					}
					else if (spri->extra == 0) act2->extra = 0;
					
					if (spri2->picnum != RADIUSEXPLOSION && Owner && Owner->s->statnum < MAXSTATUS)
					{
						if (spri2->picnum == APLAYER)
						{
							int p = spri2->yvel;
							
							if (isWorldTour() && act2->picnum == FLAMETHROWERFLAME && Owner->s->picnum == APLAYER)
							{
								ps[p].numloogs = -1 - spri->yvel;
							}
							
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


int movesprite_ex_d(DDukeActor* actor, int xchange, int ychange, int zchange, unsigned int cliptype, Collision &result)
{
	int clipdist;
	int dasectnum;

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
			if (spri->picnum == LIZMAN)
				clipdist = 292;
			else if (actorflag(actor, SFLAG_BADGUY))
				clipdist = spri->clipdist << 2;
			else
				clipdist = 192;

			clipmove_ex(&pos, &dasectnum, ((xchange * TICSPERFRAME) << 11), ((ychange * TICSPERFRAME) << 11), clipdist, (4 << 8), (4 << 8), cliptype, result);
		}

		// conditional code from hell...
		if (dasectnum < 0 || (dasectnum >= 0 &&
			((actor->actorstayput != nullptr && actor->actorstayput != dasectp) ||
			 ((spri->picnum == BOSS2) && spri->pal == 0 && dasectp->lotag != 3) ||
			 ((spri->picnum == BOSS1 || spri->picnum == BOSS2) && dasectp->lotag == ST_1_ABOVE_WATER) ||
			 (dasectp->lotag == ST_1_ABOVE_WATER && (spri->picnum == LIZMAN || (spri->picnum == LIZTROOP && spri->zvel == 0)))
			))
		 )
		{
			if (dasectp->lotag == ST_1_ABOVE_WATER && spri->picnum == LIZMAN)
				spri->ang = (krand()&2047);
			else if ((actor->temp_data[0]&3) == 1 && spri->picnum != COMMANDER)
				spri->ang = (krand()&2047);
			setsprite(actor,spri->pos);
			if (dasectnum < 0) dasectnum = 0;
			return result.setSector(dasectnum);
		}
		if ((result.type == kHitWall || result.type == kHitSprite) && (actor->cgg == 0)) spri->ang += 768;
	}
	else
	{
		if (spri->statnum == STAT_PROJECTILE)
			clipmove_ex(&pos, &dasectnum, ((xchange * TICSPERFRAME) << 11), ((ychange * TICSPERFRAME) << 11), 8, (4 << 8), (4 << 8), cliptype, result);
		else
			clipmove_ex(&pos, &dasectnum, ((xchange * TICSPERFRAME) << 11), ((ychange * TICSPERFRAME) << 11), (int)(spri->clipdist << 2), (4 << 8), (4 << 8), cliptype, result);
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

void lotsofmoney_d(DDukeActor *actor, int n)
{
	lotsofstuff(actor, n, MONEY);
}

void lotsofmail_d(DDukeActor *actor, int n)
{
	lotsofstuff(actor, n, MAIL);
}

void lotsofpaper_d(DDukeActor *actor, int n)
{
	lotsofstuff(actor, n, PAPER);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void guts_d(DDukeActor* actor, int gtype, int n, int p)
{
	auto s = actor->s;
	int gutz, floorz;
	int j;
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
		auto spawned = EGS(s->sector(), s->x + (r5 & 255) - 128, s->y + (r4 & 255) - 128, gutz - (r3 & 8191), gtype, -32, sx, sy, a, 48 + (r2 & 31), -512 - (r1 & 2047), ps[p].GetActor(), 5);
		if (spawned)
		{
			if (spawned->s->picnum == JIBS2)
			{
				spawned->s->xrepeat >>= 2;
				spawned->s->yrepeat >>= 2;
			}
			if (pal != 0)
				spawned->s->pal = pal;
		}
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
	int p;
	int psect, ssect;
	int j;

	DukeStatIterator iti(STAT_ZOMBIEACTOR);

	while (auto act = iti.Next())
	{
		auto s = act->s;
		p = findplayer(act, &x);

		ssect = psect = s->sectnum;

		auto pa = ps[p].GetActor();
		if (pa->s->extra > 0)
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
							if (s->sector()->ceilingstat&1)
								s->shade = s->sector()->ceilingshade;
							else s->shade = s->sector()->floorshade;

							act->timetosleep = 0;
							changeactorstat(act, STAT_STANDABLE);
							break;

						default:
							act->timetosleep = 0;
							check_fta_sounds_d(act);
							changeactorstat(act, STAT_ACTOR);
							break;
					}
					else act->timetosleep = 0;
				}
			}
			if (badguy(act))
			{
				if (s->sector()->ceilingstat & 1)
					s->shade = s->sector()->ceilingshade;
				else s->shade = s->sector()->floorshade;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

DDukeActor* ifhitsectors_d(int sectnum)
{
	DukeStatIterator it(STAT_MISC);
	while (auto a1 = it.Next())
	{
		if (a1->s->picnum == EXPLOSION2 && sectnum == a1->s->sectnum)
			return a1;
	}
	return nullptr;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int ifhitbyweapon_d(DDukeActor *actor)
{
	int p;
	auto spri = actor->s;
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

				switch(actor->picnum)
				{
					case RADIUSEXPLOSION:
					case RPG:
					case HYDRENT:
					case HEAVYHBOMB:
					case SEENINE:
					case OOZFILTER:
					case EXPLODINGBARREL:
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
					if (actor->picnum == SHRINKSPARK && spri->xrepeat < 24)
						return -1;

				if (isWorldTour() && actor->picnum == FIREFLY && spri->xrepeat < 48)
				{
					if (actor->picnum != RADIUSEXPLOSION && actor->picnum != RPG)
						return -1;
				}

				spri->extra -= actor->extra;
				auto Owner = actor->GetOwner();
				if (spri->picnum != RECON && Owner && Owner->s->statnum < MAXSTATUS)
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

		if (hitowner->s->picnum == APLAYER && hitowner != ps[p].GetActor())
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
	int j, x;

	DukeStatIterator iti(STAT_FALLER);
	while (auto act = iti.Next())
	{
		auto s = act->s;
		auto sectp = s->sector();

		if (act->temp_data[0] == 0)
		{
			s->z -= (16 << 8);
			act->temp_data[1] = s->ang;
			x = s->extra;
			j = fi.ifhitbyweapon(act);
			if (j >= 0)
			{
				if (j == FIREEXT || j == RPG || j == RADIUSEXPLOSION || j == SEENINE || j == OOZFILTER)
				{
					if (s->extra <= 0)
					{
						act->temp_data[0] = 1;
						DukeStatIterator itj(STAT_FALLER);
						while (auto a2 = itj.Next())
						{
							if (a2->s->hitag == s->hitag)
							{
								a2->temp_data[0] = 1;
								a2->s->cstat &= (65535 - 64);
								if (a2->s->picnum == CEILINGSTEAM || a2->s->picnum == STEAM)
									a2->s->cstat |= 32768;
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
					ssp(act, CLIPMASK0);
				}

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
					j = 1 + (krand() & 7);
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
// Duke only
//
//---------------------------------------------------------------------------

static void movetripbomb(DDukeActor *actor)
{
	auto s = actor->s;
	int j, x;
	int lTripBombControl = GetGameVar("TRIPBOMB_CONTROL", TRIPBOMB_TRIPWIRE, nullptr, -1);
	if (lTripBombControl & TRIPBOMB_TIMER)
	{
		// we're on a timer....
		if (s->extra >= 0)
		{
			s->extra--;
			if (s->extra == 0)
			{
				actor->temp_data[2] = 16;
				S_PlayActorSound(LASERTRIP_ARMING, actor);
			}
		}
	}
	if (actor->temp_data[2] > 0)
	{
		actor->temp_data[2]--;
		if (actor->temp_data[2] == 8)
		{
			S_PlayActorSound(LASERTRIP_EXPLODE, actor);
			for (j = 0; j < 5; j++) RANDOMSCRAP(actor);
			x = s->extra;
			fi.hitradius(actor, gs.tripbombblastradius, x >> 2, x >> 1, x - (x >> 2), x);

			auto spawned = spawn(actor, EXPLOSION2);
			if (spawned)
			{
				spawned->s->ang = s->ang;
				spawned->s->xvel = 348;
				ssp(spawned, CLIPMASK0);
			}

			DukeStatIterator it(STAT_MISC);
			while (auto a1 = it.Next())
			{
				if (a1->s->picnum == LASERLINE && s->hitag == a1->s->hitag)
					a1->s->xrepeat = a1->s->yrepeat = 0;
			}
			deletesprite(actor);
		}
		return;
	}
	else
	{
		x = s->extra;
		s->extra = 1;
		int16_t l = s->ang;
		j = fi.ifhitbyweapon(actor);
		if (j >= 0)
		{ 
			actor->temp_data[2] = 16; 
		}
		s->extra = x;
		s->ang = l;
	}

	if (actor->temp_data[0] < 32)
	{
		findplayer(actor, &x);
		if (x > 768) actor->temp_data[0]++;
		else if (actor->temp_data[0] > 16) actor->temp_data[0]++;
	}
	if (actor->temp_data[0] == 32)
	{
		int16_t l = s->ang;
		s->ang = actor->temp_data[5];

		actor->temp_data[3] = s->x; actor->temp_data[4] = s->y;
		s->x += bcos(actor->temp_data[5], -9);
		s->y += bsin(actor->temp_data[5], -9);
		s->z -= (3 << 8);

		// Laser fix from EDuke32.
		int const oldSectNum = s->sectnum;
		int       curSectNum = s->sectnum;

		updatesectorneighbor(s->x, s->y, &curSectNum, 2048);
		changeactorsect(actor, curSectNum);

		DDukeActor* hit;
		x = hitasprite(actor, &hit);

		actor->lastvx = x;

		s->ang = l;

		if (lTripBombControl & TRIPBOMB_TRIPWIRE)
		{
			// we're on a trip wire
			while (x > 0)
			{
				auto spawned = spawn(actor, LASERLINE);
				if (spawned)
				{
					setsprite(spawned, spawned->s->pos);
					spawned->s->hitag = s->hitag;
					spawned->temp_data[1] = spawned->s->z;

					if (x < 1024)
					{
						spawned->s->xrepeat = x >> 5;
						break;
					}
					x -= 1024;

					s->x += bcos(actor->temp_data[5], -4);
					s->y += bsin(actor->temp_data[5], -4);
					updatesectorneighbor(s->x, s->y, &curSectNum, 2048);

					if (curSectNum == -1)
						break;

					changeactorsect(actor, curSectNum);

					// this is a hack to work around the LASERLINE sprite's art tile offset
					changeactorsect(spawned, curSectNum);
				}
			}
		}

		actor->temp_data[0]++;
		s->x = actor->temp_data[3]; s->y = actor->temp_data[4];
		s->z += (3 << 8);
		changeactorsect(actor, oldSectNum);
		actor->temp_data[3] = 0;
		if (hit && lTripBombControl & TRIPBOMB_TRIPWIRE)
		{
			actor->temp_data[2] = 13;
			S_PlayActorSound(LASERTRIP_ARMING, actor);
		}
		else actor->temp_data[2] = 0;
	}
	if (actor->temp_data[0] == 33)
	{
		actor->temp_data[1]++;


		actor->temp_data[3] = s->x; actor->temp_data[4] = s->y;
		s->x += bcos(actor->temp_data[5], -9);
		s->y += bsin(actor->temp_data[5], -9);
		s->z -= (3 << 8);
		setsprite(actor, s->pos);

		x = hitasprite(actor, nullptr);

		s->x = actor->temp_data[3]; s->y = actor->temp_data[4];
		s->z += (3 << 8);
		setsprite(actor, s->x, s->y, s->z);

		if (actor->lastvx != x && lTripBombControl & TRIPBOMB_TRIPWIRE)
		{
			actor->temp_data[2] = 13;
			S_PlayActorSound(LASERTRIP_ARMING, actor);
		}
	}
}

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
		if (j == FIREEXT || j == RPG || j == RADIUSEXPLOSION || j == SEENINE || j == OOZFILTER)
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
// Duke only
//
//---------------------------------------------------------------------------

static void movefireext(DDukeActor* actor)
{
	int j = fi.ifhitbyweapon(actor);
	if (j == -1) return;

	for (int k = 0; k < 16; k++)
	{
		auto spawned = EGS(actor->s->sector(), actor->s->x, actor->s->y, actor->s->z - (krand() % (48 << 8)), SCRAP3 + (krand() & 3), -8, 48, 48, krand() & 2047, (krand() & 63) + 64, -(krand() & 4095) - (actor->s->zvel >> 2), actor, 5);
		if(spawned) spawned->s->pal = 2;
	}

	spawn(actor, EXPLOSION2);
	S_PlayActorSound(PIPEBOMB_EXPLODE, actor);
	S_PlayActorSound(GLASS_HEAVYBREAK, actor);

	if (actor->s->hitag > 0)
	{
		DukeStatIterator it(STAT_STANDABLE);
		while (auto a1 = it.Next())
		{
			if (actor->s->hitag == a1->s->hitag && (a1->s->picnum == OOZFILTER || a1->s->picnum == SEENINE))
				if (a1->s->shade != -32)
					a1->s->shade = -32;
		}

		int x = actor->s->extra;
		spawn(actor, EXPLOSION2);
		fi.hitradius(actor, gs.pipebombblastradius, x >> 2, x - (x >> 1), x - (x >> 2), x);
		S_PlayActorSound(PIPEBOMB_EXPLODE, actor);
		detonate(actor, EXPLOSION2);
	}
	else
	{
		fi.hitradius(actor, gs.seenineblastradius, 10, 15, 20, 25);
		deletesprite(actor);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void moveviewscreen(DDukeActor* actor)
{
	const int VIEWSCR_DIST = 8192;	// was originally 2048, was increased to this by EDuke32 and RedNukem.
	if (actor->s->xrepeat == 0) deletesprite(actor);
	else
	{
		int x;
		int p = findplayer(actor, &x);

		x = dist(actor, ps[p].GetActor()); // the result from findplayer is not really useful.
		if (x >= VIEWSCR_DIST && camsprite == actor)
		{
			camsprite = nullptr;
			actor->s->yvel = 0;
			actor->temp_data[0] = 0;
		}
	}
}

//---------------------------------------------------------------------------
//
// Duke only
//
//---------------------------------------------------------------------------

static void movesidebolt(DDukeActor* actor)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	int x;
	auto sectp = s->sector();

	findplayer(actor, &x);
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

	if ((krand() & 1) && sectp->floorpicnum == HURTRAIL)
		S_PlayActorSound(SHORT_CIRCUIT, actor);

	if (s->picnum == SIDEBOLT1 + 4) s->picnum = SIDEBOLT1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void movebolt(DDukeActor *actor)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	int x;
	auto sectp = s->sector();

	findplayer(actor, &x);
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

	if (s->picnum == (BOLT1+1) && (krand()&7) == 0 && sectp->floorpicnum == HURTRAIL)
		S_PlayActorSound(SHORT_CIRCUIT,actor);

	if (s->picnum==BOLT1+4) s->picnum=BOLT1;

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

void movestandables_d(void)
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

		else if (picnum == TRIPBOMB)
		{
			movetripbomb(act);
		}

		else if (picnum >= CRACK1 && picnum <= CRACK1 + 3)
		{
			movecrack(act);
		}

		else if (picnum == FIREEXT)
		{
			movefireext(act);
		}

		else if (picnum == OOZFILTER || picnum == SEENINE || picnum == SEENINEDEAD || picnum == (SEENINEDEAD + 1))
		{
			moveooz(act, SEENINE, SEENINEDEAD, OOZFILTER, EXPLOSION2);
		}

		else if (picnum == MASTERSWITCH)
		{
			movemasterswitch(act, SEENINE, OOZFILTER);
		}

		else if (picnum == VIEWSCREEN || picnum == VIEWSCREEN2)
		{
			moveviewscreen(act);
		}

		else if (picnum == TRASH)
		{
			movetrash(act);
		}

		else if (picnum >= SIDEBOLT1 && picnum <= SIDEBOLT1 + 3)
		{
			movesidebolt(act);
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

		else if (isIn(picnum, CANWITHSOMETHING, CANWITHSOMETHING2, CANWITHSOMETHING3, CANWITHSOMETHING4))
		{
			movecanwithsomething(act);
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

static bool movefireball(DDukeActor* actor)
{
	auto s = actor->s;
	auto Owner = actor->GetOwner();

	if (s->sector()->lotag == 2)
	{
		deletesprite(actor);
		return true;
	}

	if (!Owner || Owner->s->picnum != FIREBALL)
	{
		if (actor->temp_data[0] >= 1 && actor->temp_data[0] < 6)
		{
			float siz = 1.0f - (actor->temp_data[0] * 0.2f);
			// This still needs work- it stores an actor reference in a general purpose integer field.
			int trail = actor->temp_data[1];
			auto ball = spawn(actor, FIREBALL);
			if (ball)
			{
				auto spr = ball->s;
				actor->temp_data[1] = ball->GetSpriteIndex();

				spr->xvel = s->xvel;
				spr->yvel = s->yvel;
				spr->zvel = s->zvel;
				if (actor->temp_data[0] > 1)
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
				spr->yrepeat = spr->xrepeat = (uint8_t)(s->xrepeat * siz);
				spr->cstat = s->cstat;
				spr->extra = 0;

				FireProj proj = { spr->x, spr->y, spr->z, spr->xvel, spr->yvel, spr->zvel };

				fire.Insert(ball->GetSpriteIndex(), proj);
				changeactorstat(ball, STAT_PROJECTILE);
			}
		}
		actor->temp_data[0]++;
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

static bool weaponhitsprite(DDukeActor* proj, DDukeActor *targ, bool fireball)
{
	auto s = proj->s;
	if (s->picnum == FREEZEBLAST && targ->s->pal == 1)
		if (badguy(targ) || targ->s->picnum == APLAYER)
		{
			auto spawned = spawn(targ, TRANSPORTERSTAR);
			if (spawned)
			{
				spawned->s->pal = 1;
				spawned->s->xrepeat = 32;
				spawned->s->yrepeat = 32;
			}

			deletesprite(proj);
			return true;
		}

	if (!isWorldTour() || s->picnum != FIREBALL || fireball)
		fi.checkhitsprite(targ, proj);

	if (targ->s->picnum == APLAYER)
	{
		int p = targ->s->yvel;
		auto Owner = proj->GetOwner();

		if (ud.multimode >= 2 && fireball && Owner && Owner->s->picnum == APLAYER)
		{
			ps[p].numloogs = -1 - s->yvel;
		}

		S_PlayActorSound(PISTOL_BODYHIT, targ);

		if (s->picnum == SPIT)
		{
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

static bool weaponhitwall(DDukeActor *proj, walltype* wal, const vec3_t &oldpos)
{
	auto s = proj->s;
	if (s->picnum != RPG && s->picnum != FREEZEBLAST && s->picnum != SPIT &&
		(!isWorldTour() || s->picnum != FIREBALL) &&
		(wal->overpicnum == MIRROR || wal->picnum == MIRROR))
	{
		auto delta = wal->delta();
		int k = getangle(delta.x, delta.y);
		s->ang = ((k << 1) - s->ang) & 2047;
		proj->SetOwner(proj);
		spawn(proj, TRANSPORTERSTAR);
		return true;
	}
	else
	{
		setsprite(proj, oldpos);
		fi.checkhitwall(proj, wal, s->x, s->y, s->z, s->picnum);

		if (s->picnum == FREEZEBLAST)
		{
			if (wal->overpicnum != MIRROR && wal->picnum != MIRROR)
			{
				s->extra >>= 1;
				s->yvel--;
			}

			auto delta = wal->delta();
			int k = getangle(delta.x, delta.y);
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

static bool weaponhitsector(DDukeActor* proj, const vec3_t& oldpos, bool fireball)
{
	auto s = proj->s;
	setsprite(proj, oldpos);

	if (s->zvel < 0)
	{
		if (s->sector()->ceilingstat & 1)
			if (s->sector()->ceilingpal == 0)
			{
				deletesprite(proj);
				return true;
			}

		fi.checkhitceiling(s->sector());
	}
	else if (fireball)
	{
		auto spawned = spawn(proj, LAVAPOOL);
		if (spawned)
		{
			spawned->SetOwner(proj);
			spawned->SetHitOwner(proj);
			spawned->s->yvel = s->yvel;
		}
		deletesprite(proj);
		return true;
	}

	if (s->picnum == FREEZEBLAST)
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

static void weaponcommon_d(DDukeActor* proj)
{
	auto s = proj->s;
	if (s->picnum == COOLEXPLOSION1)
		if (!S_CheckActorSoundPlaying(proj, WIERDSHOT_FLY))
			S_PlayActorSound(WIERDSHOT_FLY, proj);

	int k, ll;
	vec3_t oldpos = s->pos;

	if (s->picnum == RPG && s->sector()->lotag == 2)
	{
		k = s->xvel >> 1;
		ll = s->zvel >> 1;
	}
	else
	{
		k = s->xvel;
		ll = s->zvel;
	}

	getglobalz(proj);

	switch (s->picnum)
	{
	case RPG:
		if (proj->picnum != BOSS2 && s->xrepeat >= 10 && s->sector()->lotag != 2)
		{
			auto spawned = spawn(proj, SMALLSMOKE);
			if (spawned) spawned->s->z += (1 << 8);
		}
		break;

	case FIREBALL:
		if (movefireball(proj)) return;
		break;
	}

	Collision coll;
	movesprite_ex(proj,
		MulScale(k, bcos(s->ang), 14),
		MulScale(k, bsin(s->ang), 14), ll, CLIPMASK1, coll);

	if (s->picnum == RPG && proj->temp_actor != nullptr)
		if (FindDistance2D(s->x - proj->temp_actor->s->x, s->y - proj->temp_actor->s->y) < 256)
			coll.setSprite(proj->temp_actor);

	if (s->sectnum < 0)
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
			if ((s->z > proj->floorz && s->sector()->lotag != 1) ||
				(s->z > proj->floorz + (16 << 8) && s->sector()->lotag == 1))
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
			auto spawned = EGS(s->sector(),
				s->x + MulScale(k, bcos(s->ang), 9),
				s->y + MulScale(k, bsin(s->ang), 9),
				s->z + ((k * Sgn(s->zvel)) * abs(s->zvel / 24)), FIRELASER, -40 + (k << 2),
				s->xrepeat, s->yrepeat, 0, 0, 0, proj->GetOwner(), 5);
			if (spawned)
			{
				spawned->s->cstat = 128;
				spawned->s->pal = s->pal;
			}
		}
	}
	else if (s->picnum == SPIT) if (s->zvel < 6144)
		s->zvel += gs.gravity - 112;

	if (coll.type != 0)
	{
		if (s->picnum == COOLEXPLOSION1)
		{
			if (coll.type == kHitSprite && coll.actor->s->picnum != APLAYER)
			{
				return;
			}
			s->xvel = 0;
			s->zvel = 0;
		}

		bool fireball = (isWorldTour() && s->picnum == FIREBALL && (!proj->GetOwner() || proj->GetOwner()->s->picnum != FIREBALL));

		if (coll.type == kHitSprite)
		{
			if (weaponhitsprite(proj, coll.actor, fireball)) return;
		}
		else if (coll.type == kHitWall)
		{
			if (weaponhitwall(proj, coll.wall(), oldpos)) return;
		}
		else if (coll.type == kHitSector)
		{
			if (weaponhitsector(proj, oldpos, fireball)) return;
		}

		if (s->picnum != SPIT)
		{
			if (s->picnum == RPG)
			{
				// j is only needed for the hit type mask.
				rpgexplode(proj, coll.type, oldpos, EXPLOSION2, EXPLOSION2BOT, -1, RPG_EXPLODE);
			}
			else if (s->picnum == SHRINKSPARK)
			{
				spawn(proj, SHRINKEREXPLOSION);
				S_PlayActorSound(SHRINKER_HIT, proj);
				fi.hitradius(proj, gs.shrinkerblastradius, 0, 0, 0, 0);
			}
			else if (s->picnum != COOLEXPLOSION1 && s->picnum != FREEZEBLAST && s->picnum != FIRELASER && (!isWorldTour() || s->picnum != FIREBALL))
			{
				auto k = spawn(proj, EXPLOSION2);
				if (k)
				{
					k->s->xrepeat = k->s->yrepeat = s->xrepeat >> 1;
					if (coll.type == kHitSector)
					{
						if (s->zvel < 0)
						{
							k->s->cstat |= 8; k->s->z += (72 << 8);
						}
					}
				}
			}
			if (fireball)
			{
				auto spawned = spawn(proj, EXPLOSION2);
				if (spawned) spawned->s->xrepeat = spawned->s->yrepeat = (short)(s->xrepeat >> 1);
			}
		}
		if (s->picnum != COOLEXPLOSION1)
		{
			deletesprite(proj);
			return;
		}
	}
	if (s->picnum == COOLEXPLOSION1)
	{
		s->shade++;
		if (s->shade >= 40)
		{
			deletesprite(proj);
			return;
		}
	}
	else if (s->picnum == RPG && s->sector()->lotag == 2 && s->xrepeat >= 10 && rnd(140))
		spawn(proj, WATERBUBBLE);

}
//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveweapons_d(void)
{
	DukeStatIterator it(STAT_PROJECTILE);
	while (auto act = it.Next())
	{
		if (act->s->sectnum < 0)
		{
			deletesprite(act);
			continue;
		}


		switch(act->s->picnum)
		{
		case RADIUSEXPLOSION:
		case KNEE:
			deletesprite(act);
			continue;
		case TONGUE:
			movetongue(act, TONGUE, INNERJAW);
			continue;

		case FREEZEBLAST:
			if (act->s->yvel < 1 || act->s->extra < 2 || (act->s->xvel|act->s->zvel) == 0)
			{
				auto spawned = spawn(act,TRANSPORTERSTAR);
				if (spawned)
				{
					spawned->s->pal = 1;
					spawned->s->xrepeat = 32;
					spawned->s->yrepeat = 32;
				}
				deletesprite(act);
				continue;
			}
			[[fallthrough]];
		case FIREBALL:
			// Twentieth Anniversary World Tour
			if (act->s->picnum == FIREBALL && !isWorldTour()) break;
			[[fallthrough]];
		case SHRINKSPARK:
		case RPG:
		case FIRELASER:
		case SPIT:
		case COOLEXPLOSION1:
			weaponcommon_d(act);
			break;

		case SHOTSPARK1:
		{
			int x;
			int p = findplayer(act, &x);
			execute(act, p, x);
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
	int ll;
	
	DukeStatIterator iti(STAT_TRANSPORT);
	while (auto act = iti.Next())
	{
		auto spr = act->s;
		auto Owner = act->GetOwner();
		
		if (Owner == act)
		{
			continue;
		}

		auto sectp = spr->sector();
		int sectlotag = sectp->lotag;
		int onfloorz = act->temp_data[4];
		
		if (act->temp_data[0] > 0) act->temp_data[0]--;
		
		DukeSectIterator itj(spr->sectnum);
		while (auto act2 = itj.Next()) 
		{
			auto spr2 = act2->s;
			switch (spr2->statnum)
			{
			case STAT_PLAYER:
				
				if (act2->GetOwner())
				{
					int p = act2->PlayerIndex();
					
					ps[p].on_warping_sector = 1;
					
					if (ps[p].transporter_hold == 0 && ps[p].jumping_counter == 0)
					{
						if (ps[p].on_ground && sectlotag == 0 && onfloorz && ps[p].jetpack_on == 0)
						{
							if (spr->pal == 0)
							{
								spawn(act, TRANSPORTERBEAM);
								S_PlayActorSound(TELEPORTER, act);
							}
							
							for (int k = connecthead; k >= 0; k = connectpoint2[k])
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
							ps[p].oposz = ps[p].pos.z = Owner->s->z - gs.playerheight;
							
							changeactorsect(act2, Owner->s->sectnum);
							ps[p].cursectnum = spr2->sectnum;
							
							if (spr->pal == 0)
							{
								auto k = spawn(Owner, TRANSPORTERBEAM);
								if (k) S_PlayActorSound(TELEPORTER, k);
							}
							
							break;
						}
					}
					else if (!(sectlotag == 1 && ps[p].on_ground == 1)) break;
					
					if (onfloorz == 0 && abs(spr->z - ps[p].pos.z) < 6144)
						if ((ps[p].jetpack_on == 0) || (ps[p].jetpack_on && (PlayerInput(p, SB_JUMP))) ||
							(ps[p].jetpack_on && PlayerInput(p, SB_CROUCH)))
						{
							ps[p].oposx = ps[p].pos.x += Owner->s->x - spr->x;
							ps[p].oposy = ps[p].pos.y += Owner->s->y - spr->y;
							
							if (ps[p].jetpack_on && (PlayerInput(p, SB_JUMP) || ps[p].jetpack_on < 11))
								ps[p].pos.z = Owner->s->z - 6144;
							else ps[p].pos.z = Owner->s->z + 6144;
							ps[p].oposz = ps[p].pos.z;
							
							auto pa = ps[p].GetActor();
							pa->s->opos = ps[p].pos;
							
							changeactorsect(act2, Owner->s->sectnum);
							ps[p].cursectnum = Owner->s->sectnum;
							
							break;
						}
					
					int k = 0;
					
					if (onfloorz && sectlotag == ST_1_ABOVE_WATER && ps[p].on_ground && ps[p].pos.z > (sectp->floorz - (16 << 8)) && (PlayerInput(p, SB_CROUCH) || ps[p].poszv > 2048))
						// if( onfloorz && sectlotag == 1 && ps[p].pos.z > (sectp->floorz-(6<<8)) )
					{
						k = 1;
						if (screenpeek == p)
						{
							FX_StopAllSounds();
						}
						if (ps[p].GetActor()->s->extra > 0)
							S_PlayActorSound(DUKE_UNDERWATER, act2);
						ps[p].oposz = ps[p].pos.z =
						Owner->getSector()->ceilingz + (7 << 8);
						
						ps[p].posxv = 4096 - (krand() & 8192);
						ps[p].posyv = 4096 - (krand() & 8192);
						
					}
					
					if (onfloorz && sectlotag == ST_2_UNDERWATER && ps[p].pos.z < (sectp->ceilingz + (6 << 8)))
					{
						k = 1;
						//     if( spr2->extra <= 0) break;
						if (screenpeek == p)
						{
							FX_StopAllSounds();
						}
						S_PlayActorSound(DUKE_GASP, act2);
						
						ps[p].oposz = ps[p].pos.z =
						Owner->getSector()->floorz - (7 << 8);
						
						ps[p].jumping_toggle = 1;
						ps[p].jumping_counter = 0;
					}
					
					if (k == 1)
					{
						ps[p].oposx = ps[p].pos.x += Owner->s->x - spr->x;
						ps[p].oposy = ps[p].pos.y += Owner->s->y - spr->y;
						
						if (!Owner || Owner->GetOwner() != Owner)
							ps[p].transporter_hold = -2;
						ps[p].cursectnum = Owner->s->sectnum;
						
						changeactorsect(act2, Owner->s->sectnum);
						setsprite(ps[p].GetActor(), ps[p].pos.x, ps[p].pos.y, ps[p].pos.z + gs.playerheight);
						
						if ((krand() & 255) < 32)
							spawn(act2, WATERSPLASH2);
						
						if (sectlotag == 1)
							for (int l = 0; l < 9; l++)
						{
							auto q = spawn(ps[p].GetActor(), WATERBUBBLE);
							if (q) q->s->z += krand() & 16383;
						}
					}
				}
				break;
				
			case STAT_ACTOR:
				switch (spr2->picnum)
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
					if (spr2->extra > 0)
						continue;
				}
				[[fallthrough]];
			case STAT_PROJECTILE:
			case STAT_MISC:
			case STAT_FALLER:
			case STAT_DUMMYPLAYER:
				
				ll = abs(spr2->zvel);
				
				{
					warpspriteto = 0;
					if (ll && sectlotag == 2 && spr2->z < (sectp->ceilingz + ll))
						warpspriteto = 1;
					
					if (ll && sectlotag == 1 && spr2->z > (sectp->floorz - ll))
						warpspriteto = 1;
					
					if (sectlotag == 0 && (onfloorz || abs(spr2->z - spr->z) < 4096))
					{
						if ((!Owner || Owner->GetOwner() != Owner) && onfloorz && act->temp_data[0] > 0 && spr2->statnum != STAT_MISC)
						{
							act->temp_data[0]++;
							goto BOLT;
						}
						warpspriteto = 1;
					}
					
					if (warpspriteto) switch (spr2->picnum)
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
							spr2->cstat &= 32767;
							break;
						}
						[[fallthrough]];
					default:
						if (spr2->statnum == 5 && !(sectlotag == 1 || sectlotag == 2))
							break;
						[[fallthrough]];

					case WATERBUBBLE:
						//if( rnd(192) && a2->s.picnum == WATERBUBBLE)
						// break;
						
						if (sectlotag > 0)
						{
							auto k = spawn(act2, WATERSPLASH2);
							if (k && sectlotag == 1 && spr2->statnum == 4)
							{
								k->s->xvel = spr2->xvel >> 1;
								k->s->ang = spr2->ang;
								ssp(k, CLIPMASK0);
							}
						}
						
						switch (sectlotag)
						{
						case 0:
							if (onfloorz)
							{
								if (spr2->statnum == STAT_PROJECTILE || (checkcursectnums(spr->sectnum) == -1 && checkcursectnums(Owner->s->sectnum) == -1))
								{
									spr2->x += (Owner->s->x - spr->x);
									spr2->y += (Owner->s->y - spr->y);
									spr2->z -= spr->z - Owner->getSector()->floorz;
									spr2->ang = Owner->s->ang;
									
									spr2->backupang();
									
									if (spr->pal == 0)
									{
										auto k = spawn(act, TRANSPORTERBEAM);
										if (k) S_PlayActorSound(TELEPORTER, k);
										
										k = spawn(Owner, TRANSPORTERBEAM);
										if (k) S_PlayActorSound(TELEPORTER, k);
									}
									
									if (Owner && Owner->GetOwner() == Owner)
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
						case 1:
							spr2->x += (Owner->s->x - spr->x);
							spr2->y += (Owner->s->y - spr->y);
							spr2->z = Owner->getSector()->ceilingz + ll;
							
							spr2->backupz();
							
							changeactorsect(act2, Owner->s->sectnum);
							
							break;
						case 2:
							spr2->x += (Owner->s->x - spr->x);
							spr2->y += (Owner->s->y - spr->y);
							spr2->z = Owner->getSector()->floorz - ll;
							
							spr2->backupz();
							
							changeactorsect(act2, Owner->s->sectnum);
							
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

static void greenslime(DDukeActor *actor)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	auto sectp = s->sector();
	int j;

	// #ifndef isShareware()
	if (ud.multimode < 2)
	{
		if (actor_tog == 1)
		{
			s->cstat = 32768;
			return;
		}
		else if (actor_tog == 2) s->cstat = 257;
	}
	// #endif

	t[1] += 128;

	if (sectp->floorstat & 1)
	{
		deletesprite(actor);
		return;
	}

	int x;
	int p = findplayer(actor, &x);

	if (x > 20480)
	{
		actor->timetosleep++;
		if (actor->timetosleep > SLEEPTIME)
		{
			actor->timetosleep = 0;
			changeactorstat(actor, 2);
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
		makeitfall(actor);
		s->cstat = 257;
		s->picnum = GREENSLIME + 2;
		s->extra = 1;
		s->pal = 1;
		j = fi.ifhitbyweapon(actor);
		if (j >= 0)
		{
			if (j == FREEZEBLAST)
				return;
			for (j = 16; j >= 0; j--)
			{
				auto k = EGS(s->sector(), s->x, s->y, s->z, GLASSPIECES + (j % 3), -32, 36, 36, krand() & 2047, 32 + (krand() & 63), 1024 - (krand() & 1023), actor, 5);
				k->s->pal = 1;
			}
			ps[p].actors_killed++;
			S_PlayActorSound(GLASS_BREAKING, actor);
			deletesprite(actor);
		}
		else if (x < 1024 && ps[p].quick_kick == 0)
		{
			j = getincangle(ps[p].angle.ang.asbuild(), getangle(s->x - ps[p].pos.x, s->y - ps[p].pos.y));
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
		if (ps[p].GetActor()->s->extra < 1)
		{
			t[0] = 0;
			return;
		}

		setsprite(actor, s->pos);

		s->ang = ps[p].angle.ang.asbuild();

		if ((PlayerInput(p, SB_FIRE) || (ps[p].quick_kick > 0)) && ps[p].GetActor()->s->extra > 0)
			if (ps[p].quick_kick > 0 || (ps[p].curr_weapon != HANDREMOTE_WEAPON && ps[p].curr_weapon != HANDBOMB_WEAPON && ps[p].curr_weapon != TRIPBOMB_WEAPON && ps[p].ammo_amount[ps[p].curr_weapon] >= 0))
			{
				for (x = 0; x < 8; x++)
				{
					auto j = EGS(s->sector(), s->x, s->y, s->z - (8 << 8), SCRAP3 + (krand() & 3), -8, 48, 48, krand() & 2047, (krand() & 63) + 64, -(krand() & 4095) - (s->zvel >> 2), actor, 5);
					j->s->pal = 6;
				}

				S_PlayActorSound(SLIM_DYING, actor);
				S_PlayActorSound(SQUISHED, actor);
				if ((krand() & 255) < 32)
				{
					auto j = spawn(actor, BLOODPOOL);
					if (j) j->s->pal = 0;
				}
				ps[p].actors_killed++;
				t[0] = -3;
				if (ps[p].somethingonplayer == actor)
					ps[p].somethingonplayer = nullptr;
				deletesprite(actor);
				return;
			}

		s->z = ps[p].pos.z + ps[p].pyoff - t[2] + (8 << 8);

		s->z += -ps[p].horizon.horiz.asq16() >> 12;

		if (t[2] > 512)
			t[2] -= 128;

		if (t[2] < 348)
			t[2] += 128;

		if (ps[p].newOwner != nullptr)
		{
			ps[p].newOwner = nullptr;
			ps[p].pos.x = ps[p].oposx;
			ps[p].pos.y = ps[p].oposy;
			ps[p].pos.z = ps[p].oposz;
			ps[p].angle.restore();

			updatesector(ps[p].pos.x, ps[p].pos.y, &ps[p].cursectnum);

			DukeStatIterator it(STAT_ACTOR);
			while (auto ac = it.Next())
			{
				if (ac->s->picnum == CAMERA1) ac->s->yvel = 0;
			}
		}

		if (t[3] > 0)
		{
			static const uint8_t frames[] = { 5,5,6,6,7,7,6,5 };

			s->picnum = GREENSLIME + frames[t[3]];

			if (t[3] == 5)
			{
				auto psp = ps[p].GetActor();
				psp->s->extra += -(5 + (krand() & 3));
				S_PlayActorSound(SLIM_ATTACK, actor);
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

		s->xrepeat = 20 + bsin(t[1], -13);
		s->yrepeat = 15 + bsin(t[1], -13);

		s->x = ps[p].pos.x + ps[p].angle.ang.bcos(-7);
		s->y = ps[p].pos.y + ps[p].angle.ang.bsin(-7);

		return;
	}

	else if (s->xvel < 64 && x < 768)
	{
		if (ps[p].somethingonplayer == nullptr)
		{
			ps[p].somethingonplayer = actor;
			if (t[0] == 3 || t[0] == 2) //Falling downward
				t[2] = (12 << 8);
			else t[2] = -(13 << 8); //Climbing up duke
			t[0] = -4;
		}
	}

	j = fi.ifhitbyweapon(actor);
	if (j >= 0)
	{
		S_PlayActorSound(SLIM_DYING, actor);

		if (ps[p].somethingonplayer == actor)
			ps[p].somethingonplayer = nullptr;

		if (j == FREEZEBLAST)
		{
			S_PlayActorSound(SOMETHINGFROZE, actor); 
			t[0] = -5; t[3] = 0;
			return;
		}
		ps[p].actors_killed++;

		if ((krand() & 255) < 32)
		{
			auto j = spawn(actor, BLOODPOOL);
			if (j) j->s->pal = 0;
		}

		for (x = 0; x < 8; x++)
		{
			auto j = EGS(s->sector(), s->x, s->y, s->z - (8 << 8), SCRAP3 + (krand() & 3), -8, 48, 48, krand() & 2047, (krand() & 63) + 64, -(krand() & 4095) - (s->zvel >> 2), actor, 5);
			if (j) j->s->pal = 6;
		}
		t[0] = -3;
		deletesprite(actor);
		return;
	}
	// All weap
	if (t[0] == -1) //Shrinking down
	{
		makeitfall(actor);

		s->cstat &= 65535 - 8;
		s->picnum = GREENSLIME + 4;

		if (s->xrepeat > 32) s->xrepeat -= krand() & 7;
		if (s->yrepeat > 16) s->yrepeat -= krand() & 7;
		else
		{
			s->xrepeat = 40;
			s->yrepeat = 16;
			actor->temp_actor = nullptr;
			t[0] = 0;
		}

		return;
	}
	else if (t[0] != -2) getglobalz(actor);

	if (t[0] == -2) //On top of somebody (an enemy)
	{
		auto s5 = actor->temp_actor;
		makeitfall(actor);
		if (s5)
		{
			s5->s->xvel = 0;

			int l = s5->s->ang;

			s->z = s5->s->z;
			s->x = s5->s->x + bcos(l, -11);
			s->y = s5->s->y + bsin(l, -11);

			s->picnum = GREENSLIME + 2 + (global_random & 1);

			if (s->yrepeat < 64) s->yrepeat += 2;
			else
			{
				if (s->xrepeat < 32) s->xrepeat += 4;
				else
				{
					t[0] = -1;
					x = ldist(actor, s5);
					if (x < 768) {
						s5->s->xrepeat = 0;
					}
				}
			}
		}
		return;
	}

	//Check randomly to see of there is an actor near
	if (rnd(32))
	{
		DukeSectIterator it(s->sectnum);
		while (auto a2 = it.Next())
		{
			if (gs.actorinfo[a2->s->picnum].flags & SFLAG_GREENSLIMEFOOD)
			{
				if (ldist(actor, a2) < 768 && (abs(s->z - a2->s->z) < 8192)) //Gulp them
				{
					actor->temp_actor = a2;
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
			S_PlayActorSound(SLIM_ROAM, actor);

		if (t[0] == 2)
		{
			s->zvel = 0;
			s->cstat &= (65535 - 8);

			if ((sectp->ceilingstat & 1) || (actor->ceilingz + 6144) < s->z)
			{
				s->z += 2048;
				t[0] = 3;
				return;
			}
		}
		else
		{
			s->cstat |= 8;
			makeitfall(actor);
		}

		if (everyothertime & 1) ssp(actor, CLIPMASK0);

		if (s->xvel > 96)
		{
			s->xvel -= 2;
			return;
		}
		else
		{
			if (s->xvel < 32) s->xvel += 4;
			s->xvel = 64 - bcos(t[1], -9);

			s->ang += getincangle(s->ang,
				getangle(ps[p].pos.x - s->x, ps[p].pos.y - s->y)) >> 3;
			// TJR
		}

		s->xrepeat = 36 + bcos(t[1], -11);
		s->yrepeat = 16 + bsin(t[1], -13);

		if (rnd(4) && (sectp->ceilingstat & 1) == 0 &&
			abs(actor->floorz - actor->ceilingz)
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
		if (s->z < actor->ceilingz + 4096)
		{
			s->z = actor->ceilingz + 4096;
			s->xvel = 0;
			t[0] = 2;
		}
	}

	if (t[0] == 3)
	{
		s->picnum = GREENSLIME + 1;

		makeitfall(actor);

		if (s->z > actor->floorz - (8 << 8))
		{
			s->yrepeat -= 4;
			s->xrepeat += 2;
		}
		else
		{
			if (s->yrepeat < (40 - 4)) s->yrepeat += 8;
			if (s->xrepeat > 8) s->xrepeat -= 4;
		}

		if (s->z > actor->floorz - 2048)
		{
			s->z = actor->floorz - 2048;
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

static void flamethrowerflame(DDukeActor *actor)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];
	auto sectp = s->sector();
	int x;
	int p = findplayer(actor, &x);
	execute(actor, p, x);
	t[0]++;
	if (sectp->lotag == 2)
	{
		spawn(actor, EXPLOSION2)->s->shade = 127;
		deletesprite(actor);
		return;
	}

	int dax = s->x;
	int day = s->y;
	int daz = s->z;
	int xvel = s->xvel;

	getglobalz(actor);

	int ds = t[0] / 6;
	if (s->xrepeat < 80)
		s->yrepeat = s->xrepeat += ds;
	s->clipdist += ds;
	if (t[0] <= 2)
		t[3] = krand() % 10;
	if (t[0] > 30) 
	{
		spawn(actor, EXPLOSION2)->s->shade = 127;
		deletesprite(actor);
		return;
	}

	Collision coll;
	movesprite_ex(actor, MulScale(xvel, bcos(s->ang), 14),
		MulScale(xvel, bsin(s->ang), 14), s->zvel, CLIPMASK1, coll);

	if (s->sectnum < 0)
	{
		deletesprite(actor);
		return;
	}

	if (coll.type != kHitSprite)
	{
		if (s->z < actor->ceilingz)
		{
			coll.setSector(s->sectnum);
			s->zvel = -1;
		}
		else if ((s->z > actor->floorz && s->sector()->lotag != 1)
			|| (s->z > actor->floorz + (16 << 8) && s->sector()->lotag == 1))
		{
			coll.setSector(s->sectnum);
			if (s->sector()->lotag != 1)
				s->zvel = 1;
		}
	}

	if (coll.type != 0) {
		s->xvel = s->yvel = s->zvel = 0;
		if (coll.type == kHitSprite)
		{
			fi.checkhitsprite(coll.actor, actor);
			if (coll.actor->s->picnum == APLAYER)
				S_PlayActorSound(PISTOL_BODYHIT, coll.actor);
		}
		else if (coll.type == kHitWall)
		{
			setsprite(actor, dax, day, daz);
			fi.checkhitwall(actor, coll.wall(), s->x, s->y, s->z, s->picnum);
		}
		else if (coll.type == kHitSector)
		{
			setsprite(actor, dax, day, daz);
			if (s->zvel < 0)
				fi.checkhitceiling(s->sector());
		}

		if (s->xrepeat >= 10)
		{
			x = s->extra;
			fi.hitradius(actor, gs.rpgblastradius, x >> 2, x >> 1, x - (x >> 2), x);
		}
		else
		{
			x = s->extra + (global_random & 3);
			fi.hitradius(actor, (gs.rpgblastradius >> 1), x >> 2, x >> 1, x - (x >> 2), x);
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
	auto Owner = actor->GetOwner();
	auto sectp = s->sector();
	int x, l;

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

	if (s->picnum != BOUNCEMINE)
	{
		makeitfall(actor);

		if (sectp->lotag != 1 && s->z >= actor->floorz - (FOURSLEIGHT) && s->yvel < 3)
		{
			if (s->yvel > 0 || (s->yvel == 0 && actor->floorz == sectp->floorz))
				S_PlayActorSound(PIPEBOMB_BOUNCE, actor);
			s->zvel = -((4 - s->yvel) << 8);
			if (s->sector()->lotag == 2)
				s->zvel >>= 2;
			s->yvel++;
		}
		if (s->z < actor->ceilingz) // && sectp->lotag != 2 )
		{
			s->z = actor->ceilingz + (3 << 8);
			s->zvel = 0;
		}
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
		}
	}
	else t[5] = 0;

	if (t[3] == 0 && (s->picnum == BOUNCEMINE || s->picnum == MORTER) && (coll.type || x < 844))
	{
		t[3] = 1;
		t[4] = 0;
		l = 0;
		s->xvel = 0;
		goto DETONATEB;
	}

	if ( Owner && Owner->s->picnum == APLAYER)
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

	if (coll.type== kHitWall)
	{
		auto wal = coll.wall();
		fi.checkhitwall(actor, wal, s->x, s->y, s->z, s->picnum);

		auto delta = wal->delta();
		int k = getangle(delta.x, delta.y);

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
			case HEAVYHBOMB: m = gs.pipebombblastradius; break;
			case MORTER: m = gs.morterblastradius; break;
			case BOUNCEMINE: m = gs.bouncemineblastradius; break;
			}

			fi.hitradius(actor, m, x >> 2, x >> 1, x - (x >> 2), x);
			spawn(actor, EXPLOSION2);
			if (s->zvel == 0)	spawn(actor, EXPLOSION2BOT);
			S_PlayActorSound(PIPEBOMB_EXPLODE, actor);
			for (x = 0; x < 8; x++)
				RANDOMSCRAP(actor);
		}

		if (s->yrepeat)
		{
			s->yrepeat = 0;
			return;
		}

		if (t[4] > 20)
		{
			if (Owner != actor || ud.respawn_items == 0)
			{
				deletesprite(actor);
				return;
			}
			else
			{
				t[2] = gs.respawnitemtime;
				spawn(actor, RESPAWNMARKERRED);
				s->cstat = 32768;
				s->yrepeat = 9;
				return;
			}
		}
	}
	else if (s->picnum == HEAVYHBOMB && x < 788 && t[0] > 7 && s->xvel == 0)
		if (cansee(s->x, s->y, s->z - (8 << 8), s->sectnum, ps[p].pos.x, ps[p].pos.y, ps[p].pos.z, ps[p].cursectnum))
			if (ps[p].ammo_amount[HANDBOMB_WEAPON] < gs.max_ammo_amount[HANDBOMB_WEAPON])
			{
				if (ud.coop >= 1 && Owner == actor)
				{
					for (int j = 0; j < ps[p].weapreccnt; j++)
						if (ps[p].weaprecs[j] == s->picnum)
							continue;

					if (ps[p].weapreccnt < 255) // DukeGDX has 16 here.
						ps[p].weaprecs[ps[p].weapreccnt++] = s->picnum;
				}

				addammo(HANDBOMB_WEAPON, &ps[p], 1);
				S_PlayActorSound(DUKE_GET, ps[p].GetActor());

				if (ps[p].gotweapon[HANDBOMB_WEAPON] == 0 || Owner == ps[p].GetActor())
					fi.addweapon(&ps[p], HANDBOMB_WEAPON);

				if (!Owner || Owner->s->picnum != APLAYER)
				{
					SetPlayerPal(&ps[p], PalEntry(32, 0, 32, 0));
				}

				if (Owner != actor || ud.respawn_items == 0)
				{
					if (Owner == actor && ud.coop >= 1)
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

void moveactors_d(void)
{
	int x;
	int sect, p;
	unsigned int k;
	
	DukeStatIterator it(STAT_ACTOR);
	while (auto act = it.Next())
	{
		auto s = act->s;
		sect = s->sectnum;

		if (s->xrepeat == 0 || sect < 0 || sect >= MAXSECTORS)
		{ 
			deletesprite(act);
			continue;
		}

		int *t = &act->temp_data[0];


		switch (s->picnum)
		{
		case FLAMETHROWERFLAME:
			if (isWorldTour()) flamethrowerflame(act);
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
				int j = fi.ifhitbyweapon(act);
				if (j >= 0)
				{
					s->cstat = 32 + 128;
					k = 1;

					DukeStatIterator it(STAT_ACTOR);
					while (auto act2 = it.Next())
					{
						if (act2->s->lotag == s->lotag &&
							act2->s->picnum == s->picnum)
						{
							if ((act2->s->hitag && !(act2->s->cstat & 32)) ||
								(!act2->s->hitag && (act2->s->cstat & 32))
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
						fi.operateforcefields(act, s->lotag);
						operatemasterswitches(s->lotag);
					}
				}
			}
			continue;

		case RESPAWNMARKERRED:
		case RESPAWNMARKERYELLOW:
		case RESPAWNMARKERGREEN:
			if (!respawnmarker(act, RESPAWNMARKERYELLOW, RESPAWNMARKERGREEN)) continue;
			break;

		case HELECOPT:
		case DUKECAR:

			s->z += s->zvel;
			t[0]++;

			if (t[0] == 4) S_PlayActorSound(WAR_AMBIENCE2, act);

			if (t[0] > (26 * 8))
			{
				S_PlaySound(RPG_EXPLODE);
				for (int j = 0; j < 32; j++) 
						RANDOMSCRAP(act);
				earthquaketime = 16;
				deletesprite(act);
				continue;
			} 
			else if ((t[0] & 3) == 0)
				spawn(act, EXPLOSION2);
			ssp(act, CLIPMASK0);
			break;
		case RAT:
			if (!rat(act, true)) continue;
			break;
		case QUEBALL:
		case STRIPEBALL:
			if (!queball(act, POCKET, QUEBALL, STRIPEBALL)) continue;
			break;
		case FORCESPHERE:
			forcesphere(act, FORCESPHERE);
			continue;

		case RECON:
			recon(act, EXPLOSION2, FIRELASER, RECO_ATTACK, RECO_PAIN, RECO_ROAM, 10, [](DDukeActor* i)->int { return PIGCOP; });
			continue;

		case OOZ:
		case OOZ2:
			ooz(act);
			continue;

		case GREENSLIME:
		case GREENSLIME + 1:
		case GREENSLIME + 2:
		case GREENSLIME + 3:
		case GREENSLIME + 4:
		case GREENSLIME + 5:
		case GREENSLIME + 6:
		case GREENSLIME + 7:
			greenslime(act);
			continue;

		case BOUNCEMINE:
		case MORTER:
			spawn(act, FRAMEEFFECT1)->temp_data[0] = 3;
			[[fallthrough]];

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
		if (ud.multimode < 2 && badguy(act))
		{
			if (actor_tog == 1)
			{
				s->cstat = 32768;
				continue;
			}
			else if (actor_tog == 2) s->cstat = 257;
		}
		// #endif

		p = findplayer(act, &x);

		execute(act, p, x);
	}

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void fireflyflyingeffect(DDukeActor *actor)
{
	int x, p = findplayer(actor, &x);
	execute(actor, p, x);

	auto Owner = actor->GetOwner();
	if (!Owner || Owner->s->picnum != FIREFLY) 
	{
		deletesprite(actor);
		return;
	}

	if (Owner->s->xrepeat >= 24 || Owner->s->pal == 1)
		actor->s->cstat |= 0x8000;
	else
		actor->s->cstat &= ~0x8000;

	double dx = Owner->s->x - ps[p].GetActor()->s->x;
	double dy = Owner->s->y - ps[p].GetActor()->s->y;
	double dist = sqrt(dx * dx + dy * dy);
	if (dist != 0.0) 
	{
		dx /= dist;
		dy /= dist;
	}

	actor->s->x = (int) (Owner->s->x - (dx * -10.0));
	actor->s->y = (int) (Owner->s->y - (dy * -10.0));
	actor->s->z = Owner->s->z + 2048;

	if (Owner->s->extra <= 0) 
	{
		deletesprite(actor);
	}

}
//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveexplosions_d(void)  // STATNUM 5
{
	int p;
	int x;
	
	DukeStatIterator it(STAT_MISC);
	while (auto act = it.Next())
	{
		auto s = act->s;

		if (s->sectnum < 0 || s->xrepeat == 0) 
		{
			deletesprite(act);
			continue;
		}

		int* t = &act->temp_data[0];
		auto sectp = s->sector();

		switch (s->picnum)
		{
		case FIREFLYFLYINGEFFECT:
			if (isWorldTour()) fireflyflyingeffect(act);
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
				auto Owner = act->GetOwner();
				if (t[0] == 8) s->picnum = NUKEBUTTON + 1;
				else if (t[0] == 16 && Owner)
				{
					s->picnum = NUKEBUTTON + 2;
					ps[Owner->PlayerIndex()].fist_incs = 1;
				}
				if (Owner && ps[Owner->PlayerIndex()].fist_incs == 26)
					s->picnum = NUKEBUTTON + 3;
			}
			continue;

		case FORCESPHERE:
			forcesphereexplode(act);
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
			[[fallthrough]];

		case FIRELASER:
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
		case MONEY + 1:
		case MAIL + 1:
		case PAPER + 1:
			act->floorz = s->z = getflorzofslope(s->sectnum, s->x, s->y);
			break;
		case MONEY:
		case MAIL:
		case PAPER:
			money(act, BLOODPOOL);
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
			jibs(act, JIBS6, true, false, false, s->picnum == DUKELEG || s->picnum == DUKETORSO || s->picnum == DUKEGUN, false);

			continue;
		case BLOODPOOL:
		case PUKE:
			bloodpool(act, s->picnum == PUKE, TIRE);

			continue;

		case LAVAPOOL:
		case ONFIRE:
		case ONFIRESMOKE:
		case BURNEDCORPSE:
		case LAVAPOOLBUBBLE:
		case WHISPYSMOKE:
			if (!isWorldTour())
				continue;
			[[fallthrough]];

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
			p = findplayer(act, &x);
			execute(act, p, x);
			continue;

		case SHELL:
		case SHOTGUNSHELL:
			shell(act, (sectp->floorz + (24 << 8)) < s->z);
			continue;

		case GLASSPIECES:
		case GLASSPIECES + 1:
		case GLASSPIECES + 2:
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

void handle_se06_d(DDukeActor* actor)
{
	auto s = actor->s;
	auto t = &actor->temp_data[0];

	auto sc = actor->getSector();
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
		}
	}
	else s->xvel = k;

	DukeStatIterator it(STAT_EFFECTOR);
	while (auto act2 = it.Next())
	{
		if ((act2->s->lotag == 14) && (sh == act2->s->hitag) && (act2->temp_data[0] == t[0]))
		{
			act2->s->xvel = s->xvel;
			//if( t[4] == 1 )
			{
				if (act2->temp_data[5] == 0)
					act2->temp_data[5] = dist(act2, actor);
				int x = Sgn(dist(act2, actor) - act2->temp_data[5]);
				if (act2->s->extra)
					x = -x;
				s->xvel += x;
			}
			act2->temp_data[4] = t[4];
		}
	}
	handle_se14(actor, true, RPG, JIBS6);
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void handle_se28(DDukeActor* actor)
{
	auto s = actor->s;
	int* t = &actor->temp_data[0];

	if (t[5] > 0)
	{
		t[5]--;
		return;
	}

	if (t[0] == 0)
	{
		int x;
		findplayer(actor, &x);
		if (x > 15500)
			return;
		t[0] = 1;
		t[1] = 64 + (krand() & 511);
		t[2] = 0;
	}
	else
	{
		t[2]++;
		if (t[2] > t[1])
		{
			t[0] = 0;
			ps[screenpeek].visibility = ud.const_visibility;
			return;
		}
		else if (t[2] == (t[1] >> 1))
			S_PlayActorSound(THUNDER, actor);
		else if (t[2] == (t[1] >> 3))
			S_PlayActorSound(LIGHTNING_SLAP, actor);
		else if (t[2] == (t[1] >> 2))
		{
			DukeStatIterator it(STAT_DEFAULT);
			while (auto act2 = it.Next())
			{
				if (act2->s->picnum == NATURALLIGHTNING && act2->s->hitag == s->hitag)
					act2->s->cstat |= 32768;
			}
		}
		else if (t[2] > (t[1] >> 3) && t[2] < (t[1] >> 2))
		{
			int j = !!cansee(s->x, s->y, s->z, s->sectnum, ps[screenpeek].pos.x, ps[screenpeek].pos.y, ps[screenpeek].pos.z, ps[screenpeek].cursectnum);

			if (rnd(192) && (t[2] & 1))
			{
				if (j) ps[screenpeek].visibility = 0;
			}
			else if (j)	ps[screenpeek].visibility = ud.const_visibility;

			DukeStatIterator it(STAT_DEFAULT);
			while (auto act2 = it.Next())
			{
				if (act2->s->picnum == NATURALLIGHTNING && act2->s->hitag == s->hitag)
				{
					if (rnd(32) && (t[2] & 1))
					{
						act2->s->cstat &= 32767;
						spawn(act2, SMALLSMOKE);

						int x;
						int p = findplayer(actor, &x);
						auto psa = ps[p].GetActor();
						x = ldist(psa, act2);
						if (x < 768)
						{
							if (S_CheckActorSoundPlaying(psa, DUKE_LONGTERM_PAIN) < 1)
								S_PlayActorSound(DUKE_LONGTERM_PAIN, psa);
							S_PlayActorSound(SHORT_CIRCUIT, psa);
							psa->s->extra -= 8 + (krand() & 7);
							SetPlayerPal(&ps[p], PalEntry(32, 16, 0, 0));
						}
						return;
					}
					else act2->s->cstat |= 32768;
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

void moveeffectors_d(void)   //STATNUM 3
{
	int l;

	clearfriction();
	
	DukeStatIterator it(STAT_EFFECTOR);
	while (auto act = it.Next())
	{
		auto sc = act->getSector();
		switch (act->s->lotag)
		{
		case SE_0_ROTATING_SECTOR:
			handle_se00(act, LASERLINE);
			break;
			
		case SE_1_PIVOT: //Nothing for now used as the pivot
			handle_se01(act);
			break;
			
		case SE_6_SUBWAY:
			handle_se06_d(act);
			break;

		case SE_14_SUBWAY_CAR:
			handle_se14(act, true, RPG, JIBS6);
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
			handle_se08(act, false);
			break;

		case SE_10_DOOR_AUTO_CLOSE:
		{
			static const int tags[] = { 20, 21, 22, 26, 0};
			handle_se10(act, tags);
			break;
		}
		case SE_11_SWINGING_DOOR:
			handle_se11(act);
			break;
			
		case SE_12_LIGHT_SWITCH:
			handle_se12(act);
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

		case SE_24_CONVEYOR:
		case SE_34:
		{
			static const int16_t list1[] = { BLOODPOOL, PUKE, FOOTPRINTS, FOOTPRINTS2, FOOTPRINTS3, FOOTPRINTS4, BULLETHOLE, BLOODSPLAT1, BLOODSPLAT2, BLOODSPLAT3, BLOODSPLAT4, -1 };
			static const int16_t list2[] = { BOLT1, BOLT1 + 1,BOLT1 + 2, BOLT1 + 3, SIDEBOLT1, SIDEBOLT1 + 1, SIDEBOLT1 + 2, SIDEBOLT1 + 3, -1 };
			handle_se24(act, list1, list2, true, TRIPBOMB, LASERLINE, CRANE, 2);
			break;
		}
		case SE_35:
			handle_se35(act, SMALLSMOKE, EXPLOSION2);
			break;

		case SE_25_PISTON: //PISTONS
			if (act->temp_data[4] == 0) break;
			handle_se25(act, 3, -1, -1);
			break;

		case SE_26:
			handle_se26(act);
			break;

		case SE_27_DEMO_CAM:
			handle_se27(act);
			break;
		case SE_28_LIGHTNING:
			handle_se28(act);
			break;

		case SE_29_WAVES:
			act->s->hitag += 64;
			l = MulScale(act->s->yvel, bsin(act->s->hitag), 12);
			sc->floorz = act->s->z + l;
			break;
		case SE_31_FLOOR_RISE_FALL: // True Drop Floor
			handle_se31(act, true);
			break;

		case SE_32_CEILING_RISE_FALL: // True Drop Ceiling
			handle_se32(act);
			break;

		case SE_33_QUAKE_DEBRIS:
			if (earthquaketime > 0 && (krand() & 7) == 0)
				RANDOMSCRAP(act);
			break;
		case SE_36_PROJ_SHOOTER:

			if (act->temp_data[0])
			{
				if (act->temp_data[0] == 1)
					fi.shoot(act, sc->extra);
				else if (act->temp_data[0] == 26 * 5)
					act->temp_data[0] = 0;
				act->temp_data[0]++;
			}
			break;

		case SE_128_GLASS_BREAKING:
			handle_se128(act);
			break;

		case 130:
			handle_se130(act, 80, EXPLOSION2);
			break;
		case 131:
			handle_se130(act, 40, EXPLOSION2);
			break;
		}
	}

	//Sloped sin-wave floors!
	it.Reset(STAT_EFFECTOR);
	while (auto act = it.Next())
	{
		if (act->s->lotag != SE_29_WAVES) continue;
		auto sc = act->getSector();
		if (sc->wallnum != 4) continue;
		auto wal = sc->firstWall() + 2;
		alignflorslope(act->s->sectnum, wal->x, wal->y, wal->nextSector()->floorz);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void move_d(DDukeActor *actor, int playernum, int xvel)
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
		if (ps[playernum].newOwner != nullptr)
			goalang = getangle(ps[playernum].oposx - spr->x, ps[playernum].oposy - spr->y);
		else goalang = getangle(ps[playernum].pos.x - spr->x, ps[playernum].pos.y - spr->y);
		angdif = getincangle(spr->ang, goalang) >> 2;
		if (angdif > -8 && angdif < 0) angdif = 0;
		spr->ang += angdif;
	}

	if (a & spin)
		spr->ang += bsin(t[0] << 3, -6);

	if (a & face_player_slow)
	{
		if (ps[playernum].newOwner != nullptr)
			goalang = getangle(ps[playernum].oposx - spr->x, ps[playernum].oposy - spr->y);
		else goalang = getangle(ps[playernum].pos.x - spr->x, ps[playernum].pos.y - spr->y);
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
		if (t[0] < 16)
			spr->zvel -= bcos(t[0] << 4, -5);
	}

	if (a & face_player_smart)
	{
		int newx, newy;

		newx = ps[playernum].pos.x + (ps[playernum].posxv / 768);
		newy = ps[playernum].pos.y + (ps[playernum].posyv / 768);
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
		return;
	}

	auto moveptr = &ScriptCode[t[1]];

	if (a & geth) spr->xvel += (*moveptr - spr->xvel) >> 1;
	if (a & getv) spr->zvel += ((*(moveptr + 1) << 4) - spr->zvel) >> 1;

	if (a & dodgebullet)
		dodge(actor);

	if (spr->picnum != APLAYER)
		alterang(a, actor, playernum);

	if (spr->xvel > -6 && spr->xvel < 6) spr->xvel = 0;

	a = badguy(actor);

	if (spr->xvel || spr->zvel)
	{
		if (a && spr->picnum != ROTATEGUN)
		{
			if ((spr->picnum == DRONE || spr->picnum == COMMANDER) && spr->extra > 0)
			{
				if (spr->picnum == COMMANDER)
				{
					actor->floorz = l = getflorzofslope(spr->sectnum, spr->x, spr->y);
					if (spr->z > (l - (8 << 8)))
					{
						if (spr->z > (l - (8 << 8))) spr->z = l - (8 << 8);
						spr->zvel = 0;
					}

					actor->ceilingz = l = getceilzofslope(spr->sectnum, spr->x, spr->y);
					if ((spr->z - l) < (80 << 8))
					{
						spr->z = l + (80 << 8);
						spr->zvel = 0;
					}
				}
				else
				{
					if (spr->zvel > 0)
					{
						actor->floorz = l = getflorzofslope(spr->sectnum, spr->x, spr->y);
						if (spr->z > (l - (30 << 8)))
							spr->z = l - (30 << 8);
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
			}
			else if (spr->picnum != ORGANTIC)
			{
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
		}
		else if (spr->picnum == APLAYER)
			if ((spr->z - actor->ceilingz) < (32 << 8))
				spr->z = actor->ceilingz + (32 << 8);

		daxvel = spr->xvel;
		angdif = spr->ang;

		if (a && spr->picnum != ROTATEGUN)
		{
			if (xvel < 960 && spr->xrepeat > 16)
			{

				daxvel = -(1024 - xvel);
				angdif = getangle(ps[playernum].pos.x - spr->x, ps[playernum].pos.y - spr->y);

				if (xvel < 512)
				{
					ps[playernum].posxv = 0;
					ps[playernum].posyv = 0;
				}
				else
				{
					ps[playernum].posxv = MulScale(ps[playernum].posxv, gs.playerfriction - 0x2000, 16);
					ps[playernum].posyv = MulScale(ps[playernum].posyv, gs.playerfriction - 0x2000, 16);
				}
			}
			else if (spr->picnum != DRONE && spr->picnum != SHARK && spr->picnum != COMMANDER)
			{
				if (spr->oz != spr->z || (ud.multimode < 2 && ud.player_skill < 2))
				{
					if ((t[0] & 1) || ps[playernum].actorsqu == actor) return;
					else daxvel <<= 1;
				}
				else
				{
					if ((t[0] & 3) || ps[playernum].actorsqu == actor) return;
					else daxvel <<= 2;
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
			spr->shade += (spr->sector()->ceilingshade - spr->shade) >> 1;
		else spr->shade += (spr->sector()->floorshade - spr->shade) >> 1;

		if (spr->sector()->floorpicnum == MIRROR)
			deletesprite(actor);
	}
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void fall_d(DDukeActor *actor, int g_p)
{
	fall_common(actor, g_p, JIBS6, DRONE, BLOODPOOL, SHOTSPARK1, SQUISHED, THUD, nullptr);
}

bool spawnweapondebris_d(int picnum, int dnum)
{
	return picnum == BLIMP && dnum == SCRAP1;
}

void respawnhitag_d(DDukeActor* actor)
{
	switch (actor->s->picnum)
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
		if (actor->s->yvel) fi.operaterespawns(actor->s->yvel);
		break;
	default:
		if (actor->s->hitag >= 0) fi.operaterespawns(actor->s->hitag);
		break;
	}
}

void checktimetosleep_d(DDukeActor *actor)
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
