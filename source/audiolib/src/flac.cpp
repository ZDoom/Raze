/*
 Copyright (C) 2009 Jonathon Fowler <jf@jonof.id.au>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

 */

/**
 * FLAC source support for MultiVoc
 */

#include "compat.h"

#ifdef HAVE_FLAC

#define FLAC__NO_DLL

#ifdef __APPLE__
#include <FLAC/all.h>
#else
#include "FLAC/all.h"
#endif

#include "_multivc.h"
#include "multivoc.h"
#include "pitch.h"
#include "pragmas.h"

typedef struct
{
    void *ptr;
    size_t length;
    size_t pos;

    FLAC__StreamDecoder *stream;
    FLAC__uint64 sample_pos;

    char *block;
    size_t blocksize;

    VoiceNode *owner;
} flac_data;

// callbacks, round 1

static size_t read_flac(void *ptr, size_t size, size_t nmemb, FLAC__IOHandle datasource)
{
    flac_data *flac = (flac_data *)datasource;
    size_t nread = 0;
    size_t bytes;

    errno = 0;

    if (flac->length == flac->pos)
    {
        return 0;
    }

    for (; nmemb > 0; nmemb--, nread++)
    {
        bytes = flac->length - flac->pos;
        if (size < bytes)
        {
            bytes = size;
        }

        memcpy(ptr, (uint8_t *)flac->ptr + flac->pos, bytes);
        flac->pos += bytes;
        ptr = (uint8_t *)ptr + bytes;

        if (flac->length == flac->pos)
        {
            nread++;
            break;
        }
    }

    return nread;
}

static size_t write_flac(const void *ptr, size_t size, size_t nmemb, FLAC__IOHandle datasource)
{
    UNREFERENCED_PARAMETER(ptr);
    UNREFERENCED_PARAMETER(size);
    UNREFERENCED_PARAMETER(nmemb);
    UNREFERENCED_PARAMETER(datasource);

    return 0;
}

static int seek_flac(FLAC__IOHandle datasource, FLAC__int64 offset, int whence)
{
    flac_data *flac = (flac_data *)datasource;

    switch (whence)
    {
        case SEEK_SET: flac->pos = 0; break;
        case SEEK_CUR: break;
        case SEEK_END: flac->pos = flac->length; break;
    }

    flac->pos += offset;

    if (flac->pos > flac->length)
    {
        flac->pos = flac->length;
    }

    return 0;
}

static FLAC__int64 tell_flac(FLAC__IOHandle datasource)
{
    flac_data *flac = (flac_data *)datasource;

    return flac->pos;
}

static FLAC__int64 length_flac(FLAC__IOHandle datasource)
{
    flac_data *flac = (flac_data *)datasource;

    return flac->length;
}

static int eof_flac(FLAC__IOHandle datasource)
{
    flac_data *flac = (flac_data *)datasource;

    return (flac->pos == flac->length);
}

static int close_flac(FLAC__IOHandle datasource)
{
    UNREFERENCED_PARAMETER(datasource);
    return 0;
}

static FLAC__IOCallbacks flac_callbacks = {
    read_flac, write_flac, seek_flac, tell_flac, eof_flac, close_flac,
};


// callbacks, round 2

FLAC__StreamDecoderReadStatus read_flac_stream(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes,
                                               void *client_data)
{
    UNREFERENCED_PARAMETER(decoder);
    if (*bytes > 0)
    {
        *bytes = read_flac(buffer, sizeof(FLAC__byte), *bytes, client_data);
        if (errno)
            return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
        else if (*bytes == 0)
            return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
        else
            return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
    }
    else
        return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
}

FLAC__StreamDecoderSeekStatus seek_flac_stream(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset,
                                               void *client_data)
{
    UNREFERENCED_PARAMETER(decoder);
    if (seek_flac(client_data, absolute_byte_offset, SEEK_SET) < 0)
        return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
    else
        return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__StreamDecoderTellStatus tell_flac_stream(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset,
                                               void *client_data)
{
    UNREFERENCED_PARAMETER(decoder);
    *absolute_byte_offset = tell_flac(client_data);
    return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus length_flac_stream(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length,
                                                   void *client_data)
{
    UNREFERENCED_PARAMETER(decoder);
    *stream_length = length_flac(client_data);
    return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

FLAC__bool eof_flac_stream(const FLAC__StreamDecoder *decoder, void *client_data)
{
    UNREFERENCED_PARAMETER(decoder);
    return eof_flac(client_data);
}

FLAC__StreamDecoderWriteStatus write_flac_stream(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame,
                                                 const FLAC__int32 *const ibuffer[], void *client_data)
{
    flac_data *fd = (flac_data *)client_data;
    VoiceNode *voice = fd->owner;
    FLAC__uint64 samples = frame->header.blocksize;

    UNREFERENCED_PARAMETER(decoder);

    voice->channels = frame->header.channels;
    voice->bits = frame->header.bits_per_sample;
    voice->SamplingRate = frame->header.sample_rate;

    if (frame->header.number_type == FLAC__FRAME_NUMBER_TYPE_FRAME_NUMBER)
        fd->sample_pos = frame->header.number.frame_number;
    else if (frame->header.number_type == FLAC__FRAME_NUMBER_TYPE_SAMPLE_NUMBER)
        fd->sample_pos = frame->header.number.sample_number;

    if ((FLAC__uint64)(uintptr_t)voice->LoopEnd > 0 &&
        fd->sample_pos + samples >= (FLAC__uint64)(uintptr_t)voice->LoopEnd)
    {
        samples = (FLAC__uint64)(uintptr_t)voice->LoopEnd - fd->sample_pos;
        if (!FLAC__stream_decoder_seek_absolute(fd->stream, (FLAC__uint64)(uintptr_t)voice->LoopStart))
            MV_Printf("MV_GetNextFLACBlock FLAC__stream_decoder_seek_absolute: LOOP_START %ul, LOOP_END %ul\n",
                      (FLAC__uint64)(uintptr_t)voice->LoopStart, (FLAC__uint64)(uintptr_t)voice->LoopEnd);
    }

    size_t const size = samples * voice->channels * (voice->bits >> 3);

    voice->position = 0;
    voice->BlockLength = 0;
    // CODEDUP multivoc.c MV_SetVoicePitch
    voice->RateScale = divideu32(voice->SamplingRate * voice->PitchScale, MV_MixRate);
    voice->FixedPointBufferSize = (voice->RateScale * MV_MIXBUFFERSIZE) - voice->RateScale;
    MV_SetVoiceMixMode(voice);

    char * block = fd->block;

    if (size > fd->blocksize)
    {
        block = (char *)Xmalloc(size);

        if (block == nullptr)
            return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

    {
        char *obuffer = block;
        FLAC__uint64 sample;
        uint8_t channel;

        // this loop is adapted from code in ov_read_filter() in vorbisfile.c in libvorbis
        for (sample = 0; sample < samples; ++sample)
            for (channel = 0; channel < frame->header.channels; ++channel)
            {
                int8_t byte;
                FLAC__int32 val = ibuffer[channel][sample];
                if (val > (1 << (voice->bits - 1)) - 1)
                    val = (1 << (voice->bits - 1)) - 1;
                else if (val < -(1 << (voice->bits - 1)))
                    val = -(1 << (voice->bits - 1));
                for (byte = 0; byte < voice->bits; byte += 8) *obuffer++ = ((val >> byte) & 0x000000FF);
            }
    }

    voice->sound = block;
    voice->length = samples << 16;

    if (block != fd->block)
    {
        char * oldblock = fd->block;
        fd->block = block;
        fd->blocksize = size;
        Xfree(oldblock);
    }

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void error_flac_stream(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
    // flac_data * fd = (flac_data *) client_data;
    UNREFERENCED_PARAMETER(client_data);
    UNREFERENCED_PARAMETER(decoder);
    MV_Printf("%s\n", FLAC__StreamDecoderErrorStatusString[status]);
    // FLAC__stream_decoder_flush(fd->stream);
}

int MV_GetFLACPosition(VoiceNode *voice)
{
    FLAC__uint64 position = 0;
    flac_data *fd = (flac_data *)voice->rawdataptr;

    FLAC__stream_decoder_get_decode_position(fd->stream, &position);

    return position;
}

void MV_SetFLACPosition(VoiceNode *voice, int position)
{
    flac_data *fd = (flac_data *)voice->rawdataptr;

    FLAC__stream_decoder_seek_absolute(fd->stream, position);
}

/*---------------------------------------------------------------------
Function: MV_GetNextFLACBlock

Controls playback of FLAC data
---------------------------------------------------------------------*/

static playbackstatus MV_GetNextFLACBlock(VoiceNode *voice)

{
    flac_data *fd = (flac_data *)voice->rawdataptr;
    FLAC__StreamDecoderState decode_state;
    // FLAC__bool decode_status;

    if ((FLAC__uint64)(uintptr_t)voice->LoopEnd > 0 && fd->sample_pos >= (FLAC__uint64)(uintptr_t)voice->LoopEnd)
        if (!FLAC__stream_decoder_seek_absolute(fd->stream, (FLAC__uint64)(uintptr_t)voice->LoopStart))
            MV_Printf("MV_GetNextFLACBlock FLAC__stream_decoder_seek_absolute: LOOP_START %ul, LOOP_END %ul\n",
                      (FLAC__uint64)(uintptr_t)voice->LoopStart, (FLAC__uint64)(uintptr_t)voice->LoopEnd);

    /*decode_status =*/FLAC__stream_decoder_process_single(fd->stream);
    decode_state = FLAC__stream_decoder_get_state(fd->stream);

    /*
        if (!decode_status)
        {
            MV_Printf("MV_GetNextFLACBlock: %s\n", FLAC__StreamDecoderStateString[decode_state]);
            return NoMoreData;
        }
    */

    if (decode_state == FLAC__STREAM_DECODER_SEEK_ERROR)
    {
        FLAC__stream_decoder_flush(fd->stream);
        decode_state = FLAC__stream_decoder_get_state(fd->stream);
    }

    if (decode_state == FLAC__STREAM_DECODER_END_OF_STREAM)
    {
        if (voice->LoopSize > 0)
        {
            if (!FLAC__stream_decoder_seek_absolute(fd->stream, (FLAC__uint64)(uintptr_t)voice->LoopStart))
                MV_Printf("MV_GetNextFLACBlock FLAC__stream_decoder_seek_absolute: LOOP_START %ul\n",
                          (FLAC__uint64)(uintptr_t)voice->LoopStart);
        }
        else
            return NoMoreData;
    }

#if 0
    // unnecessary: duplicated in write_flac_stream()
    voice->channels     = FLAC__stream_decoder_get_channels(fd->stream);
    voice->bits         = FLAC__stream_decoder_get_bits_per_sample(fd->stream);
    voice->SamplingRate = FLAC__stream_decoder_get_sample_rate(fd->stream);
    // CODEDUP multivoc.c MV_SetVoicePitch
    voice->RateScale    = ( voice->SamplingRate * voice->PitchScale ) / MV_MixRate;
    voice->FixedPointBufferSize = ( voice->RateScale * MV_MIXBUFFERSIZE ) - voice->RateScale;
    MV_SetVoiceMixMode( voice );
#endif

    return KeepPlaying;
}


/*---------------------------------------------------------------------
Function: MV_PlayFLAC3D

Begin playback of sound data at specified angle and distance
from listener.
---------------------------------------------------------------------*/

int MV_PlayFLAC3D(char *ptr, uint32_t length, int loophow, int pitchoffset, int angle, int distance, int priority, float volume, uint32_t callbackval)
{
    int left;
    int right;
    int mid;
    int vol;
    int status;

    if (!MV_Installed)
        return MV_SetErrorCode(MV_NotInstalled);

    if (distance < 0)
    {
        distance = -distance;
        angle += MV_NUMPANPOSITIONS / 2;
    }

    vol = MIX_VOLUME(distance);

    // Ensure angle is within 0 - 127
    angle &= MV_MAXPANPOSITION;

    left = MV_PanTable[angle][vol].left;
    right = MV_PanTable[angle][vol].right;
    mid = max(0, 255 - distance);

    status = MV_PlayFLAC(ptr, length, loophow, -1, pitchoffset, mid, left, right, priority, volume, callbackval);

    return status;
}


/*---------------------------------------------------------------------
Function: MV_PlayFLAC

Begin playback of sound data with the given sound levels and
priority.
---------------------------------------------------------------------*/

int MV_PlayFLAC(char *ptr, uint32_t length, int loopstart, int loopend, int pitchoffset, int vol, int left, int right, int priority, float volume, uint32_t callbackval)
{
    VoiceNode *voice;
    flac_data *fd = 0;
    FLAC__Metadata_Chain *metadata_chain;

    UNREFERENCED_PARAMETER(loopend);

    if (!MV_Installed)
        return MV_SetErrorCode(MV_NotInstalled);

    fd = (flac_data *)Xcalloc(1, sizeof(flac_data));
    if (!fd)
        return MV_SetErrorCode(MV_InvalidFile);

    fd->ptr = ptr;
    fd->pos = 0;
    fd->blocksize = 0;
    fd->length = length;

    fd->block = nullptr;

    fd->stream = FLAC__stream_decoder_new();
    fd->sample_pos = 0;

    FLAC__stream_decoder_set_metadata_ignore_all(fd->stream);

    if (FLAC__stream_decoder_init_stream(fd->stream, read_flac_stream, seek_flac_stream, tell_flac_stream,
                                         length_flac_stream, eof_flac_stream, write_flac_stream,
                                         /*metadata_flac_stream*/ nullptr, error_flac_stream,
                                         (void *)fd) != FLAC__STREAM_DECODER_INIT_STATUS_OK)
    {
        Xfree(fd);
        MV_Printf("MV_PlayFLAC: %s\n", FLAC__stream_decoder_get_resolved_state_string(fd->stream));
        return MV_SetErrorCode(MV_InvalidFile);
    }

    // Request a voice from the voice pool
    voice = MV_AllocVoice(priority);
    if (voice == nullptr)
    {
        FLAC__stream_decoder_finish(fd->stream);
        FLAC__stream_decoder_delete(fd->stream);
        Xfree(fd);
        return MV_SetErrorCode(MV_NoVoices);
    }

    fd->owner = voice;

    voice->wavetype = FMT_FLAC;
    voice->rawdataptr = (void *)fd;
    voice->GetSound = MV_GetNextFLACBlock;
    voice->NextBlock = fd->block;
    voice->LoopCount = 0;
    voice->BlockLength = 0;
    voice->PitchScale = PITCH_GetScale(pitchoffset);
    voice->next = nullptr;
    voice->prev = nullptr;
    voice->priority = priority;
    voice->callbackval = callbackval;

    voice->Paused = FALSE;

    voice->LoopStart = 0;
    voice->LoopEnd = 0;
    voice->LoopSize = (loopstart >= 0 ? 1 : 0);

    // parse metadata
    // loop parsing designed with multiple repetitions in mind
    // In retrospect, it may be possible to MV_GetVorbisCommentLoops(voice, (vorbis_comment *)
    // &tags->data.vorbis_comment)
    // but libvorbisfile may be confused by the signedness of char* vs FLAC__byte* and this code does not depend on
    // HAVE_VORBIS.
    metadata_chain = FLAC__metadata_chain_new();
    if (metadata_chain != nullptr)
    {
        if (FLAC__metadata_chain_read_with_callbacks(metadata_chain, fd, flac_callbacks))
        {
            FLAC__Metadata_Iterator *metadata_iterator = FLAC__metadata_iterator_new();
            if (metadata_iterator != nullptr)
            {
                char *vc_loopstart = nullptr;
                char *vc_loopend = nullptr;
                char *vc_looplength = nullptr;

                FLAC__metadata_iterator_init(metadata_iterator, metadata_chain);

                do
                {
                    FLAC__StreamMetadata *tags = FLAC__metadata_iterator_get_block(metadata_iterator);

                    if (tags->type == FLAC__METADATA_TYPE_STREAMINFO)
                    {
                        const FLAC__StreamMetadata_StreamInfo *info = &tags->data.stream_info;

                        if (info->channels != 1 && info->channels != 2)
                        {
                            FLAC__metadata_object_delete(tags);
                            FLAC__metadata_iterator_delete(metadata_iterator);
                            // FLAC__metadata_chain_delete(metadata_chain);
                            FLAC__stream_decoder_finish(fd->stream);
                            FLAC__stream_decoder_delete(fd->stream);
                            Xfree(fd);
                            return MV_SetErrorCode(MV_InvalidFile);
                        }

                        voice->channels = info->channels;
                        voice->bits = info->bits_per_sample;
                        voice->SamplingRate = info->sample_rate;
                    }

                    // load loop tags from metadata
                    if (tags->type == FLAC__METADATA_TYPE_VORBIS_COMMENT)
                    {
                        FLAC__uint32 comment;
                        for (comment = 0; comment < tags->data.vorbis_comment.num_comments; ++comment)
                        {
                            const char *entry = (const char *)tags->data.vorbis_comment.comments[comment].entry;
                            if (entry != nullptr && entry[0] != '\0')
                            {
                                const char *value = strchr(entry, '=');
                                const size_t field = value - entry;
                                value += 1;

                                for (int t = 0; t < loopStartTagCount && vc_loopstart == nullptr; ++t)
                                {
                                    char const * const tag = loopStartTags[t];
                                    if (field == strlen(tag) && Bstrncasecmp(entry, tag, field) == 0)
                                        vc_loopstart = Xstrdup(value);
                                }

                                for (int t = 0; t < loopEndTagCount && vc_loopend == nullptr; ++t)
                                {
                                    char const * const tag = loopEndTags[t];
                                    if (field == strlen(tag) && Bstrncasecmp(entry, tag, field) == 0)
                                        vc_loopend = Xstrdup(value);
                                }

                                for (int t = 0; t < loopLengthTagCount && vc_looplength == nullptr; ++t)
                                {
                                    char const * const tag = loopLengthTags[t];
                                    if (field == strlen(tag) && Bstrncasecmp(entry, tag, field) == 0)
                                        vc_looplength = Xstrdup(value);
                                }
                            }
                        }
                    }

                    FLAC__metadata_object_delete(
                    tags);  // If it were not for this, I would assign pointers instead of strdup().
                } while (FLAC__metadata_iterator_next(metadata_iterator));

                if (vc_loopstart != nullptr)
                {
                    {
                        const FLAC__int64 flac_loopstart = atol(vc_loopstart);
                        if (flac_loopstart >= 0)  // a loop starting at 0 is valid
                        {
                            voice->LoopStart = (const char *)(intptr_t)flac_loopstart;
                            voice->LoopSize = 1;
                        }
                    }
                    free(vc_loopstart);
                }
                if (vc_loopend != nullptr)
                {
                    if (voice->LoopSize > 0)
                    {
                        const FLAC__int64 flac_loopend = atol(vc_loopend);
                        if (flac_loopend > 0)  // a loop ending at 0 is invalid
                            voice->LoopEnd = (const char *)(intptr_t)flac_loopend;
                    }
                    free(vc_loopend);
                }
                if (vc_looplength != nullptr)
                {
                    if (voice->LoopSize > 0 && voice->LoopEnd == 0)
                    {
                        const FLAC__int64 flac_looplength = atol(vc_looplength);
                        if (flac_looplength > 0)  // a loop of length 0 is invalid
                            voice->LoopEnd = (const char *)((intptr_t)flac_looplength + (intptr_t)voice->LoopStart);
                    }
                    free(vc_looplength);
                }

                FLAC__metadata_iterator_delete(metadata_iterator);
            }
            else
                MV_Printf("Error allocating FLAC__Metadata_Iterator!\n");
        }
        else
            MV_Printf("%s\n", FLAC__Metadata_ChainStatusString[FLAC__metadata_chain_status(metadata_chain)]);

        // FLAC__metadata_chain_delete(metadata_chain); // when run with GDB, this throws SIGTRAP about freed heap
        // memory being modified
    }
    else
        MV_Printf("Error allocating FLAC__Metadata_Chain!\n");

    // CODEDUP multivoc.c MV_SetVoicePitch
    voice->RateScale = divideu32(voice->SamplingRate * voice->PitchScale, MV_MixRate);
    voice->FixedPointBufferSize = (voice->RateScale * MV_MIXBUFFERSIZE) - voice->RateScale;
    MV_SetVoiceMixMode(voice);

    MV_SetVoiceVolume(voice, vol, left, right, volume);
    MV_PlayVoice(voice);

    return voice->handle;
}


void MV_ReleaseFLACVoice(VoiceNode *voice)
{
    flac_data *fd = (flac_data *)voice->rawdataptr;

    if (voice->wavetype != FMT_FLAC)
    {
        return;
    }

    voice->rawdataptr = 0;
    if (fd->stream != nullptr)
    {
    auto stream = fd->stream;
    fd->stream = nullptr;
        FLAC__stream_decoder_finish(stream);
        FLAC__stream_decoder_delete(stream);
    }
    auto block = fd->block;
    voice->length = 0;
    voice->sound = nullptr;
    fd->block = nullptr;
    Xfree(block);
    Xfree(fd);
}
#else
#include "_multivc.h"

int MV_PlayFLAC(char *ptr, uint32_t ptrlength, int loopstart, int loopend, int pitchoffset,
    int vol, int left, int right, int priority, float volume, uint32_t callbackval)
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
    UNREFERENCED_PARAMETER(volume);
    UNREFERENCED_PARAMETER(callbackval);

    MV_Printf("MV_PlayFLAC: FLAC support not included in this binary.\n");
    return -1;
}

int MV_PlayFLAC3D(char *ptr, uint32_t ptrlength, int loophow, int pitchoffset, int angle,
    int distance, int priority, float volume, uint32_t callbackval)
{
    UNREFERENCED_PARAMETER(ptr);
    UNREFERENCED_PARAMETER(ptrlength);
    UNREFERENCED_PARAMETER(loophow);
    UNREFERENCED_PARAMETER(pitchoffset);
    UNREFERENCED_PARAMETER(angle);
    UNREFERENCED_PARAMETER(distance);
    UNREFERENCED_PARAMETER(priority);
    UNREFERENCED_PARAMETER(volume);
    UNREFERENCED_PARAMETER(callbackval);

    MV_Printf("MV_PlayFLAC: FLAC support not included in this binary.\n");
    return -1;
}
#endif  // HAVE_FLAC
