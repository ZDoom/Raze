#pragma once

#include <string>
#include <vector>
#include <memory>
#include "zmusic.h"
#include "../../libraries/music_common/fileio.h"

// Note: Bools here are stored as ints to allow having a simpler interface.

struct FluidConfig
{
	std::string fluid_lib;
	std::string fluid_patchset;
	int fluid_reverb = false;
	int fluid_chorus = false;
	int fluid_voices = 128;
	int fluid_interp = 1;
	int fluid_samplerate = 0;
	int fluid_threads = 1;
	int fluid_chorus_voices = 3;
	int fluid_chorus_type = 0;
	float fluid_gain = 0.5f;
	float fluid_reverb_roomsize = 0.61f;
	float fluid_reverb_damping = 0.23f;
	float fluid_reverb_width = 0.76f;
	float fluid_reverb_level = 0.57f;
	float fluid_chorus_level = 1.2f;
	float fluid_chorus_speed = 0.3f;
	float fluid_chorus_depth = 8;
};

struct OPLConfig
{
	int numchips = 2;
	int core = 0;
	int fullpan = true;
	int genmidiset = false;
	uint8_t OPLinstruments[36 * 175]; // it really is 'struct GenMidiInstrument OPLinstruments[GENMIDI_NUM_TOTAL]'; but since this is a public header it cannot pull in a dependency from oplsynth.
};

namespace TimidityPlus
{
	class Instruments;
	class SoundFontReaderInterface;
}

struct TimidityConfig
{
	std::string timidity_config;

	MusicIO::SoundFontReaderInterface* reader;
	std::string readerName;
	std::string loadedConfig;
	std::shared_ptr<TimidityPlus::Instruments> instruments;	// this is held both by the config and the device

};

struct DumbConfig
{
	int  mod_samplerate;
    int  mod_volramp;
    int  mod_interp;
    int  mod_autochip;
    int  mod_autochip_size_force;
    int  mod_autochip_size_scan;
    int  mod_autochip_scan_threshold;
    float mod_dumb_mastervolume;
};

// The rest is not used yet.

struct MiscConfig
{
	int snd_midiprecache;
	float gme_stereodepth;
	int snd_streambuffersize = 64;
	int snd_mididevice;
	int snd_outputrate = 44100;
	float snd_musicvolume = 1.f;
	float relative_volume = 1.f;
	float snd_mastervolume = 1.f;
};

extern FluidConfig fluidConfig;
extern OPLConfig oplConfig;
extern TimidityConfig timidityConfig;
extern DumbConfig dumbConfig;
extern MiscConfig miscConfig;
extern Callbacks musicCallbacks;

