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

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void RunScreenJob(DScreenJob *job, std::function<void(bool skipped)> completion, bool clearbefore)
{
	if (clearbefore)
	{
		twod->ClearScreen();
		videoNextPage();
	}
	
	auto startTime = I_nsTime();
	
	// Input later needs to be event based so that these screens can do more than be skipped.
	inputState.ClearAllInput();
	while(true)
	{
		auto now = I_nsTime();
		handleevents();
		bool skiprequest = inputState.CheckAllInput();
		int state = job->Frame(now - startTime, skiprequest);
		videoNextPage();
		if (state < 1)
		{
			job->Destroy();
			completion(state < 0);
			return;
		}
	}
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
					soundEngine->StartSound(SOURCE_None, nullptr, nullptr, CHAN_BODY, CHANF_NONE, sound, 1.f, ATTN_NONE);
			}
		}
		curframe++;
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

void PlayVideo(const char* filename, const AnimSound* ans, const int* frameticks, std::function<void(bool skipped)> completion)
{
	if (!filename)	// this is for chaining optional videos without special case coding by the caller.
	{
		completion(false);
		return;
	}
	auto fr = fileSystem.OpenFileReader(filename);
	if (!fr.isOpen())
	{
		Printf("%s: Unable to open video\n", filename);
		completion(true);
		return;
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
			return;
		}
		RunScreenJob(anm, completion);
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
		completion(true);
		return;
	}
}

