/*
**
** music.cpp
**
** music engine - borrowed from GZDoom
**
** Copyright 1999-2016 Randy Heit
** Copyright 2002-2016 Christoph Oelckers
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

#include <zmusic.h>
#include "z_music.h"
#include "zstring.h"
#include "backend/i_sound.h"
#include "name.h"
#include "s_music.h"
#include "i_music.h"
#include "printf.h"
#include "files.h"
#include "filesystem.h"
#include "cmdlib.h"
#include "gamecvars.h"
#include "c_dispatch.h"
#include "gamecontrol.h"
#include "filereadermusicinterface.h"
#include "v_text.h"
#include "mapinfo.h"
#include "serializer.h"

MusPlayingInfo mus_playing;
MusicAliasMap MusicAliases;
MidiDeviceMap MidiDevices;
MusicVolumeMap MusicVolumes;
MusicAliasMap LevelMusicAliases;
bool MusicPaused;
static bool mus_blocked;
static FString lastStartedMusic;
EXTERN_CVAR(Float, mus_volume)
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

int LookupMusicLump(const char* fn)
{
	if (mus_extendedlookup)
	{
		FString name = StripExtension(fn);
		int l = fileSystem.FindFileWithExtensions(name, knownMusicExts, countof(knownMusicExts));
		if (l >= 0) return l;
	}
	return fileSystem.FindFile(fn);
}

//==========================================================================
//
// Music lookup in various places.
//
//==========================================================================

FileReader LookupMusic(const char* musicname)
{
	FileReader reader;
	FString mus = MusicFileExists(musicname);
	if (mus.IsNotEmpty())
	{
		// Load an external file.
		reader.OpenFile(mus);
	}
	if (!reader.isOpen())
	{
		int lumpnum = LookupMusicLump(musicname);
		if (mus_extendedlookup && lumpnum >= 0)
		{
			// EDuke also looks in a subfolder named after the main game resource. Do this as well if extended lookup is active.
			auto rfn = fileSystem.GetResourceFileName(fileSystem.GetFileContainer(lumpnum));
			auto rfbase = ExtractFileBase(rfn);
			FStringf aliasMusicname("music/%s/%s", rfbase.GetChars(), musicname);
			lumpnum = LookupMusicLump(aliasMusicname);
		}
		if (lumpnum == -1)
		{
			// Always look in the 'music' subfolder as well. This gets used by multiple setups to store ripped CD tracks.
			FStringf aliasMusicname("music/%s", musicname);
			lumpnum = LookupMusicLump(aliasMusicname);
		}
		if (lumpnum == -1 && (g_gameType & GAMEFLAG_SW))
		{
			// Some Shadow Warrioe distributions have the music in a subfolder named 'classic'. Check that, too.
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

//==========================================================================
//
// 
//
// Create a sound system stream for the currently playing song 
//==========================================================================

static std::unique_ptr<SoundStream> musicStream;

static bool FillStream(SoundStream* stream, void* buff, int len, void* userdata)
{
	bool written = ZMusic_FillStream(mus_playing.handle, buff, len);
	
	if (!written)
	{
		memset((char*)buff, 0, len);
		return false;
	}
	return true;
}


void S_CreateStream()
{
	if (!mus_playing.handle) return;
	SoundStreamInfo fmt;
	ZMusic_GetStreamInfo(mus_playing.handle, &fmt);
	if (fmt.mBufferSize > 0) // if buffer size is 0 the library will play the song itself (e.g. Windows system synth.)
	{
		int flags = fmt.mNumChannels < 0 ? 0 : SoundStream::Float;
		if (abs(fmt.mNumChannels) < 2) flags |= SoundStream::Mono;

		musicStream.reset(GSnd->CreateStream(FillStream, fmt.mBufferSize, flags, fmt.mSampleRate, nullptr));
		if (musicStream) musicStream->Play(true, 1);
	}
}

void S_PauseStream(bool paused)
{
	if (musicStream) musicStream->SetPaused(paused);
}

void S_StopStream()
{
	if (musicStream)
	{
		musicStream->Stop();
		musicStream.reset();
	}
}


//==========================================================================
//
// starts playing this song
//
//==========================================================================

static bool S_StartMusicPlaying(ZMusic_MusicStream song, bool loop, float rel_vol, int subsong)
{
	if (rel_vol > 0.f)
	{
		float factor = relative_volume / saved_relative_volume;
		saved_relative_volume = rel_vol;
		I_SetRelativeVolume(saved_relative_volume * factor);
	}
	ZMusic_Stop(song);
	if (!ZMusic_Start(song, subsong, loop))
	{
		return false;
	}

	// Notify the sound system of the changed relative volume
	mus_volume.Callback();
	return true;
}


//==========================================================================
//
// S_PauseSound
//
// Stop music and sound effects, during game PAUSE.
//==========================================================================

void S_PauseMusic ()
{
	if (mus_playing.handle && !MusicPaused)
	{
		ZMusic_Pause(mus_playing.handle);
		S_PauseStream(true);
		MusicPaused = true;
	}
}

//==========================================================================
//
// S_ResumeSound
//
// Resume music and sound effects, after game PAUSE.
//==========================================================================

void S_ResumeMusic ()
{
	if (mus_playing.handle && MusicPaused)
	{
		ZMusic_Resume(mus_playing.handle);
		S_PauseStream(false);
		MusicPaused = false;
	}
}

//==========================================================================
//
// S_UpdateSound
//
//==========================================================================

void S_UpdateMusic ()
{
	mus_blocked = false;
	if (mus_playing.handle != nullptr)
	{
		ZMusic_Update(mus_playing.handle);
		
		// [RH] Update music and/or playlist. IsPlaying() must be called
		// to attempt to reconnect to broken net streams and to advance the
		// playlist when the current song finishes.
		if (!ZMusic_IsPlaying(mus_playing.handle))
		{
			S_StopMusic(true);
		}
	}
}

//==========================================================================
//
// S_ChangeCDMusic
//
// Starts a CD track as music.
//==========================================================================

bool S_ChangeCDMusic (int track, unsigned int id, bool looping)
{
	char temp[32];

	if (id != 0)
	{
		mysnprintf (temp, countof(temp), ",CD,%d,%x", track, id);
	}
	else
	{
		mysnprintf (temp, countof(temp), ",CD,%d", track);
	}
	return S_ChangeMusic (temp, 0, looping);
}

//==========================================================================
//
// S_StartMusic
//
// Starts some music with the given name.
//==========================================================================

bool S_StartMusic (const char *m_id)
{
	return S_ChangeMusic (m_id, 0, false);
}

//==========================================================================
//
// S_ChangeMusic
//
// Starts playing a music, possibly looping.
//
// [RH] If music is a MOD, starts it at position order. If name is of the
// format ",CD,<track>,[cd id]" song is a CD track, and if [cd id] is
// specified, it will only be played if the specified CD is in a drive.
//==========================================================================

bool S_ChangeMusic(const char* musicname, int order, bool looping, bool force)
{
	lastStartedMusic = musicname;	// remember the last piece of music that was requested to be played.
	if (musicname == nullptr || musicname[0] == 0)
	{
		// Don't choke if the map doesn't have a song attached
		S_StopMusic (true);
		mus_playing.name = "";
		mus_playing.LastSong = "";
		return true;
	}
	if (*musicname == '/') musicname++;

	FString DEH_Music;

	if (!mus_playing.name.IsEmpty() &&
		mus_playing.handle != nullptr &&
		stricmp (mus_playing.name, musicname) == 0 &&
		ZMusic_IsLooping(mus_playing.handle) == zmusic_bool(looping))
	{
		if (order != mus_playing.baseorder)
		{
			if (ZMusic_SetSubsong(mus_playing.handle, order))
			{
				mus_playing.baseorder = order;
			}
		}
		else if (!ZMusic_IsPlaying(mus_playing.handle))
		{
			if (!ZMusic_Start(mus_playing.handle, order, looping))
			{
				Printf("Unable to start %s: %s\n", mus_playing.name.GetChars(), ZMusic_GetLastError());
			}
			S_CreateStream();

		}
		return true;
	}

	if (strnicmp (musicname, ",CD,", 4) == 0)
	{
		int track = strtoul (musicname+4, nullptr, 0);
		const char *more = strchr (musicname+4, ',');
		unsigned int id = 0;

		if (more != nullptr)
		{
			id = strtoul (more+1, nullptr, 16);
		}
		S_StopMusic (true);
		mus_playing.handle = ZMusic_OpenCDSong (track, id);
		if (mus_playing.handle == nullptr)
		{
			Printf("Unable to start CD Audio for track #%d, ID %d\n", track, id);
		}
	}
	else
	{
		int lumpnum = -1;
		int length = 0;
		ZMusic_MusicStream handle = nullptr;
		MidiDeviceSetting *devp = MidiDevices.CheckKey(musicname);

		// Strip off any leading file:// component.
		if (strncmp(musicname, "file://", 7) == 0)
		{
			musicname += 7;
		}

		FileReader reader = LookupMusic(musicname);
		if (!reader.isOpen()) return false;

		// shutdown old music
		S_StopMusic (true);

		// Just record it if volume is 0 or music was disabled
		if (mus_volume <= 0 || !mus_enabled)
		{
			mus_playing.loop = looping;
			mus_playing.name = musicname;
			mus_playing.baseorder = order;
			mus_playing.LastSong = musicname;
			return true;
		}

		// load & register it
		if (handle != nullptr)
		{
			mus_playing.handle = handle;
		}
		else
		{
			auto mreader = GetMusicReader(reader);	// this passes the file reader to the newly created wrapper.
			mus_playing.handle = ZMusic_OpenSong(mreader, devp? (EMidiDevice)devp->device : MDEV_DEFAULT, devp? devp->args.GetChars() : "");
			if (mus_playing.handle == nullptr)
			{
				Printf("Unable to load %s: %s\n", mus_playing.name.GetChars(), ZMusic_GetLastError());
			}
		}
	}

	mus_playing.loop = looping;
	mus_playing.name = musicname;
	mus_playing.baseorder = 0;
	mus_playing.LastSong = "";

	if (mus_playing.handle != 0)
	{ // play it
		auto volp = MusicVolumes.CheckKey(musicname);
		float vol = volp ? *volp : 1.f;
		if (!S_StartMusicPlaying(mus_playing.handle, looping, vol, order))
		{
			Printf("Unable to start %s: %s\n", mus_playing.name.GetChars(), ZMusic_GetLastError());
			return false;
		}
		S_CreateStream();
		mus_playing.baseorder = order;
		return true;
	}
	return false;
}

//==========================================================================
//
// S_RestartMusic
//
//==========================================================================

void S_RestartMusic ()
{
	if (!mus_playing.LastSong.IsEmpty() && mus_volume > 0 && mus_enabled)
	{
		FString song = mus_playing.LastSong;
		mus_playing.LastSong = "";
		S_ChangeMusic (song, mus_playing.baseorder, mus_playing.loop, true);
	}
	else
	{
		S_StopMusic(true);
	}
}

//==========================================================================
//
// S_MIDIDeviceChanged
//
//==========================================================================


void S_MIDIDeviceChanged(int newdev)
{
	auto song = mus_playing.handle;
	if (song != nullptr && ZMusic_IsMIDI(song) && ZMusic_IsPlaying(song))
	{
		// Reload the song to change the device
		auto mi = mus_playing;
		S_StopMusic(true);
		S_ChangeMusic(mi.name, mi.baseorder, mi.loop);
	}
}

//==========================================================================
//
// S_GetMusic
//
//==========================================================================

int S_GetMusic (const char **name)
{
	int order;

	if (mus_playing.name.IsNotEmpty())
	{
		*name = mus_playing.name;
		order = mus_playing.baseorder;
	}
	else
	{
		*name = nullptr;
		order = 0;
	}
	return order;
}

//==========================================================================
//
// S_StopMusic
//
//==========================================================================

void S_StopMusic (bool force)
{
	try
	{
		// [RH] Don't stop if a playlist is active.
		if (!mus_playing.name.IsEmpty())
		{
			if (mus_playing.handle != nullptr)
			{
				S_ResumeMusic();
				S_StopStream();
				ZMusic_Stop(mus_playing.handle);
				auto h = mus_playing.handle;
				mus_playing.handle = nullptr;
				ZMusic_Close(h);
			}
			mus_playing.LastSong = std::move(mus_playing.name);
		}
	}
	catch (const CRecoverableError& )
	{
		//Printf("Unable to stop %s: %s\n", mus_playing.name.GetChars(), err.what());
		if (mus_playing.handle != nullptr)
		{
			auto h = mus_playing.handle;
			mus_playing.handle = nullptr;
			ZMusic_Close(h);
		}
		mus_playing.name = "";
	}
}

//==========================================================================
//
// CCMD changemus
//
//==========================================================================

CCMD (changemus)
{
	if (MusicEnabled())
	{
		if (argv.argc() > 1)
		{
			S_ChangeMusic (argv[1], argv.argc() > 2 ? atoi (argv[2]) : 0);
		}
		else
		{
			const char *currentmus = mus_playing.name.GetChars();
			if(currentmus != nullptr && *currentmus != 0)
			{
				Printf ("currently playing %s\n", currentmus);
			}
			else
			{
				Printf ("no music playing\n");
			}
		}
	}
	else
	{
		Printf("Music is disabled\n");
	}
}

//==========================================================================
//
// CCMD stopmus
//
//==========================================================================

CCMD (stopmus)
{
	S_StopMusic (false);
	mus_playing.LastSong = "";	// forget the last played song so that it won't get restarted if some volume changes occur
}

static FString lastMusicLevel, lastMusic;
int Mus_Play(const char *mapname, const char *fn, bool loop)
{
	if (mus_blocked) return 1;	// Caller should believe it succeeded.
	// Store the requested names for resuming.
	lastMusicLevel = mapname;
	lastMusic = fn;
	
	if (!MusicEnabled())
	{
		return 0;
	}

	// Allow per level music substitution.
	// For most cases using $musicalias would be sufficient, but that method only works if a level actually has some music defined at all.
	// This way it can be done with an add-on definition lump even in cases like Redneck Rampage where no music definitions exist 
	// or where music gets reused for multiple levels but replacement is wanted individually.
	if (mapname && *mapname)
	{
		if (*mapname == '/') mapname++;
		FName *check = LevelMusicAliases.CheckKey(FName(mapname, true));
		if (check) fn = check->GetChars();
	}

	// Now perform music aliasing. This also needs to be done before checking identities because multiple names can map to the same song.
	FName* aliasp = MusicAliases.CheckKey(fn);
	if (aliasp != nullptr)
	{
		if (*aliasp == NAME_None)
		{
			return true;	// flagged to be ignored
		}
		fn = aliasp->GetChars();
	}

	if (!mus_restartonload)
	{
		// If the currently playing piece of music is the same, do not restart. Note that there's still edge cases where this may fail to detect identities.
		if (mus_playing.handle != nullptr && lastStartedMusic.CompareNoCase(fn) == 0 && mus_playing.loop)
			return true;
	}

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

void Mus_Fade(double seconds)
{
	// Todo: Blood uses this, but the streamer cannot currently fade the volume.
	Mus_Stop();
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


