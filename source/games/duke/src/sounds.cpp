//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2020 - Christoph Oelckers

This file is part of Enhanced Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms

EDuke enhancements integrated: 04/13/2003 - Matt Saettler

Note: EDuke source was in transition.  Changes are in-progress in the
source as it is released.

*/
//-------------------------------------------------------------------------


#include "ns.h"	// Must come before everything else!

#include "g_input.h"

#include "duke3d.h"
#include "dukeactor.h"
#include "raze_music.h"
#include "mapinfo.h"
#include "raze_sound.h"
#include "gamestate.h"
#include "names_d.h"
#include "i_music.h"
#include "vm.h"
#include "s_music.h"

CVAR(Bool, wt_forcemidi, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG) // quick hack to disable the oggs, which are of lower quality than playing the MIDIs with a good synth and sound font.
CVAR(Bool, wt_forcevoc, false, CVAR_ARCHIVE|CVAR_GLOBALCONFIG) // The same for sound effects. The re-recordings are rather poor and disliked
CVAR(Bool, wt_commentary, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)

BEGIN_DUKE_NS

int32_t g_cdTrack = -1;

static FSoundID currentCommentarySound;

void UnmuteSounds()
{
	soundEngine->EnumerateChannels([](FSoundChan* chan)
		{
			if (chan->UserData == 1)
				soundEngine->SetVolume(chan, chan->Volume * 4.f);
			chan->UserData = 0;
			return 0;
		});
}

void MuteSounds()
{
	soundEngine->EnumerateChannels([](FSoundChan* chan)
		{
			if (chan->UserData == 0)
				soundEngine->SetVolume(chan, chan->Volume * 0.25f);
			chan->UserData = 1;
			return 0;
		});
}

class DukeSoundEngine : public RazeSoundEngine
{
	// client specific parts of the sound engine go in this class.
	void CalcPosVel(int type, const void* source, const float pt[3], int channum, int chanflags, FSoundID chanSound, FVector3* pos, FVector3* vel, FSoundChan* chan) override;
	TArray<uint8_t> ReadSound(int lumpnum) override;

public:
	DukeSoundEngine()
	{
		S_Rolloff.RolloffType = ROLLOFF_Doom;   // Seems like Duke uses the same rolloff type as Doom.
		S_Rolloff.MinDistance = 144;            // was originally 576 which looks like a bug and sounds like crap.
		S_Rolloff.MaxDistance = 1088;

	}
	void StopChannel(FSoundChan* chan) override
 	{
		if (chan && chan->SysChannel != NULL && !(chan->ChanFlags & CHANF_EVICTED) && chan->SourceType == SOURCE_Actor)
		{
			chan->Source = NULL;
			chan->SourceType = SOURCE_Unattached;
		}
		auto sndid = chan->SoundID;
		SoundEngine::StopChannel(chan);
	}

	void SoundDone(FISoundChannel* ichan) override
	{
		FSoundChan* schan = static_cast<FSoundChan*>(ichan);

		if (schan != NULL && schan->SoundID == currentCommentarySound)
		{
			UnloadSound(schan->SoundID.index());
			currentCommentarySound = NO_SOUND;
			if (currentCommentarySprite) currentCommentarySprite->spr.picnum = DEVELOPERCOMMENTARY;
			I_SetRelativeVolume(1.0f);
			UnmuteSounds();
		}
		SoundEngine::SoundDone(schan);
	}

};

void GameInterface::StartSoundEngine()
{
	soundEngine = new DukeSoundEngine;
}

static FSoundID GetReplacementSound(FSoundID soundNum)
{
	if (wt_forcevoc && isWorldTour() && soundEngine->isValidSoundId(soundNum))
	{
		auto const* snd = soundEngine->GetUserData(soundNum);
		int sndx = snd[kWorldTourMapping];
		if (sndx > 0) soundNum = FSoundID::fromInt(sndx);
	}
	return soundNum;
}

//==========================================================================
//
//
// 
//==========================================================================

TArray<uint8_t> DukeSoundEngine::ReadSound(int lumpnum)
{
	auto wlump = fileSystem.OpenFileReader(lumpnum);
	return wlump.Read();
}

//==========================================================================
//
// 
//
//==========================================================================

void S_CacheAllSounds(void)
{
	for(unsigned i = 0; i < soundEngine->GetNumSounds(); i++)
	{
		soundEngine->CacheSound(FSoundID::fromInt(i));
		if ((i & 31) == 0)
			I_GetEvent();
	}
}

//==========================================================================
//
// 
//
//==========================================================================

static inline int S_GetPitch(FSoundID soundid)
{
	auto const* snd = soundEngine->GetUserData(soundid);
	if (!snd) return 0;
	int const   range = abs(snd[kPitchEnd] - snd[kPitchStart]);
	return (range == 0) ? snd[kPitchStart] : min(snd[kPitchStart], snd[kPitchEnd]) + rand() % range;
}

float S_ConvertPitch(int lpitch)
{
	return powf(2, lpitch / 1200.f);   // I hope I got this right that ASS uses a linear scale where 1200 is a full octave.
}

int S_GetUserFlags(FSoundID soundid)
{
	if (!soundEngine->isValidSoundId(soundid)) return 0;
	auto const* snd = soundEngine->GetUserData(soundid);
	if (!snd) return 0;
	return snd[kFlags];
}

//==========================================================================
//
// 
//
//==========================================================================

int S_DefineSound(unsigned index, const char *filename, int minpitch, int maxpitch, int priority, int type, int distance, float volume)
{
	FSoundID s_index = soundEngine->FindSoundByResIDNoHash(index); // cannot use the hash while editing.
	if (!s_index.isvalid())
	{
		// If the slot isn't defined, give it a meaningful name containing the index.
		s_index = soundEngine->FindSoundTentative(FStringf("ConSound@%04d", index));
	}
	auto sfx = soundEngine->GetWritableSfx(s_index);

	// Check if we are allowed to override this sound.
	// If it is still tentative, it's ok. Otherwise check if SF_CONDEFINED is set, This means it was defined by CON and may be overwritten.
	// If the sound was defined by other means we leave it alone, but set the flags to defaults, if they are not present.

	bool settable = sfx->bTentative;

	if (!settable)
	{
		if (sfx->UserData.Size() >= kMaxUserData)
		{
			auto& sndinf = sfx->UserData;
			settable = !!(sndinf[kFlags] & SF_CONDEFINED);
		}
	}
	if (!settable)
	{
		if (sfx->UserData.Size() < kMaxUserData)
		{
			// sound is defined but has no userdata. 
			// This means it was defined through SNDINFO without setting extended properties and should not be overwritten.
			// Set everything to 0 to have default handling.
			sfx->UserData.Resize(kMaxUserData);
			auto& sndinf = sfx->UserData;
			sndinf[kPitchStart] = 0;
			sndinf[kPitchEnd] = 0;
			sndinf[kPriority] = 0; // Raze's sound engine does not use this.
			sndinf[kVolAdjust] = 0;
			sndinf[kWorldTourMapping] = 0;
			sndinf[kFlags] = 0;
		}
	}

	sfx->ResourceId = index;
	sfx->UserData.Resize(kMaxUserData);
	auto& sndinf = sfx->UserData;
	sndinf[kFlags] = (type & ~SF_ONEINST_INTERNAL) | SF_CONDEFINED;
	if (sndinf[kFlags] & SF_LOOP)
		sndinf[kFlags] |= SF_ONEINST_INTERNAL;

	// Take care of backslashes in sound names. Also double backslashes which occur in World Tour.
	FString fn = filename;
	fn.Substitute("\\\\", "\\");
	FixPathSeperator(fn);
	sfx->lumpnum = S_LookupSound(fn);
	// For World Tour allow falling back on the classic sounds if the Oggs cannot be found
	if (isWorldTour() && sfx->lumpnum == -1)
	{
		fn.ToLower();
		fn.Substitute("sound/", "");
		fn.Substitute(".ogg", ".voc");
		sfx->lumpnum = S_LookupSound(fn);
	}
	sndinf[kPitchStart] = clamp<int>(minpitch, INT16_MIN, INT16_MAX);
	sndinf[kPitchEnd] = clamp<int>(maxpitch, INT16_MIN, INT16_MAX);
	sndinf[kPriority] = priority & 255;
	sndinf[kVolAdjust] = clamp<int>(distance, INT16_MIN, INT16_MAX);
	sndinf[kWorldTourMapping] = 0;
	sfx->Volume = volume;
	//sfx->NearLimit = index == TELEPORTER + 1? 6 : 0; // the teleporter sound cannot be unlimited due to how it gets used.
	sfx->bTentative = false;
	return 0;
}


inline bool S_IsAmbientSFX(DDukeActor* actor)
{
	return (issoundcontroller(actor) && actor->spr.lotag < 999);
}

//==========================================================================
//
// 
//
//==========================================================================

static int GetPositionInfo(DDukeActor* actor, FSoundID soundid, sectortype* sect,
							 const DVector3 &cam, const DVector3 &pos, int *distPtr, FVector3 *sndPos)
{
	// There's a lot of hackery going on here that could be mapped to rolloff and attenuation parameters.
	// However, ultimately rolloff would also just reposition the sound source so this can remain as it is.

	int orgsndist = 0, sndist = 0;
	auto const* snd = soundEngine->GetUserData(soundid);
	int userflags = snd ? snd[kFlags] : 0;
	int dist_adjust = snd ? snd[kVolAdjust] : 0;

	FVector3 sndorg = GetSoundPos(pos);
	FVector3 campos = GetSoundPos(cam);

	if (!actor->isPlayer() || actor->PlayerIndex() != screenpeek)
	{
		orgsndist = sndist = int(16 * (sndorg - campos).Length());

		if ((userflags & (SF_GLOBAL | SF_DTAG)) != SF_GLOBAL && issoundcontroller(actor) && actor->spr.lotag < 999 && (actor->sector()->lotag & 0xff) < ST_9_SLIDING_ST_DOOR)
			sndist = DivScale(sndist, actor->spr.hitag + 1, 14);
	}

	sndist += dist_adjust;
	if (sndist < 0) sndist = 0;

	if (sect!= nullptr && sndist && !issoundcontroller(actor) && !cansee(cam.plusZ(-24), sect, actor->spr.pos.plusZ(-24), actor->sector()))
		sndist += sndist >> (isRR() ? 2 : 5);

	// Here the sound distance was clamped to a minimum of 144*4. 
	// It's better to handle rolloff in the backend instead of whacking the sound origin here.
	// That way the lower end can be made customizable instead of losing all precision right here at the source.
	if (sndist < 0) sndist = 0;

	if (distPtr)
	{
		*distPtr = sndist;
	}

	if (sndPos)
	{
		// Now calculate the virtual position in sound system coordinates.
		FVector3 sndvec = sndorg - campos;
		if (orgsndist > 0)
		{
			float scale = float(sndist) / orgsndist;   // adjust by what was calculated above;
			*sndPos = campos + sndvec * scale;
		}
		else *sndPos = campos;
	}

	return false;
}

//==========================================================================
//
//
//
//==========================================================================

void S_GetCamera(DVector3* c, DAngle* ca, sectortype** cs)
{
	if (ud.cameraactor == nullptr)
	{
		auto p = &ps[screenpeek];
		if (c)
		{
			if (p->GetActor()) *c = p->GetActor()->getPosWithOffsetZ();
			else  c->Zero();
		}
		if (cs) *cs = p->cursector;
		if (ca) *ca = p->Angles.ZzANGLE();
	}
	else
	{
		if (c) *c =  ud.cameraactor->spr.pos;
		if (cs) *cs = ud.cameraactor->sector();
		if (ca) *ca = ud.cameraactor->spr.Angles.Yaw;
	}
}

//=========================================================================
//
// CalcPosVel
//
// The game specific part of the sound updater.
//
//=========================================================================

void DukeSoundEngine::CalcPosVel(int type, const void* source, const float pt[3], int channum, int chanflags, FSoundID chanSound, FVector3* pos, FVector3* vel, FSoundChan* chan)
{
	if (pos != nullptr)
	{
		DVector3 campos;
		sectortype* camsect;

		S_GetCamera(&campos, nullptr, &camsect);
		if (vel) vel->Zero();

		if (type == SOURCE_Unattached)
		{
			pos->X = pt[0];
			pos->Y = pt[1];
			pos->Z = pt[2];
		}
		else if (type == SOURCE_Actor)
		{
			auto aactor = (DDukeActor*)source;
			if (aactor != nullptr)
			{
				GetPositionInfo(aactor, chanSound, camsect, campos, aactor->spr.pos, nullptr, pos);
				/*
				if (vel) // DN3D does not properly maintain this.
				{
					vel->X = float(actor->Vel.X * TICRATE);
					vel->Y = float(actor->Vel.Z * TICRATE);
					vel->Z = float(actor->Vel.Y * TICRATE);
				}
				*/
			}
		}
		if ((chanflags & CHANF_LISTENERZ) && type != SOURCE_None)
		{
			pos->Y = (float)campos.Z / 256.f;
		}
	}
}


//==========================================================================
//
//
//
//==========================================================================

void GameInterface::UpdateSounds(void)
{
	SoundListener listener;
	DVector3 c;
	DAngle ca;
	sectortype* cs;

	if (isRR() && !Mus_IsPlaying() && !paused && gamestate == GS_LEVEL)
		S_PlayRRMusic(); 

	S_GetCamera(&c, &ca, &cs);

	listener.angle = -float(ca.Radians());
	listener.velocity.Zero();
	listener.position = GetSoundPos(c);
	listener.underwater = false; 
	// This should probably use a real environment instead of the pitch hacking in S_PlaySound3D.
	// listenactor->waterlevel == 3;
	//assert(primaryLevel->Zones.Size() > listenactor->Sector->ZoneNumber);
	listener.Environment = 0;// primaryLevel->Zones[listenactor->Sector->ZoneNumber].Environment;
	listener.valid = true;

	listener.ListenerObject = ud.cameraactor == nullptr ? nullptr : ud.cameraactor.Get();
	soundEngine->SetListener(listener);
}


//==========================================================================
//
//
//
//==========================================================================

int S_PlaySound3D(FSoundID soundid, DDukeActor* actor, const DVector3& pos, int channel, EChanFlags flags)
{
	auto const pl = &ps[myconnectindex];
	if (!soundEngine->isValidSoundId(soundid) || !SoundEnabled() || actor == nullptr || !playrunning() ||
		(pl->timebeforeexit > 0 && pl->timebeforeexit <= REALGAMETICSPERSEC * 3)) return -1;

	soundid = GetReplacementSound(soundid);
	int userflags = S_GetUserFlags(soundid);

	if ((userflags & (SF_DTAG | SF_GLOBAL)) == SF_DTAG)
	{
		// Duke-Tag sound does not play in 3D.
		return S_PlaySound(soundid);
	}

	if (userflags & SF_TALK)
	{
		if (snd_speech == 0 || (ud.multimode > 1 && actor->isPlayer() && actor->PlayerIndex() != screenpeek && ud.coop != 1)) return -1;
		bool foundone =  soundEngine->EnumerateChannels([&](FSoundChan* chan)
			{
				auto sid = chan->OrgID;
				auto flags = S_GetUserFlags(sid);
				return !!(flags & SF_TALK);
			});
		// don't play if any Duke talk sounds are already playing
		if (foundone) return -1;

		// When in single player, force all talk sounds to originate from the player actor, no matter what is being used to start them.
		// Fixes a problem with quake06.voc in E3L4.
		if (ud.multimode == 1)
		{
			actor = pl->GetActor();
		}
	}

	int32_t sndist;
	FVector3 sndpos;    // this is in sound engine space.

	DVector3 campos;
	sectortype* camsect;

	S_GetCamera(&campos, nullptr, &camsect);
	GetPositionInfo(actor, soundid, camsect, campos, pos, &sndist, &sndpos);
	int pitch = S_GetPitch(soundid);

	auto sfx = soundEngine->GetSfx(soundid);
	bool explosion = ((userflags & (SF_GLOBAL | SF_DTAG)) == (SF_GLOBAL | SF_DTAG)) || 
		((sfx->ResourceId == PIPEBOMB_EXPLODE || sfx->ResourceId == LASERTRIP_EXPLODE || sfx->ResourceId == RPG_EXPLODE));

	bool underwater = ps[screenpeek].insector() && ps[screenpeek].cursector->lotag == ST_2_UNDERWATER;
	if (explosion)
	{
		if (underwater)
			pitch -= 1024;
	}
	else
	{
		if (sndist > 32767 && !issoundcontroller(actor) && (userflags & (SF_LOOP | SF_MSFX)) == 0)
			return -1;

		if (underwater && (userflags & SF_TALK) == 0)
			pitch = -768;
	}

	bool is_playing = soundEngine->GetSoundPlayingInfo(SOURCE_Any, nullptr, soundid);
	if (is_playing && !issoundcontroller(actor))
		S_StopSound(soundid, actor);

	int const repeatp = (userflags & SF_LOOP);

	if (repeatp && (userflags & SF_ONEINST_INTERNAL) && is_playing)
	{
		return -1;
	}

	// These explosion sounds originally used some distance hackery to make them louder but due to how the rolloff was set up they always played at full volume as a result.
	// I think it is better to lower their attenuation so that they are louder than the rest but still fade in the distance.
	// For the original effect, attenuation needs to be set to ATTN_NONE here.
	float attenuation;
	if (explosion) attenuation = 0.5f;
	else attenuation = (userflags & (SF_GLOBAL | SF_DTAG)) == SF_GLOBAL ? ATTN_NONE : ATTN_NORM;

	if (userflags & SF_LOOP) flags |= CHANF_LOOP;
	float vol = attenuation == ATTN_NONE ? 0.8f : 1.f;
	if (currentCommentarySound != NO_SOUND) vol *= 0.25f;
	auto chan = soundEngine->StartSound(SOURCE_Actor, actor, &sndpos, CHAN_AUTO, flags, soundid, vol, attenuation, nullptr, S_ConvertPitch(pitch));
	if (chan) chan->UserData = (currentCommentarySound != NO_SOUND);
	return chan ? 0 : -1;
}

//==========================================================================
//
//
//
//==========================================================================

int S_PlaySound(FSoundID soundid, int channel, EChanFlags flags, float vol)
{
	if (!soundEngine->isValidSoundId(soundid) || !SoundEnabled()) return -1;

	soundid = GetReplacementSound(soundid);

	int userflags = S_GetUserFlags(soundid);
	if ((!(snd_speech & 1) && (userflags & SF_TALK)))
		return -1;

	int const pitch = S_GetPitch(soundid);

	if (userflags & SF_LOOP) flags |= CHANF_LOOP;
	if (currentCommentarySound != NO_SOUND) vol *= 0.25f;
	auto chan = soundEngine->StartSound(SOURCE_None, nullptr, nullptr, channel, flags, soundid, vol, ATTN_NONE, nullptr, S_ConvertPitch(pitch));
	if (chan) chan->UserData = (currentCommentarySound != NO_SOUND);
	return chan ? 0 : -1;
}

//==========================================================================
//
//
//
//==========================================================================

int S_PlayActorSound(FSoundID soundNum, DDukeActor* actor, int channel, EChanFlags flags)
{
	return (actor == nullptr ? S_PlaySound(soundNum, channel, flags) :
		S_PlaySound3D(soundNum, actor, actor->spr.pos, channel, flags));
}

void S_StopSound(FSoundID soundid, DDukeActor* actor, int channel)
{
	soundid = GetReplacementSound(soundid);

	if (!actor) soundEngine->StopSoundID(soundid);
	else
	{
		if (channel == -1) soundEngine->StopSound(SOURCE_Actor, actor, -1, soundid);
		else soundEngine->StopSound(SOURCE_Actor, actor, channel);

		// StopSound kills the actor reference so this cannot be delayed until ChannelEnded gets called. At that point the actor may also not be valid anymore.
		if (S_IsAmbientSFX(actor) && actor->sector()->lotag < 3)  // ST_2_UNDERWATER
			actor->temp_data[0] = 0;
	}
}

void S_ChangeSoundPitch(FSoundID soundid, DDukeActor* actor, int pitchoffset)
{
	double expitch = pow(2, pitchoffset / 1200.);   // I hope I got this right that ASS uses a linear scale where 1200 is a full octave.
	if (!actor)
	{
		soundEngine->ChangeSoundPitch(SOURCE_Unattached, nullptr, CHAN_AUTO, expitch, soundid);
	}
	else
	{
		soundEngine->ChangeSoundPitch(SOURCE_Actor, actor, CHAN_AUTO, expitch, soundid);
	}
}

//==========================================================================
//
//
//
//==========================================================================

int S_CheckActorSoundPlaying(DDukeActor* actor, FSoundID soundid, int channel)
{
	soundid = GetReplacementSound(soundid);

	if (actor == nullptr) return soundEngine->GetSoundPlayingInfo(SOURCE_Any, nullptr, soundid);
	return soundEngine->IsSourcePlayingSomething(SOURCE_Actor, actor, channel, soundid);
}

// Check if actor <i> is playing any sound.
int S_CheckAnyActorSoundPlaying(DDukeActor* actor)
{
	if (!actor) return false;
	return soundEngine->IsSourcePlayingSomething(SOURCE_Actor, actor, CHAN_AUTO);
}

int S_CheckSoundPlaying(FSoundID soundid)
{
	return soundEngine->GetSoundPlayingInfo(SOURCE_Any, nullptr, soundid);
}

//==========================================================================
//
//
//
//==========================================================================

void S_MenuSound(void)
{
	static int menunum;
	static const uint16_t menusnds[] =
	{
		LASERTRIP_EXPLODE,
		DUKE_GRUNT,
		DUKE_LAND_HURT,
		CHAINGUN_FIRE,
		SQUISHED,
		KICK_HIT,
		PISTOL_RICOCHET,
		PISTOL_BODYHIT,
		PISTOL_FIRE,
		SHOTGUN_FIRE,
		BOS1_WALK,
		RPG_EXPLODE,
		PIPEBOMB_BOUNCE,
		PIPEBOMB_EXPLODE,
		NITEVISION_ONOFF,
		RPG_SHOOT,
		SELECT_WEAPON
	};
	int s = isRR() ? 390 : menusnds[menunum++ % countof(menusnds)];
	if (s != -1)
		S_PlaySound(s, CHAN_AUTO, CHANF_UI);
}

//==========================================================================
//
// Music
//
//==========================================================================

static bool cd_disabled = false;    // This is in case mus_redbook is enabled but no tracks found so that the regular music system can be switched on.

static void MusPlay(const char* music, bool loop)
{
	if (isWorldTour())
	{
		if (wt_forcemidi)
		{
			FString alternative = music;
			alternative.Substitute(".ogg", ".mid");
			int num = fileSystem.FindFile(alternative);
			if (num >= 0)
			{
				int file = fileSystem.GetFileContainer(num);
				if (file == 1)
				{
					Mus_Play(alternative, loop);
					return;
				}
			}
		}
	}
	int result = Mus_Play(music, loop);
	// do not remain silent if playing World Tour when the user has deleted the music.
	if (!result && isWorldTour())
	{
		FString alternative = music;
		alternative.Substitute(".ogg", ".mid");
		Mus_Play(alternative, loop);
	}
}

void S_PlayLevelMusic(MapRecord *mi)
{
	if (isRR() && mi->music.IsEmpty() && mus_redbook && !cd_disabled) return;
	MusPlay(mi->music, true);
}

void S_PlaySpecialMusic(unsigned int m)
{
	if (isRR() || m >= specialmusic.Size()) return;   // Can only be MUS_LOADING, isRR() does not use it.
	auto& musicfn = specialmusic[m];
	if (musicfn.IsNotEmpty())
	{
		MusPlay(musicfn, true);
	}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void S_PlayRRMusic(int newTrack)
{
	if (!isRR() || !mus_redbook || cd_disabled || currentLevel->music.IsNotEmpty())
		return;
	Mus_Stop();

	for (int i = 0; i < 10; i++)
	{
		g_cdTrack = newTrack != -1 ? newTrack : g_cdTrack + 1;
		if (newTrack != 10 && (g_cdTrack > 9 || g_cdTrack < 2))
			g_cdTrack = 2;

		FStringf filename("redneck%s%02d.ogg", isRRRA()? "rides" : "", g_cdTrack);
		if (Mus_Play(filename, false)) return;

		filename.Format("track%02d.ogg", g_cdTrack);
		if (Mus_Play(filename, false)) return;
	}
	// If none of the tracks managed to start, disable the CD music for this session so that regular music can play if defined.
	cd_disabled = true;
}


void S_PlayBonusMusic()
{
	if (MusicEnabled() && mus_enabled)
		S_PlaySound(BONUSMUSIC, CHAN_AUTO, CHANF_UI);
}


void S_WorldTourMappingsForOldSounds()
{
	// This tries to retrieve the original sounds for World Tour's often inferior replacements.
	// It's really ironic that despite their low quality they often sound a lot better than the new ones.
	if (!isWorldTour()) return;
	unsigned maxsnd = soundEngine->GetNumSounds();
	for(unsigned i = 1; i < maxsnd; i++)
	{
		auto sfx = soundEngine->GetSfx(FSoundID::fromInt(i));
		FString fname = sfx->name.GetChars();
		if (!fname.Right(4).CompareNoCase(".ogg"))
		{
			// All names here follow the same convention. We must strip the "sound/" folder and replace the extension to get the original VOCs.
			fname.ToLower();
			fname.Substitute("sound/", "");
			fname.Substitute(".ogg", ".voc");
			int lump = fileSystem.FindFile(fname); // in this case we absolutely do not want the extended lookup that's optionally performed by S_LookupSound.
			if (lump >= 0)
			{
				auto newsfx = soundEngine->AllocateSound();
				*newsfx = *sfx;
				newsfx->name = fname;
				newsfx->lumpnum = lump;
				sfx->UserData[kWorldTourMapping] = soundEngine->GetNumSounds() - 1;
			}
		}
	}
}

static TArray<FString> Commentaries;

void S_ParseDeveloperCommentary()
{
	int lumpnum = fileSystem.FindFile("def/developer_commentary.def");
	if (lumpnum < 0) return;
	FScanner sc;
	sc.OpenLumpNum(lumpnum);
	try
	{
		sc.SetCMode(true);
		sc.MustGetStringName("def");
		sc.MustGetStringName("developercommentary");
		sc.MustGetStringName("{");
		while (!sc.CheckString("}"))
		{
			FString path;
			int num = -1;
			sc.MustGetStringName("def");
			sc.MustGetStringName("sound");
			sc.MustGetStringName("{");
			while (!sc.CheckString("}"))
			{
				sc.MustGetString();
				if (sc.Compare("path"))
				{
					sc.MustGetStringName(":");
					sc.MustGetString();
					path = sc.String;
					sc.MustGetStringName(";");
				}
				else if (sc.Compare("num"))
				{
					sc.MustGetStringName(":");
					sc.MustGetNumber();
					num = sc.Number;
					sc.MustGetStringName(";");
				}
			}
			sc.MustGetStringName(";");
			if (Commentaries.Size() <= (unsigned)num) Commentaries.Resize(num + 1);
			Commentaries[num] = std::move(path);
		}
		//sc.MustGetStringName(";");
	}
	catch (const std::exception& ex)
	{
		Printf("Failed to read developer commentary definitions:\n%s", ex.what());
		return;
	}
}

void StopCommentary()
{
	if (currentCommentarySound.isvalid())
	{
		soundEngine->StopSound(SOURCE_None, nullptr, CHAN_VOICE, currentCommentarySound);
	}
}

bool StartCommentary(int tag, DDukeActor* actor)
{
	if (wt_commentary && Commentaries.Size() > (unsigned)tag && Commentaries[tag].IsNotEmpty())
	{
		FSoundID id = soundEngine->FindSound(Commentaries[tag]);
		if (!id.isvalid())
		{
			int lump = fileSystem.FindFile(Commentaries[tag]);
			if (lump < 0)
			{
				Commentaries[tag] = "";
				return false;
			}
			id = FSoundID(soundEngine->AddSoundLump(Commentaries[tag], lump, 0));
		}
		StopCommentary();
		MuteSounds();
		soundEngine->StartSound(SOURCE_None, nullptr, nullptr, CHAN_VOICE, CHANF_UI | CHANF_TRANSIENT | CHANF_OVERLAP, id, 1.f, 0.f);
		currentCommentarySound = id;
		currentCommentarySprite = actor;
		I_SetRelativeVolume(0.25f);
		return true;
	}
	return false;
}

END_DUKE_NS
