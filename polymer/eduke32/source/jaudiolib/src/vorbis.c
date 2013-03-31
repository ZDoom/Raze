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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 
 */

/**
 * OggVorbis source support for MultiVoc
 */

#ifdef HAVE_VORBIS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <errno.h>
#include "pitch.h"
#include "multivoc.h"
#include "_multivc.h"

#define OV_EXCLUDE_STATIC_CALLBACKS

#if defined __APPLE__
# include <vorbis/vorbisfile.h>
#elif defined GEKKO
# define USING_TREMOR
# include <tremor/ivorbisfile.h>
#else
# include "vorbis/vorbisfile.h"
#endif

#define BLOCKSIZE 0x8000


typedef struct {
   void * ptr;
   size_t length;
   size_t pos;
   
   OggVorbis_File vf;
   
   char block[BLOCKSIZE];
   int32_t lastbitstream;
} vorbis_data;

// designed with multiple calls in mind
static void MV_GetVorbisCommentLoops(VoiceNode *voice, vorbis_comment *vc)
{
    int32_t comment;
    uint8_t loopTagCount;

    const char *vc_loopstart = NULL;
    const char *vc_loopend = NULL;
    const char *vc_looplength = NULL;

    if (vc == NULL)
        return;

    for (comment = 0; comment < vc->comments; ++comment)
    {
        const char *entry = (const char *) vc->user_comments[comment];
        if (entry != NULL && entry[0] != '\0')
        {
            const char *value = strchr(entry,'=');
            const size_t field = value-entry;
            value += 1;

            for (loopTagCount = 0; loopTagCount < loopStartTagCount && vc_loopstart == NULL; ++loopTagCount)
                if (strncasecmp(entry, loopStartTags[loopTagCount], field) == 0)
                    vc_loopstart = value;

            for (loopTagCount = 0; loopTagCount < loopEndTagCount && vc_loopend == NULL; ++loopTagCount)
                if (strncasecmp(entry, loopEndTags[loopTagCount], field) == 0)
                    vc_loopend = value;

            for (loopTagCount = 0; loopTagCount < loopLengthTagCount && vc_looplength == NULL; ++loopTagCount)
                if (strncasecmp(entry, loopLengthTags[loopTagCount], field) == 0)
                    vc_looplength = value;
        }
    }

    if (vc_loopstart != NULL)
    {
        {
            const ogg_int64_t ov_loopstart = atol(vc_loopstart);
            if (ov_loopstart >= 0) // a loop starting at 0 is valid
            {
                voice->LoopStart = (const char *) (intptr_t) ov_loopstart;
                voice->LoopSize = 1;
            }
        }
    }
    if (vc_loopend != NULL)
    {
        if (voice->LoopSize > 0)
        {
            const ogg_int64_t ov_loopend = atol(vc_loopend);
            if (ov_loopend > 0) // a loop ending at 0 is invalid
                voice->LoopEnd = (const char *) (intptr_t) ov_loopend;
        }
    }
    if (vc_looplength != NULL)
    {
        if (voice->LoopSize > 0 && voice->LoopEnd == 0)
        {
            const ogg_int64_t ov_looplength = atol(vc_looplength);
            if (ov_looplength > 0) // a loop of length 0 is invalid
                voice->LoopEnd = (const char *) ((intptr_t) ov_looplength + (intptr_t) voice->LoopStart);
        }
    }
}

// callbacks

static size_t read_vorbis(void * ptr, size_t size, size_t nmemb, void * datasource)
{
   vorbis_data * vorb = (vorbis_data *) datasource;
   size_t nread = 0;
   size_t bytes;
   
   errno = 0;

   if (vorb->length == vorb->pos) {
      return 0;
   }
   
   for (; nmemb > 0; nmemb--, nread++) {
      bytes = vorb->length - vorb->pos;
      if (size < bytes) {
         bytes = size;
      }
      
      memcpy(ptr, (uint8_t *)vorb->ptr + vorb->pos, bytes);
      vorb->pos += bytes;
      ptr = (uint8_t *)ptr + bytes;
      
      if (vorb->length == vorb->pos) {
         nread++;
         break;
      }
   }
   
   return nread;
}


static int32_t seek_vorbis(void * datasource, ogg_int64_t offset, int32_t whence)
{
   vorbis_data * vorb = (vorbis_data *) datasource;
   
   switch (whence) {
      case SEEK_SET: vorb->pos = 0; break;
      case SEEK_CUR: break;
      case SEEK_END: vorb->pos = vorb->length; break;
   }
   
   vorb->pos += offset;

   if (vorb->pos > vorb->length) {
      vorb->pos = vorb->length;
   }
   
   return vorb->pos;
}

static int32_t close_vorbis(void * datasource)
{
    UNREFERENCED_PARAMETER(datasource);
   return 0;
}

static long tell_vorbis(void * datasource)
{
   vorbis_data * vorb = (vorbis_data *) datasource;
   
   return vorb->pos;
}

static ov_callbacks vorbis_callbacks = {
   read_vorbis,
   seek_vorbis,
   close_vorbis,
   tell_vorbis
};


/*---------------------------------------------------------------------
Function: MV_GetNextVorbisBlock

Controls playback of OggVorbis data
---------------------------------------------------------------------*/

static playbackstatus MV_GetNextVorbisBlock
(
 VoiceNode *voice
 )

{
   vorbis_data * vd = (vorbis_data *) voice->extra;
   int32_t bytes, bytesread;
   int32_t bitstream, err;

   voice->Playing = TRUE;
   
   bytesread = 0;
   do {
#ifdef USING_TREMOR
       bytes = ov_read(&vd->vf, vd->block + bytesread, BLOCKSIZE - bytesread, &bitstream);
#else
       bytes = ov_read(&vd->vf, vd->block + bytesread, BLOCKSIZE - bytesread, 0, 2, 1, &bitstream);
#endif
       //fprintf(stderr, "ov_read = %d\n", bytes);
       if (bytes > 0) {
           bytesread += bytes;
           if ((ogg_int64_t)(intptr_t)voice->LoopEnd > 0
                && ov_pcm_tell(&vd->vf) >= (ogg_int64_t)(intptr_t)voice->LoopEnd) {
                   err = ov_pcm_seek(&vd->vf,(ogg_int64_t)(intptr_t)voice->LoopStart);
                   if (err != 0) {
                       MV_Printf("MV_GetNextVorbisBlock ov_pcm_seek: LOOP_START %l, LOOP_END %l, err %d\n",
                           (ogg_int64_t)(intptr_t)voice->LoopStart, (ogg_int64_t)(intptr_t)voice->LoopEnd, err);
                   } else {
                       continue;
                   }
               }
            continue;
       }
       else if (bytes == OV_HOLE) continue;
       else if (bytes == 0) {
           if (voice->LoopSize > 0) {
               err = ov_pcm_seek(&vd->vf,(ogg_int64_t)(intptr_t)voice->LoopStart);
               if (err != 0) {
                   MV_Printf("MV_GetNextVorbisBlock ov_pcm_seek: LOOP_START %l, err %d\n",
                       (ogg_int64_t)(intptr_t)voice->LoopStart, err);
               } else {
                   continue;
               }
           } else {
               break;
           }
       } else if (bytes < 0) {
           MV_Printf("MV_GetNextVorbisBlock ov_read: err %d\n", bytes);
           voice->Playing = FALSE;
           return NoMoreData;
       }
   } while (bytesread < BLOCKSIZE);

   if (bytesread == 0) {
      voice->Playing = FALSE;
      return NoMoreData;
   }
      
   if (bitstream != vd->lastbitstream) {
      vorbis_info * vi = 0;
      
      vi = ov_info(&vd->vf, -1);
      if (!vi || (vi->channels != 1 && vi->channels != 2)) {
         voice->Playing = FALSE;
         return NoMoreData;
      }
      
      voice->channels = vi->channels;
      voice->SamplingRate = vi->rate;
      voice->RateScale    = ( voice->SamplingRate * voice->PitchScale ) / MV_MixRate;
      voice->FixedPointBufferSize = ( voice->RateScale * MV_MIXBUFFERSIZE ) - voice->RateScale;
      MV_SetVoiceMixMode( voice );
      vd->lastbitstream = bitstream;
   }

   bytesread /= 2 * voice->channels;
   
   voice->position    = 0;
   voice->sound       = vd->block;
   voice->BlockLength = 0;
   voice->length      = bytesread << 16; // ???: Should the literal 16 be voice->bits?

#ifdef GEKKO
    {
        // If libtremor had the three additional ov_read() parameters that libvorbis has,
        // this would be better handled using the endianness parameter.
        int16_t *data = (int16_t*)(voice->sound); // assumes signed 16-bit
        for (bytesread = 0; bytesread < BLOCKSIZE / 2; ++bytesread)
            data[bytesread] = (data[bytesread] & 0xff) << 8 | ((data[bytesread] & 0xff00) >> 8);
    }
#endif
   
   return KeepPlaying;
}


/*---------------------------------------------------------------------
Function: MV_PlayVorbis3D

Begin playback of sound data at specified angle and distance
from listener.
---------------------------------------------------------------------*/

int32_t MV_PlayVorbis3D
(
 char *ptr,
 uint32_t ptrlength,
 int32_t loophow,
 int32_t  pitchoffset,
 int32_t  angle,
 int32_t  distance,
 int32_t  priority,
 uint32_t callbackval
 )

{
   int32_t left;
   int32_t right;
   int32_t mid;
   int32_t volume;
   int32_t status;
   
   if ( !MV_Installed )
   {
      MV_SetErrorCode( MV_NotInstalled );
      return MV_Error;
   }
   
   if ( distance < 0 )
   {
      distance  = -distance;
      angle    += MV_NUMPANPOSITIONS / 2;
   }
   
   volume = MIX_VOLUME( distance );
   
   // Ensure angle is within 0 - 127
   angle &= MV_MAXPANPOSITION;
   
   left  = MV_PanTable[ angle ][ volume ].left;
   right = MV_PanTable[ angle ][ volume ].right;
   mid   = max( 0, 255 - distance );
   
   status = MV_PlayVorbis(ptr, ptrlength, loophow, -1, pitchoffset, mid, left, right, priority, callbackval);
   
   return status;
}


/*---------------------------------------------------------------------
Function: MV_PlayVorbis

Begin playback of sound data with the given sound levels and
priority.
---------------------------------------------------------------------*/

int32_t MV_PlayVorbis
(
 char *ptr,
 uint32_t ptrlength,
 int32_t   loopstart,
 int32_t   loopend,
 int32_t   pitchoffset,
 int32_t   vol,
 int32_t   left,
 int32_t   right,
 int32_t   priority,
 uint32_t callbackval
 )

{
   VoiceNode   *voice;
   int32_t          status;
   vorbis_data * vd = 0;
   vorbis_info * vi = 0;

   UNREFERENCED_PARAMETER(loopend);

   if ( !MV_Installed )
   {
      MV_SetErrorCode( MV_NotInstalled );
      return MV_Error;
   }
   
   vd = (vorbis_data *) malloc( sizeof(vorbis_data) );
   if (!vd) {
      MV_SetErrorCode( MV_InvalidVorbisFile );
      return MV_Error;
   }
   
   memset(vd, 0, sizeof(vorbis_data));
   vd->ptr = ptr;
   vd->pos = 0;
   vd->length = ptrlength;
   vd->lastbitstream = -1;
   
   status = ov_open_callbacks((void *) vd, &vd->vf, 0, 0, vorbis_callbacks);
   if (status < 0) {
      MV_Printf("MV_PlayVorbis: err %d\n", status);
      MV_SetErrorCode( MV_InvalidVorbisFile );
      return MV_Error;
   }
   
   vi = ov_info(&vd->vf, 0);
   if (!vi) {
      ov_clear(&vd->vf);
      free(vd);
      MV_SetErrorCode( MV_InvalidVorbisFile );
      return MV_Error;
   }
   
   if (vi->channels != 1 && vi->channels != 2) {
      ov_clear(&vd->vf);
      free(vd);
      MV_SetErrorCode( MV_InvalidVorbisFile );
      return MV_Error;
   }
   
   // Request a voice from the voice pool
   voice = MV_AllocVoice( priority );
   if ( voice == NULL )
   {
      ov_clear(&vd->vf);
      free(vd);
      MV_SetErrorCode( MV_NoVoices );
      return MV_Error;
   }
   
   voice->wavetype    = Vorbis;
   voice->bits        = 16;
   voice->channels    = vi->channels;
   voice->extra       = (void *) vd;
   voice->GetSound    = MV_GetNextVorbisBlock;
   voice->NextBlock   = vd->block;
   voice->DemandFeed  = NULL;
   voice->LoopCount   = 0;
   voice->BlockLength = 0;
   voice->length      = 0;
   voice->next        = NULL;
   voice->prev        = NULL;
   voice->priority    = priority;
   voice->callbackval = callbackval;

   voice->LoopStart   = 0;
   voice->LoopEnd     = 0;
   voice->LoopSize    = (loopstart >= 0 ? 1 : 0);

    // load loop tags from metadata
    MV_GetVorbisCommentLoops(voice, ov_comment(&vd->vf, 0));

   voice->Playing     = TRUE;
   voice->Paused      = FALSE;

   MV_SetVoicePitch(voice, vi->rate, pitchoffset);
   MV_SetVoiceMixMode( voice );

   MV_SetVoiceVolume( voice, vol, left, right );
   MV_PlayVoice( voice );
   
   return voice->handle;
}

void MV_ReleaseVorbisVoice( VoiceNode * voice )
{
   vorbis_data * vd = (vorbis_data *) voice->extra;
   
   if (voice->wavetype != Vorbis) {
      return;
   }
   
   ov_clear(&vd->vf);
   free(vd);
   
   voice->extra = 0;
}

#endif //HAVE_VORBIS
