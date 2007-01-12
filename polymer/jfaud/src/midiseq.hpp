#ifndef __midiseq_hpp__
#define __midiseq_hpp__

#include "midibuffer.hpp"
#include "midifile.hpp"

class MidiSequencer {
private:
	class TrackDef {
	private:
		unsigned int delta;
		unsigned char const *evtptr;
		unsigned char command;
		unsigned int evtlen;
		int loopcount;	// loop repeat counter (-1 = infinite)
		unsigned int loopstart, loopend;

		unsigned char *start, *pos, flags;
		unsigned int  len;
	public:
		TrackDef();
		~TrackDef();

		void Reset();
		void Setup(unsigned char *start, unsigned int len) {
			this->start = this->pos = start;
			this->len = len;
		}

		void SetFlag(char flag) { flags |= flag; }
		void ClrFlag(char flag) { flags &= ~flag; }
		char TestFlag(char flag) const { return flags & flag; }

		unsigned int GetPos() const { return (unsigned int)(pos-start); }
		unsigned char *GetPosPtr() const { return pos; }
		void SetPos(unsigned int off) { pos = start + off; }
		void Rewind() { pos = start; }
		bool EOT() const { return GetPos() >= len; }
		int  GetByte() { if (EOT()) return -1; return *(pos++); }
		int  PeekByte(unsigned ahead) { if ((unsigned int)(pos+ahead-start) >= len) return -1; return pos[ahead]; }

		bool          FetchEvent();
		unsigned int  GetMidiNum();
		unsigned int  GetDelta() const { return delta; }
		unsigned char const *GetEventPtr() const { return evtptr; }
		unsigned char GetCommand() const { return command; }
		unsigned int  GetEventLen() const { return evtlen; }
		int           GetLoopCount() const { return loopcount; }
		unsigned int  GetLoopStart() const { return loopstart; }
		unsigned int  GetLoopEnd() const { return loopend; }
		void          SetDelta(unsigned int d) { delta = d; }
		void          SetLoopCount(int count) { loopcount = count; }
		void          SetLoopStart(unsigned int off) { loopstart = off; }
		void          SetLoopEnd(unsigned int off) { loopend = off; }
	};

	MidiBuffer *midifile;
	TrackDef   *seqtrack;
	unsigned char lastcommand;
	bool infiniteloop;
	unsigned char evtbuf[4];	// for synthesised events
	
	unsigned int GetMidiNum(TrackDef *track);

	void Scan(int device);

public:
	enum EMIDIDevice {
		DEV_GeneralMidi = 0,	// general midi
		DEV_SoundCanvas,	// roland sound canvas
		DEV_SoundBlasterEmu,	// sound blaster emu8k (AWE cards)
		DEV_WaveBlaster,	// wave blaster
		DEV_SoundBlasterOpl,	// sound blaster fm synth (opl2,3)
		DEV_ProAudio,		// media vision pro audio
		DEV_SoundMan16,		// logitech sound man 16
		DEV_Adlib,		// adlib
		DEV_SoundScape,		// esoniq soundscape
		DEV_Gus,		// gravis ultrasound
		DEV_ALL = 127
	};
	
	MidiSequencer(MidiFile *);
	~MidiSequencer();

	bool IsValid(void) const;

	void SetDevice(EMIDIDevice dev);
	void SetLoop(bool loop);
	void Rewind(void);

	int GetEvent(unsigned *delta, unsigned char *command, unsigned *len, unsigned char const **evtdata);
		// returns 0 if a message was returned, 1 if the end of the song was reached, or -1 on error
	unsigned int GetPPQN(void) const;
};

#endif

