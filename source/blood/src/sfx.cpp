//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------
#include <string.h>
#include "build.h"
#include "compat.h"
#include "common_game.h"
#include "fx_man.h"

#include "config.h"
#include "gameutil.h"
#include "player.h"
#include "resource.h"
#include "sfx.h"
#include "sound.h"
#include "trig.h"

POINT2D earL, earR, earL0, earR0; // Ear position
VECTOR2D earVL, earVR; // Ear velocity ?
int lPhase, rPhase, lVol, rVol, lPitch, rPitch;

struct BONKLE
{
    int at0;
    int at4;
    DICTNODE *at8;
    int atc;
    spritetype *at10;
    int at14;
    int at18;
    int at1c;
    POINT3D at20;
    POINT3D at2c;
    //int at20;
    //int at24;
    //int at28;
    //int at2c;
    //int at30;
    //int at34;
    int at38;
    int at3c;
};

BONKLE Bonkle[256];
BONKLE *BonkleCache[256];

int nBonkles;

void sfxInit(void)
{
    for (int i = 0; i < 256; i++)
        BonkleCache[i] = &Bonkle[i];
    nBonkles = 0;
}

void sfxTerm()
{
}

int Vol3d(int angle, int dist)
{
    return dist - mulscale16(dist, 0x2000 - mulscale30(0x2000, Cos(angle)));
}

void Calc3DValues(BONKLE *pBonkle)
{
    int dx = pBonkle->at20.x - gMe->pSprite->x;
    int dy = pBonkle->at20.y - gMe->pSprite->y;
    int dz = pBonkle->at20.z - gMe->pSprite->z;
    int angle = getangle(dx, dy);
    dx >>= 4;
    dy >>= 4;
    dz >>= 8;
    int distance = ksqrt(dx*dx + dy * dy + dz * dz);
    distance = ClipLow((distance >> 2) + (distance >> 3), 64);
    int v14, v18;
    v14 = v18 = scale(pBonkle->at1c, 80, distance);
    int sinVal = Sin(angle);
    int cosVal = Cos(angle);
    int v8 = dmulscale30r(cosVal, pBonkle->at20.x - pBonkle->at2c.x, sinVal, pBonkle->at20.y - pBonkle->at2c.y);

    int distanceL = approxDist(pBonkle->at20.x - earL.x, pBonkle->at20.y - earL.y);
    lVol = Vol3d(angle - (gMe->pSprite->ang - 85), v18);
    int phaseLeft = mulscale16r(distanceL, pBonkle->at3c == 1 ? 4114 : 8228);
    lPitch = scale(pBonkle->at18, dmulscale30r(cosVal, earVL.dx, sinVal, earVL.dy) + 5853, v8 + 5853);

    int distanceR = approxDist(pBonkle->at20.x - earR.x, pBonkle->at20.y - earR.y);
    rVol = Vol3d(angle - (gMe->pSprite->ang + 85), v14);
    int phaseRight = mulscale16r(distanceR, pBonkle->at3c == 1 ? 4114 : 8228);
    rPitch = scale(pBonkle->at18, dmulscale30r(cosVal, earVR.dx, sinVal, earVR.dy) + 5853, v8 + 5853);

    int phaseMin = ClipHigh(phaseLeft, phaseRight);
    lPhase = phaseRight - phaseMin;
    rPhase = phaseLeft - phaseMin;
}

void sfxPlay3DSound(int x, int y, int z, int soundId, int nSector)
{
    if (!SoundToggle)
        return;
    if (soundId < 0)
        ThrowError("Invalid sound ID");
    DICTNODE *hRes = gSoundRes.Lookup(soundId, "SFX");
    if (!hRes)
        return;

    SFX *pEffect = (SFX*)gSoundRes.Load(hRes);
    hRes = gSoundRes.Lookup(pEffect->rawName, "RAW");
    if (!hRes)
        return;
    int v1c, v18;
    v1c = v18 = mulscale16(pEffect->pitch, sndGetRate(pEffect->format));
    if (nBonkles >= 256)
        return;
    BONKLE *pBonkle = BonkleCache[nBonkles++];
    pBonkle->at10 = NULL;
    pBonkle->at20.x = x;
    pBonkle->at20.y = y;
    pBonkle->at20.z = z;
    pBonkle->at38 = nSector;
    FindSector(x, y, z, &pBonkle->at38);
    pBonkle->at2c = pBonkle->at20;
    pBonkle->atc = soundId;
    pBonkle->at8 = hRes;
    pBonkle->at1c = pEffect->relVol;
    pBonkle->at18 = v18;
    pBonkle->at3c = pEffect->format;
    int size = hRes->size;
    char *pData = (char*)gSoundRes.Lock(hRes);
    Calc3DValues(pBonkle);
    int priority = 1;
    if (priority < lVol)
        priority = lVol;
    if (priority < rVol)
        priority = rVol;
    if (gDoppler)
    {
        DisableInterrupts();
        pBonkle->at0 = FX_PlayRaw(pData + lPhase, size - lPhase, lPitch, 0, lVol, lVol, 0, priority, 1.f, (intptr_t)&pBonkle->at0);
        pBonkle->at4 = FX_PlayRaw(pData + rPhase, size - rPhase, rPitch, 0, rVol, 0, rVol, priority, 1.f, (intptr_t)&pBonkle->at4);
        RestoreInterrupts();
    }
    else
    {
        pBonkle->at0 = FX_PlayRaw(pData + lPhase, size - lPhase, v1c, 0, lVol, lVol, rVol, priority, 1.f, (intptr_t)&pBonkle->at0);
        pBonkle->at4 = 0;
    }
}

void sfxPlay3DSound(spritetype *pSprite, int soundId, int a3, int a4)
{
    if (!SoundToggle)
        return;
    if (!pSprite)
        return;
    if (soundId < 0)
        return;
    DICTNODE *hRes = gSoundRes.Lookup(soundId, "SFX");
    if (!hRes)
        return;

    SFX *pEffect = (SFX*)gSoundRes.Load(hRes);
    hRes = gSoundRes.Lookup(pEffect->rawName, "RAW");
    if (!hRes)
        return;
    int size = hRes->size;
    if (size <= 0)
        return;
    int v14;
    v14 = mulscale16(pEffect->pitch, sndGetRate(pEffect->format));
    BONKLE *pBonkle = NULL;
    if (a3 >= 0)
    {
        int i;
        for (i = 0; i < nBonkles; i++)
        {
            pBonkle = BonkleCache[i];
            if (pBonkle->at14 == a3 && (pBonkle->at10 == pSprite || (a4 & 1) != 0))
            {
                if ((a4 & 4) != 0 && pBonkle->at14 == a3)
                    return;
                if ((a4 & 2) != 0 && pBonkle->atc == soundId)
                    return;
                if (pBonkle->at0 > 0)
                    FX_StopSound(pBonkle->at0);
                if (pBonkle->at4 > 0)
                    FX_StopSound(pBonkle->at4);
                if (pBonkle->at8)
                {
                    gSoundRes.Unlock(pBonkle->at8);
                    pBonkle->at8 = NULL;
                }
                break;
            }
        }
        if (i == nBonkles)
        {
            if (nBonkles >= 256)
                return;
            pBonkle = BonkleCache[nBonkles++];
        }
        pBonkle->at10 = pSprite;
        pBonkle->at14 = a3;
    }
    else
    {
        if (nBonkles >= 256)
            return;
        pBonkle = BonkleCache[nBonkles++];
        pBonkle->at10 = NULL;
    }
    pBonkle->at20.x = pSprite->x;
    pBonkle->at20.y = pSprite->y;
    pBonkle->at20.z = pSprite->z;
    pBonkle->at38 = pSprite->sectnum;
    pBonkle->at2c = pBonkle->at20;
    pBonkle->atc = soundId;
    pBonkle->at8 = hRes;
    pBonkle->at1c = pEffect->relVol;
    pBonkle->at18 = v14;
    Calc3DValues(pBonkle);
    int priority = 1;
    if (priority < lVol)
        priority = lVol;
    if (priority < rVol)
        priority = rVol;
    int loopStart = pEffect->loopStart;
    int loopEnd = ClipLow(size - 1, 0);
    if (a3 < 0)
        loopStart = -1;
    DisableInterrupts();
    char *pData = (char*)gSoundRes.Lock(hRes);
    if (loopStart >= 0)
    {
        if (gDoppler)
        {
            pBonkle->at0 = FX_PlayLoopedRaw(pData + lPhase, size - lPhase, pData + loopStart, pData + loopEnd, lPitch, 0, lVol, lVol, 0, priority, 1.f, (intptr_t)&pBonkle->at0);
            pBonkle->at4 = FX_PlayLoopedRaw(pData + rPhase, size - rPhase, pData + loopStart, pData + loopEnd, rPitch, 0, rVol, 0, rVol, priority, 1.f, (intptr_t)&pBonkle->at4);
        }
        else
        {
            pBonkle->at0 = FX_PlayLoopedRaw(pData + lPhase, size - lPhase, pData + loopStart, pData + loopEnd, v14, 0, lVol, lVol, rVol, priority, 1.f, (intptr_t)&pBonkle->at0);
            pBonkle->at4 = 0;
        }
    }
    else
    {
        pData = (char*)gSoundRes.Lock(pBonkle->at8);
        if (gDoppler)
        {
            pBonkle->at0 = FX_PlayRaw(pData + lPhase, size - lPhase, lPitch, 0, lVol, lVol, 0, priority, 1.f, (intptr_t)&pBonkle->at0);
            pBonkle->at4 = FX_PlayRaw(pData + rPhase, size - rPhase, rPitch, 0, rVol, 0, rVol, priority, 1.f, (intptr_t)&pBonkle->at4);
        }
        else
        {
            pBonkle->at0 = FX_PlayRaw(pData + lPhase, size - lPhase, v14, 0, lVol, lVol, rVol, priority, 1.f, (intptr_t)&pBonkle->at0);
            pBonkle->at4 = 0;
        }
    }
    RestoreInterrupts();
}

// By NoOne: same as previous, but allows to set custom pitch for sound AND volume. Used by SFX gen now.
void sfxPlay3DSoundCP(spritetype* pSprite, int soundId, int a3, int a4, int pitch, int volume)
{
    if (!SoundToggle || !pSprite || soundId < 0) return;
    DICTNODE* hRes = gSoundRes.Lookup(soundId, "SFX");
    if (!hRes) return;

    SFX* pEffect = (SFX*)gSoundRes.Load(hRes);
    hRes = gSoundRes.Lookup(pEffect->rawName, "RAW");
    if (!hRes) return;
    int size = hRes->size;
    if (size <= 0) return;
    
    if (pitch <= 0) pitch = pEffect->pitch;
    else pitch -= Random(pEffect->pitchRange);

    int v14;
    v14 = mulscale16(pitch, sndGetRate(pEffect->format));
    
    BONKLE * pBonkle = NULL;
    if (a3 >= 0)
    {
        int i;
        for (i = 0; i < nBonkles; i++)
        {
            pBonkle = BonkleCache[i];
            if (pBonkle->at14 == a3 && (pBonkle->at10 == pSprite || (a4 & 1) != 0))
            {
                if ((a4 & 4) != 0 && pBonkle->at14 == a3)
                    return;
                if ((a4 & 2) != 0 && pBonkle->atc == soundId)
                    return;
                if (pBonkle->at0 > 0)
                    FX_StopSound(pBonkle->at0);
                if (pBonkle->at4 > 0)
                    FX_StopSound(pBonkle->at4);
                if (pBonkle->at8)
                {
                    gSoundRes.Unlock(pBonkle->at8);
                    pBonkle->at8 = NULL;
                }
                break;
            }
        }
        if (i == nBonkles)
        {
            if (nBonkles >= 256)
                return;
            pBonkle = BonkleCache[nBonkles++];
        }
        pBonkle->at10 = pSprite;
        pBonkle->at14 = a3;
    }
    else
    {
        if (nBonkles >= 256)
            return;
        pBonkle = BonkleCache[nBonkles++];
        pBonkle->at10 = NULL;
    }
    pBonkle->at20.x = pSprite->x;
    pBonkle->at20.y = pSprite->y;
    pBonkle->at20.z = pSprite->z;
    pBonkle->at38 = pSprite->sectnum;
    pBonkle->at2c = pBonkle->at20;
    pBonkle->atc = soundId;
    pBonkle->at8 = hRes;
    pBonkle->at1c = (volume <= 0) ? pEffect->relVol : volume;
    pBonkle->at18 = v14;
    Calc3DValues(pBonkle);
    int priority = 1;
    if (priority < lVol)
        priority = lVol;
    if (priority < rVol)
        priority = rVol;
    int loopStart = pEffect->loopStart;
    int loopEnd = ClipLow(size - 1, 0);
    if (a3 < 0)
        loopStart = -1;
    DisableInterrupts();
    char* pData = (char*)gSoundRes.Lock(hRes);
    if (loopStart >= 0)
    {
        if (gDoppler)
        {
            pBonkle->at0 = FX_PlayLoopedRaw(pData + lPhase, size - lPhase, pData + loopStart, pData + loopEnd, lPitch, 0, lVol, lVol, 0, priority, 1.f, (intptr_t)& pBonkle->at0);
            pBonkle->at4 = FX_PlayLoopedRaw(pData + rPhase, size - rPhase, pData + loopStart, pData + loopEnd, rPitch, 0, rVol, 0, rVol, priority, 1.f, (intptr_t)& pBonkle->at4);
        }
        else
        {
            pBonkle->at0 = FX_PlayLoopedRaw(pData + lPhase, size - lPhase, pData + loopStart, pData + loopEnd, v14, 0, lVol, lVol, rVol, priority, 1.f, (intptr_t)& pBonkle->at0);
            pBonkle->at4 = 0;
        }
    }
    else
    {
        pData = (char*)gSoundRes.Lock(pBonkle->at8);
        if (gDoppler)
        {
            pBonkle->at0 = FX_PlayRaw(pData + lPhase, size - lPhase, lPitch, 0, lVol, lVol, 0, priority, 1.f, (intptr_t)& pBonkle->at0);
            pBonkle->at4 = FX_PlayRaw(pData + rPhase, size - rPhase, rPitch, 0, rVol, 0, rVol, priority, 1.f, (intptr_t)& pBonkle->at4);
        }
        else
        {
            pBonkle->at0 = FX_PlayRaw(pData + lPhase, size - lPhase, v14, 0, lVol, lVol, rVol, priority, 1.f, (intptr_t)& pBonkle->at0);
            pBonkle->at4 = 0;
        }
    }
    RestoreInterrupts();
}


void sfxKill3DSound(spritetype *pSprite, int a2, int a3)
{
    if (!pSprite)
        return;
    for (int i = nBonkles - 1; i >= 0; i--)
    {
        BONKLE *pBonkle = BonkleCache[i];
        if (pBonkle->at10 == pSprite && (a2 < 0 || a2 == pBonkle->at14) && (a3 < 0 || a3 == pBonkle->atc))
        {
            if (pBonkle->at0 > 0)
            {
                FX_EndLooping(pBonkle->at0);
                FX_StopSound(pBonkle->at0);
            }
            if (pBonkle->at4 > 0)
            {
                FX_EndLooping(pBonkle->at4);
                FX_StopSound(pBonkle->at4);
            }
            if (pBonkle->at8)
            {
                gSoundRes.Unlock(pBonkle->at8);
                pBonkle->at8 = NULL;
            }
            BonkleCache[i] = BonkleCache[--nBonkles];
            BonkleCache[nBonkles] = pBonkle;
            break;
        }
    }
}

void sfxKillAllSounds(void)
{
    for (int i = nBonkles - 1; i >= 0; i--)
    {
        BONKLE *pBonkle = BonkleCache[i];
        if (pBonkle->at0 > 0)
        {
            FX_EndLooping(pBonkle->at0);
            FX_StopSound(pBonkle->at0);
        }
        if (pBonkle->at4 > 0)
        {
            FX_EndLooping(pBonkle->at4);
            FX_StopSound(pBonkle->at4);
        }
        if (pBonkle->at8)
        {
            gSoundRes.Unlock(pBonkle->at8);
            pBonkle->at8 = NULL;
        }
        BonkleCache[i] = BonkleCache[--nBonkles];
        BonkleCache[nBonkles] = pBonkle;
    }
}

void sfxUpdate3DSounds(void)
{
    int dx = mulscale30(Cos(gMe->pSprite->ang + 512), 43);
    earL0 = earL;
    int dy = mulscale30(Sin(gMe->pSprite->ang + 512), 43);
    earR0 = earR;
    earL.x = gMe->pSprite->x - dx;
    earL.y = gMe->pSprite->y - dy;
    earR.x = gMe->pSprite->x + dx;
    earR.y = gMe->pSprite->y + dy;
    earVL.dx = earL.x - earL0.x;
    earVL.dy = earL.y - earL0.y;
    earVR.dx = earR.x - earR0.x;
    earVR.dy = earR.y - earR0.y;
    for (int i = nBonkles - 1; i >= 0; i--)
    {
        BONKLE *pBonkle = BonkleCache[i];
        if (pBonkle->at0 > 0 || pBonkle->at4 > 0)
        {
            if (!pBonkle->at8)
                continue;
            if (pBonkle->at10)
            {
                pBonkle->at2c = pBonkle->at20;
                pBonkle->at20.x = pBonkle->at10->x;
                pBonkle->at20.y = pBonkle->at10->y;
                pBonkle->at20.z = pBonkle->at10->z;
                pBonkle->at38 = pBonkle->at10->sectnum;
            }
            Calc3DValues(pBonkle);
            DisableInterrupts();
            if (pBonkle->at0 > 0)
            {
                if (pBonkle->at4 > 0)
                {
                    FX_SetPan(pBonkle->at0, lVol, lVol, 0);
                    FX_SetFrequency(pBonkle->at0, lPitch);
                }
                else
                    FX_SetPan(pBonkle->at0, lVol, lVol, rVol);
            }
            if (pBonkle->at4 > 0)
            {
                FX_SetPan(pBonkle->at4, rVol, 0, rVol);
                FX_SetFrequency(pBonkle->at4, rPitch);
            }
            RestoreInterrupts();
        }
        else
        {
            gSoundRes.Unlock(pBonkle->at8);
            pBonkle->at8 = NULL;
            BonkleCache[i] = BonkleCache[--nBonkles];
            BonkleCache[nBonkles] = pBonkle;
        }
    }
}

void sfxSetReverb(bool toggle)
{
    if (toggle)
    {
        FX_SetReverb(128);
        FX_SetReverbDelay(10);
    }
    else
        FX_SetReverb(0);
}

void sfxSetReverb2(bool toggle)
{
    if (toggle)
    {
        FX_SetReverb(128);
        FX_SetReverbDelay(20);
    }
    else
        FX_SetReverb(0);
}
