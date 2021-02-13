/*
** def.cpp
** Rewritten .def parser free of Build license restrictions.
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
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OFf
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/  


#include "build.h"
#include "compat.h"

#include "mdsprite.h"  // md3model_t
#include "buildtiles.h"
#include "bitmap.h"
#include "m_argv.h"
#include "gamecontrol.h"
#include "palettecontainer.h"
#include "mapinfo.h"
#include "sc_man.h"





bool ParseDefFile(const char* file, FScanner *parent)
{
	int lump = fileSystem.FindFile(file);
	bool success = false;
	if (lump == -1)
	{
		if (!parent) Printf(PRINT_BOLD, "%sd: file not found\n", file);
		else parent->ScriptError("%s: file not found\n", file);
		return false;
	}
	FScanner sc;
	if (parent) sc.symbols = std::move(parent->symbols); // the child parser needs to add to the parent's symbol table so transfer its ownership for the parsing run.
	sc.OpenLumpNum(lump);
	sc.SetCMode(true);
	sc.SetNoOctals(true);
	sc.SetNoFatalErrors(true);
	FString str;

	while (sc.GetString())
	{	
		if (sc.Compare({"#include", "include"}))
		{
			if (!sc.MustGetString())
				ParseDefFile(sc.String, &sc);
		}
		else if (sc.Compare({"#includedefault", "includedefault"}))
		{
			ParseDefFile(G_DefaultDefFile(), &sc);
		}
		else if (sc.Compare({"#define", "define"}))
		{
			parseDefine(sc);
		}
		else if (sc.Compare("definetexture"))
		{
			parseDefineTexture(sc);
		}
		else if (sc.Compare("defineskybox"))
		{
            parseDefineSkybox(sc);
		}
		else if (sc.Compare("definetint"))
		{
            parseDefineTint(sc);
		}
		else if (sc.Compare("alphahack")) // why 'hack'?
		{
            parseAlphaHack(sc);
		}
		else if (sc.Compare("alphahackrange")) // why 'hack'?
		{
            parseAlphaHackRange(sc);
		}
		if (sc.Compare({"spritecol", "2dcolidxrange"))	// only used by Mapster32 so just read over them and ignore the result
		{
            parseDiscard<3>(sc);
		}
		else if (sc.Compare("2dcol")) // same here
		{
            parseDiscard<4>(sc);
		}
		else if (sc.Compare("fogpal"))
		{
            parseFogPal(sc);
		}
		else if (sc.Compare("nofloorpalrange"))
		{
            parseNoFloorpalRange(sc);
		}
		else if (sc.Compare("loadgrp"))
		{
            parseLoadGrp(sc);
		}
		else if (sc.Compare("cachesize") || sc.Compare("shadefactor"))
		{
            parseDiscard<1>(sc);
		}
		else if (sc.Compare("artfile"))
		{
            parseArtFile(sc);
		}

	}
	success = true;

	if (parent) parent->symbols = std::move(sc.symbols);
	return success;
}