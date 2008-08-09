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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
Modifications for JonoF's port by Jonathon Fowler (jonof@edgenetwk.com)
*/
//-------------------------------------------------------------------------

#include "compat.h"
#include "types.h"
#include "util_lib.h"
#include "animlib.h"

//****************************************************************************
//
// GLOBALS
//
//****************************************************************************

//****************************************************************************
//
// LOCALS
//
//****************************************************************************
static anim_t * anim=NULL;
static boolean Anim_Started = false;

//****************************************************************************
//
//      CheckAnimStarted ()
//
//****************************************************************************

static void CheckAnimStarted(char * funcname)
{
    if (!Anim_Started)
        Error("ANIMLIB_%s: Anim has not been initialized\n",funcname);
}
//****************************************************************************
//
//      findpage ()
//              - given a frame number return the large page number it resides in
//
//****************************************************************************

uint16 findpage(uint16 framenumber)
{
    uint16 i;

    CheckAnimStarted("findpage");
    for (i=0; i<anim->lpheader.nLps; i++)
    {
        if
        (
            anim->LpArray[i].baseRecord <= framenumber &&
            anim->LpArray[i].baseRecord + anim->LpArray[i].nRecords > framenumber
        )
            return(i);
    }
    return(i);
}


//****************************************************************************
//
//      loadpage ()
//      - seek out and load in the large page specified
//
//****************************************************************************

void loadpage(uint16 pagenumber, uint16 *pagepointer)
{
    int32 size;
    byte * buffer;

    CheckAnimStarted("loadpage");
    buffer = anim->buffer;
    if (anim->curlpnum != pagenumber)
    {
        anim->curlpnum = pagenumber;
        buffer += 0xb00 + (pagenumber*0x10000);
        size = sizeof(lp_descriptor);
        /*
        Bmemcpy(&anim->curlp,buffer,size);

        // JBF: why didn't this get read from the LpArray[] table?
        anim->curlp.baseRecord = B_LITTLE16(anim->curlp.baseRecord);
        anim->curlp.nRecords   = B_LITTLE16(anim->curlp.nRecords);
        anim->curlp.nBytes     = B_LITTLE16(anim->curlp.nBytes);
         */
        Bmemcpy(&anim->curlp, &anim->LpArray[pagenumber], size);

        buffer += size + sizeof(uint16);
        Bmemcpy(pagepointer,buffer,anim->curlp.nBytes+(anim->curlp.nRecords*2));
    }
}


//****************************************************************************
//
//      CPlayRunSkipDump ()
//      - This version of the decompressor is here for portability to non PC's
//
//****************************************************************************

void CPlayRunSkipDump(char *srcP, char *dstP)
{
    signed char cnt;
    uint16 wordCnt;
    byte pixel;


nextOp:
    cnt = (signed char) *srcP++;
    if (cnt > 0)
        goto dump;
    if (cnt == 0)
        goto run;
    cnt -= 0x80;
    if (cnt == 0)
        goto longOp;
    /* shortSkip */
    dstP += cnt;                    /* adding 7-bit count to 32-bit pointer */
    goto nextOp;
dump:
    do
    {
        *dstP++ = *srcP++;
    }
    while (--cnt);
    goto nextOp;
run:
    wordCnt = (byte)*srcP++;                /* 8-bit unsigned count */
    pixel = *srcP++;
    do
    {
        *dstP++ = pixel;
    }
    while (--wordCnt);

    goto nextOp;
longOp:
    wordCnt = B_LITTLE16(*((uint16 *)srcP));
    srcP += sizeof(uint16);
    if ((int16)wordCnt <= 0)
        goto notLongSkip;       /* Do SIGNED test. */

    /* longSkip. */
    dstP += wordCnt;
    goto nextOp;

notLongSkip:
    if (wordCnt == 0)
        goto stop;
    wordCnt -= 0x8000;              /* Remove sign bit. */
    if (wordCnt >= 0x4000)
        goto longRun;

    /* longDump. */
    do
    {
        *dstP++ = *srcP++;
    }
    while (--wordCnt);
    goto nextOp;

longRun:
    wordCnt -= 0x4000;              /* Clear "longRun" bit. */
    pixel = *srcP++;
    do
    {
        *dstP++ = pixel;
    }
    while (--wordCnt);
    goto nextOp;

stop:   /* all done */
    ;
}



//****************************************************************************
//
//      renderframe ()
//      - draw the frame sepcified from the large page in the buffer pointed to
//
//****************************************************************************

void renderframe(uint16 framenumber, uint16 *pagepointer)
{
    uint16 offset=0;
    uint16 i;
    uint16 destframe;
    byte *ppointer;

    CheckAnimStarted("renderframe");
    destframe = framenumber - anim->curlp.baseRecord;

    for (i = 0; i < destframe; i++)
    {
        offset += B_LITTLE16(pagepointer[i]);
    }
    ppointer = (byte *)pagepointer;

    ppointer+=anim->curlp.nRecords*2+offset;
    if (ppointer[1])
    {
        ppointer += (4 + B_LITTLE16(((uint16 *)ppointer)[1]) + (B_LITTLE16(((uint16 *)ppointer)[1]) & 1));
    }
    else
    {
        ppointer+=4;
    }

    CPlayRunSkipDump((char *)ppointer, (char *)anim->imagebuffer);
}


//****************************************************************************
//
//      drawframe ()
//      - high level frame draw routine
//
//****************************************************************************

void drawframe(uint16 framenumber)
{
    CheckAnimStarted("drawframe");
    loadpage(findpage(framenumber), anim->thepage);
    renderframe(framenumber, anim->thepage);
}


//****************************************************************************
//
//      ANIM_LoadAnim ()
//
//****************************************************************************

void ANIM_LoadAnim(char * buffer)
{
    uint16 i;
    int32 size;

    if (!Anim_Started)
    {
        anim = SafeMalloc(sizeof(anim_t));
        Anim_Started = true;
    }

    anim->buffer = (unsigned char *)buffer;
    anim->curlpnum = 0xffff;
    anim->currentframe = -1;
    size = sizeof(lpfileheader);
    Bmemcpy(&anim->lpheader, buffer, size);

    anim->lpheader.id              = B_LITTLE32(anim->lpheader.id);
    anim->lpheader.maxLps          = B_LITTLE16(anim->lpheader.maxLps);
    anim->lpheader.nLps            = B_LITTLE16(anim->lpheader.nLps);
    anim->lpheader.nRecords        = B_LITTLE32(anim->lpheader.nRecords);
    anim->lpheader.maxRecsPerLp    = B_LITTLE16(anim->lpheader.maxRecsPerLp);
    anim->lpheader.lpfTableOffset  = B_LITTLE16(anim->lpheader.lpfTableOffset);
    anim->lpheader.contentType     = B_LITTLE32(anim->lpheader.contentType);
    anim->lpheader.width           = B_LITTLE16(anim->lpheader.width);
    anim->lpheader.height          = B_LITTLE16(anim->lpheader.height);
    anim->lpheader.nFrames         = B_LITTLE32(anim->lpheader.nFrames);
    anim->lpheader.framesPerSecond = B_LITTLE16(anim->lpheader.framesPerSecond);

    buffer += size+128;
    // load the color palette
    for (i = 0; i < 768; i += 3)
    {
        anim->pal[i+2] = *buffer++;
        anim->pal[i+1] = *buffer++;
        anim->pal[i] = *buffer++;
        buffer++;
    }
    // read in large page descriptors
    size = sizeof(anim->LpArray);
    Bmemcpy(&anim->LpArray,buffer,size);

    for (i = 0; i < size/sizeof(lp_descriptor); i++)
    {
        anim->LpArray[i].baseRecord = B_LITTLE16(anim->LpArray[i].baseRecord);
        anim->LpArray[i].nRecords   = B_LITTLE16(anim->LpArray[i].nRecords);
        anim->LpArray[i].nBytes     = B_LITTLE16(anim->LpArray[i].nBytes);
    }
}

//****************************************************************************
//
//      ANIM_FreeAnim ()
//
//****************************************************************************

void ANIM_FreeAnim(void)
{
    if (Anim_Started)
    {
        SafeFree(anim);
        Anim_Started = false;
    }
}

//****************************************************************************
//
//      ANIM_NumFrames ()
//
//****************************************************************************

int32 ANIM_NumFrames(void)
{
    CheckAnimStarted("NumFrames");
    return anim->lpheader.nRecords;
}

//****************************************************************************
//
//      ANIM_DrawFrame ()
//
//****************************************************************************

byte * ANIM_DrawFrame(int32 framenumber)
{
    int32 cnt;

    CheckAnimStarted("DrawFrame");
    if ((anim->currentframe != -1) && (anim->currentframe<=framenumber))
    {
        for (cnt = anim->currentframe; cnt < framenumber; cnt++)
            drawframe(cnt);
    }
    else
    {
        for (cnt = 0; cnt < framenumber; cnt++)
            drawframe(cnt);
    }
    anim->currentframe = framenumber;
    return anim->imagebuffer;
}

//****************************************************************************
//
//      ANIM_GetPalette ()
//
//****************************************************************************

byte * ANIM_GetPalette(void)
{
    CheckAnimStarted("GetPalette");
    return anim->pal;
}
