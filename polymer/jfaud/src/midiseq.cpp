#define JFAUD_INTERNAL
#include "sysdefs.h"
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstdio"
#else
# include <cstdio>
#endif
#include "midiseq.hpp"
#include "log.h"

#ifndef SCREWED_UP_CPP
using namespace std;
#endif

// Track flags
enum {
	IgnoreTrack = 1,
	IgnorePrograms = 2,
	IgnoreVolumes = 4,
	TrackDisabled = 8
};

// Command lengths
static const int cmdlens[] = {
        2 /*noteoff 0x80*/, 2 /*noteon 0x90*/, 2 /*aftertouch 0xA0*/, 2 /*control 0xB0*/,
        1 /*program 0xC0*/, 1 /*channelaftertouch 0xD0*/, 2 /*pitchwheel 0xE0*/, 0 /*syscommon 0xF0*/
};

unsigned int MidiSequencer::GetMidiNum(TrackDef *track)
{
        int val = 0;
	int by;
        do {
                by = track->GetByte();
		if (by < 0) return 0;
                val <<= 7;
                val |= (by&0x7f);
        } while (by&0x80);
        return val;
}

unsigned int MidiSequencer::TrackDef::GetMidiNum()
{
        int val = 0;
	int by;
        do {
                by = GetByte();
		if (by < 0) return 0;
                val <<= 7;
                val |= (by&0x7f);
        } while (by&0x80);
        return val;
}


MidiSequencer::TrackDef::TrackDef()
	: start(NULL), pos(NULL), len(0)
{
	Reset();
}

MidiSequencer::TrackDef::~TrackDef()
{
}

void MidiSequencer::TrackDef::Reset()
{
	flags = 0;
	delta = 0;
	evtptr = NULL;
	command = 0;
	evtlen = 0;
	loopcount = 0;
	loopstart = 0;
	loopend = 0;
}

bool MidiSequencer::TrackDef::FetchEvent()
{
	int cmd,length;
        
	if (EOT()) return false;

	delta = GetMidiNum();
	if (PeekByte(0) & 0x80) command = GetByte();
	cmd = command;
	length = cmdlens[ (cmd>>4) & 7 ];
	evtptr = GetPosPtr();
	evtlen = length;

	if (length == 0) {	// system common event
		switch (cmd) {
			case 0xF0:
				// sysex
				do evtlen++; while (GetByte() != 0xF7);
				break;
			case 0xFF:
				// meta
				cmd = GetByte();
				length = GetByte();
				evtlen += length + 2;
				while (length--) GetByte();
				break;
			case 0xF2:
				// two-byte messages
				evtlen++; GetByte();
			case 0xF1:
			case 0xF3:
				// one-byte messages
				evtlen++; GetByte();
			default:
				// command-byte-only messages
				break;
		}
	} else {
		while (length--) GetByte();
	}

	return 0;
}

void MidiSequencer::Scan(int device)
{
	unsigned i;
	char foundincl;
	int deltatime;
	unsigned char command, parm1, parm2;

	for (i=0; i<midifile->GetNumTracks(); i++) {
		foundincl = 0;
		command = 0;
		seqtrack[i].Rewind();
		seqtrack[i].Reset();
		while (!seqtrack[i].EOT()) {
			deltatime = GetMidiNum(&seqtrack[i]);
			if (seqtrack[i].PeekByte(0) & 0x80) {
				command = seqtrack[i].GetByte();
			} else if (!command) {
				while (!((command = seqtrack[i].GetByte()) & 0x80)) ;
			}

			if ((command&0xF0) == 0xB0) {	// controller change
				parm1 = seqtrack[i].GetByte();
				parm2 = seqtrack[i].GetByte();
				switch (parm1) {
					case 110:	// EMIDI include
						if (parm2 == 127 || parm2 == device) {
							seqtrack[i].ClrFlag(IgnoreTrack);
						} else if (!foundincl) {
							seqtrack[i].SetFlag(IgnoreTrack);
						}
						foundincl = 1;
						break;
                                                
					case 111:	// EMIDI exclude
						if (parm2 == 127 || parm2 == device) {
							seqtrack[i].SetFlag(IgnoreTrack);
						}
						break;

					case 112:	// EMIDI program
						seqtrack[i].SetFlag(IgnorePrograms);
						break;

					case 113:	// EMIDI volume
						seqtrack[i].SetFlag(IgnoreVolumes);
						break;
				}
			} else if ((command&0xF0) == 0xF0) {	// meta event
				switch (command) {
					case 0xF0:
						while (seqtrack[i].GetByte() != 0xF7) ;
						break;
					case 0xFF:
						seqtrack[i].GetByte();
						for (parm2=seqtrack[i].GetByte(); parm2>0; parm2--)
							seqtrack[i].GetByte();
						break;
					case 0xF2:
						seqtrack[i].GetByte();
					case 0xF1: case 0xF3:
						seqtrack[i].GetByte();
					default:
						break;
				}
				command = 0;
			} else {
				for (parm2=cmdlens[ (command>>4) & 7 ]; parm2>0; parm2--)
					seqtrack[i].GetByte();
			}
		}
		seqtrack[i].Rewind();
	}

#ifdef DEBUG
	for (i=0; i<midifile->GetNumTracks(); i++) {
		_JFAud_LogMsg("MidiSequencer::Scan(%d): track %d is %s\n",device,i,
				seqtrack[i].TestFlag(IgnoreTrack)?"ignored":"enabled");
	}
#endif
}



MidiSequencer::MidiSequencer(MidiFile *fh)
	: midifile(NULL), seqtrack(NULL), lastcommand(0), infiniteloop(false)
{
	MidiBuffer *buf;

	if (!fh || !fh->IsValid() || !fh->IsUsable()) return;

	buf = fh->ReadData();
	if (!buf) return;

	if (buf->GetNumTracks() < 1) {
		delete buf;
		return;
	}

	seqtrack = new TrackDef[ buf->GetNumTracks() ];
	if (!seqtrack) {
		delete buf;
		return;
	} else {
		unsigned i;

		for (i=0; i < buf->GetNumTracks(); i++) {
			seqtrack[i].Setup(buf->GetTrackData(i), buf->GetTrackLength(i));
			seqtrack[i].Reset();

			// prime the track
	                if (seqtrack[i].FetchEvent())
				seqtrack[i].SetFlag(TrackDisabled);
		}
	}

	midifile = buf;
}

MidiSequencer::~MidiSequencer()
{
	if (seqtrack) delete [] seqtrack;
	if (midifile) delete midifile;
}

bool MidiSequencer::IsValid() const
{
	return midifile != NULL;
}


void MidiSequencer::SetDevice(EMIDIDevice dev)
{
	unsigned i;

	if ((unsigned)dev > DEV_ALL) dev = DEV_ALL;
        
	lastcommand = 0;
	Scan((int)dev);

        for (i=0; i<midifile->GetNumTracks(); i++) {
                if (seqtrack[i].TestFlag(IgnoreTrack)) {
			seqtrack[i].SetFlag(TrackDisabled);
			continue;
		}
                if (seqtrack[i].FetchEvent())
			seqtrack[i].SetFlag(TrackDisabled);
        }
}

void MidiSequencer::SetLoop(bool loop)
{
	infiniteloop = loop;
}

void MidiSequencer::Rewind(void)
{
	unsigned i;

	if (!midifile) return;

        for (i=0; i<midifile->GetNumTracks(); i++) seqtrack[i].Rewind();
	lastcommand = 0;
}

int MidiSequencer::GetEvent(unsigned *delta, unsigned char *command, unsigned *len, unsigned char const **evtdata)
{
	unsigned int deltaacc = 0, mindelta = 0xfffffff;
	int mindeltatrk = -1, j;
	unsigned i;
	char gotevent = 0;

	TrackDef *curtrack;

again:
	// find the track with the nearest event
	mindelta = 0xffffffff;
	mindeltatrk = -1;
	for (i=0;i<midifile->GetNumTracks();i++) {
		if (seqtrack[i].TestFlag(TrackDisabled)) continue;
		if (seqtrack[i].GetDelta() >= mindelta) continue;
		mindelta = seqtrack[i].GetDelta();
		mindeltatrk = i;
	}
	if (mindeltatrk < 0) {
		if (!infiniteloop) return 1;
		for (i=0;i<midifile->GetNumTracks();i++) {
			if (seqtrack[i].TestFlag(IgnoreTrack)) {
				seqtrack[i].SetFlag(TrackDisabled);
				continue;
			}
			seqtrack[i].ClrFlag(TrackDisabled);
			seqtrack[i].Rewind();
			if (seqtrack[i].FetchEvent())
				seqtrack[i].SetFlag(TrackDisabled);
		}
		goto again;
	}

	// check out the event
	curtrack = &seqtrack[mindeltatrk];
	switch (curtrack->GetCommand() & 0xF0) {
		case 0xC0:	// program
			if (curtrack->TestFlag(IgnorePrograms))
				break;
		case 0x80:	// note off
		case 0x90:	// note on
		case 0xA0:	// aftertouch
		case 0xD0:	// channel aftertouch
		case 0xE0:	// pitch wheel
			gotevent = 1;
			break;
		case 0xF0:	// system common event
			// we keep tempo meta events so they can be interpreted, but all other metas get swallowed
			if (curtrack->GetCommand() == 0xFF && *(curtrack->GetEventPtr()) == 0x51)
				gotevent = 1;
			else if (curtrack->GetCommand() == 0xFF &&
				 *(curtrack->GetEventPtr()) >= 1 &&
				 *(curtrack->GetEventPtr()) <= 7) {
/*			
				char msg[256], *type;
				memset(msg,0,256);
				memcpy(msg, curtrack->GetEventPtr()+2, *(curtrack->GetEventPtr()+1));
				switch (*(curtrack->GetEventPtr())) {
					case 1: type = "text"; break;
					case 2: type = "copyright"; break;
					case 3: type = "track name"; break;
					case 4: type = "instrument name"; break;
					case 5: type = "lyric"; break;
					case 6: type = "marker"; break;
					case 7: type = "cue point"; break;
					default: type = "?"; break;
				}
				debuglog(DEBUGLVL_WORDY,jsprintf("%s event: %s",type,msg));
*/
			}
			else if (curtrack->GetCommand() == 0xFF && *(curtrack->GetEventPtr()) == 0x2F)
				curtrack->SetFlag(TrackDisabled);
			break;
		case 0xB0:	// control
			switch (*(curtrack->GetEventPtr())) {
				case 110:	// EMIDI include
				case 111:	// EMIDI exclude
				case 114:	// EMIDI context start
				case 115:	// EMIDI context end
					break;
				case 113:	// EMIDI volume
					evtbuf[0] = 7;		// cook up a normal volume change event
					evtbuf[1] = *(curtrack->GetEventPtr()+1);
					*command  = curtrack->GetCommand();
					*evtdata  = evtbuf;
					*len      = 2;
					gotevent  = 2;
					break;
				case 112:	// EMIDI program
					*command  = 0xC0 | (curtrack->GetCommand() & 15);	// cook up a normal
					evtbuf[0] = *(curtrack->GetEventPtr()+1) & 0x7F;	// program change event
					*evtdata  = evtbuf;
					*len      = 1;
					gotevent  = 2;
					break;
				case 116:	// EMIDI loop begin
				case 118:	// EMIDI song loop begin
					if (*(curtrack->GetEventPtr()+1) == 0) {
						j = -1;
					} else {
						j = *(curtrack->GetEventPtr()+1);
						//debuglog(DEBUGLVL_WORDY,jsprintf("Track %d: Entering %d-cycle loop",mindeltatrk,j));
					}

					if (*(curtrack->GetEventPtr()) == 118) {
						// song-wide loop
						for (i=0;i<midifile->GetNumTracks();i++) {
							seqtrack[i].SetLoopCount(j);
							seqtrack[i].SetLoopStart( seqtrack[i].GetPos() );
						}
					} else {
						curtrack->SetLoopCount(j);
						curtrack->SetLoopStart( curtrack->GetPos() );
					}
					break;
				case 117:	// EMIDI loop end
				case 119:	// EMIDI song loop end
					if (*(curtrack->GetEventPtr()+1) != 127) {
						break;
					}
					if (!infiniteloop && curtrack->GetLoopCount() < 0) {
						break;
					}

					if (*(curtrack->GetEventPtr()) == 118) {
						// song-wide loop
						for (i=0;i<midifile->GetNumTracks();i++)
							seqtrack[i].SetLoopEnd( seqtrack[i].GetPos() );
					}

					if (curtrack->GetLoopCount()) {
						if (curtrack->GetLoopCount() > 0) {
							curtrack->SetLoopCount( curtrack->GetLoopCount() - 1 );
							//debuglog(DEBUGLVL_WORDY,jsprintf("Track %d: %d to go...",mindeltatrk,curtrack->GetLoopCount()));
						}
						curtrack->SetPos( curtrack->GetLoopStart() );
					}
					break;
				case 7:		// volume change
					if (curtrack->TestFlag(IgnoreVolumes))
						break;
				default:
					gotevent = 1;
					break;
			}
			break;
	}

	if (gotevent) {
		if (gotevent == 1) {
			*command = curtrack->GetCommand();
			*evtdata = curtrack->GetEventPtr();
			*len     = curtrack->GetEventLen();
		}
		lastcommand = *command;
		*delta = deltaacc + mindelta;
	}
        
	// move all events forward and get a new one for the track we just ate
	for (i=0;i<midifile->GetNumTracks();i++) {
		if (seqtrack[i].TestFlag(TrackDisabled)) continue;
		if (i == (unsigned)mindeltatrk) {
			if (seqtrack[i].FetchEvent()) {
				seqtrack[i].SetFlag(TrackDisabled);
			}
		} else {
			seqtrack[i].SetDelta( seqtrack[i].GetDelta() - mindelta );
		}
	}

	if (gotevent) {
		return 0;
	} else {
		deltaacc += mindelta;
		goto again;
	}

	return -1;
}

unsigned int MidiSequencer::GetPPQN(void) const
{
	if (!midifile) return 0;
	return midifile->GetPPQN();
}

