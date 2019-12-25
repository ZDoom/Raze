//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT, sirlemonhead
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
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
#include "ns.h"
#include "compat.h"
#include "baselayer.h"
#include "build.h"
#include "engine.h"
#include "exhumed.h"
#include "config.h"
#include "sound.h"
#include "init.h"
#include "object.h"
#include "player.h"
#include "random.h"
#include "snake.h"
#include "trigdat.h"
#include "sequence.h"
#include "cd.h"
#include "sound/s_soundinternal.h"

BEGIN_PS_NS

const char *SoundFiles[kMaxSoundFiles] =
{
  "spl_big",
  "spl_smal",
  "bubble_l",
  "grn_drop",
  "p_click",
  "grn_roll",
  "cosprite",
  "m_chant0",
  "anu_icu",
  "item_reg",
  "item_spe", // 10
  "item_key",
  "torch_on", // 12
  "jon_bnst",
  "jon_gasp",
  "jon_land",
  "jon_gags",
  "jon_fall",
  "jon_drwn",
  "jon_air1",
  "jon_glp1", // 20
  "jon_bbwl",
  "jon_pois",
  "amb_ston",
  "cat_icu",
  "bubble_h",
  "set_land",
  "jon_hlnd",
  "jon_laf2",
  "spi_jump",
  "jon_scub", // 30
  "item_use",
  "tr_arrow",
  "swi_foot",
  "swi_ston",
  "swi_wtr1",
  "tr_fire",
  "m_skull5",
  "spi_atak",
  "anu_hit",
  "fishdies", // 40
  "scrp_icu",
  "jon_wade",
  "amb_watr",
  "tele_1",
  "wasp_stg",
  "res",
  "drum4",
  "rex_icu",
  "m_hits_u",
  "q_tail", // 50
  "vatr_mov",
  "jon_hit3",
  "jon_t_2", // 53
  "jon_t_1",
  "jon_t_5",
  "jon_t_6",
  "jon_t_8",
  "jon_t_4",
  "rasprit1",
  "jon_fdie", // 60
  "wijaf1",
  "ship_1",
  "saw_on",
  "ra_on",
  "amb_ston", // 65
  "vatr_stp", // 66
  "mana1",
  "mana2",
  "ammo",
  "pot_pc1", // 70?
  "pot_pc2",
  "weapon",
  "alarm",
  "tick1",
  "scrp_zap", // 75
  "jon_t_3",
  "jon_laf1",
  "blasted",
  "jon_air2" // 79
};

short nAmbientChannel = -1;

short nStopSound;
short nStoneSound;
short nSwitchSound;
short nLocalEyeSect;
short nElevSound;
short nCreepyTimer;

bool looped[kMaxSoundFiles];

struct ActiveSound
{
    short snd_sprite;
    short snd_id;
    short snd_volume;
    short snd_angle;
    short snd_ambientflag;
    short snd_priority;
    int snd_handle;
    FSoundChan* snd_channel;
    int snd_pitch;
    int snd_time;
    int snd_x;
    int snd_y;
    int snd_z;
    short snd_sector;
};

ActiveSound sActiveSound[kMaxSounds];
short StaticSound[kMaxSoundFiles];
int fakesources[] = { 0, 1, 2, 3 };
int nLocalChan = 0;

enum
{
    nSwirlyChan1 = 1,
    nSwirlyChan2,
    nSwirlyChan3,
    nSwirlyChan4,
};

void SetLocalChan(int c)
{
    nLocalChan = c;
}

//==========================================================================
//
//
//
//==========================================================================

class EXSoundEngine : public SoundEngine
{
    // client specific parts of the sound engine go in this class.
    void CalcPosVel(int type, const void* source, const float pt[3], int channum, int chanflags, FSoundID chanSound, FVector3* pos, FVector3* vel, FSoundChan* chan) override;
    TArray<uint8_t> ReadSound(int lumpnum) override;
    void ChannelEnded(FISoundChannel* chan) override
    {
        for (auto& inf : sActiveSound)
        {
            if (inf.snd_channel == chan) inf.snd_channel = nullptr;
        }
    }

public:
    EXSoundEngine()
    {
        int eax = 260;
        TArray<uint8_t> disttable(256, 1);

        for (int i = 0; i < 256; i++)
        {
            if (eax > 65280)
            {
                disttable[i] = 0;
            }
            else
            {
                disttable[i] = eax >> 8;

                eax = (eax * eax) >> 8;
            }
        }

        S_Rolloff.RolloffType = ROLLOFF_Custom;
        S_Rolloff.MinDistance = 0;
        S_Rolloff.MaxDistance = 4096; // It's really this big
        Init(disttable, 255);
    }
};

//==========================================================================
//
//
// 
//==========================================================================

TArray<uint8_t> EXSoundEngine::ReadSound(int lumpnum)
{
    auto wlump = fileSystem.OpenFileReader(lumpnum);
    return wlump.Read();
}


//==========================================================================
//
//
//
//==========================================================================

void InitFX(void)
{
    if (soundEngine) return; // just in case.
    soundEngine = new EXSoundEngine;

    auto& S_sfx = soundEngine->GetSounds();
    S_sfx.Resize(kMaxSoundFiles+1);
    for (auto& sfx : S_sfx) { sfx.Clear(); sfx.lumpnum = sfx_empty; }
    for (size_t i = 0; i < kMaxSoundFiles; i++)
    {
        FStringf filename("%s.voc", SoundFiles[i]);
        auto lump = fileSystem.FindFile(filename);
        if (lump > 0)
        {
            auto check = fileSystem.GetFileData(lump);
            if (check.Size() > 26 && check[26] == 6 && !memcmp("Creative Voice File", check.Data(), 19))
            {
                // This game uses the actual loop point information in the sound data as its only means to check if a sound is looped.
                looped[i] = true;
            }
            auto& newsfx = S_sfx[i];
            newsfx.name = SoundFiles[i];
            newsfx.lumpnum = lump;
            newsfx.NearLimit = 6;
            newsfx.bTentative = false;
        }
        else if (!ISDEMOVER)  // demo tries to load sound files it doesn't have
            Printf("Unable to open sound '%s'!\n", filename.GetChars());
        StaticSound[i] = i;
    }
    soundEngine->HashSounds();
    for (auto& sfx : S_sfx)
    {
        soundEngine->CacheSound(&sfx);
    }

    memset(sActiveSound, 255, sizeof(sActiveSound));
    for (int i = 0; i < kMaxSounds; i++)
    {
        sActiveSound[i].snd_channel = nullptr;
        sActiveSound[i].snd_handle = -1;
    }

    nCreepyTimer = kCreepyCount;
}


//==========================================================================
//
//
//
//==========================================================================

void GetSpriteSoundPitch(short nSector, int* pVolume, int* pPitch, int nLocalSectFlags)
{
    if (nSector < 0)
        return;
    if ((SectFlag[nSector] ^ nLocalSectFlags) & kSectUnderwater)
    {
        *pVolume >>= 1;
        *pPitch -= 1200;
    }
}

//==========================================================================
//
//
//
//==========================================================================

void BendAmbientSound(void)
{
    if (nAmbientChannel < 0)
        return;
    ActiveSound* pASound = &sActiveSound[nAmbientChannel];
    if (pASound->snd_channel)
    {
        soundEngine->SetPitch(pASound->snd_channel, (nDronePitch + 11800) / 11025.f);
    }
}

//==========================================================================
//
//
//
//==========================================================================

void PlayLocalSound(short nSound, short nRate)
{
    if (nSound < 0 || nSound >= kMaxSounds || !soundEngine->isValidSoundId(nSound + 1))
    {
        initprintf("PlayLocalSound: Invalid sound nSound == %i, nRate == %i\n", nSound, nRate);
        return;
    }

    if (nLocalChan == nAmbientChannel)
        nAmbientChannel = -1;

    int bLoop = looped[nSound];

    ActiveSound* pASound = &sActiveSound[nLocalChan];
    if (pASound->snd_channel != nullptr)
        soundEngine->StopChannel(pASound->snd_channel);

    // There is exactly one occurence in the entire game which alters the pitch.
    pASound->snd_channel = soundEngine->StartSound(SOURCE_Unattached, nullptr, nullptr, CHAN_BODY, CHANF_OVERLAP, nSound + 1, 1.f, ATTN_NONE, nullptr);

    if (nRate)
    {
        float ratefac = (11025 + nRate) / 11025.f;
        soundEngine->SetPitch(pASound->snd_channel, ratefac);
    }

    pASound->snd_id = nSound;
}

//==========================================================================
//
//
//
//==========================================================================

int LocalSoundPlaying(void)
{
    return sActiveSound[nLocalChan].snd_channel != nullptr;
}

int GetLocalSound(void)
{
    if (LocalSoundPlaying() == -1)
        return -1;

    return sActiveSound[nLocalChan].snd_id & 0x1ff;
}

//==========================================================================
//
//
//
//==========================================================================

void StopLocalSound(void)
{
    if (nLocalChan == nAmbientChannel)
        nAmbientChannel = -1;

    if (LocalSoundPlaying())
    {
        soundEngine->StopChannel(sActiveSound[nLocalChan].snd_channel);
        sActiveSound[nLocalChan].snd_channel = nullptr;
    }
}

//==========================================================================
//
//
//
//==========================================================================
int nNextFreq;
int nSwirlyFrames;

void StartSwirly(int nActiveSound)
{
    ActiveSound* pASound = &sActiveSound[nActiveSound];
    pASound->snd_angle = rand() & 0x7ff;

    short nPitch = nNextFreq - RandomSize(9);
    pASound->snd_pitch = nPitch;
    nNextFreq = 25000 - RandomSize(10) * 6;
    if (nNextFreq > 32000)
        nNextFreq = 32000;

    int nVolume = nSwirlyFrames + 1;
    if (nVolume >= 220)
        nVolume = 220;

    pASound->snd_volume = nVolume;
    if (pASound->snd_channel) soundEngine->StopChannel(pASound->snd_channel);

    pASound->snd_channel = soundEngine->StartSound(SOURCE_Swirly, &fakesources[nActiveSound-1], nullptr, CHAN_BODY, 0, kSound67, nVolume / 255.f, ATTN_NONE, nullptr, nPitch / 11025.f);
}

//==========================================================================
//
//
//
//==========================================================================

void StartSwirlies()
{
    StopAllSounds();

    nNextFreq = 19000;
    nSwirlyFrames = 0;

    for (int i = nSwirlyChan1; i <= nSwirlyChan4; i++)
        StartSwirly(i);
}

//==========================================================================
//
//
//
//==========================================================================

void UpdateSwirlies()
{
    nSwirlyFrames++;
    for (int i = nSwirlyChan1; i <= nSwirlyChan4; i++)
    {
        ActiveSound* pASound = &sActiveSound[i];

        if (pASound->snd_channel == nullptr)
            StartSwirly(i);
    }
}

//==========================================================================
//
//
//
//==========================================================================

void SoundBigEntrance(void)
{
    StopAllSounds();
    ActiveSound* pASound = sActiveSound+1;
    for (int i = 0; i < 4; i++, pASound++)
    {
        short nPitch = i * 512 - 1200;
        pASound->snd_pitch = nPitch;
        if (pASound->snd_channel) soundEngine->StopChannel(pASound->snd_channel);
        pASound->snd_channel = soundEngine->StartSound(SOURCE_EXBoss, &fakesources[i], nullptr, CHAN_BODY, 0, kSoundTorchOn, 200 / 255.f, ATTN_NONE, nullptr, nPitch / 11025.f);
    }
}


//==========================================================================
//
//
//
//==========================================================================

void EXSoundEngine::CalcPosVel(int type, const void* source, const float pt[3], int channum, int chanflags, FSoundID chanSound, FVector3* pos, FVector3* vel, FSoundChan* chan)
{
    if (pos != nullptr)
    {
        vec3_t campos;
        if (nSnakeCam > -1)
        {
            Snake* pSnake = &SnakeList[nSnakeCam];
            spritetype* pSnakeSprite = &sprite[pSnake->nSprites[0]];
            campos.x = pSnakeSprite->x;
            campos.y = pSnakeSprite->y;
            campos.z = pSnakeSprite->z;
        }
        else
        {
            campos = { initx, inity, initz };
        }
        auto fcampos = GetSoundPos(&campos);

        if (type == SOURCE_Unattached)
        {
            pos->X = pt[0];
            pos->Y = pt[1];
            pos->Z = pt[2];
        }
        // Do some angular magic. The original was just 2D panning which in a 3D sound field is not sufficient.
        else if (type == SOURCE_Swirly)
        {
            int which = *(int*)source;
            float phase = ((int)totalclock << (4 + which)) * (M_PI / 1024);
            pos->X = fcampos.X + 256 * cos(phase);
            pos->Y = fcampos.Y + 256 * sin(phase);
        }
        else  if (type == SOURCE_EXBoss)
        {
            int which = *(int*)source;
            *pos = fcampos;
            // Should be positioned in 90° intervals.
            switch (which)
            {
            default:
            case 0: pos->X -= 256; break;
            case 1: pos->Y -= 256; break;
            case 2: pos->X += 256; break;
            case 3: pos->Y += 256; break;
            }
        }
        else if (type == SOURCE_Actor)
        {
            auto actor = (spritetype*)source;
            assert(actor != nullptr);
            if (actor != nullptr)
            {
                *pos = GetSoundPos(&actor->pos);
            }
        }
        if ((chanflags & CHANF_LISTENERZ) && type != SOURCE_None)
        {
            pos->Y = fcampos.Z;
        }
    }
}

//==========================================================================
//
//
//
//==========================================================================

int GetDistFromDXDY(int dx, int dy)
{
    int nSqr = dx*dx+dy*dy;
    return (nSqr>>3)-(nSqr>>5);
}

void UpdateSounds()
{
    if (nFreeze)
        return;

    int nLocalSectFlags = SectFlag[nPlayerViewSect[nLocalPlayer]];

    int x, y;
    short ang;
    if (nSnakeCam > -1)
    {
        Snake *pSnake = &SnakeList[nSnakeCam];
        spritetype *pSnakeSprite = &sprite[pSnake->nSprites[0]];
        x = pSnakeSprite->x;
        y = pSnakeSprite->y;
        ang = pSnakeSprite->ang;
    }
    else
    {
        x = initx;
        y = inity;
        ang = inita;
    }
    ActiveSound* pASound = sActiveSound;
    pASound++;
    for (int i = 1; i < kMaxActiveSounds; i++, pASound++)
    {
        if (pASound->snd_channel != nullptr)
        {
            short nSoundSprite = pASound->snd_sprite;
            int nPitch = pASound->snd_pitch;
            short nSoundSect;
            if (nSoundSprite >= 0)
            {
                if (nSoundSprite == nLocalSpr)
                    nSoundSect = nPlayerViewSect[nLocalPlayer];
                else
                    nSoundSect = sprite[nSoundSprite].sectnum;
            }
            else
                nSoundSect = pASound->snd_sector;

            int nVolume = pASound->snd_volume;
            GetSpriteSoundPitch(nSoundSect, &nVolume, &nPitch, nLocalSectFlags);
            soundEngine->SetPitch(pASound->snd_channel, (11025 + nPitch) / 11025.f);
            soundEngine->SetVolume(pASound->snd_channel, nVolume / 255.f);
        }
    }
}

//==========================================================================
//
//
//
//==========================================================================

int soundx, soundy, soundz;
short soundsect;

short PlayFX2(unsigned short nSound, short nSprite)
{
    if ((nSound&0x1ff) >= kMaxSounds || !soundEngine->isValidSoundId(nSound+1))
    {
        initprintf("PlayFX2: Invalid sound nSound == %i, nSprite == %i\n", nSound, nSprite);
        return -1;
    }

    int nLocalSectFlags = SectFlag[nPlayerViewSect[nLocalPlayer]];
    short v1c;
    short vcx;
    if (nSprite < 0)
    {
        vcx = 0;
        v1c = 0;
    }
    else
    {
        v1c = nSprite&0x2000;
        vcx = nSprite&0x4000;
        nSprite &= 0xfff;
        soundx = sprite[nSprite].x;
        soundy = sprite[nSprite].y;
        soundz = sprite[nSprite].z;
    }
    int dx, dy;

    dx = initx-soundx;
    dy = inity-soundy;

    dx >>= 8; dy >>= 8;

    short nSoundAng;
    if ((dx|dy) == 0)
        nSoundAng = 0;
    else
        nSoundAng = AngleDelta(GetMyAngle(dx, dy), inita, 1024);

    int nDist = GetDistFromDXDY(dx, dy);
    if (nDist >= 255)
    {
        if ((int16_t)nSound > -1)
            StopSpriteSound(nSound);
        return -1;
    }

    int nVolume;

    if (!v1c)
    {
        nVolume = snd_fxvolume+10-(Sin(nDist<<1)>>6)-10;
        if (nVolume <= 0)
        {
            if ((int16_t)nSound > -1)
                StopSpriteSound(nSound);
            return -1;
        }
        if (nVolume > 255)
            nVolume = 255;
    }
    else
        nVolume = snd_fxvolume;

    short vc = nSound & (~0x1ff);
    short v4 = nSound & 0x2000;
    short v8 = nSound & 0x1000;
    short v14 = nSound & 0x4000;
    short v10 = (nSound&0xe00)>>9;
    int v2c = 0x7fffffff;
    ActiveSound* v38 = NULL;
    ActiveSound* v28 = NULL;
    ActiveSound* vdi = NULL;
    nSound &= 0x1ff;

    short priority;

    if (v8 || v14)
        priority = 1000;
    else if (nSprite != -1 && vcx)
        priority = 2000;
    else
        priority = 0;
    ActiveSound* pASound = sActiveSound;
    pASound++;
    for (int i = 1; i < kMaxActiveSounds; i++, pASound++)
    {
        if (pASound->snd_channel == nullptr)
        {
            vdi = pASound;
        }
        else if (priority >= pASound->snd_priority)
        {
            if (v2c > pASound->snd_time && pASound->snd_priority <= priority)
            {
                v28 = pASound;
                v2c = pASound->snd_time;
            }
            if (!v8)
            {
                if (nSound == pASound->snd_id)
                {
                    if (v4 == 0 && nSprite == pASound->snd_sprite)
                        return -1;
                    if (priority >= pASound->snd_priority)
                        v38 = pASound;
                }
                else if (nSprite == pASound->snd_sprite)
                {
                    if (v4 || nSound != pASound->snd_id)
                    {
                        vdi = pASound;
                        break;
                    }
                }
            }
        }
    }

    if (!vdi)
    {
        if (v38)
            vdi = v38;
        else if (v28)
            vdi = v28;
    }

    if (vdi->snd_channel != nullptr)
    {
        soundEngine->StopChannel(vdi->snd_channel);
        vdi->snd_channel = nullptr;
        if (short(vdi - sActiveSound) == nAmbientChannel) // f_2c was never set to anything other than 0.
            nAmbientChannel = -1;
    }

    int nPitch;
    if (v10)
        nPitch = -(totalmoves&((1<<v10)-1))*16;
    else
        nPitch = 0;

    if (vdi)
    {
        vdi->snd_pitch = nPitch;
        if (nSprite < 0)
        {
            vdi->snd_x = soundx;
            vdi->snd_y = soundy;
            vdi->snd_sector = soundsect;
        }
        GetSpriteSoundPitch(soundsect, &nVolume, &nPitch, nLocalSectFlags);
        vdi->snd_volume = nVolume;
        vdi->snd_angle = nSoundAng;
        vdi->snd_sprite = nSprite;
        vdi->snd_id = nSound;
        vdi->snd_time = (int)totalclock;
        vdi->snd_priority = priority;
        vdi->snd_ambientflag = vc;

        int bLoop = looped[nSound];

        if (nSprite)
        {
            vdi->snd_channel = soundEngine->StartSound(SOURCE_Actor, &sprite[nSprite], nullptr, CHAN_BODY, CHANF_OVERLAP, nSound+1, nVolume / 255.f, ATTN_NORM, nullptr, (11025 + nPitch) / 11025.f);
        }
        else
        {
            vec3_t v = { soundx, soundy, soundz };
            FVector3 vv = GetSoundPos(&v);
            vdi->snd_channel = soundEngine->StartSound(SOURCE_Unattached, nullptr, &vv, CHAN_BODY, CHANF_OVERLAP, nSound+1, nVolume / 255.f, ATTN_NORM, nullptr, (11025 + nPitch) / 11025.f);
        }

        if (v14)
            nAmbientChannel = v14;

        // Nuke: added nSprite >= 0 check
        if (nSprite != nLocalSpr && nSprite >= 0 && (sprite[nSprite].cstat&257))
            nCreepyTimer = kCreepyCount;

        return v14;
    }
    return -1;
}

//==========================================================================
//
//
//
//==========================================================================

short PlayFXAtXYZ(unsigned short ax, int x, int y, int z, int nSector)
{
    soundx = x;
    soundy = y;
    soundz = z;
    soundsect = nSector&0x3fff;
    short nSnd = PlayFX2(ax, -1);
    if (nSnd > -1 && (nSector&0x4000))
        sActiveSound[nSnd].snd_priority = 2000;
    return nSnd;
}

//==========================================================================
//
//
//
//==========================================================================

void CheckAmbience(short nSector)
{
    if (SectSound[nSector] != -1)
    {
        short nSector2 = SectSoundSect[nSector];
        walltype* pWall = &wall[sector[nSector2].wallptr];
        if (nAmbientChannel < 0)
        {
            PlayFXAtXYZ(SectSound[nSector] | 0x4000, pWall->x, pWall->y, sector[nSector2].floorz, nSector);
            return;
        }
        ActiveSound* pASound = &sActiveSound[nAmbientChannel];
        if (nSector == nSector2)
        {
            spritetype* pSprite = &sprite[PlayerList[0].nSprite];
            pASound->snd_x = pSprite->x;
            pASound->snd_y = pSprite->y;
            pASound->snd_z = pSprite->z;
        }
        else
        {
            pASound->snd_x = pWall->x;
            pASound->snd_y = pWall->y;
            pASound->snd_z = sector[nSector2].floorz;
        }
    }
    else if (nAmbientChannel != -1)
    {
        if (sActiveSound[nAmbientChannel].snd_channel)
            soundEngine->StopChannel(sActiveSound[nAmbientChannel].snd_channel);
        sActiveSound[nAmbientChannel].snd_channel = nullptr;
        nAmbientChannel = -1;
    }
}


//==========================================================================
//
//
//
//==========================================================================

void UpdateCreepySounds()
{
    if (levelnum == 20 || nFreeze)
        return;
    spritetype* pSprite = &sprite[PlayerList[nLocalPlayer].nSprite];
    nCreepyTimer--;
    if (nCreepyTimer <= 0)
    {
        if (nCreaturesLeft > 0 && !(SectFlag[nPlayerViewSect[nLocalPlayer]] & 0x2000))
        {
            int vsi = seq_GetFrameSound(SeqOffsets[kSeqCreepy], totalmoves % SeqSize[SeqOffsets[kSeqCreepy]]);
            if (vsi >= 0 && (vsi & 0x1ff) < kMaxSounds)
            {
                int vdx = (totalmoves + 32) & 31;
                if (totalmoves & 1)
                    vdx = -vdx;
                int vax = (totalmoves + 32) & 63;
                if (totalmoves & 2)
                    vax = -vax;

                PlayFXAtXYZ(vsi, pSprite->x + vdx, pSprite->y + vax, pSprite->z, pSprite->sectnum);
            }
        }
        nCreepyTimer = kCreepyCount;
    }
}


//==========================================================================
//
//
//
//==========================================================================

short D3PlayFX(unsigned short nSound, short nVal)
{
    return PlayFX2(nSound, nVal);
}

void StopSpriteSound(short nSprite)
{
    for (int i = 0; i < kMaxActiveSounds; i++)
    {
        if (sActiveSound[i].snd_channel != nullptr && nSprite == sActiveSound[i].snd_sprite)
        {
            soundEngine->StopChannel(sActiveSound[i].snd_channel);
            sActiveSound[i].snd_channel = nullptr;
            return;
        }
    }
}

void StopAllSounds(void)
{
    soundEngine->StopAllChannels();
    for (int i = 0; i < kMaxActiveSounds; i++)
    {
        sActiveSound[i].snd_channel = nullptr;
    }

    nAmbientChannel = -1;
}

//==========================================================================
//
//
//
//==========================================================================

void PlayTitleSound(void)
{
    PlayLocalSound(StaticSound[kSound10], 0);
}

void PlayLogoSound(void)
{
    PlayLocalSound(StaticSound[kSound28], 7000);
}

void PlayGameOverSound(void)
{
    PlayLocalSound(StaticSound[kSound28], 0);
}

int LoadSound(const char* name)
{
    return soundEngine->FindSound(name) - 1;
}

END_PS_NS
