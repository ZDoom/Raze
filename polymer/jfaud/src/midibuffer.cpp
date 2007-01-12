#define JFAUD_INTERNAL
#include "sysdefs.h"
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstring"
#else
# include <cstring>
#endif
#include "midibuffer.hpp"

#ifndef SCREWED_UP_CPP
using namespace std;
#endif

MidiBuffer::MidiBuffer()
	: numtracks(0), ppqn(0), tracklength(NULL), tracks(NULL)
{
}

MidiBuffer::~MidiBuffer()
{
	unsigned i;

	if (tracks) {
		for (i=0;i<numtracks;i++) {
			if (tracks[i])
				delete [] tracks[i];
		}
		delete [] tracks;
	}

	if (tracklength) delete [] tracklength;
}

unsigned int MidiBuffer::GetTrackLength(unsigned int track) const
{
	if (track >= numtracks || !tracklength) return 0;
	return tracklength[track];
}

unsigned char * MidiBuffer::GetTrackData(unsigned int track) const
{
	if (track >= numtracks || !tracks) return NULL;
	return tracks[track];
}
	
bool MidiBuffer::Allocate(unsigned int numtracks)
{
	if (tracks || tracklength) {
		delete [] tracks;
		delete [] tracklength;

		tracks = NULL;
		tracklength = NULL;
		this->numtracks = 0;
	}

	if (numtracks == 0) return true;

	tracks = new unsigned char*[numtracks];
	tracklength = new unsigned int[numtracks];

	if (!tracks || !tracklength) {
		if (tracks) delete [] tracks;
		if (tracklength) delete [] tracklength;
		return false;
	}

	memset(tracks, 0, sizeof(unsigned char *)*numtracks);
	memset(tracklength, 0, sizeof(unsigned int)*numtracks);

	this->numtracks = numtracks;

	return true;
}

bool MidiBuffer::AllocateTrack(unsigned int track, unsigned int length)
{
	if (track > numtracks || !tracks) return false;

	if (tracks[track]) {
		delete [] tracks[track];
		tracks[track] = NULL;
		tracklength[track] = 0;
	}

	if (length == 0) return true;

	tracks[track] = new unsigned char[length];
	if (!tracks[track]) return false;

	memset(tracks[track], 0, length);
	tracklength[track] = length;

	return true;
}

