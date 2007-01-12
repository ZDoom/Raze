#define JFAUD_INTERNAL
#include "sysdefs.h"
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstdio"
#else
# include <cstdio>
#endif

#include "log.h"
#include "midifile_smf.hpp"

#ifndef SCREWED_UP_CPP
using namespace std;
#endif

static unsigned int getbignum(unsigned char *ptr, int size)
{
        unsigned int val = 0;
        if (size > 4) size = 4;
        while (size--) {
                val <<= 8;
                val |= *(ptr++);
        }
        return val;
}


MidiFile_SMF::MidiFile_SMF(JFAudFile *tfh)
	: isvalid(false), isusable(false), numtracks(0), datastart(-1)
{
	unsigned char headerbuf[8];
	unsigned i, chunksz;

	tfh->Rewind();

	if (tfh->Read(4*2, headerbuf) != 4*2) return;
	if (headerbuf[0] != 'M' || headerbuf[1] != 'T' || headerbuf[2] != 'h' || headerbuf[3] != 'd') return;
	isvalid = true;
	if ((chunksz=getbignum(&headerbuf[4], 4)) < 6) return;
	
	if (tfh->Read(2*3, headerbuf) != 2*3) return;

	i = getbignum(&headerbuf[0], 2);	// format
	numtracks = getbignum(&headerbuf[2], 2);
	if (i > 1 || numtracks < 1) return;

	ppqn = getbignum(&headerbuf[4], 2);

	datastart = tfh->Tell();

	fh = tfh;
	isusable = true;
}

MidiFile_SMF::~MidiFile_SMF()
{
	// We're responsible for deleting the file handle if we acknowledged the file as being
	// one of our own and it was usable by us in the constructor. Otherwise, the caller of
	// the constructor cleans up the file handle.
	if (fh && isvalid && isusable) delete fh;
}

MidiBuffer * MidiFile_SMF::ReadData(void)
{
	MidiBuffer *midi;
	unsigned i, tracklen;
	unsigned char headerbuf[8];
	
	midi = new MidiBuffer();
	if (!midi) return NULL;

	midi->SetPPQN(ppqn);
	fh->Seek(datastart, JFAudFile::Set);

	if (!midi->Allocate(numtracks)) {
		delete midi;
		return NULL;
	}

	for (i=0; i<numtracks; i++) {
		if (fh->Read(4*2, headerbuf) != 4*2) {
			delete midi;
			return NULL;
		}

		if (headerbuf[0] != 'M' || headerbuf[1] != 'T' || headerbuf[2] != 'r' || headerbuf[3] != 'k') {
			delete midi;
			return NULL;
		}

		tracklen = getbignum(&headerbuf[4], 4);
		if (!midi->AllocateTrack(i, tracklen)) {
			delete midi;
			return NULL;
		}

		if (fh->Read(tracklen, midi->GetTrackData(i)) != tracklen) {
			delete midi;
			return NULL;
		}
	}

#ifdef DEBUG
	_JFAud_LogMsg("MidiFile_SMF::ReadData(): %d tracks, %d PPQN\n", midi->GetNumTracks(), midi->GetPPQN());
	for (i=0; i<numtracks; i++) {
		_JFAud_LogMsg("MidiFile_SMF::ReadData(): track %d, %d bytes\n", i, midi->GetTrackLength(i));
	}
#endif

	return midi;
}

