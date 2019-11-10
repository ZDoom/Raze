#include "zmusic/zmusic.h"
#include "z_music.h"
#include "zstring.h"
#include "name.h"
#include "s_music.h"
#include "printf.h"
#include "files.h"
#include "filesystem.h"
#include "cmdlib.h"
#include "gamecvars.h"
#include "filereadermusicinterface.h"

MusPlayingInfo mus_playing;
MusicAliasMap MusicAliases;
MidiDeviceMap MidiDevices;


bool S_ChangeMusic(const char* musicname, int order, bool looping, bool force)
{
	if (musicname == nullptr || musicname[0] == 0)
	{
		// Don't choke if the map doesn't have a song attached
		//S_StopMusic(true);
		mus_playing.name = "";
		mus_playing.LastSong = "";
		return true;
	}

	FString DEH_Music;

	FName* aliasp = MusicAliases.CheckKey(musicname);
	if (aliasp != nullptr)
	{
		if (*aliasp == NAME_None)
		{
			return true;	// flagged to be ignored
		}
		musicname = aliasp->GetChars();
	}

	if (!mus_playing.name.IsEmpty() &&
		mus_playing.handle != nullptr &&
		stricmp(mus_playing.name, musicname) == 0 &&
		ZMusic_IsLooping(mus_playing.handle) == looping)
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
			try
			{
				ZMusic_Start(mus_playing.handle, looping, order);
				//S_CreateStream();
			}
			catch (const std::runtime_error & err)
			{
				Printf("Unable to start %s: %s\n", mus_playing.name.GetChars(), err.what());
			}

		}
		return true;
	}

	if (strnicmp(musicname, ",CD,", 4) == 0)
	{
		int track = strtoul(musicname + 4, nullptr, 0);
		const char* more = strchr(musicname + 4, ',');
		unsigned int id = 0;

		if (more != nullptr)
		{
			id = strtoul(more + 1, nullptr, 16);
		}
		//S_StopMusic(true);
		mus_playing.handle = ZMusic_OpenCDSong(track, id);
	}
	else
	{
		int lumpnum = -1;
		int length = 0;
		MusInfo* handle = nullptr;
		MidiDeviceSetting* devp = MidiDevices.CheckKey(musicname);

		// Strip off any leading file:// component.
		if (strncmp(musicname, "file://", 7) == 0)
		{
			musicname += 7;
		}

		FileReader reader;
		if (!FileExists(musicname))
		{
			if ((lumpnum = fileSystem.FindFile(musicname)) == -1)
			{
				Printf("Music \"%s\" not found\n", musicname);
				return false;
			}
			if (handle == nullptr)
			{
				if (fileSystem.FileLength(lumpnum) == 0)
				{
					return false;
				}
				reader = fileSystem.ReopenFileReader(lumpnum);
			}
		}
		else
		{
			// Load an external file.
			if (!reader.OpenFile(musicname))
			{
				return false;
			}
		}

		// shutdown old music
		//S_StopMusic(true);

		// Just record it if volume is 0
		if (mus_volume <= 0)
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
			try
			{
				auto mreader = new FileReaderMusicInterface(reader);
				mus_playing.handle = ZMusic_OpenSong(mreader, devp ? (EMidiDevice)devp->device : MDEV_DEFAULT, devp ? devp->args.GetChars() : "");
			}
			catch (const std::runtime_error & err)
			{
				Printf("Unable to load %s: %s\n", mus_playing.name.GetChars(), err.what());
			}
		}
	}

	mus_playing.loop = looping;
	mus_playing.name = musicname;
	mus_playing.baseorder = 0;
	mus_playing.LastSong = "";

	if (mus_playing.handle != 0)
	{ // play it
		try
		{
			//S_StartMusicPlaying(mus_playing.handle, looping, S_GetMusicVolume(musicname), order);
			//S_CreateStream();
			mus_playing.baseorder = order;
		}
		catch (const std::runtime_error & err)
		{
			Printf("Unable to start %s: %s\n", mus_playing.name.GetChars(), err.what());
		}
		return true;
	}
	return false;
}

void Mus_Play(const char* fn, bool loop)
{

}
void Mus_Stop()
{
}

void Mus_SetVolume(float vol)
{
}

void Mus_SetPaused(bool on)
{
}

