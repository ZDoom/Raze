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
#include "duke3d_ed.h"
#include "gamedef.h"
#include "gamevar.h"
#include "gameexec.h"

BEGIN_DUKE_NS

#if 0
char parse(void);

void parseifelse(int condition)
{
	if( condition )
	{
		// skip 'else' pointer.. and...
		insptr+=2;
		parse();
	}
	else
	{
		insptr = (int *) *(insptr+1);
		if(*insptr == 10)
		{
			// else...

			// skip 'else' and...
			insptr+=2;
			
			parse();
		}
	}
}

// int *it = 0x00589a04;

char parse(void)
{
	int j, l, s;

	if(killit_flag) return 1;

	switch(*insptr)
	{
		case 3:
			insptr++;
			parseifelse( rnd(*insptr));
			break;
		case 45:

			if(g_x > 1024)
			{
				short temphit, sclip, angdif;

				if( badguy(g_sp) && g_sp->xrepeat > 56 )
				{
					sclip = 3084;
					angdif = 48;
				}
				else
				{
					sclip = 768;
					angdif = 16;
				}

				j = hitasprite(g_i,&temphit);
				if(j == (1<<30))
				{
					parseifelse(1);
					break;
				}
				if(j > sclip)
				{
					if(temphit >= 0 && sprite[temphit].picnum == g_sp->picnum)
						j = 0;
					else
					{
						g_sp->ang += angdif;j = hitasprite(g_i,&temphit);g_sp->ang -= angdif;
						if(j > sclip)
						{
							if(temphit >= 0 && sprite[temphit].picnum == g_sp->picnum)
								j = 0;
							else
							{
								g_sp->ang -= angdif;j = hitasprite(g_i,&temphit);g_sp->ang += angdif;
								if( j > 768 )
								{
									if(temphit >= 0 && sprite[temphit].picnum == g_sp->picnum)
										j = 0;
									else j = 1;
								}
								else j = 0;
							}
						}
						else j = 0;
					}
				}
				else j =  0;
			}
			else j = 1;

			parseifelse(j);
			break;
		case 91:
			j = cansee(g_sp->x,g_sp->y,g_sp->z-((TRAND&41)<<8),g_sp->sectnum,ps[g_p].posx,ps[g_p].posy,ps[g_p].posz/*-((TRAND&41)<<8)*/,sprite[ps[g_p].i].sectnum);
			parseifelse(j);
			if( j ) hittype[g_i].timetosleep = SLEEPTIME;
			break;

		case 49:
			parseifelse(hittype[g_i].actorstayput == -1);
			break;
		case 5:
		{
			spritetype *s;
			short sect;

			// select sprite for monster to target
			// if holoduke is on, let them target holoduke first.
			// 
			if(ps[g_p].holoduke_on >= 0)
			{
				s = &sprite[ps[g_p].holoduke_on];
				j = cansee(g_sp->x,g_sp->y,g_sp->z-(TRAND&((32<<8)-1)),g_sp->sectnum,
					   s->x,s->y,s->z,s->sectnum);
				
				if(j == 0)
				{
					// they can't see player's holoduke
					// check for player...
					s = &sprite[ps[g_p].i];
				}
			}
			else s = &sprite[ps[g_p].i];	// holoduke not on. look for player

			// can they see player, (or player's holoduke)
			j = cansee(g_sp->x,g_sp->y,g_sp->z-(TRAND&((47<<8))),g_sp->sectnum,
				s->x,s->y,s->z-(24<<8),s->sectnum);

			if(j == 0)
			{
				// they can't see it.

				// Huh?.  This does nothing....
				// (the result is always j==0....)
				if( ( abs(hittype[g_i].lastvx-g_sp->x)+abs(hittype[g_i].lastvy-g_sp->y) ) <
					( abs(hittype[g_i].lastvx-s->x)+abs(hittype[g_i].lastvy-s->y) ) )
						j = 0;

				// um yeah, this if() will always fire....
				if( j == 0 )
				{
					// search around for target player
					
					// also modifies 'target' x&y if found..
					
					j = furthestcanseepoint(g_i,s,&hittype[g_i].lastvx,&hittype[g_i].lastvy);

					if(j == -1) j = 0;
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

			if( j == 1 && ( g_sp->statnum == 1 || g_sp->statnum == 6 ) )
				hittype[g_i].timetosleep = SLEEPTIME;

			parseifelse(j == 1);
			break;
		}

		case 6:
			parseifelse(ifhitbyweapon(g_i) >= 0);
			break;
		case 27:
			parseifelse( ifsquished(g_i, g_p) == 1);
			break;
		case 26:
			{
				j = g_sp->extra;
				if(g_sp->picnum == APLAYER)
					j--;
				parseifelse(j < 0);
			}
			break;
		case 24:
			insptr++;
			g_t[5] = *insptr;
			g_t[4] = *(int *)(g_t[5]);		  // Action
			g_t[1] = *(int *)(g_t[5]+4);		// move
			g_sp->hitag = *(int *)(g_t[5]+8);	  // Ai
			g_t[0] = g_t[2] = g_t[3] = 0;
			if(g_sp->hitag&random_angle)
				g_sp->ang = TRAND&2047;
			insptr++;
			break;
		case 7:
			insptr++;
			g_t[2] = 0;
			g_t[3] = 0;
			g_t[4] = *insptr;
			insptr++;
			break;

		case 8:
			insptr++;
			parseifelse(g_x < *insptr);
			if(g_x > MAXSLEEPDIST && hittype[g_i].timetosleep == 0)
				hittype[g_i].timetosleep = SLEEPTIME;
			break;
		case 9:
			insptr++;
			parseifelse(g_x > *insptr);
			if(g_x > MAXSLEEPDIST && hittype[g_i].timetosleep == 0)
				hittype[g_i].timetosleep = SLEEPTIME;
			break;
		case 10:
			insptr = (int *) *(insptr+1);
			break;
		case 100:
			insptr++;
			g_sp->extra += *insptr;
			insptr++;
			break;
		case 11:
			insptr++;
			g_sp->extra = *insptr;
			insptr++;
			break;
		case 94:
			insptr++;

			if(ud.coop >= 1 && ud.multimode > 1)
			{
				if(*insptr == 0)
				{
					for(j=0;j < ps[g_p].weapreccnt;j++)
						if( ps[g_p].weaprecs[j] == g_sp->picnum )
							break;

					parseifelse(j < ps[g_p].weapreccnt && g_sp->owner == g_i);
				}
				else if(ps[g_p].weapreccnt < 16)
				{
					ps[g_p].weaprecs[ps[g_p].weapreccnt++] = g_sp->picnum;
					parseifelse(g_sp->owner == g_i);
				}
			}
			else parseifelse(0);
			break;
		case 95:
			insptr++;
			if(g_sp->picnum == APLAYER)
				g_sp->pal = ps[g_sp->yvel].palookup;
			else g_sp->pal = hittype[g_i].tempang;
			hittype[g_i].tempang = 0;
			break;
		case 104:
			insptr++;
			checkweapons(&ps[g_sp->yvel]);
			break;
		case 106:
			insptr++;
			break;
		case 97:
			insptr++;
			if(Sound[g_sp->yvel].num == 0)
				spritesound(g_sp->yvel,g_i);
			break;
		case 96:
			insptr++;

			if( ud.multimode > 1 && g_sp->picnum == APLAYER )
			{
				if(ps[otherp].quick_kick == 0)
					ps[otherp].quick_kick = 14;
			}
			else if(g_sp->picnum != APLAYER && ps[g_p].quick_kick == 0)
				ps[g_p].quick_kick = 14;
			break;
		case 28:
			insptr++;

	    // JBF 20030805: As I understand it, if xrepeat becomes 0 it basically kills the
	    // sprite, which is why the "sizeto 0 41" calls in 1.3d became "sizeto 4 41" in
	    // 1.4, so instead of patching the CONs I'll surruptitiously patch the code here
	    //if (!PLUTOPAK && *insptr == 0) *insptr = 4;
	    
            j = ((*insptr)-g_sp->xrepeat)<<1;
            g_sp->xrepeat += ksgn(j);

            insptr++;

            if( ( g_sp->picnum == APLAYER && g_sp->yrepeat < 36 ) || *insptr < g_sp->yrepeat || ((g_sp->yrepeat*(tilesizy[g_sp->picnum]+8))<<2) < (hittype[g_i].floorz - hittype[g_i].ceilingz) )
            {
                j = ((*insptr)-g_sp->yrepeat)<<1;
                if( abs(j) ) g_sp->yrepeat += ksgn(j);
            }

			insptr++;

			break;
		case 99:
			insptr++;
			g_sp->xrepeat = (char) *insptr;
			insptr++;
			g_sp->yrepeat = (char) *insptr;
			insptr++;
			break;
		case 13:
			insptr++;
			shoot(g_i,(short)*insptr);
			insptr++;
			break;
		case 87:
			insptr++;
			if( Sound[*insptr].num == 0 )
				spritesound((short) *insptr,g_i);
			insptr++;
			break;
		case CON_IFSOUND:
			insptr++;
			parseifelse( Sound[*insptr].num == 0 );
			break;
		case 89:
			insptr++;
			if( Sound[*insptr].num > 0 )
				stopsound((short)*insptr);
			insptr++;
			break;
		case 92:
			insptr++;
			if(g_p == screenpeek || ud.coop==1)
				spritesound((short) *insptr,ps[screenpeek].i);
			insptr++;
			break;
		case 15:
			insptr++;
			spritesound((short) *insptr,g_i);
			insptr++;
			break;
		case 84:
			insptr++;
			ps[g_p].tipincs = 26;
			break;
		case 16:
			insptr++;
			g_sp->xoffset = 0;
			g_sp->yoffset = 0;
//			  if(!gotz)
			{
				int c;

				if( floorspace(g_sp->sectnum) )
					c = 0;
				else
				{
					if( ceilingspace(g_sp->sectnum) || sector[g_sp->sectnum].lotag == 2)
						c = gc/6;
					else c = gc;
				}

				if( hittype[g_i].cgg <= 0 || (sector[g_sp->sectnum].floorstat&2) )
				{
					getglobalz(g_i);
					hittype[g_i].cgg = 6;
				}
				else hittype[g_i].cgg --;

				if( g_sp->z < (hittype[g_i].floorz-FOURSLEIGHT) )
				{
					g_sp->zvel += c;
					g_sp->z+=g_sp->zvel;

					if(g_sp->zvel > 6144) g_sp->zvel = 6144;
				}
				else
				{
					g_sp->z = hittype[g_i].floorz - FOURSLEIGHT;

					if( badguy(g_sp) || ( g_sp->picnum == APLAYER && g_sp->owner >= 0) )
					{

						if( g_sp->zvel > 3084 && g_sp->extra <= 1)
						{
							if(g_sp->pal != 1 && g_sp->picnum != DRONE)
							{
								if(g_sp->picnum == APLAYER && g_sp->extra > 0)
									goto SKIPJIBS;
								guts(g_sp,JIBS6,15,g_p);
								spritesound(SQUISHED,g_i);
								spawn(g_i,BLOODPOOL);
							}

							SKIPJIBS:

							hittype[g_i].picnum = SHOTSPARK1;
							hittype[g_i].extra = 1;
							g_sp->zvel = 0;
						}
						else if(g_sp->zvel > 2048 && sector[g_sp->sectnum].lotag != 1)
						{

							j = g_sp->sectnum;
							pushmove(&g_sp->x,&g_sp->y,&g_sp->z,&j,128L,(4L<<8),(4L<<8),CLIPMASK0);
							if(j != g_sp->sectnum && j >= 0 && j < MAXSECTORS)
								changespritesect(g_i,j);

							spritesound(THUD,g_i);
						}
					}
					if(sector[g_sp->sectnum].lotag == 1)
						switch (g_sp->picnum)
						{
							case OCTABRAIN:
							case COMMANDER:
							case DRONE:
								break;
							default:
								g_sp->z += (24<<8);
								break;
						}
					else g_sp->zvel = 0;
				}
			}

			break;
		case 4:
		case 12:
		case 18:
			return 1;
		case 30:
			insptr++;
			return 1;
		case 2:
			insptr++;
			if( ps[g_p].ammo_amount[*insptr] >= max_ammo_amount[*insptr] )
			{
				killit_flag = 2;
				break;
			}
			addammo( *insptr, &ps[g_p], *(insptr+1) );
			if(ps[g_p].curr_weapon == KNEE_WEAPON)
				if( ps[g_p].gotweapon[*insptr] )
					addweapon( &ps[g_p], *insptr );
			insptr += 2;
			break;
		case 86:
			insptr++;
			lotsofmoney(g_sp,*insptr);
			insptr++;
			break;
		case 102:
			insptr++;
			lotsofmail(g_sp,*insptr);
			insptr++;
			break;
		case 105:
			insptr++;
			hittype[g_i].timetosleep = (short)*insptr;
			insptr++;
			break;
		case 103:
			insptr++;
			lotsofpaper(g_sp,*insptr);
			insptr++;
			break;
		case 88:
			insptr++;
			ps[g_p].actors_killed += *insptr;
			hittype[g_i].actorstayput = -1;
			insptr++;
			break;
		case 93:
			insptr++;
			spriteglass(g_i,*insptr);
			insptr++;
			break;
		case 22:
			insptr++;
			killit_flag = 1;
			break;
		case 23: // addweapon
			insptr++;
			if( ps[g_p].gotweapon[*insptr] == 0 ) addweapon( &ps[g_p], *insptr );
			else if( ps[g_p].ammo_amount[*insptr] >= max_ammo_amount[*insptr] )
			{
				 killit_flag = 2;
				 break;
			}
			addammo( *insptr, &ps[g_p], *(insptr+1) );
			if(ps[g_p].curr_weapon == KNEE_WEAPON)
				if( ps[g_p].gotweapon[*insptr] )
					addweapon( &ps[g_p], *insptr );
			insptr+=2;
			break;
		case 68:
			insptr++;
			printf("%ld\n",*insptr);
			insptr++;
			break;
		case 69:
			insptr++;
			ps[g_p].timebeforeexit = *insptr;
			ps[g_p].customexitsound = -1;
			ud.eog = 1;
			insptr++;
			break;
		case 25:
			insptr++;

			if(ps[g_p].newowner >= 0)
			{
				ps[g_p].newowner = -1;
				ps[g_p].posx = ps[g_p].oposx;
				ps[g_p].posy = ps[g_p].oposy;
				ps[g_p].posz = ps[g_p].oposz;
				ps[g_p].ang = ps[g_p].oang;
				updatesector(ps[g_p].posx,ps[g_p].posy,&ps[g_p].cursectnum);
				setpal(&ps[g_p]);

				j = headspritestat[1];
				while(j >= 0)
				{
					if(sprite[j].picnum==CAMERA1)
						sprite[j].yvel = 0;
					j = nextspritestat[j];
				}
			}

			j = sprite[ps[g_p].i].extra;

			if(g_sp->picnum != ATOMICHEALTH)
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
							spritesound(DUKE_GOTHEALTHATLOW,ps[g_p].i);

					ps[g_p].last_extra = j;
				}

				sprite[ps[g_p].i].extra = j;
			}

			insptr++;
			break;
		case 17:
			{
				int *tempscrptr;

				tempscrptr = insptr+2;

				insptr = (int *) *(insptr+1);
				while(1) if(parse()) break;
				insptr = tempscrptr;
			}
			break;
		case 29:
			insptr++;
			while(1) if(parse()) break;
			break;
		case 32:
			g_t[0]=0;
			insptr++;
			g_t[1] = *insptr;
			insptr++;
			g_sp->hitag = *insptr;
			insptr++;
			if(g_sp->hitag&random_angle)
				g_sp->ang = TRAND&2047;
			break;
		case 31:
			insptr++;
			if(g_sp->sectnum >= 0 && g_sp->sectnum < MAXSECTORS)
				spawn(g_i,*insptr);
			insptr++;
			break;
		case 33:
			insptr++;
			parseifelse( hittype[g_i].picnum == *insptr);
			break;
		case 21:
			insptr++;
			parseifelse(g_t[5] == *insptr);
			break;
		case 34:
			insptr++;
			parseifelse(g_t[4] == *insptr);
			break;
		case 35:
			insptr++;
			parseifelse(g_t[2] >= *insptr);
			break;
		case 36:
			insptr++;
			g_t[2] = 0;
			break;
		case 37:
			{
				short dnum;

				insptr++;
				dnum = *insptr;
				insptr++;

				if(g_sp->sectnum >= 0 && g_sp->sectnum < MAXSECTORS)
					for(j=(*insptr)-1;j>=0;j--)
				{
					if(g_sp->picnum == BLIMP && dnum == SCRAP1)
						s = 0;
					else s = (TRAND%3);

					l = EGS(g_sp->sectnum,
							g_sp->x+(TRAND&255)-128,g_sp->y+(TRAND&255)-128,g_sp->z-(8<<8)-(TRAND&8191),
							dnum+s,g_sp->shade,32+(TRAND&15),32+(TRAND&15),
							TRAND&2047,(TRAND&127)+32,
							-(TRAND&2047),g_i,5);
					if(g_sp->picnum == BLIMP && dnum == SCRAP1)
						sprite[l].yvel = weaponsandammosprites[j%14];
					else sprite[l].yvel = -1;
					sprite[l].pal = g_sp->pal;
				}
				insptr++;
			}
			break;
		case 52:
			insptr++;
			g_t[0] = (short) *insptr;
			insptr++;
			break;
		case 101:
			insptr++;
			g_sp->cstat |= (short)*insptr;
			insptr++;
			break;
		case 110:
			insptr++;
			g_sp->clipdist = (short) *insptr;
			insptr++;
			break;
		case 40:
			insptr++;
			g_sp->cstat = (short) *insptr;
			insptr++;
			break;
		case 41:
			insptr++;
			parseifelse(g_t[1] == *insptr);
			break;
		case 42:
			insptr++;

//AddLog("resetplayer");				
			if(ud.multimode < 2)
			{
				if( lastsavedpos >= 0 && ud.recstat != 2 )
				{
					ps[g_p].gm = MODE_MENU;
					KB_ClearKeyDown(sc_Space);
					cmenu(15000);
				}
				else ps[g_p].gm = MODE_RESTART;
				killit_flag = 2;
			}
			else
			{
				pickrandomspot(g_p);
				g_sp->x = hittype[g_i].bposx = ps[g_p].bobposx = ps[g_p].oposx = ps[g_p].posx;
				g_sp->y = hittype[g_i].bposy = ps[g_p].bobposy = ps[g_p].oposy =ps[g_p].posy;
				g_sp->z = hittype[g_i].bposy = ps[g_p].oposz =ps[g_p].posz;
				updatesector(ps[g_p].posx,ps[g_p].posy,&ps[g_p].cursectnum);
				setsprite(ps[g_p].i,ps[g_p].posx,ps[g_p].posy,ps[g_p].posz+PHEIGHT);
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
				ps[g_p].horiz = 100;
				ps[g_p].on_crane = -1;
				ps[g_p].frag_ps = g_p;
				ps[g_p].horizoff = 0;
				ps[g_p].opyoff = 0;
				ps[g_p].wackedbyactor = -1;
				ps[g_p].shield_amount = max_armour_amount;
				ps[g_p].dead_flag = 0;
				ps[g_p].pals_time = 0;
				ps[g_p].footprintcount = 0;
				ps[g_p].weapreccnt = 0;
				ps[g_p].fta = 0;
				ps[g_p].ftq = 0;
				ps[g_p].posxv = ps[g_p].posyv = 0;
				ps[g_p].rotscrnang = 0;

				ps[g_p].falling_counter = 0;

				hittype[g_i].extra = -1;
				hittype[g_i].owner = g_i;

				hittype[g_i].cgg = 0;
				hittype[g_i].movflag = 0;
				hittype[g_i].tempang = 0;
				hittype[g_i].actorstayput = -1;
				hittype[g_i].dispicnum = 0;
				hittype[g_i].owner = ps[g_p].i;

				resetinventory(g_p);
				resetweapons(g_p);

				cameradist = 0;
				cameraclock = totalclock;
			}
			setpal(&ps[g_p]);
//AddLog("EOF: resetplayer");

			break;
		case 43:
			parseifelse( abs(g_sp->z-sector[g_sp->sectnum].floorz) < (32<<8) && sector[g_sp->sectnum].lotag == 1);
			break;
		case 44:
			parseifelse( sector[g_sp->sectnum].lotag == 2);
			break;
		case 46:
			insptr++;
			parseifelse(g_t[0] >= *insptr);
			break;
		case 53:
			insptr++;
			parseifelse(g_sp->picnum == *insptr);
			break;
		case 47:
			insptr++;
			g_t[0] = 0;
			break;
		case 48:
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
					switch(g_sp->pal)
					{
						case  0: ps[g_p].got_access |= 1;break;
						case 21: ps[g_p].got_access |= 2;break;
						case 23: ps[g_p].got_access |= 4;break;
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
		case 50:
			hitradius(g_i,*(insptr+1),*(insptr+2),*(insptr+3),*(insptr+4),*(insptr+5));
			insptr+=6;
			break;
		case 51:
			{
				insptr++;

				l = *insptr;
				j = 0;

				s = g_sp->xvel;

				if( (l&8) && ps[g_p].on_ground && (sync[g_p].bits&2) )
					   j = 1;
				else if( (l&16) && ps[g_p].jumping_counter == 0 && !ps[g_p].on_ground &&
					ps[g_p].poszv > 2048 )
						j = 1;
				else if( (l&32) && ps[g_p].jumping_counter > 348 )
					   j = 1;
				else if( (l&1) && s >= 0 && s < 8)
					   j = 1;
				else if( (l&2) && s >= 8 && !(sync[g_p].bits&(1<<5)) )
					   j = 1;
				else if( (l&4) && s >= 8 && sync[g_p].bits&(1<<5) )
					   j = 1;
				else if( (l&64) && ps[g_p].posz < (g_sp->z-(48<<8)) )
					   j = 1;
				else if( (l&128) && s <= -8 && !(sync[g_p].bits&(1<<5)) )
					   j = 1;
				else if( (l&256) && s <= -8 && (sync[g_p].bits&(1<<5)) )
					   j = 1;
				else if( (l&512) && ( ps[g_p].quick_kick > 0 || ( ps[g_p].curr_weapon == KNEE_WEAPON && ps[g_p].kickback_pic > 0 ) ) )
					   j = 1;
				else if( (l&1024) && sprite[ps[g_p].i].xrepeat < 32 )
					   j = 1;
				else if( (l&2048) && ps[g_p].jetpack_on )
					   j = 1;
				else if( (l&4096) && ps[g_p].steroids_amount > 0 && ps[g_p].steroids_amount < 400 )
					   j = 1;
				else if( (l&8192) && ps[g_p].on_ground)
					   j = 1;
				else if( (l&16384) && sprite[ps[g_p].i].xrepeat > 32 && sprite[ps[g_p].i].extra > 0 && ps[g_p].timebeforeexit == 0 )
					   j = 1;
				else if( (l&32768) && sprite[ps[g_p].i].extra <= 0)
					   j = 1;
				else if( (l&65536L) )
				{
					if(g_sp->picnum == APLAYER && ud.multimode > 1)
						j = getincangle(ps[otherp].ang,getangle(ps[g_p].posx-ps[otherp].posx,ps[g_p].posy-ps[otherp].posy));
					else
						j = getincangle(ps[g_p].ang,getangle(g_sp->x-ps[g_p].posx,g_sp->y-ps[g_p].posy));

					if( j > -128 && j < 128 )
						j = 1;
					else
						j = 0;
				}

				parseifelse((int) j);

			}
			break;
		case 56:
			insptr++;
			parseifelse(g_sp->extra <= *insptr);
			break;
		case 58:
			insptr += 2;
			guts(g_sp,*(insptr-1),*insptr,g_p);
			insptr++;
			break;
		case 59:
			insptr++;
//			  if(g_sp->owner >= 0 && sprite[g_sp->owner].picnum == *insptr)
  //			  parseifelse(1);
//			  else
			parseifelse( hittype[g_i].picnum == *insptr);
			break;
		case 61:
			insptr++;
			forceplayerangle(&ps[g_p]);
			return 0;
		case 62:
			insptr++;
			parseifelse( (( hittype[g_i].floorz - hittype[g_i].ceilingz ) >> 8 ) < *insptr);
			break;
		case 63:
			parseifelse( sync[g_p].bits&(1<<29));
			break;
		case 64:
			parseifelse(sector[g_sp->sectnum].ceilingstat&1);
			break;
		case 65:
			parseifelse(ud.multimode > 1);
			break;
		case 66:
			insptr++;
			if( sector[g_sp->sectnum].lotag == 0 )
			{
				neartag(g_sp->x,g_sp->y,g_sp->z-(32<<8),g_sp->sectnum,g_sp->ang,&neartagsector,&neartagwall,&neartagsprite,&neartaghitdist,768L,1);
				if( neartagsector >= 0 && isanearoperator(sector[neartagsector].lotag) )
					if( (sector[neartagsector].lotag&0xff) == 23 || sector[neartagsector].floorz == sector[neartagsector].ceilingz )
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
		case 67:
			parseifelse(ceilingspace(g_sp->sectnum));
			break;

		case 74:
			insptr++;
			if(g_sp->picnum != APLAYER)
				hittype[g_i].tempang = g_sp->pal;
			g_sp->pal = *insptr;
			insptr++;
			break;

		case 77:
			insptr++;
			g_sp->picnum = *insptr;
			insptr++;
			break;

		case 70:
			parseifelse( dodge(g_sp) == 1);
			break;
		case 71:
			if( badguy(g_sp) )
				parseifelse( ud.respawn_monsters );
			else if( inventory(g_sp) )
				parseifelse( ud.respawn_inventory );
			else
				parseifelse( ud.respawn_items );
			break;
		case 72:
			insptr++;
//			  getglobalz(g_i);
			parseifelse( (hittype[g_i].floorz - g_sp->z) <= ((*insptr)<<8));
			break;
		case 73:
			insptr++;
//			  getglobalz(g_i);
			parseifelse( ( g_sp->z - hittype[g_i].ceilingz ) <= ((*insptr)<<8));
			break;
		case 14:

			insptr++;
			ps[g_p].pals_time = *insptr;
			insptr++;
			for(j=0;j<3;j++)
			{
				ps[g_p].pals[j] = *insptr;
				insptr++;
			}
			break;

/*		  case 74:
			insptr++;
			getglobalz(g_i);
			parseifelse( (( hittype[g_i].floorz - hittype[g_i].ceilingz ) >> 8 ) >= *insptr);
			break;
*/
#ifdef WW2
		case CON_ADDLOG:
		{	int l;
			int lFile;
			insptr++;
			lFile=*(insptr++);	// file
			l=*(insptr++);	// line
			sprintf(g_szBuf,"ADDLOG: %s L=%ld",g_achSourceFiles[lFile],l);
			AddLog(g_szBuf);
			break;
		}
		case CON_ADDLOGVAR:
		{	int l;
			char szBuf[256];
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
	sprintf(g_szBuf,"ADDLOGVAR: %s L=%ld INVALID VARIABLE",g_achSourceFiles[lFile],l);
	AddLog(g_szBuf);
	sprintf(g_szBuf,"Offset=%0lX\n",scriptptr-script);
	AddLog(g_szBuf);
				break;	// out of switch
			}
			sprintf(szBuf,"ADDLOGVAR: %s L=%ld %s ",g_achSourceFiles[lFile],l, aGameVars[*insptr].szLabel);
			strcpy(g_szBuf,szBuf);
			
			if( aGameVars[*insptr].dwFlags & GAMEVAR_FLAG_READONLY)
			{
				sprintf(szBuf," (read-only)");
				strcat(g_szBuf,szBuf);
			}
			if( aGameVars[*insptr].dwFlags & GAMEVAR_FLAG_PERPLAYER)
			{
				sprintf(szBuf," (Per Player. Player=%d)",g_p);
			}
			else if( aGameVars[*insptr].dwFlags & GAMEVAR_FLAG_PERACTOR)
			{
				sprintf(szBuf," (Per Actor. Actor=%d)",g_i);
			}
			else
			{
				sprintf(szBuf," (Global)");
			}
			strcat(g_szBuf,szBuf);
			sprintf(szBuf," =%ld",	GetGameVarID(*insptr, g_i, g_p));
			strcat(g_szBuf,szBuf);
			AddLog(g_szBuf);
			insptr++;
			break;
		}
		case CON_SETVAR:
		{	int i;
			insptr++;
			i=*(insptr++);	// ID of def
			SetGameVarID(i, *insptr, g_i, g_p );
			insptr++;
			break;
		}
		case CON_SETVARVAR:
		{	int i;
			insptr++;
			i=*(insptr++);	// ID of def
			SetGameVarID(i, GetGameVarID(*insptr, g_i, g_p), g_i, g_p );
//			aGameVars[i].lValue = aGameVars[*insptr].lValue;
			insptr++;
			break;
		}
		case CON_ADDVAR:
		{	int i;		
			insptr++;
			i=*(insptr++);	// ID of def
//sprintf(g_szBuf,"AddVar %d to Var ID=%d, g_i=%d, g_p=%d\n",*insptr, i, g_i, g_p);
//AddLog(g_szBuf);
			SetGameVarID(i, GetGameVarID(i, g_i, g_p) + *insptr, g_i, g_p );
			insptr++;
			break;
		}
		case CON_SIN:
		{	int i;
			int lValue;
			insptr++;
			i=*(insptr++);	// ID of def
			lValue=GetGameVarID(*insptr, g_i, g_p);
			lValue=sintable[lValue&2047];
			SetGameVarID(i, lValue , g_i, g_p );
			insptr++;
			break;
		}
		
		case CON_ADDVARVAR:
		{	int i;
			insptr++;
			i=*(insptr++);	// ID of def
			SetGameVarID(i, GetGameVarID(i, g_i, g_p) + GetGameVarID(*insptr, g_i, g_p), g_i, g_p );
			insptr++;
			break;
		}
		case CON_SPGETLOTAG:
		{	
			insptr++;
			SetGameVarID(g_iLoTagID, g_sp->lotag, g_i, g_p);
			break;
		}
		case CON_SPGETHITAG:
		{	
			insptr++;
			SetGameVarID(g_iHiTagID, g_sp->hitag, g_i, g_p);
			break;
		}
		case CON_SECTGETLOTAG:
		{	
			insptr++;
			SetGameVarID(g_iLoTagID, sector[g_sp->sectnum].lotag, g_i, g_p);
			break;
		}
		case CON_SECTGETHITAG:
		{	
			insptr++;
			SetGameVarID(g_iHiTagID, sector[g_sp->sectnum].hitag, g_i, g_p);
			break;
		}
		case CON_GETTEXTUREFLOOR:
		{	
			insptr++;
			SetGameVarID(g_iTextureID, sector[g_sp->sectnum].floorpicnum, g_i, g_p);
			break;
		}

		case CON_IFVARVARAND:
		{
			int i;
			insptr++;
			i=*(insptr++);	// ID of def
			j=0;
			if(GetGameVarID(i, g_i, g_p) & GetGameVarID(*(insptr), g_i, g_p) )
			{
				j=1;
			}
			parseifelse( j );
			break;
		}
		case CON_IFVARVARN:
		{
			int i;
			insptr++;
			i=*(insptr++);	// ID of def
			j=0;
			if(GetGameVarID(i, g_i, g_p) != GetGameVarID(*(insptr), g_i, g_p) )
			{
				j=1;
			}
			parseifelse( j );
			break;
		}
		case CON_IFVARVARE:
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
		case CON_IFVARVARG:
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
		case CON_IFVARVARL:
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
		case CON_IFVARE:
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
		case CON_IFVARN:
		{
			int i;
			insptr++;
			i=*(insptr++);	// ID of def
			j=0;
			if(GetGameVarID(i, g_i, g_p) != *insptr)
			{
				j=1;
			}
			parseifelse( j );
			break;
		}
		case CON_IFVARAND:
		{
			int i;
			insptr++;
			i=*(insptr++);	// ID of def
			j=0;
			if(GetGameVarID(i, g_i, g_p) & *insptr)
			{
				j=1;
			}
			parseifelse( j );
			break;
		}
		case CON_IFVARG:
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
		case CON_IFVARL:
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
#endif
		case 78:
			insptr++;
			parseifelse( sprite[ps[g_p].i].extra < *insptr);
			break;

		case 75:
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
						switch(g_sp->pal)
						{
							case  0: if(ps[g_p].got_access&1) j = 1;break;
							case 21: if(ps[g_p].got_access&2) j = 1;break;
							case 23: if(ps[g_p].got_access&4) j = 1;break;
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
		case 38:
			insptr++;
			if( ps[g_p].knee_incs == 0 && sprite[ps[g_p].i].xrepeat >= 40 )
				if( cansee(g_sp->x,g_sp->y,g_sp->z-(4<<8),g_sp->sectnum,ps[g_p].posx,ps[g_p].posy,ps[g_p].posz+(16<<8),sprite[ps[g_p].i].sectnum) )
			{
				ps[g_p].knee_incs = 1;
				if(ps[g_p].weapon_pos == 0)
					ps[g_p].weapon_pos = -1;
				ps[g_p].actorsqu = g_i;
			}
			break;
		case 90:
			{
				short s1;

				s1 = g_sp->sectnum;

				j = 0;

					updatesector(g_sp->x+108,g_sp->y+108,&s1);
					if( s1 == g_sp->sectnum )
					{
						updatesector(g_sp->x-108,g_sp->y-108,&s1);
						if( s1 == g_sp->sectnum )
						{
							updatesector(g_sp->x+108,g_sp->y-108,&s1);
							if( s1 == g_sp->sectnum )
							{
								updatesector(g_sp->x-108,g_sp->y+108,&s1);
								if( s1 == g_sp->sectnum )
									j = 1;
							}
						}
					}
					parseifelse( j );
			}

			break;
		case 80:
			insptr++;
			FTA(*insptr,&ps[g_p]);
			insptr++;
			break;
		case 81:
			parseifelse( floorspace(g_sp->sectnum));
			break;
		case 82:
			parseifelse( (hittype[g_i].movflag&49152) > 16384 );
			break;
		case 83:
			insptr++;
			switch(g_sp->picnum)
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
					if(g_sp->yvel) operaterespawns(g_sp->yvel);
					break;
				default:
					if(g_sp->hitag >= 0) operaterespawns(g_sp->hitag);
					break;
			}
			break;
		case 85:
			insptr++;
			parseifelse( g_sp->pal == *insptr);
			break;

		case 111:
			insptr++;
			j = abs(getincangle(ps[g_p].ang,g_sp->ang));
			parseifelse( j <= *insptr);
			break;

		case 109:

			for(j=1;j<NUM_SOUNDS;j++)
				if( SoundOwner[j][0].i == g_i )
					break;

			parseifelse( j == NUM_SOUNDS );
			break;
		default:
#ifdef WW2
sprintf(g_szBuf,"Unrecognized PCode of %ld  in parse.  Killing current sprite.",*insptr);
AddLog(g_szBuf);
sprintf(g_szBuf,"Offset=%0lX",scriptptr-script);
AddLog(g_szBuf);
#endif
			killit_flag = 1;
			break;
	}
	return 0;
}

void LoadActor(short i,short p,int x)
{
	char done;

	g_i = i;	// Sprite ID
	g_p = p;	// Player ID
	g_x = x;	// ??
	g_sp = &sprite[g_i];	// Pointer to sprite structure
	g_t = &hittype[g_i].temp_data[0];	// Sprite's 'extra' data

	if( actorLoadEventScrptr[g_sp->picnum] == 0 ) return;

	insptr = 4 + (actorLoadEventScrptr[g_sp->picnum]);

	killit_flag = 0;

	if(g_sp->sectnum < 0 || g_sp->sectnum >= MAXSECTORS)
	{
//		if(badguy(g_sp))
//			ps[g_p].actors_killed++;
		deletesprite(g_i);
		return;
	}
	do
		done = parse();
	while( done == 0 );

	if(killit_flag == 1)
	{
		// if player was set to squish, first stop that...
		if (g_p >= 0 )
		{
			if(ps[g_p].actorsqu == g_i)
				ps[g_p].actorsqu = -1;
		}
		deletesprite(g_i);
	}
	else
	{
		move();

		if( g_sp->statnum == 1)
		{
			if( badguy(g_sp) )
			{
				if( g_sp->xrepeat > 60 ) return;
				if( ud.respawn_monsters == 1 && g_sp->extra <= 0 ) return;
			}
			else if( ud.respawn_items == 1 && (g_sp->cstat&32768) ) return;

			if(hittype[g_i].timetosleep > 1)
				hittype[g_i].timetosleep--;
			else if(hittype[g_i].timetosleep == 1)
				 changespritestat(g_i,2);
		}

		else if(g_sp->statnum == 6)
		{
			switch(g_sp->picnum)
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
					if(hittype[g_i].timetosleep > 1)
						hittype[g_i].timetosleep--;
					else if(hittype[g_i].timetosleep == 1)
						changespritestat(g_i,2);
					break;
			}
		}
	}

}

void execute(short i,short p,int x)
{
	char done;

	g_i = i;	// Sprite ID
	g_p = p;	// Player ID
	g_x = x;	// ??
	g_sp = &sprite[g_i];	// Pointer to sprite structure
	g_t = &hittype[g_i].temp_data[0];	// Sprite's 'extra' data

	if( actorscrptr[g_sp->picnum] == 0 ) return;

	insptr = 4 + (actorscrptr[g_sp->picnum]);

	killit_flag = 0;

	if(g_sp->sectnum < 0 || g_sp->sectnum >= MAXSECTORS)
	{
		if(badguy(g_sp))
			ps[g_p].actors_killed++;
		deletesprite(g_i);
		return;
	}

	if(g_t[4])
	{
		g_sp->lotag += TICSPERFRAME;
		if(g_sp->lotag > *(int *)(g_t[4]+16) )
		{
			g_t[2]++;
			g_sp->lotag = 0;
			g_t[3] +=  *(int *)( g_t[4]+12 );
		}
		if( abs(g_t[3]) >= abs( *(int *)(g_t[4]+4) * *(int *)(g_t[4]+12) ) )
			g_t[3] = 0;
	}

	do
		done = parse();
	while( done == 0 );

	if(killit_flag == 1)
	{
		// if player was set to squish, first stop that...
		if(ps[g_p].actorsqu == g_i)
			ps[g_p].actorsqu = -1;
		deletesprite(g_i);
	}
	else
	{
		move();

		if( g_sp->statnum == 1)
		{
			if( badguy(g_sp) )
			{
				if( g_sp->xrepeat > 60 ) return;
				if( ud.respawn_monsters == 1 && g_sp->extra <= 0 ) return;
			}
			else if( ud.respawn_items == 1 && (g_sp->cstat&32768) ) return;

			if(hittype[g_i].timetosleep > 1)
				hittype[g_i].timetosleep--;
			else if(hittype[g_i].timetosleep == 1)
				 changespritestat(g_i,2);
		}

		else if(g_sp->statnum == 6)
			switch(g_sp->picnum)
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
					if(hittype[g_i].timetosleep > 1)
						hittype[g_i].timetosleep--;
					else if(hittype[g_i].timetosleep == 1)
						changespritestat(g_i,2);
					break;
			}
	}
}

#endif

END_DUKE_NS
