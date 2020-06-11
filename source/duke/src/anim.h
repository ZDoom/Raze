//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

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

#ifndef anim_h_
#define anim_h_

BEGIN_DUKE_NS

struct animsound_t {
    uint16_t frame = 0;
    int16_t sound = 0;
};

struct dukeanim_t
{
    FString name;
    double frameaspect1 = 0, frameaspect2 = 0;
    uint8_t* animbuf = nullptr;
    TArray<animsound_t> Sounds;
    uint8_t framedelay = 0;
    uint8_t frameflags = 0;
};

extern dukeanim_t* g_animPtr;
extern TArray<dukeanim_t> g_Animations;
extern dukeanim_t * Anim_Find(const char *s);
extern dukeanim_t * Anim_Create(const char *fn);
int32_t Anim_Play(const char *fn);
void Anim_Init(void);

END_DUKE_NS
#endif

