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

BEGIN_PS_NS

enum
{
	kMaxSequences	= 4096,
	kMaxSEQFiles	= 78,
	kMaxSEQFrames	= 18000,
	kMaxSEQChunks	= 21000
};

int16_t nPilotLightFrame;
int16_t nPilotLightCount;

static FTextureID nShadowPic;
static int16_t nShadowWidth = 1;
int16_t nFlameHeight = 1;

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

static TMap<FName, TArray<Seq>> FileSeqMap;

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

TArray<Seq>* getFileSeqs(const FName nSeqFile)
{
    return FileSeqMap.CheckKey(nSeqFile);
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
    auto& gSequences = FileSeqMap.Insert(FName(seqName), TArray<Seq>(nSeqs, true));

    // Read all this data into something sane.
    for (int nSeq = 0; nSeq < nSeqs; nSeq++)
    {
        // Store unused sequence flags, they may be used with future expansion.
        auto& gSeq = gSequences[nSeq];
        gSeq.flags = nSeqFlags[nSeq];

        // Determine where we are in our frame array.
        const int firstFrame = nSeqFrames[nSeq];
        const int lastFrame = nSeqFrameCount[nSeq] + firstFrame;

        // Get sequence's frame array and resize.
        auto& gSeqFrames = gSeq.frames;
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
                    tileGetTextureID(nSeqFrameChunkPicnum[nChunk]),
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

void seq_LoadSequences()
{
    for (unsigned i = 0; i < kMaxSEQFiles; i++)
    {
        if (addSeq(SeqNames[i]) == 0)
        {
            Printf("Error loading '%s'\n", SeqNames[i]);
        }
    }

    nShadowPic = getSequence("shadow").getFirstFrameTexture();
    nShadowWidth = (int16_t)TexMan.GetGameTexture(nShadowPic)->GetDisplayWidth();

    nFlameHeight = (int16_t)TexMan.GetGameTexture(getSequence("firepoof").getFirstFrameTexture())->GetDisplayHeight();

    nPilotLightCount = getSequence("flamer", 3).frames.Size();
    nPilotLightFrame = 0;

    const auto& fontSeq = getSequence("font2");
    const int nFontFirstChar = legacyTileNum(fontSeq.getFirstFrameTexture());

    for (unsigned i = 0; i < fontSeq.frames.Size(); i++)
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
        const auto& pilotlightSeq = getSequence("flamer", 3);
        const auto& seqFrame = pilotlightSeq.frames[0];

        for (unsigned i = 0; i < seqFrame.chunks.Size(); i++)
        {
            const auto& frameChunk = seqFrame.chunks[i];
            const double x = frameChunk.xpos + (160 + xOffset);
            const double y = frameChunk.ypos + (100 + yOffset);

            hud_drawsprite(x, y, 65536, PlayerList[nLocalPlayer].pActor->spr.Angles.Yaw.Normalized180().Degrees() * 2., legacyTileNum(frameChunk.tex), 0, 0, 1);
        }
    }
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void seq_DrawGunSequence(const Seq& weapSeq, int16_t frameIndex, double xOffs, double yOffs, int nShade, int nPal, DAngle angle, bool align)
{
    const auto& seqFrame = weapSeq.frames[frameIndex];

    if (seqFrame.flags & 4)
        nShade = -100;

    for (unsigned i = 0; i < seqFrame.chunks.Size(); i++)
    {
        const auto& frameChunk = seqFrame.chunks[i];

        int x = frameChunk.xpos + 160;
        int y = frameChunk.ypos + 100;

        int stat = 0;
        if (frameChunk.flags & 1)
            stat |= RS_XFLIPHUD;

        if (frameChunk.flags & 2)
            stat |= RS_YFLIPHUD;
		
		if (align) stat |= RS_ALIGN_R;

        double alpha = 1;
        if (PlayerList[nLocalPlayer].nInvisible) {
            alpha = 0.3;
        }

        hud_drawsprite(x + xOffs, y + yOffs, 65536, angle.Degrees(), legacyTileNum(frameChunk.tex), nShade, nPal, stat, alpha);
    }
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

    const auto& seqFrame = getSequence(seqFile, seqIndex + seqOffset).frames[frameIndex];
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
    pTSprite->setspritetexture(frameChunk.tex);
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

    const auto fileSeqs = getFileSeqs(seqFile);
    const auto& baseFrame = fileSeqs->operator[](seqIndex).frames[frameIndex];
    const auto& drawFrame = fileSeqs->operator[](seqIndex + seqOffset).frames[frameIndex];
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
        tsp->setspritetexture(seqFrameChunk.tex);
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
            pTSprite->setspritetexture(nShadowPic);

            const auto nTexWidth = (int)TexMan.GetGameTexture(drawFrame.getFirstChunkTexture())->GetDisplayWidth();
            const auto nScale = max(((nTexWidth << 5) / nShadowWidth) - int16_t((nFloorZ - pTSprite->pos.Z) * 2.), 1) * REPEAT_SCALE;

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

DEFINE_FIELD_X(SeqFrameChunk, SeqFrameChunk, xpos);
DEFINE_FIELD_X(SeqFrameChunk, SeqFrameChunk, ypos);
DEFINE_FIELD_X(SeqFrameChunk, SeqFrameChunk, tex);
DEFINE_FIELD_X(SeqFrameChunk, SeqFrameChunk, flags);

DEFINE_FIELD_X(SeqFrame, SeqFrame, sound);
DEFINE_FIELD_X(SeqFrame, SeqFrame, flags);

DEFINE_FIELD_X(Seq, Seq, flags);

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

DEFINE_ACTION_FUNCTION(_SeqFrame, playSound)
{
    PARAM_SELF_STRUCT_PROLOGUE(SeqFrame);
    self->playSound(PlayerList[nLocalPlayer].pActor);
    return 0;
}

DEFINE_ACTION_FUNCTION(_SeqFrame, Size)
{
    PARAM_SELF_STRUCT_PROLOGUE(SeqFrame);
    ACTION_RETURN_INT(self->chunks.Size());
}

DEFINE_ACTION_FUNCTION(_SeqFrame, getChunk)
{
    PARAM_SELF_STRUCT_PROLOGUE(SeqFrame);
    PARAM_INT(chunkId);
    ACTION_RETURN_POINTER(self->chunks.Data(chunkId));
}

DEFINE_ACTION_FUNCTION(_Seq, Size)
{
    PARAM_SELF_STRUCT_PROLOGUE(Seq);
    ACTION_RETURN_INT(self->frames.Size());
}

DEFINE_ACTION_FUNCTION(_Seq, getFrame)
{
    PARAM_SELF_STRUCT_PROLOGUE(Seq);
    PARAM_INT(frameId);
    ACTION_RETURN_POINTER(self->frames.Data(frameId));
}

DEFINE_ACTION_FUNCTION(_Exhumed, GetStatusSequence)
{
    PARAM_PROLOGUE;
    PARAM_INT(seqId);
    ACTION_RETURN_POINTER(getFileSeqs("status")->Data(seqId));
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
            ("shadowwidth", nShadowWidth)
            ("flameheight", nFlameHeight)
            .EndObject();
    }
}

END_PS_NS
