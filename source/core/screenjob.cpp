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
		int frame = int((now - startTime) * 120 / 1'000'000'000);
		bool skiprequest = inputState.CheckAllInput();
		twod->ClearScreen();
		int state = job->Frame(frame, skiprequest);
		videoNextPage();
		if (state < 1)
		{
			completion(state < 0);
			return;
		}
	}
}

void PlayVideo(const char *filename, AnimSound *ans, int frameticks, std::function<void(bool skipped)> completion);


#if 0
void RunScreen(const char *classname, VMValue *params, int numparams, std::function<void(bool aborted)> completion)
{
	int ticker = 0;
	auto ototalclock = totalclock;
	
	PClass *cls = PClass::FindClass(classname);
	if (cls != nullptr && cls->IsDescendantOf(NAME_GameScreen))
	{
		auto func = dyn_cast<PFunction>(cls->FindSymbol("Init", true));
		if (func != nullptr && !(func->Variants[0].Flags & (VARF_Protected | VARF_Private)))	// skip internal classes which have a protected init method.
		{
			TArray<VMValue> args(numparams+1);
			for(int i = 0;i<numparams;i++) args[i+1] = params[i];
			auto item = cls->CreateNew(); 			
			if (!item) 
			{
				completion(true);
				return;
			}
			args[0] = item;
			VMCallWithDefaults(func->Variants[0].Implementation, args, nullptr, 0); 	
			
			while (true)
			{
				handleevents();
				event_t *ev;
				// This should later run off D_ProcessEvents, but currently requires a local copy here
				while (eventtail != eventhead)
				{
					ev = &events[eventtail];
					eventtail = (eventtail + 1) & (MAXEVENTS - 1);
					if (ev->type == EV_None)
						continue;
					if (ev->type == EV_DeviceChange)
						UpdateJoystickMenu(I_UpdateDeviceList());
#if 0	// this isn't safe with the current engine structure
					if (C_Responder (ev))
						continue;				// console ate the event
					if (M_Responder (ev))
						continue;				// menu ate the event
#endif

					IFVIRTUALPTRNAME(item, NAME_GameScreen, OnEvent)
					{
						FInputEvent e = ev;
						VMValue params[] = { (DObject*)item, &e };
						int retval;
						VMReturn ret(&retval);
						VMCall(func, params, 2, &ret, 1);
						if (!retval)
						{
							completion(true);
							return;
						}
					} 
				}
				
				while ((int)(totalclock - ototalclock) >= 1)
				{
					IFVIRTUALPTRNAME(item, NAME_GameScreen, Tick)
					{
						FInputEvent e = ev;
						VMValue params[] = { (DObject*)item, ticker };
						ticker++;
						int retval;
						VMReturn ret(&retval);
						VMCall(func, params, 2, &ret, 1);
						if (!retval)
						{
							completion(false);
							return;
						}
					} 
				}
				IFVIRTUALPTRNAME(item, NAME_GameScreen, Draw)
				{
					FInputEvent e = ev;
					VMValue params[] = { (DObject*)item };
					ticker++;
					VMCall(func, params, 1, nullptr, 0);
				} 
				videoNextPage();
			}
		}
	}
	completion(true);
}
#endif