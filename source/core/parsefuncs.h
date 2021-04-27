
/*
** parsefuncs.h
** handlers for .def parser
** only to be included by the actual parser
**
**---------------------------------------------------------------------------
** Copyright 2021 Christoph Oelckers
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
**
*/

void parseAnimTileRange(FScanner& sc, FScriptPosition& pos)
{
	SetAnim set;
	if (!sc.GetNumber(set.tile1, true)) return;
	if (!sc.GetNumber(set.tile2, true)) return;
	if (!sc.GetNumber(set.speed, true)) return;
	if (!sc.GetNumber(set.type, true)) return;
	processSetAnim("animtilerange", pos, set);
}

//===========================================================================
//
//
//
//===========================================================================

static void parseCutscene(FScanner& sc, CutsceneDef& cdef)
{
	FScanner::SavedPos eblockend;

	if (sc.StartBraces(&eblockend)) return;
	FString sound;
	while (!sc.FoundEndBrace(eblockend))
	{
		sc.MustGetString();
		if (sc.Compare("video")) { sc.GetString(cdef.video); cdef.function = ""; }
		else if (sc.Compare("function")) { sc.GetString(cdef.function); cdef.video = ""; }
		else if (sc.Compare("sound")) sc.GetString(sound);
		else if (sc.Compare("fps")) sc.GetNumber(cdef.framespersec);
		else if (sc.Compare("transitiononly")) cdef.transitiononly = true;
		else if (sc.Compare("clear")) { cdef.function = "none"; cdef.video = ""; } // this means 'play nothing', not 'not defined'.
	}
	if (sound.IsNotEmpty())
	{
		cdef.sound = soundEngine->FindSound(sound);
		if (cdef.sound == 0)
		{
			int lump = fileSystem.FindFile(sound);
			if (lump < 0) return;
			cdef.sound = FSoundID(soundEngine->AddSoundLump(sound, lump, 0));
		}
	}
}

void parseDefineCutscene(FScanner& sc, FScriptPosition& pos)
{
	int scenenum = -1;

	if (!sc.GetString()) return;

	if (sc.Compare("intro"))
	{
		parseCutscene(sc, globalCutscenes.Intro);
	}
	else if (sc.Compare("mapintro")) // sets the global default for a map entry handler.
	{
		parseCutscene(sc, globalCutscenes.DefaultMapIntro);
	}
	else if (sc.Compare("mapoutro")) // sets the global default for a map exit handler.
	{
		parseCutscene(sc, globalCutscenes.DefaultMapOutro);
	}
	else if (sc.Compare("sharewareend")) // sets screens to be shown after the shareware version ends.
	{
		parseCutscene(sc, globalCutscenes.SharewareEnd);
	}
	else if (sc.Compare("loading")) // sets the loading screen when entering a level.
	{
		parseCutscene(sc, globalCutscenes.LoadingScreen);
	}
	else if (sc.Compare({ "episode", "volume", "cluster" }))
	{
		FScanner::SavedPos eblockend;
		sc.MustGetNumber();
		if (sc.Number < 1 || sc.Number > MAXVOLUMES)
		{
			sc.ScriptError("episode number %d out of range. Must be positive", sc.Number);
			return;
		}
		int vol = sc.Number - 1;

		if (sc.StartBraces(&eblockend)) return;
		while (!sc.FoundEndBrace(eblockend))
		{
			sc.MustGetString();
			if (sc.Compare("intro")) parseCutscene(sc, volumeList[vol].intro);
			else if (sc.Compare("outro")) parseCutscene(sc, volumeList[vol].outro);
			else if (sc.Compare("flags")) sc.GetNumber(volumeList[vol].flags);
		}
	}
	else if (sc.Compare("map"))
	{
		FScanner::SavedPos eblockend;
		sc.MustGetString();
		auto maprec = FindMapByName(sc.String);
		if (!maprec)
		{
			sc.ScriptError("%s: map not found", sc.String);
			return;
		}
		if (sc.StartBraces(&eblockend)) return;
		while (!sc.FoundEndBrace(eblockend))
		{
			sc.MustGetString();
			if (sc.Compare("intro")) parseCutscene(sc, maprec->intro);
			else if (sc.Compare("outro")) parseCutscene(sc, maprec->outro);
		}
	}
	else if (sc.Compare("summary")) sc.GetString(globalCutscenes.SummaryScreen);
	else if (sc.Compare("mpsummary")) sc.GetString(globalCutscenes.MPSummaryScreen);

}

