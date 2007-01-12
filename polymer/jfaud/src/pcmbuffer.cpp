#define JFAUD_INTERNAL
#include "sysdefs.h"
#ifdef SCREWED_UP_CPP
# include "watcomhax/cstdlib"
#else
# include <cstdlib>
#endif

#include "pcmbuffer.hpp"

#ifndef SCREWED_UP_CPP
using namespace std;
#endif

PcmBuffer::~PcmBuffer()
{
	if (data) free(data);
}

bool PcmBuffer::Allocate(unsigned int nsamp, unsigned int srate, unsigned int chans, unsigned int bps)
{
	if (data) free(data);
	numsamples     = 0;
	maxsamples     = nsamp;
	samplerate     = srate;
	channels       = chans;
	bytespersample = bps;
	data = calloc(channels*bytespersample, maxsamples);
	return (data!=NULL);
}

bool PcmBuffer::ConvertToNByte(int n)
{
	if (n == bytespersample) return true;	// nothing to do
	if (n < 1 || n > 2) return false;
	
	if (n > bytespersample) {	// enlarging
		unsigned char *p;

		p = (unsigned char *)realloc(data, numsamples * n * channels);
		if (!p) return false;
		
		data = p;
		maxsamples = numsamples;
		if (bytespersample == 1 && n == 2) {
			// convert 8bit to 16bit
			short *to;
			unsigned char *from;
			int i;
			to = (short*)data + numsamples * channels - 1;
			from = (unsigned char*)data + numsamples * channels - 1;
			for (i = numsamples*channels-1; i>=0; i--)
				*(to--) = (short)(*(from--) ^ 128) << 8;
		} else {
			// convert everything else
			int copy, zero, i, j;
			unsigned char *q;
			copy = bytespersample-1;
			zero = n - bytespersample-1;
			q = (unsigned char *)data + numsamples * n * channels - 1;
			p = (unsigned char *)data + numsamples * bytespersample * channels - 1;
			for (j = numsamples*channels-1; j>=0; j--) {
#if B_BIG_ENDIAN != 0
				for (i = zero; i>=0; i--) *(q--) = 0;
#endif
				if (bytespersample == 1) *(q--) = *(p--) ^ 128;
				else for (i = copy; i>=0; i--) *(q--) = *(p--);
#if B_LITTLE_ENDIAN != 0
				for (i = zero; i>=0; i--) *(q--) = 0;
#endif
			}
		}
	} else {	// shrinking
		int copy, zero, i, j;
		unsigned char *p, *q;
		zero = bytespersample - n;
		p = q = (unsigned char *)data;
#if B_LITTLE_ENDIAN != 0
		p += zero;
#endif
		for (j = numsamples*channels-1; j>=0; j--) {
			if (n == 1) *(q++) = *(p++) ^ 128;
			else for (i = bytespersample; i>0; i--) *(q++) = *(p++);
			p += zero;
		}
	}
	bytespersample = n;
	return true;
}
