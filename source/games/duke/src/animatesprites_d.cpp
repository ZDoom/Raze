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

void animatesprites_d(tspriteArray& tsprites, int x, int y, int a, int smoothratio)
{
	DVector2 viewVec(x * inttoworld, y * inttoworld);
	int k, p;
	int l, t1, t3, t4;
	tspritetype* t;
	DDukeActor* h;

	for (unsigned j = 0; j < tsprites.Size(); j++)
	{
		t = tsprites.get(j);
		h = static_cast<DDukeActor*>(t->ownerActor);

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
				t->cstat &= ~CSTAT_SPRITE_XFLIP;
				break;
			}

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
			if (((t->cstat & CSTAT_SPRITE_ALIGNMENT_WALL)) || (badguypic(t->picnum) && t->extra > 0) || t->statnum == STAT_PLAYER)
				continue;
		}

		if (t->sectp != nullptr)
			t->shade = clamp<int>(t->sectp->ceilingstat & CSTAT_SECTOR_SKY ? t->sectp->ceilingshade : t->sectp->floorshade, -127, 127);
	}


	//Between drawrooms() and drawmasks() is the perfect time to animate sprites
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
		case NATURALLIGHTNING:
			t->shade = -127;
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
			t->pos.Z = interpolatedvaluef(pp->opos.Z, pp->pos.Z, smoothratio) + gs.playerheight;
		}
		else if (!actorflag(h, SFLAG_NOINTERPOLATE))
		{
			t->pos = h->interpolatedvec3(smoothratio / 65536.);
		}

		auto sectp = h->sector();
		t1 = h->temp_data[1];
		t3 = h->temp_data[3];
		t4 = h->temp_data[4];

		switch (h->spr.picnum)
		{
		case DUKELYINGDEAD:
			t->pos.Z += 24;
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
		case FORCESPHERE:
			if (t->statnum == STAT_MISC && OwnerAc)
			{
				int sqa = getangle( OwnerAc->spr.pos.XY() - ps[screenpeek].pos.XY());
				int sqb = getangle(OwnerAc->spr.pos.XY() - t->pos.XY());

				if (abs(getincangle(sqa, sqb)) > 512)
					if (ldist(OwnerAc, t) < ldist(ps[screenpeek].GetActor(), OwnerAc))
						t->xrepeat = t->yrepeat = 0;
			}
			continue;
		case BURNING:
		case BURNING2:
			if (OwnerAc && OwnerAc->spr.statnum == STAT_PLAYER)
			{
				if (display_mirror == 0 && OwnerAc->spr.yvel == screenpeek && ps[screenpeek].over_shoulder_on == 0)
					t->xrepeat = 0;
				else
				{
					t->angle = VecToAngle(viewVec - t->pos.XY());
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
			continue;
		case VIEWSCREEN:
		case VIEWSCREEN2:
			if (camsprite != nullptr && h->GetHitOwner() && h->GetHitOwner()->temp_data[0] == 1)
			{
				t->picnum = STATIC;
				t->cstat |= randomFlip();
				t->xrepeat += 8;
				t->yrepeat += 8;
			}
			else if (camsprite && camsprite == h->GetHitOwner())
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
			if (hw_models && md_tilehasmodel(h->spr.picnum, h->spr.pal) >= 0) 
			{
				t->cstat &= ~CSTAT_SPRITE_XFLIP;
				break;
			}

			k = getangle(h->spr.pos - viewVec);
			k = (((h->int_ang() + 3072 + 128 - k) & 2047) / 170);
			if (k > 6)
			{
				k = 12 - k;
				t->cstat |= CSTAT_SPRITE_XFLIP;
			}
			else t->cstat &= ~CSTAT_SPRITE_XFLIP;
			t->picnum = RPG + k;
			break;

		case RECON:
			if (hw_models && md_tilehasmodel(h->spr.picnum, h->spr.pal) >= 0) 
			{
				t->cstat &= ~CSTAT_SPRITE_XFLIP;
				break;
			}

			k = getangle(h->spr.pos - viewVec);
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
					t->z = interpolatedvalue(omyz, myz, smoothratio) + gs_playerheight;
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
						newtspr->picnum = FLAMETHROWERSPRITE;   
					break;
				case DEVISTATOR_WEAPON:  newtspr->picnum = DEVISTATORSPRITE;     break;
				}

				if (h->GetOwner()) newtspr->pos.Z = ps[p].pos.Z - 12;
				else newtspr->pos.Z = h->spr.pos.Z - 51;
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
			}

			if (!h->GetOwner())
			{
				if (hw_models && md_tilehasmodel(h->spr.picnum, h->spr.pal) >= 0) 
				{
					k = 0;
					t->cstat &= ~CSTAT_SPRITE_XFLIP;
				}
				else
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
					h->spr.yoffset = (int8_t)(v * (1/REPEAT_SCALE) / h->spr.yrepeat);
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

			if (h->attackertype == BLIMP && t->picnum == SCRAP1 && h->spr.yvel >= 0)
				t->picnum = h->spr.yvel;
			else t->picnum += h->temp_data[0];
			t->shade -= 6;

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

			if (sectp->floorpal && !actorflag(h, SFLAG2_NOFLOORPAL))
				copyfloorpal(t, sectp);
			break;
		}

		if (gs.actorinfo[h->spr.picnum].scriptaddress && !actorflag(h, SFLAG2_DONTANIMATE))
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
					k = getangle(h->spr.pos - viewVec);
					k = (((h->int_ang() + 3072 + 128 - k) & 2047) >> 8) & 7;
					if (k > 4)
					{
						k = 8 - k;
						t->cstat |= CSTAT_SPRITE_XFLIP;
					}
					else t->cstat &= ~CSTAT_SPRITE_XFLIP;
					break;
				case 7:
					k = getangle(h->spr.pos - viewVec);
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
					k = 0;
					break;
				}

				t->picnum += k + ScriptCode[t4] + l * t3;

				if (l > 0)
				{
					while (t->picnum >= 0 && t->picnum < MAXTILES && !tileGetTexture(t->picnum)->isValid())
						t->picnum -= l;       //Hack, for actors 
				}

				if (h->dispicnum >= 0)
					h->dispicnum = t->picnum;
			}
			else if (display_mirror == 1)
				t->cstat |= CSTAT_SPRITE_XFLIP;
		}

		if (h->spr.statnum == STAT_DUMMYPLAYER || badguy(h) || (h->spr.picnum == APLAYER && h->GetOwner()))
		{
			if (t->statnum != 99 && h->spr.picnum != EXPLOSION2 && h->spr.picnum != HANGLIGHT && h->spr.picnum != DOMELITE)
			{
				if (h->spr.picnum != HOTMEAT)
				{
					if (r_shadows && !(h->spr.cstat2 & CSTAT2_SPRITE_NOSHADOW))
					{
						double floorz;

						if ((sectp->lotag & 0xff) > 2 || h->spr.statnum == 4 || h->spr.statnum == 5 || h->spr.picnum == DRONE || h->spr.picnum == COMMANDER)
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

					if (ps[screenpeek].heat_amount > 0 && ps[screenpeek].heat_on)
					{
						t->pal = 6;
						t->shade = 0;
					}
				}
			}
		}

		switch (h->spr.picnum)
		{
		case LASERLINE:
			if (!OwnerAc) break;
			if (t->sectp->lotag == 2) t->pal = 8;
			t->set_int_z(OwnerAc->int_pos().Z - (3 << 8));
			if (gs.lasermode == 2 && ps[screenpeek].heat_on == 0)
				t->yrepeat = 0;
			t->shade = -127;
			break;
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
			t->cstat |= CSTAT_SPRITE_YCENTER;
			[[fallthrough]];
		case BURNING:
		case BURNING2:
			if (!OwnerAc) break;
			if (!actorflag(OwnerAc, SFLAG_NOFLOORFIRE))
				t->pos.Z = t->sectp->floorz;
			t->shade = -127;
			break;
		case COOLEXPLOSION1:
			t->shade = -127;
			t->picnum += (h->spr.shade >> 1);
			break;
		case PLAYERONWATER:
			if (hw_models && md_tilehasmodel(h->spr.picnum, h->spr.pal) >= 0) 
			{
				k = 0;
				t->cstat &= ~CSTAT_SPRITE_XFLIP;
			}
			else
			{
			k = (((t->int_ang() + 3072 + 128 - a) & 2047) >> 8) & 7;
			if (k > 4)
			{
				k = 8 - k;
				t->cstat |= CSTAT_SPRITE_XFLIP;
			}
			else t->cstat &= ~CSTAT_SPRITE_XFLIP;
		}

			t->picnum = h->spr.picnum + k + ((h->temp_data[0] < 4) * 5);
			if (OwnerAc) t->shade = OwnerAc->spr.shade;

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
			t->cstat |= (CSTAT_SPRITE_XFLIP | CSTAT_SPRITE_YFLIP);
			if (h->temp_data[0] > 1) t->cstat &= ~CSTAT_SPRITE_XFLIP;
			if (h->temp_data[0] > 2) t->cstat &= ~(CSTAT_SPRITE_XFLIP | CSTAT_SPRITE_YFLIP);
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
			if (hw_models && md_tilehasmodel(h->spr.picnum, h->spr.pal) >= 0) 
			{
				t->cstat &= ~CSTAT_SPRITE_XFLIP;
				break;
			}

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
