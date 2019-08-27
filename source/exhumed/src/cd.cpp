
#include "compat.h"
#include "baselayer.h"
#include "cd.h"
#include <stdio.h>
#include <stdlib.h>

extern short word_9AC30;

int cd_check_device_present()
{
	return 1;
}

int initcdaudio()
{
	if (!cd_check_device_present())
	{
		word_9AC30 = 1;

		// return to text video mode
		initprintf("No MSCDEX driver installed!\n");
		exit(0);
	}
	
	return 1;
}

void setCDaudiovolume(int val)
{

}

int playCDtrack(int nTrack)
{
	return 1;
}

void StartfadeCDaudio()
{

}

int StepFadeCDaudio()
{
	return 0;
}

int CDplaying()
{
	return 1;
}

void StopCD()
{

}