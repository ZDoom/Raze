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
#include "mapinfo.h"

static DObject* runner;
static SummaryInfo sinfo;
static PClass* runnerclass;
static PType* runnerclasstype;
static PType* maprecordtype;
static PType* summaryinfotype;
static CompletionFunc completion;
static int ticks;
static SummaryInfo summaryinfo;

//=============================================================================
//
//
//
//=============================================================================

void Job_Init()
{
	static bool done = false;
	if (!done)
	{
		done = true;
		GC::AddMarkerFunc([] { GC::Mark(runner); });
	}
	runnerclass = PClass::FindClass("ScreenJobRunner");
	if (!runnerclass) I_FatalError("ScreenJobRunner not defined");
	runnerclasstype = NewPointer(runnerclass);

	maprecordtype = NewPointer(NewStruct("MapRecord", nullptr, true));
	summaryinfotype = NewPointer(NewStruct("SummaryInfo", nullptr, true));
}

//=============================================================================
//
//
//
//=============================================================================

static VMFunction* LookupFunction(const char* qname, bool validate = true)
{
	int p = strcspn(qname, ".");
	if (p == 0) I_Error("Call to undefined function %s", qname);
	FString clsname(qname, p);
	FString funcname = qname + p + 1;

	auto func = PClass::FindFunction(clsname, funcname);
	if (func == nullptr) I_Error("Call to undefined function %s", qname);
	if (validate)
	{
		// these conditions must be met by all functions for this interface.
		if (func->Proto->ReturnTypes.Size() != 0) I_Error("Bad cutscene function %s. Return value not allowed", qname);
		if (func->ImplicitArgs != 0) I_Error("Bad cutscene function %s. Must be static", qname);
	}
	return func;
}

//=============================================================================
//
//
//
//=============================================================================

void CallCreateFunction(const char* qname, DObject* runner)
{
	auto func = LookupFunction(qname);
	if (func->Proto->ArgumentTypes.Size() != 1) I_Error("Bad cutscene function %s. Must receive precisely one argument.", qname);
	if (func->Proto->ArgumentTypes[0] != runnerclasstype) I_Error("Bad cutscene function %s. Must receive ScreenJobRunner reference.", qname);
	VMValue val = runner;
	VMCall(func, &val, 1, nullptr, 0);
}

//=============================================================================
//
//
//
//=============================================================================

void CallCreateMapFunction(const char* qname, DObject* runner, MapRecord* map)
{
	auto func = LookupFunction(qname);
	if (func->Proto->ArgumentTypes.Size() == 1) return CallCreateFunction(qname, runner);	// accept functions without map parameter as well here.
	if (func->Proto->ArgumentTypes.Size() != 2) I_Error("Bad map-cutscene function %s. Must receive precisely two arguments.", qname);
	if (func->Proto->ArgumentTypes[0] != runnerclasstype && func->Proto->ArgumentTypes[1] != maprecordtype) 
		I_Error("Bad cutscene function %s. Must receive ScreenJobRunner and MapRecord reference.", qname);
	VMValue val[2] = { runner, map };
	VMCall(func, val, 2, nullptr, 0);
}

//=============================================================================
//
//
//
//=============================================================================

void CallCreateSummaryFunction(const char* qname, DObject* runner, MapRecord* map, SummaryInfo* info, MapRecord* map2)
{
	auto func = LookupFunction(qname);
	auto s = func->Proto->ArgumentTypes.Size();
	auto at = func->Proto->ArgumentTypes.Data();
	if (s != 3 && s != 4) I_Error("Bad map-cutscene function %s. Must receive precisely three or four arguments.", qname);
	if (at[0] != runnerclasstype && at[1] != maprecordtype && at[2] != summaryinfotype && (s == 3 || at[3] == maprecordtype))
		I_Error("Bad cutscene function %s. Must receive ScreenJobRunner, MapRecord and SummaryInfo reference,", qname);
	if (info) summaryinfo = *info; // must be copied to a persistent location.
	else summaryinfo = {};
	VMValue val[] = { runner, map, &summaryinfo, map2 };
	VMCall(func, val, s, nullptr, 0);
}

//=============================================================================
//
//
//
//=============================================================================

DObject* CreateRunner(bool clearbefore = true)
{
	auto obj = runnerclass->CreateNew();
	auto func = LookupFunction("ScreenJobRunner.Init", false);
	VMValue val[3] = { obj, clearbefore, false };
	VMCall(func, val, 3, nullptr, 0);
	return obj;
}

//=============================================================================
//
//
//
//=============================================================================

void AddGenericVideo(DObject* runner, const FString& fn, int soundid, int fps)
{
	auto obj = runnerclass->CreateNew();
	auto func = LookupFunction("ScreenJobRunner.AddGenericVideo", false);
	VMValue val[] = { runner, &fn, soundid, fps };
	VMCall(func, val, 4, nullptr, 0);
}

//=============================================================================
//
//
//
//=============================================================================

void CutsceneDef::Create(DObject* runner)
{
	if (function.IsNotEmpty())
	{
		CallCreateFunction(function, runner);
	}
	else if (video.IsNotEmpty())
	{
		AddGenericVideo(runner, video, GetSound(), framespersec);
	}
}

//=============================================================================
//
//
//
//=============================================================================

bool CutsceneDef::Create(DObject* runner, MapRecord* map, bool transition)
{
	if (!transition && transitiononly) return false;
	if (function.CompareNoCase("none") == 0) 
		return true;	// play nothing but return as being validated
	if (function.IsNotEmpty())
	{
		CallCreateMapFunction(function, runner, map);
		return true;
	}
	else if (video.IsNotEmpty())
	{
		AddGenericVideo(runner, video, GetSound(), framespersec);
		return true;
	}
	return false;
}

//=============================================================================
//
//
//
//=============================================================================

void DeleteScreenJob()
{
	if (runner) runner->Destroy();
	runner = nullptr;
}

void EndScreenJob()
{
	DeleteScreenJob();
	if (completion) completion(false);
	completion = nullptr;
}


//=============================================================================
//
//
//
//=============================================================================

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
	FInputEvent evt = ev;
	if (runner)
	{
		IFVIRTUALPTRNAME(runner, NAME_ScreenJobRunner, OnEvent)
		{
			int result = 0;
			VMValue parm[] = { runner, &evt };
			VMReturn ret(&result);
			VMCall(func, parm, 2, &ret, 1);
			return result;
		}
	}
	return false;
}

//=============================================================================
//
//
//
//=============================================================================

bool ScreenJobTick()
{
	ticks++;
	if (runner)
	{
		IFVIRTUALPTRNAME(runner, NAME_ScreenJobRunner, OnTick)
		{
			int result = 0;
			VMValue parm[] = { runner };
			VMReturn ret(&result);
			VMCall(func, parm, 1, &ret, 1);
			return result;
		}
	}
	return false;
}

//=============================================================================
//
//
//
//=============================================================================

void ScreenJobDraw()
{
	double smoothratio = I_GetTimeFrac();

	if (runner)
	{
		twod->ClearScreen();
		IFVIRTUALPTRNAME(runner, NAME_ScreenJobRunner, RunFrame)
		{
			VMValue parm[] = { runner, smoothratio };
			VMCall(func, parm, 2, nullptr, 0);
		}
	}
}

//=============================================================================
//
//
//
//=============================================================================

bool ScreenJobValidate()
{
	if (runner)
	{
		IFVIRTUALPTRNAME(runner, NAME_ScreenJobRunner, Validate)
		{
			int res;
			VMValue parm[] = { runner };
			VMReturn ret(&res);
			VMCall(func, parm, 2, &ret, 1);
			return res;
		}
	}
	return false;
}

//=============================================================================
//
//
//
//=============================================================================

bool StartCutscene(CutsceneDef& cs, int flags, const CompletionFunc& completion_)
{
	if ((cs.function.IsNotEmpty() || cs.video.IsNotEmpty()) && cs.function.CompareNoCase("none") != 0)
	{
		completion = completion_;
		runner = CreateRunner();
		GC::WriteBarrier(runner);
		try
		{
			cs.Create(runner);
			if (!ScreenJobValidate())
			{
				runner->Destroy();
				runner = nullptr;
				return false;
			}
			if (flags & SJ_DELAY) intermissiondelay = 10;	// need to wait a bit at the start to let the timer catch up.
			else intermissiondelay = 0;
			gameaction = (flags & SJ_BLOCKUI) ? ga_intro : ga_intermission;
		}
		catch (...)
		{
			if (runner) runner->Destroy();
			runner = nullptr;
			throw;
		}
		return true;
	}
	return false;
}

bool StartCutscene(const char* s, int flags, const CompletionFunc& completion)
{
	CutsceneDef def;
	def.function = s;
	return StartCutscene(def, 0, completion);
}

//=============================================================================
//
//
//
//=============================================================================

void PlayLogos(gameaction_t complete_ga, gameaction_t def_ga, bool stopmusic)
{
	Mus_Stop();
	FX_StopAllSounds(); // JBF 20031228
	if (userConfig.nologo)
	{
		gameaction = def_ga;
	}
	else
	{
		if (!StartCutscene(globalCutscenes.Intro, SJ_BLOCKUI|SJ_DELAY, [=](bool) { 
			gameaction = complete_ga; 
			})) gameaction = def_ga;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void ShowScoreboard(int numplayers, const CompletionFunc& completion_)
{
	completion = completion_;
	runner = CreateRunner();
	GC::WriteBarrier(runner);

	const char* qname = globalCutscenes.MPSummaryScreen;
	auto func = LookupFunction(qname);
	if (func->Proto->ArgumentTypes.Size() != 2) I_Error("Bad map-cutscene function %s. Must receive precisely two arguments.", qname);
	if (func->Proto->ArgumentTypes[0] != runnerclasstype && func->Proto->ArgumentTypes[1] != TypeSInt32)
		I_Error("Bad cutscene function %s. Must receive ScreenJobRunner reference and integer.", qname);
	VMValue val[2] = { runner, numplayers };
	VMCall(func, val, 2, nullptr, 0);
	if (!ScreenJobValidate())
	{
		runner->Destroy();
		runner = nullptr;
		if (completion) completion(false);
		completion = nullptr;
		return;
	}
	gameaction = ga_intermission;
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

void ShowIntermission(MapRecord* fromMap, MapRecord* toMap, SummaryInfo* info, CompletionFunc completion_)
{
	completion = completion_;
	runner = CreateRunner();
	GC::WriteBarrier(runner);

	// retrieve cluster relations for cluster-based cutscenes.
	ClusterDef* fromcluster = nullptr, *tocluster = nullptr;
	if (fromMap) fromcluster = FindCluster(fromMap->cluster);
	if (toMap) tocluster = FindCluster(toMap->cluster);
	if (fromcluster == tocluster) fromcluster = tocluster = nullptr;


	try
	{
		if (fromMap)
		{
			if (!fromMap->outro.Create(runner, fromMap, !!toMap))
			{
				if (fromcluster == nullptr || !fromcluster->outro.Create(runner, fromMap, !!toMap))
					globalCutscenes.DefaultMapOutro.Create(runner, fromMap, !!toMap);
			}

		}
		if (fromMap || (g_gameType & GAMEFLAG_PSEXHUMED))
			CallCreateSummaryFunction(globalCutscenes.SummaryScreen, runner, fromMap, info, toMap);

		if (toMap)
		{
			if (!toMap->intro.Create(runner, toMap, !!fromMap))
			{
				if  (tocluster == nullptr || !tocluster->intro.Create(runner, toMap, !!fromMap))
					globalCutscenes.DefaultMapIntro.Create(runner, toMap, !!fromMap);
			}
			// Skip the load screen if the level is started from the console.
			// In this case the load screen is not helpful as it blocks the actual level start, 
			// requiring closing and reopening the console first before entering any commands that need the level.
			if (ConsoleState == c_up || ConsoleState == c_rising)
				globalCutscenes.LoadingScreen.Create(runner, toMap, true);
		}
		else if (isShareware())
		{
			globalCutscenes.SharewareEnd.Create(runner);
		}
		if (!ScreenJobValidate())
		{
			runner->Destroy();
			runner = nullptr;
			if (completion) completion(false);
			completion = nullptr;
			return;
		}
		gameaction = ga_intermission;
	}
	catch (...)
	{
		if (runner) runner->Destroy();
		runner = nullptr;
		throw;
	}
}

CCMD(testcutscene)
{
	if (argv.argc() < 2)
	{
		Printf("Usage: testcutscene <buildfunction>\n");
		return;
	}
	try
	{
		if (StartCutscene(argv[1], 0, [](bool) {}))
		{
			C_HideConsole();
		}
	}
	catch (const CRecoverableError& err)
	{
		Printf(TEXTCOLOR_RED "Unable to play cutscene: %s\n", err.what());
	}
}



/* 
Blood:
		if (!userConfig.nologo && gGameOptions.nGameType == 0) playlogos();
		else
		{
			gameaction = ga_mainmenu;
		}
	RunScreenJob(jobs, [](bool) {
		Mus_Stop();
		gameaction = ga_mainmenu;
		}, SJ_BLOCKUI);

Exhumed:
		if (!userConfig.nologo) DoTitle([](bool) { gameaction = ga_mainmenu; });
		else gameaction = ga_mainmenu;

SW:
		if (!userConfig.nologo) Logo([](bool)
			{
				gameaction = ga_mainmenunostopsound;
			});
		else gameaction = ga_mainmenu;

*/