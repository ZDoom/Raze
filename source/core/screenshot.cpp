/*
** screenshot.cpp
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
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include "version.h"
#include "m_png.h"
#include "i_specialpaths.h"
#include "m_argv.h"
#include "cmdlib.h"
#include "gamecontrol.h"
#include "printf.h"
#include "c_dispatch.h"
#include "v_video.h"

EXTERN_CVAR(Float, png_gamma)
//
// screencapture
//

static FileWriter *opennextfile(const char *fn)
{
	static int count = 0;
	FString name;
    do
    {
		name.Format(fn, count++);
    } while (FileExists(name));
    return FileWriter::Open(name);
}



CVAR(String, screenshotname, "", CVAR_ARCHIVE)	// not GLOBALCONFIG - allow setting this per game.
CVAR(String, screenshot_dir, "", CVAR_ARCHIVE)					// same here.

//
// WritePNGfile
//
static void WritePNGfile(FileWriter* file, const uint8_t* buffer, const PalEntry* palette,
	ESSType color_type, int width, int height, int pitch, float gamma)
{
	FStringf software(GAMENAME " %s", GetVersionString());
	if (!M_CreatePNG(file, buffer, palette, color_type, width, height, pitch, gamma) ||
		!M_AppendPNGText(file, "Software", software) ||
		!M_FinishPNG(file))
	{
		Printf("Failed writing screenshot\n");
	}
}


static int SaveScreenshot()
{
	PalEntry Palette[256];

	size_t dirlen;
	FString autoname = Args->CheckValue("-shotdir");
	if (autoname.IsEmpty())
	{
		autoname = screenshot_dir;
	}
	dirlen = autoname.Len();
	if (dirlen == 0)
	{
		autoname = M_GetScreenshotsPath();
		dirlen = autoname.Len();
	}
	if (dirlen > 0)
	{
		if (autoname[dirlen - 1] != '/' && autoname[dirlen - 1] != '\\')
		{
			autoname += '/';
		}
	}
	autoname = NicePath(autoname);
	CreatePath(autoname);

	if (**screenshotname) autoname << screenshotname;
	else autoname << currentGame;
	autoname << "_%04d.png";
    FileWriter *fil = opennextfile(autoname);

	if (fil == nullptr)
    {
        return -1;
    }

	float gamma;
	int pitch;
	ESSType ctype;
	auto imgBuf = screen->GetScreenshotBuffer(pitch, ctype, gamma);
	WritePNGfile(fil, imgBuf.Data(), Palette, ctype, screen->GetWidth(), screen->GetHeight(), pitch, gamma);
	delete fil;
	Printf("screenshot saved\n");
    return 0;
}

CCMD(screenshot)
{
	SaveScreenshot();
}

