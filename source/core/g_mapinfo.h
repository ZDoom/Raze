/*
** g_level.h
**
**---------------------------------------------------------------------------
** Copyright 1998-2006 Randy Heit
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

#ifndef __G_LEVEL_H__
#define __G_LEVEL_H__

#include "autosegs.h"
#include "vectors.h"
#include "sc_man.h"
#include "file_zip.h"

struct FMapInfoParser
{
	FScanner sc;
	bool Internal;
	MapRecord* defaultinfoptr;

	FMapInfoParser(bool internal = false)
	{
		Internal = internal;
	}

	bool CheckLegacyMapDefinition(FString& mapname);
	bool ParseLookupName(FString &dest);
	void ParseMusic(FString &name, int &order);
	void ParseLumpOrTextureName(FString &name);

	void ParseCutscene(CutsceneDef& cdef);
	void ParseCluster();
	void ParseMapName(FString &mapname);
	MapRecord *ParseMapHeader(MapRecord &defaultinfo);
	void ParseMapDefinition(MapRecord &leveldef);
	void ParseEpisodeInfo ();
	void ParseCutsceneInfo();
	void ParseGameInfo();
	void ParseMapInfo (int lump, MapRecord &gamedefaults, MapRecord &defaultinfo);

	void ParseOpenBrace();
	bool ParseCloseBrace();
	bool CheckAssign();
	void ParseAssign();
	void MustParseAssign();
	void ParseComma();
	bool CheckNumber();
	bool CheckFloat();
	void SkipToNext();
	void CheckEndOfFile(const char *block);
};

#if defined(_MSC_VER)
#pragma section(SECTION_YREG,read)
#define MSVC_YSEG __declspec(allocate(SECTION_YREG))
#define GCC_YSEG
#else
#define MSVC_YSEG
#define GCC_YSEG __attribute__((section(SECTION_YREG))) __attribute__((used))
#endif 

#define DEFINE_MAP_OPTION(name, old) \
	static void MapOptHandler_##name(FMapInfoParser &parse, MapRecord *info); \
	static FMapOptInfo MapOpt_##name = \
		{ #name, MapOptHandler_##name, old }; \
	MSVC_YSEG FMapOptInfo *mapopt_##name GCC_YSEG = &MapOpt_##name; \
	static void MapOptHandler_##name(FMapInfoParser &parse, MapRecord *info)


struct FMapOptInfo
{
	const char *name;
	void (*handler) (FMapInfoParser &parse, MapRecord *levelinfo);
	bool old;
};


void G_ParseMapInfo();


#endif //__G_LEVEL_H__
