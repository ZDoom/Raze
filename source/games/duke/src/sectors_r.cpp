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
	if (isRRRA() &&ps[screenpeek].sea_sick_stat == 1)
	{
		for (auto& wal : wall)
		{
			if (wal.wallpicnum == RTILE_RRTILE7873)
				wal.addxpan(6);
			else if (wal.wallpicnum == RTILE_RRTILE7870)
				wal.addxpan(6);
		}
	}

	for (int p = 0; p < numanimwalls; p++)
	{
		auto wal = animwall[p].wall;
		int j = wal->wallpicnum;

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
				animwall[p].tag = wal->wallpicnum;
				wal->wallpicnum = RTILE_SCREENBREAK6;
			}

			continue;

		case RTILE_SCREENBREAK6:
		case RTILE_SCREENBREAK7:
		case RTILE_SCREENBREAK8:

			if (animwall[p].tag >= 0)
				wal->wallpicnum = animwall[p].tag;
			else
			{
				wal->wallpicnum++;
				if (wal->wallpicnum == (RTILE_SCREENBREAK6 + 3))
					wal->wallpicnum = RTILE_SCREENBREAK6;
			}
			continue;

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

bool checkaccessswitch_r(int snum, int switchpal, DDukeActor* act, walltype* wwal)
{
	if (ps[snum].access_incs == 0)
	{
		if (switchpal == 0)
		{
			if (ps[snum].keys[1])
				ps[snum].access_incs = 1;
			else
			{
				FTA(70, &ps[snum]);
				if (isRRRA()) S_PlayActorSound(99, act ? act : ps[snum].GetActor());
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
		return 1;
	}
	return 0;
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
	FTextureID texid;

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
		picnum = wwal->wallpicnum;
		switchpal = wwal->pal;
	}
	texid = tileGetTextureID(picnum);

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
		if (checkaccessswitch_r(snum, switchpal, act, wwal))
			return 0;
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
		if (isadoorwall(texid) == 0) return 0;
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
			switch (wal.wallpicnum)
			{
			case RTILE_DIPSWITCH:
			case RTILE_TECHSWITCH:
			case RTILE_ALIENSWITCH:
				if (!act && &wal == wwal) wal.wallpicnum++;
				else if (wal.hitag == 0) correctdips++;
				numdips++;
				break;
			case RTILE_DIPSWITCHON:
			case RTILE_TECHSWITCHON:
			case RTILE_ALIENSWITCHON:
				if (!act && &wal == wwal) wal.wallpicnum--;
				else if (wal.hitag == 1) correctdips++;
				numdips++;
				break;
			case RTILE_MULTISWITCH:
			case RTILE_MULTISWITCH_2:
			case RTILE_MULTISWITCH_3:
			case RTILE_MULTISWITCH_4:
				wal.wallpicnum++;
				if (wal.wallpicnum > (RTILE_MULTISWITCH_4))
					wal.wallpicnum = RTILE_MULTISWITCH;
				break;
			case RTILE_MULTISWITCH2:
			case RTILE_MULTISWITCH2_2:
			case RTILE_MULTISWITCH2_3:
			case RTILE_MULTISWITCH2_4:
				if (!isRRRA()) break;
				wal.wallpicnum++;
				if (wal.wallpicnum > (RTILE_MULTISWITCH2_4))
					wal.wallpicnum = RTILE_MULTISWITCH2;
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
				wal.wallpicnum++;
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
				wal.wallpicnum--;
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
		if (isadoorwall(texid) == 0) break;
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

		if (hitag == 0 && isadoorwall(texid) == 0)
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
			checkhitwall(p->GetActor(), wal, p->GetActor()->getPosWithOffsetZ() + p->GetActor()->spr.Angles.Yaw.ToVector() * 2);
			break;

		}
	}
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
				if (hitscanwall->overtexture() == mirrortex && snum == screenpeek)
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
				if (hitscanwall->overtexture() == mirrortex)
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
			if (near.hitWall->lotag > 0 && isadoorwall(near.hitWall->walltexture()))
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
