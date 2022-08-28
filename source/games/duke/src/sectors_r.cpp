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
		for (auto& wal : wall)
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

void operaterespawns_r(int low)
{
	DukeStatIterator it(STAT_FX);
	while (auto act = it.Next())
	{
		if (act->spr.lotag == low) switch (act->spr.picnum)
		{
		case RESPAWN:
		{
			if (badguypic(act->spr.hitag) && ud.monsters_off) break;

			auto star = spawn(act, TRANSPORTERSTAR);
			if (star) star->add_int_z(-(32 << 8));

			act->spr.extra = 66 - 12;   // Just a way to killit
			break;
		}
		case RRTILE7424:
			if (isRRRA() && !ud.monsters_off)
				ChangeActorStat(act, 119);
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
		case MULTISWITCH2:
		case MULTISWITCH2 + 1:
		case MULTISWITCH2 + 2:
		case MULTISWITCH2 + 3:
			if (!isRRRA()) break;
			other->spr.picnum++;
			if (other->spr.picnum > (MULTISWITCH2 + 3))
				other->spr.picnum = MULTISWITCH2;
			break;

		case RRTILE2214:
			//if (ud.level_numbe r > 6) ud.level_numbe r = 0; ??? Looks like some leftover garbage.
			other->spr.picnum++;
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
			if (other->spr.picnum == DIPSWITCH3)
				if (other->spr.hitag == 999)
				{
					DukeStatIterator it1(107);
					while (auto other2 = it1.Next())
					{
						if (other2->spr.picnum == RRTILE3410)
						{
							other2->spr.picnum++;
							other2->spr.hitag = 100;
							other2->spr.extra = 0;
							S_PlayActorSound(474, other2);
						}
						else if (other2->spr.picnum == RRTILE295)
							deletesprite(other2);
					}
					other->spr.picnum++;
					break;
				}
			if (other->spr.picnum == NUKEBUTTON)
				chickenplant = 0;
			if (other->spr.picnum == RRTILE8660)
			{
				BellTime = 132;
				BellSprite = other;
			}
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
		case NUKEBUTTON + 1:
		case RRTILE2697 + 1:
		case RRTILE2707 + 1:
			if (other->spr.picnum == NUKEBUTTON + 1)
				chickenplant = 1;
			if (other->spr.hitag != 999)
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

	vec3_t v = { sx, sy, ps[snum].player_int_pos().Z };
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
				act->spr.picnum++;
			}
			else if (picnum == RRTILE8464)
			{
				act->spr.picnum = act->spr.picnum + 1;
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
					S_PlaySound3D(SWITCH_ON, act, v);
					DukeSpriteIterator itr;
					while (auto actt = itr.Next())
					{
						int jpn = actt->spr.picnum;
						int jht = actt->spr.hitag;
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
						// This once was a linear search over sprites[] so bring things back in order, just to be safe.
						if (switches[0]->GetIndex() > switches[1]->GetIndex()) std::swap(switches[0], switches[1]);
						if (switches[0]->GetIndex() > switches[2]->GetIndex()) std::swap(switches[0], switches[2]);
						if (switches[1]->GetIndex() > switches[2]->GetIndex()) std::swap(switches[1], switches[2]);

						S_PlaySound3D(78, act, v);
						for (j = 0; j < switchcount; j++)
						{
							switches[j]->spr.hitag = 0;
							if (picnum >= MULTISWITCH2)
								switches[j]->spr.picnum = MULTISWITCH2 + 3;
							else
								switches[j]->spr.picnum = MULTISWITCH + 3;
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

		DukeStatIterator itr(STAT_EFFECTOR);
		while (auto other = itr.Next())
		{
			if (other->spr.hitag == lotag)
			{
				switch (other->spr.lotag)
				{
				case 46:
				case SE_47_LIGHT_SWITCH:
				case SE_48_LIGHT_SWITCH:
					if (!isRRRA()) break;
					[[fallthrough]];
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

void activatebysector_r(sectortype* sect, DDukeActor* activator)
{
	DukeSectIterator it(sect);
	while (auto act = it.Next())
	{
		if (act->spr.picnum == ACTIVATOR)
		{
			operateactivators(act->spr.lotag, -1);
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

	if (wal == nullptr)
	{
		for (j = n - 1; j >= 0; j--)
		{
			a = actor->int_ang() - 256 + (krand() & 511) + 1024;
			EGS(actor->sector(), actor->int_pos().X, actor->int_pos().Y, actor->int_pos().Z, POPCORN, -32, 36, 36, a, 32 + (krand() & 63), 1024 - (krand() & 1023), actor, 5);
		}
		return;
	}

	j = n + 1;

	int x1 = wal->wall_int_pos().X;
	int y1 = wal->wall_int_pos().Y;

	auto delta = wal->delta();

	x1 -= Sgn(delta.X);
	y1 += Sgn(delta.Y);

	delta.X /= j;
	delta.Y /= j;

	for (j = n; j > 0; j--)
	{
		x1 += delta.X;
		y1 += delta.Y;

		updatesector(x1, y1, &sect);
		if (sect)
		{
			z = sect->int_floorz() - (krand() & (abs(sect->int_ceilingz() - sect->int_floorz())));
			if (z < -(32 << 8) || z >(32 << 8))
				z = actor->int_pos().Z - (32 << 8) + (krand() & ((64 << 8) - 1));
			a = actor->int_ang() - 1024;
			EGS(actor->sector(), x1, y1, z, POPCORN, -32, 36, 36, a, 32 + (krand() & 63), -(krand() & 1023), actor, 5);
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

				case RRTILE1973:
				{
					sectortype* sptr = nullptr;
					updatesector(x, y, &sptr);
					if (sptr == nullptr) return;
					wal->overpicnum = GLASS2;
					lotsofpopcorn(spr, wal, 64);
					wal->cstat = 0;

					if (wal->twoSided())
						wal->nextWall()->cstat = 0;

					auto spawned = EGS(sptr, x, y, z, SECTOREFFECTOR, 0, 0, 0, ps[0].angle.ang.Buildang(), 0, 0, spr, 3);
					if (spawned)
					{
						spawned->spr.lotag = SE_128_GLASS_BREAKING;
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

					if (wal->twoSided())
						wal->nextWall()->cstat = 0;

					auto spawned = EGS(sptr, x, y, z, SECTOREFFECTOR, 0, 0, 0, ps[0].angle.ang.Buildang(), 0, 0, spr, 3);
					if (spawned)
					{
						spawned->spr.lotag = SE_128_GLASS_BREAKING;
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
					if (wal->twoSided())
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
		auto sect = wal->nextWall()->nextSector();
		DukeSectIterator it(sect);
		while (auto act = it.Next())
		{
			if (act->spr.lotag == 6)
			{
				act->spriteextra++;
				if (act->spriteextra == 25)
				{
					for(auto& wl : wallsofsector(act->sector()))
					{
						if (wl.twoSided()) wl.nextSector()->lotag = 0;
					}
					act->sector()->lotag = 0;
					S_StopSound(act->spr.lotag);
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

		if (!wal->twoSided()) return;
		darkestwall = 0;

		for (auto& wl : wallsofsector(wal->nextSector()))
			if (wl.shade > darkestwall)
				darkestwall = wl.shade;

		j = krand() & 1;
		DukeStatIterator it(STAT_EFFECTOR);
		while (auto act = it.Next())
		{
			if (act->spr.hitag == wal->lotag && act->spr.lotag == 3)
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

void checkplayerhurt_r(player_struct* p, const Collision &coll)
{
	if (coll.type == kHitSprite)
	{
		switch (coll.actor()->spr.picnum)
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
				p->GetActor()->spr.extra -= 2;
				p->hurt_delay2 = 16;
				SetPlayerPal(p, PalEntry(32, 32, 0, 0));
				S_PlayActorSound(DUKE_LONGTERM_PAIN, p->GetActor());
			}
			break;
		case CACTUS:
			if (!isRRRA() && p->hurt_delay < 8)
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
	case BIGFORCE:
		p->hurt_delay = 26;
		fi.checkhitwall(p->GetActor(), wal,
			p->player_int_pos().X + p->angle.ang.Cos() * (1 << 5),
			p->player_int_pos().Y + p->angle.ang.Sin() * (1 << 5),
			p->player_int_pos().Z, -1);
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
				if (act1->spr.picnum == SECTOREFFECTOR && (act1->spr.lotag == 12 || (isRRRA() && (act1->spr.lotag == 47 || act1->spr.lotag == 48))))
				{
					DukeStatIterator itr(STAT_EFFECTOR);
					while (auto act2 = itr.Next())
					{
						if (act2->spr.hitag == act1->spr.hitag)
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
			if (act1->spr.hitag == (sectp->hitag) && act1->spr.lotag == 3)
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

	if (isRRRA()) switch (targ->spr.picnum)
	{
	case RRTILE8464:
		break;
	case RRTILE8487:
	case RRTILE8489:
		S_PlayActorSound(471, targ);
		targ->spr.picnum++;
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
		targ->spr.picnum++;
		S_PlayActorSound(VENT_BUST, targ);
		break;
	case RRTILE7879:
		targ->spr.picnum++;
		S_PlayActorSound(495, targ);
		fi.hitradius(targ, 10, 0, 0, 1, 1);
		break;
	case RRTILE7648:
	case RRTILE7694:
	case RRTILE7700:
	case RRTILE7702:
	case RRTILE7711:
		targ->spr.picnum++;
		S_PlayActorSound(47, targ);
		break;
	case RRTILE7636:
		targ->spr.picnum += 3;
		S_PlayActorSound(VENT_BUST, targ);
		break;
	case RRTILE7875:
		targ->spr.picnum += 3;
		S_PlayActorSound(VENT_BUST, targ);
		break;
	case RRTILE7640:
		targ->spr.picnum += 2;
		S_PlayActorSound(VENT_BUST, targ);
		break;
	case RRTILE7595:
	case RRTILE7704:
		targ->spr.picnum = RRTILE7705;
		S_PlayActorSound(495, targ);
		break;
	case RRTILE8579:
		targ->spr.picnum = RRTILE5014;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7441:
		targ->spr.picnum = RRTILE5016;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7534:
		targ->spr.picnum = RRTILE5029;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7545:
		targ->spr.picnum = RRTILE5030;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7547:
		targ->spr.picnum = RRTILE5031;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7574:
		targ->spr.picnum = RRTILE5032;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7575:
		targ->spr.picnum = RRTILE5033;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7578:
		targ->spr.picnum = RRTILE5034;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7478:
		targ->spr.picnum = RRTILE5035;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8525:
		targ->spr.picnum = RRTILE5036;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8537:
		targ->spr.picnum = RRTILE5062;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8215:
		targ->spr.picnum = RRTILE5064;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8216:
		targ->spr.picnum = RRTILE5065;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8217:
		targ->spr.picnum = RRTILE5066;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8218:
		targ->spr.picnum = RRTILE5067;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8220:
		targ->spr.picnum = RRTILE5068;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8221:
		targ->spr.picnum = RRTILE5069;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8312:
		targ->spr.picnum = RRTILE5071;
		S_PlayActorSound(472, targ);
		break;
	case RRTILE8395:
		targ->spr.picnum = RRTILE5072;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8423:
		targ->spr.picnum = RRTILE5073;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE3462:
		targ->spr.picnum = RRTILE5074;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case UWHIP:
		targ->spr.picnum = RRTILE5075;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8608:
		targ->spr.picnum = RRTILE5083;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8609:
		targ->spr.picnum = RRTILE5084;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8567:
	case RRTILE8568:
	case RRTILE8569:
	case RRTILE8570:
	case RRTILE8571:
		targ->spr.picnum = RRTILE5082;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8640:
		targ->spr.picnum = RRTILE5085;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8611:
		targ->spr.picnum = RRTILE5086;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case TECHLIGHTBUST2:
		targ->spr.picnum = TECHLIGHTBUST4;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8497:
		targ->spr.picnum = RRTILE5076;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8162:
	case RRTILE8163:
	case RRTILE8164:
	case RRTILE8165:
	case RRTILE8166:
	case RRTILE8167:
	case RRTILE8168:
		ChangeActorStat(targ, STAT_MISC);
		targ->spr.picnum = RRTILE5063;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8589:
	case RRTILE8590:
	case RRTILE8591:
	case RRTILE8592:
	case RRTILE8593:
	case RRTILE8594:
	case RRTILE8595:
		ChangeActorStat(targ, STAT_MISC);
		targ->spr.picnum = RRTILE8588;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE3497:
		targ->spr.picnum = RRTILE5076;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE3498:
		targ->spr.picnum = RRTILE5077;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE3499:
		targ->spr.picnum = RRTILE5078;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8503:
		targ->spr.picnum = RRTILE5079;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7901:
		targ->spr.picnum = RRTILE5080;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7696:
		targ->spr.picnum = RRTILE7697;
		S_PlayActorSound(DUKE_SHUCKS, targ);
		break;
	case RRTILE7806:
		targ->spr.picnum = RRTILE5043;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7885:
	case RRTILE7890:
		targ->spr.picnum = RRTILE5045;
		S_PlayActorSound(495, targ);
		fi.hitradius(targ, 10, 0, 0, 1, 1);
		break;
	case RRTILE7886:
		targ->spr.picnum = RRTILE5046;
		S_PlayActorSound(495, targ);
		fi.hitradius(targ, 10, 0, 0, 1, 1);
		break;
	case RRTILE7887:
		targ->spr.picnum = RRTILE5044;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		fi.hitradius(targ, 10, 0, 0, 1, 1);
		break;
	case RRTILE7900:
		targ->spr.picnum = RRTILE5047;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7906:
		targ->spr.picnum = RRTILE5048;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7912:
	case RRTILE7913:
		targ->spr.picnum = RRTILE5049;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8047:
		targ->spr.picnum = RRTILE5050;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8596:
		targ->spr.picnum = RRTILE8598;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8059:
		targ->spr.picnum = RRTILE5051;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8060:
		targ->spr.picnum = RRTILE5052;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8222:
		targ->spr.picnum = RRTILE5053;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8223:
		targ->spr.picnum = RRTILE5054;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8224:
		targ->spr.picnum = RRTILE5055;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8370:
		targ->spr.picnum = RRTILE5056;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8371:
		targ->spr.picnum = RRTILE5057;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8372:
		targ->spr.picnum = RRTILE5058;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8373:
		targ->spr.picnum = RRTILE5059;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8396:
		targ->spr.picnum = RRTILE5038;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8397:
		targ->spr.picnum = RRTILE5039;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8398:
		targ->spr.picnum = RRTILE5040;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8399:
		targ->spr.picnum = RRTILE5041;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8385:
		targ->spr.picnum = RRTILE8386;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8387:
		targ->spr.picnum = RRTILE8388;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8389:
		targ->spr.picnum = RRTILE8390;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8391:
		targ->spr.picnum = RRTILE8392;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE7553:
		targ->spr.picnum = RRTILE5035;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8475:
		targ->spr.picnum = RRTILE5075;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8498:
		targ->spr.picnum = RRTILE5077;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8499:
		targ->spr.picnum = RRTILE5078;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE2445:
		targ->spr.picnum = RRTILE2450;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE2123:
		targ->spr.picnum = RRTILE2124;
		S_PlayActorSound(GLASS_BREAKING, targ);
		lotsofglass(targ, nullptr, 10);
		break;
	case RRTILE3773:
		targ->spr.picnum = RRTILE8651;
		S_PlayActorSound(GLASS_BREAKING, targ);
		lotsofglass(targ, nullptr, 10);
		break;
	case RRTILE7533:
		targ->spr.picnum = RRTILE5035;
		S_PlayActorSound(495, targ);
		fi.hitradius(targ, 10, 0, 0, 1, 1);
		break;
	case RRTILE8394:
		targ->spr.picnum = RRTILE5072;
		S_PlayActorSound(495, targ);
		break;
	case RRTILE8461:
	case RRTILE8462:
		targ->spr.picnum = RRTILE5074;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8679:
		targ->spr.picnum = RRTILE8680;
		S_PlayActorSound(DUKE_SHUCKS, targ);
		fi.hitradius(targ, 10, 0, 0, 1, 1);
		if (targ->spr.lotag != 0)
		{
			DukeSpriteIterator it;
			while (auto act = it.Next())
			{
				if (act->spr.picnum == RRTILE8679 && act->spr.pal == 4)
				{
					if (act->spr.lotag == targ->spr.lotag)
						act->spr.picnum = RRTILE8680;
				}
			}
		}
		break;
	case RRTILE3584:
		targ->spr.picnum = RRTILE8681;
		S_PlayActorSound(495, targ);
		fi.hitradius(targ, 250, 0, 0, 1, 1);
		break;
	case RRTILE8682:
		targ->spr.picnum = RRTILE8683;
		S_PlayActorSound(GLASS_HEAVYBREAK, targ);
		break;
	case RRTILE8099:
		if (targ->spr.lotag == 5)
		{
			targ->spr.lotag = 0;
			targ->spr.picnum = RRTILE5087;
			S_PlayActorSound(340, targ);
			DukeSpriteIterator it;
			while (auto act = it.Next())
			{
				if (act->spr.picnum == RRTILE8094)
					act->spr.picnum = RRTILE5088;
			}
		}
		break;
	case RRTILE2431:
		if (targ->spr.pal != 4)
		{
			targ->spr.picnum = RRTILE2451;
			if (targ->spr.lotag != 0)
			{
				DukeSpriteIterator it;
				while (auto act = it.Next())
				{
					if (act->spr.picnum == RRTILE2431 && act->spr.pal == 4)
					{
						if (targ->spr.lotag == act->spr.lotag)
							act->spr.picnum = RRTILE2451;
					}
				}
			}
		}
		break;
	case RRTILE2443:
		if (targ->spr.pal != 19)
			targ->spr.picnum = RRTILE2455;
		break;
	case RRTILE2455:
		S_PlayActorSound(SQUISHED, targ);
		fi.guts(targ, RRTILE2465, 3, myconnectindex);
		deletesprite(targ);
		break;
	case RRTILE2451:
		if (targ->spr.pal != 4)
		{
			S_PlayActorSound(SQUISHED, targ);
			if (targ->spr.lotag != 0)
			{
				DukeSpriteIterator it;
				while (auto act = it.Next())
				{
					if (act->spr.picnum == RRTILE2451 && act->spr.pal == 4)
					{
						if (targ->spr.lotag == act->spr.lotag)
						{
							fi.guts(targ, RRTILE2460, 12, myconnectindex);
							fi.guts(targ, RRTILE2465, 3, myconnectindex);
							act->spr.xrepeat = 0;
							act->spr.yrepeat = 0;
							targ->spr.xrepeat = 0;
							targ->spr.yrepeat = 0;
						}
					}
				}
			}
			else
			{
				fi.guts(targ, RRTILE2460, 12, myconnectindex);
				fi.guts(targ, RRTILE2465, 3, myconnectindex);
				targ->spr.xrepeat = 0;
				targ->spr.yrepeat = 0;
			}
		}
		break;
	case RRTILE2437:
		S_PlayActorSound(439, targ);
		break;
	}

	switch (targ->spr.picnum)
	{
	case RRTILE3114:
		targ->spr.picnum = RRTILE3117;
		break;
	case RRTILE2876:
		targ->spr.picnum = RRTILE2990;
		break;
	case RRTILE3152:
		targ->spr.picnum = RRTILE3218;
		break;
	case RRTILE3153:
		targ->spr.picnum = RRTILE3219;
		break;
	case RRTILE2030:
		targ->spr.picnum = RRTILE2034;
		S_PlayActorSound(GLASS_BREAKING, targ);
		lotsofglass(targ, nullptr, 10);
		break;
	case RRTILE2893:
	case RRTILE2915:
	case RRTILE3115:
	case RRTILE3171:
		switch (targ->spr.picnum)
		{
		case RRTILE2915:
			targ->spr.picnum = RRTILE2977;
			break;
		case RRTILE2893:
			targ->spr.picnum = RRTILE2978;
			break;
		case RRTILE3115:
			targ->spr.picnum = RRTILE3116;
			break;
		case RRTILE3171:
			targ->spr.picnum = RRTILE3216;
			break;
		}
		S_PlayActorSound(GLASS_BREAKING, targ);
		lotsofglass(targ, nullptr, 10);
		break;
	case RRTILE2156:
	case RRTILE2158:
	case RRTILE2160:
	case RRTILE2175:
		targ->spr.picnum++;
		S_PlayActorSound(GLASS_BREAKING, targ);
		lotsofglass(targ, nullptr, 10);
		break;
	case RRTILE2137:
	case RRTILE2151:
	case RRTILE2152:
		S_PlayActorSound(GLASS_BREAKING, targ);
		lotsofglass(targ, nullptr, 10);
		targ->spr.picnum++;
		for (k = 0; k < 6; k++)
			EGS(targ->sector(), targ->int_pos().X, targ->int_pos().Y, targ->int_pos().Z - (8 << 8), SCRAP6 + (krand() & 15), -8, 48, 48, krand() & 2047, (krand() & 63) + 64, -(krand() & 4095) - (targ->spr.zvel >> 2), targ, 5);
		break;
	case BOWLINGBALL:
		proj->spr.xvel = (targ->spr.xvel >> 1) + (targ->spr.xvel >> 2);
		proj->add_int_ang(-(krand() & 16));
		S_PlayActorSound(355, targ);
		break;

	case STRIPEBALL:
	case QUEBALL:
	case BOWLINGPIN:
	case BOWLINGPIN + 1:
	case HENSTAND:
	case HENSTAND + 1:
		if (proj->spr.picnum == QUEBALL || proj->spr.picnum == STRIPEBALL)
		{
			proj->spr.xvel = (targ->spr.xvel >> 1) + (targ->spr.xvel >> 2);
			proj->add_int_ang(-((targ->int_ang() << 1) + 1024));
			targ->set_int_ang(getangle(targ->int_pos().X - proj->int_pos().X, targ->int_pos().Y - proj->int_pos().Y) - 512);
			if (S_CheckSoundPlaying(POOLBALLHIT) < 2)
				S_PlayActorSound(POOLBALLHIT, targ);
		}
		else if (proj->spr.picnum == BOWLINGPIN || proj->spr.picnum == BOWLINGPIN + 1)
		{
			proj->spr.xvel = (targ->spr.xvel >> 1) + (targ->spr.xvel >> 2);
			proj->add_int_ang(-(((targ->int_ang() << 1) + krand()) & 64));
			targ->set_int_ang((targ->int_ang() + krand()) & 16);
			S_PlayActorSound(355, targ);
		}
		else if (proj->spr.picnum == HENSTAND || proj->spr.picnum == HENSTAND + 1)
		{
			proj->spr.xvel = (targ->spr.xvel >> 1) + (targ->spr.xvel >> 2);
			proj->add_int_ang(-(((targ->int_ang() << 1) + krand()) & 16));
			targ->set_int_ang((targ->int_ang() + krand()) & 16);
			S_PlayActorSound(355, targ);
		}
		else
		{
			if (krand() & 3)
			{
				targ->spr.xvel = 164;
				targ->spr.angle = proj->spr.angle;
			}
		}
		break;

	case TREE1:
	case TREE2:
	case TIRE:
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
				auto spawned = EGS(targ->sector(), targ->int_pos().X, targ->int_pos().Y, targ->int_pos().Z - (krand() % (48 << 8)), SCRAP6 + (krand() & 3), -8, 48, 48, krand() & 2047, (krand() & 63) + 64, -(krand() & 4095) - (targ->spr.zvel >> 2), targ, 5);
				if (spawned) spawned->spr.pal = 8;
			}

			if (targ->spr.picnum == CACTUS)
				targ->spr.picnum = CACTUSBROKE;
			targ->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
			//	   else deletesprite(i);
		}
		break;


	case FANSPRITE:
		targ->spr.picnum = FANSPRITEBROKE;
		targ->spr.cstat &= ~CSTAT_SPRITE_BLOCK_ALL;
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
		if (gs.actorinfo[SHOTSPARK1].scriptaddress && proj->spr.extra != ScriptCode[gs.actorinfo[SHOTSPARK1].scriptaddress])
		{
			for (j = 0; j < 15; j++)
				EGS(targ->sector(), targ->int_pos().X, targ->int_pos().Y, targ->sector()->int_floorz() - (12 << 8) - (j << 9), SCRAP1 + (krand() & 15), -8, 64, 64,
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
		targ->set_int_ang(krand() & 2047);
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
		targ->spr.xrepeat = 0;
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
		targ->spr.picnum = TOILETBROKE;
		if(krand() & 1) targ->spr.cstat |= CSTAT_SPRITE_XFLIP;
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

		if ((proj->spr.picnum == SHRINKSPARK || proj->spr.picnum == FREEZEBLAST || proj->GetOwner() != targ) && targ->spr.statnum != 4)
		{
			if (badguy(targ) == 1)
			{
				if (proj->spr.picnum == RPG) proj->spr.extra <<= 1;
				else if (isRRRA() && proj->spr.picnum == RPG2) proj->spr.extra <<= 1;

				if ((targ->spr.picnum != DRONE))
					if (proj->spr.picnum != FREEZEBLAST)
						//if (actortype[targ->spr.picnum] == 0) //TRANSITIONAL. Cannot be done right with EDuke mess backing the engine. 
						{
							auto spawned = spawn(proj, JIBS6);
							if (spawned)
							{
								if (proj->spr.pal == 6)
									spawned->spr.pal = 6;
								spawned->spr.pos.Z += 4;
								spawned->spr.xvel = 16;
								spawned->spr.xrepeat = spawned->spr.yrepeat = 24;
								spawned->add_int_ang(32 - (krand() & 63));
							}
						}

				auto Owner = proj->GetOwner();

				if (Owner && Owner->spr.picnum == APLAYER && targ->spr.picnum != DRONE)
					if (ps[Owner->PlayerIndex()].curr_weapon == SHOTGUN_WEAPON)
					{
						fi.shoot(targ, BLOODSPLAT3);
						fi.shoot(targ, BLOODSPLAT1);
						fi.shoot(targ, BLOODSPLAT2);
						fi.shoot(targ, BLOODSPLAT4);
					}

				if (targ->spr.statnum == 2)
				{
					ChangeActorStat(targ, 1);
					targ->timetosleep = SLEEPTIME;
				}
			}

			if (targ->spr.statnum != 2)
			{
				if (proj->spr.picnum == FREEZEBLAST && ((targ->spr.picnum == APLAYER && targ->spr.pal == 1) || (gs.freezerhurtowner == 0 && proj->GetOwner() == targ)))
					return;

				targ->attackertype = proj->spr.picnum;
				targ->hitextra += proj->spr.extra;
				if (targ->spr.picnum != COW)
					targ->hitang = proj->int_ang();
				targ->SetHitOwner(proj->GetOwner());
			}

			if (targ->spr.statnum == 10)
			{
				p = targ->spr.yvel;
				if (ps[p].newOwner != nullptr)
				{
					ps[p].newOwner = nullptr;
					ps[p].restorexyz();

					updatesector(ps[p].player_int_pos().X, ps[p].player_int_pos().Y, &ps[p].cursector);

					DukeStatIterator it(STAT_EFFECTOR);
					while (auto act = it.Next())
					{
						if (actorflag(act, SFLAG2_CAMERA)) act->spr.yvel = 0;
					}
				}
				auto Owner = targ->GetHitOwner();
				if (!Owner || Owner->spr.picnum != APLAYER)
					if (ud.player_skill >= 3)
						proj->spr.extra += (proj->spr.extra >> 1);
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
	player_struct* p;
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
		if (!isRRRA() || !RRRA_ExitedLevel)
		{
			setnextmap(false);
			RRRA_ExitedLevel = 1;
		}
		return;
	case -2:
		p->cursector->lotag = 0;
		p->timebeforeexit = 26 * 8;
		p->customexitsound = p->cursector->hitag;
		return;
	default:
		if (p->cursector->lotag >= 10000)
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


	if (!(PlayerInput(snum, SB_OPEN)))
		p->toggle_key_flag = 0;

	else if (!p->toggle_key_flag)
	{
		near.hitActor = nullptr;
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

			if ((hitscanwall->cstat & CSTAT_WALL_MASKED))
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
			neartag(p->player_int_pos(), p->GetActor()->sector(), p->angle.oang.Buildang(), near , 1280, 3);
		}

		if (p->newOwner != nullptr)
			neartag({ p->player_int_opos().X, p->player_int_opos().Y, p->player_int_opos().Z }, p->GetActor()->sector(), p->angle.oang.Buildang(), near, 1280L, 1);
		else
		{
			neartag(p->player_int_pos(), p->GetActor()->sector(), p->angle.oang.Buildang(), near, 1280, 1);
			if (near.actor() == nullptr && near.hitWall == nullptr && near.hitSector == nullptr)
				neartag({ p->player_int_pos().X, p->player_int_pos().Y, p->player_int_pos().Z + (8 << 8) }, p->GetActor()->sector(), p->angle.oang.Buildang(), near, 1280, 1);
			if (near.actor() == nullptr && near.hitWall == nullptr && near.hitSector == nullptr)
				neartag({ p->player_int_pos().X, p->player_int_pos().Y, p->player_int_pos().Z + (16 << 8) }, p->GetActor()->sector(), p->angle.oang.Buildang(), near, 1280, 1);
			if (near.actor() == nullptr && near.hitWall == nullptr && near.hitSector == nullptr)
			{
				neartag({ p->player_int_pos().X, p->player_int_pos().Y, p->player_int_pos().Z + (16 << 8) }, p->GetActor()->sector(), p->angle.oang.Buildang(), near, 1280, 3);
				if (near.actor() != nullptr)
				{
					switch (near.actor()->spr.picnum)
					{
					case FEM10:
					case NAKED1:
					case STATUE:
					case TOUGHGAL:
						return;
					case COW:
						near.actor()->spriteextra = 1;
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
				neartagsprite->spr.extra = 60;
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
					if (p->GetActor()->spr.extra <= (gs.max_player_health - (gs.max_player_health / 10)))
					{
						p->GetActor()->spr.extra += gs.max_player_health / 10;
						p->last_extra = p->GetActor()->spr.extra;
					}
					else if (p->GetActor()->spr.extra < gs.max_player_health)
						p->GetActor()->spr.extra = gs.max_player_health;
				}
				else if (S_CheckActorSoundPlaying(pact, DUKE_GRUNT) == 0)
					S_PlayActorSound(DUKE_GRUNT, pact);
				return;
			case WATERFOUNTAIN:
				if (neartagsprite->temp_data[0] != 1)
				{
					neartagsprite->temp_data[0] = 1;
					neartagsprite->SetOwner(p->GetActor());

					if (p->GetActor()->spr.extra < gs.max_player_health)
					{
						p->GetActor()->spr.extra++;
						S_PlayActorSound(DUKE_DRINKING, pact);
					}
				}
				return;
			case PLUG:
				S_PlayActorSound(SHORT_CIRCUIT, pact);
				p->GetActor()->spr.extra -= 2 + (krand() & 3);
				SetPlayerPal(p, PalEntry(32, 48, 48, 64));
				break;
			}
		}

		if (!PlayerInput(snum, SB_OPEN)) return;

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
		}

		if (near.hitSector && (near.hitSector->lotag & 16384) == 0 && isanearoperator(near.hitSector->lotag))
		{
			DukeSectIterator it(near.hitSector);
			while (auto act = it.Next())
			{
				if (act->spr.picnum == ACTIVATOR || act->spr.picnum == MASTERSWITCH)
					return;
			}
			if (haskey(near.hitSector, snum))
				operatesectors(near.hitSector, p->GetActor());
			else
			{
				if (neartagsprite && neartagsprite->spriteextra > 3)
					S_PlayActorSound(99, pact);
				else
					S_PlayActorSound(419, pact);
				FTA(41, p);
			}
		}
		else if ((p->GetActor()->sector()->lotag & 16384) == 0)
		{
			if (isanunderoperator(p->GetActor()->sector()->lotag))
			{
				DukeSectIterator it(p->GetActor()->sector());
				while (auto act = it.Next())
				{
					if (act->spr.picnum == ACTIVATOR || act->spr.picnum == MASTERSWITCH)
						return;
				}
				if (haskey(near.hitSector, snum))
					operatesectors(p->GetActor()->sector(), p->GetActor());
				else
				{
					if (neartagsprite && neartagsprite->spriteextra > 3)
						S_PlayActorSound(99, pact);
					else
						S_PlayActorSound(419, pact);
					FTA(41, p);
				}
			}
			else fi.checkhitswitch(snum, near.hitWall, nullptr);
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
	assert(wlwal->twoSided());
	auto nextsect = wlwal->nextSector();

	double movestep = min(sectp->hitag * maptoworld, 1.);
	if (movestep == 0) movestep = 4 * maptoworld;

	double max_x = INT32_MIN, max_y = INT32_MIN, min_x = INT32_MAX, min_y = INT32_MAX;
	for (auto& wal : wallsofsector(nextsect))
	{
		double x = wal.pos.X;
		double y = wal.pos.Y;
		if (x > max_x)
			max_x = x;
		if (y > max_y)
			max_y = y;
		if (x < min_x)
			min_x = x;
		if (y < min_y)
			min_y = y;
	}

	double margin = movestep + maptoworld;
	max_x += margin;
	max_y += margin;
	min_x -= margin;
	min_y -= margin;
	int pos_ok = 1;
	if (!inside(max_x, max_y, sectp) ||
		!inside(max_x, min_y, sectp) ||
		!inside(min_x, min_y, sectp) ||
		!inside(min_x, max_y, sectp))
		pos_ok = 0;

	for (auto& wal : wallsofsector(nextsect))
	{
		switch (wlwal->lotag)
		{
		case 42:
		case 41:
		case 40:
		case 43:
			vertexscan(&wal, [=](walltype* w)
				{
					StartInterpolation(w, wlwal->lotag == 41 || wlwal->lotag == 43 ? Interp_Wall_X : Interp_Wall_Y);
				});
			break;
		}
	}

	if (pos_ok)
	{
		if (S_CheckActorSoundPlaying(ps[snum].GetActor(), 389) == 0)
			S_PlayActorSound(389, ps[snum].GetActor());
		for(auto& wal : wallsofsector(nextsect))
		{
			auto vec = wal.pos;
			switch (wlwal->lotag)
			{
			case 42:
				vec.Y += movestep;
				dragpoint(&wal, vec);
				break;
			case 41:
				vec.X -= movestep;
				dragpoint(&wal, vec);
				break;
			case 40:
				vec.Y -= movestep;
				dragpoint(&wal, vec);
				break;
			case 43:
				vec.X += movestep;
				dragpoint(&wal, vec);
				break;
			}
		}
	}
	else
	{
		movestep -= 2 * maptoworld;
		for(auto& wal : wallsofsector(nextsect))
		{
			auto vec = wal.pos;
			switch (wlwal->lotag)
			{
			case 42:
				vec.Y -= movestep;
				dragpoint(&wal, vec);
				break;
			case 41:
				vec.X += movestep;
				dragpoint(&wal, vec);
				break;
			case 40:
				vec.Y += movestep;
				dragpoint(&wal, vec);
				break;
			case 43:
				vec.X -= movestep;
				dragpoint(&wal, vec);
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

void tearitup(sectortype* sect)
{
	DukeSectIterator it(sect);
	while (auto act = it.Next())
	{
		if (act->spr.picnum == DESTRUCTO)
		{
			act->attackertype = SHOTSPARK1;
			act->hitextra = 1;
		}
	}
}
END_DUKE_NS
