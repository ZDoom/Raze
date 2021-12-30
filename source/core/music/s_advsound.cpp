/*
** s_advsound.cpp
** Routines for managing SNDINFO lumps and ambient sounds
** Thid is a stripped down version only handling the music related settings.
**
**---------------------------------------------------------------------------
** Copyright 1998-2008 Randy Heit
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

// HEADER FILES ------------------------------------------------------------


#include "c_dispatch.h"
#include "filesystem.h"
#include "v_text.h"
#include "s_music.h"
#include "sc_man.h"
#include <zmusic.h>

#include "raze_music.h"

// MACROS ------------------------------------------------------------------

enum SICommands
{
	SI_MusicVolume,
	SI_MidiDevice,
	SI_MusicAlias,
};


// This specifies whether Timidity or Windows playback is preferred for a certain song (only useful for Windows.)
extern MusicAliasMap MusicAliases;
extern MidiDeviceMap MidiDevices;
extern MusicVolumeMap MusicVolumes;

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

extern bool IsFloat (const char *str);

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void S_AddSNDINFO (int lumpnum);


// PRIVATE DATA DEFINITIONS ------------------------------------------------

static const char *SICommandStrings[] =
{
	"$musicvolume",
	"$mididevice",
	"$musicalias",
	NULL
};


// CODE --------------------------------------------------------------------

//==========================================================================
//
// S_ParseSndInfo
//
// Parses all loaded SNDINFO lumps.
// Also registers Blood SFX files and Strife's voices.
//==========================================================================

void S_ParseSndInfo ()
{
	int lump, lastlump = 0;

	while ((lump = fileSystem.FindLumpFullName("engine/mussetting.txt", &lastlump)) >= 0)
	{
		S_AddSNDINFO (lump);
	}
}

//==========================================================================
//
// S_AddSNDINFO
//
// Reads a SNDINFO and does what it says.
//
//==========================================================================

static void S_AddSNDINFO (int lump)
{
	bool skipToEndIf;
	TArray<uint32_t> list;

	FScanner sc(lump);
	skipToEndIf = false;

	while (sc.GetString ())
	{
		if (skipToEndIf)
		{
			if (sc.Compare ("$endif"))
			{
				skipToEndIf = false;
			}
			continue;
		}

		if (sc.String[0] == '$')
		{ // Got a command
			switch (sc.MatchString (SICommandStrings))
			{
			case SI_MusicVolume: {
				sc.MustGetString();
				FName musname (sc.String);
				sc.MustGetFloat();
				MusicVolumes[musname] = (float)sc.Float;
				}
				break;

			case SI_MusicAlias: {
				sc.MustGetString();
				int mlump = fileSystem.FindFile(sc.String);
				if (mlump < 0)
					mlump = fileSystem.FindFile(FStringf("music/%s", sc.String));
				if (mlump >= 0)
				{
					// do not set the alias if a later WAD defines its own music of this name
					int file = fileSystem.GetFileContainer(mlump);
					int sndifile = fileSystem.GetFileContainer(sc.LumpNum);
					if (file > sndifile)
					{
						sc.MustGetString();
						continue;
					}
				}
				FName alias = sc.String;
				sc.MustGetString();
				FName mapped = sc.String;

				// only set the alias if the lump it maps to exists.
				if (mapped == NAME_None || fileSystem.FindFile(sc.String) >= 0)
				{
					MusicAliases[alias] = mapped;
				}
				}
				break;

			case SI_MidiDevice: {
				sc.MustGetString();
				FName nm = sc.String;
				FScanner::SavedPos save = sc.SavePos();

				sc.SetCMode(true);
				sc.MustGetString();
				MidiDeviceSetting devset;
				// Important: This needs to handle all the devices not present in ZMusic Lite to be able to load configs made for GZDoom.
				// Also let the library handle the fallback so this can adjust automatically if the feature set gets extended.
				if (sc.Compare("timidity")) devset.device = MDEV_TIMIDITY;
				else if (sc.Compare("fmod") || sc.Compare("sndsys")) devset.device = MDEV_SNDSYS;
				else if (sc.Compare("standard")) devset.device = MDEV_STANDARD;
				else if (sc.Compare("opl")) devset.device = MDEV_OPL;
				else if (sc.Compare("default")) devset.device = MDEV_DEFAULT;
				else if (sc.Compare("fluidsynth")) devset.device = MDEV_FLUIDSYNTH;
				else if (sc.Compare("gus")) devset.device = MDEV_GUS;
				else if (sc.Compare("wildmidi")) devset.device = MDEV_WILDMIDI;
				else if (sc.Compare("adl")) devset.device = MDEV_ADL;
				else if (sc.Compare("opn")) devset.device = MDEV_OPN;
				else sc.ScriptError("Unknown MIDI device %s\n", sc.String);
				if (sc.CheckString(","))
				{
					sc.SetCMode(false);
					sc.MustGetString();
					devset.args = sc.String;
				}
				else
				{
					// This does not really do what one might expect, because the next token has already been parsed and can be a '$'.
					// So in order to continue parsing without C-Mode, we need to reset and parse the last token again.
					sc.SetCMode(false);
					sc.RestorePos(save);
					sc.MustGetString();
				}
				MidiDevices[nm] = devset;
				}
				break;

			}
		}
	}
}

