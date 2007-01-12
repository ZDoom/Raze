#ifndef __midisynth_hpp__
#define __midisynth_hpp__

#ifdef JFAUD_INTERNAL
# include "midiseq.hpp"
#else
class MidiSequencer;
#endif

class JFAudMidiSynth {
private:
protected:
public:
	JFAudMidiSynth();
	virtual ~JFAudMidiSynth();

	virtual bool Open(const char *dev) = 0;
	virtual bool Close(void) = 0;
	virtual bool Update(void) = 0;

	virtual bool SetMedia(MidiSequencer *) = 0;

	virtual bool Play(void) = 0;
	virtual bool Stop(void) = 0;
	virtual bool Pause(void) = 0;
	virtual bool Resume(void) = 0;

	virtual bool SetLoop(bool onf) = 0;
	virtual bool GetLoop(void) const = 0;
};

#endif
