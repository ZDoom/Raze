#ifndef __midisynth_win32_hpp__
#define __midisynth_win32_hpp__

#include "midisynth.hpp"

class MidiSynth_Win32;

struct hdrtyp {
	MIDIHDR header;
	int n;
	MidiSynth_Win32 *synth;
};

#define STREAMBUFLEN 256       // *4 = 4K of event data per quarter note

class MidiSynth_Win32 : public JFAudMidiSynth {
private:
	bool loop, paused, justpaused;

	HMIDISTRM streamhnd;
	struct hdrtyp headers[2];
	DWORD bufdata[2][STREAMBUFLEN], buffullness[2];
	int buffersplaying;
	
	MidiSequencer *seq;
	MidiSequencer::EMIDIDevice devtype;

	unsigned evtdelta, evtlength;
	unsigned char evtcommand;
	unsigned char const *evtdata;
	int bufferdata(int num);

	bool Reset(bool stop = true);
	void StopAllNotes(void);
	
	// for threaded buffering
	HANDLE threadhnd; DWORD threadid;
	CRITICAL_SECTION mutex;
	static DWORD WINAPI BufThread(class MidiSynth_Win32 *myself);

	// for callback notification
	static void CALLBACK midiCallback(HMIDIOUT handle, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);

	// used internally by both thread and callback
	void _BufferDone(int n);
	bool _Update();
	void Lock();
	void Unlock();
protected:
public:
	MidiSynth_Win32();
	virtual ~MidiSynth_Win32();

	virtual bool Open(const char *dev);
	virtual bool Close(void);
	virtual bool Update(void);
	static char **Enumerate(char **def);

	virtual bool SetMedia(MidiSequencer *);

	virtual bool Play(void);
	virtual bool Stop(void);
	virtual bool Pause(void);
	virtual bool Resume(void);

	virtual bool SetLoop(bool onf);
	virtual bool GetLoop(void) const;

};

#ifndef _INC_MMSYSTEM
# undef DWORD
#endif

#endif
