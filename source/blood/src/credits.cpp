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
#include "common_game.h"
#include "blood.h"
#include "controls.h"
#include "globals.h"
#include "sound.h"
#include "view.h"
#include "animtexture.h"
#include "../glbackend/glbackend.h"
#include "raze_sound.h"
#include "v_2ddrawer.h"
#include "screenjob.h"
#include "gamestate.h"
#include "seq.h"
#include "menu.h"

BEGIN_BLD_NS


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void playlogos()
{
	JobDesc jobs[6];
	int job = 0;
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


	if (fileSystem.FindFile("logo.smk"))
	{
		jobs[job++] = { PlayVideo("logo.smk", &logosound[0], 0) };
	}
	else
	{
		jobs[job++] = { Create<DBlackScreen>(1), []() { sndStartSample("THUNDER2", 128, -1); }};
		jobs[job++] = { Create<DImageScreen>(2050) };
	}
	if (fileSystem.FindFile("gti.smk"))
	{
		jobs[job++] = { PlayVideo("gti.smk", &logosound[2], 0) };
	}
	else
	{
		jobs[job++] = { Create<DBlackScreen>(1), []() { sndStartSample("THUNDER2", 128, -1); }};
		jobs[job++] = { Create<DImageScreen>(2052) };
	}
	jobs[job++] = { Create<DBlackScreen>(1), []() { sndPlaySpecialMusicOrNothing(MUS_INTRO); sndStartSample("THUNDER2", 128, -1); }};
	jobs[job++] = { Create<DImageScreen>(2518, DScreenJob::fadein) };

	RunScreenJob(jobs, job, [](bool) { 
		gamestate = GS_DEMOSCREEN;
		M_StartControlPanel(false);
		M_SetMenu(NAME_Mainmenu);
	}, true, true);
}

void playSmk(const char *smk, const char *wav, int wavid, CompletionFunc func)
{
	JobDesc jobs{};
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
	}
	FString smkk = smk;
	FixPathSeperator(smkk);
	smksound[0].soundnum = id;
	jobs.job = PlayVideo(smkk, smksound, nullptr);
	RunScreenJob(&jobs, 1, func);
}

void levelPlayIntroScene(int nEpisode, CompletionFunc completion)
{
    gGameOptions.uGameFlags &= ~4;
	Mus_Stop();
    sndKillAllSounds();
    sfxKillAllSounds();
    ambKillAll();
    seqKillAll();
    EPISODEINFO *pEpisode = &gEpisodeInfo[nEpisode];
	playSmk(pEpisode->cutsceneAName, pEpisode->cutsceneASound, pEpisode->at9028, completion);
}

void levelPlayEndScene(int nEpisode, CompletionFunc completion)
{
    gGameOptions.uGameFlags &= ~8;
    Mus_Stop();
    sndKillAllSounds();
    sfxKillAllSounds();
    ambKillAll();
    seqKillAll();
    EPISODEINFO *pEpisode = &gEpisodeInfo[nEpisode];
    playSmk(pEpisode->cutsceneBName, pEpisode->cutsceneBSound, pEpisode->at902c, completion);
}



END_BLD_NS
