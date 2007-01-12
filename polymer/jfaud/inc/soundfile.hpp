#ifndef __soundfile_hpp__
#define __soundfile_hpp__

#include "file.hpp"

class SoundFile {
public:
	typedef enum { TYPE_WAVEFORM, TYPE_MIDI } Type;
	typedef enum { InitOK, InitDisabled, InitFailed } InitState;

	SoundFile() { }
	virtual ~SoundFile() { }
	virtual bool IsValid() const = 0;
	virtual bool IsUsable() const { return IsValid(); }

	// Sub-classes should implement these
	//static InitState Init() { return InitDisabled; }
	//static bool Uninit() { return false; }
	
	virtual Type GetType() const = 0;
	virtual const char *GetTypeName() const = 0;
};

SoundFile *IdentifySoundFile(JFAudFile *);

#endif
