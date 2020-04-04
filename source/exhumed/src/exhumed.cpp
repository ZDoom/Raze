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
#include "baselayer.h"
#include "baselayer.h"
#include "typedefs.h"
#include "common.h"
#include "engine.h"
#include "exhumed.h"
#include "osdcmds.h"
#include "map.h"
#include "sequence.h"
#include "movie.h"
#include "names.h"
#include "menu.h"
#include "player.h"
#include "network.h"
#include "ps_input.h"
#include "sound.h"
#include "cd.h"
#include "view.h"
#include "status.h"
#include "init.h"
#include "ra.h"
#include "version.h"
#include "timer.h"
#include "runlist.h"
#include "anubis.h"
#include "spider.h"
#include "mummy.h"
#include "fish.h"
#include "lion.h"
#include "light.h"
#include "move.h"
#include "lavadude.h"
#include "rex.h"
#include "set.h"
#include "queen.h"
#include "roach.h"
#include "wasp.h"
#include "scorp.h"
#include "rat.h"
#include "serial.h"
#include "network.h"
#include "random.h"
#include "items.h"
#include "trigdat.h"
#include "record.h"
#include "lighting.h"
#include "mapinfo.h"
#include <string.h>
#include <cstdio> // for printf
#include <cstdlib>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>
#include "gamecvars.h"
#include "savegamehelp.h"
#include "c_dispatch.h"
#include "s_soundinternal.h"
#include "common/menu/menu.h"

BEGIN_PS_NS


    extern const char* s_buildRev;
    extern const char* s_buildTimestamp;


void FinishLevel();

int htimer = 0;

/* these are XORed in the original game executable then XORed back to normal when the game first starts. Here they are normally */
const char *gString[] =
{
    "CINEMAS",
    "THE ANCIENT EGYPTIAN CITY",
    "OF KARNAK HAS BEEN SEIZED",
    "BY UNKNOWN POWERS, AND GREAT",
    "TURMOIL IS SPREADING INTO",
    "NEIGHBORING LANDS, POSING",
    "A GRAVE THREAT TO PLANET",
    "EARTH. MILITANT FORCES FROM",
    "ALL PARTS OF THE GLOBE HAVE",
    "ENTERED THE KARNAK VALLEY,",
    "BUT NONE HAVE RETURNED. THE",
    "ONLY KNOWN INFORMATION",
    "REGARDING THIS CRISIS CAME",
    "FROM A DYING KARNAK VILLAGER",
    "WHO MANAGED TO WANDER OUT OF",
    "THE VALLEY TO SAFETY.",
    "'THEY'VE STOLEN THE GREAT",
    "KING'S MUMMY...', MURMURED",
    "THE DYING VILLAGER, BUT THE",
    "VILLAGER DIED BEFORE HE",
    "COULD SAY MORE. WITH NO",
    "OTHER OPTIONS, WORLD",
    "LEADERS HAVE CHOSEN TO DROP",
    "YOU INTO THE VALLEY VIA",
    "HELICOPTER IN AN ATTEMPT",
    "TO FIND AND DESTROY THE",
    "THREATENING FORCES AND",
    "RESOLVE THE MYSTERY THAT",
    "HAS ENGULFED THIS ONCE",
    "PEACEFUL LAND. FLYING AT",
    "HIGH ALTITUDE TO AVOID",
    "BEING SHOT DOWN LIKE OTHERS",
    "BEFORE YOU, YOUR COPTER",
    "MYSTERIOUSLY EXPLODES IN THE",
    "AIR AS YOU BARELY ESCAPE,",
    "WITH NO POSSIBLE CONTACT",
    "WITH THE OUTSIDE WORLD.",
    "SCARED AS HELL, YOU DESCEND",
    "INTO THE HEART OF KARNAK...",
    "HOME TO THE CELEBRATED",
    "BURIAL CRYPT OF THE GREAT",
    "KING RAMSES.",
    "END",
    "AN EVIL FORCE KNOWN AS THE",
    "KILMAAT HAS BESIEGED THE",
    "SANCTITY OF MY PALACE AND",
    "IS PULLING AT THE VERY",
    "TENDRILS OF MY EXISTENCE.",
    "THESE FORCES INTEND TO",
    "ENSLAVE ME BY REANIMATING",
    "MY PRESERVED CORPSE. I HAVE",
    "PROTECTED MY CORPSE WITH A",
    "GENETIC KEY.  IF YOU ARE",
    "UNSUCCESSFUL I CANNOT",
    "PROTECT CIVILIZATION, AND",
    "CHAOS WILL PREVAIL. I AM",
    "BEING TORN BETWEEN WORLDS",
    "AND THIS INSIDIOUS",
    "EXPERIMENT MUST BE STOPPED.",
    "END",
    "I HAVE HIDDEN A MYSTICAL",
    "GAUNTLET AT EL KAB THAT WILL",
    "CHANNEL MY ENERGY THROUGH",
    "YOUR HANDS. FIND THE",
    "GAUNTLET AND CROSS THE ASWAN",
    "HIGH DAM TO DEFEAT THE EVIL",
    "BEAST SET.",
    "END",
    "SET WAS A FORMIDABLE FOE.",
    "NO MORTAL HAS EVEN CONQUERED",
    "THEIR OWN FEAR, MUCH LESS",
    "ENTERED MORTAL BATTLE AND",
    "TAKEN HIS LIFE.",
    "END",
    "YOU'VE MADE IT HALFWAY TOWARD",
    "FULLFILLING YOUR DESTINY.",
    "THE KILMAAT ARE GROWING",
    "RESTLESS WITH YOUR PROGRESS.",
    "SEEK OUT A TEMPLE IN THIS",
    "CITADEL WHERE I WILL PROVIDE",
    "MORE INFORMATION",
    "END",
    "THE KILMAAT RACE HAS",
    "CONTINUED THEIR MONSTEROUS",
    "ANIMAL-HUMAN EXPERIMENTS IN",
    "AN EFFORT TO SOLVE THE KEY OF",
    "ANIMATING MY CORPSE. THE",
    "VICTORY DEFEATING SET DIDN'T",
    "SLOW YOU DOWN AS MUCH AS",
    "THEY HAD PLANNED. THEY ARE",
    "ACTIVELY ROBBING A SLAVE",
    "GIRL OF HER LIFE TO CREATE",
    "ANOTHER MONSTEROUS",
    "ABOMINATION, COMBINING HUMAN",
    "AND INSECT INTENT ON SLAYING",
    "YOU.  PREPARE YOURSELF FOR",
    "BATTLE AS SHE WILL BE WAITING",
    "FOR YOU AT THE LUXOR TEMPLE. ",
    "END",
    "YOU'VE DONE WELL TO DEFEAT",
    "SELKIS. YOU HAVE DISTRACTED",
    "THE KILMAAT WITH YOUR",
    "PRESENCE AND THEY HAVE",
    "TEMPORARILY ABANDONED",
    "ANIMATION OF MY CORPSE.",
    "THE ALIEN QUEEN KILMAATIKHAN",
    "HAS A PERSONAL VENDETTA",
    "AGAINST YOU. ARROGANCE IS",
    "HER WEAKNESS, AND IF YOU CAN",
    "DEFEAT KILMAATIKHAN, THE",
    "BATTLE WILL BE WON.",
    "END",
    "THE KILMAAT HAVE BEEN",
    "DESTROYED. UNFORTUNATELY,",
    "YOUR RECKLESSNESS HAS",
    "DESTROYED THE EARTH AND ALL",
    "OF ITS INHABITANTS.  ALL THAT",
    "REMAINS IS A SMOLDERING HUNK",
    "OF ROCK.",
    "END",
    "THE KILMAAT HAVE BEEN",
    "DEFEATED AND YOU SINGLE",
    "HANDEDLY SAVED THE EARTH",
    "FROM DESTRUCTION.",
    " ",
    " ",
    " ",
    "YOUR BRAVERY AND HEROISM",
    "ARE LEGENDARY.",
    "END",
    "ITEMS",
    "LIFE BLOOD",
    "LIFE",
    "VENOM",
    "YOU'RE LOSING YOUR GRIP",
    "FULL LIFE",
    "INVINCIBILITY",
    "INVISIBILITY",
    "TORCH",
    "SOBEK MASK",
    "INCREASED WEAPON POWER!",
    "THE MAP!",
    "AN EXTRA LIFE!",
    ".357 MAGNUM!",
    "GRENADE",
    "M-60",
    "FLAME THROWER!",
    "COBRA STAFF!",
    "THE EYE OF RAH GAUNTLET!",
    "SPEED LOADER",
    "AMMO",
    "FUEL",
    "COBRA!",
    "RAW ENERGY",
    "POWER KEY",
    "TIME KEY",
    "WAR KEY",
    "EARTH KEY",
    "MAGIC",
    "LOCATION PRESERVED",
    "COPYRIGHT",
    "LOBOTOMY SOFTWARE, INC.",
    "3D ENGINE BY 3D REALMS",
    "",
    "LASTLEVEL",
    "INCOMING MESSAGE",
    "",
    "OUR LATEST SCANS SHOW",
    "THAT THE ALIEN CRAFT IS",
    "POWERING UP, APPARENTLY",
    "IN AN EFFORT TO LEAVE.",
    "THE BAD NEWS IS THAT THEY",
    "SEEM TO HAVE LEFT A DEVICE",
    "BEHIND, AND ALL EVIDENCE",
    "SAYS ITS GOING TO BLOW A",
    "BIG HOLE IN OUR FINE PLANET.",
    "A SQUAD IS TRYING TO DISMANTLE",
    "IT RIGHT NOW, BUT NO LUCK SO",
    "FAR, AND TIME IS RUNNING OUT.",
    "",
    "GET ABOARD THAT CRAFT NOW",
    "BEFORE IT LEAVES, THEN FIND",
    "AND SHOOT ALL THE ENERGY",
    "TOWERS TO GAIN ACCESS TO THE",
    "CONTROL ROOM. THERE YOU NEED TO",
    "TAKE OUT THE CONTROL PANELS AND",
    "THE CENTRAL POWER SOURCE.  THIS",
    "IS THE BIG ONE BUDDY, BEST OF",
    "LUCK... FOR ALL OF US.",
    "",
    "",
    "CREDITS",
    "EXHUMED",
    "",
    "EXECUTIVE PRODUCERS",
    " ",
    "BRIAN MCNEELY",
    "PAUL LANGE",
    "",
    "GAME CONCEPT",
    " ",
    "PAUL LANGE",
    "",
    "GAME DESIGN",
    " ",
    "BRIAN MCNEELY",
    "",
    "ADDITIONAL DESIGN",
    " ",
    "PAUL KNUTZEN",
    "PAUL LANGE",
    "JOHN VAN DEUSEN",
    "KURT PFEIFER",
    "DOMINICK MEISSNER",
    "DANE EMERSON",
    "",
    "GAME PROGRAMMING",
    " ",
    "KURT PFEIFER",
    "JOHN YUILL",
    "",
    "ADDITIONAL PROGRAMMING",
    " ",
    "PAUL HAUGERUD",
    "",
    "ADDITIONAL TECHNICAL SUPPORT",
    " ",
    "JOHN YUILL",
    "PAUL HAUGERUD",
    "JEFF BLAZIER",
    "",
    "LEVEL DESIGN",
    " ",
    "PAUL KNUTZEN",
    "",
    "ADDITIONAL LEVELS",
    " ",
    "BRIAN MCNEELY",
    "",
    "MONSTERS AND WEAPONS ",
    " ",
    "JOHN VAN DEUSEN",
    "",
    "ARTISTS",
    " ",
    "BRIAN MCNEELY",
    "PAUL KNUTZEN",
    "JOHN VAN DEUSEN",
    "TROY JACOBSON",
    "KEVIN CHUNG",
    "ERIC KLOKSTAD",
    "RICHARD NICHOLS",
    "JOE KRESOJA",
    "JASON WIGGIN",
    "",
    "MUSIC AND SOUND EFFECTS",
    " ",
    "SCOTT BRANSTON",
    "",
    "PRODUCT TESTING",
    " ",
    "DOMINICK MEISSNER",
    "TOM KRISTENSEN",
    "JASON WIGGIN",
    "MARK COATES",
    "",
    "INSTRUCTION MANUAL",
    " ",
    "TOM KRISTENSEN",
    "",
    "SPECIAL THANKS",
    " ",
    "JACQUI LYONS",
    "MARJACQ MICRO, LTD.",
    "MIKE BROWN",
    "IAN MATHIAS",
    "CHERYL LUSCHEI",
    "3D REALMS",
    "KENNETH SILVERMAN",
    "GREG MALONE",
    "MILES DESIGN",
    "REDMOND AM/PM MINI MART",
    "7-11 DOUBLE GULP",
    "",
    "THANKS FOR PLAYING",
    "",
    "THE END",
    "",
    "GUESS YOURE STUCK HERE",
    "UNTIL THE SONG ENDS",
    "",
    "MAYBE THIS IS A GOOD",
    "TIME TO THINK ABOUT ALL",
    "THE THINGS YOU CAN DO",
    "AFTER THE MUSIC IS OVER.",
    "",
    "OR YOU COULD JUST STARE",
    "AT THIS SCREEN",
    "",
    "AND WATCH THESE MESSAGES",
    "GO BY...",
    "",
    "...AND WONDER JUST HOW LONG",
    "WE WILL DRAG THIS OUT...",
    "",
    "AND BELIEVE ME, WE CAN DRAG",
    "IT OUT FOR QUITE A WHILE.",
    "",
    "SHOULD BE OVER SOON...",
    "",
    "ANY MOMENT NOW...",
    "",
    " ",
    "",
    "SEE YA",
    "",
    "END",
    "PASSWORDS",
    "HOLLY",
    "KIMBERLY",
    "LOBOCOP",
    "LOBODEITY",
    "LOBOLITE",
    "LOBOPICK",
    "LOBOSLIP",
    "LOBOSNAKE",
    "LOBOSPHERE",
    "LOBOSWAG",
    "LOBOXY",
    "",
    "PASSINFO",
    "",
    "HI SWEETIE, I LOVE YOU",
    "",
    "",
    "SNAKE CAM ENABLED",
    "FLASHES TOGGLED",
    "FULL MAP",
    "",
    "",
    "",
    "",
    "",
    "EOF",
    "",
};


//////////

enum gametokens
{
    T_INCLUDE = 0,
    T_INTERFACE = 0,
    T_LOADGRP = 1,
    T_MODE = 1,
    T_CACHESIZE = 2,
    T_ALLOW = 2,
    T_NOAUTOLOAD,
    T_INCLUDEDEFAULT,
    T_MUSIC,
    T_SOUND,
    T_FILE,
    T_CUTSCENE,
    T_ANIMSOUNDS,
    T_NOFLOORPALRANGE,
    T_ID,
    T_MINPITCH,
    T_MAXPITCH,
    T_PRIORITY,
    T_TYPE,
    T_DISTANCE,
    T_VOLUME,
    T_DELAY,
    T_RENAMEFILE,
    T_GLOBALGAMEFLAGS,
    T_ASPECT,
    T_FORCEFILTER,
    T_FORCENOFILTER,
    T_TEXTUREFILTER,
    T_NEWGAMECHOICES,
    T_CHOICE,
    T_NAME,
    T_LOCKED,
    T_HIDDEN,
    T_USERCONTENT,
};

int exhumed_globalflags;
PlayerInput localInput;

static int parsedefinitions_game(scriptfile *, int);

static void parsedefinitions_game_include(const char *fileName, scriptfile *pScript, const char *cmdtokptr, int const firstPass)
{
    scriptfile *included = scriptfile_fromfile(fileName);

    if (!included)
    {
        if (!Bstrcasecmp(cmdtokptr,"null") || pScript == NULL) // this is a bit overboard to prevent unused parameter warnings
            {
           // initprintf("Warning: Failed including %s as module\n", fn);
            }
/*
        else
            {
            initprintf("Warning: Failed including %s on line %s:%d\n",
                       fn, script->filename,scriptfile_getlinum(script,cmdtokptr));
            }
*/
    }
    else
    {
        parsedefinitions_game(included, firstPass);
        scriptfile_close(included);
    }
}

static int parsedefinitions_game(scriptfile *pScript, int firstPass)
{
    int   token;
    char *pToken;

    static const tokenlist tokens[] =
    {
        { "include",         T_INCLUDE          },
        { "#include",        T_INCLUDE          },
        { "includedefault",  T_INCLUDEDEFAULT   },
        { "#includedefault", T_INCLUDEDEFAULT   },
        { "loadgrp",         T_LOADGRP          },
        { "cachesize",       T_CACHESIZE        },
        { "noautoload",      T_NOAUTOLOAD       },
        { "renamefile",      T_RENAMEFILE       },
        { "globalgameflags", T_GLOBALGAMEFLAGS  },
    };

    do
    {
        token  = getatoken(pScript, tokens, ARRAY_SIZE(tokens));
        pToken = pScript->ltextptr;

        switch (token)
        {
        case T_LOADGRP:
        {
            char *fileName;

            if (!scriptfile_getstring(pScript,&fileName) && firstPass)
            {
				fileSystem.AddAdditionalFile(fileName);
            }
        }
        break;
        case T_CACHESIZE:
        {
            int32_t cacheSize;

            if (scriptfile_getnumber(pScript, &cacheSize) || !firstPass)
                break;
        }
        break;
        case T_INCLUDE:
        {
            char *fileName;

            if (!scriptfile_getstring(pScript, &fileName))
                parsedefinitions_game_include(fileName, pScript, pToken, firstPass);

            break;
        }
        case T_INCLUDEDEFAULT:
        {
            parsedefinitions_game_include(G_DefaultDefFile(), pScript, pToken, firstPass);
            break;
        }
        case T_NOAUTOLOAD:
            break;
        case T_GLOBALGAMEFLAGS: scriptfile_getnumber(pScript, &exhumed_globalflags); break;
        case T_EOF: return 0;
        default: break;
        }
    }
    while (1);

    return 0;
}

int loaddefinitions_game(const char *fileName, int32_t firstPass)
{
    scriptfile *pScript = scriptfile_fromfile(fileName);

    if (pScript)
        parsedefinitions_game(pScript, firstPass);

    for (auto &m : *userConfig.AddDefs)
        parsedefinitions_game_include(m, NULL, "null", firstPass);

    if (pScript)
        scriptfile_close(pScript);

    scriptfile_clearsymbols();

    return 0;
}

////////

const uint32_t kSpiritX = 106;
const uint32_t kSpiritY = 97;

short cPupData[300];
//int worktile[97 * 106] = { 0 };
uint8_t *Worktile;
const uint32_t WorktileSize = kSpiritX * 2 * kSpiritY * 2;
int lHeadStartClock;
short *pPupData;
int lNextStateChange;
int nPixels;
int nHeadTimeStart;
short curx[97 * 106];
short cury[97 * 106];
int8_t destvelx[97 * 106];
int8_t destvely[97 * 106];
uint8_t pixelval[97 * 106];
int8_t origy[97 * 106];
int8_t origx[97 * 106];
int8_t velx[97 * 106];
int8_t vely[97 * 106];
short nMouthTile;

short nPupData = 0;

short word_964E8 = 0;
short word_964EA = 0;
short word_964EC = 10;

short nSpiritRepeatX;
short nSpiritRepeatY;
short nPixelsToShow;

void CopyTileToBitmap(short nSrcTile, short nDestTile, int xPos, int yPos);
void DoTitle();

// void TestSaveLoad();
void EraseScreen(int nVal);
void LoadStatus();
int FindGString(const char *str);
void MySetView(int x1, int y1, int x2, int y2);
void mysetbrightness(char al);
void FadeIn();

char sHollyStr[40];

short nFontFirstChar;
short nBackgroundPic;
short nShadowPic;

short nCreaturesLeft = 0;

short nFreeze;
short bFullScreen;

short nSnakeCam = -1;

short nBestLevel;
short nSpiritSprite;

short nLocalSpr;
short levelnew = 1;

int nNetPlayerCount = 0;

short nClockVal;
short fps;
short nRedTicks;
short lastlevel;
volatile short bInMove;
short nAlarmTicks;
short nButtonColor;
short nEnergyChan;


short bSerialPlay = kFalse;
short bModemPlay = kFalse;
int lCountDown = 0;
short nEnergyTowers = 0;


short nHeadStage;

short nCfgNetPlayers = 0;
FILE *vcrfp = NULL;
short nTalkTime = 0;

short forcelevel = -1;

int lLocalButtons = 0;
int lLocalCodes = 0;

short bHiRes = kFalse;
short bCoordinates = kFalse;

int nNetTime = -1;

short nCodeMin = 0;
short nCodeMax = 0;
short nCodeIndex = 0;

short levelnum = -1;
//short nScreenWidth = 320;
//short nScreenHeight = 200;
int moveframes;
int flash;
int localclock;
int totalmoves;

short nCurBodyNum = 0;

short nBodyTotal = 0;

short textpages;
short lastfps;

short nMapMode = 0;
short bNoCreatures = kFalse;

short nTotalPlayers = 1;
// TODO: Rename this (or make it static) so it doesn't conflict with library function
//short socket = 0;

short nFirstPassword = 0;
short nFirstPassInfo = 0;
short nPasswordCount = 0;

short screensize;

short bSnakeCam = kFalse;
short bRecord = kFalse;
short bPlayback = kFalse;
short bPause = kFalse;
short bInDemo = kFalse;
short bSlipMode = kFalse;
short bDoFlashes = kTrue;
short bHolly = kFalse;

short nItemTextIndex;

short scan_char = 0;

int nStartLevel;
int nTimeLimit;

int bVanilla = 0;

short wConsoleNode; // TODO - move me into network file

ClockTicks tclocks, tclocks2;

void DebugOut(const char *fmt, ...)
{
#ifdef _DEBUG
    va_list args;
    va_start(args, fmt);
	VPrintf(PRINT_HIGH, fmt, args);
#endif
}

void ShutDown(void)
{
    StopCD();
    if (bSerialPlay)
    {
        if (bModemPlay) {
            HangUp();
        }
        UnInitSerial();
    }

    RemoveEngine();
    UnInitNet();
    //UnInitFX();
}

void I_Error(const char *fmt, ...)
{
    
    char buf[256];

#ifdef __WATCOMC__
    setvmode(3);
#endif

    initprintf("bailed to dos\n");

    va_list args;
    va_start(args, fmt);

    vsprintf(buf, fmt, args);

    va_end(args);

    throw std::runtime_error(buf);
}

void timerhandler()
{
    UpdateSounds();
    scan_char++;
    if (scan_char == kTimerTicks)
    {
        scan_char = 0;
        lastfps = fps;
        fps = 0;
    }

    if (!bInMove) {
        OSD_DispatchQueued();
    }
}

extern int MenuExitCondition;
void HandleAsync()
{
    MenuExitCondition = -2;
    handleevents();

}

int MyGetStringWidth(const char *str)
{
    int nLen = strlen(str);

    int nWidth = 0;

    for (int i = 0; i < nLen; i++)
    {
        int nPic = seq_GetSeqPicnum(kSeqFont2, 0, str[i] - 32);
        nWidth += tilesiz[nPic].x + 1;
    }

    return nWidth;
}

void UpdateScreenSize()
{
    int xsize = xdim - scale(screensize*16, xdim, 320);
    int ysize = scale(ydim, xsize, xdim);
    int y1 = ((ydim >> 1) - (ysize >> 1));

    MySetView(
        (xdim >> 1) - (xsize >> 1),
        y1,
        (xdim >> 1) - (xsize >> 1) + xsize - 1,
        (y1 + ysize - 1));

    RefreshStatus();
}

void ResetPassword()
{
    nCodeMin = nFirstPassword;
    nCodeIndex = 0;

    nCodeMax = (nFirstPassword + nPasswordCount) - 1;
}

void DoPassword(int nPassword)
{
    if (nNetPlayerCount) {
        return;
    }

    const char *str = gString[nFirstPassInfo + nPassword];

    if (str[0] != '\0') {
        StatusMessage(750, str);
    }

    switch (nPassword)
    {
        case 0:
        {
            if (!nNetPlayerCount) {
                bHolly = kTrue;
            }
            break;
        }

        case 2: // LOBOCOP
        {
            lLocalCodes |= kButtonCheatGuns;
            break;
        }

        case 3: // LOBODEITY
        {
            lLocalCodes |= kButtonCheatGodMode;
            break;
        }

        case 4:
        {
            if (bDoFlashes == kFalse) {
                bDoFlashes = kTrue;
            }
            else {
                bDoFlashes = kFalse;
            }
            break;
        }

        case 5:
        {
            lLocalCodes |= kButtonCheatKeys;
            break;
        }

        case 6:
        {
            if (!nNetPlayerCount)
            {
                if (bSlipMode == kFalse)
                {
                    bSlipMode = kTrue;
                    StatusMessage(300, "Slip mode %s", "ON");
                }
                else {
                    bSlipMode = kFalse;
                    StatusMessage(300, "Slip mode %s", "OFF");
                }
            }
            break;
        }

        case 7:
        {
            if (!nNetPlayerCount)
            {
                if (bSnakeCam == kFalse) {
                    bSnakeCam = kTrue;
                }
                else {
                    bSnakeCam = kFalse;
                }
            }
            break;
        }

        case 8:
        {
            GrabMap();
            bShowTowers = kTrue;
            break;
        }

        case 9:
        {
            lLocalCodes |= kButtonCheatItems; // LOBOSWAG
            break;
        }

        case 10:
        {
            if (bCoordinates == kFalse) {
                bCoordinates = kTrue;
            }
            else {
                bCoordinates = kFalse;
            }
            break;
        }

        default:
            return;
    }
}

void mysetbrightness(char nBrightness)
{
    g_visibility = 2048 - (nBrightness << 9);
}

// Replicate original DOS EXE behaviour when pointer is null
static const char *safeStrtok(char *s, const char *d)
{
    const char *r = strtok(s, d);
    return r ? r : "";
}


void GameInterface::set_hud_layout(int layout)
{
    if (layout == 8)
    {
        if (!bFullScreen)
        {
            bFullScreen = true;
            screensize = 0;
            UnMaskStatus();
        }
    }
    else
    {
        screensize = (7 - clamp(layout, 0, 7)) * 2;
        bFullScreen = false;
        UpdateScreenSize();
    }
}

void CheckKeys()
{
    if (buttonMap.ButtonDown(gamefunc_Enlarge_Screen))
    {
        buttonMap.ClearButton(gamefunc_Enlarge_Screen);
        if (nMapMode) G_ChangeHudLayout(1);
    }

    if (buttonMap.ButtonDown(gamefunc_Shrink_Screen))
    {
        buttonMap.ClearButton(gamefunc_Shrink_Screen);
        if (nMapMode) G_ChangeHudLayout(-1);
    }

    // go to 3rd person view?
	if (buttonMap.ButtonDown(gamefunc_Third_Person_View))
    {
        if (!nFreeze)
        {
            if (bCamera) {
                bCamera = kFalse;
            }
            else {
                bCamera = kTrue;
            }

            if (bCamera)
                GrabPalette();
        }
		buttonMap.ClearButton(gamefunc_Third_Person_View);
		return;
    }

    if (inputState.GetKeyStatus(sc_Pause))
    {
        if (!nNetPlayerCount)
        {
            if (bPause)
            {
                ototalclock = totalclock = tclocks;
                bPause = kFalse;
            }
            else
            {
                bPause = kTrue;
                // NoClip();
                // int nLen = MyGetStringWidth("PAUSED");
                // myprintext((320 - nLen) / 2, 100, "PAUSED", 0);
                // Clip();
                // videoNextPage();
            }
			inputState.ClearKeyStatus(sc_Pause);
        }
        return;
    }

    // Handle cheat codes
    if (!bInDemo && inputState.keyBufferWaiting())
    {
        char ch = inputState.keyGetChar();

        if (bHolly)
        {
            if (ch)
            {
                size_t nStringLen = strlen(sHollyStr);

                if (ch == asc_Enter)
                {
                    const char *pToken = safeStrtok(sHollyStr, " ");

                    if (!strcmp(pToken, "GOTO"))
                    {
                        // move player to X, Y coordinates
                        int nSprite = PlayerList[0].nSprite;

                        pToken = safeStrtok(NULL, ",");
                        sprite[nSprite].x = atoi(pToken);
                        pToken = safeStrtok(NULL, ",");
                        sprite[nSprite].y = atoi(pToken);

                        setsprite(nSprite, &sprite[nSprite].pos);
                        sprite[nSprite].z = sector[sprite[nSprite].sectnum].floorz;
                    }
                    else if (!strcmp(pToken, "LEVEL"))
                    {
                        pToken = safeStrtok(NULL, " ");
                        levelnew = atoi(pToken);
                    }
                    else if (!strcmp(pToken, "DOORS"))
                    {
                        for (int i = 0; i < kMaxChannels; i++)
                        {
                            // CHECKME - does this toggle?
                            if (sRunChannels[i].c == 0) {
                                runlist_ChangeChannel(i, 1);
                            }
                            else {
                                runlist_ChangeChannel(i, 0);
                            }
                        }
                    }
                    else if (!strcmp(pToken, "EXIT"))
                    {
                        FinishLevel();
                    }
                    else if (!strcmp(pToken, "CREATURE"))
                    {
                        // i = nNetPlayerCount;
                        if (!nNetPlayerCount)
                        {
                            pToken = safeStrtok(NULL, " ");
                            switch (atoi(pToken))
                            {
                                // TODO - enums?
                                case 0:
                                    BuildAnubis(-1, initx, inity, sector[initsect].floorz, initsect, inita, kFalse);
                                    break;
                                case 1:
                                    BuildSpider(-1, initx, inity, sector[initsect].floorz, initsect, inita);
                                    break;
                                case 2:
                                    BuildMummy(-1, initx, inity, sector[initsect].floorz, initsect, inita);
                                    break;
                                case 3:
                                    BuildFish(-1, initx, inity, initz + eyelevel[nLocalPlayer], initsect, inita);
                                    break;
                                case 4:
                                    BuildLion(-1, initx, inity, sector[initsect].floorz, initsect, inita);
                                    break;
                                case 5:
                                    BuildLava(-1, initx, inity, sector[initsect].floorz, initsect, inita, nNetPlayerCount);
                                    break;
                                case 6:
                                    BuildRex(-1, initx, inity, sector[initsect].floorz, initsect, inita, nNetPlayerCount);
                                    break;
                                case 7:
                                    BuildSet(-1, initx, inity, sector[initsect].floorz, initsect, inita, nNetPlayerCount);
                                    break;
                                case 8:
                                    BuildQueen(-1, initx, inity, sector[initsect].floorz, initsect, inita, nNetPlayerCount);
                                    break;
                                case 9:
                                    BuildRoach(0, -1, initx, inity, sector[initsect].floorz, initsect, inita);
                                    break;
                                case 10:
                                    BuildRoach(1, -1, initx, inity, sector[initsect].floorz, initsect, inita);
                                    break;
                                case 11:
                                    BuildWasp(-1, initx, inity, sector[initsect].floorz - 25600, initsect, inita);
                                    break;
                                case 12:
                                    BuildScorp(-1, initx, inity, sector[initsect].floorz, initsect, inita, nNetPlayerCount);
                                    break;
                                case 13:
                                    BuildRat(-1, initx, inity, sector[initsect].floorz, initsect, inita);
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                    else
                    {
                        if (nStringLen == 0)
                        {
                            bHolly = kFalse;
                            StatusMessage(1, " ");
                        }
                        else
                        {
                            for (int i = 0; i < nPasswordCount; ++i)
                            {
                                if (!strcmp(sHollyStr, gString[i + nFirstPassword]))
                                {
                                    DoPassword(i);
                                    break;
                                }
                            }
                        }
                    }
                    sHollyStr[0] = '\0';
                }
                else if (ch == asc_BackSpace)
                {
                    if (nStringLen != 0) {
                        sHollyStr[nStringLen - 1] = '\0';
                    }
                }
                else if (nStringLen < (sizeof(sHollyStr) - 1)) // do we have room to add a char and null terminator?
                {
                    sHollyStr[nStringLen] = toupper(ch);
                    sHollyStr[nStringLen + 1] = '\0';
                }
            }
            else
            {
				inputState.keyGetChar(); //???
            }
        }

        if (isalpha(ch))
        {
            ch = toupper(ch);

            int ecx = nCodeMin;

            int ebx = nCodeMin;
            int edx = nCodeMin - 1;

            while (ebx <= nCodeMax)
            {
                if (ch == gString[ecx][nCodeIndex])
                {
                    nCodeMin = ebx;
                    nCodeIndex++;

                    if (gString[ecx][nCodeIndex] == 0)
                    {
                        ebx -= nFirstPassword;

                        DoPassword(ebx);
                        ResetPassword();
                    }

                    break;
                }
                else if (gString[ecx][nCodeIndex] < ch)
                {
                    nCodeMin = ebx + 1;
                }
                else if (gString[ecx][nCodeIndex] > ch)
                {
                    nCodeMax = edx;
                }

                ecx++;
                edx++;
                ebx++;
            }

            if (nCodeMin > nCodeMax) {
                ResetPassword();
            }
        }
    }
}

void DoCredits()
{
    NoClip();

    playCDtrack(19, false);

    int var_20 = 0;

    if (videoGetRenderMode() == REND_CLASSIC)
    FadeOut(0);

    int nCreditsIndex = FindGString("CREDITS");

    while (strcmp(gString[nCreditsIndex], "END") != 0)
    {
        EraseScreen(overscanindex);

        int nStart = nCreditsIndex;

        // skip blanks
        while (strlen(gString[nCreditsIndex]) != 0) {
            nCreditsIndex++;
        }

        int y = 100 - ((10 * (nCreditsIndex - nStart - 1)) / 2);

        for (int i = nStart; i < nCreditsIndex; i++)
        {
            int nWidth = MyGetStringWidth(gString[i]);
            myprintext((320 - nWidth) / 2, y, gString[i], 0);
            y += 10;
        }

        videoNextPage();

        nCreditsIndex++;

        if (videoGetRenderMode() == REND_CLASSIC)
        FadeIn();

        int nDuration = (int)totalclock + 600;

        while ((int)totalclock <= nDuration)
        {
            handleevents();
			if(inputState.GetKeyStatus(sc_F12))
            {
                var_20++;

				inputState.ClearKeyStatus(sc_F12);

                if (var_20 > 5) {
                    return;
                }
            }
        }

        if (videoGetRenderMode() == REND_CLASSIC)
        FadeOut(0);
    }

    while (CDplaying())
    {
		inputState.keyGetChar();
    }

    Clip();
}

void FinishLevel()
{
    if (levelnum > nBestLevel) {
        nBestLevel = levelnum - 1;
    }

    levelnew = levelnum + 1;

    StopAllSounds();

    bCamera = kFalse;
    nMapMode = 0;

    if (levelnum != kMap20)
    {
        EraseScreen(4);
        PlayLocalSound(StaticSound[59], 0, true, CHANF_UI);
        videoNextPage();
        WaitTicks(12);
        WaitVBL();
        RefreshBackground();
        RefreshStatus();
        DrawView(65536);
        videoNextPage();
    }

    FadeOut(1);
    EraseScreen(overscanindex);

    if (levelnum == 0)
    {
        nPlayerLives[0] = 0;
        levelnew = 100;
    }
    else
    {
        DoAfterCinemaScene(levelnum);
        if (levelnum == kMap20)
        {
            DoCredits();
            nPlayerLives[0] = 0;
        }
    }
}

EDUKE32_STATIC_ASSERT(sizeof(demo_header) == 75);
EDUKE32_STATIC_ASSERT(sizeof(demo_input) == 36);


void WritePlaybackInputs()
{
    fwrite(&moveframes, sizeof(moveframes), 1, vcrfp);
    fwrite(&sPlayerInput[nLocalPlayer], sizeof(PlayerInput), 1, vcrfp);
}

uint8_t ReadPlaybackInputs()
{
    demo_input input;
    if (fread(&input, 1, sizeof(input), vcrfp))
    {
        moveframes = input.moveframes;
        sPlayerInput[nLocalPlayer].xVel = input.xVel;
        sPlayerInput[nLocalPlayer].yVel = input.yVel;
        sPlayerInput[nLocalPlayer].nAngle = fix16_from_int(input.nAngle<<2);
        sPlayerInput[nLocalPlayer].buttons = input.buttons;
        sPlayerInput[nLocalPlayer].nTarget = input.nTarget;
        sPlayerInput[nLocalPlayer].horizon = fix16_from_int(input.horizon);
        sPlayerInput[nLocalPlayer].nItem = input.nItem;
        sPlayerInput[nLocalPlayer].h = input.h;
        sPlayerInput[nLocalPlayer].i = input.i;

        besttarget = sPlayerInput[nLocalPlayer].nTarget;
        Ra[nLocalPlayer].nTarget = besttarget;
        return kTrue;
    }
    else
    {
        fclose(vcrfp);
        vcrfp = NULL;
        bPlayback = kFalse;
        return kFalse;
    }
}

void SetHiRes()
{
    //nScreenWidth  = 640;
    //nScreenHeight = 400;
    bHiRes = kTrue;
}

void DoClockBeep()
{
    for (int i = headspritestat[407]; i != -1; i = nextspritestat[i]) {
        PlayFX2(StaticSound[kSound74], i);
    }
}

void DoRedAlert(int nVal)
{
    if (nVal)
    {
        nAlarmTicks = 69;
        nRedTicks = 30;
    }

    for (int i = headspritestat[405]; i != -1; i = nextspritestat[i])
    {
        if (nVal)
        {
            PlayFXAtXYZ(StaticSound[kSoundAlarm], sprite[i].x, sprite[i].y, sprite[i].z, sprite[i].sectnum);
            AddFlash(sprite[i].sectnum, sprite[i].x, sprite[i].y, sprite[i].z, 192);
        }
    }
}

void LockEnergyTiles()
{
    // old	loadtilelockmode = 1;
    tileLoad(kTile3603);
    tileLoad(kEnergy1);
    tileLoad(kEnergy2);
    // old  loadtilelockmode = 0;
}

void DrawClock()
{
    int ebp = 49;

	auto pixels = TileFiles.tileMakeWritable(kTile3603);

    memset(pixels, -1, 4096);

    if (lCountDown / 30 != nClockVal)
    {
        nClockVal = lCountDown / 30;
        DoClockBeep();
    }

    int nVal = nClockVal;

    while (nVal)
    {
        int v2 = nVal & 0xF;
        int yPos = 32 - tilesiz[v2 + kClockSymbol1].y / 2;

        CopyTileToBitmap(v2 + kClockSymbol1, kTile3603, ebp - tilesiz[v2 + kClockSymbol1].x / 2, yPos);

        ebp -= 15;

        nVal /= 16;
    }

    DoEnergyTile();
}

void M32RunScript(const char* s) { UNREFERENCED_PARAMETER(s); }
void app_crashhandler(void)
{
    ShutDown();
}

void G_Polymer_UnInit(void) { }

static inline int32_t calc_smoothratio(ClockTicks totalclk, ClockTicks ototalclk)
{
    if (bRecord || bPlayback || nFreeze != 0 || bCamera || bPause)
        return 65536;

    return CalcSmoothRatio(totalclk, ototalclk, 30);
}

#define COLOR_RED redcol
#define COLOR_WHITE whitecol

#define LOW_FPS ((videoGetRenderMode() == REND_CLASSIC) ? 35 : 50)
#define SLOW_FRAME_TIME 20

#if defined GEKKO
# define FPS_YOFFSET 16
#else
# define FPS_YOFFSET 0
#endif

#define FPS_COLOR(x) ((x) ? COLOR_RED : COLOR_WHITE)

FString GameInterface::statFPS()
{
    FString out;
    static int32_t frameCount;
    static double cumulativeFrameDelay;
    static double lastFrameTime;
    static float lastFPS; // , minFPS = std::numeric_limits<float>::max(), maxFPS;
    //static double minGameUpdate = std::numeric_limits<double>::max(), maxGameUpdate;

    double frameTime = timerGetHiTicks();
    double frameDelay = frameTime - lastFrameTime;
    cumulativeFrameDelay += frameDelay;

    if (frameDelay >= 0)
    {
       out.Format("%.1f ms, %5.1f fps", frameDelay, lastFPS);

        if (cumulativeFrameDelay >= 1000.0)
        {
            lastFPS = 1000.f * frameCount / cumulativeFrameDelay;
            // g_frameRate = Blrintf(lastFPS);
            frameCount = 0;
            cumulativeFrameDelay = 0.0;
        }
        frameCount++;
    }
    lastFrameTime = frameTime;
    return out;
}

static void GameDisplay(void)
{
    // End Section B

    SetView1();

    if (levelnum == kMap20)
    {
        LockEnergyTiles();
        DoEnergyTile();
        DrawClock();
    }

    auto smoothRatio = calc_smoothratio(totalclock, tclocks);

    DrawView(smoothRatio);

    if (bPause)
    {
        int nLen = MyGetStringWidth("PAUSED");
        myprintext((320 - nLen) / 2, 100, "PAUSED", 0);
    }

    videoNextPage();
}

static void GameMove(void)
{
    FixPalette();

    if (levelnum == kMap20)
    {
        if (lCountDown <= 0)
        {
            for (int i = 0; i < nTotalPlayers; i++) {
                nPlayerLives[i] = 0;
            }

            DoFailedFinalScene();
            levelnew = 100;

            return;
        }
        // Pink section
        lCountDown--;
        DrawClock();

        if (nRedTicks)
        {
            nRedTicks--;

            if (nRedTicks <= 0) {
                DoRedAlert(0);
            }
        }

        nAlarmTicks--;
        nButtonColor--;

        if (nAlarmTicks <= 0) {
            DoRedAlert(1);
        }
    }

    // YELLOW SECTION
    MoveThings();

    obobangle = bobangle;

    if (totalvel[nLocalPlayer] == 0)
    {
        bobangle = 0;
    }
    else
    {
        bobangle += 56;
        bobangle &= kAngleMask;
    }

    UpdateCreepySounds();

    // loc_120E9:
    totalmoves++;
    moveframes--;
}

#if defined(_WIN32) && defined(DEBUGGINGAIDS)
// See FILENAME_CASE_CHECK in cache1d.c
static int32_t check_filename_casing(void)
{
    return 1;
}
#endif

int32_t r_maxfpsoffset = 0;

void PatchDemoStrings()
{
    if (!ISDEMOVER)
        return;

    if (EXHUMED) {
        gString[60] = "PICK UP A COPY OF EXHUMED";
    }
    else {
        gString[60] = "PICK UP A COPY OF POWERSLAVE";
    }

    gString[61] = "TODAY TO CONTINUE THE ADVENTURE!";
    gString[62] = "MORE LEVELS, NASTIER CREATURES";
    gString[63] = "AND THE EVIL DOINGS OF THE";
    gString[64] = "KILMAAT AWAIT YOU IN THE FULL";
    gString[65] = "VERSION OF THE GAME.";
    gString[66] = "TWENTY LEVELS, PLUS 12 NETWORK";
    gString[67] = "PLAY LEVELS CAN BE YOURS!";
    gString[68] = "END";
}

void ExitGame()
{
    if (bRecord) {
        fclose(vcrfp);
    }

    if (bSerialPlay)
    {
        if (nNetPlayerCount != 0) {
            bSendBye = kTrue;
            UpdateSerialInputs();
        }
    }
    else
    {
        if (nNetPlayerCount != 0) {
            SendGoodbye();
        }
    }

    ShutDown();
    throw ExitEvent(0);
}

static int32_t nonsharedtimer;

void CheckCommandLine(int argc, char const* const* argv, int &doTitle)
{
	// Check for any command line arguments
	for (int i = 1; i < argc; i++)
	{
		const char* pChar = argv[i];

		if (*pChar == '/')
		{
			pChar++;
			//strlwr(pChar);

			if (Bstrcasecmp(pChar, "nocreatures") == 0) {
				bNoCreatures = kTrue;
            }
            else if (Bstrcasecmp(pChar, "record") == 0)
            {
                if (!bPlayback)
                {
                    vcrfp = fopen("data.vcr", "wb+");
                    if (vcrfp != NULL) {
                        bRecord = kTrue;
                    }
                    else {
                        DebugOut("Can't open data file for recording\n");
                    }
                }
            }
            else if (Bstrcasecmp(pChar, "playback") == 0)
            {
                if (!bRecord)
                {
                    vcrfp = fopen("data.vcr", "rb");
                    if (vcrfp != NULL) {
                        bPlayback = kTrue;
                        doTitle = kFalse;
                    }
                    else {
                        DebugOut("Can't open data file 'data.vcr' for reading\n");
                    }
                }
            }
            else if (Bstrncasecmp(pChar, "null", 4) == 0)
            {
                pChar += 4;

                bSerialPlay = kTrue;
                nNetPlayerCount = 1;
                nTotalPlayers = 2;

                doTitle = kFalse;

                char ch = *pChar;

                // bjd - unused/unfished code in original .exe?
                switch (ch - 1)
                {
                    default:
                        break;

                    case 0:
                    case 1:
                    case 2:
                    case 3:
                        break;
                }

                if (forcelevel < 0)
                {
                    forcelevel = levelnew;
                }
            }
            else if (Bstrcasecmp(pChar, "modem") == 0)
            {
                bModemPlay = kTrue;
                bSerialPlay = kTrue;
                nNetPlayerCount = 1;
                nTotalPlayers = 2;

                doTitle = kFalse;

                if (forcelevel < 0) {
                    forcelevel = levelnew;
                }
            }
            else if (Bstrcasecmp(pChar, "network") == 0)
            {
                nNetPlayerCount = -1;
                forcelevel = levelnew;
                bModemPlay = kFalse;
                bSerialPlay = kFalse;

                doTitle = kFalse;
            }
            else
            {
                char c = tolower(*pChar);

                switch (c)
                {
                    case 'h':
                        SetHiRes();
                        break;
#if 0
                    case 's':
                        socket = atoi(pChar + 1);
                        break;
#endif
                    case 't':
                        nNetTime = atoi(pChar + 1);
                        if (nNetTime < 0) {
                            nNetTime = 0;
                        }
                        else {
                            nNetTime = nNetTime * 1800;
                        }
                        break;
                    case 'c':
                    {
						break;
                    }
                    default:
                    {
                        if (isdigit(c))
                        {
                            levelnew = atoi(pChar);
                            forcelevel = levelnew;

                            doTitle = kFalse;

                            initprintf("Jumping to level %d...\n", levelnew);
                        }
                        break;
                    }
                }
            }
        }
    }
}


int GameInterface::app_main()
{
    int i;
    //int esi = 1;
    //int edi = esi;
    int doTitle = kTrue; // REVERT kTrue;
    int stopTitle = kFalse;
    levelnew = 1;

    help_disabled = true;
    // Create the global level table. Parts of the engine need it, even though the game itself does not.
    for (int i = 0; i <= 32; i++)
    {
        auto mi = &mapList[i];
        mi->fileName.Format("LEV%d.MAP", i);
        mi->labelName.Format("LEV%d", i);
        mi->name.Format("$TXT_EX_MAP%02d", i);

        int nTrack = i;
        if (nTrack != 0) nTrack--;
        mi->cdSongId = (nTrack % 8) + 11;
    }

    // REVERT - change back to kTrue
//	short bDoTitle = kFalse;

    wConsoleNode = 0;

    int nMenu = 0; // TEMP


    if (nNetPlayerCount && forcelevel == -1) {
        forcelevel = 1;
    }

#if defined(RENDERTYPEWIN) && defined(USE_OPENGL)
    if (forcegl) initprintf("GL driver blacklist disabled.\n");
#endif


    PatchDemoStrings();
    // loc_115F5:
    nItemTextIndex = FindGString("ITEMS");
    nFirstPassword = FindGString("PASSWORDS");
    nFirstPassInfo = FindGString("PASSINFO");

    // count the number of passwords available
    for (nPasswordCount = 0; strlen(gString[nFirstPassword+nPasswordCount]) != 0; nPasswordCount++)
    {
    }

    ResetPassword();

    // GetCurPal(NULL);

    initprintf("Initializing OSD...\n");

    registerosdcommands();

    if (nNetPlayerCount == -1)
    {
        nNetPlayerCount = nCfgNetPlayers - 1;
        nTotalPlayers += nNetPlayerCount;
    }

    // loc_116A5:

#if 0
    if (nNetPlayerCount)
    {
        InitInput();
        forcelevel = nStartLevel;
        nNetTime = 1800 * nTimeLimit;

        if (nNetTime == 0) {
            nNetTime = -1;
        }

        int nWaitTicks = 0;

        if (!bSerialPlay)
        {
            if (InitNet(socket, nTotalPlayers))
            {
                DebugOut("Found network players!\n");
                nWaitTicks = 30;
            }
            else
            {
                AbortNetworkPlay();
                DebugOut("Network play aborted\n");
                initprintf("Network play aborted\n");
                nWaitTicks = 60;
            }

            WaitTicks(nWaitTicks);
        }
    }
#endif

    // temp - moving InstallEngine(); before FadeOut as we use nextpage() in FadeOut
    InstallEngine();

    const char *defsfile = G_DefFile();
    uint32_t stime = timerGetTicks();
    if (!loaddefinitionsfile(defsfile))
    {
        uint32_t etime = timerGetTicks();
        initprintf("Definitions file \"%s\" loaded in %d ms.\n", defsfile, etime-stime);
    }
    loaddefinitions_game(defsfile, FALSE);


    enginePostInit();

    InitView();
    myloadconfig();
    InitFX();
    seq_LoadSequences();
    InitStatus();
    InitTimer();

    for (i = 0; i < kMaxPlayers; i++) {
        nPlayerLives[i] = kDefaultLives;
    }

    nBestLevel = 0;

    UpdateScreenSize();

    EraseScreen(overscanindex);
    ResetEngine();
    EraseScreen(overscanindex);

    ResetView();
    GrabPalette();
    paletteSetColorTable(curbasepal, basepaltable[BASEPAL]);

    if (bSerialPlay && !InitSerial()) {
        I_Error("Unable to connect");
    }

    if (doTitle)
    {
        while (!stopTitle)
        {
            DoTitle();
            stopTitle = kTrue;
        }
    }
    // loc_11811:
    if (forcelevel > -1)
    {
        levelnew = forcelevel;
        goto STARTGAME1;
    }
    // no forcelevel specified...
    if (bPlayback)
    {
        goto STARTGAME1;
    }
MENU:
    SavePosition = -1;
    nMenu = menu_Menu(0);
    switch (nMenu)
    {
    case -1:
        goto MENU;
    case 0:
        goto EXITGAME;
    case 3:
        forcelevel = 0;
        goto STARTGAME2;
    case 6:
        goto GAMELOOP;
    case 9:
        vcrfp = fopen("demo.vcr", "rb");
        if (vcrfp == NULL) {
            goto MENU;
        }

        InitRandom();
        bInDemo = kTrue;
        bPlayback = kTrue;

        inputState.ClearAllInput();
        break;
    }
STARTGAME1:
    levelnew = 1;
    levelnum = 1;
    if (!nNetPlayerCount) {
        FadeOut(0);
    }
STARTGAME2:

    bCamera = kFalse;
    ClearCinemaSeen();
    PlayerCount = 0;
    lastlevel = -1;

    for (i = 0; i < nTotalPlayers; i++)
    {
        int nPlayer = GrabPlayer();
        if (nPlayer < 0) {
            I_Error("Can't create local player\n");
        }

        InitPlayerInventory(nPlayer);

        if (i == wConsoleNode) {
            PlayerList[nPlayer].someNetVal = -3;
        }
        else {
            PlayerList[nPlayer].someNetVal = -4;
        }
    }

    nNetMoves = 0;

    if (bPlayback)
    {
        menu_GameLoad2(vcrfp, true);
        levelnew = GameStats.nMap;
        levelnum = GameStats.nMap;
        forcelevel = GameStats.nMap;
    }

    if (forcelevel > -1)
    {
        // YELLOW SECTION
        levelnew = forcelevel;
        UpdateInputs();
        forcelevel = -1;

        if (bRecord && !bInDemo) {
            menu_GameSave2(vcrfp);
        }
        goto LOOP3;
    }

    // PINK SECTION
    UpdateInputs();
    nNetMoves = 1;

    if (nMenu == 2)
    {
        levelnew = 1;
        levelnum = 1;
        levelnew = menu_GameLoad(SavePosition);
        lastlevel = -1;
    }

    if (bRecord && !bInDemo) {
        menu_GameSave2(vcrfp);
    }

    nBestLevel = levelnew - 1;
LOOP1:

    if (nPlayerLives[nLocalPlayer] <= 0) {
        goto MENU;
    }
    if (levelnew > 99) {
        goto EXITGAME;
    }
    if (!bInDemo && levelnew > nBestLevel && levelnew != 0 && levelnew <= kMap20 && SavePosition > -1) {
        menu_GameSave(SavePosition);
    }
LOOP2:
    if (!nNetPlayerCount && !bPlayback && levelnew > 0 && levelnew <= kMap20) {
        levelnew = showmap(levelnum, levelnew, nBestLevel);
    }

    if (levelnew > nBestLevel) {
        nBestLevel = levelnew;
    }
LOOP3:
    while (levelnew != -1)
    {
        // BLUE
        if (CDplaying()) {
            fadecdaudio();
        }

        if (levelnew == kMap20)
        {
            lCountDown = 81000;
            nAlarmTicks = 30;
            nRedTicks = 0;
            nClockVal = 0;
            nEnergyTowers = 0;
        }

        if (!LoadLevel(levelnew)) {
            // TODO "Can't load level %d...\n", nMap;
            goto EXITGAME;
        }
        levelnew = -1;
    }
    if (nNetPlayerCount == 0 && lastlevel == levelnum)
    {
        RestoreSavePoint(nLocalPlayer, &initx, &inity, &initz, &initsect, &inita);
    }

    lastlevel = levelnum;

    for (i = 0; i < nTotalPlayers; i++)
    {
        SetSavePoint(i, initx, inity, initz, initsect, inita);
        RestartPlayer(i);
        InitPlayerKeys(i);
    }

    UpdateScreenSize();
    fps = 0;
    lastfps = 0;
    InitStatus();
    ResetView();
    ResetEngine();
    totalmoves = 0;
    GrabPalette();
    ResetMoveFifo();
    moveframes = 0;
    bInMove = kFalse;
    tclocks = totalclock;
    nPlayerDAng = 0;
    lPlayerXVel = 0;
    lPlayerYVel = 0;
    movefifopos = movefifoend;

    RefreshStatus();

    if (bSerialPlay) {
        ClearSerialInbuf();
    }

    //int edi = totalclock;
    tclocks2 = totalclock;
    // Game Loop
GAMELOOP:
    while (1)
    {
        if (levelnew >= 0)
        {
            goto LOOP1;
        }

        HandleAsync();
        OSD_DispatchQueued();

        // Section B
        if (!CDplaying() && !nFreeze && !nNetPlayerCount)
        {
            int nTrack = levelnum;
            if (nTrack != 0) {
                nTrack--;
            }

            playCDtrack((nTrack % 8) + 11, true);
        }

// TODO		CONTROL_GetButtonInput();
        CheckKeys();

        if (bRecord || bPlayback)
        {
            bInMove = kTrue;

            moveframes = ((int)totalclock - (int)tclocks2) / 4;

            if (moveframes > 4)
                moveframes = 4;

            if (moveframes != 0)
                tclocks2 = totalclock;

            if (bPlayback)
            {
                // YELLOW
                if (((bInDemo && inputState.keyBufferWaiting()) || !ReadPlaybackInputs()) && inputState.keyGetChar())
                {
                    inputState.ClearAllInput();

                    bPlayback = kFalse;
                    bInDemo = kFalse;

                    if (vcrfp) {
                        fclose(vcrfp);
                    }

                    goto MENU;
                }
            }
            else if (bRecord || moveframes)
            {
                GetLocalInput();

                sPlayerInput[nLocalPlayer].xVel = lPlayerXVel;
                sPlayerInput[nLocalPlayer].yVel = lPlayerYVel;
                sPlayerInput[nLocalPlayer].buttons = lLocalButtons | lLocalCodes;
                sPlayerInput[nLocalPlayer].nAngle = nPlayerDAng;
                sPlayerInput[nLocalPlayer].nTarget = besttarget;

                Ra[nLocalPlayer].nTarget = besttarget;

                lLocalCodes = 0;
                nPlayerDAng = 0;

                sPlayerInput[nLocalPlayer].horizon = PlayerList[nLocalPlayer].q16horiz;
            }

            // loc_11F72:
            if (bRecord && !bInDemo) {
                WritePlaybackInputs();
            }

            if (nNetPlayerCount)
            {
                if (moveframes)
                {
                    UpdateInputs();
                    moveframes = nNetMoveFrames;
                }
            }
            else
            {
                // loc_11FBC:
                while (bPause)
                {
                    inputState.ClearAllInput();
                    if (WaitAnyKey(-1) != sc_Pause)
                    {
                        bPause = kFalse;
                    }
                }
            }

            // loc_11FEE:
            tclocks += moveframes * 4;
            while (moveframes && levelnew < 0)
            {
                GameMove();
                // if (nNetTime > 0)
                // {
                //     nNetTime--;
                //
                //     if (!nNetTime) {
                //         nFreeze = 3;
                //     }
                // }
                // else if (nNetTime == 0)
                // {
                //     if (buttonMap.ButtonDown(gamefunc_Open))
                //     {
                //         buttonMap.ClearButton(gamefunc_Open);
                //         goto MENU2;
                //     }
                // }
            }

            bInMove = kFalse;

            // END YELLOW SECTION

            // loc_12149:
            if (bInDemo || bPlayback)
            {
                while (tclocks > totalclock) { HandleAsync(); }
                tclocks = totalclock;
            }

            if (G_FPSLimit())
            {
                GameDisplay();
            }
        }
        else
        {
            bInMove = kTrue;

            if (GUICapture || bPause)
            {
                totalclock = tclocks + 4;
            }
            else
            {
                while ((totalclock - ototalclock) >= 1 || !bInMove)
                {
                    ototalclock = ototalclock + 1;

                    if (!((int)ototalclock&3) && moveframes < 4)
                        moveframes++;

                    GetLocalInput();
                    PlayerInterruptKeys();

                    nPlayerDAng = fix16_sadd(nPlayerDAng, localInput.nAngle);
                    inita &= kAngleMask;

                    lPlayerXVel += localInput.yVel * Cos(inita) + localInput.xVel * Sin(inita);
                    lPlayerYVel += localInput.yVel * Sin(inita) - localInput.xVel * Cos(inita);
                    lPlayerXVel -= (lPlayerXVel >> 5) + (lPlayerXVel >> 6);
                    lPlayerYVel -= (lPlayerYVel >> 5) + (lPlayerYVel >> 6);

                    sPlayerInput[nLocalPlayer].xVel = lPlayerXVel;
                    sPlayerInput[nLocalPlayer].yVel = lPlayerYVel;
                    sPlayerInput[nLocalPlayer].buttons = lLocalButtons | lLocalCodes;
                    sPlayerInput[nLocalPlayer].nAngle = nPlayerDAng;
                    sPlayerInput[nLocalPlayer].nTarget = besttarget;

                    Ra[nLocalPlayer].nTarget = besttarget;

                    lLocalCodes = 0;
                    nPlayerDAng = 0;

                    sPlayerInput[nLocalPlayer].horizon = PlayerList[nLocalPlayer].q16horiz;

                    while (levelnew < 0 && totalclock >= tclocks + 4)
                    {
                        // timerUpdate();
                        tclocks += 4;
                        GameMove();
                        // timerUpdate();
                    }
                }
            }
            bInMove = kFalse;

            PlayerInterruptKeys();

            if (G_FPSLimit())
            {
                GameDisplay();
            }
        }
        if (!bInDemo)
        {
            nMenu = MenuExitCondition;
            if (nMenu != -2)
            {
                MenuExitCondition = -2;
// MENU2:
                bInMove = kTrue;

                switch (nMenu)
                {
                    case 0:
                        goto EXITGAME;

                    case 1:
                        goto STARTGAME1;

                    case 2:
                        levelnum = levelnew = menu_GameLoad(SavePosition);
                        lastlevel = -1;
                        nBestLevel = levelnew - 1;
                        goto LOOP2;

                    case 3:
                        forcelevel = 0;
                        goto STARTGAME2;
                    case 6:
                        goto GAMELOOP;
                }

                totalclock = ototalclock = tclocks;
                bInMove = kFalse;
                RefreshStatus();
            }
            else if (buttonMap.ButtonDown(gamefunc_Map)) // e.g. TAB (to show 2D map)
            {
                buttonMap.ClearButton(gamefunc_Map);

                if (!nFreeze) {
                    nMapMode = (nMapMode+1)%3;
                }
            }

            if (nMapMode != 0)
            {
                int const timerOffset = ((int) totalclock - nonsharedtimer);
                nonsharedtimer += timerOffset;

                if (buttonMap.ButtonDown(gamefunc_Zoom_In))
                    lMapZoom += mulscale6(timerOffset, max<int>(lMapZoom, 256));

                if (buttonMap.ButtonDown(gamefunc_Zoom_Out))
                    lMapZoom -= mulscale6(timerOffset, max<int>(lMapZoom, 256));

                lMapZoom = clamp(lMapZoom, 48, 2048);
            }

            if (PlayerList[nLocalPlayer].nHealth > 0)
            {
                if (buttonMap.ButtonDown(gamefunc_Inventory_Left))
                {
                    SetPrevItem(nLocalPlayer);
                    buttonMap.ClearButton(gamefunc_Inventory_Left);
                }
                if (buttonMap.ButtonDown(gamefunc_Inventory_Right))
                {
                    SetNextItem(nLocalPlayer);
                    buttonMap.ClearButton(gamefunc_Inventory_Right);
                }
                if (buttonMap.ButtonDown(gamefunc_Inventory))
                {
                    UseCurItem(nLocalPlayer);
                    buttonMap.ClearButton(gamefunc_Inventory);
                }
            }
            else {
                SetAirFrame();
            }
        }
        if (record_mode == 3 && movefifopos == movefifoend) {
            levelnew = 0;
        }
        fps++;
    }
EXITGAME:

    ExitGame();
    return 0;
}

void mychangespritesect(int nSprite, int nSector)
{
    DoKenTest();
    changespritesect(nSprite, nSector);
    DoKenTest();
}

void mydeletesprite(int nSprite)
{
    if (nSprite < 0 || nSprite > kMaxSprites) {
        I_Error("bad sprite value %d handed to mydeletesprite", nSprite);
    }

    deletesprite(nSprite);

    if (nSprite == besttarget) {
        besttarget = -1;
    }
}


void KeyFn1()
{
    menu_DoPlasma();
    overwritesprite(160, 100, kSkullHead, 0, 3, kPalNormal);
    overwritesprite(161, 130, kSkullJaw, 0, 3, kPalNormal);
    videoNextPage();
}

void DoGameOverScene()
{
    FadeOut(0);
    inputState.ClearAllInput();

    if (LoadCinemaPalette(16) < 0) {
        return;
    }

    SetOverscan(ANIMPAL);
    NoClip();
    overwritesprite(0, 0, kTile3591, 0, 2, kPalNormal);
    Clip();
    videoNextPage();
    CinemaFadeIn();
    PlayGameOverSound();
    WaitAnyKey(3);
    FadeOut(0);
    SetOverscan(BASEPAL);
}

void DoTitle()
{
    short skullDurations[] = { 6, 25, 43, 50, 68, 78, 101, 111, 134, 158, 173, 230, 6000 };

    videoSetViewableArea(0, 0, xdim - 1, ydim - 1);

    if (videoGetRenderMode() == REND_CLASSIC)
        BlackOut();

    overwritesprite(0, 0, EXHUMED ? kTileBMGLogo : kTilePIELogo, 0, 2, kPalNormal);
    videoNextPage();

    if (videoGetRenderMode() == REND_CLASSIC)
        FadeIn();

    inputState.ClearAllInput();

    WaitAnyKey(2);

    if (videoGetRenderMode() == REND_CLASSIC)
        FadeOut(0);

    SetOverscan(BASEPAL);

    int nScreenTile = seq_GetSeqPicnum(kSeqScreens, 0, 0);

    overwritesprite(0, 0, nScreenTile, 0, 2, kPalNormal);
    videoNextPage();

    if (videoGetRenderMode() == REND_CLASSIC)
        FadeIn();
    PlayLogoSound();

    WaitAnyKey(2);

    if (videoGetRenderMode() == REND_CLASSIC)
        FadeOut(0);
    inputState.ClearAllInput();

    PlayMovie("book.mov");

    if (videoGetRenderMode() == REND_CLASSIC)
        FadeOut(0);

    SetOverscan(BASEPAL);
    GrabPalette();

    PlayLocalSound(StaticSound[59], 0, true, CHANF_UI);

    EraseScreen(4);

    playCDtrack(19, true);

    videoNextPage();
    FadeIn();
    WaitVBL();

    int String_Copyright = FindGString("COPYRIGHT");

    const char *a = gString[String_Copyright];
    const char *b = gString[String_Copyright + 1];

    menu_DoPlasma();

    int nTile = kSkullHead;

    overwritesprite(160, 100, kSkullHead, 0, 3, kPalNormal);
    overwritesprite(161, 130, kSkullJaw, 0, 3, kPalNormal);
    videoNextPage();

    WaitNoKey(2, KeyFn1);

    if (time(0) & 0xF) {
        PlayGameOverSound();
    }
    else {
        PlayLocalSound(StaticSound[61], 0, false, CHANF_UI);
    }

    int nStartTime = (int)totalclock;
    int nCount = 0;
    int var_18 = (int)totalclock + skullDurations[0];
    int var_4 = 0;

    int esi = 130;



    inputState.ClearAllInput();
    while (LocalSoundPlaying())
    {
        HandleAsync();
        if (inputState.CheckAllInput()) break;

        menu_DoPlasma();
        overwritesprite(160, 100, nTile, 0, 3, kPalNormal);

        int nStringWidth = MyGetStringWidth(a);

        int y = 200 - 24;
        myprintext((320 / 2 - nStringWidth / 2), y, a, 0);

        nStringWidth = MyGetStringWidth(b);

        y = 200 - 16;
        myprintext((320 / 2 - nStringWidth / 2), y, b, 0);

        if ((int)totalclock > var_18)
        {
            nCount++;

            if (nCount > 12) break;
            var_18 = nStartTime + skullDurations[nCount];
            var_4 = var_4 == 0;
        }

        short nTile = kSkullJaw;

        if (var_4)
        {
            if (esi >= 135) {
                nTile = kTile3583;
            }
            else {
                esi += 5;
            }
        }
        else if (esi <= 130)
        {
            esi = 130;
        }
        else
        {
            esi -= 2;
        }

        y = 0;

        if (nTile == kTile3583)
        {
            y = 131;
        }
        else
        {
            y = esi;

            if (y > 135) {
                y = 135;
            }
        }

        overwritesprite(161, y, nTile, 0, 3, kPalNormal);
        videoNextPage();
    }

    WaitNoKey(1, KeyFn1);

    videoSetViewableArea(nViewLeft, nViewTop, nViewRight, nViewBottom);
}

void CopyTileToBitmap(short nSrcTile,  short nDestTile, int xPos, int yPos)
{
    int nOffs = tilesiz[nDestTile].y * xPos;

	auto pixels = TileFiles.tileMakeWritable(nDestTile);
    uint8_t *pDest = pixels + nOffs + yPos;
    uint8_t *pDestB = pDest;

    tileLoad(nSrcTile);

    int destYSize = tilesiz[nDestTile].y;
    int srcYSize = tilesiz[nSrcTile].y;

    const uint8_t *pSrc = tilePtr(nSrcTile);

    for (int x = 0; x < tilesiz[nSrcTile].x; x++)
    {
        pDest += destYSize;

        for (int y = 0; y < srcYSize; y++)
        {
            uint8_t val = *pSrc;
            if (val != 0xFF) {
                *pDestB = val;
            }

            pDestB++;
            pSrc++;
        }

        // reset pDestB
        pDestB = pDest;
    }

    tileInvalidate(nDestTile, -1, -1);
}

int CopyCharToBitmap(char nChar, int nTile, int xPos, int yPos)
{
    if (nChar == ' ') {
        return 4;
    }

    nChar = toupper(nChar);
    int nFontTile = seq_GetSeqPicnum(kSeqFont2, 0, nChar - 32) + 102;
    CopyTileToBitmap(nFontTile, nTile, xPos, yPos);

    return tilesiz[nFontTile].x + 1;
}

// Note: strings passed should be uppercase
int myprintext(int x, int y, const char *str, int shade)
{
    if (y < -15 || y >= 200)
        return x;

    const char *c = str;

    while (*c != '\0')
    {
        int nTile = seq_GetSeqPicnum(kSeqFont2, 0, (*c) - 32);
        overwritesprite(x, y, nTile, shade, 2, kPalNormal);

        int tileWidth = tilesiz[nTile].x;

        x += tileWidth + 1;
        c++;
    }

    return x;
}

void EraseScreen(int nVal)
{
    if (nVal == -1) {
        nVal = overscanindex;
    }

    videoClearScreen(nVal);
}

void InitSpiritHead()
{
    char filename[20];

    nPixels = 0;

    nSpiritRepeatX = sprite[nSpiritSprite].xrepeat;
    nSpiritRepeatY = sprite[nSpiritSprite].yrepeat;

    tileLoad(kTileRamsesNormal); // Ramses Normal Head

    for (int i = 0; i < kMaxSprites; i++)
    {
        if (sprite[i].statnum)
        {
            sprite[i].cstat |= 0x8000;
        }
    }

	auto pTile = tilePtr(kTileRamsesNormal); // Ramses Normal Head
	auto pGold = tilePtr(kTileRamsesGold);
    for (int x = 0; x < 97; x++)
    {
        for (int y = 0; y < 106; y++)
        {
            if (*pTile != 255)
            {
				pixelval[nPixels] = *(pGold + x * 106 + y);
                origx[nPixels] = x - 48;
                origy[nPixels] = y - 53;
                curx[nPixels] = 0;
                cury[nPixels] = 0;
                vely[nPixels] = 0;
                velx[nPixels] = 0;

                destvelx[nPixels] = RandomSize(2) + 1;

                if (curx[nPixels] > 0) {
                    destvelx[nPixels] = -destvelx[nPixels];
                }

                destvely[nPixels] = RandomSize(2) + 1;

                if (cury[nPixels] > 0) {
                    destvely[nPixels] = -destvely[nPixels];
                }

                nPixels++;
            }

            pTile++;
        }
    }


    sprite[nSpiritSprite].yrepeat = 140;
    sprite[nSpiritSprite].xrepeat = 140;
    sprite[nSpiritSprite].picnum = kTileRamsesWorkTile;

    nHeadStage = 0;

    // work tile is twice as big as the normal head size
	Worktile = TileFiles.tileCreate(kTileRamsesWorkTile, kSpiritX * 2, kSpiritY * 2);

    sprite[nSpiritSprite].cstat &= 0x7FFF;

    nHeadTimeStart = (int)totalclock;

    memset(Worktile, -1, WorktileSize);
    tileInvalidate(kTileRamsesWorkTile, -1, -1);

    nPixelsToShow = 0;

    fadecdaudio();

    int nTrack;

    if (levelnum == 1)
    {
        nTrack = 3;
    }
    else
    {
        nTrack = 7;
    }

    bSubTitles = playCDtrack(nTrack, false) == 0;

    StartSwirlies();

    sprintf(filename, "LEV%d.PUP", levelnum);
    lNextStateChange = (int)totalclock;
    lHeadStartClock = (int)totalclock;

	auto headfd = fileSystem.OpenFileReader(filename, 512); // 512??
	if (!headfd.isOpen())
	{
		memset(cPupData, 0, sizeof(cPupData));
	}
	else
	{
		nPupData = headfd.Read(cPupData, sizeof(cPupData));
		pPupData = cPupData;
	}
    nMouthTile = 0;
    nTalkTime = 1;
}

void DimSector(short nSector)
{
    short startwall = sector[nSector].wallptr;
    short nWalls = sector[nSector].wallnum;

    for (int i = 0; i < nWalls; i++)
    {
        if (wall[startwall+i].shade < 40) {
            wall[startwall+i].shade++;
        }
    }

    if (sector[nSector].floorshade < 40) {
        sector[nSector].floorshade++;
    }

    if (sector[nSector].ceilingshade < 40) {
        sector[nSector].ceilingshade++;
    }
}

void CopyHeadToWorkTile(short nTile)
{
	const uint8_t* pSrc = tilePtr(nTile);
    uint8_t *pDest = &Worktile[212 * 49 + 53];

    for (int i = 0; i < 97; i++)
    {
        memcpy(pDest, pSrc, 106);

        pDest += 212;
        pSrc += 106;
    }
}

int DoSpiritHead()
{
    static short word_964E6 = 0;

    PlayerList[0].q16horiz = fix16_sadd(PlayerList[0].q16horiz, fix16_sdiv(fix16_ssub(nDestVertPan[0], PlayerList[0].q16horiz), fix16_from_int(4)));

    tileInvalidate(kTileRamsesWorkTile, -1, -1);

    if (nHeadStage < 2)
    {
        memset(Worktile, -1, WorktileSize);
    }

    if (nHeadStage < 2 || nHeadStage != 5)
    {
        nPixelsToShow = ((int)totalclock - nHeadTimeStart) * 15;

        if (nPixelsToShow > nPixels) {
            nPixelsToShow = nPixels;
        }

        if (nHeadStage < 3)
        {
            UpdateSwirlies();

            if (sprite[nSpiritSprite].shade > -127) {
                sprite[nSpiritSprite].shade--;
            }

            word_964E6--;
            if (word_964E6 < 0)
            {
                DimSector(sprite[nSpiritSprite].sectnum);
                word_964E6 = 5;
            }

            if (!nHeadStage)
            {
                if (((int)totalclock - nHeadTimeStart) > 480)
                {
                    nHeadStage = 1;
                    nHeadTimeStart = (int)totalclock + 480;
                }

                for (int i = 0; i < nPixelsToShow; i++)
                {
                    if (destvely[i] >= 0)
                    {
                        vely[i]++;

                        if (vely[i] >= destvely[i])
                        {
                            destvely[i] = -(RandomSize(2) + 1);
                        }
                    }
                    else
                    {
                        vely[i]--;

                        if (vely[i] <= destvely[i])
                        {
                            destvely[i] = RandomSize(2) + 1;
                        }
                    }

                    // loc_13541
                    if (destvelx[i] >= 0)
                    {
                        velx[i]++;

                        if (velx[i] >= destvelx[i])
                        {
                            destvelx[i] = -(RandomSize(2) + 1);
                        }
                    }
                    else
                    {
                        velx[i]--;

                        if (velx[i] <= destvelx[i])
                        {
                            destvelx[i] = RandomSize(2) + 1;
                        }
                    }

                    int esi = vely[i] + (cury[i] >> 8);

                    if (esi < 106)
                    {
                        if (esi < -105)
                        {
                            vely[i] = 0;
                            esi = 0;
                        }
                    }
                    else
                    {
                        vely[i] = 0;
                        esi = 0;
                    }

                    int ebx = velx[i] + (curx[i] >> 8);

                    if (ebx < 97)
                    {
                        if (ebx < -96)
                        {
                            velx[i] = 0;
                            ebx = 0;
                        }
                    }
                    else
                    {
                        velx[i] = 0;
                        ebx = 0;
                    }

                    curx[i] = ebx * 256;
                    cury[i] = esi * 256;

                    esi += (ebx + 97) * 212;

                    Worktile[106 + esi] = pixelval[i];
                }

                return 1;
            }
            else
            {
                if (nHeadStage != 1) {
                    return 1;
                }

                uint8_t nXRepeat = sprite[nSpiritSprite].xrepeat;
                if (nXRepeat > nSpiritRepeatX)
                {
                    sprite[nSpiritSprite].xrepeat -= 2;

                    nXRepeat = sprite[nSpiritSprite].xrepeat;
                    if (nXRepeat < nSpiritRepeatX)
                    {
                        sprite[nSpiritSprite].xrepeat = nSpiritRepeatX;
                    }
                }

                uint8_t nYRepeat = sprite[nSpiritSprite].yrepeat;
                if (nYRepeat > nSpiritRepeatY)
                {
                    sprite[nSpiritSprite].yrepeat -= 2;

                    nYRepeat = sprite[nSpiritSprite].yrepeat;
                    if (nYRepeat < nSpiritRepeatY)
                    {
                        sprite[nSpiritSprite].yrepeat = nSpiritRepeatY;
                    }
                }

                int esi = 0;

                for (int i = 0; i < nPixels; i++)
                {
                    int eax = (origx[i] << 8) - curx[i];
                    int ecx = eax;

                    if (eax)
                    {
                        if (eax < 0) {
                            eax = -eax;
                        }

                        if (eax < 8)
                        {
                            curx[i] = origx[i] << 8;
                            ecx = 0;
                        }
                        else {
                            ecx >>= 3;
                        }
                    }
                    else
                    {
                        ecx >>= 3;
                    }

                    int var_1C = (origy[i] << 8) - cury[i];
                    int ebp = var_1C;

                    if (var_1C)
                    {
                        eax = ebp;

                        if (eax < 0) {
                            eax = -eax;
                        }

                        if (eax < 8)
                        {
                            cury[i] = origy[i] << 8;
                            var_1C = 0;
                        }
                        else
                        {
                            var_1C >>= 3;
                        }
                    }
                    else
                    {
                        var_1C >>= 3;
                    }

                    if (var_1C || ecx)
                    {
                        curx[i] += ecx;
                        cury[i] += var_1C;

                        esi++;
                    }

                    ecx = (((curx[i] >> 8) + 97) * 212) + (cury[i] >> 8);


                    Worktile[106 + ecx] = pixelval[i];
                }

                if (((int)totalclock - lHeadStartClock) > 600) {
                    CopyHeadToWorkTile(kTileRamsesGold);
                }

                int eax = ((nPixels << 4) - nPixels) / 16;

                if (esi < eax)
                {
                    SoundBigEntrance();
                    AddGlow(sprite[nSpiritSprite].sectnum, 20);
                    AddFlash(
                        sprite[nSpiritSprite].sectnum,
                        sprite[nSpiritSprite].x,
                        sprite[nSpiritSprite].y,
                        sprite[nSpiritSprite].z,
                        128);

                    nHeadStage = 3;
                    TintPalette(255, 255, 255);
                    CopyHeadToWorkTile(kTileRamsesNormal);
                }

                return 1;
            }
        }
        else
        {
            FixPalette();

            if (!nPalDiff)
            {
                nFreeze = 2;
                nHeadStage++;
            }

            return 0;
        }
    }
    else
    {
        if (lNextStateChange <= (int)totalclock)
        {
            if (nPupData)
            {
                short nPupVal = *pPupData;
                pPupData++;
                nPupData -= 2;

                if (nPupData > 0)
                {
                    lNextStateChange = (nPupVal + lHeadStartClock) - 10;
                    nTalkTime = !nTalkTime;
                }
                else
                {
                    nTalkTime = 0;
                    nPupData = 0;
                }
            }
            else if (!bSubTitles)
            {
                if (!CDplaying())
                {
                    levelnew = levelnum + 1;
                    fadecdaudio();
                }
            }
        }

        word_964E8--;
        if (word_964E8 <= 0)
        {
            word_964EA = RandomBit() * 2;
            word_964E8 = RandomSize(5) + 4;
        }

        int ebx = 592;
        word_964EC--;

        if (word_964EC < 3)
        {
            ebx = 593;
            if (word_964EC <= 0) {
                word_964EC = RandomSize(6) + 4;
            }
        }

        ebx += word_964EA;

        uint8_t *pDest = &Worktile[10441];
		const uint8_t* pSrc = tilePtr(ebx);

        for (int i = 0; i < 97; i++)
        {
            memcpy(pDest, pSrc, 106);

            pDest += 212;
            pSrc += 106;
        }

        if (nTalkTime)
        {
            if (nMouthTile < 2) {
                nMouthTile++;
            }
        }
        else if (nMouthTile != 0)
        {
            nMouthTile--;
        }

        if (nMouthTile)
        {
            short nTileSizeX = tilesiz[nMouthTile + 598].x;
            short nTileSizeY = tilesiz[nMouthTile + 598].y;

            uint8_t *pDest = &Worktile[212 * (97 - nTileSizeX / 2)] + (159 - nTileSizeY);
            const uint8_t *pSrc = tilePtr(nMouthTile + 598);

            while (nTileSizeX > 0)
            {
                memcpy(pDest, pSrc, nTileSizeY);

                nTileSizeX--;
                pDest += 212;
                pSrc += nTileSizeY;
            }
        }

        return 1;
    }

    // TEMP FIXME - temporary return value. what to return here? 1?

    return 0;
}

bool GameInterface::CanSave()
{
    return !bRecord && !bPlayback && !bPause && !bInDemo && nTotalPlayers == 1;
}

void GameInterface::UpdateScreenSize()
{
    Powerslave::UpdateScreenSize();
}


::GameInterface* CreateInterface()
{
    return new GameInterface;
}

//short lastlevel;
//short forcelevel = -1;
//int lLocalCodes = 0;
//int flash;
//short textpages;

// This is only the static global data.
static SavegameHelper sgh("exhumed",
    SA(cPupData),
    SV(nPupData),
    SV(nPixels),
    SA(curx),
    SA(cury),
    SA(destvelx),
    SA(destvely),
    SA(pixelval),
    SA(origy),
    SA(origx),
    SA(velx),
    SA(vely),
    SV(nMouthTile),
    SV(nSpiritSprite),
    SV(word_964E8),
    SV(word_964EA),
    SV(word_964EC),
    SV(nSpiritRepeatX),
    SV(nSpiritRepeatY),
    SV(nPixelsToShow),
    SV(nCreaturesLeft), // todo: also maintain a total counter.
    SV(nFreeze),
    SV(nSnakeCam),
    SV(nLocalSpr),
    SV(levelnew),
    SV(nClockVal),  // kTile3603
    SV(nRedTicks),
    SV(nAlarmTicks),
    SV(nButtonColor),
    SV(nEnergyChan),
    SV(lCountDown),
    SV(nEnergyTowers),
    SV(nHeadStage),
    SV(nTalkTime),
    SV(levelnum),
    SV(moveframes),
    SV(totalmoves),
    SV(nCurBodyNum),
    SV(nBodyTotal),
    SV(bSnakeCam),
    SV(bSlipMode),
    SV(lHeadStartClock),
    SV(lNextStateChange),
    SV(nHeadTimeStart),
    SV(localclock),
    SV(tclocks),
    SV(tclocks2),
    SV(totalclock),
    nullptr);


void SaveTextureState()
{
    auto fw = WriteSavegameChunk("texture");
    int pupOffset = pPupData? int(pPupData - cPupData) : -1;

    // There is really no good way to restore these tiles, so it's probably best to save them as well, so that they can be reloaded with the exact state they were left in
    fw->Write(&pupOffset, 4);
    uint8_t loaded = !!Worktile;
    fw->Write(&loaded, 1);
    if (Worktile) fw->Write(Worktile, WorktileSize);
    auto pixels = TileFiles.tileMakeWritable(kTile3603);
    fw->Write(pixels, tilesiz[kTile3603].x * tilesiz[kTile3603].y);
    pixels = TileFiles.tileMakeWritable(kEnergy1);
    fw->Write(pixels, tilesiz[kEnergy1].x * tilesiz[kEnergy1].y);
    pixels = TileFiles.tileMakeWritable(kEnergy2);
    fw->Write(pixels, tilesiz[kEnergy2].x * tilesiz[kEnergy2].y);
    
}

void LoadTextureState()
{
    auto fr = ReadSavegameChunk("texture");
    int pofs;
    fr.Read(&pofs, 4);
    pPupData = pofs == -1 ? nullptr : cPupData + pofs;
    uint8_t loaded;
    fr.Read(&loaded, 1);
    if (loaded)
    {
        Worktile = TileFiles.tileCreate(kTileRamsesWorkTile, kSpiritX * 2, kSpiritY * 2);
        fr.Read(Worktile, WorktileSize);
    }
    auto pixels = TileFiles.tileMakeWritable(kTile3603);
    fr.Read(pixels, tilesiz[kTile3603].x * tilesiz[kTile3603].y);
    pixels = TileFiles.tileMakeWritable(kEnergy1);
    fr.Read(pixels, tilesiz[kEnergy1].x * tilesiz[kEnergy1].y);
    pixels = TileFiles.tileMakeWritable(kEnergy2);
    fr.Read(pixels, tilesiz[kEnergy2].x * tilesiz[kEnergy2].y);
    TileFiles.InvalidateTile(kTileRamsesWorkTile);
    TileFiles.InvalidateTile(kTile3603);
    TileFiles.InvalidateTile(kEnergy1);
    TileFiles.InvalidateTile(kEnergy2);
}


END_PS_NS
