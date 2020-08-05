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

#include "ns.h"
// scriplib.c
#include "build.h"


#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "game.h"

#include "sprite.h"
#include "jsector.h"
#include "parent.h"
#include "scriptfile.h"
#include "menu.h"
#include "quotemgr.h"
#include "mapinfo.h"

BEGIN_SW_NS

ParentalStruct aVoxelArray[MAXTILES];


/*
=============================================================================

                        PARSING STUFF

=============================================================================
*/
#define MAXTOKEN    255

static char* script_p, * scriptend_p;
static char    token[MAXTOKEN];
static int     grabbed;
static int     scriptline;
static SWBOOL    endofscript;
static SWBOOL    tokenready;                     // only TRUE if UnGetToken was just called

/*
==============
=
= LoadScriptFile
=
==============
*/

TArray<uint8_t> LoadScriptFile(const char *filename)
{
    FileReader fp;

	if (!(fp = fileSystem.OpenFileReader(filename)).isOpen())
	{
		// If there's no script file, forget it.
		return TArray<uint8_t>();
	}

    auto scriptbuffer = fp.Read();

    if (scriptbuffer.Size() != 0)
    {
        scriptbuffer.Push(0);
        scriptline = 1;
        endofscript = FALSE;
        tokenready = FALSE;
    }
    return scriptbuffer;
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
            Printf("Error: Line %i is incomplete\n",scriptline);
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
                Printf("Error: Line %i is incomplete\n",scriptline);
            endofscript = TRUE;
            return;
        }
        if (*script_p++ == '\n')
        {
            if (!crossline)
                Printf("Error: Line %i is incomplete\n",scriptline);
            scriptline++;
        }
    }

    if (script_p >= scriptend_p)
    {
        if (!crossline)
Printf("Error: Line %i is incomplete\n", scriptline);
endofscript = TRUE;
return;
    }

    if (*script_p == '#')   // # is comment field
    {
        if (!crossline)
            Printf("Error: Line %i is incomplete\n", scriptline);
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
        //          Printf("Error: Token too large on line %i\n",scriptline);
    }

    *token_p = 0;
}




// Load all the voxel files using swvoxfil.txt script file
// Script file format:

//          # - Comment
//          spritenumber (in artfile), voxel number, filename
//          Ex. 1803 0 medkit2.kvx
//              1804 1 shotgun.kvx
//              etc....

void LoadKVXFromScript(const char* filename)
{
    int lNumber = 0, lTile = 0; // lNumber is the voxel no. and lTile is the editart tile being
    // replaced.

    int grabbed = 0;          // Number of lines parsed

    // zero out the array memory with -1's for pics not being voxelized
    memset(&aVoxelArray[0], -1, sizeof(struct TILE_INFO_TYPE) * MAXTILES);
    for (grabbed = 0; grabbed < MAXTILES; grabbed++)
    {
        aVoxelArray[grabbed].Voxel = -1;
        aVoxelArray[grabbed].Parental = -1;
    }

    grabbed = 0;

    // Load the file
    auto buffer = LoadScriptFile(filename);
    if (!buffer.Size())
    {
        return;
    }
    script_p = (char*)buffer.Data();
    scriptend_p = (char*)&buffer.Last();

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

        // Load the voxel file into memory
        if (!qloadkvx(lNumber,token))
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

    int grabbed=0;          // Number of lines parsed

    // Load the file
    auto buffer = LoadScriptFile(filename);
    if (!buffer.Size())
    {
        return;
    }
    script_p = (char*)buffer.Data();
    scriptend_p = (char*)&buffer.Last();

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

    script_p = NULL;
}


/*
 * Here begins JonoF's modding enhancement stuff
 */


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
	CM_THEME,
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
	{ "theme",       CM_THEME     },
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
},
cm_theme_tokens[] = {
	{ "song",        CM_SONG      },
	{ "music",       CM_SONG      },
	{ "cdatrack",    CM_CDATRACK  },
	{ "cdtrack",     CM_CDATRACK  },
};
#define cm_numtokens           (sizeof(cm_tokens)/sizeof(cm_tokens[0]))
#define cm_map_numtokens       (sizeof(cm_map_tokens)/sizeof(cm_map_tokens[0]))
#define cm_episode_numtokens   (sizeof(cm_episode_tokens)/sizeof(cm_episode_tokens[0]))
#define cm_skill_numtokens     (sizeof(cm_skill_tokens)/sizeof(cm_skill_tokens[0]))
#define cm_inventory_numtokens (sizeof(cm_inventory_tokens)/sizeof(cm_inventory_tokens[0]))
#define cm_weapons_numtokens   (sizeof(cm_weapons_tokens)/sizeof(cm_weapons_tokens[0]))
#define cm_theme_numtokens     (sizeof(cm_theme_tokens)/sizeof(cm_theme_tokens[0]))


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

            // first map entry may not be used, max. amount needs investigation
            if (curmap < 1 || curmap > MAX_LEVELS_REG)
            {
                Printf("Error: map number %d not in range 1-%d on line %s:%d\n",
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

					mapList[curmap].SetFileName(t);
                    break;
                }
                case CM_SONG:
                {
                    char *t;
                    if (scriptfile_getstring(script, &t)) break;

					mapList[curmap].music = t;
                    break;
                }
                case CM_TITLE:
                {
                    char *t;
                    if (scriptfile_getstring(script, &t)) break;

					mapList[curmap].SetName(t);
                    break;
                }
                case CM_BESTTIME:
                {
                    int n;
                    if (scriptfile_getnumber(script, &n)) break;

					mapList[curmap].designerTime = n;
                    break;
                }
                case CM_PARTIME:
                {
                    int n;
                    if (scriptfile_getnumber(script, &n)) break;

					mapList[curmap].parTime = n;
                    break;
                }
                case CM_CDATRACK:
                {
                    int n;
                    if (scriptfile_getnumber(script, &n)) break;
                    mapList[curmap].cdSongId = n;
                    break;
                }
                default:
                    Printf("Error on line %s:%d\n",
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

            if ((unsigned)--curmap >= 2u)
            {
                Printf("Error: episode number %d not in range 1-2 on line %s:%d\n",
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
					gVolumeNames[curmap] = t;
                    break;
                }
                case CM_SUBTITLE:
                {
                    char *t;
                    if (scriptfile_getstring(script, &t)) break;
					gVolumeSubtitles[curmap] = t;
                    break;
                }
                default:
                    Printf("Error on line %s:%d\n",
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

            if ((unsigned)--curmap >= 4u)
            {
                Printf("Error: skill number %d not in range 1-4 on line %s:%d\n",
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

					gSkillNames[curmap] = t;
                    break;
                }
                default:
                    Printf("Error on line %s:%d\n",
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

                quoteMgr.InitializeQuote(QUOTE_COOKIE + fc, t);
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

                quoteMgr.InitializeQuote(QUOTE_KEYMSG + fc, t);
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

                quoteMgr.InitializeQuote(QUOTE_DOORMSG + fc, t);
                fc++;
            }
            break;
        }
        case CM_INVENTORY:
        {
            char *invnumptr;
            int in;
            char *name = NULL;
            int amt = -1;

            if (scriptfile_getsymbol(script, &in)) break;
            invnumptr = script->ltextptr;
            if (scriptfile_getbraces(script, &braceend)) break;

            if ((unsigned)--in >= (unsigned)InvDecl_TOTAL)
            {
                Printf("Error: inventory item number not in range 1-%d on line %s:%d\n",
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
                    Printf("Error on line %s:%d\n",
                                script->filename,
                                scriptfile_getlinum(script,script->ltextptr));
                    break;
                }
            }

            if (name)
            {
                quoteMgr.InitializeQuote(QUOTE_INVENTORY + in, name);
            }
            if (amt >= 0)
            {
                InventoryDecls[in].amount = amt;
            }
            break;
        }
        case CM_WEAPON:
        {
            char *wpnnumptr;
            char *name = NULL, *ammo = NULL;
            int maxammo = -1, damagemin = -1, damagemax = -1, pickup = -1, wpickup = -1;
            int in,id;

            if (scriptfile_getsymbol(script, &in)) break;
            wpnnumptr = script->ltextptr;
            if (scriptfile_getbraces(script, &braceend)) break;

            if ((unsigned)--in >= (unsigned)SIZ(weaponmap))
            {
                Printf("Error: weapon number not in range 1-%d on line %s:%d\n",
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
                    Printf("Error on line %s:%d\n",
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
                    quoteMgr.InitializeQuote(QUOTE_WPNFIST + in, name);
                }
                if (wpickup >= 0) DamageData[id].weapon_pickup = wpickup;
            }
            if (weaponmap[in].editable & WM_AMMO)
            {
                if (ammo)
                {
                    quoteMgr.InitializeQuote(QUOTE_AMMOFIST + in, ammo);
                }
                if (pickup >= 0) DamageData[id].ammo_pickup = pickup;
            }
            break;
        }
		case CM_THEME:
		{
			char *epnumptr;
			char *name = NULL;
			int trak = -1;

			if (scriptfile_getnumber(script, &curmap)) break; epnumptr = script->ltextptr;
			if (scriptfile_getbraces(script, &braceend)) break;
			if ((unsigned)--curmap >= 6u)
			{
				Printf("Error: theme number %d not in range 1-6 on line %s:%d\n",
						curmap, script->filename,
						scriptfile_getlinum(script,epnumptr));
				script->textptr = braceend;
			break;
		    }
			while (script->textptr < braceend)
			{
				if (!(token = scriptfile_gettoken(script))) break;
				if (token == braceend) break;
				switch (cm_transtok(token, cm_theme_tokens, cm_theme_numtokens))
				{
					case CM_SONG:
						if (scriptfile_getstring(script, &name)) break;
						break;
					case CM_CDATRACK:
						if (scriptfile_getnumber(script, &trak)) break;
						break;
					default:
						Printf("Error on line %s:%d\n",
								script->filename,
							scriptfile_getlinum(script,script->ltextptr));
						break;
				}
			}
			if (name)
            {
               ThemeSongs[curmap] = name;
			}
			if (trak >= 2)
			{
			   ThemeTrack[curmap] = trak;
			}
			break;
		}
        case CM_SECRET:
        case CM_QUIT:
        default:
            Printf("Error on line %s:%d\n",
                        script->filename,
                        scriptfile_getlinum(script,script->ltextptr));
            break;
        }
    }

    scriptfile_close(script);
    scriptfile_clearsymbols();
}

END_SW_NS
