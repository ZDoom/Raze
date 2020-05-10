//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2017-2019 Nuke.YKT

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
#include "sounds_common.h"
#include "names_rr.h"

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




END_DUKE_NS
