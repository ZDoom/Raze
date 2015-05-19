//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

// CONSOLE.C
// Handles all argument storing and user console variable modifications.
// Copyright (c) 1996 by Jim Norwood

#include "build.h"

#include "mytypes.h"
#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "player.h"
#include "lists.h"
#include "warp.h"
#include "quake.h"

#include "common_game.h"
#include "function.h"
#include "control.h"
#include "trigger.h"

#include "savedef.h"
#include "menus.h"
#include "net.h"
#include "pal.h"

#include "weapon.h"
#include "text.h"
#include "jsector.h"

// DEFINES ///////////////////////////////////////////////////////////////////////////////////
#define MAX_USER_ARGS           100
#define MAX_CONSOLE_COMMANDS    100
#define MAX_HISTORY             20

SWBOOL SpriteInfo = FALSE;
extern SWBOOL QuitFlag;

// FUNCTION PROTOTYPES ///////////////////////////////////////////////////////////////////////
void CON_ProcessOptions(void);
void CON_ClearConsole(void);
uint8_t CON_CommandCmp(const char *str1, const char *str2, int len);
void CheatInput(void);

// Modify actor routines
void CON_ModXrepeat(void);
void CON_ModYrepeat(void);
void CON_ModTranslucent(void);
void CON_GetHelp(void);
void CON_Sound(void);
void CON_Reverb(void);
void CON_Heap(void);
void CON_Cache(void);
void CON_SoundTest(void);
void CON_SpriteInfo(void);
void CON_KillSprite(void);
void CON_SpriteDetail(void);
void CON_UserDetail(void);
void CON_Quit(void);
void CON_LoadSetup(void);
void CON_DamageData(void);
void CON_WinPachinko(void);
void CON_Tweak(void);
void CON_Bunny(void);
void CON_CheckHeap(void);
void CON_DumpHeap(void);
void CON_ShowMirror(void);
void CON_MultiNameChange(void);
void CON_DumpSoundList(void);

// STRUCTURES ////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    const char *command;              // Text string representing the command that calls this function
    void (*function)(void);     // Function assigned to the command, take no parameters

} CON_COMMAND, *CON_COMMANDp;

// Contains any commands that don't get added by particular setup functions
CON_COMMAND pre_commands[] =
{
#if DEBUG
    {"bobbing",     CON_ProcessOptions},
    {"swnext",      CheatInput},
    {"swprev",      CheatInput},
    {"swsecret",    CheatInput},
    {"swstart",     CheatInput},
    {"swres",       CheatInput},
    {"swloc",       CheatInput},
    {"swroom",      CheatInput},
    {"swmap",       CheatInput},
    {"swvox",       CheatInput},
    {"swsave",      CheatInput},
#endif
#if DEBUG
    {"george",      CheatInput},
    {"blackburn",   CheatInput},
    {"reverb",      CON_Reverb},
    {"mem",         CON_Heap},
    {"cache",       CON_Cache},
    {"xrepeat",     CON_ModXrepeat},
    {"yrepeat",     CON_ModYrepeat},
    {"translucent", CON_ModTranslucent},
    {"spriteinfo",  CON_SpriteInfo},
    {"kill",        CON_KillSprite},
    {"showsprite",  CON_SpriteDetail},
    {"showuser",    CON_UserDetail},
    {"damage",      CON_DamageData},
    {"tweak",       CON_Tweak},
    {"checkheap",   CON_CheckHeap},
    {"dumpheap",    CON_DumpHeap},
    {"showmirror",  CON_ShowMirror},
    {"clear",       CON_ClearConsole},
    {"dumpsounds",  CON_DumpSoundList},
    {"help",        CON_GetHelp},
//{"quit",        CON_Quit},
#endif
    {"swchan",      CheatInput},
    {"swgimme",     CheatInput},
    {"swtrek##",    CheatInput},
    {"swgreed",     CheatInput},
    {"swghost",     CheatInput},
    {"swstart",     CheatInput},
    {"swres",       CheatInput},
    {"swloc",       CheatInput},
    {"swmap",       CheatInput},
    {"swsave",      CheatInput},
    {"sound",       CON_SoundTest},
    {"winpachinko", CON_WinPachinko},
    {"config",      CON_LoadSetup},
    {"swtrix",      CON_Bunny},
    {"swname",      CON_MultiNameChange},
    {NULL, NULL}
};


// GLOBALS ///////////////////////////////////////////////////////////////////////////////////

CON_COMMAND commandlist[MAX_CONSOLE_COMMANDS];  // Console command array
CON_COMMANDp commandptr;    // Pointer to a command

int16_t numcommands=0;    // Total number of commands in the command list

char command_history[MAX_HISTORY][256]; // History of what has been typed in lately
int16_t curr_history=0; // Line currently being pointed to in the history array
int16_t numhistory=0;

// Array which stores all the user arguments passed into the game.
static char user_args[MAX_USER_ARGS][256];
static uint8_t con_argnum=0;   // Total number of arguments that were passed into the game

char con_message[80]; // Holds the current console message to send to adduserquote

// FUNCTIONS /////////////////////////////////////////////////////////////////////////////////


//
// Frank's neato input string checker, useful for my stuff too.
//
uint8_t CON_CommandCmp(const char *str1, const char *str2, int len)
{
    const char *cp1 = str1;
    const char *cp2 = str2;

    do
    {
        if (*cp1 != *cp2)
        {
            if (*cp1 != '#' && *cp2 != '#')
                return FALSE;
            else if ((*cp1 == '#' && !isdigit(*cp2)) || (*cp2 == '#' && !isdigit(*cp1)))
                return FALSE;
        }

        cp1++;
        cp2++;
    }
    while (--len);

    return TRUE;
}

SWBOOL IsCommand(char *str)
{
    int i;
    char first[512];

    sscanf(str,"%s",first);

    for (i = 0; i < numcommands; i++)
    {
        // Don't even try if they aren't the same length
        if (strlen(first) != strlen(commandlist[i].command))
            continue;

        // See if it's in there
        if (CON_CommandCmp(first, commandlist[i].command, strlen(first)))
        {
            return TRUE;
        }
    }

    return FALSE;
}

//
// Sends a message to the user quote array
//

void CON_Message(const char *message, ...)
{
    va_list argptr;

    va_start(argptr,message);
    vsprintf(&con_message[0],message,argptr);
    va_end(argptr);

    // Send message to user quote array for immediate display
    adduserquote(&con_message[0]);
}

//
// Sends a message to the console quote array
//

void CON_ConMessage(const char *message, ...)
{
    va_list argptr;

    va_start(argptr,message);
    vsprintf(&con_message[0],message,argptr);
    va_end(argptr);

    // Send message to user quote array for immediate display
    addconquote(&con_message[0]);
}

//
// Stores user arguments passed in on the command line for later inspection
//
void CON_StoreArg(const char *userarg)
{
    if (con_argnum < MAX_USER_ARGS)
    {
        strcpy(&user_args[con_argnum][0],userarg);
        Bstrlwr(&user_args[con_argnum][0]);
        con_argnum++;
    }
}

//
// Checkes the user command array to see if user did in fact pass in a particular argument
//
SWBOOL CON_CheckParm(const char *userarg)
{
    int16_t i;

    for (i=0; i<con_argnum; i++)
    {
        if (!strcmp(&user_args[i][0],userarg))
            return TRUE;   // Yep, it's in there
    }

    return FALSE;   // Not a parameter that was passed in
}

//
// Scrolls up and down through previous user commands like DosKey
// Copies the history text string into the MessageInputCommand
//
void CON_CommandHistory(signed char dir)
{
    if (curr_history + dir < numhistory)
        curr_history += dir;
    if (curr_history < 0) curr_history = 0;
    if (curr_history > MAX_HISTORY) curr_history = MAX_HISTORY;

    strcpy(MessageInputString, command_history[curr_history]);
}

void CON_AddHistory(const char *commandstr)
{
    int i;

    for (i=MAX_HISTORY-1; i>=0; i--)
    {
        strcpy(command_history[i],command_history[i-1]);
    }
    strcpy(command_history[0],commandstr);
    if ((++numhistory) > MAX_HISTORY) numhistory = MAX_HISTORY;
}


//
// Adds a command name to the command list and assigns the appropriate function pointer
//
SWBOOL CON_AddCommand(const char *command, void (*function)(void))
{
    if (command != NULL && function != NULL && numcommands < MAX_CONSOLE_COMMANDS)
    {
//      strcpy(commandlist[numcommands].command, command);
        commandlist[numcommands].command = command;
        commandlist[numcommands].function = function;

        // Increment counter to set up for next command insertion
        numcommands++;

        ASSERT(numcommands <= MAX_CONSOLE_COMMANDS);

        return TRUE;
    }

    return FALSE;
}

//
// Process commands
// Returns TRUE upon success
//
void CON_ProcessUserCommand(void)
{
    int16_t i=0;
    char temp_message[256],command_str[256];

    strcpy(temp_message,MessageInputString);
    sscanf(Bstrlwr(temp_message),"%s", command_str); // Get the base command type

    for (i = 0; i < numcommands; i++)
    {
        // Don't even try if they aren't the same length
        if (strlen(command_str) != strlen(commandlist[i].command)) continue;
        // See if it's in there
        if (CON_CommandCmp(command_str, commandlist[i].command, strlen(command_str)))
        {
            if (commandlist[i].function)
            {
                (*commandlist[i].function)();
                CON_AddHistory(MessageInputString); // Keep history only of valid input
                return;
            }
        }
    }

    if (ConPanel)
        CON_ConMessage("Syntax Error or Command not enabled!");
}

//
// Initialize the console command list with the pre_command startup array
//
void CON_InitConsole(void)
{
    CON_COMMANDp i;

    for (i = &pre_commands[0]; i->command != NULL; i++)
    {
        if (!CON_AddCommand(i->command, i->function))
        {
            printf("CON_InitConsole: Failed to add command contained in pre_commands list.\n");
            TerminateGame();
            exit(0);
        }
    }

    //printf("CON_InitConsole: Command list initialized.\n");
}

//
// Process as a command, anything that could be set in the options menu as well
//
void CON_ProcessOptions(void)
{

}

// Clear the console screen
void CON_ClearConsole(void)
{
    short i;

    for (i=0; i<MAXCONQUOTES; i++)
        strcpy(con_quote[i],"\0");
}

/////////////////////////////////////////////////////////////////////////////////////////////
// The user console programming function library ////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

SWBOOL CheckValidSprite(short SpriteNum)
{
    if (SpriteNum < 0 || SpriteNum > 6144)
    {
        CON_ConMessage("ERROR: Sprite %d is out of range.",SpriteNum);
        return FALSE;
    }
    return TRUE;
}

// Get help on a console command
void CON_GetHelp(void)
{
    char base[80], command[80];
    short i;


    if (sscanf(MessageInputString,"%s %s",base,command) < 2)
    {
        CON_ConMessage("Usage: help [keyword]");
        return;
    }

    Bstrlwr(command);    // Make sure operator is all lower case

    if (!strcmp(command, "xrepeat"))
    {
        CON_ConMessage("Usage: xrepeat [repeat value 0-255],");
        CON_ConMessage("   [User ID (-1 for all ID's)], [SpriteNum (-1 for all of type ID)]");
        return;
    }
    else if (!strcmp(command, "yrepeat"))
    {
        CON_ConMessage("Usage: yrepeat [repeat value 0-255],");
        CON_ConMessage("   [User ID (-1 for all ID's)], [SpriteNum (-1 for all of type ID)]");
        return;
    }
    else if (!strcmp(command, "translucent"))
    {
        CON_ConMessage("Usage: translucent [OFF/ON 0-1],");
        CON_ConMessage("   [User ID (-1 for all ID's)], [SpriteNum (-1 for all of type ID)]");
        return;
    }
    else
    {
        CON_ConMessage("No help was located on that subject.");
    }
}

// Modify sprites xrepeat value
void CON_ModXrepeat(void)
{
    char base[80];
    int16_t op1=64,op2=-1,op3=-1;
    short i;


    if (sscanf(MessageInputString,"%s %hd %hd %hd",base,&op1,&op2,&op3) < 4)
    {
        strcpy(MessageInputString,"help xrepeat");
        CON_GetHelp();
        return;
    }

    if (op3 == -1)
    {
        for (i=0; i<MAXSPRITES; i++)
        {
            SPRITEp sp = &sprite[i];
            USERp u = User[i];

            if (op2 == -1)
                sp->xrepeat = op1;
            else
            {
                if (u->ID == op2)
                    sp->xrepeat = op1;
            }
        }
        if (op2 == -1)
            CON_ConMessage("Xrepeat set to %d for all u->ID's for all sprites.",op1);
        else
            CON_ConMessage("Xrepeat set to %d for u->ID = %d for all sprites.",op1,op2);
    }
    else
    {
        // Do it only for one sprite
        SPRITEp sp = &sprite[op3];
        USERp u = User[op3];

        if (!CheckValidSprite(op3)) return;

        sp->xrepeat = op1;
        CON_ConMessage("Xrepeat set to %d for sprite %d.",op1,op3);
    }
}

// Modify sprites yrepeat value
void CON_ModYrepeat(void)
{
    char base[80];
    int16_t op1=64,op2=-1,op3=-1;
    short i;


    if (sscanf(MessageInputString,"%s %hd %hd %hd",base,&op1,&op2,&op3) < 4)
    {
        strcpy(MessageInputString,"help yrepeat");
        CON_GetHelp();
        return;
    }


    if (op3 == -1)
    {
        for (i=0; i<MAXSPRITES; i++)
        {
            SPRITEp sp = &sprite[i];
            USERp u = User[i];

            if (op2 == -1)
                sp->yrepeat = op1;
            else
            {
                if (u->ID == op2)
                    sp->yrepeat = op1;
            }
        }
        if (op2 == -1)
            CON_ConMessage("Yrepeat set to %d for all u->ID's for all sprites.",op1);
        else
            CON_ConMessage("Yrepeat set to %d for u->ID = %d for all sprites.",op1,op2);
    }
    else
    {
        // Do it only for one sprite
        SPRITEp sp = &sprite[op3];
        USERp u = User[op3];

        if (!CheckValidSprite(op3)) return;

        sp->yrepeat = op1;
        CON_ConMessage("Yrepeat set to %d for sprite %d.",op1,op3);
    }
}

void CON_ModTranslucent(void)
{
    char base[80];
    int16_t op1=0;
    SPRITEp sp;
    USERp u;

    // Format: translucent [SpriteNum]
    if (sscanf(MessageInputString,"%s %hd",base,&op1) < 2)
    {
        strcpy(MessageInputString,"help translucent");
        CON_GetHelp();
        return;
    }

    if (!CheckValidSprite(op1)) return;

    sp = &sprite[op1];
    u = User[op1];

    if (TEST(sp->cstat,CSTAT_SPRITE_TRANSLUCENT))
    {
        RESET(sp->cstat,CSTAT_SPRITE_TRANSLUCENT);
        CON_ConMessage("Translucence RESET for sprite %d.",op1);
    }
    else
    {
        SET(sp->cstat,CSTAT_SPRITE_TRANSLUCENT);
        CON_ConMessage("Translucence SET for sprite %d.",op1);
    }
}

void CON_SoundTest(void)
{
    int handle;
    int zero=0;
    char base[80];
    int16_t op1=0;

    // Format: sound [number]
    if (sscanf(MessageInputString,"%s %hd",base,&op1) < 2)
    {
        strcpy(MessageInputString,"help sound");
        CON_GetHelp();
        return;
    }

    if (op1 < 0 || op1 >= DIGI_MAX)
    {
        CON_ConMessage("Sound number out of range.");
        return;
    }

    handle = PlaySound(op1,&zero,&zero,&zero,v3df_none);
}


void CON_Reverb(void)
{
    char base[80];
    int16_t op1=0;
    PLAYERp pp = Player + screenpeek;

    // Format: reverb [number]
    if (sscanf(MessageInputString,"%s %hd",base,&op1) < 2)
    {
        strcpy(MessageInputString,"help reverb");
        CON_GetHelp();
        return;
    }

    CON_ConMessage("Reverb is now set to %d.",op1);
    COVER_SetReverb(op1);
    pp->Reverb = op1;
}

void CON_Heap(void)
{
    /*
    int totalmemory=0;
    extern int TotalMemory, ActualHeap;
    int i;
    void *testheap;

    totalmemory = Z_AvailHeap();
    CON_ConMessage("Total heap at game startup = %d", TotalMemory);
    CON_ConMessage("ActualHeap reserved for non-cache use = %d", ActualHeap);
    CON_ConMessage("Total unallocated blocks in bytes minus reserved heap = %d", totalmemory);
    CON_ConMessage("NOTE: Allocation exceeding ActualHeap will result in out of memory");
    // Find remaining heap space unused
    i = ActualHeap;
    while(i>0)
    {
    testheap = AllocMem(i);
    if(!testheap)
        i-=1024L; // Decrease in 1k increments
    else
        {
        CON_ConMessage("Heap test result (+ or - 1k):");
        CON_ConMessage("=============================");
        CON_ConMessage("Unallocated heap space remaining  = %d",i);
        CON_ConMessage("Unallocated heap space used  = %d",ActualHeap - i);
        FreeMem(testheap);
        i=0; // Beam us out of here Scotty!
        }
    }

    if(ActualHeap < 50000L)
    {
    CON_ConMessage("ALERT: Memory is critically low!");
    }
    */
}

int TileRangeMem(int start)
{
    int i;
    int total=0;

    switch (start)
    {
    case 4096: // Evil Ninja
        for (i=4096; i<=4239; i++)
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 800:  // Hornet
        for (i=800; i<=811; i++)
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 817:
        for (i=817; i<=819; i++) // Bouncing Betty
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 820: // Skull
        for (i=820; i<=854; i++)
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 960:
        for (i=960; i<=1016; i++) // Serpent God
            total += tilesiz[i].x*tilesiz[i].y;
        for (i=1300; i<=1314; i++)
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 1024:
        for (i=1024; i<=1175; i++) // LoWang
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 1320:
        for (i=1320; i<=1396; i++) // Skeletor Priest
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 1400:
        for (i=1400; i<=1440; i++) // Coolie
            total += tilesiz[i].x*tilesiz[i].y;
        for (i=4260; i<=4266; i++)
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 1441:
        for (i=1441; i<=1450; i++) // Coolie Ghost
            total += tilesiz[i].x*tilesiz[i].y;
        for (i=4267; i<=4312; i++)
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 1469:
        for (i=1469; i<=1497; i++) // Guardian
            total += tilesiz[i].x*tilesiz[i].y;
        for (i=1504; i<=1518; i++)
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 1580:
        for (i=1580; i<=1644; i++) // Little Ripper
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 4320:
        for (i=4320; i<=4427; i++) // Big Ripper
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 2540:
        for (i=2540; i<=2546; i++) // Trashcan
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 4430:
        for (i=4430; i<=4479; i++) // Fish
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 4490:
        for (i=4490; i<=4544; i++) // Sumo
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 5023:
        for (i=5023; i<=5026; i++) // Toilet Girl
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 5032:
        for (i=5032; i<=5035; i++) // Wash Girl
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 2000:
        for (i=2000; i<=2002; i++) // Chop Stick Panel
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 2004:
        for (i=2004; i<=2009; i++) // Uzi Panel
            total += tilesiz[i].x*tilesiz[i].y;
        for (i=2040; i<=2043; i++) // Uzi Overlays
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 2010:
        for (i=2010; i<=2019; i++) // Rail Panel
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 2130:
        for (i=2130; i<=2137; i++) // Shuriken Panel
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 2050:
        for (i=2050; i<=2053; i++) // Heart Panel
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 2054:
        for (i=2054; i<=2057; i++) // HotHead Panel
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 2070:
        for (i=2070; i<=2077; i++) // Rocket Launcher Panel
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 2080:
        for (i=2080; i<=2083; i++) // Sword Panel
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 4090:
        for (i=4090; i<=4093; i++) // Bloody Sword Panel
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 2121:
        for (i=2121; i<=2126; i++) // 40MM Panel
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 2211:
        for (i=2211; i<=2216; i++) // Shotgun Panel
            total += tilesiz[i].x*tilesiz[i].y;
        for (i=2225; i<=2227; i++) // Shotgun Quad-Mode Panel
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    case 2220:
        for (i=2220; i<=2224; i++) // Sticky Bomb Panel
            total += tilesiz[i].x*tilesiz[i].y;
        break;
    }

    return total;
}

void CON_Cache(void)
{
    char incache[8192]; // 8192 so it can index maxwalls as well
    int i,j,tottiles,totsprites,totactors;


    memset(incache,0,8192);

    // Calculate all level tiles, non-actor stuff
    for (i=0; i<numsectors; i++)
    {
        incache[sector[i].ceilingpicnum] = 1;
        incache[sector[i].floorpicnum] = 1;
    }

    for (i=0; i<numwalls; i++)
    {
        incache[wall[i].picnum] = 1;
        if (wall[i].overpicnum >= 0)
            incache[wall[i].overpicnum] = 1;
    }

    tottiles = 0;
    for (i=0; i<8192; i++)
        if (incache[i] > 0)
            tottiles += tilesiz[i].x*tilesiz[i].y;

    //////////////////////////////////////////////

    memset(incache,0,8192);

    // Sprites on the stat list get counted as cached, others don't
    for (i=0; i<MAXSPRITES; i++)
        if (sprite[i].statnum < MAXSTATUS)
            incache[sprite[i].picnum] = 1;

    totsprites = 0;
    totactors = 0;

    for (i=0; i<MAXSPRITES; i++)
    {
        if (incache[i] > 0)
        {
            switch (i)
            {
            case 4096:
                totactors+=TileRangeMem(4096);
                incache[4096]=0;
                break;
            case 800:
                totactors+=TileRangeMem(800);
                incache[800]=0;
                break;
            case 817:
                totactors+=TileRangeMem(817);
                incache[817]=0;
                break;
            case 820:
                totactors+=TileRangeMem(820);
                incache[820]=0;
                break;
            case 960:
                totactors+=TileRangeMem(960);
                incache[960]=0;
                break;
            //case 1024:   // Lo Wang is calculated later
            //     totactors+=TileRangeMem(1024);
            //     incache[1024]=0;
            //break;
            case 1320:
                totactors+=TileRangeMem(1320);
                incache[1320]=0;
                break;
            case 1400:
                totactors+=TileRangeMem(1400);
                incache[1400]=0;
                break;
            case 1441:
                totactors+=TileRangeMem(1441);
                incache[1441]=0;
                break;
            case 1469:
                totactors+=TileRangeMem(1469);
                incache[1469]=0;
                break;
            case 1580:
                totactors+=TileRangeMem(1580);
                incache[1580]=0;
                break;
            case 4320:
                totactors+=TileRangeMem(4320);
                incache[4320]=0;
                break;
            case 2540:
                totactors+=TileRangeMem(2540);
                incache[2540]=0;
                break;
            case 4430:
                totactors+=TileRangeMem(4430);
                incache[4430]=0;
                break;
            case 4490:
                totactors+=TileRangeMem(4490);
                incache[4490]=0;
                break;
            case 5023:
                totactors+=TileRangeMem(5023);
                incache[5023]=0;
                break;
            case 5032:
                totactors+=TileRangeMem(5032);
                incache[5032]=0;
                break;
            case 2000:
                totactors+=TileRangeMem(2000);
                incache[2000]=0;
                break;
            case 2004:
                totactors+=TileRangeMem(2004);
                incache[2004]=0;
                break;
            case 2010:
                totactors+=TileRangeMem(2010);
                incache[2010]=0;
                break;
            case 2130:
                totactors+=TileRangeMem(2130);
                incache[2130]=0;
                break;
            case 2050:
                totactors+=TileRangeMem(2050);
                incache[2050]=0;
                break;
            case 2054:
                totactors+=TileRangeMem(2054);
                incache[2054]=0;
                break;
            case 2070:
                totactors+=TileRangeMem(2070);
                incache[2070]=0;
                break;
            case 2080:
                totactors+=TileRangeMem(2080);
                incache[2080]=0;
                break;
            case 4090:
                totactors+=TileRangeMem(4090);
                incache[4090]=0;
                break;
            case 2121:
                totactors+=TileRangeMem(2121);
                incache[2121]=0;
                break;
            case 2211:
                totactors+=TileRangeMem(2211);
                incache[2211]=0;
                break;
            case 2220:
                totactors+=TileRangeMem(2220);
                incache[2220]=0;
                break;

            default: totsprites += tilesiz[i].x*tilesiz[i].y;
            }
        }
    }

    CON_ConMessage("/////////////////////////////////////////////");
    CON_ConMessage("Current Memory Consumption:");
    CON_ConMessage("Total Tiles        = %d",tottiles);
    CON_ConMessage("Total Sprites      = %d",totsprites);
    CON_ConMessage("Total Actors       = %d",totactors);
    CON_ConMessage("Total Memory       = %d",(tottiles+totsprites+totactors));
    CON_ConMessage("Total with LoWang  = %d",(tottiles+totsprites+totactors+TileRangeMem(1024)));
    CON_ConMessage("/////////////////////////////////////////////");

}

void CON_SpriteInfo(void)
{
    SpriteInfo++;
    if (SpriteInfo > 2) SpriteInfo = 0;

    if (SpriteInfo == 0)
        CON_ConMessage("Sprite information is OFF.");
    else if (SpriteInfo == 1)
        CON_ConMessage("Sprite information is ON (Brief Mode).");
    else
        CON_ConMessage("Sprite information is ON (Verbose Mode).");
}

void CON_KillSprite(void)
{
    char base[80];
    int16_t op1=0;
    SPRITEp sp;
    short i;
    USERp u;

    // Format: kill [SpriteNum]
    if (sscanf(MessageInputString,"%s %hd",base,&op1) < 2)
    {
        strcpy(MessageInputString,"help kill");
        CON_GetHelp();
        return;
    }

    if (op1 == -1)
    {
        for (i=0; i<MAXSPRITES; i++)
        {
            u = User[i];
            if (!u->PlayerP)
                SetSuicide(i);
        }
        CON_ConMessage("Killed all sprites except Players.");
    }
    else
    {
        if (!CheckValidSprite(op1)) return;

        SetSuicide(op1);
        CON_ConMessage("Killed sprite %d.",op1);
    }

}

void CON_SpriteDetail(void)
{
    char base[80];
    int16_t op1=0;
    SPRITEp sp;
    short i;

    // Format: showsprite [SpriteNum]
    if (sscanf(MessageInputString,"%s %hd",base,&op1) < 2)
    {
        strcpy(MessageInputString,"help showsprite");
        CON_GetHelp();
        return;
    }

    if (!CheckValidSprite(op1)) return;
    sp = &sprite[op1];

    CON_ConMessage("x = %d, y = %d, z = %d",sp->x,sp->y,sp->z);
    CON_ConMessage("cstat = %d, picnum = %d",sp->cstat,sp->picnum);
    CON_ConMessage("shade = %d, pal = %d, clipdist = %d",sp->shade,sp->pal,sp->clipdist);
    CON_ConMessage("xrepeat = %d, yrepeat = %d",sp->xrepeat, sp->yrepeat);
    CON_ConMessage("xoffset = %d, yoffset = %d",sp->xoffset, sp->yoffset);
    CON_ConMessage("sectnum = %d, statnum = %d",sp->sectnum, sp->statnum);
    CON_ConMessage("ang = %d, owner = %d",sp->ang,sp->owner);
    CON_ConMessage("xvel = %d, yvel = %d, zvel = %d",sp->xvel,sp->yvel,sp->zvel);
    CON_ConMessage("lotag = %d, hitag = %d, extra = %d",sp->lotag,sp->hitag,sp->extra);
}

void CON_UserDetail(void)
{
    char base[80];
    int16_t op1=0;
    SPRITEp sp;
    short i;
    USERp u;

    // Format: showuser [SpriteNum]
    if (sscanf(MessageInputString,"%s %hd",base,&op1) < 2)
    {
        strcpy(MessageInputString,"help showsprite");
        CON_GetHelp();
        return;
    }

    if (!CheckValidSprite(op1)) return;
    sp = &sprite[op1];
    u = User[op1];

    if (!u) return;

    CON_ConMessage("State = %p, Rot = %p",u->State,u->Rot);
    CON_ConMessage("StateStart = %p, StateEnd = %p",u->StateStart,u->StateEnd);
    CON_ConMessage("ActorActionFunc = %p",u->ActorActionFunc);
    CON_ConMessage("ActorActionSet = %p",u->ActorActionSet);
    CON_ConMessage("Personality = %p",u->Personality);
    CON_ConMessage("Attrib = %p",u->Attrib);
    CON_ConMessage("Flags = %d, Flags2 = %d, Tics = %d",u->Flags,u->Flags2,u->Tics);
    CON_ConMessage("RotNum = %d, ID = %d",u->RotNum,u->ID);
    CON_ConMessage("Health = %d, MaxHealth = %d",u->Health,u->MaxHealth);
    CON_ConMessage("LastDamage = %d, PainThreshold = %d",u->LastDamage,u->PainThreshold);
    CON_ConMessage("jump_speed = %d, jump_grav = %d",u->jump_speed,u->jump_grav);
    CON_ConMessage("xchange = %d, ychange = %d, zchange = %d",u->xchange,u->ychange,u->zchange);
    CON_ConMessage("ret = %d, WaitTics = %d, spal = %d",u->ret,u->WaitTics,u->spal);
}

void CON_Quit(void)
{
    if (CommPlayers >= 2)
        MultiPlayQuitFlag = TRUE;
    else
        QuitFlag = TRUE;
}

void CON_MultiNameChange(void)
{
    char base[16],command[16];

    // Format: swname [name]
    if (sscanf(MessageInputString,"%6s %12s",base,command) < 2)
        return;

    SendMulitNameChange(command);
}

void CON_LoadSetup(void)
{
    /*
    char base[80],command[80];
    extern char setupfilename[64];

    // Format: showuser [SpriteNum]
    if (sscanf(MessageInputString,"%s %s",base,command) < 2)
    {
    strcpy(MessageInputString,"help config");
    CON_GetHelp();
    return;
    }

    if (!SafeFileExists(command))
    {
    CON_ConMessage("CON_LoadSetup: %s does not exist.",command);
    return;
    } else
    {
    strcpy(setupfilename,command);
    }
    initkeys();
    CONFIG_ReadSetup();
    CONTROL_Startup( ControllerType, &GetTime, 120 );
    SetupGameButtons();

    if (CONTROL_JoystickEnabled)
    {
    CONTROL_CenterJoystick(CenterCenter, UpperLeft, LowerRight, CenterThrottle,
      CenterRudder);
    }
    CON_ConMessage("Loaded new config file.");
    */
    CON_ConMessage("JonoF: Maybe later");
}

char *damagename[] =
{
    "WPN_STAR","WPN_UZI",
    "WPN_SHOTGUN","WPN_MICRO",
    "WPN_GRENADE","WPN_MINE",
    "WPN_RAIL","WPN_HEART",
    "WPN_HOTHEAD","WPN_NAPALM"
    "WPN_RING","WPN_ROCKET",
    "WPN_SWORD","WPN_FIST",
    "DMG_NAPALM","DMG_MIRV_METEOR",
    "DMG_SERP_METEOR","DMG_ELECTRO_SHARD",
    "DMG_SECTOR_EXP","DMG_BOLT_EXP",
    "DMG_TANK_SHELL_EXP","DMG_FIREBALL_EXP",
    "DMG_NAPALM_EXP","DMG_SKULL_EXP",
    "DMG_BASIC_EXP","DMG_GRENADE_EXP",
    "DMG_MINE_EXP","DMG_MINE_SHRAP",
    "DMG_MICRO_EXP","DMG_NUCLEAR_EXP",
    "DMG_RADIATION_CLOUD","DMG_FLASHBOMB",
    "DMG_FIREBALL_FLAMES","DMG_RIPPER_SLASH",
    "DMG_SKEL_SLASH","DMG_COOLG_BASH",
    "DMG_COOLG_FIRE","DMG_GORO_CHOP",
    "DMG_GORO_FIREBALL","DMG_SERP_SLASH",
    "DMG_LAVA_BOULDER","DMG_LAVA_SHARD",
    "DMG_HORNET_STING","DMG_EEL_ELECTRO",
    "DMG_SPEAR_TRAP","DMG_VOMIT",
    "DMG_BLADE"
};

void CON_DamageData(void)
{

    char base[80],field[80];
    int16_t op1=0;
    unsigned int op2, i;
    SPRITEp sp;
    USERp u;

    // Format: damage [field] [item] [value]
    if (sscanf(MessageInputString,"%s %s %hd %u",base,field,&op1,&op2) < 3)
    {
        strcpy(MessageInputString,"help damage");
        CON_GetHelp();
        return;
    }

    if (op1 < -1 || op1 > 46)
    {
        CON_ConMessage("Damage Data index is out of range.");
        return;
    }

    if (!strcmp(field,"damage_lo"))
    {
        DamageData[op1].damage_lo = op2;
        CON_ConMessage("DamageData[%s].damage_lo = %d",damagename[op1],op2);
    }
    else if (!strcmp(field,"damage_hi"))
    {
        DamageData[op1].damage_hi = op2;
        CON_ConMessage("DamageData[%s].damage_hi = %d",damagename[op1],op2);
    }
    else if (!strcmp(field,"radius"))
    {
        DamageData[op1].radius = op2;
        CON_ConMessage("DamageData[%s].radius = %d",damagename[op1],op2);
    }
    else if (!strcmp(field,"max_ammo"))
    {
        DamageData[op1].max_ammo = op2;
        CON_ConMessage("DamageData[%s].max_ammo = %d",damagename[op1],op2);
    }
    else if (!strcmp(field,"min_ammo"))
    {
        DamageData[op1].min_ammo = op2;
        CON_ConMessage("DamageData[%s].min_ammo = %d",damagename[op1],op2);
    }
    if (!strcmp(field,"show"))
    {
        if (op1 == -1)
        {
            for (i=op2; i<=op2+10; i+=2)
            {
                if (i<47)
                    CON_ConMessage("[%d] = %s  [%d] = %s",i,damagename[i],i+1,damagename[i+1]);
            }
        }
        else
        {
            CON_ConMessage(" ");
            CON_ConMessage("Item = %s:",damagename[op1]);
            CON_ConMessage("damage_lo = %d, damag_hi = %d",DamageData[op1].damage_lo,DamageData[op1].damage_hi);
            CON_ConMessage("radius = %u",DamageData[op1].radius);
            CON_ConMessage("min_ammo = %d, max_ammo = %d",DamageData[op1].min_ammo,DamageData[op1].max_ammo);
            CON_ConMessage(" ");
        }
    }
}

void CON_WinPachinko(void)
{
    extern SWBOOL Pachinko_Win_Cheat;
    PLAYERp pp = Player + myconnectindex;
    extern void CheckSndData(char *file, int line);

    if (CommEnabled)
        return;

    Pachinko_Win_Cheat = !Pachinko_Win_Cheat;

    //CheckSndData( __FILE__, __LINE__ );

    if (Pachinko_Win_Cheat)
        PutStringInfo(pp,"Pachinko Win Cheat Enabled");
    else
        PutStringInfo(pp,"Pachinko Win Cheat Disabled");
}

void CON_Tweak(void)
{
    char base[80], command[80];
    int op1=0;

    // Format: tweak [weapon] [number]
    if (sscanf(MessageInputString,"%s %s %d",base,command,&op1) < 3)
    {
        strcpy(MessageInputString,"help tweak");
        CON_GetHelp();
        return;
    }

    Bstrlwr(command);    // Make sure operator is all lower case
    if (!strcmp(command,"adjust"))
    {
        extern short ADJUST;
        ADJUST = op1;
        CON_ConMessage("Zvelocity ADJUST set to %d.",op1);
    }
    else if (!strcmp(command,"adjustv"))
    {
        extern int ADJUSTV;
        ADJUSTV = op1;
        CON_ConMessage("Zvelocity ADJUSTV set to %d.",op1);
    }
}

void CON_Bunny(void)
{
    PLAYERp pp = Player + myconnectindex;

    if (CommEnabled)
        return;

    pp->BunnyMode = !pp->BunnyMode;
    if (pp->BunnyMode)
        PutStringInfo(pp,"Bunny rockets enabled!");
    else
        PutStringInfo(pp,"Bunny rockets disabled!");
}

void CON_CheckHeap(void)
{
    /*
    switch( _heapchk() )
    {
    case _HEAPOK:
      CON_ConMessage( "OK - heap is good\n" );
      break;
    case _HEAPEMPTY:
      CON_ConMessage( "OK - heap is empty\n" );
      break;
    case _HEAPBADBEGIN:
      CON_ConMessage( "ERROR - heap is damaged\n" );
      break;
    case _HEAPBADNODE:
      CON_ConMessage( "ERROR - bad node in heap\n" );
      break;
    }
    */
    CON_ConMessage("JonoF: Not now");
}

/*
void heap_dump( void )
  {
    struct _heapinfo h_info;
    int heap_status;

    h_info._pentry = NULL;
    for(;;) {
      heap_status = _heapwalk( &h_info );
      if( heap_status != _HEAPOK ) break;
      printf( "  %s block at %Fp of size %4.4X\n",
        (h_info._useflag == _USEDENTRY ? "USED" : "FREE"),
        h_info._pentry, h_info._size );

    }

    switch( heap_status ) {
    case _HEAPEND:
      printf( "OK - end of heap\n" );
      break;
    case _HEAPEMPTY:
      printf( "OK - heap is empty\n" );
      break;
    case _HEAPBADBEGIN:
      printf( "ERROR - heap is damaged\n" );
      break;
    case _HEAPBADPTR:
      printf( "ERROR - bad pointer to heap\n" );
      break;
    case _HEAPBADNODE:

      printf( "ERROR - bad node in heap\n" );
    }
  }
  */

void CON_DumpHeap(void)
{
    //heap_dump(); // Dump it.
    CON_ConMessage("JonoF: Not now");
}

void CON_ShowMirror(void)
{
    char base[80];
    int16_t op1=0;

    // Format: showmirror [SpriteNum]
    if (sscanf(MessageInputString,"%s %hd",base,&op1) < 2)
    {
        strcpy(MessageInputString,"help showmirror");
        CON_GetHelp();
        return;
    }

    if (op1 < 0 || op1 > 9)
    {
        CON_ConMessage("Mirror number is out of range!");
        return;
    }

    CON_ConMessage("camera is the ST1 sprite used as the view spot");
    CON_ConMessage("camspite is the SpriteNum of the drawtotile tile in editart");
    CON_ConMessage("camspic is the tile number of the drawtotile in editart");
    CON_ConMessage("iscamera is whether or not this mirror is a camera type");
    CON_ConMessage(" ");
    CON_ConMessage("mirror[%d].mirrorwall = %d",op1,mirror[op1].mirrorwall);
    CON_ConMessage("mirror[%d].mirrorsector = %d",op1,mirror[op1].mirrorsector);
    CON_ConMessage("mirror[%d].camera = %d",op1,mirror[op1].camera);
    CON_ConMessage("mirror[%d].camsprite = %d",op1,mirror[op1].camsprite);
    CON_ConMessage("mirror[%d].campic = %d",op1,mirror[op1].campic);
    CON_ConMessage("mirror[%d].iscamera = %d",op1,mirror[op1].ismagic);
}

void CON_DumpSoundList(void)
{
    extern void DumpSounds(void);

    DumpSounds();
    CON_Message("Sounds dumped to dbg.foo");

}

