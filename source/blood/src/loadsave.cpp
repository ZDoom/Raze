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
#include <stdio.h>
#include "build.h"
#include "compat.h"
#include "mmulti.h"
#include "common_game.h"
#include "config.h"
#include "ai.h"
#include "asound.h"
#include "blood.h"
#include "demo.h"
#include "globals.h"
#include "db.h"
#include "messages.h"
#include "menu.h"
#include "network.h"
#include "loadsave.h"
#include "resource.h"
#include "screen.h"
#include "sectorfx.h"
#include "seq.h"
#include "sfx.h"
#include "sound.h"
#include "view.h"

GAMEOPTIONS gSaveGameOptions[10];
char *gSaveGamePic[10];
unsigned int gSavedOffset = 0;

unsigned int dword_27AA38 = 0;
unsigned int dword_27AA3C = 0;
unsigned int dword_27AA40 = 0;
void *dword_27AA44 = NULL;

LoadSave LoadSave::head(123);
FILE *LoadSave::hSFile = NULL;
int LoadSave::hLFile = -1;

short word_27AA54 = 0;

void sub_76FD4(void)
{
    if (!dword_27AA44)
        dword_27AA44 = Resource::Alloc(0x186a0);
}

void LoadSave::Save(void)
{
    ThrowError("Pure virtual function called");
}

void LoadSave::Load(void)
{
    ThrowError("Pure virtual function called");
}

void LoadSave::Read(void *pData, int nSize)
{
    dword_27AA38 += nSize;
    dassert(hLFile != -1);
    if (kread(hLFile, pData, nSize) != nSize)
        ThrowError("Error reading save file.");
}

void LoadSave::Write(void *pData, int nSize)
{
    dword_27AA38 += nSize;
    dword_27AA3C += nSize;
    dassert(hSFile != NULL);
    if (fwrite(pData, 1, nSize, hSFile) != (size_t)nSize)
        ThrowError("File error #%d writing save file.", errno);
}

void LoadSave::LoadGame(char *pzFile)
{
    bool demoWasPlayed = gDemo.at1;
    if (gDemo.at1)
        gDemo.Close();

    sndKillAllSounds();
    sfxKillAllSounds();
    ambKillAll();
    seqKillAll();
    if (!gGameStarted)
    {
        memset(xsprite, 0, sizeof(xsprite));
        memset(sprite, 0, sizeof(spritetype)*kMaxSprites);
        automapping = 1;
    }
    hLFile = kopen4load(pzFile, 0);
    if (hLFile == -1)
        ThrowError("Error loading save file.");
    LoadSave *rover = head.next;
    while (rover != &head)
    {
        rover->Load();
        rover = rover->next;
    }
    kclose(hLFile);
    hLFile = -1;
    if (!gGameStarted)
        scrLoadPLUs();
    InitSectorFX();
    viewInitializePrediction();
    PreloadCache();
    if (!bVanilla && !gMe->packInfo[1].at0) // if diving suit is not active, turn off reverb sound effect
        sfxSetReverb(0);
    ambInit();
#ifdef YAX_ENABLE
    yax_update(numyaxbunches > 0 ? 2 : 1);
#endif
    memset(myMinLag, 0, sizeof(myMinLag));
    otherMinLag = 0;
    myMaxLag = 0;
    gNetFifoClock = 0;
    gNetFifoTail = 0;
    memset(gNetFifoHead, 0, sizeof(gNetFifoHead));
    gPredictTail = 0;
    gNetFifoMasterTail = 0;
    memset(gFifoInput, 0, sizeof(gFifoInput));
    memset(gChecksum, 0, sizeof(gChecksum));
    memset(gCheckFifo, 0, sizeof(gCheckFifo));
    memset(gCheckHead, 0, sizeof(gCheckHead));
    gSendCheckTail = 0;
    gCheckTail = 0;
    gBufferJitter = 0;
    bOutOfSync = 0;
    for (int i = 0; i < gNetPlayers; i++)
        playerSetRace(&gPlayer[i], gPlayer[i].at5f);
    viewSetMessage("");
    viewSetErrorMessage("");
    if (!gGameStarted)
    {
        netWaitForEveryone(0);
        memset(gPlayerReady, 0, sizeof(gPlayerReady));
    }
    gFrameTicks = 0;
    gFrame = 0;
    gCacheMiss = 0;
    gFrameRate = 0;
    totalclock = 0;
    gPaused = 0;
    gGameStarted = 1;
    bVanilla = false;

    if (MusicRestartsOnLoadToggle
        || demoWasPlayed
        || (gMusicPrevLoadedEpisode != gGameOptions.nEpisode || gMusicPrevLoadedLevel != gGameOptions.nLevel))
    {
        levelTryPlayMusic(gGameOptions.nEpisode, gGameOptions.nLevel);
    }
    gMusicPrevLoadedEpisode = gGameOptions.nEpisode;
    gMusicPrevLoadedLevel = gGameOptions.nLevel;

    netBroadcastPlayerInfo(myconnectindex);
    //sndPlaySong(gGameOptions.zLevelSong, 1);
}

void LoadSave::SaveGame(char *pzFile)
{
    hSFile = fopen(pzFile, "wb");
    if (hSFile == NULL)
        ThrowError("File error #%d creating save file.", errno);
    dword_27AA38 = 0;
    dword_27AA40 = 0;
    LoadSave *rover = head.next;
    while (rover != &head)
    {
        rover->Save();
        if (dword_27AA38 > dword_27AA40)
            dword_27AA40 = dword_27AA38;
        dword_27AA38 = 0;
        rover = rover->next;
    }
    fclose(hSFile);
    hSFile = NULL;
}

class MyLoadSave : public LoadSave
{
public:
    virtual void Load(void);
    virtual void Save(void);
};

void MyLoadSave::Load(void)
{
    psky_t *pSky = tileSetupSky(0);
    int id;
    Read(&id, sizeof(id));
    if (id != 0x5653424e/*'VSBN'*/)
        ThrowError("Old saved game found");
    short version;
    Read(&version, sizeof(version));
    if (version != BYTEVERSION)
        ThrowError("Incompatible version of saved game found!");
    Read(&gGameOptions, sizeof(gGameOptions));
    Read(&numsectors, sizeof(numsectors));
    Read(&numwalls, sizeof(numwalls));
    Read(&numsectors, sizeof(numsectors));
    int nNumSprites;
    Read(&nNumSprites, sizeof(nNumSprites));
    memset(sector, 0, sizeof(sector[0])*kMaxSectors);
    memset(wall, 0, sizeof(wall[0])*kMaxWalls);
    memset(sprite, 0, sizeof(sprite[0])*kMaxSprites);
    Read(sector, sizeof(sector[0])*numsectors);
    Read(wall, sizeof(wall[0])*numwalls);
    Read(sprite, sizeof(sprite[0])*kMaxSprites);
    Read(qsector_filler, sizeof(qsector_filler[0])*numsectors);
    Read(qsprite_filler, sizeof(qsprite_filler[0])*kMaxSprites);
    Read(&randomseed, sizeof(randomseed));
    Read(&parallaxtype, sizeof(parallaxtype));
    Read(&showinvisibility, sizeof(showinvisibility));
    Read(&pSky->horizfrac, sizeof(pSky->horizfrac));
    Read(&pSky->yoffs, sizeof(pSky->yoffs));
    Read(&pSky->yscale, sizeof(pSky->yscale));
    Read(&gVisibility, sizeof(gVisibility));
    Read(&g_visibility, sizeof(g_visibility));
    Read(&parallaxvisibility, sizeof(parallaxvisibility));
    Read(pSky->tileofs, sizeof(pSky->tileofs));
    Read(&pSky->lognumtiles, sizeof(pSky->lognumtiles));
    Read(headspritesect, sizeof(headspritesect));
    Read(headspritestat, sizeof(headspritestat));
    Read(prevspritesect, sizeof(prevspritesect));
    Read(prevspritestat, sizeof(prevspritestat));
    Read(nextspritesect, sizeof(nextspritesect));
    Read(nextspritestat, sizeof(nextspritestat));
    Read(show2dsector, sizeof(show2dsector));
    Read(show2dwall, sizeof(show2dwall));
    Read(show2dsprite, sizeof(show2dsprite));
    Read(&automapping, sizeof(automapping));
    Read(gotpic, sizeof(gotpic));
    Read(gotsector, sizeof(gotsector));
    Read(&gFrameClock, sizeof(gFrameClock));
    Read(&gFrameTicks, sizeof(gFrameTicks));
    Read(&gFrame, sizeof(gFrame));
    ClockTicks nGameClock;
    Read(&totalclock, sizeof(totalclock));
    totalclock = nGameClock;
    Read(&gLevelTime, sizeof(gLevelTime));
    Read(&gPaused, sizeof(gPaused));
    Read(&gbAdultContent, sizeof(gbAdultContent));
    Read(baseWall, sizeof(baseWall[0])*numwalls);
    Read(baseSprite, sizeof(baseSprite[0])*nNumSprites);
    Read(baseFloor, sizeof(baseFloor[0])*numsectors);
    Read(baseCeil, sizeof(baseCeil[0])*numsectors);
    Read(velFloor, sizeof(velFloor[0])*numsectors);
    Read(velCeil, sizeof(velCeil[0])*numsectors);
    Read(&gHitInfo, sizeof(gHitInfo));
    Read(&byte_1A76C6, sizeof(byte_1A76C6));
    Read(&byte_1A76C8, sizeof(byte_1A76C8));
    Read(&byte_1A76C7, sizeof(byte_1A76C7));
    Read(&byte_19AE44, sizeof(byte_19AE44));
    Read(gStatCount, sizeof(gStatCount));
    Read(nextXSprite, sizeof(nextXSprite));
    Read(nextXWall, sizeof(nextXWall));
    Read(nextXSector, sizeof(nextXSector));
    memset(xsprite, 0, sizeof(xsprite));
    for (int nSprite = 0; nSprite < kMaxSprites; nSprite++)
    {
        if (sprite[nSprite].statnum < kMaxStatus)
        {
            int nXSprite = sprite[nSprite].extra;
            if (nXSprite > 0)
                Read(&xsprite[nXSprite], sizeof(XSPRITE));
        }
    }
    memset(xwall, 0, sizeof(xwall));
    for (int nWall = 0; nWall < numwalls; nWall++)
    {
        int nXWall = wall[nWall].extra;
        if (nXWall > 0)
            Read(&xwall[nXWall], sizeof(XWALL));
    }
    memset(xsector, 0, sizeof(xsector));
    for (int nSector = 0; nSector < numsectors; nSector++)
    {
        int nXSector = sector[nSector].extra;
        if (nXSector > 0)
            Read(&xsector[nXSector], sizeof(XSECTOR));
    }
    Read(xvel, nNumSprites*sizeof(xvel[0]));
    Read(yvel, nNumSprites*sizeof(yvel[0]));
    Read(zvel, nNumSprites*sizeof(zvel[0]));
    Read(&gMapRev, sizeof(gMapRev));
    Read(&gSongId, sizeof(gSkyCount));
    Read(&gFogMode, sizeof(gFogMode));
#ifdef YAX_ENABLE
    Read(&numyaxbunches, sizeof(numyaxbunches));
#endif
    gCheatMgr.sub_5BCF4();
}

void MyLoadSave::Save(void)
{
    psky_t *pSky = tileSetupSky(0);
    int nNumSprites = 0;
    int id = 0x5653424e/*'VSBN'*/;
    Write(&id, sizeof(id));
    short version = BYTEVERSION;
    Write(&version, sizeof(version));
    for (int nSprite = 0; nSprite < kMaxSprites; nSprite++)
    {
        if (sprite[nSprite].statnum < kMaxStatus && nSprite > nNumSprites)
            nNumSprites = nSprite;
    }
    //nNumSprites += 2;
    nNumSprites++;
    Write(&gGameOptions, sizeof(gGameOptions));
    Write(&numsectors, sizeof(numsectors));
    Write(&numwalls, sizeof(numwalls));
    Write(&numsectors, sizeof(numsectors));
    Write(&nNumSprites, sizeof(nNumSprites));
    Write(sector, sizeof(sector[0])*numsectors);
    Write(wall, sizeof(wall[0])*numwalls);
    Write(sprite, sizeof(sprite[0])*kMaxSprites);
    Write(qsector_filler, sizeof(qsector_filler[0])*numsectors);
    Write(qsprite_filler, sizeof(qsprite_filler[0])*kMaxSprites);
    Write(&randomseed, sizeof(randomseed));
    Write(&parallaxtype, sizeof(parallaxtype));
    Write(&showinvisibility, sizeof(showinvisibility));
    Write(&pSky->horizfrac, sizeof(pSky->horizfrac));
    Write(&pSky->yoffs, sizeof(pSky->yoffs));
    Write(&pSky->yscale, sizeof(pSky->yscale));
    Write(&gVisibility, sizeof(gVisibility));
    Write(&g_visibility, sizeof(g_visibility));
    Write(&parallaxvisibility, sizeof(parallaxvisibility));
    Write(pSky->tileofs, sizeof(pSky->tileofs));
    Write(&pSky->lognumtiles, sizeof(pSky->lognumtiles));
    Write(headspritesect, sizeof(headspritesect));
    Write(headspritestat, sizeof(headspritestat));
    Write(prevspritesect, sizeof(prevspritesect));
    Write(prevspritestat, sizeof(prevspritestat));
    Write(nextspritesect, sizeof(nextspritesect));
    Write(nextspritestat, sizeof(nextspritestat));
    Write(show2dsector, sizeof(show2dsector));
    Write(show2dwall, sizeof(show2dwall));
    Write(show2dsprite, sizeof(show2dsprite));
    Write(&automapping, sizeof(automapping));
    Write(gotpic, sizeof(gotpic));
    Write(gotsector, sizeof(gotsector));
    Write(&gFrameClock, sizeof(gFrameClock));
    Write(&gFrameTicks, sizeof(gFrameTicks));
    Write(&gFrame, sizeof(gFrame));
    ClockTicks nGameClock = totalclock;
    Write(&nGameClock, sizeof(nGameClock));
    Write(&gLevelTime, sizeof(gLevelTime));
    Write(&gPaused, sizeof(gPaused));
    Write(&gbAdultContent, sizeof(gbAdultContent));
    Write(baseWall, sizeof(baseWall[0])*numwalls);
    Write(baseSprite, sizeof(baseSprite[0])*nNumSprites);
    Write(baseFloor, sizeof(baseFloor[0])*numsectors);
    Write(baseCeil, sizeof(baseCeil[0])*numsectors);
    Write(velFloor, sizeof(velFloor[0])*numsectors);
    Write(velCeil, sizeof(velCeil[0])*numsectors);
    Write(&gHitInfo, sizeof(gHitInfo));
    Write(&byte_1A76C6, sizeof(byte_1A76C6));
    Write(&byte_1A76C8, sizeof(byte_1A76C8));
    Write(&byte_1A76C7, sizeof(byte_1A76C7));
    Write(&byte_19AE44, sizeof(byte_19AE44));
    Write(gStatCount, sizeof(gStatCount));
    Write(nextXSprite, sizeof(nextXSprite));
    Write(nextXWall, sizeof(nextXWall));
    Write(nextXSector, sizeof(nextXSector));
    for (int nSprite = 0; nSprite < kMaxSprites; nSprite++)
    {
        if (sprite[nSprite].statnum < kMaxStatus)
        {
            int nXSprite = sprite[nSprite].extra;
            if (nXSprite > 0)
                Write(&xsprite[nXSprite], sizeof(XSPRITE));
        }
    }
    for (int nWall = 0; nWall < numwalls; nWall++)
    {
        int nXWall = wall[nWall].extra;
        if (nXWall > 0)
            Write(&xwall[nXWall], sizeof(XWALL));
    }
    for (int nSector = 0; nSector < numsectors; nSector++)
    {
        int nXSector = sector[nSector].extra;
        if (nXSector > 0)
            Write(&xsector[nXSector], sizeof(XSECTOR));
    }
    Write(xvel, nNumSprites*sizeof(xvel[0]));
    Write(yvel, nNumSprites*sizeof(yvel[0]));
    Write(zvel, nNumSprites*sizeof(zvel[0]));
    Write(&gMapRev, sizeof(gMapRev));
    Write(&gSongId, sizeof(gSkyCount));
    Write(&gFogMode, sizeof(gFogMode));
#ifdef YAX_ENABLE
    Write(&numyaxbunches, sizeof(numyaxbunches));
#endif
}

void LoadSavedInfo(void)
{
    auto pList = klistpath("./", "game*.sav", CACHE1D_FIND_FILE);
    int nCount = 0;
    for (auto pIterator = pList; pIterator != NULL && nCount < 10; pIterator = pIterator->next, nCount++)
    {
        int hFile = kopen4loadfrommod(pIterator->name, 0);
        if (hFile == -1)
            ThrowError("Error loading save file header.");
        int vc;
        short v4;
        vc = 0;
        v4 = word_27AA54;
        if ((uint32_t)kread(hFile, &vc, sizeof(vc)) != sizeof(vc))
        {
            kclose(hFile);
            continue;
        }
        if (vc != 0x5653424e/*'VSBN'*/)
        {
            kclose(hFile);
            continue;
        }
        kread(hFile, &v4, sizeof(v4));
        if (v4 != BYTEVERSION)
        {
            kclose(hFile);
            continue;
        }
        if ((uint32_t)kread(hFile, &gSaveGameOptions[nCount], sizeof(gSaveGameOptions[0])) != sizeof(gSaveGameOptions[0]))
            ThrowError("Error reading save file.");
        strcpy(strRestoreGameStrings[gSaveGameOptions[nCount].nSaveGameSlot], gSaveGameOptions[nCount].szUserGameName);
        kclose(hFile);
    }
    klistfree(pList);
}

void UpdateSavedInfo(int nSlot)
{
    strcpy(strRestoreGameStrings[gSaveGameOptions[nSlot].nSaveGameSlot], gSaveGameOptions[nSlot].szUserGameName);
}

static MyLoadSave *myLoadSave;


void LoadSaveSetup(void)
{
    void ActorLoadSaveConstruct(void);
    void AILoadSaveConstruct(void);
    void EndGameLoadSaveConstruct(void);
    void EventQLoadSaveConstruct(void);
    void LevelsLoadSaveConstruct(void);
    void MessagesLoadSaveConstruct(void);
    void MirrorLoadSaveConstruct(void);
    void PlayerLoadSaveConstruct(void);
    void SeqLoadSaveConstruct(void);
    void TriggersLoadSaveConstruct(void);
    void ViewLoadSaveConstruct(void);
    void WarpLoadSaveConstruct(void);
    void WeaponLoadSaveConstruct(void);

    myLoadSave = new MyLoadSave();

    ActorLoadSaveConstruct();
    AILoadSaveConstruct();
    EndGameLoadSaveConstruct();
    EventQLoadSaveConstruct();
    LevelsLoadSaveConstruct();
    MessagesLoadSaveConstruct();
    MirrorLoadSaveConstruct();
    PlayerLoadSaveConstruct();
    SeqLoadSaveConstruct();
    TriggersLoadSaveConstruct();
    ViewLoadSaveConstruct();
    WarpLoadSaveConstruct();
    WeaponLoadSaveConstruct();
}