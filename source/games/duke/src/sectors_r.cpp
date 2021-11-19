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
#include "sounds.h"
#include "names_r.h"
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

bool isadoorwall_r(int dapic)
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
		case RRTILE1856:
		case RRTILE1877:
			return 1;
	}
	return 0;
}

bool isablockdoor(int dapic)
{
	switch (dapic)
	{
		case RRTILE1792:
		case RRTILE1801:
		case RRTILE1805:
		case RRTILE1807:
		case RRTILE1808:
		case RRTILE1812:
		case RRTILE1821:
		case RRTILE1826:
		case RRTILE1850:
		case RRTILE1851:
		case RRTILE1856:
		case RRTILE1877:
		case RRTILE1938:
		case RRTILE1942:
		case RRTILE1944:
		case RRTILE1945:
		case RRTILE1951:
		case RRTILE1961:
		case RRTILE1964:
		case RRTILE1985:
		case RRTILE1995:
		case RRTILE2022:
		case RRTILE2052:
		case RRTILE2053:
		case RRTILE2060:
		case RRTILE2074:
		case RRTILE2132:
		case RRTILE2136:
		case RRTILE2139:
		case RRTILE2150:
		case RRTILE2178:
		case RRTILE2186:
		case RRTILE2319:
		case RRTILE2321:
		case RRTILE2326:
		case RRTILE2329:
		case RRTILE2578:
		case RRTILE2581:
		case RRTILE2610:
		case RRTILE2613:
		case RRTILE2621:
		case RRTILE2622:
		case RRTILE2676:
		case RRTILE2732:
		case RRTILE2831:
		case RRTILE2832:
		case RRTILE2842:
		case RRTILE2940:
		case RRTILE2970:
		case RRTILE3083:
		case RRTILE3100:
		case RRTILE3155:
		case RRTILE3195:
		case RRTILE3232:
		case RRTILE3600:
		case RRTILE3631:
		case RRTILE3635:
		case RRTILE3637:
		case RRTILE3643+2:
		case RRTILE3643+3:
		case RRTILE3647:
		case RRTILE3652:
		case RRTILE3653:
		case RRTILE3671:
		case RRTILE3673:
		case RRTILE3684:
		case RRTILE3708:
		case RRTILE3714:
		case RRTILE3716:
		case RRTILE3723:
		case RRTILE3725:
		case RRTILE3737:
		case RRTILE3754:
		case RRTILE3762:
		case RRTILE3763:
		case RRTILE3764:
		case RRTILE3765:
		case RRTILE3767:
		case RRTILE3793:
		case RRTILE3814:
		case RRTILE3815:
		case RRTILE3819:
		case RRTILE3827:
		case RRTILE3837:
			return true;
			
		case RRTILE1996:
		case RRTILE2382:
		case RRTILE2961:
		case RRTILE3804:
		case RRTILE7430:
		case RRTILE7467:
		case RRTILE7469:
		case RRTILE7470:
		case RRTILE7475:
		case RRTILE7566:
		case RRTILE7576:
		case RRTILE7716:
		case RRTILE8063:
		case RRTILE8067:
		case RRTILE8076:
		case RRTILE8106:
		case RRTILE8379:
		case RRTILE8380:
		case RRTILE8565:
		case RRTILE8605:
			return isRRRA();
	}
	return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void animatewalls_r(void)
{
	int t;

	if (isRRRA() &&ps[screenpeek].sea_sick_stat == 1)
	{
		for (auto& wal : walls())
		{
			if (wal.picnum == RRTILE7873)
				wal.addxpan(6);
			else if (wal.picnum == RRTILE7870)
				wal.addxpan(6);
		}
	}

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

			if ((krand() & 255) < 16)
			{
				animwall[p].tag = wal->picnum;
				wal->picnum = SCREENBREAK6;
			}

			continue;

		case SCREENBREAK6:
		case SCREENBREAK7:
		case SCREENBREAK8:

			if (animwall[p].tag >= 0)
				wal->picnum = animwall[p].tag;
			else
			{
				wal->picnum++;
				if (wal->picnum == (SCREENBREAK6 + 3))
					wal->picnum = SCREENBREAK6;
			}
			continue;

		}

		if (wal->cstat & 16)
			switch (wal->overpicnum)
			{
			case W_FORCEFIELD:
			case W_FORCEFIELD + 1:
			case W_FORCEFIELD + 2:

				t = animwall[p].tag;

				if (wal->cstat & 254)
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

void operaterespawns_r(int low)
{
	DukeStatIterator it(STAT_FX);
	while (auto act = it.Next())
	{
		if (act->s->lotag == low) switch (act->s->picnum)
		{
		case RESPAWN:
		{
			if (badguypic(act->s->hitag) && ud.monsters_off) break;

			auto star = spawn(act, TRANSPORTERSTAR);
			if (star) star->s->z -= (32 << 8);

			act->s->extra = 66 - 12;   // Just a way to killit
			break;
		}
		case RRTILE7424:
			if (isRRRA() && !ud.monsters_off)
				changeactorstat(act, 119);
			break;

		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void operateforcefields_r(DDukeActor* act, int low)
{
	operateforcefields_common(act, low, { BIGFORCE });
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool checkhitswitch_r(int snum, walltype* wwal, DDukeActor* act)
{
	uint8_t switchpal;
	int lotag, hitag, picnum, correctdips, numdips;
	int sx, sy;

	if (wwal == nullptr && act == nullptr) return 0;
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
		lotag = wwal->lotag;
		if (lotag == 0) return 0;
		hitag = wwal->hitag;
		sx = wwal->x;
		sy = wwal->y;
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
	case ACCESSSWITCH:
	case ACCESSSWITCH2:
		if (ps[snum].access_incs == 0)
		{
			if (switchpal == 0)
			{
				if (ps[snum].keys[1])
					ps[snum].access_incs = 1;
				else
				{
					FTA(70, &ps[snum]);
					if (isRRRA()) S_PlayActorSound(99, act? act : ps[snum].GetActor());
				}
			}

			else if (switchpal == 21)
			{
				if (ps[snum].keys[2])
					ps[snum].access_incs = 1;
				else
				{
					FTA(71, &ps[snum]);
					if (isRRRA()) S_PlayActorSound(99, act ? act : ps[snum].GetActor());
				}
			}

			else if (switchpal == 23)
			{
				if (ps[snum].keys[3])
					ps[snum].access_incs = 1;
				else
				{
					FTA(72, &ps[snum]);
					if (isRRRA()) S_PlayActorSound(99, act ? act : ps[snum].GetActor());
				}
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
		goto goOn1;

	case MULTISWITCH2:
	case MULTISWITCH2 + 1:
	case MULTISWITCH2 + 2:
	case MULTISWITCH2 + 3:
	case RRTILE8464:
	case RRTILE8660:
		if (!isRRRA()) break;
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
	case NUKEBUTTON:
	case NUKEBUTTON + 1:
	case RRTILE2214:
	case RRTILE2697:
	case RRTILE2697 + 1:
	case RRTILE2707:
	case RRTILE2707 + 1:
		goOn1:
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
		case MULTISWITCH2:
		case MULTISWITCH2 + 1:
		case MULTISWITCH2 + 2:
		case MULTISWITCH2 + 3:
			if (!isRRRA()) break;
			si->picnum++;
			if (si->picnum > (MULTISWITCH2 + 3))
				si->picnum = MULTISWITCH2;
			break;

		case RRTILE2214:
			//if (ud.level_numbe r > 6) ud.level_numbe r = 0; ??? Looks like some leftover garbage.
			si->picnum++;
			break;
		case RRTILE8660:
			if (!isRRRA()) break;
			[[fallthrough]];
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
		case NUKEBUTTON:
		case RRTILE2697:
		case RRTILE2707:
			if (si->picnum == DIPSWITCH3)
				if (si->hitag == 999)
				{
					DukeStatIterator it1(107);
					while (auto other2 = it1.Next())
					{
						if (other2->s->picnum == RRTILE3410)
						{
							other2->s->picnum++;
							other2->s->hitag = 100;
							other2->s->extra = 0;
							S_PlayActorSound(474, other2);
						}
						else if (other2->s->picnum == RRTILE295)
							deletesprite(other2);
					}
					si->picnum++;
					break;
				}
			if (si->picnum == NUKEBUTTON)
				chickenplant = 0;
			if (si->picnum == RRTILE8660)
			{
				BellTime = 132;
				BellSprite = other;
			}
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
		case NUKEBUTTON + 1:
		case RRTILE2697 + 1:
		case RRTILE2707 + 1:
			if (si->picnum == NUKEBUTTON + 1)
				chickenplant = 1;
			if (si->hitag != 999)
				si->picnum--;
			break;
		}
	}

	for (auto& wal : walls())
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
			case MULTISWITCH2:
			case MULTISWITCH2 + 1:
			case MULTISWITCH2 + 2:
			case MULTISWITCH2 + 3:
				if (!isRRRA()) break;
				wal.picnum++;
				if (wal.picnum > (MULTISWITCH2 + 3))
					wal.picnum = MULTISWITCH2;
				break;
			case RRTILE8660:
				if (!isRRRA()) break;
				[[fallthrough]];
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
			case RRTILE2697:
			case RRTILE2707:
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
			case RRTILE2697 + 1:
			case RRTILE2707 + 1:
				wal.picnum--;
				break;
			}
	}

	if (lotag == -1)
	{
		setnextmap(false);
	}

	vec3_t v = { sx, sy, ps[snum].pos.z };
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
		goto goOn2;
	case MULTISWITCH2:
	case MULTISWITCH2 + 1:
	case MULTISWITCH2 + 2:
	case MULTISWITCH2 + 3:
	case RRTILE8464:
	case RRTILE8660:
		if (!isRRRA()) break;
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
	case RRTILE2697:
	case RRTILE2697 + 1:
	case RRTILE2707:
	case RRTILE2707 + 1:
		goOn2:
		if (isRRRA())
		{
			if (picnum == RRTILE8660 && act)
			{
				BellTime = 132;
				BellSprite = act;
				act->s->picnum++;
			}
			else if (picnum == RRTILE8464)
			{
				act->s->picnum = act->s->picnum + 1;
				if (hitag == 10001)
				{
					if (ps[snum].SeaSick == 0)
						ps[snum].SeaSick = 350;
					operateactivators(668, snum);
					operatemasterswitches(668);
					S_PlayActorSound(328, ps[snum].GetActor());
					return 1;
				}
			}
			else if (hitag == 10000)
			{
				if (picnum == MULTISWITCH || picnum == (MULTISWITCH + 1) ||
					picnum == (MULTISWITCH + 2) || picnum == (MULTISWITCH + 3) ||
					picnum == MULTISWITCH2 || picnum == (MULTISWITCH2 + 1) ||
					picnum == (MULTISWITCH2 + 2) || picnum == (MULTISWITCH2 + 3))
				{
					DDukeActor* switches[3];
					int switchcount = 0, j;
					S_PlaySound3D(SWITCH_ON, act, &v);
					DukeLinearSpriteIterator it;
					while (auto actt = it.Next())
					{
						int jpn = actt->s->picnum;
						int jht = actt->s->hitag;
						if ((jpn == MULTISWITCH || jpn == MULTISWITCH2) && jht == 10000)
						{
							if (switchcount < 3)
							{
								switches[switchcount] = actt;
								switchcount++;
							}
						}
					}
					if (switchcount == 3)
					{
						S_PlaySound3D(78, act, &v);
						for (j = 0; j < switchcount; j++)
						{
							switches[j]->s->hitag = 0;
							if (picnum >= MULTISWITCH2)
								switches[j]->s->picnum = MULTISWITCH2 + 3;
							else
								switches[j]->s->picnum = MULTISWITCH + 3;
							checkhitswitch_r(snum, nullptr, switches[j]);
						}
					}
					return 1;
				}
			}
		}
		if (picnum == MULTISWITCH || picnum == (MULTISWITCH + 1) ||
			picnum == (MULTISWITCH + 2) || picnum == (MULTISWITCH + 3))
			lotag += picnum - MULTISWITCH;
		if (isRRRA())
		{
			if (picnum == MULTISWITCH2 || picnum == (MULTISWITCH2 + 1) ||
				picnum == (MULTISWITCH2 + 2) || picnum == (MULTISWITCH2 + 3))
				lotag += picnum - MULTISWITCH2;
		}

		DukeStatIterator it(STAT_EFFECTOR);
		while (auto other = it.Next())
		{
			if (other->s->hitag == lotag)
			{
				switch (other->s->lotag)
				{
				case 46:
				case SE_47_LIGHT_SWITCH:
				case SE_48_LIGHT_SWITCH:
					if (!isRRRA()) break;
					[[fallthrough]];
				case SE_12_LIGHT_SWITCH:
					other->getSector()->floorpal = 0;
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

void activatebysector_r(sectortype* sect, DDukeActor* activator)
{
	DukeSectIterator it(sect);
	while (auto act = it.Next())
	{
		if (act->s->picnum == ACTIVATOR)
		{
			operateactivators(act->s->lotag, -1);
			//			return;
		}
	}

	if (sect->lotag != 22)
		operatesectors(sect, activator);
}


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

static void lotsofpopcorn(DDukeActor *actor, walltype* wal, int n)
{
	int j, z;
	int a;

	sectortype* sect = nullptr;
	auto sp = actor->s;

	if (wal == nullptr)
	{
		for (j = n - 1; j >= 0; j--)
		{
			a = sp->ang - 256 + (krand() & 511) + 1024;
			EGS(sp->sector(), sp->x, sp->y, sp->z, POPCORN, -32, 36, 36, a, 32 + (krand() & 63), 1024 - (krand() & 1023), actor, 5);
		}
		return;
	}

	j = n + 1;

	int x1 = wal->x;
	int y1 = wal->y;

	auto delta = wal->delta();

	x1 -= Sgn(delta.x);
	y1 += Sgn(delta.y);

	delta.x /= j;
	delta.y /= j;

	for (j = n; j > 0; j--)
	{
		x1 += delta.x;
		y1 += delta.y;

		updatesector(x1, y1, &sect);
		if (sect)
		{
			z = sect->floorz - (krand() & (abs(sect->ceilingz - sect->floorz)));
			if (z < -(32 << 8) || z >(32 << 8))
				z = sp->z - (32 << 8) + (krand() & ((64 << 8) - 1));
			a = sp->ang - 1024;
			EGS(sp->sector(), x1, y1, z, POPCORN, -32, 36, 36, a, 32 + (krand() & 63), -(krand() & 1023), actor, 5);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checkhitwall_r(DDukeActor* spr, walltype* wal, int x, int y, int z, int atwith)
{
	int j;
	int sn = -1, darkestwall;
	spritetype* s;

	if (wal->overpicnum == MIRROR)
	{
		switch (atwith)
		{
		case RPG2:
			if (!isRRRA()) break;
			[[fallthrough]];
		case HEAVYHBOMB:
		case RADIUSEXPLOSION:
		case RPG:
		case HYDRENT:
		case SEENINE:
		case OOZFILTER:
		case EXPLODINGBARREL:
			lotsofglass(spr, wal, 70);
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

				case RRTILE1973:
				{
					sectortype* sptr = nullptr;
					updatesector(x, y, &sptr);
					if (sptr == nullptr) return;
					wal->overpicnum = GLASS2;
					lotsofpopcorn(spr, wal, 64);
					wal->cstat = 0;

					if (wal->nextwall >= 0)
						wal->nextWall()->cstat = 0;

					auto spawned = EGS(sptr, x, y, z, SECTOREFFECTOR, 0, 0, 0, ps[0].angle.ang.asbuild(), 0, 0, spr, 3);
					if (spawned)
					{
						spawned->s->lotag = 128;
						spawned->temp_walls[0] = wal;
						S_PlayActorSound(GLASS_BREAKING, spawned);
					}
					return;
				}
				case GLASS:
				{
					sectortype* sptr = nullptr;
					updatesector(x, y, &sptr);
					if (sptr == nullptr) return;
					wal->overpicnum = GLASS2;
					lotsofglass(spr, wal, 10);
					wal->cstat = 0;

					if (wal->nextwall >= 0)
						wal->nextWall()->cstat = 0;

					auto spawned = EGS(sptr, x, y, z, SECTOREFFECTOR, 0, 0, 0, ps[0].angle.ang.asbuild(), 0, 0, spr, 3);
					if (spawned)
					{
						spawned->s->lotag = 128;
						spawned->temp_data[1] = 2;
						spawned->temp_walls[0] = wal;
						S_PlayActorSound(GLASS_BREAKING, spawned);
					}
					return;
				}
				case STAINGLASS1:
					updatesector(x, y, &sn); if (sn < 0) return;
					lotsofcolourglass(spr, wal, 80);
					wal->cstat = 0;
					if (wal->nextwall >= 0)
						wal->nextWall()->cstat = 0;
					S_PlayActorSound(VENT_BUST, spr);
					S_PlayActorSound(GLASS_BREAKING, spr);
					return;
				}

	switch (wal->picnum)
	{
	case RRTILE8464:
		if (isRRRA()) break;
		break;
	case RRTILE3643:
	case RRTILE3643 + 1:
	case RRTILE3643 + 2:
	case RRTILE3643 + 3:
	{
		int sect;
		sect = wal->nextWall()->nextsector;
		DukeSectIterator it(sect);
		while (auto act = it.Next())
		{
			s = act->s;
			if (s->lotag == 6)
			{
				act->spriteextra++;
				if (act->spriteextra == 25)
				{
					for(auto& wl : wallsofsector(s->sectnum))
					{
						if (wl.nextsector >= 0) wl.nextSector()->lotag = 0;
					}
					s->sector()->lotag = 0;
					S_StopSound(act->s->lotag);
					S_PlayActorSound(400, act);
					deletesprite(act);
				}
			}
		}
		return;
	}
	case RRTILE7555:
		if (!isRRRA()) break;
		wal->picnum = SBMOVE;
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7441:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5016;
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7559:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5017;
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7433:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5018;
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7557:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5019;
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7553:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5020;
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7552:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5021;
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7568:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5022;
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7540:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5023;
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7558:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5024;
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7554:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5025;
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7579:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5026;
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7561:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5027;
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7580:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5037;
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE8227:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5070;
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE8503:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5079;
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE8567:
	case RRTILE8568:
	case RRTILE8569:
	case RRTILE8570:
	case RRTILE8571:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5082;
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE7859:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5081;
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE8496:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5061;
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE8617:
		if (!isRRRA()) break;
		if (numplayers < 2)
		{
			wal->picnum = RRTILE8618;
			S_PlayActorSound(47, spr);
		}
		return;
	case RRTILE8620:
		if (!isRRRA()) break;
		wal->picnum = RRTILE8621;
		S_PlayActorSound(47, spr);
		return;
	case RRTILE8622:
		if (!isRRRA()) break;
		wal->picnum = RRTILE8623;
		S_PlayActorSound(495, spr);
		return;
	case RRTILE7657:
		if (!isRRRA()) break;
		wal->picnum = RRTILE7659;
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;
	case RRTILE8497:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5076;
		S_PlayActorSound(495, spr);
		return;
	case RRTILE7533:
		if (!isRRRA()) break;
		wal->picnum = RRTILE5035;
		S_PlayActorSound(495, spr);
		return;

	case COLAMACHINE:
	case VENDMACHINE:
		breakwall(wal->picnum + 2, spr, wal);
		S_PlayActorSound(GLASS_BREAKING, spr);
		return;

	case OJ:

	case SCREENBREAK6:
	case SCREENBREAK7:
	case SCREENBREAK8:

		lotsofglass(spr, wal, 30);
		wal->picnum = W_SCREENBREAK + (krand() % (isRRRA() ? 2 : 3));
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;

	case ATM:
		wal->picnum = ATMBROKE;
		fi.lotsofmoney(spr, 1 + (krand() & 7));
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		break;

	case WALLLIGHT1:
	case WALLLIGHT3:
	case WALLLIGHT4:
	case TECHLIGHT2:
	case TECHLIGHT4:
	case RRTILE1814:
	case RRTILE1939:
	case RRTILE1986:
	case RRTILE1988:
	case RRTILE2123:
	case RRTILE2125:
	case RRTILE2636:
	case RRTILE2878:
	case RRTILE2898:
	case RRTILE3200:
	case RRTILE3202:
	case RRTILE3204:
	case RRTILE3206:
	case RRTILE3208:

		if (rnd(128))
			S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		else S_PlayActorSound(GLASS_BREAKING, spr);
		lotsofglass(spr, wal, 30);

		if (wal->picnum == RRTILE1814)
			wal->picnum = RRTILE1817;

		if (wal->picnum == RRTILE1986)
			wal->picnum = RRTILE1987;

		if (wal->picnum == RRTILE1939)
			wal->picnum = RRTILE2004;

		if (wal->picnum == RRTILE1988)
			wal->picnum = RRTILE2005;

		if (wal->picnum == RRTILE2898)
			wal->picnum = RRTILE2899;

		if (wal->picnum == RRTILE2878)
			wal->picnum = RRTILE2879;

		if (wal->picnum == RRTILE2123)
			wal->picnum = RRTILE2124;

		if (wal->picnum == RRTILE2125)
			wal->picnum = RRTILE2126;

		if (wal->picnum == RRTILE3200)
			wal->picnum = RRTILE3201;

		if (wal->picnum == RRTILE3202)
			wal->picnum = RRTILE3203;

		if (wal->picnum == RRTILE3204)
			wal->picnum = RRTILE3205;

		if (wal->picnum == RRTILE3206)
			wal->picnum = RRTILE3207;

		if (wal->picnum == RRTILE3208)
			wal->picnum = RRTILE3209;

		if (wal->picnum == RRTILE2636)
			wal->picnum = RRTILE2637;

		if (wal->picnum == WALLLIGHT1)
			wal->picnum = WALLLIGHTBUST1;

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

		for (auto& wl : wallsofsector(wal->nextsector))
			if (wl.shade > darkestwall)
				darkestwall = wl.shade;

		j = krand() & 1;
		DukeStatIterator it(STAT_EFFECTOR);
		while (auto act = it.Next())
		{
			if (act->s->hitag == wal->lotag && act->s->lotag == 3)
			{
				act->temp_data[2] = j;
				act->temp_data[3] = darkestwall;
				act->temp_data[4] = 1;
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

void checkplayerhurt_r(struct player_struct* p, const Collision &coll)
{
	if (coll.type == kHitSprite)
	{
		switch (coll.actor->s->picnum)
		{
		case RRTILE2430:
		case RRTILE2431:
		case RRTILE2432:
		case RRTILE2443:
		case RRTILE2446:
		case RRTILE2451:
		case RRTILE2455:
			if (isRRRA() && p->hurt_delay2 < 8)
			{
				p->GetActor()->s->extra -= 2;
				p->hurt_delay2 = 16;
				SetPlayerPal(p, PalEntry(32, 32, 0, 0));
				S_PlayActorSound(DUKE_LONGTERM_PAIN, p->GetActor());
			}
			break;
		case CACTUS:
			if (!isRRRA() && p->hurt_delay < 8)
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
	auto wal = coll.wall();

	if (p->hurt_delay > 0) p->hurt_delay--;
	else if (wal->cstat & 85) switch (wal->overpicnum)
	{
	case BIGFORCE:
		p->hurt_delay = 26;
		fi.checkhitwall(p->GetActor(), wal,
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

bool checkhitceiling_r(sectortype* sectp)
{
	int j;

	switch (sectp->ceilingpicnum)
	{
	case WALLLIGHT1:
	case WALLLIGHT3:
	case WALLLIGHT4:
	case TECHLIGHT2:
	case TECHLIGHT4:
	case RRTILE1939:
	case RRTILE1986:
	case RRTILE1988:
	case RRTILE2123:
	case RRTILE2125:
	case RRTILE2878:
	case RRTILE2898:


		ceilingglass(ps[myconnectindex].GetActor(), sectp, 10);
		S_PlayActorSound(GLASS_BREAKING, ps[screenpeek].GetActor());

		if (sectp->ceilingpicnum == WALLLIGHT1)
			sectp->ceilingpicnum = WALLLIGHTBUST1;

		if (sectp->ceilingpicnum == WALLLIGHT3)
			sectp->ceilingpicnum = WALLLIGHTBUST3;

		if (sectp->ceilingpicnum == WALLLIGHT4)
			sectp->ceilingpicnum = WALLLIGHTBUST4;

		if (sectp->ceilingpicnum == TECHLIGHT2)
			sectp->ceilingpicnum = TECHLIGHTBUST2;

		if (sectp->ceilingpicnum == TECHLIGHT4)
			sectp->ceilingpicnum = TECHLIGHTBUST4;

		if (sectp->ceilingpicnum == RRTILE1986)
			sectp->ceilingpicnum = RRTILE1987;

		if (sectp->ceilingpicnum == RRTILE1939)
			sectp->ceilingpicnum = RRTILE2004;

		if (sectp->ceilingpicnum == RRTILE1988)
			sectp->ceilingpicnum = RRTILE2005;

		if (sectp->ceilingpicnum == RRTILE2898)
			sectp->ceilingpicnum = RRTILE2899;

		if (sectp->ceilingpicnum == RRTILE2878)
			sectp->ceilingpicnum = RRTILE2879;

		if (sectp->ceilingpicnum == RRTILE2123)
			sectp->ceilingpicnum = RRTILE2124;

		if (sectp->ceilingpicnum == RRTILE2125)
			sectp->ceilingpicnum = RRTILE2126;


		if (!sectp->hitag)
		{
			DukeSectIterator it(sectp);
			while (auto act1 = it.Next())
			{
				auto spr1 = act1->s;
				if (spr1->picnum == SECTOREFFECTOR && (spr1->lotag == 12 || (isRRRA() && (spr1->lotag == 47 || spr1->lotag == 48))))
				{
					DukeStatIterator it(STAT_EFFECTOR);
					while (auto act2 = it.Next())
					{
						if (act2->s->hitag == spr1->hitag)
							act2->temp_data[3] = 1;
					}
					break;
				}
			}
		}

		j = krand() & 1;
		DukeStatIterator it(STAT_EFFECTOR);
		while (auto act1 = it.Next())
		{
			auto spr1 = act1->s;
			if (spr1->hitag == (sectp->hitag) && spr1->lotag == 3)
			{
				act1->temp_data[2] = j;
				act1->temp_data[4] = 1;
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

void checkhitsprite_r(DDukeActor* targ, DDukeActor* proj)
{
	int j, k, p;
	spritetype* s = targ->s;
	auto pspr = proj->s;

	if (isRRRA()) switch (s->picnum)
	{
	case RRTILE8464:
		break;
	case RRTILE8487:
	case RRTILE8489:
		S_PlayActorSound(471, targ);
		s->picnum++;
		break;
	case RRTILE7638:
	case RRTILE7644:
	case RRTILE7646:
	case RRTILE7650:
	case RRTILE7653:
	case RRTILE7655:
	case RRTILE7691:
	case RRTILE7876:
	case RRTILE7881:
	case RRTILE7883:
		s->picnum++;
		S_PlayActorSound(VENT_BUST, targ);
		break;
	case RRTILE7879:
		s->picnum++;
		S_PlayActorSound(495, targ);
		fi.hitradius(targ, 10, 0, 0, 1, 1);
		break;
	case RRTILE7648:
	case RRTILE7694:
	case RRTILE7700:
	case RRTILE7702:
	case RRTILE7711:
		s->picnum++;
		S_PlayActorSound(47, targ);
		break;
	case RRTILE7636:
		s->picnum += 3;
		S_PlayActorSound(VENT_BUST, targ);
		break;
	case RRTILE7875:
		s->picnum += 3;
		S_PlayActorSound(VENT_BUST, targ);
		break;
	case RRTILE7640:
		s->picnum += 2;
		S_PlayActorSound(VENT_BUST, targ);
		break;
	case RRTILE7595:
	case RRTILE7704:
		s->picnum = RRTILE7705;
		S_PlayActorSound(495, targ);
		break;
	case RRTILE8579:
		s->picnum = RRTILE5014;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7441:
		s->picnum = RRTILE5016;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7534:
		s->picnum = RRTILE5029;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7545:
		s->picnum = RRTILE5030;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7547:
		s->picnum = RRTILE5031;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7574:
		s->picnum = RRTILE5032;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7575:
		s->picnum = RRTILE5033;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7578:
		s->picnum = RRTILE5034;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7478:
		s->picnum = RRTILE5035;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8525:
		s->picnum = RRTILE5036;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8537:
		s->picnum = RRTILE5062;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8215:
		s->picnum = RRTILE5064;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8216:
		s->picnum = RRTILE5065;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8217:
		s->picnum = RRTILE5066;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8218:
		s->picnum = RRTILE5067;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8220:
		s->picnum = RRTILE5068;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8221:
		s->picnum = RRTILE5069;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8312:
		s->picnum = RRTILE5071;
		S_PlayActorSound(472, targ);
		break;
	case RRTILE8395:
		s->picnum = RRTILE5072;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8423:
		s->picnum = RRTILE5073;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE3462:
		s->picnum = RRTILE5074;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case UWHIP:
		s->picnum = RRTILE5075;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8608:
		s->picnum = RRTILE5083;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8609:
		s->picnum = RRTILE5084;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8567:
	case RRTILE8568:
	case RRTILE8569:
	case RRTILE8570:
	case RRTILE8571:
		s->picnum = RRTILE5082;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8640:
		s->picnum = RRTILE5085;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8611:
		s->picnum = RRTILE5086;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case TECHLIGHTBUST2:
		s->picnum = TECHLIGHTBUST4;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8497:
		s->picnum = RRTILE5076;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8162:
	case RRTILE8163:
	case RRTILE8164:
	case RRTILE8165:
	case RRTILE8166:
	case RRTILE8167:
	case RRTILE8168:
		changeactorstat(targ, STAT_MISC);
		s->picnum = RRTILE5063;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8589:
	case RRTILE8590:
	case RRTILE8591:
	case RRTILE8592:
	case RRTILE8593:
	case RRTILE8594:
	case RRTILE8595:
		changeactorstat(targ, STAT_MISC);
		s->picnum = RRTILE8588;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE3497:
		s->picnum = RRTILE5076;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE3498:
		s->picnum = RRTILE5077;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE3499:
		s->picnum = RRTILE5078;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8503:
		s->picnum = RRTILE5079;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7901:
		s->picnum = RRTILE5080;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7696:
		s->picnum = RRTILE7697;
		S_PlayActorSound(DUKE_SHUCKS, targ);
		break;
	case RRTILE7806:
		s->picnum = RRTILE5043;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7885:
	case RRTILE7890:
		s->picnum = RRTILE5045;
		S_PlayActorSound(495, targ);
		fi.hitradius(targ, 10, 0, 0, 1, 1);
		break;
	case RRTILE7886:
		s->picnum = RRTILE5046;
		S_PlayActorSound(495, targ);
		fi.hitradius(targ, 10, 0, 0, 1, 1);
		break;
	case RRTILE7887:
		s->picnum = RRTILE5044;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		fi.hitradius(targ, 10, 0, 0, 1, 1);
		break;
	case RRTILE7900:
		s->picnum = RRTILE5047;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7906:
		s->picnum = RRTILE5048;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7912:
	case RRTILE7913:
		s->picnum = RRTILE5049;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8047:
		s->picnum = RRTILE5050;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8596:
		s->picnum = RRTILE8598;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8059:
		s->picnum = RRTILE5051;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8060:
		s->picnum = RRTILE5052;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8222:
		s->picnum = RRTILE5053;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8223:
		s->picnum = RRTILE5054;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8224:
		s->picnum = RRTILE5055;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8370:
		s->picnum = RRTILE5056;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8371:
		s->picnum = RRTILE5057;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8372:
		s->picnum = RRTILE5058;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8373:
		s->picnum = RRTILE5059;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8396:
		s->picnum = RRTILE5038;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8397:
		s->picnum = RRTILE5039;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8398:
		s->picnum = RRTILE5040;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8399:
		s->picnum = RRTILE5041;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8385:
		s->picnum = RRTILE8386;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8387:
		s->picnum = RRTILE8388;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8389:
		s->picnum = RRTILE8390;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8391:
		s->picnum = RRTILE8392;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7553:
		s->picnum = RRTILE5035;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8475:
		s->picnum = RRTILE5075;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8498:
		s->picnum = RRTILE5077;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8499:
		s->picnum = RRTILE5078;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE2445:
		s->picnum = RRTILE2450;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE2123:
		s->picnum = RRTILE2124;
		S_PlayActorSound(GLASS_BREAKING, targ);
		lotsofglass(targ, nullptr, 10);
		break;
	case RRTILE3773:
		s->picnum = RRTILE8651;
		S_PlayActorSound(GLASS_BREAKING, targ);
		lotsofglass(targ, nullptr, 10);
		break;
	case RRTILE7533:
		s->picnum = RRTILE5035;
		S_PlayActorSound(495, targ);
		fi.hitradius(targ, 10, 0, 0, 1, 1);
		break;
	case RRTILE8394:
		s->picnum = RRTILE5072;
		S_PlayActorSound(495, targ);
		break;
	case RRTILE8461:
	case RRTILE8462:
		s->picnum = RRTILE5074;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8679:
		s->picnum = RRTILE8680;
		S_PlayActorSound(DUKE_SHUCKS, targ);
		fi.hitradius(targ, 10, 0, 0, 1, 1);
		if (s->lotag != 0)
		{
			DukeSpriteIterator it;
			while (auto act = it.Next())
			{
				if (act->s->picnum == RRTILE8679 && act->s->pal == 4)
				{
					if (act->s->lotag == s->lotag)
						act->s->picnum = RRTILE8680;
				}
			}
		}
		break;
	case RRTILE3584:
		s->picnum = RRTILE8681;
		S_PlayActorSound(495, targ);
		fi.hitradius(targ, 250, 0, 0, 1, 1);
		break;
	case RRTILE8682:
		s->picnum = RRTILE8683;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8099:
		if (s->lotag == 5)
		{
			s->lotag = 0;
			s->picnum = RRTILE5087;
			S_PlayActorSound(340, targ);
			DukeSpriteIterator it;
			while (auto act = it.Next())
			{
				if (act->s->picnum == RRTILE8094)
					act->s->picnum = RRTILE5088;
			}
		}
		break;
	case RRTILE2431:
		if (s->pal != 4)
		{
			s->picnum = RRTILE2451;
			if (s->lotag != 0)
			{
				DukeSpriteIterator it;
				while (auto act = it.Next())
				{
					if (act->s->picnum == RRTILE2431 && act->s->pal == 4)
					{
						if (s->lotag == act->s->lotag)
							act->s->picnum = RRTILE2451;
					}
				}
			}
		}
		break;
	case RRTILE2443:
		if (s->pal != 19)
			s->picnum = RRTILE2455;
		break;
	case RRTILE2455:
		S_PlayActorSound(SQUISHED, targ);
		fi.guts(targ, RRTILE2465, 3, myconnectindex);
		deletesprite(targ);
		break;
	case RRTILE2451:
		if (s->pal != 4)
		{
			S_PlayActorSound(SQUISHED, targ);
			if (s->lotag != 0)
			{
				DukeSpriteIterator it;
				while (auto act = it.Next())
				{
					if (act->s->picnum == RRTILE2451 && act->s->pal == 4)
					{
						if (s->lotag == act->s->lotag)
						{
							fi.guts(targ, RRTILE2460, 12, myconnectindex);
							fi.guts(targ, RRTILE2465, 3, myconnectindex);
							act->s->xrepeat = 0;
							act->s->yrepeat = 0;
							s->xrepeat = 0;
							s->yrepeat = 0;
						}
					}
				}
			}
			else
			{
				fi.guts(targ, RRTILE2460, 12, myconnectindex);
				fi.guts(targ, RRTILE2465, 3, myconnectindex);
				s->xrepeat = 0;
				s->yrepeat = 0;
			}
		}
		break;
	case RRTILE2437:
		S_PlayActorSound(439, targ);
		break;
	}

	switch (s->picnum)
	{
	case RRTILE3114:
		s->picnum = RRTILE3117;
		break;
	case RRTILE2876:
		s->picnum = RRTILE2990;
		break;
	case RRTILE3152:
		s->picnum = RRTILE3218;
		break;
	case RRTILE3153:
		s->picnum = RRTILE3219;
		break;
	case RRTILE2030:
		s->picnum = RRTILE2034;
		S_PlayActorSound(GLASS_BREAKING, targ);
		lotsofglass(targ, nullptr, 10);
		break;
	case RRTILE2893:
	case RRTILE2915:
	case RRTILE3115:
	case RRTILE3171:
		switch (s->picnum)
		{
		case RRTILE2915:
			s->picnum = RRTILE2977;
			break;
		case RRTILE2893:
			s->picnum = RRTILE2978;
			break;
		case RRTILE3115:
			s->picnum = RRTILE3116;
			break;
		case RRTILE3171:
			s->picnum = RRTILE3216;
			break;
		}
		S_PlayActorSound(GLASS_BREAKING, targ);
		lotsofglass(targ, nullptr, 10);
		break;
	case RRTILE2156:
	case RRTILE2158:
	case RRTILE2160:
	case RRTILE2175:
		s->picnum++;
		S_PlayActorSound(GLASS_BREAKING, targ);
		lotsofglass(targ, nullptr, 10);
		break;
	case RRTILE2137:
	case RRTILE2151:
	case RRTILE2152:
		S_PlayActorSound(GLASS_BREAKING, targ);
		lotsofglass(targ, nullptr, 10);
		s->picnum++;
		for (k = 0; k < 6; k++)
			EGS(s->sector(), s->x, s->y, s->z - (8 << 8), SCRAP6 + (krand() & 15), -8, 48, 48, krand() & 2047, (krand() & 63) + 64, -(krand() & 4095) - (s->zvel >> 2), targ, 5);
		break;
	case BOWLINGBALL:
		pspr->xvel = (s->xvel >> 1) + (s->xvel >> 2);
		pspr->ang -= (krand() & 16);
		S_PlayActorSound(355, targ);
		break;

	case STRIPEBALL:
	case QUEBALL:
	case BOWLINGPIN:
	case BOWLINGPIN + 1:
	case HENSTAND:
	case HENSTAND + 1:
		if (pspr->picnum == QUEBALL || pspr->picnum == STRIPEBALL)
		{
			pspr->xvel = (s->xvel >> 1) + (s->xvel >> 2);
			pspr->ang -= (s->ang << 1) + 1024;
			s->ang = getangle(s->x - pspr->x, s->y - pspr->y) - 512;
			if (S_CheckSoundPlaying(POOLBALLHIT) < 2)
				S_PlayActorSound(POOLBALLHIT, targ);
		}
		else if (pspr->picnum == BOWLINGPIN || pspr->picnum == BOWLINGPIN + 1)
		{
			pspr->xvel = (s->xvel >> 1) + (s->xvel >> 2);
			pspr->ang -= ((s->ang << 1) + krand()) & 64;
			s->ang = (s->ang + krand()) & 16;
			S_PlayActorSound(355, targ);
		}
		else if (pspr->picnum == HENSTAND || pspr->picnum == HENSTAND + 1)
		{
			pspr->xvel = (s->xvel >> 1) + (s->xvel >> 2);
			pspr->ang -= ((s->ang << 1) + krand()) & 16;
			s->ang = (s->ang + krand()) & 16;
			S_PlayActorSound(355, targ);
		}
		else
		{
			if (krand() & 3)
			{
				s->xvel = 164;
				s->ang = pspr->ang;
			}
		}
		break;

	case TREE1:
	case TREE2:
	case TIRE:
	case BOX:
		switch (pspr->picnum)
		{
		case RPG2:
			if (!isRRRA()) break;
			[[fallthrough]];
		case RADIUSEXPLOSION:
		case RPG:
		case FIRELASER:
		case HYDRENT:
		case HEAVYHBOMB:
		case TRIPBOMBSPRITE:
		case COOLEXPLOSION1:
		case OWHIP:
		case UWHIP:
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
		case RPG2:
			if (!isRRRA()) break;
			[[fallthrough]];
		case RADIUSEXPLOSION:
		case RPG:
		case FIRELASER:
		case HYDRENT:
		case HEAVYHBOMB:
		case TRIPBOMBSPRITE:
		case COOLEXPLOSION1:
		case OWHIP:
		case UWHIP:
			for (k = 0; k < 64; k++)
			{
				auto j = EGS(s->sector(), s->x, s->y, s->z - (krand() % (48 << 8)), SCRAP6 + (krand() & 3), -8, 48, 48, krand() & 2047, (krand() & 63) + 64, -(krand() & 4095) - (s->zvel >> 2), targ, 5);
				j->s->pal = 8;
			}

			if (s->picnum == CACTUS)
				s->picnum = CACTUSBROKE;
			s->cstat &= ~257;
			//	   else deletesprite(i);
			break;
		}
		break;


	case FANSPRITE:
		s->picnum = FANSPRITEBROKE;
		s->cstat &= (65535 - 257);
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		for (j = 0; j < 16; j++) RANDOMSCRAP(targ);

		break;
	case WATERFOUNTAIN:
	case WATERFOUNTAIN + 1:
	case WATERFOUNTAIN + 2:
	case WATERFOUNTAIN + 3:
		spawn(targ, TOILETWATER);
		break;
	case SATELITE:
	case FUELPOD:
	case SOLARPANNEL:
	case ANTENNA:
		if (gs.actorinfo[SHOTSPARK1].scriptaddress && pspr->extra != ScriptCode[gs.actorinfo[SHOTSPARK1].scriptaddress])
		{
			for (j = 0; j < 15; j++)
				EGS(s->sector(), s->x, s->y, s->sector()->floorz - (12 << 8) - (j << 9), SCRAP1 + (krand() & 15), -8, 64, 64,
					krand() & 2047, (krand() & 127) + 64, -(krand() & 511) - 256, targ, 5);
			spawn(targ, EXPLOSION2);
			deletesprite(targ);
		}
		break;
	case RRTILE1824:
		if (!isRRRA()) break;
		[[fallthrough]];
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
			lotsofcolourglass(targ, nullptr, 40);
			S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		}
		else if (s->picnum == VASE)
			lotsofglass(targ, nullptr, 40);

		S_PlayActorSound(GLASS_BREAKING, targ);
		s->ang = krand() & 2047;
		lotsofglass(targ, nullptr, 8);
		deletesprite(targ);
		break;
	case RRTILE2654:
	case RRTILE2656:
	case RRTILE3172:
		if (!isRRRA()) break;
		[[fallthrough]];
	case BOTTLE7:
		S_PlayActorSound(GLASS_BREAKING, targ);
		lotsofglass(targ, nullptr, 10);
		deletesprite(targ);
		break;
	case FORCESPHERE:
	{
		s->xrepeat = 0;
		auto Owner = targ->GetOwner();
		if (Owner)
		{
			Owner->temp_data[0] = 32;
			Owner->temp_data[1] = !Owner->temp_data[1];
			Owner->temp_data[2] ++;
		}
		spawn(targ, EXPLOSION2);
		break;
	}
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
			if (j) j->s->z = s->sector()->floorz - (32 << 8);
		}
		break;

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
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		for (j = 0; j < 16; j++) RANDOMSCRAP(targ);
		deletesprite(targ);
		break;
	case PLAYERONWATER:
		targ = targ->GetOwner();
		if (!targ) break;
		s = targ->s;
		[[fallthrough]];
	default:
		if ((s->cstat & 16) && s->hitag == 0 && s->lotag == 0 && s->statnum == 0)
			break;

		if ((pspr->picnum == SHRINKSPARK || pspr->picnum == FREEZEBLAST || proj->GetOwner() != targ) && s->statnum != 4)
		{
			if (badguy(targ) == 1)
			{
				if (pspr->picnum == RPG) pspr->extra <<= 1;
				else if (isRRRA() && pspr->picnum == RPG2) pspr->extra <<= 1;

				if ((s->picnum != DRONE))
					if (pspr->picnum != FREEZEBLAST)
						//if (actortype[s->picnum] == 0) //TRANSITIONAL. Cannot be done right with EDuke mess backing the engine. 
						{
							auto spawned = spawn(proj, JIBS6);
							if (spawned)
							{
								if (pspr->pal == 6)
									spawned->s->pal = 6;
								spawned->s->z += (4 << 8);
								spawned->s->xvel = 16;
								spawned->s->xrepeat = spawned->s->yrepeat = 24;
								spawned->s->ang += 32 - (krand() & 63);
							}
						}

				auto Owner = proj->GetOwner();

				if (Owner && Owner->s->picnum == APLAYER && s->picnum != DRONE)
					if (ps[Owner->PlayerIndex()].curr_weapon == SHOTGUN_WEAPON)
					{
						fi.shoot(targ, BLOODSPLAT3);
						fi.shoot(targ, BLOODSPLAT1);
						fi.shoot(targ, BLOODSPLAT2);
						fi.shoot(targ, BLOODSPLAT4);
					}

				if (s->statnum == 2)
				{
					changeactorstat(targ, 1);
					targ->timetosleep = SLEEPTIME;
				}
			}

			if (s->statnum != 2)
			{
				if (pspr->picnum == FREEZEBLAST && ((s->picnum == APLAYER && s->pal == 1) || (gs.freezerhurtowner == 0 && proj->GetOwner() == targ)))
					return;

				targ->picnum = pspr->picnum;
				targ->extra += pspr->extra;
				if (s->picnum != COW)
					targ->ang = pspr->ang;
				targ->SetHitOwner(proj->GetOwner());
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

					updatesector(ps[p].pos.x, ps[p].pos.y, &ps[p].cursectnum);

					DukeStatIterator it(STAT_EFFECTOR);
					while (auto act = it.Next())
					{
						if (act->s->picnum == CAMERA1) act->s->yvel = 0;
					}
				}
				auto Owner = targ->GetHitOwner();
				if (!Owner || Owner->s->picnum != APLAYER)
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

void checksectors_r(int snum)
{
	int oldz;
	struct player_struct* p;
	walltype* hitscanwall;
	sectortype* ntsector = nullptr;
	walltype* ntwall = nullptr;
	DDukeActor* neartagsprite = nullptr;
	int neartaghitdist = 0;

	p = &ps[snum];
	auto pact = p->GetActor();

	if (!p->insector()) return;

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
		if (!isRRRA() || !RRRA_ExitedLevel)
		{
			setnextmap(false);
			RRRA_ExitedLevel = 1;
		}
		return;
	case -2:
		p->cursector()->lotag = 0;
		p->timebeforeexit = 26 * 8;
		p->customexitsound = p->cursector()->hitag;
		return;
	default:
		if (p->cursector()->lotag >= 10000)
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


	if (!(PlayerInput(snum, SB_OPEN)))
		p->toggle_key_flag = 0;

	else if (!p->toggle_key_flag)
	{
		neartagsprite = nullptr;
		p->toggle_key_flag = 1;
		hitscanwall = nullptr;

		hitawall(p, &hitscanwall);

		if (hitscanwall != nullptr)
		{
			if (isRRRA())
			{
				if (hitscanwall->overpicnum == MIRROR && snum == screenpeek)
					if (numplayers == 1)
					{
						if (S_CheckActorSoundPlaying(pact, 27) == 0 && S_CheckActorSoundPlaying(pact, 28) == 0 && S_CheckActorSoundPlaying(pact, 29) == 0
							&& S_CheckActorSoundPlaying(pact, 257) == 0 && S_CheckActorSoundPlaying(pact, 258) == 0)
						{
							int snd = krand() % 5;
							if (snd == 0)
								S_PlayActorSound(27, pact);
							else if (snd == 1)
								S_PlayActorSound(28, pact);
							else if (snd == 2)
								S_PlayActorSound(29, pact);
							else if (snd == 3)
								S_PlayActorSound(257, pact);
							else if (snd == 4)
								S_PlayActorSound(258, pact);
						}
						return;
					}
			}
			else
			{
				if (hitscanwall->overpicnum == MIRROR)
					if (hitscanwall->lotag > 0 && S_CheckActorSoundPlaying(pact, hitscanwall->lotag) == 0 && snum == screenpeek)
					{
						S_PlayActorSound(hitscanwall->lotag, pact);
						return;
					}
			}
			
			if ((hitscanwall->cstat & 16))
				if (hitscanwall->lotag)
					return;
			
		}
		if (isRRRA())
		{
			if (p->OnMotorcycle)
			{
				if (p->MotoSpeed < 20)
				{
					OffMotorcycle(p);
						return;
				}
				return;
			}
			if (p->OnBoat)
			{
				if (p->MotoSpeed < 20)
				{
					OffBoat(p);
					return;
				}
				return;
			}
			neartag(p->pos.x, p->pos.y, p->pos.z, p->GetActor()->s->sectnum, p->angle.oang.asbuild(), &ntsector, &ntwall, &neartagsprite, &neartaghitdist, 1280L, 3);
		}

		if (p->newOwner != nullptr)
			neartag(p->oposx, p->oposy, p->oposz, p->GetActor()->s->sectnum, p->angle.oang.asbuild(), &ntsector, &ntwall, &neartagsprite, &neartaghitdist, 1280L, 1);
		else
		{
			neartag(p->pos.x, p->pos.y, p->pos.z, p->GetActor()->s->sectnum, p->angle.oang.asbuild(), &ntsector, &ntwall, &neartagsprite, &neartaghitdist, 1280L, 1);
			if (neartagsprite == nullptr && ntwall == nullptr && ntsector == nullptr)
				neartag(p->pos.x, p->pos.y, p->pos.z + (8 << 8), p->GetActor()->s->sectnum, p->angle.oang.asbuild(), &ntsector, &ntwall, &neartagsprite, &neartaghitdist, 1280L, 1);
			if (neartagsprite == nullptr && ntwall == nullptr && ntsector == nullptr)
				neartag(p->pos.x, p->pos.y, p->pos.z + (16 << 8), p->GetActor()->s->sectnum, p->angle.oang.asbuild(), &ntsector, &ntwall, &neartagsprite, &neartaghitdist, 1280L, 1);
			if (neartagsprite == nullptr && ntwall == nullptr && ntsector == nullptr)
			{
				neartag(p->pos.x, p->pos.y, p->pos.z + (16 << 8), p->GetActor()->s->sectnum, p->angle.oang.asbuild(), &ntsector, &ntwall, &neartagsprite, &neartaghitdist, 1280L, 3);
				if (neartagsprite != nullptr)
				{
					switch (neartagsprite->s->picnum)
					{
					case FEM10:
					case NAKED1:
					case STATUE:
					case TOUGHGAL:
						return;
					case COW:
						neartagsprite->spriteextra = 1;
						return;
					}
				}

				neartagsprite = nullptr;
				ntwall = nullptr;
				ntsector = nullptr;
			}
		}

		if (p->newOwner == nullptr && neartagsprite == nullptr && ntsector == nullptr && ntwall == nullptr)
			if (isanunderoperator(p->GetActor()->getSector()->lotag))
				ntsector = p->GetActor()->s->sector();

		if (ntsector && (ntsector->lotag & 16384))
			return;

		if (neartagsprite == nullptr && ntwall == nullptr)
			if (p->cursector()->lotag == 2)
			{
				DDukeActor* hit;
				oldz = hitasprite(p->GetActor(), &hit);
				if (hit) neartagsprite = hit;
				if (oldz > 1280) neartagsprite = nullptr;
			}

		if (neartagsprite != nullptr)
		{
			if (fi.checkhitswitch(snum, nullptr, neartagsprite)) return;

			switch (neartagsprite->s->picnum)
			{
			case RRTILE8448:
				if (!isRRRA()) return;
				if (!S_CheckActorSoundPlaying(neartagsprite, 340))
					S_PlayActorSound(340, neartagsprite);
				return;
			case RRTILE8704:
				if (!isRRRA()) return;
				if (numplayers == 1)
				{
					// This is from RedneckGDX - the version in RR Reconstruction looked like broken nonsense.
					if (S_CheckActorSoundPlaying(neartagsprite, 445) || sound445done != 0)
					{
						if (!S_CheckActorSoundPlaying(neartagsprite, 445) && !S_CheckActorSoundPlaying(neartagsprite, 446) && 
							!S_CheckActorSoundPlaying(neartagsprite, 447) && sound445done != 0)
						{
							if ((krand() % 2) == 1)
								S_PlayActorSound(446, neartagsprite);
							else
								S_PlayActorSound(447, neartagsprite);
						}
					}
					else
					{
						S_PlayActorSound(445, neartagsprite);
						sound445done = 1;
					}
				}
				return;
			case EMPTYBIKE:
				if (!isRRRA()) return;
				OnMotorcycle(p, neartagsprite);
				return;
			case EMPTYBOAT:
				if (!isRRRA()) return;
				OnBoat(p, neartagsprite);
				return;
			case RRTILE8164:
			case RRTILE8165:
			case RRTILE8166:
			case RRTILE8167:
			case RRTILE8168:
			case RRTILE8591:
			case RRTILE8592:
			case RRTILE8593:
			case RRTILE8594:
			case RRTILE8595:
				if (!isRRRA()) return;
				neartagsprite->s->extra = 60;
				S_PlayActorSound(235, neartagsprite);
				return;

			case TOILET:
			case STALL:
			case RRTILE2121:
			case RRTILE2122:
				if (p->last_pissed_time == 0)
				{
					S_PlayActorSound(435, pact);

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
				else if (S_CheckActorSoundPlaying(pact, DUKE_GRUNT) == 0)
					S_PlayActorSound(DUKE_GRUNT, pact);
				return;
			case WATERFOUNTAIN:
				if (neartagsprite->temp_data[0] != 1)
				{
					neartagsprite->temp_data[0] = 1;
					neartagsprite->SetOwner(p->GetActor());

					if (p->GetActor()->s->extra < gs.max_player_health)
					{
						p->GetActor()->s->extra++;
						S_PlayActorSound(DUKE_DRINKING, pact);
					}
				}
				return;
			case PLUG:
				S_PlayActorSound(SHORT_CIRCUIT, pact);
				p->GetActor()->s->extra -= 2 + (krand() & 3);
				SetPlayerPal(p, PalEntry(32, 48, 48, 64));
				break;
			}
		}

		if (!PlayerInput(snum, SB_OPEN)) return;

		if (ntwall == nullptr && ntsector == nullptr && neartagsprite == nullptr)
			if (abs(hits(p->GetActor())) < 512)
			{
				if ((krand() & 255) < 16)
					S_PlayActorSound(DUKE_SEARCH2, pact);
				else S_PlayActorSound(DUKE_SEARCH, pact);
				return;
			}

		if (ntwall != nullptr)
		{
			if (ntwall->lotag > 0 && fi.isadoorwall(ntwall->picnum))
			{
				if (hitscanwall == ntwall || hitscanwall == nullptr)
					fi.checkhitswitch(snum, ntwall, nullptr);
				return;
			}
		}

		if (ntsector && (ntsector->lotag & 16384) == 0 && isanearoperator(ntsector->lotag))
		{
			DukeSectIterator it(ntsector);
			while (auto act = it.Next())
			{
				if (act->s->picnum == ACTIVATOR || act->s->picnum == MASTERSWITCH)
					return;
			}
			if (haskey(ntsector, snum))
				operatesectors(ntsector, p->GetActor());
			else
			{
				if (neartagsprite && neartagsprite->spriteextra > 3)
					S_PlayActorSound(99, pact);
				else
					S_PlayActorSound(419, pact);
				FTA(41, p);
			}
		}
		else if ((p->GetActor()->getSector()->lotag & 16384) == 0)
		{
			if (isanunderoperator(p->GetActor()->getSector()->lotag))
			{
				DukeSectIterator it(p->GetActor()->s->sectnum);
				while (auto act = it.Next())
				{
					if (act->s->picnum == ACTIVATOR || act->s->picnum == MASTERSWITCH)
						return;
				}
				if (haskey(ntsector, snum))
					operatesectors(p->GetActor()->s->sector(), p->GetActor());
				else
				{
					if (neartagsprite && neartagsprite->spriteextra > 3)
						S_PlayActorSound(99, pact);
					else
						S_PlayActorSound(419, pact);
					FTA(41, p);
				}
			}
			else fi.checkhitswitch(snum, ntwall, nullptr);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void dofurniture(walltype* wlwal, sectortype* sectp, int snum)
{
	int nextsect = wlwal->nextsector;
	int var_C;
	int x;
	int y;
	int min_x;
	int min_y;
	int max_x;
	int max_y;
	int ins;
	int var_cx;

	var_C = 1;
	max_x = max_y = -0x20000;
	min_x = min_y = 0x20000;
	var_cx = sectp->hitag;
	if (var_cx > 16)
		var_cx = 16;
	else if (var_cx == 0)
		var_cx = 4;
	for(auto& wal : wallsofsector(nextsect))
	{
		x = wal.x;
		y = wal.y;
		if (x > max_x)
			max_x = x;
		if (y > max_y)
			max_y = y;
		if (x < min_x)
			min_x = x;
		if (y < min_y)
			min_y = y;
	}
	max_x += var_cx + 1;
	max_y += var_cx + 1;
	min_x -= var_cx + 1;
	min_y -= var_cx + 1;
	ins = inside(max_x, max_y, sectp);
	if (!ins)
		var_C = 0;
	ins = inside(max_x, min_y, sectp);
	if (!ins)
		var_C = 0;
	ins = inside(min_x, min_y, sectp);
	if (!ins)
		var_C = 0;
	ins = inside(min_x, max_y, sectp);
	if (!ins)
		var_C = 0;
	if (var_C)
	{
		if (S_CheckActorSoundPlaying(ps[snum].GetActor(), 389) == 0)
			S_PlayActorSound(389, ps[snum].GetActor());
		for(auto& wal : wallsofsector(nextsect))
		{
			x = wal.x;
			y = wal.y;
			switch (wlwal->lotag)
			{
			case 42:
				y = wal.y + var_cx;
				dragpoint(&wal, x, y);
				break;
			case 41:
				x = wal.x - var_cx;
				dragpoint(&wal, x, y);
				break;
			case 40:
				y = wal.y - var_cx;
				dragpoint(&wal, x, y);
				break;
			case 43:
				x = wal.x + var_cx;
				dragpoint(&wal, x, y);
				break;
			}
		}
	}
	else
	{
		for(auto& wal : wallsofsector(nextsect))
		{
			x = wal.x;
			y = wal.y;
			switch (wlwal->lotag)
			{
			case 42:
				y = wal.y - (var_cx - 2);
				dragpoint(&wal, x, y);
				break;
			case 41:
				x = wal.x + (var_cx - 2);
				dragpoint(&wal, x, y);
				break;
			case 40:
				y = wal.y + (var_cx - 2);
				dragpoint(&wal, x, y);
				break;
			case 43:
				x = wal.x - (var_cx - 2);
				dragpoint(&wal, x, y);
				break;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void tearitup(int sect)
{
	DukeSectIterator it(sect);
	while (auto act = it.Next())
	{
		if (act->s->picnum == DESTRUCTO)
		{
			act->picnum = SHOTSPARK1;
			act->extra = 1;
		}
	}
}
END_DUKE_NS
