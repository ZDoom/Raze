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
#include "screenjob_.h"
#include "i_time.h"
#include "v_2ddrawer.h"
#include "animlib.h"
#include "v_draw.h"
#include "s_soundinternal.h"
#include "animtexture.h"
#include "gamestate.h"
#include "razemenu.h"
#include "raze_sound.h"
#include "gamecontrol.h"
#include "raze_music.h"
#include "vm.h"
#include "mapinfo.h"

static PType* maprecordtype;
static PType* summaryinfotype;
static int ticks;
static SummaryInfo summaryinfo;

//=============================================================================
//
//
//
//=============================================================================

void Local_Job_Init()
{
	maprecordtype = NewPointer(NewStruct("MapRecord", nullptr, true));
	summaryinfotype = NewPointer(NewStruct("SummaryInfo", nullptr, true));
}
//=============================================================================
//
//
//
//=============================================================================

static void CallCreateMapFunction(const char* qname, DObject* runner, MapRecord* map)
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
	if (qname == nullptr || *qname == 0) return;	// no level summary defined.
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

bool CreateCutscene(CutsceneDef* cs, DObject* runner, MapRecord* map, bool transition)
{
	if (!transition && cs->transitiononly) return false;
	if (cs->function.CompareNoCase("none") == 0)
		return true;	// play nothing but return as being validated
	if (cs->function.IsNotEmpty())
	{
		CallCreateMapFunction(cs->function, runner, map);
		return true;
	}
	else if (cs->video.IsNotEmpty())
	{
		AddGenericVideo(runner, cs->video, cs->GetSound(), cs->framespersec);
		return true;
	}
	return false;
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
		if (!StartCutscene(globalCutscenes.Intro, SJ_BLOCKUI, [=](bool) { 
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
	Printf("Created runner at %p\n", runner);
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
	if (runner != nullptr) 
		return;	// protection against double exits.
	if (fromMap == toMap)
	{
		// don't show intermission when restarting the same level.
		completion_(false);
		return;
	}
	bool bossexit = g_bossexit;
	g_bossexit = false;

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
		if (fromMap && (!(fromMap->gameflags & LEVEL_BOSSONLYCUTSCENE) || bossexit))
		{
			if (!CreateCutscene(&fromMap->outro, runner, fromMap, !!toMap))
			{
				if (fromcluster == nullptr || !CreateCutscene(&fromcluster->outro, runner, fromMap, !!toMap))
					CreateCutscene(&globalCutscenes.DefaultMapOutro, runner, fromMap, !!toMap);
			}

		}
		if (fromMap || (g_gameType & GAMEFLAG_PSEXHUMED))
			CallCreateSummaryFunction(globalCutscenes.SummaryScreen, runner, fromMap, info, toMap);

		if (toMap) 
		{
			if (!CreateCutscene(&toMap->intro, runner, toMap, !!fromMap))
			{
				if  (tocluster == nullptr || !CreateCutscene(&tocluster->intro, runner, toMap, !!fromMap))
					CreateCutscene(&globalCutscenes.DefaultMapIntro, runner, toMap, !!fromMap);
			}
			// Skip the load screen if the level is started from the console or loading screens are disabled.
			// In this case the load screen is not helpful as it blocks the actual level start, 
			// requiring closing and reopening the console first before entering any commands that need the level.
			if ((ConsoleState == c_up || ConsoleState == c_rising) && cl_loadingscreens)
				CreateCutscene(&globalCutscenes.LoadingScreen, runner, toMap, true);
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
	