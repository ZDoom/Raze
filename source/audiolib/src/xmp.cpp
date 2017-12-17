
#include "compat.h"

#ifdef HAVE_XMP

#include "pitch.h"
#include "multivoc.h"
#include "_multivc.h"

#define BUILDING_STATIC
#include "libxmp-lite/xmp.h"

typedef struct {
    void * ptr;
    VoiceNode *owner;
    size_t length;
    xmp_context context;
    int time;
} xmp_data;

int32_t MV_GetXMPPosition(VoiceNode *voice)
{
    xmp_data * xmpd = (xmp_data *)voice->rawdataptr;
    return xmpd->time;
}

void MV_SetXMPPosition(VoiceNode *voice, int32_t position)
{
    xmp_data * xmpd = (xmp_data *)voice->rawdataptr;
    xmp_seek_time(xmpd->context, position);
}

static playbackstatus MV_GetNextXMPBlock(VoiceNode *voice)
{
    xmp_data * xmpd = (xmp_data *)voice->rawdataptr;
    struct xmp_frame_info mi;

    if (xmp_play_frame(xmpd->context) != 0)
    {
        if (voice->LoopSize > 0)
        {
            xmp_restart_module(xmpd->context);
            if (xmp_play_frame(xmpd->context) != 0)
            {
                voice->Playing = FALSE;
                return NoMoreData;
            }
        }
        else
        {
            voice->Playing = FALSE;
            return NoMoreData;
        }
    }

    xmp_get_frame_info(xmpd->context, &mi);

    xmpd->time = mi.time;

    voice->sound        = (char const *)mi.buffer;
    voice->length       = mi.buffer_size << 14; // since 2-channel, 16-bit is hardcoded
    // voice->length       = (mi.buffer_size << 16) / (voice->channels * (voice->bits >> 3));
    voice->position     = 0;
    voice->BlockLength  = 0;
    voice->Playing = TRUE;

    MV_SetVoiceMixMode(voice);

    return KeepPlaying;
}

int32_t MV_PlayXMP3D(char *ptr, uint32_t ptrlength, int32_t loophow, int32_t pitchoffset, int32_t angle,
                     int32_t distance, int32_t priority, uint32_t callbackval)
{
    int32_t left;
    int32_t right;
    int32_t mid;
    int32_t volume;
    int32_t status;

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return MV_Error;
    }

    if (distance < 0)
    {
        distance  = -distance;
        angle    += MV_NUMPANPOSITIONS / 2;
    }

    volume = MIX_VOLUME(distance);

    // Ensure angle is within 0 - 127
    angle &= MV_MAXPANPOSITION;

    left  = MV_PanTable[angle][volume].left;
    right = MV_PanTable[angle][volume].right;
    mid   = max( 0, 255 - distance );

    status = MV_PlayXMP(ptr, ptrlength, loophow, -1, pitchoffset, mid, left, right, priority, callbackval);

    return status;
}

int32_t MV_PlayXMP(char *ptr, uint32_t ptrlength, int32_t loopstart, int32_t loopend, int32_t pitchoffset, int32_t vol,
                   int32_t left, int32_t right, int32_t priority, uint32_t callbackval)
{
    VoiceNode   *voice;
    xmp_data * xmpd = 0;
    int retval;

    UNREFERENCED_PARAMETER(loopend);

    if (!MV_Installed)
    {
        MV_SetErrorCode(MV_NotInstalled);
        return MV_Error;
    }

    xmpd = (xmp_data *)calloc(1, sizeof(xmp_data));
    if (!xmpd)
    {
        MV_SetErrorCode(MV_InvalidFile);
        return MV_Error;
    }

    xmpd->ptr = ptr;
    xmpd->length = ptrlength;

    if ((xmpd->context = xmp_create_context()) == NULL)
    {
        free(xmpd);
        MV_SetErrorCode(MV_InvalidFile);
        return MV_Error;
    }

    if ((retval = xmp_load_module_from_memory(xmpd->context, ptr, ptrlength)) != 0)
    {
        free(xmpd);
        MV_Printf("MV_PlayXMP: xmp_load_module_from_memory failed (%i)\n", retval);
        MV_SetErrorCode(MV_InvalidFile);
        return MV_Error;
    }

    // Request a voice from the voice pool
    voice = MV_AllocVoice(priority);
    if (voice == NULL)
    {
        xmp_release_module(xmpd->context);
        xmp_free_context(xmpd->context);
        free(xmpd);
        MV_SetErrorCode(MV_NoVoices);
        return MV_Error;
    }

    xmpd->owner = voice;

    voice->length      = 0;
    voice->sound       = 0;

    voice->wavetype    = FMT_XMP;
    voice->rawdataptr  = (void*)xmpd;
    voice->GetSound    = MV_GetNextXMPBlock;
    voice->LoopCount   = 0;
    voice->BlockLength = 0;
    voice->PitchScale  = PITCH_GetScale(pitchoffset);
    voice->next        = NULL;
    voice->prev        = NULL;
    voice->priority    = priority;
    voice->callbackval = callbackval;

    voice->bits        = 16;
    voice->channels    = 2;
    voice->SamplingRate = MV_MixRate;

    voice->Playing     = TRUE;
    voice->Paused      = FALSE;

    voice->LoopStart   = 0;
    voice->LoopEnd     = 0;
    voice->LoopSize    = loopstart >= 0 ? 1 : 0;

    xmp_start_player(xmpd->context, MV_MixRate, 0);

    // CODEDUP multivoc.c MV_SetVoicePitch
    voice->RateScale = (voice->SamplingRate * voice->PitchScale) / MV_MixRate;
    voice->FixedPointBufferSize = (voice->RateScale * MV_MIXBUFFERSIZE) - voice->RateScale;
    MV_SetVoiceMixMode(voice);

    MV_SetVoiceVolume(voice, vol, left, right);
    MV_PlayVoice(voice);

    return voice->handle;
}

void MV_ReleaseXMPVoice(VoiceNode * voice)
{
    xmp_data * xmpd = (xmp_data *) voice->rawdataptr;

    if (voice->wavetype != FMT_XMP)
        return;

    voice->rawdataptr = 0;

    xmp_end_player(xmpd->context);
    xmp_release_module(xmpd->context);
    xmp_free_context(xmpd->context);
    free(xmpd);
}

#else

#include "_multivc.h"

static char const NoXMP[] = "MV_PlayXMP: libxmp-lite support not included in this binary.\n";

int32_t MV_PlayXMP(char *ptr, uint32_t ptrlength, int32_t loopstart, int32_t loopend, int32_t pitchoffset, int32_t vol,
                   int32_t left, int32_t right, int32_t priority, uint32_t callbackval)
{
    UNREFERENCED_PARAMETER(ptr);
    UNREFERENCED_PARAMETER(ptrlength);
    UNREFERENCED_PARAMETER(loopstart);
    UNREFERENCED_PARAMETER(loopend);
    UNREFERENCED_PARAMETER(pitchoffset);
    UNREFERENCED_PARAMETER(vol);
    UNREFERENCED_PARAMETER(left);
    UNREFERENCED_PARAMETER(right);
    UNREFERENCED_PARAMETER(priority);
    UNREFERENCED_PARAMETER(callbackval);

    MV_Printf(NoXMP);
    return -1;
}

int32_t MV_PlayXMP3D(char *ptr, uint32_t ptrlength, int32_t loophow, int32_t pitchoffset, int32_t angle,
                     int32_t distance, int32_t priority, uint32_t callbackval)
{
    UNREFERENCED_PARAMETER(ptr);
    UNREFERENCED_PARAMETER(ptrlength);
    UNREFERENCED_PARAMETER(loophow);
    UNREFERENCED_PARAMETER(pitchoffset);
    UNREFERENCED_PARAMETER(angle);
    UNREFERENCED_PARAMETER(distance);
    UNREFERENCED_PARAMETER(priority);
    UNREFERENCED_PARAMETER(callbackval);

    MV_Printf(NoXMP);
    return -1;
}

#endif

// KEEPINSYNC libxmp-lite/src/*_load.c

static int it_test_memory(char const *ptr, uint32_t ptrlength)
{
    static char const it_magic[] = "IMPM";

    if (ptrlength < sizeof(it_magic)-1 ||
        memcmp(ptr, it_magic, sizeof(it_magic)-1))
        return -1;

    return 0;
}

static int mod_test_memory(char const *ptr, uint32_t ptrlength)
{
    if (ptrlength < 1084)
        return -1;

    char const * const buf = ptr + 1080;

    if (!strncmp(buf + 2, "CH", 2) && isdigit((int)buf[0]) && isdigit((int)buf[1]))
    {
        int i = (buf[0] - '0') * 10 + buf[1] - '0';
        if (i > 0 && i <= 32)
            return 0;
    }

    if (!strncmp(buf + 1, "CHN", 3) && isdigit((int)*buf))
    {
        if (*buf >= '0' && *buf <= '9')
            return 0;
    }

    if (!memcmp(buf, "M.K.", 4))
        return 0;

    return -1;
}

static int s3m_test_memory(char const *ptr, uint32_t ptrlength)
{
    static char const s3m_magic[] = "SCRM";
    #define s3m_magic_offset 44

    if (ptrlength < s3m_magic_offset + sizeof(s3m_magic)-1 ||
        memcmp(ptr + s3m_magic_offset, s3m_magic, sizeof(s3m_magic)-1) ||
        ptr[29] != 0x10)
        return -1;

    return 0;
}

static int xm_test_memory(char const *ptr, uint32_t ptrlength)
{
    static char const xm_magic[] = "Extended Module: ";

    if (ptrlength < sizeof(xm_magic)-1 ||
        memcmp(ptr, xm_magic, sizeof(xm_magic)-1))
        return -1;

    return 0;
}

static int mtm_test_memory(char const *ptr, uint32_t ptrlength)
{
    static char const mtm_magic[] = "MTM\x10";

    if (ptrlength < sizeof(mtm_magic)-1 ||
        memcmp(ptr, mtm_magic, sizeof(mtm_magic)-1))
        return -1;

    return 0;
}

int MV_IdentifyXMP(char const *ptr, uint32_t ptrlength)
{
    static decltype(mod_test_memory) * const module_test_functions[] =
    {
        it_test_memory,
        mod_test_memory,
        s3m_test_memory,
        xm_test_memory,
        mtm_test_memory,
    };

    for (auto const test_module : module_test_functions)
    {
        if (test_module(ptr, ptrlength) == 0)
            return 1;
    }

    return 0;
}
