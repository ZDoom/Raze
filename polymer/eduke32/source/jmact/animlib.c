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
static int32_t Anim_Started = FALSE;

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

uint16_t findpage(uint16_t framenumber)
{
    // curlpnum is initialized to 0xffff, obviously
    uint16_t i = (uint16_t)(anim->curlpnum & ~0xffff);

    CheckAnimStarted("findpage");

    if (framenumber < anim->currentframe)
        i = 0;

    for (; i<anim->lpheader.nLps; i++)
        if (anim->LpArray[i].baseRecord <= framenumber &&
                anim->LpArray[i].baseRecord + anim->LpArray[i].nRecords > framenumber)
            return(i);

    return(i);
}


//****************************************************************************
//
//      loadpage ()
//      - seek out and load in the large page specified
//
//****************************************************************************

void loadpage(uint16_t pagenumber, uint16_t **pagepointer)
{
    CheckAnimStarted("loadpage");

    if (anim->curlpnum == pagenumber)
        return;

    anim->curlp = &anim->LpArray[(anim->curlpnum = pagenumber)];
    *pagepointer = (uint16_t *)(anim->buffer + 0xb00 + (pagenumber*IMAGEBUFFERSIZE) +
                                sizeof(lp_descriptor) + sizeof(uint16_t));
}


//****************************************************************************
//
//      CPlayRunSkipDump ()
//      - I found this less obfuscated version of the anm decompressor around,
//        says it's (c) 1998 "Jari Komppa aka Sol/Trauma".  This code is
//        public domain and has been modified a bit by me.
//
//      - As a side note, it looks like this format came about in 1989 and
//        never went anywhere after that, and appears to have been the format
//        used by Electronic Arts' DeluxePaint Animation, which never made it
//        past version 1.0.
//
//****************************************************************************

static void CPlayRunSkipDump(uint8_t * srcP, uint8_t * dstP)
{
    do
    {
        int32_t color, count=*srcP++;

        if (count==0)
        {
            /* Short RLE */
            count=*srcP;
            color=*(srcP+1);
            srcP += 2;
            Bmemset(dstP,color,count);
            dstP+=count;
        }
        else if ((count&0x80) == 0)
        {
            /* Short copy */
            Bmemcpy(dstP,srcP,count);
            dstP+=count;
            srcP+=count;
        }
        else
        {
            /* long op or short skip */
            count &= ~0x80;

            if (count > 0) /* short skip */
                dstP+=count;
            else
            {
                /* long op */
                count = *srcP+((*(srcP+1))<<8);
                srcP += 2;

                if (count==0) /* stop sign */
                    return;

                if ((count&0x8000) == 0) /* long skip */
                    dstP+=count;
                else
                {
                    count &= ~0x8000;

                    if ((count&0x4000) == 0)
                    {
                        /* long copy */
                        Bmemcpy(dstP,srcP,count);
                        dstP+=count;
                        srcP+=count;
                        continue;
                    }

                    /* and finally, long RLE. */
                    count &= ~0x4000;
                    color=*srcP++;
                    Bmemset(dstP,color,count);
                    dstP+=count;
                }
            }
        }
    }
    while (1);
}


//****************************************************************************
//
//      renderframe ()
//      - draw the frame sepcified from the large page in the buffer pointed to
//
//****************************************************************************

void renderframe(uint16_t framenumber, uint16_t *pagepointer)
{
    uint16_t offset=0;
    uint16_t i;
    uint16_t destframe;
    uint8_t *ppointer;

    CheckAnimStarted("renderframe");
    destframe = framenumber - anim->curlp->baseRecord;

    for (i = 0; i < destframe; i++)
        offset += B_LITTLE16(pagepointer[i]);

    ppointer = (uint8_t *)pagepointer;

    ppointer+=anim->curlp->nRecords*2+offset;
    if (ppointer[1])
        ppointer += (4 + B_LITTLE16(((uint16_t *)ppointer)[1]) + (B_LITTLE16(((uint16_t *)ppointer)[1]) & 1));
    else
        ppointer+=4;

    CPlayRunSkipDump((uint8_t *)ppointer, (uint8_t *)anim->imagebuffer);
}


//****************************************************************************
//
//      drawframe ()
//      - high level frame draw routine
//
//****************************************************************************

void drawframe(uint16_t framenumber)
{
    CheckAnimStarted("drawframe");
    loadpage(findpage(framenumber), &anim->thepage);
    renderframe(framenumber, anim->thepage);
}


//****************************************************************************
//
//      ANIM_LoadAnim ()
//
//****************************************************************************

void ANIM_LoadAnim(char * buffer)
{
    uint16_t i;

    if (!Anim_Started)
    {
        anim = Bmalloc(sizeof(anim_t));
        Anim_Started = TRUE;
    }

    anim->buffer = (uint8_t *)buffer;
    anim->curlpnum = 0xffff;
    anim->currentframe = -1;
    Bmemcpy(&anim->lpheader, buffer, sizeof(lpfileheader));

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

    buffer += sizeof(lpfileheader)+128;
    // load the color palette
    for (i = 0; i < 768; i += 3)
    {
        anim->pal[i+2] = (*buffer++)>>2;
        anim->pal[i+1] = (*buffer++)>>2;
        anim->pal[i] = (*buffer++)>>2;
        buffer++;
    }
    // read in large page descriptors

    Bmemcpy(&anim->LpArray, buffer,sizeof(anim->LpArray));

    for (i = 0; i < sizeof(anim->LpArray)/sizeof(lp_descriptor); i++)
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
        Bfree(anim);
        Anim_Started = FALSE;
    }
}

//****************************************************************************
//
//      ANIM_NumFrames ()
//
//****************************************************************************

int32_t ANIM_NumFrames(void)
{
    CheckAnimStarted("NumFrames");
    return anim->lpheader.nRecords;
}

//****************************************************************************
//
//      ANIM_DrawFrame ()
//
//****************************************************************************

uint8_t * ANIM_DrawFrame(int32_t framenumber)
{
    int32_t cnt;

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

uint8_t * ANIM_GetPalette(void)
{
    CheckAnimStarted("GetPalette");
    return anim->pal;
}
