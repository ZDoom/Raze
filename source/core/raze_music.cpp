/*
**
** raze_music.cpp
** music player for Build games
**
** Copyright 2019-2020 Christoph Oelckers
**
**---------------------------------------------------------------------------
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

#include "raze_music.h"
#include "s_music.h"
#include "c_cvars.h"
#include "cmdlib.h"
#include "filesystem.h"
#include "files.h"
#include "i_music.h"

#include "gamecontrol.h"
#include "serializer.h"

static bool mus_blocked;
static FString lastStartedMusic;
TArray<FString> specialmusic;

MusicAliasMap MusicAliases;

CVAR(Bool, printmusicinfo, false, 0)
CVAR(Bool, mus_extendedlookup, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)


// Order is: streaming formats, module formats, emulated formats and MIDI formats - for external files the first one found wins so ambiguous names should be avoided
static const char* knownMusicExts[] = {
	"OGG",	"FLAC",	"MP3",	"MP2",	"XA",	"XM",	"MOD",
	"IT",	"S3M",	"MTM",	"STM",	"669",	"PTM",	"AMF",
	"OKT",	"DSM",	"AMFF",	"SPC",	"VGM",	"VGZ",	"AY",
	"GBS",	"GYM",	"HES",	"KSS",	"NSF",	"NSFE",	"SAP",
	"MID",	"HMP",	"HMI",	"XMI",	"VOC"
};


//==========================================================================
//
// Music file name lookup
//
//==========================================================================

FString G_SetupFilenameBasedMusic(const char* fn, const char* defmusic)
{
	FString name = StripExtension(fn);
	FString test;

	// Test if a real file with this name exists with all known extensions for music.
	for (auto& ext : knownMusicExts)
	{
		test.Format("%s.%s", name.GetChars(), ext);
		if (FileExists(test)) return test;
#ifdef __unix__
		test.Format("%s.%s", name.GetChars(), FString(ext).MakeLower().GetChars());
		if (FileExists(test)) return test;
#endif
	}
	return defmusic;
}

FString MusicFileExists(const char* fn)
{
	if (mus_extendedlookup) return G_SetupFilenameBasedMusic(fn, nullptr);
	if (FileExists(fn)) return fn;
	return FString();
}

int LookupMusic(const char* fn, bool onlyextended)
{
	if (mus_extendedlookup || onlyextended)
	{
		FString name = StripExtension(fn);
		int l = fileSystem.FindFileWithExtensions(name, knownMusicExts, countof(knownMusicExts));
		if (l >= 0 || onlyextended) return l;
	}
	return fileSystem.CheckNumForFullName(fn, true, ns_music);
}

//==========================================================================
//
// Music lookup in various places.
//
//==========================================================================

FileReader OpenMusic(const char* musicname)
{
	FileReader reader;
	if (!mus_restartonload)
	{
		// If the currently playing piece of music is the same, do not restart. Note that there's still edge cases where this may fail to detect identities.
		if (mus_playing.handle != nullptr && lastStartedMusic.CompareNoCase(musicname) == 0 && mus_playing.loop)
			return reader;
	}
	lastStartedMusic = musicname;	// remember the last piece of music that was requested to be played.

	FString mus = MusicFileExists(musicname);
	if (mus.IsNotEmpty())
	{
		// Load an external file.
		reader.OpenFile(mus);
	}
	if (!reader.isOpen())
	{
		int lumpnum = LookupMusic(musicname);
		if (mus_extendedlookup || lumpnum < 0)
		{
			if (lumpnum >= 0)
			{
				// EDuke also looks in a subfolder named after the main game resource. Do this as well if extended lookup is active.
				auto rfn = fileSystem.GetResourceFileName(fileSystem.GetFileContainer(lumpnum));
				auto rfbase = ExtractFileBase(rfn);
				FStringf aliasMusicname("music/%s/%s", rfbase.GetChars(), musicname);
				int newlumpnum = LookupMusic(aliasMusicname);
				if (newlumpnum >= 0) lumpnum = newlumpnum;
			}

			// Always look in the 'music' subfolder as well. This gets used by multiple setups to store ripped CD tracks.
			FStringf aliasMusicname("music/%s", musicname);
			int newlumpnum = LookupMusic(aliasMusicname, lumpnum >= 0);
			if (newlumpnum >= 0) lumpnum = newlumpnum;
		}

		if (lumpnum == -1 && (g_gameType & GAMEFLAG_SW))
		{
			// Some Shadow Warrior distributions have the music in a subfolder named 'classic'. Check that, too.
			FStringf aliasMusicname("classic/music/%s", musicname);
			lumpnum = fileSystem.FindFile(aliasMusicname);
		}
		if (lumpnum > -1)
		{
			if (fileSystem.FileLength(lumpnum) >= 0)
			{
				reader = fileSystem.ReopenFileReader(lumpnum);
				if (!reader.isOpen())
				{
					Printf(TEXTCOLOR_RED "Unable to play music " TEXTCOLOR_WHITE "\"%s\"\n", musicname);
				}
				else if (printmusicinfo) Printf("Playing music from file system %s:%s\n", fileSystem.GetResourceFileFullName(fileSystem.GetFileContainer(lumpnum)), fileSystem.GetFileFullPath(lumpnum).GetChars());
			}
		}
	}
	else if (printmusicinfo) Printf("Playing music from external file %s\n", musicname);
	return reader;
}


static FString LookupMusicCB(const char* musicname, int& order)
{
	// Now perform music aliasing. This also needs to be done before checking identities because multiple names can map to the same song.
	FName* aliasp = MusicAliases.CheckKey(musicname);
	if (aliasp != nullptr)
	{
		if (*aliasp == NAME_None)
		{
			return true;	// flagged to be ignored
		}
		return aliasp->GetChars();
	}
	return musicname;
}


static FString lastMusic;
int Mus_Play(const char *fn, bool loop)
{
	if (mus_blocked) return 1;	// Caller should believe it succeeded.
	if (*fn == '/') fn++;
	// Store the requested names for resuming.
	lastMusic = fn;
	
	return S_ChangeMusic(fn, 0, loop, true);
}

bool Mus_IsPlaying()
{
	return mus_playing.handle != nullptr;
}

void Mus_Stop()
{
	if (mus_blocked) return;
	S_StopMusic(true);
}

void Mus_SetPaused(bool on)
{
	if (on) S_PauseMusic();
	else S_ResumeMusic();
}

void Mus_Serialize(FSerializer &arc)
{
	if (arc.BeginObject("music"))
	{
		if (arc.isWriting())
		{
			FString music = mus_playing.name;
			if (music.IsEmpty()) music = mus_playing.LastSong;

			arc.AddString("music", music);
		}
		else arc("music", mus_playing.LastSong);

		arc("baseorder", mus_playing.baseorder)
			("loop", mus_playing.loop)
			.EndObject();

		// this is to prevent scripts from resetting the music after it has been loaded from the savegame.
		if (arc.isReading()) mus_blocked = true;
		// Actual music resuming cannot be performed here, it must be done in the game code.
	}
}

void Mus_ResumeSaved()
{
	S_RestartMusic();
}

void Mus_UpdateMusic()
{
	mus_blocked = false;
	S_UpdateMusic();
}

void Mus_InitMusic()
{
	I_InitMusic();
	static MusicCallbacks mus_cb =
	{
		LookupMusicCB,
		OpenMusic
	};
	S_SetMusicCallbacks(&mus_cb);
}
