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
#include "concmd.h"
#include "duke3d.h"
#include "gamevar.h"
#include "mapinfo.h"

BEGIN_DUKE_NS

// Player Actions - used by ifp instruction.
enum playeraction_t {
	pstanding                   = 0x00000001,
	pwalking                    = 0x00000002,
	prunning                    = 0x00000004,
	pducking                    = 0x00000008,
	pfalling                    = 0x00000010,
	pjumping                    = 0x00000020,
	phigher                     = 0x00000040,
	pwalkingback                = 0x00000080,
	prunningback                = 0x00000100,
	pkicking                    = 0x00000200,
	pshrunk                     = 0x00000400,
	pjetpack                    = 0x00000800,
	ponsteroids                 = 0x00001000,
	ponground                   = 0x00002000,
	palive                      = 0x00004000,
	pdead                       = 0x00008000,
	pfacing                     = 0x00010000
};



struct ParseState
{
	int g_i, g_p;
	int g_x;
	int* g_t;
	uint8_t killit_flag;
	spritetype* g_sp;
	int* insptr;

	int parse(void);
	void parseifelse(int condition);
};

int furthestcanseepoint(int i, spritetype* ts, int* dax, int* day);
bool ifsquished(int i, int p);
void fakebubbaspawn(int g_i, int g_p);
void tearitup(int sect);
void destroyit(int g_i);
void mamaspawn(int g_i);
void forceplayerangle(struct player_struct* p);

bool killthesprite = false;

void addspritetodelete(int spnum)
{
	killthesprite = true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------
void VM_Execute(native_t loop);

void ParseState::parseifelse(int condition)
{
	if( condition )
	{
		// skip 'else' pointer.. and...
		insptr+=2;
		parse();
	}
	else
	{
		insptr = &ScriptCode[*(insptr+1)];
		if(*insptr == 10)
		{
			// else...

			// skip 'else' and...
			insptr+=2;
			parse();
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static int ifcanshoottarget(int g_i, int g_p, int g_x)
{
	int j;
	auto g_sp = &sprite[g_i];
	if (g_x > 1024)
	{
		short temphit, sclip, angdif;

		if (badguy(g_sp) && g_sp->xrepeat > 56)
		{
			sclip = 3084;
			angdif = 48;
		}
		else
		{
			sclip = 768;
			angdif = 16;
		}

		j = hitasprite(g_i, &temphit);
		if (j == (1 << 30))
		{
			return 1;
		}
		if (j > sclip)
		{
			if (temphit >= 0 && sprite[temphit].picnum == g_sp->picnum)
				j = 0;
			else
			{
				g_sp->ang += angdif; j = hitasprite(g_i, &temphit); g_sp->ang -= angdif;
				if (j > sclip)
				{
					if (temphit >= 0 && sprite[temphit].picnum == g_sp->picnum)
						j = 0;
					else
					{
						g_sp->ang -= angdif; j = hitasprite(g_i, &temphit); g_sp->ang += angdif;
						if (j > 768)
						{
							if (temphit >= 0 && sprite[temphit].picnum == g_sp->picnum)
								j = 0;
							else j = 1;
						}
						else j = 0;
					}
				}
				else j = 0;
			}
		}
		else j = 0;
	}
	else j = 1;
	return j;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

static bool ifcansee(int g_i, int g_p)
{
	auto g_sp = &sprite[g_i];
	spritetype* s;
	int j;

	// select sprite for monster to target
	// if holoduke is on, let them target holoduke first.
	// 
	if (ps[g_p].holoduke_on >= 0 && !isRR())
	{
		s = &sprite[ps[g_p].holoduke_on];
		j = cansee(g_sp->x, g_sp->y, g_sp->z - (krand() & ((32 << 8) - 1)), g_sp->sectnum,
			s->x, s->y, s->z, s->sectnum);

		if (j == 0)
		{
			// they can't see player's holoduke
			// check for player...
			s = &sprite[ps[g_p].i];
		}
	}
	else s = &sprite[ps[g_p].i];	// holoduke not on. look for player

	// can they see player, (or player's holoduke)
	j = cansee(g_sp->x, g_sp->y, g_sp->z - (krand() & ((47 << 8))), g_sp->sectnum,
		s->x, s->y, s->z - ((isRR()? 28 : 24) << 8), s->sectnum);

	if (j == 0)
	{
		// they can't see it.

		// Huh?.  This does nothing....
		// (the result is always j==0....)
		if ((abs(hittype[g_i].lastvx - g_sp->x) + abs(hittype[g_i].lastvy - g_sp->y)) <
			(abs(hittype[g_i].lastvx - s->x) + abs(hittype[g_i].lastvy - s->y)))
			j = 0;

		// um yeah, this if() will always fire....
		if (j == 0)
		{
			// search around for target player

			// also modifies 'target' x&y if found..

			j = furthestcanseepoint(g_i, s, &hittype[g_i].lastvx, &hittype[g_i].lastvy);

			if (j == -1) j = 0;
			else j = 1;
		}
	}
	else
	{
		// else, they did see it.
		// save where we were looking...
		hittype[g_i].lastvx = s->x;
		hittype[g_i].lastvy = s->y;
	}

	if (j == 1 && (g_sp->statnum == 1 || g_sp->statnum == 6))
		hittype[g_i].timetosleep = SLEEPTIME;

	return j == 1;
}


// int *it = 0x00589a04;

int ParseState::parse(void)
{
	int j, l, s;

	if(killit_flag) return 1;

	switch (*insptr)
	{
	case concmd_ifrnd:
	{
		insptr++;
		// HACK ALERT! The fire animation uses a broken ifrnd setup to delay its start because original CON has no variables.
		// But the chosen random value of 16/255 is too low and can cause delays of a second or more.
		int spnum = sprite[g_i].picnum;
		if (spnum == TILE_FIRE && g_t[4] == 0 && *insptr == 16)
		{
			parseifelse(rnd(64));
			break;
		}
		parseifelse(rnd(*insptr));
		break;
	}
	case concmd_ifcanshoottarget:
		parseifelse(ifcanshoottarget(g_i, g_p, g_x));
		break;
	case concmd_ifcanseetarget:
		j = cansee(g_sp->x, g_sp->y, g_sp->z - ((krand() & 41) << 8), g_sp->sectnum, ps[g_p].posx, ps[g_p].posy, ps[g_p].posz/*-((krand()&41)<<8)*/, sprite[ps[g_p].i].sectnum);
		parseifelse(j);
		if (j) hittype[g_i].timetosleep = SLEEPTIME;
		break;
	case concmd_ifnocover:
		j = cansee(g_sp->x, g_sp->y, g_sp->z, g_sp->sectnum, ps[g_p].posx, ps[g_p].posy, ps[g_p].posz, sprite[ps[g_p].i].sectnum);
		parseifelse(j);
		if (j) hittype[g_i].timetosleep = SLEEPTIME;
		break;

	case concmd_ifactornotstayput:
		parseifelse(hittype[g_i].actorstayput == -1);
		break;
	case concmd_ifcansee:
		parseifelse(ifcansee(g_i, g_p));
		break;

	case concmd_ifhitweapon:
		parseifelse(fi.ifhitbyweapon(g_i) >= 0);
		break;
	case concmd_ifsquished:
		parseifelse(ifsquished(g_i, g_p) == 1);
		break;
	case concmd_ifdead:
	{
		j = g_sp->extra;
		if (g_sp->picnum == TILE_APLAYER)
			j--;
		parseifelse(j < 0);
	}
	break;
	case concmd_ai:
		insptr++;
		g_t[5] = *insptr;
		g_t[4] = ScriptCode[g_t[5]];		  // Action
		g_t[1] = ScriptCode[g_t[5] + 1];		// move
		g_sp->hitag = ScriptCode[g_t[5] + 2];	  // Ai
		g_t[0] = g_t[2] = g_t[3] = 0;
		if (g_sp->hitag & random_angle)
			g_sp->ang = krand() & 2047;
		insptr++;
		break;
	case concmd_action:
		insptr++;
		g_t[2] = 0;
		g_t[3] = 0;
		g_t[4] = *insptr;
		insptr++;
		break;

	case concmd_ifpdistl:
		insptr++;
		parseifelse(g_x < *insptr);
		if (g_x > MAXSLEEPDIST && hittype[g_i].timetosleep == 0)
			hittype[g_i].timetosleep = SLEEPTIME;
		break;
	case concmd_ifpdistg:
		insptr++;
		parseifelse(g_x > * insptr);
		if (g_x > MAXSLEEPDIST && hittype[g_i].timetosleep == 0)
			hittype[g_i].timetosleep = SLEEPTIME;
		break;
	case concmd_else:
		insptr = &ScriptCode[*(insptr + 1)];
		break;
	case concmd_addstrength:
		insptr++;
		g_sp->extra += *insptr;
		insptr++;
		break;
	case concmd_strength:
		insptr++;
		g_sp->extra = *insptr;
		insptr++;
		break;
	case concmd_smacksprite:
		switch (krand() & 1)
		{
		case 0:
			g_sp->ang = (+512 + g_sp->ang + (krand() & 511)) & 2047;
			break;
		case 1:
			g_sp->ang = (-512 + g_sp->ang - (krand() & 511)) & 2047;
			break;
		}
		insptr++;
		break;
	case concmd_fakebubba:
		insptr++;
		fakebubbaspawn(g_i, g_p);
		break;

	case concmd_rndmove:
		g_sp->ang = krand() & 2047;
		g_sp->xvel = 25;
		insptr++;
		break;
	case concmd_mamatrigger:
		operateactivators(667, ps[g_p].i);
		insptr++;
		break;
	case concmd_mamaspawn:
		mamaspawn(g_i);
		insptr++;
		break;
	case concmd_mamaquake:
		if (g_sp->pal == 31)
			earthquaketime = 4;
		else if (g_sp->pal == 32)
			earthquaketime = 6;
		insptr++;
		break;
	case concmd_garybanjo:
		if (banjosound == 0)
		{
			short rnum = (krand() & 3) + 1;
			if (rnum == 4)
			{
				banjosound = 262;
			}
			else if (rnum == 1)
			{
				banjosound = 272;
			}
			else if (rnum == 2)
			{
				banjosound = 273;
			}
			else
			{
				banjosound = 273;
			}
			S_PlayActorSound(banjosound, g_i, CHAN_WEAPON);
		}
		else if (!S_CheckSoundPlaying(g_i, banjosound))
			S_PlayActorSound(banjosound, g_i, CHAN_WEAPON);
		insptr++;
		break;
	case concmd_motoloopsnd:
		if (!S_CheckSoundPlaying(g_i, 411))
			S_PlayActorSound(411, g_i, CHAN_VOICE);
		insptr++;
		break;
	case concmd_ifgotweaponce:
		insptr++;

		if (ud.coop >= 1 && ud.multimode > 1)
		{
			if (*insptr == 0)
			{
				for (j = 0; j < ps[g_p].weapreccnt; j++)
					if (ps[g_p].weaprecs[j] == g_sp->picnum)
						break;

				parseifelse(j < ps[g_p].weapreccnt&& g_sp->owner == g_i);
			}
			else if (ps[g_p].weapreccnt < 16)
			{
				ps[g_p].weaprecs[ps[g_p].weapreccnt++] = g_sp->picnum;
				parseifelse(g_sp->owner == g_i);
			}
		}
		else parseifelse(0);
		break;
	case concmd_getlastpal:
		insptr++;
		if (g_sp->picnum == TILE_APLAYER)
			g_sp->pal = ps[g_sp->yvel].palookup;
		else
		{
			// Copied from DukeGDX.
			if (g_sp->picnum == TILE_EGG && hittype[g_i].temp_data[5] == TILE_EGG + 2 && g_sp->pal == 1) 
			{
				ps[connecthead].max_actors_killed++; //revive the egg
				hittype[g_i].temp_data[5] = 0;
			}
			g_sp->pal = hittype[g_i].tempang;
		}
		hittype[g_i].tempang = 0;
		break;
	case concmd_tossweapon:
		insptr++;
		fi.checkweapons(&ps[g_sp->yvel]);
		break;
	case concmd_nullop:
		insptr++;
		break;
	case concmd_mikesnd:
		insptr++;
		if (!S_CheckSoundPlaying(g_i, g_sp->yvel))
			S_PlayActorSound(g_sp->yvel, g_i, CHAN_VOICE);
		break;
	case concmd_pkick:
		insptr++;

		if (ud.multimode > 1 && g_sp->picnum == TILE_APLAYER)
		{
			if (ps[otherp].quick_kick == 0)
				ps[otherp].quick_kick = 14;
		}
		else if (g_sp->picnum != TILE_APLAYER && ps[g_p].quick_kick == 0)
			ps[g_p].quick_kick = 14;
		break;
	case concmd_sizeto:
		insptr++;

		// JBF 20030805: As I understand it, if xrepeat becomes 0 it basically kills the
		// sprite, which is why the "sizeto 0 41" calls in 1.3d became "sizeto 4 41" in
		// 1.4, so instead of patching the CONs I'll surruptitiously patch the code here
		//if (!PLUTOPAK && *insptr == 0) *insptr = 4;

		j = ((*insptr) - g_sp->xrepeat) << 1;
		g_sp->xrepeat += ksgn(j);

		insptr++;

		if ((g_sp->picnum == TILE_APLAYER && g_sp->yrepeat < 36) || *insptr < g_sp->yrepeat || ((g_sp->yrepeat * (tilesiz[g_sp->picnum].y + 8)) << 2) < (hittype[g_i].floorz - hittype[g_i].ceilingz))
		{
			j = ((*insptr) - g_sp->yrepeat) << 1;
			if (abs(j)) g_sp->yrepeat += ksgn(j);
		}

		insptr++;

		break;
	case concmd_sizeat:
		insptr++;
		g_sp->xrepeat = (uint8_t)*insptr;
		insptr++;
		g_sp->yrepeat = (uint8_t)*insptr;
		insptr++;
		break;
	case concmd_shoot:
		insptr++;
		fi.shoot(g_i, (short)*insptr);
		insptr++;
		break;
	case concmd_ifsoundid:
		insptr++;
		parseifelse((short)*insptr == ambientlotag[g_sp->ang]);
		break;
	case concmd_ifsounddist:
		insptr++;
		if (*insptr == 0)
			parseifelse(ambienthitag[g_sp->ang] > g_x);
		else if (*insptr == 1)
			parseifelse(ambienthitag[g_sp->ang] < g_x);
		break;
	case concmd_soundtag:
		insptr++;
		S_PlayActorSound(ambientlotag[g_sp->ang], g_i);
		break;
	case concmd_soundtagonce:
		insptr++;
		if (!S_CheckSoundPlaying(g_i, ambientlotag[g_sp->ang]))
			S_PlayActorSound(ambientlotag[g_sp->ang], g_i);
		break;
	case concmd_soundonce:
		insptr++;
		if (!S_CheckSoundPlaying(g_i, *insptr++))
			S_PlayActorSound(*(insptr - 1), g_i);
		break;
	case concmd_stopsound:
		insptr++;
		if (S_CheckSoundPlaying(g_i, *insptr))
			S_StopSound((int)*insptr);
		insptr++;
		break;
	case concmd_globalsound:
		insptr++;
		if (g_p == screenpeek || ud.coop == 1)
			S_PlayActorSound((int)*insptr, ps[screenpeek].i);
		insptr++;
		break;
	case concmd_smackbubba:
		insptr++;
		if (!isRRRA() || g_sp->pal != 105)
		{
			setnextmap(false);
		}
		break;
	case concmd_mamaend:
		insptr++;
		ps[myconnectindex].MamaEnd = 150;
		break;

	case concmd_ifactorhealthg:
		insptr++;
		parseifelse(g_sp->extra > (short)*insptr);
		break;
	case concmd_ifactorhealthl:
		insptr++;
		parseifelse(g_sp->extra < (short)*insptr);
		break;
	case concmd_sound:
		insptr++;
		S_PlayActorSound((short) *insptr,g_i);
		insptr++;
		break;
	case concmd_tip:
		insptr++;
		ps[g_p].tipincs = 26;
		break;
	case concmd_iftipcow:
	case concmd_ifhittruck: // both have the same code.
		if (hittype[g_i].spriteextra == 1) // 
		{
			j = 1;
			hittype[g_i].spriteextra++;
		}
		else
			j = 0;
		parseifelse(j > 0);
		break;
	case concmd_tearitup:
		insptr++;
		tearitup(g_sp->sectnum);
		break;
	case concmd_fall:
		insptr++;
		g_sp->xoffset = 0;
		g_sp->yoffset = 0;
		fi.fall(g_i, g_p);
		break;
	case concmd_enda:
	case concmd_break:
	case concmd_ends:
	case concmd_endevent:
		return 1;
	case concmd_rightbrace:
		insptr++;
		return 1;
	case concmd_addammo:
		insptr++;
		if( ps[g_p].ammo_amount[*insptr] >= max_ammo_amount[*insptr] )
		{
			killit_flag = 2;
			break;
		}
		addammo( *insptr, &ps[g_p], *(insptr+1) );
		if(ps[g_p].curr_weapon == KNEE_WEAPON)
			if( ps[g_p].gotweapon[*insptr] )
				fi.addweapon( &ps[g_p], *insptr );
		insptr += 2;
		break;
	case concmd_money:
		insptr++;
		fi.lotsofmoney(g_sp,*insptr);
		insptr++;
		break;
	case concmd_mail:
		insptr++;
		fi.lotsofmail(g_sp,*insptr);
		insptr++;
		break;
	case concmd_sleeptime:
		insptr++;
		hittype[g_i].timetosleep = (short)*insptr;
		insptr++;
		break;
	case concmd_paper:
		insptr++;
		fi.lotsofpaper(g_sp,*insptr);
		insptr++;
		break;
	case concmd_addkills:
		insptr++;
		if (isRR())
		{
			if (hittype[g_i].spriteextra < 1 || hittype[g_i].spriteextra == 128)
			{
				if (actorfella(g_i))
					ps[g_p].actors_killed += *insptr;
			}
		}
		else ps[g_p].actors_killed += *insptr;
		hittype[g_i].actorstayput = -1;
		insptr++;
		break;
	case concmd_lotsofglass:
		insptr++;
		spriteglass(g_i,*insptr);
		insptr++;
		break;
	case concmd_killit:
		insptr++;
		killit_flag = 1;
		break;
	case concmd_addweapon:
		insptr++;
		if( ps[g_p].gotweapon[*insptr] == 0 ) fi.addweapon( &ps[g_p], *insptr );
		else if( ps[g_p].ammo_amount[*insptr] >= max_ammo_amount[*insptr] )
		{
				killit_flag = 2;
				break;
		}
		addammo( *insptr, &ps[g_p], *(insptr+1) );
		if(ps[g_p].curr_weapon == KNEE_WEAPON)
			if( ps[g_p].gotweapon[*insptr] )
				fi.addweapon( &ps[g_p], *insptr );
		insptr+=2;
		break;
	case concmd_debug:
		insptr++;
		Printf("%d\n",*insptr);
		insptr++;
		break;
	case concmd_endofgame:
		insptr++;
		ps[g_p].timebeforeexit = *insptr;
		ps[g_p].customexitsound = -1;
		ud.eog = 1;
		insptr++;
		break;

	case concmd_isdrunk: // todo: move out to player_r.
		insptr++;
		ps[g_p].drink_amt += *insptr;
		j = sprite[ps[g_p].i].extra;
		if (j > 0)
			j += *insptr;
		if (j > max_player_health * 2)
			j = max_player_health * 2;
		if (j < 0)
			j = 0;

		if (ud.god == 0)
		{
			if (*insptr > 0)
			{
				if ((j - *insptr) < (max_player_health >> 2) &&
					j >= (max_player_health >> 2))
					S_PlayActorSound(DUKE_GOTHEALTHATLOW, ps[g_p].i);

				ps[g_p].last_extra = j;
			}

			sprite[ps[g_p].i].extra = j;
		}
		if (ps[g_p].drink_amt > 100)
			ps[g_p].drink_amt = 100;

		if (sprite[ps[g_p].i].extra >= max_player_health)
		{
			sprite[ps[g_p].i].extra = max_player_health;
			ps[g_p].last_extra = max_player_health;
		}
		insptr++;
		break;
	case concmd_strafeleft:
		insptr++;
		fi.movesprite(g_i, sintable[(g_sp->ang + 1024) & 2047] >> 10, sintable[(g_sp->ang + 512) & 2047] >> 10, g_sp->zvel, CLIPMASK0);
		break;
	case concmd_straferight:
		insptr++;
		fi.movesprite(g_i, sintable[(g_sp->ang - 0) & 2047] >> 10, sintable[(g_sp->ang - 512) & 2047] >> 10, g_sp->zvel, CLIPMASK0);
		break;
	case concmd_larrybird:
		insptr++;
		ps[g_p].posz = sector[sprite[ps[g_p].i].sectnum].ceilingz;
		sprite[ps[g_p].i].z = ps[g_p].posz;
		break;
	case concmd_destroyit:
		insptr++;
		destroyit(g_i);
		break;
	case concmd_iseat: // move out to player_r.
		insptr++;
		ps[g_p].eat += *insptr;
		if (ps[g_p].eat > 100)
		{
			ps[g_p].eat = 100;
		}
		ps[g_p].drink_amt -= *insptr;
		if (ps[g_p].drink_amt < 0)
			ps[g_p].drink_amt = 0;
		j = sprite[ps[g_p].i].extra;
		if (g_sp->picnum != TILE_ATOMICHEALTH)
		{
			if (j > max_player_health && *insptr > 0)
			{
				insptr++;
				break;
			}
			else
			{
				if (j > 0)
					j += (*insptr) * 3;
				if (j > max_player_health && *insptr > 0)
					j = max_player_health;
			}
		}
		else
		{
			if (j > 0)
				j += *insptr;
			if (j > (max_player_health << 1))
				j = (max_player_health << 1);
		}

		if (j < 0) j = 0;

		if (ud.god == 0)
		{
			if (*insptr > 0)
			{
				if ((j - *insptr) < (max_player_health >> 2) &&
					j >= (max_player_health >> 2))
					S_PlayActorSound(229, ps[g_p].i);

				ps[g_p].last_extra = j;
			}

			sprite[ps[g_p].i].extra = j;
		}

		insptr++;
		break;

	case concmd_addphealth: // todo: move out to player.
		insptr++;

		if(!isRR() && ps[g_p].newowner >= 0)
		{
			ps[g_p].newowner = -1;
			ps[g_p].posx = ps[g_p].oposx;
			ps[g_p].posy = ps[g_p].oposy;
			ps[g_p].posz = ps[g_p].oposz;
			ps[g_p].q16ang = ps[g_p].oq16ang;
			updatesector(ps[g_p].posx,ps[g_p].posy,&ps[g_p].cursectnum);
			setpal(&ps[g_p]);

			j = headspritestat[1];
			while (j >= 0)
			{
				if (sprite[j].picnum == TILE_CAMERA1)
					sprite[j].yvel = 0;
				j = nextspritestat[j];
			}
		}

		j = sprite[ps[g_p].i].extra;

		if(g_sp->picnum != TILE_ATOMICHEALTH)
		{
			if( j > max_player_health && *insptr > 0 )
			{
				insptr++;
				break;
			}
			else
			{
				if(j > 0)
					j += *insptr;
				if ( j > max_player_health && *insptr > 0 )
					j = max_player_health;
			}
		}
		else
		{
			if( j > 0 )
				j += *insptr;
			if ( j > (max_player_health<<1) )
				j = (max_player_health<<1);
		}

		if(j < 0) j = 0;

		if(ud.god == 0)
		{
			if(*insptr > 0)
			{
				if( ( j - *insptr ) < (max_player_health>>2) &&
					j >= (max_player_health>>2) )
						S_PlayActorSound(isRR()? 229 : DUKE_GOTHEALTHATLOW,ps[g_p].i);

				ps[g_p].last_extra = j;
			}

			sprite[ps[g_p].i].extra = j;
		}

		insptr++;
		break;

	case concmd_state:
		{
			auto tempscrptr = insptr + 2;
			insptr = &ScriptCode[*(insptr + 1)];
			while (1) if (parse()) break;
			insptr = tempscrptr;
		}
		break;
	case concmd_leftbrace:
		insptr++;
		while (1) if (parse()) break;
		break;
	case concmd_move:
		g_t[0]=0;
		insptr++;
		g_t[1] = *insptr;
		insptr++;
		g_sp->hitag = *insptr;
		insptr++;
		if(g_sp->hitag&random_angle)
			g_sp->ang = krand()&2047;
		break;
	case concmd_spawn:
		insptr++;
		if(g_sp->sectnum >= 0 && g_sp->sectnum < MAXSECTORS)
			fi.spawn(g_i,*insptr);
		insptr++;
		break;
	case concmd_ifwasweapon:
	case concmd_ifspawnedby:	// these two are the same
		insptr++;
		parseifelse( hittype[g_i].picnum == *insptr);
		break;
	case concmd_ifai:
		insptr++;
		parseifelse(g_t[5] == *insptr);
		break;
	case concmd_ifaction:
		insptr++;
		parseifelse(g_t[4] == *insptr);
		break;
	case concmd_ifactioncount:
		insptr++;
		parseifelse(g_t[2] >= *insptr);
		break;
	case concmd_resetactioncount:
		insptr++;
		g_t[2] = 0;
		break;
	case concmd_debris:
	{
			short dnum;

			insptr++;
			dnum = *insptr;
			insptr++;
			bool weap = fi.spawnweapondebris(g_sp->picnum, dnum);

			if(g_sp->sectnum >= 0 && g_sp->sectnum < MAXSECTORS)
				for(j=(*insptr)-1;j>=0;j--)
			{
				if(weap)
					s = 0;
				else s = (krand()%3);

				l = EGS(g_sp->sectnum,
					g_sp->x + (krand() & 255) - 128, g_sp->y + (krand() & 255) - 128, g_sp->z - (8 << 8) - (krand() & 8191),
					dnum + s, g_sp->shade, 32 + (krand() & 15), 32 + (krand() & 15),
					krand() & 2047, (krand() & 127) + 32, -(krand() & 2047), g_i, 5);
				if(weap)
					sprite[l].yvel = weaponsandammosprites[j%14];
				else sprite[l].yvel = -1;
				sprite[l].pal = g_sp->pal;
			}
			insptr++;
		}
		break;
	case concmd_count:
		insptr++;
		g_t[0] = (short) *insptr;
		insptr++;
		break;
	case concmd_cstator:
		insptr++;
		g_sp->cstat |= (short)*insptr;
		insptr++;
		break;
	case concmd_clipdist:
		insptr++;
		g_sp->clipdist = (short) *insptr;
		insptr++;
		break;
	case concmd_cstat:
		insptr++;
		g_sp->cstat = (short) *insptr;
		insptr++;
		break;
	case concmd_newpic:
		insptr++;
		g_sp->picnum = (short)*insptr;
		insptr++;
		break;
	case concmd_ifmove:
		insptr++;
		parseifelse(g_t[1] == *insptr);
		break;
	case concmd_resetplayer:
		insptr++;

//AddLog("resetplayer");				
		if(ud.multimode < 2)
		{
#if 0
			if( lastsavedpos >= 0 && ud.recstat != 2 )
			{
				KB_ClearKeyDown(sc_Space);
				cmenu(15000);
			}
			else
#endif
			ps[g_p].gm = MODE_RESTART;
			killit_flag = 2;
		}
		else
		{
			// I am not convinced this is even remotely smart to be executed from here...
			pickrandomspot(g_p);
			g_sp->x = hittype[g_i].bposx = ps[g_p].bobposx = ps[g_p].oposx = ps[g_p].posx;
			g_sp->y = hittype[g_i].bposy = ps[g_p].bobposy = ps[g_p].oposy = ps[g_p].posy;
			g_sp->z = hittype[g_i].bposy = ps[g_p].oposz = ps[g_p].posz;
			updatesector(ps[g_p].posx, ps[g_p].posy, &ps[g_p].cursectnum);
			setsprite(ps[g_p].i, ps[g_p].posx, ps[g_p].posy, ps[g_p].posz + PHEIGHT);
			g_sp->cstat = 257;

			g_sp->shade = -12;
			g_sp->clipdist = 64;
			g_sp->xrepeat = 42;
			g_sp->yrepeat = 36;
			g_sp->owner = g_i;
			g_sp->xoffset = 0;
			g_sp->pal = ps[g_p].palookup;

			ps[g_p].last_extra = g_sp->extra = max_player_health;
			ps[g_p].wantweaponfire = -1;
			ps[g_p].sethoriz(100);
			ps[g_p].on_crane = -1;
			ps[g_p].frag_ps = g_p;
			ps[g_p].sethorizoff(0);
			ps[g_p].opyoff = 0;
			ps[g_p].wackedbyactor = -1;
			ps[g_p].shield_amount = max_armour_amount;
			ps[g_p].dead_flag = 0;
			ps[g_p].pals.a = 0;
			ps[g_p].footprintcount = 0;
			ps[g_p].weapreccnt = 0;
			ps[g_p].ftq = 0;
			ps[g_p].posxv = ps[g_p].posyv = 0;
			if (!isRR()) ps[g_p].setrotscrnang(0);

			ps[g_p].falling_counter = 0;

			hittype[g_i].extra = -1;
			hittype[g_i].owner = g_i;

			hittype[g_i].cgg = 0;
			hittype[g_i].movflag = 0;
			hittype[g_i].tempang = 0;
			hittype[g_i].actorstayput = -1;
			hittype[g_i].dispicnum = 0;
			hittype[g_i].owner = ps[g_p].i;
			hittype[g_i].temp_data[4] = 0;

			resetinventory(g_p);
			resetweapons(g_p);

			//cameradist = 0;
			//cameraclock = totalclock;
		}
		setpal(&ps[g_p]);
		break;
	case concmd_ifcoop:
		parseifelse(ud.coop || numplayers > 2);
		break;
	case concmd_ifonmud:
		parseifelse(abs(g_sp->z - sector[g_sp->sectnum].floorz) < (32 << 8) && sector[g_sp->sectnum].floorpicnum == 3073); // eew, hard coded tile numbers... :?
		break;
	case concmd_ifonwater:
		parseifelse( abs(g_sp->z-sector[g_sp->sectnum].floorz) < (32<<8) && sector[g_sp->sectnum].lotag == ST_1_ABOVE_WATER);
		break;
	case concmd_ifmotofast:
		parseifelse(ps[g_p].MotoSpeed > 60);
		break;
	case concmd_ifonmoto:
		parseifelse(ps[g_p].OnMotorcycle == 1);
		break;
	case concmd_ifonboat:
		parseifelse(ps[g_p].OnBoat == 1);
		break;
	case concmd_ifsizedown:
		g_sp->xrepeat--;
		g_sp->yrepeat--;
		parseifelse(g_sp->xrepeat <= 5);
		break;
	case concmd_ifwind:
		parseifelse(WindTime > 0);
		break;

	case concmd_ifinwater:
		parseifelse( sector[g_sp->sectnum].lotag == 2);
		break;
	case concmd_ifcount:
		insptr++;
		parseifelse(g_t[0] >= *insptr);
		break;
	case concmd_ifactor:
		insptr++;
		parseifelse(g_sp->picnum == *insptr);
		break;
	case concmd_resetcount:
		insptr++;
		g_t[0] = 0;
		break;
	case concmd_addinventory:
		insptr+=2;
		switch(*(insptr-1))
		{
			case 0:
				ps[g_p].steroids_amount = *insptr;
				ps[g_p].inven_icon = 2;
				break;
			case 1:
				ps[g_p].shield_amount +=		  *insptr;// 100;
				if(ps[g_p].shield_amount > max_player_health)
					ps[g_p].shield_amount = max_player_health;
				break;
			case 2:
				ps[g_p].scuba_amount =			   *insptr;// 1600;
				ps[g_p].inven_icon = 6;
				break;
			case 3:
				ps[g_p].holoduke_amount =		   *insptr;// 1600;
				ps[g_p].inven_icon = 3;
				break;
			case 4:
				ps[g_p].jetpack_amount =		   *insptr;// 1600;
				ps[g_p].inven_icon = 4;
				break;
			case 6:
				if (isRR())
				{
					switch (g_sp->lotag)
					{
					case 100: ps[g_p].keys[1] = 1; break;
					case 101: ps[g_p].keys[2] = 1; break;
					case 102: ps[g_p].keys[3] = 1; break;
					case 103: ps[g_p].keys[4] = 1; break;
					}
				}
				else
				{
					switch (g_sp->pal)
					{
					case  0: ps[g_p].got_access |= 1; break;
					case 21: ps[g_p].got_access |= 2; break;
					case 23: ps[g_p].got_access |= 4; break;
					}
				}
				break;
			case 7:
				ps[g_p].heat_amount = *insptr;
				ps[g_p].inven_icon = 5;
				break;
			case 9:
				ps[g_p].inven_icon = 1;
				ps[g_p].firstaid_amount = *insptr;
				break;
			case 10:
				ps[g_p].inven_icon = 7;
				ps[g_p].boot_amount = *insptr;
				break;
		}
		insptr++;
		break;
	case concmd_hitradius:
		fi.hitradius(g_i,*(insptr+1),*(insptr+2),*(insptr+3),*(insptr+4),*(insptr+5));
		insptr+=6;
		break;
	case concmd_ifp:
	{
			insptr++;

			l = *insptr;
			j = 0;

			s = g_sp->xvel;

			// sigh... this was yet another place where number literals were used as bit masks for every single value, making the code totally unreadable.
			if( (l& pducking) && ps[g_p].on_ground && (PlayerInput(g_p, SKB_CROUCH) ^ !!(ps[g_p].crouch_toggle) ))
					j = 1;
			else if( (l& pfalling) && ps[g_p].jumping_counter == 0 && !ps[g_p].on_ground &&	ps[g_p].poszv > 2048 )
					j = 1;
			else if( (l& pjumping) && ps[g_p].jumping_counter > 348 )
					j = 1;
			else if( (l& pstanding) && s >= 0 && s < 8)
					j = 1;
			else if( (l& pwalking) && s >= 8 && !(PlayerInput(g_p, SKB_RUN)) )
					j = 1;
			else if( (l& prunning) && s >= 8 && PlayerInput(g_p, SKB_RUN) )
					j = 1;
			else if( (l& phigher) && ps[g_p].posz < (g_sp->z-(48<<8)) )
					j = 1;
			else if( (l& pwalkingback) && s <= -8 && !(PlayerInput(g_p, SKB_RUN)) )
					j = 1;
			else if( (l& prunningback) && s <= -8 && (PlayerInput(g_p, SKB_RUN)) )
					j = 1;
			else if( (l& pkicking) && ( ps[g_p].quick_kick > 0 || ( ps[g_p].curr_weapon == KNEE_WEAPON && ps[g_p].kickback_pic > 0 ) ) )
					j = 1;
			else if( (l& pshrunk) && sprite[ps[g_p].i].xrepeat < (isRR() ? 8 : 32))
					j = 1;
			else if( (l& pjetpack) && ps[g_p].jetpack_on )
					j = 1;
			else if( (l& ponsteroids) && ps[g_p].steroids_amount > 0 && ps[g_p].steroids_amount < 400 )
					j = 1;
			else if( (l& ponground) && ps[g_p].on_ground)
					j = 1;
			else if( (l& palive) && sprite[ps[g_p].i].xrepeat > (isRR() ? 8 : 32) && sprite[ps[g_p].i].extra > 0 && ps[g_p].timebeforeexit == 0 )
					j = 1;
			else if( (l& pdead) && sprite[ps[g_p].i].extra <= 0)
					j = 1;
			else if( (l& pfacing) )
			{
				if (g_sp->picnum == TILE_APLAYER && ud.multimode > 1)
					j = getincangle(ps[otherp].getang(), getangle(ps[g_p].posx - ps[otherp].posx, ps[g_p].posy - ps[otherp].posy));
				else
					j = getincangle(ps[g_p].getang(), getangle(g_sp->x - ps[g_p].posx, g_sp->y - ps[g_p].posy));

				if( j > -128 && j < 128 )
					j = 1;
				else
					j = 0;
			}

			parseifelse((int) j);

		}
		break;
	case concmd_ifstrength:
		insptr++;
		parseifelse(g_sp->extra <= *insptr);
		break;
	case concmd_guts:
		insptr += 2;
		fi.guts(g_sp,*(insptr-1),*insptr,g_p);
		insptr++;
		break;
	case concmd_slapplayer:
		insptr++;
		forceplayerangle(&ps[g_p]);
		ps[g_p].posxv -= sintable[(ps[g_p].getang() + 512) & 2047] << 7;
		ps[g_p].posyv -= sintable[ps[g_p].getang() & 2047] << 7;
		return 0;
	case concmd_wackplayer:
		insptr++;
		if (!isRR())
			forceplayerangle(&ps[g_p]);
		else
		{
			ps[g_p].posxv -= sintable[(ps[g_p].getang() + 512) & 2047] << 10;
			ps[g_p].posyv -= sintable[ps[g_p].getang() & 2047] << 10;
			ps[g_p].jumping_counter = 767;
			ps[g_p].jumping_toggle = 1;
		}
		return 0;
	case concmd_ifgapzl:
		insptr++;
		parseifelse( (( hittype[g_i].floorz - hittype[g_i].ceilingz ) >> 8 ) < *insptr);
		break;
	case concmd_ifhitspace:
		parseifelse(PlayerInput(g_p, SKB_OPEN));
		break;
	case concmd_ifoutside:
		parseifelse(sector[g_sp->sectnum].ceilingstat & 1);
		break;
	case concmd_ifmultiplayer:
		parseifelse(ud.multimode > 1);
		break;
	case concmd_operate:
		insptr++;
		if( sector[g_sp->sectnum].lotag == 0 )
		{
			int16_t neartagsector, neartagwall, neartagsprite;
			int32_t neartaghitdist;
			neartag(g_sp->x,g_sp->y,g_sp->z-(32<<8),g_sp->sectnum,g_sp->ang,&neartagsector,&neartagwall,&neartagsprite,&neartaghitdist,768L,1);
			if( neartagsector >= 0 && isanearoperator(sector[neartagsector].lotag) )
				if( (sector[neartagsector].lotag&0xff) == ST_23_SWINGING_DOOR || sector[neartagsector].floorz == sector[neartagsector].ceilingz )
					if( (sector[neartagsector].lotag&16384) == 0 )
						if( (sector[neartagsector].lotag&32768) == 0 )
					{
						j = headspritesect[neartagsector];
						while(j >= 0)
						{
							if(sprite[j].picnum == ACTIVATOR)
								break;
							j = nextspritesect[j];
						}
						if(j == -1)
							operatesectors(neartagsector,g_i);
					}
		}
		break;
	case concmd_ifinspace:
		parseifelse(fi.ceilingspace(g_sp->sectnum));
		break;

	case concmd_spritepal:
		insptr++;
		if(g_sp->picnum != TILE_APLAYER)
			hittype[g_i].tempang = g_sp->pal;
		g_sp->pal = *insptr;
		insptr++;
		break;

	case concmd_cactor:
		insptr++;
		g_sp->picnum = *insptr;
		insptr++;
		break;

	case concmd_ifbulletnear:
		parseifelse( dodge(g_sp) == 1);
		break;
	case concmd_ifrespawn:
		if( badguy(g_sp) )
			parseifelse( ud.respawn_monsters );
		else if( inventory(g_sp) )
			parseifelse( ud.respawn_inventory );
		else
			parseifelse( ud.respawn_items );
		break;
	case concmd_iffloordistl:
		insptr++;
		parseifelse( (hittype[g_i].floorz - g_sp->z) <= ((*insptr)<<8));
		break;
	case concmd_ifceilingdistl:
		insptr++;
		parseifelse( ( g_sp->z - hittype[g_i].ceilingz ) <= ((*insptr)<<8));
		break;
	case concmd_palfrom:
		insptr++;
		SetPlayerPal(&ps[g_p], PalEntry(insptr[0], insptr[1], insptr[2], insptr[3]));
		insptr += 4;
		break;

/*		  case 74:
		insptr++;
		getglobalz(g_i);
		parseifelse( (( hittype[g_i].floorz - hittype[g_i].ceilingz ) >> 8 ) >= *insptr);
		break;
*/
	case concmd_addlog:
	{	int l;
		int lFile;
		insptr++;
		lFile=*(insptr++);	// file
		l=*(insptr++);	// line
		// this was only printing file name and line number as debug output.
		break;
	}
	case concmd_addlogvar:
	{	int l;
		int lFile;
		insptr++;
		lFile=*(insptr++);	// file
		l=*(insptr++);	// l=Line number, *instpr=varID
		if( (*insptr >= iGameVarCount)
			|| *insptr < 0
			)
		{
			// invalid varID
			insptr++;
			break;	// out of switch
		}
		DPrintf(DMSG_NOTIFY, "ADDLOGVAR: ");
			
		if( aGameVars[*insptr].dwFlags & GAMEVAR_FLAG_READONLY)
		{
			DPrintf(DMSG_NOTIFY, " (read-only)");
		}
		if( aGameVars[*insptr].dwFlags & GAMEVAR_FLAG_PERPLAYER)
		{
			DPrintf(DMSG_NOTIFY, " (Per Player. Player=%d)",g_p);
		}
		else if( aGameVars[*insptr].dwFlags & GAMEVAR_FLAG_PERACTOR)
		{
			DPrintf(DMSG_NOTIFY, " (Per Actor. Actor=%d)",g_i);
		}
		else
		{
			DPrintf(DMSG_NOTIFY, " (Global)");
		}
		DPrintf(DMSG_NOTIFY, " =%d",	GetGameVarID(*insptr, g_i, g_p));
		insptr++;
		break;
	}
	case concmd_setvar:
	{	int i;
		insptr++;
		i=*(insptr++);	// ID of def
		SetGameVarID(i, *insptr, g_i, g_p );
		insptr++;
		break;
	}
	case concmd_setvarvar:
	{	int i;
		insptr++;
		i=*(insptr++);	// ID of def
		SetGameVarID(i, GetGameVarID(*insptr, g_i, g_p), g_i, g_p );
//			aGameVars[i].lValue = aGameVars[*insptr].lValue;
		insptr++;
		break;
	}
	case concmd_addvar:
	{	int i;		
		insptr++;
		i=*(insptr++);	// ID of def
//sprintf(g_szBuf,"AddVar %d to Var ID=%d, g_i=%d, g_p=%d\n",*insptr, i, g_i, g_p);
//AddLog(g_szBuf);
		SetGameVarID(i, GetGameVarID(i, g_i, g_p) + *insptr, g_i, g_p );
		insptr++;
		break;
	}
		
	case concmd_addvarvar:
	{	int i;
		insptr++;
		i=*(insptr++);	// ID of def
		SetGameVarID(i, GetGameVarID(i, g_i, g_p) + GetGameVarID(*insptr, g_i, g_p), g_i, g_p );
		insptr++;
		break;
	}
	case concmd_ifvarvare:
	{
		int i;
		insptr++;
		i=*(insptr++);	// ID of def
		j=0;
		if(GetGameVarID(i, g_i, g_p) == GetGameVarID(*(insptr), g_i, g_p) )
		{
			j=1;
		}
		parseifelse( j );
		break;
	}
	case concmd_ifvarvarg:
	{
		int i;
		insptr++;
		i=*(insptr++);	// ID of def
		j=0;
		if(GetGameVarID(i, g_i, g_p) > GetGameVarID(*(insptr), g_i, g_p) )
		{
			j=1;
		}
		parseifelse( j );
		break;
	}
	case concmd_ifvarvarl:
	{
		int i;
		insptr++;
		i=*(insptr++);	// ID of def
		j=0;
		if(GetGameVarID(i, g_i, g_p) < GetGameVarID(*(insptr), g_i, g_p) )
		{
			j=1;
		}
		parseifelse( j );
		break;
	}
	case concmd_ifvare:
	{
		int i;
		insptr++;
		i=*(insptr++);	// ID of def
		j=0;
		if(GetGameVarID(i, g_i, g_p) == *insptr)
		{
			j=1;
		}
		parseifelse( j );
		break;
	}
	case concmd_ifvarg:
	{
		int i;
		insptr++;
		i=*(insptr++);	// ID of def
		j=0;
		if(GetGameVarID(i, g_i, g_p) > *insptr)
		{
			j=1;
		}
		parseifelse( j );
		break;
	}
	case concmd_ifvarl:
	{
		int i;
		insptr++;
		i=*(insptr++);	// ID of def
		j=0;
		if(GetGameVarID(i, g_i, g_p) < *insptr)
		{
			j=1;
		}
		parseifelse( j );
		break;
	}
	case concmd_ifphealthl:
		insptr++;
		parseifelse( sprite[ps[g_p].i].extra < *insptr);
		break;

	case concmd_ifpinventory:
	{
			insptr++;
			j = 0;
			switch(*(insptr++))
			{
				case 0:if( ps[g_p].steroids_amount != *insptr)
						j = 1;
					break;
				case 1:if(ps[g_p].shield_amount != max_player_health )
						j = 1;
					break;
				case 2:if(ps[g_p].scuba_amount != *insptr) j = 1;break;
				case 3:if(ps[g_p].holoduke_amount != *insptr) j = 1;break;
				case 4:if(ps[g_p].jetpack_amount != *insptr) j = 1;break;
				case 6:
					if (isRR())
					{
						switch (g_sp->lotag)
						{
						case 100: if (ps[g_p].keys[1]) j = 1; break;
						case 101: if (ps[g_p].keys[2]) j = 1; break;
						case 102: if (ps[g_p].keys[3]) j = 1; break;
						case 103: if (ps[g_p].keys[4]) j = 1; break;
						}
					}
					else
					{
						switch (g_sp->pal)
						{
						case  0: if (ps[g_p].got_access & 1) j = 1; break;
						case 21: if (ps[g_p].got_access & 2) j = 1; break;
						case 23: if (ps[g_p].got_access & 4) j = 1; break;
						}
					}
					break;
				case 7:if(ps[g_p].heat_amount != *insptr) j = 1;break;
				case 9:
					if(ps[g_p].firstaid_amount != *insptr) j = 1;break;
				case 10:
					if(ps[g_p].boot_amount != *insptr) j = 1;break;
			}

			parseifelse(j);
			break;
		}
	case concmd_pstomp:
		insptr++;
		if( ps[g_p].knee_incs == 0 && sprite[ps[g_p].i].xrepeat >= (isRR()? 9: 40) )
			if( cansee(g_sp->x,g_sp->y,g_sp->z-(4<<8),g_sp->sectnum,ps[g_p].posx,ps[g_p].posy,ps[g_p].posz+(16<<8),sprite[ps[g_p].i].sectnum) )
		{
			ps[g_p].knee_incs = 1;
			if(ps[g_p].weapon_pos == 0)
				ps[g_p].weapon_pos = -1;
			ps[g_p].actorsqu = g_i;
		}
		break;
	case concmd_ifawayfromwall:
	{
		short s1;

		s1 = g_sp->sectnum;

		j = 0;

		updatesector(g_sp->x + 108, g_sp->y + 108, &s1);
		if (s1 == g_sp->sectnum)
		{
			updatesector(g_sp->x - 108, g_sp->y - 108, &s1);
			if (s1 == g_sp->sectnum)
			{
				updatesector(g_sp->x + 108, g_sp->y - 108, &s1);
				if (s1 == g_sp->sectnum)
				{
					updatesector(g_sp->x - 108, g_sp->y + 108, &s1);
					if (s1 == g_sp->sectnum)
						j = 1;
				}
			}
		}
		parseifelse(j);
		break;
	}

	case concmd_quote:
		insptr++;
		FTA(*insptr,&ps[g_p]);
		insptr++;
		break;
	case concmd_ifinouterspace:
		parseifelse( fi.floorspace(g_sp->sectnum));
		break;
	case concmd_ifnotmoving:
		parseifelse( (hittype[g_i].movflag&49152) > 16384 );
		break;
	case concmd_respawnhitag:
		insptr++;
		fi.respawnhitag(g_sp);
		break;
	case concmd_ifspritepal:
		insptr++;
		parseifelse( g_sp->pal == *insptr);
		break;

	case concmd_ifangdiffl:
		insptr++;
		j = abs(getincangle(ps[g_p].getang(),g_sp->ang));
		parseifelse( j <= *insptr);
		break;

	case concmd_ifnosounds:
		parseifelse(!S_CheckAnyActorSoundPlaying(g_i) );
		break;

	case concmd_ifplaybackon: //Twentieth Anniversary World Tour
		parseifelse(false);
		break;

	default:
		Printf(TEXTCOLOR_RED "Unrecognized PCode of %d  in parse.  Killing current sprite.\n",*insptr);
		Printf(TEXTCOLOR_RED "Offset=%0X\n",int(insptr-ScriptCode.Data()));
		killit_flag = 1;
		break;
	}
	return 0;
}

void execute(int i,int p,int x)
{
	if (actorinfo[sprite[i].picnum].scriptaddress == 0) return;

	int done;
	spritetype* g_sp;

	ParseState s;
	s.g_i = i;	// Sprite ID
	s.g_p = p;	// Player ID
	s.g_x = x;	// ??
	g_sp = s.g_sp = &sprite[i];	// Pointer to sprite structure
	s.g_t = &hittype[i].temp_data[0];	// Sprite's 'extra' data

	if (actorinfo[g_sp->picnum].scriptaddress == 0) return;
	s.insptr = &ScriptCode[4 + (actorinfo[g_sp->picnum].scriptaddress)];

	s.killit_flag = 0;

	if(g_sp->sectnum < 0 || g_sp->sectnum >= MAXSECTORS)
	{
		if(badguy(g_sp))
			ps[p].actors_killed++;
		deletesprite(i);
		return;
	}

	if (s.g_t[4])
	{
		// This code was utterly cryptic in the original source.
		auto ptr = &ScriptCode[s.g_t[4]];
		int numframes = ptr[1];
		int increment = ptr[3];
		int delay =  ptr[4];

		g_sp->lotag += TICSPERFRAME;
		if (g_sp->lotag > delay)
		{
			s.g_t[2]++;
			g_sp->lotag = 0;
			s.g_t[3] += increment;
		}
		if (abs(s.g_t[3]) >= abs(numframes * increment))
			s.g_t[3] = 0;
	}

	do
		done = s.parse();
	while( done == 0 );

	if(s.killit_flag == 1)
	{
		// if player was set to squish, first stop that...
		if(ps[p].actorsqu == i)
			ps[p].actorsqu = -1;
		killthesprite = true;
	}
	else
	{
		fi.move(i, p, x);

		if (g_sp->statnum == STAT_ACTOR)
		{
			if (badguy(g_sp))
			{
				if (g_sp->xrepeat > 60) goto quit;
				if (ud.respawn_monsters == 1 && g_sp->extra <= 0) goto quit;
			}
			else if (ud.respawn_items == 1 && (g_sp->cstat & 32768)) goto quit;

			if (hittype[i].timetosleep > 1)
				hittype[i].timetosleep--;
			else if (hittype[i].timetosleep == 1)
				changespritestat(i, STAT_ZOMBIEACTOR);
		}

		else if (g_sp->statnum == STAT_STANDABLE)
			fi.checktimetosleep(i);
	}
quit:
	if (killthesprite) deletesprite(i);
	killthesprite = false;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void OnEvent(int iEventID, int p, int i, int x)
{
	char done;

	if (iEventID >= MAXGAMEEVENTS)
	{
		Printf("Invalid Event ID\n");
		return;
	}
	if (apScriptGameEvent[iEventID] == 0)
	{
		return;
	}

	ParseState s;
	s.g_i = i;	// current sprite ID
	s.g_p = p;	/// current player ID
	s.g_x = x;	// ?
	s.g_sp = &sprite[i];
	s.g_t = &hittype[i].temp_data[0];

	s.insptr = &ScriptCode[apScriptGameEvent[iEventID]];

	s.killit_flag = 0;
	do
		done = s.parse();
	while (done == 0);
}


END_DUKE_NS
