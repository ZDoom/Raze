#ifndef __midifile_hpp__
#define __midifile_hpp__

#include "soundfile.hpp"
#include "midibuffer.hpp"

class MidiFile : public SoundFile {
public:
	typedef enum {
		FORMAT_UNKNOWN = -1,
		FORMAT_FIRST = 0,
		FORMAT_SMF = 0,
		FORMAT_LAST = FORMAT_SMF
	} Format;

	MidiFile() { }
	virtual ~MidiFile() { }

	virtual Type GetType() const { return TYPE_MIDI; }
	virtual const char *GetTypeName() const { return "MIDI"; }
	virtual Format GetFormat() const = 0;
	virtual const char *GetFormatName() const = 0;

	// Sub-classes should implement these
	//static InitState Init() { return InitDisabled; }
	//static bool Uninit() { return false; }
	
	virtual MidiBuffer *ReadData(void) = 0;
};

bool InitialiseMIDIReaders(void);
void UninitialiseMIDIReaders(void);
MidiFile *IdentifyMIDIFile(JFAudFile *);

#endif
