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
#include "prediction.h"
#include "names_d.h"
#include "dukeactor.h"
#include "gamefuncs.h"

EXTERN_CVAR(Bool, wt_commentary)

BEGIN_DUKE_NS

void animatesprites_d(spritetype* tsprite, int& spritesortcnt, int x, int y, int a, int smoothratio)
{
	int i, j, k, p;
	int l, t1, t3, t4;
	spritetype* s;
	tspritetype* t;
	DDukeActor* h;

	for (j = 0; j < spritesortcnt; j++)
	{
		t = &tsprite[j];
		i = t->owner;
		h = &hittype[i];
		s = h->s;

		switch (t->picnum)
		{
		case DEVELOPERCOMMENTARY:
		case DEVELOPERCOMMENTARY + 1:
			if (isWorldTour() && !wt_commentary)
				t->xrepeat = t->yrepeat = 0;
			break;
		case BLOODPOOL:
		case PUKE:
		case FOOTPRINTS:
		case FOOTPRINTS2:
		case FOOTPRINTS3:
		case FOOTPRINTS4:
			if (t->shade == 127) continue;
			break;
		case RESPAWNMARKERRED:
		case RESPAWNMARKERYELLOW:
		case RESPAWNMARKERGREEN:
			if (ud.marker == 0)
				t->xrepeat = t->yrepeat = 0;
			continue;
		case CHAIR3:
			if (hw_models && md_tilehasmodel(t->picnum, t->pal) >= 0) 
			{
				t->cstat &= ~4;
				break;
			}

			k = (((t->ang + 3072 + 128 - a) & 2047) >> 8) & 7;
			if (k > 4)
			{
				k = 8 - k;
				t->cstat |= 4;
			}
			else t->cstat &= ~4;
			t->picnum = s->picnum + k;
			break;
		case BLOODSPLAT1:
		case BLOODSPLAT2:
		case BLOODSPLAT3:
		case BLOODSPLAT4:
			if (t->pal == 6)
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
		case GREENSLIME + 1:
		case GREENSLIME + 2:
		case GREENSLIME + 3:
		case GREENSLIME + 4:
		case GREENSLIME + 5:
		case GREENSLIME + 6:
		case GREENSLIME + 7:
			break;
		default:
			if (((t->cstat & 16)) || (badguy(t) && t->extra > 0) || t->statnum == 10)
				continue;
		}

		if (t->sector()->ceilingstat & 1)
			l = t->sector()->ceilingshade;
		else
			l = t->sector()->floorshade;

		if (l < -127) l = -127;
		if (l > 128) l = 127;
		t->shade = l;
	}


	//Between drawrooms() and drawmasks() is the perfect time to animate sprites
	for (j = 0; j < spritesortcnt; j++)  
	{
		t = &tsprite[j];
		i = t->owner;
		h = &hittype[i];
		s = h->s;
		auto OwnerAc = h->GetOwner();
		auto Owner = OwnerAc ? OwnerAc->s : nullptr;

		switch (s->picnum)
		{
		case SECTOREFFECTOR:
			if (t->lotag == 27 && ud.recstat == 1)
			{
				t->picnum = 11 + ((PlayClock >> 3) & 1);
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

		if (t->statnum == 99) continue;
		if (s->statnum != STAT_ACTOR && s->picnum == APLAYER && ps[s->yvel].newOwner == nullptr && h->GetOwner())
		{
			t->x -= MulScale(MaxSmoothRatio - smoothratio, ps[s->yvel].pos.x - ps[s->yvel].oposx, 16);
			t->y -= MulScale(MaxSmoothRatio - smoothratio, ps[s->yvel].pos.y - ps[s->yvel].oposy, 16);
			t->z = interpolatedvalue(ps[s->yvel].oposz, ps[s->yvel].pos.z, smoothratio);
			t->z += PHEIGHT_DUKE;
		}
		else if (s->picnum != CRANEPOLE)
		{
			t->pos = s->interpolatedvec3(smoothratio);
		}

		auto sectp = s->sector();
		t1 = h->temp_data[1];
		t3 = h->temp_data[3];
		t4 = h->temp_data[4];

		switch (s->picnum)
		{
		case DUKELYINGDEAD:
			t->z += (24 << 8);
			break;
		case BLOODPOOL:
		case FOOTPRINTS:
		case FOOTPRINTS2:
		case FOOTPRINTS3:
		case FOOTPRINTS4:
			if (t->pal == 6)
				t->shade = -127;
		case PUKE:
		case MONEY:
		case MONEY + 1:
		case MAIL:
		case MAIL + 1:
		case PAPER:
		case PAPER + 1:
			break;
		case TRIPBOMB:
			continue;
		case FORCESPHERE:
			if (t->statnum == STAT_MISC && Owner)
			{
				int sqa =
					getangle(
						Owner->x - ps[screenpeek].pos.x,
						Owner->y - ps[screenpeek].pos.y);
				int sqb =
					getangle(
						Owner->x - t->x,
						Owner->y - t->y);

				if (abs(getincangle(sqa, sqb)) > 512)
					if (ldist(Owner, t) < ldist(ps[screenpeek].GetActor()->s, Owner))
						t->xrepeat = t->yrepeat = 0;
			}
			continue;
		case BURNING:
		case BURNING2:
			if (Owner && Owner->statnum == STAT_PLAYER)
			{
				if (display_mirror == 0 && Owner->yvel == screenpeek && ps[screenpeek].over_shoulder_on == 0)
					t->xrepeat = 0;
				else
				{
					t->ang = getangle(x - t->x, y - t->y);
					t->x = Owner->x;
					t->y = Owner->y;
					t->x += bcos(t->ang, -10);
					t->y += bsin(t->ang, -10);
				}
			}
			break;

		case ATOMICHEALTH:
			t->z -= (4 << 8);
			break;
		case CRYSTALAMMO:
			t->shade = bsin(PlayClock << 4, -10);
			continue;
		case VIEWSCREEN:
		case VIEWSCREEN2:
			if (camsprite != nullptr && h->GetHitOwner()->temp_data[0] == 1)
			{
				t->picnum = STATIC;
				t->cstat |= (rand() & 12);
				t->xrepeat += 8;
				t->yrepeat += 8;
			}
			else if (camsprite == h->GetHitOwner())
			{
				t->picnum = TILE_VIEWSCR;
			}
			break;

		case SHRINKSPARK:
			t->picnum = SHRINKSPARK + ((PlayClock >> 4) & 3);
			break;
		case GROWSPARK:
			t->picnum = GROWSPARK + ((PlayClock >> 4) & 3);
			break;
		case RPG:
			if (hw_models && md_tilehasmodel(s->picnum, s->pal) >= 0) 
			{
				t->cstat &= ~4;
				break;
			}

			k = getangle(s->x - x, s->y - y);
			k = (((s->ang + 3072 + 128 - k) & 2047) / 170);
			if (k > 6)
			{
				k = 12 - k;
				t->cstat |= 4;
			}
			else t->cstat &= ~4;
			t->picnum = RPG + k;
			break;

		case RECON:
			if (hw_models && md_tilehasmodel(s->picnum, s->pal) >= 0) 
			{
				t->cstat &= ~4;
				break;
			}

			k = getangle(s->x - x, s->y - y);
			if (h->temp_data[0] < 4)
				k = (((s->ang + 3072 + 128 - k) & 2047) / 170);
			else k = (((s->ang + 3072 + 128 - k) & 2047) / 170);

			if (k > 6)
			{
				k = 12 - k;
				t->cstat |= 4;
			}
			else t->cstat &= ~4;

			if (abs(t3) > 64) k += 7;
			t->picnum = RECON + k;

			break;

		case APLAYER:

			p = s->yvel;

			if (t->pal == 1) t->z -= (18 << 8);

			if (ps[p].over_shoulder_on > 0 && ps[p].newOwner == nullptr)
			{
				t->cstat |= 2;
				if (screenpeek == myconnectindex && numplayers >= 2)
				{
					t->x = interpolatedvalue(omyx, myx, smoothratio);
					t->y = interpolatedvalue(omyy, myy, smoothratio);
					t->z = interpolatedvalue(omyz, myz, smoothratio) + PHEIGHT_DUKE;
					t->ang = interpolatedangle(omyang, myang, smoothratio).asbuild();
					t->sectnum = mycursectnum;
				}
			}

			if ((display_mirror == 1 || screenpeek != p || !h->GetOwner()) && ud.multimode > 1 && cl_showweapon && ps[p].GetActor()->s->extra > 0 && ps[p].curr_weapon > 0)
			{
				auto newtspr = &tsprite[spritesortcnt];
				memcpy(newtspr, t, sizeof(spritetype));

				newtspr->statnum = 99;

				newtspr->yrepeat = (t->yrepeat >> 3);
				if (t->yrepeat < 4) t->yrepeat = 4;

				newtspr->shade = t->shade;
				newtspr->cstat = 0;

				switch (ps[p].curr_weapon)
				{
				case PISTOL_WEAPON:      newtspr->picnum = FIRSTGUNSPRITE;       break;
				case SHOTGUN_WEAPON:     newtspr->picnum = SHOTGUNSPRITE;        break;
				case CHAINGUN_WEAPON:    newtspr->picnum = CHAINGUNSPRITE;       break;
				case RPG_WEAPON:         newtspr->picnum = RPGSPRITE;            break;
				case HANDREMOTE_WEAPON:
				case HANDBOMB_WEAPON:    newtspr->picnum = HEAVYHBOMB;           break;
				case TRIPBOMB_WEAPON:    newtspr->picnum = TRIPBOMBSPRITE;       break;
				case GROW_WEAPON:        newtspr->picnum = GROWSPRITEICON;       break;
				case SHRINKER_WEAPON:    newtspr->picnum = SHRINKERSPRITE;       break;
				case FREEZE_WEAPON:      newtspr->picnum = FREEZESPRITE;         break;
				case FLAMETHROWER_WEAPON: //Twentieth Anniversary World Tour
					if (isWorldTour())
						newtspr->picnum = FLAMETHROWERSPRITE;   break;
				case DEVISTATOR_WEAPON:  newtspr->picnum = DEVISTATORSPRITE;     break;
				}

				if (h->GetOwner())
					newtspr->z = ps[p].pos.z - (12 << 8);
				else newtspr->z = s->z - (51 << 8);
				if (ps[p].curr_weapon == HANDBOMB_WEAPON)
				{
					newtspr->xrepeat = 10;
					newtspr->yrepeat = 10;
				}
				else
				{
					newtspr->xrepeat = 16;
					newtspr->yrepeat = 16;
				}
				newtspr->pal = 0;
				spritesortcnt++;
			}

			if (!h->GetOwner())
			{
				if (hw_models && md_tilehasmodel(s->picnum, s->pal) >= 0) 
				{
					k = 0;
					t->cstat &= ~4;
				}
				else
				{
					k = (((s->ang + 3072 + 128 - a) & 2047) >> 8) & 7;
					if (k > 4)
					{
						k = 8 - k;
						t->cstat |= 4;
					}
					else t->cstat &= ~4;
				}

				if (t->sector()->lotag == 2) k += 1795 - 1405;
				else if ((h->floorz - s->z) > (64 << 8)) k += 60;

				t->picnum += k;
				t->pal = ps[p].palookup;

				goto PALONLY;
			}

			if (ps[p].on_crane == nullptr && (s->sector()->lotag & 0x7ff) != 1)
			{
				l = s->z - ps[p].GetActor()->floorz + (3 << 8);
				if (l > 1024 && s->yrepeat > 32 && s->extra > 0)
					s->yoffset = (int8_t)(l / (s->yrepeat << 2));
				else s->yoffset = 0;
			}

			if (ps[p].newOwner != nullptr)
			{
				t4 = ScriptCode[gs.actorinfo[APLAYER].scriptaddress + 1];
				t3 = 0;
				t1 = ScriptCode[gs.actorinfo[APLAYER].scriptaddress + 2];
			}

			if (ud.cameraactor == nullptr && ps[p].newOwner == nullptr)
				if (h->GetOwner() && display_mirror == 0 && ps[p].over_shoulder_on == 0)
					if (ud.multimode < 2 || (ud.multimode > 1 && p == screenpeek))
					{
						t->owner = -1;
						t->xrepeat = t->yrepeat = 0;
						continue;
					}

		PALONLY:

			if (sectp->floorpal)
				copyfloorpal(t, sectp);

			if (!h->GetOwner()) continue;

			if (t->z > h->floorz && t->xrepeat < 32)
				t->z = h->floorz;

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
			if (t->pal == 6) t->shade = -120;

		case SCRAP1:
		case SCRAP2:
		case SCRAP3:
		case SCRAP4:
		case SCRAP5:
		case SCRAP6:
		case SCRAP6 + 1:
		case SCRAP6 + 2:
		case SCRAP6 + 3:
		case SCRAP6 + 4:
		case SCRAP6 + 5:
		case SCRAP6 + 6:
		case SCRAP6 + 7:

			if (h->picnum == BLIMP && t->picnum == SCRAP1 && s->yvel >= 0)
				t->picnum = s->yvel;
			else t->picnum += h->temp_data[0];
			t->shade -= 6;

			if (sectp->floorpal)
				copyfloorpal(t, sectp);
			break;

		case WATERBUBBLE:
			if (t->sector()->floorpicnum == FLOORSLIME)
			{
				t->pal = 7;
				break;
			}
		default:

			if (sectp->floorpal)
				copyfloorpal(t, sectp);
			break;
		}

		if (gs.actorinfo[s->picnum].scriptaddress)
		{
			if (t4)
			{
				l = ScriptCode[t4 + 2];

				if (hw_models && md_tilehasmodel(s->picnum, s->pal) >= 0) 
				{
					k = 0;
					t->cstat &= ~4;
				}
				else switch (l) 
				{
				case 2:
					k = (((s->ang + 3072 + 128 - a) & 2047) >> 8) & 1;
					break;

				case 3:
				case 4:
					k = (((s->ang + 3072 + 128 - a) & 2047) >> 7) & 7;
					if (k > 3)
					{
						t->cstat |= 4;
						k = 7 - k;
					}
					else t->cstat &= ~4;
					break;

				case 5:
					k = getangle(s->x - x, s->y - y);
					k = (((s->ang + 3072 + 128 - k) & 2047) >> 8) & 7;
					if (k > 4)
					{
						k = 8 - k;
						t->cstat |= 4;
					}
					else t->cstat &= ~4;
					break;
				case 7:
					k = getangle(s->x - x, s->y - y);
					k = (((s->ang + 3072 + 128 - k) & 2047) / 170);
					if (k > 6)
					{
						k = 12 - k;
						t->cstat |= 4;
					}
					else t->cstat &= ~4;
					break;
				case 8:
					k = (((s->ang + 3072 + 128 - a) & 2047) >> 8) & 7;
					t->cstat &= ~4;
					break;
				default:
					k = 0;
					break;
				}

				t->picnum += k + ScriptCode[t4] + l * t3;

				if (l > 0)
					while (!tileGetTexture(t->picnum)->isValid() && t->picnum > 0)
						t->picnum -= l;       //Hack, for actors 

				if (h->dispicnum >= 0)
					h->dispicnum = t->picnum;
			}
			else if (display_mirror == 1)
				t->cstat |= 4;
		}

		if (s->statnum == STAT_DUMMYPLAYER || badguy(s) || (s->picnum == APLAYER && h->GetOwner()))
			if (t->statnum != 99 && s->picnum != EXPLOSION2 && s->picnum != HANGLIGHT && s->picnum != DOMELITE)
				if (s->picnum != HOTMEAT)
				{
					if (h->dispicnum < 0)
					{
						h->dispicnum++;
						continue;
					}
					else if (r_shadows && spritesortcnt < (MAXSPRITESONSCREEN - 2))
					{
						int daz;

						if ((sectp->lotag & 0xff) > 2 || s->statnum == 4 || s->statnum == 5 || s->picnum == DRONE || s->picnum == COMMANDER)
							daz = sectp->floorz;
						else
							daz = h->floorz;


						if ((s->z - daz) < (8 << 8) && ps[screenpeek].pos.z < daz)
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

							if (hw_models && md_tilehasmodel(t->picnum, t->pal) >= 0)
							{
								shadowspr->yrepeat = 0;
								// 512:trans reverse
								//1024:tell MD2SPRITE.C to use Z-buffer hacks to hide overdraw issues
								shadowspr->clipdist |= TSPR_FLAGS_MDHACK;
								shadowspr->cstat |= 512;
							}
							else
							{
								// Alter the shadow's position so that it appears behind the sprite itself.
								int look = getangle(shadowspr->x - ps[screenpeek].pos.x, shadowspr->y - ps[screenpeek].pos.y);
								shadowspr->x += bcos(look, -9);
								shadowspr->y += bsin(look, -9);
							}
							spritesortcnt++;
						}
					}

					if (ps[screenpeek].heat_amount > 0 && ps[screenpeek].heat_on)
					{
						t->pal = 6;
						t->shade = 0;
					}
				}


		switch (s->picnum)
		{
		case LASERLINE:
			if (!Owner) break;
			if (t->sector()->lotag == 2) t->pal = 8;
			t->z = Owner->z - (3 << 8);
			if (gs.lasermode == 2 && ps[screenpeek].heat_on == 0)
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
			if (t->picnum == EXPLOSION2)
			{
				ps[screenpeek].visibility = -127;
				lastvisinc = PlayClock + 32;
			}
			t->shade = -127;
			break;
		case FIRE:
		case FIRE2:
			t->cstat |= 128;
		case BURNING:
		case BURNING2:
			if (!Owner) break;
			if (Owner->picnum != TREE1 && Owner->picnum != TREE2)
				t->z = t->sector()->floorz;
			t->shade = -127;
			break;
		case COOLEXPLOSION1:
			t->shade = -127;
			t->picnum += (s->shade >> 1);
			break;
		case PLAYERONWATER:
			if (hw_models && md_tilehasmodel(s->picnum, s->pal) >= 0) 
			{
				k = 0;
				t->cstat &= ~4;
			}
			else
			{
			k = (((t->ang + 3072 + 128 - a) & 2047) >> 8) & 7;
			if (k > 4)
			{
				k = 8 - k;
				t->cstat |= 4;
			}
			else t->cstat &= ~4;
		}

			t->picnum = s->picnum + k + ((h->temp_data[0] < 4) * 5);
			if (Owner) t->shade = Owner->shade;

			break;

		case WATERSPLASH2:
			t->picnum = WATERSPLASH2 + t1;
			break;
		case REACTOR2:
			t->picnum = s->picnum + h->temp_data[2];
			break;
		case SHELL:
			t->picnum = s->picnum + (h->temp_data[0] & 1);
		case SHOTGUNSHELL:
			t->cstat |= 12;
			if (h->temp_data[0] > 1) t->cstat &= ~4;
			if (h->temp_data[0] > 2) t->cstat &= ~12;
			break;
		case FRAMEEFFECT1:
			if (Owner && Owner->statnum < MAXSTATUS)
			{
				if (Owner->picnum == APLAYER)
					if (ud.cameraactor == nullptr)
						if (screenpeek == Owner->yvel && display_mirror == 0)
						{
							t->owner = -1;
							break;
						}
				if ((Owner->cstat & 32768) == 0)
				{
					t->picnum = OwnerAc->dispicnum;
					t->pal = Owner->pal;
					t->shade = Owner->shade;
					t->ang = Owner->ang;
					t->cstat = 2 | Owner->cstat;
				}
			}
			break;

		case CAMERA1:
		case RAT:
			if (hw_models && md_tilehasmodel(s->picnum, s->pal) >= 0) 
			{
				t->cstat &= ~4;
				break;
			}

			k = (((t->ang + 3072 + 128 - a) & 2047) >> 8) & 7;
			if (k > 4)
			{
				k = 8 - k;
				t->cstat |= 4;
			}
			else t->cstat &= ~4;
			t->picnum = s->picnum + k;
			break;
		}

		h->dispicnum = t->picnum;
		if (t->sector()->floorpicnum == MIRROR)
			t->xrepeat = t->yrepeat = 0;
	}
}


END_DUKE_NS
