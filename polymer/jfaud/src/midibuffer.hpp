#ifndef __midibuffer_hpp__
#define __midibuffer_hpp__

#include "buffer.hpp"

class MidiBuffer : public Buffer {
private:
	unsigned int numtracks, ppqn;
	unsigned int *tracklength;
	unsigned char **tracks;
protected:
public:
	MidiBuffer();
	virtual ~MidiBuffer();

	virtual Type GetType() const { return TYPE_MIDI; }

	unsigned int GetNumTracks() const { return numtracks; }
	unsigned int GetPPQN() const { return ppqn; }
	unsigned int GetTrackLength(unsigned int track) const;
	unsigned char * GetTrackData(unsigned int track) const;
	
	void SetPPQN(unsigned int ppqn) { this->ppqn = ppqn; }
	
	bool Allocate(unsigned int numtracks);
	bool AllocateTrack(unsigned int track, unsigned int length);
};

#endif
