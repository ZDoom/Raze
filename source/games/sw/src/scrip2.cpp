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


#include "names2.h"
#include "panel.h"
#include "game.h"

#include "sprite.h"
#include "jsector.h"
#include "parent.h"
#include "sc_man.h"
#include "razemenu.h"
#include "quotemgr.h"
#include "mapinfo.h"
#include "hw_voxels.h"

BEGIN_SW_NS

TILE_INFO_TYPE aVoxelArray[MAXTILES];


/*
=============================================================================

                        PARSING STUFF

=============================================================================
*/
#define MAXTOKEN    255

static char* script_p, * scriptend_p;
static char    token[MAXTOKEN];
static int     scriptline;
static bool    endofscript;
static bool    tokenready;                     // only true if UnGetToken was just called

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
        endofscript = false;
        tokenready = false;
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

void GetToken(bool crossline)
{
    char    *token_p;

    if (tokenready)                         // is a token already waiting?
    {
        tokenready = false;
        return;
    }

    if (script_p >= scriptend_p)
    {
        if (!crossline)
            Printf("Error: Line %i is incomplete\n",scriptline);
        endofscript = true;
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
            endofscript = true;
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
endofscript = true;
return;
    }

    if (*script_p == '#')   // # is comment field
    {
        if (!crossline)
            Printf("Error: Line %i is incomplete\n", scriptline);
        while (*script_p++ != '\n')
            if (script_p >= scriptend_p)
            {
                endofscript = true;
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

    // zero out the array memory with -1's for pics not being voxelized
    memset(&aVoxelArray[0], -1, sizeof(struct TILE_INFO_TYPE) * MAXTILES);

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
        GetToken(true);     // Crossing a line boundary on the end of line to first token
        // of a new line is permitted (and expected)
        if (endofscript)
            break;

        lTile = atol(token);

        GetToken(false);
        lNumber = atol(token);

        GetToken(false);

        // Load the voxel file into memory
        if (!voxDefine(lNumber,token))
        {
            // Store the sprite and voxel numbers for later use
            aVoxelArray[lTile].Voxel = lNumber; // Voxel num
        }

        if (lNumber >= nextvoxid)   // JBF: so voxels in the def file append to the list
            nextvoxid = lNumber + 1;
    }
    while (script_p < scriptend_p);

    script_p = nullptr;
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
        if (!stricmp(tok, set[i].str))
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
    FScanner sc;

    int lump = fileSystem.FindFile(filename);
    if (lump < 0) return;

    sc.OpenLumpNum(lump);
    sc.SetNoOctals(true);
    sc.SetCMode(true);
    sc.SetNoFatalErrors(true);

    // predefine constants for some stuff to give convenience and eliminate the need for a 'define' directive
    sc.AddSymbol("INV_ARMOR",      1+InvDecl_Armor);
    sc.AddSymbol("INV_KEVLAR",     1+InvDecl_Kevlar);
    sc.AddSymbol("INV_SM_MEDKIT",  1+InvDecl_SmMedkit);
    sc.AddSymbol("INV_FORTUNE",    1+InvDecl_Booster);
    sc.AddSymbol("INV_MEDKIT",     1+InvDecl_Medkit);
    sc.AddSymbol("INV_GAS_BOMB",   1+InvDecl_ChemBomb);
    sc.AddSymbol("INV_FLASH_BOMB", 1+InvDecl_FlashBomb);
    sc.AddSymbol("INV_CALTROPS",   1+InvDecl_Caltrops);
    sc.AddSymbol("INV_NIGHT_VIS",  1+InvDecl_NightVision);
    sc.AddSymbol("INV_REPAIR_KIT", 1+InvDecl_RepairKit);
    sc.AddSymbol("INV_SMOKE_BOMB", 1+InvDecl_Cloak);

    {
        unsigned i;
        for (i=0; i<SIZ(weaponmap); i++)
            sc.AddSymbol(weaponmap[i].sym, 1+i);
    }

    MapRecord* curMap = nullptr;
    while (sc.GetString())
    {
        switch (cm_transtok(sc.String, cm_tokens, cm_numtokens))
        {
        case CM_MAP:
        {
            sc.MustGetNumber(true);
            int mapno = sc.ParseError? -1 : sc.Number;
            curMap = FindMapByLevelNum(mapno);
            if (!curMap)
            {
                curMap = AllocateMap();
                curMap->levelNumber = mapno;
                curMap->cluster = mapno < 5 ? 1 : 2;
            }
            if (sc.CheckString("{"))

            while (!sc.CheckString("}"))
            {
                sc.MustGetString();
                switch (cm_transtok(sc.String, cm_map_tokens, cm_map_numtokens))
                {
                case CM_FILENAME:
                {
                    sc.MustGetString();
					curMap->SetFileName(sc.String);
                    break;
                }
                case CM_SONG:
                {
                    sc.MustGetString();
                    curMap->music = sc.String;
                    break;
                }
                case CM_TITLE:
                {
                    sc.MustGetString();
                    curMap->SetName(sc.String);
                    break;
                }
                case CM_BESTTIME:
                {
                    sc.MustGetNumber();
                    curMap->designerTime = sc.Number;
                    break;
                }
                case CM_PARTIME:
                {
                    sc.MustGetNumber();
                    curMap->parTime = sc.Number;
                    break;
                }
                case CM_CDATRACK:
                {
                    sc.MustGetNumber();
                    curMap->cdSongId = sc.Number;
                    break;
                }
                default:
                    sc.ScriptError("Unknown keyword %s", sc.String);
                    break;
                }
            }
            break;
        }

        case CM_EPISODE:
        {
            int curep;

            sc.MustGetNumber();
            curep = sc.Number;

            if (sc.ParseError) curep = -1;
            else if ((unsigned)curep > 2u)
            {
                sc.ScriptMessage("Episode number %d not in range 1-2\n", curep + 1);
                curep = -1;
                break;
            }

            if (sc.CheckString("{"))
            while (!sc.CheckString("}"))
            {
                sc.MustGetString();
                switch (cm_transtok(sc.String, cm_episode_tokens, cm_episode_numtokens))
                {
                case CM_TITLE:
                {
                    sc.MustGetString();
                    auto vol = MustFindVolume(curep);
                    auto clust = MustFindCluster(curep);
                    vol->name = clust->name = sc.String;
                    break;
                }
                case CM_SUBTITLE:
                {
                    sc.MustGetString();
                    if (curep != -1)
                    {
                        auto vol = MustFindVolume(curep);
                        vol->subtitle = sc.String;
                    }
                    break;
                }
                default:
                    sc.ScriptError("Unknown keyword %s", sc.String);
                    break;
                }
            }
            break;
        }

        case CM_SKILL:
        {
            int curskill;
            sc.MustGetNumber();
            curskill = sc.Number;
            if (sc.ParseError) curskill = -1;
            if ((unsigned)--curskill >= 4u)
            {
                sc.ScriptMessage("Skill number %d not in range 1-4", curskill + 1);
                curskill = -1;
                break;
            }

            if (sc.CheckString("{"))
            while (!sc.CheckString("}"))
            {
                sc.MustGetString();
                switch (cm_transtok(sc.String, cm_skill_tokens, cm_skill_numtokens))
                {
                case CM_TITLE:
                {
                    sc.MustGetString();
                    if (curskill != -1) gSkillNames[curskill] = sc.String;
                    break;
                }
                default:
                    sc.ScriptError("Unknown keyword %s", sc.String);
                    break;
                }
            }
            break;
        }

        case CM_COOKIE:
        {
            int fc = 0;
            if (sc.CheckString("{"))
            while (!sc.CheckString("}"))
            {
                sc.MustGetString();
                if (fc < MAX_FORTUNES)
                {
                    quoteMgr.InitializeQuote(QUOTE_COOKIE + fc, sc.String);
                }
                fc++;
            }
            break;
        }
        case CM_GOTKEY:
        {
            int fc = 0;
            if (sc.CheckString("{"))
            while (!sc.CheckString("}"))
            {
                sc.MustGetString();
                if (fc < MAX_KEYS)
                {
                    quoteMgr.InitializeQuote(QUOTE_KEYMSG + fc, sc.String);
                }
                fc++;
            }
            break;
        }
        case CM_NEEDKEY:
        {
            int fc = 0;
            if (sc.CheckString("{"))
            while (!sc.CheckString("}"))
            {
                sc.MustGetString();
                if (fc < MAX_KEYS)
                {
                    quoteMgr.InitializeQuote(QUOTE_DOORMSG + fc, sc.String);
                }
                fc++;
            }
            break;
        }
        case CM_INVENTORY:
        {
            int in;
            FString name;
            int amt = -1;

            sc.MustGetNumber();
            in = sc.Number;

            if (sc.ParseError) in = -1;
            if ((unsigned)--in >= (unsigned)InvDecl_TOTAL)
            {
                sc.ScriptMessage("Inventory item number %d not in range 1-%d\n", in, InvDecl_TOTAL);
                in = -1;
                break;
            }

            if (sc.CheckString("{"))
            while (!sc.CheckString("}"))
            {
                sc.MustGetString();
                switch (cm_transtok(sc.String, cm_inventory_tokens, cm_inventory_numtokens))
                {
                case CM_TITLE:
                    sc.MustGetToken(TK_StringConst);
                    name = sc.String;
                    break;
                case CM_AMOUNT:
                    sc.MustGetNumber();
                    amt = sc.Number;
                    break;
                default:
                    sc.ScriptError("Unknown keyword %s", sc.String);
                    break;
                }
            }

            if (in == -1) break;
            if (name.IsNotEmpty())
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
            FString name, ammo;
            int maxammo = -1, damagemin = -1, damagemax = -1, pickup = -1, wpickup = -1;
            int in,id;

            sc.MustGetNumber();
            in = sc.Number;

            if (sc.ParseError) in = -1;
            if ((unsigned)--in >= (unsigned)SIZ(weaponmap))
            {
                sc.ScriptMessage("Error: weapon number %d not in range 1-%d", in+1, (int)SIZ(weaponmap));
                in = -1;
                break;
            }

            if (sc.CheckString("{"))
            while (!sc.CheckString("}"))
            {
                sc.MustGetString();
                switch (cm_transtok(sc.String, cm_weapons_tokens, cm_weapons_numtokens))
                {
                case CM_TITLE:
                    sc.MustGetToken(TK_StringConst);
                    name = sc.String;
                    break;
                case CM_AMMONAME:
                    sc.MustGetToken(TK_StringConst);
                    ammo = sc.String;
                    break;
                case CM_MAXAMMO:
                    sc.MustGetNumber();
                    maxammo = sc.Number;
                    break;
               case CM_DAMAGEMIN:
                    sc.MustGetNumber();
                    damagemin = sc.Number;
                    break;
                case CM_DAMAGEMAX:
                    sc.MustGetNumber();
                    damagemax = sc.Number;
                    break;
                case CM_AMOUNT:
                    sc.MustGetNumber();
                    pickup = sc.Number;
                    break;
                case CM_WEAPON:
                    sc.MustGetNumber();
                    wpickup = sc.Number;
                    break;
                default:
                    sc.ScriptError("Unknown keyword %s", sc.String);
                    break;
                }
            }
            if (in == -1) break;
            id = weaponmap[in].dmgid;
            if (weaponmap[in].editable & WM_DAMAGE)
            {
                if (damagemin >= 0) DamageData[id].damage_lo = damagemin;
                if (damagemax >= 0) DamageData[id].damage_hi = damagemax;
            }
            if (weaponmap[in].editable & WM_WEAP)
            {
                if (maxammo >= 0) DamageData[id].max_ammo = maxammo;
                if (name.IsNotEmpty())
                {
                    quoteMgr.InitializeQuote(QUOTE_WPNFIST + in, name);
                }
                if (wpickup >= 0) DamageData[id].weapon_pickup = wpickup;
            }
            if (weaponmap[in].editable & WM_AMMO)
            {
                if (ammo.IsNotEmpty())
                {
                    quoteMgr.InitializeQuote(QUOTE_AMMOFIST + in, ammo);
                }
                if (pickup >= 0) DamageData[id].ammo_pickup = pickup;
            }
            break;
        }
		case CM_THEME:
		{
			FString name;
			int trak = -1;
            int curtheme;

            sc.MustGetNumber();
            curtheme = sc.Number;

            if (sc.ParseError) curtheme = -1;
            if ((unsigned)--curtheme >= 6u)
			{
				sc.ScriptMessage("Theme number %d not in range 1-6", curtheme+1);
                curtheme = -1;
		    }
            if (sc.CheckString("{"))
            while (!sc.CheckString("}"))
            {
                sc.MustGetString();
                switch (cm_transtok(sc.String, cm_theme_tokens, cm_theme_numtokens))
				{
					case CM_SONG:
                        sc.MustGetToken(TK_StringConst);
                        name = sc.String;
                        break;
					case CM_CDATRACK:
                        sc.MustGetNumber();
                        trak = sc.Number;
						break;
					default:
                        sc.ScriptError("Unknown keyword %s", sc.String);
                        break;
				}
			}
            if (curtheme == -1) break;
			if (name.IsNotEmpty())
            {
               ThemeSongs[curtheme] = name;
			}
			if (trak >= 2)
			{
			   ThemeTrack[curtheme] = trak;
			}
			break;
		}
        case CM_SECRET:
        case CM_QUIT:
        default:
            sc.ScriptError("Unknown keyword %s", sc.String);
            break;
        }
    }
    auto vol0 = MustFindVolume(1);
    auto vol1 = MustFindVolume(2);
    auto map1 = FindMapByLevelNum(1);
    auto map5 = FindMapByLevelNum(5);
    if (vol0 && map1) vol0->startmap = map1->labelName;
    if (vol1 && map5) vol1->startmap = map5->labelName;
}

END_SW_NS
