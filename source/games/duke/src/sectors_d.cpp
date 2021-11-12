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
	int i, j, p, t;

	for (p = 0; p < numanimwalls; p++)
	{
		i = animwall[p].wallnum;
		j = wall[i].picnum;

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
				animwall[p].tag = wall[i].picnum;
				wall[i].picnum = SCREENBREAK6;
			}

			continue;

		case SCREENBREAK6:
		case SCREENBREAK7:
		case SCREENBREAK8:

			if (animwall[p].tag >= 0 && wall[i].extra != FEMPIC2 && wall[i].extra != FEMPIC3)
				wall[i].picnum = animwall[p].tag;
			else
			{
				wall[i].picnum++;
				if (wall[i].picnum == (SCREENBREAK6 + 3))
					wall[i].picnum = SCREENBREAK6;
			}
			continue;

		}

		if (wall[i].cstat & 16)
			switch (wall[i].overpicnum)
			{
			case W_FORCEFIELD:
			case W_FORCEFIELD + 1:
			case W_FORCEFIELD + 2:

				t = animwall[p].tag;

				if (wall[i].cstat & 254)
				{
					wall[i].addxpan(-t / 4096.f); // bcos(t, -12);
					wall[i].addypan(-t / 4096.f); // bsin(t, -12);

					if (wall[i].extra == 1)
					{
						wall[i].extra = 0;
						animwall[p].tag = 0;
					}
					else
						animwall[p].tag += 128;

					if (animwall[p].tag < (128 << 4))
					{
						if (animwall[p].tag & 128)
							wall[i].overpicnum = W_FORCEFIELD;
						else wall[i].overpicnum = W_FORCEFIELD + 1;
					}
					else
					{
						if ((krand() & 255) < 32)
							animwall[p].tag = 128 << (krand() & 3);
						else wall[i].overpicnum = W_FORCEFIELD + 1;
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
		if (act->s->lotag == low) switch (act->s->picnum)
		{
		case RESPAWN:
			if (badguypic(act->s->hitag) && ud.monsters_off) break;

			auto star = spawn(act, TRANSPORTERSTAR);
			star->s->z -= (32 << 8);

			act->s->extra = 66 - 12;   // Just a way to killit
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

bool checkhitswitch_d(int snum, int ww, DDukeActor *act)
{
	uint8_t switchpal;
	int i, x, lotag, hitag, picnum, correctdips, numdips;
	int sx, sy;

	if (ww < 0 && act == nullptr) return 0;
	correctdips = 1;
	numdips = 0;
	
	if (act)
	{
		lotag = act->s->lotag;
		if (lotag == 0) return 0;
		hitag = act->s->hitag;
		sx = act->s->x;
		sy = act->s->y;
		picnum = act->s->picnum;
		switchpal = act->s->pal;
	}
	else
	{
		auto wal = &wall[ww];
		lotag = wal->lotag;
		if (lotag == 0) return 0;
		hitag = wal->hitag;
		sx = wal->x;
		sy = wal->y;
		picnum = wal->picnum;
		switchpal = wal->pal;
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
			act->s->picnum = DEVELOPERCOMMENTARY;
			return true;
		}
		return false;
	case DEVELOPERCOMMENTARY: //Twentieth Anniversary World Tour
		if (act)
		{
			if (StartCommentary(lotag, act))
				act->s->picnum = DEVELOPERCOMMENTARY+1;
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
					ps[snum].access_wallnum = ww;
				else
					ps[snum].access_spritenum = act;
			}

			return 0;
		}
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
		auto si = other->s;
		if (lotag == si->lotag) switch (si->picnum)
		{
		case DIPSWITCH:
		case TECHSWITCH:
		case ALIENSWITCH:
			if (act && act == other) si->picnum++;
			else if (si->hitag == 0) correctdips++;
			numdips++;
			break;
		case TECHSWITCH + 1:
		case DIPSWITCH + 1:
		case ALIENSWITCH + 1:
			if (act && act == other) si->picnum--;
			else if (si->hitag == 1) correctdips++;
			numdips++;
			break;
		case MULTISWITCH:
		case MULTISWITCH + 1:
		case MULTISWITCH + 2:
		case MULTISWITCH + 3:
			si->picnum++;
			if (si->picnum > (MULTISWITCH + 3))
				si->picnum = MULTISWITCH;
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
			si->picnum++;
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
			si->picnum--;
			break;
		}
	}

	for (i = 0; i < numwalls; i++)
	{
		x = i;
		if (lotag == wall[x].lotag)
			switch (wall[x].picnum)
			{
			case DIPSWITCH:
			case TECHSWITCH:
			case ALIENSWITCH:
				if (!act && i == ww) wall[x].picnum++;
				else if (wall[x].hitag == 0) correctdips++;
				numdips++;
				break;
			case DIPSWITCH + 1:
			case TECHSWITCH + 1:
			case ALIENSWITCH + 1:
				if (!act && i == ww) wall[x].picnum--;
				else if (wall[x].hitag == 1) correctdips++;
				numdips++;
				break;
			case MULTISWITCH:
			case MULTISWITCH + 1:
			case MULTISWITCH + 2:
			case MULTISWITCH + 3:
				wall[x].picnum++;
				if (wall[x].picnum > (MULTISWITCH + 3))
					wall[x].picnum = MULTISWITCH;
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
				wall[x].picnum++;
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
				wall[x].picnum--;
				break;
			}
	}

	if (lotag == (short)65535)
	{
		setnextmap(false);
		return 1;
	}

	vec3_t v = { sx, sy, ps[snum].pos.z };
	switch (picnum)
	{
	default:
		if (fi.isadoorwall(picnum) == 0) break;
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
					S_PlaySound3D(ALIEN_SWITCH1, act, &v);
				else S_PlaySound3D(ALIEN_SWITCH1, ps[snum].GetActor(), &v);
			}
			else
			{
				if (act)
					S_PlaySound3D(SWITCH_ON, act, &v);
				else S_PlaySound3D(SWITCH_ON, ps[snum].GetActor(), &v);
			}
			if (numdips != correctdips) break;
			S_PlaySound3D(END_OF_LEVEL_WARN, ps[snum].GetActor(), &v);
		}
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

		DukeStatIterator it(STAT_EFFECTOR);
		while (auto other = it.Next())
		{
			if (other->s->hitag == lotag)
			{
				switch (other->s->lotag)
				{
				case SE_12_LIGHT_SWITCH:
					sector[other->s->sectnum].floorpal = 0;
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
				S_PlaySound3D(SWITCH_ON, act, &v);
			else S_PlaySound3D(SWITCH_ON, ps[snum].GetActor(), &v);
		}
		else if (hitag != 0)
		{
			auto flags = S_GetUserFlags(hitag);

			if (act && (flags & SF_TALK) == 0)
				S_PlaySound3D(hitag, act, &v);
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

void activatebysector_d(int sect, DDukeActor* activator)
{
	int didit = 0;

	DukeSectIterator it(sect);
	while (auto act = it.Next())
	{
		if (act->s->picnum == ACTIVATOR)
		{
			operateactivators(act->s->lotag, -1);
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

void checkhitwall_d(DDukeActor* spr, int dawallnum, int x, int y, int z, int atwith)
{
	int j, sn = -1, darkestwall;
	walltype* wal;

	wal = &wall[dawallnum];

	if (wal->overpicnum == MIRROR)
	{
		switch (atwith)
		{
		case HEAVYHBOMB:
		case RADIUSEXPLOSION:
		case RPG:
		case HYDRENT:
		case SEENINE:
		case OOZFILTER:
		case EXPLODINGBARREL:
			lotsofglass(spr, dawallnum, 70);
			wal->cstat &= ~16;
			wal->overpicnum = MIRRORBROKE;
			wal->portalflags = 0;
			S_PlayActorSound(GLASS_HEAVYBREAK, spr);
			return;
		}
	}

	if (((wal->cstat & 16) || wal->overpicnum == BIGFORCE) && wal->nextsector >= 0)
		if (wal->nextSector()->floorz > z)
			if (wal->nextSector()->floorz - wal->nextSector()->ceilingz)
				switch (wal->overpicnum)
				{
				case W_FORCEFIELD:
				case W_FORCEFIELD + 1:
				case W_FORCEFIELD + 2:
					wal->extra = 1; // tell the forces to animate
				case BIGFORCE:
				{
					updatesector(x, y, &sn);
					if (sn < 0) return;
					DDukeActor* spawned;
					if (atwith == -1)
						spawned = EGS(sn, x, y, z, FORCERIPPLE, -127, 8, 8, 0, 0, 0, spr, 5);
					else
					{
						if (atwith == CHAINGUN)
							spawned = EGS(sn, x, y, z, FORCERIPPLE, -127, 16 + spr->s->xrepeat, 16 + spr->s->yrepeat, 0, 0, 0, spr, 5);
						else spawned = EGS(sn, x, y, z, FORCERIPPLE, -127, 32, 32, 0, 0, 0, spr, 5);
					}

					spawned->s->cstat |= 18 + 128;
					spawned->s->ang = getangle(wal->x - wall[wal->point2].x,	wal->y - wall[wal->point2].y) - 512;

					S_PlayActorSound(SOMETHINGHITFORCE, spawned);

					return;
				}
				case FANSPRITE:
					wal->overpicnum = FANSPRITEBROKE;
					wal->cstat &= 65535 - 65;
					if (wal->nextwall >= 0)
					{
						wal->nextWall()->overpicnum = FANSPRITEBROKE;
						wal->nextWall()->cstat &= 65535 - 65;
					}
					S_PlayActorSound(VENT_BUST, spr);
					S_PlayActorSound(GLASS_BREAKING, spr);
					return;

				case GLASS:
				{
					updatesector(x, y, &sn); if (sn < 0) return;
					wal->overpicnum = GLASS2;
					lotsofglass(spr, dawallnum, 10);
					wal->cstat = 0;

					if (wal->nextwall >= 0)
						wal->nextWall()->cstat = 0;

					auto spawned = EGS(sn, x, y, z, SECTOREFFECTOR, 0, 0, 0, ps[0].angle.ang.asbuild(), 0, 0, spr, 3);
					spawned->s->lotag = 128; 
					spawned->temp_data[1] = 5;
					spawned->temp_data[2] = dawallnum;
					S_PlayActorSound(GLASS_BREAKING, spawned);
					return;
				}
				case STAINGLASS1:
					updatesector(x, y, &sn); if (sn < 0) return;
					lotsofcolourglass(spr, dawallnum, 80);
					wal->cstat = 0;
					if (wal->nextwall >= 0)
						wal->nextWall()->cstat = 0;
					S_PlayActorSound(VENT_BUST, spr);
					S_PlayActorSound(GLASS_BREAKING, spr);
					return;
				}

	switch (wal->picnum)
	{
	case COLAMACHINE:
	case VENDMACHINE:
		breakwall(wal->picnum + 2, spr, dawallnum);
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

		lotsofglass(spr, dawallnum, 30);
		wal->picnum = W_SCREENBREAK + (krand() % 3);
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;

	case W_TECHWALL5:
	case W_TECHWALL6:
	case W_TECHWALL7:
	case W_TECHWALL8:
	case W_TECHWALL9:
		breakwall(wal->picnum + 1, spr, dawallnum);
		return;
	case W_MILKSHELF:
		breakwall(W_MILKSHELFBROKE, spr, dawallnum);
		return;

	case W_TECHWALL10:
		breakwall(W_HITTECHWALL10, spr, dawallnum);
		return;

	case W_TECHWALL1:
	case W_TECHWALL11:
	case W_TECHWALL12:
	case W_TECHWALL13:
	case W_TECHWALL14:
		breakwall(W_HITTECHWALL1, spr, dawallnum);
		return;

	case W_TECHWALL15:
		breakwall(W_HITTECHWALL15, spr, dawallnum);
		return;

	case W_TECHWALL16:
		breakwall(W_HITTECHWALL16, spr, dawallnum);
		return;

	case W_TECHWALL2:
		breakwall(W_HITTECHWALL2, spr, dawallnum);
		return;

	case W_TECHWALL3:
		breakwall(W_HITTECHWALL3, spr, dawallnum);
		return;

	case W_TECHWALL4:
		breakwall(W_HITTECHWALL4, spr, dawallnum);
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
		lotsofglass(spr, dawallnum, 30);

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

		sn = wal->nextsector;
		if (sn < 0) return;
		darkestwall = 0;

		wal = &wall[sector[sn].wallptr];
		for (int i = sector[sn].wallnum; i > 0; i--, wal++)
			if (wal->shade > darkestwall)
				darkestwall = wal->shade;

		j = krand() & 1;
		DukeStatIterator it(STAT_EFFECTOR);
		while (auto effector = it.Next())
		{
			if (effector->s->hitag == wall[dawallnum].lotag && effector->s->lotag == 3)
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
		switch (coll.actor->s->picnum)
		{
		case CACTUS:
			if (p->hurt_delay < 8)
			{
				p->GetActor()->s->extra -= 5;
				p->hurt_delay = 16;
				SetPlayerPal(p, PalEntry(32, 32, 0, 0));
				S_PlayActorSound(DUKE_LONGTERM_PAIN, p->GetActor());
			}
			break;
		}
		return;
	}

	if (coll.type != kHitWall) return;
	int j = coll.index;

	if (p->hurt_delay > 0) p->hurt_delay--;
	else if (wall[j].cstat & 85) switch (wall[j].overpicnum)
	{
	case W_FORCEFIELD:
	case W_FORCEFIELD + 1:
	case W_FORCEFIELD + 2:
		p->GetActor()->s->extra -= 5;

		p->hurt_delay = 16;
		SetPlayerPal(p, PalEntry(32, 32, 0, 0));

		p->posxv = -p->angle.ang.bcos(8);
		p->posyv = -p->angle.ang.bsin(8);
		S_PlayActorSound(DUKE_LONGTERM_PAIN, p->GetActor());

		fi.checkhitwall(p->GetActor(), j,
			p->pos.x + p->angle.ang.bcos(-9),
			p->pos.y + p->angle.ang.bsin(-9),
			p->pos.z, -1);

		break;

	case BIGFORCE:
		p->hurt_delay = 26;
		fi.checkhitwall(p->GetActor(), j,
			p->pos.x + p->angle.ang.bcos(-9),
			p->pos.y + p->angle.ang.bsin(-9),
			p->pos.z, -1);
		break;

	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool checkhitceiling_d(int sn)
{
	auto sectp = &sector[sn];
	int j;

	switch (sectp->ceilingpicnum)
	{
	case WALLLIGHT1:
	case WALLLIGHT2:
	case WALLLIGHT3:
	case WALLLIGHT4:
	case TECHLIGHT2:
	case TECHLIGHT4:

		ceilingglass(ps[myconnectindex].GetActor(), sn, 10);
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
			DukeSectIterator it(sn);
			while (auto act = it.Next())
			{
				if (act->s->picnum == SECTOREFFECTOR && act->s->lotag == 12)
				{
					DukeStatIterator it1(STAT_EFFECTOR);
					while (auto act2 = it1.Next())
					{
						if (act2->s->hitag == act->s->hitag)
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
			if (act->s->hitag == (sectp->hitag) && act->s->lotag == 3)
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
	spritetype* s = targ->s;
	auto pspr = proj->s;

	switch (s->picnum)
	{
	case WTGLASS1:
	case WTGLASS2:
		if (!isWorldTour())
			break;
		S_PlayActorSound(GLASS_BREAKING, targ);
		lotsofglass(targ, -1, 10);
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
		if (pspr->picnum == QUEBALL || pspr->picnum == STRIPEBALL)
		{
			pspr->xvel = (s->xvel >> 1) + (s->xvel >> 2);
			pspr->ang -= (s->ang << 1) + 1024;
			s->ang = getangle(s->x - pspr->x, s->y - pspr->y) - 512;
			if (S_CheckSoundPlaying(POOLBALLHIT) < 2)
				S_PlayActorSound(POOLBALLHIT, targ);
		}
		else
		{
			if (krand() & 3)
			{
				s->xvel = 164;
				s->ang = pspr->ang;
			}
			else
			{
				lotsofglass(targ, -1, 3);
				deletesprite(targ);
			}
		}
		break;
	case TREE1:
	case TREE2:
	case TIRE:
	case CONE:
	case BOX:
		switch (pspr->picnum)
		{
		case RADIUSEXPLOSION:
		case RPG:
		case FIRELASER:
		case HYDRENT:
		case HEAVYHBOMB:
			if (targ->temp_data[0] == 0)
			{
				s->cstat &= ~257;
				targ->temp_data[0] = 1;
				spawn(targ, BURNING);
			}
			break;
		}
		break;
	case CACTUS:
		//		case CACTUSBROKE:
		switch (pspr->picnum)
		{
		case RADIUSEXPLOSION:
		case RPG:
		case FIRELASER:
		case HYDRENT:
		case HEAVYHBOMB:
			for (k = 0; k < 64; k++)
			{
				auto j = EGS(s->sectnum, s->x, s->y, s->z - (krand() % (48 << 8)), SCRAP3 + (krand() & 3), -8, 48, 48, krand() & 2047, (krand() & 63) + 64, -(krand() & 4095) - (s->zvel >> 2), targ, 5);
				j->s->pal = 8;
			}

			if (s->picnum == CACTUS)
				s->picnum = CACTUSBROKE;
			s->cstat &= ~257;
			//	   else deletesprite(i);
			break;
		}
		break;

	case HANGLIGHT:
	case GENERICPOLE2:
		for (k = 0; k < 6; k++)
			EGS(s->sectnum, s->x, s->y, s->z - (8 << 8), SCRAP1 + (krand() & 15), -8, 48, 48, krand() & 2047, (krand() & 63) + 64, -(krand() & 4095) - (s->zvel >> 2), targ, 5);
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		deletesprite(targ);
		break;


	case FANSPRITE:
		s->picnum = FANSPRITEBROKE;
		s->cstat &= (65535 - 257);
		if (s->sector()->floorpicnum == FANSHADOW)
			s->sector()->floorpicnum = FANSHADOWBROKE;

		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		for (j = 0; j < 16; j++) RANDOMSCRAP(targ);

		break;
	case WATERFOUNTAIN:
	case WATERFOUNTAIN + 1:
	case WATERFOUNTAIN + 2:
	case WATERFOUNTAIN + 3:
		s->picnum = WATERFOUNTAINBROKE;
		spawn(targ, TOILETWATER);
		break;
	case SATELITE:
	case FUELPOD:
	case SOLARPANNEL:
	case ANTENNA:
		if (gs.actorinfo[SHOTSPARK1].scriptaddress && pspr->extra != ScriptCode[gs.actorinfo[SHOTSPARK1].scriptaddress])
		{
			for (j = 0; j < 15; j++)
				EGS(s->sectnum, s->x, s->y, s->sector()->floorz - (12 << 8) - (j << 9), SCRAP1 + (krand() & 15), -8, 64, 64,
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
		if (s->picnum == BOTTLE10)
			fi.lotsofmoney(targ, 4 + (krand() & 3));
		else if (s->picnum == STATUE || s->picnum == STATUEFLASH)
		{
			lotsofcolourglass(targ, -1, 40);
			S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		}
		else if (s->picnum == VASE)
			lotsofglass(targ, -1, 40);

		S_PlayActorSound(GLASS_BREAKING, targ);
		s->ang = krand() & 2047;
		lotsofglass(targ, -1, 8);
		deletesprite(targ);
		break;
	case FETUS:
		s->picnum = FETUSBROKE;
		S_PlayActorSound(GLASS_BREAKING, targ);
		lotsofglass(targ, -1, 10);
		break;
	case FETUSBROKE:
		for (j = 0; j < 48; j++)
		{
			fi.shoot(targ, BLOODSPLAT1);
			s->ang += 333;
		}
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		S_PlayActorSound(SQUISHED, targ);
	case BOTTLE7:
		S_PlayActorSound(GLASS_BREAKING, targ);
		lotsofglass(targ, -1, 10);
		deletesprite(targ);
		break;
	case HYDROPLANT:
		s->picnum = BROKEHYDROPLANT;
		S_PlayActorSound(GLASS_BREAKING, targ);
		lotsofglass(targ, -1, 10);
		break;

	case FORCESPHERE:
		s->xrepeat = 0;
		if (targ->GetOwner())
		{
			targ->GetOwner()->temp_data[0] = 32;
			targ->GetOwner()->temp_data[1] = !targ->GetOwner()->temp_data[1];
			targ->GetOwner()->temp_data[2] ++;
		}
		spawn(targ, EXPLOSION2);
		break;

	case BROKEHYDROPLANT:
		if (s->cstat & 1)
		{
			S_PlayActorSound(GLASS_BREAKING, targ);
			s->z += 16 << 8;
			s->cstat = 0;
			lotsofglass(targ, -1, 5);
		}
		break;

	case TOILET:
		s->picnum = TOILETBROKE;
		s->cstat |= (krand() & 1) << 2;
		s->cstat &= ~257;
		spawn(targ, TOILETWATER);
		S_PlayActorSound(GLASS_BREAKING, targ);
		break;

	case STALL:
		s->picnum = STALLBROKE;
		s->cstat |= (krand() & 1) << 2;
		s->cstat &= ~257;
		spawn(targ, TOILETWATER);
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;

	case HYDRENT:
		s->picnum = BROKEFIREHYDRENT;
		spawn(targ, TOILETWATER);
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;

	case GRATE1:
		s->picnum = BGRATE1;
		s->cstat &= (65535 - 256 - 1);
		S_PlayActorSound(VENT_BUST, targ);
		break;

	case CIRCLEPANNEL:
		s->picnum = CIRCLEPANNELBROKE;
		s->cstat &= (65535 - 256 - 1);
		S_PlayActorSound(VENT_BUST, targ);
		break;
	case PANNEL1:
	case PANNEL2:
		s->picnum = BPANNEL1;
		s->cstat &= (65535 - 256 - 1);
		S_PlayActorSound(VENT_BUST, targ);
		break;
	case PANNEL3:
		s->picnum = BPANNEL3;
		s->cstat &= (65535 - 256 - 1);
		S_PlayActorSound(VENT_BUST, targ);
		break;
	case PIPE1:
	case PIPE2:
	case PIPE3:
	case PIPE4:
	case PIPE5:
	case PIPE6:
		switch (s->picnum)
		{
		case PIPE1:s->picnum = PIPE1B; break;
		case PIPE2:s->picnum = PIPE2B; break;
		case PIPE3:s->picnum = PIPE3B; break;
		case PIPE4:s->picnum = PIPE4B; break;
		case PIPE5:s->picnum = PIPE5B; break;
		case PIPE6:s->picnum = PIPE6B; break;
		}
		{
			auto j = spawn(targ, STEAM);
			j->s->z = s->sector()->floorz - (32 << 8);
		}
		break;

	case MONK:
	case LUKE:
	case INDY:
	case JURYGUY:
		S_PlayActorSound(s->lotag, targ);
		spawn(targ, s->hitag);
	case SPACEMARINE:
	{
		s->extra -= pspr->extra;
		if (s->extra > 0) break;
		s->ang = krand() & 2047;
		fi.shoot(targ, BLOODSPLAT1);
		s->ang = krand() & 2047;
		fi.shoot(targ, BLOODSPLAT2);
		s->ang = krand() & 2047;
		fi.shoot(targ, BLOODSPLAT3);
		s->ang = krand() & 2047;
		fi.shoot(targ, BLOODSPLAT4);
		s->ang = krand() & 2047;
		fi.shoot(targ, BLOODSPLAT1);
		s->ang = krand() & 2047;
		fi.shoot(targ, BLOODSPLAT2);
		s->ang = krand() & 2047;
		fi.shoot(targ, BLOODSPLAT3);
		s->ang = krand() & 2047;
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
		s->picnum = BROKENCHAIR;
		s->cstat = 0;
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
		s = targ->s;
	default:
		if ((s->cstat & 16) && s->hitag == 0 && s->lotag == 0 && s->statnum == 0)
			break;

		if ((pspr->picnum == FREEZEBLAST || proj->GetOwner() != targ) && s->statnum != 4)
		{
			if (badguy(targ) == 1)
			{
				if (isWorldTour() && s->picnum == FIREFLY && s->xrepeat < 48)
					break;

				if (pspr->picnum == RPG) pspr->extra <<= 1;

				if ((s->picnum != DRONE) && (s->picnum != ROTATEGUN) && (s->picnum != COMMANDER) && (s->picnum < GREENSLIME || s->picnum > GREENSLIME + 7))
					if (pspr->picnum != FREEZEBLAST)
						//if (actortype[s->picnum] == 0) //TRANSITIONAL. Cannot be done right with EDuke mess backing the engine. 
						{
							auto spawned = spawn(proj, JIBS6);
							if (pspr->pal == 6)
								spawned->s->pal = 6;
							spawned->s->z += (4 << 8);
							spawned->s->xvel = 16;
							spawned->s->xrepeat = spawned->s->yrepeat = 24;
							spawned->s->ang += 32 - (krand() & 63);
						}

				auto Owner = proj->GetOwner();

				if (Owner && Owner->s->picnum == APLAYER && s->picnum != ROTATEGUN && s->picnum != DRONE)
					if (ps[Owner->PlayerIndex()].curr_weapon == SHOTGUN_WEAPON)
					{
						fi.shoot(targ, BLOODSPLAT3);
						fi.shoot(targ, BLOODSPLAT1);
						fi.shoot(targ, BLOODSPLAT2);
						fi.shoot(targ, BLOODSPLAT4);
					}

				if (s->picnum != TANK && !bossguy(targ) && s->picnum != RECON && s->picnum != ROTATEGUN)
				{
					if ((s->cstat & 48) == 0)
						s->ang = (pspr->ang + 1024) & 2047;
					s->xvel = -(pspr->extra << 2);
					j = s->sectnum;
					pushmove(&s->pos, &j, 128L, (4 << 8), (4 << 8), CLIPMASK0);
					if (j != s->sectnum && j >= 0 && j < MAXSECTORS)
						changeactorsect(targ, j);
				}

				if (s->statnum == 2)
				{
					changeactorstat(targ, 1);
					targ->timetosleep = SLEEPTIME;
				}
				if ((s->xrepeat < 24 || s->picnum == SHARK) && pspr->picnum == SHRINKSPARK) return;
			}

			if (s->statnum != 2)
			{
				if (pspr->picnum == FREEZEBLAST && ((s->picnum == APLAYER && s->pal == 1) || (gs.freezerhurtowner == 0 && proj->GetOwner() == targ)))
					return;


				int hitpic = pspr->picnum;
				auto Owner = proj->GetOwner();
				if (Owner && Owner->s->picnum == APLAYER)
				{
					if (s->picnum == APLAYER && ud.coop != 0 && ud.ffire == 0)
						return;

					auto tOwner = targ->GetOwner();
					if (isWorldTour() && hitpic == FIREBALL && tOwner && tOwner->s->picnum != FIREBALL)
						hitpic = FLAMETHROWERFLAME;
				}

				targ->picnum = hitpic;
				targ->extra += pspr->extra;
				targ->ang = pspr->ang;
				targ->SetHitOwner(Owner);
			}

			if (s->statnum == 10)
			{
				p = s->yvel;
				if (ps[p].newOwner != nullptr)
				{
					ps[p].newOwner = nullptr;
					ps[p].pos.x = ps[p].oposx;
					ps[p].pos.y = ps[p].oposy;
					ps[p].pos.z = ps[p].oposz;
					ps[p].angle.restore();

					updatesector(ps[p].pos.x, ps[p].pos.y, &ps[p].cursectnum);

					DukeStatIterator it(STAT_ACTOR);
					while (auto j = it.Next())
					{
						if (j->s->picnum == CAMERA1) j->s->yvel = 0;
					}
				}

				if (s->xrepeat < 24 && pspr->picnum == SHRINKSPARK)
					return;

				auto hitowner = targ->GetHitOwner();
				if (!hitowner || hitowner->s->picnum != APLAYER)
					if (ud.player_skill >= 3)
						pspr->extra += (pspr->extra >> 1);
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

void checksectors_d(int snum)
{
	int i = -1, oldz;
	struct player_struct* p;
	int j, hitscanwall;
	int neartagsector, neartagwall;
	DDukeActor* neartagsprite;
	int neartaghitdist;

	p = &ps[snum];
	auto pact = p->GetActor();

	switch (p->cursector()->lotag)
	{

	case 32767:
		p->cursector()->lotag = 0;
		FTA(9, p);
		p->secret_rooms++;
		SECRET_Trigger(p->cursectnum);
		return;
	case -1:
		p->cursector()->lotag = 0;
		setnextmap(false);
		return;
	case -2:
		p->cursector()->lotag = 0;
		p->timebeforeexit = 26 * 8;
		p->customexitsound = p->cursector()->hitag;
		return;
	default:
		if (p->cursector()->lotag >= 10000 && p->cursector()->lotag < 16383)
		{
			if (snum == screenpeek || ud.coop == 1)
				S_PlayActorSound(p->cursector()->lotag - 10000, pact);
			p->cursector()->lotag = 0;
		}
		break;

	}

	//After this point the the player effects the map with space

	if (chatmodeon || p->GetActor()->s->extra <= 0) return;

	if (ud.cashman && PlayerInput(snum, SB_OPEN))
		fi.lotsofmoney(p->GetActor(), 2);

	if (p->newOwner != nullptr)
	{
		if (abs(PlayerInputSideVel(snum)) > 768 || abs(PlayerInputForwardVel(snum)) > 768)
		{
			i = -1;
			goto CLEARCAMERAS;
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
				i = -1;
				goto CLEARCAMERAS;
			}
			return;
		}

		neartagsprite = nullptr;
		p->toggle_key_flag = 1;
		hitscanwall = -1;

		i = hitawall(p, &hitscanwall);

		if (i < 1280 && hitscanwall >= 0 && wall[hitscanwall].overpicnum == MIRROR)
			if (wall[hitscanwall].lotag > 0 && S_CheckSoundPlaying(wall[hitscanwall].lotag) == 0 && snum == screenpeek)
			{
				S_PlayActorSound(wall[hitscanwall].lotag, pact);
				return;
			}

		if (hitscanwall >= 0 && (wall[hitscanwall].cstat & 16))
			if (wall[hitscanwall].lotag)
				return;

		if (p->newOwner != nullptr)
			neartag(p->oposx, p->oposy, p->oposz, p->GetActor()->s->sectnum, p->angle.oang.asbuild(), &neartagsector, &neartagwall, &neartagsprite, &neartaghitdist, 1280L, 1);
		else
		{
			neartag(p->pos.x, p->pos.y, p->pos.z, p->GetActor()->s->sectnum, p->angle.oang.asbuild(), &neartagsector, &neartagwall, &neartagsprite, &neartaghitdist, 1280L, 1);
			if (neartagsprite == nullptr && neartagwall == -1 && neartagsector == -1)
				neartag(p->pos.x, p->pos.y, p->pos.z + (8 << 8), p->GetActor()->s->sectnum, p->angle.oang.asbuild(), &neartagsector, &neartagwall, &neartagsprite, &neartaghitdist, 1280L, 1);
			if (neartagsprite == nullptr && neartagwall == -1 && neartagsector == -1)
				neartag(p->pos.x, p->pos.y, p->pos.z + (16 << 8), p->GetActor()->s->sectnum, p->angle.oang.asbuild(), &neartagsector, &neartagwall, &neartagsprite, &neartaghitdist, 1280L, 1);
			if (neartagsprite == nullptr && neartagwall == -1 && neartagsector == -1)
			{
				neartag(p->pos.x, p->pos.y, p->pos.z + (16 << 8), p->GetActor()->s->sectnum, p->angle.oang.asbuild(), &neartagsector, &neartagwall, &neartagsprite, &neartaghitdist, 1280L, 3);
				if (neartagsprite != nullptr)
				{
					switch (neartagsprite->s->picnum)
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

				neartagsprite = nullptr;
				neartagwall = -1;
				neartagsector = -1;
			}
		}

		if (p->newOwner == nullptr && neartagsprite == nullptr && neartagsector == -1 && neartagwall == -1)
			if (isanunderoperator(p->GetActor()->getSector()->lotag))
				neartagsector = p->GetActor()->s->sectnum;

		if (neartagsector >= 0 && (sector[neartagsector].lotag & 16384))
			return;

		if (neartagsprite == nullptr && neartagwall == -1)
			if (p->cursector()->lotag == 2)
			{
				DDukeActor* hit;
				oldz = hitasprite(p->GetActor(), &hit);
				if (hit) neartagsprite = hit;
				if (oldz > 1280) neartagsprite = nullptr;

			}

		if (neartagsprite != nullptr)
		{
			if (fi.checkhitswitch(snum, -1, neartagsprite)) return;

			switch (neartagsprite->s->picnum)
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
					if (p->GetActor()->s->extra <= (gs.max_player_health - (gs.max_player_health / 10)))
					{
						p->GetActor()->s->extra += gs.max_player_health / 10;
						p->last_extra = p->GetActor()->s->extra;
					}
					else if (p->GetActor()->s->extra < gs.max_player_health)
						p->GetActor()->s->extra = gs.max_player_health;
				}
				else if (S_CheckActorSoundPlaying(neartagsprite, FLUSH_TOILET) == 0)
					S_PlayActorSound(FLUSH_TOILET, neartagsprite);
				return;

			case NUKEBUTTON:

				hitawall(p, &j);
				if (j >= 0 && wall[j].overpicnum == 0)
					if (neartagsprite->temp_data[0] == 0)
					{
						neartagsprite->temp_data[0] = 1;
						neartagsprite->SetOwner(p->GetActor());
						p->buttonpalette = neartagsprite->s->pal;
						if (p->buttonpalette)
							ud.secretlevel = neartagsprite->s->lotag;
						else ud.secretlevel = 0;
					}
				return;
			case WATERFOUNTAIN:
				if (neartagsprite->temp_data[0] != 1)
				{
					neartagsprite->temp_data[0] = 1;
					neartagsprite->SetOwner(p->GetActor());

					if (p->GetActor()->s->extra < gs.max_player_health)
					{
						p->GetActor()->s->extra++;
						S_PlayActorSound(DUKE_DRINKING, p->GetActor());
					}
				}
				return;
			case PLUG:
				S_PlayActorSound(SHORT_CIRCUIT, pact);
				p->GetActor()->s->extra -= 2 + (krand() & 3);
				SetPlayerPal(p, PalEntry(32, 48, 48, 64));
				break;
			case VIEWSCREEN:
			case VIEWSCREEN2:
			{
				DukeStatIterator it(STAT_ACTOR);
				while (auto acti = it.Next())
				{
					auto spr = acti->s;
					if (spr->picnum == CAMERA1 && spr->yvel == 0 && neartagsprite->s->hitag == spr->lotag)
					{
						spr->yvel = 1; //Using this camera
						if (snum == screenpeek) S_PlaySound(MONITOR_ACTIVE);

						neartagsprite->SetOwner(acti);
						neartagsprite->s->yvel = 1;
						camsprite = neartagsprite;


						j = p->cursectnum;
						p->cursectnum = spr->sectnum;
						p->cursectnum = j;

						// parallaxtype = 2;
						p->newOwner = acti;
						return;
					}
				}
				i = -1;
			}

		CLEARCAMERAS:

			if (i < 0)
			{
				p->pos.x = p->oposx;
				p->pos.y = p->oposy;
				p->pos.z = p->oposz;
				p->newOwner = nullptr;

				updatesector(p->pos.x, p->pos.y, &p->cursectnum);

				DukeStatIterator it(STAT_ACTOR);
				while (auto act = it.Next())
				{
					if (act->s->picnum == CAMERA1) act->s->yvel = 0;
				}
			}
			else if (p->newOwner != nullptr)
				p->newOwner = nullptr;

			return;
			}
		}

		if (!PlayerInput(snum, SB_OPEN)) return;
		else if (p->newOwner != nullptr) { i = -1; goto CLEARCAMERAS; }

		if (neartagwall == -1 && neartagsector == -1 && neartagsprite == nullptr)
			if (abs(hits(p->GetActor())) < 512)
			{
				if ((krand() & 255) < 16)
					S_PlayActorSound(DUKE_SEARCH2, pact);
				else S_PlayActorSound(DUKE_SEARCH, pact);
				return;
			}

		if (neartagwall >= 0)
		{
			if (wall[neartagwall].lotag > 0 && fi.isadoorwall(wall[neartagwall].picnum))
			{
				if (hitscanwall == neartagwall || hitscanwall == -1)
					fi.checkhitswitch(snum, neartagwall, nullptr);
				return;
			}
			else if (p->newOwner != nullptr)
			{
				i = -1;
				goto CLEARCAMERAS;
			}
		}

		if (neartagsector >= 0 && (sector[neartagsector].lotag & 16384) == 0 && isanearoperator(sector[neartagsector].lotag))
		{
			DukeSectIterator it(neartagsector);
			while (auto act = it.Next())
			{
				if (act->s->picnum == ACTIVATOR || act->s->picnum == MASTERSWITCH)
					return;
			}
			operatesectors(neartagsector, p->GetActor());
		}
		else if ((p->GetActor()->getSector()->lotag & 16384) == 0)
		{
			if (isanunderoperator(p->GetActor()->getSector()->lotag))
			{
				DukeSectIterator it(p->GetActor()->s->sectnum);
				while (auto act = it.Next())
				{
					if (act->s->picnum == ACTIVATOR || act->s->picnum == MASTERSWITCH) return;
				}
				operatesectors(p->GetActor()->s->sectnum, p->GetActor());
			}
			else fi.checkhitswitch(snum, neartagwall, nullptr);
		}
	}
}





END_DUKE_NS
