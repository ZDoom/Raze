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
#include "models/modeldata.h"

BEGIN_DUKE_NS


void animatesprites_r(tspriteArray& tsprites, const DVector2& viewVec, DAngle viewang, double interpfrac)
{
	DAngle kang;
	int k, p;
	int l, t1, t3, t4;
	tspritetype* t;
	DDukeActor* h;

	int bg = 0;

	for (unsigned j = 0; j < tsprites.Size(); j++)
	{
		t = tsprites.get(j);
		h = static_cast<DDukeActor*>(t->ownerActor);

		if (!actorflag(h, SFLAG2_FORCESECTORSHADE) && ((t->cstat & CSTAT_SPRITE_ALIGNMENT_WALL)) || (badguypic(t->picnum) && t->extra > 0) || t->statnum == STAT_PLAYER)
		{
			if (h->sector()->shadedsector == 1 && h->spr.statnum != 1)
			{
				t->shade = 16;
			}
			continue;
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
			if (t->lotag == SE_27_DEMO_CAM && ud.recstat == 1)
			{
				t->picnum = 11 + ((PlayClock >> 3) & 1);
				t->cstat |= CSTAT_SPRITE_YCENTER;
			}
			else
				t->scale = DVector2(0, 0);
			break;
		default:
			break;
		}

		if (t->statnum == 99) continue;
		auto pp = &ps[h->PlayerIndex()];
		if (h->spr.statnum != STAT_ACTOR && h->isPlayer() && pp->newOwner == nullptr && h->GetOwner())
		{
			t->pos = h->interpolatedpos(interpfrac);
			h->spr.scale = DVector2(0.375, 0.265625);
		}
		else if (!actorflag(h, SFLAG_NOINTERPOLATE))
		{
			t->pos = h->interpolatedpos(interpfrac);
		}

		if (actorflag(h, SFLAG2_INTERPOLATEANGLE))
		{
			t->Angles.Yaw = h->interpolatedyaw(interpfrac);
		}


		auto sectp = h->sector();
		if (h->GetClass() != RUNTIME_CLASS(DDukeActor))
		{
			bool res = CallAnimate(h, t);
			// some actors have 4, some 6 rotation frames - in true Build fashion there's no pointers what to do here without flagging it.
			if (actorflag(h, SFLAG2_ALWAYSROTATE1) || (t->clipdist & TSPR_ROTATE8FRAMES))
				applyRotation1(h, t, viewang);
			else if (actorflag(h, SFLAG2_ALWAYSROTATE2) || (t->clipdist & TSPR_ROTATE12FRAMES))
				applyRotation2(h, t, viewang);
			if (sectp->floorpal && !actorflag(h, SFLAG2_NOFLOORPAL))
				copyfloorpal(t, sectp);

			if (res)
			{
				if (h->dispicnum >= 0)
					h->dispicnum = t->picnum;
				continue;
			}
		}

		t1 = h->temp_data[1];
		t3 = h->temp_data[3];
		t4 = h->temp_data[4];

		switch (h->spr.picnum)
		{
		case DUKELYINGDEAD:
			h->spr.scale = DVector2(0.375, 0.265625);
			if (h->spr.extra > 0)
				t->pos.Z += 6;
			break;
		case POWDERKEG:
			continue;
		case BURNING:
			if (OwnerAc && OwnerAc->spr.statnum == STAT_PLAYER)
			{
				if (display_mirror == 0 && OwnerAc->PlayerIndex() == screenpeek && ps[OwnerAc->PlayerIndex()].over_shoulder_on == 0)
					t->scale = DVector2(0, 0);
				else
				{
					t->Angles.Yaw = (viewVec - t->pos.XY()).Angle();
					t->pos.XY() = OwnerAc->spr.pos.XY() + t->Angles.Yaw.ToVector();
				}
			}
			break;

		case ATOMICHEALTH:
			t->pos.Z -= 4;
			break;
		case CRYSTALAMMO:
			t->shade = int(BobVal(PlayClock << 4) * 16);
			break;
		case APLAYER:

			p = h->PlayerIndex();

			if (t->pal == 1) t->pos.Z -= 18;

			if (ps[p].over_shoulder_on > 0 && ps[p].newOwner == nullptr)
			{
				t->cstat |= CSTAT_SPRITE_TRANSLUCENT;
#if 0 // multiplayer only
				if (screenpeek == myconnectindex && numplayers >= 2)
				{
					t->pos = interpolatedvalue(omypos, mypos, interpfrac).plusZ(gs_playerheight);
					t->angle = interpolatedvalue(omyang, myang, interpfrac);
					t->sector = mycursectnum;
				}
#endif
			}

			if ((display_mirror == 1 || screenpeek != p || !h->GetOwner()) && ud.multimode > 1 && cl_showweapon && ps[p].GetActor()->spr.extra > 0 && ps[p].curr_weapon > 0)
			{
				auto newtspr = tsprites.newTSprite();
				*newtspr = *t;

				newtspr->statnum = 99;

				newtspr->scale.Y = (max(t->scale.Y * 0.125, 0.0625));

				newtspr->shade = t->shade;
				newtspr->cstat = 0;

				switch (ps[p].curr_weapon)
				{
				case PISTOL_WEAPON:      newtspr->picnum = FIRSTGUNSPRITE;       break;
				case SHOTGUN_WEAPON:     newtspr->picnum = SHOTGUNSPRITE;        break;
				case RIFLEGUN_WEAPON:    newtspr->picnum = RIFLEGUNSPRITE;       break;
				case CROSSBOW_WEAPON:         newtspr->picnum = CROSSBOWSPRITE;            break;
				case CHICKEN_WEAPON:        newtspr->picnum = CROSSBOWSPRITE; break;
				case THROWINGDYNAMITE_WEAPON:
				case DYNAMITE_WEAPON:    newtspr->picnum = DYNAMITE;           break;
				case POWDERKEG_WEAPON:    newtspr->picnum = POWDERKEG;       break;
				case BOWLING_WEAPON:     newtspr->picnum = BOWLINGBALLSPRITE;                 break;
				case THROWSAW_WEAPON:    newtspr->picnum = RIPSAWSPRITE;          break;
				case BUZZSAW_WEAPON:        newtspr->picnum = RIPSAWSPRITE;          break;
				case ALIENBLASTER_WEAPON:      newtspr->picnum = ALIENBLASTERSPRITE;     break;
				case TIT_WEAPON:  newtspr->picnum = TITSPRITE;         break;
				}

				if (h->GetOwner()) newtspr->pos.Z = ps[p].GetActor()->getOffsetZ() - 12;
				else newtspr->pos.Z = h->spr.pos.Z - 51;
				if (ps[p].curr_weapon == HANDBOMB_WEAPON)
				{
					newtspr->scale = DVector2(0.15625, 0.15625);
				}
				else if (ps[p].OnMotorcycle || ps[p].OnBoat)
				{
					newtspr->scale = DVector2(0, 0);
				}
				else
				{
					newtspr->scale = DVector2(0.25, 0.25);
				}
				newtspr->pal = 0;
			}

			if (!h->GetOwner())
			{
				if (hw_models && modelManager.CheckModel(h->spr.picnum, h->spr.pal)) 
				{
					k = 0;
					t->cstat &= ~CSTAT_SPRITE_XFLIP;
				} else
				{
					k = angletorotation1(h->spr.Angles.Yaw, viewang);
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
				if (v > 4 && h->spr.scale.Y > 0.5 && h->spr.extra > 0)
					h->spr.yoffset = (int8_t)(v / h->spr.scale.Y);
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
						t->scale = DVector2(0, 0);
						continue;
					}

		PALONLY:

			if (sectp->floorpal)
				copyfloorpal(t, sectp);

			if (!h->GetOwner()) continue;

			if (t->pos.Z > h->floorz && t->scale.X < 0.5)
				t->pos.Z = h->floorz;

			if (ps[p].OnMotorcycle && p == screenpeek)
			{
				t->picnum = RRTILE7219;
				t->scale = DVector2(0.28125, 0.28125);
				t4 = 0;
				t3 = 0;
				t1 = 0;
			}
			else if (ps[p].OnMotorcycle)
			{
				k = angletorotation2(h->spr.Angles.Yaw, viewang);
				if (k > 6)
				{
					k = 12 - k;
					t->cstat |= CSTAT_SPRITE_XFLIP;
				}
				else t->cstat &= ~CSTAT_SPRITE_XFLIP;

				t->picnum = RRTILE7213 + k;
				t->scale = DVector2(0.28125, 0.28125);
				t4 = 0;
				t3 = 0;
				t1 = 0;
			}
			else if (ps[p].OnBoat && p == screenpeek)
			{
				t->picnum = RRTILE7190;
				t->scale = DVector2(0.5, 0.5);
				t4 = 0;
				t3 = 0;
				t1 = 0;
			}
			else if (ps[p].OnBoat)
			{
				k = angletorotation2(h->spr.Angles.Yaw, viewang);

				if (k > 6)
				{
					k = 12 - k;
					t->cstat |= CSTAT_SPRITE_XFLIP;
				}
				else t->cstat &= ~CSTAT_SPRITE_XFLIP;

				t->picnum = RRTILE7184 + k;
				t->scale = DVector2(0.5, 0.5);
				t4 = 0;
				t3 = 0;
				t1 = 0;
			}

			break;

		case WATERBUBBLE:
			if (t->sectp->floorpicnum == FLOORSLIME)
			{
				t->pal = 7;
				break;
			}
			[[fallthrough]];
		default:

			if (sectp->floorpal)
				copyfloorpal(t, sectp);
			break;
		}

		if (gs.actorinfo[h->spr.picnum].scriptaddress && (t->cstat & CSTAT_SPRITE_ALIGNMENT_MASK) != CSTAT_SPRITE_ALIGNMENT_SLAB)
		{
			if (t4)
			{
				l = ScriptCode[t4 + 2];

				if (hw_models && modelManager.CheckModel(h->spr.picnum, h->spr.pal)) 
				{
					k = 0;
					t->cstat &= ~CSTAT_SPRITE_XFLIP;
				} 
				else switch (l) 
				{
				case 2:
					k = angletorotation1(h->spr.Angles.Yaw, viewang, 8, 1);
					break;

				case 3:
				case 4:
					k = angletorotation1(h->spr.Angles.Yaw, viewang, 7);
					if (k > 3)
					{
						t->cstat |= CSTAT_SPRITE_XFLIP;
						k = 7 - k;
					}
					else t->cstat &= ~CSTAT_SPRITE_XFLIP;
					break;

				case 5:
					kang = (h->spr.pos - viewVec).Angle();
					k = angletorotation1(h->spr.Angles.Yaw, kang);
					if (k > 4)
					{
						k = 8 - k;
						t->cstat |= CSTAT_SPRITE_XFLIP;
					}
					else t->cstat &= ~CSTAT_SPRITE_XFLIP;
					break;
				case 7:
					kang = (h->spr.pos - viewVec).Angle();
					k = angletorotation2(h->spr.Angles.Yaw, kang);
					if (k > 6)
					{
						k = 12 - k;
						t->cstat |= CSTAT_SPRITE_XFLIP;
					}
					else t->cstat &= ~CSTAT_SPRITE_XFLIP;
					break;
				case 8:
					k = angletorotation1(h->spr.Angles.Yaw, viewang);
					t->cstat &= ~CSTAT_SPRITE_XFLIP;
					break;
				default:
					bg = badguy(h);
					if (bg && h->spr.statnum == 2 && h->spr.extra > 0)
					{
						kang = (h->spr.pos - viewVec).Angle();
						k = angletorotation1(h->spr.Angles.Yaw, kang);
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
					t->scale = DVector2(0, 0);
				}

				if (h->dispicnum >= 0)
					h->dispicnum = t->picnum;
			}
			else if (display_mirror == 1)
				t->cstat |= CSTAT_SPRITE_XFLIP;
		}

		if (!isRRRA() && h->spr.picnum == SBMOVE)
			t->shade = -127;

		if (h->spr.statnum == STAT_DUMMYPLAYER || badguy(h) || (h->isPlayer() && h->GetOwner()))
		{
			drawshadows(tsprites, t, h);
		}



		switch (h->spr.picnum)
		{
			if (!isRRRA()) break;
			[[fallthrough]];
		case EXPLOSION2:
		case ATOMICHEALTH:
		case CHAINGUN:
		case EXPLOSION3:
			if (t->picnum == EXPLOSION2)
			{
				ps[screenpeek].visibility = -127;
				lastvisinc = PlayClock + 32;
				t->pal = 0;
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

			k = angletorotation1(t->Angles.Yaw, viewang);
			if (k > 4)
			{
				k = 8 - k;
				t->cstat |= CSTAT_SPRITE_XFLIP;
			}
			else t->cstat &= ~CSTAT_SPRITE_XFLIP;

			t->picnum = h->spr.picnum + k + ((h->temp_data[0] < 4) * 5);
			if (OwnerAc) t->shade = OwnerAc->spr.shade;
			break;
		}

		h->dispicnum = t->picnum;
		if (t->sectp->floorpicnum == MIRROR)
			t->scale = DVector2(0, 0);
	}
}
END_DUKE_NS
