#ifndef __cda_win32_hpp__
#define __cda_win32_hpp__

#include "cda.hpp"

class CDA_Win32 : public JFAudCDA {
private:
	struct toc {
		unsigned int start;
		unsigned int end;
		unsigned int isdata;
	} toc[99];
	int numtracks, pausepos, playend;
	bool isvalid;
	State laststate;

	bool readTOC();

protected:
public:
	CDA_Win32(const char *name);
	virtual ~CDA_Win32();
	virtual bool IsValid() const { return isvalid; }
	static char **Enumerate(char **def);

	virtual int GetNumTracks() const { return numtracks; }
	virtual bool IsTrackPlayable(int n) const;
	virtual bool PlayTrack(int n);
	virtual bool Pause();
	virtual bool Resume();

	virtual State CheckDisc();
	virtual State GetPlayMode();
};

#endif
