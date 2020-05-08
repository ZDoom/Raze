//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2017-2019 - Nuke.YKT
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

This file is a combination of code from the following sources:
- EDuke 2 by Matt Saettler
- JFDuke by Jonathon Fowler (jf@jonof.id.au),
- DukeGDX and RedneckGDX by Alexander Makarov-[M210] (m210-2007@mail.ru)
- Redneck Rampage reconstructed source by Nuke.YKT

*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "global.h"
#include "namesdyn.h"

BEGIN_DUKE_NS

bool ceilingspace_d(int sectnum);
bool ceilingspace_r(int sectnum);
bool floorspace_d(int sectnum);
bool floorspace_r(int sectnum);
void addweapon_d(struct player_struct *p, int weapon);
void addweapon_r(struct player_struct *p, int weapon);
void hitradius_d(short i, int  r, int  hp1, int  hp2, int  hp3, int  hp4);
void hitradius_r(short i, int  r, int  hp1, int  hp2, int  hp3, int  hp4);
int movesprite_d(short spritenum, int xchange, int ychange, int zchange, unsigned int cliptype);
int movesprite_r(short spritenum, int xchange, int ychange, int zchange, unsigned int cliptype);
void lotsofmoney_d(spritetype *s, short n);
void lotsofmail_d(spritetype *s, short n);
void lotsofpaper_d(spritetype *s, short n);
void lotsoffeathers_r(spritetype *s, short n);
void guts_d(spritetype* s, short gtype, short n, short p);
void guts_r(spritetype* s, short gtype, short n, short p);
void gutsdir_d(spritetype* s, short gtype, short n, short p);
void gutsdir_r(spritetype* s, short gtype, short n, short p);
int ifhitsectors_d(int sectnum);
int ifhitsectors_r(int sectnum);
int ifhitbyweapon_r(int sn);
int ifhitbyweapon_d(int sn);

bool ceilingspace(int sectnum)
{
	return isRR()? ceilingspace_r(sectnum) : ceilingspace_d(sectnum);
}

bool floorspace(int sectnum)
{
	return isRR()? floorspace_r(sectnum) : floorspace_d(sectnum);
}

void addweapon(struct player_struct *p, int weapon)
{
	if (isRR()) addweapon_r(p, weapon);
	else addweapon_d(p, weapon);
}

void hitradius(short i, int  r, int  hp1, int  hp2, int  hp3, int  hp4)
{
	if (isRR()) hitradius_r(i, r, hp1, hp2, hp3, hp4);
	else hitradius_d(i, r, hp1, hp2, hp3, hp4);
}

int movesprite(short spritenum, int xchange, int ychange, int zchange, unsigned int cliptype)
{
	if (isRR()) return movesprite_r(spritenum, xchange, ychange, zchange, cliptype);
	else return movesprite_d(spritenum, xchange, ychange, zchange, cliptype);
}

void lotsofmoney(spritetype *s, short n)
{
	if (isRR()) lotsoffeathers_r(s, n);
	else lotsofmoney_d(s, n);
}

void lotsofmail(spritetype *s, short n)
{
	if (isRR()) lotsoffeathers_r(s, n);
	else lotsofmail_d(s, n);
}

void lotsofpaper(spritetype *s, short n)
{
	if (isRR()) lotsoffeathers_r(s, n);
	else lotsofpaper_d(s, n);
}

void guts(spritetype* s, short gtype, short n, short p)
{
	if (isRR()) guts_r(s, gtype, n, p);
	else guts_d(s, gtype, n, p);
}

void gutsdir(spritetype* s, short gtype, short n, short p)
{
	if (isRR()) gutsdir_r(s, gtype, n, p);
	else gutsdir_d(s, gtype, n, p);
}

int ifhitsectors(int sectnum)
{
	return isRR()? ifhitsectors_r(sectnum) : ifhitsectors_d(sectnum);
}

int ifhitbyweapon(int sectnum)
{
	return isRR()? ifhitbyweapon_r(sectnum) : ifhitbyweapon_d(sectnum);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void addammo(short weapon, struct player_struct* p, short amount)
{
	p->ammo_amount[weapon] += amount;

	if (p->ammo_amount[weapon] > max_ammo_amount[weapon])
		p->ammo_amount[weapon] = max_ammo_amount[weapon];
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checkavailinven(struct player_struct* p)
{

	if (p->firstaid_amount > 0)
		p->inven_icon = ICON_FIRSTAID;
	else if (p->steroids_amount > 0)
		p->inven_icon = ICON_STEROIDS;
	else if (p->holoduke_amount > 0)
		p->inven_icon = ICON_HOLODUKE;
	else if (p->jetpack_amount > 0)
		p->inven_icon = ICON_JETPACK;
	else if (p->heat_amount > 0)
		p->inven_icon = ICON_HEATS;
	else if (p->scuba_amount > 0)
		p->inven_icon = ICON_SCUBA;
	else if (p->boot_amount > 0)
		p->inven_icon = ICON_BOOTS;
	else p->inven_icon = ICON_NONE;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checkavailweapon(struct player_struct* p)
{
	short i, snum;
	int weap;

	if (p->wantweaponfire >= 0)
	{
		weap = p->wantweaponfire;
		p->wantweaponfire = -1;

		if (weap == p->curr_weapon) return;
		else if (p->gotweapon[weap] && p->ammo_amount[weap] > 0)
		{
			addweapon(p, weap);
			return;
		}
	}

	weap = p->curr_weapon;
	if (p->gotweapon[weap] && p->ammo_amount[weap] > 0)
		return;

	snum = sprite[p->i].yvel;

	// Note: RedNukem has this restriction, but the original source and RedneckGDX do not.
#if 1 // TRANSITIONAL
	int max = ((isRR()) ? DEVISTATOR_WEAPON : FREEZE_WEAPON);
#else
	int max = FREEZE_WEAPON;
#endif
	for (i = 0; i < 10; i++)
	{
		weap = ud.wchoice[snum][i];
		if ((g_gameType & GAMEFLAG_SHAREWARE) && weap > 6) continue;

		if (weap == 0) weap = max;
		else weap--;

		if (weap == KNEE_WEAPON || (p->gotweapon[weap] && p->ammo_amount[weap] > 0))
			break;
	}

	if (i == HANDREMOTE_WEAPON) weap = KNEE_WEAPON;

	// Found the weapon

	p->last_weapon = p->curr_weapon;
	p->random_club_frame = 0;
	p->curr_weapon = weap;
	if (isWW2GI())
	{
		SetGameVarID(g_iWeaponVarID, p->curr_weapon, p->i, snum);
		if (p->curr_weapon >= 0)
		{
			SetGameVarID(g_iWorksLikeVarID, aplWeaponWorksLike[p->curr_weapon][snum], p->i, snum);
		}
		else
		{
			SetGameVarID(g_iWorksLikeVarID, -1, p->i, snum);
		}
		OnEvent(EVENT_CHANGEWEAPON, p->i, snum, -1);
	}

	p->kickback_pic = 0;
	if (p->holster_weapon == 1)
	{
		p->holster_weapon = 0;
		p->weapon_pos = 10;
	}
	else p->weapon_pos = -1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void clearcamera(player_struct* ps)
{
	ps->newowner = -1;
	ps->posx = ps->oposx;
	ps->posy = ps->oposy;
	ps->posz = ps->oposz;
	ps->q16ang = ps->oq16ang;
	updatesector(ps->posx, ps->posy, &ps->cursectnum);
	setpal(ps);

	int k = headspritestat[1];
	while (k >= 0)
	{
		if (sprite[k].picnum == CAMERA1)
			sprite[k].yvel = 0;
		k = nextspritestat[k];
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int ssp(short i, unsigned int cliptype) //The set sprite function
{
	spritetype* s;
	int movetype;

	s = &sprite[i];

	movetype = movesprite(i,
		(s->xvel * (sintable[(s->ang + 512) & 2047])) >> 14,
		(s->xvel * (sintable[s->ang & 2047])) >> 14, s->zvel,
		cliptype);

	return (movetype == 0);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void insertspriteq(int i)
{
	if (spriteqamount > 0)
	{
		if (spriteq[spriteqloc] >= 0)
			sprite[spriteq[spriteqloc]].xrepeat = 0;
		spriteq[spriteqloc] = i;
		spriteqloc = (spriteqloc + 1) % spriteqamount;
	}
	else sprite[i].xrepeat = sprite[i].yrepeat = 0;
}

//---------------------------------------------------------------------------
//
// consolidation of several nearly identical functions
//
//---------------------------------------------------------------------------

void lotsofstuff(spritetype* s, short n, int spawntype)
{
	short i, j;
	for (i = n; i > 0; i--)
	{
		short r1 = krand(), r2 = krand();	// using the RANDCORRECT version from RR.
		// TRANSITIONAL RedNukem sets the spawner as owner.
		j = EGS(s->sectnum, s->x, s->y, s->z - (r2 % (47 << 8)), spawntype, -32, 8, 8, r1 & 2047, 0, 0, 0, 5);
		sprite[j].cstat = krand() & 12;
	}
}

//---------------------------------------------------------------------------
//
// movesector - why is this in actors.cpp?
//
//---------------------------------------------------------------------------

void ms(short i)
{
	//T1,T2 and T3 are used for all the sector moving stuff!!!

	short startwall, endwall, x;
	int tx, ty;
	spritetype* s;

	s = &sprite[i];

	s->x += (s->xvel * (sintable[(s->ang + 512) & 2047])) >> 14;
	s->y += (s->xvel * (sintable[s->ang & 2047])) >> 14;

	int j = hittype[i].temp_data[1];
	int k = hittype[i].temp_data[2];

	startwall = sector[s->sectnum].wallptr;
	endwall = startwall + sector[s->sectnum].wallnum;
	for (x = startwall; x < endwall; x++)
	{
		rotatepoint(
			0, 0,
			msx[j], msy[j],
			k & 2047, &tx, &ty);

		dragpoint(x, s->x + tx, s->y + ty);

		j++;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movecyclers(void)
{
	short q, j, x, t, s, * c;
	walltype* wal;
	char cshade;

	for (q = numcyclers - 1; q >= 0; q--)
	{

		c = &cyclers[q][0];
		s = c[0];

		t = c[3];
		j = t + (sintable[c[1] & 2047] >> 10);
		cshade = c[2];

		if (j < cshade) j = cshade;
		else if (j > t)  j = t;

		c[1] += sector[s].extra;
		if (c[5])
		{
			wal = &wall[sector[s].wallptr];
			for (x = sector[s].wallnum; x > 0; x--, wal++)
				if (wal->hitag != 1)
				{
					wal->shade = j;

					if ((wal->cstat & 2) && wal->nextwall >= 0)
						wall[wal->nextwall].shade = j;

				}
			sector[s].floorshade = sector[s].ceilingshade = j;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movedummyplayers(void)
{
	short i, p, nexti;

	i = headspritestat[STAT_DUMMYPLAYER];
	while (i >= 0)
	{
		nexti = nextspritestat[i];

		p = sprite[sprite[i].owner].yvel;

		if ((!isRR() && ps[p].on_crane >= 0) || sector[ps[p].cursectnum].lotag != 1 || sprite[ps[p].i].extra <= 0)
		{
			ps[p].dummyplayersprite = -1;
			deletesprite(i);
			i = nexti;
			continue;
		}
		else
		{
			if (ps[p].on_ground && ps[p].on_warping_sector == 1 && sector[ps[p].cursectnum].lotag == 1)
			{
				sprite[i].cstat = 257;
				sprite[i].z = sector[sprite[i].sectnum].ceilingz + (27 << 8);
				sprite[i].ang = ps[p].getang();
				if (hittype[i].temp_data[0] == 8)
					hittype[i].temp_data[0] = 0;
				else hittype[i].temp_data[0]++;
			}
			else
			{
				if (sector[sprite[i].sectnum].lotag != 2) sprite[i].z = sector[sprite[i].sectnum].floorz;
				sprite[i].cstat = (short)32768;
			}
		}

		sprite[i].x += (ps[p].posx - ps[p].oposx);
		sprite[i].y += (ps[p].posy - ps[p].oposy);
		setsprite(i, sprite[i].x, sprite[i].y, sprite[i].z);
		i = nexti;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int otherp;
void moveplayers(void) //Players
{
	short i, nexti;
	int otherx;
	spritetype* s;
	struct player_struct* p;

	i = headspritestat[STAT_PLAYER];
	while (i >= 0)
	{
		nexti = nextspritestat[i];

		s = &sprite[i];
		p = &ps[s->yvel];
		if (s->owner >= 0)
		{
			if (p->newowner >= 0) //Looking thru the camera
			{
				s->x = p->oposx;
				s->y = p->oposy;
				hittype[i].bposz = s->z = p->oposz + PHEIGHT;
				s->ang = p->getoang();
				setsprite(i, s->x, s->y, s->z);
			}
			else
			{
				if (ud.multimode > 1)
					otherp = findotherplayer(s->yvel, &otherx);
				else
				{
					otherp = s->yvel;
					otherx = 0;
				}

				execute(i, s->yvel, otherx);

				p->oq16ang = p->q16ang;

				if (ud.multimode > 1)
					if (sprite[ps[otherp].i].extra > 0)
					{
						if (s->yrepeat > 32 && sprite[ps[otherp].i].yrepeat < 32)
						{
							if (otherx < 1400 && p->knee_incs == 0)
							{
								p->knee_incs = 1;
								p->weapon_pos = -1;
								p->actorsqu = ps[otherp].i;
							}
						}
					}
				if (ud.god)
				{
					s->extra = p->max_player_health;
					s->cstat = 257;
					if (!isWW2GI() && !isRR())
						p->jetpack_amount = 1599;
				}


				if (s->extra > 0)
				{
					// currently alive...

					hittype[i].owner = i;

					if (ud.god == 0)
						if (ceilingspace(s->sectnum) || floorspace(s->sectnum))
							quickkill(p);
				}
				else
				{

					p->posx = s->x;
					p->posy = s->y;
					p->posz = s->z - (20 << 8);

					p->newowner = -1;

					if (p->wackedbyactor >= 0 && sprite[p->wackedbyactor].statnum < MAXSTATUS)
					{
						int ang = p->getang();
						ang += getincangle(ang, getangle(sprite[p->wackedbyactor].x - p->posx, sprite[p->wackedbyactor].y - p->posy)) >> 1;
						p->setang(ang & 2047);
					}

				}
				s->ang = p->getang();
			}
		}
		else
		{
			if (p->holoduke_on == -1)
			{
				deletesprite(i);
				i = nexti;
				continue;
			}

			hittype[i].bposx = s->x;
			hittype[i].bposy = s->y;
			hittype[i].bposz = s->z;

			s->cstat = 0;

			if (s->xrepeat < 42)
			{
				s->xrepeat += 4;
				s->cstat |= 2;
			}
			else s->xrepeat = 42;
			if (s->yrepeat < 36)
				s->yrepeat += 4;
			else
			{
				s->yrepeat = 36;
				if (sector[s->sectnum].lotag != ST_2_UNDERWATER)
					makeitfall(i);
				if (s->zvel == 0 && sector[s->sectnum].lotag == ST_1_ABOVE_WATER)
					s->z += (32 << 8);
			}

			if (s->extra < 8)
			{
				s->xvel = 128;
				s->ang = p->getang();
				s->extra++;
				//IFMOVING;		// JBF 20040825: is really "if (ssp(i,CLIPMASK0)) ;" which is probably
				ssp(i, CLIPMASK0);	// not the safest of ideas because a zealous optimiser probably sees
							// it as redundant, so I'll call the "ssp(i,CLIPMASK0)" explicitly.
			}
			else
			{
				s->ang = 2047 - (p->getang());
				setsprite(i, s->x, s->y, s->z);
			}
		}

		if (sector[s->sectnum].ceilingstat & 1)
			s->shade += (sector[s->sectnum].ceilingshade - s->shade) >> 1;
		else
			s->shade += (sector[s->sectnum].floorshade - s->shade) >> 1;

		i = nexti;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movefx(void)
{
	short i, j, nexti, p;
	int x, ht;
	spritetype* s;

	i = headspritestat[STAT_FX];
	while (i >= 0)
	{
		s = &sprite[i];

		nexti = nextspritestat[i];

		switch (s->picnum)
		{
		case RESPAWN:
			if (sprite[i].extra == 66)
			{
				j = spawn(i, sprite[i].hitag);
				if (isRRRA()) 
				{
					respawn_rrra(i, j);
				}
				else
				{
					deletesprite(i);
				}
			}
			else if (sprite[i].extra > (66 - 13))
				sprite[i].extra++;
			break;

		case MUSICANDSFX:

			ht = s->hitag;

			if (hittype[i].temp_data[1] != (int)SoundEnabled())
			{
				hittype[i].temp_data[1] = SoundEnabled();
				hittype[i].temp_data[0] = 0;
			}

			if (s->lotag >= 1000 && s->lotag < 2000)
			{
				x = ldist(&sprite[ps[screenpeek].i], s);
				if (x < ht && hittype[i].temp_data[0] == 0)
				{
					FX_SetReverb(s->lotag - 1000);
					hittype[i].temp_data[0] = 1;
				}
				if (x >= ht && hittype[i].temp_data[0] == 1)
				{
					FX_SetReverb(0);
					FX_SetReverbDelay(0);
					hittype[i].temp_data[0] = 0;
				}
			}
			else if (s->lotag < 999 && (unsigned)sector[s->sectnum].lotag < ST_9_SLIDING_ST_DOOR && snd_ambience && sector[sprite[i].sectnum].floorz != sector[sprite[i].sectnum].ceilingz)
			{
				auto flags = S_GetUserFlags(s->lotag);
				if (flags & SF_MSFX)
				{
					int x = dist(&sprite[ps[screenpeek].i], s);

					if (x < ht && hittype[i].temp_data[0] == 0)
					{
						// Start playing an ambience sound.
						A_PlaySound(s->lotag, i, CHAN_AUTO, CHANF_LOOP);
						hittype[i].temp_data[0] = 1;  // AMBIENT_SFX_PLAYING
					}
					else if (x >= ht && hittype[i].temp_data[0] == 1)
					{
						// Stop playing ambience sound because we're out of its range.
						S_StopEnvSound(s->lotag, i);
					}
				}

				if ((flags & (SF_GLOBAL | SF_DTAG)) == SF_GLOBAL)
				{
					if (hittype[i].temp_data[4] > 0) hittype[i].temp_data[4]--;
					else for (p = connecthead; p >= 0; p = connectpoint2[p])
						if (p == myconnectindex && ps[p].cursectnum == s->sectnum)
						{
							S_PlaySound(s->lotag + (unsigned)global_random % (s->hitag + 1));
							hittype[i].temp_data[4] = 26 * 40 + (global_random % (26 * 40));
						}
				}
			}
			break;
		}
		i = nexti;
	}
}

//---------------------------------------------------------------------------
//
// split out of movestandables
//
//---------------------------------------------------------------------------

void movecrane(int i, int crane)
{
	auto t = &hittype[i].temp_data[0];
	auto s = &sprite[i];
	int sect = s->sectnum;
	int x;

	//t[0] = state
	//t[1] = checking sector number

	if (s->xvel) getglobalz(i);

	if (t[0] == 0) //Waiting to check the sector
	{
		int j = headspritesect[t[1]];
		while (j >= 0)
		{
			int nextj = nextspritesect[j];
			switch (sprite[j].statnum)
			{
			case STAT_ACTOR:
			case STAT_ZOMBIEACTOR:
			case STAT_STANDABLE:
			case STAT_PLAYER:
				s->ang = getangle(msx[t[4] + 1] - s->x, msy[t[4] + 1] - s->y);
				setsprite(j, msx[t[4] + 1], msy[t[4] + 1], sprite[j].z);
				t[0]++;
				deletesprite(i);
				return;
			}
			j = nextj;
		}
	}

	else if (t[0] == 1)
	{
		if (s->xvel < 184)
		{
			s->picnum = crane + 1;
			s->xvel += 8;
		}
		//IFMOVING;	// JBF 20040825: see my rant above about this
		ssp(i, CLIPMASK0);
		if (sect == t[1])
			t[0]++;
	}
	else if (t[0] == 2 || t[0] == 7)
	{
		s->z += (1024 + 512);

		if (t[0] == 2)
		{
			if ((sector[sect].floorz - s->z) < (64 << 8))
				if (s->picnum > crane) s->picnum--;

			if ((sector[sect].floorz - s->z) < (4096 + 1024))
				t[0]++;
		}
		if (t[0] == 7)
		{
			if ((sector[sect].floorz - s->z) < (64 << 8))
			{
				if (s->picnum > crane) s->picnum--;
				else
				{
					if (s->owner == -2)
					{
						auto p = findplayer(s, &x);
						spritesound(isRR() ? 390 : DUKE_GRUNT, ps[p].i);
						if (ps[p].on_crane == i)
							ps[p].on_crane = -1;
					}
					t[0]++;
					s->owner = -1;
				}
			}
		}
	}
	else if (t[0] == 3)
	{
		s->picnum++;
		if (s->picnum == (crane + 2))
		{
			auto p = checkcursectnums(t[1]);
			if (p >= 0 && ps[p].on_ground)
			{
				s->owner = -2;
				ps[p].on_crane = i;
				spritesound(isRR() ? 390 : DUKE_GRUNT, ps[p].i);
				ps[p].setang(s->ang + 1024);
			}
			else
			{
				int j = headspritesect[t[1]];
				while (j >= 0)
				{
					switch (sprite[j].statnum)
					{
					case 1:
					case 6:
						s->owner = j;
						break;
					}
					j = nextspritesect[j];
				}
			}

			t[0]++;//Grabbed the sprite
			t[2] = 0;
			return;
		}
	}
	else if (t[0] == 4) //Delay before going up
	{
		t[2]++;
		if (t[2] > 10)
			t[0]++;
	}
	else if (t[0] == 5 || t[0] == 8)
	{
		if (t[0] == 8 && s->picnum < (crane + 2))
			if ((sector[sect].floorz - s->z) > 8192)
				s->picnum++;

		if (s->z < msx[t[4] + 2])
		{
			t[0]++;
			s->xvel = 0;
		}
		else
			s->z -= (1024 + 512);
	}
	else if (t[0] == 6)
	{
		if (s->xvel < 192)
			s->xvel += 8;
		s->ang = getangle(msx[t[4]] - s->x, msy[t[4]] - s->y);
		//IFMOVING;	// JBF 20040825: see my rant above about this
		ssp(i, CLIPMASK0);
		if (((s->x - msx[t[4]]) * (s->x - msx[t[4]]) + (s->y - msy[t[4]]) * (s->y - msy[t[4]])) < (128 * 128))
			t[0]++;
	}

	else if (t[0] == 9)
		t[0] = 0;

	setsprite(msy[t[4] + 2], s->x, s->y, s->z - (34 << 8));

	if (s->owner != -1)
	{
		auto p = findplayer(s, &x);

		int j = ifhitbyweapon(i);
		if (j >= 0)
		{
			if (s->owner == -2)
				if (ps[p].on_crane == i)
					ps[p].on_crane = -1;
			s->owner = -1;
			s->picnum = crane;
			return;
		}

		if (s->owner >= 0)
		{
			setsprite(s->owner, s->x, s->y, s->z);

			hittype[s->owner].bposx = s->x;
			hittype[s->owner].bposy = s->y;
			hittype[s->owner].bposz = s->z;

			s->zvel = 0;
		}
		else if (s->owner == -2)
		{
			auto ang = ps[p].getang();
			ps[p].oposx = ps[p].posx = s->x - (sintable[(ang + 512) & 2047] >> 6);
			ps[p].oposy = ps[p].posy = s->y - (sintable[ang & 2047] >> 6);
			ps[p].oposz = ps[p].posz = s->z + (2 << 8);
			setsprite(ps[p].i, ps[p].posx, ps[p].posy, ps[p].posz);
			ps[p].cursectnum = sprite[ps[p].i].sectnum;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movefountain(int i, int fountain)
{
	auto t = &hittype[i].temp_data[0];
	auto s = &sprite[i];
	int x;
	if (t[0] > 0)
	{
		if (t[0] < 20)
		{
			t[0]++;

			s->picnum++;

			if (s->picnum == fountain + 3)
				s->picnum = fountain + 1;
		}
		else
		{
			findplayer(s, &x);

			if (x > 512)
			{
				t[0] = 0;
				s->picnum = fountain;
			}
			else t[0] = 1;
		}
	}
}
//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveflammable(int i, int tire, int box, int pool)
{
	auto s = &sprite[i];
	int j;
	if (hittype[i].temp_data[0] == 1)
	{
		hittype[i].temp_data[1]++;
		if ((hittype[i].temp_data[1] & 3) > 0) return;

		if (!isRR() && s->picnum == tire && hittype[i].temp_data[1] == 32)
		{
			s->cstat = 0;
			j = spawn(i, pool);
			sprite[j].shade = 127;
		}
		else
		{
			if (s->shade < 64) s->shade++;
			else
			{
				deletesprite(i);
				return;
			}
		}

		j = s->xrepeat - (krand() & 7);
		if (j < 10) 
		{
			deletesprite(i);
			return;
		}

		s->xrepeat = j;

		j = s->yrepeat - (krand() & 7);
		if (j < 4) 
		{
			deletesprite(i);
			return;
		}
		s->yrepeat = j;
	}
	if (box >= 0 && s->picnum == box)
	{
		makeitfall(i);
		hittype[i].ceilingz = sector[s->sectnum].ceilingz;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void detonate(int i, int explosion)
{
	auto s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	earthquaketime = 16;

	int j = headspritestat[STAT_EFFECTOR];
	while (j >= 0)
	{
		if (s->hitag == sprite[j].hitag)
		{
			if (sprite[j].lotag == SE_13_EXPLOSIVE)
			{
				if (hittype[j].temp_data[2] == 0)
					hittype[j].temp_data[2] = 1;
			}
			else if (sprite[j].lotag == SE_8_UP_OPEN_DOOR_LIGHTS)
				hittype[j].temp_data[4] = 1;
			else if (sprite[j].lotag == SE_18_INCREMENTAL_SECTOR_RISE_FALL)
			{
				if (hittype[j].temp_data[0] == 0)
					hittype[j].temp_data[0] = 1;
			}
			else if (sprite[j].lotag == SE_21_DROP_FLOOR)
				hittype[j].temp_data[0] = 1;
		}
		j = nextspritestat[j];
	}

	s->z -= (32 << 8);

	if ((t[3] == 1 && s->xrepeat) || s->lotag == -99)
	{
		int x = s->extra;
		spawn(i, explosion);
		hitradius(i, seenineblastradius, x >> 2, x - (x >> 1), x - (x >> 2), x);
		spritesound(PIPEBOMB_EXPLODE, i);
	}

	if (s->xrepeat)
		for (int x = 0; x < 8; x++) RANDOMSCRAP(s, i);

	deletesprite(i);

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movemasterswitch(int i, int spectype1, int spectype2)
{
	auto s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	if (s->yvel == 1)
	{
		s->hitag--;
		if (s->hitag <= 0)
		{
			operatesectors(s->sectnum, i);

			int j = headspritesect[s->sectnum];
			while (j >= 0)
			{
				if (sprite[j].statnum == 3)
				{
					switch (sprite[j].lotag)
					{
					case SE_2_EARTHQUAKE:
					case SE_21_DROP_FLOOR:
					case SE_31_FLOOR_RISE_FALL:
					case SE_32_CEILING_RISE_FALL:
					case SE_36_PROJ_SHOOTER:
						hittype[j].temp_data[0] = 1;
						break;
					case SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT:
						hittype[j].temp_data[4] = 1;
						break;
					}
				}
				else if (sprite[j].statnum == 6)
				{
					if (sprite[j].picnum == spectype1 || sprite[j].picnum == spectype2) // SEENINE and OOZFILTER
					{
						sprite[j].shade = -31;
					}
				}
				j = nextspritesect[j];
			}
			deletesprite(i);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movetrash(int i)
{
	auto s = &sprite[i];
	if (s->xvel == 0) s->xvel = 1;
	if (ssp(i, CLIPMASK0))
	{
		makeitfall(i);
		if (krand() & 1) s->zvel -= 256;
		if (klabs(s->xvel) < 48)
			s->xvel += (krand() & 3);
	}
	else deletesprite(i);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movewaterdrip(int i, int drip)
{
	auto s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int sect = s->sectnum;

	if (t[1])
	{
		t[1]--;
		if (t[1] == 0)
			s->cstat &= 32767;
	}
	else
	{
		makeitfall(i);
		ssp(i, CLIPMASK0);
		if (s->xvel > 0) s->xvel -= 2;

		if (s->zvel == 0)
		{
			s->cstat |= 32768;

			if (s->pal != 2 && (isRR() || s->hitag == 0))
				spritesound(SOMETHING_DRIPPING, i);

			if (sprite[s->owner].picnum != drip)
			{
				deletesprite(i);
			}
			else
			{
				hittype[i].bposz = s->z = t[0];
				t[1] = 48 + (krand() & 31);
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movedoorshock(int i)
{
	auto s = &sprite[i];
	int sect = s->sectnum;
	int j = abs(sector[sect].ceilingz - sector[sect].floorz) >> 9;
	s->yrepeat = j + 4;
	s->xrepeat = 16;
	s->z = sector[sect].floorz;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movetouchplate(int i, int plate)
{
	auto s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int sect = s->sectnum;
	int x;
	int p;

	if (t[1] == 1 && s->hitag >= 0) //Move the sector floor
	{
		x = sector[sect].floorz;

		if (t[3] == 1)
		{
			if (x >= t[2])
			{
				sector[sect].floorz = x;
				t[1] = 0;
			}
			else
			{
				sector[sect].floorz += sector[sect].extra;
				p = checkcursectnums(sect);
				if (p >= 0) ps[p].posz += sector[sect].extra;
			}
		}
		else
		{
			if (x <= s->z)
			{
				sector[sect].floorz = s->z;
				t[1] = 0;
			}
			else
			{
				sector[sect].floorz -= sector[sect].extra;
				p = checkcursectnums(sect);
				if (p >= 0)
					ps[p].posz -= sector[sect].extra;
			}
		}
		return;
	}

	if (t[5] == 1) return;

	p = checkcursectnums(sect);
	if (p >= 0 && (ps[p].on_ground || s->ang == 512))
	{
		if (t[0] == 0 && !check_activator_motion(s->lotag))
		{
			t[0] = 1;
			t[1] = 1;
			t[3] = !t[3];
			operatemasterswitches(s->lotag);
			operateactivators(s->lotag, p);
			if (s->hitag > 0)
			{
				s->hitag--;
				if (s->hitag == 0) t[5] = 1;
			}
		}
	}
	else t[0] = 0;

	if (t[1] == 1)
	{
		int j = headspritestat[STAT_STANDABLE];
		while (j >= 0)
		{
			if (j != i && sprite[j].picnum == plate && sprite[j].lotag == s->lotag)
			{
				hittype[j].temp_data[1] = 1;
				hittype[j].temp_data[3] = t[3];
			}
			j = nextspritestat[j];
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void moveooz(int i, int seenine, int seeninedead, int ooz, int explosion)
{
	auto s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int j;
	if (s->shade != -32 && s->shade != -33)
	{
		if (s->xrepeat)
			j = (ifhitbyweapon(i) >= 0);
		else
			j = 0;

		if (j || s->shade == -31)
		{
			if (j) s->lotag = 0;

			t[3] = 1;

			j = headspritestat[STAT_STANDABLE];
			while (j >= 0)
			{
				if (s->hitag == sprite[j].hitag && (sprite[j].picnum == seenine || sprite[j].picnum == ooz))
					sprite[j].shade = -32;
				j = nextspritestat[j];
			}
		}
	}
	else
	{
		if (s->shade == -32)
		{
			if (s->lotag > 0)
			{
				s->lotag -= 3;
				if (s->lotag <= 0) s->lotag = -99;
			}
			else
				s->shade = -33;
		}
		else
		{
			if (s->xrepeat > 0)
			{
				hittype[i].temp_data[2]++;
				if (hittype[i].temp_data[2] == 3)
				{
					if (s->picnum == ooz)
					{
						hittype[i].temp_data[2] = 0;
						detonate(i, explosion);
						return;
					}
					if (s->picnum != (seeninedead + 1))
					{
						hittype[i].temp_data[2] = 0;

						if (s->picnum == seeninedead) s->picnum++;
						else if (s->picnum == seenine)
							s->picnum = seeninedead;
					}
					else
					{
						detonate(i, explosion);
						return;
					}
				}
				return;
			}
			detonate(i, explosion);
			return;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void movecanwithsomething(int i)
{
	auto s = &sprite[i];
	makeitfall(i);
	int j = ifhitbyweapon(i);
	if (j >= 0)
	{
		spritesound(VENT_BUST, i);
		for (j = 0; j < 10; j++)
			RANDOMSCRAP(s, i);

		if (s->lotag) spawn(i, s->lotag);

		deletesprite(i);
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void bounce(int i)
{
	int k, l, daang, dax, day, daz, xvect, yvect, zvect;
	short hitsect;
	spritetype* s = &sprite[i];

	xvect = mulscale10(s->xvel, sintable[(s->ang + 512) & 2047]);
	yvect = mulscale10(s->xvel, sintable[s->ang & 2047]);
	zvect = s->zvel;

	hitsect = s->sectnum;

	k = sector[hitsect].wallptr; l = wall[k].point2;
	daang = getangle(wall[l].x - wall[k].x, wall[l].y - wall[k].y);

	if (s->z < (hittype[i].floorz + hittype[i].ceilingz) >> 1)
		k = sector[hitsect].ceilingheinum;
	else
		k = sector[hitsect].floorheinum;

	dax = mulscale14(k, sintable[(daang) & 2047]);
	day = mulscale14(k, sintable[(daang + 1536) & 2047]);
	daz = 4096;

	k = xvect * dax + yvect * day + zvect * daz;
	l = dax * dax + day * day + daz * daz;
	if ((abs(k) >> 14) < l)
	{
		k = divscale17(k, l);
		xvect -= mulscale16(dax, k);
		yvect -= mulscale16(day, k);
		zvect -= mulscale16(daz, k);
	}

	s->zvel = zvect;
	s->xvel = ksqrt(dmulscale8(xvect, xvect, yvect, yvect));
	s->ang = getangle(xvect, yvect);
}

//---------------------------------------------------------------------------
//
// taken out of moveweapon
//
//---------------------------------------------------------------------------

void movetongue(int i, int tongue, int jaw)
{
	spritetype* s = &sprite[i];

	hittype[i].temp_data[0] = sintable[(hittype[i].temp_data[1]) & 2047] >> 9;
	hittype[i].temp_data[1] += 32;
	if (hittype[i].temp_data[1] > 2047)
	{
		deletesprite(i);
		return;
	}

	if (sprite[s->owner].statnum == MAXSTATUS)
		if (badguy(&sprite[s->owner]) == 0)
		{
			deletesprite(i);
			return;
		}

	s->ang = sprite[s->owner].ang;
	s->x = sprite[s->owner].x;
	s->y = sprite[s->owner].y;
	if (sprite[s->owner].picnum == APLAYER)
		s->z = sprite[s->owner].z - (34 << 8);
	for (int k = 0; k < hittype[i].temp_data[0]; k++)
	{
		int q = EGS(s->sectnum,
			s->x + ((k * sintable[(s->ang + 512) & 2047]) >> 9),
			s->y + ((k * sintable[s->ang & 2047]) >> 9),
			s->z + ((k * ksgn(s->zvel)) * abs(s->zvel / 12)), tongue, -40 + (k << 1),
			8, 8, 0, 0, 0, i, 5);
		sprite[q].cstat = 128;
		sprite[q].pal = 8;
	}
	int k = hittype[i].temp_data[0];	// do not depend on the above loop counter.
	int q = EGS(s->sectnum,
		s->x + ((k * sintable[(s->ang + 512) & 2047]) >> 9),
		s->y + ((k * sintable[s->ang & 2047]) >> 9),
		s->z + ((k * ksgn(s->zvel)) * abs(s->zvel / 12)), jaw, -40,
		32, 32, 0, 0, 0, i, 5);
	sprite[q].cstat = 128;
	if (hittype[i].temp_data[1] > 512 && hittype[i].temp_data[1] < (1024))
		sprite[q].picnum = jaw + 1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool respawnmarker(int i, int yellow, int green)
{
	hittype[i].temp_data[0]++;
	if (hittype[i].temp_data[0] > respawnitemtime)
	{
		deletesprite(i);
		return false;
	}
	if (hittype[i].temp_data[0] >= (respawnitemtime >> 1) && hittype[i].temp_data[0] < ((respawnitemtime >> 1) + (respawnitemtime >> 2)))
		sprite[i].picnum = yellow;
	else if (hittype[i].temp_data[0] > ((respawnitemtime >> 1) + (respawnitemtime >> 2)))
		sprite[i].picnum = green;
	makeitfall(i);
	return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool rat(int i, bool makesound)
{
	spritetype* s = &sprite[i];
	makeitfall(i);
	if (ssp(i, CLIPMASK0))
	{
		if (makesound && (krand() & 255) == 0) spritesound(RATTY, i);
		s->ang += (krand() & 31) - 15 + (sintable[(hittype[i].temp_data[0] << 8) & 2047] >> 11);
	}
	else
	{
		hittype[i].temp_data[0]++;
		if (hittype[i].temp_data[0] > 1)
		{
			deletesprite(i);
			return false;
		}
		else s->ang = (krand() & 2047);
	}
	if (s->xvel < 128)
		s->xvel += 2;
	s->ang += (krand() & 3) - 6;
	return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool queball(int i, int pocket, int queball, int stripeball)
{
	spritetype* s = &sprite[i];
	int j, nextj;
	if (s->xvel)
	{
		j = headspritestat[0];
		while (j >= 0)
		{
			nextj = nextspritestat[j];
			if (sprite[j].picnum == pocket && ldist(&sprite[j], s) < 52)
			{
				deletesprite(i);
				return false;
			}
			j = nextj;
		}

		j = clipmove(&s->x, &s->y, &s->z, &s->sectnum,
			(((s->xvel * (sintable[(s->ang + 512) & 2047])) >> 14) * TICSPERFRAME) << 11,
			(((s->xvel * (sintable[s->ang & 2047])) >> 14) * TICSPERFRAME) << 11,
			24L, (4 << 8), (4 << 8), CLIPMASK1);

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
				checkhitsprite(i, j);
			}
		}
		s->xvel--;
		if (s->xvel < 0) s->xvel = 0;
		if (s->picnum == stripeball)
		{
			s->cstat = 257;
			s->cstat |= 4 & s->xvel;
			s->cstat |= 8 & s->xvel;
		}
	}
	else
	{
		int x;
		int p = findplayer(s, &x);

		if (x < 1596)
		{

			//                        if(s->pal == 12)
			{
				j = getincangle(ps[p].getang(), getangle(s->x - ps[p].posx, s->y - ps[p].posy));
				if (j > -64 && j < 64 && PlayerInput(p, SK_OPEN))
					if (ps[p].toggle_key_flag == 1)
					{
						int a = headspritestat[1];
						while (a >= 0)
						{
							if (sprite[a].picnum == queball || sprite[a].picnum == stripeball)
							{
								j = getincangle(ps[p].getang(), getangle(sprite[a].x - ps[p].posx, sprite[a].y - ps[p].posy));
								if (j > -64 && j < 64)
								{
									int l;
									findplayer(&sprite[a], &l);
									if (x > l) break;
								}
							}
							a = nextspritestat[a];
						}
						if (a == -1)
						{
							if (s->pal == 12)
								s->xvel = 164;
							else s->xvel = 140;
							s->ang = ps[p].getang();
							ps[p].toggle_key_flag = 2;
						}
					}
			}
		}
		if (x < 512 && s->sectnum == ps[p].cursectnum)
		{
			s->ang = getangle(s->x - ps[p].posx, s->y - ps[p].posy);
			s->xvel = 48;
		}
	}
	return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void forcesphere(int i, int forcesphere)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int sect = s->sectnum;
	if (s->yvel == 0)
	{
		s->yvel = 1;

		for (int l = 512; l < (2048 - 512); l += 128)
			for (int j = 0; j < 2048; j += 128)
			{
				int k = spawn(i, forcesphere);
				sprite[k].cstat = 257 + 128;
				sprite[k].clipdist = 64;
				sprite[k].ang = j;
				sprite[k].zvel = sintable[l & 2047] >> 5;
				sprite[k].xvel = sintable[(l + 512) & 2047] >> 9;
				sprite[k].owner = i;
			}
	}

	if (t[3] > 0)
	{
		if (s->zvel < 6144)
			s->zvel += 192;
		s->z += s->zvel;
		if (s->z > sector[sect].floorz)
			s->z = sector[sect].floorz;
		t[3]--;
		if (t[3] == 0)
		{
			deletesprite(i);
			return;
		}
		else if (t[2] > 10)
		{
			int j = headspritestat[5];
			while (j >= 0)
			{
				if (sprite[j].owner == i && sprite[j].picnum == forcesphere)
					hittype[j].temp_data[1] = 1 + (krand() & 63);
				j = nextspritestat[j];
			}
			t[3] = 64;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void recon(int i, int explosion, int firelaser, int attacksnd, int painsnd, int roamsnd, int shift, int (*getspawn)(int i))
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int sect = s->sectnum;
	int j, a;

	getglobalz(i);

	if (sector[s->sectnum].ceilingstat & 1)
		s->shade += (sector[s->sectnum].ceilingshade - s->shade) >> 1;
	else s->shade += (sector[s->sectnum].floorshade - s->shade) >> 1;

	if (s->z < sector[sect].ceilingz + (32 << 8))
		s->z = sector[sect].ceilingz + (32 << 8);

	if (ud.multimode < 2)
	{
		if (actor_tog == 1)
		{
			s->cstat = (short)32768;
			return;
		}
		else if (actor_tog == 2) s->cstat = 257;
	}
	j = ifhitbyweapon(i); if (j >= 0)
	{
		if (s->extra < 0 && t[0] != -1)
		{
			t[0] = -1;
			s->extra = 0;
		}
		if (painsnd >= 0) spritesound(painsnd, i);
		RANDOMSCRAP(s, i);
	}

	if (t[0] == -1)
	{
		s->z += 1024;
		t[2]++;
		if ((t[2] & 3) == 0) spawn(i, explosion);
		getglobalz(i);
		s->ang += 96;
		s->xvel = 128;
		j = ssp(i, CLIPMASK0);
		if (j != 1 || s->z > hittype[i].floorz)
		{
			for (int l = 0; l < 16; l++)
				RANDOMSCRAP(s, i);
			spritesound(LASERTRIP_EXPLODE, i);
			spawn(i, getspawn(i));
			ps[myconnectindex].actors_killed++;
			deletesprite(i);
		}
		return;
	}
	else
	{
		if (s->z > hittype[i].floorz - (48 << 8))
			s->z = hittype[i].floorz - (48 << 8);
	}

	int x;
	int p = findplayer(s, &x);
	j = s->owner;

	// 3 = findplayerz, 4 = shoot

	if (t[0] >= 4)
	{
		t[2]++;
		if ((t[2] & 15) == 0)
		{
			a = s->ang;
			s->ang = hittype[i].tempang;
			if (attacksnd >= 0) spritesound(attacksnd, i);
			shoot(i, firelaser);
			s->ang = a;
		}
		if (t[2] > (26 * 3) || !cansee(s->x, s->y, s->z - (16 << 8), s->sectnum, ps[p].posx, ps[p].posy, ps[p].posz, ps[p].cursectnum))
		{
			t[0] = 0;
			t[2] = 0;
		}
		else hittype[i].tempang +=
			getincangle(hittype[i].tempang, getangle(ps[p].posx - s->x, ps[p].posy - s->y)) / 3;
	}
	else if (t[0] == 2 || t[0] == 3)
	{
		t[3] = 0;
		if (s->xvel > 0) s->xvel -= 16;
		else s->xvel = 0;

		if (t[0] == 2)
		{
			int l = ps[p].posz - s->z;
			if (abs(l) < (48 << 8)) t[0] = 3;
			else s->z += sgn(ps[p].posz - s->z) << shift; // The shift here differs between Duke and RR.
		}
		else
		{
			t[2]++;
			if (t[2] > (26 * 3) || !cansee(s->x, s->y, s->z - (16 << 8), s->sectnum, ps[p].posx, ps[p].posy, ps[p].posz, ps[p].cursectnum))
			{
				t[0] = 1;
				t[2] = 0;
			}
			else if ((t[2] & 15) == 0 && attacksnd >= 0)
			{
				spritesound(attacksnd, i);
				shoot(i, firelaser);
			}
		}
		s->ang += getincangle(s->ang, getangle(ps[p].posx - s->x, ps[p].posy - s->y)) >> 2;
	}

	if (t[0] != 2 && t[0] != 3)
	{
		int l = ldist(&sprite[j], s);
		if (l <= 1524)
		{
			a = s->ang;
			s->xvel >>= 1;
		}
		else a = getangle(sprite[j].x - s->x, sprite[j].y - s->y);

		if (t[0] == 1 || t[0] == 4) // Found a locator and going with it
		{
			l = dist(&sprite[j], s);

			if (l <= 1524) { if (t[0] == 1) t[0] = 0; else t[0] = 5; }
			else
			{
				// Control speed here
				if (l > 1524) { if (s->xvel < 256) s->xvel += 32; }
				else
				{
					if (s->xvel > 0) s->xvel -= 16;
					else s->xvel = 0;
				}
			}

			if (t[0] < 2) t[2]++;

			if (x < 6144 && t[0] < 2 && t[2] > (26 * 4))
			{
				t[0] = 2 + (krand() & 2);
				t[2] = 0;
				hittype[i].tempang = s->ang;
			}
		}

		if (t[0] == 0 || t[0] == 5)
		{
			if (t[0] == 0)
				t[0] = 1;
			else t[0] = 4;
			j = s->owner = LocateTheLocator(s->hitag, -1);
			if (j == -1)
			{
				s->hitag = j = hittype[i].temp_data[5];
				s->owner = LocateTheLocator(j, -1);
				j = s->owner;
				if (j == -1)
				{
					deletesprite(i);
					return;
				}
			}
			else s->hitag++;
		}

		t[3] = getincangle(s->ang, a);
		s->ang += t[3] >> 3;

		if (s->z < sprite[j].z)
			s->z += 1024;
		else s->z -= 1024;
	}

	if (roamsnd >= 0 && S_CheckSoundPlaying(roamsnd) < 2)
		A_PlaySound(roamsnd, i);

	ssp(i, CLIPMASK0);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void ooz(int i)
{
	getglobalz(i);

	int j = (hittype[i].floorz - hittype[i].ceilingz) >> 9;
	if (j > 255) j = 255;

	int x = 25 - (j >> 1);
	if (x < 8) x = 8;
	else if (x > 48) x = 48;

	spritetype* s = &sprite[i];
	s->yrepeat = j;
	s->xrepeat = x;
	s->z = hittype[i].floorz;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void reactor(int i, int REACTOR, int REACTOR2, int REACTORBURNT, int REACTOR2BURNT)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	int sect = s->sectnum;

	if (t[4] == 1)
	{
		int j = headspritesect[sect];
		while (j >= 0)
		{
			if (sprite[j].picnum == SECTOREFFECTOR)
			{
				if (sprite[j].lotag == 1)
				{
					sprite[j].lotag = (short)65535;
					sprite[j].hitag = (short)65535;
				}
			}
			else if (sprite[j].picnum == REACTOR)
			{
				sprite[j].picnum = REACTORBURNT;
			}
			else if (sprite[j].picnum == REACTOR2)
			{
				sprite[j].picnum = REACTOR2BURNT;
			}
			else if (sprite[j].picnum == REACTORBURNT || sprite[j].picnum == REACTOR2BURNT)
			{
				sprite[j].cstat = (short)32768;
			}
			j = nextspritesect[j];
		}
		return;
	}

	if (t[1] >= 20)
	{
		t[4] = 1;
		return;
	}

	int x;
	int p = findplayer(s, &x);

	t[2]++;
	if (t[2] == 4) t[2] = 0;

	if (x < 4096)
	{
		if ((krand() & 255) < 16)
		{
			if (!S_CheckSoundPlaying(DUKE_LONGTERM_PAIN))
				spritesound(DUKE_LONGTERM_PAIN, ps[p].i);

			spritesound(SHORT_CIRCUIT, i);

			sprite[ps[p].i].extra--;
			SetPlayerPal(&ps[p], PalEntry(32, 32, 0, 0));
		}
		t[0] += 128;
		if (t[3] == 0)
			t[3] = 1;
	}
	else t[3] = 0;

	if (t[1])
	{
		int j;
		t[1]++;

		t[4] = s->z;
		s->z = sector[sect].floorz - (krand() % (sector[sect].floorz - sector[sect].ceilingz));

		switch (t[1])
		{
		case 3:
			//Turn on all of those flashing sectoreffector.
			hitradius(i, 4096,
				impact_damage << 2,
				impact_damage << 2,
				impact_damage << 2,
				impact_damage << 2);
			j = headspritestat[6];
			while (j >= 0)
			{
				if (sprite[j].picnum == MASTERSWITCH)
					if (sprite[j].hitag == s->hitag)
						if (sprite[j].yvel == 0)
							sprite[j].yvel = 1;
				j = nextspritestat[j];
			}
			break;

		case 4:
		case 7:
		case 10:
		case 15:
			j = headspritesect[sect];
			while (j >= 0)
			{
				int l = nextspritesect[j];

				if (j != i)
				{
					deletesprite(j);
					return;
				}
				j = l;
			}
			break;
		}
		for (x = 0; x < 16; x++)
			RANDOMSCRAP(s, i);

		s->z = t[4];
		t[4] = 0;

	}
	else
	{
		int j = ifhitbyweapon(i);
		if (j >= 0)
		{
			for (x = 0; x < 32; x++)
				RANDOMSCRAP(s, i);
			if (s->extra < 0)
				t[1] = 1;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void camera(int i)
{
	spritetype* s = &sprite[i];
	auto t = &hittype[i].temp_data[0];
	if (t[0] == 0)
	{
		t[1] += 8;
		if (camerashitable)
		{
			int j = ifhitbyweapon(i);
			if (j >= 0)
			{
				t[0] = 1; // static
				s->cstat = (short)32768;
				for (int x = 0; x < 5; x++)
					RANDOMSCRAP(s, i);
				return;
			}
		}

		if (s->hitag > 0)
		{
			if (t[1] < s->hitag)
				s->ang += 8;
			else if (t[1] < (s->hitag * 3))
				s->ang -= 8;
			else if (t[1] < (s->hitag << 2))
				s->ang += 8;
			else
			{
				t[1] = 8;
				s->ang += 16;
			}
		}
	}
}

END_DUKE_NS
