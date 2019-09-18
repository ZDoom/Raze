#include "compat.h"
#include "baselayer.h"
#include "build.h"
#include "cache1d.h"
#include "engine.h"
#include "exhumed.h"
#include "sound.h"
#include "init.h"
#include "object.h"
#include "player.h"
#include "random.h"
#include "snake.h"
#include "trigdat.h"
#include "sequence.h"

#if 0
extern "C" {
#include "usrhooks.h"
#include "music.h"
#include "multivoc.h"
#include "fx_man.h"
#include "midi.h"
#include "mpu401.h"
}
#endif

short gMusicVolume = 200;
short gFXVolume = 200;
short nSoundsPlaying = 0;
short nAmbientChannel = -1;

short nStopSound;
short nStoneSound;
short nSwitchSound;

short nLocalEyeSect;
short nElevSound;
short nCreepyTimer;

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

short StaticSound[kMaxSoundFiles];

// TODO - temp location. Needs to exist within config file
//
// Sound variables
//
//int FXDevice;
//int MusicDevice;
//int FXVolume;
//int MusicVolume;
//int SoundToggle;
//int MusicToggle;
//int VoiceToggle;
//int AmbienceToggle;
//fx_blaster_config BlasterConfig;
//int NumVoices;
//int NumChannels;
//int NumBits;
//int MixRate;
//int32 MidiPort;
//int ReverseStereo;

int nNextFreq;
int nTotalSoundBytes;
int nSoundCount;
short nSwirlyFrames;

short nDistTable[256];

struct ActiveSound
{
/*
    short _0
    short _2
    short _4
    short _6
    byte _7

    short _10
    short _12
    int _14
    int _18
    int _22;
    int _26

    int _30; // x val
    int _34; // y val
    int _38; // z val
    short _42
    short _44
*/
    short f_0;
    short f_2;
    short f_4;
    short f_6;
    short f_8;
    short f_a;
    short f_c;
    int f_e;
    int f_12;
    int f_16;
    int f_1a;
    int f_1e;
    int f_22;
    int f_26;
    short f_2a;
    short f_2c;
};

ActiveSound sActiveSound[kMaxSounds];

int AIL_allocate_sample_handle(int);
int AIL_allocate_sequence_handle(int);
void AIL_startup(void);
void AIL_set_preference(int, int);
int AIL_install_DIG_INI(void);
int AIL_install_MDI_INI(void);
void AIL_shutdown(void);
void AIL_end_sequence(int);
void AIL_set_sequence_volume(int, int, int);
void AIL_init_sequence(int, char*, int);
void AIL_start_sequence(int);
void AIL_set_sequence_loop_count(int, int);
void AIL_set_sequence_volume(int, int, int);
void AIL_set_sample_playback_rate(int, int);
void AIL_end_sample(int);
int AIL_sample_status(int);
void AIL_set_sample_volume(int, int);
void AIL_set_sample_pan(int, int);
void AIL_init_sample(int);
void AIL_set_sample_file(int, char*, int);
void AIL_start_sample(int);
void AIL_set_sample_loop_count(int, int);
int AIL_sample_playback_rate(int);
char AIL_error[256];
char pMusicBuf[45000];
char szSoundName[kMaxSounds][kMaxSoundNameLen];
char *SoundBuf[kMaxSounds];
char SoundLock[kMaxSounds];

extern char message_text[80];
extern short message_timer;

int S, dig, mdi, handle;
int nActiveSounds;

short nLocalSectFlags;
short nLocalChan;


void CreateDistTable(void)
{
    int eax = 260;

    for (int i = 0; i < 256; i++)
    {
        if (eax > 65280)
        {
            nDistTable[i] = 255;
        }
        else
        {
            nDistTable[i] = eax >> 8;

            eax = (eax * eax) >> 8;
        }
    }
}

void InitSoundInfo(void)
{
    int i;
    memset(sActiveSound, 255, sizeof(sActiveSound));
    for (i = 0; i < kMaxSounds; i++)
    {
        sActiveSound[i].f_2c = 0;
        sActiveSound[i].f_e = AIL_allocate_sample_handle(dig);
    }
    nActiveSounds = i;

    handle = AIL_allocate_sample_handle(dig);
}

void InitFX(void)
{
    AIL_startup();
    if (bNoSound)
    {
        dig = 0;
        return;
    }
    CreateDistTable();

    AIL_set_preference(1, 1);
    AIL_set_preference(8, 0);
    AIL_set_preference(7, 1);

    dig = AIL_install_DIG_INI();
    if (!dig)
    {
        DebugOut("Error initializing sound card!\n");
        initprintf("Error initializing sound card!\n");
        DebugOut("ERROR: %s\n", AIL_error);
        return;
    }
    InitSoundInfo();

    nTotalSoundBytes = 0;
    nSoundCount = 0;
    nCreepyTimer = 450;

#if 0
    int status = FX_Init(FXDevice, NumVoices, NumChannels, NumBits, MixRate);
    if (status == FX_Ok)
    {
        FX_SetVolume(FXVolume);

        if (ReverseStereo == 1) {
            FX_SetReverseStereo(!FX_GetReverseStereo());
        }
    }

    if (status != FX_Ok) {
//		Error("Sound startup error: %s", FX_ErrorString(FX_Error));
    }

    status = FX_SetCallBack(testcallback);

    if (status != FX_Ok) {
//		Error("Sound startup error: %s", FX_ErrorString(FX_Error));
    }
#endif
}

void UnInitFX()
{
    AIL_shutdown();
}

#if 0
void InitMusic()
{
    if (bNoSound)
    {
        mdi = 0;
        return;
    }

    mdi = AIL_install_MDI_INI();
    if (!mdi)
    {
        initprintf("AIL_install midi failed\n");
        return;
    }
    S = AIL_allocate_sequence_handle(mdi);
    if (!S)
    {
        initprintf("Midi handle failed");
        return;
    }
}

void UnInitMusic()
{
    if (mdi)
        AIL_end_sequence(S);
}

void FadeSong()
{
    AIL_set_sequence_volume(S, 0, 1000);
}

int LoadSong(const char *song)
{
    FILE *f;
    if (!mdi)
        return 0;

    AIL_end_sequence(S);
    f = fopen(song, "rb");
    if (!f)
    {
        sprintf(message_text, "Unable to midi open file '%s'!\n", song);
        message_timer = 1000;
        return 0;
    }

    if (fread(pMusicBuf, sizeof(pMusicBuf), 1, f) == sizeof(pMusicBuf))
    {
        sprintf(message_text, "Programmer error: not enough memory allocated for song file.\n");
        message_timer = 1000;
        fclose(f);
        return 0;
    }
    fclose(f);

    AIL_init_sequence(S, pMusicBuf, 0);
    AIL_start_sequence(S);
    AIL_set_sequence_loop_count(S, 0);
    AIL_set_sequence_volume(S, gMusicVolume>>1, 0);
}
#else
void InitMusic()
{
}

void UnInitMusic()
{
}

void FadeSong()
{
}

int LoadSong(const char *song)
{
    UNREFERENCED_PARAMETER(song);
    return 0;
}
#endif

int LoadSound(const char *sound)
{
    if (!dig)
        //return 0;
        return -1;
    int i;
    for (i = 0; i < nSoundCount; i++)
    {
        if (!strnicmp(szSoundName[i], sound, kMaxSoundNameLen))
            return i;
    }

    if (i >= kMaxSounds)
        bail2dos("Too many sounds being loaded... increase array size\n");

    strncpy(szSoundName[i], sound, kMaxSoundNameLen);

    char buffer[32];
    buffer[0] = 0;

    strncat(buffer, szSoundName[i], kMaxSoundNameLen);

    char *pzTail = buffer+strlen(buffer);
    while (*pzTail == ' ' && pzTail != buffer)
        *pzTail-- = '\0';

    strcat(buffer, ".voc");

    buildvfs_kfd hVoc = kopen4loadfrommod(buffer, 0);

    if (hVoc != buildvfs_kfd_invalid)
    {
        int nSize = kfilelength(hVoc);
        SoundLock[i] = 255; // TODO: implement cache lock properly
        cacheAllocateBlock((intptr_t*)&SoundBuf[i], nSize, &SoundLock[i]);
        if (!SoundBuf[i])
            bail2dos("Error allocating buf '%s' to %lld  (size=%ld)!\n", buffer, (intptr_t)&SoundBuf[i], nSize);

        if (kread(hVoc, SoundBuf[i], nSize) != nSize)
            bail2dos("Error reading '%s'!\n", buffer);
    }
    else
    {
        bail2dos("Unable to open sound '%s'!\n", buffer);
        SoundBuf[i] = NULL;
        //return hVoc;
        return -1;
    }
    kclose(hVoc);
    nSoundCount++;
    return i;
}

#if 0
void FreeSounds(void)
{
    int i;
    for (i = 0; i < nSoundCount; i++)
        free(SoundBuf[i]);
}
#endif

void LoadFX(void)
{
    if (!dig)
        return;

    int i;
    for (i = 0; i < kMaxSoundFiles; i++)
    {
        StaticSound[i] = LoadSound(SoundFiles[i]);
        if (StaticSound[i] < 0)
            bail2dos("error loading static sound #%d ('%.8s')\n", i, SoundFiles[i]);
    }
}

void GetSpriteSoundPitch(short nSector, int *pVolume, int *pPitch)
{
    if (nSector < 0)
        return;
    if ((SectFlag[nSector]^nLocalSectFlags) & kSectUnderwater)
    {
        *pVolume >>= 1;
        *pPitch -= 1200;
    }
}

void BendAmbientSound(void)
{
    if (nAmbientChannel < 0)
        return;
    ActiveSound *pASound = &sActiveSound[nAmbientChannel];
    AIL_set_sample_playback_rate(pASound->f_e, nDronePitch+11000);
}

short PlayFXAtXYZ(unsigned short ax, int x, int y, int z, int nSector);

void CheckAmbience(short nSector)
{
    if (SectSound[nSector] != -1)
    {
        short nSector2 = SectSoundSect[nSector];
        walltype *pWall = &wall[sector[nSector2].wallptr];
        if (nAmbientChannel < 0)
        {
            PlayFXAtXYZ(SectSound[nSector]|0x4000, pWall->x, pWall->y, sector[nSector2].floorz, nSector);
            return;
        }
        ActiveSound *pASound = &sActiveSound[nAmbientChannel];
        if (nSector == nSector2)
        {
            spritetype *pSprite = &sprite[PlayerList[0].nSprite];
            pASound->f_1e = pSprite->x;
            pASound->f_22 = pSprite->y;
            pASound->f_26 = pSprite->z;
        }
        else
        {
            pASound->f_1e = pWall->x;
            pASound->f_22 = pWall->y;
            pASound->f_26 = sector[nSector2].floorz;
        }
    }
    else if (nAmbientChannel != -1)
    {
        AIL_end_sample(sActiveSound[nAmbientChannel].f_e);
        nAmbientChannel = -1;
    }
}

int GetDistFromDXDY(int dx, int dy)
{
    int nSqr = dx*dx+dy*dy;
    return (nSqr>>3)-(nSqr>>5);
}

void MuteSounds(void)
{
    if (!dig)
        return;

    int i;
    ActiveSound *pASound = sActiveSound;
    pASound++;
    for (i = 1; i < kMaxActiveSounds; i++, pASound++)
    {
        if (AIL_sample_status(pASound->f_e) == 4)
            AIL_set_sample_volume(pASound->f_e, 0);
    }
}

void SetChanPan(int nSound, int nPan)
{
    AIL_set_sample_pan(sActiveSound[nSound].f_e, nPan);
}

void SoundBigEntrance(void)
{
    StopAllSounds();
    ActiveSound *pASound = sActiveSound;
    for (int i = 0; i < 4; i++, pASound++)
    {
        AIL_init_sample(pASound->f_e);
        short nPitch = i*512-1200;
        pASound->f_16 = nPitch;
        AIL_set_sample_playback_rate(pASound->f_e, 11000+nPitch);
        AIL_set_sample_file(pASound->f_e, SoundBuf[12], -1);
        AIL_set_sample_volume(pASound->f_e, 200);
        AIL_set_sample_pan(pASound->f_e, 63-(i&1)*127);
        AIL_start_sample(pASound->f_e);
    }
}

void StartSwirly(int nActiveSound)
{
    ActiveSound *pASound = &sActiveSound[nActiveSound];
    pASound->f_6 &= 0x7ff;

    AIL_init_sample(pASound->f_e);
    short nPitch = nNextFreq-RandomSize(9);
    pASound->f_16 = nPitch;
    nNextFreq = 25000-RandomSize(10)*6;
    if (nNextFreq > 32000)
        nNextFreq = 32000;

    AIL_set_sample_file(pASound->f_e, SoundBuf[StaticSound[kSound67]], -1);

    int nVolume = nSwirlyFrames+1;
    if (nVolume >= 220)
        nVolume = 220;

    AIL_set_sample_volume(pASound->f_e, nVolume);
    AIL_set_sample_playback_rate(pASound->f_e, nPitch);
    AIL_set_sample_loop_count(pASound->f_e, 1);
    AIL_set_sample_pan(pASound->f_e, pASound->f_6);
    AIL_start_sample(pASound->f_e);
}

void StartSwirlies()
{
    StopAllSounds();

    nNextFreq = 19000;
    nSwirlyFrames = 0;

    for (int i = 1; i <= 4; i++)
        StartSwirly(i);
}

void UpdateSwirlies()
{
    nSwirlyFrames++;
    ActiveSound* pASound = sActiveSound;
    pASound++;
    for (int i = 1; i <= 4; i++, pASound++)
    {
        if (AIL_sample_status(pASound->f_e) != 4)
            StartSwirly(i);

        AIL_set_sample_pan(pASound->f_e, 64+(Sin((int)totalclock<<(4+i))>>8));
    }
}

void UpdateSounds()
{
    if (!dig || nFreeze)
        return;

    nLocalSectFlags = SectFlag[nPlayerViewSect[nLocalPlayer]];
    spritetype *pSprite = &sprite[PlayerList[nLocalPlayer].nSprite];
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
        if (AIL_sample_status(pASound->f_e) == 4)
        {
            short nSoundSprite = pASound->f_0;
            int dx, dy;
            if (nSoundSprite < 0)
            {
                dx = x-pASound->f_1e;
                dy = y-pASound->f_22;
            }
            else
            {
                dx = sprite[nSoundSprite].x-pASound->f_1e;
                dy = sprite[nSoundSprite].y-pASound->f_22;
            }

            dx >>= 8;
            dy >>= 8;

            short nSoundAng;
            if ((dx|dy) == 0)
                nSoundAng = 0;
            else
                nSoundAng = AngleDelta(GetMyAngle(dx, dy), ang, 1024);

            int nDist = GetDistFromDXDY(dx, dy);
            if (nDist >= 255)
            {
                AIL_end_sample(pASound->f_e);
                if (pASound->f_a&0x4000)
                    nAmbientChannel = -1;
                return;
            }

            int nVolume = gFXVolume+10-(Sin(nDist<<1)>>6);
            if (nVolume < 0)
                nVolume = 0;
            if (nVolume > 255)
                nVolume = 255;
            
            int nPitch = pASound->f_16;
            short nSoundSect;
            if (nSoundSprite >= 0)
            {
                if (nSoundSprite == nLocalSpr)
                    nSoundSect = nPlayerViewSect[nLocalPlayer];
                else
                    nSoundSect = sprite[nSoundSprite].sectnum;
            }
            else
                nSoundSect = pASound->f_2a;

            GetSpriteSoundPitch(nSoundSect, &nVolume, &nPitch);

            if (pASound->f_12 != nPitch)
                pASound->f_12 = nPitch;

            if (nVolume != pASound->f_4 || nSoundAng != pASound->f_6)
            {
                int nPan = 63+(Sin(nSoundAng&1023)>>8);
                AIL_set_sample_volume(pASound->f_e, nVolume);
                AIL_set_sample_pan(pASound->f_e, nPan);

                if (nPitch < 0)
                    AIL_set_sample_playback_rate(pASound->f_e, 7000);

                pASound->f_4 = nVolume;
                pASound->f_6 = nSoundAng;
            }
        }
    }

    if (!nFreeze && levelnum != 20)
    {
        nCreepyTimer--;
        if (nCreepyTimer <= 0)
        {
            if (nCreaturesLeft > 0 && !(SectFlag[nPlayerViewSect[nLocalPlayer]]&0x2000))
            {
                int vsi = seq_GetFrameSound(SeqOffsets[kSeqCreepy], totalmoves%SeqSize[SeqOffsets[kSeqCreepy]]);
                int vdx = (totalmoves+32)&31;
                if (totalmoves & 1)
                    vdx = -vdx;
                int vax = (totalmoves+32)&63;
                if (totalmoves & 2)
                    vax = -vax;

                PlayFXAtXYZ(vsi, pSprite->x+vdx, pSprite->y+vax, pSprite->z, pSprite->sectnum);
            }
            nCreepyTimer = 450;
        }
    }
}

int LocalSoundPlaying(void)
{
    if (!dig)
        return 0;

    return AIL_sample_status(sActiveSound[nLocalChan].f_e) == 4;
}

int GetLocalSound(void)
{
    if (LocalSoundPlaying() == -1)
        return -1;

    return sActiveSound[nLocalChan].f_2&0x1ff;
}

void UpdateLocalSound(void)
{
    if (!dig)
        return;

    AIL_set_sample_volume(sActiveSound[nLocalChan].f_e, gFXVolume>>1);
}

void StopLocalSound(void)
{
    if (!dig)
        return;

    if (nLocalChan == nAmbientChannel)
        nAmbientChannel = -1;

    if (LocalSoundPlaying())
        AIL_end_sample(sActiveSound[nLocalChan].f_e);
}

void SetLocalChan(int nChannel)
{
    nLocalChan = nChannel;
}

void PlaySound(int nSound)
{
    if (!dig)
        return;

    AIL_init_sample(handle);
    AIL_set_sample_file(handle, SoundBuf[nSound], -1);
    AIL_set_sample_volume(handle, gFXVolume>>1);

    if (SoundBuf[nSound][26] == 6)
        AIL_set_sample_loop_count(handle, 0);

    AIL_start_sample(handle);
}

void PlayLocalSound(int nSound, short nRate)
{
    if (!dig)
        return;

    if (nLocalChan == nAmbientChannel)
        nAmbientChannel = -1;

    ActiveSound* pASound = &sActiveSound[nLocalChan];
    AIL_init_sample(pASound->f_e);
    AIL_set_sample_file(pASound->f_e, SoundBuf[nSound], -1);
    AIL_set_sample_volume(pASound->f_e, gFXVolume>>1);

    if (nRate)
        AIL_set_sample_playback_rate(pASound->f_e, AIL_sample_playback_rate(pASound->f_e)+nRate);

    if (SoundBuf[nSound][26] == 6)
        AIL_set_sample_loop_count(pASound->f_e, 0);

    AIL_start_sample(pASound->f_e);
    pASound->f_2 = nSound;
    SetLocalChan(0);
}

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

int soundx, soundy, soundz;
short soundsect;

short PlayFX2(unsigned short nSound, short nSprite)
{
    if (!dig)
        return -1;

    nLocalSectFlags = SectFlag[nPlayerViewSect[nLocalPlayer]];
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
        if (nSound > -1)
            StopSpriteSound(nSound);
        return -1;
    }

    int nVolume;

    if (!v1c)
    {
        nVolume = gFXVolume+10-(Sin(nDist<<1)>>6)-10;
        if (nVolume <= 0)
        {
            if (nSound > -1)
                StopSpriteSound(nSound);
            return -1;
        }
        if (nVolume > 255)
            nVolume = 255;
    }
    else
        nVolume = 255;

    short vc = nSound & (~0x1ff);
    short v4 = nSound & 0x2000;
    short v8 = nSound & 0x1000;
    short v14 = nSound & 0x4000;
    short v10 = (nSound&0xe000)>>9;
    int v2c = 0x7fffffff;
    ActiveSound* v38 = NULL;
    ActiveSound* v28 = NULL;
    ActiveSound* vdi = NULL;
    nSound &= 0x1ff;

    short v20;

    if (v8 || v14)
        v20 = 1000;
    else if (nSprite != -1 && vcx)
        v20 = 2000;
    else
        v20 = 0;
    ActiveSound* pASound = sActiveSound;
    pASound++;
    for (int i = 1; i < kMaxActiveSounds; i++, pASound++)
    {
        if (AIL_sample_status(pASound->f_e) != 4)
            vdi = pASound;
        else if (v20 >= pASound->f_c)
        {
            if (v2c > pASound->f_1a && pASound->f_c <= v20)
            {
                v28 = pASound;
                v2c = pASound->f_1a;
            }
            if (!v8)
            {
                if (nSound == pASound->f_2)
                {
                    if (v4 == 0 && nSprite == pASound->f_0)
                        return -1;
                    if (v20 >= pASound->f_c)
                        v38 = pASound;
                }
                else if (nSprite == pASound->f_0)
                {
                    if (v4 || nSound != pASound->f_2)
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

    if (AIL_sample_status(vdi->f_e) == 4)
    {
        AIL_end_sample(vdi->f_e);
        if (vdi->f_2c == nAmbientChannel)
            nAmbientChannel = -1;
    }

    int nPitch;
    if (v10)
        nPitch = -(totalmoves&((1<<v10)-1));
    else
        nPitch = 0;

    if (vdi)
    {
        vdi->f_16 = nPitch;
        vdi->f_8 = nVolume;
        if (nSprite < 0)
        {
            vdi->f_1e = soundx;
            vdi->f_22 = soundy;
            vdi->f_2a = soundsect;
        }
        GetSpriteSoundPitch(soundsect, &nVolume, &nPitch);
        vdi->f_12 = nPitch;
        vdi->f_4 = nVolume;
        vdi->f_6 = nSoundAng;
        vdi->f_0 = nSprite;
        vdi->f_2 = nSound;
        vdi->f_1a = (int)totalclock;
        vdi->f_c = v20;
        vdi->f_a = vc;
        short nPan = 63+(Sin(nSoundAng&1023)>>8);

        AIL_init_sample(vdi->f_e);
        AIL_set_sample_file(vdi->f_e, SoundBuf[nSound], -1);
        AIL_set_sample_pan(vdi->f_e, nPan);
        AIL_set_sample_volume(vdi->f_e, nVolume>>1);

        if (SoundBuf[nSound][26] == 6)
            AIL_set_sample_loop_count(pASound->f_e, 0);

        if (nPitch)
            AIL_set_sample_playback_rate(pASound->f_e, AIL_sample_playback_rate(pASound->f_e)+nPitch);

        AIL_start_sample(pASound->f_e);

        if (v14)
            nAmbientChannel = v14;

        // Nuke: added nSprite >= 0 check
        if (nSprite != nLocalSpr && nSprite >= 0 && (sprite[nSprite].cstat&257))
            nCreepyTimer = 450;

        return v14;
    }
    return -1;
}

short PlayFXAtXYZ(unsigned short ax, int x, int y, int z, int nSector)
{
    soundx = x;
    soundy = y;
    soundz = z;
    soundsect = nSector&0x3fff;
    short nSnd = PlayFX2(ax, -1);
    if (nSnd > -1 && (nSector&0x4000))
        sActiveSound[nSnd].f_c = 2000;
    return nSnd;
}

short D3PlayFX(unsigned short nSound, short nVal)
{
    return PlayFX2(nSound, nVal);
}

void StopSpriteSound(short nSprite)
{
    if (!dig)
        return;

    for (int i = 0; i < kMaxActiveSounds; i++)
    {
        if (AIL_sample_status(sActiveSound[i].f_e) == 4 && nSprite == sActiveSound[i].f_0)
        {
            AIL_end_sample(sActiveSound[i].f_e);
            return;
        }
    }
}

void StopAllSounds(void)
{
    if (!dig)
        return;

    for (int i = 0; i < kMaxActiveSounds; i++)
    {
        AIL_end_sample(sActiveSound[i].f_e);
    }

    nSoundsPlaying = 0;
    nAmbientChannel = -1;
}

#if 0
void Lock(void)
{
    AIL_lock();
}

void Unlock(void)
{
    AIL_unlock();
}

#endif
