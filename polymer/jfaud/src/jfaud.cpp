#define JFAUD_INTERNAL

#include "sysdefs.h"
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstring"
# include "watcomhax/cstdarg"
# include "watcomhax/cstdio"
# include "watcomhax/cstdlib"
#else
# include <cstring>
# include <cstdarg>
# include <cstdio>
# include <cstdlib>
#endif

#ifdef _WIN32
// because wtypes.h decides to be stupid and declare a variable named 'int64' which
// sysdefs.h makes a type
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <mmsystem.h>
# undef PlaySound
# define DIRECTSOUND_VERSION 0x0300
# include <dsound.h>
#endif

#include "log.h"
#include "stdfile.hpp"
#include "waveformfile.hpp"
#include "waveformfile_raw.hpp"
#include "midifile.hpp"
#include "soundfile.hpp"
#include "cda_null.hpp"
#include "soundcache.hpp"
#include "midisynth.hpp"

#include "nullmixer.hpp"
#if USEAL
# include "almixer.hpp"
#endif
#include "softwaremixer.hpp"

#ifdef _WIN32
# include "midisynth_win32.hpp"
# include "cda_win32.hpp"
# include "waveout_dsound.hpp"
#else
# include "cda_sdl.hpp"
# include "waveout_sdl.hpp"
# include <SDL/SDL.h>
#endif

#include "jfaud.hpp"

#ifdef _WIN32
# define vsnprintf _vsnprintf
#endif

#ifndef SCREWED_UP_CPP
using namespace std;
#endif

//{{{ log
static void _JFAud_DefLogFunc(const char *f) { fputs(f,stderr); }
static void (*_JFAud_LogFunc)(const char *) = _JFAud_DefLogFunc;

void _JFAud_LogMsg(const char *f, ...)
{
	char s[512];
	va_list va;
	va_start(va,f);
	vsnprintf(s,sizeof(s),f,va);
	va_end(va);
	_JFAud_LogFunc(s);
}

void JFAud_SetLogFunc(void (*func)(const char *))
{
	if (!func) func = _JFAud_DefLogFunc;
	_JFAud_LogFunc = func;
}
//}}}

JFAud::JFAud()
	: useropenfunc(NULL),
	  wavemixer(NULL),
	  waveout(NULL),
	  midisynth(NULL),
	  numwavechans(0),
	  wavechans(NULL),
	  musicchan(NULL),
	  cddev(NULL),
	  cache(NULL),
	  winhnd(NULL)
{
}

JFAud::~JFAud()
{
	Uninit();
}

//{{{ Initialisation and general control
bool JFAud::InitWave(const char *name, int numvoices, int frequency)
{
	char *devid = NULL;
	int drv = -1, n;
	char *devs[] = {
		"software",
#if USEAL
		"openal",
#endif
	};

	if (wavemixer) return false;
	if (numvoices < 1) numvoices = 1;

	if (name) devid = strchr(name, ':');
	if (devid) devid++;

	if (!name) drv = 0;
	else {
		for (n = 0; n < sizeof(devs)/sizeof(devs[0]); n++) {
			if (devid && !strncasecmp(name, devs[n], devid-name)) { drv = n; break; }
			else if (!devid && !strcasecmp(name, devs[n])) { drv = n; break; }
		}
	}

	if (!cache) {
		cache = new SoundCache();
	}

	switch (drv) {
		case 0: {
#ifdef _WIN32
			WaveOut_DSound *wout;
#else
			WaveOut_SDL *wout;
#endif
			SoftwareMixer *mixer;
#ifndef _WIN32
			if (!SDL_WasInit(SDL_INIT_AUDIO)) {
				if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
					return false;
			}
#endif
			// 1. Create the software mixer object (uninitialised)
			mixer = new SoftwareMixer();
			if (!mixer) return false;

			// 2. Create the output device object, passing the software mixer object
			//    to the constructor
#ifdef _WIN32
			wout = new WaveOut_DSound(mixer);
#else
			wout = new WaveOut_SDL(mixer);
#endif
			if (!wout) {
				delete mixer;
				return false;
			}

#ifdef _WIN32
			wout->SetWindowHandle((HWND)winhnd);
#endif

			// 3. Initialise the output device object
			// 4. Initialise the software mixer with the frequency and chans of the output
			if (!wout->Init(frequency, 2, 16) || !mixer->Setup(wout, numvoices)) {
				delete mixer;
				delete wout;
				return false;
			}

			// 5. Unpause the output device which will call the software mixer for data
			wout->Pause(false);

			if (waveout) delete waveout;
			if (wavemixer) delete wavemixer;
			waveout = static_cast<WaveOut*>(wout);
			wavemixer = static_cast<JFAudMixer*>(mixer);
		} break;
#if USEAL
		case 1: {
			ALMixer *mixer;

			mixer = new ALMixer();
			if (!mixer) return false;

			if (!mixer->Init() || !mixer->Open(devid, frequency, &numvoices)) {
				delete mixer;
				return false;
			}

			if (numvoices < 1) {
				delete mixer;
				return false;
			}

			if (wavemixer) delete wavemixer;
			wavemixer = static_cast<JFAudMixer*>(mixer);
		} break;
#endif

		default: return false;
	}

	numwavechans = numvoices;
	wavechans = new struct _wavechan[numvoices];
	if (!wavechans) {
		Uninit();
		return false;
	} else {
		memset(wavechans, 0, sizeof(struct _wavechan) * numvoices);
	}

	return InitialiseWaveformReaders();

	//return true;
}

bool JFAud::SetWindowHandle(void *h)
{
	winhnd = h;
	return true;
}

bool JFAud::SetCacheSize(unsigned cachee, unsigned object)
{
	if (cache) return cache->SetCacheSize(cachee, object);
	return false;
}

bool JFAud::SetCacheItemAge(unsigned age)
{
	if (cache) return cache->SetMaxItemAge(age);
	return false;
}

bool JFAud::InitMIDI(const char *name)
{
#ifdef _WIN32
	MidiSynth_Win32 *synth;

	if (midisynth) return false;

	synth = new MidiSynth_Win32();
	if (!synth) return false;

	if (synth->Open(name)) {
		midisynth = static_cast<JFAudMidiSynth*>(synth);
		return true;
	}

	delete synth;
#endif
	return false;
}

bool JFAud::InitCDA(const char *name)
{
	if (cddev) return false;

#ifdef _WIN32
	cddev = new CDA_Win32(name);
#else
	if (!SDL_WasInit(SDL_INIT_CDROM)) {
		if (SDL_InitSubSystem(SDL_INIT_CDROM) < 0)
			return false;
	}
	cddev = new CDA_SDL(name);
#endif

	if (!cddev) return false;
	if (!cddev->IsValid()) {
		delete cddev;
		cddev = NULL;
		return false;
	}

	cddev->CheckDisc();

	return true;
}

bool JFAud::Uninit(void)
{
	int i;

	if (wavechans) {
		for (i=0; i<numwavechans; i++) {
			if (wavechans[i].h) wavemixer->ReleaseChannel(wavechans[i].h);
		}
		delete [] wavechans;
		wavechans = NULL;
		musicchan = NULL;
		UninitialiseWaveformReaders();
	}

	if (waveout) {
		waveout->Pause(true);
		delete waveout;
		waveout = NULL;
	}
	
	if (wavemixer) {
		delete wavemixer;
		wavemixer = NULL;
	}

	if (midisynth) {
		delete midisynth;
		midisynth = NULL;
	}

	if (cddev) {
		delete cddev;
		cddev = NULL;
	}

	if (cache) {
		delete cache;
		cache = NULL;
	}

#ifndef _WIN32
	SDL_QuitSubSystem(SDL_INIT_CDROM|SDL_INIT_AUDIO);
#endif

	return true;
}

bool JFAud::Update(bool agecache)
{
	int i;
	
	if (wavemixer) wavemixer->Update();
	for (i=numwavechans-1; i>=0; i--) wavechans[i].age++;

	if (cache && agecache) cache->Update();

	if (midisynth) midisynth->Update();

	return true;
}

bool JFAud::AgeCache(void)
{
	if (cache) return cache->Update();
	return false;
}
//}}}

//{{{ Enumeration
char **JFAud::EnumerateWaveDevices(const char *name, char **def)
{
	char **ar = NULL, *p;
	int i, siz = 0;

	if (!name) {
		const char *devices[] = {
			"software",
#if USEAL
			"openal",
#endif
		};

		// calculate the size of the pointers table plus the length of all the strings
		siz = sizeof(char*) * (arsiz(devices)+1);
		for (i=0; i<arsiz(devices); i++) siz += 1+strlen(devices[i]);

		// allocate it
		ar = (char **)calloc(1, siz);
		if (!ar) return ar;
		
		// compose the pointers table and put the strings after it
		p = (char*)ar + sizeof(char*) * (arsiz(devices)+1);
		for (i=0; i<arsiz(devices); i++) {
			ar[i] = p;
			strcpy(p, devices[i]);
			p += 1+strlen(devices[i]);
		}

		if (def) *def = ar[0];

		return ar;
	}

#if USEAL
	if (!strcasecmp(name, "openal")) {
		ALMixer *mixer;

		mixer = new ALMixer();
		if (!mixer) return NULL;

		if (!mixer->Init()) {
			delete mixer;
			return NULL;
		}

		ar = mixer->Enumerate(def);

		delete mixer;
	}
#endif
	
	return ar;
}

char **JFAud::EnumerateMIDIDevices(char **def)
{
	char **ar = NULL;

#ifdef _WIN32
	ar = MidiSynth_Win32::Enumerate(def);
#endif
	
	return ar;
}

char **JFAud::EnumerateCDADevices(char **def)
{
	char **ar = NULL;

#ifdef _WIN32
	ar = CDA_Win32::Enumerate(def);
#endif
	
	return ar;
}
//}}}

//{{{ Sound effect playback
struct _wavechan * JFAud::FindFreeWaveChan(int priority)
{
	int firstfree, oldestused = -1;

	for (firstfree = numwavechans-1; firstfree >= 0; firstfree--) {
		if (!wavechans[firstfree].h) break;

		if (wavechans[firstfree].priority <= priority) {
			if (oldestused < 0)
				oldestused = firstfree;
			else if (wavechans[firstfree].priority < wavechans[oldestused].priority)
				oldestused = firstfree;
			else if (wavechans[firstfree].age > wavechans[oldestused].age)
				oldestused = firstfree;
		}
	}
	if (firstfree < 0) firstfree = oldestused;

	return firstfree < 0 ? NULL : &wavechans[firstfree];
}

JFAudMixerChannel *JFAud::PlaySound(const char *filename, const char *subfilename, int priority)
{
	JFAudFile *file = NULL, *ffile;
	JFAudMixerChannel *chan;

	if (!wavemixer || !filename) return NULL;

	if (cache) {
		// first, see if the file we're being asked to play is in the sound cache
		file = cache->CheckCache(filename, subfilename);
		if (file) {
			// it is, so go ahead and use it
#ifdef DEBUG
			_JFAud_LogMsg("JFAud::PlaySound: playing %s(%s) from cache\n", filename, subfilename);
#endif
			ffile = file;
			chan = PlaySoundFile(&ffile, priority);
			if (ffile) delete file;
			return chan;
		}
	}
	
	// We see if the user wants to take over file opening from us
	if (useropenfunc) {
		file = useropenfunc(filename, subfilename);
	} else {
		StdFile *sfile;
		sfile = new StdFile(filename, subfilename);
		file = static_cast<JFAudFile *>(sfile);
	}
	if (!file) return NULL;
	if (!file->IsOpen()) {
		// open failed to yield a valid file handle
		delete file;
#ifdef DEBUG
		if (!subfilename) subfilename = "";
		_JFAud_LogMsg("JFAud::PlaySound: failed opening %s(%s)\n", filename, subfilename);
#endif
		return NULL;
	}

	if (cache) {
		// try and cache the file
		ffile = cache->CacheFile(file, filename, subfilename);
		if (ffile) {
			// caching succeeded, so throw away the file and play from the cache
			delete file;
			file = ffile;
			chan = PlaySoundFile(&ffile, priority);
			if (ffile) delete file;
			return chan;
		}
	}

	// otherwise, play direct from the file
	ffile = file;
	chan = PlaySoundFile(&ffile, priority);

	if (ffile) delete file;		// PlaySoundFile didn't take control of the file, so we clean it up

	return chan;
}

JFAudMixerChannel *JFAud::PlaySoundFile(JFAudFile **file, int priority)
{
	JFAudMixerChannel *chan;
	WaveformFile *sound;

	struct _wavechan *wavechan;

	if (!wavemixer) return NULL;
	if (!file || !(*file) || !(*file)->IsOpen()) return NULL;

	wavechan = FindFreeWaveChan(priority);

	// if everyone's more important than us, there's nothing we can do
	if (!wavechan) return NULL;

	if (wavechan->h) {
		wavemixer->ReleaseChannel(wavechan->h);
		wavechan->h = NULL;
	}

	// See if the file is one of the recognised waveform formats
	sound = IdentifyWaveformFile(*file);
	if (!sound) return NULL;

	*file = NULL;
#ifdef DEBUG
	_JFAud_LogMsg("JFAud::PlaySoundFile: format %s\n", sound->GetFormatName());
#endif

	// Get a waveform channel
	chan = wavemixer->AcquireChannel();
	if (!chan) {
		delete sound;
		// sound owns file now that it was properly identified
		// and deleted it in its destructor
		return NULL;
	}

	// Attach the waveform source to the channel
	if (!chan->SetMedia(sound)) {
		delete sound;
		wavemixer->ReleaseChannel(chan);
		return NULL;
	}

	wavechan->h = chan;
	wavechan->priority = priority;
	wavechan->age = 0;

	return chan;
}

JFAudMixerChannel *JFAud::PlayRawSound(const char *filename, const char *subfilename, int priority, int samplerate, int channels, int bytespersample, bool bigendian)
{
	JFAudFile *file, *ffile;
	JFAudMixerChannel *chan;

	if (!wavemixer || !filename) return NULL;

	// We see if the user wants to take over file opening from us
	if (useropenfunc) {
		file = useropenfunc(filename, subfilename);
	} else {
		StdFile *sfile;
		sfile = new StdFile(filename, subfilename);
		file = static_cast<JFAudFile *>(sfile);
	}
	if (!file) return NULL;
	if (!file->IsOpen()) {
		// open failed to yield a valid file handle
		delete file;
#ifdef DEBUG
		if (!subfilename) subfilename = "";
		_JFAud_LogMsg("JFAud::PlaySound: failed opening %s(%s)\n", filename, subfilename);
#endif
		return NULL;
	}

	ffile = file;
	chan = PlayRawSoundFile(&ffile, priority, samplerate, channels, bytespersample, bigendian);

	if (ffile) delete file;		// PlayRawSoundFile didn't take control of the file, so we clean it up

	return chan;
}

JFAudMixerChannel *JFAud::PlayRawSoundFile(JFAudFile **file, int priority, int samplerate, int channels, int bytespersample, bool bigendian)
{
	JFAudMixerChannel *chan;
	WaveformFile_Raw *sound;

	struct _wavechan *wavechan;

	if (!wavemixer) return NULL;
	if (!file || !(*file) || !(*file)->IsOpen()) return NULL;

	wavechan = FindFreeWaveChan(priority);

	// if everyone's more important than us, there's nothing we can do
	if (!wavechan) return NULL;

	if (wavechan->h) {
		wavemixer->ReleaseChannel(wavechan->h);
		wavechan->h = NULL;
	}

	// See if the file is one of the recognised waveform formats
	sound = new WaveformFile_Raw(*file, samplerate, channels, bytespersample, bigendian);
	if (!sound) return NULL;
	if (!sound->IsValid()) {
		delete sound;
		return NULL;
	}
	*file = NULL;

	// Get a waveform channel
	chan = wavemixer->AcquireChannel();
	if (!chan) {
		delete sound;
		// sound owns file now that it was properly identified
		// and deleted it in its destructor
		return NULL;
	}

	// Attach the waveform source to the channel
	if (!chan->SetMedia(sound)) {
		delete sound;
		wavemixer->ReleaseChannel(chan);
		return NULL;
	}

	wavechan->h = chan;
	wavechan->priority = priority;
	wavechan->age = 0;

	return chan;
}


bool JFAud::FreeSound(JFAudMixerChannel *chan)
{
	int i;

	if (!wavechans || !chan) return false;

	for (i=numwavechans-1;i>=0;i--) {
		if (chan == wavechans[i].h) {
			wavemixer->ReleaseChannel(chan);
			wavechans[i].h = NULL;

			return true;
		}
	}

	return false;
}

bool JFAud::IsValidSound(JFAudMixerChannel *chan) const
{
	int i;

	if (!wavechans || !chan) return false;

	for (i=numwavechans-1;i>=0;i--)
		if (chan == wavechans[i].h)
			return true;

	return false;
}
//}}}

//{{{ Music playback
bool JFAud::PlayMusic(const char *filename, const char *subfilename)
{
	JFAudFile *file, *ffile;
	SoundFile *sound;
	bool r;

	if (!wavemixer && !midisynth) return false;

	StopMusic();

	// We see if the user wants to take over file opening from us
	if (useropenfunc) {
		file = useropenfunc(filename, subfilename);
	} else {
		StdFile *sfile;
		sfile = new StdFile(filename, subfilename);
		file = static_cast<JFAudFile *>(sfile);
	}
	if (!file) return false;
	if (!file->IsOpen()) {
		// open failed to yield a valid file handle
		delete file;
#ifdef DEBUG
		if (!subfilename) subfilename = "";
		_JFAud_LogMsg("JFAud::PlayMusic: failed opening %s(%s)\n", filename, subfilename);
#endif
		return false;
	}

	ffile = file;
	r = PlayMusicFile(&ffile);

	if (ffile) delete file;		// PlayMusicFile didn't take control of the file, so we clean it up

	return r;
}

bool JFAud::PlayMusicFile(JFAudFile **file)
{
	SoundFile *sound;

	if (!wavemixer && !midisynth) return false;

	StopMusic();

	// See if the file is one of the recognised formats
	sound = IdentifySoundFile(*file);
	if (!sound) return false;

	*file = NULL;
#ifdef DEBUG
	_JFAud_LogMsg("JFAud::PlayMusicFile: format %s\n",
			sound->GetType() == SoundFile::TYPE_WAVEFORM ?
				(static_cast<WaveformFile*>(sound))->GetFormatName() :
				(static_cast<MidiFile*>(sound))->GetFormatName()
			);
#endif

	if (sound->GetType() == SoundFile::TYPE_WAVEFORM) {
		struct _wavechan *wavechan;
		JFAudMixerChannel *chan;

		if (!wavemixer) {
			// can't play waveform files because the device isn't initialised
			delete sound;
			return false;
		}

		wavechan = FindFreeWaveChan(0x7fffffff);

		// if everyone's more important than us (!), there's nothing we can do
		if (!wavechan) {
			delete sound;
#ifdef DEBUG
			_JFAud_LogMsg("JFAud::PlayMusicFile: no free channels\n");
#endif
			return false;
		}

		if (wavechan->h) {
			wavemixer->ReleaseChannel(wavechan->h);
			wavechan->h = NULL;
		}

		// Get a waveform channel
		chan = wavemixer->AcquireChannel();
		if (!chan) {
			delete sound;
			// sound owns file now that it was properly identified
			// and deleted it in its destructor
#ifdef DEBUG
			_JFAud_LogMsg("JFAud::PlayMusicFile: failed acquiring a channel\n");
#endif
			return false;
		}

		// Attach the waveform source to the channel
		if (!chan->SetMedia(static_cast<WaveformFile*>(sound))) {
			delete sound;
			wavemixer->ReleaseChannel(chan);
#ifdef DEBUG
			_JFAud_LogMsg("JFAud::PlayMusicFile: failed setting channel media\n");
#endif
			return false;
		}

		chan->SetGain(1.0);
		chan->SetPitch(1.0);
		chan->SetPosition(0.0,0.0,0.0);
		chan->SetVelocity(0.0,0.0,0.0);
		chan->SetDirection(0.0,0.0,1.0);
		chan->SetRolloff(0.0);
		chan->SetLoop(true);
		chan->SetFollowListener(true);
		chan->SetFilter(JFAudMixerChannel::Filter4Point);

		musicchan = wavechan;
		wavechan->h = chan;
		wavechan->priority = 0x7fffffff;
		wavechan->age = 0;

		chan->Play();
		
	} else if (sound->GetType() == SoundFile::TYPE_MIDI) {
		MidiSequencer *seq;

		if (!midisynth) {
			// can't play midi files because the device isn't initialised
			delete sound;
			return false;
		}

		seq = new MidiSequencer(static_cast<MidiFile*>(sound));
		delete sound;	// sound is no longer necessary if the sequencer was created successfully, or if it failed
		if (seq && !seq->IsValid()) { delete seq; seq = NULL; }
		if (!seq) {
#ifdef DEBUG
			_JFAud_LogMsg("JFAud::PlayMusicFile: couldn't create a sequencer\n");
#endif
			return false;
		}

		if (!midisynth->SetMedia(seq)) {
#ifdef DEBUG
			_JFAud_LogMsg("JFAud::PlayMusicFile: failed setting synthesiser\n");
#endif
			return false;
		}

		midisynth->SetLoop(true);
		midisynth->Play();
		midisynth->Resume();
	} else {
		delete sound;
#ifdef DEBUG
		_JFAud_LogMsg("JFAud::PlayMusicFile: unknown file type\n");
#endif
		return false;
	}
	
	return true;
}

bool JFAud::PauseMusic(bool onf)
{
	if (musicchan) {
		return onf ? musicchan->h->Pause() : musicchan->h->Play();
	}
	if (midisynth) {
		return onf ? midisynth->Pause() : midisynth->Resume();
	}
	return false;
}

bool JFAud::StopMusic(void)
{
	if (musicchan) {
		FreeSound(musicchan->h);
		musicchan = NULL;
	}
	if (midisynth) {
		midisynth->Stop();
	}
	return true;
}
//}}}

// vim:fdm=marker:

