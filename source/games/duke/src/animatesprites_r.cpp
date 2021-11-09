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
#include "prediction.h"
#include "dukeactor.h"
#include "gamefuncs.h"

BEGIN_DUKE_NS


void animatesprites_r(spritetype* tsprite, int& spritesortcnt, int x, int y, int a, int smoothratio)
{
	int i, j, k, p;
	int l, t1, t3, t4;
	spritetype* s;
	tspritetype* t;
	DDukeActor* h;

	int bg = 0;

	for (j = 0; j < spritesortcnt; j++)
	{
		t = &tsprite[j];
		i = t->owner;
		h = &hittype[i];
		s = h->s;

		switch (t->picnum)
		{
		case BLOODPOOL:
		case FOOTPRINTS:
		case FOOTPRINTS2:
		case FOOTPRINTS3:
		case FOOTPRINTS4:
			if (t->shade == 127) continue;
			break;
		case CHAIR3:

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

		case RRTILE1947:
		case RRTILE2859:
		case RRTILE3774:
		case RRTILE5088:
		case RRTILE8094:
		case RRTILE8096:
			if (isRRRA()) continue;

		case NEON1:
		case NEON2:
		case NEON3:
		case NEON4:
		case NEON5:
		case NEON6:
			continue;
		default:
			if (((t->cstat & 16)) || (badguy(t) && t->extra > 0) || t->statnum == 10)
			{
				if (shadedsector[s->sectnum] == 1 && s->statnum != 1)
				{
					s->shade = 16;
					t->shade = 16;
				}
				continue;
			}
		}

		if (t->sector()->ceilingstat & 1)
		{
			if (badguy(s))
				l = s->shade;
			else
				l = s->shade;
		}
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
		default:
			break;
		}

		if (t->statnum == 99) continue;
		if (s->statnum != STAT_ACTOR && s->picnum == APLAYER && ps[s->yvel].newOwner == nullptr && h->GetOwner())
		{
			t->x -= MulScale(MaxSmoothRatio - smoothratio, ps[s->yvel].pos.x - ps[s->yvel].oposx, 16);
			t->y -= MulScale(MaxSmoothRatio - smoothratio, ps[s->yvel].pos.y - ps[s->yvel].oposy, 16);
			t->z = interpolatedvalue(ps[s->yvel].oposz, ps[s->yvel].pos.z, smoothratio);
			t->z += PHEIGHT_RR;
			s->xrepeat = 24;
			s->yrepeat = 17;
		}
		else if (s->picnum != CRANEPOLE)
		{
			t->pos = s->interpolatedvec3(smoothratio);
		}

		//sect = s->sectnum;
		auto sectp = s->sector();
		t1 = h->temp_data[1];
		t3 = h->temp_data[3];
		t4 = h->temp_data[4];

		switch (s->picnum)
		{
		case RESPAWNMARKERRED:
		case RESPAWNMARKERYELLOW:
		case RESPAWNMARKERGREEN:
			t->picnum = 861 + ((PlayClock >> 4) & 13);
			if (s->picnum == RESPAWNMARKERRED)
				t->pal = 0;
			else if (s->picnum == RESPAWNMARKERYELLOW)
				t->pal = 1;
			else
				t->pal = 2;
			if (ud.marker == 0)
				t->xrepeat = t->yrepeat = 0;
			break;
		case DUKELYINGDEAD:
			s->xrepeat = 24;
			s->yrepeat = 17;
			if (s->extra > 0)
				t->z += (6 << 8);
			break;
		case BLOODPOOL:
		case FOOTPRINTS:
		case FOOTPRINTS2:
		case FOOTPRINTS3:
		case FOOTPRINTS4:
			if (t->pal == 6)
				t->shade = -127;
		case MONEY:
		case MONEY + 1:
			break;
		case TRIPBOMBSPRITE:
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
			if (Owner && Owner->statnum == STAT_PLAYER)
			{
				if (display_mirror == 0 && Owner->yvel == screenpeek && ps[Owner->yvel].over_shoulder_on == 0)
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
			break;
		case SHRINKSPARK:
			if (Owner && (Owner->picnum == CHEER || Owner->picnum == CHEERSTAYPUT) && isRRRA())
			{
				t->picnum = CHEERBLADE + ((PlayClock >> 4) & 3);
				t->shade = -127;
			}
			else
				t->picnum = SHRINKSPARK + ((PlayClock >> 4) & 7);
			break;
		case CHEERBOMB:
			if (isRRRA())
			{
				t->picnum = CHEERBOMB + ((PlayClock >> 4) & 3);
				break;
			}
			else goto default_case;
		case SPIT:
			if (isRRRA() && Owner)
			{
				if (Owner->picnum == MINION && Owner->pal == 8)
					t->picnum = RRTILE3500 + ((PlayClock >> 4) % 6);
				else if (Owner->picnum == MINION && Owner->pal == 19)
				{
					t->picnum = RRTILE5090 + ((PlayClock >> 4) & 3);
					t->shade = -127;
				}
				else if (Owner->picnum == MAMA)
				{
					k = getangle(s->x - x, s->y - y);
					k = (((s->ang + 3072 + 128 - k) & 2047) >> 8) & 7;
					if (k > 4)
					{
						k = 8 - k;
						t->cstat |= 4;
					}
					else t->cstat &= ~4;
					t->picnum = RRTILE7274 + k;
				}
				else
					t->picnum = SPIT + ((PlayClock >> 4) & 3);
			}
			else
				t->picnum = SPIT + ((PlayClock >> 4) & 3);
			break;
		case EMPTYBIKE:
			if (!isRRRA()) goto default_case;
			k = getangle(s->x - x, s->y - y);
			k = (((s->ang + 3072 + 128 - k) & 2047) / 170);
			if (k > 6)
			{
				k = 12 - k;
				t->cstat |= 4;
			}
			else t->cstat &= ~4;
			t->picnum = EMPTYBIKE + k;
			break;
		case EMPTYBOAT:
			if (!isRRRA()) goto default_case;
			k = getangle(s->x - x, s->y - y);
			k = (((s->ang + 3072 + 128 - k) & 2047) / 170);
			if (k > 6)
			{
				k = 12 - k;
				t->cstat |= 4;
			}
			else t->cstat &= ~4;
			t->picnum = EMPTYBOAT + k;
			break;
		case RPG:
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
		case RPG2:
			if (!isRRRA()) goto default_case;
			k = getangle(s->x - x, s->y - y);
			k = (((s->ang + 3072 + 128 - k) & 2047) / 170);
			if (k > 6)
			{
				k = 12 - k;
				t->cstat |= 4;
			}
			else t->cstat &= ~4;
			t->picnum = RPG2 + k;
			break;

		case RECON:

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
					t->z = interpolatedvalue(omyz, myz, smoothratio) + PHEIGHT_RR;
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
				case RIFLEGUN_WEAPON:    newtspr->picnum = CHAINGUNSPRITE;       break;
				case CROSSBOW_WEAPON:         newtspr->picnum = RPGSPRITE;            break;
				case CHICKEN_WEAPON:        newtspr->picnum = RPGSPRITE; break;
				case THROWINGDYNAMITE_WEAPON:
				case DYNAMITE_WEAPON:    newtspr->picnum = HEAVYHBOMB;           break;
				case POWDERKEG_WEAPON:    newtspr->picnum = TRIPBOMBSPRITE;       break;
				case BOWLING_WEAPON:     newtspr->picnum = 3437;                 break;
				case THROWSAW_WEAPON:    newtspr->picnum = SHRINKSPARK;          break;
				case BUZZSAW_WEAPON:        newtspr->picnum = SHRINKSPARK;          break;
				case ALIENBLASTER_WEAPON:      newtspr->picnum = DEVISTATORSPRITE;     break;
				case TIT_WEAPON:  newtspr->picnum = FREEZESPRITE;         break;
				}

				if (h->GetOwner())
					newtspr->z = ps[p].pos.z - (12 << 8);
				else newtspr->z = s->z - (51 << 8);
				if (ps[p].curr_weapon == HANDBOMB_WEAPON)
				{
					newtspr->xrepeat = 10;
					newtspr->yrepeat = 10;
				}
				else if (ps[p].OnMotorcycle || ps[p].OnBoat)
				{
					newtspr->xrepeat = 0;
					newtspr->yrepeat = 0;
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
				} else
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

			if (ps[p].OnMotorcycle && p == screenpeek)
			{
				t->picnum = RRTILE7219;
				t->xrepeat = 18;
				t->yrepeat = 18;
				t4 = 0;
				t3 = 0;
				t1 = 0;
			}
			else if (ps[p].OnMotorcycle)
			{
				k = (((s->ang + 3072 + 128 - a) & 2047) / 170);
				if (k > 6)
				{
					k = 12 - k;
					t->cstat |= 4;
				}
				else t->cstat &= ~4;

				t->picnum = RRTILE7213 + k;
				t->xrepeat = 18;
				t->yrepeat = 18;
				t4 = 0;
				t3 = 0;
				t1 = 0;
			}
			else if (ps[p].OnBoat && p == screenpeek)
			{
				t->picnum = RRTILE7190;
				t->xrepeat = 32;
				t->yrepeat = 32;
				t4 = 0;
				t3 = 0;
				t1 = 0;
			}
			else if (ps[p].OnBoat)
			{
				k = (((s->ang + 3072 + 128 - a) & 2047) / 170);
				if (k > 6)
				{
					k = 12 - k;
					t->cstat |= 4;
				}
				else t->cstat &= ~4;

				t->picnum = RRTILE7184 + k;
				t->xrepeat = 32;
				t->yrepeat = 32;
				t4 = 0;
				t3 = 0;
				t1 = 0;
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
			if (isRRRA()) goto stuff;
			else goto default_case;

		case MINJIBA:
		case MINJIBB:
		case MINJIBC:
			if (isRRRA() && t->pal == 19)
				t->shade = -127;
		case JIBS1:
		case JIBS2:
		case JIBS3:
		case JIBS4:
		case JIBS5:
		case JIBS6:
		case DUKEGUN:
		case DUKETORSO:
		case DUKELEG:
		case BILLYJIBA:
		case BILLYJIBB:
		case HULKJIBA:
		case HULKJIBB:
		case HULKJIBC:
		case COOTJIBA:
		case COOTJIBB:
		case COOTJIBC:
		stuff:
			if (t->pal == 6) t->shade = -120;

			if (shadedsector[s->sectnum] == 1)
				t->shade = 16;

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

			if (t->picnum == SCRAP1 && s->yvel >= 0)
				t->picnum = s->yvel;
			else t->picnum += h->temp_data[0];

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
		default_case:

			if (sectp->floorpal)
				copyfloorpal(t, sectp);
			break;
		}

		if (gs.actorinfo[s->picnum].scriptaddress && (t->cstat & 48) != 48)
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
					bg = badguy(s);
					if (bg && s->statnum == 2 && s->extra > 0)
					{
						k = getangle(s->x - x, s->y - y);
						k = (((s->ang + 3072 + 128 - k) & 2047) >> 8) & 7;
						if (k > 4)
						{
							k = 8 - k;
							t->cstat |= 4;
						}
						else t->cstat &= ~4;
						break;
					}
					k = 0;
					bg = 0;
					break;
				}

				t->picnum += k + ScriptCode[t4] + l * t3;

				if (l > 0)
					while (t->picnum >= 0 && t->picnum < MAXTILES && !tileGetTexture(t->picnum)->isValid() && t->picnum > 0)
						t->picnum -= l;       //Hack, for actors 
				if (t->picnum < 0 || t->picnum >= MAXTILES)
				{
					t->picnum = 0;
					t->xrepeat = t->yrepeat = 0;
				}

				if (h->dispicnum >= 0)
					h->dispicnum = t->picnum;
			}
			else if (display_mirror == 1)
				t->cstat |= 4;
		}

		if (!isRRRA() && s->picnum == SBMOVE)
			t->shade = -127;

		if (s->statnum == 13 || badguy(s) || (s->picnum == APLAYER && h->GetOwner()))
			if ((s->cstat & 48) == 0 && t->statnum != 99)
				if (s->picnum != EXPLOSION2 && s->picnum != DOMELITE && s->picnum != TORNADO && s->picnum != EXPLOSION3 && (s->picnum != SBMOVE || isRRRA()))
				{
					if (h->dispicnum < 0)
					{
						h->dispicnum++;
						continue;
					}
					else if (r_shadows && spritesortcnt < (MAXSPRITESONSCREEN - 2))
					{
						int daz;

						if (isRRRA() && sectp->lotag == 160) continue;
						if ((sectp->lotag & 0xff) > 2 || s->statnum == 4 || s->statnum == 5 || s->picnum == DRONE)
							daz = sectp->floorz;
						else
							daz = h->floorz;

						if ((s->z - daz) < (8 << 8))
							if (ps[screenpeek].pos.z < daz)
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
				}


		switch (s->picnum)
		{
		case RPG2:
		case RRTILE1790:
			if (!isRRRA()) break;
		case EXPLOSION2:
		case FREEZEBLAST:
		case ATOMICHEALTH:
		case FIRELASER:
		case SHRINKSPARK:
		case CHAINGUN:
		case RPG:
		case EXPLOSION3:
		case COOLEXPLOSION1:
		case OWHIP:
		case UWHIP:
			if (t->picnum == EXPLOSION2)
			{
				ps[screenpeek].visibility = -127;
				lastvisinc = PlayClock + 32;
				t->pal = 0;
			}
			else if (t->picnum == FIRELASER)
			{
				t->picnum = FIRELASER + ((PlayClock >> 2) & 5);
			}
			t->shade = -127;
			break;
		case UFOBEAM:
		case RRTILE3586:
		case LADDER:
			t->cstat |= 32768;
			s->cstat |= 32768;
			break;
		case DESTRUCTO:
			t->cstat |= 32768;
			break;
		case FIRE:
		case BURNING:
			if (Owner && Owner->picnum != TREE1 && Owner->picnum != TREE2)
				t->z = t->sector()->floorz;
			t->shade = -127;
			break;
		case WALLLIGHT3:
		case WALLLIGHT1:
		case RRTILE3668:
		case RRTILE3795:
		case RRTILE5035:
		case RRTILE7505:
		case RRTILE7506:
		case RRTILE7533:
		case RRTILE8216:
		case RRTILE8218:
		case RRTILE8220:
			if (!isRRRA()) break;
		case RRTILE1878:
		case RRTILE1952:
		case RRTILE1953:
		case RRTILE1990:
		case RRTILE2050:
		case RRTILE2056:
		case RRTILE2072:
		case RRTILE2075:
		case RRTILE2083:
		case RRTILE2097:
		case RRTILE2156:
		case RRTILE2157:
		case RRTILE2158:
		case RRTILE2159:
		case RRTILE2160:
		case RRTILE2161:
		case RRTILE2175:
		case RRTILE2176:
		case RRTILE2357:
		case RRTILE2564:
		case RRTILE2573:
		case RRTILE2574:
		case RRTILE2583:
		case RRTILE2604:
		case RRTILE2689:
		case RRTILE2893:
		case RRTILE2894:
		case RRTILE2915:
		case RRTILE2945:
		case RRTILE2946:
		case RRTILE2947:
		case RRTILE2948:
		case RRTILE2949:
		case RRTILE2977:
		case RRTILE2978:
		case RRTILE3116:
		case RRTILE3171:
		case RRTILE3216:
		case RRTILE3720:
			t->shade = -127;
			break;
		case CHEER:
			if (!isRRRA()) break;
			if (t->picnum >= CHEER + 102 && t->picnum <= CHEER + 151)
				t->shade = -127;
			break;
		case MINION:
			if (!isRRRA()) break;
			if (t->pal == 19)
				t->shade = -127;
			break;
		case BIKER:
			if (!isRRRA()) break;
			if (t->picnum >= BIKER + 54 && t->picnum <= BIKER + 58)
				t->shade = -127;
			else if (t->picnum >= BIKER + 84 && t->picnum <= BIKER + 88)
				t->shade = -127;
			break;
		case BILLYRAY:
		case BILLYRAYSTAYPUT:
			if (!isRRRA()) break;
			if (t->picnum >= BILLYRAY + 5 && t->picnum <= BILLYRAY + 9)
				t->shade = -127;
			break;
		case RRTILE2034:
			t->picnum = RRTILE2034 + ((PlayClock >> 2) & 1);
			break;
		case RRTILE2944:
			t->shade = -127;
			t->picnum = RRTILE2944 + ((PlayClock >> 2) & 4);
			break;
		case PLAYERONWATER:

			k = (((t->ang + 3072 + 128 - a) & 2047) >> 8) & 7;
			if (k > 4)
			{
				k = 8 - k;
				t->cstat |= 4;
			}
			else t->cstat &= ~4;

			t->picnum = s->picnum + k + ((h->temp_data[0] < 4) * 5);
			if (Owner) t->shade = Owner->shade;
			break;
		case MUD:
			t->picnum = MUD + t1;
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
					if (Owner->picnum == APLAYER)
						t->picnum = 1554;
					else
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
