#ifndef __cda_null_hpp__
#define __cda_null_hpp__

#include "cda.hpp"

class CDA_Null : public JFAudCDA {
private:

protected:
public:
	CDA_Null(const char *name);
	virtual ~CDA_Null();
	virtual bool IsValid() const;

	virtual int GetNumTracks() const { return 0; }
	virtual bool IsTrackPlayable(int n) const { return false; }
	virtual bool PlayTrack(int n);
	virtual bool Pause();
	virtual bool Resume();

	virtual State CheckDisc();
	virtual State GetPlayMode();
};

#endif
