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
#include "vm.h"

// PRIMITIVE
BEGIN_DUKE_NS

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
			if (wal.picnum == RTILE_RRTILE7873)
				wal.addxpan(6);
			else if (wal.picnum == RTILE_RRTILE7870)
				wal.addxpan(6);
		}
	}

	for (int p = 0; p < numanimwalls; p++)
	{
		auto wal = animwall[p].wall;
		int j = wal->picnum;

		switch (j)
		{
		case RTILE_SCREENBREAK1:
		case RTILE_SCREENBREAK2:
		case RTILE_SCREENBREAK3:
		case RTILE_SCREENBREAK4:
		case RTILE_SCREENBREAK5:

		case RTILE_SCREENBREAK9:
		case RTILE_SCREENBREAK10:
		case RTILE_SCREENBREAK11:
		case RTILE_SCREENBREAK12:
		case RTILE_SCREENBREAK13:

			if ((krand() & 255) < 16)
			{
				animwall[p].tag = wal->picnum;
				wal->picnum = RTILE_SCREENBREAK6;
			}

			continue;

		case RTILE_SCREENBREAK6:
		case RTILE_SCREENBREAK7:
		case RTILE_SCREENBREAK8:

			if (animwall[p].tag >= 0)
				wal->picnum = animwall[p].tag;
			else
			{
				wal->picnum++;
				if (wal->picnum == (RTILE_SCREENBREAK6 + 3))
					wal->picnum = RTILE_SCREENBREAK6;
			}
			continue;

		}

		if (wal->cstat & CSTAT_WALL_MASKED)
			switch (wal->overpicnum)
			{
			case RTILE_W_FORCEFIELD:
			case RTILE_W_FORCEFIELD + 1:
			case RTILE_W_FORCEFIELD + 2:

				t = animwall[p].tag;

				if (wal->cstat & CSTAT_WALL_ANY_EXCEPT_BLOCK)
				{
					wal->addxpan(-t / 4096.f);
					wal->addypan(-t / 4096.f);

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
							wal->overpicnum = RTILE_W_FORCEFIELD;
						else wal->overpicnum = RTILE_W_FORCEFIELD + 1;
					}
					else
					{
						if ((krand() & 255) < 32)
							animwall[p].tag = 128 << (krand() & 3);
						else wal->overpicnum = RTILE_W_FORCEFIELD + 1;
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

void operateforcefields_r(DDukeActor* act, int low)
{
	operateforcefields_common(act, low, { RTILE_BIGFORCE });
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
	DVector2 pos;

	if (wwal == nullptr && act == nullptr) return 0;
	correctdips = 1;
	numdips = 0;

	if (act)
	{
		lotag = act->spr.lotag;
		if (lotag == 0) return 0;
		hitag = act->spr.hitag;
		pos = act->spr.pos.XY();
		picnum = act->spr.picnum;
		switchpal = act->spr.pal;
	}
	else
	{
		lotag = wwal->lotag;
		if (lotag == 0) return 0;
		hitag = wwal->hitag;
		pos = wwal->pos;
		picnum = wwal->picnum;
		switchpal = wwal->pal;
	}

	switch (picnum)
	{
	case RTILE_DIPSWITCH:
	case RTILE_DIPSWITCHON:
	case RTILE_TECHSWITCH:
	case RTILE_TECHSWITCHON:
	case RTILE_ALIENSWITCH:
	case RTILE_ALIENSWITCHON:
		break;
	case RTILE_ACCESSSWITCH:
	case RTILE_ACCESSSWITCH2:
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

	case RTILE_MULTISWITCH2:
	case RTILE_MULTISWITCH2_2:
	case RTILE_MULTISWITCH2_3:
	case RTILE_MULTISWITCH2_4:
	case RTILE_IRONWHEELSWITCH:
	case RTILE_RRTILE8660:
		if (!isRRRA()) break;
		[[fallthrough]];
	case RTILE_DIPSWITCH2:
	case RTILE_DIPSWITCH2ON:
	case RTILE_DIPSWITCH3:
	case RTILE_DIPSWITCH3ON:
	case RTILE_MULTISWITCH:
	case RTILE_MULTISWITCH_2:
	case RTILE_MULTISWITCH_3:
	case RTILE_MULTISWITCH_4:
	case RTILE_PULLSWITCH:
	case RTILE_PULLSWITCHON:
	case RTILE_HANDSWITCH:
	case RTILE_HANDSWITCHON:
	case RTILE_SLOTDOOR:
	case RTILE_SLOTDOORON:
	case RTILE_LIGHTSWITCH:
	case RTILE_LIGHTSWITCHON:
	case RTILE_SPACELIGHTSWITCH:
	case RTILE_SPACELIGHTSWITCHON:
	case RTILE_SPACEDOORSWITCH:
	case RTILE_SPACEDOORSWITCHON:
	case RTILE_FRANKENSTINESWITCH:
	case RTILE_FRANKENSTINESWITCHON:
	case RTILE_LIGHTSWITCH2:
	case RTILE_LIGHTSWITCH2ON:
	case RTILE_POWERSWITCH1:
	case RTILE_POWERSWITCH1ON:
	case RTILE_LOCKSWITCH1:
	case RTILE_LOCKSWITCH1ON:
	case RTILE_POWERSWITCH2:
	case RTILE_POWERSWITCH2ON:
	case RTILE_CHICKENPLANTBUTTON:
	case RTILE_CHICKENPLANTBUTTONON:
	case RTILE_RRTILE2214:
	case RTILE_RRTILE2697:
	case RTILE_RRTILE2697 + 1:
	case RTILE_RRTILE2707:
	case RTILE_RRTILE2707 + 1:
		goOn1:
		if (check_activator_motion(lotag)) return 0;
		break;
	default:
		if (isadoorwall(picnum) == 0) return 0;
		break;
	}

	DukeStatIterator it(STAT_DEFAULT);
	while (auto other = it.Next())
	{
		if (lotag == other->spr.lotag) switch (other->spr.picnum)
		{
		case RTILE_DIPSWITCH:
		case RTILE_TECHSWITCH:
		case RTILE_ALIENSWITCH:
			if (act && act == other) other->spr.picnum++;
			else if (other->spr.hitag == 0) correctdips++;
			numdips++;
			break;
		case RTILE_TECHSWITCHON:
		case RTILE_DIPSWITCHON:
		case RTILE_ALIENSWITCHON:
			if (act && act == other) other->spr.picnum--;
			else if (other->spr.hitag == 1) correctdips++;
			numdips++;
			break;
		case RTILE_MULTISWITCH:
		case RTILE_MULTISWITCH_2:
		case RTILE_MULTISWITCH_3:
		case RTILE_MULTISWITCH_4:
			other->spr.picnum++;
			if (other->spr.picnum > (RTILE_MULTISWITCH_4))
				other->spr.picnum = RTILE_MULTISWITCH;
			break;
		case RTILE_MULTISWITCH2:
		case RTILE_MULTISWITCH2_2:
		case RTILE_MULTISWITCH2_3:
		case RTILE_MULTISWITCH2_4:
			if (!isRRRA()) break;
			other->spr.picnum++;
			if (other->spr.picnum > (RTILE_MULTISWITCH2_4))
				other->spr.picnum = RTILE_MULTISWITCH2;
			break;

		case RTILE_RRTILE2214:
			other->spr.picnum++;
			break;
		case RTILE_RRTILE8660:
			if (!isRRRA()) break;
			[[fallthrough]];
		case RTILE_ACCESSSWITCH:
		case RTILE_ACCESSSWITCH2:
		case RTILE_SLOTDOOR:
		case RTILE_LIGHTSWITCH:
		case RTILE_SPACELIGHTSWITCH:
		case RTILE_SPACEDOORSWITCH:
		case RTILE_FRANKENSTINESWITCH:
		case RTILE_LIGHTSWITCH2:
		case RTILE_POWERSWITCH1:
		case RTILE_LOCKSWITCH1:
		case RTILE_POWERSWITCH2:
		case RTILE_HANDSWITCH:
		case RTILE_PULLSWITCH:
		case RTILE_DIPSWITCH2:
		case RTILE_DIPSWITCH3:
		case RTILE_CHICKENPLANTBUTTON:
		case RTILE_RRTILE2697:
		case RTILE_RRTILE2707:
			if (other->spr.picnum == RTILE_DIPSWITCH3)
				if (other->spr.hitag == 999)
				{
					DukeStatIterator it1(STAT_LUMBERMILL);
					while (auto other2 = it1.Next())
					{
						CallOnUse(other2, nullptr);
					}
					other->spr.picnum++;
					break;
				}
			if (other->spr.picnum == RTILE_CHICKENPLANTBUTTON)
				ud.chickenplant = 0;
			if (other->spr.picnum == RTILE_RRTILE8660)
			{
				BellTime = 132;
				BellSprite = other;
			}
			other->spr.picnum++;
			break;
		case RTILE_PULLSWITCHON:
		case RTILE_HANDSWITCHON:
		case RTILE_LIGHTSWITCH2ON:
		case RTILE_POWERSWITCH1ON:
		case RTILE_LOCKSWITCH1ON:
		case RTILE_POWERSWITCH2ON:
		case RTILE_SLOTDOORON:
		case RTILE_LIGHTSWITCHON:
		case RTILE_SPACELIGHTSWITCHON:
		case RTILE_SPACEDOORSWITCHON:
		case RTILE_FRANKENSTINESWITCHON:
		case RTILE_DIPSWITCH2ON:
		case RTILE_DIPSWITCH3ON:
		case RTILE_CHICKENPLANTBUTTONON:
		case RTILE_RRTILE2697 + 1:
		case RTILE_RRTILE2707 + 1:
			if (other->spr.picnum == RTILE_CHICKENPLANTBUTTONON)
				ud.chickenplant = 1;
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
			case RTILE_DIPSWITCH:
			case RTILE_TECHSWITCH:
			case RTILE_ALIENSWITCH:
				if (!act && &wal == wwal) wal.picnum++;
				else if (wal.hitag == 0) correctdips++;
				numdips++;
				break;
			case RTILE_DIPSWITCHON:
			case RTILE_TECHSWITCHON:
			case RTILE_ALIENSWITCHON:
				if (!act && &wal == wwal) wal.picnum--;
				else if (wal.hitag == 1) correctdips++;
				numdips++;
				break;
			case RTILE_MULTISWITCH:
			case RTILE_MULTISWITCH_2:
			case RTILE_MULTISWITCH_3:
			case RTILE_MULTISWITCH_4:
				wal.picnum++;
				if (wal.picnum > (RTILE_MULTISWITCH_4))
					wal.picnum = RTILE_MULTISWITCH;
				break;
			case RTILE_MULTISWITCH2:
			case RTILE_MULTISWITCH2_2:
			case RTILE_MULTISWITCH2_3:
			case RTILE_MULTISWITCH2_4:
				if (!isRRRA()) break;
				wal.picnum++;
				if (wal.picnum > (RTILE_MULTISWITCH2_4))
					wal.picnum = RTILE_MULTISWITCH2;
				break;
			case RTILE_RRTILE8660:
				if (!isRRRA()) break;
				[[fallthrough]];
			case RTILE_ACCESSSWITCH:
			case RTILE_ACCESSSWITCH2:
			case RTILE_SLOTDOOR:
			case RTILE_LIGHTSWITCH:
			case RTILE_SPACELIGHTSWITCH:
			case RTILE_SPACEDOORSWITCH:
			case RTILE_LIGHTSWITCH2:
			case RTILE_POWERSWITCH1:
			case RTILE_LOCKSWITCH1:
			case RTILE_POWERSWITCH2:
			case RTILE_PULLSWITCH:
			case RTILE_HANDSWITCH:
			case RTILE_DIPSWITCH2:
			case RTILE_DIPSWITCH3:
			case RTILE_RRTILE2697:
			case RTILE_RRTILE2707:
				wal.picnum++;
				break;
			case RTILE_HANDSWITCHON:
			case RTILE_PULLSWITCHON:
			case RTILE_LIGHTSWITCH2ON:
			case RTILE_POWERSWITCH1ON:
			case RTILE_LOCKSWITCH1ON:
			case RTILE_POWERSWITCH2ON:
			case RTILE_SLOTDOORON:
			case RTILE_LIGHTSWITCHON:
			case RTILE_SPACELIGHTSWITCHON:
			case RTILE_SPACEDOORSWITCHON:
			case RTILE_DIPSWITCH2ON:
			case RTILE_DIPSWITCH3ON:
			case RTILE_RRTILE2697 + 1:
			case RTILE_RRTILE2707 + 1:
				wal.picnum--;
				break;
			}
	}

	if (lotag == -1)
	{
		setnextmap(false);
	}

	DVector3 v(pos, ps[snum].GetActor()->getOffsetZ());
	switch (picnum)
	{
	default:
		if (isadoorwall(picnum) == 0) break;
		[[fallthrough]];
	case RTILE_DIPSWITCH:
	case RTILE_DIPSWITCHON:
	case RTILE_TECHSWITCH:
	case RTILE_TECHSWITCHON:
	case RTILE_ALIENSWITCH:
	case RTILE_ALIENSWITCHON:
		if (picnum == RTILE_DIPSWITCH || picnum == RTILE_DIPSWITCHON ||
			picnum == RTILE_ALIENSWITCH || picnum == RTILE_ALIENSWITCHON ||
			picnum == RTILE_TECHSWITCH || picnum == RTILE_TECHSWITCHON)
		{
			if (picnum == RTILE_ALIENSWITCH || picnum == RTILE_ALIENSWITCHON)
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
	case RTILE_MULTISWITCH2:
	case RTILE_MULTISWITCH2_2:
	case RTILE_MULTISWITCH2_3:
	case RTILE_MULTISWITCH2_4:
	case RTILE_IRONWHEELSWITCH:
	case RTILE_RRTILE8660:
		if (!isRRRA()) break;
		[[fallthrough]];
	case RTILE_DIPSWITCH2:
	case RTILE_DIPSWITCH2ON:
	case RTILE_DIPSWITCH3:
	case RTILE_DIPSWITCH3ON:
	case RTILE_MULTISWITCH:
	case RTILE_MULTISWITCH_2:
	case RTILE_MULTISWITCH_3:
	case RTILE_MULTISWITCH_4:
	case RTILE_ACCESSSWITCH:
	case RTILE_ACCESSSWITCH2:
	case RTILE_SLOTDOOR:
	case RTILE_SLOTDOORON:
	case RTILE_LIGHTSWITCH:
	case RTILE_LIGHTSWITCHON:
	case RTILE_SPACELIGHTSWITCH:
	case RTILE_SPACELIGHTSWITCHON:
	case RTILE_SPACEDOORSWITCH:
	case RTILE_SPACEDOORSWITCHON:
	case RTILE_FRANKENSTINESWITCH:
	case RTILE_FRANKENSTINESWITCHON:
	case RTILE_LIGHTSWITCH2:
	case RTILE_LIGHTSWITCH2ON:
	case RTILE_POWERSWITCH1:
	case RTILE_POWERSWITCH1ON:
	case RTILE_LOCKSWITCH1:
	case RTILE_LOCKSWITCH1ON:
	case RTILE_POWERSWITCH2:
	case RTILE_POWERSWITCH2ON:
	case RTILE_HANDSWITCH:
	case RTILE_HANDSWITCHON:
	case RTILE_PULLSWITCH:
	case RTILE_PULLSWITCHON:
	case RTILE_RRTILE2697:
	case RTILE_RRTILE2697 + 1:
	case RTILE_RRTILE2707:
	case RTILE_RRTILE2707 + 1:
		goOn2:
		if (isRRRA())
		{
			if (picnum == RTILE_RRTILE8660 && act)
			{
				BellTime = 132;
				BellSprite = act;
				act->spr.picnum++;
			}
			else if (picnum == RTILE_IRONWHEELSWITCH)
			{
				act->spr.picnum = act->spr.picnum + 1;
				if (hitag == 10001)
				{
					if (ps[snum].SeaSick == 0)
						ps[snum].SeaSick = 350;
					operateactivators(668, &ps[snum]);
					operatemasterswitches(668);
					S_PlayActorSound(328, ps[snum].GetActor());
					return 1;
				}
			}
			else if (hitag == 10000)
			{
				if (picnum == RTILE_MULTISWITCH || picnum == (RTILE_MULTISWITCH_2) ||
					picnum == (RTILE_MULTISWITCH_3) || picnum == (RTILE_MULTISWITCH_4) ||
					picnum == RTILE_MULTISWITCH2 || picnum == (RTILE_MULTISWITCH2_2) ||
					picnum == (RTILE_MULTISWITCH2_3) || picnum == (RTILE_MULTISWITCH2_4))
				{
					DDukeActor* switches[3];
					int switchcount = 0, j;
					S_PlaySound3D(SWITCH_ON, act, v);
					DukeSpriteIterator itr;
					while (auto actt = itr.Next())
					{
						int jpn = actt->spr.picnum;
						int jht = actt->spr.hitag;
						if ((jpn == RTILE_MULTISWITCH || jpn == RTILE_MULTISWITCH2) && jht == 10000)
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
							if (picnum >= RTILE_MULTISWITCH2)
								switches[j]->spr.picnum = RTILE_MULTISWITCH2_4;
							else
								switches[j]->spr.picnum = RTILE_MULTISWITCH_4;
							checkhitswitch_r(snum, nullptr, switches[j]);
						}
					}
					return 1;
				}
			}
		}
		if (picnum == RTILE_MULTISWITCH || picnum == (RTILE_MULTISWITCH_2) ||
			picnum == (RTILE_MULTISWITCH_3) || picnum == (RTILE_MULTISWITCH_4))
			lotag += picnum - RTILE_MULTISWITCH;
		if (isRRRA())
		{
			if (picnum == RTILE_MULTISWITCH2 || picnum == (RTILE_MULTISWITCH2_2) ||
				picnum == (RTILE_MULTISWITCH2_3) || picnum == (RTILE_MULTISWITCH2_4))
				lotag += picnum - RTILE_MULTISWITCH2;
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

		operateactivators(lotag, &ps[snum]);
		fi.operateforcefields(ps[snum].GetActor(), lotag);
		operatemasterswitches(lotag);

		if (picnum == RTILE_DIPSWITCH || picnum == RTILE_DIPSWITCHON ||
			picnum == RTILE_ALIENSWITCH || picnum == RTILE_ALIENSWITCHON ||
			picnum == RTILE_TECHSWITCH || picnum == RTILE_TECHSWITCHON) return 1;

		if (hitag == 0 && isadoorwall(picnum) == 0)
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
		if (isactivator(act))
		{
			operateactivators(act->spr.lotag, nullptr);
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
	sectortype* sect = nullptr;

	if (wal == nullptr)
	{
		for (int j = n - 1; j >= 0; j--)
		{
			DAngle a = actor->spr.Angles.Yaw - DAngle45 + DAngle180 + randomAngle(90);
			auto vel = krandf(4) + 2;
			auto zvel = 4 - krandf(4);

			CreateActor(actor->sector(), actor->spr.pos, RTILE_POPCORN, -32, DVector2(0.5625, 0.5625), a, vel, zvel, actor, 5);
		}
		return;
	}

	auto pos = wal->pos;
	auto delta = wal->delta() / (n + 1);

	pos.X -= Sgn(delta.X) * maptoworld;
	pos.Y += Sgn(delta.Y) * maptoworld;

	for (int j = n; j > 0; j--)
	{
		pos += delta;
		sect = actor->sector();
		updatesector(DVector3(pos, sect->floorz), &sect);
		if (sect)
		{
			double z = sect->floorz - krandf(abs(sect->ceilingz - sect->floorz));
			if (abs(z) > 32)
				z = actor->spr.pos.Z - 32 + krandf(64);
			DAngle a = actor->spr.Angles.Yaw - DAngle180;
			auto vel = krandf(4) + 2;
			auto zvel = -krandf(4);

			CreateActor(actor->sector(), DVector3(pos, z), RTILE_POPCORN, -32, DVector2(0.5625, 0.5625), a, vel, zvel, actor, 5);
		}
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checkhitwall_r(DDukeActor* spr, walltype* wal, const DVector3& pos, int atwith)
{
	int j;
	int darkestwall;

	if (wal->overpicnum == RTILE_MIRROR && atwith != -1 && gs.actorinfo[atwith].flags2 & SFLAG2_BREAKMIRRORS)
	{
		lotsofglass(spr, wal, 70);
		wal->cstat &= ~CSTAT_WALL_MASKED;
		wal->overpicnum = RTILE_MIRRORBROKE;
		wal->portalflags = 0;
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;
	}

	if (((wal->cstat & CSTAT_WALL_MASKED) || wal->overpicnum == RTILE_BIGFORCE) && wal->twoSided())
		if (wal->nextSector()->floorz > pos.Z)
			if (wal->nextSector()->floorz - wal->nextSector()->ceilingz)
				switch (wal->overpicnum)
				{
				case RTILE_FANSPRITE:
					wal->overpicnum = RTILE_FANSPRITEBROKE;
					wal->cstat &= ~(CSTAT_WALL_BLOCK | CSTAT_WALL_BLOCK_HITSCAN);
					if (wal->twoSided())
					{
						wal->nextWall()->overpicnum = RTILE_FANSPRITEBROKE;
						wal->nextWall()->cstat &= ~(CSTAT_WALL_BLOCK | CSTAT_WALL_BLOCK_HITSCAN);
					}
					S_PlayActorSound(VENT_BUST, spr);
					S_PlayActorSound(GLASS_BREAKING, spr);
					return;

				case RTILE_RRTILE1973:
				{
					sectortype* sptr = nullptr;
					updatesector(pos, &sptr);
					if (sptr == nullptr) return;
					wal->overpicnum = RTILE_GLASS2;
					lotsofpopcorn(spr, wal, 64);
					wal->cstat = 0;

					if (wal->twoSided())
						wal->nextWall()->cstat = 0;

					auto spawned = CreateActor(sptr, pos, SECTOREFFECTOR, 0, DVector2(0, 0), ps[0].GetActor()->spr.Angles.Yaw, 0., 0., spr, 3);
					if (spawned)
					{
						spawned->spr.lotag = SE_128_GLASS_BREAKING;
						spawned->temp_walls[0] = wal;
						S_PlayActorSound(GLASS_BREAKING, spawned);
					}
					return;
				}
				case RTILE_GLASS:
				{
					sectortype* sptr = nullptr;
					updatesector(pos, &sptr);
					if (sptr == nullptr) return;
					wal->overpicnum = RTILE_GLASS2;
					lotsofglass(spr, wal, 10);
					wal->cstat = 0;

					if (wal->twoSided())
						wal->nextWall()->cstat = 0;

					auto spawned = CreateActor(sptr, pos, SECTOREFFECTOR, 0, DVector2(0, 0), ps[0].GetActor()->spr.Angles.Yaw, 0., 0., spr, 3);
					if (spawned)
					{
						spawned->spr.lotag = SE_128_GLASS_BREAKING;
						spawned->temp_data[1] = 2;
						spawned->temp_walls[0] = wal;
						S_PlayActorSound(GLASS_BREAKING, spawned);
					}
					return;
				}
				case RTILE_STAINGLASS1:
				{
					sectortype* sptr = nullptr;
					updatesector(pos, &sptr);
					if (sptr == nullptr) return;
					lotsofcolourglass(spr, wal, 80);
					wal->cstat = 0;
					if (wal->twoSided())
						wal->nextWall()->cstat = 0;
					S_PlayActorSound(VENT_BUST, spr);
					S_PlayActorSound(GLASS_BREAKING, spr);
					return;
				}
				}

	auto data = breakWallMap.CheckKey(wal->picnum);
	if (data)
	{
		if (!data->handler)
		{
			wal->picnum = data->brokentex;
			S_PlayActorSound(data->breaksound, spr);
		}
		else
		{
			VMValue args[4] = { wal, data->brokentex, data->breaksound.index(), spr};
			VMCall(data->handler, args, 4, nullptr, 0);
		}
	}
	else switch (wal->picnum)
	{
	case RTILE_IRONWHEELSWITCH:
		if (isRRRA()) break;
		break;
	case RTILE_PICKUPSIDE:
	case RTILE_PICKUPFRONT:
	case RTILE_PICKUPBACK1:
	case RTILE_PICKUPBACK2:
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
					for(auto& wl : act->sector()->walls)
					{
						if (wl.twoSided()) wl.nextSector()->lotag = 0;
					}
					act->sector()->lotag = 0;
					S_StopSound(act->spr.lotag);
					S_PlayActorSound(400, act);
					act->Destroy();
				}
			}
		}
		return;
	}
	case RTILE_COLAMACHINE:
	case RTILE_VENDMACHINE:
		breakwall(wal->picnum + 2, spr, wal);
		S_PlayActorSound(GLASS_BREAKING, spr);
		return;

	case RTILE_OJ:

	case RTILE_SCREENBREAK6:
	case RTILE_SCREENBREAK7:
	case RTILE_SCREENBREAK8:

		lotsofglass(spr, wal, 30);
		wal->picnum = RTILE_W_SCREENBREAK + (krand() % (isRRRA() ? 2 : 3));
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		return;

	case RTILE_ATM:
		wal->picnum = RTILE_ATMBROKE;
		fi.lotsofmoney(spr, 1 + (krand() & 7));
		S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		break;

	case RTILE_WALLLIGHT1:
	case RTILE_WALLLIGHT3:
	case RTILE_WALLLIGHT4:
	case RTILE_TECHLIGHT2:
	case RTILE_TECHLIGHT4:
	case RTILE_RRTILE1814:
	case RTILE_RRTILE1939:
	case RTILE_RRTILE1986:
	case RTILE_RRTILE1988:
	case RTILE_RRTILE2123:
	case RTILE_RRTILE2125:
	case RTILE_RRTILE2636:
	case RTILE_RRTILE2878:
	case RTILE_RRTILE2898:
	case RTILE_RRTILE3200:
	case RTILE_RRTILE3202:
	case RTILE_RRTILE3204:
	case RTILE_RRTILE3206:
	case RTILE_RRTILE3208:

		if (rnd(128))
			S_PlayActorSound(GLASS_HEAVYBREAK, spr);
		else S_PlayActorSound(GLASS_BREAKING, spr);
		lotsofglass(spr, wal, 30);

		if (wal->picnum == RTILE_RRTILE1814)
			wal->picnum = RTILE_RRTILE1817;

		if (wal->picnum == RTILE_RRTILE1986)
			wal->picnum = RTILE_RRTILE1987;

		if (wal->picnum == RTILE_RRTILE1939)
			wal->picnum = RTILE_RRTILE2004;

		if (wal->picnum == RTILE_RRTILE1988)
			wal->picnum = RTILE_RRTILE2005;

		if (wal->picnum == RTILE_RRTILE2898)
			wal->picnum = RTILE_RRTILE2899;

		if (wal->picnum == RTILE_RRTILE2878)
			wal->picnum = RTILE_RRTILE2879;

		if (wal->picnum == RTILE_RRTILE2123)
			wal->picnum = RTILE_RRTILE2124;

		if (wal->picnum == RTILE_RRTILE2125)
			wal->picnum = RTILE_RRTILE2126;

		if (wal->picnum == RTILE_RRTILE3200)
			wal->picnum = RTILE_RRTILE3201;

		if (wal->picnum == RTILE_RRTILE3202)
			wal->picnum = RTILE_RRTILE3203;

		if (wal->picnum == RTILE_RRTILE3204)
			wal->picnum = RTILE_RRTILE3205;

		if (wal->picnum == RTILE_RRTILE3206)
			wal->picnum = RTILE_RRTILE3207;

		if (wal->picnum == RTILE_RRTILE3208)
			wal->picnum = RTILE_RRTILE3209;

		if (wal->picnum == RTILE_RRTILE2636)
			wal->picnum = RTILE_RRTILE2637;

		if (wal->picnum == RTILE_WALLLIGHT1)
			wal->picnum = RTILE_WALLLIGHTBUST1;

		if (wal->picnum == RTILE_WALLLIGHT3)
			wal->picnum = RTILE_WALLLIGHTBUST3;

		if (wal->picnum == RTILE_WALLLIGHT4)
			wal->picnum = RTILE_WALLLIGHTBUST4;

		if (wal->picnum == RTILE_TECHLIGHT2)
			wal->picnum = RTILE_TECHLIGHTBUST2;

		if (wal->picnum == RTILE_TECHLIGHT4)
			wal->picnum = RTILE_TECHLIGHTBUST4;

		if (!wal->lotag) return;

		if (!wal->twoSided()) return;
		darkestwall = 0;

		for (auto& wl : wal->nextSector()->walls)
			if (wl.shade > darkestwall)
				darkestwall = wl.shade;

		j = krand() & 1;
		DukeStatIterator it(STAT_EFFECTOR);
		while (auto act = it.Next())
		{
			if (act->spr.hitag == wal->lotag && act->spr.lotag == SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT)
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
		CallOnHurt(coll.actor(), p);
		return;
	}

	if (coll.type == kHitWall)
	{
		auto wal = coll.hitWall;

		if (p->hurt_delay > 0) p->hurt_delay--;
		else if (wal->cstat & (CSTAT_WALL_BLOCK | CSTAT_WALL_ALIGN_BOTTOM | CSTAT_WALL_MASKED | CSTAT_WALL_BLOCK_HITSCAN)) switch (wal->overpicnum)
		{
		case RTILE_BIGFORCE:
			p->hurt_delay = 26;
			fi.checkhitwall(p->GetActor(), wal, p->GetActor()->getPosWithOffsetZ() + p->GetActor()->spr.Angles.Yaw.ToVector() * 2, -1);
			break;

		}
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
	case RTILE_WALLLIGHT1:
	case RTILE_WALLLIGHT3:
	case RTILE_WALLLIGHT4:
	case RTILE_TECHLIGHT2:
	case RTILE_TECHLIGHT4:
	case RTILE_RRTILE1939:
	case RTILE_RRTILE1986:
	case RTILE_RRTILE1988:
	case RTILE_RRTILE2123:
	case RTILE_RRTILE2125:
	case RTILE_RRTILE2878:
	case RTILE_RRTILE2898:


		ceilingglass(ps[myconnectindex].GetActor(), sectp, 10);
		S_PlayActorSound(GLASS_BREAKING, ps[screenpeek].GetActor());

		if (sectp->ceilingpicnum == RTILE_WALLLIGHT1)
			sectp->ceilingpicnum = RTILE_WALLLIGHTBUST1;

		if (sectp->ceilingpicnum == RTILE_WALLLIGHT3)
			sectp->ceilingpicnum = RTILE_WALLLIGHTBUST3;

		if (sectp->ceilingpicnum == RTILE_WALLLIGHT4)
			sectp->ceilingpicnum = RTILE_WALLLIGHTBUST4;

		if (sectp->ceilingpicnum == RTILE_TECHLIGHT2)
			sectp->ceilingpicnum = RTILE_TECHLIGHTBUST2;

		if (sectp->ceilingpicnum == RTILE_TECHLIGHT4)
			sectp->ceilingpicnum = RTILE_TECHLIGHTBUST4;

		if (sectp->ceilingpicnum == RTILE_RRTILE1986)
			sectp->ceilingpicnum = RTILE_RRTILE1987;

		if (sectp->ceilingpicnum == RTILE_RRTILE1939)
			sectp->ceilingpicnum = RTILE_RRTILE2004;

		if (sectp->ceilingpicnum == RTILE_RRTILE1988)
			sectp->ceilingpicnum = RTILE_RRTILE2005;

		if (sectp->ceilingpicnum == RTILE_RRTILE2898)
			sectp->ceilingpicnum = RTILE_RRTILE2899;

		if (sectp->ceilingpicnum == RTILE_RRTILE2878)
			sectp->ceilingpicnum = RTILE_RRTILE2879;

		if (sectp->ceilingpicnum == RTILE_RRTILE2123)
			sectp->ceilingpicnum = RTILE_RRTILE2124;

		if (sectp->ceilingpicnum == RTILE_RRTILE2125)
			sectp->ceilingpicnum = RTILE_RRTILE2126;


		if (!sectp->hitag)
		{
			DukeSectIterator it(sectp);
			while (auto act1 = it.Next())
			{
				if (iseffector(act1) && (act1->spr.lotag == SE_12_LIGHT_SWITCH || (isRRRA() && (act1->spr.lotag == 47 || act1->spr.lotag == 48))))
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

void checkhitdefault_r(DDukeActor* targ, DDukeActor* proj)
{
	if ((targ->spr.cstat & CSTAT_SPRITE_ALIGNMENT_WALL) && targ->spr.hitag == 0 && targ->spr.lotag == 0 && targ->spr.statnum == 0)
		return;

	if ((proj->spr.picnum == RTILE_SAWBLADE || proj->spr.picnum == RTILE_FREEZEBLAST || proj->GetOwner() != targ) && targ->spr.statnum != 4)
	{
		if (badguy(targ) == 1)
		{
			if (proj->spr.picnum == RTILE_RPG) proj->spr.extra <<= 1;
			else if (isRRRA() && proj->spr.picnum == RTILE_RPG2) proj->spr.extra <<= 1;

			if ((targ->spr.picnum != RTILE_DRONE))
				if (proj->spr.picnum != RTILE_FREEZEBLAST)
					//if (actortype[targ->spr.picnum] == 0)  
				{
					auto spawned = spawn(proj, RTILE_JIBS6);
					if (spawned)
					{
						if (proj->spr.pal == 6)
							spawned->spr.pal = 6;
						spawned->spr.pos.Z += 4;
						spawned->vel.X = 1;
						spawned->spr.scale = DVector2(0.375, 0.375);
						spawned->spr.Angles.Yaw = DAngle22_5 / 4 - randomAngle(22.5 / 2);
					}
				}

			auto Owner = proj->GetOwner();

			if (Owner && Owner->isPlayer() && targ->spr.picnum != RTILE_DRONE)
				if (ps[Owner->PlayerIndex()].curr_weapon == SHOTGUN_WEAPON)
				{
					fi.shoot(targ, -1, PClass::FindActor("DukeBloodSplat3"));
					fi.shoot(targ, -1, PClass::FindActor("DukeBloodSplat1"));
					fi.shoot(targ, -1, PClass::FindActor("DukeBloodSplat2"));
					fi.shoot(targ, -1, PClass::FindActor("DukeBloodSplat4"));
				}

			if (targ->spr.statnum == STAT_ZOMBIEACTOR)
			{
				ChangeActorStat(targ, STAT_ACTOR);
				targ->timetosleep = SLEEPTIME;
			}
		}

		if (targ->spr.statnum != 2)
		{
			if (proj->spr.picnum == RTILE_FREEZEBLAST && ((targ->isPlayer() && targ->spr.pal == 1) || (gs.freezerhurtowner == 0 && proj->GetOwner() == targ)))
				return;

			targ->attackertype = proj->spr.picnum;
			targ->hitextra += proj->spr.extra;
			if (targ->spr.picnum != RTILE_COW)
				targ->hitang = proj->spr.Angles.Yaw;
			targ->SetHitOwner(proj->GetOwner());
		}

		if (targ->spr.statnum == 10)
		{
			auto p = targ->PlayerIndex();
			if (ps[p].newOwner != nullptr)
			{
				ps[p].newOwner = nullptr;
				ps[p].GetActor()->restorepos();

				updatesector(ps[p].GetActor()->getPosWithOffsetZ(), &ps[p].cursector);

				DukeStatIterator it(STAT_EFFECTOR);
				while (auto act = it.Next())
				{
					if (actorflag(act, SFLAG2_CAMERA)) act->spr.yint = 0;
				}
			}
			auto Owner = targ->GetHitOwner();
			if (!Owner || !Owner->isPlayer())
				if (ud.player_skill >= 3)
					proj->spr.extra += (proj->spr.extra >> 1);
		}

	}
}

void checkhitsprite_r(DDukeActor* targ, DDukeActor* proj)
{
	if (targ->GetClass() != RUNTIME_CLASS(DDukeActor))
	{
		CallOnHit(targ, proj);
		return;
	}

	if (isRRRA()) switch (targ->spr.picnum)
	{
	case RTILE_IRONWHEELSWITCH:
		break;
	}

	if (targ->spr.picnum == RTILE_PLAYERONWATER)
	{
		targ = targ->GetOwner();
		if (!targ) return;
	}
	checkhitdefault_r(targ, proj);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void checksectors_r(int snum)
{
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
		SECRET_Trigger(sectindex(p->cursector));
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
				if (hitscanwall->overpicnum == RTILE_MIRROR && snum == screenpeek)
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
				if (hitscanwall->overpicnum == RTILE_MIRROR)
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
			neartag(p->GetActor()->getPosWithOffsetZ(), p->GetActor()->sector(), p->GetActor()->PrevAngles.Yaw, near , 80., NT_Lotag | NT_Hitag);
		}

		if (p->newOwner != nullptr)
			neartag(p->GetActor()->getPrevPosWithOffsetZ(), p->GetActor()->sector(), p->GetActor()->PrevAngles.Yaw, near, 80., NT_Lotag);
		else
		{
			neartag(p->GetActor()->getPosWithOffsetZ(), p->GetActor()->sector(), p->GetActor()->PrevAngles.Yaw, near, 80., NT_Lotag);
			if (near.actor() == nullptr && near.hitWall == nullptr && near.hitSector == nullptr)
				neartag(p->GetActor()->getPosWithOffsetZ().plusZ(8), p->GetActor()->sector(), p->GetActor()->PrevAngles.Yaw, near, 80., NT_Lotag);
			if (near.actor() == nullptr && near.hitWall == nullptr && near.hitSector == nullptr)
				neartag(p->GetActor()->getPosWithOffsetZ().plusZ(16), p->GetActor()->sector(), p->GetActor()->PrevAngles.Yaw, near, 80., NT_Lotag);
			if (near.actor() == nullptr && near.hitWall == nullptr && near.hitSector == nullptr)
			{
				neartag(p->GetActor()->getPosWithOffsetZ().plusZ(16), p->GetActor()->sector(), p->GetActor()->PrevAngles.Yaw, near, 80., NT_Lotag | NT_Hitag);
				if (near.actor() != nullptr)
				{
					if (actorflag(near.actor(), SFLAG2_TRIGGERRESPAWN))
						return;

					switch (near.actor()->spr.picnum)
					{
					case RTILE_COW:
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
				double dist = hitasprite(p->GetActor(), &hit);
				if (hit) near.hitActor = hit;
				if (dist > 80) near.hitActor = nullptr;
			}

		auto const neartagsprite = near.actor();
		if (neartagsprite != nullptr)
		{
			if (fi.checkhitswitch(snum, nullptr, neartagsprite)) return;

			if (neartagsprite->GetClass() != RUNTIME_CLASS(DDukeActor))
			{
				if (CallOnUse(neartagsprite, p))
					return;
			}
		}

		if (!PlayerInput(snum, SB_OPEN)) return;

		if (near.hitWall == nullptr && near.hitSector == nullptr && near.actor() == nullptr)
			if (hits(p->GetActor()) < 32)
			{
				if ((krand() & 255) < 16)
					S_PlayActorSound(DUKE_SEARCH2, pact);
				else S_PlayActorSound(DUKE_SEARCH, pact);
				return;
			}

		if (near.hitWall)
		{
			if (near.hitWall->lotag > 0 && isadoorwall(near.hitWall->picnum))
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
				if (isactivator(act) || ismasterswitch(act))
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
					if (isactivator(act) || ismasterswitch(act))
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
	for (auto& wal : nextsect->walls)
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

	for (auto& wal : nextsect->walls)
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
		for(auto& wal : nextsect->walls)
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
		for(auto& wal : nextsect->walls)
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
		if (act->spr.picnum == RTILE_DESTRUCTO)
		{
			act->attackertype = RTILE_SHOTSPARK1;
			act->hitextra = 1;
		}
	}
}
END_DUKE_NS
