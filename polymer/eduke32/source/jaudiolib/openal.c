#ifdef _WIN32
#include "dsound.h"
#endif

#include "baselayer.h"
#include "compat.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <vorbis/vorbisfile.h>
#include "_multivc.h"

ALCdevice  * device=NULL;
ALCcontext * context=NULL;

char *ALdoing = NULL;
int AL_Error;
int bufsize;
int openal_disabled = 0;

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

sounddef1 music;

#ifdef _WIN32
// Windows
static HANDLE hALDLL = NULL;
#else
#include <dlfcn.h>
static void *alhandle = NULL;
#endif

static char *aldriver = NULL;

static void (AL_APIENTRY * balGetSourcei)(ALuint sid,  ALenum param, ALint* value);
static void (AL_APIENTRY * balSourcef)(ALuint sid, ALenum param, ALfloat value);
static void (AL_APIENTRY * balSourcePlay)(ALuint sid);
static void (AL_APIENTRY * balSourcePause)(ALuint sid);
static ALCenum(ALC_APIENTRY * balcGetError)(ALCdevice *device);
static ALenum(AL_APIENTRY * balGetError)(void);
static void (AL_APIENTRY * balBufferData)(ALuint bid, ALenum format, const ALvoid* data, ALsizei size, ALsizei freq);
static void (AL_APIENTRY * balGenBuffers)(ALsizei n, ALuint* buffers);
static void (AL_APIENTRY * balGenSources)(ALsizei n, ALuint* sources);
static void (AL_APIENTRY * balSourcei)(ALuint sid, ALenum param, ALint value);
static void (AL_APIENTRY * balSourceQueueBuffers)(ALuint sid, ALsizei numEntries, const ALuint *bids);
static void (AL_APIENTRY * balSourceStop)(ALuint sid);
static void (AL_APIENTRY * balSourceUnqueueBuffers)(ALuint sid, ALsizei numEntries, ALuint *bids);
static void (AL_APIENTRY * bbalDeleteSources)(ALsizei n, const ALuint* sources);
static ALCboolean(ALC_APIENTRY * balcMakeContextCurrent)(ALCcontext *context);
static void (AL_APIENTRY * balDeleteSources)(ALsizei n, const ALuint* sources);
static void (AL_APIENTRY * balDeleteBuffers)(ALsizei n, const ALuint* buffers);
static void (ALC_APIENTRY * balcDestroyContext)(ALCcontext *context);
static ALCboolean(ALC_APIENTRY * balcCloseDevice)(ALCdevice *device);
static ALCdevice *(ALC_APIENTRY * balcOpenDevice)(const ALCchar *devicename);
static ALCcontext *(ALC_APIENTRY * balcCreateContext)(ALCdevice *device, const ALCint* attrlist);
static const ALchar*(AL_APIENTRY * balGetString)(ALenum param);

static void * algetproc_(const char *s, int *err, int fatal)
{
    void *t;
#if defined _WIN32
    t = (void*)GetProcAddress(hALDLL,s);
#else
    t = (void*)dlsym(alhandle,s);
#endif
    if (!t && fatal)
    {
        initprintf("Failed to find %s in %s\n", s, aldriver);
        *err = 1;
    }
    return t;
}
#define ALGETPROC(s)        algetproc_(s,&err,1)
#define ALGETPROCSOFT(s)    algetproc_(s,&err,0)

static int unloadaldriver(void)
{
#ifdef _WIN32
    if (!hALDLL) return 0;
#endif

    if (aldriver)
        free(aldriver);
    aldriver = NULL;

#ifdef _WIN32
    if (hALDLL)
        FreeLibrary(hALDLL);
    hALDLL = NULL;
#else
    if (alhandle)
        dlclose(alhandle);
    alhandle = NULL;
#endif

    balGetSourcei    = NULL;
    balSourcef    = NULL;
    balSourcePlay    = NULL;
    balSourcePause    = NULL;
    balcGetError    = NULL;
    balGetError    = NULL;
    balBufferData    = NULL;
    balGenBuffers    = NULL;
    balGenSources    = NULL;
    balSourcei    = NULL;
    balSourceQueueBuffers    = NULL;
    balSourceStop    = NULL;
    balSourceUnqueueBuffers    = NULL;
    bbalDeleteSources    = NULL;
    balcMakeContextCurrent    = NULL;
    balDeleteSources    = NULL;
    balDeleteBuffers    = NULL;
    balcDestroyContext    = NULL;
    balcCloseDevice    = NULL;
    balcOpenDevice    = NULL;
    balcCreateContext    = NULL;
    balGetString    = NULL;

    return 0;
}

static int loadaldriver(void)
{
    void *t;
    int err=0;
    char *driver;

#ifdef _WIN32
    if (hALDLL)
        return 0;
#endif

//    if (!driver)
    {
#ifdef _WIN32
        driver = "OpenAL32.DLL";
#elif defined __APPLE__
        driver = "/System/Library/Frameworks/OpenGL.framework/OpenGL"; // FIXME: like I know anything about Apple.  Hah.
#else
        driver = "libopenal.so";
#endif
    }

    initprintf("Loading %s\n",driver);

#ifdef _WIN32
    hALDLL = LoadLibrary(driver);
    if (!hALDLL) return -1;
#else
    alhandle = dlopen(driver, RTLD_NOW|RTLD_GLOBAL);
    if (!alhandle) return -1;
#endif
    aldriver = strdup(driver);

    balGetSourcei = ALGETPROC("alGetSourcei");
    balSourcef    = ALGETPROC("alSourcef");
    balSourcePlay    = ALGETPROC("alSourcePlay");
    balSourcePause    = ALGETPROC("alSourcePause");
    balcGetError    = ALGETPROC("alcGetError");
    balGetError    = ALGETPROC("alGetError");
    balBufferData    = ALGETPROC("alBufferData");
    balGenBuffers    = ALGETPROC("alGenBuffers");
    balGenSources    = ALGETPROC("alGenSources");
    balSourcei    = ALGETPROC("alSourcei");
    balSourceQueueBuffers    = ALGETPROC("alSourceQueueBuffers");
    balSourceStop    = ALGETPROC("alSourceStop");
    balSourceUnqueueBuffers    = ALGETPROC("alSourceUnqueueBuffers");
    balDeleteSources    = ALGETPROC("alDeleteSources");
    balcMakeContextCurrent    = ALGETPROC("alcMakeContextCurrent");
    balDeleteSources    = ALGETPROC("alDeleteSources");
    balDeleteBuffers    = ALGETPROC("alDeleteBuffers");
    balcDestroyContext    = ALGETPROC("alcDestroyContext");
    balcCloseDevice    = ALGETPROC("alcCloseDevice");
    balcOpenDevice    = ALGETPROC("alcOpenDevice");
    balcCreateContext    = ALGETPROC("alcCreateContext");
    balGetString    = ALGETPROC("alGetString");

    if (err) unloadaldriver();
    return err;
}

char *ALC_ErrorString(int code)
{
    switch (code)
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
    switch (code)
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
    AL_Error=balcGetError(device);
    if (AL_Error!=ALC_NO_ERROR&&show)initprintf("%s(%s)\n",ALC_ErrorString(AL_Error),ALdoing);
    AL_Error=balGetError();
    if (AL_Error!= AL_NO_ERROR&&show)initprintf("%s(%s)\n", AL_ErrorString(AL_Error),ALdoing);
}
extern ov_callbacks cb;


int AL_Init()
{
    Bmemset(&music,0,sizeof(music)); // "music.def.size=0" means music not playing
    if (loadaldriver())
    {
        initprintf("Failed loading OpenAL driver.\nDownload OpenAL 1.1 or greater from http://www.openal.org/downloads.html.");
        openal_disabled = 1;

        return 1;
    }

    ALdoing="Init";
//    device=alcOpenDevice(ud.config.HardwareAL?"Generic Hardware":"Generic Software");
    device=balcOpenDevice(NULL);
    check(1);

    if (device)
    {
        context=balcCreateContext(device,NULL);
        check(1);
    }

    if (context)
    {
//        char *s,*t,*u,i;

        balcMakeContextCurrent(context);check(1);
        initprintf("OpenAL Information:\n"
                   " Version:  %s\n"
                   " Vendor:   %s\n"
                   " Renderer: %s\n"
//                   " Extensions:\n"
                   ,balGetString(AL_VERSION),balGetString(AL_VENDOR),balGetString(AL_RENDERER));

        /*        s = Bstrdup(balGetString(AL_EXTENSIONS));
                if (!s) initprintf(balGetString(AL_EXTENSIONS));
                else
                {
                    i = 0; t = u = s;
                    while (*t)
                    {
                        if (*t == ' ')
                        {
                            if (i&1)
                            {
                                *t = 0;
                                initprintf("   %s\n",u);
                                u = t+1;
                            }
                            i++;
                        }
                        t++;
                    }
                    if (i&1) initprintf("   %s\n",u);
                    Bfree(s);
                }
        */
    }
    else initprintf("OpenAL initialization failed.\n");

    return 0;
}

void AL_Shutdown()
{
    if (openal_disabled)return;

    ALdoing="Shut";
    balcMakeContextCurrent(NULL);
    check(1);
    balcDestroyContext(context);
    check(1);
    balcCloseDevice(device);
    unloadaldriver();
}

#define BUFFER_SIZE 65536 // (4096 * 4*8*8)
int AL_MusicVolume;
extern int Musicsize;


void AL_Stop();
int update();
int stream(ALuint buffer);
void open1(char *ptr,int sizef,char loop);



void AL_Pause()       {if (music.def.size)balSourcePause(music.source);}
void AL_Continue()    {if (music.def.size)balSourcePlay(music.source);}
void AL_Update()      {if (music.def.size&&!update(0))AL_Stop();}
int  AL_isntALmusic() {return !music.def.size;}

void AL_SetMusicVolume(int volume)
{
    AL_MusicVolume=volume;
    if (music.def.size)balSourcef(music.source,AL_GAIN,volume/(255.));
}

int isplaying()
{
    ALenum state;
    balGetSourcei(music.source,AL_SOURCE_STATE,&state);
    return state==AL_PLAYING;
}

int update()
{
    int processed=0;
    int active=1;
    ALuint buffer;

    ALdoing="update";
    balGetSourcei(music.source,AL_BUFFERS_PROCESSED,&processed);
    check(1);
    if (processed)
        switch (music.type)
        {
        case 1:
            while (processed--)
            {
                balSourceUnqueueBuffers(music.source,1,&buffer);
                check(1);
                active=stream(buffer);
                if (active)
                {
                    balSourceQueueBuffers(music.source,1,&buffer);
                    check(1);
                }
            }
            break;
        }
    return active;
}

void AL_Stop()
{
    int queued=0;ALuint buffer;

    if (!music.def.size)return;

    balSourceStop(music.source);
    balGetSourcei(music.source,AL_BUFFERS_QUEUED,&queued);

    ALdoing="release";
    while (queued--)
    {
        balSourceUnqueueBuffers(music.source,1,&buffer);
        check(1);
    }
    balDeleteSources(1,&music.source);check(1);
    balDeleteBuffers(2, music.buffers);check(1);

    if (music.type==1)ov_clear(&music.def.oggStream);
    Bmemset(&music,0,sizeof(sounddef1));
}

int stream(ALuint buffer)
{
    ALsizei  size=0;
    int  section,result;
    static char pcm[BUFFER_SIZE];

    while (size<BUFFER_SIZE)
    {
        result=ov_read(&music.def.oggStream,pcm+size,BUFFER_SIZE-size,0,2,1,&section);
        if (result==0&&music.loop)ov_pcm_seek(&music.def.oggStream,0);else
        if (result> 0)size+=result;else break;
    }
    if (!size)
        return 0;
    ALdoing="stream";
    balBufferData(buffer,music.format,pcm,size,music.rate);
    check(1);
    return 1;
}

void AL_PlaySong(char *ptr,int loop)
{
    vorbis_info* vorbisInfo;
    int bf=2,i;
    ALenum format;
    ALsizei size;
    ALsizei freq;
    ALvoid* data;

    if (!context)
        return;
    Bmemset(&music,0,sizeof(sounddef1));
    switch (*ptr)
    {
    case 'O':
        music.type=1;
        break;
    default:
        return;
    }
    music.def.size=Musicsize;
    music.loop=loop;
    music.def.ptrsnd=ptr;

    switch (music.type)
    {
    case 1:
        if ((i=ov_open_callbacks(&music.def,&music.def.oggStream,0,0,cb))<0)
        {
            Bmemset(&music,0,sizeof(sounddef1));
            initprintf("Music error: %s\n",AL_ErrorString(i));
            return;
        }
        vorbisInfo=ov_info(&music.def.oggStream,-1);
        if (!vorbisInfo)
        {
            initprintf("Music error: vorbisInfo\n");
            Bmemset(&music,0,sizeof(sounddef1));
            return;
        }
        music.rate=vorbisInfo->rate;
        music.format=(vorbisInfo->channels==1)?AL_FORMAT_MONO16:AL_FORMAT_STEREO16;
        break;
    }

    ALdoing="Open";
    balGenBuffers(2, music.buffers);check(1);
    balGenSources(1,&music.source);check(1);
    balSourcei(music.source,AL_SOURCE_RELATIVE,AL_TRUE);

    switch (music.type)
    {
    case 1: 
        stream(music.buffers[0]);
        if (!stream(music.buffers[1]))
            bf=1;
        balSourceQueueBuffers(music.source,bf,music.buffers);
        break;
    }

    AL_SetMusicVolume(AL_MusicVolume);
    AL_Continue();
}
