#define JFAUD_INTERNAL
#include "sysdefs.h"
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstdlib"
# include "watcomhax/cstring"
# include "watcomhax/cmath"
# include "watcomhax/cstdio"
#else
# include <cstdlib>
# include <cstring>
# include <cmath>
# include <cstdio>
#endif
#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif
#include "softwaremixer.hpp"
#include "waveout.hpp"
#include "log.h"

#ifndef SCREWED_UP_CPP
using namespace std;
#endif

#define STREAMCHUNK	22050		// samples. 1sec @ 22.05KHz
//#define TESTTONE

#define CHANGED_BUFFER 1
#define CHANGED_RATE 2
#define CHANGED_POSITION 4

#define SHIFTFACT 8
#define OUTFACT   (8+2)
//#define SHIFTFACT 8
//#define OUTFACT (8+1)

//{{{ Distance attenuation functions
float SoftwareMixerChannel::DistanceLinear(float distance, float refdist, float maxdist, float rolloff)
{
	if (rolloff <= 0.0) return 1.0;
	if (distance <= refdist) return 1.0;
	if (distance >= maxdist) return 0.0;
	return 1.0-rolloff*((distance-refdist)/(maxdist-refdist));
}

float SoftwareMixerChannel::DistanceInverse(float distance, float refdist, float maxdist, float rolloff)
{
	if (rolloff <= 0.0) return 1.0;
	if (distance <= refdist) return 1.0;
	if (distance >= maxdist) return 0.0;
	float f = refdist + (rolloff * (distance - refdist));
	if (f <= 0.0) return 1.0;
	return refdist/f;
}
//}}}

//{{{ Sample interpolation functions
#define COEFTABLESIZE 5	// 1 LSH this many bits
static short CoefTable4[1<<COEFTABLESIZE][4];/* = {
	{      0,      0,  32767,      0, },
	{      0,   -472,  32694,    553, },
	{      2,   -864,  32478,   1191, },
	{      7,  -1181,  32120,   1915, },
	{     18,  -1428,  31624,   2725, },
	{     34,  -1610,  30995,   3621, },
	{     57,  -1732,  30239,   4601, },
	{     87,  -1801,  29363,   5661, },
	{    124,  -1822,  28377,   6798, },
	{    169,  -1802,  27290,   8006, },
	{    221,  -1747,  26112,   9280, },
	{    279,  -1664,  24855,  10611, },
	{    341,  -1558,  23531,  11991, },
	{    408,  -1435,  22151,  13411, },
	{    476,  -1301,  20728,  14861, },
	{    544,  -1161,  19275,  16329, },
	{    610,  -1019,  17805,  17805, },
	{    673,   -879,  16329,  19275, },
	{    729,   -744,  14861,  20728, },
	{    777,   -617,  13411,  22151, },
	{    815,   -500,  11991,  23531, },
	{    841,   -396,  10611,  24855, },
	{    852,   -304,   9280,  26112, },
	{    848,   -226,   8006,  27290, },
	{    827,   -161,   6798,  28377, },
	{    788,   -109,   5661,  29363, },
	{    731,    -69,   4601,  30239, },
	{    654,    -40,   3621,  30995, },
	{    558,    -21,   2725,  31624, },
	{    444,     -9,   1915,  32120, },
	{    312,     -3,   1191,  32478, },
	{    163,     -1,    553,  32694, },
};*/


int SoftwareMixerChannel::InterpolateNearest(SoftwareMixerChannel *chan, int offset, int stride)
{
	return (int)chan->bufferp[ (chan->samplepos>>32)*stride+offset ];
}

int SoftwareMixerChannel::InterpolateLinear(SoftwareMixerChannel *chan, int offset, int stride)
{
	int sample1, sample2;
	int s;
	unsigned int sub;

	s = chan->samplepos>>32;
	sample1 = chan->bufferp[ s*stride + offset ];
	s++;
	if (s < chan->buffer->GetNumSamples()) {
		sample2 = chan->bufferp[ s*stride + offset ];
	} else {
		s -= chan->buffer->GetNumSamples();
		if (!chan->streamed) {
			sample2 = chan->bufferp[ s*stride + offset ];
		} else {
			if (!chan->buffer2) return sample1;	// no buffer2 yet so the best we can do is nearest
			if (chan->buffer2->GetNumSamples() <= s) return sample1;	// somehow we jumped buffer2 completely
			sample2 = ((short*)chan->buffer2->GetData())[ s*stride + offset ];
		}
	}
	sub = chan->samplepos & INT64_C(0xffffffff);
	return sample1 + ((int)((sample2-sample1)*(sub>>17)) >> 15);
}

int SoftwareMixerChannel::Interpolate4Point(SoftwareMixerChannel *chan, int offset, int stride)
{
	const int ncoefs = 4;
	int samples[ncoefs], mixed = 0;
	unsigned int sub;
	int s, i;

	short *bufp[3], *coeftable;
	unsigned int bufs[3];
	
	s = (chan->samplepos >> 32) - (ncoefs / 2);
	sub = (chan->samplepos & INT64_C(0xffffffff)) >> (32-COEFTABLESIZE);

	bufp[0] = bufp[2] = NULL;
	bufs[0] = bufs[2] = 0;
	bufp[1] = (short *)chan->buffer->GetData();
	bufs[1] = chan->buffer->GetNumSamples();
	if (!chan->streamed) {
		if (chan->loop) {
			bufp[0] = bufp[2] = bufp[1];
			bufs[0] = bufs[2] = bufs[1];
		}
	} else {
		if (chan->buffer0) {
			bufp[0] = (short *)chan->buffer0->GetData();
			bufs[0] = chan->buffer0->GetNumSamples();
		}
		if (chan->buffer2) {
			bufp[2] = (short *)chan->buffer2->GetData();
			bufs[2] = chan->buffer2->GetNumSamples();
		}
	}

	memset(samples, 0, sizeof(samples));
	for (i=0; i<ncoefs; i++, s++) {
		if ((unsigned int)s < bufs[1]) {
			samples[i] = bufp[1][ s*stride + offset ];
		} else if (s < 0) {
			if (bufp[0]) samples[i] = bufp[0][ (s + bufs[0])*stride + offset ];
		} else {
			if (bufp[2]) samples[i] = bufp[2][ (s - bufs[1])*stride + offset ];
		}
	}

	coeftable = CoefTable4[sub];
	for (i=0; i<ncoefs; i++, s++)
		mixed += (samples[i] * coeftable[i]) >> 16;
	
	if (mixed < -32768) return -32768;
	else if (mixed > 32767) return 32767;
	return mixed;
}
//}}}

//{{{ n-to-n mixing functions
void SoftwareMixerChannel::Mix1To1(SoftwareMixerChannel *chan, int **mmixbuffer, int *mmb, int v1, int v2)
{
	int *mixbuffer = *mmixbuffer, mb = *mmb, s, v = v1+v2;
	for (;
	     mb >= 0 && (int)(chan->samplepos>>32) < chan->buffer->GetNumSamples();
	     chan->samplepos += chan->sampleinc, mb--) {
		s = chan->filterfunc(chan, 0, 1) << SHIFTFACT;
		*(mixbuffer++) += s * v / 256;
	}
	*mmixbuffer = mixbuffer; *mmb = mb;
}
void SoftwareMixerChannel::Mix1To2(SoftwareMixerChannel *chan, int **mmixbuffer, int *mmb, int v1, int v2)
{
	int *mixbuffer = *mmixbuffer, mb = *mmb, s;
	for (;
	     mb >= 0 && (int)(chan->samplepos>>32) < chan->buffer->GetNumSamples();
	     chan->samplepos += chan->sampleinc, mb--) {
		s = chan->filterfunc(chan, 0, 1) << SHIFTFACT;
		*(mixbuffer++) += s * v1 / 256;
		*(mixbuffer++) += s * v2 / 256;
	}
	*mmixbuffer = mixbuffer; *mmb = mb;
}
void SoftwareMixerChannel::Mix2To1(SoftwareMixerChannel *chan, int **mmixbuffer, int *mmb, int v1, int v2)
{
	int *mixbuffer = *mmixbuffer, mb = *mmb, s, v = v1+v2;
	for (;
	     mb >= 0 && (int)(chan->samplepos>>32) < chan->buffer->GetNumSamples();
	     chan->samplepos += chan->sampleinc, mb--) {
		s = ((chan->filterfunc(chan,0,2) + chan->filterfunc(chan,1,2)) << SHIFTFACT) / 2;
		*(mixbuffer++) += s * v / 256;
	}
	*mmixbuffer = mixbuffer; *mmb = mb;
}
void SoftwareMixerChannel::Mix2To2(SoftwareMixerChannel *chan, int **mmixbuffer, int *mmb, int v1, int v2)
{
	int *mixbuffer = *mmixbuffer, mb = *mmb, s1, s2;
	for (;
	     mb >= 0 && (int)(chan->samplepos>>32) < chan->buffer->GetNumSamples();
	     chan->samplepos += chan->sampleinc, mb--) {
		s1 = chan->filterfunc(chan,0,2) << SHIFTFACT;
		s2 = chan->filterfunc(chan,1,2) << SHIFTFACT;
		*(mixbuffer++) += s1 * v1 / 256;
		*(mixbuffer++) += s2 * v2 / 256;
	}
	*mmixbuffer = mixbuffer; *mmb = mb;
}
void SoftwareMixerChannel::DownMix2To2(SoftwareMixerChannel *chan, int **mmixbuffer, int *mmb, int v1, int v2)
{
	int *mixbuffer = *mmixbuffer, mb = *mmb, s;
	for (;
	     mb >= 0 && (int)(chan->samplepos>>32) < chan->buffer->GetNumSamples();
	     chan->samplepos += chan->sampleinc, mb--) {
		s = ((chan->filterfunc(chan,0,2) + chan->filterfunc(chan,1,2)) << SHIFTFACT) / 2;
		*(mixbuffer++) += s * v1 / 256;
		*(mixbuffer++) += s * v2 / 256;
	}
	*mmixbuffer = mixbuffer; *mmb = mb;
}
//}}}


void SoftwareMixerChannel::MixSome(int *mixbuffer, int mb)
{
	int i;

	if (state != Playing || !media) return;	// paused or stopped
	
	for (mb -= 1; mb >= 0; ) {
		if (!UpdateProps() || !mixfunc) return;

		mixfunc(this, &mixbuffer, &mb, volumes[0], volumes[1]);

		if ((int)(samplepos >> 32) >= buffer->GetNumSamples()) {
			samplepos -= (int64_t)buffer->GetNumSamples() << 32;
			if (streamed) {
				if (buffer0) delete buffer0;
				buffer0 = buffer;
				buffer = buffer2;
				buffer2 = NULL;
				changed |= CHANGED_BUFFER;
			} else if (!loop) {
				state = Stopped;
				if (stopcallback) stopcallback(stopcallbackid);
				return;
			}
		}
	}
}

bool SoftwareMixerChannel::UpdateProps(void)
{
	if (!media) return false;

	if ((!streamed && !buffer) || (streamed && !buffer2)) {
		buffer2 = media->ReadSamples(streamed ? STREAMCHUNK : 0, streamed ? loop : false);
		if (buffer2 && (buffer2->GetNumSamples() == 0 || !buffer2->ConvertToNByte(2))) {
			delete buffer2;
			buffer2 = NULL;
		}
		
		if (!buffer) {
			buffer = buffer2;
			buffer2 = NULL;
			changed |= CHANGED_BUFFER;
		}
	}
	
	if (!buffer) {
		// error, or out of data
		state = Stopped;
		if (stopcallback) stopcallback(stopcallbackid);
		return false;
	}
	if (changed) {
		if (changed & CHANGED_BUFFER) {
			bufferp = (short *)buffer->GetData();
			if (buffer->GetSampleRate() != oldsamplerate) {
				changed |= CHANGED_RATE;
				oldsamplerate = buffer->GetSampleRate();
			}

			if (buffer->GetNumChannels() == 1 && owner->chans == 2) {
				mixfunc = Mix1To2;
			} else if (buffer->GetNumChannels() == 2 && owner->chans == 2) {
				if (follow) mixfunc = Mix2To2;
				else mixfunc = DownMix2To2;
			} else if (buffer->GetNumChannels() == 1 && owner->chans == 1) {
				mixfunc = Mix1To1;
			} else if (buffer->GetNumChannels() == 2 && owner->chans == 1) {
				mixfunc = Mix2To1;
			}
		}
		if (changed & CHANGED_RATE) {
			sampleinc = (int64_t)(((double)buffer->GetSampleRate() * pitch / owner->frequency) * (INT64_C(1)<<32));
		}
		if (changed & CHANGED_POSITION) {
			float distance, atten;
			float dx, dy, dz;
			float t, b;
			
			// owner->at[xyz] is a unit vector
			
			if (follow) {
				dx = posx;
				dy = posy;
				dz = posz;
			} else {
				dx = posx - owner->posx;
				dy = posy - owner->posy;
				dz = posz - owner->posz;
			}

			distance = sqrt(dx*dx + dy*dy + dz*dz);
			if (distance >= maxdist) {
				volumes[0] = volumes[1] = 0;
			} else {
				if (distance <= refdist) atten = gain*128.0;
				else atten = gain*128.0*distfunc(distance,refdist,maxdist,rolloff);
			
				b = dx*dx + dz*dz;
				if (b <= 0.0) {
					volumes[0] = volumes[1] = (int)(2.0*atten);
				} else {
					t = (dx*owner->atz - dz*owner->atx)/sqrt(b);
					volumes[0] = (int)((1.0+t) * atten);
					volumes[1] = (int)((1.0-t) * atten);
				}
			}
			//_JFAud_LogMsg("d=%+f,%+f,%+f distance=%+f atten=%+f t=%+f l=%d r=%d\n",dx,dy,dz,distance,atten,t,volumes[1],volumes[0]);
		}
		changed = 0;
	}

	return true;
}

void SoftwareMixerChannel::Cleanup(void)
{
	if (stopcallback && state != Stopped) {
		stopcallback(stopcallbackid);
	}
	stopcallback = NULL;
	stopcallbackid = 0;
	
	samplepos = 0;
	mixfunc = NULL;
	filterfunc = InterpolateLinear;
	distfunc = DistanceInverse;
	state = Stopped;
	gain = 1.0;
	pitch = 1.0;
	posx = posy = posz = 0.0;
	refdist = 1.0;
	maxdist = 10.0;
	rolloff = 1.0;
	streamed = false;
	loop = false;
	follow = false;
	
	if (buffer0) delete buffer0;
	if (buffer) delete buffer;
	if (buffer2) delete buffer2;
	if (media) delete media;
	buffer0 = NULL;
	buffer = NULL;
	buffer2 = NULL;
	oldsamplerate = 0;
	volumes[0] = volumes[0] = 0;
	changed = 0;
	media = NULL;
}

SoftwareMixerChannel::SoftwareMixerChannel(SoftwareMixer *own)
	: owner(own), media(NULL),
	  buffer0(NULL), buffer(NULL), buffer2(NULL),
	  oldsamplerate(0), samplepos(0), sampleinc(0),
	  bufferp(NULL), changed(0),
	  mixfunc(NULL), filterfunc(InterpolateLinear), distfunc(DistanceInverse),
	  state(Stopped), gain(1.0), pitch(1.0),
	  posx(0.0), posy(0.0), posz(0.0),
	  refdist(1.0), maxdist(10.0), rolloff(1.0),
	  streamed(false), loop(false), follow(false)
{
	volumes[0] = volumes[1] = 0;
}

SoftwareMixerChannel::~SoftwareMixerChannel()
{
	Cleanup();
}

bool SoftwareMixerChannel::SetMedia(WaveformFile *file)
{
	if (!file) return false;
	
	owner->waveout->Lock();
	if (media) {
		// clean up after our predecessor
		delete media;
		media = NULL;
	}
	
	media = file;
	
	if (file->GetPCMLength() >= (2*STREAMCHUNK)) streamed = true;
	else streamed = false;
	changed = -1;
	owner->waveout->Unlock();
	
	return true;
}

bool SoftwareMixerChannel::SetFilter(Filter which)
{
	switch (which) {
		case Filter4Point:
			owner->waveout->Lock();
			filterfunc = Interpolate4Point;
			owner->waveout->Unlock();
			return true;
		case FilterLinear:
			owner->waveout->Lock();
			filterfunc = InterpolateLinear;
			owner->waveout->Unlock();
			return true;
		case FilterNearest:
			owner->waveout->Lock();
			filterfunc = InterpolateNearest;
			owner->waveout->Unlock();
			return true;
		default:
			return false;
	}
}

bool SoftwareMixerChannel::SetDistanceModel(DistanceModel which)
{
	switch (which) {
		case DistanceModelInverse:
			owner->waveout->Lock();
			distfunc = DistanceInverse;
			owner->waveout->Unlock();
			return true;
		case DistanceModelLinear:
			owner->waveout->Lock();
			distfunc = DistanceLinear;
			owner->waveout->Unlock();
			return true;
		default:
			return false;
	}
}

bool SoftwareMixerChannel::Play(void)
{
	if (!media) return false;
	owner->waveout->Lock();
	state = Playing;
	UpdateProps();
	owner->waveout->Unlock();
	return true;
}

bool SoftwareMixerChannel::Pause(void)
{
	if (!media) return false;
	owner->waveout->Lock();
	state = Paused;
	owner->waveout->Unlock();
	return true;
}

bool SoftwareMixerChannel::Update(void)
{
	return true;
}

bool SoftwareMixerChannel::IsPlaying(void) const { return state == Playing; }
bool SoftwareMixerChannel::IsPaused(void) const { return state == Paused; }
bool SoftwareMixerChannel::IsStopped(void) const { return state == Stopped; }

bool SoftwareMixerChannel::SetGain(float gain)
{
	owner->waveout->Lock();
	this->gain = gain;
	changed |= CHANGED_POSITION;
	owner->waveout->Unlock();
	return true;
}

bool SoftwareMixerChannel::SetPitch(float pitch)
{
	owner->waveout->Lock();
	this->pitch = pitch;
	changed |= CHANGED_RATE;
	owner->waveout->Unlock();
	return true;
}

bool SoftwareMixerChannel::SetPosition(float x, float y, float z)
{
	owner->waveout->Lock();
	posx = x; posy = y; posz = z;
	changed |= CHANGED_POSITION;
	owner->waveout->Unlock();
	return true;
}

bool SoftwareMixerChannel::SetVelocity(float x, float y, float z) { return false; }
bool SoftwareMixerChannel::SetDirection(float x, float y, float z) { return false; }

bool SoftwareMixerChannel::SetRefDist(float refdist)
{
	owner->waveout->Lock();
	this->refdist = refdist;
	changed |= CHANGED_POSITION;
	owner->waveout->Unlock();
	return true;
}

bool SoftwareMixerChannel::SetMaxDist(float maxdist)
{
	owner->waveout->Lock();
	this->maxdist = maxdist;
	changed |= CHANGED_POSITION;
	owner->waveout->Unlock();
	return true;
}

bool SoftwareMixerChannel::SetRolloff(float rolloff)
{
	owner->waveout->Lock();
	this->rolloff = rolloff;
	changed |= CHANGED_POSITION;
	owner->waveout->Unlock();
	return true;
}

bool SoftwareMixerChannel::SetLoop(bool onf)
{
	owner->waveout->Lock();
	loop = onf;
	owner->waveout->Unlock();
	return true;
}

bool SoftwareMixerChannel::SetFollowListener(bool onf)
{
	owner->waveout->Lock();
	follow = onf;
	changed |= CHANGED_POSITION;
	owner->waveout->Unlock();
	return true;
}

float SoftwareMixerChannel::GetGain(void) const { return gain; }
float SoftwareMixerChannel::GetPitch(void) const { return pitch; }
void SoftwareMixerChannel::GetPosition(float *x, float *y, float *z) const { *x = posx; *y = posy; *z = posz; }
void SoftwareMixerChannel::GetVelocity(float *x, float *y, float *z) const { *x = *y = *z = 0.0; }
void SoftwareMixerChannel::GetDirection(float *x, float *y, float *z) const { *x = *y = *z = 0.0; }
float SoftwareMixerChannel::GetRefDist(void) const { return refdist; }
float SoftwareMixerChannel::GetMaxDist(void) const { return maxdist; }
float SoftwareMixerChannel::GetRolloff(void) const { return rolloff; }
bool SoftwareMixerChannel::GetLoop(void) const { return loop; }
bool SoftwareMixerChannel::GetFollowListener(void) const { return follow; }

SoftwareMixer::SoftwareMixer()
	: waveout(NULL), voices(NULL), nvoices(0),
	  frequency(0), bytesamp(0), chans(0),
	  mixbuffer(NULL), mixbufferlen(0), mixbufferused(0), mixbufferpos(0),
	  posx(0.0), posy(0.0), posz(0.0),
	  atx(0.0), aty(1.0), atz(0.0), upx(0.0), upy(0.0), upz(1.0),
	  velx(0.0), vely(0.0), velz(0.0),
	  gain(1.0)
{
	// build the 4 coefficient interpolation table
	const int ncoefs = 4;
	double sinc, window, xx, sub;
	int i, j;
	for (j = 0; j < (1<<COEFTABLESIZE); j++) {
		for (i = 0; i < ncoefs; i++) {
			sub = (double)j/(double)(1<<COEFTABLESIZE);
			xx = (double)(i-ncoefs/2)-sub;
			if (xx == 0.0) sinc = 1.0; else { sinc = xx*M_PI; sinc = sin(sinc)/sinc; }
			window = cos(xx*M_PI*2.0/(double)ncoefs)*0.5+0.5;
			CoefTable4[j][i] = (int)floor(32767.0*(sinc*window));
		}
	}
}

SoftwareMixer::~SoftwareMixer()
{
	if (voices) {
		for (int i=nvoices-1;i>=0;i--) if (voices[i].chan) delete voices[i].chan;
		delete [] voices;
	}
	if (mixbuffer) delete [] mixbuffer;
}

void SoftwareMixer::DoMix(void)
{
	int i;
	if (mixbufferpos >= mixbufferused) {
		memset(mixbuffer, 0, sizeof(int)*mixbufferlen*chans);
		for (i=nvoices-1; i>=0; i--) {
			if (!voices[i].used) continue;
			voices[i].chan->MixSome(mixbuffer, mixbufferlen);
		}
		
		mixbufferpos = 0;
		mixbufferused = mixbufferlen;
	}
}

bool SoftwareMixer::Setup(WaveOut *dev, int maxvoices)
{
	int i;

	waveout   = dev;
	frequency = dev->GetSampleRate();
	chans     = dev->GetChannels();
	bytesamp  = dev->GetBitsPerSample() / 8;

	// the size of our internal 32bit mixing buffer should contain at
	// least enough samples to match what the wave output device is
	// likely to ask for at any one time
	mixbufferlen = dev->GetMixBufferLen();
	mixbufferpos = mixbufferused = 0;
	mixbuffer = new int[mixbufferlen*chans];
	if (!mixbuffer) return false;

	voices = new struct _voicestat [maxvoices];
	if (!voices) { delete [] mixbuffer; mixbuffer = NULL; return false; }
	for (i=maxvoices-1;i>=0;i--) {
	       voices[i].used = false;
	       voices[i].chan = new SoftwareMixerChannel(this);
	       if (!voices[i].chan) {
		       for (i++; i<maxvoices; i++) delete voices[i].chan;
		       delete [] voices;
		       voices = NULL;
		       return false;
	       }
	}
	nvoices = maxvoices;
	
	return true;
}

JFAudMixerChannel *SoftwareMixer::AcquireChannel(void)
{
	int i;
	
	waveout->Lock();
	for (i=nvoices-1; i>=0; i--) {
		if (voices[i].used) continue;
		voices[i].used = true;
		waveout->Unlock();
		return static_cast<JFAudMixerChannel *>(voices[i].chan);
	}
	waveout->Unlock();
	return NULL;
}

bool SoftwareMixer::ReleaseChannel(JFAudMixerChannel *ch)
{
	int i;
	
	waveout->Lock();
	for (i=nvoices-1; i>=0; i--) {
		if (!voices[i].used) continue;
		if (voices[i].chan != static_cast<SoftwareMixerChannel *>(ch)) continue;
		voices[i].used = false;
		voices[i].chan->Cleanup();
		waveout->Unlock();
		return true;
	}
	waveout->Unlock();
	return false;
}

bool SoftwareMixer::Update()
{
	return true;
}

void SoftwareMixer::MixSome(void *buf, int bytes)
{
#ifdef TESTTONE
	int s;
	static int i = 0;
	unsigned char *c = (unsigned char *)buf;
	
	for (; bytes >= bytesamp; bytes -= bytesamp) {
		s = (int)((float)0x7fffffff * sin((float)i++ * 2.0 * M_PI * 440.0 / (float)frequency));
		
		if (bytesamp == 1) *(c++) = (unsigned char)((s>>24)+128);
		else if (bytesamp == 2) {
			*(short *)c = (short)(s>>16);
			c += 2;
		}			
	}
#else
	int i, *rp;
	char *bp = (char *)buf;
	int samples = bytes / (bytesamp*chans);
	
	// FIXME: assumes the caller isn't stupid enough to ask for a partial sample
	
	while (samples > 0) {
		DoMix();
		
		// convert and copy the 32bit samples from our mixing buffer to what is wanted in the output
		i = min(mixbufferlen - mixbufferpos, samples);
		switch (bytesamp) {
			case 1: {
				int n, *in = &mixbuffer[mixbufferpos*chans];
				unsigned char *out = (unsigned char *)bp;
				for (n = i*chans-1; n>=0; n--) *(out++) = (unsigned char)max(-128,min(127,(*(in++) >> 24))) ^ 128;
			} break;
			case 2: {
				int n, *in = &mixbuffer[mixbufferpos*chans];
				short *out = (short *)bp;
				for (n = i*chans-1; n>=0; n--, in++) *(out++) = (short)max(-32768,min(32767,(*in >> OUTFACT)));
			} break;
		}
		mixbufferpos += i;
		bp += bytesamp*chans*i;
		samples -= i;
	}
#endif
}

bool SoftwareMixer::SetListenerPosition(float x, float y, float z)
{
	int i;

	waveout->Lock();
	posx = x; posy = y; posz = z;
	for (i=nvoices-1; i>=0; i--) {
		if (!voices[i].used) continue;
		voices[i].chan->changed |= CHANGED_POSITION;
	}
	waveout->Unlock();
	return true;
}

bool SoftwareMixer::SetListenerOrientation(float atx, float aty, float atz, float upx, float upy, float upz)
{
	int i;
	
	// make sure everything passed is a unit vector
	if (atx == 0.0 && aty == 0.0 && atz == 0.0) aty = 1.0;
	else {
		float l = sqrt(atx*atx + aty*aty + atz*atz);
		atx /= l; aty /= l; atz /= l;
	}
	if (upx == 0.0 && upy == 0.0 && upz == 0.0) upz = 1.0;
	else {
		float l = sqrt(upx*upx + upy*upy + upz*upz);
		upx /= l; upy /= l; upz /= l;
	}

	waveout->Lock();
	this->atx = atx; this->aty = aty; this->atz = atz;
	this->upx = upx; this->upy = upy; this->upz = upz;
	for (i=nvoices-1; i>=0; i--) {
		if (!voices[i].used) continue;
		voices[i].chan->changed |= CHANGED_POSITION;
	}
	waveout->Unlock();
	return true;
}

bool SoftwareMixer::SetListenerVelocity(float x, float y, float z)
{
	waveout->Lock();
	velx = x; vely = y; velz = z;
	waveout->Unlock();
	return true;
}

bool SoftwareMixer::SetListenerGain(float gain)
{
	waveout->Lock();
	this->gain = gain;
	waveout->Unlock();
	return true;
}

void SoftwareMixer::GetListenerPosition(float *x, float *y, float *z) const
{
	*x = posx; *y = posy; *z = posz;
}

void SoftwareMixer::GetListenerOrientation(float *atx, float *aty, float *atz, float *upx, float *upy, float *upz) const
{
	*atx = this->atx; *aty = this->aty; *atz = this->atz;
	*upx = this->upx; *upy = this->upy; *upz = this->upz;
}

void SoftwareMixer::GetListenerVelocity(float *x, float *y, float *z) const
{
	*x = velx; *y = vely; *z = velz;
}

float SoftwareMixer::GetListenerGain(void) const
{
	return gain;
}

// vim:fdm=marker:

