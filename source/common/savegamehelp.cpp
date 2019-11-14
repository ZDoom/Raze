/*
** savegame.cpp
** 
**---------------------------------------------------------------------------
** Copyright 2019 Christoph Oelckers
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
** This is for keeping my sanity while working with the horrible mess
** that is the savegame code in Duke Nukem.
** Without handling this in global variables it is a losing proposition
** to save custom data along with the regular snapshot. :(
** With this the savegame code can mostly pretend to load from and write
** to files while really using a composite archive.
*/  

#include "compositesaveame.h"
#include "savegamehelp.h"


static CompositeSavegameWriter savewriter;
static FResourceFile *savereader;

void OpenSaveGameForWrite(const char *name)
{
	savewriter.Clear();
	savewriter.SetFileName(name);
}

bool OpenSaveGameForRead(const char *name)
{
	if (savereader) delete savereader;
	savereader = FResourceFile::OpenResourceFile(name, true, true);
	return savereader != nullptr;
}

FileWriter *WriteSavegameChunk(const char *name)
{
	return &savewriter.NewElement(name);
}

FileReader ReadSavegameChunk(const char *name)
{
	if (!savereader) return FileReader();
	auto lump = savereader->FindLump(name);
	if (!lump) return FileReader();
	return lump->NewReader();
}

bool FinishSavegameWrite()
{
	return savewriter.WriteToFile();
}

void FinishSavegameRead()
{
	delete savereader;
	savereader = nullptr;
}
