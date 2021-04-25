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
#include "vm.h"


#if 0
IMPLEMENT_CLASS(DScreenJob, true, false)
IMPLEMENT_CLASS(DSkippableScreenJob, true, false)
IMPLEMENT_CLASS(DBlackScreen, true, false)
IMPLEMENT_CLASS(DImageScreen, true, false)

DEFINE_FIELD(DScreenJob, flags)
DEFINE_FIELD(DScreenJob, fadetime)
DEFINE_FIELD_NAMED(DScreenJob, state, jobstate)
DEFINE_FIELD(DScreenJob, fadestate)
DEFINE_FIELD(DScreenJob, ticks)
DEFINE_FIELD(DScreenJob, pausable)

DEFINE_FIELD(DBlackScreen, wait)
DEFINE_FIELD(DBlackScreen, cleared)

DEFINE_FIELD(DImageScreen, tilenum)
DEFINE_FIELD(DImageScreen, trans)
DEFINE_FIELD(DImageScreen, waittime)
DEFINE_FIELD(DImageScreen, cleared)
DEFINE_FIELD(DImageScreen, texid)

DEFINE_ACTION_FUNCTION(DScreenJob, Init)
{
	// todo
	return 0;
}

DEFINE_ACTION_FUNCTION(DScreenJob, ProcessInput)
{
	PARAM_SELF_PROLOGUE(DScreenJob);
	ACTION_RETURN_BOOL(self->ProcessInput());
}

DEFINE_ACTION_FUNCTION(DScreenJob, Start)
{
	PARAM_SELF_PROLOGUE(DScreenJob);
	self->Start();
	return 0;
}

DEFINE_ACTION_FUNCTION(DScreenJob, OnEvent)
{
	PARAM_SELF_PROLOGUE(DScreenJob);
	PARAM_POINTER(evt, FInputEvent);
	if (evt->Type != EV_KeyDown)
	{
		// not needed in the transition phase
		ACTION_RETURN_BOOL(false);
	}
	event_t ev = {};
	ev.type = EV_KeyDown;
	ev.data1 = evt->KeyScan;
	ACTION_RETURN_BOOL(self->OnEvent(&ev));
}

DEFINE_ACTION_FUNCTION(DScreenJob, OnTick)
{
	PARAM_SELF_PROLOGUE(DScreenJob);
	self->OnTick();
	return 0;
}

DEFINE_ACTION_FUNCTION(DScreenJob, Draw)
{
	PARAM_SELF_PROLOGUE(DScreenJob);
	PARAM_FLOAT(smooth);
	self->Draw(smooth);
	return 0;
}

DEFINE_ACTION_FUNCTION(DSkippableScreenJob, Init)
{
	// todo
	return 0;
}

DEFINE_ACTION_FUNCTION(DSkippableScreenJob, Skipped)
{
	PARAM_SELF_PROLOGUE(DSkippableScreenJob);
	self->Skipped();
	return 0;
}

DEFINE_ACTION_FUNCTION(DBlackScreen, Init)
{
	// todo
	return 0;
}

DEFINE_ACTION_FUNCTION(DImageScreen, Init)
{
	// todo
	return 0;
}




void DScreenJob::OnDestroy()
{
	if (flags & stopmusic) Mus_Stop();
	if (flags & stopsound) FX_StopAllSounds();
}

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

ScreenJobRunner::ScreenJobRunner(TArray<DScreenJob*>& jobs_, CompletionFunc completion_, bool clearbefore_, bool skipall_)
	: completion(std::move(completion_)), clearbefore(clearbefore_), skipall(skipall_)
{
	jobs = std::move(jobs_);
	// Release all jobs from the garbage collector - the code as it is cannot deal with them getting collected. This should be removed later once the GC is working.
	for (unsigned i = 0; i < jobs.Size(); i++)
	{
		jobs[i]->Release();
	}
	AdvanceJob(false);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

ScreenJobRunner::~ScreenJobRunner()
{
	DeleteJobs();
}

void ScreenJobRunner::DeleteJobs()
{
	for (auto& job : jobs)
	{
		job->ObjectFlags |= OF_YesReallyDelete;
		delete job;
	}
	jobs.Clear();
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void ScreenJobRunner::AdvanceJob(bool skip)
{
	if (index >= 0)
	{
		//if (jobs[index].postAction) jobs[index].postAction();
		jobs[index]->Destroy();
	}
	index++;
	while (index < jobs.Size() && (jobs[index] == nullptr || (skip && skipall)))
	{
		if (jobs[index] != nullptr) jobs[index]->Destroy();
		index++;
	}
	actionState = clearbefore ? State_Clear : State_Run;
	if (index < jobs.Size())
	{
		jobs[index]->fadestate = !paused && jobs[index]->flags & DScreenJob::fadein? DScreenJob::fadein : DScreenJob::visible;
		jobs[index]->Start();
	}
	inputState.ClearAllInput();
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

int ScreenJobRunner::DisplayFrame(double smoothratio)
{
	auto& job = jobs[index];
	auto now = I_GetTimeNS();
	bool processed = job->ProcessInput();

	if (job->fadestate == DScreenJob::fadein)
	{
		double ms = (job->ticks + smoothratio) * 1000 / GameTicRate / job->fadetime;
		float screenfade = (float)clamp(ms, 0., 1.);
		twod->SetScreenFade(screenfade);
		if (screenfade == 1.f) job->fadestate = DScreenJob::visible;
	}
	int state = job->DrawFrame(smoothratio);
	twod->SetScreenFade(1.f);
	return state;
}

int ScreenJobRunner::FadeoutFrame(double smoothratio)
{
	auto& job = jobs[index];
	double ms = (fadeticks + smoothratio) * 1000 / GameTicRate / job->fadetime;
	float screenfade = 1.f - (float)clamp(ms, 0., 1.);
	twod->SetScreenFade(screenfade);
	job->DrawFrame(1.);
	return (screenfade > 0.f);
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool ScreenJobRunner::OnEvent(event_t* ev)
{
	if (paused || index >= jobs.Size()) return false;

	if (jobs[index]->state != DScreenJob::running) return false;

	return jobs[index]->OnEvent(ev);
}

void ScreenJobRunner::OnFinished()
{
	if (completion) completion(false);
	completion = nullptr; // only finish once.
}

void ScreenJobRunner::OnTick()
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
		if (jobs[index]->state == DScreenJob::running)
		{
			jobs[index]->ticks++;
			jobs[index]->OnTick();
		}
		else if (jobs[index]->state == DScreenJob::stopping)
		{
			fadeticks++;
		}
	}
}
	
//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

bool ScreenJobRunner::RunFrame()
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
	if (menuon) last_paused_tic = jobs[index]->ticks;
	else if (last_paused_tic == jobs[index]->ticks) menuon = true;
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
			if (jobs[index]->flags & DScreenJob::fadeout)
			{
				jobs[index]->fadestate = DScreenJob::fadeout;
				jobs[index]->state = DScreenJob::stopping;
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
			jobs[index]->state = DScreenJob::stopped;
			AdvanceJob(terminateState < 0);
		}
	}
	return true;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

ScreenJobRunner *runner;

#endif


#if 0
void RunScreenJob(TArray<DScreenJob*>& jobs, CompletionFunc completion, int flags)
{
	assert(completion != nullptr);
	videoclearFade();
	if (jobs.Size())
	{
		runner = new ScreenJobRunner(jobs, completion, !(flags & SJ_DONTCLEAR), !!(flags & SJ_SKIPALL));
		gameaction = (flags & SJ_BLOCKUI)? ga_intro : ga_intermission;
	}
	else
	{
		completion(false);
	}
}
#endif

void DeleteScreenJob()
{
	/*
	if (runner)
	{
		delete runner;
		runner = nullptr;
	}
	twod->SetScreenFade(1);*/
}

void EndScreenJob()
{
	//if (runner) runner->OnFinished();
	DeleteScreenJob();
}


bool ScreenJobResponder(event_t* ev)
{
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

	//if (runner) return runner->OnEvent(ev);
	return false;
}

void ScreenJobTick()
{
	//if (runner) runner->OnTick();
}

bool ScreenJobDraw()
{
	// we cannot recover from this because we have no completion callback to call.
	/*
	if (!runner)
	{
		// We can get here before a gameaction has been processed. In that case just draw a black screen and wait.
		if (gameaction == ga_nothing) I_Error("Trying to run a non-existent screen job");
		twod->ClearScreen();
		return false;
	}
	auto res = runner->RunFrame();
	*/ int res = 0;
	if (!res)
	{
		assert((gamestate != GS_INTERMISSION && gamestate != GS_INTRO) || gameaction != ga_nothing);
		DeleteScreenJob();
	}
	return res;
}
