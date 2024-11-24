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
#include "s_soundinternal.h"
#include "i_music.h"

#include "gamecontrol.h"
#include <zmusic.h>

#include "raze_music.h"
#include "games/duke/src/sounds.h"


// MACROS ------------------------------------------------------------------

enum SICommands
{
	SI_Random,
	SI_MusicVolume,
	SI_MidiDevice,
	SI_MusicAlias,
	SI_ResourceID,
	SI_Alias,
	SI_Limit,
	SI_Singular,
	SI_PitchSet,
	SI_PitchSetDuke,
	SI_DukeFlags,
	SI_SWFlags,
	SI_Loop,
	SI_BloodRelVol,
	SI_Volume,
	SI_Attenuation,
	SI_Rolloff,
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
	"$random",
	"$musicvolume",
	"$mididevice",
	"$musicalias",
	"$resourceid",
	"$alias",
	"$limit",
	"$singular",
	"$pitchset",
	"$pitchsetduke",
	"$dukeflags",
	"$swflags",
	"$loop",
	"$bloodrelvol",
	"$volume",
	"$attenuation",
	"$rolloff",
	NULL
};

static const int DEFAULT_LIMIT = 6;
// CODE --------------------------------------------------------------------

//==========================================================================
//
// S_AddSound
//
// If logical name is already in S_sfx, updates it to use the new sound
// lump. Otherwise, adds the new mapping by using S_AddSoundLump().
//==========================================================================

static FSoundID S_AddSound(const char* logicalname, int lumpnum, FScanner* sc)
{
	FSoundID sfxid = soundEngine->FindSoundNoHash(logicalname);

	if (sfxid.isvalid())
	{ // If the sound has already been defined, change the old definition
		auto sfx = soundEngine->GetWritableSfx(sfxid);

		if (sfx->bRandomHeader)
		{
			FRandomSoundList* rnd = soundEngine->ResolveRandomSound(sfx);
			rnd->Choices.Reset();
			rnd->Owner = NO_SOUND;
		}
		sfx->lumpnum = lumpnum;
		sfx->bRandomHeader = false;
		sfx->link = sfxinfo_t::NO_LINK;
		sfx->bTentative = false;
		if (sfx->NearLimit == -1)
		{
			sfx->NearLimit = 6;
			sfx->LimitRange = 256 * 256;
		}
		//sfx->PitchMask = CurrentPitchMask;
	}
	else
	{ // Otherwise, create a new definition.
		sfxid = soundEngine->AddSoundLump(logicalname, lumpnum, 0);
	}

	return sfxid;
}

FSoundID S_AddSound(const char* logicalname, const char* lumpname, FScanner* sc)
{
	int lump = S_LookupSound(lumpname);
	if (lump == -1 && sc && fileSystem.GetFileContainer(sc->LumpNum) > fileSystem.GetMaxBaseNum())
		sc->ScriptMessage("%s: sound file not found", sc->String);
	return S_AddSound(logicalname, lump, sc);
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
	TArray<FSoundID> list;
	int wantassigns = -1;

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

		int cmd;
		if (sc.String[0] == '$') cmd = sc.MatchString(SICommandStrings);
		else cmd = -1;
		{ // Got a command
			switch (cmd)
			{
			case SI_Random: {
				// $random <logical name> { <logical name> ... }
				FRandomSoundList random;

				list.Clear();
				sc.MustGetString();
				FSoundID Owner = S_AddSound(sc.String, -1, &sc);
				sc.MustGetStringName("{");
				while (sc.GetString() && !sc.Compare("}"))
				{
					FSoundID sfxto = soundEngine->FindSoundTentative(sc.String);
					if (sfxto == random.Owner)
					{
						Printf("Definition of random sound '%s' refers to itself recursively.\n", sc.String);
						continue;
					}
					list.Push(sfxto);
				}
				if (list.Size() == 1)
				{ // Only one sound: treat as $alias
					auto sfxp = soundEngine->GetWritableSfx(Owner);
					sfxp->link = list[0];
					sfxp->NearLimit = -1;
				}
				else if (list.Size() > 1)
				{ // Only add non-empty random lists
					soundEngine->AddRandomSound(Owner, list);
				}
			}
				break;

			case SI_MusicVolume: {
				sc.MustGetString();
				int lumpnum = mus_cb.FindMusic(sc.String);
				if (!sc.CheckFloat())
				{
					sc.MustGetString();
					char* p;
					double f = strtod(sc.String, &p);
					if (!stricmp(p, "db")) sc.Float = dBToAmplitude((float)sc.Float);
					else sc.ScriptError("Bad value for music volume: %s", sc.String);
				}
				if (lumpnum >= 0) MusicVolumes[lumpnum] = (float)sc.Float;
				}
				break;

			case SI_MusicAlias: {
				sc.MustGetString();
				int mlump = fileSystem.FindFile(sc.String);
				if (mlump < 0)
					mlump = fileSystem.FindFile(FStringf("music/%s", sc.String).GetChars());
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
				int lumpnum = mus_cb.FindMusic(sc.String);
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
				if (lumpnum >= 0) MidiDevices[lumpnum] = devset;
				}
				break;

			case SI_Alias: {
				// $alias <name of alias> <name of real sound>
				FSoundID sfxfrom;

				sc.MustGetString ();
				sfxfrom = S_AddSound (sc.String, -1, &sc);
				sc.MustGetString ();
				auto sfx = soundEngine->GetWritableSfx(sfxfrom);
				sfx->link = soundEngine->FindSoundTentative (sc.String);
				sfx->NearLimit = -1;	// Aliases must use the original sound's limit. (Can be changed later)
				}
				break;

			case SI_Limit: {
				// $limit <logical name> <max channels> [<distance>]
				FSoundID sfxfrom;

				sc.MustGetString ();
				sfxfrom = soundEngine->FindSoundTentative (sc.String);
				sc.MustGetNumber ();
				auto sfx = soundEngine->GetWritableSfx(sfxfrom);
				sfx->NearLimit = min(max(sc.Number, 0), 255);
				if (sc.CheckFloat())
				{
					sfx->LimitRange = float(sc.Float * sc.Float);
				}
				}
				break;

			case SI_Singular: {
				// $singular <logical name>
				FSoundID sfx;

				sc.MustGetString ();
				sfx = soundEngine->FindSoundTentative (sc.String, DEFAULT_LIMIT);
				auto sfxp = soundEngine->GetWritableSfx(sfx);
				sfxp->bSingular = true;
				}
				break;

			case SI_Volume: {
				// $volume <logical name> <volume>
				FSoundID sfx;

				sc.MustGetString();
				sfx = soundEngine->FindSoundTentative(sc.String);
				sc.MustGetFloat();
				auto sfxp = soundEngine->GetWritableSfx(sfx);
				sfxp->Volume = (float)sc.Float;
			}
		  break;

			case SI_Attenuation: {
				// $attenuation <logical name> <attenuation>
				FSoundID sfx;

				sc.MustGetString();
				sfx = soundEngine->FindSoundTentative(sc.String);
				sc.MustGetFloat();
				auto sfxp = soundEngine->GetWritableSfx(sfx);
				sfxp->Attenuation = (float)sc.Float;
			}
		   break;

			case SI_Rolloff: {
				// $rolloff *|<logical name> [linear|log|custom] <min dist> <max dist/rolloff factor>
				// Using * for the name makes it the default for sounds that don't specify otherwise.
				FRolloffInfo* rolloff;
				int type;
				FSoundID sfx;

				sc.MustGetString();
				if (sc.Compare("*"))
				{
					sfx = INVALID_SOUND;
					rolloff = &soundEngine->GlobalRolloff();
				}
				else
				{
					sfx = soundEngine->FindSoundTentative(sc.String);
					auto sfxp = soundEngine->GetWritableSfx(sfx);
					rolloff = &sfxp->Rolloff;
				}
				type = ROLLOFF_Doom;
				if (!sc.CheckFloat())
				{
					sc.MustGetString();
					if (sc.Compare("linear"))
					{
						rolloff->RolloffType = ROLLOFF_Linear;
					}
					else if (sc.Compare("log"))
					{
						rolloff->RolloffType = ROLLOFF_Log;
					}
					else if (sc.Compare("custom"))
					{
						rolloff->RolloffType = ROLLOFF_Custom;
					}
					else
					{
						sc.ScriptError("Unknown rolloff type '%s'", sc.String);
					}
					sc.MustGetFloat();
				}
				rolloff->MinDistance = (float)sc.Float;
				sc.MustGetFloat();
				rolloff->MaxDistance = (float)sc.Float;
				break;
			}

			case SI_PitchSet: {
				// $pitchset <logical name> <pitch amount as float> [range maximum]
				FSoundID sfx;

				sc.MustGetString();
				sfx = soundEngine->FindSoundTentative(sc.String);
				sc.MustGetFloat();
				auto sfxp = soundEngine->GetWritableSfx(sfx);
				sfxp->DefPitch = (float)sc.Float;
				if (sc.CheckFloat())
				{
					sfxp->DefPitchMax = (float)sc.Float;
				}
				else
				{
					sfxp->DefPitchMax = 0;
				}
			}
			break;

			case SI_PitchSetDuke: {
				// $pitchset <logical name> <pitch amount as float> [range maximum]
				// Same as above, but uses Duke's value range of 1200 units per octave.
				FSoundID sfx;

				sc.MustGetString();
				sfx = soundEngine->FindSoundTentative(sc.String);
				sc.MustGetFloat();
				auto sfxp = soundEngine->GetWritableSfx(sfx);
				sfxp->DefPitch = (float)pow(2, sc.Float / 1200.);
				if (sc.CheckFloat())
				{
					sfxp->DefPitchMax = (float)pow(2, sc.Float / 1200.);
				}
				else
				{
					sfxp->DefPitchMax = 0;
				}
				break;
			}

			case SI_ResourceID: {
				// $resourceid <logical name> <resource id>
				// Assigns a resource ID to the given sound.
				sc.MustGetString();
				FSoundID sfx = soundEngine->FindSoundTentative(sc.String, DEFAULT_LIMIT);

				sc.MustGetNumber();
				// remove resource ID from any previously defined sound.
				for (unsigned i = 0; i < soundEngine->GetNumSounds(); i++)
				{
					auto sfxp = soundEngine->GetWritableSfx(FSoundID::fromInt(i));
					if (sfxp->ResourceId == sc.Number) sfxp->ResourceId = -1;

				}
				auto sfxp = soundEngine->GetWritableSfx(sfx);
				sfxp->ResourceId = sc.Number;
				break;
				}

			case SI_DukeFlags: {
				static const char* dukeflags[] = { "LOOP", "MSFX", "TALK", "GLOBAL", nullptr};

				// dukeflags <logical name> <flag> <flag> <flag>..
				sc.MustGetString();
				auto sfxid = soundEngine->FindSoundTentative(sc.String, DEFAULT_LIMIT);
				int flags = 0;
				while (sc.GetString())
				{
					int bit = sc.MatchString(dukeflags);
					if (bit == -1)
					{
						sc.UnGet();
						break;
					}
					flags |= 1 << bit;
				}
				if (isDukeEngine())
				{
					auto sfx = soundEngine->GetWritableSfx(sfxid);
					if (sfx->UserData.Size() < Duke3d::kMaxUserData)
					{
						sfx->UserData.Resize(Duke3d::kMaxUserData);
						memset(sfx->UserData.Data(), 0, Duke3d::kMaxUserData * sizeof(int));
					}
					sfx->UserVal = flags;
				}
				else
				{
					sc.ScriptMessage("'$dukeflags' is not available in the current game and will be ignored");
				}
				break;

			}

			case SI_SWFlags: {
				static const char* swflags[] = { "PLAYERVOICE", "PLAYERSPEECH", "LOOP", nullptr };

				// swflags <logical name> <flag> <flag> <flag>..
				sc.MustGetString();
				auto sfxid = soundEngine->FindSoundTentative(sc.String, DEFAULT_LIMIT);
				int flags = 0;
				while (sc.GetString())
				{
					int bit = sc.MatchString(swflags);
					if (bit == -1)
					{
						sc.UnGet();
						break;
					}
					flags |= 1 << bit;
				}
				if (isSWALL())
				{
					auto sfx = soundEngine->GetWritableSfx(sfxid);
					sfx->UserVal = flags;
				}
				else
				{
					sc.ScriptMessage("'$swflags' is not available in the current game and will be ignored");
				}
				break;

			}

			case SI_Loop: {
				// loop <logical name> <start> <end>
				// Sets loop points for the given sound in samples. Only really useful for WAV - for Ogg and FLAC use the metadata they can contain.
				sc.MustGetString();
				auto sfxid = soundEngine->FindSoundTentative(sc.String, DEFAULT_LIMIT);
				auto sfx = soundEngine->GetWritableSfx(sfxid);
				sc.MustGetNumber();
				sfx->LoopStart = sc.Number;
				if (sc.CheckNumber())
					sfx->LoopEnd = sc.Number;
				break;
			}

			case SI_BloodRelVol: {
				// bloodrelvol <logical name> <value>
				// Sets Blood's hacky volume modifier.
				sc.MustGetString();
				auto sfxid = soundEngine->FindSoundTentative(sc.String, DEFAULT_LIMIT);
				auto sfx = soundEngine->GetWritableSfx(sfxid);
				sc.MustGetNumber();
				if (isBlood())
				{
					auto sfx = soundEngine->GetWritableSfx(sfxid);
					sfx->UserVal = sc.Number;
				}
				else
				{
					sc.ScriptMessage("'$bloodrelvol' is not available in the current game and will be ignored");
				}
				break;
			}


			default:
			{ // Got a logical sound mapping
				if (sc.String[0] == '$')
				{
					sc.ScriptError("%s: Unknown keyword", sc.String);
				}
				FString name (sc.String);
				if (wantassigns == -1)
				{
					wantassigns = sc.CheckString("=");
				}
				else if (wantassigns)
				{
					sc.MustGetStringName("=");
				}

				sc.MustGetString ();
				S_AddSound (name.GetChars(), sc.String, &sc);
			}

			}

		}
	}
}

//==========================================================================
//
// S_ParseSndInfo
//
// Parses all loaded SNDINFO lumps.
//
//==========================================================================

void S_ParseSndInfo()
{
	int lump;

	soundEngine->Clear();
	MusicAliases.Clear();
	MidiDevices.Clear();
	MusicVolumes.Clear();

	S_AddSound("{ no sound }", "DSEMPTY", nullptr);	// Sound 0 is no sound at all

	int lastlump = 0;
	while ((lump = fileSystem.FindLump("SNDINFO", &lastlump, false)) != -1)
	{
		S_AddSNDINFO(lump);
	}
	soundEngine->HashSounds();
}

