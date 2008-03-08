#include "dsound.h"

#include "compat.h"
#include "winlayer.h"

#include <al/al.h>
#include <al/alc.h>
#include <vorbis/vorbisfile.h>
#include "_multivc.h"

ALCdevice  * device=NULL;
ALCcontext * context=NULL;

char *ALdoing="";
int AL_Error;
int bufsize;

typedef struct SD
{
    ALuint buffers[16];
    ALuint source;
    ALenum format;

    char loop;
    char type;
    int rate;
    sounddef def;
}sounddef1;

sounddef1 sounds1[2];

char *ALC_ErrorString(int code)
{
        switch(code)
        {
                case ALC_NO_ERROR:
                    return "OpenAL error: There is no current error.";
                case ALC_INVALID_DEVICE:
                    return "OpenAL error: No device.";
                case ALC_INVALID_CONTEXT:
                    return "OpenAL error: Invalid context ID.";
                case ALC_INVALID_ENUM:
                    return "OpenAL error: Invalid parameter.";
                case ALC_INVALID_VALUE:
                    return "OpenAL error: Invalid enum parameter value.";
                case ALC_OUT_OF_MEMORY:
                    return "OpenAL error: Unable to allocate memory.";
                default:
                    return "OpenAL error: Unknown error.";
        }
}
char *AL_ErrorString(int code)
{
        switch(code)
        {
                case AL_NO_ERROR:
                    return "OpenAL error: There is no current error.";
                case AL_INVALID_NAME:
                    return "OpenAL error: Invalid name parameter.";
                case AL_INVALID_ENUM:
                    return "OpenAL error: Invalid parameter.";
                case AL_INVALID_VALUE:
                    return "OpenAL error: Invalid enum parameter value.";
                case AL_INVALID_OPERATION:
                    return "OpenAL error: Illegal call.";
                case AL_OUT_OF_MEMORY:
                    return "OpenAL error: Unable to allocate memory.";
                case OV_EFAULT:
                    return "Internal logic fault (bug or heap/stack corruption.";
                case OV_EREAD:
                    return "Read from media.";
                case OV_EIMPL:
                    return "The bitstream makes use of a feature not implemented in this library version.";
                case OV_EINVAL:
                    return "Invalid argument value.";
                case OV_ENOTVORBIS:
                    return "Not Vorbis data.";
                case OV_EBADHEADER:
                    return "Invalid Vorbis header.";
                case OV_EVERSION:
                    return "Vorbis version mismatch.";
                case OV_ENOTAUDIO:
                    return "Packet data submitted to vorbis_synthesis is not audio data.";
                case OV_EBADPACKET:
                    return "Invalid packet submitted to vorbis_synthesis.";
                case OV_EBADLINK:
                    return "Invalid stream section supplied to libvorbis/libvorbisfile, or the requested link is corrupt.";
                case OV_ENOSEEK:
                    return "Bitstream is not seekable.";
                default:
                    return "Unknown OpenAL/Ogg error.";
        }
}
void check(int show)
{
    AL_Error=alcGetError(device);
    if(AL_Error!=ALC_NO_ERROR&&show)initprintf("%s(%s)\n",ALC_ErrorString(AL_Error),ALdoing);
    AL_Error=alGetError();
    if(AL_Error!= AL_NO_ERROR&&show)initprintf("%s(%s)\n", AL_ErrorString(AL_Error),ALdoing);
}
extern ov_callbacks cb;


void AL_Init()
{
    ALdoing="Init";
//    device=alcOpenDevice(ud.config.HardwareAL?"Generic Hardware":"Generic Software");
    device=alcOpenDevice("Generic Hardware");check(1);
    if(device){context=alcCreateContext(device,NULL);check(1);}
    if(context)
    {
       alcMakeContextCurrent(context);check(1);
       initprintf("OpenAL Information:\n"
                  " Version:  %s\n"
                  " Vendor:   %s\n\n",alGetString(AL_VERSION),alGetString(AL_VENDOR));
    } else initprintf("OpenAL initialisation failed. Try http://www.openal.org/\n");

    ALdoing="Open";
    alGenBuffers(16, sounds1[1].buffers);
    check(1);
    alGenSources(1,&sounds1[1].source);
    check(1);
}

void AL_Shutdown()
{
    ALdoing="Delete source";
    alDeleteSources(1,&sounds1[1].source);
    check(1);
    ALdoing="Delete buffers";
    alDeleteBuffers(16, sounds1[1].buffers);
    check(1);

    ALdoing="Shut";
    alcMakeContextCurrent(NULL);
    check(1);
    alcDestroyContext(context);
    check(1);
    alcCloseDevice(device);
}

#define BUFFER_SIZE (4096 * 4*8*8)
int AL_MusicVolume;
sounddef1 music;
extern int Musicsize;


void AL_stop();
int update();
int stream(ALuint buffer);
void open1(char *ptr,int sizef,char loop);



void AL_Pause()       {if(music.def.size)alSourcePause(music.source);}
void AL_Continue()    {if(music.def.size)alSourcePlay(music.source);}
void AL_Update()      {if(music.def.size&&!update(0))AL_stop();}
int  AL_isntALmusic() {return !music.def.size;}

void AL_SetMusicVolume(int volume)
{
    AL_MusicVolume=volume;
    if(music.def.size)alSourcef(music.source,AL_GAIN,volume/(255.));
}

int isplaying()
{
    ALenum state;
    alGetSourcei(music.source,AL_SOURCE_STATE,&state);
    return state==AL_PLAYING;
}

int update()
{
    int processed=0;
    int active=1;
    ALuint buffer;

    ALdoing="update";
    alGetSourcei(music.source,AL_BUFFERS_PROCESSED,&processed);
    check(1);
    if(processed)
    switch(music.type)
    {
        case 1:
            while(processed--)
            {
                alSourceUnqueueBuffers(music.source,1,&buffer);
                check(1);
                active=stream(buffer);
                if(active)
                {
                    alSourceQueueBuffers(music.source,1,&buffer);
                    check(1);
                }
            }
            break;
    }
    return active;
}

void AL_stop()
{
    int queued=0;ALuint buffer;

    if(!music.def.size)return;

    alSourceStop(music.source);
    alGetSourcei(music.source,AL_BUFFERS_QUEUED,&queued);

    ALdoing="release";
    while(queued--)
    {
        alSourceUnqueueBuffers(music.source,1,&buffer);
        check(1);
    }
    alDeleteSources(1,&music.source);check(1);
    alDeleteBuffers(2, music.buffers);check(1);

    if(music.type==1)ov_clear(&music.def.oggStream);
    Bmemset(&music,0,sizeof(sounddef1));
}

int stream(ALuint buffer)
{
    char pcm[BUFFER_SIZE];
    ALsizei  size=0;
    int  section,result;

    while(size<BUFFER_SIZE)
    {
        result=ov_read(&music.def.oggStream,pcm+size,BUFFER_SIZE-size,0,2,1,&section);
        if(result==0&&music.loop)ov_pcm_seek(&music.def.oggStream,0);else
        if(result> 0)size+=result;else break;
    }
    if(!size)return 0;
    ALdoing="stream";
    alBufferData(buffer,music.format,pcm,size,music.rate);
    check(1);
    return 1;
}

void AL_PlaySong(char *ptr,int loop)
{
    vorbis_info* vorbisInfo;
    int bf=2,i;
    ALenum format;ALsizei size;ALsizei freq;ALvoid* data;

    if(!context)return;
    Bmemset(&music,0,sizeof(sounddef1));
    switch(*ptr)
    {
        case 'O':music.type=1;break;
        default: return;
    }
    music.def.size=Musicsize;
    music.loop=loop;
    music.def.ptrsnd=ptr;

    switch(music.type)
    {
        case 1:
            if((i=ov_open_callbacks(&music.def,&music.def.oggStream,0,0,cb))<0)
            {
                Bmemset(&music,0,sizeof(sounddef1));
                initprintf("Music error: %s\n",AL_ErrorString(i));
                return;
            }
            vorbisInfo=ov_info(&music.def.oggStream,-1);
            if(!vorbisInfo)
            {
                initprintf("Music error: vorbisInfo\n");
                Bmemset(&music,0,sizeof(sounddef1));
                return;
            }
           music.rate=vorbisInfo->rate;
           music.format=(vorbisInfo->channels==1)?AL_FORMAT_MONO16:AL_FORMAT_STEREO16;break;
    }

    ALdoing="Open";
    alGenBuffers(2, music.buffers);check(1);
    alGenSources(1,&music.source);check(1);
    alSourcei(music.source,AL_SOURCE_RELATIVE,AL_TRUE);

    switch(music.type)
    {
        case 1: stream(music.buffers[0]);
            if(!stream(music.buffers[1]))bf=1;
            alSourceQueueBuffers(music.source,bf,music.buffers);
            break;
    }

    AL_SetMusicVolume(AL_MusicVolume);
    AL_Continue();
}
