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
#include "razemenu.h"
#include "raze_sound.h"
#include "SmackerDecoder.h"
#include "movie/playmve.h"
#include "gamecontrol.h"
#include <vpx/vpx_decoder.h>
#include <vpx/vp8dx.h>
#include "raze_music.h"


IMPLEMENT_CLASS(DScreenJob, true, false)
IMPLEMENT_CLASS(DImageScreen, true, false)


bool DSkippableScreenJob::OnEvent(event_t* evt)
{
	if (evt->type == EV_GUI_KeyDown) state = skipped;
	return true;
}


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
	if (!tex)
	{
		twod->ClearScreen();
		return 0;
	}
	int span = int(clock / 1'000'000);
	twod->ClearScreen();
	DrawTexture(twod, tex, 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_LegacyRenderStyle, STYLE_Normal, DTA_TranslationIndex, trans, TAG_DONE);
	// Only end after having faded out.
	return skiprequest ? -1 : span > waittime? 0 : 1;
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
	int64_t startTime = -1;
	int64_t lastTime = -1;
	int actionState;
	int terminateState;
	uint64_t clock = 0;

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
		lastTime= startTime = -1;
		clock = 0;
		inputState.ClearAllInput();
	}

	int DisplayFrame()
	{
		auto& job = jobs[index];
		auto now = I_GetTimeNS();
		bool processed = job.job->ProcessInput();
		if (startTime == -1)
		{
			lastTime = startTime = now;
		}
		else if (!M_Active())
		{
			clock += now - lastTime;
			if (clock == 0) clock = 1;
		}
		bool skiprequest = clock > 100'000'000 && inputState.CheckAllInput() && !processed && job.job->fadestate != DScreenJob::fadeout;
		lastTime = now;

		if (screenfade < 1.f && !M_Active())
		{
			float ms = (clock / 1'000'000) / job.job->fadetime;
			screenfade = clamp(ms, 0.f, 1.f);
			twod->SetScreenFade(screenfade);
			if (job.job->fadestate != DScreenJob::fadeout)
				job.job->fadestate = DScreenJob::fadein;
		}
		else
		{
			job.job->fadestate = DScreenJob::visible;
			screenfade = 1.f;
		}
		job.job->SetClock(clock);
		int state = job.job->Frame(clock, skiprequest, I_GetTimeFrac());
		clock = job.job->GetClock();
		if (clock == 0) clock = 1;
		return state;
	}

	int FadeoutFrame()
	{
		auto now = I_GetTimeNS();

		if (startTime == -1)
		{
			lastTime = startTime = now;
		}
		else if (!M_Active())
		{
			clock += now - lastTime;
			if (clock == 0) clock = 1;
		}
		lastTime = now;

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

	bool OnEvent(event_t* ev)
	{
		if (index >= jobs.Size()) return false;
		if (jobs[index].job->state != DScreenJob::running) return false;
		return jobs[index].job->OnEvent(ev);
	}

	void OnFinished()
	{
		if (completion) completion(false);
		completion = nullptr; // only finish once.
	}

	void OnTick()
	{
		if (index >= jobs.Size())
		{
			//DeleteJobs();
			//twod->SetScreenFade(1);
			//twod->ClearScreen(); // This must not leave the 2d buffer empty.
			//if (gamestate == GS_INTRO) OnFinished();
			//else Net_WriteByte(DEM_ENDSCREENJOB);	// intermissions must be terminated synchronously.
		}
		else
		{
			if (jobs[index].job->state != DScreenJob::running) return;
			jobs[index].job->ticks++;
			jobs[index].job->OnTick();
		}
	}
	
	bool RunFrame()
	{
		if (index >= jobs.Size())
		{
			DeleteJobs();
 			twod->SetScreenFade(1);
			twod->ClearScreen(); // This must not leave the 2d buffer empty.
			if (completion) completion(false);
			return false;
		}
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
					startTime = -1;
					clock = 0;
					jobs[index].job->fadestate = DScreenJob::fadeout;
					jobs[index].job->state = DScreenJob::stopping;
					gamestate = GS_INTRO;	// block menu and console during fadeout - this can cause timing problems.
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
				jobs[index].job->state = DScreenJob::stopped;
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
		gameaction = blockingui? ga_intro : ga_intermission;
	}
	else
	{
		completion(false);
	}
}

void DeleteScreenJob()
{
	if (runner)
	{
		delete runner;
		runner = nullptr;
	}
	twod->SetScreenFade(1);
}

void EndScreenJob()
{
	if (runner) runner->OnFinished();
	DeleteScreenJob();
}


bool ScreenJobResponder(event_t* ev)
{
	if (runner) return runner->OnEvent(ev);
	return false;
}

void ScreenJobTick()
{
	if (runner) runner->OnTick();
}

void ScreenJobDraw()
{
	// we cannot recover from this because we have no completion callback to call.
	if (!runner)
	{
		// We can get here before a gameaction has been processed. In that case just draw a black screen and wait.
		if (gameaction == ga_nothing) I_Error("Trying to run a non-existent screen job");
		twod->ClearScreen();
		return;
	}
	auto res = runner->RunFrame();
	if (!res)
	{
		assert((gamestate != GS_INTERMISSION && gamestate != GS_INTRO) || gameaction != ga_nothing);
		DeleteScreenJob();
	}
}

