#ifndef __cda_hpp__
#define __cda_hpp__

class JFAudCDA {
private:
protected:
public:
	typedef enum { NOT_READY, READY, PLAYING, PAUSED } State;
	
	JFAudCDA() { }
	virtual ~JFAudCDA() { }
	virtual bool IsValid() const = 0;

	virtual int GetNumTracks() const = 0;
	virtual bool IsTrackPlayable(int n) const = 0;
	virtual bool PlayTrack(int n) = 0;
	virtual bool Pause() = 0;
	virtual bool Resume() = 0;

	virtual State CheckDisc() = 0;
	virtual State GetPlayMode() = 0;
};

#endif
