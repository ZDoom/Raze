//-------------------------------------------------------------------------
/*
Copyright (C) 2020 - Christoph Oelckers

This is free software; you can redistribute it and/or
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

*/
//-------------------------------------------------------------------------

#include "ns.h"
#include "global.h"
#include "dukeactor.h"

BEGIN_DUKE_NS

//---------------------------------------------------------------------------
//
// Dispatcher for functions where different variants exist for the two families of games.
//
//---------------------------------------------------------------------------

bool checkaccessswitch_d(int snum, int pal, DDukeActor *act, walltype* w);
bool checkaccessswitch_r(int snum, int pal, DDukeActor* act, walltype* w);
void activatebysector_d(sectortype* sect, DDukeActor* j);
void activatebysector_r(sectortype* sect, DDukeActor* j);
void checksectors_d(int snum);
void checksectors_r(int snum);

void addweapon_d(DDukePlayer* p, int weapon, bool wswitch);
void addweapon_r(DDukePlayer* p, int weapon, bool wswitch);
int ifhitbyweapon_r(DDukeActor* sn);
int ifhitbyweapon_d(DDukeActor* sn);
void incur_damage_d(DDukePlayer* p);
void incur_damage_r(DDukePlayer* p);
void selectweapon_d(int snum, int j);
void selectweapon_r(int snum, int j);
int doincrements_d(DDukePlayer* p);
int doincrements_r(DDukePlayer* p);
void checkweapons_d(DDukePlayer* p);
void checkweapons_r(DDukePlayer* p);
void processinput_d(int snum);
void processinput_r(int snum);
void displayweapon_d(int snum, double interpfrac);
void displayweapon_r(int snum, double interpfrac);
void displaymasks_d(int snum, int p, double interpfrac);
void displaymasks_r(int snum, int p, double interpfrac);
void think_d();
void think_r();
void movetransports_d();
void movetransports_r();
void animatesprites_d(tspriteArray& tsprites, const DVector2& viewVec, DAngle viewang, double interpfrac);
void animatesprites_r(tspriteArray& tsprites, const DVector2& viewVec, DAngle viewang, double interpfrac);

Dispatcher fi;

void SetDispatcher()
{
	if (!isRR())
	{
		fi = {
		think_d,
		movetransports_d,
		checkaccessswitch_d,
		activatebysector_d,
		checksectors_d,

		addweapon_d,
		ifhitbyweapon_d,

		incur_damage_d,
		selectweapon_d,
		doincrements_d,
		checkweapons_d,
		processinput_d,
		displayweapon_d,
		displaymasks_d,
		animatesprites_d,
		};
	}
	else
	{
		fi = {
		think_r,
		movetransports_r,
		checkaccessswitch_r,
		activatebysector_r,
		checksectors_r,

		addweapon_r,
		ifhitbyweapon_r,

		incur_damage_r,
		selectweapon_r,
		doincrements_r,
		checkweapons_r,
		processinput_r,
		displayweapon_r,
		displaymasks_r,
		animatesprites_r,
		};
	}
}

END_DUKE_NS
