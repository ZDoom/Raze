/*
 * softsurface.cpp
 *  An 8-bit rendering surface that can quickly upscale and blit 8-bit paletted buffers to an external 32-bit buffer.
 *
 * Copyright © 2018, Alex Dawson. All rights reserved.
 */

#include "softsurface.h"

#include "pragmas.h"
#include "build.h"

static uint8_t* buffer;
static vec2_t bufferRes;

static vec2_t destBufferRes;

static uint32_t xScale16;
static uint32_t yScale16;
static uint32_t recXScale16;

static uint32_t pPal[256];

// lookup table to find the source position within a scanline
static uint16_t* scanPosLookupTable;

template <uint32_t multiple>
static uint32_t roundUp(uint32_t num)
{
    return (num+multiple-1)/multiple * multiple;
}

static uint32_t countTrailingZeros(uint32_t u)
{
#if (defined __GNUC__  && __GNUC__>=3) || defined __clang__
    return __builtin_ctz(u);
#elif defined _MSC_VER
    DWORD result;
    _BitScanForward(&result, u);
    return result;
#else
    uint32_t last = u;
    for (; u != 0; last = u, u >>= 1);
    return last;
#endif
}

bool softsurface_initialize(vec2_t bufferResolution,
                            vec2_t destBufferResolution)
{
    if (buffer)
        softsurface_destroy();

    bufferRes = bufferResolution;
    destBufferRes = destBufferResolution;

    xScale16 = divscale16(destBufferRes.x, bufferRes.x);
    yScale16 = divscale16(destBufferRes.y, bufferRes.y);
    recXScale16 = divscale16(bufferRes.x, destBufferRes.x);

    // allocate one continuous block of memory large enough to hold the buffer, the palette,
    // and the scanPosLookupTable while maintaining alignment for each
    uint32_t bufferSize = roundUp<16>(bufferRes.x * bufferRes.y);
    buffer = (uint8_t*) Xaligned_alloc(16, bufferSize + sizeof(uint16_t)*destBufferRes.x);
    scanPosLookupTable = (uint16_t*) (buffer + bufferSize);

    // calculate the scanPosLookupTable for horizontal scaling
    uint32_t incr = recXScale16;
    for (int32_t i = 0; i < destBufferRes.x; ++i)
    {
        scanPosLookupTable[i] = incr >> 16;
        incr += recXScale16;
    }

    return true;
}

void softsurface_destroy()
{
    if (!buffer)
        return;

    ALIGNED_FREE_AND_NULL(buffer);
    scanPosLookupTable = 0;

    xScale16 = 0;
    yScale16 = 0;
    recXScale16 = 0;

    bufferRes = {};
    destBufferRes = {};
}

void softsurface_setPalette(void* pPalette,
                            uint32_t destRedMask,
                            uint32_t destGreenMask,
                            uint32_t destBlueMask)
{
    if (!buffer)
        return;
    if (!pPalette)
        return;

    uint32_t destRedShift = countTrailingZeros(destRedMask);
    uint32_t destRedLoss = 8 - countTrailingZeros((destRedMask>>destRedShift)+1);
    uint32_t destGreenShift = countTrailingZeros(destGreenMask);
    uint32_t destGreenLoss = 8 - countTrailingZeros((destGreenMask>>destGreenShift)+1);
    uint32_t destBlueShift = countTrailingZeros(destBlueMask);
    uint32_t destBlueLoss = 8 - countTrailingZeros((destBlueMask>>destBlueShift)+1);

    uint8_t* pUI8Palette = (uint8_t*) pPalette;
    for (int i = 0; i < 256; ++i)
    {
        pPal[i] = ((pUI8Palette[sizeof(uint32_t)*i] >> destRedLoss << destRedShift) & destRedMask) |
                  ((pUI8Palette[sizeof(uint32_t)*i+1] >> destGreenLoss << destGreenShift) & destGreenMask) |
                  ((pUI8Palette[sizeof(uint32_t)*i+2] >> destBlueLoss << destBlueShift) & destBlueMask);
    }
}

uint8_t* softsurface_getBuffer()
{
    return buffer;
}

vec2_t softsurface_getBufferResolution()
{
    return bufferRes;
}

vec2_t softsurface_getDestinationBufferResolution()
{
    return destBufferRes;
}

#define BLIT(x) pDst[x] = *((UINTTYPE*)(pPal+pSrc[pScanPos[x]]))
#define BLIT2(x) BLIT(x); BLIT(x+1)
#define BLIT4(x) BLIT2(x); BLIT2(x+2)
#define BLIT8(x) BLIT4(x); BLIT4(x+4)
#define BLIT16(x) BLIT8(x); BLIT8(x+8)
#define BLIT32(x) BLIT16(x); BLIT16(x+16)
#define BLIT64(x) BLIT32(x); BLIT32(x+32)
template <typename UINTTYPE>
void softsurface_blitBufferInternal(UINTTYPE* destBuffer)
{
    const uint8_t* __restrict pSrc = buffer;
    UINTTYPE* __restrict pDst = destBuffer;
    const UINTTYPE* const pEnd = destBuffer+destBufferRes.x*mulscale16(yScale16, bufferRes.y);
    uint32_t remainder = 0;
    while (pDst < pEnd)
    {
        uint16_t* __restrict pScanPos = scanPosLookupTable;
        UINTTYPE* const pScanEnd = pDst+destBufferRes.x;
        while (pDst < pScanEnd-64)
        {
            BLIT64(0);
            pDst += 64;
            pScanPos += 64;
        }
        while (pDst < pScanEnd)
        {
            BLIT(0);
            ++pDst;
            ++pScanPos;
        }
        pSrc += bufferRes.x;

        static const uint32_t MASK16 = (1<<16)-1;
        uint32_t linesCopied = 1;
        uint32_t linesToCopy = yScale16+remainder;
        remainder = linesToCopy & MASK16;
        linesToCopy = (linesToCopy >> 16)-1;
        const UINTTYPE* const __restrict pScanLineSrc = pDst-destBufferRes.x;
        while (linesToCopy)
        {
            uint32_t lines = min(linesCopied, linesToCopy);
            memcpy(pDst, pScanLineSrc, sizeof(UINTTYPE)*lines*destBufferRes.x);
            pDst += lines*destBufferRes.x;
            linesToCopy -= lines;
        }
    }
}

void softsurface_blitBuffer(uint32_t* destBuffer,
                            uint32_t destBpp)
{
    if (!buffer)
        return;
    if (!destBuffer)
        return;

    switch (destBpp)
    {
    case 15:
        softsurface_blitBufferInternal<uint16_t>((uint16_t*) destBuffer);
        break;
    case 16:
        softsurface_blitBufferInternal<uint16_t>((uint16_t*) destBuffer);
        break;
    case 24:
        softsurface_blitBufferInternal<uint32_t>(destBuffer);
        break;
    case 32:
        softsurface_blitBufferInternal<uint32_t>(destBuffer);
        break;
    default:
        return;
    }
}
