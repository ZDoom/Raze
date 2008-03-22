#ifndef __OPENAL_H
#define __OPENAL_H

#include <vorbis/vorbisfile.h>

int AL_Init();
void AL_Shutdown();
void AL_Update();
char *AL_ErrorString(int code);
char *ALC_ErrorString(int code);

void AL_Stop();
int  AL_isntALmusic();
void AL_PlaySong(char *song,int loopflag);
void AL_Pause();
void AL_Continue();
void AL_SetMusicVolume(int volume);

int openal_disabled;

typedef struct sounddef
{
        unsigned pos;
        char *ptrsnd;
        unsigned size;
        OggVorbis_File  oggStream;
} sounddef;
#endif
