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


IMPLEMENT_CLASS(DScreenJob, true, false)
IMPLEMENT_CLASS(DImageScreen, true, false)

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
	return skiprequest ? -1 : 1;
}


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void RunScreenJob(JobDesc *jobs, int count, CompletionFunc completion, bool clearbefore)
{
	// Release all jobs from the garbage collector - the code as it is cannot deal with them getting collected.
	for (int i = 0; i < count; i++)
	{
		jobs[i].job->Release();
	}
	bool skipped = false;
	for (int i = 0; i < count; i++)
	{
		auto job = jobs[i];
		if (job.job != nullptr && (!skipped || !job.ignoreifskipped))
		{
			skipped = false;
			if (clearbefore)
			{
				twod->ClearScreen();
				videoNextPage();
			}

			auto startTime = I_nsTime();

			// Input later needs to be event based so that these screens can do more than be skipped.
			inputState.ClearAllInput();

			float screenfade = job.job->fadestyle & DScreenJob::fadein ? 0.f : 1.f;

			while (true)
			{
				auto now = I_nsTime();
				handleevents();
				bool skiprequest = inputState.CheckAllInput();
				auto clock = now - startTime;
				if (screenfade < 1.f)
				{
					float ms = (clock / 1'000'000) / job.job->fadetime;
					screenfade = clamp(ms, 0.f, 1.f);
					twod->SetScreenFade(screenfade);
					job.job->fadestate = DScreenJob::fadein;
				}
				else job.job->fadestate = DScreenJob::visible;
				job.job->SetClock(clock);
				int state = job.job->Frame(clock, skiprequest);
				startTime -= job.job->GetClock() - clock;
				// Must lock before displaying.
				if (state < 1 && job.job->fadestyle & DScreenJob::fadeout)
					twod->Lock();

				videoNextPage();
				if (state < 1)
				{
					if (job.job->fadestyle & DScreenJob::fadeout)
					{
						job.job->fadestate = DScreenJob::fadeout;
						startTime = now;
						float screenfade2;
						do
						{
							now = I_nsTime();
							auto clock = now - startTime;
							float ms = (clock / 1'000'000) / job.job->fadetime;
							screenfade2 = clamp(screenfade - ms, 0.f, 1.f);
							twod->SetScreenFade(screenfade2);
							if (screenfade2 <= 0.f) twod->Unlock(); // must unlock before displaying.
							videoNextPage();
							
						} while (screenfade2 > 0.f);
					}
					skipped = state < 0;
					job.job->Destroy();
					job.job->ObjectFlags |= OF_YesReallyDelete;
					delete job.job;
					twod->SetScreenFade(1);
					break;
				}
			}
		}
		if (job.postAction) job.postAction();
	}
	if (completion) completion(false);
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
	int lastframe = -1;
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
			return -1;
		}

		if (totalclock < ototalclock - 1)
		{
			twod->ClearScreen();
			DrawTexture(twod, animtex.GetFrame(), 0, 0, DTA_FullscreenEx, 3, DTA_Masked, false, TAG_DONE);
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
				else
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

DScreenJob *PlayVideo(const char* filename, const AnimSound* ans, const int* frameticks)
{
	auto nothing = []()->DScreenJob* { return Create<DScreenJob>(); };
	if (!filename)
	{
		return nothing();
	}
	auto fr = fileSystem.OpenFileReader(filename);
	if (!fr.isOpen())
	{
		Printf("%s: Unable to open video\n", filename);
		return nothing();
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
		// todo
	}
	else if (!memcmp(id, "Interplay MVE file", 18))
	{
		// todo
	}
	// add more formats here.
	else
	{
		Printf("%s: Unknown video format\n", filename);
	}
	return nothing();
}

