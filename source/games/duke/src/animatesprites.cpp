//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2020 - Christoph Oelckers

This file is part of Raze

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
#include "dukeactor.h"
#include "gamefuncs.h"
#include "models/modeldata.h"

#include "buildtiles.h"

BEGIN_DUKE_NS

void drawshadows(tspriteArray& tsprites, tspritetype* t, DDukeActor* h)
{
	if (r_shadows && !(h->flags1 & SFLAG_NOSHADOW) && !(h->spr.cstat2 & CSTAT2_SPRITE_NOSHADOW))
	{
		auto sectp = t->sectp;
		double floorz;

		if ((sectp->lotag & 0xff) > 2 || h->spr.statnum == STAT_PROJECTILE || h->spr.statnum == STAT_MISC || (h->flags2 & SFLAG2_FLOATING))
			floorz = sectp->floorz;
		else
			floorz = h->floorz;

		const auto spactpos = getPlayer(screenpeek)->GetActor()->getPosWithOffsetZ();

		if (h->spr.pos.Z - floorz < 8 && spactpos.Z < floorz)
		{
			auto shadowspr = tsprites.newTSprite();
			*shadowspr = *t;

			shadowspr->statnum = STAT_TEMP;

			shadowspr->scale.Y = (max(t->scale.Y * 0.125, 0.0625));
			shadowspr->shade = 127;
			shadowspr->cstat |= CSTAT_SPRITE_TRANSLUCENT;

			shadowspr->pos.Z = floorz;
			shadowspr->pal = 4;

			if (hw_models && modelManager.CheckModel(t->spritetexture(), t->pal))
			{
				shadowspr->scale.Y = (0);
				// 512:trans reverse
				//1024:tell MD2SPRITE.C to use Z-buffer hacks to hide overdraw issues (todo: use a stencil to do this right.)
				shadowspr->clipdist |= TSPR_FLAGS_MDHACK;
				shadowspr->cstat |= CSTAT_SPRITE_TRANS_FLIP;
			}
			else
			{
				// Alter the shadow's position so that it appears behind the sprite itself.
				auto look = (shadowspr->pos.XY() - spactpos.XY()).Angle();
				shadowspr->pos += look.ToVector() * 2;
			}
		}
	}
}

// ---------------------------------------------------------------------------
//
// some ugly stuff here: RRRA forces some animations fullbright, 
// but there is no good way to set this up for CON in any decent way.
// These frames are being hacked in here. No need to make this configurable, though. 
// For our new format this can be done as a real feature.
//
// ---------------------------------------------------------------------------

bool RRRAFullbrightHack(tspritetype* t, int k)
{
	if (t->ownerActor->IsKindOf(RedneckBillyRayClass))
	{
		return k >= 102 && k <= 151;
	}
	else if (t->ownerActor->IsKindOf(RedneckBikerClass))
	{
		return (k >= 54 && k <= 58) || (k >= 84 && k <= 88);
	}
	else if (t->ownerActor->IsKindOf(RedneckCheerleaderClass))
	{
		return k >= 102 && k <= 151;
	}
	return false;
}

void applyanimations(tspritetype* t, DDukeActor* h, const DVector2& viewVec, DAngle viewang)
{
	if (!(h->flags2 & SFLAG2_DONTANIMATE))// && (t->cstat & CSTAT_SPRITE_ALIGNMENT_MASK) != CSTAT_SPRITE_ALIGNMENT_SLAB)
	{
		DAngle kang;
		auto action = h->curAction;
		int k = 0, l = 0;
		if (h->curAction->name != NAME_None)
		{
			l = action->rotationtype;

			if (tilehasmodelorvoxel(h->spr.spritetexture(), h->spr.pal))
			{
				k = 0;
				t->cstat &= ~CSTAT_SPRITE_XFLIP;
			}
			else switch (l)
			{
			case 2:
				k = angletorotation1(t->Angles.Yaw, viewang, 8, 1);
				break;

			case 3:
			case 4:
				k = angletorotation1(t->Angles.Yaw, viewang, 7);
				if (k > 3)
				{
					t->cstat |= CSTAT_SPRITE_XFLIP;
					k = 7 - k;
				}
				else t->cstat &= ~CSTAT_SPRITE_XFLIP;
				break;

			case 5:
				kang = (t->pos.XY() - viewVec).Angle();
				k = angletorotation1(t->Angles.Yaw, kang);
				if (k > 4)
				{
					k = 8 - k;
					t->cstat |= CSTAT_SPRITE_XFLIP;
				}
				else t->cstat &= ~CSTAT_SPRITE_XFLIP;
				break;
			case 7:
				kang = (t->pos.XY() - viewVec).Angle();
				k = angletorotation2(t->Angles.Yaw, kang);
				if (k > 6)
				{
					k = 12 - k;
					t->cstat |= CSTAT_SPRITE_XFLIP;
				}
				else t->cstat &= ~CSTAT_SPRITE_XFLIP;
				break;
			case 8:
				k = angletorotation1(t->Angles.Yaw, viewang);
				t->cstat &= ~CSTAT_SPRITE_XFLIP;
				break;
			default:
				if (isRR())
				{
					bool bg = badguy(h);
					if (bg && h->spr.statnum == STAT_ZOMBIEACTOR && h->spr.extra > 0)
					{
						kang = (t->pos.XY() - viewVec).Angle();
						k = angletorotation1(t->Angles.Yaw, kang);
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
			}

			k += action->offset + l * h->curframe;
			int texid = t->spritetexture().GetIndex() + k; // we cannot work with texture IDs here because their arithmetics are limited.

			if (isRRRA() && RRRAFullbrightHack(t, k)) t->shade = -127;

			FGameTexture* tex = TexMan.GameByIndex(texid);
			if (l > 0)
			{
				while(1)
				{
					if (!tex || tex->isValid()) break;
					texid -= l;       //back up one frame if this one is invald.
					tex = TexMan.GameByIndex(texid);
				}
			}

			if (!tex)
			{
				t->setspritetexture(FNullTextureID());
				t->scale = DVector2(0, 0);
			}
			else t->setspritetexture(tex->GetID());

			if (h->dispictex.isValid())
				h->dispictex = t->spritetexture();
		}

		if (h->flags4 & SFLAG4_FLASHFRAME0)
		{
			if (t->spritetexture() == h->spr.spritetexture())
			{
				getPlayer(screenpeek)->visibility = -127;
				lastvisinc = PlayClock + 32;
			}
		}

	}
}
END_DUKE_NS
