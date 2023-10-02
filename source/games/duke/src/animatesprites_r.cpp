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
#include "prediction.h"
#include "dukeactor.h"
#include "gamefuncs.h"
#include "models/modeldata.h"

BEGIN_DUKE_NS


void animatesprites_r(tspriteArray& tsprites, const DVector2& viewVec, DAngle viewang, double interpfrac)
{
	int k, p;
	tspritetype* t;
	DDukeActor* h;

	int bg = 0;

	for (unsigned j = 0; j < tsprites.Size(); j++)
	{
		t = tsprites.get(j);
		h = static_cast<DDukeActor*>(t->ownerActor);

		if (!(h->flags2 & SFLAG2_FORCESECTORSHADE) && ((t->cstat & CSTAT_SPRITE_ALIGNMENT_WALL)) || badguy(static_cast<DDukeActor*>(t->ownerActor)) || t->statnum == STAT_PLAYER)
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

		if (iseffector(h))
		{
			if (t->lotag == SE_27_DEMO_CAM && ud.recstat == 1)
			{
				t->setspritetexture(TexMan.CheckForTexture("DEMOCAM", ETextureType::Any));
				t->cstat |= CSTAT_SPRITE_YCENTER;
			}
			else
				t->scale = DVector2(0, 0);
			continue;
		}

		if (t->statnum == STAT_TEMP) continue;
		auto pp = getPlayer(h->PlayerIndex());
		if (h->spr.statnum != STAT_ACTOR && h->isPlayer() && pp->newOwner == nullptr && h->GetOwner())
		{
			t->pos = h->interpolatedpos(interpfrac);
			t->Angles.Yaw = h->interpolatedyaw(interpfrac);
			h->spr.scale = DVector2(0.375, 0.265625);
		}
		else if (!(h->flags1 & SFLAG_NOINTERPOLATE))
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
			if (h->dispictex.isValid())
				h->dispictex = t->spritetexture();
			continue;
		}

		if (h->isPlayer())
		{
			p = h->PlayerIndex();

			if (t->pal == 1) t->pos.Z -= 18;

			if (getPlayer(p)->over_shoulder_on > 0 && getPlayer(p)->newOwner == nullptr)
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

			if ((display_mirror == 1 || screenpeek != p || !h->GetOwner()) && ud.multimode > 1 && cl_showweapon && getPlayer(p)->GetActor()->spr.extra > 0 && getPlayer(p)->curr_weapon > 0)
			{
				auto newtspr = tsprites.newTSprite();
				*newtspr = *t;

				newtspr->statnum = 99;

				newtspr->scale.Y = (max(t->scale.Y * 0.125, 0.0625));

				newtspr->shade = t->shade;
				newtspr->cstat = 0;

				const char* texname = nullptr;
				switch (getPlayer(p)->curr_weapon)
				{
				case PISTOL_WEAPON:           texname = "FIRSTGUNSPRITE";       break;
				case SHOTGUN_WEAPON:          texname = "SHOTGUNSPRITE";        break;
				case RIFLEGUN_WEAPON:         texname = "RIFLEGUNSPRITE";       break;
				case CROSSBOW_WEAPON:         texname = "CROSSBOWSPRITE";       break;
				case CHICKEN_WEAPON:          texname = "CROSSBOWSPRITE";       break;
				case THROWINGDYNAMITE_WEAPON:
				case DYNAMITE_WEAPON:         texname = "DYNAMITE";             break;
				case POWDERKEG_WEAPON:        texname = "POWDERKEG";            break;
				case BOWLING_WEAPON:          texname = "BOWLINGBALLSPRITE";    break;
				case THROWSAW_WEAPON:         texname = "RIPSAWSPRITE";         break;
				case BUZZSAW_WEAPON:          texname = "RIPSAWSPRITE";         break;
				case ALIENBLASTER_WEAPON:     texname = "ALIENBLASTERSPRITE";   break;
				case TIT_WEAPON:              texname = "TITSPRITE";            break;
				}
				t->setspritetexture(TexMan.CheckForTexture(texname, ETextureType::Any));

				if (h->GetOwner()) newtspr->pos.Z = getPlayer(p)->GetActor()->getOffsetZ() - 12;
				else newtspr->pos.Z = h->spr.pos.Z - 51;
				if (getPlayer(p)->curr_weapon == HANDBOMB_WEAPON)
				{
					newtspr->scale = DVector2(0.15625, 0.15625);
				}
				else if (getPlayer(p)->OnMotorcycle || getPlayer(p)->OnBoat)
				{
					newtspr->scale = DVector2(0, 0);
				}
				else
				{
					newtspr->scale = DVector2(0.25, 0.25);
				}
				newtspr->pal = 0;
			}

			if (getPlayer(p)->on_crane == nullptr && (h->sector()->lotag & 0x7ff) != 1)
			{
				double v = h->spr.pos.Z - getPlayer(p)->GetActor()->floorz + 3;
				if (v > 4 && h->spr.scale.Y > 0.5 && h->spr.extra > 0)
					h->spr.yoffset = (int8_t)(v / h->spr.scale.Y);
				else h->spr.yoffset = 0;
			}

			if (ud.cameraactor == nullptr && getPlayer(p)->newOwner == nullptr)
				if (h->GetOwner() && display_mirror == 0 && getPlayer(p)->over_shoulder_on == 0)
					if (ud.multimode < 2 || (ud.multimode > 1 && p == screenpeek))
					{
						t->ownerActor = nullptr;
						t->scale = DVector2(0, 0);
						continue;
					}

			if (t->pos.Z > h->floorz && t->scale.X < 0.5)
				t->pos.Z = h->floorz;

			if (getPlayer(p)->OnMotorcycle && p == screenpeek)
			{
				t->setspritetexture(TexMan.CheckForTexture("PLAYERONBIKEBACK", ETextureType::Any));
				t->scale = DVector2(0.28125, 0.28125);
				drawshadows(tsprites, t, h);
				continue;
			}
			else if (getPlayer(p)->OnMotorcycle)
			{
				t->setspritetexture(TexMan.CheckForTexture("PLAYERONBIKE", ETextureType::Any));
				applyRotation2(h, t, viewang);
				t->scale = DVector2(0.28125, 0.28125);
				drawshadows(tsprites, t, h);
				continue;
			}
			else if (getPlayer(p)->OnBoat && p == screenpeek)
			{
				t->setspritetexture(TexMan.CheckForTexture("PLAYERONBOATBACK", ETextureType::Any));
				t->scale = DVector2(0.5, 0.5);
				drawshadows(tsprites, t, h);
				continue;
			}
			else if (getPlayer(p)->OnBoat)
			{
				t->setspritetexture(TexMan.CheckForTexture("PLAYERONBOAT", ETextureType::Any));
				k = angletorotation2(h->spr.Angles.Yaw, viewang);
				applyRotation2(h, t, viewang);
				t->scale = DVector2(0.5, 0.5);
				drawshadows(tsprites, t, h);
				continue;
			}
		}
		applyanimations(t, h, viewVec, viewang);

		if (h->spr.statnum == STAT_DUMMYPLAYER || badguy(h) || (h->isPlayer() && h->GetOwner()))
		{
			drawshadows(tsprites, t, h);
		}

		h->dispictex = t->spritetexture();
		if (t->sectp->floortexture == mirrortex)
			t->scale = DVector2(0, 0);
	}
}
END_DUKE_NS
