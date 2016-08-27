//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
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

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
Modifications for JonoF's port by Jonathon Fowler (jf@jonof.id.au)
*/
//-------------------------------------------------------------------------

#include "compat.h"
#include "animlib.h"

//****************************************************************************
//
// LOCALS
//
//****************************************************************************

# pragma pack(push,1)

/* structure declarations for deluxe animate large page files, doesn't
   need to be in the header because there are no exposed functions
   that use any of this directly */

typedef struct lpfileheaderstruct
{
    uint32_t id;              /* 4 uint8_tacter ID == "LPF " */
    uint16_t maxLps;          /* max # largePages allowed. 256 FOR NOW.   */
    uint16_t nLps;            /* # largePages in this file. */
    uint32_t nRecords;        /* # records in this file.  65534 is current limit + ring */
    uint16_t maxRecsPerLp;    /* # records permitted in an lp. 256 FOR NOW.   */
    uint16_t lpfTableOffset;  /* Absolute Seek position of lpfTable.  1280 FOR NOW. */
    uint32_t contentType;     /* 4 character ID == "ANIM" */
    uint16_t width;           /* Width of screen in pixels. */
    uint16_t height;          /* Height of screen in pixels. */
    uint8_t variant;          /* 0==ANIM. */
    uint8_t version;          /* 0==frame rate in 18/sec, 1= 70/sec */
    uint8_t hasLastDelta;     /* 1==Last record is a delta from last-to-first frame. */
    uint8_t lastDeltaValid;   /* 0==Ignore ring frame. */
    uint8_t pixelType;        /* 0==256 color. */
    uint8_t CompressionType;  /* 1==(RunSkipDump) Only one used FOR NOW. */
    uint8_t otherRecsPerFrm;  /* 0 FOR NOW. */
    uint8_t bitmaptype;       /* 1==320x200, 256-color.  Only one implemented so far. */
    uint8_t recordTypes[32];  /* Not yet implemented. */
    uint32_t nFrames;         /* Number of actual frames in the file, includes ring frame. */
    uint16_t framesPerSecond; /* Number of frames to play per second. */
    uint16_t pad2[29];        /* 58 bytes of filler to round up to 128 bytes total. */
}
lpfileheader;                 /* (comments from original source) */

// this is the format of a large page structure
typedef struct
{
    uint16_t baseRecord;   // Number of first record in this large page.
    uint16_t nRecords;        // Number of records in lp.
    // bit 15 of "nRecords" == "has continuation from previous lp".
    // bit 14 of "nRecords" == "final record continues on next lp".
    uint16_t nBytes;                  // Total number of bytes of contents, excluding header.
} lp_descriptor;

#pragma pack(pop)

#define IMAGEBUFFERSIZE 0x10000

typedef struct
{
    uint16_t framecount;          // current frame of anim
    lpfileheader * lpheader;           // file header will be loaded into this structure
    lp_descriptor * LpArray; // arrays of large page structs used to find frames
    uint16_t curlpnum;               // initialize to an invalid Large page number
    lp_descriptor * curlp;        // header of large page currently in memory
    uint16_t * thepage;     // buffer where current large page is loaded
    uint8_t imagebuffer[IMAGEBUFFERSIZE]; // buffer where anim frame is decoded
    uint8_t * buffer;
    uint8_t pal[768];
    int32_t  currentframe;
} anim_t;

static anim_t * anim = NULL;

//****************************************************************************
//
//      findpage ()
//              - return the large page number a given frame resides in
//
//****************************************************************************

static inline uint16_t findpage(uint16_t framenumber)
{
    // curlpnum is initialized to 0xffff, obviously
    uint16_t i = (uint16_t)(anim->curlpnum & ~0xffff);
    int32_t j = 0;

    if (framenumber < anim->currentframe)
        i = 0, j++;

    // this scans the last used page and higher first and then scans the
    // previously accessed pages afterwards if it doesn't find anything
    do
    {
        for (; i<anim->lpheader->nLps; i++)
            if (anim->LpArray[i].baseRecord <= framenumber &&
                anim->LpArray[i].baseRecord + anim->LpArray[i].nRecords > framenumber)
                return i;

        if (!j && i == anim->lpheader->nLps)
        {
            // handle out of order pages... I don't think any Duke .ANM files
            // have them, but they're part of the file spec
            i = 0, j++;
            continue;
        }
    }
    while (0);

    return i;
}


//****************************************************************************
//
//      loadpage ()
//      - seek out and set pointers to the large page specified
//
//****************************************************************************

static inline void loadpage(uint16_t pagenumber, uint16_t **pagepointer)
{
    if (anim->curlpnum == pagenumber)
        return;

    anim->curlp = &anim->LpArray[(anim->curlpnum = pagenumber)];
    *pagepointer = (uint16_t *)(anim->buffer + 0xb00 + (pagenumber*IMAGEBUFFERSIZE) +
                                sizeof(lp_descriptor) + sizeof(uint16_t));
}


//****************************************************************************
//
//      decodeframe ()
//      - I found this less obfuscated version of the .ANM "decompressor",
//        (c) 1998 "Jari Komppa aka Sol/Trauma".  This code is public domain
//        but has been mostly rewritten by me.
//
//      - As a side note, it looks like this format came about in 1989 and
//        never went anywhere after that, and appears to have been the format
//        used by Electronic Arts' DeluxePaint Animation, which never made it
//        past version 1.0.
//
//****************************************************************************

static void decodeframe(uint8_t * srcP, uint8_t * dstP)
{
    do
    {
        int32_t count=*srcP++;

        if (!count) /* Short RLE */
        {
            int32_t color = *(srcP+1);
            count = *(uint8_t *)((srcP += sizeof(int16_t)) - sizeof(int16_t));
            Bmemset((dstP += count) - count, color, count);
            continue;
        }
        else if ((count & 0x80) == 0) /* Short copy */
        {
            Bmemcpy((dstP += count) - count, (srcP += count) - count, count);
            continue;
        }
        else if ((count &= ~0x80) > 0) /* short skip */
        {
            dstP += count;
            continue;
        }

        /* long op */
        count = B_LITTLE16(B_UNBUF16(srcP));
        srcP += sizeof(int16_t);

        if (!count) /* stop sign */
            return;
        else if ((count & 0x8000) == 0) /* long skip */
        {
            dstP += count;
            continue;
        }
        else if ((count &= ~0x8000) & 0x4000) /* long RLE */
        {
            int32_t color = *srcP++;
            count &= ~0x4000;
            Bmemset((dstP += count) - count, color, count);
            continue;
        }

        /* long copy */
        Bmemcpy((dstP += count) - count, (srcP += count) - count, count);
    }
    while (1);
}


//****************************************************************************
//
//      renderframe ()
//      - draw the frame sepcified from the large page in the buffer pointed to
//
//****************************************************************************

static void renderframe(uint16_t framenumber, uint16_t *pagepointer)
{
    uint8_t *ppointer;
    uint16_t offset = 0;
    uint16_t frame = framenumber - anim->curlp->baseRecord;

    while (frame--) offset += B_LITTLE16(pagepointer[frame]);

    ppointer = (uint8_t *)(pagepointer) + anim->curlp->nRecords*2 + offset + 4;

    if ((ppointer-4)[1])
        ppointer += B_LITTLE16(((uint16_t *)(ppointer-4))[1]) + (B_LITTLE16(((uint16_t *)(ppointer-4))[1]) & 1);

    decodeframe((uint8_t *)ppointer, (uint8_t *)anim->imagebuffer);
}


//****************************************************************************
//
//      drawframe ()
//      - high level frame draw routine
//
//****************************************************************************

static inline void drawframe(uint16_t framenumber)
{
    loadpage(findpage(framenumber), &anim->thepage);
    renderframe(framenumber, anim->thepage);
}

// <length> is the file size, for consistency checking.
int32_t ANIM_LoadAnim(uint8_t *buffer, int32_t length)
{
    int32_t i;

    length -= sizeof(lpfileheader)+128+768;
    if (length < 0)
        return -1;

    anim = (anim_t *)Xrealloc(anim, sizeof(anim_t));

    anim->curlpnum = 0xffff;
    anim->currentframe = -1;

    // this just modifies the data in-place instead of copying it elsewhere now
    anim->lpheader = (lpfileheader *)(anim->buffer = (uint8_t *)buffer);

    anim->lpheader->id              = B_LITTLE32(anim->lpheader->id);
    anim->lpheader->maxLps          = B_LITTLE16(anim->lpheader->maxLps);
    anim->lpheader->nLps            = B_LITTLE16(anim->lpheader->nLps);
    anim->lpheader->nRecords        = B_LITTLE32(anim->lpheader->nRecords);
    anim->lpheader->maxRecsPerLp    = B_LITTLE16(anim->lpheader->maxRecsPerLp);
    anim->lpheader->lpfTableOffset  = B_LITTLE16(anim->lpheader->lpfTableOffset);
    anim->lpheader->contentType     = B_LITTLE32(anim->lpheader->contentType);
    anim->lpheader->width           = B_LITTLE16(anim->lpheader->width);
    anim->lpheader->height          = B_LITTLE16(anim->lpheader->height);
    anim->lpheader->nFrames         = B_LITTLE32(anim->lpheader->nFrames);
    anim->lpheader->framesPerSecond = B_LITTLE16(anim->lpheader->framesPerSecond);

    length -= anim->lpheader->nLps * sizeof(lp_descriptor);
    if (length < 0)
        return -2;

    buffer += sizeof(lpfileheader)+128;

    // load the color palette
    for (i = 0; i < 768; i += 3)
    {
        anim->pal[i+2] = *buffer++;
        anim->pal[i+1] = *buffer++;
        anim->pal[i] = *buffer++;
        buffer++;
    }

    // set up large page descriptors
    anim->LpArray = (lp_descriptor *)buffer;

    // theoretically we should be able to play files with more than 256 frames now
    // assuming the utilities to create them can make them that way
    for (i = 0; i < anim->lpheader->nLps; i++)
    {
        anim->LpArray[i].baseRecord = B_LITTLE16(anim->LpArray[i].baseRecord);
        anim->LpArray[i].nRecords   = B_LITTLE16(anim->LpArray[i].nRecords);
        anim->LpArray[i].nBytes     = B_LITTLE16(anim->LpArray[i].nBytes);
    }

    return 0;
}


void ANIM_FreeAnim(void)
{
    DO_FREE_AND_NULL(anim);
}


int32_t ANIM_NumFrames(void)
{
    return anim->lpheader->nRecords;
}


uint8_t * ANIM_DrawFrame(int32_t framenumber)
{
    bssize_t cnt = anim->currentframe;

    // handle first play and looping or rewinding
    if (cnt < 0 || cnt > framenumber)
        cnt = 0;

    do drawframe(cnt++);
    while (cnt < framenumber);

    anim->currentframe = framenumber;
    return anim->imagebuffer;
}


uint8_t * ANIM_GetPalette(void)
{
    return anim->pal;
}
