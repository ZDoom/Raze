/*
** screenjob.cpp
**
** Generic asynchronous screen display
**
**---------------------------------------------------------------------------
** Copyright 2020 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include "types.h"
#include "build.h"
#include "screenjob.h"
#include "i_time.h"
#include "v_2ddrawer.h"
#include "animlib.h"
#include "v_draw.h"
#include "s_soundinternal.h"
#include "animtexture.h"
#include "gamestate.h"
#include "menu.h"
#include "raze_sound.h"
#include "SmackerDecoder.h"
#include "movie/playmve.h"
#include "gamecontrol.h"


IMPLEMENT_CLASS(DScreenJob, true, false)
IMPLEMENT_CLASS(DImageScreen, true, false)


int DBlackScreen::Frame(uint64_t clock, bool skiprequest)
{
	int span = int(clock / 1'000'000);
	twod->ClearScreen();
	return span < wait ? 1 : -1;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int DImageScreen::Frame(uint64_t clock, bool skiprequest)
{
	if (tilenum > 0)
	{
		tex = tileGetTexture(tilenum, true);
	}
	if (!tex) return 0;
	int span = int(clock / 1'000'000);
	twod->ClearScreen();
	DrawTexture(twod, tex, 0, 0, DTA_FullscreenEx, 3, DTA_LegacyRenderStyle, STYLE_Normal, TAG_DONE);
	// Only end after having faded out.
	return skiprequest ? -1 : span > waittime? 0 : 1;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DAnmPlayer : public DScreenJob
{
	// This doesn't need its own class type
	anim_t anim;
	TArray<uint8_t> buffer;
	int numframes = 0;
	int curframe = 1;
	int frametime = 0;
	int ototalclock = 0;
	AnimTextures animtex;
	const AnimSound* animSnd;
	const int* frameTicks;

public:
	bool isvalid() { return numframes > 0; }

	DAnmPlayer(FileReader& fr, const AnimSound* ans, const int *frameticks)
		: animSnd(ans), frameTicks(frameticks)
	{
		buffer = fr.ReadPadded(1);
		fr.Close();

		if (ANIM_LoadAnim(&anim, buffer.Data(), buffer.Size() - 1) < 0)
		{
			return;
		}
		numframes = ANIM_NumFrames(&anim);
		animtex.SetSize(AnimTexture::Paletted, 320, 200);
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	int Frame(uint64_t clock, bool skiprequest) override
	{
		int totalclock = int(clock * 120 / 1'000'000'000);

		if (curframe > 4 && totalclock > frametime + 60)
		{
			Printf("WARNING: slowdown in video playback, aborting\n");
			soundEngine->StopAllChannels();
			return -1;
		}

		if (totalclock < ototalclock - 1)
		{
			twod->ClearScreen();
			DrawTexture(twod, animtex.GetFrame(), 0, 0, DTA_FullscreenEx, 3, DTA_Masked, false, TAG_DONE);
			if (skiprequest) soundEngine->StopAllChannels();
			return skiprequest? -1 : 1;
		}

		animtex.SetFrame(ANIM_GetPalette(&anim), ANIM_DrawFrame(&anim, curframe));
		frametime = totalclock;

		twod->ClearScreen();
		DrawTexture(twod, animtex.GetFrame(), 0, 0, DTA_FullscreenEx, 3, DTA_Masked, false, TAG_DONE);

		int delay = 20;
		if (frameTicks)
		{
			if (curframe == 0) delay = frameTicks[0];
			else if (curframe < numframes - 1) delay = frameTicks[1];
			else delay = frameTicks[2];
		}
		ototalclock += delay;

		for (int i = 0; animSnd[i].framenum >= 0; i++)
		{
			if (animSnd[i].framenum == curframe)
			{
				int sound = animSnd[i].soundnum;
				if (sound == -1)
					soundEngine->StopAllChannels();
				else if (SoundEnabled())
					soundEngine->StartSound(SOURCE_None, nullptr, nullptr, CHAN_AUTO, CHANF_UI, sound, 1.f, ATTN_NONE);
			}
		}
		curframe++;
		if (skiprequest) soundEngine->StopAllChannels();
		return skiprequest ? -1 : curframe < numframes? 1 : 0;
	}

	void OnDestroy() override
	{
		buffer.Reset();
		animtex.Clean();
	}
};

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DMvePlayer : public DScreenJob
{
	InterplayDecoder decoder;
	bool failed = false;
	
public:
	bool isvalid() { return !failed; }

	DMvePlayer(FileReader& fr)
	{
		failed = !decoder.Open(fr);
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	int Frame(uint64_t clock, bool skiprequest) override
	{
		if (failed) return -1;
		bool playon = decoder.RunFrame(clock);
		twod->ClearScreen();
		DrawTexture(twod, decoder.animTex().GetFrame(), 0, 0, DTA_FullscreenEx, 3, TAG_DONE);

		return skiprequest ? -1 : playon ? 1 : 0;
	}

	void OnDestroy() override
	{
		decoder.Close();
	}
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DSmkPlayer : public DScreenJob
{
	SmackerHandle hSMK{};
	uint32_t nWidth, nHeight;
	uint8_t palette[768];
	AnimTextures animtex;
	TArray<uint8_t> pFrame;
	int nFrameRate;
	int nFrames;
	bool fullscreenScale;
	uint64_t nFrameNs;
	int nFrame = 0;
	const AnimSound* animSnd;
	FString filename;

public:
	bool isvalid() { return hSMK.isValid; }

	DSmkPlayer(const char *fn, const AnimSound* ans, bool fixedviewport)
	{
		hSMK = Smacker_Open(fn);
		if (!hSMK.isValid)
		{
			return;
		}
		Smacker_GetFrameSize(hSMK, nWidth, nHeight);
		pFrame.Resize(nWidth * nHeight + std::max(nWidth, nHeight));
		nFrameRate = Smacker_GetFrameRate(hSMK);
		nFrameNs = 1'000'000'000 / nFrameRate;
		nFrames = Smacker_GetNumFrames(hSMK);
		Smacker_GetPalette(hSMK, palette);
		fullscreenScale = (!fixedviewport || (nWidth <= 320 && nHeight <= 200) || nWidth >= 640 || nHeight >= 480);
		animSnd = ans;
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	int Frame(uint64_t clock, bool skiprequest) override
	{
		int frame = clock / nFrameNs;

		if (clock == 0)
		{
			animtex.SetSize(AnimTexture::Paletted, nWidth, nHeight);
		}
		twod->ClearScreen();
		if (frame > nFrame)
		{
			Smacker_GetPalette(hSMK, palette);
			Smacker_GetFrame(hSMK, pFrame.Data());
			animtex.SetFrame(palette, pFrame.Data());
		}
		if (fullscreenScale)
		{
			DrawTexture(twod, animtex.GetFrame(), 0, 0, DTA_FullscreenEx, 3, TAG_DONE);
		}
		else
		{
			DrawTexture(twod, animtex.GetFrame(), 320, 240, DTA_VirtualWidth, 640, DTA_VirtualHeight, 480, DTA_CenterOffset, true, TAG_DONE);
		}
		if (frame > nFrame)
		{
			nFrame++;
			Smacker_GetNextFrame(hSMK);
			for (int i = 0; animSnd[i].framenum >= 0; i++)
			{
				if (animSnd[i].framenum == nFrame)
				{
					int sound = animSnd[i].soundnum;
					if (sound == -1)
						soundEngine->StopAllChannels();
					else
						soundEngine->StartSound(SOURCE_None, nullptr, nullptr, CHAN_AUTO, CHANF_UI, sound, 1.f, ATTN_NONE);
				}
			}
		}

		return skiprequest ? -1 : nFrame < nFrames ? 1 : 0;
	}

	void OnDestroy() override
	{
		Smacker_Close(hSMK);
		soundEngine->StopAllChannels();
	}
};

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

DScreenJob* PlayVideo(const char* filename, const AnimSound* ans, const int* frameticks)
{
	auto nothing = []()->DScreenJob* { return Create<DScreenJob>(); };
	if (!filename)
	{
		return nothing();
	}
	auto fr = fileSystem.OpenFileReader(filename);
	if (!fr.isOpen())
	{
		int nLen = strlen(filename);
		// Strip the drive letter and retry.
		if (nLen >= 3 && isalpha(filename[0]) && filename[1] == ':' && filename[2] == '/')
		{
			filename += 3;
			fr = fileSystem.OpenFileReader(filename);
		}
		if (!fr.isOpen())
		{
			Printf("%s: Unable to open video\n", filename);
			return nothing();
		}
	}
	char id[20] = {};

	fr.Read(&id, 20);
	fr.Seek(-20, FileReader::SeekCur);

	if (!memcmp(id, "LPF ", 4))
	{
		auto anm = Create<DAnmPlayer>(fr, ans, frameticks);
		if (!anm->isvalid())
		{
			Printf("%s: invalid ANM file.\n", filename);
			anm->Destroy();
			return nothing();
		}
		return anm;
	}
	else if (!memcmp(id, "SMK2", 4))
	{
		fr.Close();
		auto anm = Create<DSmkPlayer>(filename, ans, true); // Fixme: Handle Blood's video scaling behavior more intelligently.
		if (!anm->isvalid())
		{
			Printf("%s: invalid SMK file.\n", filename);
			anm->Destroy();
			return nothing();
		}
		return anm;
	}
	else if (!memcmp(id, "Interplay MVE File", 18))
	{
		auto anm = Create<DMvePlayer>(fr);
		if (!anm->isvalid())
		{
			anm->Destroy();
			return nothing();
		}
		return anm;
	}
	// add more formats here.
	else
	{
		Printf("%s: Unknown video format\n", filename);
	}
	return nothing();
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class ScreenJobRunner
{
	enum
	{
		State_Clear,
		State_Run,
		State_Fadeout
	};
	TArray<JobDesc> jobs;
	CompletionFunc completion;
	int index = -1;
	float screenfade;
	bool clearbefore;
	uint64_t startTime = -1;
	uint64_t lastTime = -1;
	int actionState;
	int terminateState;

public:
	ScreenJobRunner(JobDesc* jobs_, int count, CompletionFunc completion_, bool clearbefore_)
		: completion(std::move(completion_)), clearbefore(clearbefore_)
	{
		jobs.Resize(count);
		memcpy(jobs.Data(), jobs_, count * sizeof(JobDesc));
		// Release all jobs from the garbage collector - the code as it is cannot deal with them getting collected. This should be removed later once the GC is working.
		for (int i = 0; i < count; i++)
		{
			jobs[i].job->Release();
		}
		AdvanceJob(false);
	}

	~ScreenJobRunner()
	{
		DeleteJobs();
	}

	void DeleteJobs()
	{
		for (auto& job : jobs)
		{
			job.job->ObjectFlags |= OF_YesReallyDelete;
			delete job.job;
		}
		jobs.Clear();
	}

	void AdvanceJob(bool skip)
	{
		if (index >= 0)
		{
			if (jobs[index].postAction) jobs[index].postAction();
			jobs[index].job->Destroy();
		}
		index++;
		while (index < jobs.Size() && (jobs[index].job == nullptr || (skip && jobs[index].ignoreifskipped)))
		{
			if (jobs[index].job != nullptr) jobs[index].job->Destroy();
			index++;
		}
		actionState = clearbefore ? State_Clear : State_Run;
		if (index < jobs.Size()) screenfade = jobs[index].job->fadestyle & DScreenJob::fadein ? 0.f : 1.f;
		startTime = -1;
		inputState.ClearAllInput();
	}

	int DisplayFrame()
	{
		auto& job = jobs[index];
		auto now = I_nsTime();
		bool skiprequest = inputState.CheckAllInput();
		if (startTime == -1) startTime = now;

		if (M_Active())
		{
			startTime += now - lastTime;
		}
		lastTime = now;

		auto clock = now - startTime;
		if (screenfade < 1.f)
		{
			float ms = (clock / 1'000'000) / job.job->fadetime;
			screenfade = clamp(ms, 0.f, 1.f);
			if (!M_Active()) twod->SetScreenFade(screenfade);
			job.job->fadestate = DScreenJob::fadein;
		}
		else job.job->fadestate = DScreenJob::visible;
		job.job->SetClock(clock);
		int state = job.job->Frame(clock, skiprequest);
		startTime -= job.job->GetClock() - clock;
		return state;
	}

	int FadeoutFrame()
	{
		auto now = I_nsTime();

		if (M_Active())
		{
			startTime += now - lastTime;
		}
		lastTime = now;

		auto clock = now - startTime;
		float ms = (clock / 1'000'000) / jobs[index].job->fadetime;
		float screenfade2 = clamp(screenfade - ms, 0.f, 1.f);
		if (!M_Active()) twod->SetScreenFade(screenfade2);
		if (screenfade2 <= 0.f)
		{
			twod->Unlock(); // must unlock before displaying.
			return 0;
		}
		return 1;
	}

	bool RunFrame()
	{
		if (index >= jobs.Size())
		{
			DeleteJobs();
 			twod->SetScreenFade(1);
			if (completion) completion(false);
			return false;
		}
		handleevents();
		if (actionState == State_Clear)
		{
			actionState = State_Run;
			twod->ClearScreen();
		}
		else if (actionState == State_Run)
		{
			terminateState = DisplayFrame();
			if (terminateState < 1)
			{
				// Must lock before displaying.
				if (jobs[index].job->fadestyle & DScreenJob::fadeout)
				{
					twod->Lock();
					startTime = I_nsTime();
					jobs[index].job->fadestate = DScreenJob::fadeout;
					actionState = State_Fadeout;
				}
				else
				{
					AdvanceJob(terminateState < 0);
				}
			}
		}
		else if (actionState == State_Fadeout)
		{
			int ended = FadeoutFrame();
			if (ended < 1)
			{
				AdvanceJob(terminateState < 0);
			}
		}
		return true;
	}
};

ScreenJobRunner *runner;

void RunScreenJob(JobDesc* jobs, int count, CompletionFunc completion, bool clearbefore, bool blockingui)
{
	assert(completion != nullptr);
	videoclearFade();
	if (count)
	{
		runner = new ScreenJobRunner(jobs, count, completion, clearbefore);
		gamestate = blockingui? GS_INTRO : GS_INTERMISSION;
	}
}

void DeleteScreenJob()
{
	if (runner)
	{
		delete runner;
		runner = nullptr;
	}
}

void RunScreenJobFrame()
{
	// we cannot recover from this because we have no completion callback to call.
	if (!runner) I_Error("Trying to run a non-existent screen job");
	auto res = runner->RunFrame();
	if (!res)
	{
		assert(gamestate != GS_INTERMISSION && gamestate != GS_INTRO);
		DeleteScreenJob();
	}
}

