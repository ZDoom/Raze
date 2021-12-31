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

//****************************************************************************
//
// sounds.h
//
//****************************************************************************



#ifndef sw_sounds_public_
#define sw_sounds_public_

BEGIN_SW_NS

void SoundStartup(void);
void SoundShutdown(void);
void MusicStartup(void);
void MusicShutdown(void);

// 3D sound engine declarations //////////////////////////////////////////////
// Flag settings used to turn on and off 3d sound options
enum
{
    v3df_none       = 0,    // Default, take no action, use all defaults
    v3df_follow     = 1,    // 1 = Do coordinate updates on sound
    // Use this only if the sprite won't be deleted soon
    v3df_kill       = 2,    // 1 = Sound is to be deleted
    v3df_doppler    = 4,    // 1 = Don't use doppler pitch variance
    v3df_dontpan        = 8,    // 1 = Don't do panning of sound
    v3df_ambient    = 16,   // 1 = Sound is ambient, use ambient struct info.
    v3df_intermit   = 32,   // 1 = Intermittant sound
    v3df_init       = 64,   // 1 = First pass of sound, don't play it.
    // This is mainly used for intermittent sounds
    v3df_nolookup   = 128,   // don't use ambient table lookup
};

struct VOCstruct;

void Terminate3DSounds(void);

class DSWActor;

void PlaySpriteSound(DSWActor* actor, int attrib_ndx, int flags);
void DeleteNoSoundOwner(DSWActor* actor);
void DeleteNoFollowSoundOwner(DSWActor*);

inline bool CacheSound(int num, int type) { return false; }
void COVER_SetReverb(int amt);
void UnInitSound(void);
void InitFX(void);
void StopFX(void);
void StopSound(void);
void StartAmbientSound(void);
void StopAmbientSound(void);
bool PlaySong(const char *song_file_name, int cdaudio_track, bool isThemeTrack = false); //(nullptr, nullptr, -1, false) starts the normal level music.
void PlaySoundRTS(int rts_num);

// Ambient Sound Structure
struct AMB_INFO
{
    int16_t name;
    int16_t diginame;
    int ambient_flags;
    int maxtics;                       // When tics reaches this number next
    // sound happens
};

// VOC File flag settings for digi_entries.
enum
{
    vf_normal = 0,
    vf_loop = 1
};

//
// Table that describes the voc file and how it will be played
// Can be easily extended, but you may need to change digi.h
//

struct VOCstruct
{
    char name[14];                      // name of voc file on disk
    uint8_t* data;                         // pointer to voc data
    int datalen;                        // length of voc data
    int16_t pitch_lo;                     // lo pitch value
    int16_t pitch_hi;                     // hi pitch value
    uint8_t priority;                      // priority at which vocs are played
    int16_t voc_num;                      // Backward reference to parent sound
    int voc_distance;                   // Sound's distance effectiveness
    int voc_flags;                // Various allowable flag settings for voc
    uint16_t lock;                          // locking byte for caching
    uint8_t playing;                       // number of this type of sound currently playing
};


extern VOCstruct voc[];

END_SW_NS

#endif
