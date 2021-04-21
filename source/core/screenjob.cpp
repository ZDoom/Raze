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
	if (evt->type == EV_KeyDown && !specialKeyEvent(evt))
	{
		state = skipped;
		Skipped();
	}
	return true;
}

void DBlackScreen::OnTick()
{
	if (cleared)
	{
		int span = ticks * 1000 / GameTicRate;
		if (span > wait) state = finished;
	}
}

void DBlackScreen::Draw(double)
{
	cleared = true;
	twod->ClearScreen();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void DImageScreen::OnTick()
{
	if (cleared)
	{
		int span = ticks * 1000 / GameTicRate;
		if (span > waittime) state = finished;
	}
}


void DImageScreen::Draw(double smoothratio)
{
	if (tilenum > 0) tex = tileGetTexture(tilenum, true);
	twod->ClearScreen();
	if (tex) DrawTexture(twod, tex, 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_LegacyRenderStyle, STYLE_Normal, DTA_TranslationIndex, trans, TAG_DONE);
	cleared = true;
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
	bool skipall;
	int actionState;
	int terminateState;
	int fadeticks = 0;
	int last_paused_tic = -1;

public:
	ScreenJobRunner(JobDesc* jobs_, int count, CompletionFunc completion_, bool clearbefore_, bool skipall_)
		: completion(std::move(completion_)), clearbefore(clearbefore_), skipall(skipall_)
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
		while (index < jobs.Size() && (jobs[index].job == nullptr || (skip && skipall)))
		{
			if (jobs[index].job != nullptr) jobs[index].job->Destroy();
			index++;
		}
		actionState = clearbefore ? State_Clear : State_Run;
		if (index < jobs.Size())
		{
			jobs[index].job->fadestate = !paused && jobs[index].job->fadestyle & DScreenJob::fadein? DScreenJob::fadein : DScreenJob::visible;
			jobs[index].job->Start();
		}
		inputState.ClearAllInput();
	}

	int DisplayFrame(double smoothratio)
	{
		auto& job = jobs[index];
		auto now = I_GetTimeNS();
		bool processed = job.job->ProcessInput();

		if (job.job->fadestate == DScreenJob::fadein)
		{
			double ms = (job.job->ticks + smoothratio) * 1000 / GameTicRate / job.job->fadetime;
			float screenfade = (float)clamp(ms, 0., 1.);
			twod->SetScreenFade(screenfade);
			if (screenfade == 1.f) job.job->fadestate = DScreenJob::visible;
		}
		int state = job.job->DrawFrame(smoothratio);
		twod->SetScreenFade(1.f);
		return state;
	}

	int FadeoutFrame(double smoothratio)
	{
		auto& job = jobs[index];
		double ms = (fadeticks + smoothratio) * 1000 / GameTicRate / job.job->fadetime;
		float screenfade = 1.f - (float)clamp(ms, 0., 1.);
		twod->SetScreenFade(screenfade);
		job.job->DrawFrame(1.);
		return (screenfade > 0.f);
	}

	bool OnEvent(event_t* ev)
	{
		if (paused || index >= jobs.Size()) return false;

		if (ev->type == EV_KeyDown)
		{
			// We never reach the key binding checks in G_Responder, so for the console we have to check for ourselves here.
			auto binding = Bindings.GetBinding(ev->data1);
			if (binding.CompareNoCase("toggleconsole") == 0)
			{
				C_ToggleConsole();
				return true;
			}
		}

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
		if (paused) return;
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
			if (jobs[index].job->state == DScreenJob::running)
			{
				jobs[index].job->ticks++;
				jobs[index].job->OnTick();
			}
			else if (jobs[index].job->state == DScreenJob::stopping)
			{
				fadeticks++;
			}
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

		// ensure that we won't go back in time if the menu is dismissed without advancing our ticker
		bool menuon = paused;
		if (menuon) last_paused_tic = jobs[index].job->ticks;
		else if (last_paused_tic == jobs[index].job->ticks) menuon = true;
		double smoothratio = menuon ? 1. : I_GetTimeFrac();

		if (actionState == State_Clear)
		{
			actionState = State_Run;
			twod->ClearScreen();
		}
		else if (actionState == State_Run)
		{
			terminateState = DisplayFrame(smoothratio);
			if (terminateState < 1)
			{
				// Must lock before displaying.
				if (jobs[index].job->fadestyle & DScreenJob::fadeout)
				{
					jobs[index].job->fadestate = DScreenJob::fadeout;
					jobs[index].job->state = DScreenJob::stopping;
					actionState = State_Fadeout;
					fadeticks = 0;
				}
				else
				{
					AdvanceJob(terminateState < 0);
				}
			}
		}
		else if (actionState == State_Fadeout)
		{
			int ended = FadeoutFrame(smoothratio);
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

void RunScreenJob(JobDesc* jobs, int count, CompletionFunc completion, bool clearbefore, bool blockingui, bool skipall)
{
	assert(completion != nullptr);
	videoclearFade();
	if (count)
	{
		runner = new ScreenJobRunner(jobs, count, completion, clearbefore, skipall);
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

bool ScreenJobDraw()
{
	// we cannot recover from this because we have no completion callback to call.
	if (!runner)
	{
		// We can get here before a gameaction has been processed. In that case just draw a black screen and wait.
		if (gameaction == ga_nothing) I_Error("Trying to run a non-existent screen job");
		twod->ClearScreen();
		return false;
	}
	auto res = runner->RunFrame();
	if (!res)
	{
		assert((gamestate != GS_INTERMISSION && gamestate != GS_INTRO) || gameaction != ga_nothing);
		DeleteScreenJob();
	}
	return res;
}

