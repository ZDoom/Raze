//-------------------------------------------------------------------------
/*
Copyright (C) 2015 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#include "duke3d.h"
#include "animsounds.h"

void endanimsounds(int32_t fr)
{
    switch (ud.volume_number)
    {
        case 0:
            break;
        case 1:
            switch (fr)
            {
                case 1:
                    S_PlaySound(WIND_AMBIENCE);
                    break;
                case 26:
                    S_PlaySound(ENDSEQVOL2SND1);
                    break;
                case 36:
                    S_PlaySound(ENDSEQVOL2SND2);
                    break;
                case 54:
                    S_PlaySound(THUD);
                    break;
                case 62:
                    S_PlaySound(ENDSEQVOL2SND3);
                    break;
                case 75:
                    S_PlaySound(ENDSEQVOL2SND4);
                    break;
                case 81:
                    S_PlaySound(ENDSEQVOL2SND5);
                    break;
                case 115:
                    S_PlaySound(ENDSEQVOL2SND6);
                    break;
                case 124:
                    S_PlaySound(ENDSEQVOL2SND7);
                    break;
            }
            break;
        case 2:
            switch (fr)
            {
                case 1:
                    S_PlaySound(WIND_REPEAT);
                    break;
                case 98:
                    S_PlaySound(DUKE_GRUNT);
                    break;
                case 82+20:
                    S_PlaySound(THUD);
                    S_PlaySound(SQUISHED);
                    break;
                case 104+20:
                    S_PlaySound(ENDSEQVOL3SND3);
                    break;
                case 114+20:
                    S_PlaySound(ENDSEQVOL3SND2);
                    break;
                case 158:
                    S_PlaySound(PIPEBOMB_EXPLODE);
                    break;
            }
            break;
    }
}

void logoanimsounds(int32_t fr)
{
    switch (fr)
    {
        case 1:
            S_PlaySound(FLY_BY);
            break;
        case 19:
            S_PlaySound(PIPEBOMB_EXPLODE);
            break;
    }
}

void intro4animsounds(int32_t fr)
{
    switch (fr)
    {
        case 1:
            S_PlaySound(INTRO4_B);
            break;
        case 12:
        case 34:
            S_PlaySound(SHORT_CIRCUIT);
            break;
        case 18:
            S_PlaySound(INTRO4_5);
            break;
    }
}

void first4animsounds(int32_t fr)
{
    switch (fr)
    {
        case 1:
            S_PlaySound(INTRO4_1);
            break;
        case 12:
            S_PlaySound(INTRO4_2);
            break;
        case 7:
            S_PlaySound(INTRO4_3);
            break;
        case 26:
            S_PlaySound(INTRO4_4);
            break;
    }
}

void intro42animsounds(int32_t fr)
{
    switch (fr)
    {
        case 10:
            S_PlaySound(INTRO4_6);
            break;
    }
}

void endanimvol41(int32_t fr)
{
    switch (fr)
    {
        case 3:
            S_PlaySound(DUKE_UNDERWATER);
            break;
        case 35:
            S_PlaySound(VOL4ENDSND1);
            break;
    }
}

void endanimvol42(int32_t fr)
{
    switch (fr)
    {
        case 11:
            S_PlaySound(DUKE_UNDERWATER);
            break;
        case 20:
            S_PlaySound(VOL4ENDSND1);
            break;
        case 39:
            S_PlaySound(VOL4ENDSND2);
            break;
        case 50:
            FX_StopAllSounds();
            break;
    }
}

void endanimvol43(int32_t fr)
{
    switch (fr)
    {
        case 1:
            S_PlaySound(BOSS4_DEADSPEECH);
            break;
        case 40:
            S_PlaySound(VOL4ENDSND1);
            S_PlaySound(DUKE_UNDERWATER);
            break;
        case 50:
            S_PlaySound(BIGBANG);
            break;
    }
}
