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
#include "models/modeldata.h"

EXTERN_CVAR(Bool, wt_commentary)

BEGIN_DUKE_NS

void animatesprites_d(tspriteArray& tsprites, const DVector2& viewVec, DAngle viewang, double interpfrac)
{
	int p;
	tspritetype* t;
	DDukeActor* h;

	for (unsigned j = 0; j < tsprites.Size(); j++)
	{
		t = tsprites.get(j);
		h = static_cast<DDukeActor*>(t->ownerActor);

		if (!(h->flags2 & SFLAG2_FORCESECTORSHADE) && ((t->cstat & CSTAT_SPRITE_ALIGNMENT_WALL)) || (badguy(static_cast<DDukeActor*>(t->ownerActor)) && t->extra > 0) || t->statnum == STAT_PLAYER)
		{
			if (h->sector()->shadedsector == 1 && h->spr.statnum != STAT_ACTOR)
			{
				t->shade = 16;
			}
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

		if (iseffector(h))
		{
			if (t->lotag == SE_27_DEMO_CAM && ud.recstat == 1)
			{
				t->picnum = 11 + ((PlayClock >> 3) & 1);
				t->cstat |= CSTAT_SPRITE_YCENTER;
			}
			else
				t->scale = DVector2(0, 0);
			break;
		}

		if (t->statnum == STAT_TEMP) continue;
		auto pp = &ps[h->PlayerIndex()];
		if ((h->spr.statnum != STAT_ACTOR && h->isPlayer() && pp->newOwner == nullptr && h->GetOwner()) || !(h->flags1 & SFLAG_NOINTERPOLATE))
		{
			t->pos = h->interpolatedpos(interpfrac);
			t->Angles.Yaw = h->interpolatedyaw(interpfrac);
		}


		auto sectp = h->sector();
		bool res = CallAnimate(h, t);
		// some actors have 4, some 6 rotation frames - in true Build fashion there's no pointers what to do here without flagging it.
		if ((h->flags2 & SFLAG2_ALWAYSROTATE1) || (t->clipdist & TSPR_ROTATE8FRAMES))
			applyRotation1(h, t, viewang);
		else if ((h->flags2 & SFLAG2_ALWAYSROTATE2) || (t->clipdist & TSPR_ROTATE12FRAMES))
			applyRotation2(h, t, viewang);
		if (sectp->floorpal && !(h->flags2 & SFLAG2_NOFLOORPAL) && !(t->clipdist & TSPR_NOFLOORPAL))
			copyfloorpal(t, sectp);

		if (res)
		{
			if (h->dispicnum >= 0)
				h->dispicnum = t->picnum;
			continue;
		}

		if (h->spr.picnum == DTILE_APLAYER)
		{
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

				newtspr->statnum = STAT_TEMP;

				newtspr->scale.Y = (max(t->scale.Y * 0.125, 0.0625));

				newtspr->shade = t->shade;
				newtspr->cstat = 0;

				switch (ps[p].curr_weapon)
				{
				case PISTOL_WEAPON:      newtspr->picnum = DTILE_FIRSTGUNSPRITE;       break;
				case SHOTGUN_WEAPON:     newtspr->picnum = DTILE_SHOTGUNSPRITE;        break;
				case CHAINGUN_WEAPON:    newtspr->picnum = DTILE_CHAINGUNSPRITE;       break;
				case RPG_WEAPON:         newtspr->picnum = DTILE_RPGSPRITE;            break;
				case HANDREMOTE_WEAPON:
				case HANDBOMB_WEAPON:    newtspr->picnum = DTILE_HEAVYHBOMB;           break;
				case TRIPBOMB_WEAPON:    newtspr->picnum = DTILE_TRIPBOMBSPRITE;       break;
				case GROW_WEAPON:        newtspr->picnum = DTILE_GROWSPRITEICON;       break;
				case SHRINKER_WEAPON:    newtspr->picnum = DTILE_SHRINKERSPRITE;       break;
				case FREEZE_WEAPON:      newtspr->picnum = DTILE_FREEZESPRITE;         break;
				case FLAMETHROWER_WEAPON: //Twentieth Anniversary World Tour
					if (isWorldTour())
						newtspr->picnum = DTILE_FLAMETHROWERSPRITE;
					break;
				case DEVISTATOR_WEAPON:  newtspr->picnum = DTILE_DEVISTATORSPRITE;     break;
				}

				if (h->GetOwner()) newtspr->pos.Z = ps[p].GetActor()->getOffsetZ() - 12;
				else newtspr->pos.Z = h->spr.pos.Z - 51;
				if (ps[p].curr_weapon == HANDBOMB_WEAPON)
				{
					newtspr->scale = DVector2(0.15625, 0.15625);
				}
				else
				{
					newtspr->scale = DVector2(0.25, 0.25);
				}
				newtspr->pal = 0;
			}

			if (!h->GetOwner())
			{
				applyRotation1(h, t, viewang);

				if (t->sectp->lotag == ST_2_UNDERWATER) t->picnum += DTILE_APLAYERSWIMMING - DTILE_APLAYER;
				else if ((h->floorz - h->spr.pos.Z) > 64) t->picnum += DTILE_APLAYERJUMP - DTILE_APLAYER;

				t->pal = ps[p].palookup;
				continue;
			}
			if (ps[p].on_crane == nullptr && (h->sector()->lotag & 0x7ff) != 1)
			{
				double v = h->spr.pos.Z - ps[p].GetActor()->floorz + 3;
				if (v > 4 && h->spr.scale.Y > 0.5 && h->spr.extra > 0)
					h->spr.yoffset = (int8_t)(v / h->spr.scale.Y);
				else h->spr.yoffset = 0;
			}

			if (ud.cameraactor == nullptr && ps[p].newOwner == nullptr)
				if (h->GetOwner() && display_mirror == 0 && ps[p].over_shoulder_on == 0)
					if (ud.multimode < 2 || (ud.multimode > 1 && p == screenpeek))
					{
						t->ownerActor = nullptr;
						t->scale = DVector2(0, 0);
						continue;
					}


			if (sectp->floorpal)
				copyfloorpal(t, sectp);

			if (t->pos.Z > h->floorz && t->scale.X < 0.5)
				t->pos.Z = h->floorz;

		}

		applyanimations(t, h, viewVec, viewang);

		if (h->spr.statnum == STAT_DUMMYPLAYER || badguy(h) || (h->isPlayer() && h->GetOwner()))
		{
			drawshadows(tsprites, t, h);
			if (ps[screenpeek].heat_amount > 0 && ps[screenpeek].heat_on)
			{
				t->pal = 6;
				t->shade = 0;
			}
		}

		h->dispicnum = t->picnum;
		if (t->sectp->floortexture == mirrortex)
			t->scale = DVector2(0, 0);
	}
}


END_DUKE_NS
