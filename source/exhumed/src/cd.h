
#ifndef __cd_h__
#define __cd_h__

int initcdaudio();
void setCDaudiovolume(int val);
int playCDtrack(int nTrack);
void StartfadeCDaudio();
int StepFadeCDaudio();
int CDplaying();
void StopCD();

#endif
