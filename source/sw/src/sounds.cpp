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
#include "compat.h"
#include "build.h"
#include "cache1d.h"

#include "keys.h"

#include "names2.h"
#include "mytypes.h"
#include "fx_man.h"
#include "music.h"
#include "gamedefs.h"
#include "config.h"


#include "panel.h"
#include "game.h"
#include "sounds.h"
#include "ai.h"
#include "net.h"

#include "cache.h"
#include "text.h"
#include "rts.h"
#include "menus.h"

#ifdef _WIN32
#include "winlayer.h"
#endif

extern USERp User[MAXSPRITES];
void DumpSounds(void);

// Parentally locked sounds list
int PLocked_Sounds[] =
{
    483,328,334,335,338,478,450,454,452,453,456,457,458,455,460,462,
    461,464,466,465,467,463,342,371,254,347,350,432,488,489,490,76,339,
    499,500,506,479,480,481,482,78,600,467,548,547,544,546,545,542,541,540,
    539,536,529,525,522,521,515,516,612,611,589,625,570,569,567,565,
    558,557
};

uint8_t RedBookSong[40] =
{
    2,4,9,12,10, // Title and ShareWare levels
    5,6,8,11,12,5,10,4,6,9,7,10,8,7,9,10,11,5, // Registered levels
    11,8,7,13,5,6,  // Deathmatch levels
    13 // Fight boss
};

// Global vars used by ambient sounds to set spritenum of ambient sounds for later lookups in
// the sprite array so FAFcansee can know the sound sprite's current sector location
SWBOOL Use_SoundSpriteNum = FALSE;
int16_t SoundSpriteNum = -1;  // Always set this back to -1 for proper validity checking!

SWBOOL MusicInitialized = FALSE;
SWBOOL FxInitialized = FALSE;

void SoundCallBack(unsigned int num);
SWBOOL LoadSong(const char *track);

#define MUSIC_ID -65536

#define NUM_SAMPLES 10

char *BitNames[2] =
{
    "8-bit", "16-bit"
};

char *ChannelNames[2] =
{
    "Mono", "Stereo"
};

char *VoiceNames[8] =
{
    "1", "2", "3", "4", "5", "6", "7", "8"
};

int music;
int soundfx;
int num_voices;

int NumSounds = 0;

int angle;
int distance;
int voice;

int loopflag;

typedef enum
{
    SongTypeNone,
    SongTypeMIDI,
    SongTypeWave,
} SongType_t;

char *SongPtr = NULL;
int SongLength = 0;
char *SongName = NULL;
int SongTrack = 0;
SongType_t SongType = SongTypeNone;
int SongVoice = -1;
extern SWBOOL DemoMode;

//
// Includes digi.h to build the table
//

#define DIGI_TABLE
VOC_INFO voc[] =
{
#include "digi.h"
};

#undef  DIGI_TABLE

//
// Includes ambient.h to build the table of ambient sounds for game
//

#define AMBIENT_TABLE
AMB_INFO ambarray[] =
{
#include "ambient.h"
};
#undef  AMBIENT_TABLE
#define MAX_AMBIENT_SOUNDS 82

#define MAXSONGS        10              // This is the max songs per episode

SWBOOL OpenSound(VOC_INFOp vp, int *handle, int *length);
int ReadSound(int handle, VOC_INFOp vp, int length);

// 3d sound engine function prototype
VOC3D_INFOp Insert3DSound(void);

#if 0
// DEBUG
char *globsndata[DIGI_MAX], *globvpdata[DIGI_MAX];
int glength[DIGI_MAX];
#endif

/*
===================
=
= My stuff
=
===================
*/

int PlayerPainVocs[] =
{
    DIGI_PLAYERPAIN1,
    DIGI_PLAYERPAIN2,
    DIGI_PLAYERPAIN3,
    DIGI_PLAYERPAIN4,
    DIGI_PLAYERPAIN5
};

// Don't have these sounds yet
int PlayerLowHealthPainVocs[] =
{
    DIGI_HURTBAD1,
    DIGI_HURTBAD2,
    DIGI_HURTBAD3,
    DIGI_HURTBAD4,
    DIGI_HURTBAD5
};

int TauntAIVocs[] =
{
    DIGI_TAUNTAI1,
    DIGI_TAUNTAI2,
    DIGI_TAUNTAI3,
    DIGI_TAUNTAI4,
    DIGI_TAUNTAI5,
    DIGI_TAUNTAI6,
    DIGI_TAUNTAI7,
    DIGI_TAUNTAI8,
    DIGI_TAUNTAI9,
    DIGI_TAUNTAI10,
    DIGI_COWABUNGA,
    DIGI_NOCHARADE,
    DIGI_TIMETODIE,
    DIGI_EATTHIS,
    DIGI_FIRECRACKERUPASS,
    DIGI_HOLYCOW,
    DIGI_HAHA2,
    DIGI_HOLYPEICESOFCOW,
    DIGI_HOLYSHIT,
    DIGI_HOLYPEICESOFSHIT,
    DIGI_PAYINGATTENTION,
    DIGI_EVERYBODYDEAD,
    DIGI_KUNGFU,
    DIGI_HOWYOULIKEMOVE,
    DIGI_HAHA3,
    DIGI_NOMESSWITHWANG,
    DIGI_RAWREVENGE,
    DIGI_YOULOOKSTUPID,
    DIGI_TINYDICK,
    DIGI_NOTOURNAMENT,
    DIGI_WHOWANTSWANG,
    DIGI_MOVELIKEYAK,
    DIGI_ALLINREFLEXES
};

int PlayerGetItemVocs[] =
{
    DIGI_GOTITEM1,
    DIGI_HAHA1,
    DIGI_BANZAI,
    DIGI_COWABUNGA,
    DIGI_TIMETODIE
};

int PlayerYellVocs[] =
{
    DIGI_PLAYERYELL1,
    DIGI_PLAYERYELL2,
    DIGI_PLAYERYELL3
};

extern unsigned char lumplockbyte[];

#if 0
// DEBUG
void CheckSndData(char *file, int line)
{
    short i;

    //return;

    for (i = 0; i<DIGI_MAX; i++)
    {
        if (!globsndata[i] && !globvpdata[i]) continue;

        if (memcmp(globsndata[i], globvpdata[i], glength[i]) != 0)
        {
            printf("%s %d\n",file,line);
            printf("CheckSndData: Data is not the same! num = %d",i);
            exit(0);
        }
    }
}
#endif

//
// Routine called when a sound is finished playing
//

void
SoundCallBack(unsigned int num)
{
    VOC_INFOp vp;

    if ((int) num == MUSIC_ID)
    {
        return;
    }

    // RTS sounds are negative
    if ((int)num < 0)
    {
        ASSERT(-num < 11);
        lumplockbyte[-num]--;
        return;
    }

    vp = &voc[num];

    // Update counter
    //vp->playing--;
    vp->lock--;
}

//

void
ClearSoundLocks(void)
{
    unsigned i;

    for (i = 0; i < SIZ(voc); i++)
    {
        if (voc[i].lock >= 200)
            voc[i].lock = 199;
    }

    for (i = 0; i < 11; i++)
    {
        if (lumplockbyte[i] >= 200)
            lumplockbyte[i] = 199;
    }
}

void
UnInitSound(void)
{
    SoundShutdown();
    MusicShutdown();
}

void
InitFX(void)
{
    VOC_INFOp vp;
    short i;

#if 0
    // DEBUG
    for (i=0; i<DIGI_MAX; i++)
    {
        globsndata[i] = globvpdata[i] = NULL;
        glength[i] = 0;
    }
#endif

    //ExternalSoundMod();

    // Select which cards to use
    SoundStartup();

    // Get the current volume of the music and sound fx
    gs.SoundVolume = FX_GetVolume();

    for (vp = voc; vp < &voc[SIZ(voc)]; vp++)
    {
        vp->playing = 0;
    }

    // Set up our fx callback so we can display the sounds that are playing
    FX_SetCallBack(SoundCallBack);
}

void
InitMusic(void)
{
    // Select which cards to use
    MusicStartup();
    //SendGeneralMidiSysX();
}

void
ExternalSoundMod(void)
{
    FILE *fin;
    VOC_INFOp vp;
    char name[40];
    char new_name[40];
    int pri;
    int pitch_lo, pitch_hi;
    int ret;

    fin = fopen("swextern.snd", "r");

    if (!fin)
        return;

    while (TRUE)
    {
        ret = fscanf(fin, "%s %s %d %d", name, new_name, &pitch_lo, &pitch_hi);

        if (ret == EOF)
            break;

        for (vp = voc; vp < &voc[SIZ(voc)]; vp++)
        {
            if (!vp->name)
                continue;

            if (!Bstrcasecmp(name, vp->name))
            {
                // vp->priority = pri;
                strcpy(vp->name, new_name);
                vp->pitch_lo = pitch_lo;
                vp->pitch_hi = pitch_hi;
            }
        }
    }

    fclose(fin);
}

extern short Level;

SWBOOL
PlaySong(char *song_file_name, int cdaudio_track, SWBOOL loop, SWBOOL restart)
{
    if (!gs.MusicOn)
    {
        return FALSE;
    }

    if (DemoMode)
        return FALSE;

    if (!restart)
    {
        if (SongType == SongTypeWave)
        {
            if (SongTrack > 0 && SongTrack == cdaudio_track)
            {
                // ogg replacement for a CD track
                return TRUE;
            }
            else if (SongName && song_file_name && !strcmp(SongName, song_file_name))
            {
                return TRUE;
            }
        }
        else if (SongType == SongTypeMIDI)
        {
            if (SongName && song_file_name && !strcmp(SongName, song_file_name))
            {
                return TRUE;
            }
        }
    }

    StopSong();

    if (!SW_SHAREWARE)
    {
        if (cdaudio_track >= 0)
        {
            char waveformtrack[MAXWAVEFORMTRACKLENGTH];
            Bstrncpy(waveformtrack, gs.WaveformTrackName, MAXWAVEFORMTRACKLENGTH - 1);

            char *numPos = Bstrstr(waveformtrack, "??");

            if (numPos && (numPos-waveformtrack) < MAXWAVEFORMTRACKLENGTH - 2)
            {
                static const char *tracktypes[] = { ".flac", ".ogg" };
                const size_t tracknamebaselen = Bstrlen(waveformtrack);
                size_t i;

                numPos[0] = '0' + (cdaudio_track / 10) % 10;
                numPos[1] = '0' + cdaudio_track % 10;

                for (i = 0; i < ARRAY_SIZE(tracktypes); ++i)
                {
                    waveformtrack[tracknamebaselen] = '\0';
                    Bstrncat(waveformtrack, tracktypes[i], MAXWAVEFORMTRACKLENGTH);

                    if (LoadSong(waveformtrack))
                    {
                        SongVoice = FX_Play(SongPtr, SongLength, 0, 0, 0,
                                                      255, 255, 255, FX_MUSIC_PRIORITY, MUSIC_ID);
                        if (SongVoice > FX_Ok)
                        {
                            SongType = SongTypeWave;
                            SongTrack = cdaudio_track;
                            SongName = Bstrdup(waveformtrack);
                            return TRUE;
                        }
                    }
                }

                buildprintf("Can't find CD track %i!\n", cdaudio_track);
            }
            else
            {
                buildprintf("Make sure to have \"??\" as a placeholder for the track number in your WaveformTrackName!\n");
                buildprintf("  e.g. WaveformTrackName = \"MUSIC/Track??\"\n");
            }
        }
    }

    if (!song_file_name || !LoadSong(song_file_name))
    {
        return FALSE;
    }

    if (!memcmp(SongPtr, "MThd", 4))
    {
        MUSIC_PlaySong(SongPtr, /*SongLength,*/  MUSIC_LoopSong);
        SongType = SongTypeMIDI;
        SongName = strdup(song_file_name);
        return TRUE;
    }
    else
    {
        SongVoice = FX_Play(SongPtr, SongLength, 0, 0, 0,
                                      255, 255, 255, FX_MUSIC_PRIORITY, MUSIC_ID);
        if (SongVoice > FX_Ok)
        {
            SongType = SongTypeWave;
            SongName = strdup(song_file_name);
            return TRUE;
        }
    }

    return FALSE;
}

void
StopFX(void)
{
    FX_StopAllSounds();
}

void
StopSong(void)
{
    if (DemoMode)
        return;

    if (SongType == SongTypeWave && SongVoice >= 0)
    {
        FX_StopSound(SongVoice);
    }
    else if (SongType == SongTypeMIDI)
    {
        MUSIC_StopSong();
    }
    SongType = SongTypeNone;

    DO_FREE_AND_NULL(SongName);
    SongTrack = 0;

    if (SongPtr)
    {
        FreeMem(SongPtr);
        SongPtr = 0;
        SongLength = 0;
    }
}

void
PauseSong(SWBOOL pauseon)
{
    if (!gs.MusicOn) return;

    if (SongType == SongTypeWave && SongVoice >= 0)
    {
        FX_PauseVoice(SongVoice, pauseon);
    }
}

void
SetSongVolume(int volume)
{
}

SWBOOL
SongIsPlaying(void)
{
    return FALSE;
}

void
StopSound(void)
{
    StopFX();
    StopSong();
}

//
// Sound Distance Calculation
//

#define MAXLEVLDIST 19000   // The higher the number, the further away you can hear sound

short
SoundDist(int x, int y, int z, int basedist)
{
    double tx, ty, tz;
    double sqrdist,retval;
    double decay,decayshift;
    extern short screenpeek;

#define DECAY_CONST 4000


    tx = fabs(Player[screenpeek].posx - x);
    ty = fabs(Player[screenpeek].posy - y);
    tz = fabs((Player[screenpeek].posz - z) >> 4);

    // Use the Pythagreon Theorem to compute the magnitude of a 3D vector
    sqrdist = fabs(tx*tx + ty*ty + tz*tz);
    retval = sqrt(sqrdist);

    if (basedist < 0) // if basedist is negative
    {
        short i;

        decayshift=2;
        decay = labs(basedist) / DECAY_CONST;

        for (i=0; i<decay; i++)
            decayshift *= 2;

        if (labs(basedist/decayshift) >= retval)
            retval = 0;
        else
            retval *= decay;
    }
    else
    {
        if (basedist > retval)
            retval = 0;
        else
            retval -= basedist;
    }

    retval = retval * 256 / MAXLEVLDIST;

    if (retval < 0) retval = 0;
    if (retval > 255) retval = 255;

    return retval;
}

//
// Angle calcuations - may need to be checked to make sure they are right
//

short
SoundAngle(int x, int y)
{
    extern short screenpeek;

    short angle, delta_angle;

    angle = getangle(x - Player[screenpeek].posx, y - Player[screenpeek].posy);

    delta_angle = GetDeltaAngle(angle, Player[screenpeek].pang);

    // convert a delta_angle to a real angle if negative
    if (delta_angle < 0)
        delta_angle = NORM_ANGLE((1024 + delta_angle) + 1024);

    // convert 2048 degree angle to 32 degree angle
    return delta_angle >> 6;
}

int _PlayerSound(char *file, int line, int num, int *x, int *y, int *z, Voc3D_Flags flags, PLAYERp pp)
//PlayerSound(int num, int *x, int *y, int *z, Voc3D_Flags flags, PLAYERp pp)
{
    int handle;
    VOC_INFOp vp;

    if (Prediction)
        return 0;

    if (pp < Player || pp >= Player + MAX_SW_PLAYERS)
    {
        TerminateGame();
        printf("Player Sound invalid player: file %s, line %d\n",file,line);
        exit(0);
    }

    PRODUCTION_ASSERT(pp >= Player && pp < Player+MAX_SW_PLAYERS);
    PRODUCTION_ASSERT(num >= 0 && num < DIGI_MAX);

    if (TEST(pp->Flags, PF_DEAD)) return 0; // You're dead, no talking!

    // If this is a player voice and he's already yacking, forget it.
    vp = &voc[num];
    if (vp == NULL)
    {
        TerminateGame();
        printf("vp == NULL in PlayerSound, num = %d\n",num);
        exit(0);
    }

    // Not a player voice, bail.
    if (vp->priority != PRI_PLAYERVOICE && vp->priority != PRI_PLAYERDEATH)
        return 0;

    // He wasn't talking, but he will be now.
    if (!pp->PlayerTalking)
    {
        pp->PlayerTalking = TRUE;
        pp->TalkVocnum = num;   // Set the voc number
        pp->TalkVocHandle = PlaySound(num, x, y, z, flags); // Play the sound
        if (pp->TalkVocHandle < 0)
        {
            pp->PlayerTalking = FALSE;
            pp->TalkVocnum = -1;
            pp->TalkVocHandle = -1;
        }
    }

    return 0;
}

void LockSound(int num)
{
    VOC_INFOp vp = &voc[num];
    // if data is not locked
    if (vp->lock <= CACHE_UNLOCK_MAX)
    {
        vp->lock = CACHE_LOCK_START;
    }
    else
    // if data is already locked
    {
        vp->lock++;
        if (vp->lock >= CACHE_LOCK_MAX || vp->lock == 0)
        {
            DumpSounds();
            TerminateGame();
            printf("lock > MAX, num = %d",num);
            exit(0);
        }
        //ASSERT(vp->lock < CACHE_LOCK_MAX);
        //ASSERT(vp->lock != 0);
    }
}

SWBOOL CacheSound(int num, int type)
{
    VOC_INFOp vp = &voc[num];

    PRODUCTION_ASSERT(num >= 0 && num < DIGI_MAX);

    // if no data we need to cache it in
    if (!vp->data)
    {
        int handle;
        int length;

        if (!OpenSound(vp, &handle, &length))
        {
            sprintf(ds,"Could not open sound %s, num %d, priority %d\n",vp->name,num,vp->priority);
            CON_ConMessage(ds);
            return FALSE;
        }

        if (vp != NULL)
        {
            //FILE *fp;

            /*
            if (type == CACHE_SOUND_PLAY)
                // start it out locked at the min
                vp->lock = CACHE_LOCK_START;
            else
            if (type == CACHE_SOUND_PRECACHE)
                // start it out unlocked at the max
            */
            vp->lock = CACHE_UNLOCK_MAX;

            allocache((intptr_t*)&vp->data, length, &vp->lock);

#if 0
            // DEBUG
            globsndata[num] = AllocMem(length);
            glength[num] = length;

            fp = fopen(vp->name, "rb");
            if (fp != NULL)
            {
                fread(globsndata[num], length, 1, fp);
                ASSERT(globsndata[num] != NULL);
                fclose(fp);
            }
#endif
            ///////

            ASSERT(vp->data);
            ReadSound(handle, vp, length);

#if 0
            //DEBUG
            globvpdata[num] = vp->data;
            CheckSndData(__FILE__, __LINE__);
#endif
        }
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////
// Play a sound
////////////////////////////////////////////////////////////////////////////

#define SOUND_UNIT  MAXLEVLDIST/255
// NOTE: If v3df_follow == 1, x,y,z are considered literal coordinates
int
PlaySound(int num, int *x, int *y, int *z, Voc3D_Flags flags)
{
    VOC_INFOp vp;
    VOC3D_INFOp v3p;
    int pitch = 0;
    short angle, sound_dist;
    int tx, ty, tz;
    uint8_t priority;
    SPRITEp sp=NULL;

    // DEBUG
    //extern SWBOOL Pachinko_Win_Cheat;


    // Don't play game sounds when in menus
    //if (UsingMenus && (*x!=0 || *y!=0 || *z!=0)) return(-1);

    // Weed out parental lock sounds if PLock is active
    if (gs.ParentalLock || Global_PLock)
    {
        unsigned i;

        for (i=0; i<sizeof(PLocked_Sounds); i++)
        {
            if (num == PLocked_Sounds[i])
                return -1;
        }
    }

    if (Prediction)
        return -1;

    if (!gs.FxOn)
        return -1;

    PRODUCTION_ASSERT(num >= 0 && num < DIGI_MAX);

    // Reset voice
    voice = -1;

    // This is used for updating looping sounds in Update3DSounds
    if (Use_SoundSpriteNum && SoundSpriteNum >= 0)
    {
        ASSERT(SoundSpriteNum >= 0 && SoundSpriteNum < MAXSPRITES);
        sp = &sprite[SoundSpriteNum];
    }

    if (gs.Ambient && TEST(flags,v3df_ambient) && !TEST(flags,v3df_nolookup))  // Look for invalid ambient numbers
    {
        if (num < 0 || num > MAX_AMBIENT_SOUNDS)
        {
            sprintf(ds,"Invalid or out of range ambient sound number %d\n",num);
            PutStringInfo(Player+screenpeek, ds);
            return -1;
        }
    }


    // Call queue management to add sound to play list.
    // 3D sound manager will update playing sound 10x per second until
    // the sound ends, at which time it is removed from both the 3D
    // sound list as well as the actual cache.
    v3p = Insert3DSound();

    // If the ambient flag is set, do a name conversion to point to actual
    // digital sound entry.
    v3p->num = num;
    v3p->priority = 0;
    v3p->FX_Ok = FALSE; // Hasn't played yet

    if (gs.Ambient && TEST(flags,v3df_ambient) && !TEST(flags,v3df_nolookup))
    {
        v3p->maxtics = STD_RANDOM_RANGE(ambarray[num].maxtics);
        flags |= ambarray[num].ambient_flags;   // Add to flags if any
        num = ambarray[num].diginame;
    }

    PRODUCTION_ASSERT(num >= 0 && num < DIGI_MAX);


    // Assign voc to voc pointer
    vp = &voc[num];
    if (UsingMenus && *x==0 && *y==0 && *z==0)  // Menus sound outdo everything
        priority = 100;
    else
        priority = vp->priority;
    v3p->vp = vp;

    // Assign voc info to 3d struct for future reference
    v3p->x = x;
    v3p->y = y;
    v3p->z = z;
    v3p->fx = *x;
    v3p->fy = *y;
    v3p->fz = *z;
    v3p->flags = flags;

    if (flags & v3df_follow)
    {
        tx = *x;
        ty = *y;
        if (!z)
            tz = 0;                     // Some sound calls don't have a z
        // value
        else
            tz = *z;
    }
    else
    {
        // Don't use pointers to coordinate values.
        tx = v3p->fx;
        ty = v3p->fy;
        tz = v3p->fz;
    }

    // Special case stuff for sounds being played in a level
    if (*x==0 && *y==0 && *z==0)
        tx = ty = tz = 0;

    if ((vp->voc_flags & vf_loop) && Use_SoundSpriteNum && SoundSpriteNum >= 0 && sp)
    {
        tx=sp->x;
        ty=sp->y;
        tz=sp->z;
        //CON_Message("Using sp to set tx=%ld,ty=%ld,tz=%ld",tx,ty,tz);
    }

    // Calculate sound angle
    if (flags & v3df_dontpan)               // If true, don't do panning
        angle = 0;
    else
        angle = SoundAngle(tx, ty);

    // Calculate sound distance
    if (tx == 0 && ty == 0 && tz == 0)
        sound_dist = 255;  // Special case for menus sounds,etc.
    else
        sound_dist = SoundDist(tx, ty, tz, vp->voc_distance);

    v3p->doplr_delta = sound_dist;      // Save of distance for doppler
    // effect

//  //DSPRINTF(ds,"sound dist = %d\n",sound_dist);
//  MONO_PRINT(ds);

    // Can the ambient sound see the player?  If not, tone it down some.
    if ((vp->voc_flags & vf_loop) && Use_SoundSpriteNum && SoundSpriteNum >= 0)
    {
        PLAYERp pp = Player+screenpeek;

        //MONO_PRINT("PlaySound:Checking sound cansee");
        if (!FAFcansee(tx, ty, tz, sp->sectnum,pp->posx, pp->posy, pp->posz, pp->cursectnum))
        {
            //MONO_PRINT("PlaySound:Reducing sound distance");
            sound_dist += ((sound_dist/2)+(sound_dist/4));  // Play more quietly
            if (sound_dist > 255) sound_dist = 255;

            // Special Cases
            if (num == DIGI_WHIPME) sound_dist = 255;
        }
    }

    // Assign ambient priorities based on distance
    if (gs.Ambient && TEST(flags, v3df_ambient))
    {
        v3p->priority = v3p->vp->priority - (sound_dist / 26);
        priority = v3p->priority;
    }

    if (!CacheSound(num, CACHE_SOUND_PLAY))
    {
        v3p->flags = v3df_kill;
        v3p->handle = -1;
        v3p->dist = 0;
        v3p->deleted = TRUE;            // Sound init failed, remove it!
        return -1;
    }

    LockSound(num);

    if (sound_dist < 5)
        angle = 0;

    // Check for pitch bending
    if (vp->pitch_lo > vp->pitch_hi)
        ASSERT(vp->pitch_lo <= vp->pitch_hi);

    if (vp->pitch_hi == vp->pitch_lo)
        pitch = vp->pitch_lo;
    else if (vp->pitch_hi != vp->pitch_lo)
        pitch = vp->pitch_lo + (STD_RANDOM_RANGE(vp->pitch_hi - vp->pitch_lo));

#if 0
    // DEBUG
    if (Pachinko_Win_Cheat)
    {
        CheckSndData(__FILE__, __LINE__);
        Pachinko_Win_Cheat = FALSE;
        CON_Message("S O U N D S   C H E C K E D");
    }
#endif

    // Request playback and play it as a looping sound if flag is set.
    if (vp->voc_flags & vf_loop)
    {
        short loopvol=0;

        if ((loopvol = 255-sound_dist) <= 0)
            loopvol = 0;

        if (sound_dist < 255 || (flags & v3df_init))
        {
            voice = FX_Play((char *)vp->data, vp->datalen, 0, 0,
                                      pitch, loopvol, loopvol, loopvol, priority, num);
        }
        else
            voice = -1;

    }
    else
    //if(!flags & v3df_init)  // If not initing sound, play it
    if (tx==0 && ty==0 && tz==0)     // It's a non-inlevel sound
    {
        voice = FX_Play((char *)vp->data, vp->datalen, -1, -1, pitch, 255, 255, 255, priority, num);
    }
    else     // It's a 3d sound
    {
        if (sound_dist < 255)
        {
            voice = FX_Play3D((char *)vp->data, vp->datalen, FX_ONESHOT, pitch, angle, sound_dist, priority, num);
        }
        else
            voice = -1;
    }

    // If sound played, update our counter
    if (voice > FX_Ok)
    {
        //vp->playing++;
        v3p->FX_Ok = TRUE;
    }
    else
    {
        vp->lock--;
    }

    // Assign voc info to 3d struct for future reference
    v3p->handle = voice;                // Save the current voc handle in struct
    v3p->dist = sound_dist;
    v3p->tics = 0;                      // Reset tics
    if (flags & v3df_init)
        v3p->flags ^= v3df_init;        // Turn init off now

    return voice;
}

void PlaySoundRTS(int rts_num)
{
    char *rtsptr;
    int voice=-1;

    if (RTS_NumSounds() <= 0 || !gs.FxOn)
        return;

    rtsptr = (char *)RTS_GetSound(rts_num - 1);

    ASSERT(rtsptr);

    voice = FX_Play3D(rtsptr, RTS_SoundLength(rts_num - 1), FX_ONESHOT, 0, 0, 0, 255, -rts_num);

    if (voice <= FX_Ok)
    {
        lumplockbyte[rts_num]--;
    }
}

///////////////////////////////////////////////

SWBOOL
OpenSound(VOC_INFOp vp, int *handle, int *length)
{
    *handle = kopen4load(vp->name, 0);

    if (*handle == -1)
    {
        return FALSE;
    }

    *length = kfilelength(*handle);

    return TRUE;
}


int
ReadSound(int handle, VOC_INFOp vp, int length)
{
    if (kread(handle, vp->data, length) != length)
    {
        TerminateGame();
        printf("Error reading file '%s'.\n", vp->name);
        exit(0);
    }

    vp->datalen = length;

    kclose(handle);
    return 0;
}

SWBOOL
LoadSong(const char *filename)
{
    int handle;
    int size;
    char *ptr;

    if ((handle = kopen4load(filename, 0)) == -1)
    {
        return FALSE;
    }

    size = kfilelength(handle);

    ptr = (char *) AllocMem(size);
    if (ptr == NULL)
    {
        kclose(handle);
        return FALSE;
    }

    if (kread(handle, ptr, size) != size)
    {
        FreeMem(ptr);
        kclose(handle);
        return FALSE;
    }

    kclose(handle);

    SongPtr = ptr;
    SongLength = size;

    return TRUE;
}


void FlipStereo(void)
{
    FX_SetReverseStereo(gs.FlipStereo);
}

void
SoundStartup(void)
{
    int32_t status;
    void *initdata = 0;

    // if they chose None lets return
    if (FXDevice < 0)
    {
        gs.FxOn = FALSE;
        return;
    }

#ifdef MIXERTYPEWIN
    initdata = (void *) win_gethwnd();
#endif

    //gs.FxOn = TRUE;

    status = FX_Init(NumVoices, NumChannels, MixRate, initdata);
    if (status == FX_Ok)
    {
        FxInitialized = TRUE;
        FX_SetVolume(gs.SoundVolume);

        if (gs.FlipStereo)
            FX_SetReverseStereo(!FX_GetReverseStereo());
    }
    if (status != FX_Ok)
    {
        buildprintf("Sound error: %s\n",FX_ErrorString(FX_Error));
    }

    FX_SetCallBack(SoundCallBack);
}

/*
===================
=
= SoundShutdown
=
===================
*/

void
SoundShutdown(void)
{
    int32_t status;

    // if they chose None lets return
    if (FXDevice < 0)
    {
        return;
    }

    if (!FxInitialized)
        return;

    status = FX_Shutdown();
    if (status != FX_Ok)
    {
        buildprintf("Sound error: %s\n",FX_ErrorString(FX_Error));
    }
}


/*
===================
=
= MusicStartup
=
===================
*/

#if 0
void loadtmb(void)
{
    char tmb[8000];
    int fil, l;

    fil = kopen4load("swtimbr.tmb",0);
    if (fil == -1)
        return;

    l = min((size_t)kfilelength(fil), sizeof(tmb));
    kread(fil,tmb,l);
    MUSIC_RegisterTimbreBank(tmb);
    kclose(fil);
}
#endif

void MusicStartup(void)
{
    // if they chose None lets return
    if (MusicDevice < 0)
    {
        gs.MusicOn = FALSE;
        return;
    }

    if (MUSIC_Init(0, 0) == MUSIC_Ok || MUSIC_Init(1, 0) == MUSIC_Ok)
    {
        MusicInitialized = TRUE;
        MUSIC_SetVolume(gs.MusicVolume);
    }
    else
    {
        buildprintf("Music error: %s\n",MUSIC_ErrorString(MUSIC_ErrorCode));
        gs.MusicOn = FALSE;
    }

#if 0
    if (MusicInitialized)
        loadtmb();
#endif
}

void COVER_SetReverb(int amt)
{
    FX_SetReverb(amt);
}

/*
===================
=
= MusicShutdown
=
===================
*/

void
MusicShutdown(void)
{
    int32_t status;

    // if they chose None lets return
    if (MusicDevice < 0)
        return;

    if (!MusicInitialized)
        return;

    StopSong();

    status = MUSIC_Shutdown();
    if (status != MUSIC_Ok)
    {
        buildprintf("Music error: %s\n",MUSIC_ErrorString(MUSIC_ErrorCode));
    }
}

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////
//
//  3D sound engine
//  Sound management routines that keep a list of
//  all sounds being played in a level.
//  Doppler and Panning effects are achieved here.
//
///////////////////////////////////////////////
// Declare and initialize linked list of vocs.
VOC3D_INFOp voc3dstart = NULL;
VOC3D_INFOp voc3dend = NULL;

//////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////
// Initialize new vocs in the 3D sound queue
///////////////////////////////////////////////
VOC3D_INFOp
InitNew3DSound(VOC3D_INFOp v3p)
{
    v3p->handle = -1;                    // Initialize handle to new sound
    // value
    v3p->owner = -1;
    v3p->deleted = FALSE;                // Used for when sound gets deleted

    return v3p;
}

///////////////////////////////////////////////
// Inserts new vocs in the 3D sound queue
///////////////////////////////////////////////
VOC3D_INFOp
Insert3DSound(void)
{
    VOC3D_INFOp vp, old;

    // Allocate memory for new sound
    // You can allocate new sounds as long as memory holds out.
    // If you run out of memory for sounds, you got problems anyway.
    vp = (VOC3D_INFOp) AllocMem(sizeof(VOC3D_INFO));
    ASSERT(vp != NULL);
    memset(vp,0xCC,sizeof(VOC3D_INFO)); // Zero out the memory

    if (!voc3dend)                      // First item in list
    {
        vp->next = vp->prev = NULL;
        voc3dend = vp;
        voc3dstart = vp;
        InitNew3DSound(vp);
        return vp;
    }

    old = voc3dend;                     // Put it on the end
    old->next = vp;
    vp->next = NULL;
    vp->prev = old;
    voc3dend = vp;

    InitNew3DSound(vp);
    return vp;

}

/////////////////////////////////////////////////////
// Deletes vocs in the 3D sound queue with no owners
/////////////////////////////////////////////////////
void
DeleteNoSoundOwner(short spritenum)
{
    VOC3D_INFOp vp, dp;

    vp = voc3dstart;

    while (vp)
    {
        dp = NULL;
        if (vp->owner == spritenum && vp->owner >= 0 && (vp->vp->voc_flags & vf_loop))
        {
            //DSPRINTF(ds,"Deleting owner %d\n",vp->owner);
            //MONO_PRINT(ds);

            // Make sure to stop active
            // sounds
            if (FX_SoundActive(vp->handle))
            {
                FX_StopSound(vp->handle);
            }

#if 0
            // Clean up the sound active counter for cacheing locks
            if (vp->FX_Ok) // Only decrement if sound ever played
                vp->vp->playing--; // Decrement instance of sound playing
            if (vp->vp->playing == 0 && vp->vp->lock > CACHE_UNLOCK_MAX)
                vp->vp->lock = CACHE_UNLOCK_MAX;
#endif


            dp = vp;                    // Point to sound to be deleted

            if (vp->prev)
            {
                vp->prev->next = vp->next;
            }
            else
            {
                voc3dstart = vp->next;  // New first item
                if (voc3dstart)
                    voc3dstart->prev = NULL;
            }

            if (vp->next)
            {
                vp->next->prev = vp->prev;      // Middle element
            }
            else
            {
                voc3dend = vp->prev;    // Delete last element
            }
        }

        vp = vp->next;

        if (dp != NULL)
            FreeMem(dp);                // Return memory to heap
    }
}

// This is called from KillSprite to kill a follow sound with no valid sprite owner
// Stops and active sound with the follow bit set, even play once sounds.
void DeleteNoFollowSoundOwner(short spritenum)
{
    VOC3D_INFOp vp, dp;
    SPRITEp sp = &sprite[spritenum];

    vp = voc3dstart;

    while (vp)
    {
        dp = NULL;
        // If the follow flag is set, compare the x and y addresses.
        if ((vp->flags & v3df_follow) && vp->x == &sp->x && vp->y == &sp->y)
        {
            if (FX_SoundActive(vp->handle))
            {
                FX_StopSound(vp->handle);
            }

#if 0
            if (vp->FX_Ok) // Only decrement if sound ever played
                vp->vp->playing--; // Decrement instance of sound playing
            if (vp->vp->playing == 0 && vp->vp->lock > CACHE_UNLOCK_MAX)
                vp->vp->lock = CACHE_UNLOCK_MAX;
#endif

            dp = vp;                    // Point to sound to be deleted

            if (vp->prev)
            {
                vp->prev->next = vp->next;
            }
            else
            {
                voc3dstart = vp->next;  // New first item
                if (voc3dstart)
                    voc3dstart->prev = NULL;
            }

            if (vp->next)
            {
                vp->next->prev = vp->prev;      // Middle element
            }
            else
            {
                voc3dend = vp->prev;    // Delete last element
            }
        }

        vp = vp->next;

        if (dp != NULL)
            FreeMem(dp);                // Return memory to heap
    }
}

///////////////////////////////////////////////
// Deletes vocs in the 3D sound queue
///////////////////////////////////////////////
void
Delete3DSounds(void)
{
    VOC3D_INFOp vp, dp;
    PLAYERp pp;
    int cnt=0;


    vp = voc3dstart;

    while (vp)
    {
        dp = NULL;
        if (vp->deleted)
        {
#if 0
            if (vp->FX_Ok) // Only decrement if sound ever played
                vp->vp->playing--; // Decrement instance of sound playing
            if (vp->vp->playing == 0 && vp->vp->lock > CACHE_UNLOCK_MAX)
                vp->vp->lock = CACHE_UNLOCK_MAX;
#endif

            //DSPRINTF(ds,"Delete3DSounds: deleting owner %d digi %d\n",vp->owner,vp->num);
            //MONO_PRINT(ds);
            // Reset Player talking flag if a voice was deleted
            //if(vp->num > DIGI_FIRSTPLAYERVOICE && vp->num < DIGI_LASTPLAYERVOICE)
            if (!vp->vp)
            {
                printf("Delete3DSounds(): NULL vp->vp\n");
            }
            else    // JBF: added null check
            if (vp->vp->priority == PRI_PLAYERVOICE || vp->vp->priority == PRI_PLAYERDEATH)
            {
                int16_t pnum;

                TRAVERSE_CONNECT(pnum)
                {
                    pp = &Player[pnum];

                    if (vp->num == pp->TalkVocnum)
                    {
                        pp->PlayerTalking = FALSE;
                        pp->TalkVocnum = -1;
                        pp->TalkVocHandle = -1;
                        //DSPRINTF(ds,"DELETED PLAYER VOICE VOC! NUM=%d\n",vp->num);
                        //MONO_PRINT(ds);
                    }
                }
            }

            dp = vp;                    // Point to sound to be deleted
            if (vp->prev)
            {
                vp->prev->next = vp->next;
            }
            else
            {
                voc3dstart = vp->next;  // New first item
                if (voc3dstart)
                    voc3dstart->prev = NULL;
            }

            if (vp->next)
            {
                vp->next->prev = vp->prev;      // Middle element
            }
            else
            {
                voc3dend = vp->prev;    // Delete last element
            }
        }

        vp = vp->next;

        if (dp != NULL)
        {
            FreeMem(dp);                // Return memory to heap
        }
    }
}

////////////////////////////////////////////////////////////////////////////
// Play a sound
////////////////////////////////////////////////////////////////////////////

int
RandomizeAmbientSpecials(int handle)
{
#define MAXRNDAMB 12
    int ambrand[] =
    {
        56,57,58,59,60,61,62,63,64,65,66,67
    };
    short i;

    // If ambient sound is found in the array, randomly pick a new sound
    for (i=0; i<MAXRNDAMB; i++)
    {
        if (handle == ambrand[i])
            return ambrand[STD_RANDOM_RANGE(MAXRNDAMB-1)];
    }

    return handle;   // Give back the sound, no new one was found
}

void
DoTimedSound(VOC3D_INFOp p)
{
    p->tics += synctics;

    if (p->tics >= p->maxtics)
    {
        if (!FX_SoundActive(p->handle))
        {
            // Check for special case ambient sounds
            p->num = RandomizeAmbientSpecials(p->num);

            // Sound was bumped from active sounds list, try to play again.
            // Don't bother if voices are already maxed out.
            if (FX_SoundsPlaying() < NumVoices)
            {
                if (p->flags & v3df_follow)
                {
                    PlaySound(p->num, p->x, p->y, p->z, p->flags);
                    p->deleted = TRUE;  // Mark old sound for deletion
                }
                else
                {
                    PlaySound(p->num, &p->fx, &p->fy, &p->fz, p->flags);
                    p->deleted = TRUE;  // Mark old sound for deletion
                }
            }
        }                           // !FX_SoundActive

        p->tics = 0;
        //while (p->tics >= p->maxtics)  // Really stupid thing to do!
        //    {
        //    p->tics -= p->maxtics;
        //    }
    }
}

void
StopAmbientSound(void)
{
    VOC3D_INFOp p;
    extern SWBOOL InMenuLevel;

    if (InMenuLevel) return;

    p = voc3dstart;

    while (p)
    {
        // kill ambient sounds if Ambient is off
        if (TEST(p->flags,v3df_ambient))
            SET(p->flags, v3df_kill);

        if (p->flags & v3df_kill)
        {
            if (FX_SoundActive(p->handle))
                FX_StopSound(p->handle); // Make sure to stop active sounds

            p->deleted = TRUE;
        }

        p = p->next;
    }

    Delete3DSounds();
}

void
StartAmbientSound(void)
{
    VOC3D_INFOp p;
    short i,nexti;
    extern SWBOOL InMenuLevel;

    if (InMenuLevel) return; // Don't restart ambience if no level is active! Will crash game.

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_AMBIENT], i, nexti)
    {
        SPRITEp sp = &sprite[i];

        PlaySound(sp->lotag, &sp->x, &sp->y, &sp->z, v3df_ambient | v3df_init
                  | v3df_doppler | v3df_follow);
        Set3DSoundOwner(i);  // Ambient sounds need this to get sectnum for later processing
    }
}

///////////////////////////////////////////////
// Main function to update 3D sound array
///////////////////////////////////////////////
typedef struct
{
    VOC3D_INFOp p;
    short dist;
    uint8_t priority;
} TVOC_INFO, *TVOC_INFOp;

void
DoUpdateSounds3D(void)
{
    VOC3D_INFOp p;
    SWBOOL looping;
    int pitch = 0, pitchmax;
    int delta;
    short dist, angle;
    SWBOOL deletesound = FALSE;

    TVOC_INFO TmpVocArray[32];
    int i;
    static SWBOOL MoveSkip8 = 0;

    if (UsingMenus) return;

    // This function is already only call 10x per sec, this widdles it down even more!
    MoveSkip8 = (MoveSkip8 + 1) & 15;

    //CON_Message("Sounds Playing = %d",FX_SoundsPlaying());

    // Zero out the temporary array
    //memset(&TmpVocArray[0],0,sizeof(TmpVocArray));
    for (i=0; i<32; i++)
    {
        TmpVocArray[i].p = NULL;
        TmpVocArray[i].dist = 0;
        TmpVocArray[i].priority = 0;
    }

    p = voc3dstart;

    while (p)
    {
        ASSERT(p->num >= 0 && p->num < DIGI_MAX);

        looping = p->vp->voc_flags & vf_loop;

//      //DSPRINTF(ds,"sound %d FX_SoundActive = %d\n,",p->num,FX_SoundActive(p->handle));
//      MONO_PRINT(ds);

        // If sprite owner is dead, kill this sound as long as it isn't ambient
        if (looping && p->owner == -1 && !TEST(p->flags,v3df_ambient))
        {
            SET(p->flags, v3df_kill);
        }

        // Is the sound slated for death? Kill it, otherwise play it.
        if (p->flags & v3df_kill)
        {
            if (FX_SoundActive(p->handle))
                FX_StopSound(p->handle); // Make sure to stop active sounds

            //DSPRINTF(ds,"%d had v3df_kill.\n",p->num);
            //MONO_PRINT(ds);
            p->deleted = TRUE;
        }
        else
        {
            if (!FX_SoundActive(p->handle) && !looping)
            {
                if (p->flags & v3df_intermit)
                {
                    DoTimedSound(p);
                }
                else
                //if(p->owner == -1 && !TEST(p->flags,v3df_ambient))
                {
                    //DSPRINTF(ds,"%d is now inactive.\n",p->num);
                    //MONO_PRINT(ds);
                    p->deleted = TRUE;
                }
            }
            else if (FX_SoundActive(p->handle))
            {
                if (p->flags & v3df_follow)
                {
                    dist = SoundDist(*p->x, *p->y, *p->z, p->vp->voc_distance);
                    angle = SoundAngle(*p->x, *p->y);
                }
                else
                {
                    if (p->fx == 0 && p->fy == 0 && p->fz == 0)
                        dist = 0;
                    else
                        dist = SoundDist(p->fx, p->fy, p->fz, p->vp->voc_distance);
                    angle = SoundAngle(p->fx, p->fy);
                }

                // Can the ambient sound see the player?  If not, tone it down some.
                if ((p->vp->voc_flags & vf_loop) && p->owner != -1)
                {
                    PLAYERp pp = Player+screenpeek;
                    SPRITEp sp = &sprite[p->owner];

                    //MONO_PRINT("Checking sound cansee");
                    if (!FAFcansee(sp->x, sp->y, sp->z, sp->sectnum,pp->posx, pp->posy, pp->posz, pp->cursectnum))
                    {
                        //MONO_PRINT("Reducing sound distance");
                        dist += ((dist/2)+(dist/4));  // Play more quietly
                        if (dist > 255) dist = 255;

                        // Special cases
                        if (p->num == 76 && TEST(p->flags,v3df_ambient))
                        {
                            dist = 255; // Cut off whipping sound, it's secret
                        }

                    }
                }

                if (dist >= 255 && p->vp->voc_distance == DIST_NORMAL)
                {
                    FX_StopSound(p->handle);    // Make sure to stop active
                    // sounds
                }
                else
                {
                    // Handle Panning Left and Right
                    if (!(p->flags & v3df_dontpan))
                        FX_Pan3D(p->handle, angle, dist);
                    else
                        FX_Pan3D(p->handle, 0, dist);

                    // Handle Doppler Effects
#define DOPPLERMAX  400
                    if (!(p->flags & v3df_doppler) && FX_SoundActive(p->handle))
                    {
                        pitch -= (dist - p->doplr_delta);

                        if (p->vp->pitch_lo != 0 && p->vp->pitch_hi != 0)
                        {
                            if (abs(p->vp->pitch_lo) > abs(p->vp->pitch_hi))
                                pitchmax = abs(p->vp->pitch_lo);
                            else
                                pitchmax = abs(p->vp->pitch_hi);

                        }
                        else
                            pitchmax = DOPPLERMAX;

                        if (pitch > pitchmax)
                            pitch = pitchmax;
                        if (pitch < -pitchmax)
                            pitch = -pitchmax;

                        p->doplr_delta = dist;  // Save new distance to
                        // struct
                        FX_SetPitch(p->handle, pitch);
                    }
                }
            }
            else if (!FX_SoundActive(p->handle) && looping)
            {
                if (p->flags & v3df_follow)
                {
                    dist = SoundDist(*p->x, *p->y, *p->z, p->vp->voc_distance);
                    angle = SoundAngle(*p->x, *p->y);
                }
                else
                {
                    dist = SoundDist(p->fx, p->fy, p->fz, p->vp->voc_distance);
                    angle = SoundAngle(p->fx, p->fy);
                }

                // Sound was bumped from active sounds list, try to play
                // again.
                // Don't bother if voices are already maxed out.
                // Sort looping vocs in order of priority and distance
                //if (FX_SoundsPlaying() < NumVoices && dist <= 255)
                if (dist <= 255)
                {
                    for (i=0; i<min((int)SIZ(TmpVocArray), NumVoices); i++)
                    {
                        if (p->priority >= TmpVocArray[i].priority)
                        {
                            if (!TmpVocArray[i].p || dist < TmpVocArray[i].dist)
                            {
                                ASSERT(p->num >= 0 && p->num < DIGI_MAX);
                                TmpVocArray[i].p = p;
                                TmpVocArray[i].dist = dist;
                                TmpVocArray[i].priority = p->priority;
                                break;
                            }
                        }
                    }
                }
            }                       // !FX_SoundActive
        }                           // if(p->flags & v3df_kill)

        p = p->next;
    }                               // while(p)

//  //DSPRINTF(ds,"Num vocs in list: %d, Sounds playing: %d\n",numelems,FX_SoundsPlaying());
//  MONO_PRINT(ds);

    // Process all the looping sounds that said they wanted to get back in
    // Only update these sounds 5x per second!  Woo hoo!, aren't we optimized now?
    //if(MoveSkip8==0)
    //    {
    for (i=0; i<min((int)SIZ(TmpVocArray), NumVoices); i++)
    {
        int handle;

        p = TmpVocArray[i].p;

        //if (FX_SoundsPlaying() >= NumVoices || !p) break;
        if (!p) break;

        ASSERT(p->num >= 0 && p->num < DIGI_MAX);

        if (p->flags & v3df_follow)
        {
            if (p->owner == -1)
            {
                int enumber;
                enumber = p->num;
                TerminateGame();
                printf("Owner == -1 on looping sound with follow flag set!\n");
                printf("p->num = %d\n",enumber);
                exit(0);
            }

            Use_SoundSpriteNum = TRUE;
            SoundSpriteNum = p->owner;

            handle = PlaySound(p->num, p->x, p->y, p->z, p->flags);
            //if(handle >= 0 || TEST(p->flags,v3df_ambient)) // After a valid PlaySound, it's ok to use voc3dend
            voc3dend->owner = p->owner; // Transfer the owner
            p->deleted = TRUE;

            Use_SoundSpriteNum = FALSE;
            SoundSpriteNum = -1;

            //MONO_PRINT("TmpVocArray playing a follow sound");
        }
        else
        {
            if (p->owner == -1)
            {
                int enumber;
                enumber = p->num;
                TerminateGame();
                printf("Owner == -1 on looping sound, no follow flag.\n");
                printf("p->num = %d\n",enumber);
                exit(0);
            }

            Use_SoundSpriteNum = TRUE;
            SoundSpriteNum = p->owner;

            handle = PlaySound(p->num, &p->fx, &p->fy, &p->fz, p->flags);
            //if(handle >= 0 || TEST(p->flags,v3df_ambient))
            voc3dend->owner = p->owner; // Transfer the owner
            p->deleted = TRUE;

            Use_SoundSpriteNum = FALSE;
            SoundSpriteNum = -1;

            //MONO_PRINT("TmpVocArray playing a non-follow sound");
        }
    }
    //    } // MoveSkip8

    // Clean out any deleted sounds now
    Delete3DSounds();
}

//////////////////////////////////////////////////
// Terminate the sounds list
//////////////////////////////////////////////////
void
Terminate3DSounds(void)
{
    VOC3D_INFOp vp;

    vp = voc3dstart;

    while (vp)
    {
        FX_StopSound(vp->handle);       // Make sure to stop active sounds
        vp->deleted = TRUE;
        vp = vp->next;
    }

    Delete3DSounds();                   // Now delete all remaining sounds
    ClearSoundLocks();
}

void
DumpSounds(void)
{
    VOC3D_INFOp vp;

    vp = voc3dstart;

    while (vp)
    {
        if (TEST(vp->flags,v3df_ambient))
            sprintf(ds,"vp->num=%d, vp->owner=%d, vp->vp->lock=%d",ambarray[vp->num].diginame,vp->owner,vp->vp->lock);
        else
            sprintf(ds,"vp->num=%d, vp->owner=%d, vp->vp->lock=%d",vp->num,vp->owner,vp->vp->lock);
        DebugWriteString(ds);
        if (vp->owner >= 0)
        {
            SPRITEp sp = &sprite[vp->owner];
            sprintf(ds,"sp->picnum=%d, sp->hitag=%d, sp->lotag=%d, sp->owner=%d\n",sp->picnum,sp->hitag,sp->lotag,sp->owner);
            DebugWriteString(ds);
        }
        vp = vp->next;
    }
}


//////////////////////////////////////////////////
// Set owner to check when to kill looping sounds
// Must be called immediately after PlaySound call
// since this only assigns value to last sound
// on voc list
//////////////////////////////////////////////////
void
Set3DSoundOwner(short spritenum)
{
    VOC3D_INFOp p;

//  ASSERT(p->handle != -1); // Check for bogus sounds

    p = voc3dend;
    if (!p) return;

    // Queue up sounds with ambient flag even if they didn't play right away!
    if (p->handle != -1 || TEST(p->flags,v3df_ambient))
    {
        p->owner = spritenum;
    }
    else
    {
        p->deleted = TRUE;
        p->flags = v3df_kill;
    }
}

//////////////////////////////////////////////////
// Play a sound using special sprite setup
//////////////////////////////////////////////////
void
PlaySpriteSound(short spritenum, int attrib_ndx, Voc3D_Flags flags)
{
    SPRITEp sp = &sprite[spritenum];
    USERp u = User[spritenum];

    ASSERT(u);

//  //DSPRINTF(ds,"index = %d, digi num = %d\n",attrib_ndx,u->Attrib->Sounds[attrib_ndx]);
//  MONO_PRINT(ds);
    PlaySound(u->Attrib->Sounds[attrib_ndx], &sp->x, &sp->y, &sp->z, flags);
}

// vim:ts=4:sw=4:expandtab:
