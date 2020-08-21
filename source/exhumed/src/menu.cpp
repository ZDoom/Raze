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
#include "compat.h"
#include "build.h"
#include "exhumed.h"
#include "aistuff.h"
#include "player.h"
#include "sequence.h"
#include "menu.h"
#include "names.h"
#include "engine.h"
#include "c_bind.h"
#include "status.h"
#include "sound.h"
#include "names.h"
#include "ps_input.h"
#include "view.h"
#include "raze_sound.h"
#include "menu.h"
#include "v_2ddrawer.h"
#include "gamestate.h"
#include <string>

#include <assert.h>

#ifdef __WATCOMC__
#include <stdlib.h>
#endif

BEGIN_PS_NS


#define kSaveFileName       "savgamea.sav"
#define kMaxSaveSlots		5
#define kMaxSaveSlotChars	25

GameStat GameStats;

short nCinemaSeen[30];

uint8_t energytile[66 * 66] = {0};

short nLeft[50] = {0};
int line;

short SavePosition = -1;

uint8_t *cur;
uint8_t *dest;


unsigned int nRandom = 0x41C6167E;
int dword_9AB57 = 0x1F;
short word_9AB5B = 0;

int keytimer = 0;

short nMenuKeys[] = { sc_N, sc_L, sc_M, sc_V, sc_Q, sc_None }; // select a menu item using the keys. 'N' for New Gane, 'V' for voume etc. 'M' picks Training for some reason...


void menu_ResetKeyTimer();

enum {
    kMenuNewGame = 0,
    kMenuLoadGame,
    kMenuTraining,
    kMenuVolume,
    kMenuQuitGame,
    kMenuMaxItems
};


void ClearCinemaSeen()
{
    memset(nCinemaSeen, 0, sizeof(nCinemaSeen));
}

unsigned int menu_RandomBit2()
{
    unsigned int result = nRandom & 1;

    if ( --dword_9AB57 > 0 )
    {
        nRandom = (result << 31) | (nRandom >> 1);
    }
    else
    {
        dword_9AB57 = 31;
        nRandom ^= nRandom >> 4;
    }
    return result;
}

void InitEnergyTile()
{
    memset(energytile, 96, sizeof(energytile));
}

void DoEnergyTile()
{
    nButtonColor += nButtonColor < 0 ? 8 : 0;

	auto energy1 = TileFiles.tileMakeWritable(kEnergy1);
	auto energy2 = TileFiles.tileMakeWritable(kEnergy2);
	uint8_t *ptr1 = energy1 + 1984;
    uint8_t *ptr2 = energy2 + 2048;

    short nColor = nButtonColor + 161;

    int i, j;

    for (i = 0; i < 32; i++)
    {
        memset(ptr1, nColor, 64);
        memset(ptr2, nColor, 64);

        ptr1 -= 64;
        ptr2 += 64;

        nColor++;

        if (nColor >= 168) {
            nColor = 160;
        }
    }

    tileInvalidate(kEnergy1, -1, -1);

    if (nSmokeSparks)
    {
        uint8_t *c = &energytile[67]; // skip a line
        uint8_t *ptrW = energy2;

        for (i = 0; i < 64; i++)
        {
            for (j = 0; j < 64; j++)
            {
                uint8_t val = *c;

                if (val != 96)
                {
                    if (val > 158) {
                        *ptrW = val - 1;
                    }
                    else {
                        *ptrW = 96;
                    }
                }
                else
                {
                    if (menu_RandomBit2()) {
                        *ptrW = *c;
                    }
                    else
                    {
                        uint8_t al = *(c + 1);
                        uint8_t ah = *(c - 1);

                        if (al <= ah) {
                            al = ah;
                        }

                        uint8_t cl = al;

                        al = *(c - 66);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(c + 66);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(c + 66);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(c + 66);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(c - 65);
                        if (cl <= al) {
                            cl = al;
                        }

                        al = *(c - 67);
                        if (cl > al) {
                            al = cl;
                        }

                        cl = al;

                        if (al <= 159) {
                            *ptrW = 96;
                        }
                        else
                        {
                            if (!menu_RandomBit2()) {
                                cl--;
                            }

                            *ptrW = cl;
                        }
                    }     
                }

                c++;
                ptrW++;
            }

            c += 2;
        }

        c = &energytile[67];
        ptrW = energy2;

        // copy back to energytile[]
        for (i = 0; i < 64; i++)
        {
            memcpy(c, ptrW, 64);
            c += 66;
            ptrW += 64;
        }

        ptrW = energy2;

        // kEnergy2 is 64 x 64
        for (i = 0; i < 4096; i++)
        {
            if (ptrW[i] == 96) {
                ptrW[i] = 255; // -1?
            }
        }

        word_9AB5B--;
        if (word_9AB5B <= 0)
        {
            int randSize  = (RandomSize(5) & 0x1F) + 16;
            int randSize2 = (RandomSize(5) & 0x1F) + 16;

            int val = randSize << 5;
            val += randSize;
            val *= 2;
            val += randSize2;

            assert(val < 4356);

            energytile[val] = 175;
            word_9AB5B = 1;
        }
        tileInvalidate(kEnergy2, -1, -1);
    }
}



int menu_NewGameMenu()
{

    return 0;
}

int menu_LoadGameMenu()
{
    return 0;
}

void menu_GameLoad2(FILE *fp, bool bIsDemo)
{
    if (bIsDemo)
    {
        demo_header header;
        fread(&header, 1, sizeof(demo_header), fp);

        GameStats.nMap = header.nMap;
        GameStats.nWeapons = header.nWeapons;
        GameStats.nCurrentWeapon = header.nCurrentWeapon;
        GameStats.clip = header.clip;
        GameStats.items = header.items;
        GameStats.player.nHealth = header.nHealth;
        GameStats.player.field_2 = header.field_2;
        GameStats.player.nAction = header.nAction;
        GameStats.player.nSprite = header.nSprite;
        GameStats.player.bIsMummified = header.bIsMummified;
        GameStats.player.someNetVal = header.someNetVal;
        GameStats.player.invincibility = header.invincibility;
        GameStats.player.nAir = header.nAir;
        GameStats.player.nSeq = header.nSeq;
        GameStats.player.nMaskAmount = header.nMaskAmount;
        GameStats.player.keys = header.keys;
        GameStats.player.nMagic = header.nMagic;
        Bmemcpy(GameStats.player.items, header.item, sizeof(header.item));
        Bmemcpy(GameStats.player.nAmmo, header.nAmmo, sizeof(header.nAmmo));
        Bmemcpy(GameStats.player.pad, header.pad, sizeof(header.pad));
        GameStats.player.nCurrentWeapon = header.nCurrentWeapon2;
        GameStats.player.field_3FOUR = header.field_3FOUR;
        GameStats.player.bIsFiring = header.bIsFiring;
        GameStats.player.field_38 = header.field_38;
        GameStats.player.field_3A = header.field_3A;
        GameStats.player.field_3C = header.field_3C;
        GameStats.player.nRun = header.nRun;
        GameStats.nLives = header.nLives;
    }
    else
        fread(&GameStats, sizeof(GameStats), 1, fp);

    nPlayerWeapons[nLocalPlayer] = GameStats.nWeapons;

    PlayerList[nLocalPlayer].nCurrentWeapon = GameStats.nCurrentWeapon;
    nPlayerClip[nLocalPlayer] = GameStats.clip;

    int nPistolBullets = PlayerList[nLocalPlayer].nAmmo[kWeaponPistol];
    if (nPistolBullets >= 6) {
        nPistolBullets = 6;
    }

    nPistolClip[nLocalPlayer] = nPistolBullets;

    memcpy(&PlayerList[nLocalPlayer], &GameStats.player, sizeof(Player));

    nPlayerItem[nLocalPlayer]  = GameStats.items;
    nPlayerLives[nLocalPlayer] = GameStats.nLives;

    SetPlayerItem(nLocalPlayer, nPlayerItem[nLocalPlayer]);
    CheckClip(nLocalPlayer);
}

short menu_GameLoad(int nSlot)
{
    memset(&GameStats, 0, sizeof(GameStats));

    FILE *fp = fopen(kSaveFileName, "rb");
    if (fp == NULL) {
        return 0;
    }

    fseek(fp, 125, SEEK_SET);
    fseek(fp, nSlot * sizeof(GameStats), SEEK_CUR);

    menu_GameLoad2(fp);
    fclose(fp);

    return GameStats.nMap;
}

void menu_GameSave2(FILE *fp)
{
    memset(&GameStats, 0, sizeof(GameStats));

    GameStats.nMap = (uint8_t)levelnew;
    GameStats.nWeapons = nPlayerWeapons[nLocalPlayer];
    GameStats.nCurrentWeapon = PlayerList[nLocalPlayer].nCurrentWeapon;
    GameStats.clip   = nPlayerClip[nLocalPlayer];
    GameStats.items  = nPlayerItem[nLocalPlayer];
    GameStats.nLives = nPlayerLives[nLocalPlayer];

    memcpy(&GameStats.player, &PlayerList[nLocalPlayer], sizeof(GameStats.player));

    fwrite(&GameStats, sizeof(GameStats), 1, fp);
}

void menu_GameSave(int nSaveSlot)
{
    if (nSaveSlot < 0) {
        return;
    }

    FILE *fp = fopen(kSaveFileName, "rb+");
    if (fp != NULL)
    {
        fseek(fp, 125, SEEK_SET); // skip save slot names
        fseek(fp, sizeof(GameStat) * nSaveSlot, SEEK_CUR);
        menu_GameSave2(fp);
        fclose(fp);
    }
}

#define kMaxCinemaPals	16
const char *cinpalfname[kMaxCinemaPals] = {
    "3454.pal",
    "3452.pal",
    "3449.pal",
    "3445.pal",
    "set.pal",
    "3448.pal",
    "3446.pal",
    "hsc1.pal",
    "2972.pal",
    "2973.pal",
    "2974.pal",
    "2975.pal",
    "2976.pal",
    "heli.pal",
    "2978.pal",
    "terror.pal"
};

int linecount;
int nextclock;
short nHeight;
short nCrawlY;
short cinematile;
int currentCinemaPalette;


void uploadCinemaPalettes()
{
    for (int i = 0; i < countof(cinpalfname); i++)
    {
        uint8_t palette[768] = {};
        auto hFile = fileSystem.OpenFileReader(cinpalfname[i]);
        if (hFile.isOpen())
            hFile.Read(palette, 768);
        for (auto& c : palette)
            c <<= 2;
        paletteSetColorTable(ANIMPAL+i, palette, false, true);
    }
}

//int IncrementCinemaFadeIn()
//{
//    dest = cinemapal;
//    cur = curpal;
//
//    int ebx = 0;
//
//    for (int i = 0; i < 768; i++)
//    {
//        ebx++;
//
//        if (*cur < *dest)
//        {
//            (*cur)++;
//        }
//        else if (*cur == *dest)
//        {
//            ebx--;
//        }
//        else
//        {
//            (*cur)--;
//        }
//
//        cur++;
//        dest++;
//    }
//
//    MySetPalette(curpal);
//    return ebx;
//}

void CinemaFadeIn()
{
    BlackOut();

#ifdef USE_OPENGL
    if (videoGetRenderMode() >= REND_POLYMOST)
    {
        videoNextPage();
        return;
    }
#endif

    int val;

    do
    {
        val = DoFadeIn();
        WaitTicks(2);

        // need to page flip in each iteration of the loop for non DOS version
        videoNextPage();

    } while (val > 0);
}

void ComputeCinemaText(int nLine)
{
    linecount = 0;

    while (1)
    {
        if (!strcmp(gString[linecount + nLine], "END")) {
            break;
        }

        int nWidth = MyGetStringWidth(gString[linecount + nLine]);
        nLeft[linecount] = 160 - nWidth / 2;

        linecount++;
    }

    nCrawlY = 199;
    nHeight = linecount * 10;

    inputState.ClearAllInput();
}

void ReadyCinemaText(uint16_t nVal)
{
    line = FindGString("CINEMAS");
    if (line < 0) {
        return;
    }

    while (nVal)
    {
        while (strcmp(gString[line], "END")) {
            line++;
        }

        line++;
        nVal--;
    }

    ComputeCinemaText(line);
}

bool AdvanceCinemaText()
{
    bool bDoText = nHeight + nCrawlY > 0;

    if (bDoText || CDplaying())
    {
        nextclock = (int)totalclock + 15; // NOTE: Value was 14 in original code but seems a touch too fast now

        if (bDoText)
        {
            short y = nCrawlY;
            int i = 0;

            while (i < linecount && y <= 199)
            {
                if (y >= -10) {
                    myprintext(nLeft[i], y, gString[line + i], 0, currentCinemaPalette);
                }

                i++;
                y += 10;
            }

            nCrawlY--;
        }

        while (1)
        {
            HandleAsync();

            if (inputState.CheckAllInput())
            {
                break;
            }

            if (nextclock <= (int)totalclock) {
                return true;
            }
        }
    }

    return false;
}

void DoCinemaText(short nVal)
{
    ReadyCinemaText(nVal);

    bool bContinue = true;

    while (bContinue)
    {
        overwritesprite(0, 0, cinematile, 0, 2, kPalNormal, currentCinemaPalette);

        bContinue = AdvanceCinemaText();

        WaitVBL();
        videoNextPage();
    }
}

void GoToTheCinema(int nVal)
{
    switch (nVal - 1)
    {
        default:
            return;

        case 0:
        {
            cinematile = 3454;
            break;
        }

        case 1:
        {
            cinematile = 3452;
            break;
        }

        case 2:
        {
            cinematile = 3449;
            break;
        }

        case 3:
        {
            cinematile = 3445;
            break;
        }

        case 4:
        {
            cinematile = 3451;
            break;
        }

        case 5:
        {
            cinematile = 3448;
            break;
        }

        case 6:
        {
            cinematile = 3446;
            break;
        }
    }
    currentCinemaPalette = nVal;

    FadeOut(false);
    StopAllSounds();
    NoClip();

    overwritesprite(0, 0, kMovieTile, 100, 2, kPalNormal, currentCinemaPalette);
    videoNextPage();

//	int386(16, (const union REGS *)&val, (union REGS *)&val)

    overwritesprite(0, 0, cinematile, 0, 2, kPalNormal, currentCinemaPalette);
    videoNextPage();

    CinemaFadeIn();
    inputState.ClearAllInput();

    int ebx = -1;
    int edx = -1;

    switch (nVal - 1)
    {
        default:
            WaitAnyKey(10);
            break;

        case 0:
            ebx = 4;
            edx = ebx;
            break;

        case 1:
            ebx = 0;
            break;

        case 2:
            ebx = 2;
            edx = ebx;
            break;

        case 3:
            ebx = 7;
            break;

        case 4:
            ebx = 3;
            edx = ebx;
            break;

        case 5:
            ebx = 8;
            edx = ebx;
            break;

        case 6:
            ebx = 6;
            edx = ebx;
            break;
    }

    if (ebx != -1)
    {
        if (edx != -1)
        {
            if (CDplaying()) {
                fadecdaudio();
            }

            playCDtrack(edx + 2, false);
        }

        DoCinemaText(ebx);
    }

    FadeOut(true);

    overwritesprite(0, 0, kMovieTile, 100, 2, kPalNormal, currentCinemaPalette);
    videoNextPage();

    GrabPalette();

    // quit the game if we've finished level 4 and displayed the advert text
    if (ISDEMOVER && nVal == 3) {
        ExitGame();
    }
}


short nBeforeScene[] = { 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 };


void CheckBeforeScene(int nLevel)
{
    if (nLevel == kMap20)
    {
        DoLastLevelCinema();
        return;
    }

    short nScene = nBeforeScene[nLevel];

    if (nScene)
    {
        if (!nCinemaSeen[nScene])
        {
            GoToTheCinema(nScene);
            nCinemaSeen[nScene] = 1;
        }
    }
}

int SyncScreenJob();

int showmap(short nLevel, short nLevelNew, short nLevelBest)
{
    FadeOut(0);
    EraseScreen(overscanindex);
    GrabPalette();
    BlackOut();

    if (nLevelNew != 11) {
        CheckBeforeScene(nLevelNew);
    }

	int selectedLevel;
	menu_DrawTheMap(nLevel, nLevelNew, nLevelBest, [&](int lev){
		gamestate = GS_LEVEL;
		selectedLevel = lev;
	});
	SyncScreenJob();
    if (selectedLevel == 11) {
        CheckBeforeScene(selectedLevel);
    }

    return selectedLevel;
}

void DoAfterCinemaScene(int nLevel)
{
    short nAfterScene[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 7, 0, 0, 0, 0, 6 };

    if (nAfterScene[nLevel]) {
        GoToTheCinema(nAfterScene[nLevel]);
    }
}

void DoFailedFinalScene()
{
    videoSetViewableArea(0, 0, xdim - 1, ydim - 1);

    if (CDplaying()) {
        fadecdaudio();
    }

    playCDtrack(9, false);
    FadeToWhite();

    GoToTheCinema(4);
}

int FindGString(const char *str)
{
    int i = 0;

    while (1)
    {
        if (!strcmp(gString[i], str))
            return i + 1;

        if (!strcmp(gString[i], "EOF"))
            break;

        i++;
    }

    return -1;
}

uint8_t CheckForEscape()
{
    return inputState.CheckAllInput();
}

void DoStatic(int a, int b)
{
    RandomLong(); // nothing done with the result of this?

    auto pixels = TileFiles.tileMakeWritable(kTileLoboLaptop);

    int v2 = 160 - a / 2;
    int v4 = 81  - b / 2;

    int var_18 = v2 + a;
    int v5 = v4 + b;

    auto pTile = (pixels + (200 * v2)) + v4;

    tileInvalidate(kTileLoboLaptop, -1, -1);

    while (v2 < var_18)
    {
        uint8_t *pStart = pTile;
        pTile += 200;

        int v7 = v4;

        while (v7 < v5)
        {
            *pStart = RandomBit() * 16;

            v7++;
            pStart++;
        }
        v2++;
    }

	tileInvalidate(kTileLoboLaptop, 0, 0);
    overwritesprite(0, 0, kTileLoboLaptop, 0, 2, kPalNormal);
    videoNextPage();
}

void DoLastLevelCinema()
{
    FadeOut(0);

    videoSetViewableArea(0, 0, xdim - 1, ydim - 1);

    EraseScreen(-1);
    RestorePalette();

    int nString = FindGString("LASTLEVEL");

    PlayLocalSound(StaticSound[kSound75], 0, false, CHANF_UI);

	auto pixels = TileFiles.tileMakeWritable(kTileLoboLaptop);
	// uh, what?
    //memcpy((void*)waloff[kTileLoboLaptop], (void*)waloff[kTileLoboLaptop], tilesiz[kTileLoboLaptop].x * tilesiz[kTileLoboLaptop].y);

    int var_24 = 16;
    int var_28 = 12;

    int nEndTime = (int)totalclock + 240;

    while (inputState.keyBufferWaiting()) {
        inputState.keyGetChar();
    }

    while (nEndTime > (int)totalclock)
    {
        HandleAsync();

        if (var_24 >= 116)
        {
            if (var_28 < 192)
                var_28 += 20;
        }
        else
        {
            var_24 += 20;
        }

        DoStatic(var_28, var_24);

        // WaitVBL();
        int time = (int)totalclock + 4;
        while ((int)totalclock < time) {
            HandleAsync();
        }
    }

    // loc_3AD75

    do
    {  
    LABEL_11:

        HandleAsync();

        if (strlen(gString[nString]) == 0)
            break;

        int esi = nString;

        while (strlen(gString[esi]) != 0)
            esi++;

        int ebp = esi;

        ebp -= nString;
        ebp <<= 2;
        ebp = 81 - ebp;

        int var_1C = esi - nString;

        // loc_3ADD7
        while (1)
        {
            HandleAsync();

            if (strlen(gString[nString]) == 0)
                break;

            int xPos = 70;

            const char *nChar = gString[nString];

            nString++;

			TileFiles.tileMakeWritable(kTileLoboLaptop);
            while (*nChar)
            {
                HandleAsync();

                if (*nChar != ' ') {
                    PlayLocalSound(StaticSound[kSound71], 0, false, CHANF_UI);
                }

                xPos += CopyCharToBitmap(*nChar, kTileLoboLaptop, xPos, ebp);
                nChar++;

                overwritesprite(0, 0, kTileLoboLaptop, 0, 2, kPalNormal);
                videoNextPage();

                // WaitVBL();
                int time = (int)totalclock + 4;
                while ((int)totalclock < time) {
                    HandleAsync();
                }

                if (CheckForEscape())
                    goto LABEL_28;
            }

            ebp += 8;
        }

        nString++;

        inputState.ClearAllInput();

        int v11 = (kTimerTicks * (var_1C + 2)) + (int)totalclock;

        do
        {
            HandleAsync();

            if (v11 <= (int)totalclock)
                goto LABEL_11;
        } while (!inputState.keyBufferWaiting());
    }
    while (inputState.keyGetChar() != 27);

LABEL_28:
    PlayLocalSound(StaticSound[kSound75], 0, false, CHANF_UI);

    nEndTime = (int)totalclock + 240;

    while (nEndTime > (int)totalclock)
    {
        HandleAsync();

        DoStatic(var_28, var_24);

        // WaitVBL();
        int time = (int)totalclock + 4;
        while ((int)totalclock < time) {
            HandleAsync();
        }

        if (var_28 > 20) {
            var_28 -= 20;
            continue;
        }

        if (var_24 > 20) {
            var_24 -= 20;
            continue;
        }

        break;
    }

    EraseScreen(-1);
    tileLoad(kTileLoboLaptop);
    FadeOut(0);
}

static SavegameHelper sgh("menu",
    SA(nCinemaSeen),
    SA(energytile),
    SV(nButtonColor),
    SV(word_9AB5B),
    nullptr);

END_PS_NS
