#define JFAUD_INTERNAL
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstdlib"
#else
# include <cstdlib>
#endif

#include "sysdefs.h"
#include "log.h"
#include "cda_sdl.hpp"
#if defined __APPLE__
# include <SDL/SDL.h>
#else
# include "SDL.h"
#endif

#ifndef SCREWED_UP_CPP
using namespace std;
#endif

static SDL_CD *cddev = NULL;

CDA_SDL::CDA_SDL(const char *name)
	: isvalid(false)
{
	int i = 0;

	if (cddev) return;	// one device at a time, for simplicity
	if (!SDL_WasInit(SDL_INIT_CDROM)) return;
	
	if (SDL_CDNumDrives() < 1)
		return;

	if (name) i = atoi(name);
	if ((unsigned) i > (unsigned)SDL_CDNumDrives()) return;
	
#ifdef DEBUG
	_JFAud_LogMsg("CDA_SDL::CDA_SDL(): opening drive %d (%s)\n", i, SDL_CDName(i));
#endif
	cddev = SDL_CDOpen(i);
	if (!cddev) return;

	isvalid = true;
}

CDA_SDL::~CDA_SDL()
{
	if (isvalid && cddev) {
		SDL_CDClose(cddev);
		cddev = NULL;
	}
}

char **CDA_SDL::Enumerate(char **def)
{
	return NULL;	// FIXME: implement
}

int CDA_SDL::GetNumTracks() const
{
	if (!isvalid) return 0;

	if (SDL_CDStatus(cddev) <= CD_TRAYEMPTY) return 0;
	return cddev->numtracks;
}

bool CDA_SDL::IsTrackPlayable(int n) const
{
	if (!isvalid) return false;

	if (SDL_CDStatus(cddev) <= CD_TRAYEMPTY) return false;
	if ((unsigned) n >= (unsigned)cddev->numtracks) return false;

	return (cddev->track[n].type == SDL_AUDIO_TRACK);
}

bool CDA_SDL::PlayTrack(int n)
{
	if (!isvalid) return false;

	if (SDL_CDStatus(cddev) <= CD_TRAYEMPTY) return false;
	if ((unsigned) n >= (unsigned)cddev->numtracks) return false;
	if (cddev->track[n].type != SDL_AUDIO_TRACK) return false;

	return (SDL_CDPlay(cddev, cddev->track[n].offset, cddev->track[n].length) == 0);
}

bool CDA_SDL::Pause()
{
	if (!isvalid) return false;
	return (SDL_CDPause(cddev) == 0);
}

bool CDA_SDL::Resume()
{
	if (!isvalid) return false;
	return (SDL_CDResume(cddev) == 0);
}

JFAudCDA::State CDA_SDL::CheckDisc()
{
	if (!isvalid) return JFAudCDA::NOT_READY;
	
	return (SDL_CDStatus(cddev) <= CD_TRAYEMPTY) ? JFAudCDA::NOT_READY : JFAudCDA::READY;
}

JFAudCDA::State CDA_SDL::GetPlayMode()
{
	if (!isvalid) return JFAudCDA::NOT_READY;

	switch (SDL_CDStatus(cddev)) {
		case CD_STOPPED: return JFAudCDA::READY;
		case CD_PLAYING: return JFAudCDA::PLAYING;
		case CD_PAUSED:  return JFAudCDA::PAUSED;
		default: return JFAudCDA::NOT_READY;
	}
}

