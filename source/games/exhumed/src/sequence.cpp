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

int16_t sequences = 0;
int16_t frames = 0;
int16_t chunks = 0;
int16_t nPilotLightFrame;
int16_t nPilotLightCount;

int16_t nPilotLightBase;

int16_t nShadowWidth = 1;
int16_t nFlameHeight = 1;

static int16_t SeqBase[kMaxSequences];
static int16_t SeqSize[kMaxSequences];
static int16_t SeqFlag[kMaxSequences]; // not used at all.

static int16_t FrameSound[kMaxSEQFrames];
static int16_t FrameSize[kMaxSEQFrames];
static int16_t FrameBase[kMaxSEQFrames];
static int16_t FrameFlag[kMaxSEQFrames];

static int16_t ChunkYpos[kMaxSEQChunks];
static int16_t ChunkXpos[kMaxSEQChunks];
static int16_t ChunkPict[kMaxSEQChunks];
static int16_t ChunkFlag[kMaxSEQChunks];


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

static int16_t SeqOffsets[kMaxSEQFiles];
static TMap<FName, SeqArray> FileSeqMap;


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int getSeqFromId(const int nSeqFileId, const int nSeq)
{
    return SeqOffsets[nSeqFileId] + nSeq;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int getSeqFrame(const int nSeq, const int nFrame)
{
    return SeqBase[nSeq] + nFrame;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int getSeqFrameCount(const int nSeq)
{
    return SeqSize[nSeq];
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int getSeqFrameChunk(const int nFrame)
{
    return FrameBase[nFrame];
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int getSeqFrameFlags(const int nFrame)
{
    return FrameFlag[nFrame];
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int getSeqFrameSound(const int nFrame)
{
    return FrameSound[nFrame];
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int getSeqFrameChunkCount(const int nFrame)
{
    return FrameSize[nFrame];
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int getSeqFrameChunkPosX(const int nChunk)
{
    return ChunkXpos[nChunk];
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int getSeqFrameChunkPosY(const int nChunk)
{
    return ChunkYpos[nChunk];
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int getSeqFrameChunkPicnum(const int nChunk)
{
    return ChunkPict[nChunk];
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int getSeqFrameChunkFlags(const int nChunk)
{
    return ChunkFlag[nChunk];
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

const SeqFrameArray& getSequence(const FName nSeqFile, const unsigned nSeqIndex)
{
    return FileSeqMap.CheckKey(nSeqFile)->operator[](nSeqIndex);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int addSeq(const char *seqName)
{
    const FStringf seqfilename("%s.seq", seqName);
    const auto hFile = fileSystem.ReopenFileReader(fileSystem.FindFile(seqfilename), true);

    if (!hFile.isOpen())
    {
        Printf("Unable to open '%s'!\n", seqfilename.GetChars());
        return 0;
    }

    uint16_t tag;
    hFile.Read(&tag, sizeof(tag));
    if (tag < MAKE_ID('I', 'H', 0, 0) || (tag > MAKE_ID('I', 'H', 0, 0) && tag != MAKE_ID('D', 'S', 0, 0)))
    {
        Printf("Unsupported sequence version!\n");
        return 0;
    }

    int16_t CenterX, CenterY, nSeqs;
    hFile.Read(&CenterX, sizeof(CenterX));
    hFile.Read(&CenterY, sizeof(CenterY));
    hFile.Read(&nSeqs, sizeof(nSeqs));

    if (nSeqs <= 0)
    {
        Printf("Invalid sequence count!\n");
        return 0;
    }

    TArray<int16_t> nSeqFrames(nSeqs, true);
    TArray<int16_t> nSeqFrameCount(nSeqs, true);
    TArray<int16_t> nSeqFlags(nSeqs, true);
    hFile.Read(nSeqFrames.Data(), nSeqs * sizeof(int16_t));
    hFile.Read(nSeqFrameCount.Data(), nSeqs * sizeof(int16_t));
    hFile.Read(nSeqFlags.Data(), nSeqs * sizeof(int16_t));

    int16_t nFrames;
    hFile.Read(&nFrames, sizeof(nFrames));

    if (nFrames <= 0 )
    {
        Printf("Invalid frame count!\n");
        return 0;
    }

    TArray<int16_t> nSeqFrameChunks(nFrames, true);
    TArray<int16_t> nSeqFrameChunkCount(nFrames, true);
    TArray<int16_t> nSeqFrameFlags(nFrames, true);
    TArray<int16_t> nSeqFrameSounds(nFrames, true);
    hFile.Read(nSeqFrameChunks.Data(), nFrames * sizeof(int16_t));
    hFile.Read(nSeqFrameChunkCount.Data(), nFrames * sizeof(int16_t));
    hFile.Read(nSeqFrameFlags.Data(), nFrames * sizeof(int16_t));
    memset(nSeqFrameSounds.Data(), -1, nFrames * sizeof(int16_t));

    int16_t nChunks;
    hFile.Read(&nChunks, sizeof(nChunks));

    if (nChunks <= 0)
    {
        Printf("Invalid chunk count!\n");
        return 0;
    }

    TArray<int16_t> nSeqFrameChunkPosX(nChunks, true);
    TArray<int16_t> nSeqFrameChunkPosY(nChunks, true);
    TArray<int16_t> nSeqFrameChunkPicnum(nChunks, true);
    TArray<int16_t> nSeqFrameChunkFlags(nChunks, true);
    hFile.Read(nSeqFrameChunkPosX.Data(), nChunks * sizeof(int16_t));
    hFile.Read(nSeqFrameChunkPosY.Data(), nChunks * sizeof(int16_t));
    hFile.Read(nSeqFrameChunkPicnum.Data(), nChunks * sizeof(int16_t));
    hFile.Read(nSeqFrameChunkFlags.Data(), nChunks * sizeof(int16_t));

    if (tag == MAKE_ID('D', 'S', 0, 0))
    {
        int16_t nSounds;
        hFile.Read(&nSounds, sizeof(nSounds));
        TArray<char> buffer(nSounds * 10, true);
        memset(buffer.Data(), 0, nSounds * 10);

        for (int i = 0; i < nSounds; i++)
        {
            hFile.Read(&buffer[i * 10], 8);
        }

        int16_t nSounds2;
        hFile.Read(&nSounds2, sizeof(nSounds2));

        for (int i = 0; i < nSounds2; i++)
        {
            int16_t nSeqFrame, nSndIndex;
            hFile.Read(&nSeqFrame, sizeof(nSeqFrame));
            hFile.Read(&nSndIndex, sizeof(nSndIndex));

            int ndx = (nSndIndex & 0x1FF);
            int hSound = 0;

            if (ndx >= nSounds)
            {
                Printf("Invalid sound index %d in %s, maximum is %d\n", ndx, seqfilename.GetChars(), nSounds);
            }
            else
            {
                hSound = LoadSound(&buffer[ndx * 10]);
            }

            nSeqFrameSounds[nSeqFrame] = hSound | (nSndIndex & 0xFE00);
        }
    }

    // Add hastable entry for the amount of sequences this file contains.
    auto& gSequences = FileSeqMap.Insert(FName(seqName), SeqArray(nSeqs, true));

    // Read all this data into something sane.
    for (int nSeq = 0; nSeq < nSeqs; nSeq++)
    {
        // Determine where we are in our frame array.
        const int firstFrame = nSeqFrames[nSeq];
        const int lastFrame = nSeqFrameCount[nSeq] + firstFrame;

        // Get sequence's frame array and resize.
        auto& gSeqFrames = gSequences[nSeq];
        gSeqFrames.Resize(lastFrame - firstFrame);

        // Build out frame array.
        for (int nFrame = firstFrame; nFrame < lastFrame; nFrame++)
        {
            // Store reference to this frame and start filling.
            auto& gSeqFrame = gSeqFrames[nFrame - firstFrame];
            gSeqFrame.sound = nSeqFrameSounds[nFrame];
            gSeqFrame.flags = nSeqFrameFlags[nFrame];

            // Determine where we are in our chunk array.
            const int firstChunk = nSeqFrameChunks[nFrame];
            const int lastChunk = nSeqFrameChunkCount[nFrame] + firstChunk;

            // Build out chunk array.
            for (int nChunk = firstChunk; nChunk < lastChunk; nChunk++)
            {
                gSeqFrame.chunks.Push({
                    (int16_t)(nSeqFrameChunkPosX[nChunk] - CenterX),
                    (int16_t)(nSeqFrameChunkPosY[nChunk] - CenterY),
                    nSeqFrameChunkPicnum[nChunk],
                    nSeqFrameChunkFlags[nChunk],
                });
            }
        }
    }

    return nSeqs;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int seq_ReadSequence(const char *seqName)
{
    const int16_t StartFrameCount = frames;
    const FStringf seqfilename("%s.seq", seqName);
    const auto hFile = fileSystem.ReopenFileReader(fileSystem.FindFile(seqfilename), true);

    if (!hFile.isOpen())
    {
        Printf("Unable to open '%s'!\n", seqfilename.GetChars());
        return 0;
    }

    uint16_t tag;
    hFile.Read(&tag, sizeof(tag));
    if (tag < MAKE_ID('I', 'H', 0, 0) || (tag > MAKE_ID('I', 'H', 0, 0) && tag != MAKE_ID('D', 'S', 0, 0)))
    {
        Printf("Unsupported sequence version!\n");
        return 0;
    }

    int16_t centerx, centery; // TODO - are global vars?
    int16_t nSeqs;
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

    for (int i = 0; i < nSeqs; i++)
    {
        SeqBase[sequences + i] += frames;
    }

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

    for (int i = 0; i < nFrames; i++)
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

    for (int i = 0; i < nChunks; i++)
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
        int16_t var_20;
        hFile.Read(&var_20, sizeof(var_20));
        TArray<char> buffer(var_20 * 10, true);
        memset(buffer.Data(), 0, var_20 * 10);

        for (int i = 0; i < var_20; i++)
        {
            hFile.Read(&buffer[i * 10], 8);
        }

        int16_t var_24;
        hFile.Read(&var_24, sizeof(var_24));

        for (int i = 0; i < var_24; i++)
        {
            int16_t var_28, var_2C;
            hFile.Read(&var_28, sizeof(var_28));
            hFile.Read(&var_2C, sizeof(var_2C));

            int ndx = (var_2C & 0x1FF);
            int hSound = 0;
            if (ndx >= var_20)
            {
                Printf("bad sound index %d in %s, maximum is %d\n", ndx, seqfilename.GetChars(), var_20);
            }
            else
                hSound = LoadSound(&buffer[ndx*10]);

            assert(StartFrameCount + var_28 < kMaxSEQFrames);
            FrameSound[StartFrameCount + var_28] = hSound | (var_2C & 0xFE00);
        }
    }

    return nSeqs;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int seq_GetFirstSeqPicnum(int nSeq)
{
    return getSeqFrameChunkPicnum(getSeqFrameChunk(getSeqFrame(getSeqFromId(nSeq))));
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void seq_LoadSequences()
{
    int i;

    for (i = 0; i < kMaxSEQFiles; i++)
    {
        SeqOffsets[i] = sequences;

        if (seq_ReadSequence(SeqNames[i]) == 0 || addSeq(SeqNames[i]) == 0)
        {
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

    nPilotLightBase  = getSeqFrame(getSeqFromId(kSeqFlamer, 3));
    nPilotLightCount = getSeqFrameCount(getSeqFromId(kSeqFlamer, 3));
    nPilotLightFrame = 0;

    nFontFirstChar = seq_GetFirstSeqPicnum(kSeqFont2);

    int16_t nSize = getSeqFrameCount(getSeqFromId(kSeqFont2));

    for (i = 0; i < nSize; i++)
    {
        auto tex = tileGetTexture(nFontFirstChar + i);
        tex->SetOffsets(0, 0);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void seq_DrawPilotLightSeq(double xOffset, double yOffset)
{
    auto pSect = PlayerList[nLocalPlayer].pPlayerViewSect;

    if (!(pSect->Flag & kSectUnderwater))
    {
        int16_t nFrame = nPilotLightBase + nPilotLightFrame;
        int16_t nFrameBase = getSeqFrameChunk(nFrame);
        int16_t nFrameSize = getSeqFrameChunkCount(nFrame);

        while (1)
        {
            nFrameSize--;
            if (nFrameSize < 0)
                return;

            int16_t nTile = getSeqFrameChunkPicnum(nFrameBase);
            double x = getSeqFrameChunkPosX(nFrameBase) + (160 + xOffset);
            double y = getSeqFrameChunkPosY(nFrameBase) + (100 + yOffset);

            hud_drawsprite(x, y, 65536, PlayerList[nLocalPlayer].pActor->spr.Angles.Yaw.Normalized180().Degrees() * 2., nTile, 0, 0, 1);
            nFrameBase++;
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int seq_DrawGunSequence(int nSeqOffset, int16_t dx, double xOffs, double yOffs, int nShade, int nPal, DAngle angle, bool align)
{
    int nFrame = getSeqFrame(nSeqOffset, dx);
    int nFrameBase = getSeqFrameChunk(nFrame);
    int nFrameSize = getSeqFrameChunkCount(nFrame);
    int frameFlag = getSeqFrameFlags(nFrame);

    while (1)
    {
        nFrameSize--;
        if (nFrameSize < 0)
            break;

        int x = getSeqFrameChunkPosX(nFrameBase) + 160;
        int y = getSeqFrameChunkPosY(nFrameBase) + 100;

        int stat = 0;
        if (getSeqFrameChunkFlags(nFrameBase) & 1)
            stat |= RS_XFLIPHUD;

        if (getSeqFrameChunkFlags(nFrameBase) & 2)
            stat |= RS_YFLIPHUD;
		
		if (align) stat |= RS_ALIGN_R;

        int16_t nTile = getSeqFrameChunkPicnum(nFrameBase);

        if (frameFlag & 4)
            nShade = -100;

        double alpha = 1;
        if (PlayerList[nLocalPlayer].nInvisible) {
            alpha = 0.3;
        }

        hud_drawsprite(x + xOffs, y + yOffs, 65536, angle.Degrees(), nTile, nShade, nPal, stat, alpha);
        nFrameBase++;
    }

    return frameFlag;
}

int seq_GetFrameSound(int val, int edx)
{
    return getSeqFrameSound(getSeqFrame(val, edx));
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void seq_MoveSequence(DExhumedActor* actor, int16_t nSeq, int16_t nFrame)
{
    assert(nSeq >= 0); // TEMP

    int nSound = getSeqFrameSound(getSeqFrame(nSeq, nFrame));
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

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void playFrameSound(DExhumedActor* actor, const SeqFrame& seqFrame)
{
    const int nSound = seqFrame.sound;

    if (nSound == -1)
        return;

    if (actor)
    {
        D3PlayFX(nSound, actor);
    }
    else
    {
        PlayLocalSound(nSound, 0);
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int seq_GetSeqPicnum2(int16_t nSeq, int16_t nFrame)
{
    return getSeqFrameChunkPicnum(getSeqFrameChunk(getSeqFrame(nSeq, nFrame)));
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

int seq_GetSeqPicnum(int16_t nSeq, int16_t edx, int16_t ebx)
{
    return getSeqFrameChunkPicnum(getSeqFrameChunk(getSeqFrame(getSeqFromId(nSeq, edx))));
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void seq_PlotArrowSequence(const int nSprite, const FName seqFile, const int16_t seqIndex, const int frameIndex)
{
    tspritetype* pTSprite = mytspriteArray->get(nSprite);

    const DAngle nAngle = (nCamerapos.XY() - pTSprite->pos.XY()).Angle();
    const int seqOffset = (((pTSprite->Angles.Yaw + DAngle90 + DAngle22_5 - nAngle).Buildang()) & kAngleMask) >> 8;

    const auto& seqFrame = getSequence(seqFile, seqIndex + seqOffset)[frameIndex];
    const auto& frameChunk = seqFrame.chunks[0];

    auto nStat = pTSprite->cstat | CSTAT_SPRITE_YCENTER;

    if (seqOffset & 3) {
        nStat |= CSTAT_SPRITE_ALIGNMENT_WALL | CSTAT_SPRITE_YFLIP;
    }
    else {
        nStat &= ~(CSTAT_SPRITE_ALIGNMENT_WALL | CSTAT_SPRITE_YFLIP);
    }

    pTSprite->cstat = nStat;
    pTSprite->shade = (seqFrame.flags & 4) ? pTSprite->shade - 100 : pTSprite->shade;
    pTSprite->statnum = seqFrame.chunks.Size();

    if (frameChunk.flags & 1)
    {
        pTSprite->xoffset = (int8_t)frameChunk.xpos;
        pTSprite->cstat |= CSTAT_SPRITE_XFLIP;
    }
    else
    {
        pTSprite->xoffset = (int8_t)-frameChunk.xpos;
    }

    pTSprite->yoffset = -frameChunk.ypos;
    pTSprite->picnum = frameChunk.picnum;
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void seq_PlotSequence(const int nSprite, const FName seqFile, const int16_t seqIndex, const int16_t frameIndex, const int16_t nFlags)
{
    tspritetype* pTSprite = mytspriteArray->get(nSprite);

    int seqOffset = 0;

    if (!(nFlags & 1))
    {
        const DAngle nAngle = (nCamerapos.XY() - pTSprite->pos.XY()).Angle();
        seqOffset = (((pTSprite->Angles.Yaw + DAngle22_5 - nAngle).Buildang()) & kAngleMask) >> 8;
    }

    const auto& baseFrame = getSequence(seqFile, seqIndex)[frameIndex];
    const auto& drawFrame = getSequence(seqFile, seqIndex + seqOffset)[frameIndex];
    const auto chunkCount = drawFrame.chunks.Size();

    const auto nShade = (baseFrame.flags & 4) ? pTSprite->shade - 100 : pTSprite->shade;
    const auto nStatnum = (nFlags & 0x100) ? -3 : 100;

    for (unsigned i = 0; i < chunkCount; i++)
    {
        const auto& seqFrameChunk = drawFrame.chunks[i];

        tspritetype* tsp = mytspriteArray->newTSprite();
        tsp->pos = pTSprite->pos;
        tsp->shade = nShade;
        tsp->pal = pTSprite->pal;
        tsp->scale = pTSprite->scale;
        tsp->Angles.Yaw = pTSprite->Angles.Yaw;
        tsp->ownerActor = pTSprite->ownerActor;
        tsp->sectp = pTSprite->sectp;
        tsp->cstat = pTSprite->cstat |= CSTAT_SPRITE_YCENTER;
        tsp->clipdist = pTSprite->clipdist;
        tsp->statnum = chunkCount - i + nStatnum + 1;

        if (seqFrameChunk.flags & 1)
        {
            tsp->xoffset = (int8_t)seqFrameChunk.xpos;
            tsp->cstat |= CSTAT_SPRITE_XFLIP; // x-flipped
        }
        else
        {
            tsp->xoffset = -seqFrameChunk.xpos;
        }

        tsp->yoffset = -seqFrameChunk.ypos;
        tsp->picnum = seqFrameChunk.picnum;
    }

    if (!(pTSprite->cstat & CSTAT_SPRITE_BLOCK_ALL) || (pTSprite->ownerActor->spr.statnum == 100 && nNetPlayerCount))
    {
        pTSprite->ownerActor = nullptr;
    }
    else
    {
        const auto pSector = pTSprite->sectp;
        const double nFloorZ = pSector->floorz;

        if (nFloorZ <= PlayerList[nLocalPlayer].pActor->viewzoffset + PlayerList[nLocalPlayer].pActor->spr.pos.Z)
        {
            pTSprite->ownerActor = nullptr;
        }
        else
        {
            pTSprite->picnum = nShadowPic;

            const auto nPict = drawFrame.chunks[0].picnum;
            const auto nScale = max(((tileWidth(nPict) << 5) / nShadowWidth) - int16_t((nFloorZ - pTSprite->pos.Z) * 2.), 1) * REPEAT_SCALE;

            pTSprite->cstat = CSTAT_SPRITE_ALIGNMENT_FLOOR | CSTAT_SPRITE_TRANSLUCENT;
            pTSprite->pos.Z = pSector->floorz;
			pTSprite->scale = DVector2(nScale, nScale);
            pTSprite->statnum = -3;
            pTSprite->pal = 0;
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

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
