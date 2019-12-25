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

#include "ns.h"

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
#include "gamecontrol.h"
#include "trigger.h"

#include "savedef.h"
#include "menus.h"
#include "network.h"
#include "pal.h"

#include "weapon.h"
#include "text.h"
#include "jsector.h"

BEGIN_SW_NS

// DEFINES ///////////////////////////////////////////////////////////////////////////////////
#define MAX_USER_ARGS           100
#define MAX_CONSOLE_COMMANDS    100
#define MAX_HISTORY             20

SWBOOL SpriteInfo = FALSE;
extern SWBOOL QuitFlag;
extern SWBOOL MultiPlayQuitFlag;

// FUNCTION PROTOTYPES ///////////////////////////////////////////////////////////////////////
void CON_ProcessOptions(void);
void CON_ClearConsole(void);
uint8_t CON_CommandCmp(const char *str1, const char *str2, int len);
void CheatInput(void);

// Modify actor routines
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
void CON_Bunny(void);
void CON_CheckHeap(void);
void CON_DumpHeap(void);
void CON_ShowMirror(void);
void CON_MultiNameChange(void);

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
    {"showmirror",  CON_ShowMirror},
    {"clear",       CON_ClearConsole},
#endif
	{"swgod",        CheatInput},
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
	{"swmedic",      CheatInput},
	{"swkeys",       CheatInput},
	{"swredcard",    CheatInput},
	{"swbluecard",   CheatInput},
	{"swgreencard",  CheatInput},
	{"swyellowcard", CheatInput},
	{"swgoldkey",    CheatInput},
	{"swsilverkey",  CheatInput},
	{"swbronzekey",  CheatInput},
	{"swredkey",     CheatInput},
	{"swgun#",       CheatInput},
	{"swquit",       CheatInput},
	{"swexit",       CheatInput},
    {"swtrix",      CON_Bunny},
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

SWBOOL IsCommand(const char *str)
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
        OSD_Printf("Syntax Error or Command not enabled!");
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

void CON_Reverb(void)
{
    char base[80];
    int16_t op1=0;
    PLAYERp pp = Player + screenpeek;

    // Format: reverb [number]
    if (sscanf(MessageInputString,"%s %hd",base,&op1) < 2)
    {
        strcpy(MessageInputString,"help reverb");
        return;
    }

    OSD_Printf("Reverb is now set to %d.",op1);
    COVER_SetReverb(op1);
    pp->Reverb = op1;
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

void CON_ShowMirror(void)
{
    char base[80];
    int16_t op1=0;

    // Format: showmirror [SpriteNum]
    if (sscanf(MessageInputString,"%s %hd",base,&op1) < 2)
    {
        strcpy(MessageInputString,"help showmirror");
        return;
    }

    if (op1 < 0 || op1 > 9)
    {
        OSD_Printf("Mirror number is out of range!");
        return;
    }

    OSD_Printf("camera is the ST1 sprite used as the view spot");
    OSD_Printf("camspite is the SpriteNum of the drawtotile tile in editart");
    OSD_Printf("camspic is the tile number of the drawtotile in editart");
    OSD_Printf("iscamera is whether or not this mirror is a camera type");
    OSD_Printf(" ");
    OSD_Printf("mirror[%d].mirrorwall = %d",op1,mirror[op1].mirrorwall);
    OSD_Printf("mirror[%d].mirrorsector = %d",op1,mirror[op1].mirrorsector);
    OSD_Printf("mirror[%d].camera = %d",op1,mirror[op1].camera);
    OSD_Printf("mirror[%d].camsprite = %d",op1,mirror[op1].camsprite);
    OSD_Printf("mirror[%d].campic = %d",op1,mirror[op1].campic);
    OSD_Printf("mirror[%d].iscamera = %d",op1,mirror[op1].ismagic);
}


END_SW_NS
