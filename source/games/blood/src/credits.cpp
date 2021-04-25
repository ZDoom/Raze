//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
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

#include "build.h"
#include "compat.h"
#include "SmackerDecoder.h"
#include "blood.h"
#include "animtexture.h"
#include "raze_sound.h"
#include "v_2ddrawer.h"
#include "screenjob.h"
#include "gamestate.h"
#include "razemenu.h"

BEGIN_BLD_NS

#if 0
class DBloodIntroImage : public DImageScreen
{
	bool mus;

public:
	DBloodIntroImage(int tilenum, int flags = 0, bool withmusic = false) : DImageScreen(tilenum, flags), mus(withmusic)
	{}

	void Start() override
	{
		sndStartSample("THUNDER2", 128, -1);
		if (mus) sndPlaySpecialMusicOrNothing(MUS_INTRO);
	}
};
#endif

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void playlogos()
{
#if 0
	TArray<DScreenJob*> jobs;
	static AnimSound logosound[] =
	{
		{ 1, -1 },
		{ -1, -1 },
		{ 1, -1 },
		{ -1,-1 }
	};
	
	if (logosound[0].soundnum == -1)
	{
		logosound[0].soundnum = S_FindSound("logo.wav");
		logosound[2].soundnum = S_FindSound("gt.wav");
	}


	if (!userConfig.nologo)
	{
		if (fileSystem.FindFile("logo.smk") != -1)
		{
			jobs.Push(PlayVideo("logo.smk", &logosound[0], 0));
		}
		else
		{
			jobs.Push(Create<DBloodIntroImage>(2050));
		}
		if (fileSystem.FindFile("gti.smk") != -1)
		{
			jobs.Push(PlayVideo("gti.smk", &logosound[2], 0));
		}
		else
		{
			jobs.Push(Create<DBloodIntroImage>(2052));
		}
	}
	jobs.Push(Create<DBlackScreen>(1));
	jobs.Push(Create<DBloodIntroImage>(2518, DScreenJob::fadein, true));

	RunScreenJob(jobs, [](bool) { 
		Mus_Stop();
		gameaction = ga_mainmenu;
		}, SJ_BLOCKUI);
#endif
}

void playSmk(const char *smk, const char *wav, int wavid, CompletionFunc func)
{
#if 0
	TArray<DScreenJob*> jobs;
	static AnimSound smksound[] =
	{
		{ 1, -1 },
		{ -1, -1 },
	};
	int id = S_FindSoundByResID(wavid);
	if (id <= 0)
	{
		FString wavv = wav;
		FixPathSeperator(wavv);
		id = S_FindSound(wavv);
		// Strip the drive letter and retry.
		if (id <= 0 && wavv.Len() > 3 && wavv[1] == ':' && isalpha(wavv[0]) && wavv[2] == '/')
		{
			id = S_FindSound(wavv.GetChars() + 3);
		}
	}
	FString smkk = smk;
	FixPathSeperator(smkk);
	smksound[0].soundnum = id;
	jobs.Push(PlayVideo(smkk, smksound, nullptr));
	RunScreenJob(jobs, func);
#endif
}

void levelPlayIntroScene(int nEpisode, CompletionFunc completion)
{
	Mus_Stop();
    sndKillAllSounds();
    sfxKillAllSounds();
    ambKillAll();
    seqKillAll();
    EPISODEINFO *pEpisode = &gEpisodeInfo[nEpisode];
	playSmk(pEpisode->cutsceneAName, pEpisode->cutsceneASound, pEpisode->cutsceneAWave, completion);
}

void levelPlayEndScene(int nEpisode, CompletionFunc completion)
{
    gGameOptions.uGameFlags &= ~GF_PlayCutscene;
    Mus_Stop();
    sndKillAllSounds();
    sfxKillAllSounds();
    ambKillAll();
    seqKillAll();
    EPISODEINFO *pEpisode = &gEpisodeInfo[nEpisode];
    playSmk(pEpisode->cutsceneBName, pEpisode->cutsceneBSound, pEpisode->cutsceneBWave, completion);
}



END_BLD_NS
