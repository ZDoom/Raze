//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
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
#include "ns.h"
#include "gamehud.h"
#include "sequence.h"
#include "engine.h"
#include "exhumed.h"
#include "sound.h"
#include "player.h"
#include "aistuff.h"
#include "view.h"
#include <string.h>
#include <stdio.h>

// TEMP
#include <assert.h>

BEGIN_PS_NS

enum
{
	kMaxSequences	= 4096,
	kMaxSEQFiles	= 78,
	kMaxSEQFrames	= 18000,
	kMaxSEQChunks	= 21000
};

short sequences = 0;
short frames = 0;
short chunks = 0;
short nPilotLightFrame;
short nPilotLightCount;

short nPilotLightBase;

short nShadowWidth = 1;
short nFlameHeight = 1;

short SeqBase[kMaxSequences];
short SeqSize[kMaxSequences];
short SeqFlag[kMaxSequences];

short FrameSound[kMaxSEQFrames];
short FrameSize[kMaxSEQFrames];
short FrameBase[kMaxSEQFrames];
short FrameFlag[kMaxSEQFrames];

short ChunkYpos[kMaxSEQChunks];
short ChunkXpos[kMaxSEQChunks];
short ChunkPict[kMaxSEQChunks];
short ChunkFlag[kMaxSEQChunks];


const char *SeqNames[kMaxSEQFiles] =
{
  "rothands",
  "sword",
  "pistol",
  "m_60",
  "flamer", // 4
  "grenade",
  "cobra",
  "bonesaw",
  "scramble",
  "glove",
  "mummy", // 10
  "skull",
  "poof",
  "kapow",
  "fireball",
  "bubble",
  "spider", // 16
  "anubis",
  "anuball",
  "fish",
  "snakehed", // 20?
  "snakbody",
  "wasp",
  "cobrapow",
  "scorp",
  "joe", // 25
  "status",
  "dead",
  "deadex",
  "anupoof",
  "skulpoof", // 30
  "bullet",
  "shadow",
  "grenroll",
  "grenboom",
  "splash",
  "grenpow",
  "skulstrt",
  "firepoof",
  "bloodhit",
  "lion", // 40
  "items",
  "lavag", // 42
  "lsplash",
  "lavashot",
  "smokebal",
  "firepot",
  "rex",
  "set", // 48
  "queen",
  "roach", // 50
  "hawk",
  "setghost",
  "setgblow",
  "bizztail",
  "bizzpoof",
  "queenegg",
  "roacshot",
  "backgrnd",
  "screens", // 59
  "arrow",
  "fonts",
  "drips",
  "firetrap",
  "magic2",
  "creepy",
  "slider", // 66
  "ravolt",
  "eyehit",
  "font2", // 69
  "seebubbl",
  "blood",
  "drum",
  "poof2",
  "deadbrn",
  "grenbubb",
  "rochfire",
  "rat"
};

short SeqOffsets[kMaxSEQFiles];


int seq_ReadSequence(const char *seqName)
{
    int i;
    char buffer[200];
    buffer[0] = '\0';

    strcat(buffer, seqName);
    strcat(buffer, ".seq");

    auto hFile = fileSystem.ReopenFileReader(fileSystem.FindFile(buffer), true);
    if (!hFile.isOpen())
    {
        Printf("Unable to open '%s'!\n", buffer);
        return 0;
    }

    unsigned short tag;
    hFile.Read(&tag, sizeof(tag));
    if (tag < MAKE_ID('I', 'H', 0, 0) || (tag > MAKE_ID('I', 'H', 0, 0) && tag != MAKE_ID('D', 'S', 0, 0)))
    {
        Printf("Unsupported sequence version!\n");
        return 0;
    }

    short centerx, centery; // TODO - are global vars?
    short nSeqs;
    hFile.Read(&centerx, sizeof(centerx));
    hFile.Read(&centery, sizeof(centery));
    hFile.Read(&nSeqs, sizeof(nSeqs));

    if (nSeqs <= 0 || sequences + nSeqs >= kMaxSequences)
    {
        if (nSeqs < 0)
        {
            Printf("Invalid sequence count!\n");
            return 0;
        }
        else {
            I_Error("Not enough sequences available!  Increase array!\n");
        }
    }

    hFile.Read(&SeqBase[sequences], nSeqs * sizeof(SeqBase[0]));
    hFile.Read(&SeqSize[sequences], nSeqs * sizeof(SeqSize[0]));
    hFile.Read(&SeqFlag[sequences], nSeqs * sizeof(SeqFlag[0]));

    for (i = 0; i < nSeqs; i++)
    {
        SeqBase[sequences + i] += frames;
    }

    short vdi = frames;

    int16_t nFrames;
    hFile.Read(&nFrames, sizeof(nFrames));

    if (nFrames <= 0 || frames + nFrames >= kMaxSEQFrames)
    {
        if (nFrames < 0 )
        {
            Printf("Invalid frame count!\n");
            return 0;
        }
        else {
            I_Error("Not enough frames available!  Increase FRAMEMAX!\n");
        }
    }

    hFile.Read(&FrameBase[frames], nFrames * sizeof(FrameBase[0]));
    hFile.Read(&FrameSize[frames], nFrames * sizeof(FrameSize[0]));
    hFile.Read(&FrameFlag[frames], nFrames * sizeof(FrameFlag[0]));
    memset(&FrameSound[frames], -1,  nFrames * sizeof(FrameSound[0]));

    for (i = 0; i < nFrames; i++)
    {
        FrameBase[frames + i] += chunks;
    }

    int16_t nChunks;
    hFile.Read(&nChunks, sizeof(nChunks));

    if (nChunks < 0 || chunks + nChunks >= kMaxSEQChunks)
    {
        if (nChunks < 0 )
        {
            Printf("Invalid chunk count!\n");
            return 0;
        }
        else {
            I_Error("Not enough chunks available!  Increase CHUNKMAX!\n");
        }
    }

    hFile.Read(&ChunkXpos[chunks], nChunks * sizeof(ChunkXpos[0]));
    hFile.Read(&ChunkYpos[chunks], nChunks * sizeof(ChunkYpos[0]));
    hFile.Read(&ChunkPict[chunks], nChunks * sizeof(ChunkPict[0]));
    hFile.Read(&ChunkFlag[chunks], nChunks * sizeof(ChunkFlag[0]));

    for (i = 0; i < nChunks; i++)
    {
        ChunkXpos[chunks + i] -= centerx;
        ChunkYpos[chunks + i] -= centery;
    }

    sequences += nSeqs;
    FrameBase[frames + nFrames] = chunks + nChunks;
    frames += nFrames;
    SeqBase[sequences] = frames;
    chunks += nChunks;

    if (tag == MAKE_ID('D', 'S', 0, 0))
    {
        short var_20;
        hFile.Read(&var_20, sizeof(var_20));

        for (i = 0; i < var_20; i++)
        {
            hFile.Read(&buffer[i * 10], 8);
        }

        short var_24;
        hFile.Read(&var_24, sizeof(var_24));

        for (i = 0; i < var_24; i++)
        {
            short var_28, var_2C;
            hFile.Read(&var_28, sizeof(var_28));
            hFile.Read(&var_2C, sizeof(var_2C));

            int hSound = LoadSound(&buffer[(var_2C&0x1FF)*10]);

            assert(vdi + var_28 < 18000);
            FrameSound[vdi + var_28] = hSound | (var_2C & 0xFE00);
        }
    }

    return nSeqs;
}

int seq_GetFirstSeqPicnum(int nSeq)
{
    int i = SeqOffsets[nSeq];
    i = SeqBase[i];
    i = FrameBase[i];
    i = ChunkPict[i];

    return i;
}

void seq_LoadSequences()
{
    int i;

    for (i = 0; i < kMaxSEQFiles; i++)
    {
        SeqOffsets[i] = sequences;

        if (seq_ReadSequence(SeqNames[i]) == 0) {
            Printf("Error loading '%s'\n", SeqNames[i]);
        }
    }

#if 0
    FILE* f = fopen("seq.dump", "wb");

    fwrite(SeqBase, 1, sizeof(SeqBase), f);
    fwrite(SeqSize, 1, sizeof(SeqSize), f);
    fwrite(SeqFlag, 1, sizeof(SeqFlag), f);
    fwrite("++++++++++++++++", 1, 16, f);
    fwrite(FrameSound, 1, sizeof(FrameSound), f);
    fwrite("++++++++++++++++", 1, 16, f);
    fwrite(FrameSize, 1, sizeof(FrameSize), f);
    fwrite(FrameBase, 1, sizeof(FrameBase), f);
    fwrite(FrameFlag, 1, sizeof(FrameFlag), f);
    fwrite(ChunkYpos, 1, sizeof(ChunkYpos), f);
    fwrite(ChunkXpos, 1, sizeof(ChunkXpos), f);
    fwrite(ChunkPict, 1, sizeof(ChunkPict), f);
    fwrite(ChunkFlag, 1, sizeof(ChunkFlag), f);
    fclose(f);
#endif

    nShadowPic = seq_GetFirstSeqPicnum(kSeqShadow);
    nShadowWidth = tileWidth(nShadowPic);

    nFlameHeight = tileHeight(seq_GetFirstSeqPicnum(kSeqFirePoof));

    nBackgroundPic = seq_GetFirstSeqPicnum(kSeqBackgrnd);

    nPilotLightBase  = SeqBase[SeqOffsets[kSeqFlamer] + 3];
    nPilotLightCount = SeqSize[SeqOffsets[kSeqFlamer] + 3];
    nPilotLightFrame = 0;

    nFontFirstChar = seq_GetFirstSeqPicnum(kSeqFont2);

    short nSize = SeqSize[SeqOffsets[kSeqFont2]];

    for (i = 0; i < nSize; i++)
    {
        auto tex = tileGetTexture(nFontFirstChar + i);
        tex->SetOffsets(0, 0);
    }
}

short seq_GetFrameFlag(short val, short nFrame)
{
    return FrameFlag[SeqBase[val] + nFrame];
}

void seq_DrawPilotLightSeq(double xOffset, double yOffset)
{
    short nSect = PlayerList[nLocalPlayer].nPlayerViewSect;

    if (!(SectFlag[nSect] & kSectUnderwater))
    {
        short nFrame = nPilotLightBase + nPilotLightFrame;
        short nFrameBase = FrameBase[nFrame];
        short nFrameSize = FrameSize[nFrame];

        while (1)
        {
            nFrameSize--;
            if (nFrameSize < 0)
                return;

            short nTile = ChunkPict[nFrameBase];
            double x = ChunkXpos[nFrameBase] + (160 + xOffset);
            double y = ChunkYpos[nFrameBase] + (100 + yOffset);

            hud_drawsprite(x, y, 65536, fmod(-2 * PlayerList[nLocalPlayer].angle.ang.asbuildf(), kAngleMask + 1), nTile, 0, 0, 1);
            nFrameBase++;
        }
    }
}

/*
    6 parameters

    arg0 - shade?

*/

int seq_DrawGunSequence(int nSeqOffset, short dx, double xOffs, double yOffs, int nShade, int nPal)
{
    int nFrame = SeqBase[nSeqOffset] + dx;
    int nFrameBase = FrameBase[nFrame];
    int nFrameSize = FrameSize[nFrame];
    int frameFlag = FrameFlag[nFrame];

    while (1)
    {
        nFrameSize--;
        if (nFrameSize < 0)
            break;

        int x = ChunkXpos[nFrameBase] + 160;
        int y = ChunkYpos[nFrameBase] + 100;

        int stat = 0;
        if (ChunkFlag[nFrameBase] & 1)
            stat |= RS_XFLIPHUD;

        if (ChunkFlag[nFrameBase] & 2)
            stat |= RS_YFLIPHUD;

        short nTile = ChunkPict[nFrameBase];

        if (frameFlag & 4)
            nShade = -100;

        double alpha = 1;
        if (PlayerList[nLocalPlayer].nInvisible) {
            alpha = 0.3;
        }

        hud_drawsprite(x + xOffs, y + yOffs, 65536, 0, nTile, nShade, nPal, stat, alpha);
        nFrameBase++;
    }

    return frameFlag;
}

int seq_GetFrameSound(int val, int edx)
{
    return FrameSound[SeqBase[val] + edx];
}

void seq_MoveSequence(DExhumedActor* actor, short nSeq, short bx)
{
    assert(nSeq >= 0); // TEMP

    int nSound = FrameSound[SeqBase[nSeq] + bx];
    if (nSound == -1) {
        return;
    }

    if (actor) {
        D3PlayFX(nSound, actor);
    }
    else {
        PlayLocalSound(nSound, 0);
    }
}

int seq_GetSeqPicnum2(short nSeq, short nFrame)
{
    short nBase = FrameBase[SeqBase[nSeq] + nFrame];
    return ChunkPict[nBase];
}

int seq_GetSeqPicnum(short nSeq, short edx, short ebx)
{
    edx += SeqOffsets[nSeq];
    ebx += SeqBase[edx];
    short c = FrameBase[ebx];

    return ChunkPict[c];
}

int seq_PlotArrowSequence(short nSprite, short nSeq, int nVal)
{
    tspriteptr_t pTSprite = &mytsprite[nSprite];
    int nAngle = GetMyAngle(nCamerax - pTSprite->x, nCameray - pTSprite->y);

    int nSeqOffset = ((((pTSprite->ang + 512) - nAngle) + 128) & kAngleMask) >> 8;

    short nFrame = SeqBase[nSeqOffset + nSeq] + nVal;

    short nFrameBase = FrameBase[nFrame];
    short nFrameSize = FrameSize[nFrame];

    uint8_t nShade = pTSprite->shade;
    short nStat = pTSprite->cstat;

    nStat |= 0x80;

    if (nSeqOffset & 3) {
        nStat |= 0x18;
    }
    else {
        nStat &= 0x0E7;
    }

    if (FrameFlag[nFrame] & 4) {
        nShade -= 100;
    }

    pTSprite->cstat = nStat;
    pTSprite->shade = nShade;
    pTSprite->statnum = nFrameSize;

    if (ChunkFlag[nFrameBase] & 1)
    {
        pTSprite->xoffset = (int8_t)ChunkXpos[nFrameBase];
        pTSprite->cstat |= 4;
    }
    else
    {
        pTSprite->xoffset = (int8_t)-ChunkXpos[nFrameBase];
    }

    pTSprite->yoffset = -ChunkYpos[nFrameBase];
    pTSprite->picnum = ChunkPict[nFrameBase];

    return ChunkPict[nFrameBase];
}

int seq_PlotSequence(short nSprite, short edx, short nFrame, short ecx)
{
    tspriteptr_t pTSprite = &mytsprite[nSprite];
    int nAngle = GetMyAngle(nCamerax - pTSprite->x, nCameray - pTSprite->y);

    int val;

    if (ecx & 1)
    {
        val = 0;
    }
    else
    {
        val = (((pTSprite->ang - nAngle) + 128) & kAngleMask) >> 8;
    }

    int eax = SeqBase[edx] + nFrame;
    int edi = SeqBase[edx + val] + nFrame;

    short nBase = FrameBase[edi];
    short nSize = FrameSize[edi];

    int8_t shade = pTSprite->shade;

    if (FrameFlag[eax] & 4)
    {
        shade -= 100;
    }

    short nPict = ChunkPict[nBase];

    if (ecx & 0x100)
    {
        edx = -3;
    }
    else
    {
        edx = 100;
    }

    int esi = nSize + 1;
    esi += edx;

    int var_14 = edx + 1;
    short nOwner = pTSprite->owner;

    while (1)
    {
        esi--;
        nSize--;

        if (esi < var_14) {
            break;
        }

        tspriteptr_t tsp = &mytsprite[(*myspritesortcnt)++];
        tsp->x       = pTSprite->x;
        tsp->y       = pTSprite->y;
        tsp->z       = pTSprite->z;
        tsp->shade   = shade;
        tsp->pal     = pTSprite->pal;
        tsp->xrepeat = pTSprite->xrepeat;
        tsp->yrepeat = pTSprite->yrepeat;
        tsp->ang     = pTSprite->ang;
        tsp->owner   = pTSprite->owner;
        tsp->sectnum = pTSprite->sectnum;
        tsp->cstat   = pTSprite->cstat |= 0x80;
        tsp->statnum = esi;

        if (ChunkFlag[nBase] & 1)
        {
            tsp->xoffset = (int8_t)ChunkXpos[nBase];
            tsp->cstat |= 4; // x-flipped
        }
        else
        {
            tsp->xoffset = -ChunkXpos[nBase];
        }

        tsp->yoffset = -ChunkYpos[nBase];
        tsp->picnum = ChunkPict[nBase];

        nBase++;
    }

    if (!(pTSprite->cstat & 0x101) || (sprite[nOwner].statnum == 100 && nNetPlayerCount))
    {
        pTSprite->owner = -1;
    }
    else
    {
        int nSector =pTSprite->sectnum;
        int nFloorZ = sector[nSector].floorz;

        if (nFloorZ <= PlayerList[nLocalPlayer].eyelevel + initz) {
            pTSprite->owner = -1;
        }
        else
        {
            pTSprite->picnum = nShadowPic;

            int edx = ((tileWidth(nPict) << 5) / nShadowWidth) - ((nFloorZ - pTSprite->z) >> 10);
            if (edx < 1) {
                edx = 1;
            }

            pTSprite->cstat = 0x22; // transluscence, floor sprite
            pTSprite->z = nFloorZ;
            pTSprite->yrepeat = (uint8_t)edx;
            pTSprite->xrepeat = (uint8_t)edx;
            pTSprite->statnum = -3;
            pTSprite->pal = 0;
        }
    }

    return nPict;
}

void SerializeSequence(FSerializer& arc)
{
    if (arc.BeginObject("sequence"))
    {
        arc("pilotlightframe", nPilotLightFrame)
            ("pilotlightcount", nPilotLightCount)
            ("pilotlightbase", nPilotLightBase)
            ("shadowwidth", nShadowWidth)
            ("flameheight", nFlameHeight)
            .EndObject();
    }
}

END_PS_NS
