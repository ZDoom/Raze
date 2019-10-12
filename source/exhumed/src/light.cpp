
#include "light.h"
#include "engine.h"
#include "exhumed.h"
#include "view.h"
#include "cd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define kMaxGrads	12

const char *GradList[kMaxGrads] = {
    "normal.rmp",
    "nodim.rmp",
    "torch.rmp",
    "notorch.rmp",
    "brite.rmp",
    "redbrite.rmp",
    "grnbrite.rmp",
    "normal.rmp",
    "nodim.rmp",
    "torch.rmp",
    "notorch.rmp",
    "brite.rmp"
};

int rtint = 0;
int gtint = 0;
int btint = 0;
char *origpalookup[kMaxPalookups];
//unsigned char curpal[768];
//unsigned char kenpal[768];
palette_t *fadedestpal;
palette_t *fadecurpal;
short nPalDelay;
short nPalDiff;
short overscanindex;

// keep a local copy of the palette that would have been sent to the VGA display adapter
uint8_t vgaPalette[768];


void MyLoadPalette()
{
    //int hFile = kopen4load("PALETTE.DAT", 1);
    //if (hFile == -1)
    //{
    //    initprintf("Error reading palette 'PALETTE.DAT'\n");
    //    return;
    //}
    //
    //kread(hFile, kenpal, sizeof(kenpal));
    //kclose(hFile);
    videoSetPalette(0, BASEPAL, 0);
    SetOverscan(BASEPAL);
}

int LoadPaletteLookups()
{
    uint8_t buffer[256*64];
    numshades = 64;

    for (int i = 0; i < kMaxGrads; i++)
    {
        int hFile = kopen4load(GradList[i], 1);
        if (hFile == -1)
        {
            initprintf("Error reading palette lookup '%s'\n", GradList[i]);
            return 0;
        }

        kread(hFile, buffer, 256*64);
        // TODO: dumb hack
        if (palookup[i])
            ALIGNED_FREE_AND_NULL(palookup[i]);
        paletteSetLookupTable(i, buffer);
        kclose(hFile);

        origpalookup[i] = palookup[i];
    }

    return 1;
}

void SetGreenPal()
{
    for (int i = 0; i < 12; i++)
    {
        palookup[i] = palookup[6];
    }

    palookup[5] = origpalookup[5];
}

void RestoreGreenPal()
{
    for (int i = 0; i < 12; i++)
    {
        palookup[i] = origpalookup[i];
    }
}

void WaitVBL()
{
#ifdef __WATCOMC__
    while (!(inp(0x3da) & 8));
#endif
}

//void MySetPalette(unsigned char *palette)
//{
//    WaitVBL();
//
//    // TODO
//    kensetpalette(palette);
//
//    memcpy(vgaPalette, palette, sizeof(vgaPalette));
//}

//void GetCurPal(unsigned char *palette)
//{
//    if (!palette) {
//        memcpy(curpal, vgaPalette, sizeof(curpal));
//    }
//    else {
//        memcpy(palette, vgaPalette, sizeof(curpal));
//    }
//}

void GrabPalette()
{
    SetOverscan(BASEPAL);

    memcpy(curpalettefaded, curpalette, sizeof(curpalette));
    videoUpdatePalette(0, 256);

    nPalDiff  = 0;
    nPalDelay = 0;

    btint = 0;
    gtint = 0;
    rtint = 0;
}

void BlackOut()
{
    for (int i = 0; i < 256; i++)
    {
        curpalettefaded[i].r = 0;
        curpalettefaded[i].g = 0;
        curpalettefaded[i].b = 0;
    }
    videoUpdatePalette(0, 256);
}

void RestorePalette()
{
    memcpy(curpalettefaded, curpalette, sizeof(curpalette));
    videoUpdatePalette(0, 256);
}

void WaitTicks(int nTicks)
{
    if (htimer)
    {
        nTicks += (int)totalclock;
        while (nTicks > (int)totalclock) { HandleAsync(); }
    }
    else
    {
        while (nTicks > 0) {
            nTicks--;
            WaitVBL();
        }
    }
}

// unused
void DoFadeToRed()
{
    for (int i = 0; i < 256; i++)
    {
        if (curpalettefaded[i].g > 0)
        {
            curpalettefaded[i].g -= 4;
            if (curpalettefaded[i].g < 0)
                curpalettefaded[i].g = 0;
        }

        if (curpalettefaded[i].b > 0)
        {
            curpalettefaded[i].b -= 4;
            if (curpalettefaded[i].b < 0)
                curpalettefaded[i].b = 0;
        }
    }

    videoUpdatePalette(0, 256);
}

void FadeToWhite()
{
    int ebx = 0;
    int const palstep = (videoGetRenderMode() >= REND_POLYMOST) ? 255 : 4;
    int const fadestep = (videoGetRenderMode() >= REND_POLYMOST) ? 1 : 64;

    for (int i = 0; i < fadestep; i++)
    {
        palette_t *pPal = curpalettefaded;

        for (int j = 0; j < 256; j++)
        {
            if (pPal->r < 255)
            {
                pPal->r += palstep;
                if (pPal->r > 255)
                    pPal->r = 255;
                ebx++;
            }
            if (pPal->g < 255)
            {
                pPal->g += palstep;
                if (pPal->g > 255)
                    pPal->g = 255;
                ebx++;
            }
            if (pPal->b < 255)
            {
                pPal->b += palstep;
                if (pPal->b > 255)
                    pPal->b = 255;
                ebx++;
            }
            pPal++;
        }

        videoUpdatePalette(0, 256);
        WaitTicks(2);

        // need to page flip in each iteration of the loop for non DOS version
        videoNextPage();

        if (!ebx) {
            return;
        }
    }
}

void FadeOut(int bFadeMusic)
{
    int const palstep = (videoGetRenderMode() >= REND_POLYMOST) ? 255 : 4;
    int const fadestep = (videoGetRenderMode() >= REND_POLYMOST) ? 1 : 64;
    if (bFadeMusic) {
        StartfadeCDaudio();
    }

    for (int i = fadestep; i > 0; i--)
    {
        int v4 = 0;
        palette_t *pPal = curpalettefaded;

        for (int j = 0; j < 256; j++)
        {
            if (pPal->r > 0)
            {
                pPal->r -= palstep;
                if (pPal->r < 0)
                    pPal->r = 0;
                v4++;
            }
            if (pPal->g > 0)
            {
                pPal->g -= palstep;
                if (pPal->g < 0)
                    pPal->g = 0;
                v4++;
            }
            if (pPal->b > 0)
            {
                pPal->b -= palstep;
                if (pPal->b < 0)
                    pPal->b = 0;
                v4++;
            }
            pPal++;
        }

        videoUpdatePalette(0, 256);
        WaitTicks(2);

        // need to page flip in each iteration of the loop for non DOS version
        videoNextPage();

        if (v4 == 0) {
            break;
        }

        if (bFadeMusic) {
            StepFadeCDaudio();
        }
    }

    if (bFadeMusic) {
        while (StepFadeCDaudio() != 0) {}
    }

    EraseScreen(overscanindex);
}

void StartFadeIn()
{
    //fadedestpal = curpalette;
    //fadecurpal = curpal;
}

int DoFadeIn()
{
    int v2 = 0;

    for (int i = 0; i < 256; i++)
    {
        if (curpalettefaded[i].r != curpalette[i].r)
        {
            v2++;
            int diff = curpalette[i].r - curpalettefaded[i].r;
            if (klabs(diff) < 4)
                curpalettefaded[i].r = curpalette[i].r;
            else
                curpalettefaded[i].r += 4 * ksgn(diff);
        }
        if (curpalettefaded[i].g != curpalette[i].g)
        {
            v2++;
            int diff = curpalette[i].g - curpalettefaded[i].g;
            if (klabs(diff) < 4)
                curpalettefaded[i].g = curpalette[i].g;
            else
                curpalettefaded[i].g += 4 * ksgn(diff);
        }
        if (curpalettefaded[i].b != curpalette[i].b)
        {
            v2++;
            int diff = curpalette[i].b - curpalettefaded[i].b;
            if (klabs(diff) < 4)
                curpalettefaded[i].b = curpalette[i].b;
            else
                curpalettefaded[i].b += 4 * ksgn(diff);
        }
    }

    videoUpdatePalette(0, 256);

    return v2;
}

void FadeIn()
{
    if (videoGetRenderMode() >= REND_POLYMOST)
    {
        Bmemcpy(curpalettefaded, curpalette, sizeof(curpalette));
        videoUpdatePalette(0, 256);
        videoNextPage();
        return;
    }
    StartFadeIn();

    int val;

    do
    {
        val = DoFadeIn();
        WaitTicks(2);

        // need to page flip in each iteration of the loop for non DOS version
        videoNextPage();
    } while (val);
}

void FixPalette()
{
    if (!nPalDiff) {
        return;
    }

    if (nPalDelay > 0)
    {
        nPalDelay--;
        return;
    }

    nPalDelay = 5;

    for (int i = 0; i < 256; i++)
    {
        short nVal;

        nVal = curpalettefaded[i].r - curpalette[i].r;
        if (nVal > 0)
        {
            if (nVal > 20)
            {
                curpalettefaded[i].r -= 20;
            }
            else
            {
                curpalettefaded[i].r = curpalette[i].r;
            }
        }

        nVal = curpalettefaded[i].g - curpalette[i].g;
        if (nVal > 0)
        {
            if (nVal > 20)
            {
                curpalettefaded[i].g -= 20;
            }
            else
            {
                curpalettefaded[i].g = curpalette[i].g;
            }
        }

        nVal = curpalettefaded[i].b - curpalette[i].b;
        if (nVal > 0)
        {
            if (nVal > 20)
            {
                curpalettefaded[i].b -= 20;
            }
            else
            {
                curpalettefaded[i].b = curpalette[i].b;
            }
        }
    }

    nPalDiff -= 20;
    gtint -= 20;
    rtint -= 20;

    if (gtint < 0) {
        gtint = 0;
    }

    if (rtint < 0) {
        rtint = 0;
    }

    if (nPalDiff < 0) {
        nPalDiff = 0;
    }

    videoUpdatePalette(0, 256);
}

void TintPalette(int r, int g, int b)
{
    int r2 = r;
    int g2 = g;
    int b2 = b;

    palette_t *pPal = curpalettefaded;

    if (bCamera) {
        return;
    }

    // range limit R between 20 and 255 if positive
    if (r > 255)
    {
        r = 255;
    }
    else
    {
        if (r && r < 20) {
            r = 20;
        }
    }

    // range limit G between 20 and 255 if positive
    if (g > 255)
    {
        g = 255;
    }
    else
    {
        if (g && g < 20) {
            g = 20;
        }
    }

    // range limit B between 20 and 255 if positive
    if (b > 255)
    {
        b = 255;
    }
    else
    {
        if (b && b < 20) {
            b = 20;
        }
    }

    // loc_17EFA
    if (g && gtint > 32) {
        return;
    }

    gtint += g;

    if (r && rtint > 256) {
        return;
    }

    rtint += r;

    // do not modify r, g or b variables from this point on
    b2 = b;
    int nVal;

    // loc_17F49
    if (klabs(r) > klabs(g)) {
        nVal = klabs(r);
    }
    else {
        nVal = klabs(g);
    }

    if (nVal < klabs(b)) {
        nVal = klabs(b);
    }

    nPalDiff += nVal;

    for (int i = 0; i < 256; i++)
    {
        pPal->r += r;
        if (pPal->r > 255) {
            pPal->r = 255;
        }

        pPal->g += g;
        if (pPal->g > 255) {
            pPal->g = 255;
        }

        pPal->b += b;
        if (pPal->b > 255) {
            pPal->b = 255;
        }

        pPal++;
    }

    nPalDelay = 0;
}

void DoOverscanSet(short someval)
{
#ifdef __WATCOMC__
    union REGS regs;

    regs.h.al = 1;
    regs.h.ah = 0x10;
    regs.h.ch = someval;

    int386(0x10, &regs, &regs);

#endif
}

// unused
void SetWhiteOverscan()
{

}

void SetOverscan(int id)
{
    if (basepaltable[id] == NULL)
        return;
    uint8_t *palette = basepaltable[id];
    int edi = 1000;
    overscanindex = 0;

    for (int i = 0; i < 256; i++)
    {
        int ebx = 0;

        for (int j = 0; j < 3; j++)
        {
            uint8_t cl = *palette;
            palette++;
            ebx += cl;
        }

        if (ebx < edi)
        {
            edi = ebx;
            overscanindex = i;
        }
    }

    DoOverscanSet(overscanindex);
}
