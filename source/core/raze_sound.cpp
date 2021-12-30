/*
** s_sound.cpp
** Main sound engine
**
**---------------------------------------------------------------------------
** Copyright 1998-2016 Randy Heit
** Copyright 2002-2019 Christoph Oelckers
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

#include "raze_sound.h"
#include "c_cvars.h"
#include "basics.h"
#include "stats.h"
#include "v_text.h"
#include "filesystem.h"
#include "serializer.h"
#include "name.h"
#include "cmdlib.h"
#include "gamecontrol.h"
#include "build.h"
#include "coreactor.h"

extern ReverbContainer* ForcedEnvironment;
static int LastReverb;

CUSTOM_CVAR(Bool, snd_reverb, true, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)
{
	FX_SetReverb(-1);
}

// This is for testing reverb settings.
CUSTOM_CVAR(Int, snd_reverbtype, -1, 0)
{
	FX_SetReverb(-1);
}

void FX_SetReverb(int strength)
{
	if (strength == -1) strength = LastReverb;
	if (snd_reverbtype > -1) 
	{
		strength = snd_reverbtype;
		ForcedEnvironment = S_FindEnvironment(strength);
	}
	else if (snd_reverb && strength > 0)
	{
		// todo: optimize environments. The original "reverb" was garbage and not usable as reference.
		if (strength < 64) strength = 0x1400;
		else if (strength < 180) strength = 0x1503;
		else if (strength < 220) strength = 0x1502;
		else strength = 0x1900;
		LastReverb = strength;
		ForcedEnvironment = S_FindEnvironment(strength);
	}
	else ForcedEnvironment = nullptr;
}


//==========================================================================
//
// S_NoiseDebug
//
// [RH] Print sound debug info. Called by status bar.
//==========================================================================

FString NoiseDebug(SoundEngine *engine)
{
	FVector3 listener;
	FVector3 origin;

	listener = engine->GetListener().position;
	int ch = 0;

	FString out;

	out.Format("*** SOUND DEBUG INFO ***\nListener: %3.2f %2.3f %2.3f\n"
		"x     y     z     vol   dist  chan  pri   flags       aud   pos   name\n", listener.X, listener.Y, listener.Z);

	for (auto chan = engine->GetChannels(); chan; chan = chan->NextChan)
	{
		if (!(chan->ChanFlags & CHANF_IS3D))
		{
			out += "---   ---   ---   ---   ";
		}
		else
		{
			engine->CalcPosVel(chan, &origin, nullptr);
			out.AppendFormat(TEXTCOLOR_GOLD "%5.0f | %5.0f | %5.0f | %5.0f ", origin.X, origin.Z, origin.Y, (origin - listener).Length());
		}
		out.AppendFormat("%-.2g     %-4d  %-4d  %sF%s3%sZ%sU%sM%sN%sA%sL%sE%sV" TEXTCOLOR_GOLD " %-5.4f %-4u  %d: %s %p\n", chan->Volume, chan->EntChannel, chan->Priority,
			(chan->ChanFlags & CHANF_FORGETTABLE) ? TEXTCOLOR_GREEN : TEXTCOLOR_DARKRED,
			(chan->ChanFlags & CHANF_IS3D) ? TEXTCOLOR_GREEN : TEXTCOLOR_DARKRED,
			(chan->ChanFlags & CHANF_LISTENERZ) ? TEXTCOLOR_GREEN : TEXTCOLOR_DARKRED,
			(chan->ChanFlags & CHANF_UI) ? TEXTCOLOR_GREEN : TEXTCOLOR_DARKRED,
			(chan->ChanFlags & CHANF_MAYBE_LOCAL) ? TEXTCOLOR_GREEN : TEXTCOLOR_DARKRED,
			(chan->ChanFlags & CHANF_NOPAUSE) ? TEXTCOLOR_GREEN : TEXTCOLOR_DARKRED,
			(chan->ChanFlags & CHANF_AREA) ? TEXTCOLOR_GREEN : TEXTCOLOR_DARKRED,
			(chan->ChanFlags & CHANF_LOOP) ? TEXTCOLOR_GREEN : TEXTCOLOR_DARKRED,
			(chan->ChanFlags & CHANF_EVICTED) ? TEXTCOLOR_GREEN : TEXTCOLOR_DARKRED,
			(chan->ChanFlags & CHANF_VIRTUAL) ? TEXTCOLOR_GREEN : TEXTCOLOR_DARKRED,
			GSnd->GetAudibility(chan), GSnd->GetPosition(chan), ((int)chan->OrgID)-1, engine->GetSounds()[chan->SoundID].name.GetChars(), chan->Source);
		ch++;
	}
	out.AppendFormat("%d channels\n", ch);
	return out;
}

ADD_STAT(sounddebug)
{
	return NoiseDebug(soundEngine);
}

CVAR(Bool, snd_extendedlookup, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG)

int S_LookupSound(const char* fn)
{
	static const char * const sndformats[] = { "OGG", "FLAC", "WAV" };
	if (snd_extendedlookup)
	{
		auto newfn = StripExtension(fn);
		int lump = fileSystem.FindFileWithExtensions(newfn, sndformats, countof(sndformats));
		if (lump >= 0) return lump;
		newfn = "sound/" + newfn;
		lump = fileSystem.FindFileWithExtensions(newfn, sndformats, countof(sndformats));
		if (lump >= 0) return lump;
	}
	return fileSystem.FindFile(fn);
}

//==========================================================================
//
// Although saving the sound system's state is supposed to be an engine
// feature, the specifics cannot be set up there, this needs to be on the client side.
//
//==========================================================================

static FSerializer& Serialize(FSerializer& arc, const char* key, FSoundChan& chan, FSoundChan* def)
{
	if (arc.BeginObject(key))
	{
		arc("sourcetype", chan.SourceType)
			("soundid", chan.SoundID)
			("orgid", chan.OrgID)
			("volume", chan.Volume)
			("distancescale", chan.DistanceScale)
			("pitch", chan.Pitch)
			("chanflags", chan.ChanFlags)
			("entchannel", chan.EntChannel)
			("priority", chan.Priority)
			("nearlimit", chan.NearLimit)
			("starttime", chan.StartTime)
			("rolloftype", chan.Rolloff.RolloffType)
			("rolloffmin", chan.Rolloff.MinDistance)
			("rolloffmax", chan.Rolloff.MaxDistance)
			("limitrange", chan.LimitRange)
			("userdata", chan.UserData)
			.Array("point", chan.Point, 3);

		assert(dynamic_cast<RazeSoundEngine*>(soundEngine));

		auto eng = static_cast<RazeSoundEngine*>(soundEngine);
		// Let's handle actor sources here becaue they are the same for all games.
		if (eng->SourceIsActor(&chan))
		{
			DCoreActor* SourceIndex = nullptr;
			if (arc.isWriting()) SourceIndex = const_cast<DCoreActor*>(reinterpret_cast<const DCoreActor*>(chan.Source));
			arc("Source", SourceIndex);
			if (arc.isReading()) chan.Source = SourceIndex;
		}
		else
		{
			int SourceIndex = 0;
			if (arc.isWriting()) SourceIndex = eng->SoundSourceIndex(&chan);
			arc("Source", SourceIndex);
			if (arc.isReading()) eng->SetSource(&chan, SourceIndex);
		}

		arc.EndObject();
	}
	return arc;
}

//==========================================================================
//
// S_SerializeSounds
//
//==========================================================================

void S_SerializeSounds(FSerializer& arc)
{
	FSoundChan* chan;

	GSnd->Sync(true);

	if (arc.isWriting())
	{
		// Count channels and accumulate them so we can store them in
		// reverse order. That way, they will be in the same order when
		// reloaded later as they are now.
		TArray<FSoundChan*> chans = soundEngine->AllActiveChannels();

		if (chans.Size() > 0 && arc.BeginArray("sounds"))
		{
			for (unsigned int i = chans.Size(); i-- != 0; )
			{
				// Replace start time with sample position.
				uint64_t start = chans[i]->StartTime;
				chans[i]->StartTime = GSnd ? GSnd->GetPosition(chans[i]) : 0;
				arc(nullptr, *chans[i]);
				chans[i]->StartTime = start;
			}
			arc.EndArray();
		}
	}
	else
	{
		unsigned int count;

		soundEngine->StopAllChannels();
		if (arc.BeginArray("sounds"))
		{
			count = arc.ArraySize();
			for (unsigned int i = 0; i < count; ++i)
			{
				chan = (FSoundChan*)soundEngine->GetChannel(nullptr);
				arc(nullptr, *chan);
				// Sounds always start out evicted when restored from a save.
				chan->ChanFlags |= CHANF_EVICTED | CHANF_ABSTIME;
			}
			arc.EndArray();
		}
		// Add a small delay so that eviction only runs once the game is up and runnnig.
		soundEngine->SetRestartTime(I_GetTime() + 2);
	}
	// Check if there's actor sounds without an actor. This can happen if a savegame is badly timed with a freshly destroyed actor.
	soundEngine->EnumerateChannels([](FSoundChan* chan)
		{
			auto eng = static_cast<RazeSoundEngine*>(soundEngine);
			if (eng->SourceIsActor(chan) && chan->Source == nullptr)
			{
				eng->StopChannel(chan);
			}
			return 0;
		});

	GSnd->Sync(false);
	GSnd->UpdateSounds();
}

//==========================================================================
//
// CCMD playsound
//
//==========================================================================

CCMD(playsound)
{
	if (argv.argc() > 1)
	{
		FSoundID id = argv[1];
		if (id == 0)
		{
			Printf("'%s' is not a sound\n", argv[1]);
		}
		else
		{
			soundEngine->StartSound(SOURCE_None, nullptr, nullptr, CHAN_AUTO, CHANF_UI | CHANF_NOPAUSE, id, 1.f, ATTN_NORM);
		}
	}
}

//==========================================================================
//
// CCMD playsound
//
//==========================================================================

CCMD(playsoundid)
{
	if (argv.argc() > 1)
	{
		FSoundID id = soundEngine->FindSoundByResID((int)strtol(argv[1], nullptr, 0));
		if (id == 0)
		{
			Printf("'%s' is not a sound\n", argv[1]);
		}
		else
		{
			soundEngine->StartSound(SOURCE_None, nullptr, nullptr, CHAN_AUTO, CHANF_UI | CHANF_NOPAUSE, id, 1.f, ATTN_NORM);
		}
	}
}

//==========================================================================
//
// CCMD listsounds
//
//==========================================================================

CCMD(listsounds)
{
	auto& S_sfx = soundEngine->GetSounds();
	for (unsigned i = 0; i < S_sfx.Size(); i++)
	{
		Printf("%4d: name = %s, resId = %d, lumpnum = %d\n", i, S_sfx[i].name.GetChars(), S_sfx[i].ResourceId, S_sfx[i].lumpnum);
	}
}
