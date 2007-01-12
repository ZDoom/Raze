#define JFAUD_INTERNAL
#include "sysdefs.h"
#include "log.h"
#include "cda_win32.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#undef PlaySound
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstdlib"
#else
# include <cstdlib>
using namespace std;
#endif

static MCIDEVICEID deviceid = 0;

static int sendCommand(UINT msg, DWORD flags, DWORD param)
{
	MCIERROR err;
	TCHAR errmsg[128];

	err = mciSendCommand(deviceid, msg, flags, param);
	if (err == 0) return 0;

	if (mciGetErrorString(LOWORD(err), errmsg, 128))
		_JFAud_LogMsg("MCI error: %s", errmsg);

	return LOWORD(err);
}

static int toMSF(int frames)
{
	int m,s,f;

	f = frames % 75;
	s = (frames / 75) % 60;
	m = (frames / 75) / 60;

	return MCI_MAKE_MSF(m,s,f);
}



CDA_Win32::CDA_Win32(const char *name)
	: numtracks(0), isvalid(false),
	  pausepos(-1), playend(-1),
	  laststate(NOT_READY)
{
	MCI_OPEN_PARMS mop;
	MCI_SET_PARMS msp;
	char drivename[4] = "?:\\";

	if (deviceid != 0) return;	// only allow one instance of the CDA device

	if (name) drivename[0] = name[0];

	memset(&mop, 0, sizeof(mop));
        mop.lpstrDeviceType = (LPCSTR)MCI_DEVTYPE_CD_AUDIO;
	mop.lpstrElementName = drivename;

        if (sendCommand(MCI_OPEN, MCI_WAIT|MCI_OPEN_SHAREABLE|MCI_OPEN_TYPE|MCI_OPEN_TYPE_ID |
				(name ? MCI_OPEN_ELEMENT : 0), (DWORD)&mop))
                return;

        deviceid = mop.wDeviceID;

        memset(&msp, 0, sizeof(msp));
        msp.dwTimeFormat = MCI_FORMAT_MSF;

        if (sendCommand(MCI_SET, MCI_WAIT|MCI_SET_TIME_FORMAT, (DWORD)&msp)) {
		MCI_GENERIC_PARMS mgp;
		memset(&mgp, 0, sizeof(mgp));
		sendCommand(MCI_CLOSE, MCI_WAIT, (DWORD)&mgp);
		deviceid = 0;
		return;
	}

	isvalid = true;
}

CDA_Win32::~CDA_Win32()
{
	if (isvalid) {
		MCI_GENERIC_PARMS mgp;

		memset(&mgp, 0, sizeof(mgp));
		sendCommand(MCI_STOP, 0, (DWORD)&mgp);
		sendCommand(MCI_CLOSE, 0, (DWORD)&mgp);
		deviceid = 0;
	}
}

char **CDA_Win32::Enumerate(char **def)
{
	DWORD lds;
	char ldstr[4] = "?:\\";
	char **rl, **rp, *p;
	int numdevs, devstrlen, i, mask;

	lds = GetLogicalDrives();
	numdevs = 0;
	for (i=0; i<32; i++) {
		mask = 1<<i;
		if (!(lds & mask)) continue;
		ldstr[0] = 'A'+i;
		if (GetDriveType(ldstr) != DRIVE_CDROM) lds &= ~mask;
		else numdevs++;
	}

	rl = (char**)calloc(1, sizeof(char*)*(numdevs+1)+(2*numdevs));
	if (!rl) return NULL;

	p = (char*)rl + sizeof(char*)*(numdevs+1);
	rp = rl;
	for (i=0; i<32; i++) {
		mask = 1<<i;
		if (!(lds & mask)) continue;
		*(rp++) = p;
		*(p++) = 'A'+i;
		*(p++) = 0;
	}

	if (def) *def = NULL;	// Windows doesn't tell us what the default CDA drive will be

	return rl;
	
}

bool CDA_Win32::IsTrackPlayable(int n) const
{
	if (!isvalid || (unsigned)n >= (unsigned)numtracks) return false;
	if (toc[n].isdata) return false;
	return true;
}

bool CDA_Win32::PlayTrack(int n)
{
	MCI_PLAY_PARMS mpp;

	if (!isvalid || CheckDisc() != READY) return false;
	if ((unsigned)n >= (unsigned)numtracks) return false;

	memset(&mpp, 0, sizeof(mpp));
	mpp.dwFrom = toMSF(toc[n].start);
	mpp.dwTo   = toMSF(toc[n].end);

	if (sendCommand(MCI_PLAY, MCI_FROM|MCI_TO, (DWORD)&mpp)) return false;
	
	pausepos = -1;
	playend  = toc[n].end;

	return true;
}

bool CDA_Win32::Pause()
{
	MCI_STATUS_PARMS msp;
	MCI_GENERIC_PARMS mgp;

	if (!isvalid || playend < 0) return false;
	if (pausepos >= 0) return true;

	memset(&msp, 0, sizeof(msp));
	memset(&mgp, 0, sizeof(mgp));

	msp.dwItem = MCI_STATUS_POSITION;
	if (sendCommand(MCI_STATUS, MCI_WAIT|MCI_STATUS_ITEM, (DWORD)&msp)) return false;
	pausepos = msp.dwReturn;

	if (sendCommand(MCI_STOP, 0, (DWORD)&mgp)) {
		pausepos = -1;
		return false;
	}

	return true;
}

bool CDA_Win32::Resume()
{
	MCI_PLAY_PARMS mpp;

	if (!isvalid || pausepos < 0) return false;

	memset(&mpp, 0, sizeof(mpp));
	mpp.dwFrom = pausepos;
	mpp.dwTo = toMSF(playend);
	pausepos = -1;

	if (sendCommand(MCI_PLAY, MCI_FROM|MCI_TO, (DWORD)&mpp)) return false;
	return true;
}

JFAudCDA::State CDA_Win32::CheckDisc()
{
        MCI_STATUS_PARMS msp;

        if (!isvalid) return NOT_READY;

        memset(&msp, 0, sizeof(msp));
        msp.dwItem = MCI_STATUS_MEDIA_PRESENT;

        if (sendCommand(MCI_STATUS, MCI_WAIT|MCI_STATUS_ITEM, (DWORD)&msp))
		return NOT_READY;

	if ((laststate == NOT_READY && msp.dwReturn) ||
	    (laststate == READY && !msp.dwReturn)) {
		if (!msp.dwReturn || !readTOC()) {
			numtracks = 0;
			pausepos = playend = -1;
			laststate = NOT_READY;
		} else {
#ifdef DEBUG
			int i;
			for (i=0;i<numtracks;i++) {
				_JFAud_LogMsg(" Trk%2d start=%d end=%d isdata=%d\n",
					i, toc[i].start, toc[i].end, toc[i].isdata);
			}
#endif
			laststate = READY;
		}
	}

#ifdef DEBUG
	_JFAud_LogMsg("CD device is %s\n", laststate == READY ? "READY" : "NOT_READY");
#endif
	return laststate;
}

JFAudCDA::State CDA_Win32::GetPlayMode()
{
	MCI_STATUS_PARMS msp;

	if (!isvalid) return NOT_READY;

	memset(&msp, 0, sizeof(msp));
	msp.dwItem = MCI_STATUS_MODE;

	if (sendCommand(MCI_STATUS, MCI_WAIT|MCI_STATUS_ITEM, (DWORD)&msp))
		return NOT_READY;

	switch (msp.dwReturn) {
		case MCI_MODE_PAUSE: return PAUSED;
		case MCI_MODE_PLAY: return PLAYING;
		case MCI_MODE_STOP:
			if (pausepos >= 0) return PAUSED;
			return READY;
		default:
			return NOT_READY;
	}
}

bool CDA_Win32::readTOC()
{
	MCI_STATUS_PARMS msp;
	int i;

	numtracks = 0;

	memset(&msp, 0, sizeof(msp));
	msp.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;

	if (sendCommand(MCI_STATUS, MCI_WAIT | MCI_STATUS_ITEM, (DWORD)&msp)) return false;
	numtracks = msp.dwReturn;

	memset(&msp, 0, sizeof(msp));
	for (i=0; i<numtracks; i++) {
		msp.dwTrack = i+1;

		msp.dwItem = MCI_STATUS_POSITION;
		if (sendCommand(MCI_STATUS, MCI_WAIT | MCI_STATUS_ITEM | MCI_TRACK, (DWORD)&msp)) {
			numtracks = 0;
			return false;
		}

		toc[i].start  = MCI_MSF_FRAME(msp.dwReturn);
		toc[i].start += MCI_MSF_SECOND(msp.dwReturn)*75;
		toc[i].start += MCI_MSF_MINUTE(msp.dwReturn)*75*60;

		msp.dwItem = MCI_STATUS_LENGTH;
		if (sendCommand(MCI_STATUS, MCI_WAIT | MCI_STATUS_ITEM | MCI_TRACK, (DWORD)&msp)) {
			numtracks = 0;
			return false;
		}

		toc[i].end  = toc[i].start;
		toc[i].end += MCI_MSF_FRAME(msp.dwReturn);
		toc[i].end += MCI_MSF_SECOND(msp.dwReturn)*75;
		toc[i].end += MCI_MSF_MINUTE(msp.dwReturn)*75*60;
		toc[i].end -= 1;

		msp.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
		if (sendCommand(MCI_STATUS, MCI_WAIT | MCI_STATUS_ITEM | MCI_TRACK, (DWORD)&msp)) {
			numtracks = 0;
			return false;
		}

		toc[i].isdata = (msp.dwReturn == MCI_CDA_TRACK_OTHER);
	}

	return true;
}

