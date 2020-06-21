//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors
Copyright (C) 2019 Christoph Oelckers

This is free software; you can redistribute it and/or
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

#include "ns.h"	// Must come before everything else!

#include "compat.h"

#include "duke3d.h"
#include "raze_music.h"
#include "mapinfo.h"
#include "raze_sound.h"

BEGIN_DUKE_NS

class DukeSoundEngine : public SoundEngine
{
    // client specific parts of the sound engine go in this class.
    void CalcPosVel(int type, const void* source, const float pt[3], int channum, int chanflags, FSoundID chanSound, FVector3* pos, FVector3* vel, FSoundChan* chan) override;
    TArray<uint8_t> ReadSound(int lumpnum) override;

public:
    DukeSoundEngine()
    {
        S_Rolloff.RolloffType = ROLLOFF_Doom;   // Seems like Duke uses the same rolloff type as Doom.
        S_Rolloff.MinDistance = 144;            // was originally 576 which looks like a bug and sounds like crap.
        S_Rolloff.MaxDistance = 1088;
    }

    void StopChannel(FSoundChan* chan) override
    {
        if (chan && chan->SysChannel != NULL && !(chan->ChanFlags & CHANF_EVICTED) && chan->SourceType == SOURCE_Actor)
        {
            chan->Source = NULL;
            chan->SourceType = SOURCE_Unattached;
        }
        SoundEngine::StopChannel(chan);
    }

};

void S_InitSound()
{
    soundEngine = new DukeSoundEngine;
}

//==========================================================================
//
//
// 
//==========================================================================

TArray<uint8_t> DukeSoundEngine::ReadSound(int lumpnum)
{
    auto wlump = fileSystem.OpenFileReader(lumpnum);
    return wlump.Read();
}

//==========================================================================
//
// 
//
//==========================================================================

void S_PauseSounds(bool paused)
{
    soundEngine->SetPaused(paused);
}

//==========================================================================
//
// 
//
//==========================================================================

void cacheAllSounds(void)
{
    auto& sfx = soundEngine->GetSounds();
    int i = 0;
    for(auto &snd : sfx)
    {
        soundEngine->CacheSound(&snd);
        if (((++i)&31) == 0)
            G_HandleAsync();
    }
}

//==========================================================================
//
// 
//
//==========================================================================

static inline int S_GetPitch(int num)
{
    auto const* snd = soundEngine->GetUserData(num+1);
    if (!snd) return 0;
    int const   range = abs(snd[kPitchEnd] - snd[kPitchStart]);
    return (range == 0) ? snd[kPitchStart] : min(snd[kPitchStart], snd[kPitchEnd]) + rand() % range;
}

float S_ConvertPitch(int lpitch)
{
    return pow(2, lpitch / 1200.);   // I hope I got this right that ASS uses a linear scale where 1200 is a full octave.
}

int S_GetUserFlags(int num)
{
    if (!soundEngine->isValidSoundId(num+1)) return 0;
    auto const* snd = soundEngine->GetUserData(num + 1);
    if (!snd) return 0;
    return snd[kFlags];
}

//==========================================================================
//
// 
//
//==========================================================================

int S_DefineSound(unsigned index, const char *filename, int minpitch, int maxpitch, int priority, int type, int distance, float volume)
{
    if ((unsigned)index >= MAXSOUNDS)
        return -1;

    auto& S_sfx = soundEngine->GetSounds();
    index++;
    unsigned oldindex = S_sfx.Size();
    if (index >= S_sfx.Size())
    {
        S_sfx.Resize(index + 1);
        for (; oldindex <= index; oldindex++)
        {
            S_sfx[oldindex].Clear();
        }
    }
    auto sfx = &S_sfx[index];
    bool alreadydefined = !sfx->bTentative;
    sfx->UserData.Resize(kMaxUserData);
    auto sndinf = sfx->UserData.Data();
    sndinf[kFlags] = type & ~SF_ONEINST_INTERNAL;
    if (sndinf[kFlags] & SF_LOOP)
        sndinf[kFlags] |= SF_ONEINST_INTERNAL;

    sfx->lumpnum = S_LookupSound(filename);
    sndinf[kPitchStart] = clamp(minpitch, INT16_MIN, INT16_MAX);
    sndinf[kPitchEnd] = clamp(maxpitch, INT16_MIN, INT16_MAX);
    sndinf[kPriority] = priority & 255;
    sndinf[kVolAdjust] = clamp(distance, INT16_MIN, INT16_MAX);
    sfx->Volume = volume;
    sfx->NearLimit = 6;
    sfx->bTentative = false;
    sfx->name = filename;
    return 0;
}


//==========================================================================
//
// 
//
//==========================================================================

static int S_CalcDistAndAng(int spriteNum, int soundNum, int sectNum,
                             const vec3_t *cam, const vec3_t *pos, int *distPtr, FVector3 *sndPos)
{
    // There's a lot of hackery going on here that could be mapped to rolloff and attenuation parameters.
    // However, ultimately rolloff would also just reposition the sound source so this can remain as it is.

    int orgsndist = 0, sndang = 0, sndist = 0, explosion = 0;
    auto const* snd = soundEngine->GetUserData(soundNum + 1);
    int userflags = snd ? snd[kFlags] : 0;
    int dist_adjust = snd ? snd[kVolAdjust] : 0;

    if (PN(spriteNum) != APLAYER || P_Get(spriteNum) != screenpeek)
    {
        orgsndist = sndist = FindDistance3D(cam->x - pos->x, cam->y - pos->y, (cam->z - pos->z));

        if ((userflags & (SF_GLOBAL | SF_DTAG)) != SF_GLOBAL && S_IsAmbientSFX(spriteNum) && (sector[SECT(spriteNum)].lotag & 0xff) < 9)  // ST_9_SLIDING_ST_DOOR
            sndist = divscale14(sndist, SHT(spriteNum) + 1);
    }

    sndist += dist_adjust;
    if (sndist < 0)
        sndist = 0;

    if (sectNum > -1 && sndist && PN(spriteNum) != MUSICANDSFX
        && !cansee(cam->x, cam->y, cam->z - (24 << 8), sectNum, SX(spriteNum), SY(spriteNum), SZ(spriteNum) - (24 << 8), SECT(spriteNum)))
        sndist += sndist>>(RR?2:5);

    // Here the sound distance was clamped to a minimum of 144*4. 
    // It's better to handle rolloff in the backend instead of whacking the sound origin here.
    // That way the lower end can be made customizable instead of losing all precision right here at the source.
    if (sndist < 0) sndist = 0;

    if (distPtr)
    {
        *distPtr = sndist;
    }

    if (sndPos)
    {
        FVector3 sndorg = GetSoundPos(pos);
        FVector3 campos = GetSoundPos(cam);
        // Now calculate the virtual position in sound system coordinates.
        FVector3 sndvec = sndorg - campos;
        if (orgsndist > 0)
        {
            float scale = float(sndist) / orgsndist;                                                // adjust by what was calculated above;
            *sndPos = campos + sndvec * scale;
        }
        else *sndPos = campos;
    }

    return false;
}

//==========================================================================
//
//
//
//==========================================================================

void S_GetCamera(vec3_t** c, int32_t* ca, int32_t* cs)
{
    if (ud.camerasprite == -1)
    {
        if (ud.overhead_on != 2)
        {
            if (c) *c = &CAMERA(pos);
            if (cs) *cs = CAMERA(sect);
            if (ca) *ca = fix16_to_int(CAMERA(q16ang));
        }
        else
        {
            auto pPlayer = g_player[screenpeek].ps;
            if (c) *c = &pPlayer->pos;
            if (cs) *cs = pPlayer->cursectnum;
            if (ca) *ca = fix16_to_int(pPlayer->q16ang);
        }
    }
    else
    {
        if (c) *c = &sprite[ud.camerasprite].pos;
        if (cs) *cs = sprite[ud.camerasprite].sectnum;
        if (ca) *ca = sprite[ud.camerasprite].ang;
    }
}

//=========================================================================
//
// CalcPosVel
//
// The game specific part of the sound updater.
//
//=========================================================================

void DukeSoundEngine::CalcPosVel(int type, const void* source, const float pt[3], int channum, int chanflags, FSoundID chanSound, FVector3* pos, FVector3* vel, FSoundChan* chan)
{
    if (pos != nullptr)
    {
        vec3_t* campos;
        int32_t camsect;

        S_GetCamera(&campos, nullptr, &camsect);
        if (vel) vel->Zero();

        if (type == SOURCE_Unattached)
        {
            pos->X = pt[0];
            pos->Y = pt[1];
            pos->Z = pt[2];
        }
        else if (type == SOURCE_Actor)
        {
            auto actor = (spritetype*)source;
            assert(actor != nullptr);
            if (actor != nullptr)
            {
                S_CalcDistAndAng(int(actor - sprite), chanSound - 1, camsect, campos, &actor->pos, nullptr, pos);
                /*
                if (vel) // DN3D does not properly maintain this.
                {
                    vel->X = float(actor->Vel.X * TICRATE);
                    vel->Y = float(actor->Vel.Z * TICRATE);
                    vel->Z = float(actor->Vel.Y * TICRATE);
                }
                */
            }
        }
        if ((chanflags & CHANF_LISTENERZ) && campos != nullptr && type != SOURCE_None)
        {
            pos->Y = campos->z / 256.f;
        }
    }
}


//==========================================================================
//
//
//
//==========================================================================

void S_Update(void)
{
    SoundListener listener;
    vec3_t* c;
    int32_t ca, cs;
	
    auto& gm = g_player[myconnectindex].ps->gm;
    if (RR && !Mus_IsPlaying() && (gm && gm & MODE_GAME))
        S_PlayRRMusic(); 

    S_GetCamera(&c, &ca, &cs);

    if (c != nullptr)
    {
        listener.angle = -(float)ca * pi::pi() / 1024; // Build uses a period of 2048.
        listener.velocity.Zero();
        listener.position = GetSoundPos(c);
        listener.underwater = false; 
        // This should probably use a real environment instead of the pitch hacking in S_PlaySound3D.
        // listenactor->waterlevel == 3;
        //assert(primaryLevel->Zones.Size() > listenactor->Sector->ZoneNumber);
        listener.Environment = 0;// primaryLevel->Zones[listenactor->Sector->ZoneNumber].Environment;
        listener.valid = true;
    }
    else
    { 
        listener.angle = 0;
        listener.position.Zero();
        listener.velocity.Zero();
        listener.underwater = false;
        listener.Environment = nullptr;
        listener.valid = false;
    }
    listener.ListenerObject = ud.camerasprite == -1 ? nullptr : &sprite[ud.camerasprite];
    soundEngine->SetListener(listener);
    soundEngine->UpdateSounds((int)totalclock);
}


//==========================================================================
//
//
//
//==========================================================================

int S_PlaySound3D(int sndnum, int spriteNum, const vec3_t* pos, int channel, EChanFlags flags)
{
    auto const pPlayer = g_player[myconnectindex].ps;
    if (!soundEngine->isValidSoundId(sndnum+1) || !SoundEnabled() || (unsigned)spriteNum >= MAXSPRITES || (pPlayer->gm & MODE_MENU) ||
        (pPlayer->timebeforeexit > 0 && pPlayer->timebeforeexit <= GAMETICSPERSEC * 3)) return -1;

    int userflags = S_GetUserFlags(sndnum);
    if ((!(snd_speech & 1) && (userflags & SF_TALK)) || ((userflags & SF_ADULT) && adult_lockout))
        return -1;

    // Duke talk
    if (userflags & SF_TALK)
    {
        if ((g_netServer || ud.multimode > 1) && PN(spriteNum) == APLAYER && P_Get(spriteNum) != screenpeek) // other player sound
        {
            if ((snd_speech & 4) != 4)
                return -1;
        }
        else if ((snd_speech & 1) != 1)
            return -1;


        bool foundone =  soundEngine->EnumerateChannels([&](FSoundChan* chan)
            {
                auto sid = chan->OrgID;
                auto flags = S_GetUserFlags(sid - 1);
                return !!(flags & SF_TALK);
            });
        // don't play if any Duke talk sounds are already playing
        if (foundone) return -1;
    }
    else if ((userflags & (SF_DTAG | SF_GLOBAL)) == SF_DTAG)  // Duke-Tag sound
    {
        return S_PlaySound(sndnum);

    }

    int32_t    sndist;
    FVector3 sndpos;    // this is in sound engine space.

    vec3_t* campos;
    int32_t camsect;

    S_GetCamera(&campos, nullptr, &camsect);
    S_CalcDistAndAng(spriteNum, sndnum, camsect, campos, pos, &sndist, &sndpos);
    int        pitch = S_GetPitch(sndnum);
    auto const pOther = g_player[screenpeek].ps;


    if (pOther->sound_pitch)
        pitch += pOther->sound_pitch;

    bool explosionp = ((userflags & (SF_GLOBAL | SF_DTAG)) == (SF_GLOBAL | SF_DTAG)) || ((sndnum == PIPEBOMB_EXPLODE || sndnum == LASERTRIP_EXPLODE || sndnum == RPG_EXPLODE));

    if (explosionp)
    {
        if (pOther->cursectnum > -1 && sector[pOther->cursectnum].lotag == ST_2_UNDERWATER)
            pitch -= 1024;
    }
    else
    {
        if (sndist > 32767 && PN(spriteNum) != MUSICANDSFX && (userflags & (SF_LOOP | SF_MSFX)) == 0)
            return -1;

        if (pOther->cursectnum > -1 && sector[pOther->cursectnum].lotag == ST_2_UNDERWATER
            && (userflags & SF_TALK) == 0)
            pitch = -768;
    }

    bool is_playing = soundEngine->GetSoundPlayingInfo(SOURCE_Any, nullptr, sndnum+1);
    if (is_playing &&  PN(spriteNum) != MUSICANDSFX)
        S_StopEnvSound(sndnum, spriteNum);

    int const repeatp = (userflags & SF_LOOP);

    if (repeatp && (userflags & SF_ONEINST_INTERNAL) && is_playing)
    {
        return -1;
    }

    // These explosion sounds originally used some distance hackery to make them louder but due to how the rolloff was set up they always played at full volume as a result.
    // I think it is better to lower their attenuation so that they are louder than the rest but still fade in the distance.
    // For the original effect, attenuation needs to be set to ATTN_NONE here.
    float attenuation;
    if (explosionp) attenuation = 0.5f;
    else attenuation = (userflags & (SF_GLOBAL | SF_DTAG)) == SF_GLOBAL ? ATTN_NONE : ATTN_NORM;

    if (userflags & SF_LOOP) flags |= CHANF_LOOP;
    auto chan = soundEngine->StartSound(SOURCE_Actor, &sprite[spriteNum], &sndpos, CHAN_AUTO, flags, sndnum+1, attenuation == ATTN_NONE? 0.8f : 1.f, attenuation, nullptr, S_ConvertPitch(pitch));
    return chan ? 0 : -1;
}

//==========================================================================
//
//
//
//==========================================================================

int S_PlaySound(int sndnum, int channel, EChanFlags flags)
{
    if (!soundEngine->isValidSoundId(sndnum+1) || !SoundEnabled()) return -1;

    int userflags = S_GetUserFlags(sndnum);
    if ((!(snd_speech & 1) && (userflags & SF_TALK)) || ((userflags & SF_ADULT) && adult_lockout))
        return -1;

    int const pitch = S_GetPitch(sndnum);

    if (userflags & SF_LOOP) flags |= CHANF_LOOP;
    auto chan = soundEngine->StartSound(SOURCE_None, nullptr, nullptr, channel, flags, sndnum + 1, 0.8f, ATTN_NONE, nullptr, S_ConvertPitch(pitch));
    return chan ? 0 : -1;
}

//==========================================================================
//
//
//
//==========================================================================

int A_PlaySound(int soundNum, int spriteNum, int channel, EChanFlags flags)
{
    return (unsigned)spriteNum >= MAXSPRITES ? S_PlaySound(soundNum, channel, flags) :
        S_PlaySound3D(soundNum, spriteNum, &sprite[spriteNum].pos, channel, flags);
}

void S_StopEnvSound(int sndNum, int sprNum, int channel)
{
    if (sprNum < -1 || sprNum >= MAXSPRITES) return;

    if (sprNum == -1) soundEngine->StopSoundID(sndNum+1);
    else
    {
        if (channel == -1) soundEngine->StopSound(SOURCE_Actor, &sprite[sprNum], -1, sndNum + 1);
        else soundEngine->StopSound(SOURCE_Actor, &sprite[sprNum], channel, -1);

        // StopSound kills the actor reference so this cannot be delayed until ChannelEnded gets called. At that point the actor may also not be valid anymore.
        if (S_IsAmbientSFX(sprNum) && sector[SECT(sprNum)].lotag < 3)  // ST_2_UNDERWATER
            actor[sprNum].t_data[0] = 0;
    }
}

void S_ChangeSoundPitch(int soundNum, int spriteNum, int pitchoffset)
{
    if (spriteNum < -1 || spriteNum >= MAXSPRITES) return;
    double expitch = pow(2, pitchoffset / 1200.);   // I hope I got this right that ASS uses a linear scale where 1200 is a full octave.
    if (spriteNum == -1)
    {
        soundEngine->ChangeSoundPitch(SOURCE_Unattached, nullptr, CHAN_AUTO, expitch, soundNum+1);
    }
    else
    {
        soundEngine->ChangeSoundPitch(SOURCE_Actor, &sprite[spriteNum], CHAN_AUTO, expitch, soundNum+1);
    }
}

//==========================================================================
//
//
//
//==========================================================================

int A_CheckSoundPlaying(int spriteNum, int soundNum, int channel)
{
    if (spriteNum == -1) return soundEngine->GetSoundPlayingInfo(SOURCE_Any, nullptr, soundNum+1);
    if ((unsigned)spriteNum >= MAXSPRITES) return false;
    return soundEngine->IsSourcePlayingSomething(SOURCE_Actor, &sprite[spriteNum], channel, soundNum+1);
}

// Check if actor <i> is playing any sound.
int A_CheckAnySoundPlaying(int spriteNum)
{
    if ((unsigned)spriteNum >= MAXSPRITES) return false;
    return soundEngine->IsSourcePlayingSomething(SOURCE_Actor, &sprite[spriteNum], CHAN_AUTO, 0);
}

int S_CheckSoundPlaying(int soundNum)
{
    return soundEngine->GetSoundPlayingInfo(SOURCE_Any, nullptr, soundNum+1);
}

//==========================================================================
//
//
//
//==========================================================================

void S_MenuSound(void)
{
    static int SoundNum;
    int const menusnds[] = {
        LASERTRIP_EXPLODE, DUKE_GRUNT,       DUKE_LAND_HURT,   CHAINGUN_FIRE, SQUISHED,      KICK_HIT,
        PISTOL_RICOCHET,   PISTOL_BODYHIT,   PISTOL_FIRE,      SHOTGUN_FIRE,  BOS1_WALK,     RPG_EXPLODE,
        PIPEBOMB_BOUNCE,   PIPEBOMB_EXPLODE, NITEVISION_ONOFF, RPG_SHOOT,     SELECT_WEAPON,
    };
    int s = RR ? 390 : menusnds[SoundNum++ % ARRAY_SIZE(menusnds)];
    if (s != -1)
        S_PlaySound(s, CHAN_AUTO, CHANF_UI);
}

//==========================================================================
//
// Music
//
//==========================================================================

static bool cd_disabled = false;    // This is in case mus_redbook is enabled but no tracks found so that the regular music system can be switched on.

static void S_SetMusicIndex(unsigned int m)
{
    ud.music_episode = m / MAXLEVELS;
    ud.music_level = m % MAXLEVELS;
}

void S_PlayLevelMusicOrNothing(unsigned int m)
{
    auto& mr = m == USERMAPMUSICFAKESLOT ? userMapRecord : mapList[m];
    if (RR && mr.music.IsEmpty() && mus_redbook && !cd_disabled) return;
    Mus_Play(mr.labelName, mr.music, true);
    S_SetMusicIndex(m);
}

int S_TryPlaySpecialMusic(unsigned int m)
{
    if (RR) return 0;   // Can only be MUS_LOADING, RR does not use it.
    auto& musicfn = mapList[m].music;
    if (musicfn.IsNotEmpty())
    {
        if (!Mus_Play(nullptr, musicfn, true))
        {
            S_SetMusicIndex(m);
            return 0;
        }
    }

    return 1;
}

void S_PlaySpecialMusicOrNothing(unsigned int m)
{
    if (S_TryPlaySpecialMusic(m))
    {
        S_SetMusicIndex(m);
    }
}

void S_PlayRRMusic(int newTrack)
{
    if (!RR || !mus_redbook || cd_disabled || currentLevel->music.IsNotEmpty())
        return;
    Mus_Stop();

    for (int i = 0; i < 10; i++)
    {
        g_cdTrack = newTrack != -1 ? newTrack : g_cdTrack + 1;
        if (newTrack != 10 && (g_cdTrack > 9 || g_cdTrack < 2))
            g_cdTrack = 2;

        FStringf filename("track%02d.ogg", g_cdTrack);
        if (Mus_Play(nullptr, filename, false)) return;
    }
    // If none of the tracks managed to start, disable the CD music for this session so that regular music can play if defined.
    cd_disabled = true;
}


END_DUKE_NS
