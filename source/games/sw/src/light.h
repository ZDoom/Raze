//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------
#pragma once
BEGIN_SW_NS


void SectorLightShade(DSWActor*, short intensity);
void DiffuseLighting(DSWActor*);
void DoLightingMatch(short match, short state);
void InitLighting(void);
void DoLighting(void);

// Descriptive Light variables mapped from other variables

inline int LIGHT_Match(DSWActor* sp) { return SP_TAG2(sp); }
inline int LIGHT_Type(DSWActor* sp) { return SP_TAG3(sp); }
inline int16_t LIGHT_MaxTics(DSWActor* sp) { return SP_TAG4((sp)); }
inline int8_t LIGHT_MaxBright(DSWActor* a) { return int8_t(SP_TAG5(a)); }
inline int8_t LIGHT_MaxDark(DSWActor* sp) { return int8_t(SP_TAG6(sp)); }
inline uint8_t& LIGHT_ShadeInc(DSWActor* sp) { return SP_TAG7(sp); }

inline bool LIGHT_Dir(DSWActor* sp) { return (!!((sp->spr.extra & SPRX_BOOL10))); }
inline void LIGHT_DirChange(DSWActor* sp) { sp->spr.extra ^= SPRX_BOOL10; }

int8_t& LIGHT_FloorShade(DSWActor* a) { return a->spr.xoffset; }
int8_t& LIGHT_CeilingShade(DSWActor* a) { return a->spr.yoffset; }
int16_t& LIGHT_Tics(DSWActor* a) { return a->spr.detail; }

inline int LIGHT_DiffuseNum(DSWActor* sp) { return SP_TAG3(sp); }
inline int16_t LIGHT_DiffuseMult(DSWActor* sp) { return SP_TAG4((sp)); }

enum LightTypes {LIGHT_CONSTANT, LIGHT_FLICKER, LIGHT_FADE, LIGHT_FLICKER_ON, LIGHT_FADE_TO_ON_OFF};
END_SW_NS
