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
#include "sounds_common.h"

// PRIMITIVE
BEGIN_DUKE_NS

//---------------------------------------------------------------------------
//
// game dependent dispatchers
//
//---------------------------------------------------------------------------

bool isadoorwall_d(int dapic);
bool isadoorwall_r(int dapic);

bool isadoorwall(int dapic)
{
    return isRR() ? isadoorwall_r(dapic) : isadoorwall_d(dapic);
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------
static bool haltsoundhack;

int callsound(int sn, int whatsprite)
{
    if (!isRRRA() && haltsoundhack)
    {
        haltsoundhack = 0;
        return -1;
    }

    int i = headspritesect[sn];
    while (i >= 0)
    {
        if (sprite[i].picnum == MUSICANDSFX && sprite[i].lotag < 1000)
        {
            if (whatsprite == -1) whatsprite = i;

            int snum = sprite[i].lotag;
            auto flags = S_GetUserFlags(snum);
            if (hittype[i].temp_data[0] == 0)
            {
                if ((flags & (SF_GLOBAL | SF_DTAG)) != SF_GLOBAL)
                {
                    if (snum)
                    {
                        if (sprite[i].hitag && snum != sprite[i].hitag)
                            S_StopEnvSound(sprite[i].hitag, hittype[i].temp_data[5]);   // changed to only stop the sound on the same actor, not all of them.
                        spritesound(snum, whatsprite);
                        hittype[i].temp_data[5] = whatsprite;
                    }

                    if ((sector[sprite[i].sectnum].lotag & 0xff) != ST_22_SPLITTING_DOOR)
                        hittype[i].temp_data[0] = 1;
                }
            }
            else if (S_IsSoundValid(sprite[i].hitag))
            {
                if ((flags & SF_LOOP) || (sprite[i].hitag && sprite[i].hitag != sprite[i].lotag))
                    S_StopEnvSound(sprite[i].lotag, hittype[i].temp_data[5]);   // changed to only stop the sound on the same actor, not all of them.
                if (sprite[i].hitag) spritesound(sprite[i].hitag, whatsprite);
                hittype[i].temp_data[0] = 0;
                hittype[i].temp_data[5] = whatsprite;
            }
            return sprite[i].lotag;
        }
        i = nextspritesect[i];
    }
    return -1;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int check_activator_motion(int lotag)
{
    int i, j;
    spritetype* s;

    i = headspritestat[STAT_ACTIVATOR];
    while (i >= 0)
    {
        if (sprite[i].lotag == lotag)
        {
            s = &sprite[i];

            for (j = animatecnt - 1; j >= 0; j--)
                if (s->sectnum == animatesect[j])
                    return(1);

            j = headspritestat[STAT_EFFECTOR];
            while (j >= 0)
            {
                if (s->sectnum == sprite[j].sectnum)
                    switch (sprite[j].lotag)
                    {
                    case SE_11_SWINGING_DOOR:
                    case SE_30_TWO_WAY_TRAIN:
                        if (hittype[j].temp_data[4])
                            return(1);
                        break;
                    case SE_18_INCREMENTAL_SECTOR_RISE_FALL:
                        if (isRRRA()) break;
                    case SE_20_STRETCH_BRIDGE:
                    case SE_31_FLOOR_RISE_FALL:
                    case SE_32_CEILING_RISE_FALL:
                        if (hittype[j].temp_data[0])
                            return(1);
                        break;
                    }

                j = nextspritestat[j];
            }
        }
        i = nextspritestat[i];
    }
    return(0);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool isanunderoperator(int lotag)
{
    switch (lotag & 0xff)
    {
    case ST_15_WARP_ELEVATOR:
    case ST_16_PLATFORM_DOWN:
    case ST_17_PLATFORM_UP:
    case ST_18_ELEVATOR_DOWN:
    case ST_19_ELEVATOR_UP:
    case ST_26_SPLITTING_ST_DOOR:
        return true;
    case ST_22_SPLITTING_DOOR:
        return !isRR();
    }
    return false;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool isanearoperator(int lotag)
{
    switch (lotag & 0xff)
    {
    case ST_9_SLIDING_ST_DOOR:
    case ST_15_WARP_ELEVATOR:
    case ST_16_PLATFORM_DOWN:
    case ST_17_PLATFORM_UP:
    case ST_18_ELEVATOR_DOWN:
    case ST_19_ELEVATOR_UP:
    case ST_20_CEILING_DOOR:
    case ST_21_FLOOR_DOOR:
    case ST_22_SPLITTING_DOOR:
    case ST_23_SWINGING_DOOR:
    case ST_25_SLIDING_DOOR:
    case ST_26_SPLITTING_ST_DOOR:
    case ST_29_TEETH_DOOR:
        return true;
    case 41:
        return isRR();
    }
    return false;
}


END_DUKE_NS
