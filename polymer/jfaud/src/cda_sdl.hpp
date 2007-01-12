#ifndef __cda_sdl_hpp__
#define __cda_sdl_hpp__

#include "cda.hpp"

class CDA_SDL : public JFAudCDA {
private:
	bool isvalid;

protected:
public:
	CDA_SDL(const char *name);
	virtual ~CDA_SDL();
	virtual bool IsValid() const { return isvalid; }
	virtual char **Enumerate(char **def);

	virtual int GetNumTracks() const;
	virtual bool IsTrackPlayable(int n) const;
	virtual bool PlayTrack(int n);
	virtual bool Pause();
	virtual bool Resume();

	virtual State CheckDisc();
	virtual State GetPlayMode();
};

#endif
