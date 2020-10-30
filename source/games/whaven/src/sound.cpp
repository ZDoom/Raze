#include "ns.h"
#include "wh.h"
#include "raze_sound.h"
#include "g_input.h"

BEGIN_WH_NS

class WHSoundEngine : public SoundEngine
{
	// client specific parts of the sound engine go in this class.
	void CalcPosVel(int type, const void* source, const float pt[3], int channum, int chanflags, FSoundID chanSound, FVector3* pos, FVector3* vel, FSoundChan* channel) override;
	TArray<uint8_t> ReadSound(int lumpnum) override;

public:
	WHSoundEngine()
	{
		S_Rolloff.RolloffType = ROLLOFF_Doom;
		S_Rolloff.MinDistance = 93;
		S_Rolloff.MaxDistance = 1064; // was originally 532, this is a bit low.
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


int ambsoundarray[8];


void sfxInit(void)
{
	soundEngine = new WHSoundEngine;

	// These are not constants!
	ambsoundarray[0] = 0;
	ambsoundarray[1] = S_WINDLOOP1;
	ambsoundarray[2] = S_WINDLOOP2;
	ambsoundarray[3] = S_WAVELOOP1;
	ambsoundarray[4] = S_LAVALOOP1;
	ambsoundarray[5] = S_WATERY;
	ambsoundarray[6] = S_STONELOOP1;
	ambsoundarray[7] = S_BATSLOOP;

	soundEngine->AddSoundLump("", sfx_empty, 0, -1, 6);	// Index 0 is unused
	auto& sfx = soundEngine->GetSounds();

	int count = 0;
	for (int i = 0; i < 1024; i++)
	{
		FStringf check("JOESND/%04d", i);
		int lump = fileSystem.FindFile(check);
		if (lump > 0)
		{
			int index = soundEngine->AddSoundLump(check, lump, 0, -1, 6);
			if (index == S_STONELOOP1) sfx[index].bSingular = true;
			sfx[index].bLoadRAW = true;
			sfx[index].b16bit = false;
			sfx[index].RawRate = 11025;
			soundEngine->CacheSound(&sfx[index]);
			if (((++count) & 31) == 0)
				I_GetEvent();
		}
		else
			soundEngine->AddSoundLump("", sfx_empty, 0, -1, 6);


	}
}

//==========================================================================
//
//
// 
//==========================================================================

TArray<uint8_t> WHSoundEngine::ReadSound(int lumpnum)
{
	auto wlump = fileSystem.OpenFileReader(lumpnum);
	return wlump.Read();
}

void WHSoundEngine::CalcPosVel(int type, const void* source, const float pt[3], int channum, int chanflags, FSoundID chanSound, FVector3* pos, FVector3* vel, FSoundChan*)
{
	if (pos != nullptr && type != SOURCE_None)
	{
		vec3_t ppos = { player[pyrn].x, player[pyrn].y, player[pyrn].z };
		FVector3 camera = GetSoundPos(&ppos);

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
			size_t index = actor - sprite;
			if (vel) *vel = { 0, 0, 0 }; // this game has no velocity management usable here.
			*pos = GetSoundPos(&actor->pos);
		}
		else if (type == SOURCE_Ambient)
		{
			*pos = camera; // just to be safe. Ambient sounds are in the world but unpositioned
		}
		if ((chanflags & CHANF_LISTENERZ))
		{
			pos->Y = camera.Y;
		}
	}
}


void GameInterface::UpdateSounds()
{
	SoundListener listener;

	soundEngine->EnumerateChannels([](FSoundChan* channel)
		{
			if (channel->UserData > 0)
			{
				channel->UserData -= (IntToFixed(TICSPERFRAME) / 360);
				if (channel->UserData < 0)
				{
					soundEngine->StopChannel(channel);
				}
			}
			return false;
		});

	if (player[pyrn].spritenum >= 0)
	{
		listener.angle = -(float)player[pyrn].ang * pi::pi() / 1024; // Build uses a period of 2048.
		listener.velocity.Zero();
		vec3_t ppos{ player[pyrn].x, player[pyrn].y, player[pyrn].z };
		listener.position = GetSoundPos(&ppos);
		listener.valid = true;
	}
	else
	{
		listener.position.Zero();
		listener.valid = false;
	}
	listener.underwater = false;
	listener.Environment = 0;

	listener.ListenerObject = &sprite[player[pyrn].spritenum];
	soundEngine->SetListener(listener);
	soundEngine->UpdateSounds(I_GetTime());
}

int playsound_internal(int sn, spritetype *spr, int x, int y, int loop, int chan) 
{
	sn++;
	if (!soundEngine->isValidSoundId(sn)) return -1;
	int sourcetype = spr ? SOURCE_Actor : x != 0 || y != 0 ? SOURCE_Unattached : SOURCE_None;
	vec3_t pos = { x, y, 0 };	
	auto spos = GetSoundPos(&pos);
	float attn = sourcetype == SOURCE_None ? 0 : 1;
	int flags = sourcetype == SOURCE_Unattached ? CHANF_LISTENERZ : CHANF_NONE;
	if (loop != 0) flags |= CHANF_LOOP;
	auto sfx = soundEngine->StartSound(sourcetype, spr, &spos, chan, EChanFlags::FromInt(flags), sn, 1.f, attn);
	if (!sfx) return -1;
	if (loop > 0) sfx->UserData = IntToFixed(loop);
	else sfx->UserData = 0;
	return 0;
}



END_WH_NS