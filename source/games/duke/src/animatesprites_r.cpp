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


void animatesprites_r(tspriteArray& tsprites, int x, int y, int a, int smoothratio)
{
	int k, p;
	int l, t1, t3, t4;
	tspritetype* t;
	DDukeActor* h;

	int bg = 0;

	for (unsigned j = 0; j < tsprites.Size(); j++)
	{
		t = tsprites.get(j);
		h = static_cast<DDukeActor*>(t->ownerActor);

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

			k = (((t->int_ang() + 3072 + 128 - a) & 2047) >> 8) & 7;
			if (k > 4)
			{
				k = 8 - k;
				t->cstat |= CSTAT_SPRITE_XFLIP;
			}
			else t->cstat &= ~CSTAT_SPRITE_XFLIP;
			t->picnum = h->spr.picnum + k;
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
			[[fallthrough]];
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
			if (((t->cstat & CSTAT_SPRITE_ALIGNMENT_WALL)) || (badguypic(t->picnum) && t->extra > 0) || t->statnum == STAT_PLAYER)
			{
				if (h->sector()->shadedsector == 1 && h->spr.statnum != 1)
				{
					h->spr.shade = 16;
					t->shade = 16;
				}
				continue;
			}
		}

		if (t->sectp != nullptr)
			t->shade = clamp<int>(t->sectp->ceilingstat & CSTAT_SECTOR_SKY ? h->spr.shade : t->sectp->floorshade, -127, 127);
	}


	for (unsigned j = 0; j < tsprites.Size(); j++)
	{
		t = tsprites.get(j);
		h = static_cast<DDukeActor*>(t->ownerActor);
		auto OwnerAc = h->GetOwner();

		switch (h->spr.picnum)
		{
		case SECTOREFFECTOR:
			if (t->lotag == 27 && ud.recstat == 1)
			{
				t->picnum = 11 + ((PlayClock >> 3) & 1);
				t->cstat |= CSTAT_SPRITE_YCENTER;
			}
			else
				t->xrepeat = t->yrepeat = 0;
			break;
		default:
			break;
		}

		if (t->statnum == 99) continue;
		auto pp = &ps[h->PlayerIndex()];
		if (h->spr.statnum != STAT_ACTOR && h->spr.picnum == APLAYER && pp->newOwner == nullptr && h->GetOwner())
		{
			t->pos.X -= MulScaleF(MaxSmoothRatio - smoothratio, pp->pos.X - pp->opos.X, 16);
			t->pos.Y -= MulScaleF(MaxSmoothRatio - smoothratio, pp->pos.Y - pp->opos.Y, 16);
			t->pos.Z = interpolatedvalue(pp->opos.Z, pp->pos.Z, smoothratio) + gs.playerheight;
			h->spr.xrepeat = 24;
			h->spr.yrepeat = 17;
		}
		else if (!actorflag(h, SFLAG_NOINTERPOLATE))
		{
			t->pos = h->interpolatedvec3(smoothratio);
		}

		auto sectp = h->sector();
		t1 = h->temp_data[1];
		t3 = h->temp_data[3];
		t4 = h->temp_data[4];

		switch (h->spr.picnum)
		{
		case RESPAWNMARKERRED:
		case RESPAWNMARKERYELLOW:
		case RESPAWNMARKERGREEN:
			t->picnum = 861 + ((PlayClock >> 4) & 13);
			if (h->spr.picnum == RESPAWNMARKERRED)
				t->pal = 0;
			else if (h->spr.picnum == RESPAWNMARKERYELLOW)
				t->pal = 1;
			else
				t->pal = 2;
			if (ud.marker == 0)
				t->xrepeat = t->yrepeat = 0;
			break;
		case DUKELYINGDEAD:
			h->spr.xrepeat = 24;
			h->spr.yrepeat = 17;
			if (h->spr.extra > 0)
				t->pos.Z += 6;
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
		case POWDERKEG:
			continue;
		case FORCESPHERE:
			if (t->statnum == STAT_MISC && OwnerAc)
			{
				int sqa = getangle(OwnerAc->spr.pos.XY() - ps[screenpeek].pos.XY());
				int sqb = getangle(OwnerAc->spr.pos.XY() - t->pos.XY());

				if (abs(getincangle(sqa, sqb)) > 512)
					if (ldist(OwnerAc, t) < ldist(ps[screenpeek].GetActor(), OwnerAc))
						t->xrepeat = t->yrepeat = 0;
			}
			continue;
		case BURNING:
			if (OwnerAc && OwnerAc->spr.statnum == STAT_PLAYER)
			{
				if (display_mirror == 0 && OwnerAc->spr.yvel == screenpeek && ps[OwnerAc->spr.yvel].over_shoulder_on == 0)
					t->xrepeat = 0;
				else
				{
					t->set_int_ang(getangle(x - t->int_pos().X, y - t->int_pos().Y));
					t->pos.X = OwnerAc->spr.pos.X + t->angle.Cos();
					t->pos.Y = OwnerAc->spr.pos.Y + t->angle.Sin();
				}
			}
			break;

		case ATOMICHEALTH:
			t->pos.Z -= 4;
			break;
		case CRYSTALAMMO:
			t->shade = bsin(PlayClock << 4, -10);
			break;
		case SHRINKSPARK:
			if (OwnerAc && (OwnerAc->spr.picnum == CHEER || OwnerAc->spr.picnum == CHEERSTAYPUT) && isRRRA())
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
			if (isRRRA() && OwnerAc)
			{
				if (OwnerAc->spr.picnum == MINION && OwnerAc->spr.pal == 8)
					t->picnum = RRTILE3500 + ((PlayClock >> 4) % 6);
				else if (OwnerAc->spr.picnum == MINION && OwnerAc->spr.pal == 19)
				{
					t->picnum = RRTILE5090 + ((PlayClock >> 4) & 3);
					t->shade = -127;
				}
				else if (OwnerAc->spr.picnum == MAMA)
				{
					k = getangle(h->int_pos().X - x, h->int_pos().Y - y);
					k = (((h->int_ang() + 3072 + 128 - k) & 2047) >> 8) & 7;
					if (k > 4)
					{
						k = 8 - k;
						t->cstat |= CSTAT_SPRITE_XFLIP;
					}
					else t->cstat &= ~CSTAT_SPRITE_XFLIP;
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
			k = getangle(h->int_pos().X - x, h->int_pos().Y - y);
			k = (((h->int_ang() + 3072 + 128 - k) & 2047) / 170);
			if (k > 6)
			{
				k = 12 - k;
				t->cstat |= CSTAT_SPRITE_XFLIP;
			}
			else t->cstat &= ~CSTAT_SPRITE_XFLIP;
			t->picnum = EMPTYBIKE + k;
			break;
		case EMPTYBOAT:
			if (!isRRRA()) goto default_case;
			k = getangle(h->int_pos().X - x, h->int_pos().Y - y);
			k = (((h->int_ang() + 3072 + 128 - k) & 2047) / 170);
			if (k > 6)
			{
				k = 12 - k;
				t->cstat |= CSTAT_SPRITE_XFLIP;
			}
			else t->cstat &= ~CSTAT_SPRITE_XFLIP;
			t->picnum = EMPTYBOAT + k;
			break;
		case RPG:
			k = getangle(h->int_pos().X - x, h->int_pos().Y - y);
			k = (((h->int_ang() + 3072 + 128 - k) & 2047) / 170);
			if (k > 6)
			{
				k = 12 - k;
				t->cstat |= CSTAT_SPRITE_XFLIP;
			}
			else t->cstat &= ~CSTAT_SPRITE_XFLIP;
			t->picnum = RPG + k;
			break;
		case RPG2:
			if (!isRRRA()) goto default_case;
			k = getangle(h->int_pos().X - x, h->int_pos().Y - y);
			k = (((h->int_ang() + 3072 + 128 - k) & 2047) / 170);
			if (k > 6)
			{
				k = 12 - k;
				t->cstat |= CSTAT_SPRITE_XFLIP;
			}
			else t->cstat &= ~CSTAT_SPRITE_XFLIP;
			t->picnum = RPG2 + k;
			break;

		case RECON:

			k = getangle(h->int_pos().X - x, h->int_pos().Y - y);
			if (h->temp_data[0] < 4)
				k = (((h->int_ang() + 3072 + 128 - k) & 2047) / 170);
			else k = (((h->int_ang() + 3072 + 128 - k) & 2047) / 170);

			if (k > 6)
			{
				k = 12 - k;
				t->cstat |= CSTAT_SPRITE_XFLIP;
			}
			else t->cstat &= ~CSTAT_SPRITE_XFLIP;

			if (abs(t3) > 64) k += 7;
			t->picnum = RECON + k;

			break;

		case APLAYER:

			p = h->spr.yvel;

			if (t->pal == 1) t->pos.Z -= 18;

			if (ps[p].over_shoulder_on > 0 && ps[p].newOwner == nullptr)
			{
				t->cstat |= CSTAT_SPRITE_TRANSLUCENT;
#if 0 // multiplayer only
				if (screenpeek == myconnectindex && numplayers >= 2)
				{
					t->x = interpolatedvalue(omyx, myx, smoothratio);
					t->y = interpolatedvalue(omyy, myy, smoothratio);
					t->z = interpolatedvalue(omyz, myz, smoothratio) + gs.playerheight;
					t->ang = interpolatedangle(omyang, myang, smoothratio).asbuild();
					t->sector = mycursectnum;
				}
#endif
			}

			if ((display_mirror == 1 || screenpeek != p || !h->GetOwner()) && ud.multimode > 1 && cl_showweapon && ps[p].GetActor()->spr.extra > 0 && ps[p].curr_weapon > 0)
			{
				auto newtspr = tsprites.newTSprite();
				*newtspr = *t;

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
				case POWDERKEG_WEAPON:    newtspr->picnum = POWDERKEG;       break;
				case BOWLING_WEAPON:     newtspr->picnum = 3437;                 break;
				case THROWSAW_WEAPON:    newtspr->picnum = SHRINKSPARK;          break;
				case BUZZSAW_WEAPON:        newtspr->picnum = SHRINKSPARK;          break;
				case ALIENBLASTER_WEAPON:      newtspr->picnum = DEVISTATORSPRITE;     break;
				case TIT_WEAPON:  newtspr->picnum = FREEZESPRITE;         break;
				}

				if (h->GetOwner()) newtspr->pos.Z = ps[p].pos.Z - 12;
				else newtspr->pos.Z = h->spr.pos.Z - 51;
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
			}

			if (!h->GetOwner())
			{
				if (hw_models && md_tilehasmodel(h->spr.picnum, h->spr.pal) >= 0) 
				{
					k = 0;
					t->cstat &= ~CSTAT_SPRITE_XFLIP;
				} else
				{
					k = (((h->int_ang() + 3072 + 128 - a) & 2047) >> 8) & 7;
					if (k > 4)
					{
						k = 8 - k;
						t->cstat |= CSTAT_SPRITE_XFLIP;
					}
					else t->cstat &= ~CSTAT_SPRITE_XFLIP;
				}

				if (t->sectp->lotag == 2) k += 1795 - 1405;
				else if ((h->floorz - h->spr.pos.Z) > 64) k += 60;

				t->picnum += k;
				t->pal = ps[p].palookup;

				goto PALONLY;
			}

			if (ps[p].on_crane == nullptr && (h->sector()->lotag & 0x7ff) != 1)
			{
				double v = h->spr.pos.Z - ps[p].GetActor()->floorz + 3;
				if (v > 4 && h->spr.yrepeat > 32 && h->spr.extra > 0)
					h->spr.yoffset = (int8_t)(v * (1 / REPEAT_SCALE) / h->spr.yrepeat);
				else h->spr.yoffset = 0;
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
						t->ownerActor = nullptr;
						t->xrepeat = t->yrepeat = 0;
						continue;
					}

		PALONLY:

			if (sectp->floorpal)
				copyfloorpal(t, sectp);

			if (!h->GetOwner()) continue;

			if (t->pos.Z > h->floorz && t->xrepeat < 32)
				t->pos.Z = h->floorz;

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
				k = (((h->int_ang() + 3072 + 128 - a) & 2047) / 170);
				if (k > 6)
				{
					k = 12 - k;
					t->cstat |= CSTAT_SPRITE_XFLIP;
				}
				else t->cstat &= ~CSTAT_SPRITE_XFLIP;

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
				k = (((h->int_ang() + 3072 + 128 - a) & 2047) / 170);
				if (k > 6)
				{
					k = 12 - k;
					t->cstat |= CSTAT_SPRITE_XFLIP;
				}
				else t->cstat &= ~CSTAT_SPRITE_XFLIP;

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
			[[fallthrough]];
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

			if (h->sector()->shadedsector == 1)
				t->shade = 16;
			[[fallthrough]];

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

			if (t->picnum == SCRAP1 && h->spr.yvel >= 0)
				t->picnum = h->spr.yvel;
			else t->picnum += h->temp_data[0];

			if (sectp->floorpal)
				copyfloorpal(t, sectp);
			break;

		case WATERBUBBLE:
			if (t->sectp->floorpicnum == FLOORSLIME)
			{
				t->pal = 7;
				break;
			}
			[[fallthrough]];
		default:
		default_case:

			if (sectp->floorpal)
				copyfloorpal(t, sectp);
			break;
		}

		if (gs.actorinfo[h->spr.picnum].scriptaddress && (t->cstat & CSTAT_SPRITE_ALIGNMENT_MASK) != CSTAT_SPRITE_ALIGNMENT_SLAB)
		{
			if (t4)
			{
				l = ScriptCode[t4 + 2];

				if (hw_models && md_tilehasmodel(h->spr.picnum, h->spr.pal) >= 0) 
				{
					k = 0;
					t->cstat &= ~CSTAT_SPRITE_XFLIP;
				} 
				else switch (l) 
				{
				case 2:
					k = (((h->int_ang() + 3072 + 128 - a) & 2047) >> 8) & 1;
					break;

				case 3:
				case 4:
					k = (((h->int_ang() + 3072 + 128 - a) & 2047) >> 7) & 7;
					if (k > 3)
					{
						t->cstat |= CSTAT_SPRITE_XFLIP;
						k = 7 - k;
					}
					else t->cstat &= ~CSTAT_SPRITE_XFLIP;
					break;

				case 5:
					k = getangle(h->int_pos().X - x, h->int_pos().Y - y);
					k = (((h->int_ang() + 3072 + 128 - k) & 2047) >> 8) & 7;
					if (k > 4)
					{
						k = 8 - k;
						t->cstat |= CSTAT_SPRITE_XFLIP;
					}
					else t->cstat &= ~CSTAT_SPRITE_XFLIP;
					break;
				case 7:
					k = getangle(h->int_pos().X - x, h->int_pos().Y - y);
					k = (((h->int_ang() + 3072 + 128 - k) & 2047) / 170);
					if (k > 6)
					{
						k = 12 - k;
						t->cstat |= CSTAT_SPRITE_XFLIP;
					}
					else t->cstat &= ~CSTAT_SPRITE_XFLIP;
					break;
				case 8:
					k = (((h->int_ang() + 3072 + 128 - a) & 2047) >> 8) & 7;
					t->cstat &= ~CSTAT_SPRITE_XFLIP;
					break;
				default:
					bg = badguy(h);
					if (bg && h->spr.statnum == 2 && h->spr.extra > 0)
					{
						k = getangle(h->int_pos().X - x, h->int_pos().Y - y);
						k = (((h->int_ang() + 3072 + 128 - k) & 2047) >> 8) & 7;
						if (k > 4)
						{
							k = 8 - k;
							t->cstat |= CSTAT_SPRITE_XFLIP;
						}
						else t->cstat &= ~CSTAT_SPRITE_XFLIP;
						break;
					}
					k = 0;
					bg = 0;
					break;
				}

				t->picnum += k + ScriptCode[t4] + l * t3;

				if (l > 0)
				{
					while (t->picnum >= 0 && t->picnum < MAXTILES && !tileGetTexture(t->picnum)->isValid())
						t->picnum -= l;       //Hack, for actors 
				}

				if (t->picnum < 0 || t->picnum >= MAXTILES)
				{
					t->picnum = 0;
					t->xrepeat = t->yrepeat = 0;
				}

				if (h->dispicnum >= 0)
					h->dispicnum = t->picnum;
			}
			else if (display_mirror == 1)
				t->cstat |= CSTAT_SPRITE_XFLIP;
		}

		if (!isRRRA() && h->spr.picnum == SBMOVE)
			t->shade = -127;

		if (h->spr.statnum == STAT_DUMMYPLAYER || badguy(h) || (h->spr.picnum == APLAYER && h->GetOwner()))
		{
			if ((h->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) == 0 && t->statnum != 99)
			{
				if (h->spr.picnum != EXPLOSION2 && h->spr.picnum != DOMELITE && h->spr.picnum != TORNADO && h->spr.picnum != EXPLOSION3 && (h->spr.picnum != SBMOVE || isRRRA()))
				{
					if (r_shadows && !(h->spr.cstat2 & CSTAT2_SPRITE_NOSHADOW))
					{
						double floorz;

						if (isRRRA() && sectp->lotag == 160) continue;
						if ((sectp->lotag & 0xff) > 2 || h->spr.statnum == 4 || h->spr.statnum == 5 || h->spr.picnum == DRONE)
							floorz = sectp->floorz;
						else
							floorz = h->floorz;

						if (h->spr.pos.Z - floorz < 8 && ps[screenpeek].pos.Z < floorz)
						{
								auto shadowspr = tsprites.newTSprite();
								*shadowspr = *t;

								shadowspr->statnum = 99;

								shadowspr->yrepeat = (t->yrepeat >> 3);
								if (t->yrepeat < 4) t->yrepeat = 4;
								shadowspr->shade = 127;
								shadowspr->cstat |= CSTAT_SPRITE_TRANSLUCENT;

								shadowspr->pos.Z = floorz;
								shadowspr->pal = 4;

								if (hw_models && md_tilehasmodel(t->picnum, t->pal) >= 0)
								{
									shadowspr->yrepeat = 0;
									// 512:trans reverse
									//1024:tell MD2SPRITE.C to use Z-buffer hacks to hide overdraw issues
									shadowspr->clipdist |= TSPR_FLAGS_MDHACK;
									shadowspr->cstat |= CSTAT_SPRITE_TRANS_FLIP;
								}
								else
								{
									// Alter the shadow's position so that it appears behind the sprite itself.
									auto look = VecToAngle(shadowspr->pos.XY() - ps[screenpeek].pos.XY());
									shadowspr->pos.X += look.Cos() * 2;
									shadowspr->pos.Y += look.Sin() * 2;
								}
						}
					}
				}
			}
		}



		switch (h->spr.picnum)
		{
		case RPG2:
		case RRTILE1790:
			if (!isRRRA()) break;
			[[fallthrough]];
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
			t->cstat |= CSTAT_SPRITE_INVISIBLE;
			h->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
			break;
		case DESTRUCTO:
			t->cstat |= CSTAT_SPRITE_INVISIBLE;
			break;
		case FIRE:
		case BURNING:
			if (!OwnerAc || !actorflag(OwnerAc, SFLAG_NOFLOORFIRE))
				t->pos.Z = t->sectp->floorz;
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
			[[fallthrough]];
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

			k = (((t->int_ang() + 3072 + 128 - a) & 2047) >> 8) & 7;
			if (k > 4)
			{
				k = 8 - k;
				t->cstat |= CSTAT_SPRITE_XFLIP;
			}
			else t->cstat &= ~CSTAT_SPRITE_XFLIP;

			t->picnum = h->spr.picnum + k + ((h->temp_data[0] < 4) * 5);
			if (OwnerAc) t->shade = OwnerAc->spr.shade;
			break;
		case MUD:
			t->picnum = MUD + t1;
			break;
		case WATERSPLASH2:
			t->picnum = WATERSPLASH2 + t1;
			break;
		case REACTOR2:
			t->picnum = h->spr.picnum + h->temp_data[2];
			break;
		case SHELL:
			t->picnum = h->spr.picnum + (h->temp_data[0] & 1);
			[[fallthrough]];
		case SHOTGUNSHELL:
			t->cstat |= CSTAT_SPRITE_XFLIP | CSTAT_SPRITE_YFLIP;
			if (h->temp_data[0] > 1) t->cstat &= ~CSTAT_SPRITE_XFLIP;
			if (h->temp_data[0] > 2) t->cstat &= ~CSTAT_SPRITE_XFLIP | CSTAT_SPRITE_YFLIP;
			break;
		case FRAMEEFFECT1:
			if (OwnerAc && OwnerAc->spr.statnum < MAXSTATUS)
			{
				if (OwnerAc->spr.picnum == APLAYER)
					if (ud.cameraactor == nullptr)
						if (screenpeek == OwnerAc->spr.yvel && display_mirror == 0)
						{
							t->ownerActor = nullptr;
							break;
						}
				if ((OwnerAc->spr.cstat & CSTAT_SPRITE_INVISIBLE) == 0)
				{
					if (OwnerAc->spr.picnum == APLAYER)
						t->picnum = 1554;
					else
						t->picnum = OwnerAc->dispicnum;
					t->pal = OwnerAc->spr.pal;
					t->shade = OwnerAc->spr.shade;
					t->angle = OwnerAc->spr.angle;
					t->cstat = CSTAT_SPRITE_TRANSLUCENT | OwnerAc->spr.cstat;
				}
			}
			break;

		case CAMERA1:
		case RAT:
			k = (((t->int_ang() + 3072 + 128 - a) & 2047) >> 8) & 7;
			if (k > 4)
			{
				k = 8 - k;
				t->cstat |= CSTAT_SPRITE_XFLIP;
			}
			else t->cstat &= ~CSTAT_SPRITE_XFLIP;
			t->picnum = h->spr.picnum + k;
			break;
		}

		h->dispicnum = t->picnum;
		if (t->sectp->floorpicnum == MIRROR)
			t->xrepeat = t->yrepeat = 0;
	}
}
END_DUKE_NS
