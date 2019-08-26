
#include "sound.h"
#include "init.h"

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

char *soundFiles[kMaxSoundFiles] =
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
int FXDevice;
int MusicDevice;
int FXVolume;
int MusicVolume;
int SoundToggle;
int MusicToggle;
int VoiceToggle;
int AmbienceToggle;
//fx_blaster_config BlasterConfig;
int NumVoices;
int NumChannels;
int NumBits;
int MixRate;
//int32 MidiPort;
int ReverseStereo;

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
};


void CreateDistTable()
{
	int eax = 260;

	for (int i = 0; i < 256; i++)
	{
		if (eax <= 65280)
		{
			nDistTable[i] = eax >> 8;

			eax = (eax * eax) >> 8;
		}
		else
		{
			nDistTable[i] = 255;
		}
	}
}

void InitSoundInfo()
{

}

int UpdateSounds()
{
	return 1;
}

int LocalSoundPlaying()
{
	return 0;
}

// for ASS only
void testcallback(unsigned long num)
{
#if 0
	short tempi, tempj, tempk;

	if ((long)num < 0)
	{
		if (lumplockbyte[-num] >= 200)
			lumplockbyte[-num]--;
		return;
	}

	tempk = Sound[num].num;

	if (tempk > 0)
	{
		if ((soundm[num] & 16) == 0)
			for (tempj = 0; tempj<tempk; tempj++)
			{
				tempi = SoundOwner[num][tempj].i;
				if (sprite[tempi].picnum == MUSICANDSFX && sector[sprite[tempi].sectnum].lotag < 3 && sprite[tempi].lotag < 999)
				{
					hittype[tempi].temp_data[0] = 0;
					if ((tempj + 1) < tempk)
					{
						SoundOwner[num][tempj].voice = SoundOwner[num][tempk - 1].voice;
						SoundOwner[num][tempj].i = SoundOwner[num][tempk - 1].i;
					}
					break;
				}
			}

		Sound[num].num--;
		SoundOwner[num][tempk - 1].i = -1;
	}

	Sound[num].lock--;
#endif
}

int USRHOOKS_GetMem(void **ptr, unsigned long size)
{
//	return(USRHOOKS_Ok);
	return 0;
}

int USRHOOKS_FreeMem(void *ptr)
{
//	return(USRHOOKS_Ok);
	return 0;
}

void InitFX()
{
#if 0
	AIL_startup();

	if (bNoSound)
	{
		dig = 0;
	}

	CreateDistTable();

	AIL_set_preference(1, 1);
	AIL_set_preference(8, 0);
	AIL_set_preference(7, 1);

	dig = AIL_install_DIG_INI();
	if (!dig)
	{
		DebugOut("Error initializing sound card!\n");
		printf("Error initializing sound card!\n");
		DebugOut("ERROR: %s\n", AIL_error);
		return;
	}
#endif
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

}

void BendAmbientSound()
{

}

void CheckAmbience(short nSector)
{
	if (SectSound[nSector] == -1)
	{
		if (nAmbientChannel == -1) {
			return;
		}


	}
	else
	{

	}
}

int sound_load_file(char *fileName)
{

	return 0;
}


void LoadStaticSounds()
{
//	if (!dword_1B30A0)
//		return;


}

void FadeSong()
{

}

void StopAllSounds()
{

}

void SetLocalChan(int nChannel)
{
}

int GetLocalSound()
{
	return 0; // TODO
}

void UpdateLocalSound()
{

}

void StopLocalSound()
{

}

void PlayLocalSound(short nSound, int val)
{

}

void D3PlayFX(int nSound, int nVal)
{

}

void StopSpriteSound(short nSprite)
{

}

void StartSwirly(int nActiveSound)
{

}

void StartSwirlies()
{
	StopAllSounds();

	nNextFreq = 19000;
	nSwirlyFrames = 0;

	int i = 1;

	do {
		StartSwirly(i);
		i++;
	} while (i <= 4);
}

void UpdateSwirlies()
{

}
