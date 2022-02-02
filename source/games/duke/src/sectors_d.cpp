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
#include "sounds.h"
#include "names_d.h"
#include "mapinfo.h"
#include "dukeactor.h"
#include "secrets.h"

// PRIMITIVE
BEGIN_DUKE_NS

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool isadoorwall_d(int dapic)
{
	switch(dapic)
	{
		case DOORTILE1:
		case DOORTILE2:
		case DOORTILE3:
		case DOORTILE4:
		case DOORTILE5:
		case DOORTILE6:
		case DOORTILE7:
		case DOORTILE8:
		case DOORTILE9:
		case DOORTILE10:
		case DOORTILE11:
		case DOORTILE12:
		case DOORTILE14:
		case DOORTILE15:
		case DOORTILE16:
		case DOORTILE17:
		case DOORTILE18:
		case DOORTILE19:
		case DOORTILE20:
		case DOORTILE21:
		case DOORTILE22:
		case DOORTILE23:
			return 1;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void animatewalls_d(void)
{
	int t;

	for (int p = 0; p < numanimwalls; p++)
	{
		auto wal = animwall[p].wall;
		int j = wal->picnum;

		switch (j)
		{
		case SCREENBREAK1:
		case SCREENBREAK2:
		case SCREENBREAK3:
		case SCREENBREAK4:
		case SCREENBREAK5:

		case SCREENBREAK9:
		case SCREENBREAK10:
		case SCREENBREAK11:
		case SCREENBREAK12:
		case SCREENBREAK13:
		case SCREENBREAK14:
		case SCREENBREAK15:
		case SCREENBREAK16:
		case SCREENBREAK17:
		case SCREENBREAK18:
		case SCREENBREAK19:

			if ((krand() & 255) < 16)
			{
				animwall[p].tag = wal->picnum;
				wal->picnum = SCREENBREAK6;
			}

			continue;

		case SCREENBREAK6:
		case SCREENBREAK7:
		case SCREENBREAK8:

			if (animwall[p].tag >= 0 && wal->extra != FEMPIC2 && wal->extra != FEMPIC3)
				wal->picnum = animwall[p].tag;
			else
			{
				wal->picnum++;
				if (wal->picnum == (SCREENBREAK6 + 3))
					wal->picnum = SCREENBREAK6;
			}
			continue;

		}

		if (wal->cstat & CSTAT_WALL_MASKED)
			switch (wal->overpicnum)
			{
			case W_FORCEFIELD:
			case W_FORCEFIELD + 1:
			case W_FORCEFIELD + 2:

				t = animwall[p].tag;

				if (wal->cstat & CSTAT_WALL_ANY_EXCEPT_BLOCK)
				{
					wal->addxpan(-t / 4096.f); // bcos(t, -12);
					wal->addypan(-t / 4096.f); // bsin(t, -12);

					if (wal->extra == 1)
					{
						wal->extra = 0;
						animwall[p].tag = 0;
					}
					else
						animwall[p].tag += 128;

					if (animwall[p].tag < (128 << 4))
					{
						if (animwall[p].tag & 128)
							wal->overpicnum = W_FORCEFIELD;
						else wal->overpicnum = W_FORCEFIELD + 1;
					}
					else
					{
						if ((krand() & 255) < 32)
							animwall[p].tag = 128 << (krand() & 3);
						else wal->overpicnum = W_FORCEFIELD + 1;
					}
				}

				break;
			}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void operaterespawns_d(int low)
{
	DukeStatIterator it(STAT_FX);
	while (auto act = it.Next())
	{
		if (act->spr.lotag == low) switch (act->spr.picnum)
		{
		case RESPAWN:
			if (badguypic(act->spr.hitag) && ud.monsters_off) break;

			auto star = spawn(act, TRANSPORTERSTAR);
			if (star)
			{
				star->add_int_z(-(32 << 8));

				act->spr.extra = 66 - 12;   // Just a way to killit
			}
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void operateforcefields_d(DDukeActor* act, int low)
{
	operateforcefields_common(act, low, { W_FORCEFIELD, W_FORCEFIELD + 1, W_FORCEFIELD + 2, BIGFORCE });
}

//---------------------------------------------------------------------------
//
// how NOT to implement switch animations...
//
//---------------------------------------------------------------------------

bool checkhitswitch_d(int snum, walltype* wwal, DDukeActor *act)
{
	uint8_t switchpal;
	int lotag, hitag, picnum, correctdips, numdips;
	int sx, sy;

	if (wwal == nullptr && act == nullptr) return 0;
	correctdips = 1;
	numdips = 0;

	if (act)
	{
		lotag = act->spr.lotag;
		if (lotag == 0) return 0;
		hitag = act->spr.hitag;
		sx = act->int_pos().X;
		sy = act->int_pos().Y;
		picnum = act->spr.picnum;
		switchpal = act->spr.pal;
	}
	else
	{
		lotag = wwal->lotag;
		if (lotag == 0) return 0;
		hitag = wwal->hitag;
		sx = wwal->wall_int_pos().X;
		sy = wwal->wall_int_pos().Y;
		picnum = wwal->picnum;
		switchpal = wwal->pal;
	}

	switch (picnum)
	{
	case DIPSWITCH:
	case DIPSWITCH + 1:
	case TECHSWITCH:
	case TECHSWITCH + 1:
	case ALIENSWITCH:
	case ALIENSWITCH + 1:
		break;
	case DEVELOPERCOMMENTARY + 1: //Twentieth Anniversary World Tour
		if (act)
		{
			StopCommentary();
			act->spr.picnum = DEVELOPERCOMMENTARY;
			return true;
		}
		return false;
	case DEVELOPERCOMMENTARY: //Twentieth Anniversary World Tour
		if (act)
		{
			if (StartCommentary(lotag, act))
				act->spr.picnum = DEVELOPERCOMMENTARY+1;
			return true;
		}
		return false;
	case ACCESSSWITCH:
	case ACCESSSWITCH2:
		if (ps[snum].access_incs == 0)
		{
			if (switchpal == 0)
			{
				if ((ps[snum].got_access & 1))
					ps[snum].access_incs = 1;
				else FTA(70, &ps[snum]);
			}

			else if (switchpal == 21)
			{
				if (ps[snum].got_access & 2)
					ps[snum].access_incs = 1;
				else FTA(71, &ps[snum]);
			}

			else if (switchpal == 23)
			{
				if (ps[snum].got_access & 4)
					ps[snum].access_incs = 1;
				else FTA(72, &ps[snum]);
			}

			if (ps[snum].access_incs == 1)
			{
				if (!act)
					ps[snum].access_wall = wwal;
				else
					ps[snum].access_spritenum = act;
			}

			return 0;
		}
		[[fallthrough]];
	case DIPSWITCH2:
	case DIPSWITCH2 + 1:
	case DIPSWITCH3:
	case DIPSWITCH3 + 1:
	case MULTISWITCH:
	case MULTISWITCH + 1:
	case MULTISWITCH + 2:
	case MULTISWITCH + 3:
	case PULLSWITCH:
	case PULLSWITCH + 1:
	case HANDSWITCH:
	case HANDSWITCH + 1:
	case SLOTDOOR:
	case SLOTDOOR + 1:
	case LIGHTSWITCH:
	case LIGHTSWITCH + 1:
	case SPACELIGHTSWITCH:
	case SPACELIGHTSWITCH + 1:
	case SPACEDOORSWITCH:
	case SPACEDOORSWITCH + 1:
	case FRANKENSTINESWITCH:
	case FRANKENSTINESWITCH + 1:
	case LIGHTSWITCH2:
	case LIGHTSWITCH2 + 1:
	case POWERSWITCH1:
	case POWERSWITCH1 + 1:
	case LOCKSWITCH1:
	case LOCKSWITCH1 + 1:
	case POWERSWITCH2:
	case POWERSWITCH2 + 1:
		if (check_activator_motion(lotag)) return 0;
		break;
	default:
		if (fi.isadoorwall(picnum) == 0) return 0;
		break;
	}

	DukeStatIterator it(STAT_DEFAULT);
	while (auto other = it.Next())
	{
		if (lotag == other->spr.lotag) switch (other->spr.picnum)
		{
		case DIPSWITCH:
		case TECHSWITCH:
		case ALIENSWITCH:
			if (act && act == other) other->spr.picnum++;
			else if (other->spr.hitag == 0) correctdips++;
			numdips++;
			break;
		case TECHSWITCH + 1:
		case DIPSWITCH + 1:
		case ALIENSWITCH + 1:
			if (act && act == other) other->spr.picnum--;
			else if (other->spr.hitag == 1) correctdips++;
			numdips++;
			break;
		case MULTISWITCH:
		case MULTISWITCH + 1:
		case MULTISWITCH + 2:
		case MULTISWITCH + 3:
			other->spr.picnum++;
			if (other->spr.picnum > (MULTISWITCH + 3))
				other->spr.picnum = MULTISWITCH;
			break;
		case ACCESSSWITCH:
		case ACCESSSWITCH2:
		case SLOTDOOR:
		case LIGHTSWITCH:
		case SPACELIGHTSWITCH:
		case SPACEDOORSWITCH:
		case FRANKENSTINESWITCH:
		case LIGHTSWITCH2:
		case POWERSWITCH1:
		case LOCKSWITCH1:
		case POWERSWITCH2:
		case HANDSWITCH:
		case PULLSWITCH:
		case DIPSWITCH2:
		case DIPSWITCH3:
			other->spr.picnum++;
			break;
		case PULLSWITCH + 1:
		case HANDSWITCH + 1:
		case LIGHTSWITCH2 + 1:
		case POWERSWITCH1 + 1:
		case LOCKSWITCH1 + 1:
		case POWERSWITCH2 + 1:
		case SLOTDOOR + 1:
		case LIGHTSWITCH + 1:
		case SPACELIGHTSWITCH + 1:
		case SPACEDOORSWITCH + 1:
		case FRANKENSTINESWITCH + 1:
		case DIPSWITCH2 + 1:
		case DIPSWITCH3 + 1:
			other->spr.picnum--;
			break;
		}
	}

	for (auto& wal : wall)
	{
		if (lotag == wal.lotag)
			switch (wal.picnum)
			{
			case DIPSWITCH:
			case TECHSWITCH:
			case ALIENSWITCH:
				if (!act && &wal == wwal) wal.picnum++;
				else if (wal.hitag == 0) correctdips++;
				numdips++;
				break;
			case DIPSWITCH + 1:
			case TECHSWITCH + 1:
			case ALIENSWITCH + 1:
				if (!act && &wal == wwal) wal.picnum--;
				else if (wal.hitag == 1) correctdips++;
				numdips++;
				break;
			case MULTISWITCH:
			case MULTISWITCH + 1:
			case MULTISWITCH + 2:
			case MULTISWITCH + 3:
				wal.picnum++;
				if (wal.picnum > (MULTISWITCH + 3))
					wal.picnum = MULTISWITCH;
				break;
			case ACCESSSWITCH:
			case ACCESSSWITCH2:
			case SLOTDOOR:
			case LIGHTSWITCH:
			case SPACELIGHTSWITCH:
			case SPACEDOORSWITCH:
			case LIGHTSWITCH2:
			case POWERSWITCH1:
			case LOCKSWITCH1:
			case POWERSWITCH2:
			case PULLSWITCH:
			case HANDSWITCH:
			case DIPSWITCH2:
			case DIPSWITCH3:
				wal.picnum++;
				break;
			case HANDSWITCH + 1:
			case PULLSWITCH + 1:
			case LIGHTSWITCH2 + 1:
			case POWERSWITCH1 + 1:
			case LOCKSWITCH1 + 1:
			case POWERSWITCH2 + 1:
			case SLOTDOOR + 1:
			case LIGHTSWITCH + 1:
			case SPACELIGHTSWITCH + 1:
			case SPACEDOORSWITCH + 1:
			case DIPSWITCH2 + 1:
			case DIPSWITCH3 + 1:
				wal.picnum--;
				break;
			}
	}

	if (lotag == -1)
	{
		setnextmap(false);
		return 1;
	}

	vec3_t v = { sx, sy, ps[snum].pos.Z };
	switch (picnum)
	{
	default:
		if (fi.isadoorwall(picnum) == 0) break;
		[[fallthrough]];
	case DIPSWITCH:
	case DIPSWITCH + 1:
	case TECHSWITCH:
	case TECHSWITCH + 1:
	case ALIENSWITCH:
	case ALIENSWITCH + 1:
		if (picnum == DIPSWITCH || picnum == DIPSWITCH + 1 ||
			picnum == ALIENSWITCH || picnum == ALIENSWITCH + 1 ||
			picnum == TECHSWITCH || picnum == TECHSWITCH + 1)
		{
			if (picnum == ALIENSWITCH || picnum == ALIENSWITCH + 1)
			{
				if (act)
					S_PlaySound3D(ALIEN_SWITCH1, act, v);
				else S_PlaySound3D(ALIEN_SWITCH1, ps[snum].GetActor(), v);
			}
			else
			{
				if (act)
					S_PlaySound3D(SWITCH_ON, act, v);
				else S_PlaySound3D(SWITCH_ON, ps[snum].GetActor(), v);
			}
			if (numdips != correctdips) break;
			S_PlaySound3D(END_OF_LEVEL_WARN, ps[snum].GetActor(), v);
		}
		[[fallthrough]];
	case DIPSWITCH2:
	case DIPSWITCH2 + 1:
	case DIPSWITCH3:
	case DIPSWITCH3 + 1:
	case MULTISWITCH:
	case MULTISWITCH + 1:
	case MULTISWITCH + 2:
	case MULTISWITCH + 3:
	case ACCESSSWITCH:
	case ACCESSSWITCH2:
	case SLOTDOOR:
	case SLOTDOOR + 1:
	case LIGHTSWITCH:
	case LIGHTSWITCH + 1:
	case SPACELIGHTSWITCH:
	case SPACELIGHTSWITCH + 1:
	case SPACEDOORSWITCH:
	case SPACEDOORSWITCH + 1:
	case FRANKENSTINESWITCH:
	case FRANKENSTINESWITCH + 1:
	case LIGHTSWITCH2:
	case LIGHTSWITCH2 + 1:
	case POWERSWITCH1:
	case POWERSWITCH1 + 1:
	case LOCKSWITCH1:
	case LOCKSWITCH1 + 1:
	case POWERSWITCH2:
	case POWERSWITCH2 + 1:
	case HANDSWITCH:
	case HANDSWITCH + 1:
	case PULLSWITCH:
	case PULLSWITCH + 1:

		if (picnum == MULTISWITCH || picnum == (MULTISWITCH + 1) ||
			picnum == (MULTISWITCH + 2) || picnum == (MULTISWITCH + 3))
			lotag += picnum - MULTISWITCH;

		DukeStatIterator itr(STAT_EFFECTOR);
		while (auto other = itr.Next())
		{
			if (other->spr.hitag == lotag)
			{
				switch (other->spr.lotag)
				{
				case SE_12_LIGHT_SWITCH:
					other->sector()->floorpal = 0;
					other->temp_data[0]++;
					if (other->temp_data[0] == 2)
						other->temp_data[0]++;

					break;
				case SE_24_CONVEYOR:
				case SE_34:
				case SE_25_PISTON:
					other->temp_data[4] = !other->temp_data[4];
					if (other->temp_data[4])
						FTA(15, &ps[snum]);
					else FTA(2, &ps[snum]);
					break;
				case SE_21_DROP_FLOOR:
					FTA(2, &ps[screenpeek]);
					break;
				}
			}
		}

		operateactivators(lotag, snum);
		fi.operateforcefields(ps[snum].GetActor(), lotag);
		operatemasterswitches(lotag);

		if (picnum == DIPSWITCH || picnum == DIPSWITCH + 1 ||
			picnum == ALIENSWITCH || picnum == ALIENSWITCH + 1 ||
			picnum == TECHSWITCH || picnum == TECHSWITCH + 1) return 1;

		if (hitag == 0 && fi.isadoorwall(picnum) == 0)
		{
			if (act)
				S_PlaySound3D(SWITCH_ON, act, v);
			else S_PlaySound3D(SWITCH_ON, ps[snum].GetActor(), v);
		}
		else if (hitag != 0)
		{
			auto flags = S_GetUserFlags(hitag);

			if (act && (flags & SF_TALK) == 0)
				S_PlaySound3D(hitag, act, v);
			else
				S_PlayActorSound(hitag, ps[snum].GetActor());
		}

		return 1;
	}
	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void activatebysector_d(sectortype* sect, DDukeActor* activator)
{
	int didit = 0;

	DukeSectIterator it(sect);
	while (auto act = it.Next())
	{
		if (act->spr.picnum == ACTIVATOR)
		{
			operateactivators(act->spr.lotag, -1);
			didit = 1;
			//			return;
		}
	}

	if (didit == 0)
		operatesectors(sect, activator);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checkhitwall_d(DDukeActor* spr, walltype* wal, int x, int y, int z, int atwith)
{
	int j, sn = -1, darkestwall;

	if (wal->overpicnum == MIRROR && gs.actorinfo[atwith].flags2 & SFLAG2_BREAKMIRRORS)
	{
		lotsofglass(spr, wal, 70);
		wal->cstat &= ~CSTAT_WALL_MASKED;
		wal->overpicnum = MIRRORBROKE;
		wal->portalflags = 0;
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;
	}

	if (((wal->cstat & CSTAT_WALL_MASKED) || wal->overpicnum == BIGFORCE) && wal->twoSided())
		if (wal->nextSector()->int_floorz() > z)
			if (wal->nextSector()->int_floorz() - wal->nextSector()->int_ceilingz())
				switch (wal->overpicnum)
				{
				case W_FORCEFIELD:
				case W_FORCEFIELD + 1:
				case W_FORCEFIELD + 2:
					wal->extra = 1; // tell the forces to animate
					[[fallthrough]];
				case BIGFORCE:
				{
					sectortype* sptr = nullptr;
					updatesector(x, y, &sptr);
					if (sptr == nullptr) return;
					DDukeActor* spawned;
					if (atwith == -1)
						spawned = EGS(sptr, x, y, z, FORCERIPPLE, -127, 8, 8, 0, 0, 0, spr, 5);
					else
					{
						if (atwith == CHAINGUN)
							spawned = EGS(sptr, x, y, z, FORCERIPPLE, -127, 16 + spr->spr.xrepeat, 16 + spr->spr.yrepeat, 0, 0, 0, spr, 5);
						else spawned = EGS(sptr, x, y, z, FORCERIPPLE, -127, 32, 32, 0, 0, 0, spr, 5);
					}
					if (spawned)
					{
						spawned->spr.cstat |= CSTAT_SPRITE_TRANSLUCENT | CSTAT_SPRITE_ALIGNMENT_WALL | CSTAT_SPRITE_YCENTER;
						auto delta = wal->delta();
						spawned->spr.ang = getangle(-delta.X, -delta.Y) - 512;

						S_PlayActorSound(SOMETHINGHITFORCE, spawned);
					}
					return;
				}
				case FANSPRITE:
					wal->overpicnum = FANSPRITEBROKE;
					wal->cstat &= ~(CSTAT_WALL_BLOCK | CSTAT_WALL_BLOCK_HITSCAN);
					if (wal->twoSided())
					{
						wal->nextWall()->overpicnum = FANSPRITEBROKE;
						wal->nextWall()->cstat &= ~(CSTAT_WALL_BLOCK | CSTAT_WALL_BLOCK_HITSCAN);
					}
					S_PlayActorSound(VENT_BUST, spr);
					S_PlayActorSound(GLASS_BREAKING, spr);
					return;

				case GLASS:
				{
					sectortype* sptr = nullptr;
					updatesector(x, y, &sptr);
					if (sptr == nullptr) return;
					wal->overpicnum = GLASS2;
					lotsofglass(spr, wal, 10);
					wal->cstat = 0;

					if (wal->twoSided())
						wal->nextWall()->cstat = 0;

					auto spawned = EGS(sptr, x, y, z, SECTOREFFECTOR, 0, 0, 0, ps[0].angle.ang.asbuild(), 0, 0, spr, 3);
					if (spawned)
					{
						spawned->spr.lotag = SE_128_GLASS_BREAKING;
						spawned->temp_data[1] = 5;
						spawned->temp_walls[0] = wal;
						S_PlayActorSound(GLASS_BREAKING, spawned);
					}
					return;
				}
				case STAINGLASS1:
					sectortype* sptr = nullptr;
					updatesector(x, y, &sptr);
					if (sptr == nullptr) return;
					lotsofcolourglass(spr, wal, 80);
					wal->cstat = 0;
					if (wal->twoSided())
						wal->nextWall()->cstat = 0;
					S_PlayActorSound(VENT_BUST, spr);
					S_PlayActorSound(GLASS_BREAKING, spr);
					return;
				}

	switch (wal->picnum)
	{
	case COLAMACHINE:
	case VENDMACHINE:
		breakwall(wal->picnum + 2, spr, wal);
		S_PlayActorSound(VENT_BUST, spr);
		return;

	case OJ:
	case FEMPIC2:
	case FEMPIC3:

	case SCREENBREAK6:
	case SCREENBREAK7:
	case SCREENBREAK8:

	case SCREENBREAK1:
	case SCREENBREAK2:
	case SCREENBREAK3:
	case SCREENBREAK4:
	case SCREENBREAK5:

	case SCREENBREAK9:
	case SCREENBREAK10:
	case SCREENBREAK11:
	case SCREENBREAK12:
	case SCREENBREAK13:
	case SCREENBREAK14:
	case SCREENBREAK15:
	case SCREENBREAK16:
	case SCREENBREAK17:
	case SCREENBREAK18:
	case SCREENBREAK19:
	case BORNTOBEWILDSCREEN:

		lotsofglass(spr, wal, 30);
		wal->picnum = W_SCREENBREAK + (krand() % 3);
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;

	case W_TECHWALL5:
	case W_TECHWALL6:
	case W_TECHWALL7:
	case W_TECHWALL8:
	case W_TECHWALL9:
		breakwall(wal->picnum + 1, spr, wal);
		return;
	case W_MILKSHELF:
		breakwall(W_MILKSHELFBROKE, spr, wal);
		return;

	case W_TECHWALL10:
		breakwall(W_HITTECHWALL10, spr, wal);
		return;

	case W_TECHWALL1:
	case W_TECHWALL11:
	case W_TECHWALL12:
	case W_TECHWALL13:
	case W_TECHWALL14:
		breakwall(W_HITTECHWALL1, spr, wal);
		return;

	case W_TECHWALL15:
		breakwall(W_HITTECHWALL15, spr, wal);
		return;

	case W_TECHWALL16:
		breakwall(W_HITTECHWALL16, spr, wal);
		return;

	case W_TECHWALL2:
		breakwall(W_HITTECHWALL2, spr, wal);
		return;

	case W_TECHWALL3:
		breakwall(W_HITTECHWALL3, spr, wal);
		return;

	case W_TECHWALL4:
		breakwall(W_HITTECHWALL4, spr, wal);
		return;

	case ATM:
		wal->picnum = ATMBROKE;
		fi.lotsofmoney(spr, 1 + (krand() & 7));
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		break;

	case WALLLIGHT1:
	case WALLLIGHT2:
	case WALLLIGHT3:
	case WALLLIGHT4:
	case TECHLIGHT2:
	case TECHLIGHT4:

		if (rnd(128))
			S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		else S_PlayActorSound(GLASS_BREAKING, spr);
		lotsofglass(spr, wal, 30);

		if (wal->picnum == WALLLIGHT1)
			wal->picnum = WALLLIGHTBUST1;

		if (wal->picnum == WALLLIGHT2)
			wal->picnum = WALLLIGHTBUST2;

		if (wal->picnum == WALLLIGHT3)
			wal->picnum = WALLLIGHTBUST3;

		if (wal->picnum == WALLLIGHT4)
			wal->picnum = WALLLIGHTBUST4;

		if (wal->picnum == TECHLIGHT2)
			wal->picnum = TECHLIGHTBUST2;

		if (wal->picnum == TECHLIGHT4)
			wal->picnum = TECHLIGHTBUST4;

		if (!wal->lotag) return;

		if (!wal->twoSided()) return;
		darkestwall = 0;

		for (auto& wl : wallsofsector(wal->nextSector()))
			if (wl.shade > darkestwall)
				darkestwall = wl.shade;

		j = krand() & 1;
		DukeStatIterator it(STAT_EFFECTOR);
		while (auto effector = it.Next())
		{
			if (effector->spr.hitag == wal->lotag && effector->spr.lotag == 3)
			{
				effector->temp_data[2] = j;
				effector->temp_data[3] = darkestwall;
				effector->temp_data[4] = 1;
			}
		}
		break;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checkplayerhurt_d(struct player_struct* p, const Collision& coll)
{
	if (coll.type == kHitSprite)
	{
		switch (coll.actor()->spr.picnum)
		{
		case CACTUS:
			if (p->hurt_delay < 8)
			{
				p->GetActor()->spr.extra -= 5;
				p->hurt_delay = 16;
				SetPlayerPal(p, PalEntry(32, 32, 0, 0));
				S_PlayActorSound(DUKE_LONGTERM_PAIN, p->GetActor());
			}
			break;
		}
		return;
	}

	if (coll.type != kHitWall) return;
	auto wal = coll.hitWall;

	if (p->hurt_delay > 0) p->hurt_delay--;
	else if (wal->cstat & (CSTAT_WALL_BLOCK | CSTAT_WALL_ALIGN_BOTTOM | CSTAT_WALL_MASKED | CSTAT_WALL_BLOCK_HITSCAN)) switch (wal->overpicnum)
	{
	case W_FORCEFIELD:
	case W_FORCEFIELD + 1:
	case W_FORCEFIELD + 2:
		p->GetActor()->spr.extra -= 5;

		p->hurt_delay = 16;
		SetPlayerPal(p, PalEntry(32, 32, 0, 0));

		p->vel.X = -p->angle.ang.bcos(8);
		p->vel.Y = -p->angle.ang.bsin(8);
		S_PlayActorSound(DUKE_LONGTERM_PAIN, p->GetActor());

		fi.checkhitwall(p->GetActor(), wal,
			p->pos.X + p->angle.ang.bcos(-9),
			p->pos.Y + p->angle.ang.bsin(-9),
			p->pos.Z, -1);

		break;

	case BIGFORCE:
		p->hurt_delay = 26;
		fi.checkhitwall(p->GetActor(), wal,
			p->pos.X + p->angle.ang.bcos(-9),
			p->pos.Y + p->angle.ang.bsin(-9),
			p->pos.Z, -1);
		break;

	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool checkhitceiling_d(sectortype* sectp)
{
	int j;

	switch (sectp->ceilingpicnum)
	{
	case WALLLIGHT1:
	case WALLLIGHT2:
	case WALLLIGHT3:
	case WALLLIGHT4:
	case TECHLIGHT2:
	case TECHLIGHT4:

		ceilingglass(ps[myconnectindex].GetActor(), sectp, 10);
		S_PlayActorSound(GLASS_BREAKING, ps[screenpeek].GetActor());

		if (sectp->ceilingpicnum == WALLLIGHT1)
			sectp->ceilingpicnum = WALLLIGHTBUST1;

		if (sectp->ceilingpicnum == WALLLIGHT2)
			sectp->ceilingpicnum = WALLLIGHTBUST2;

		if (sectp->ceilingpicnum == WALLLIGHT3)
			sectp->ceilingpicnum = WALLLIGHTBUST3;

		if (sectp->ceilingpicnum == WALLLIGHT4)
			sectp->ceilingpicnum = WALLLIGHTBUST4;

		if (sectp->ceilingpicnum == TECHLIGHT2)
			sectp->ceilingpicnum = TECHLIGHTBUST2;

		if (sectp->ceilingpicnum == TECHLIGHT4)
			sectp->ceilingpicnum = TECHLIGHTBUST4;


		if (!sectp->hitag)
		{
			DukeSectIterator it(sectp);
			while (auto act = it.Next())
			{
				if (act->spr.picnum == SECTOREFFECTOR && act->spr.lotag == 12)
				{
					DukeStatIterator it1(STAT_EFFECTOR);
					while (auto act2 = it1.Next())
					{
						if (act2->spr.hitag == act->spr.hitag)
							act2->temp_data[3] = 1;
					}
					break;
				}
			}
		}

		j = krand() & 1;
		DukeStatIterator it(STAT_EFFECTOR);
		while (auto act = it.Next())
		{
			if (act->spr.hitag == (sectp->hitag) && act->spr.lotag == 3)
			{
				act->temp_data[2] = j;
				act->temp_data[4] = 1;
			}
		}

		return 1;
	}

	return 0;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checkhitsprite_d(DDukeActor* targ, DDukeActor* proj)
{
	int j, k, p;

	switch (targ->spr.picnum)
	{
	case WTGLASS1:
	case WTGLASS2:
		if (!isWorldTour())
			break;
		S_PlayActorSound(GLASS_BREAKING, targ);
		lotsofglass(targ, nullptr, 10);
		deletesprite(targ);
		return;

	case OCEANSPRITE1:
	case OCEANSPRITE2:
	case OCEANSPRITE3:
	case OCEANSPRITE4:
	case OCEANSPRITE5:
		spawn(targ, SMALLSMOKE);
		deletesprite(targ);
		break;
	case QUEBALL:
	case STRIPEBALL:
		if (proj->spr.picnum == QUEBALL || proj->spr.picnum == STRIPEBALL)
		{
			proj->spr.xvel = (targ->spr.xvel >> 1) + (targ->spr.xvel >> 2);
			proj->spr.ang -= (targ->spr.ang << 1) + 1024;
			targ->spr.ang = getangle(targ->int_pos().X - proj->int_pos().X, targ->int_pos().Y - proj->int_pos().Y) - 512;
			if (S_CheckSoundPlaying(POOLBALLHIT) < 2)
				S_PlayActorSound(POOLBALLHIT, targ);
		}
		else
		{
			if (krand() & 3)
			{
				targ->spr.xvel = 164;
				targ->spr.ang = proj->spr.ang;
			}
			else
			{
				lotsofglass(targ, nullptr, 3);
				deletesprite(targ);
			}
		}
		break;
	case TREE1:
	case TREE2:
	case TIRE:
	case CONE:
	case BOX:
		if (actorflag(proj, SFLAG_INFLAME))
		{
			if (targ->temp_data[0] == 0)
			{
				targ->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
				targ->temp_data[0] = 1;
				spawn(targ, BURNING);
			}
		}
		break;
	case CACTUS:
		//		case CACTUSBROKE:
		if (actorflag(proj, SFLAG_INFLAME))
		{
			for (k = 0; k < 64; k++)
			{
				auto spawned = EGS(targ->sector(), targ->int_pos().X, targ->int_pos().Y, targ->int_pos().Z - (krand() % (48 << 8)), SCRAP3 + (krand() & 3), -8, 48, 48, krand() & 2047, (krand() & 63) + 64, -(krand() & 4095) - (targ->spr.zvel >> 2), targ, 5);
				spawned->spr.pal = 8;
			}

			if (targ->spr.picnum == CACTUS)
				targ->spr.picnum = CACTUSBROKE;
			targ->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		}
		break;

	case HANGLIGHT:
	case GENERICPOLE2:
		for (k = 0; k < 6; k++)
			EGS(targ->sector(), targ->int_pos().X, targ->int_pos().Y, targ->int_pos().Z - (8 << 8), SCRAP1 + (krand() & 15), -8, 48, 48, krand() & 2047, (krand() & 63) + 64, -(krand() & 4095) - (targ->spr.zvel >> 2), targ, 5);
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		deletesprite(targ);
		break;


	case FANSPRITE:
		targ->spr.picnum = FANSPRITEBROKE;
		targ->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		if (targ->sector()->floorpicnum == FANSHADOW)
			targ->sector()->floorpicnum = FANSHADOWBROKE;

		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		for (j = 0; j < 16; j++) RANDOMSCRAP(targ);

		break;
	case WATERFOUNTAIN:
	case WATERFOUNTAIN + 1:
	case WATERFOUNTAIN + 2:
	case WATERFOUNTAIN + 3:
		targ->spr.picnum = WATERFOUNTAINBROKE;
		spawn(targ, TOILETWATER);
		break;
	case SATELITE:
	case FUELPOD:
	case SOLARPANNEL:
	case ANTENNA:
		if (gs.actorinfo[SHOTSPARK1].scriptaddress && proj->spr.extra != ScriptCode[gs.actorinfo[SHOTSPARK1].scriptaddress])
		{
			for (j = 0; j < 15; j++)
				EGS(targ->sector(), targ->int_pos().X, targ->int_pos().Y, targ->sector()->int_floorz() - (12 << 8) - (j << 9), SCRAP1 + (krand() & 15), -8, 64, 64,
					krand() & 2047, (krand() & 127) + 64, -(krand() & 511) - 256, targ, 5);
			spawn(targ, EXPLOSION2);
			deletesprite(targ);
		}
		break;
	case BOTTLE1:
	case BOTTLE2:
	case BOTTLE3:
	case BOTTLE4:
	case BOTTLE5:
	case BOTTLE6:
	case BOTTLE8:
	case BOTTLE10:
	case BOTTLE11:
	case BOTTLE12:
	case BOTTLE13:
	case BOTTLE14:
	case BOTTLE15:
	case BOTTLE16:
	case BOTTLE17:
	case BOTTLE18:
	case BOTTLE19:
	case WATERFOUNTAINBROKE:
	case DOMELITE:
	case SUSHIPLATE1:
	case SUSHIPLATE2:
	case SUSHIPLATE3:
	case SUSHIPLATE4:
	case SUSHIPLATE5:
	case WAITTOBESEATED:
	case VASE:
	case STATUEFLASH:
	case STATUE:
		if (targ->spr.picnum == BOTTLE10)
			fi.lotsofmoney(targ, 4 + (krand() & 3));
		else if (targ->spr.picnum == STATUE || targ->spr.picnum == STATUEFLASH)
		{
			lotsofcolourglass(targ, nullptr, 40);
			S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		}
		else if (targ->spr.picnum == VASE)
			lotsofglass(targ, nullptr, 40);

		S_PlayActorSound(GLASS_BREAKING, targ);
		targ->spr.ang = krand() & 2047;
		lotsofglass(targ, nullptr, 8);
		deletesprite(targ);
		break;
	case FETUS:
		targ->spr.picnum = FETUSBROKE;
		S_PlayActorSound(GLASS_BREAKING, targ);
		lotsofglass(targ, nullptr, 10);
		break;
	case FETUSBROKE:
		for (j = 0; j < 48; j++)
		{
			fi.shoot(targ, BLOODSPLAT1);
			targ->spr.ang += 333;
		}
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		S_PlayActorSound(SQUISHED, targ);
		[[fallthrough]];
	case BOTTLE7:
		S_PlayActorSound(GLASS_BREAKING, targ);
		lotsofglass(targ, nullptr, 10);
		deletesprite(targ);
		break;
	case HYDROPLANT:
		targ->spr.picnum = BROKEHYDROPLANT;
		S_PlayActorSound(GLASS_BREAKING, targ);
		lotsofglass(targ, nullptr, 10);
		break;

	case FORCESPHERE:
		targ->spr.xrepeat = 0;
		if (targ->GetOwner())
		{
			targ->GetOwner()->temp_data[0] = 32;
			targ->GetOwner()->temp_data[1] = !targ->GetOwner()->temp_data[1];
			targ->GetOwner()->temp_data[2] ++;
		}
		spawn(targ, EXPLOSION2);
		break;

	case BROKEHYDROPLANT:
		if (targ->spr.cstat & CSTAT_SPRITE_BLOCK)
		{
			S_PlayActorSound(GLASS_BREAKING, targ);
			targ->add_int_z(16 << 8);
			targ->spr.cstat = 0;
			lotsofglass(targ, nullptr, 5);
		}
		break;

	case TOILET:
		targ->spr.picnum = TOILETBROKE;
		if (krand() & 1) targ->spr.cstat |= CSTAT_SPRITE_XFLIP;
		targ->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		spawn(targ, TOILETWATER);
		S_PlayActorSound(GLASS_BREAKING, targ);
		break;

	case STALL:
		targ->spr.picnum = STALLBROKE;
		if (krand() & 1) targ->spr.cstat |= CSTAT_SPRITE_XFLIP;
		targ->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		spawn(targ, TOILETWATER);
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;

	case HYDRENT:
		targ->spr.picnum = BROKEFIREHYDRENT;
		spawn(targ, TOILETWATER);
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;

	case GRATE1:
		targ->spr.picnum = BGRATE1;
		targ->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		S_PlayActorSound(VENT_BUST, targ);
		break;

	case CIRCLEPANNEL:
		targ->spr.picnum = CIRCLEPANNELBROKE;
		targ->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		S_PlayActorSound(VENT_BUST, targ);
		break;
	case PANNEL1:
	case PANNEL2:
		targ->spr.picnum = BPANNEL1;
		targ->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		S_PlayActorSound(VENT_BUST, targ);
		break;
	case PANNEL3:
		targ->spr.picnum = BPANNEL3;
		targ->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
		S_PlayActorSound(VENT_BUST, targ);
		break;
	case PIPE1:
	case PIPE2:
	case PIPE3:
	case PIPE4:
	case PIPE5:
	case PIPE6:
		switch (targ->spr.picnum)
		{
		case PIPE1:targ->spr.picnum = PIPE1B; break;
		case PIPE2:targ->spr.picnum = PIPE2B; break;
		case PIPE3:targ->spr.picnum = PIPE3B; break;
		case PIPE4:targ->spr.picnum = PIPE4B; break;
		case PIPE5:targ->spr.picnum = PIPE5B; break;
		case PIPE6:targ->spr.picnum = PIPE6B; break;
		}
		{
			auto spawned = spawn(targ, STEAM);
			if (spawned) spawned->set_int_z(targ->sector()->int_floorz() - (32 << 8));
		}
		break;

	case MONK:
	case LUKE:
	case INDY:
	case JURYGUY:
		S_PlayActorSound(targ->spr.lotag, targ);
		spawn(targ, targ->spr.hitag);
		[[fallthrough]];
	case SPACEMARINE:
	{
		targ->spr.extra -= proj->spr.extra;
		if (targ->spr.extra > 0) break;
		targ->spr.ang = krand() & 2047;
		fi.shoot(targ, BLOODSPLAT1);
		targ->spr.ang = krand() & 2047;
		fi.shoot(targ, BLOODSPLAT2);
		targ->spr.ang = krand() & 2047;
		fi.shoot(targ, BLOODSPLAT3);
		targ->spr.ang = krand() & 2047;
		fi.shoot(targ, BLOODSPLAT4);
		targ->spr.ang = krand() & 2047;
		fi.shoot(targ, BLOODSPLAT1);
		targ->spr.ang = krand() & 2047;
		fi.shoot(targ, BLOODSPLAT2);
		targ->spr.ang = krand() & 2047;
		fi.shoot(targ, BLOODSPLAT3);
		targ->spr.ang = krand() & 2047;
		fi.shoot(targ, BLOODSPLAT4);
		fi.guts(targ, JIBS1, 1, myconnectindex);
		fi.guts(targ, JIBS2, 2, myconnectindex);
		fi.guts(targ, JIBS3, 3, myconnectindex);
		fi.guts(targ, JIBS4, 4, myconnectindex);
		fi.guts(targ, JIBS5, 1, myconnectindex);
		fi.guts(targ, JIBS3, 6, myconnectindex);
		S_PlaySound(SQUISHED);
		deletesprite(targ);
		break;
	}
	case CHAIR1:
	case CHAIR2:
		targ->spr.picnum = BROKENCHAIR;
		targ->spr.cstat = 0;
		break;
	case CHAIR3:
	case MOVIECAMERA:
	case SCALE:
	case VACUUM:
	case CAMERALIGHT:
	case IVUNIT:
	case POT1:
	case POT2:
	case POT3:
	case TRIPODCAMERA:
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		for (j = 0; j < 16; j++) RANDOMSCRAP(targ);
		deletesprite(targ);
		break;
	case PLAYERONWATER:
		targ = targ->GetOwner();
		if (!targ) break;
		[[fallthrough]];
	default:
		if ((targ->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL) && targ->spr.hitag == 0 && targ->spr.lotag == 0 && targ->spr.statnum == 0)
			break;

		if ((proj->spr.picnum == FREEZEBLAST || proj->GetOwner() != targ) && targ->spr.statnum != 4)
		{
			if (badguy(targ) == 1)
			{
				if (isWorldTour() && targ->spr.picnum == FIREFLY && targ->spr.xrepeat < 48)
					break;

				if (proj->spr.picnum == RPG) proj->spr.extra <<= 1;

				if ((targ->spr.picnum != DRONE) && (targ->spr.picnum != ROTATEGUN) && (targ->spr.picnum != COMMANDER) && (targ->spr.picnum < GREENSLIME || targ->spr.picnum > GREENSLIME + 7))
					if (proj->spr.picnum != FREEZEBLAST)
						//if (actortype[targ->spr.picnum] == 0) //TRANSITIONAL. Cannot be done right with EDuke mess backing the engine. 
						{
							auto spawned = spawn(proj, JIBS6);
							if (spawned)
							{
								if (proj->spr.pal == 6)
									spawned->spr.pal = 6;
								spawned->add_int_z(4 << 8);
								spawned->spr.xvel = 16;
								spawned->spr.xrepeat = spawned->spr.yrepeat = 24;
								spawned->spr.ang += 32 - (krand() & 63);
							}
						}

				auto Owner = proj->GetOwner();

				if (Owner && Owner->spr.picnum == APLAYER && targ->spr.picnum != ROTATEGUN && targ->spr.picnum != DRONE)
					if (ps[Owner->PlayerIndex()].curr_weapon == SHOTGUN_WEAPON)
					{
						fi.shoot(targ, BLOODSPLAT3);
						fi.shoot(targ, BLOODSPLAT1);
						fi.shoot(targ, BLOODSPLAT2);
						fi.shoot(targ, BLOODSPLAT4);
					}

				if (targ->spr.picnum != TANK && !bossguy(targ) && targ->spr.picnum != RECON && targ->spr.picnum != ROTATEGUN)
				{
					if ((targ->spr.cstat & CSTAT_SPRITE_ALIGNMENT_MASK) == 0)
						targ->spr.ang = (proj->spr.ang + 1024) & 2047;
					targ->spr.xvel = -(proj->spr.extra << 2);
					auto sp = targ->sector();
					pushmove(targ, &sp, 128L, (4 << 8), (4 << 8), CLIPMASK0);
					if (sp != targ->sector() && sp != nullptr)
						ChangeActorSect(targ, sp);
				}

				if (targ->spr.statnum == 2)
				{
					ChangeActorStat(targ, 1);
					targ->timetosleep = SLEEPTIME;
				}
				if ((targ->spr.xrepeat < 24 || targ->spr.picnum == SHARK) && proj->spr.picnum == SHRINKSPARK) return;
			}

			if (targ->spr.statnum != 2)
			{
				if (proj->spr.picnum == FREEZEBLAST && ((targ->spr.picnum == APLAYER && targ->spr.pal == 1) || (gs.freezerhurtowner == 0 && proj->GetOwner() == targ)))
					return;


				int hitpic = proj->spr.picnum;
				auto Owner = proj->GetOwner();
				if (Owner && Owner->spr.picnum == APLAYER)
				{
					if (targ->spr.picnum == APLAYER && ud.coop != 0 && ud.ffire == 0)
						return;

					auto tOwner = targ->GetOwner();
					if (isWorldTour() && hitpic == FIREBALL && tOwner && tOwner->spr.picnum != FIREBALL)
						hitpic = FLAMETHROWERFLAME;
				}

				targ->attackertype = hitpic;
				targ->hitextra += proj->spr.extra;
				targ->hitang = proj->spr.ang;
				targ->SetHitOwner(Owner);
			}

			if (targ->spr.statnum == 10)
			{
				p = targ->spr.yvel;
				if (ps[p].newOwner != nullptr)
				{
					ps[p].newOwner = nullptr;
					ps[p].pos.X = ps[p].opos.X;
					ps[p].pos.Y = ps[p].opos.Y;
					ps[p].pos.Z = ps[p].opos.Z;
					ps[p].angle.restore();

					updatesector(ps[p].pos.X, ps[p].pos.Y, &ps[p].cursector);

					DukeStatIterator it(STAT_ACTOR);
					while (auto itActor = it.Next())
					{
						if (actorflag(itActor, SFLAG2_CAMERA)) itActor->spr.yvel = 0;
					}
				}

				if (targ->spr.xrepeat < 24 && proj->spr.picnum == SHRINKSPARK)
					return;

				auto hitowner = targ->GetHitOwner();
				if (!hitowner || hitowner->spr.picnum != APLAYER)
					if (ud.player_skill >= 3)
						proj->spr.extra += (proj->spr.extra >> 1);
			}

		}
		break;
	}
}

//---------------------------------------------------------------------------
//
// taken out of checksectors to eliminate some gotos.
//
//---------------------------------------------------------------------------

void clearcameras(int i, player_struct* p)
{
	if (i < 0)
	{
		p->pos.X = p->opos.X;
		p->pos.Y = p->opos.Y;
		p->pos.Z = p->opos.Z;
		p->newOwner = nullptr;

		updatesector(p->pos.X, p->pos.Y, &p->cursector);

		DukeStatIterator it(STAT_ACTOR);
		while (auto act = it.Next())
		{
			if (actorflag(act, SFLAG2_CAMERA)) act->spr.yvel = 0;
		}
	}
	else if (p->newOwner != nullptr)
		p->newOwner = nullptr;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checksectors_d(int snum)
{
	int i = -1, oldz;
	struct player_struct* p;
	walltype* hitscanwall;
	HitInfo near;

	p = &ps[snum];
	auto pact = p->GetActor();

	if (!p->insector()) return;

	switch (p->cursector->lotag)
	{

	case 32767:
		p->cursector->lotag = 0;
		FTA(9, p);
		p->secret_rooms++;
		SECRET_Trigger(sectnum(p->cursector));
		return;
	case -1:
		p->cursector->lotag = 0;
		setnextmap(false);
		return;
	case -2:
		p->cursector->lotag = 0;
		p->timebeforeexit = 26 * 8;
		p->customexitsound = p->cursector->hitag;
		return;
	default:
		if (p->cursector->lotag >= 10000 && p->cursector->lotag < 16383)
		{
			if (snum == screenpeek || ud.coop == 1)
				S_PlayActorSound(p->cursector->lotag - 10000, pact);
			p->cursector->lotag = 0;
		}
		break;

	}

	//After this point the the player effects the map with space

	if (chatmodeon || p->GetActor()->spr.extra <= 0) return;

	if (ud.cashman && PlayerInput(snum, SB_OPEN))
		fi.lotsofmoney(p->GetActor(), 2);

	if (p->newOwner != nullptr)
	{
		if (abs(PlayerInputSideVel(snum)) > 768 || abs(PlayerInputForwardVel(snum)) > 768)
		{
			clearcameras(-1, p);
			return;
		}
	}

	if (!(PlayerInput(snum, SB_OPEN)))
		p->toggle_key_flag = 0;

	else if (!p->toggle_key_flag)
	{

		if (PlayerInput(snum, SB_ESCAPE))
		{
			if (p->newOwner != nullptr)
			{
				clearcameras(-1, p);
			}
			return;
		}

		near.hitActor = nullptr;
		p->toggle_key_flag = 1;
		hitscanwall = nullptr;

		i = hitawall(p, &hitscanwall);
		if (hitscanwall != nullptr)
		{
			if (i < 1280 && hitscanwall->overpicnum == MIRROR)
				if (hitscanwall->lotag > 0 && S_CheckSoundPlaying(hitscanwall->lotag) == 0 && snum == screenpeek)
				{
					S_PlayActorSound(hitscanwall->lotag, pact);
					return;
				}

			if (hitscanwall != nullptr && (hitscanwall->cstat & CSTAT_WALL_MASKED))
				if (hitscanwall->lotag)
					return;
		}
		if (p->newOwner != nullptr)
			neartag({ p->opos.X, p->opos.Y, p->opos.Z }, p->GetActor()->sector(), p->angle.oang.asbuild(), near, 1280L, 1);
		else
		{
			neartag(p->pos, p->GetActor()->sector(), p->angle.oang.asbuild(), near, 1280, 1);
			if (near.actor() == nullptr && near.hitWall == nullptr && near.hitSector == nullptr)
				neartag({ p->pos.X, p->pos.Y, p->pos.Z + (8 << 8) }, p->GetActor()->sector(), p->angle.oang.asbuild(), near, 1280, 1);
			if (near.actor() == nullptr && near.hitWall == nullptr && near.hitSector == nullptr)
				neartag({ p->pos.X, p->pos.Y, p->pos.Z + (16 << 8) }, p->GetActor()->sector(), p->angle.oang.asbuild(), near, 1280, 1);
			if (near.actor() == nullptr && near.hitWall == nullptr && near.hitSector == nullptr)
			{
				neartag({ p->pos.X, p->pos.Y, p->pos.Z + (16 << 8) }, p->GetActor()->sector(), p->angle.oang.asbuild(), near, 1280, 3);
				if (near.actor() != nullptr)
				{
					switch (near.actor()->spr.picnum)
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
					case TOUGHGAL:
						return;
					}
				}

				near.clearObj();
			}
		}

		if (p->newOwner == nullptr && near.actor() == nullptr && near.hitWall == nullptr && near.hitSector == nullptr)
			if (isanunderoperator(p->GetActor()->sector()->lotag))
				near.hitSector = p->GetActor()->sector();

		if (near.hitSector && (near.hitSector->lotag & 16384))
			return;

		if (near.actor() == nullptr && near.hitWall == nullptr)
			if (p->cursector->lotag == 2)
			{
				DDukeActor* hit;
				oldz = hitasprite(p->GetActor(), &hit);
				if (hit) near.hitActor = hit;
				if (oldz > 1280) near.hitActor = nullptr;

			}

		auto const neartagsprite = near.actor();
		if (neartagsprite != nullptr)
		{
			if (fi.checkhitswitch(snum, nullptr, neartagsprite)) return;

			switch (neartagsprite->spr.picnum)
			{
			case TOILET:
			case STALL:
				if (p->last_pissed_time == 0)
				{
					S_PlayActorSound(DUKE_URINATE, p->GetActor());

					p->last_pissed_time = 26 * 220;
					p->transporter_hold = 29 * 2;
					if (p->holster_weapon == 0)
					{
						p->holster_weapon = 1;
						p->weapon_pos = -1;
					}
					if (p->GetActor()->spr.extra <= (gs.max_player_health - (gs.max_player_health / 10)))
					{
						p->GetActor()->spr.extra += gs.max_player_health / 10;
						p->last_extra = p->GetActor()->spr.extra;
					}
					else if (p->GetActor()->spr.extra < gs.max_player_health)
						p->GetActor()->spr.extra = gs.max_player_health;
				}
				else if (S_CheckActorSoundPlaying(neartagsprite, FLUSH_TOILET) == 0)
					S_PlayActorSound(FLUSH_TOILET, neartagsprite);
				return;

			case NUKEBUTTON:
			{
				walltype* wal;
				hitawall(p, &wal);
				if (wal != nullptr && wal->overpicnum == 0)
					if (neartagsprite->temp_data[0] == 0)
					{
						neartagsprite->temp_data[0] = 1;
						neartagsprite->SetOwner(p->GetActor());
						p->buttonpalette = neartagsprite->spr.pal;
						if (p->buttonpalette)
							ud.secretlevel = neartagsprite->spr.lotag;
						else ud.secretlevel = 0;
					}
				return;
			}
			case WATERFOUNTAIN:
				if (neartagsprite->temp_data[0] != 1)
				{
					neartagsprite->temp_data[0] = 1;
					neartagsprite->SetOwner(p->GetActor());

					if (p->GetActor()->spr.extra < gs.max_player_health)
					{
						p->GetActor()->spr.extra++;
						S_PlayActorSound(DUKE_DRINKING, p->GetActor());
					}
				}
				return;
			case PLUG:
				S_PlayActorSound(SHORT_CIRCUIT, pact);
				p->GetActor()->spr.extra -= 2 + (krand() & 3);
				SetPlayerPal(p, PalEntry(32, 48, 48, 64));
				break;
			case VIEWSCREEN:
			case VIEWSCREEN2:
			{
				DukeStatIterator it(STAT_ACTOR);
				while (auto acti = it.Next())
				{
					if (actorflag(acti, SFLAG2_CAMERA) && acti->spr.yvel == 0 && neartagsprite->spr.hitag == acti->spr.lotag)
					{
						acti->spr.yvel = 1; //Using this camera
						if (snum == screenpeek) S_PlaySound(MONITOR_ACTIVE);

						neartagsprite->SetOwner(acti);
						neartagsprite->spr.yvel = 1;
						camsprite = neartagsprite;

						p->newOwner = acti;
						return;
					}
				}
				i = -1;
			}

			clearcameras(i, p);
			return;
				}
			}

		if (!PlayerInput(snum, SB_OPEN)) return;
			else if (p->newOwner != nullptr)
		{
			clearcameras(-1, p);
			return;
			}

		if (near.hitWall == nullptr && near.hitSector == nullptr && near.actor() == nullptr)
			if (abs(hits(p->GetActor())) < 512)
			{
				if ((krand() & 255) < 16)
					S_PlayActorSound(DUKE_SEARCH2, pact);
				else S_PlayActorSound(DUKE_SEARCH, pact);
				return;
			}

		if (near.hitWall)
		{
			if (near.hitWall->lotag > 0 && fi.isadoorwall(near.hitWall->picnum))
			{
				if (hitscanwall == near.hitWall || hitscanwall == nullptr)
					fi.checkhitswitch(snum, near.hitWall, nullptr);
				return;
			}
			else if (p->newOwner != nullptr)
			{
				clearcameras(-1, p);
				return;
			}
		}

		if (near.hitSector && (near.hitSector->lotag & 16384) == 0 && isanearoperator(near.hitSector->lotag))
		{
			DukeSectIterator it(near.hitSector);
			while (auto act = it.Next())
			{
				if (act->spr.picnum == ACTIVATOR || act->spr.picnum == MASTERSWITCH)
					return;
			}
			operatesectors(near.hitSector, p->GetActor());
		}
		else if ((p->GetActor()->sector()->lotag & 16384) == 0)
		{
			if (isanunderoperator(p->GetActor()->sector()->lotag))
			{
				DukeSectIterator it(p->GetActor()->sector());
				while (auto act = it.Next())
				{
					if (act->spr.picnum == ACTIVATOR || act->spr.picnum == MASTERSWITCH) return;
				}
				operatesectors(p->GetActor()->sector(), p->GetActor());
			}
			else fi.checkhitswitch(snum, near.hitWall, nullptr);
		}
	}
}





END_DUKE_NS
