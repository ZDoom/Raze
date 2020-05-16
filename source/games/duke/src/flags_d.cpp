//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)

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
#include "actors.h"
#include "names.h"

BEGIN_DUKE_NS

void initactorflags_d()
{
	setflag(SFLAG_INTERNAL_BADGUY, {
            SHARK,
            RECON,
            DRONE,
            LIZTROOPONTOILET,
            LIZTROOPJUSTSIT,
            LIZTROOPSTAYPUT,
            LIZTROOPSHOOT,
            LIZTROOPJETPACK,
            LIZTROOPDUCKING,
            LIZTROOPRUNNING,
            LIZTROOP,
            OCTABRAIN,
            COMMANDER,
            COMMANDERSTAYPUT,
            PIGCOP,
            EGG,
            PIGCOPSTAYPUT,
            PIGCOPDIVE,
            LIZMAN,
            LIZMANSPITTING,
            LIZMANFEEDING,
            LIZMANJUMP,
            ORGANTIC,
            BOSS1,
            BOSS2,
            BOSS3,
            BOSS4,
            GREENSLIME,
            GREENSLIME+1,
            GREENSLIME+2,
            GREENSLIME+3,
            GREENSLIME+4,
            GREENSLIME+5,
            GREENSLIME+6,
            GREENSLIME+7,
            RAT,
			ROTATEGUN });
		
	// Some flags taken from RedNukem's init code. This is a good start as any to reduce the insane dependency on tile numbers for making decisions in the play code. A lot more will be added here later.
	setflag(SFLAG_NODAMAGEPUSH, { TANK, BOSS1, BOSS2, BOSS3, BOSS4, RECON, ROTATEGUN });
	setflag(SFLAG_NOWATERDIP, { OCTABRAIN, COMMANDER, DRONE });
	setflag(SFLAG_GREENSLIMEFOOD, { LIZTROOP, LIZMAN, PIGCOP, NEWBEAST });
			
	if (isWorldTour())
	{
		setflag(SFLAG_INTERNAL_BADGUY|SFLAG_NODAMAGEPUSH, { FIREFLY, BOSS5, BOSS5STAYPUT });
	}
	
	settileflag(TFLAG_WALLSWITCH, {
        HANDPRINTSWITCH,
        HANDPRINTSWITCH+1,
        ALIENSWITCH,
        ALIENSWITCH+1,
        MULTISWITCH,
        MULTISWITCH+1,
        MULTISWITCH+2,
        MULTISWITCH+3,
        ACCESSSWITCH,
        ACCESSSWITCH2,
        PULLSWITCH,
        PULLSWITCH+1,
        HANDSWITCH,
        HANDSWITCH+1,
        SLOTDOOR,
        SLOTDOOR+1,
        LIGHTSWITCH,
        LIGHTSWITCH+1,
        SPACELIGHTSWITCH,
        SPACELIGHTSWITCH+1,
        SPACEDOORSWITCH,
        SPACEDOORSWITCH+1,
        FRANKENSTINESWITCH,
        FRANKENSTINESWITCH+1,
        LIGHTSWITCH2,
        LIGHTSWITCH2+1,
        POWERSWITCH1,
        POWERSWITCH1+1,
        LOCKSWITCH1,
        LOCKSWITCH1+1,
        POWERSWITCH2,
        POWERSWITCH2+1,
        DIPSWITCH,
        DIPSWITCH+1,
        DIPSWITCH2,
        DIPSWITCH2+1,
        TECHSWITCH,
        TECHSWITCH+1,
        DIPSWITCH3,
		DIPSWITCH3+1});
}
	
	
END_DUKE_NS
