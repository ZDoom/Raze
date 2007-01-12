#ifndef __midifile_smf_hpp__
#define __midifile_smf_hpp__

#include "midifile.hpp"

class MidiFile_SMF : public MidiFile {
private:
	bool isvalid, isusable;

	unsigned numtracks, ppqn;
	int datastart;

	JFAudFile *fh;

public:
	MidiFile_SMF(JFAudFile *);
	virtual ~MidiFile_SMF();
	virtual bool IsValid() const { return isvalid; }
	virtual bool IsUsable() const { return isusable; }

	static InitState Init() { return InitOK; }
	static bool Uninit() { return true; }
	
	// these static versions retrieve the identifiers for this particular class
	static Format GetClassFormat() { return FORMAT_SMF; }
	static const char *GetClassFormatName() { return "Standard MIDI"; }
	// these virtual versions return the identifier based on whether the file is valid or not
	virtual Format GetFormat() const { return isvalid ? FORMAT_SMF : FORMAT_UNKNOWN; }
	virtual const char *GetFormatName() const { return isvalid ? "Standard MIDI" : NULL; }

	virtual MidiBuffer *ReadData(void);
};

#endif

