/*
** zcompile.cpp
**
** Entry point for the script compiler
**
**---------------------------------------------------------------------------
** Copyright 2016-2020 Christoph Oelckers
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

#include "cmdlib.h"
#include "filesystem.h"
#include "sc_man.h"
#include "zcc_parser.h"
#include "zcc_compile_raze.h"
#include "codegen.h"
#include "stats.h"
#include "printf.h"
#include "dobject.h"
#include "coreactor.h"

void InitThingdef();
void SynthesizeFlagFields();

void ParseScripts()
{
	int lump, lastlump = 0;
	FScriptPosition::ResetErrorCounter();

	while ((lump = fileSystem.FindLump("ZSCRIPT", &lastlump)) != -1)
	{
		ZCCParseState state;
		auto newns = ParseOneScript(lump, state);
		PSymbolTable symtable;

		ZCCRazeCompiler cc(state, NULL, symtable, newns, lump, state.ParseVersion);
		cc.Compile();

		if (FScriptPosition::ErrorCounter > 0)
		{
			// Abort if the compiler produced any errors. Also do not compile further lumps, because they very likely miss some stuff.
			I_Error("%d errors, %d warnings while compiling %s", FScriptPosition::ErrorCounter, FScriptPosition::WarnCounter, fileSystem.GetFileFullPath(lump).GetChars());
		}
		else if (FScriptPosition::WarnCounter > 0)
		{
			// If we got warnings, but no errors, print the information but continue.
			Printf(TEXTCOLOR_ORANGE "%d warnings while compiling %s\n", FScriptPosition::WarnCounter, fileSystem.GetFileFullPath(lump).GetChars());
		}

	}
}

void LoadScripts()
{
	cycle_t timer;

	PType::StaticInit();
	InitThingdef();
	timer.Reset(); timer.Clock();
	FScriptPosition::ResetErrorCounter();

	FScriptPosition::StrictErrors = true;
	ParseScripts();

	FunctionBuildList.Build();

	if (FScriptPosition::ErrorCounter > 0)
	{
		I_Error("%d errors during script processing", FScriptPosition::ErrorCounter);
	}
	FScriptPosition::ResetErrorCounter();

	timer.Unclock();
	if (!batchrun) Printf("script parsing took %.2f ms\n", timer.TimeMS());
	SynthesizeFlagFields();

	for (int i = PClass::AllClasses.Size() - 1; i >= 0; i--)
	{
		auto ti = (PClassActor*)PClass::AllClasses[i];
		if (!ti->IsDescendantOf(RUNTIME_CLASS(DCoreActor))) continue;
		if (ti->Size == TentativeClass)
		{
			if (ti->bOptional)
			{
				Printf(TEXTCOLOR_ORANGE "Class %s referenced but not defined\n", ti->TypeName.GetChars());
				FScriptPosition::WarnCounter++;
				// the class must be rendered harmless so that it won't cause problems.
				ti->ParentClass = RUNTIME_CLASS(DCoreActor);
				ti->Size = sizeof(DCoreActor);
			}
			else
			{
				Printf(TEXTCOLOR_RED "Class %s referenced but not defined\n", ti->TypeName.GetChars());
				FScriptPosition::ErrorCounter++;
			}
			continue;
		}

		if (GetDefaultByType(ti) == nullptr)
		{
			Printf(TEXTCOLOR_RED "No ActorInfo defined for class '%s'\n", ti->TypeName.GetChars());
			FScriptPosition::ErrorCounter++;
			continue;
		}
	}

	// Now we may call the scripted OnDestroy method.
	PClass::bVMOperational = true;
}
