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

// scriplib.c
#include "build.h"
#include "cache1d.h"

#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "game.h"

#include "parse.h"
#include "sprite.h"
#include "jsector.h"
#include "parent.h"

#define PATHSEPERATOR   '\\'

//#define COMPUTE_TOTALS    1

extern int nextvoxid;   // JBF: in game.c

ParentalStruct aVoxelArray[MAXTILES];


/*
=============================================================================

                        PARSING STUFF

=============================================================================
*/

char    token[MAXTOKEN];
char    *scriptbuffer,*script_p,*scriptend_p;
int     grabbed;
int     scriptline;
SWBOOL    endofscript;
SWBOOL    tokenready;                     // only TRUE if UnGetToken was just called

/*
==============
=
= LoadScriptFile
=
==============
*/

SWBOOL LoadScriptFile(const char *filename)
{
    int size, readsize;
    int fp;


    if ((fp=kopen4load(filename,0)) == -1)
    {
        // If there's no script file, forget it.
        return FALSE;
    }

    size = kfilelength(fp);

    scriptbuffer = (char *)AllocMem(size);

    ASSERT(scriptbuffer != NULL);

    readsize = kread(fp, scriptbuffer, size);

    kclose(fp);

    ASSERT(readsize == size);


    // Convert filebuffer to all upper case
    //strupr(scriptbuffer);

    script_p = scriptbuffer;
    scriptend_p = script_p + size;
    scriptline = 1;
    endofscript = FALSE;
    tokenready = FALSE;
    return TRUE;
}


/*
==============
=
= UnGetToken
=
= Signals that the current token was not used, and should be reported
= for the next GetToken.  Note that

GetToken (TRUE);
UnGetToken ();
GetToken (FALSE);

= could cross a line boundary.
=
==============
*/

void UnGetToken(void)
{
    tokenready = TRUE;
}


/*
==============
=
= GetToken
=
==============
*/

void GetToken(SWBOOL crossline)
{
    char    *token_p;

    if (tokenready)                         // is a token already waiting?
    {
        tokenready = FALSE;
        return;
    }

    if (script_p >= scriptend_p)
    {
        if (!crossline)
            buildprintf("Error: Line %i is incomplete\n",scriptline);
        endofscript = TRUE;
        return;
    }

//
// skip space
//
skipspace:
    while (*script_p <= 32)
    {
        if (script_p >= scriptend_p)
        {
            if (!crossline)
                buildprintf("Error: Line %i is incomplete\n",scriptline);
            endofscript = TRUE;
            return;
        }
        if (*script_p++ == '\n')
        {
            if (!crossline)
                buildprintf("Error: Line %i is incomplete\n",scriptline);
            scriptline++;
        }
    }

    if (script_p >= scriptend_p)
    {
        if (!crossline)
            buildprintf("Error: Line %i is incomplete\n",scriptline);
        endofscript = TRUE;
        return;
    }

    if (*script_p == '#')   // # is comment field
    {
        if (!crossline)
            buildprintf("Error: Line %i is incomplete\n",scriptline);
        while (*script_p++ != '\n')
            if (script_p >= scriptend_p)
            {
                endofscript = TRUE;
                return;
            }
        goto skipspace;
    }

//
// copy token
//
    token_p = token;

    while (*script_p > 32 && *script_p != '#')
    {
        *token_p++ = *script_p++;
        if (script_p == scriptend_p)
            break;
        ASSERT(token_p != &token[MAXTOKEN]);
//          buildprintf("Error: Token too large on line %i\n",scriptline);
    }

    *token_p = 0;
}


/*
==============
=
= TokenAvailable
=
= Returns true if there is another token on the line
=
==============
*/

SWBOOL TokenAvailable(void)
{
    char    *search_p;

    search_p = script_p;

    if (search_p >= scriptend_p)
        return FALSE;

    while (*search_p <= 32)
    {
        if (*search_p == '\n')
            return FALSE;
        search_p++;
        if (search_p == scriptend_p)
            return FALSE;

    }

    if (*search_p == '#')
        return FALSE;

    return TRUE;
}


// Load all the voxel files using swvoxfil.txt script file
// Script file format:

//          # - Comment
//          spritenumber (in artfile), voxel number, filename
//          Ex. 1803 0 medkit2.kvx
//              1804 1 shotgun.kvx
//              etc....

void LoadKVXFromScript(const char *filename)
{
    int lNumber=0,lTile=0; // lNumber is the voxel no. and lTile is the editart tile being
    // replaced.
    char *sName;            // KVS file being loaded in.

    int grabbed=0;          // Number of lines parsed

    sName = (char *)AllocMem(256);    // Up to 256 bytes for path
    ASSERT(sName != NULL);

    // zero out the array memory with -1's for pics not being voxelized
    memset(&aVoxelArray[0],-1,sizeof(struct TILE_INFO_TYPE)*MAXTILES);
    for (grabbed = 0; grabbed < MAXTILES; grabbed++)
    {
        aVoxelArray[grabbed].Voxel = -1;
        aVoxelArray[grabbed].Parental = -1;
    }

    grabbed = 0;

    // Load the file
    if (!LoadScriptFile(filename))
        ASSERT(TRUE==FALSE);

    do
    {
        GetToken(TRUE);     // Crossing a line boundary on the end of line to first token
        // of a new line is permitted (and expected)
        if (endofscript)
            break;

        lTile = atol(token);

        GetToken(FALSE);
        lNumber = atol(token);

        GetToken(FALSE);
        strcpy(sName,token);            // Copy the whole token as a file name and path

        // Load the voxel file into memory
        if (!qloadkvx(lNumber,sName))
        {
            // Store the sprite and voxel numbers for later use
            aVoxelArray[lTile].Voxel = lNumber; // Voxel num
        }

        if (lNumber >= nextvoxid)   // JBF: so voxels in the def file append to the list
            nextvoxid = lNumber + 1;

        grabbed++;
        ASSERT(grabbed < MAXSPRITES);

    }
    while (script_p < scriptend_p);

    FreeMem(scriptbuffer);
    FreeMem(sName);
    script_p = NULL;
}

// Load in info for all Parental lock tile targets
//          # - Comment
//          tilenumber (in artfile), replacement tile offset (if any)
//          Ex. 1803 -1       -1 = No tile replacement
//              1804 2000
//              etc....
void LoadPLockFromScript(const char *filename)
{
    int lNumber=0,lTile=0; // lNumber is the voxel no. and lTile is the editart tile being
    // replaced.
    char *sName;            // KVS file being loaded in.

    int grabbed=0;          // Number of lines parsed

    sName = (char *)AllocMem(256);    // Up to 256 bytes for path
    ASSERT(sName != NULL);

    // Load the file
    if (!LoadScriptFile(filename))
        ASSERT(TRUE==FALSE);

    do
    {
        GetToken(TRUE);     // Crossing a line boundary on the end of line to first token
        // of a new line is permitted (and expected)
        if (endofscript)
            break;

        lTile = atoi(token);

        GetToken(FALSE);
        lNumber = atoi(token);

        // Store the sprite and voxel numbers for later use
        aVoxelArray[lTile].Parental = lNumber;  // Replacement to tile, -1 for none

        grabbed++;
        ASSERT(grabbed < MAXSPRITES);

    }
    while (script_p < scriptend_p);

    FreeMem(scriptbuffer);
    FreeMem(sName);
    script_p = NULL;
}


/*
 * Here begins JonoF's modding enhancement stuff
 */

#include "scriptfile.h"

enum
{
    CM_MAP,
    CM_EPISODE,
    CM_TITLE,
    CM_FILENAME,
    CM_SONG,
    CM_CDATRACK,
    CM_BESTTIME,
    CM_PARTIME,
    CM_SUBTITLE,
    CM_SKILL,
    CM_TEXT,
    CM_COOKIE,
    CM_GOTKEY,
    CM_NEEDKEY,
    CM_INVENTORY,
    CM_AMOUNT,
    CM_WEAPON,
    CM_AMMONAME,
    CM_MAXAMMO,
    CM_DAMAGEMIN,
    CM_DAMAGEMAX,
    CM_SECRET,
    CM_QUIT,
};

static const struct _tokset
{
    const char *str;
    int tokn;
} cm_tokens[] =
{
    { "map",         CM_MAP       },
    { "level",       CM_MAP       },
    { "episode",     CM_EPISODE   },
    { "skill",       CM_SKILL     },
    { "cookie",      CM_COOKIE    },
    { "fortune",     CM_COOKIE    },
    { "gotkey",      CM_GOTKEY    },
    { "inventory",   CM_INVENTORY },
    { "weapon",      CM_WEAPON    },
    { "needkey",     CM_NEEDKEY   },
    { "secret",      CM_SECRET    },
    { "quit",        CM_QUIT      },
},
  cm_map_tokens[] =
{
    { "title",       CM_TITLE     },
    { "name",        CM_TITLE     },
    { "description", CM_TITLE     },
    { "filename",    CM_FILENAME  },
    { "file",        CM_FILENAME  },
    { "fn",          CM_FILENAME  },
    { "levelname",   CM_FILENAME  },
    { "song",        CM_SONG      },
    { "music",       CM_SONG      },
    { "songname",    CM_SONG      },
    { "cdatrack",    CM_CDATRACK  },
    { "cdtrack",     CM_CDATRACK  },
    { "besttime",    CM_BESTTIME  },
    { "partime",     CM_PARTIME   },
},
  cm_episode_tokens[] =
{
    { "title",       CM_TITLE     },
    { "name",        CM_TITLE     },
    { "description", CM_TITLE     },
    { "subtitle",    CM_SUBTITLE  },
},
  cm_skill_tokens[] =
{
    { "title",       CM_TITLE     },
    { "name",        CM_TITLE     },
    { "description", CM_TITLE     },
},
  cm_inventory_tokens[] =
{
    { "title",       CM_TITLE     },
    { "name",        CM_TITLE     },
    { "description", CM_TITLE     },
    { "amount",      CM_AMOUNT    },
},
  cm_weapons_tokens[] =
{
    { "title",       CM_TITLE     },
    { "name",        CM_TITLE     },
    { "description", CM_TITLE     },
    { "ammoname",    CM_AMMONAME  },
    { "maxammo",     CM_MAXAMMO   },
    { "mindamage",   CM_DAMAGEMIN },
    { "maxdamage",   CM_DAMAGEMAX },
    { "pickup",      CM_AMOUNT    },
    { "weaponpickup",CM_WEAPON    },
}
;
#define cm_numtokens           (sizeof(cm_tokens)/sizeof(cm_tokens[0]))
#define cm_map_numtokens       (sizeof(cm_map_tokens)/sizeof(cm_map_tokens[0]))
#define cm_episode_numtokens   (sizeof(cm_episode_tokens)/sizeof(cm_episode_tokens[0]))
#define cm_skill_numtokens     (sizeof(cm_skill_tokens)/sizeof(cm_skill_tokens[0]))
#define cm_inventory_numtokens (sizeof(cm_inventory_tokens)/sizeof(cm_inventory_tokens[0]))
#define cm_weapons_numtokens   (sizeof(cm_weapons_tokens)/sizeof(cm_weapons_tokens[0]))


static int cm_transtok(const char *tok, const struct _tokset *set, const unsigned num)
{
    unsigned i;

    for (i=0; i<num; i++)
    {
        if (!Bstrcasecmp(tok, set[i].str))
            return set[i].tokn;
    }

    return -1;
}

// Load custom map and episode information
//   level # {
//      title    "Map Name"
//      filename "filename.map"
//      song     "filename.mid"
//      cdatrack n
//      besttime secs
//      partime  secs
//   }
//
//   episode # {
//      title    "Episode Name"
//      subtitle "Caption text"
//   }
//
//   skill # {
//      name "Tiny grasshopper"
//   }
//
//   fortune {
//      "You never going to score."
//      "26-31-43-82-16-29"
//      "Sorry, you no win this time, try again."
//   }
//   gotkey {
//      "Got the RED key!"
//      "Got the BLUE key!"
//      "Got the GREEN key!"
//      ...
//   }
//   needkey {
//      "You need a RED key for this door."
//      "You need a BLUE key for this door."
//      "You need a GREEN key for this door."
//      ...
//   }
//   inventory # { name "Armour" amount 50 }
//   weapon # { name "Uzi Submachine Gun" ammoname "Uzi Clip" maxammo 200 mindamage 5 maxdamage 7 pickup 50 }
//   secret  "You found a secret area!"
//   quit    "PRESS (Y) TO QUIT, (N) TO FIGHT ON."

static LEVEL_INFO custommaps[MAX_LEVELS_REG];
static char *customfortune[MAX_FORTUNES];
static char *customkeymsg[MAX_KEYS];
static char *customkeydoormsg[MAX_KEYS];
static char *custominventoryname[InvDecl_TOTAL];
static char *customweaponname[2][MAX_WEAPONS];  // weapon, ammo

#define WM_DAMAGE  1
#define WM_WEAP   2
#define WM_AMMO   4
static struct
{
    const char *sym;
    int dmgid;
    int editable;
} weaponmap[] =
{
    { "WPN_FIST",       WPN_FIST,        WM_DAMAGE },
    { "WPN_SWORD",      WPN_SWORD,       WM_DAMAGE },
    { "WPN_SHURIKEN",   WPN_STAR,        WM_DAMAGE|WM_WEAP },
    { "WPN_STICKYBOMB", WPN_MINE,        WM_DAMAGE|WM_WEAP },
    { "WPN_UZI",        WPN_UZI,         WM_DAMAGE|WM_WEAP|WM_AMMO },
    { "WPN_MISSILE",    WPN_MICRO,       WM_DAMAGE|WM_WEAP|WM_AMMO },
    { "WPN_NUKE",       DMG_NUCLEAR_EXP, WM_DAMAGE|WM_WEAP|WM_AMMO },
    { "WPN_GRENADE",    WPN_GRENADE,     WM_DAMAGE|WM_WEAP|WM_AMMO },
    { "WPN_RAILGUN",    WPN_RAIL,        WM_DAMAGE|WM_WEAP|WM_AMMO },
    { "WPN_SHOTGUN",    WPN_SHOTGUN,     WM_DAMAGE|WM_WEAP|WM_AMMO },
    { "WPN_HOTHEAD",    WPN_HOTHEAD,     WM_DAMAGE|WM_WEAP },
    { "WPN_HEART",      WPN_HEART,       WM_DAMAGE|WM_WEAP },
    { "WPN_HOTHEAD_NAPALM", WPN_NAPALM,  WM_DAMAGE },
    { "WPN_HOTHEAD_RING",   WPN_RING,    WM_DAMAGE },
};

// FIXME: yes, we are leaking memory here at the end of the program by not freeing anything
void LoadCustomInfoFromScript(const char *filename)
{
    scriptfile *script;
    char *token;
    char *braceend;
    int curmap = -1;

    script = scriptfile_fromfile(filename);
    if (!script) return;

    // predefine constants for some stuff to give convenience and eliminate the need for a 'define' directive
    scriptfile_addsymbolvalue("INV_ARMOR",      1+InvDecl_Armor);
    scriptfile_addsymbolvalue("INV_KEVLAR",     1+InvDecl_Kevlar);
    scriptfile_addsymbolvalue("INV_SM_MEDKIT",  1+InvDecl_SmMedkit);
    scriptfile_addsymbolvalue("INV_FORTUNE",    1+InvDecl_Booster);
    scriptfile_addsymbolvalue("INV_MEDKIT",     1+InvDecl_Medkit);
    scriptfile_addsymbolvalue("INV_GAS_BOMB",   1+InvDecl_ChemBomb);
    scriptfile_addsymbolvalue("INV_FLASH_BOMB", 1+InvDecl_FlashBomb);
    scriptfile_addsymbolvalue("INV_CALTROPS",   1+InvDecl_Caltrops);
    scriptfile_addsymbolvalue("INV_NIGHT_VIS",  1+InvDecl_NightVision);
    scriptfile_addsymbolvalue("INV_REPAIR_KIT", 1+InvDecl_RepairKit);
    scriptfile_addsymbolvalue("INV_SMOKE_BOMB", 1+InvDecl_Cloak);

    {
        unsigned i;
        for (i=0; i<SIZ(weaponmap); i++)
            scriptfile_addsymbolvalue(weaponmap[i].sym, 1+i);
    }

    while ((token = scriptfile_gettoken(script)))
    {
        switch (cm_transtok(token, cm_tokens, cm_numtokens))
        {
        case CM_MAP:
        {
            char *mapnumptr;
            if (scriptfile_getnumber(script, &curmap)) break;
            mapnumptr = script->ltextptr;
            if (scriptfile_getbraces(script, &braceend)) break;

            // first map file in LevelInfo[] is bogus, last map file is NULL
            if (curmap < 1 || curmap > MAX_LEVELS_REG)
            {
                buildprintf("Error: map number %d not in range 1-%d on line %s:%d\n",
                            curmap, MAX_LEVELS_REG, script->filename,
                            scriptfile_getlinum(script,mapnumptr));
                script->textptr = braceend;
                break;
            }

            while (script->textptr < braceend)
            {
                if (!(token = scriptfile_gettoken(script))) break;
                if (token == braceend) break;
                switch (cm_transtok(token, cm_map_tokens, cm_map_numtokens))
                {
                case CM_FILENAME:
                {
                    char *t;
                    if (scriptfile_getstring(script, &t)) break;

                    //Bfree(custommaps[curmap].LevelName);
                    custommaps[curmap].LevelName = strdup(t);
                    LevelInfo[curmap].LevelName = custommaps[curmap].LevelName;
                    break;
                }
                case CM_SONG:
                {
                    char *t;
                    if (scriptfile_getstring(script, &t)) break;

                    //Bfree(custommaps[curmap].SongName);
                    custommaps[curmap].SongName = strdup(t);
                    LevelInfo[curmap].SongName = custommaps[curmap].SongName;
                    break;
                }
                case CM_TITLE:
                {
                    char *t;
                    if (scriptfile_getstring(script, &t)) break;

                    //Bfree(custommaps[curmap].Description);
                    custommaps[curmap].Description = strdup(t);
                    LevelInfo[curmap].Description = custommaps[curmap].Description;
                    break;
                }
                case CM_BESTTIME:
                {
                    int n;
                    char s[10];
                    if (scriptfile_getnumber(script, &n)) break;

                    Bsnprintf(s, 10, "%d : %02d", n/60, n%60);
                    //Bfree(custommaps[curmap].BestTime);
                    custommaps[curmap].BestTime = strdup(s);
                    LevelInfo[curmap].BestTime = custommaps[curmap].BestTime;
                    break;
                }
                case CM_PARTIME:
                {
                    int n;
                    char s[10];
                    if (scriptfile_getnumber(script, &n)) break;

                    Bsnprintf(s, 10, "%d : %02d", n/60, n%60);
                    //Bfree(custommaps[curmap].ParTime);
                    custommaps[curmap].ParTime = strdup(s);
                    LevelInfo[curmap].ParTime = custommaps[curmap].ParTime;
                    break;
                }
                case CM_CDATRACK:
                {
                    int n;
                    if (scriptfile_getnumber(script, &n)) break;
                    break;
                }
                default:
                    buildprintf("Error on line %s:%d\n",
                                script->filename,
                                scriptfile_getlinum(script,script->ltextptr));
                    break;
                }
            }
            break;
        }

        case CM_EPISODE:
        {
            char *epnumptr;
            if (scriptfile_getnumber(script, &curmap)) break;
            epnumptr = script->ltextptr;
            if (scriptfile_getbraces(script, &braceend)) break;

            // first map file in LevelInfo[] is bogus, last map file is NULL
            if ((unsigned)--curmap >= 2u)
            {
                buildprintf("Error: episode number %d not in range 1-2 on line %s:%d\n",
                            curmap, script->filename,
                            scriptfile_getlinum(script,epnumptr));
                script->textptr = braceend;
                break;
            }

            while (script->textptr < braceend)
            {
                if (!(token = scriptfile_gettoken(script))) break;
                if (token == braceend) break;
                switch (cm_transtok(token, cm_episode_tokens, cm_episode_numtokens))
                {
                case CM_TITLE:
                {
                    char *t;
                    if (scriptfile_getstring(script, &t)) break;

                    strncpy(&EpisodeNames[curmap][1], t, MAX_EPISODE_NAME_LEN);
                    EpisodeNames[curmap][MAX_EPISODE_NAME_LEN+1] = 0;
                    break;
                }
                case CM_SUBTITLE:
                {
                    char *t;
                    if (scriptfile_getstring(script, &t)) break;

                    strncpy(EpisodeSubtitles[curmap], t, MAX_EPISODE_SUBTITLE_LEN);
                    EpisodeSubtitles[curmap][MAX_EPISODE_SUBTITLE_LEN] = 0;
                    break;
                }
                default:
                    buildprintf("Error on line %s:%d\n",
                                script->filename,
                                scriptfile_getlinum(script,script->ltextptr));
                    break;
                }
            }
            break;
        }

        case CM_SKILL:
        {
            char *epnumptr;
            if (scriptfile_getnumber(script, &curmap)) break;
            epnumptr = script->ltextptr;
            if (scriptfile_getbraces(script, &braceend)) break;

            // first map file in LevelInfo[] is bogus, last map file is NULL
            if ((unsigned)--curmap >= 4u)
            {
                buildprintf("Error: skill number %d not in range 1-4 on line %s:%d\n",
                            curmap, script->filename,
                            scriptfile_getlinum(script,epnumptr));
                script->textptr = braceend;
                break;
            }

            while (script->textptr < braceend)
            {
                if (!(token = scriptfile_gettoken(script))) break;
                if (token == braceend) break;
                switch (cm_transtok(token, cm_skill_tokens, cm_skill_numtokens))
                {
                case CM_TITLE:
                {
                    char *t;
                    if (scriptfile_getstring(script, &t)) break;

                    strncpy(&SkillNames[curmap][1], t, MAX_SKILL_NAME_LEN);
                    SkillNames[curmap][MAX_SKILL_NAME_LEN+1] = 0;
                    break;
                }
                default:
                    buildprintf("Error on line %s:%d\n",
                                script->filename,
                                scriptfile_getlinum(script,script->ltextptr));
                    break;
                }
            }
            break;
        }

        case CM_COOKIE:
        {
            char *t;
            int fc = 0;

            if (scriptfile_getbraces(script, &braceend)) break;

            while (script->textptr < braceend)
            {
                if (scriptfile_getstring(script, &t)) break;

                if (fc == MAX_FORTUNES) continue;

                customfortune[fc] = strdup(t);
                if (customfortune[fc]) ReadFortune[fc] = customfortune[fc];
                fc++;
            }
            break;
        }
        case CM_GOTKEY:
        {
            char *t;
            int fc = 0;

            if (scriptfile_getbraces(script, &braceend)) break;

            while (script->textptr < braceend)
            {
                if (scriptfile_getstring(script, &t)) break;

                if (fc == MAX_KEYS) continue;

                customkeymsg[fc] = strdup(t);
                if (customkeymsg[fc]) KeyMsg[fc] = customkeymsg[fc];
                fc++;
            }
            break;
        }
        case CM_NEEDKEY:
        {
            char *t;
            int fc = 0;

            if (scriptfile_getbraces(script, &braceend)) break;

            while (script->textptr < braceend)
            {
                if (scriptfile_getstring(script, &t)) break;

                if (fc == MAX_KEYS) continue;

                customkeydoormsg[fc] = strdup(t);
                if (customkeydoormsg[fc]) KeyDoorMessage[fc] = customkeydoormsg[fc];
                fc++;
            }
            break;
        }
        case CM_INVENTORY:
        {
            char *invtokptr = script->ltextptr, *invnumptr;
            int in;
            char *name = NULL;
            int amt = -1;

            if (scriptfile_getsymbol(script, &in)) break;
            invnumptr = script->ltextptr;
            if (scriptfile_getbraces(script, &braceend)) break;

            if ((unsigned)--in >= (unsigned)InvDecl_TOTAL)
            {
                buildprintf("Error: inventory item number not in range 1-%d on line %s:%d\n",
                            InvDecl_TOTAL, script->filename,
                            scriptfile_getlinum(script,invnumptr));
                script->textptr = braceend;
                break;
            }

            while (script->textptr < braceend)
            {
                if (!(token = scriptfile_gettoken(script))) break;
                if (token == braceend) break;
                switch (cm_transtok(token, cm_inventory_tokens, cm_inventory_numtokens))
                {
                case CM_TITLE:
                    if (scriptfile_getstring(script, &name)) break;
                    break;
                case CM_AMOUNT:
                    if (scriptfile_getnumber(script, &amt)) break;
                    break;
                default:
                    buildprintf("Error on line %s:%d\n",
                                script->filename,
                                scriptfile_getlinum(script,script->ltextptr));
                    break;
                }
            }

            if (name)
            {
                Bfree(custominventoryname[in]);
                custominventoryname[in] = strdup(name);
                InventoryDecls[in].name = custominventoryname[in];
            }
            if (amt >= 0)
            {
                InventoryDecls[in].amount = amt;
            }
            break;
        }
        case CM_WEAPON:
        {
            char *wpntokptr = script->ltextptr, *wpnnumptr;
            char *name = NULL, *ammo = NULL;
            int maxammo = -1, damagemin = -1, damagemax = -1, pickup = -1, wpickup = -1;
            int in,id;

            if (scriptfile_getsymbol(script, &in)) break;
            wpnnumptr = script->ltextptr;
            if (scriptfile_getbraces(script, &braceend)) break;

            if ((unsigned)--in >= (unsigned)SIZ(weaponmap))
            {
                buildprintf("Error: weapon number not in range 1-%d on line %s:%d\n",
                            (int)SIZ(weaponmap), script->filename,
                            scriptfile_getlinum(script,wpnnumptr));
                script->textptr = braceend;
                break;
            }

            while (script->textptr < braceend)
            {
                if (!(token = scriptfile_gettoken(script))) break;
                if (token == braceend) break;
                switch (cm_transtok(token, cm_weapons_tokens, cm_weapons_numtokens))
                {
                case CM_TITLE:
                    if (scriptfile_getstring(script, &name)) break;
                    break;
                case CM_AMMONAME:
                    if (scriptfile_getstring(script, &ammo)) break;
                    break;
                case CM_MAXAMMO:
                    if (scriptfile_getnumber(script, &maxammo)) break;
                    break;
                case CM_DAMAGEMIN:
                    if (scriptfile_getnumber(script, &damagemin)) break;
                    break;
                case CM_DAMAGEMAX:
                    if (scriptfile_getnumber(script, &damagemax)) break;
                    break;
                case CM_AMOUNT:
                    if (scriptfile_getnumber(script, &pickup)) break;
                    break;
                case CM_WEAPON:
                    if (scriptfile_getnumber(script, &wpickup)) break;
                    break;
                default:
                    buildprintf("Error on line %s:%d\n",
                                script->filename,
                                scriptfile_getlinum(script,script->ltextptr));
                    break;
                }
            }
            id = weaponmap[in].dmgid;
            if (weaponmap[in].editable & WM_DAMAGE)
            {
                if (damagemin >= 0) DamageData[id].damage_lo = damagemin;
                if (damagemax >= 0) DamageData[id].damage_hi = damagemax;
            }
            if (weaponmap[in].editable & WM_WEAP)
            {
                if (maxammo >= 0) DamageData[id].max_ammo = maxammo;
                if (name)
                {
                    Bfree(customweaponname[0][id]);
                    customweaponname[0][id] = strdup(name);
                    DamageData[id].weapon_name = customweaponname[0][id];
                }
                if (wpickup >= 0) DamageData[id].weapon_pickup = wpickup;
            }
            if (weaponmap[in].editable & WM_AMMO)
            {
                if (ammo)
                {
                    Bfree(customweaponname[1][id]);
                    customweaponname[1][id] = strdup(ammo);
                    DamageData[id].ammo_name = customweaponname[1][id];
                }
                if (pickup >= 0) DamageData[id].ammo_pickup = pickup;
            }
            break;
        }
        case CM_SECRET:
        case CM_QUIT:
        default:
            buildprintf("Error on line %s:%d\n",
                        script->filename,
                        scriptfile_getlinum(script,script->ltextptr));
            break;
        }
    }

    scriptfile_close(script);
    scriptfile_clearsymbols();
}

