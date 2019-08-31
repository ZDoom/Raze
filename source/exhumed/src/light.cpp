
#include "light.h"
#include "engine.h"
#include "exhumed.h"
#include "view.h"
#include "cd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __WATCOMC__
#include <dos.h>
#include <conio.h>
#include <i86.h>

void handleevents()
{
}
#else

#ifdef __cplusplus
extern "C" {
#endif
int handleevents(void);
#ifdef __cplusplus
}
#endif
#endif

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
unsigned char curpal[768];
unsigned char kenpal[768];
unsigned char *fadedestpal;
unsigned char *fadecurpal;
short nPalDelay;
short nPalDiff;
short overscanindex;

// keep a local copy of the palette that would have been sent to the VGA display adapter
uint8_t vgaPalette[768];


void MyLoadPalette()
{
    int hFile = kopen4load("PALETTE.DAT", 1);
    if (hFile == -1)
    {
        initprintf("Error reading palette 'PALETTE.DAT'\n");
        return;
    }

    kread(hFile, kenpal, sizeof(kenpal));
    kclose(hFile);

    SetOverscan(kenpal);
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

void MySetPalette(unsigned char *palette)
{
    WaitVBL();

#ifdef __WATCOMC__
    outp(0x3C8, 0);

    int i;
    for (i = 0; i < 768; i++)
    {
        outp(0x3C9, *palette);
        palette++;
    }

#else
    // TODO
    kensetpalette(palette);

    memcpy(vgaPalette, palette, sizeof(vgaPalette));
#endif
}

void GetCurPal(unsigned char *palette)
{
#ifdef __WATCOMC__
    if (!palette) {
        palette = curpal;
    }

    outp(0x3C7, 0);

    int i;
    for (i = 0; i < 256; i++)
    {
        *palette = inp(0x3C9);
        palette++;
        *palette = inp(0x3C9);
        palette++;
        *palette = inp(0x3C9);
        palette++;
    }

#else
    if (!palette) {
        memcpy(curpal, vgaPalette, sizeof(curpal));
    }
    else {
        memcpy(palette, vgaPalette, sizeof(curpal));
    }
#endif
}

void GrabPalette()
{
    SetOverscan(kenpal);

    memcpy(curpal, kenpal, sizeof(curpal));

    MySetPalette(kenpal);

    nPalDiff  = 0;
    nPalDelay = 0;

    btint = 0;
    gtint = 0;
    rtint = 0;
}

void BlackOut()
{
    memset(curpal, 0, sizeof(curpal));
    MySetPalette(curpal);
}

void RestorePalette()
{
    memcpy(curpal, kenpal, sizeof(curpal));
    MySetPalette(curpal);
}

void WaitTicks(int nTicks)
{
    if (htimer)
    {
        nTicks += totalclock;
        while (nTicks > totalclock) { handleevents(); }
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
    for (int i = 0; i < 256; i += 3)
    {
        if (curpal[i + 1] > 0)
        {
            curpal[i + 1]--;
        }

        if (curpal[i + 2] > 0)
        {
            curpal[i + 1]--;
        }
    }

    MySetPalette(curpal);
}

void FadeToWhite()
{
    int ebx = 0;

    for (int i = 0; i < 64; i++)
    {
        uint8_t *pPal = curpal;

        for (int j = 0; j < 256; j++)
        {
            for (int k = 0; k < 3; k++)
            {
                if (*pPal < 63)
                {
                    (*pPal)++;

                    ebx++;
                }

                pPal++;
            }
        }

        MySetPalette(curpal);
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
    if (bFadeMusic) {
        StartfadeCDaudio();
    }

    for (int i = 64; i > 0; i--)
    {
        int v4 = 0;

        for (int j = 0; j < 768; j++)
        {
            if (curpal[j] > 0)
            {
                curpal[j]--;
                v4++;
            }
        }

        MySetPalette(curpal);
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
    fadedestpal = kenpal;
    fadecurpal = curpal;
}

int DoFadeIn()
{
    int v2 = 0;

    for (int i = 0; i < 768; i++)
    {
        v2++;

        if (fadecurpal[i] < fadedestpal[i])
        {
            fadecurpal[i]++;
        }

        else
        {
            if (fadecurpal[i] == fadedestpal[i])
            {
                v2--;
            }
            else {
                fadecurpal[i]--;
            }
        }
    }

    MySetPalette(fadecurpal);

    return v2;
}

void FadeIn()
{
    StartFadeIn();

    while (1)
    {
        int val = DoFadeIn();
        WaitTicks(2);

        // need to page flip in each iteration of the loop for non DOS version
        videoNextPage();

        if (!val) {
            break;
        }
    }
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

    for (int i = 0; i < 768; i++)
    {
        short nVal = curpal[i] - kenpal[i];
        if (nVal > 0)
        {
            if (nVal > 5)
            {
                curpal[i] -= 5;
            }
            else
            {
                curpal[i] = kenpal[i];
            }
        }
    }

    nPalDiff -= 5;
    gtint -= 5;
    rtint -= 5;

    if (gtint < 0) {
        gtint = 0;
    }

    if (rtint < 0) {
        rtint = 0;
    }

    if (nPalDiff < 0) {
        nPalDiff = 0;
    }

    MySetPalette(curpal);
}

void TintPalette(int r, int g, int b)
{
    int r2 = r;
    int g2 = g;
    int b2 = b;

    uint8_t *pPal = curpal;

    if (bCamera) {
        return;
    }

    // range limit R between 5 and 63 if positive
    if (r > 63)
    {
        r = 63;
    }
    else
    {
        if (r && r < 5) {
            r = 5;
        }
    }

    // range limit G between 5 and 63 if positive
    if (g > 63)
    {
        g = 63;
    }
    else
    {
        if (g && g < 5) {
            g = 5;
        }
    }

    // range limit B between 5 and 63 if positive
    if (b > 63)
    {
        b = 63;
    }
    else
    {
        if (b && b < 5) {
            b = 5;
        }
    }

    // loc_17EFA
    if (g && gtint > 8) {
        return;
    }

    gtint += g;

    if (r && rtint > 64) {
        return;
    }

    rtint += r;

    // do not modify r, g or b variables from this point on
    r2 = r;
    g2 = g;
    b2 = b;

    if (r2 < 0) {
        r2 = -r2;
    }

    // loc_17F3A
    if (g2 < 0) {
        g2 = -g2;
    }

    int nVal;

    // loc_17F49
    if (r2 > g2) {
        nVal = r;
    }
    else {
        nVal = g;
    }

    if (nVal < 0) {
        nVal = -nVal;
    }

    if (b2 < 0) {
        b2 = -b2;
    }

    if (nVal > b2) {
        nVal = b2;
    }
    else {
        if (b < 0) {
            nVal = -b;
        }
    }

    nPalDiff += nVal;

    for (int i = 0; i < 256; i++)
    {
        *pPal += r;
        if (*pPal > 63) {
            *pPal = 63;
        }

        pPal++;

        *pPal += g;
        if (*pPal > 63) {
            *pPal = 63;
        }

        pPal++;

        *pPal += b;
        if (*pPal > 63) {
            *pPal = 63;
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

void SetOverscan(unsigned char *palette)
{
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
