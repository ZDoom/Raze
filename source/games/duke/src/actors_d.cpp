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


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool ceilingspace_d(sectortype* sectp)
{
	if (sectp && (sectp->ceilingstat & CSTAT_SECTOR_SKY) && sectp->ceilingpal == 0)
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
	if (sectp && (sectp->floorstat & CSTAT_SECTOR_SKY) && sectp->ceilingpal == 0)
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
	if (actor->spr.extra > 0) switch (actor->spr.picnum)
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
		if (actor->spr.pal == 1)
			S_PlaySound(BOS2_RECOG);
		else S_PlaySound(WHIPYOURASS);
		break;
	case BOSS3:
		if (actor->spr.pal == 1)
			S_PlaySound(BOS3_RECOG);
		else S_PlaySound(RIPHEADNECK);
		break;
	case BOSS4:
	case BOSS4STAYPUT:
		if (actor->spr.pal == 1)
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
	p->curr_weapon = weapon;
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

	bool squishme = false;
	if (actor->spr.picnum == APLAYER && ud.clipping)
		return false;

	auto sectp = actor->spr.sector();
	int floorceildist = sectp->floorz - sectp->ceilingz;

	if (sectp->lotag != ST_23_SWINGING_DOOR)
	{
		if (actor->spr.pal == 1)
			squishme = floorceildist < (32 << 8) && (sectp->lotag & 32768) == 0;
		else
			squishme = floorceildist < (12 << 8);
	}

	if (squishme)
	{
		FTA(QUOTE_SQUISHED, &ps[p]);

		if (badguy(actor))
			actor->spr.xvel = 0;

		if (actor->spr.pal == 1)
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

	if(actor->spr.picnum != SHRINKSPARK && !(actor->spr.picnum == RPG && actor->spr.xrepeat < 11))
	{
		BFSSectorSearch search(actor->spr.sector());
	
		while (auto dasectp = search.GetNext())
		{
			if (((dasectp->ceilingz - actor->spr.z) >> 8) < r)
			{
				auto wal = dasectp->firstWall();
				int d = abs(wal->x - actor->spr.x) + abs(wal->y - actor->spr.y);
				if (d < r)
					fi.checkhitceiling(dasectp);
				else
				{
					auto thirdpoint = wal->point2Wall()->point2Wall();
					d = abs(thirdpoint->x - actor->spr.x) + abs(thirdpoint->y - actor->spr.y);
					if (d < r)
						fi.checkhitceiling(dasectp);
				}
			}

			for (auto& wal : wallsofsector(dasectp))
			{
				if ((abs(wal.x - actor->spr.x) + abs(wal.y - actor->spr.y)) < r)
				{
					if (wal.twoSided())
					{
						search.Add(wal.nextSector());
					}
					int x1 = (((wal.x + wal.point2Wall()->x) >> 1) + actor->spr.x) >> 1;
					int y1 = (((wal.y + wal.point2Wall()->y) >> 1) + actor->spr.y) >> 1;
					sectortype* sect = wal.sectorp();
					updatesector(x1, y1, &sect);
					if (sect && cansee(x1, y1, actor->spr.z, sect, actor->spr.x, actor->spr.y, actor->spr.z, actor->spr.sector()))
						fi.checkhitwall(actor, &wal, wal.x, wal.y, actor->spr.z, actor->spr.picnum);
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
			if (isWorldTour() && Owner)
			{
				if (Owner->spr.picnum == APLAYER && act2->spr.picnum == APLAYER && ud.coop != 0 && ud.ffire == 0 && Owner != act2)
				{
					continue;
				}
				
				if (actor->spr.picnum == FLAMETHROWERFLAME && ((Owner->spr.picnum == FIREFLY && act2->spr.picnum == FIREFLY) || (Owner->spr.picnum == BOSS5 && act2->spr.picnum == BOSS5)))
				{
					continue;
				}
			}
			
			if (x == 0 || x >= 5 || AFLAMABLE(act2->spr.picnum))
			{
				if (actor->spr.picnum != SHRINKSPARK || (act2->spr.cstat & CSTAT_SPRITE_BLOCK_ALL))
					if (dist(actor, act2) < r)
					{
						if (badguy(act2) && !cansee(act2->spr.x, act2->spr.y, act2->spr.z + q, act2->spr.sector(), actor->spr.x, actor->spr.y, actor->spr.z + q, actor->spr.sector()))
							continue;
						fi.checkhitsprite(act2, actor);
					}
			}
			else if (act2->spr.extra >= 0 && act2 != actor && (act2->spr.picnum == TRIPBOMB || badguy(act2) || act2->spr.picnum == QUEBALL || act2->spr.picnum == STRIPEBALL || (act2->spr.cstat & CSTAT_SPRITE_BLOCK_ALL) || act2->spr.picnum == DUKELYINGDEAD))
			{
				if (actor->spr.picnum == SHRINKSPARK && act2->spr.picnum != SHARK && (act2 == Owner || act2->spr.xrepeat < 24))
				{
					continue;
				}
				if (actor->spr.picnum == MORTER && act2 == Owner)
				{
					continue;
				}
				
				if (act2->spr.picnum == APLAYER) act2->spr.z -= gs.playerheight;
				int d = dist(actor, act2);
				if (act2->spr.picnum == APLAYER) act2->spr.z += gs.playerheight;
				
				if (d < r && cansee(act2->spr.x, act2->spr.y, act2->spr.z - (8 << 8), act2->spr.sector(), actor->spr.x, actor->spr.y, actor->spr.z - (12 << 8), actor->spr.sector()))
				{
					act2->ang = getangle(act2->spr.x - actor->spr.x, act2->spr.y - actor->spr.y);
					
					if (actor->spr.picnum == RPG && act2->spr.extra > 0)
						act2->picnum = RPG;
					else if (!isWorldTour())
					{
						if (actor->spr.picnum == SHRINKSPARK)
							act2->picnum = SHRINKSPARK;
						else act2->picnum = RADIUSEXPLOSION;
					}
					else
					{
						if (actor->spr.picnum == SHRINKSPARK || actor->spr.picnum == FLAMETHROWERFLAME)
							act2->picnum = actor->spr.picnum;
						else if (actor->spr.picnum != FIREBALL || !Owner || Owner->spr.picnum != APLAYER)
						{
							if (actor->spr.picnum == LAVAPOOL)
								act2->picnum = FLAMETHROWERFLAME;
							else
								act2->picnum = RADIUSEXPLOSION;
						}
						else
							act2->picnum = FLAMETHROWERFLAME;
					}
					
					if (actor->spr.picnum != SHRINKSPARK && (!isWorldTour() || actor->spr.picnum != LAVAPOOL))
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
						
						if (act2->spr.picnum != TANK && act2->spr.picnum != ROTATEGUN && act2->spr.picnum != RECON && !bossguy(act2))
						{
							if (act2->spr.xvel < 0) act2->spr.xvel = 0;
							act2->spr.xvel += (actor->spr.extra << 2);
						}
						
						if (act2->spr.picnum == PODFEM1 || act2->spr.picnum == FEM1 ||
							act2->spr.picnum == FEM2 || act2->spr.picnum == FEM3 ||
							act2->spr.picnum == FEM4 || act2->spr.picnum == FEM5 ||
							act2->spr.picnum == FEM6 || act2->spr.picnum == FEM7 ||
							act2->spr.picnum == FEM8 || act2->spr.picnum == FEM9 ||
							act2->spr.picnum == FEM10 || act2->spr.picnum == STATUE ||
							act2->spr.picnum == STATUEFLASH || act2->spr.picnum == SPACEMARINE || act2->spr.picnum == QUEBALL || act2->spr.picnum == STRIPEBALL)
							fi.checkhitsprite(act2, actor);
					}
					else if (actor->spr.extra == 0) act2->extra = 0;
					
					if (act2->spr.picnum != RADIUSEXPLOSION && Owner && Owner->spr.statnum < MAXSTATUS)
					{
						if (act2->spr.picnum == APLAYER)
						{
							int p = act2->spr.yvel;
							
							if (isWorldTour() && act2->picnum == FLAMETHROWERFLAME && Owner->spr.picnum == APLAYER)
							{
								ps[p].numloogs = -1 - actor->spr.yvel;
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
	int bg = badguy(actor);

	if (actor->spr.statnum == 5 || (bg && actor->spr.xrepeat < 4))
	{
		actor->spr.x += (xchange * TICSPERFRAME) >> 2;
		actor->spr.y += (ychange * TICSPERFRAME) >> 2;
		actor->spr.z += (zchange * TICSPERFRAME) >> 2;
		if (bg)
			SetActor(actor, actor->spr.pos);
		return result.setNone();
	}

	auto dasectp = actor->spr.sector();

	vec3_t pos = actor->spr.pos;
	pos.z -= ((tileHeight(actor->spr.picnum) * actor->spr.yrepeat) << 1);

	if (bg)
	{
		if (actor->spr.xrepeat > 60)
			clipmove(pos, &dasectp, ((xchange * TICSPERFRAME) << 11), ((ychange * TICSPERFRAME) << 11), 1024, (4 << 8), (4 << 8), cliptype, result);
		else 
		{
			if (actor->spr.picnum == LIZMAN)
				clipdist = 292;
			else if (actorflag(actor, SFLAG_BADGUY))
				clipdist = actor->spr.clipdist << 2;
			else
				clipdist = 192;

			clipmove(pos, &dasectp, ((xchange * TICSPERFRAME) << 11), ((ychange * TICSPERFRAME) << 11), clipdist, (4 << 8), (4 << 8), cliptype, result);
		}

		// conditional code from hell...
		if (dasectp == nullptr || (dasectp != nullptr &&
			((actor->actorstayput != nullptr && actor->actorstayput != dasectp) ||
			 ((actor->spr.picnum == BOSS2) && actor->spr.pal == 0 && dasectp->lotag != 3) ||
			 ((actor->spr.picnum == BOSS1 || actor->spr.picnum == BOSS2) && dasectp->lotag == ST_1_ABOVE_WATER) ||
			 (dasectp->lotag == ST_1_ABOVE_WATER && (actor->spr.picnum == LIZMAN || (actor->spr.picnum == LIZTROOP && actor->spr.zvel == 0)))
			))
		 )
		{
			if (dasectp && dasectp->lotag == ST_1_ABOVE_WATER && actor->spr.picnum == LIZMAN)
				actor->spr.ang = (krand()&2047);
			else if ((actor->temp_data[0]&3) == 1 && actor->spr.picnum != COMMANDER)
				actor->spr.ang = (krand()&2047);
			SetActor(actor,actor->spr.pos);
			if (dasectp == nullptr) dasectp = &sector[0];
			return result.setSector(dasectp);
		}
		if ((result.type == kHitWall || result.type == kHitSprite) && (actor->cgg == 0)) actor->spr.ang += 768;
	}
	else
	{
		if (actor->spr.statnum == STAT_PROJECTILE)
			clipmove(pos, &dasectp, ((xchange * TICSPERFRAME) << 11), ((ychange * TICSPERFRAME) << 11), 8, (4 << 8), (4 << 8), cliptype, result);
		else
			clipmove(pos, &dasectp, ((xchange * TICSPERFRAME) << 11), ((ychange * TICSPERFRAME) << 11), (int)(actor->spr.clipdist << 2), (4 << 8), (4 << 8), cliptype, result);
	}
	actor->spr.x = pos.X;
	actor->spr.y = pos.Y;

	if (dasectp != nullptr)
		if (dasectp != actor->spr.sector())
			ChangeActorSect(actor, dasectp);
	int daz = actor->spr.z + ((zchange * TICSPERFRAME) >> 3);
	if ((daz > actor->ceilingz) && (daz <= actor->floorz))
		actor->spr.z = daz;
	else if (result.type == kHitNone)
		return result.setSector(dasectp);

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
	int gutz, floorz;
	int j;
	int sx, sy;
	uint8_t pal;

	if (badguy(actor) && actor->spr.xrepeat < 16)
		sx = sy = 8;
	else sx = sy = 32;

	gutz = actor->spr.z - (8 << 8);
	floorz = getflorzofslopeptr(actor->spr.sector(), actor->spr.x, actor->spr.y);

	if (gutz > (floorz - (8 << 8)))
		gutz = floorz - (8 << 8);

	gutz += gs.actorinfo[actor->spr.picnum].gutsoffset;

	if (badguy(actor) && actor->spr.pal == 6)
		pal = 6;
	else if (actor->spr.picnum != LIZTROOP) // EDuke32 transfers the palette unconditionally, I'm not sure that's such a good idea.
		pal = 0;
	else
		pal = actor->spr.pal;

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
		auto spawned = EGS(actor->spr.sector(), actor->spr.x + (r5 & 255) - 128, actor->spr.y + (r4 & 255) - 128, gutz - (r3 & 8191), gtype, -32, sx, sy, a, 48 + (r2 & 31), -512 - (r1 & 2047), ps[p].GetActor(), 5);
		if (spawned)
		{
			if (spawned->spr.picnum == JIBS2)
			{
				spawned->spr.xrepeat >>= 2;
				spawned->spr.yrepeat >>= 2;
			}
			if (pal != 0)
				spawned->spr.pal = pal;
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
	sectortype* psect, *ssect;
	int j;

	DukeStatIterator iti(STAT_ZOMBIEACTOR);

	while (auto act = iti.Next())
	{
		p = findplayer(act, &x);

		ssect = psect = act->spr.sector();

		auto pa = ps[p].GetActor();
		if (pa->spr.extra > 0)
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
						if (psect == nullptr)
						{
							continue;
						}
						sx = act->spr.x + 64 - (krand() & 127);
						sy = act->spr.y + 64 - (krand() & 127);
						updatesector(px, py, &ssect);
						if (ssect == nullptr)
						{
							continue;
						}

						int r1 = krand();
						int r2 = krand();
						j = cansee(sx, sy, act->spr.z - (r2 % (52 << 8)), act->spr.sector(), px, py, ps[p].oposz - (r1 % (32 << 8)), ps[p].cursector);
					}
					else
					{
						int r1 = krand();
						int r2 = krand();
						j = cansee(act->spr.x, act->spr.y, act->spr.z - ((r2 & 31) << 8), act->spr.sector(), ps[p].oposx, ps[p].oposy, ps[p].oposz - ((r1 & 31) << 8), ps[p].cursector);
					}


					if (j) switch(act->spr.picnum)
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
							if (act->spr.sector()->ceilingstat & CSTAT_SECTOR_SKY)
								act->spr.shade = act->spr.sector()->ceilingshade;
							else act->spr.shade = act->spr.sector()->floorshade;

							act->timetosleep = 0;
							ChangeActorStat(act, STAT_STANDABLE);
							break;

						default:
							act->timetosleep = 0;
							check_fta_sounds_d(act);
							ChangeActorStat(act, STAT_ACTOR);
							break;
					}
					else act->timetosleep = 0;
				}
			}
			if (badguy(act))
			{
				if (act->spr.sector()->ceilingstat & CSTAT_SECTOR_SKY)
					act->spr.shade = act->spr.sector()->ceilingshade;
				else act->spr.shade = act->spr.sector()->floorshade;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

DDukeActor* ifhitsectors_d(sectortype* sect)
{
	DukeStatIterator it(STAT_MISC);
	while (auto a1 = it.Next())
	{
		if (a1->spr.picnum == EXPLOSION2 && sect == a1->spr.sector())
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
	auto hitowner = actor->GetHitOwner();

	if (actor->extra >= 0)
	{
		if (actor->spr.extra >= 0)
		{
			if (actor->spr.picnum == APLAYER)
			{
				if (ud.god && actor->picnum != SHRINKSPARK) return -1;

				p = actor->spr.yvel;
				
				if (hitowner &&
					hitowner->spr.picnum == APLAYER &&
					ud.coop == 1 &&
					ud.ffire == 0)
					return -1;

				actor->spr.extra -= actor->extra;

				if (hitowner)
				{
					if (actor->spr.extra <= 0 && actor->picnum != FREEZEBLAST)
					{
						actor->spr.extra = 0;

						ps[p].wackedbyactor = hitowner;

						if (hitowner->spr.picnum == APLAYER && p != hitowner->PlayerIndex())
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
					if (actor->picnum == SHRINKSPARK && actor->spr.xrepeat < 24)
						return -1;

				if (isWorldTour() && actor->picnum == FIREFLY && actor->spr.xrepeat < 48)
				{
					if (actor->picnum != RADIUSEXPLOSION && actor->picnum != RPG)
						return -1;
				}

				actor->spr.extra -= actor->extra;
				auto Owner = actor->GetOwner();
				if (actor->spr.picnum != RECON && Owner && Owner->spr.statnum < MAXSTATUS)
					actor->SetOwner(hitowner);
			}

			actor->extra = -1;
			return actor->picnum;
		}
	}


	if (ud.multimode < 2 || !isWorldTour()
		|| actor->picnum != FLAMETHROWERFLAME
		|| actor->extra >= 0
		|| actor->spr.extra > 0
		|| actor->spr.picnum != APLAYER
		|| ps[actor->PlayerIndex()].numloogs > 0
		|| hitowner == nullptr)
	{
		actor->extra = -1;
		return -1;
	}
	else
	{
		p = actor->PlayerIndex();
		actor->spr.extra = 0;
		ps[p].wackedbyactor = hitowner;

		if (hitowner->spr.picnum == APLAYER && hitowner != ps[p].GetActor())
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
		auto sectp = act->spr.sector();

		if (act->temp_data[0] == 0)
		{
			act->spr.z -= (16 << 8);
			act->temp_data[1] = act->spr.ang;
			x = act->spr.extra;
			j = fi.ifhitbyweapon(act);
			if (j >= 0)
			{
				if (j == FIREEXT || j == RPG || j == RADIUSEXPLOSION || j == SEENINE || j == OOZFILTER)
				{
					if (act->spr.extra <= 0)
					{
						act->temp_data[0] = 1;
						DukeStatIterator itj(STAT_FALLER);
						while (auto a2 = itj.Next())
						{
							if (a2->spr.hitag == act->spr.hitag)
							{
								a2->temp_data[0] = 1;
								a2->spr.cstat &= ~CSTAT_SPRITE_ONE_SIDE;
								if (a2->spr.picnum == CEILINGSTEAM || a2->spr.picnum == STEAM)
									a2->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
							}
						}
					}
				}
				else
				{
					act->extra = 0;
					act->spr.extra = x;
				}
			}
			act->spr.ang = act->temp_data[1];
			act->spr.z += (16 << 8);
		}
		else if (act->temp_data[0] == 1)
		{
			if (act->spr.lotag > 0)
			{
				act->spr.lotag-=3;
				if (act->spr.lotag <= 0)
				{
					act->spr.xvel = (32 + (krand() & 63));
					act->spr.zvel = -(1024 + (krand() & 1023));
				}
			}
			else
			{
				if (act->spr.xvel > 0)
				{
					act->spr.xvel -= 8;
					ssp(act, CLIPMASK0);
				}

				if (fi.floorspace(act->spr.sector())) x = 0;
				else
				{
					if (fi.ceilingspace(act->spr.sector()))
						x = gs.gravity / 6;
					else
						x = gs.gravity;
				}

				if (act->spr.z < (sectp->floorz - FOURSLEIGHT))
				{
					act->spr.zvel += x;
					if (act->spr.zvel > 6144)
						act->spr.zvel = 6144;
					act->spr.z += act->spr.zvel;
				}
				if ((sectp->floorz - act->spr.z) < (16 << 8))
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
	int j, x;
	int lTripBombControl = GetGameVar("TRIPBOMB_CONTROL", TRIPBOMB_TRIPWIRE, nullptr, -1).safeValue();
	if (lTripBombControl & TRIPBOMB_TIMER)
	{
		// we're on a timer....
		if (actor->spr.extra >= 0)
		{
			actor->spr.extra--;
			if (actor->spr.extra == 0)
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
			x = actor->spr.extra;
			fi.hitradius(actor, gs.tripbombblastradius, x >> 2, x >> 1, x - (x >> 2), x);

			auto spawned = spawn(actor, EXPLOSION2);
			if (spawned)
			{
				spawned->spr.ang = actor->spr.ang;
				spawned->spr.xvel = 348;
				ssp(spawned, CLIPMASK0);
			}

			DukeStatIterator it(STAT_MISC);
			while (auto a1 = it.Next())
			{
				if (a1->spr.picnum == LASERLINE && actor->spr.hitag == a1->spr.hitag)
					a1->spr.xrepeat = a1->spr.yrepeat = 0;
			}
			deletesprite(actor);
		}
		return;
	}
	else
	{
		x = actor->spr.extra;
		actor->spr.extra = 1;
		int16_t l = actor->spr.ang;
		j = fi.ifhitbyweapon(actor);
		if (j >= 0)
		{ 
			actor->temp_data[2] = 16; 
		}
		actor->spr.extra = x;
		actor->spr.ang = l;
	}

	if (actor->temp_data[0] < 32)
	{
		findplayer(actor, &x);
		if (x > 768) actor->temp_data[0]++;
		else if (actor->temp_data[0] > 16) actor->temp_data[0]++;
	}
	if (actor->temp_data[0] == 32)
	{
		int16_t l = actor->spr.ang;
		actor->spr.ang = actor->temp_data[5];

		actor->temp_data[3] = actor->spr.x; actor->temp_data[4] = actor->spr.y;
		actor->spr.x += bcos(actor->temp_data[5], -9);
		actor->spr.y += bsin(actor->temp_data[5], -9);
		actor->spr.z -= (3 << 8);

		// Laser fix from EDuke32.
		auto const oldSect = actor->spr.sector();
		auto       curSect = actor->spr.sector();

		updatesectorneighbor(actor->spr.x, actor->spr.y, &curSect, 2048);
		ChangeActorSect(actor, curSect);

		DDukeActor* hit;
		x = hitasprite(actor, &hit);

		actor->lastvx = x;

		actor->spr.ang = l;

		if (lTripBombControl & TRIPBOMB_TRIPWIRE)
		{
			// we're on a trip wire
			while (x > 0)
			{
				auto spawned = spawn(actor, LASERLINE);
				if (spawned)
				{
					SetActor(spawned, spawned->spr.pos);
					spawned->spr.hitag = actor->spr.hitag;
					spawned->temp_data[1] = spawned->spr.z;

					if (x < 1024)
					{
						spawned->spr.xrepeat = x >> 5;
						break;
					}
					x -= 1024;

					actor->spr.x += bcos(actor->temp_data[5], -4);
					actor->spr.y += bsin(actor->temp_data[5], -4);
					updatesectorneighbor(actor->spr.x, actor->spr.y, &curSect, 2048);

					if (curSect == nullptr)
						break;

					ChangeActorSect(actor, curSect);

					// this is a hack to work around the LASERLINE sprite's art tile offset
					ChangeActorSect(spawned, curSect);
				}
			}
		}

		actor->temp_data[0]++;
		actor->spr.x = actor->temp_data[3]; actor->spr.y = actor->temp_data[4];
		actor->spr.z += (3 << 8);
		ChangeActorSect(actor, oldSect);
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


		actor->temp_data[3] = actor->spr.x; actor->temp_data[4] = actor->spr.y;
		actor->spr.x += bcos(actor->temp_data[5], -9);
		actor->spr.y += bsin(actor->temp_data[5], -9);
		actor->spr.z -= (3 << 8);
		SetActor(actor, actor->spr.pos);

		x = hitasprite(actor, nullptr);

		actor->spr.x = actor->temp_data[3]; actor->spr.y = actor->temp_data[4];
		actor->spr.z += (3 << 8);
		SetActor(actor, actor->spr.pos);

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
	int* t = &actor->temp_data[0];
	if (actor->spr.hitag > 0)
	{
		t[0] = actor->spr.cstat;
		t[1] = actor->spr.ang;
		int j = fi.ifhitbyweapon(actor);
		if (j == FIREEXT || j == RPG || j == RADIUSEXPLOSION || j == SEENINE || j == OOZFILTER)
		{
			DukeStatIterator it(STAT_STANDABLE);
			while (auto a1 = it.Next())
			{
				if (actor->spr.hitag == a1->spr.hitag && (a1->spr.picnum == OOZFILTER || a1->spr.picnum == SEENINE))
					if (a1->spr.shade != -32)
						a1->spr.shade = -32;
			}
			detonate(actor, EXPLOSION2);
		}
		else
		{
			actor->spr.cstat = ESpriteFlags::FromInt(t[0]);
			actor->spr.ang = t[1];
			actor->spr.extra = 0;
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
		auto spawned = EGS(actor->spr.sector(), actor->spr.x, actor->spr.y, actor->spr.z - (krand() % (48 << 8)), SCRAP3 + (krand() & 3), -8, 48, 48, krand() & 2047, (krand() & 63) + 64, -(krand() & 4095) - (actor->spr.zvel >> 2), actor, 5);
		if(spawned) spawned->spr.pal = 2;
	}

	spawn(actor, EXPLOSION2);
	S_PlayActorSound(PIPEBOMB_EXPLODE, actor);
	S_PlayActorSound(GLASS_HEAVYBREAK, actor);

	if (actor->spr.hitag > 0)
	{
		DukeStatIterator it(STAT_STANDABLE);
		while (auto a1 = it.Next())
		{
			if (actor->spr.hitag == a1->spr.hitag && (a1->spr.picnum == OOZFILTER || a1->spr.picnum == SEENINE))
				if (a1->spr.shade != -32)
					a1->spr.shade = -32;
		}

		int x = actor->spr.extra;
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
	if (actor->spr.xrepeat == 0) deletesprite(actor);
	else
	{
		int x;
		int p = findplayer(actor, &x);

		x = dist(actor, ps[p].GetActor()); // the result from findplayer is not really useful.
		if (x >= VIEWSCR_DIST && camsprite == actor)
		{
			camsprite = nullptr;
			actor->spr.yvel = 0;
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
	int* t = &actor->temp_data[0];
	int x;
	auto sectp = actor->spr.sector();

	findplayer(actor, &x);
	if (x > 20480) return;

CLEAR_THE_BOLT2:
	if (t[2])
	{
		t[2]--;
		return;
	}
	if ((actor->spr.xrepeat | actor->spr.yrepeat) == 0)
	{
		actor->spr.xrepeat = t[0];
		actor->spr.yrepeat = t[1];
	}
	if ((krand() & 8) == 0)
	{
		t[0] = actor->spr.xrepeat;
		t[1] = actor->spr.yrepeat;
		t[2] = global_random & 4;
		actor->spr.xrepeat = actor->spr.yrepeat = 0;
		goto CLEAR_THE_BOLT2;
	}
	actor->spr.picnum++;

	if ((krand() & 1) && sectp->floorpicnum == HURTRAIL)
		S_PlayActorSound(SHORT_CIRCUIT, actor);

	if (actor->spr.picnum == SIDEBOLT1 + 4) actor->spr.picnum = SIDEBOLT1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static void movebolt(DDukeActor *actor)
{
	int* t = &actor->temp_data[0];
	int x;
	auto sectp = actor->spr.sector();

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
	if ((actor->spr.xrepeat | actor->spr.yrepeat) == 0)
	{
		actor->spr.xrepeat = t[0];
		actor->spr.yrepeat = t[1];
	}
	else if ((krand() & 8) == 0)
	{
		t[0] = actor->spr.xrepeat;
		t[1] = actor->spr.yrepeat;
		t[2] = global_random & 4;
		actor->spr.xrepeat = actor->spr.yrepeat = 0;
		goto CLEAR_THE_BOLT;
	}
	actor->spr.picnum++;

	int l = global_random & 7;
	actor->spr.xrepeat = l + 8;

	if (l & 1) actor->spr.cstat ^= CSTAT_SPRITE_TRANSLUCENT;

	if (actor->spr.picnum == (BOLT1+1) && (krand()&7) == 0 && sectp->floorpicnum == HURTRAIL)
		S_PlayActorSound(SHORT_CIRCUIT,actor);

	if (actor->spr.picnum==BOLT1+4) actor->spr.picnum=BOLT1;

	if (actor->spr.picnum & 1)
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
		int picnum = act->spr.picnum;

		if (!act->insector())
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
	auto Owner = actor->GetOwner();

	if (actor->spr.sector()->lotag == 2)
	{
		deletesprite(actor);
		return true;
	}

	if (!Owner || Owner->spr.picnum != FIREBALL)
	{
		if (actor->temp_data[0] >= 1 && actor->temp_data[0] < 6)
		{
			float siz = 1.0f - (actor->temp_data[0] * 0.2f);
			DDukeActor* trail = actor->temp_actor;
			auto ball = spawn(actor, FIREBALL);
			if (ball)
			{
				actor->temp_actor = ball;

				ball->spr.xvel = actor->spr.xvel;
				ball->spr.yvel = actor->spr.yvel;
				ball->spr.zvel = actor->spr.zvel;
				if (actor->temp_data[0] > 1)
				{
					if (trail)
					{
						FireProj* proj = &trail->fproj;
						ball->spr.x = proj->x;
						ball->spr.y = proj->y;
						ball->spr.z = proj->z;
						ball->spr.xvel = proj->xv;
						ball->spr.yvel = proj->yv;
						ball->spr.zvel = proj->zv;
					}
				}
				ball->spr.yrepeat = ball->spr.xrepeat = (uint8_t)(actor->spr.xrepeat * siz);
				ball->spr.cstat = actor->spr.cstat;
				ball->spr.extra = 0;

				ball->fproj = { ball->spr.x, ball->spr.y, ball->spr.z, ball->spr.xvel, ball->spr.yvel, ball->spr.zvel };

				ChangeActorStat(ball, STAT_PROJECTILE);
			}
		}
		actor->temp_data[0]++;
	}
	if (actor->spr.zvel < 15000)
		actor->spr.zvel += 200;
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static bool weaponhitsprite(DDukeActor* proj, DDukeActor *targ, bool fireball)
{
	if (proj->spr.picnum == FREEZEBLAST && targ->spr.pal == 1)
		if (badguy(targ) || targ->spr.picnum == APLAYER)
		{
			auto spawned = spawn(targ, TRANSPORTERSTAR);
			if (spawned)
			{
				spawned->spr.pal = 1;
				spawned->spr.xrepeat = 32;
				spawned->spr.yrepeat = 32;
			}

			deletesprite(proj);
			return true;
		}

	if (!isWorldTour() || proj->spr.picnum != FIREBALL || fireball)
		fi.checkhitsprite(targ, proj);

	if (targ->spr.picnum == APLAYER)
	{
		int p = targ->spr.yvel;
		auto Owner = proj->GetOwner();

		if (ud.multimode >= 2 && fireball && Owner && Owner->spr.picnum == APLAYER)
		{
			ps[p].numloogs = -1 - proj->spr.yvel;
		}

		S_PlayActorSound(PISTOL_BODYHIT, targ);

		if (proj->spr.picnum == SPIT)
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
	if (proj->spr.picnum != RPG && proj->spr.picnum != FREEZEBLAST && proj->spr.picnum != SPIT &&
		(!isWorldTour() || proj->spr.picnum != FIREBALL) &&
		(wal->overpicnum == MIRROR || wal->picnum == MIRROR))
	{
		int k = getangle(wal->delta());
		proj->spr.ang = ((k << 1) - proj->spr.ang) & 2047;
		proj->SetOwner(proj);
		spawn(proj, TRANSPORTERSTAR);
		return true;
	}
	else
	{
		SetActor(proj, oldpos);
		fi.checkhitwall(proj, wal, proj->spr.x, proj->spr.y, proj->spr.z, proj->spr.picnum);

		if (proj->spr.picnum == FREEZEBLAST)
		{
			if (wal->overpicnum != MIRROR && wal->picnum != MIRROR)
			{
				proj->spr.extra >>= 1;
				proj->spr.yvel--;
			}

			int k = getangle(wal->delta());
			proj->spr.ang = ((k << 1) - proj->spr.ang) & 2047;
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
	SetActor(proj, oldpos);

	if (proj->spr.zvel < 0)
	{
		if (proj->spr.sector()->ceilingstat & CSTAT_SECTOR_SKY)
			if (proj->spr.sector()->ceilingpal == 0)
			{
				deletesprite(proj);
				return true;
			}

		fi.checkhitceiling(proj->spr.sector());
	}
	else if (fireball)
	{
		auto spawned = spawn(proj, LAVAPOOL);
		if (spawned)
		{
			spawned->SetOwner(proj);
			spawned->SetHitOwner(proj);
			spawned->spr.yvel = proj->spr.yvel;
		}
		deletesprite(proj);
		return true;
	}

	if (proj->spr.picnum == FREEZEBLAST)
	{
		bounce(proj);
		ssp(proj, CLIPMASK1);
		proj->spr.extra >>= 1;
		if (proj->spr.xrepeat > 8)
			proj->spr.xrepeat -= 2;
		if (proj->spr.yrepeat > 8)
			proj->spr.yrepeat -= 2;
		proj->spr.yvel--;
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
	if (proj->spr.picnum == COOLEXPLOSION1)
		if (!S_CheckActorSoundPlaying(proj, WIERDSHOT_FLY))
			S_PlayActorSound(WIERDSHOT_FLY, proj);

	int k, ll;
	vec3_t oldpos = proj->spr.pos;

	if (proj->spr.picnum == RPG && proj->spr.sector()->lotag == 2)
	{
		k = proj->spr.xvel >> 1;
		ll = proj->spr.zvel >> 1;
	}
	else
	{
		k = proj->spr.xvel;
		ll = proj->spr.zvel;
	}

	getglobalz(proj);

	switch (proj->spr.picnum)
	{
	case RPG:
		if (proj->picnum != BOSS2 && proj->spr.xrepeat >= 10 && proj->spr.sector()->lotag != 2)
		{
			auto spawned = spawn(proj, SMALLSMOKE);
			if (spawned) spawned->spr.z += (1 << 8);
		}
		break;

	case FIREBALL:
		if (movefireball(proj)) return;
		break;
	}

	Collision coll;
	movesprite_ex(proj,
		MulScale(k, bcos(proj->spr.ang), 14),
		MulScale(k, bsin(proj->spr.ang), 14), ll, CLIPMASK1, coll);

	if (proj->spr.picnum == RPG && proj->temp_actor != nullptr)
		if (FindDistance2D(proj->spr.x - proj->temp_actor->spr.x, proj->spr.y - proj->temp_actor->spr.y) < 256)
			coll.setSprite(proj->temp_actor);

	if (!proj->spr.insector())
	{
		deletesprite(proj);
		return;
	}

	if (coll.type != kHitSprite && proj->spr.picnum != FREEZEBLAST)
	{
		if (proj->spr.z < proj->ceilingz)
		{
			coll.setSector(proj->spr.sector());
			proj->spr.zvel = -1;
		}
		else
			if ((proj->spr.z > proj->floorz && proj->spr.sector()->lotag != 1) ||
				(proj->spr.z > proj->floorz + (16 << 8) && proj->spr.sector()->lotag == 1))
			{
				coll.setSector(proj->spr.sector());
				if (proj->spr.sector()->lotag != 1)
					proj->spr.zvel = 1;
			}
	}

	if (proj->spr.picnum == FIRELASER)
	{
		for (k = -3; k < 2; k++)
		{
			auto spawned = EGS(proj->spr.sector(),
				proj->spr.x + MulScale(k, bcos(proj->spr.ang), 9),
				proj->spr.y + MulScale(k, bsin(proj->spr.ang), 9),
				proj->spr.z + ((k * Sgn(proj->spr.zvel)) * abs(proj->spr.zvel / 24)), FIRELASER, -40 + (k << 2),
				proj->spr.xrepeat, proj->spr.yrepeat, 0, 0, 0, proj->GetOwner(), 5);
			if (spawned)
			{
				spawned->spr.cstat = CSTAT_SPRITE_YCENTER;
				spawned->spr.pal = proj->spr.pal;
			}
		}
	}
	else if (proj->spr.picnum == SPIT) if (proj->spr.zvel < 6144)
		proj->spr.zvel += gs.gravity - 112;

	if (coll.type != 0)
	{
		if (proj->spr.picnum == COOLEXPLOSION1)
		{
			if (coll.type == kHitSprite && coll.actor()->spr.picnum != APLAYER)
			{
				return;
			}
			proj->spr.xvel = 0;
			proj->spr.zvel = 0;
		}

		bool fireball = (isWorldTour() && proj->spr.picnum == FIREBALL && (!proj->GetOwner() || proj->GetOwner()->spr.picnum != FIREBALL));

		if (coll.type == kHitSprite)
		{
			if (weaponhitsprite(proj, coll.actor(), fireball)) return;
		}
		else if (coll.type == kHitWall)
		{
			if (weaponhitwall(proj, coll.hitWall, oldpos)) return;
		}
		else if (coll.type == kHitSector)
		{
			if (weaponhitsector(proj, oldpos, fireball)) return;
		}

		if (proj->spr.picnum != SPIT)
		{
			if (proj->spr.picnum == RPG)
			{
				// j is only needed for the hit type mask.
				rpgexplode(proj, coll.type, oldpos, EXPLOSION2, EXPLOSION2BOT, -1, RPG_EXPLODE);
			}
			else if (proj->spr.picnum == SHRINKSPARK)
			{
				spawn(proj, SHRINKEREXPLOSION);
				S_PlayActorSound(SHRINKER_HIT, proj);
				fi.hitradius(proj, gs.shrinkerblastradius, 0, 0, 0, 0);
			}
			else if (proj->spr.picnum != COOLEXPLOSION1 && proj->spr.picnum != FREEZEBLAST && proj->spr.picnum != FIRELASER && (!isWorldTour() || proj->spr.picnum != FIREBALL))
			{
				auto k = spawn(proj, EXPLOSION2);
				if (k)
				{
					k->spr.xrepeat = k->spr.yrepeat = proj->spr.xrepeat >> 1;
					if (coll.type == kHitSector)
					{
						if (proj->spr.zvel < 0)
						{
							k->spr.cstat |= CSTAT_SPRITE_YFLIP; k->spr.z += (72 << 8);
						}
					}
				}
			}
			if (fireball)
			{
				auto spawned = spawn(proj, EXPLOSION2);
				if (spawned) spawned->spr.xrepeat = spawned->spr.yrepeat = (short)(proj->spr.xrepeat >> 1);
			}
		}
		if (proj->spr.picnum != COOLEXPLOSION1)
		{
			deletesprite(proj);
			return;
		}
	}
	if (proj->spr.picnum == COOLEXPLOSION1)
	{
		proj->spr.shade++;
		if (proj->spr.shade >= 40)
		{
			deletesprite(proj);
			return;
		}
	}
	else if (proj->spr.picnum == RPG && proj->spr.sector()->lotag == 2 && proj->spr.xrepeat >= 10 && rnd(140))
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
		if (!act->insector())
		{
			deletesprite(act);
			continue;
		}


		switch(act->spr.picnum)
		{
		case RADIUSEXPLOSION:
		case KNEE:
			deletesprite(act);
			continue;
		case TONGUE:
			movetongue(act, TONGUE, INNERJAW);
			continue;

		case FREEZEBLAST:
			if (act->spr.yvel < 1 || act->spr.extra < 2 || (act->spr.xvel|act->spr.zvel) == 0)
			{
				auto spawned = spawn(act,TRANSPORTERSTAR);
				if (spawned)
				{
					spawned->spr.pal = 1;
					spawned->spr.xrepeat = 32;
					spawned->spr.yrepeat = 32;
				}
				deletesprite(act);
				continue;
			}
			[[fallthrough]];
		case FIREBALL:
			// Twentieth Anniversary World Tour
			if (act->spr.picnum == FIREBALL && !isWorldTour()) break;
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
		auto Owner = act->GetOwner();
		
		if (Owner == act)
		{
			continue;
		}

		auto sectp = act->spr.sector();
		int sectlotag = sectp->lotag;
		int onfloorz = act->temp_data[4];
		
		if (act->temp_data[0] > 0) act->temp_data[0]--;
		
		DukeSectIterator itj(act->sector());
		while (auto act2 = itj.Next()) 
		{
			switch (act2->spr.statnum)
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
							if (act->spr.pal == 0)
							{
								spawn(act, TRANSPORTERBEAM);
								S_PlayActorSound(TELEPORTER, act);
							}
							
							for (int k = connecthead; k >= 0; k = connectpoint2[k])
							if (ps[k].cursector == Owner->sector())
							{
								ps[k].frag_ps = p;
								ps[k].GetActor()->spr.extra = 0;
							}
							
							ps[p].angle.ang = buildang(Owner->spr.ang);
							
							if (Owner->GetOwner() != Owner)
							{
								act->temp_data[0] = 13;
								Owner->temp_data[0] = 13;
								ps[p].transporter_hold = 13;
							}
							
							ps[p].bobposx = ps[p].oposx = ps[p].pos.X = Owner->spr.x;
							ps[p].bobposy = ps[p].oposy = ps[p].pos.Y = Owner->spr.y;
							ps[p].oposz = ps[p].pos.z = Owner->spr.z - gs.playerheight;
							
							ChangeActorSect(act2, Owner->sector());
							ps[p].setCursector(act2->spr.sector());
							
							if (act->spr.pal == 0)
							{
								auto k = spawn(Owner, TRANSPORTERBEAM);
								if (k) S_PlayActorSound(TELEPORTER, k);
							}
							
							break;
						}
					}
					else if (!(sectlotag == 1 && ps[p].on_ground == 1)) break;
					
					if (onfloorz == 0 && abs(act->spr.z - ps[p].pos.z) < 6144)
						if ((ps[p].jetpack_on == 0) || (ps[p].jetpack_on && (PlayerInput(p, SB_JUMP))) ||
							(ps[p].jetpack_on && PlayerInput(p, SB_CROUCH)))
						{
							ps[p].oposx = ps[p].pos.X += Owner->spr.x - act->spr.x;
							ps[p].oposy = ps[p].pos.Y += Owner->spr.y - act->spr.y;
							
							if (ps[p].jetpack_on && (PlayerInput(p, SB_JUMP) || ps[p].jetpack_on < 11))
								ps[p].pos.z = Owner->spr.z - 6144;
							else ps[p].pos.z = Owner->spr.z + 6144;
							ps[p].oposz = ps[p].pos.z;
							
							auto pa = ps[p].GetActor();
							pa->spr.opos = ps[p].pos;
							
							ChangeActorSect(act2, Owner->sector());
							ps[p].setCursector(Owner->sector());
							
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
						if (ps[p].GetActor()->spr.extra > 0)
							S_PlayActorSound(DUKE_UNDERWATER, act2);
						ps[p].oposz = ps[p].pos.z =
						Owner->sector()->ceilingz + (7 << 8);
						
						ps[p].posxv = 4096 - (krand() & 8192);
						ps[p].posyv = 4096 - (krand() & 8192);
						
					}
					
					if (onfloorz && sectlotag == ST_2_UNDERWATER && ps[p].pos.z < (sectp->ceilingz + (6 << 8)))
					{
						k = 1;
						//     if( act2->spr.extra <= 0) break;
						if (screenpeek == p)
						{
							FX_StopAllSounds();
						}
						S_PlayActorSound(DUKE_GASP, act2);
						
						ps[p].oposz = ps[p].pos.z =
						Owner->sector()->floorz - (7 << 8);
						
						ps[p].jumping_toggle = 1;
						ps[p].jumping_counter = 0;
					}
					
					if (k == 1)
					{
						ps[p].oposx = ps[p].pos.X += Owner->spr.x - act->spr.x;
						ps[p].oposy = ps[p].pos.Y += Owner->spr.y - act->spr.y;
						
						if (!Owner || Owner->GetOwner() != Owner)
							ps[p].transporter_hold = -2;
						ps[p].setCursector(Owner->sector());

						ChangeActorSect(act2, Owner->sector());
						SetActor(ps[p].GetActor(), { ps[p].pos.X, ps[p].pos.Y, ps[p].pos.z + gs.playerheight });
						
						if ((krand() & 255) < 32)
							spawn(act2, WATERSPLASH2);
						
						if (sectlotag == 1)
							for (int l = 0; l < 9; l++)
						{
							auto q = spawn(ps[p].GetActor(), WATERBUBBLE);
							if (q) q->spr.z += krand() & 16383;
						}
					}
				}
				break;
				
			case STAT_ACTOR:
				switch (act2->spr.picnum)
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
					if (act2->spr.extra > 0)
						continue;
				}
				[[fallthrough]];
			case STAT_PROJECTILE:
			case STAT_MISC:
			case STAT_FALLER:
			case STAT_DUMMYPLAYER:
				
				ll = abs(act2->spr.zvel);
				
				{
					warpspriteto = 0;
					if (ll && sectlotag == 2 && act2->spr.z < (sectp->ceilingz + ll))
						warpspriteto = 1;
					
					if (ll && sectlotag == 1 && act2->spr.z > (sectp->floorz - ll))
						warpspriteto = 1;
					
					if (sectlotag == 0 && (onfloorz || abs(act2->spr.z - act->spr.z) < 4096))
					{
						if ((!Owner || Owner->GetOwner() != Owner) && onfloorz && act->temp_data[0] > 0 && act2->spr.statnum != STAT_MISC)
						{
							act->temp_data[0]++;
							goto BOLT;
						}
						warpspriteto = 1;
					}
					
					if (warpspriteto) switch (act2->spr.picnum)
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
							act2->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;
							break;
						}
						[[fallthrough]];
					default:
						if (act2->spr.statnum == 5 && !(sectlotag == 1 || sectlotag == 2))
							break;
						[[fallthrough]];

					case WATERBUBBLE:
						//if( rnd(192) && a2->s.picnum == WATERBUBBLE)
						// break;
						
						if (sectlotag > 0)
						{
							auto k = spawn(act2, WATERSPLASH2);
							if (k && sectlotag == 1 && act2->spr.statnum == 4)
							{
								k->spr.xvel = act2->spr.xvel >> 1;
								k->spr.ang = act2->spr.ang;
								ssp(k, CLIPMASK0);
							}
						}
						
						switch (sectlotag)
						{
						case 0:
							if (onfloorz)
							{
								if (act2->spr.statnum == STAT_PROJECTILE || (checkcursectnums(act->spr.sector()) == -1 && checkcursectnums(Owner->spr.sector()) == -1))
								{
									act2->spr.x += (Owner->spr.x - act->spr.x);
									act2->spr.y += (Owner->spr.y - act->spr.y);
									act2->spr.z -= act->spr.z - Owner->sector()->floorz;
									act2->spr.ang = Owner->spr.ang;
									
									act2->spr.backupang();
									
									if (act->spr.pal == 0)
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
									
									ChangeActorSect(act2, Owner->sector());
								}
							}
							else
							{
								act2->spr.x += (Owner->spr.x - act->spr.x);
								act2->spr.y += (Owner->spr.y - act->spr.y);
								act2->spr.z = Owner->spr.z + 4096;
								
								act2->spr.backupz();
								
								ChangeActorSect(act2, Owner->sector());
							}
							break;
						case 1:
							act2->spr.x += (Owner->spr.x - act->spr.x);
							act2->spr.y += (Owner->spr.y - act->spr.y);
							act2->spr.z = Owner->sector()->ceilingz + ll;
							
							act2->spr.backupz();
							
							ChangeActorSect(act2, Owner->sector());
							
							break;
						case 2:
							act2->spr.x += (Owner->spr.x - act->spr.x);
							act2->spr.y += (Owner->spr.y - act->spr.y);
							act2->spr.z = Owner->sector()->floorz - ll;
							
							act2->spr.backupz();
							
							ChangeActorSect(act2, Owner->sector());
							
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
	int* t = &actor->temp_data[0];
	auto sectp = actor->spr.sector();
	int j;

	// #ifndef isShareware()
	if (ud.multimode < 2)
	{
		if (actor_tog == 1)
		{
			actor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
			return;
		}
		else if (actor_tog == 2) actor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
	}
	// #endif

	t[1] += 128;

	if (sectp->floorstat & CSTAT_SECTOR_SKY)
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
			ChangeActorStat(actor, 2);
			return;
		}
	}

	if (t[0] == -5) // FROZEN
	{
		t[3]++;
		if (t[3] > 280)
		{
			actor->spr.pal = 0;
			t[0] = 0;
			return;
		}
		makeitfall(actor);
		actor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
		actor->spr.picnum = GREENSLIME + 2;
		actor->spr.extra = 1;
		actor->spr.pal = 1;
		j = fi.ifhitbyweapon(actor);
		if (j >= 0)
		{
			if (j == FREEZEBLAST)
				return;
			for (j = 16; j >= 0; j--)
			{
				auto k = EGS(actor->spr.sector(), actor->spr.x, actor->spr.y, actor->spr.z, GLASSPIECES + (j % 3), -32, 36, 36, krand() & 2047, 32 + (krand() & 63), 1024 - (krand() & 1023), actor, 5);
				k->spr.pal = 1;
			}
			ps[p].actors_killed++;
			S_PlayActorSound(GLASS_BREAKING, actor);
			deletesprite(actor);
		}
		else if (x < 1024 && ps[p].quick_kick == 0)
		{
			j = getincangle(ps[p].angle.ang.asbuild(), getangle(actor->spr.x - ps[p].pos.X, actor->spr.y - ps[p].pos.Y));
			if (j > -128 && j < 128)
				ps[p].quick_kick = 14;
		}

		return;
	}

	if (x < 1596)
		actor->spr.cstat = 0;
	else actor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;

	if (t[0] == -4) //On the player
	{
		if (ps[p].GetActor()->spr.extra < 1)
		{
			t[0] = 0;
			return;
		}

		SetActor(actor, actor->spr.pos);

		actor->spr.ang = ps[p].angle.ang.asbuild();

		if ((PlayerInput(p, SB_FIRE) || (ps[p].quick_kick > 0)) && ps[p].GetActor()->spr.extra > 0)
			if (ps[p].quick_kick > 0 || (ps[p].curr_weapon != HANDREMOTE_WEAPON && ps[p].curr_weapon != HANDBOMB_WEAPON && ps[p].curr_weapon != TRIPBOMB_WEAPON && ps[p].ammo_amount[ps[p].curr_weapon] >= 0))
			{
				for (x = 0; x < 8; x++)
				{
					auto j = EGS(actor->spr.sector(), actor->spr.x, actor->spr.y, actor->spr.z - (8 << 8), SCRAP3 + (krand() & 3), -8, 48, 48, krand() & 2047, (krand() & 63) + 64, -(krand() & 4095) - (actor->spr.zvel >> 2), actor, 5);
					j->spr.pal = 6;
				}

				S_PlayActorSound(SLIM_DYING, actor);
				S_PlayActorSound(SQUISHED, actor);
				if ((krand() & 255) < 32)
				{
					auto j = spawn(actor, BLOODPOOL);
					if (j) j->spr.pal = 0;
				}
				ps[p].actors_killed++;
				t[0] = -3;
				if (ps[p].somethingonplayer == actor)
					ps[p].somethingonplayer = nullptr;
				deletesprite(actor);
				return;
			}

		actor->spr.z = ps[p].pos.z + ps[p].pyoff - t[2] + (8 << 8);

		actor->spr.z += -ps[p].horizon.horiz.asq16() >> 12;

		if (t[2] > 512)
			t[2] -= 128;

		if (t[2] < 348)
			t[2] += 128;

		if (ps[p].newOwner != nullptr)
		{
			ps[p].newOwner = nullptr;
			ps[p].pos.X = ps[p].oposx;
			ps[p].pos.Y = ps[p].oposy;
			ps[p].pos.z = ps[p].oposz;
			ps[p].angle.restore();

			updatesector(ps[p].pos.X, ps[p].pos.Y, &ps[p].cursector);

			DukeStatIterator it(STAT_ACTOR);
			while (auto ac = it.Next())
			{
				if (ac->spr.picnum == CAMERA1) ac->spr.yvel = 0;
			}
		}

		if (t[3] > 0)
		{
			static const uint8_t frames[] = { 5,5,6,6,7,7,6,5 };

			actor->spr.picnum = GREENSLIME + frames[t[3]];

			if (t[3] == 5)
			{
				auto psp = ps[p].GetActor();
				psp->spr.extra += -(5 + (krand() & 3));
				S_PlayActorSound(SLIM_ATTACK, actor);
			}

			if (t[3] < 7) t[3]++;
			else t[3] = 0;

		}
		else
		{
			actor->spr.picnum = GREENSLIME + 5;
			if (rnd(32))
				t[3] = 1;
		}

		actor->spr.xrepeat = 20 + bsin(t[1], -13);
		actor->spr.yrepeat = 15 + bsin(t[1], -13);

		actor->spr.x = ps[p].pos.X + ps[p].angle.ang.bcos(-7);
		actor->spr.y = ps[p].pos.Y + ps[p].angle.ang.bsin(-7);

		return;
	}

	else if (actor->spr.xvel < 64 && x < 768)
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
			if (j) j->spr.pal = 0;
		}

		for (x = 0; x < 8; x++)
		{
			auto j = EGS(actor->spr.sector(), actor->spr.x, actor->spr.y, actor->spr.z - (8 << 8), SCRAP3 + (krand() & 3), -8, 48, 48, krand() & 2047, (krand() & 63) + 64, -(krand() & 4095) - (actor->spr.zvel >> 2), actor, 5);
			if (j) j->spr.pal = 6;
		}
		t[0] = -3;
		deletesprite(actor);
		return;
	}
	// All weap
	if (t[0] == -1) //Shrinking down
	{
		makeitfall(actor);

		actor->spr.cstat &= ~CSTAT_SPRITE_YFLIP;
		actor->spr.picnum = GREENSLIME + 4;

		if (actor->spr.xrepeat > 32) actor->spr.xrepeat -= krand() & 7;
		if (actor->spr.yrepeat > 16) actor->spr.yrepeat -= krand() & 7;
		else
		{
			actor->spr.xrepeat = 40;
			actor->spr.yrepeat = 16;
			actor->temp_actor = nullptr;
			t[0] = 0;
		}

		return;
	}
	else if (t[0] != -2) getglobalz(actor);

	if (t[0] == -2) //On top of somebody (an enemy)
	{
		DDukeActor* s5 = actor->temp_actor;
		makeitfall(actor);
		if (s5)
		{
			s5->spr.xvel = 0;

			int l = s5->spr.ang;

			actor->spr.z = s5->spr.z;
			actor->spr.x = s5->spr.x + bcos(l, -11);
			actor->spr.y = s5->spr.y + bsin(l, -11);

			actor->spr.picnum = GREENSLIME + 2 + (global_random & 1);

			if (actor->spr.yrepeat < 64) actor->spr.yrepeat += 2;
			else
			{
				if (actor->spr.xrepeat < 32) actor->spr.xrepeat += 4;
				else
				{
					t[0] = -1;
					x = ldist(actor, s5);
					if (x < 768) {
						s5->spr.xrepeat = 0;
					}
				}
			}
		}
		return;
	}

	//Check randomly to see of there is an actor near
	if (rnd(32))
	{
		DukeSectIterator it(actor->sector());
		while (auto a2 = it.Next())
		{
			if (gs.actorinfo[a2->spr.picnum].flags & SFLAG_GREENSLIMEFOOD)
			{
				if (ldist(actor, a2) < 768 && (abs(actor->spr.z - a2->spr.z) < 8192)) //Gulp them
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
		actor->spr.picnum = GREENSLIME;

		if ((krand() & 511) == 0)
			S_PlayActorSound(SLIM_ROAM, actor);

		if (t[0] == 2)
		{
			actor->spr.zvel = 0;
			actor->spr.cstat &= ~CSTAT_SPRITE_YFLIP;

			if ((sectp->ceilingstat & CSTAT_SECTOR_SKY) || (actor->ceilingz + 6144) < actor->spr.z)
			{
				actor->spr.z += 2048;
				t[0] = 3;
				return;
			}
		}
		else
		{
			actor->spr.cstat |= CSTAT_SPRITE_YFLIP;
			makeitfall(actor);
		}

		if (everyothertime & 1) ssp(actor, CLIPMASK0);

		if (actor->spr.xvel > 96)
		{
			actor->spr.xvel -= 2;
			return;
		}
		else
		{
			if (actor->spr.xvel < 32) actor->spr.xvel += 4;
			actor->spr.xvel = 64 - bcos(t[1], -9);

			actor->spr.ang += getincangle(actor->spr.ang,
				getangle(ps[p].pos.X - actor->spr.x, ps[p].pos.Y - actor->spr.y)) >> 3;
			// TJR
		}

		actor->spr.xrepeat = 36 + bcos(t[1], -11);
		actor->spr.yrepeat = 16 + bsin(t[1], -13);

		if (rnd(4) && (sectp->ceilingstat & CSTAT_SECTOR_SKY) == 0 &&
			abs(actor->floorz - actor->ceilingz)
			< (192 << 8))
		{
			actor->spr.zvel = 0;
			t[0]++;
		}

	}

	if (t[0] == 1)
	{
		actor->spr.picnum = GREENSLIME;
		if (actor->spr.yrepeat < 40) actor->spr.yrepeat += 8;
		if (actor->spr.xrepeat > 8) actor->spr.xrepeat -= 4;
		if (actor->spr.zvel > -(2048 + 1024))
			actor->spr.zvel -= 348;
		actor->spr.z += actor->spr.zvel;
		if (actor->spr.z < actor->ceilingz + 4096)
		{
			actor->spr.z = actor->ceilingz + 4096;
			actor->spr.xvel = 0;
			t[0] = 2;
		}
	}

	if (t[0] == 3)
	{
		actor->spr.picnum = GREENSLIME + 1;

		makeitfall(actor);

		if (actor->spr.z > actor->floorz - (8 << 8))
		{
			actor->spr.yrepeat -= 4;
			actor->spr.xrepeat += 2;
		}
		else
		{
			if (actor->spr.yrepeat < (40 - 4)) actor->spr.yrepeat += 8;
			if (actor->spr.xrepeat > 8) actor->spr.xrepeat -= 4;
		}

		if (actor->spr.z > actor->floorz - 2048)
		{
			actor->spr.z = actor->floorz - 2048;
			t[0] = 0;
			actor->spr.xvel = 0;
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
	int* t = &actor->temp_data[0];
	auto sectp = actor->spr.sector();
	int x;
	int p = findplayer(actor, &x);
	execute(actor, p, x);
	t[0]++;
	if (sectp->lotag == 2)
	{
		spawn(actor, EXPLOSION2)->spr.shade = 127;
		deletesprite(actor);
		return;
	}

	int dax = actor->spr.x;
	int day = actor->spr.y;
	int daz = actor->spr.z;
	int xvel = actor->spr.xvel;

	getglobalz(actor);

	int ds = t[0] / 6;
	if (actor->spr.xrepeat < 80)
		actor->spr.yrepeat = actor->spr.xrepeat += ds;
	actor->spr.clipdist += ds;
	if (t[0] <= 2)
		t[3] = krand() % 10;
	if (t[0] > 30) 
	{
		spawn(actor, EXPLOSION2)->spr.shade = 127;
		deletesprite(actor);
		return;
	}

	Collision coll;
	movesprite_ex(actor, MulScale(xvel, bcos(actor->spr.ang), 14),
		MulScale(xvel, bsin(actor->spr.ang), 14), actor->spr.zvel, CLIPMASK1, coll);

	if (!actor->spr.insector())
	{
		deletesprite(actor);
		return;
	}

	if (coll.type != kHitSprite)
	{
		if (actor->spr.z < actor->ceilingz)
		{
			coll.setSector(actor->spr.sector());
			actor->spr.zvel = -1;
		}
		else if ((actor->spr.z > actor->floorz && actor->spr.sector()->lotag != 1)
			|| (actor->spr.z > actor->floorz + (16 << 8) && actor->spr.sector()->lotag == 1))
		{
			coll.setSector(actor->spr.sector());
			if (actor->spr.sector()->lotag != 1)
				actor->spr.zvel = 1;
		}
	}

	if (coll.type != 0) {
		actor->spr.xvel = actor->spr.yvel = actor->spr.zvel = 0;
		if (coll.type == kHitSprite)
		{
			fi.checkhitsprite(coll.actor(), actor);
			if (coll.actor()->spr.picnum == APLAYER)
				S_PlayActorSound(PISTOL_BODYHIT, coll.actor());
		}
		else if (coll.type == kHitWall)
		{
			SetActor(actor, { dax, day, daz });
			fi.checkhitwall(actor, coll.hitWall, actor->spr.x, actor->spr.y, actor->spr.z, actor->spr.picnum);
		}
		else if (coll.type == kHitSector)
		{
			SetActor(actor, { dax, day, daz });
			if (actor->spr.zvel < 0)
				fi.checkhitceiling(actor->spr.sector());
		}

		if (actor->spr.xrepeat >= 10)
		{
			x = actor->spr.extra;
			fi.hitradius(actor, gs.rpgblastradius, x >> 2, x >> 1, x - (x >> 2), x);
		}
		else
		{
			x = actor->spr.extra + (global_random & 3);
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
	auto t = &actor->temp_data[0];
	auto Owner = actor->GetOwner();
	auto sectp = actor->spr.sector();
	int x, l;

	if ((actor->spr.cstat & CSTAT_SPRITE_INVISIBLE))
	{
		t[2]--;
		if (t[2] <= 0)
		{
			S_PlayActorSound(TELEPORTER, actor);
			spawn(actor, TRANSPORTERSTAR);
			actor->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
		}
		return;
	}

	int p = findplayer(actor, &x);

	if (x < 1220) actor->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
	else actor->spr.cstat |= CSTAT_SPRITE_BLOCK_ALL;

	if (t[3] == 0)
	{
		int j = fi.ifhitbyweapon(actor);
		if (j >= 0)
		{
			t[3] = 1;
			t[4] = 0;
			l = 0;
			actor->spr.xvel = 0;
			goto DETONATEB;
		}
	}

	if (actor->spr.picnum != BOUNCEMINE)
	{
		makeitfall(actor);

		if (sectp->lotag != 1 && actor->spr.z >= actor->floorz - (FOURSLEIGHT) && actor->spr.yvel < 3)
		{
			if (actor->spr.yvel > 0 || (actor->spr.yvel == 0 && actor->floorz == sectp->floorz))
				S_PlayActorSound(PIPEBOMB_BOUNCE, actor);
			actor->spr.zvel = -((4 - actor->spr.yvel) << 8);
			if (actor->spr.sector()->lotag == 2)
				actor->spr.zvel >>= 2;
			actor->spr.yvel++;
		}
		if (actor->spr.z < actor->ceilingz) // && sectp->lotag != 2 )
		{
			actor->spr.z = actor->ceilingz + (3 << 8);
			actor->spr.zvel = 0;
		}
	}

	Collision coll;
	movesprite_ex(actor,
		MulScale(actor->spr.xvel, bcos(actor->spr.ang), 14),
		MulScale(actor->spr.xvel, bsin(actor->spr.ang), 14),
		actor->spr.zvel, CLIPMASK0, coll);

	if (actor->spr.sector()->lotag == 1 && actor->spr.zvel == 0)
	{
		actor->spr.z += (32 << 8);
		if (t[5] == 0)
		{
			t[5] = 1;
			spawn(actor, WATERSPLASH2);
		}
	}
	else t[5] = 0;

	if (t[3] == 0 && (actor->spr.picnum == BOUNCEMINE || actor->spr.picnum == MORTER) && (coll.type || x < 844))
	{
		t[3] = 1;
		t[4] = 0;
		l = 0;
		actor->spr.xvel = 0;
		goto DETONATEB;
	}

	if ( Owner && Owner->spr.picnum == APLAYER)
		l = Owner->PlayerIndex();
	else l = -1;

	if (actor->spr.xvel > 0)
	{
		actor->spr.xvel -= 5;
		if (sectp->lotag == 2)
			actor->spr.xvel -= 10;

		if (actor->spr.xvel < 0)
			actor->spr.xvel = 0;
		if (actor->spr.xvel & 8) actor->spr.cstat ^= CSTAT_SPRITE_XFLIP;
	}

	if (coll.type== kHitWall)
	{
		auto wal = coll.hitWall;
		fi.checkhitwall(actor, wal, actor->spr.x, actor->spr.y, actor->spr.z, actor->spr.picnum);

		int k = getangle(wal->delta());

		actor->spr.ang = ((k << 1) - actor->spr.ang) & 2047;
		actor->spr.xvel >>= 1;
	}

DETONATEB:

	bool bBoom = false;
	if ((l >= 0 && ps[l].hbomb_on == 0) || t[3] == 1)
		bBoom = true;
	if (isNamWW2GI() && actor->spr.picnum == HEAVYHBOMB)
	{
		actor->spr.extra--;
		if (actor->spr.extra <= 0)
			bBoom = true;
	}
	if (bBoom)
	{
		t[4]++;

		if (t[4] == 2)
		{
			x = actor->spr.extra;
			int m = 0;
			switch (actor->spr.picnum)
			{
			case HEAVYHBOMB: m = gs.pipebombblastradius; break;
			case MORTER: m = gs.morterblastradius; break;
			case BOUNCEMINE: m = gs.bouncemineblastradius; break;
			}

			fi.hitradius(actor, m, x >> 2, x >> 1, x - (x >> 2), x);
			spawn(actor, EXPLOSION2);
			if (actor->spr.zvel == 0)	spawn(actor, EXPLOSION2BOT);
			S_PlayActorSound(PIPEBOMB_EXPLODE, actor);
			for (x = 0; x < 8; x++)
				RANDOMSCRAP(actor);
		}

		if (actor->spr.yrepeat)
		{
			actor->spr.yrepeat = 0;
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
				actor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
				actor->spr.yrepeat = 9;
				return;
			}
		}
	}
	else if (actor->spr.picnum == HEAVYHBOMB && x < 788 && t[0] > 7 && actor->spr.xvel == 0)
		if (cansee(actor->spr.x, actor->spr.y, actor->spr.z - (8 << 8), actor->spr.sector(), ps[p].pos.X, ps[p].pos.Y, ps[p].pos.z, ps[p].cursector))
			if (ps[p].ammo_amount[HANDBOMB_WEAPON] < gs.max_ammo_amount[HANDBOMB_WEAPON])
			{
				if (ud.coop >= 1 && Owner == actor)
				{
					for (int j = 0; j < ps[p].weapreccnt; j++)
						if (ps[p].weaprecs[j] == actor->spr.picnum)
							continue;

					if (ps[p].weapreccnt < 255) // DukeGDX has 16 here.
						ps[p].weaprecs[ps[p].weapreccnt++] = actor->spr.picnum;
				}

				addammo(HANDBOMB_WEAPON, &ps[p], 1);
				S_PlayActorSound(DUKE_GET, ps[p].GetActor());

				if (ps[p].gotweapon[HANDBOMB_WEAPON] == 0 || Owner == ps[p].GetActor())
					fi.addweapon(&ps[p], HANDBOMB_WEAPON);

				if (!Owner || Owner->spr.picnum != APLAYER)
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
					actor->spr.cstat = CSTAT_SPRITE_INVISIBLE;
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
	int p;
	unsigned int k;
	
	DukeStatIterator it(STAT_ACTOR);
	while (auto act = it.Next())
	{
		auto sectp = act->spr.sector();

		if (act->spr.xrepeat == 0 || sectp == nullptr)
		{ 
			deletesprite(act);
			continue;
		}

		int *t = &act->temp_data[0];


		switch (act->spr.picnum)
		{
		case FLAMETHROWERFLAME:
			if (isWorldTour()) flamethrowerflame(act);
			continue;

		case DUCK:
		case TARGET:
			if (act->spr.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR)
			{
				t[0]++;
				if (t[0] > 60)
				{
					t[0] = 0;
					act->spr.cstat = CSTAT_SPRITE_YCENTER | CSTAT_SPRITE_BLOCK_ALL | CSTAT_SPRITE_ALIGNMENT_WALL;
					act->spr.extra = 1;
				}
			}
			else
			{
				int j = fi.ifhitbyweapon(act);
				if (j >= 0)
				{
					act->spr.cstat = CSTAT_SPRITE_ALIGNMENT_FLOOR | CSTAT_SPRITE_YCENTER;
					k = 1;

					DukeStatIterator it(STAT_ACTOR);
					while (auto act2 = it.Next())
					{
						if (act2->spr.lotag == act->spr.lotag &&
							act2->spr.picnum == act->spr.picnum)
						{
							if ((act2->spr.hitag && !(act2->spr.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR)) ||
								(!act2->spr.hitag && (act2->spr.cstat & CSTAT_SPRITE_ALIGNMENT_FLOOR))
								)
							{
								k = 0;
								break;
							}
						}
					}

					if (k == 1)
					{
						operateactivators(act->spr.lotag, -1);
						fi.operateforcefields(act, act->spr.lotag);
						operatemasterswitches(act->spr.lotag);
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

			act->spr.z += act->spr.zvel;
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
				act->spr.cstat = CSTAT_SPRITE_INVISIBLE;
				continue;
			}
			else if (actor_tog == 2) act->spr.cstat = CSTAT_SPRITE_BLOCK_ALL;
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
	if (!Owner || Owner->spr.picnum != FIREFLY) 
	{
		deletesprite(actor);
		return;
	}

	if (Owner->spr.xrepeat >= 24 || Owner->spr.pal == 1)
		actor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
	else
		actor->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;

	double dx = Owner->spr.x - ps[p].GetActor()->spr.x;
	double dy = Owner->spr.y - ps[p].GetActor()->spr.y;
	double dist = sqrt(dx * dx + dy * dy);
	if (dist != 0.0) 
	{
		dx /= dist;
		dy /= dist;
	}

	actor->spr.x = (int) (Owner->spr.x - (dx * -10.0));
	actor->spr.y = (int) (Owner->spr.y - (dy * -10.0));
	actor->spr.z = Owner->spr.z + 2048;

	if (Owner->spr.extra <= 0) 
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
		if (!act->spr.insector() || act->spr.xrepeat == 0) 
		{
			deletesprite(act);
			continue;
		}

		int* t = &act->temp_data[0];
		auto sectp = act->spr.sector();

		switch (act->spr.picnum)
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

			if ((global_random / (act->spr.lotag + 1) & 31) > 4) act->spr.shade = -127;
			else act->spr.shade = 127;
			continue;

		case BLOODSPLAT1:
		case BLOODSPLAT2:
		case BLOODSPLAT3:
		case BLOODSPLAT4:

			if (t[0] == 7 * 26) continue;
			act->spr.z += 16 + (krand() & 15);
			t[0]++;
			if ((t[0] % 9) == 0) act->spr.yrepeat++;
			continue;

		case NUKEBUTTON:
		case NUKEBUTTON + 1:
		case NUKEBUTTON + 2:
		case NUKEBUTTON + 3:

			if (t[0])
			{
				t[0]++;
				auto Owner = act->GetOwner();
				if (t[0] == 8) act->spr.picnum = NUKEBUTTON + 1;
				else if (t[0] == 16 && Owner)
				{
					act->spr.picnum = NUKEBUTTON + 2;
					ps[Owner->PlayerIndex()].fist_incs = 1;
				}
				if (Owner && ps[Owner->PlayerIndex()].fist_incs == 26)
					act->spr.picnum = NUKEBUTTON + 3;
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
				ps[p].GetActor()->spr.extra -= 4;
			}
			[[fallthrough]];

		case FIRELASER:
			if (act->spr.extra != 999)
				act->spr.extra = 999;
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
			act->floorz = act->spr.z = getflorzofslopeptr(act->spr.sector(), act->spr.x, act->spr.y);
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
			jibs(act, JIBS6, true, false, false, act->spr.picnum == DUKELEG || act->spr.picnum == DUKETORSO || act->spr.picnum == DUKEGUN, false);

			continue;
		case BLOODPOOL:
		case PUKE:
			bloodpool(act, act->spr.picnum == PUKE, TIRE);

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
			shell(act, (sectp->floorz + (24 << 8)) < act->spr.z);
			continue;

		case GLASSPIECES:
		case GLASSPIECES + 1:
		case GLASSPIECES + 2:
			glasspieces(act);
			continue;
		}

		if (act->spr.picnum >= SCRAP6 && act->spr.picnum <= SCRAP5 + 3)
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
	auto t = &actor->temp_data[0];

	auto sc = actor->sector();
	int sh = actor->spr.hitag;

	int k = sc->extra;

	if (t[4] > 0)
	{
		t[4]--;
		if (t[4] >= (k - (k >> 3)))
			actor->spr.xvel -= (k >> 5);
		if (t[4] > ((k >> 1) - 1) && t[4] < (k - (k >> 3)))
			actor->spr.xvel = 0;
		if (t[4] < (k >> 1))
			actor->spr.xvel += (k >> 5);
		if (t[4] < ((k >> 1) - (k >> 3)))
		{
			t[4] = 0;
			actor->spr.xvel = k;
		}
	}
	else actor->spr.xvel = k;

	DukeStatIterator it(STAT_EFFECTOR);
	while (auto act2 = it.Next())
	{
		if ((act2->spr.lotag == 14) && (sh == act2->spr.hitag) && (act2->temp_data[0] == t[0]))
		{
			act2->spr.xvel = actor->spr.xvel;
			//if( t[4] == 1 )
			{
				if (act2->temp_data[5] == 0)
					act2->temp_data[5] = dist(act2, actor);
				int x = Sgn(dist(act2, actor) - act2->temp_data[5]);
				if (act2->spr.extra)
					x = -x;
				actor->spr.xvel += x;
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
				if (act2->spr.picnum == NATURALLIGHTNING && act2->spr.hitag == actor->spr.hitag)
					act2->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
			}
		}
		else if (t[2] > (t[1] >> 3) && t[2] < (t[1] >> 2))
		{
			int j = !!cansee(actor->spr.x, actor->spr.y, actor->spr.z, actor->spr.sector(), ps[screenpeek].pos.X, ps[screenpeek].pos.Y, ps[screenpeek].pos.z, ps[screenpeek].cursector);

			if (rnd(192) && (t[2] & 1))
			{
				if (j) ps[screenpeek].visibility = 0;
			}
			else if (j)	ps[screenpeek].visibility = ud.const_visibility;

			DukeStatIterator it(STAT_DEFAULT);
			while (auto act2 = it.Next())
			{
				if (act2->spr.picnum == NATURALLIGHTNING && act2->spr.hitag == actor->spr.hitag)
				{
					if (rnd(32) && (t[2] & 1))
					{
						act2->spr.cstat &= ~CSTAT_SPRITE_INVISIBLE;
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
							psa->spr.extra -= 8 + (krand() & 7);
							SetPlayerPal(&ps[p], PalEntry(32, 16, 0, 0));
						}
						return;
					}
					else act2->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
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
		auto sc = act->sector();
		switch (act->spr.lotag)
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
			act->spr.hitag += 64;
			l = MulScale(act->spr.yvel, bsin(act->spr.hitag), 12);
			sc->floorz = act->spr.z + l;
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
		if (act->spr.lotag != SE_29_WAVES) continue;
		auto sc = act->sector();
		if (sc->wallnum != 4) continue;
		auto wal = sc->firstWall() + 2;
		alignflorslope(act->spr.sector(), wal->x, wal->y, wal->nextSector()->floorz);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void move_d(DDukeActor *actor, int playernum, int xvel)
{
	auto t = actor->temp_data;
	int l;
	int goalang, angdif;
	int daxvel;

	int a = actor->spr.hitag;

	if (a == -1) a = 0;

	t[0]++;

	if (a & face_player)
	{
		if (ps[playernum].newOwner != nullptr)
			goalang = getangle(ps[playernum].oposx - actor->spr.x, ps[playernum].oposy - actor->spr.y);
		else goalang = getangle(ps[playernum].pos.X - actor->spr.x, ps[playernum].pos.Y - actor->spr.y);
		angdif = getincangle(actor->spr.ang, goalang) >> 2;
		if (angdif > -8 && angdif < 0) angdif = 0;
		actor->spr.ang += angdif;
	}

	if (a & spin)
		actor->spr.ang += bsin(t[0] << 3, -6);

	if (a & face_player_slow)
	{
		if (ps[playernum].newOwner != nullptr)
			goalang = getangle(ps[playernum].oposx - actor->spr.x, ps[playernum].oposy - actor->spr.y);
		else goalang = getangle(ps[playernum].pos.X - actor->spr.x, ps[playernum].pos.Y - actor->spr.y);
		angdif = Sgn(getincangle(actor->spr.ang, goalang)) << 5;
		if (angdif > -32 && angdif < 0)
		{
			angdif = 0;
			actor->spr.ang = goalang;
		}
		actor->spr.ang += angdif;
	}


	if ((a & jumptoplayer) == jumptoplayer)
	{
		if (t[0] < 16)
			actor->spr.zvel -= bcos(t[0] << 4, -5);
	}

	if (a & face_player_smart)
	{
		int newx, newy;

		newx = ps[playernum].pos.X + (ps[playernum].posxv / 768);
		newy = ps[playernum].pos.Y + (ps[playernum].posyv / 768);
		goalang = getangle(newx - actor->spr.x, newy - actor->spr.y);
		angdif = getincangle(actor->spr.ang, goalang) >> 2;
		if (angdif > -8 && angdif < 0) angdif = 0;
		actor->spr.ang += angdif;
	}

	if (t[1] == 0 || a == 0)
	{
		if ((badguy(actor) && actor->spr.extra <= 0) || (actor->spr.ox != actor->spr.x) || (actor->spr.oy != actor->spr.y))
		{
			actor->spr.backupvec2();
			SetActor(actor, actor->spr.pos);
		}
		return;
	}

	auto moveptr = &ScriptCode[t[1]];

	if (a & geth) actor->spr.xvel += (*moveptr - actor->spr.xvel) >> 1;
	if (a & getv) actor->spr.zvel += ((*(moveptr + 1) << 4) - actor->spr.zvel) >> 1;

	if (a & dodgebullet)
		dodge(actor);

	if (actor->spr.picnum != APLAYER)
		alterang(a, actor, playernum);

	if (actor->spr.xvel > -6 && actor->spr.xvel < 6) actor->spr.xvel = 0;

	a = badguy(actor);

	if (actor->spr.xvel || actor->spr.zvel)
	{
		if (a && actor->spr.picnum != ROTATEGUN)
		{
			if ((actor->spr.picnum == DRONE || actor->spr.picnum == COMMANDER) && actor->spr.extra > 0)
			{
				if (actor->spr.picnum == COMMANDER)
				{
					actor->floorz = l = getflorzofslopeptr(actor->spr.sector(), actor->spr.x, actor->spr.y);
					if (actor->spr.z > (l - (8 << 8)))
					{
						if (actor->spr.z > (l - (8 << 8))) actor->spr.z = l - (8 << 8);
						actor->spr.zvel = 0;
					}

					actor->ceilingz = l = getceilzofslopeptr(actor->spr.sector(), actor->spr.x, actor->spr.y);
					if ((actor->spr.z - l) < (80 << 8))
					{
						actor->spr.z = l + (80 << 8);
						actor->spr.zvel = 0;
					}
				}
				else
				{
					if (actor->spr.zvel > 0)
					{
						actor->floorz = l = getflorzofslopeptr(actor->spr.sector(), actor->spr.x, actor->spr.y);
						if (actor->spr.z > (l - (30 << 8)))
							actor->spr.z = l - (30 << 8);
					}
					else
					{
						actor->ceilingz = l = getceilzofslopeptr(actor->spr.sector(), actor->spr.x, actor->spr.y);
						if ((actor->spr.z - l) < (50 << 8))
						{
							actor->spr.z = l + (50 << 8);
							actor->spr.zvel = 0;
						}
					}
				}
			}
			else if (actor->spr.picnum != ORGANTIC)
			{
				if (actor->spr.zvel > 0 && actor->floorz < actor->spr.z)
					actor->spr.z = actor->floorz;
				if (actor->spr.zvel < 0)
				{
					l = getceilzofslopeptr(actor->spr.sector(), actor->spr.x, actor->spr.y);
					if ((actor->spr.z - l) < (66 << 8))
					{
						actor->spr.z = l + (66 << 8);
						actor->spr.zvel >>= 1;
					}
				}
			}
		}
		else if (actor->spr.picnum == APLAYER)
			if ((actor->spr.z - actor->ceilingz) < (32 << 8))
				actor->spr.z = actor->ceilingz + (32 << 8);

		daxvel = actor->spr.xvel;
		angdif = actor->spr.ang;

		if (a && actor->spr.picnum != ROTATEGUN)
		{
			if (xvel < 960 && actor->spr.xrepeat > 16)
			{

				daxvel = -(1024 - xvel);
				angdif = getangle(ps[playernum].pos.X - actor->spr.x, ps[playernum].pos.Y - actor->spr.y);

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
			else if (actor->spr.picnum != DRONE && actor->spr.picnum != SHARK && actor->spr.picnum != COMMANDER)
			{
				if (actor->spr.oz != actor->spr.z || (ud.multimode < 2 && ud.player_skill < 2))
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
			MulScale(daxvel, bsin(angdif), 14), actor->spr.zvel, CLIPMASK0, coll);
	}

	if (a)
	{
		if (actor->spr.sector()->ceilingstat & CSTAT_SECTOR_SKY)
			actor->spr.shade += (actor->spr.sector()->ceilingshade - actor->spr.shade) >> 1;
		else actor->spr.shade += (actor->spr.sector()->floorshade - actor->spr.shade) >> 1;

		if (actor->spr.sector()->floorpicnum == MIRROR)
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
	switch (actor->spr.picnum)
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
		if (actor->spr.yvel) fi.operaterespawns(actor->spr.yvel);
		break;
	default:
		if (actor->spr.hitag >= 0) fi.operaterespawns(actor->spr.hitag);
		break;
	}
}

void checktimetosleep_d(DDukeActor *actor)
{
	if (actor->spr.statnum == STAT_STANDABLE)
	{
		switch (actor->spr.picnum)
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
				ChangeActorStat(actor, STAT_ZOMBIEACTOR);
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
