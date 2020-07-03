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
#include "game.h"
#include "names_d.h"

BEGIN_DUKE_NS 

void animatesprites_d(int x,int y,int a,int smoothratio)
{
    int i, j, k, p;
	short sect;
    int l, t1,t3,t4;
    spritetype *s;
	tspritetype *t;

    for(j=0;j < spritesortcnt; j++)
    {
        t = &tsprite[j];
        i = t->owner;
        s = &sprite[t->owner];

        switch(t->picnum)
        {
			case DEVELOPERCOMMENTARY:
			case DEVELOPERCOMMENTARY + 1:
				if(!isWorldTour() /* || !cfg.bDevCommentry)*/)
					 t->xrepeat = t->yrepeat = 0;
				break;
            case BLOODPOOL:
            case PUKE:
            case FOOTPRINTS:
            case FOOTPRINTS2:
            case FOOTPRINTS3:
            case FOOTPRINTS4:
                if(t->shade == 127) continue;
                break;
            case RESPAWNMARKERRED:
            case RESPAWNMARKERYELLOW:
            case RESPAWNMARKERGREEN:
                if(ud.marker == 0)
                    t->xrepeat = t->yrepeat = 0;
                continue;
            case CHAIR3:
				/*
				if (bpp > 8 && usemodels && md_tilehasmodel(t->picnum) >= 0) {
					t->cstat &= ~4;
					break;
				}
				*/

                k = (((t->ang+3072+128-a)&2047)>>8)&7;
                if(k>4)
                {
                    k = 8-k;
                    t->cstat |= 4;
                }
                else t->cstat &= ~4;
                t->picnum = s->picnum+k;
                break;
            case BLOODSPLAT1:
            case BLOODSPLAT2:
            case BLOODSPLAT3:
            case BLOODSPLAT4:
                if(ud.lockout) t->xrepeat = t->yrepeat = 0;
                else if(t->pal == 6)
                {
                    t->shade = -127;
                    continue;
                }
            case BULLETHOLE:
            case CRACK1:
            case CRACK2:
            case CRACK3:
            case CRACK4:
                t->shade = 16;
                continue;
            case NEON1:
            case NEON2:
            case NEON3:
            case NEON4:
            case NEON5:
            case NEON6:
                continue;
            case GREENSLIME:
            case GREENSLIME+1:
            case GREENSLIME+2:
            case GREENSLIME+3:
            case GREENSLIME+4:
            case GREENSLIME+5:
            case GREENSLIME+6:
            case GREENSLIME+7:
                break;
            default:
                if( ( (t->cstat&16) ) || ( badguy(t) && t->extra > 0) || t->statnum == 10)
                    continue;
        }

        if (sector[t->sectnum].ceilingstat&1)
            l = sector[t->sectnum].ceilingshade;
        else
            l = sector[t->sectnum].floorshade;

        if(l < -127) l = -127;
        if(l > 128) l =  127;
        t->shade = l;
    }


    for(j=0;j < spritesortcnt; j++ )  //Between drawrooms() and drawmasks()
    {                             //is the perfect time to animate sprites
        t = &tsprite[j];
        i = t->owner;
        s = &sprite[i];

        switch(s->picnum)
        {
            case SECTOREFFECTOR:
                if(t->lotag == 27 && ud.recstat == 1)
                {
                    t->picnum = 11+(((int)totalclock>>3)&1);
                    t->cstat |= 128;
                }
                else
                    t->xrepeat = t->yrepeat = 0;
                break;
            case NATURALLIGHTNING:
               t->shade = -127;
               break;

			default:
                break;
        }

        if( t->statnum == 99 ) continue;
        if( s->statnum != 1 && s->picnum == APLAYER && ps[s->yvel].newowner == -1 && s->owner >= 0 )
        {
            t->x -= mulscale16(65536-smoothratio,ps[s->yvel].posx-ps[s->yvel].oposx);
            t->y -= mulscale16(65536-smoothratio,ps[s->yvel].posy-ps[s->yvel].oposy);
            t->z = ps[s->yvel].oposz + mulscale16(smoothratio,ps[s->yvel].posz-ps[s->yvel].oposz);
            t->z += (40<<8);
        }
        else if( ( s->statnum == 0 && s->picnum != CRANEPOLE) || s->statnum == 10 || s->statnum == 6 || s->statnum == 4 || s->statnum == 5 || s->statnum == 1 )
        {
            t->x -= mulscale16(65536-smoothratio,s->x-hittype[i].bposx);
            t->y -= mulscale16(65536-smoothratio,s->y-hittype[i].bposy);
            t->z -= mulscale16(65536-smoothratio,s->z-hittype[i].bposz);
        }

        sect = s->sectnum;
        t1 = hittype[i].temp_data[1];
        t3 = hittype[i].temp_data[3];
        t4 = hittype[i].temp_data[4];

        switch(s->picnum)
        {
            case DUKELYINGDEAD:
                t->z += (24<<8);
                break;
            case BLOODPOOL:
            case FOOTPRINTS:
            case FOOTPRINTS2:
            case FOOTPRINTS3:
            case FOOTPRINTS4:
                if(t->pal == 6)
                    t->shade = -127;
            case PUKE:
            case MONEY:
            case MONEY+1:
            case MAIL:
            case MAIL+1:
            case PAPER:
            case PAPER+1:
                break;
            case TRIPBOMB:
                continue;
            case FORCESPHERE:
                if(t->statnum == 5)
                {
                    short sqa,sqb;

                    sqa =
                        getangle(
                            sprite[s->owner].x-ps[screenpeek].posx,
                            sprite[s->owner].y-ps[screenpeek].posy);
                    sqb =
                        getangle(
                            sprite[s->owner].x-t->x,
                            sprite[s->owner].y-t->y);

                    if( abs(getincangle(sqa,sqb)) > 512 )
                        if( ldist(&sprite[s->owner],t) < ldist(&sprite[ps[screenpeek].i],&sprite[s->owner]) )
                            t->xrepeat = t->yrepeat = 0;
                }
                continue;
            case BURNING:
            case BURNING2:
                if( sprite[s->owner].statnum == 10 )
                {
                    if( display_mirror == 0 && sprite[s->owner].yvel == screenpeek && ps[sprite[s->owner].yvel].over_shoulder_on == 0 )
                        t->xrepeat = 0;
                    else
                    {
                        t->ang = getangle(x-t->x,y-t->y);
                        t->x = sprite[s->owner].x;
                        t->y = sprite[s->owner].y;
                        t->x += sintable[(t->ang+512)&2047]>>10;
                        t->y += sintable[t->ang&2047]>>10;
                    }
                }
                break;

            case ATOMICHEALTH:
                t->z -= (4<<8);
                break;
            case CRYSTALAMMO:
                t->shade = (sintable[((int)totalclock<<4)&2047]>>10);
                continue;
            case VIEWSCREEN:
            case VIEWSCREEN2:
                if(camsprite >= 0 && hittype[sprite[i].owner].temp_data[0] == 1)
                {
                    t->picnum = STATIC;
                    t->cstat |= (rand()&12);
                    t->xrepeat += 8;
                    t->yrepeat += 8;
                } 
                else if (camsprite >= 0) 
                {
                    t->picnum = TILE_VIEWSCR;
				}
                break;

            case SHRINKSPARK:
                t->picnum = SHRINKSPARK+( ((int)totalclock>>4)&3 );
                break;
            case GROWSPARK:
                t->picnum = GROWSPARK+( ((int)totalclock>>4)&3 );
                break;
            case RPG:
				/*if (bpp > 8 && usemodels && md_tilehasmodel(s->picnum) >= 0) {
					t->cstat &= ~4;
					break;
				}*/
				
                 k = getangle(s->x-x,s->y-y);
                 k = (((s->ang+3072+128-k)&2047)/170);
                 if(k > 6)
                 {
                    k = 12-k;
                    t->cstat |= 4;
                 }
                 else t->cstat &= ~4;
                 t->picnum = RPG+k;
                 break;

            case RECON:
				/*if (bpp > 8 && usemodels && md_tilehasmodel(s->picnum) >= 0) {
					t->cstat &= ~4;
					break;
				}*/

                k = getangle(s->x-x,s->y-y);
	                if( hittype[i].temp_data[0] < 4 )
                    k = (((s->ang+3072+128-k)&2047)/170);
                else k = (((s->ang+3072+128-k)&2047)/170);

                if(k>6)
                {
                    k = 12-k;
                    t->cstat |= 4;
                }
                else t->cstat &= ~4;

                if( abs(t3) > 64 ) k += 7;
                t->picnum = RECON+k;

                break;

            case APLAYER:

                p = s->yvel;

                if(t->pal == 1) t->z -= (18<<8);

                if(ps[p].over_shoulder_on > 0 && ps[p].newowner < 0 )
                {
                    t->cstat |= 2;
                    if ( screenpeek == myconnectindex && numplayers >= 2 )
                    {
                        t->x = omyx + mulscale16((int)(myx - omyx), smoothratio);
                        t->y = omyy + mulscale16((int)(myy - omyy), smoothratio);
                        t->z = omyz + mulscale16((int)(myz - omyz), smoothratio) + (40 << 8);
                        t->ang = omyang + mulscale16((int)(((myang + 1024 - omyang) & 2047) - 1024), smoothratio);
                        t->sectnum = mycursectnum;
                    }
                }

                if( ( display_mirror == 1 || screenpeek != p || s->owner == -1 ) && ud.multimode > 1 && ud.showweapons && sprite[ps[p].i].extra > 0 && ps[p].curr_weapon > 0 )
                {
                    memcpy((spritetype *)&tsprite[spritesortcnt],(spritetype *)t,sizeof(spritetype));

                    tsprite[spritesortcnt].statnum = 99;

                    tsprite[spritesortcnt].yrepeat = ( t->yrepeat>>3 );
                    if(t->yrepeat < 4) t->yrepeat = 4;

                    tsprite[spritesortcnt].shade = t->shade;
                    tsprite[spritesortcnt].cstat = 0;

                    switch(ps[p].curr_weapon)
                    {
                        case PISTOL_WEAPON:      tsprite[spritesortcnt].picnum = FIRSTGUNSPRITE;       break;
                        case SHOTGUN_WEAPON:     tsprite[spritesortcnt].picnum = SHOTGUNSPRITE;        break;
                        case CHAINGUN_WEAPON:    tsprite[spritesortcnt].picnum = CHAINGUNSPRITE;       break;
                        case RPG_WEAPON:         tsprite[spritesortcnt].picnum = RPGSPRITE;            break;
                        case HANDREMOTE_WEAPON:
                        case HANDBOMB_WEAPON:    tsprite[spritesortcnt].picnum = HEAVYHBOMB;           break;
                        case TRIPBOMB_WEAPON:    tsprite[spritesortcnt].picnum = TRIPBOMBSPRITE;       break;
                        case GROW_WEAPON:        tsprite[spritesortcnt].picnum = GROWSPRITEICON;       break;
                        case SHRINKER_WEAPON:    tsprite[spritesortcnt].picnum = SHRINKERSPRITE;       break;
                        case FREEZE_WEAPON:      tsprite[spritesortcnt].picnum = FREEZESPRITE;         break;
                        case FLAMETHROWER_WEAPON: //Twentieth Anniversary World Tour
                        	if(isWorldTour())
                        		tsprite[spritesortcnt].picnum = FLAMETHROWERSPRITE;   break;
                        case DEVISTATOR_WEAPON:  tsprite[spritesortcnt].picnum = DEVISTATORSPRITE;     break;
                    }

                    if(s->owner >= 0)
                        tsprite[spritesortcnt].z = ps[p].posz-(12<<8);
                    else tsprite[spritesortcnt].z = s->z-(51<<8);
                    if(ps[p].curr_weapon == HANDBOMB_WEAPON)
                    {
                        tsprite[spritesortcnt].xrepeat = 10;
                        tsprite[spritesortcnt].yrepeat = 10;
                    }
                    else
                    {
                        tsprite[spritesortcnt].xrepeat = 16;
                        tsprite[spritesortcnt].yrepeat = 16;
                    }
                    tsprite[spritesortcnt].pal = 0;
                    spritesortcnt++;
                }

                if(s->owner == -1)
                {
					/*if (bpp > 8 && usemodels && md_tilehasmodel(s->picnum) >= 0) {
						k = 0;
						t->cstat &= ~4;
					} else*/ {
	                    k = (((s->ang+3072+128-a)&2047)>>8)&7;
    	                if(k>4)
        	            {
            	            k = 8-k;
                	        t->cstat |= 4;
                    	}
	                    else t->cstat &= ~4;
					}

                    if(sector[t->sectnum].lotag == 2) k += 1795-1405;
                    else if( (hittype[i].floorz-s->z) > (64<<8) ) k += 60;

                    t->picnum += k;
                    t->pal = ps[p].palookup;

                    goto PALONLY;
                }

                if( ps[p].on_crane == -1 && (sector[s->sectnum].lotag&0x7ff) != 1 )
                {
                    l = s->z-hittype[ps[p].i].floorz+(3<<8);
                    if( l > 1024 && s->yrepeat > 32 && s->extra > 0 )
                        s->yoffset = (signed char)(l/(s->yrepeat<<2));
                    else s->yoffset=0;
                }

                if(ps[p].newowner > -1)
                {
                    t4 = ScriptCode[actorinfo[APLAYER].scriptaddress + 1];
                    t3 = 0;
                    t1 = ScriptCode[actorinfo[APLAYER].scriptaddress + 2];
                }

                if(ud.camerasprite == -1 && ps[p].newowner == -1)
                    if(s->owner >= 0 && display_mirror == 0 && ps[p].over_shoulder_on == 0 )
                        if( ud.multimode < 2 || ( ud.multimode > 1 && p == screenpeek ) )
                {
                    t->owner = -1;
                    t->xrepeat = t->yrepeat = 0;
                    continue;
                }

                PALONLY:

                if( sector[sect].floorpal )
                    t->pal = sector[sect].floorpal;

                if(s->owner == -1) continue;

                if( t->z > hittype[i].floorz && t->xrepeat < 32 )
                    t->z = hittype[i].floorz;

                break;

            case JIBS1:
            case JIBS2:
            case JIBS3:
            case JIBS4:
            case JIBS5:
            case JIBS6:
            case HEADJIB1:
            case LEGJIB1:
            case ARMJIB1:
            case LIZMANHEAD1:
            case LIZMANARM1:
            case LIZMANLEG1:
            case DUKELEG:
            case DUKEGUN:
            case DUKETORSO:
                if(ud.lockout)
                {
                    t->xrepeat = t->yrepeat = 0;
                    continue;
                }
                if(t->pal == 6) t->shade = -120;

            case SCRAP1:
            case SCRAP2:
            case SCRAP3:
            case SCRAP4:
            case SCRAP5:
            case SCRAP6:
            case SCRAP6+1:
            case SCRAP6+2:
            case SCRAP6+3:
            case SCRAP6+4:
            case SCRAP6+5:
            case SCRAP6+6:
            case SCRAP6+7:

                if(hittype[i].picnum == BLIMP && t->picnum == SCRAP1 && s->yvel >= 0)
                    t->picnum = s->yvel;
                else t->picnum += hittype[i].temp_data[0];
                t->shade -= 6;

                if( sector[sect].floorpal )
                    t->pal = sector[sect].floorpal;
                break;

            case WATERBUBBLE:
                if(sector[t->sectnum].floorpicnum == FLOORSLIME)
                {
                    t->pal = 7;
                    break;
                }
            default:

                if( sector[sect].floorpal )
                    t->pal = sector[sect].floorpal;
                break;
        }

        if (ScriptCode[actorinfo[s->picnum].scriptaddress])
        {
            if(t4)
            {
                l = ScriptCode[t4 + 2];

				/*if (bpp > 8 && usemodels && md_tilehasmodel(s->picnum) >= 0) {
					k = 0;
					t->cstat &= ~4;
				} else*/ switch( l ) {
                    case 2:
                        k = (((s->ang+3072+128-a)&2047)>>8)&1;
                        break;

                    case 3:
                    case 4:
                        k = (((s->ang+3072+128-a)&2047)>>7)&7;
                        if(k > 3)
                        {
                            t->cstat |= 4;
                            k = 7-k;
                        }
                        else t->cstat &= ~4;
                        break;

                    case 5:
                        k = getangle(s->x-x,s->y-y);
                        k = (((s->ang+3072+128-k)&2047)>>8)&7;
                        if(k>4)
                        {
                            k = 8-k;
                            t->cstat |= 4;
                        }
                        else t->cstat &= ~4;
                        break;
                    case 7:
                        k = getangle(s->x-x,s->y-y);
                        k = (((s->ang+3072+128-k)&2047)/170);
                        if(k>6)
                        {
                            k = 12-k;
                            t->cstat |= 4;
                        }
                        else t->cstat &= ~4;
                        break;
                    case 8:
                        k = (((s->ang+3072+128-a)&2047)>>8)&7;
                        t->cstat &= ~4;
                        break;
                    default:
                        k = 0;
                        break;
                }

                t->picnum += k + ScriptCode[t4] + l * t3;

                if(l > 0) while(tilesiz[t->picnum].x == 0 && t->picnum > 0 )
                    t->picnum -= l;       //Hack, for actors

                if( hittype[i].dispicnum >= 0)
                    hittype[i].dispicnum = t->picnum;
            }
            else if(display_mirror == 1)
                t->cstat |= 4;
        }

        if( s->statnum == 13 || badguy(s) || (s->picnum == APLAYER && s->owner >= 0) )
            if(t->statnum != 99 && s->picnum != EXPLOSION2 && s->picnum != HANGLIGHT && s->picnum != DOMELITE)
                if(s->picnum != HOTMEAT)
        {
            if( hittype[i].dispicnum < 0 )
            {
                hittype[i].dispicnum++;
                continue;
            }
            else if (r_shadows && spritesortcnt < (MAXSPRITESONSCREEN - 2))
            {
                int daz;

                if ((sector[sect].lotag & 0xff) > 2 || s->statnum == 4 || s->statnum == 5 || s->picnum == DRONE || s->picnum == COMMANDER)
                    daz = sector[sect].floorz;
                else
                    daz = hittype[i].floorz;


                if ((s->z - daz) < (8 << 8) && ps[screenpeek].posz < daz)
                {
                    auto shadowspr = &tsprite[spritesortcnt];
                    *shadowspr = *t;

                    shadowspr->statnum = 99;

                    shadowspr->yrepeat = (t->yrepeat >> 3);
                    if (t->yrepeat < 4) t->yrepeat = 4;
                    shadowspr->shade = 127;
                    shadowspr->cstat |= 2;

                    shadowspr->z = daz;
                    shadowspr->pal = 4;

                    if (videoGetRenderMode() >= REND_POLYMOST)
                    {
                        /*
                        if (hw_models && md_tilehasmodel(t->picnum, t->pal) >= 0)
                        {
                            shadowspr->yrepeat = 0;
                            // 512:trans reverse
                            //1024:tell MD2SPRITE.C to use Z-buffer hacks to hide overdraw issues
                            shadowspr->clipdist |= TSPR_FLAGS_MDHACK;
                            shadowspr->cstat |= 512;
                        }
                        else
                        */
                        {
                            // Alter the shadow's position so that it appears behind the sprite itself.
                            int look = getangle(shadowspr->x - ps[screenpeek].posx, shadowspr->y - ps[screenpeek].posy);
                            shadowspr->x += sintable[(look + 2560) & 2047] >> 9;
                            shadowspr->y += sintable[(look + 2048) & 2047] >> 9;
                        }
                    }
                    spritesortcnt++;
                }
            }

            if( ps[screenpeek].heat_amount > 0 && ps[screenpeek].heat_on )
            {
                t->pal = 6;
                t->shade = 0;
            }
        }


        switch(s->picnum)
        {
            case LASERLINE:
                if(sector[t->sectnum].lotag == 2) t->pal = 8;
                t->z = sprite[s->owner].z-(3<<8);
                if(lasermode == 2 && ps[screenpeek].heat_on == 0 )
                    t->yrepeat = 0;
            case EXPLOSION2:
            case EXPLOSION2BOT:
            case FREEZEBLAST:
            case ATOMICHEALTH:
            case FIRELASER:
            case SHRINKSPARK:
            case GROWSPARK:
            case CHAINGUN:
            case SHRINKEREXPLOSION:
            case RPG:
            case FLOORFLAME:
                if(t->picnum == EXPLOSION2)
                {
                    ps[screenpeek].visibility = -127;
                    lastvisinc = (int)totalclock+32;
                    //restorepalette = 1;	// JBF 20040101: why?
                }
                t->shade = -127;
                break;
            case FIRE:
            case FIRE2:
            case BURNING:
            case BURNING2:
                if( sprite[s->owner].picnum != TREE1 && sprite[s->owner].picnum != TREE2 )
                    t->z = sector[t->sectnum].floorz;
                t->shade = -127;
                break;
            case COOLEXPLOSION1:
                t->shade = -127;
                t->picnum += (s->shade>>1);
                break;
            case PLAYERONWATER:
				/*if (bpp > 8 && usemodels && md_tilehasmodel(s->picnum) >= 0) {
					k = 0;
					t->cstat &= ~4;
				} else*/ {
	                k = (((t->ang+3072+128-a)&2047)>>8)&7;
    				if(k>4)
            	    {
                	    k = 8-k;
                    	t->cstat |= 4;
	                }
    	            else t->cstat &= ~4;
				}

                t->picnum = s->picnum+k+((hittype[i].temp_data[0]<4)*5);
                t->shade = sprite[s->owner].shade;

                break;

            case WATERSPLASH2:
                t->picnum = WATERSPLASH2+t1;
                break;
            case REACTOR2:
                t->picnum = s->picnum + hittype[i].temp_data[2];
                break;
            case SHELL:
                t->picnum = s->picnum+(hittype[i].temp_data[0]&1);
            case SHOTGUNSHELL:
                t->cstat |= 12;
                if(hittype[i].temp_data[0] > 1) t->cstat &= ~4;
                if(hittype[i].temp_data[0] > 2) t->cstat &= ~12;
                break;
            case FRAMEEFFECT1:
                if(s->owner >= 0 && sprite[s->owner].statnum < MAXSTATUS)
                {
                    if(sprite[s->owner].picnum == APLAYER)
                        if(ud.camerasprite == -1)
                            if(screenpeek == sprite[s->owner].yvel && display_mirror == 0)
                    {
                        t->owner = -1;
                        break;
                    }
                    if( (sprite[s->owner].cstat&32768) == 0 )
                    {
                        t->picnum = hittype[s->owner].dispicnum;
                        t->pal = sprite[s->owner].pal;
                        t->shade = sprite[s->owner].shade;
                        t->ang = sprite[s->owner].ang;
                        t->cstat = 2|sprite[s->owner].cstat;
                    }
                }
                break;
            
            case CAMERA1:
            case RAT:
				/*if (bpp > 8 && usemodels && md_tilehasmodel(s->picnum) >= 0) {
					t->cstat &= ~4;
					break;
				}*/
				
                k = (((t->ang+3072+128-a)&2047)>>8)&7;
                if(k>4)
                {
                    k = 8-k;
                    t->cstat |= 4;
                }
                else t->cstat &= ~4;
                t->picnum = s->picnum+k;
                break;
        }

        hittype[i].dispicnum = t->picnum;
        if(sector[t->sectnum].floorpicnum == MIRROR)
            t->xrepeat = t->yrepeat = 0;
    }
}


END_DUKE_NS