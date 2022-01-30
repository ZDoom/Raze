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

#include "ns.h"	// Must come before everything else!

#include "build.h"
#include "blood.h"
#include "raze_sound.h"

BEGIN_BLD_NS

enum {kMaxAmbChannel = 64 };

struct AMB_CHANNEL
{
    FSoundID soundID;
    int distance;
    int check;
};

AMB_CHANNEL ambChannels[kMaxAmbChannel];
int nAmbChannels = 0;

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ambProcess(void)
{
    if (!SoundEnabled())
        return;
    BloodStatIterator it(kStatAmbience);
    while (DBloodActor* actor = it.Next())
    {
        if (actor->spr.intowner < 0 || actor->spr.intowner >= kMaxAmbChannel)
            continue;
        if (actor->hasX())
        {
            if (actor->xspr.state)
            {
                int dx = actor->int_pos().X-gMe->actor->int_pos().X;
                int dy = actor->int_pos().Y-gMe->actor->int_pos().Y;
                int dz = actor->int_pos().Z-gMe->actor->int_pos().Z;
                dx >>= 4;
                dy >>= 4;
                dz >>= 8;
                int nDist = ksqrt(dx*dx+dy*dy+dz*dz);
                int vs = MulScale(actor->xspr.data4, actor->xspr.busy, 16);
                ambChannels[actor->spr.intowner].distance += ClipRange(scale(nDist, actor->xspr.data1, actor->xspr.data2, vs, 0), 0, vs);
            }
        }
    }
    AMB_CHANNEL *pChannel = ambChannels;
    for (int i = 0; i < nAmbChannels; i++, pChannel++)
    {
        if (soundEngine->IsSourcePlayingSomething(SOURCE_Ambient, pChannel, CHAN_BODY, -1))
        {
            if (pChannel->distance > 0)
            {
                soundEngine->ChangeSoundVolume(SOURCE_Ambient, pChannel, CHAN_BODY, pChannel->distance / 255.f);
            }
            else
            {
                // Stop the sound if it cannot be heard so that it doesn't occupy a physical channel.
                soundEngine->StopSound(SOURCE_Ambient, pChannel, CHAN_BODY);
            }
        }
        else if (pChannel->distance > 0)
        {
            FVector3 pt{};
            soundEngine->StartSound(SOURCE_Ambient, pChannel, &pt, CHAN_BODY, CHANF_LOOP|CHANF_TRANSIENT, pChannel->soundID, pChannel->distance / 255.f, ATTN_NONE);
        }
        pChannel->distance = 0;
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ambKillAll(void)
{
    AMB_CHANNEL *pChannel = ambChannels;
    for (int i = 0; i < nAmbChannels; i++, pChannel++)
    {
        soundEngine->StopSound(SOURCE_Ambient, pChannel, CHAN_BODY);
        pChannel->soundID = 0;
    }
    nAmbChannels = 0;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void ambInit(void)
{
    ambKillAll();
    memset(ambChannels, 0, sizeof(ambChannels));
    BloodStatIterator it(kStatAmbience);
    while (DBloodActor* actor = it.Next())
    {
        if (!actor->hasX()) continue;

        if (actor->xspr.data1 >= actor->xspr.data2) continue;

        int i; AMB_CHANNEL *pChannel = ambChannels;
        for (i = 0; i < nAmbChannels; i++, pChannel++)
            if (actor->xspr.data3 == pChannel->check) break;

        if (i == nAmbChannels) {

            if (i >= kMaxAmbChannel) {
                actor->spr.intowner = -1;
                continue;
            }

            int nSFX = actor->xspr.data3;
            auto snd = soundEngine->FindSoundByResID(nSFX);
            if (!snd) {
                //I_Error("Missing sound #%d used in ambient sound generator %d\n", nSFX);
                viewSetSystemMessage("Missing sound #%d used in ambient sound generator #%d\n", nSFX, actor->GetIndex());
                actPostSprite(actor, kStatDecoration);
                continue;
            }

            pChannel->soundID = FSoundID(snd);
            pChannel->check = nSFX;
            pChannel->distance = 0;
            nAmbChannels++;

        }

        actor->spr.intowner = i;
    }
}

END_BLD_NS
