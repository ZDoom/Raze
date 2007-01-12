#define JFAUD_INTERNAL
#include "sysdefs.h"
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstdlib"
#else
# include <cstdlib>
#endif
#include "mixer.hpp"

#ifndef SCREWED_UP_CPP
using namespace std;
#endif

JFAudMixerChannel::JFAudMixerChannel()
	: stopcallback(NULL), stopcallbackid(0)
{
}

JFAudMixerChannel::~JFAudMixerChannel()
{
}

bool JFAudMixerChannel::SetStopCallback( void (*cb)(int), int id)
{
	stopcallback = cb;
	stopcallbackid = id;
	
	return true;
}

bool JFAudMixerChannel::SetFilter(Filter which)
{
	return false;	// not supported by default
}

bool JFAudMixerChannel::SetDistanceModel(DistanceModel which)
{
	return false;
}
