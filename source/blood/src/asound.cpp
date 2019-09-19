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
#include "build.h"
#include "fx_man.h"
#include "common_game.h"
#include "blood.h"
#include "config.h"
#include "db.h"
#include "player.h"
#include "resource.h"
#include "sound.h"

#define kMaxAmbChannel 64

struct AMB_CHANNEL
{
    int at0;
    int at4;
    int at8;
    DICTNODE *atc;
    char *at10;
    int at14;
    int at18;
};

AMB_CHANNEL ambChannels[kMaxAmbChannel];
int nAmbChannels = 0;

void ambProcess(void)
{
    if (!SoundToggle)
        return;
    for (int nSprite = headspritestat[12]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];
        if (pSprite->owner < 0 || pSprite->owner >= kMaxAmbChannel)
            continue;
        int nXSprite = pSprite->extra;
        if (nXSprite > 0 && nXSprite < kMaxXSprites)
        {
            XSPRITE *pXSprite = &xsprite[nXSprite];
            if (pXSprite->state)
            {
                int dx = pSprite->x-gMe->pSprite->x;
                int dy = pSprite->y-gMe->pSprite->y;
                int dz = pSprite->z-gMe->pSprite->z;
                dx >>= 4;
                dy >>= 4;
                dz >>= 8;
                int nDist = ksqrt(dx*dx+dy*dy+dz*dz);
                int vs = mulscale16(pXSprite->data4, pXSprite->busy);
                ambChannels[pSprite->owner].at4 += ClipRange(scale(nDist, pXSprite->data1, pXSprite->data2, vs, 0), 0, vs);
            }
        }
    }
    AMB_CHANNEL *pChannel = ambChannels;
    for (int i = 0; i < nAmbChannels; i++, pChannel++)
    {
        if (pChannel->at0 > 0)
            FX_SetPan(pChannel->at0, pChannel->at4, pChannel->at4, pChannel->at4);
        else
        {
            int end = ClipLow(pChannel->at14-1, 0);
            pChannel->at0 = FX_PlayLoopedRaw(pChannel->at10, pChannel->at14, pChannel->at10, pChannel->at10+end, sndGetRate(pChannel->at18), 0,
                pChannel->at4, pChannel->at4, pChannel->at4, pChannel->at4, 1.f, (intptr_t)&pChannel->at0);
        }
        pChannel->at4 = 0;
    }
}

void ambKillAll(void)
{
    AMB_CHANNEL *pChannel = ambChannels;
    for (int i = 0; i < nAmbChannels; i++, pChannel++)
    {
        if (pChannel->at0 > 0)
        {
            FX_EndLooping(pChannel->at0);
            FX_StopSound(pChannel->at0);
        }
        if (pChannel->atc)
        {
            gSoundRes.Unlock(pChannel->atc);
            pChannel->atc = NULL;
        }
    }
    nAmbChannels = 0;
}

void ambInit(void)
{
    ambKillAll();
    memset(ambChannels, 0, sizeof(ambChannels));
    for (int nSprite = headspritestat[12]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];
        int nXSprite = pSprite->extra;
        if (nXSprite > 0 && nXSprite < kMaxXSprites)
        {
            XSPRITE *pXSprite = &xsprite[nXSprite];
            if (pXSprite->data1 < pXSprite->data2)
            {
                int i;
                AMB_CHANNEL *pChannel = ambChannels;
                for (i = 0; i < nAmbChannels; i++, pChannel++)
                    if (pXSprite->data3 == pChannel->at8)
                        break;
                if (i == nAmbChannels)
                {
                    if (i >= kMaxAmbChannel)
                    {
                        pSprite->owner = -1;
                        continue;
                    }
                    int nSFX = pXSprite->data3;
                    DICTNODE *pSFXNode = gSoundRes.Lookup(nSFX, "SFX");
                    if (!pSFXNode)
                        ThrowError("Missing sound #%d used in ambient sound generator %d\n", nSFX);
                    SFX *pSFX = (SFX*)gSoundRes.Load(pSFXNode);
                    DICTNODE *pRAWNode = gSoundRes.Lookup(pSFX->rawName, "RAW");
                    if (!pRAWNode)
                        ThrowError("Missing RAW sound \"%s\" used in ambient sound generator %d\n", pSFX->rawName, nSFX);
                    if (pRAWNode->size > 0)
                    {
                        pChannel->at14 = pRAWNode->size;
                        pChannel->at8 = nSFX;
                        pChannel->atc = pRAWNode;
                        pChannel->at14 = pRAWNode->size;
                        pChannel->at10 = (char*)gSoundRes.Lock(pRAWNode);
                        pChannel->at18 = pSFX->format;
                        nAmbChannels++;
                    }
                }
                pSprite->owner = i;
            }
        }
    }
}
