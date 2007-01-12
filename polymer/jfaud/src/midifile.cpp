#define JFAUD_INTERNAL
#include "sysdefs.h"
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstdio"
#else
# include <cstdio>
#endif

#include "log.h"
#include "soundfile.hpp"
#include "midifile.hpp"

#include "midifile_smf.hpp"

#ifndef SCREWED_UP_CPP
using namespace std;
#endif

static int init = 0;

bool InitialiseMIDIReaders(void)
{
	int i;
	SoundFile::InitState r;
	const char *name;

	if (init > 0) return true;

	_JFAud_LogMsg("Supported MIDI formats:\n");
	for (i=MidiFile::FORMAT_FIRST; i<=MidiFile::FORMAT_LAST; i++) {
		switch (i) {
			case MidiFile::FORMAT_SMF:     /*r = MidiFile_SMF::Init();*/ continue;
			default: continue;
		}

		switch (r) {
			case SoundFile::InitFailed: return false;
			case SoundFile::InitOK: _JFAud_LogMsg("   %s\n", name); break;
			case SoundFile::InitDisabled: _JFAud_LogMsg("   %s (disabled)\n", name); break;
		}
	}
	init++;
	return true;
}

void UninitialiseMIDIReaders(void)
{
	int i;

	if (init <= 0) return;

	for (i=MidiFile::FORMAT_FIRST; i<=MidiFile::FORMAT_LAST; i++) {
		switch (i) {
			case MidiFile::FORMAT_SMF:     /*MidiFile_SMF::Uninit();*/ continue;
			default: continue;
		}
	}
	init--;
}

MidiFile *IdentifyMIDIFile(JFAudFile *fh)
{
	int i;
	MidiFile *sfile;
	for (i=MidiFile::FORMAT_FIRST; i<=MidiFile::FORMAT_LAST; i++) {
		switch (i) {
			case MidiFile::FORMAT_SMF: sfile = new MidiFile_SMF(fh); break;
			default: continue;
		}

		if (!sfile) return NULL;
		if (!sfile->IsValid()) delete sfile;
		else if (!sfile->IsUsable()) {
			delete sfile;
			return NULL;
		} else return sfile;
	}
	return NULL;
}

