#define JFAUD_INTERNAL
#include "sysdefs.h"
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstdio"
# include "watcomhax/cstdlib"
#else
# include <cstdio>
# include <cstdlib>
#endif

#include "soundfile.hpp"
#include "waveformfile.hpp"
#include "midifile.hpp"

#ifndef SCREWED_UP_CPP
using namespace std;
#endif

SoundFile *IdentifySoundFile(JFAudFile *fh)
{
	WaveformFile *wfile;
	MidiFile *mfile;
	if ((mfile = IdentifyMIDIFile(fh)) != NULL) return static_cast<SoundFile*>(mfile);
	if ((wfile = IdentifyWaveformFile(fh)) != NULL) return static_cast<SoundFile*>(wfile);
	return NULL;
}



// vim:fdm=marker:fdc=2:
