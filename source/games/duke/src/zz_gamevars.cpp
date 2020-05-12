//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
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

#include "ns.h"	// Must come before everything else!

#include "duke3d.h"
#include "menus.h"
#include "savegame.h"
#include "namesdyn.h"
#include "gamevars.h"

//#include "vfs.h"

BEGIN_DUKE_NS

#define gamevars_c_

gamevar_t   aGameVars[MAXGAMEVARS];
int32_t     g_gameVarCount   = 0;

// pointers to weapon gamevar data
intptr_t *aplWeaponClip[MAX_WEAPONS];           // number of items in magazine
intptr_t *aplWeaponFireDelay[MAX_WEAPONS];      // delay to fire
intptr_t *aplWeaponFireSound[MAX_WEAPONS];      // Sound made when firing (each time for automatic)
intptr_t *aplWeaponFlags[MAX_WEAPONS];          // Flags for weapon
intptr_t *aplWeaponFlashColor[MAX_WEAPONS];     // Muzzle flash color
intptr_t *aplWeaponHoldDelay[MAX_WEAPONS];      // delay after release fire button to fire (0 for none)
intptr_t *aplWeaponInitialSound[MAX_WEAPONS];   // Sound made when weapon starts firing. zero for no sound
intptr_t *aplWeaponReload[MAX_WEAPONS];         // delay to reload (include fire)
intptr_t *aplWeaponShoots[MAX_WEAPONS];         // what the weapon shoots
intptr_t *aplWeaponShotsPerBurst[MAX_WEAPONS];  // number of shots per 'burst' (one ammo per 'burst')
intptr_t *aplWeaponSound2Sound[MAX_WEAPONS];    // Alternate sound sound ID
intptr_t *aplWeaponSound2Time[MAX_WEAPONS];     // Alternate sound time
intptr_t *aplWeaponSpawn[MAX_WEAPONS];          // the item to spawn
intptr_t *aplWeaponSpawnTime[MAX_WEAPONS];      // the frame at which to spawn an item
intptr_t *aplWeaponTotalTime[MAX_WEAPONS];      // The total time the weapon is cycling before next fire.
intptr_t *aplWeaponWorksLike[MAX_WEAPONS];      // What original the weapon works like


// Frees the memory for the *values* of game variables and arrays. Resets their
// counts to zero. Call this function as many times as needed.
//
// Returns: old g_gameVarCount | (g_gameArrayCount<<16).
int Gv_Free(void)
{
    for (auto &gameVar : aGameVars)
    {
        if (gameVar.flags & GAMEVAR_USER_MASK)
            ALIGNED_FREE_AND_NULL(gameVar.pValues);
        gameVar.flags |= GAMEVAR_RESET;
    }

    EDUKE32_STATIC_ASSERT(MAXGAMEVARS < 32768);
    int const varCount = g_gameVarCount;
    g_gameVarCount = 0;

    hash_init(&h_gamevars);

    return varCount;
}

// Calls Gv_Free() and in addition frees the labels of all game variables and
// arrays.
// Only call this function at exit
void Gv_Clear(void)
{
    Gv_Free();

    // Now, only do work that Gv_Free() hasn't done.
    for (auto & gameVar : aGameVars)
        DO_FREE_AND_NULL(gameVar.szLabel);
}

// Note that this entire function is totally architecture dependent and needs to be fixed (which won't be easy...)
int Gv_ReadSave(FileReader &kFile)
{
    char tbuf[12];

    if (kFile.Read(tbuf, 12)!=12) goto corrupt;
    if (Bmemcmp(tbuf, "BEG: EDuke32", 12)) { Printf("BEG ERR\n"); return 2; }

    Gv_Free(); // nuke 'em from orbit, it's the only way to be sure...

    if (kFile.Read(&g_gameVarCount,sizeof(g_gameVarCount)) != sizeof(g_gameVarCount)) goto corrupt;
    for (bssize_t i=0; i<g_gameVarCount; i++)
    {
        char *const olabel = aGameVars[i].szLabel;

        if (kFile.Read(&aGameVars[i], sizeof(gamevar_t)) != sizeof(gamevar_t))
            goto corrupt;

        aGameVars[i].szLabel = (char *)Xrealloc(olabel, MAXVARLABEL * sizeof(uint8_t));

        if (kFile.Read(aGameVars[i].szLabel, MAXVARLABEL) != MAXVARLABEL)
            goto corrupt;
        hash_add(&h_gamevars, aGameVars[i].szLabel,i, 1);

        if (aGameVars[i].flags & GAMEVAR_PERPLAYER)
        {
            aGameVars[i].pValues = (intptr_t*)Xaligned_alloc(PLAYER_VAR_ALIGNMENT, MAXPLAYERS * sizeof(intptr_t));
            if (kFile.Read(aGameVars[i].pValues,sizeof(intptr_t) * MAXPLAYERS) != sizeof(intptr_t) * MAXPLAYERS) goto corrupt;
        }
        else if (aGameVars[i].flags & GAMEVAR_PERACTOR)
        {
            aGameVars[i].pValues = (intptr_t*)Xaligned_alloc(ACTOR_VAR_ALIGNMENT, MAXSPRITES * sizeof(intptr_t));
            if (kFile.Read(aGameVars[i].pValues,sizeof(intptr_t) * MAXSPRITES) != sizeof(intptr_t) * MAXSPRITES) goto corrupt;
        }
    }

    Gv_InitWeaponPointers();

    // Gv_RefreshPointers();

    uint8_t savedstate[MAXVOLUMES*MAXLEVELS];
    Bmemset(savedstate, 0, sizeof(savedstate));

    if (kFile.Read(savedstate, sizeof(savedstate)) != sizeof(savedstate)) goto corrupt;

    for (bssize_t i = 0; i < (MAXVOLUMES * MAXLEVELS); i++)
    {
        G_FreeMapState(i);

        if (!savedstate[i])
            continue;

        g_mapInfo[i].savedstate = (mapstate_t *)Xaligned_alloc(ACTOR_VAR_ALIGNMENT, sizeof(mapstate_t));
        if (kFile.Read(g_mapInfo[i].savedstate, sizeof(mapstate_t)) != sizeof(mapstate_t)) return -8;

        mapstate_t &sv = *g_mapInfo[i].savedstate;

        for (bssize_t j = 0; j < g_gameVarCount; j++)
        {
            if (aGameVars[j].flags & GAMEVAR_NORESET) continue;
            if (aGameVars[j].flags & GAMEVAR_PERPLAYER)
            {
                sv.vars[j] = (intptr_t *) Xaligned_alloc(PLAYER_VAR_ALIGNMENT, MAXPLAYERS * sizeof(intptr_t));
                if (kFile.Read(sv.vars[j], sizeof(intptr_t) * MAXPLAYERS) != sizeof(intptr_t) * MAXPLAYERS) return -9;
            }
            else if (aGameVars[j].flags & GAMEVAR_PERACTOR)
            {
                sv.vars[j] = (intptr_t *) Xaligned_alloc(ACTOR_VAR_ALIGNMENT, MAXSPRITES * sizeof(intptr_t));
                if (kFile.Read(sv.vars[j], sizeof(intptr_t) * MAXSPRITES) != sizeof(intptr_t) * MAXSPRITES) return -10;
            }
        }
    }

    if (kFile.Read(tbuf, 12) != 12) return -13;
    if (Bmemcmp(tbuf, "EOF: EDuke32", 12)) { Printf("EOF ERR\n"); return 2; }

    return 0;

corrupt:
    return -7;
}

// Note that this entire function is totally architecture dependent and needs to be fixed (which won't be easy...)
void Gv_WriteSave(FileWriter &fil)
{
    //   AddLog("Saving Game Vars to File");
    fil.Write("BEG: EDuke32", 12);

	fil.Write(&g_gameVarCount,sizeof(g_gameVarCount));

    for (bssize_t i = 0; i < g_gameVarCount; i++)
    {
		fil.Write(&(aGameVars[i]), sizeof(gamevar_t));
		fil.Write(aGameVars[i].szLabel, sizeof(uint8_t) * MAXVARLABEL);

        if (aGameVars[i].flags & GAMEVAR_PERPLAYER)
			fil.Write(aGameVars[i].pValues, sizeof(intptr_t) * MAXPLAYERS);
        else if (aGameVars[i].flags & GAMEVAR_PERACTOR)
			fil.Write(aGameVars[i].pValues, sizeof(intptr_t) * MAXSPRITES);
    }

    uint8_t savedstate[MAXVOLUMES * MAXLEVELS];
    Bmemset(savedstate, 0, sizeof(savedstate));

    for (bssize_t i = 0; i < (MAXVOLUMES * MAXLEVELS); i++)
        if (g_mapInfo[i].savedstate != NULL)
            savedstate[i] = 1;

	fil.Write(savedstate, sizeof(savedstate));

    for (bssize_t i = 0; i < (MAXVOLUMES * MAXLEVELS); i++)
    {
        if (!savedstate[i]) continue;

        mapstate_t &sv = *g_mapInfo[i].savedstate;

		fil.Write(g_mapInfo[i].savedstate, sizeof(mapstate_t));

        for (bssize_t j = 0; j < g_gameVarCount; j++)
        {
            if (aGameVars[j].flags & GAMEVAR_NORESET) continue;
            if (aGameVars[j].flags & GAMEVAR_PERPLAYER)
				fil.Write(sv.vars[j], sizeof(intptr_t) * MAXPLAYERS);
            else if (aGameVars[j].flags & GAMEVAR_PERACTOR)
				fil.Write(sv.vars[j], sizeof(intptr_t) * MAXSPRITES);
        }
    }

    fil.Write("EOF: EDuke32", 12);
}

void Gv_DumpValues(void)
{
    buildprint("// Current Game Definitions\n\n");

    for (bssize_t i=0; i<g_gameVarCount; i++)
    {
        buildprint("gamevar ", aGameVars[i].szLabel, " ");

        if (aGameVars[i].flags & (GAMEVAR_INT32PTR))
            buildprint(*(int32_t *)aGameVars[i].global);
        else if (aGameVars[i].flags & (GAMEVAR_INT16PTR))
            buildprint(*(int16_t *)aGameVars[i].global);
        else
            buildprint(aGameVars[i].global);

        if (aGameVars[i].flags & (GAMEVAR_PERPLAYER))
            buildprint(" GAMEVAR_PERPLAYER");
        else if (aGameVars[i].flags & (GAMEVAR_PERACTOR))
            buildprint(" GAMEVAR_PERACTOR");
        else
            buildprint(" ", aGameVars[i].flags/* & (GAMEVAR_USER_MASK)*/);

        buildprint(" // ");
        if (aGameVars[i].flags & (GAMEVAR_SYSTEM))
            buildprint(" (system)");
        if (aGameVars[i].flags & (GAMEVAR_PTR_MASK))
            buildprint(" (pointer)");
        if (aGameVars[i].flags & (GAMEVAR_READONLY))
            buildprint(" (read only)");
        if (aGameVars[i].flags & (GAMEVAR_SPECIAL))
            buildprint(" (special)");
        buildprint("\n");
    }
    buildprint("\n// end of game definitions\n");
}

// XXX: This function is very strange.
void Gv_ResetVars(void) /* this is called during a new game and nowhere else */
{
    Gv_Free();

    for (auto &aGameVar : aGameVars)
    {
        if (aGameVar.szLabel != NULL)
            Gv_NewVar(aGameVar.szLabel, (aGameVar.flags & GAMEVAR_NODEFAULT) ? aGameVar.global : aGameVar.defaultValue, aGameVar.flags);
    }
}

void Gv_NewVar(const char *pszLabel, intptr_t lValue, uint32_t dwFlags)
{
    if (EDUKE32_PREDICT_FALSE(g_gameVarCount >= MAXGAMEVARS))
    {
        g_errorCnt++;
        C_ReportError(-1);
        Printf("%s:%d: error: too many gamevars!\n",g_scriptFileName,line_count);
        return;
    }

    if (EDUKE32_PREDICT_FALSE(Bstrlen(pszLabel) > (MAXVARLABEL-1)))
    {
        g_errorCnt++;
        C_ReportError(-1);
        Printf("%s:%d: error: variable name `%s' exceeds limit of %d characters.\n",g_scriptFileName,line_count,pszLabel, MAXVARLABEL);
        return;
    }

    int gV = hash_find(&h_gamevars,pszLabel);

    if (gV >= 0 && !(aGameVars[gV].flags & GAMEVAR_RESET))
    {
        // found it...
        if (EDUKE32_PREDICT_FALSE(aGameVars[gV].flags & (GAMEVAR_PTR_MASK)))
        {
            C_ReportError(-1);
            Printf("%s:%d: warning: cannot redefine internal gamevar `%s'.\n",g_scriptFileName,line_count,label+(g_labelCnt<<6));
            return;
        }
        else if (EDUKE32_PREDICT_FALSE(!(aGameVars[gV].flags & GAMEVAR_SYSTEM)))
        {
            // it's a duplicate in error
            g_warningCnt++;
            C_ReportError(WARNING_DUPLICATEDEFINITION);
            return;
        }
    }

    if (gV == -1)
        gV = g_gameVarCount;

    // If it's a user gamevar...
    if ((aGameVars[gV].flags & GAMEVAR_SYSTEM) == 0)
    {
        // Allocate and set its label
        if (aGameVars[gV].szLabel == NULL)
            aGameVars[gV].szLabel = (char *)Xcalloc(MAXVARLABEL,sizeof(uint8_t));

        if (aGameVars[gV].szLabel != pszLabel)
            Bstrcpy(aGameVars[gV].szLabel,pszLabel);

        // and the flags
        aGameVars[gV].flags=dwFlags;

        // only free if per-{actor,player}
        if (aGameVars[gV].flags & GAMEVAR_USER_MASK)
            ALIGNED_FREE_AND_NULL(aGameVars[gV].pValues);
    }

    // if existing is system, they only get to change default value....
    aGameVars[gV].defaultValue = lValue;
    aGameVars[gV].flags &= ~GAMEVAR_RESET;

    if (gV == g_gameVarCount)
    {
        // we're adding a new one.
        hash_add(&h_gamevars, aGameVars[gV].szLabel, g_gameVarCount++, 0);
    }

    // Set initial values. (Or, override values for system gamevars.)
    if (aGameVars[gV].flags & GAMEVAR_PERPLAYER)
    {
        if (!aGameVars[gV].pValues)
        {
            aGameVars[gV].pValues = (intptr_t *) Xaligned_alloc(PLAYER_VAR_ALIGNMENT, MAXPLAYERS * sizeof(intptr_t));
            Bmemset(aGameVars[gV].pValues, 0, MAXPLAYERS * sizeof(intptr_t));
        }
        for (bssize_t j=MAXPLAYERS-1; j>=0; --j)
            aGameVars[gV].pValues[j]=lValue;
    }
    else if (aGameVars[gV].flags & GAMEVAR_PERACTOR)
    {
        if (!aGameVars[gV].pValues)
        {
            aGameVars[gV].pValues = (intptr_t *) Xaligned_alloc(ACTOR_VAR_ALIGNMENT, MAXSPRITES * sizeof(intptr_t));
            Bmemset(aGameVars[gV].pValues, 0, MAXSPRITES * sizeof(intptr_t));
        }
        for (bssize_t j=MAXSPRITES-1; j>=0; --j)
            aGameVars[gV].pValues[j]=lValue;
    }
    else aGameVars[gV].global = lValue;
}

static int Gv_GetVarIndex(const char *szGameLabel)
{
    int const gameVar = hash_find(&h_gamevars,szGameLabel);

    if (EDUKE32_PREDICT_FALSE((unsigned)gameVar >= MAXGAMEVARS))
    {
        Printf(TEXTCOLOR_RED "Gv_GetVarIndex(): INTERNAL ERROR: couldn't find gamevar %s!\n", szGameLabel);
        return 0;
    }

    return gameVar;
}

static FORCE_INLINE int __fastcall getvar__(int const gameVar, int const spriteNum, int const playerNum)
{
    auto const &var = aGameVars[gameVar & (MAXGAMEVARS-1)];

    int       returnValue  = 0;
    int const varFlags     = var.flags & (GAMEVAR_USER_MASK|GAMEVAR_PTR_MASK);

    if (!varFlags) returnValue = var.global;
    else if (varFlags == GAMEVAR_PERACTOR)
        returnValue = var.pValues[spriteNum & (MAXSPRITES-1)];
    else if (varFlags == GAMEVAR_PERPLAYER)
        returnValue = var.pValues[playerNum & (MAXPLAYERS-1)];
    else switch (varFlags & GAMEVAR_PTR_MASK)
    {
        case GAMEVAR_RAWQ16PTR:
        case GAMEVAR_INT32PTR: returnValue = *(int32_t *)var.global; break;
        case GAMEVAR_INT16PTR: returnValue = *(int16_t *)var.global; break;
        case GAMEVAR_Q16PTR:   returnValue = fix16_to_int(*(fix16_t *)var.global); break;
    }

    return NEGATE_ON_CONDITION(returnValue, gameVar & GV_FLAG_NEGATIVE);
}

int __fastcall Gv_GetVar(int const gameVar, int const spriteNum, int const playerNum) { return getvar__(gameVar, spriteNum, playerNum); }
int __fastcall Gv_GetVar(int const gameVar) { return getvar__(gameVar, vm.spriteNum, vm.playerNum); }

void __fastcall Gv_GetManyVars(int const numVars, int32_t * const outBuf)
{
    for (native_t j = 0; j < numVars; ++j)
        outBuf[j] = getvar__(*insptr++, vm.spriteNum, vm.playerNum);
}

static FORCE_INLINE void __fastcall setvar__(int const gameVar, int const newValue, int const spriteNum, int const playerNum)
{
    gamevar_t &var = aGameVars[gameVar];
    int const varFlags = var.flags & (GAMEVAR_USER_MASK|GAMEVAR_PTR_MASK);

    if (!varFlags) var.global=newValue;
    else if (varFlags == GAMEVAR_PERACTOR)
        var.pValues[spriteNum & (MAXSPRITES-1)] = newValue;
    else if (varFlags == GAMEVAR_PERPLAYER)
        var.pValues[playerNum & (MAXPLAYERS-1)] = newValue;
    else switch (varFlags & GAMEVAR_PTR_MASK)
    {
        case GAMEVAR_RAWQ16PTR:
        case GAMEVAR_INT32PTR: *((int32_t *)var.global) = (int32_t)newValue; break;
        case GAMEVAR_INT16PTR: *((int16_t *)var.global) = (int16_t)newValue; break;
        case GAMEVAR_Q16PTR:    *(fix16_t *)var.global  = fix16_from_int((int16_t)newValue); break;
    }
    return;
}

void __fastcall Gv_SetVar(int const gameVar, int const newValue) { setvar__(gameVar, newValue, vm.spriteNum, vm.playerNum); }
void __fastcall Gv_SetVar(int const gameVar, int const newValue, int const spriteNum, int const playerNum)
{
    setvar__(gameVar, newValue, spriteNum, playerNum);
}

int Gv_GetVarByLabel(const char *szGameLabel, int const defaultValue, int const spriteNum, int const playerNum)
{
    int const gameVar = hash_find(&h_gamevars, szGameLabel);
    return EDUKE32_PREDICT_TRUE(gameVar >= 0) ? Gv_GetVar(gameVar, spriteNum, playerNum) : defaultValue;
}

static intptr_t *Gv_GetVarDataPtr(const char *szGameLabel)
{
    int const gameVar = hash_find(&h_gamevars, szGameLabel);

    if (EDUKE32_PREDICT_FALSE((unsigned)gameVar >= MAXGAMEVARS))
        return NULL;

    gamevar_t &var = aGameVars[gameVar];

    if (var.flags & (GAMEVAR_USER_MASK | GAMEVAR_PTR_MASK))
        return var.pValues;

    return &(var.global);
}

void Gv_ResetSystemDefaults(void)
{
    // call many times...
    char aszBuf[64];

    for (int weaponNum = 0; weaponNum < MAX_WEAPONS; ++weaponNum)
    {
        for (int playerNum = 0; playerNum < MAXPLAYERS; ++playerNum)
        {
            Bsprintf(aszBuf, "WEAPON%d_CLIP", weaponNum);
            aplWeaponClip[weaponNum][playerNum] = Gv_GetVarByLabel(aszBuf, 0, -1, playerNum);
            Bsprintf(aszBuf, "WEAPON%d_RELOAD", weaponNum);
            aplWeaponReload[weaponNum][playerNum] = Gv_GetVarByLabel(aszBuf, 0, -1, playerNum);
            Bsprintf(aszBuf, "WEAPON%d_FIREDELAY", weaponNum);
            aplWeaponFireDelay[weaponNum][playerNum] = Gv_GetVarByLabel(aszBuf, 0, -1, playerNum);
            Bsprintf(aszBuf, "WEAPON%d_TOTALTIME", weaponNum);
            aplWeaponTotalTime[weaponNum][playerNum] = Gv_GetVarByLabel(aszBuf, 0, -1, playerNum);
            Bsprintf(aszBuf, "WEAPON%d_HOLDDELAY", weaponNum);
            aplWeaponHoldDelay[weaponNum][playerNum] = Gv_GetVarByLabel(aszBuf, 0, -1, playerNum);
            Bsprintf(aszBuf, "WEAPON%d_FLAGS", weaponNum);
            aplWeaponFlags[weaponNum][playerNum] = Gv_GetVarByLabel(aszBuf, 0, -1, playerNum);
            Bsprintf(aszBuf, "WEAPON%d_SHOOTS", weaponNum);
            aplWeaponShoots[weaponNum][playerNum] = Gv_GetVarByLabel(aszBuf, 0, -1, playerNum);
            if ((unsigned)aplWeaponShoots[weaponNum][playerNum] >= MAXTILES)
                aplWeaponShoots[weaponNum][playerNum] = 0;
            Bsprintf(aszBuf, "WEAPON%d_SPAWNTIME", weaponNum);
            aplWeaponSpawnTime[weaponNum][playerNum] = Gv_GetVarByLabel(aszBuf, 0, -1, playerNum);
            Bsprintf(aszBuf, "WEAPON%d_SPAWN", weaponNum);
            aplWeaponSpawn[weaponNum][playerNum] = Gv_GetVarByLabel(aszBuf, 0, -1, playerNum);
            Bsprintf(aszBuf, "WEAPON%d_SHOTSPERBURST", weaponNum);
            aplWeaponShotsPerBurst[weaponNum][playerNum] = Gv_GetVarByLabel(aszBuf, 0, -1, playerNum);
            Bsprintf(aszBuf, "WEAPON%d_WORKSLIKE", weaponNum);
            aplWeaponWorksLike[weaponNum][playerNum] = Gv_GetVarByLabel(aszBuf, 0, -1, playerNum);
            Bsprintf(aszBuf, "WEAPON%d_INITIALSOUND", weaponNum);
            aplWeaponInitialSound[weaponNum][playerNum] = Gv_GetVarByLabel(aszBuf, 0, -1, playerNum);
            Bsprintf(aszBuf, "WEAPON%d_FIRESOUND", weaponNum);
            aplWeaponFireSound[weaponNum][playerNum] = Gv_GetVarByLabel(aszBuf, 0, -1, playerNum);
            Bsprintf(aszBuf, "WEAPON%d_SOUND2TIME", weaponNum);
            aplWeaponSound2Time[weaponNum][playerNum] = Gv_GetVarByLabel(aszBuf, 0, -1, playerNum);
            Bsprintf(aszBuf, "WEAPON%d_SOUND2SOUND", weaponNum);
            aplWeaponSound2Sound[weaponNum][playerNum] = Gv_GetVarByLabel(aszBuf, 0, -1, playerNum);
            Bsprintf(aszBuf, "WEAPON%d_FLASHCOLOR", weaponNum);
            aplWeaponFlashColor[weaponNum][playerNum] = Gv_GetVarByLabel(aszBuf, 0, -1, playerNum);
        }
    }

    g_aimAngleVarID  = Gv_GetVarIndex("AUTOAIMANGLE");
    g_angRangeVarID  = Gv_GetVarIndex("ANGRANGE");
    g_returnVarID    = Gv_GetVarIndex("RETURN");
    g_weaponVarID    = Gv_GetVarIndex("WEAPON");
    g_worksLikeVarID = Gv_GetVarIndex("WORKSLIKE");
    g_zRangeVarID    = Gv_GetVarIndex("ZRANGE");

}

// Will set members that were overridden at CON translation time to 1.
// For example, if
//   gamevar WEAPON1_SHOOTS 2200 GAMEVAR_PERPLAYER
// was specified at file scope, g_weaponOverridden[1].Shoots will be 1.
weapondata_t g_weaponOverridden[MAX_WEAPONS];

static weapondata_t weapondefaults[MAX_WEAPONS] = {
    /*
        WorksLike, Clip, Reload, FireDelay, TotalTime, HoldDelay,
        Flags,
        Shoots, SpawnTime, Spawn, ShotsPerBurst, InitialSound, FireSound, Sound2Time, Sound2Sound,
        FlashColor
    */
    {
        KNEE_WEAPON__STATIC, 0, 30, 7, 14, 14,
        WEAPON_RANDOMRESTART | WEAPON_AUTOMATIC,
        KNEE__STATIC, 0, 0, 0, 0, 0, 0,
        0
    },

    {
        PISTOL_WEAPON, 20, 50, 2, 5, 0,
        WEAPON_AUTOMATIC | WEAPON_HOLSTER_CLEARS_CLIP,
        SHOTSPARK1__STATIC, 2, SHELL__STATIC, 0, 0, PISTOL_FIRE__STATIC, 0, 0,
        255+(95<<8)
    },

    {
        SHOTGUN_WEAPON__STATIC, 0, 13, 4, 31, 0,
        WEAPON_CHECKATRELOAD,
        SHOTGUN__STATIC, 24, SHOTGUNSHELL__STATIC, 7, 0, SHOTGUN_FIRE__STATIC, 15, SHOTGUN_COCK__STATIC,
        255+(95<<8)
    },

    {
        CHAINGUN_WEAPON__STATIC, 0, 30, 1, 12, 10,
        WEAPON_AUTOMATIC | WEAPON_FIREEVERYTHIRD | WEAPON_AMMOPERSHOT,
        CHAINGUN__STATIC, 0, SHELL__STATIC, 0, 0, CHAINGUN_FIRE__STATIC, 0, 0,
        255+(95<<8)
    },

    {
        RPG_WEAPON__STATIC, 0, 30, 4, 20, 0,
        0,
        RPG__STATIC, 0, 0, 0, 0, 0, 0, 0,
        255+(95<<8)
    },

    {
        HANDBOMB_WEAPON__STATIC, 0, 30, 6, 19, 12,
        WEAPON_THROWIT,
        HEAVYHBOMB__STATIC, 0, 0, 0, 0, 0, 0,
        0
    },

    {
        SHRINKER_WEAPON__STATIC, 0, 0, 10, 30, 0,
        WEAPON_GLOWS,
        SHRINKER__STATIC, 0, 0, 0, SHRINKER_FIRE__STATIC, 0, 0, 0,
        176+(252<<8)+(120<<16)
    },

    {
        DEVISTATOR_WEAPON__STATIC, 0, 30, 2, 5, 5,
        WEAPON_FIREEVERYOTHER,
        RPG__STATIC, 0, 0, 2, CAT_FIRE__STATIC, 0, 0, 0,
        255+(95<<8)
    },

    {
        TRIPBOMB_WEAPON__STATIC, 0, 30, 3, 16, 0,
        WEAPON_STANDSTILL,
        HANDHOLDINGLASER__STATIC, 0, 0, 0, 0, 0, 0,
        0
    },

    {
        FREEZE_WEAPON__STATIC, 0, 0, 3, 5, 0,
        WEAPON_FIREEVERYOTHER,
        FREEZEBLAST__STATIC, 0, 0, 0, CAT_FIRE__STATIC, CAT_FIRE__STATIC, 0, 0,
        72+(88<<8)+(140<<16)
    },

    {
        HANDREMOTE_WEAPON__STATIC, 0, 30, 2, 10, 0,
        WEAPON_BOMB_TRIGGER | WEAPON_NOVISIBLE,
        0, 0, 0, 0, 0, 0, 0,
        0
    },

    {
        GROW_WEAPON__STATIC, 0, 0, 3, 30, 0,
        WEAPON_GLOWS,
        GROWSPARK__STATIC, 0, 0, 0, EXPANDERSHOOT__STATIC, EXPANDERSHOOT__STATIC, 0, 0,
        216+(52<<8)+(20<<16)
    },
};

// KEEPINSYNC with what is contained above
// XXX: ugly
static int32_t G_StaticToDynamicTile(int32_t const tile)
{
    switch (tile)
    {
#ifndef EDUKE32_STANDALONE
    case CHAINGUN__STATIC: return TILE_CHAINGUN;
    case FREEZEBLAST__STATIC: return TILE_FREEZEBLAST;
    case GROWSPARK__STATIC: return TILE_GROWSPARK;
    case HANDHOLDINGLASER__STATIC: return TILE_HANDHOLDINGLASER;
    case HEAVYHBOMB__STATIC: return TILE_HEAVYHBOMB;
    case KNEE__STATIC: return TILE_KNEE;
    case RPG__STATIC: return TILE_RPG;
    case SHELL__STATIC: return TILE_SHELL;
    case SHOTGUNSHELL__STATIC: return TILE_SHOTGUNSHELL;
    case SHOTGUN__STATIC: return TILE_SHOTGUN;
    case SHOTSPARK1__STATIC: return TILE_SHOTSPARK1;
    case SHRINKER__STATIC: return TILE_SHRINKER;
#endif
    default: return tile;
    }
}

static int32_t G_StaticToDynamicSound(int32_t const sound)
{
    switch (sound)
    {
#ifndef EDUKE32_STANDALONE
    case CAT_FIRE__STATIC: return CAT_FIRE;
    case CHAINGUN_FIRE__STATIC: return CHAINGUN_FIRE;
    case EJECT_CLIP__STATIC: return EJECT_CLIP;
    case EXPANDERSHOOT__STATIC: return EXPANDERSHOOT;
    case INSERT_CLIP__STATIC: return INSERT_CLIP;
    case PISTOL_FIRE__STATIC: return PISTOL_FIRE;
    case SELECT_WEAPON__STATIC: return SELECT_WEAPON;
    case SHOTGUN_FIRE__STATIC: return SHOTGUN_FIRE;
    case SHOTGUN_COCK__STATIC: return SHOTGUN_COCK;
    case SHRINKER_FIRE__STATIC: return SHRINKER_FIRE;
#endif
    default: return sound;
    }
}

// Initialize WEAPONx_* gamevars. Since for Lunatic, they reside on the C side,
// they're set directly. In C-CON, a new CON variable is defined together with
// its initial value.
#ifdef LUNATIC
# define ADDWEAPONVAR(Weapidx, Membname) do { \
    int32_t j; \
    for (j=0; j<MAXPLAYERS; j++) \
        g_playerWeapon[j][Weapidx].Membname = weapondefaults[Weapidx].Membname; \
} while (0)
#else
# define ADDWEAPONVAR(Weapidx, Membname) do { \
    FStringf aszBuf("WEAPON%d_" #Membname, Weapidx); \
    aszBuf.ToUpper(); \
    Gv_NewVar(aszBuf, weapondefaults[Weapidx].Membname, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM); \
} while (0)
#endif

// After CON translation, get not-overridden members from weapondefaults[] back
// into the live arrays! (That is, g_playerWeapon[][] for Lunatic, WEAPONx_*
// gamevars on the CON side in C-CON.)
#ifdef LUNATIC
# define POSTADDWEAPONVAR(Weapidx, Membname) ADDWEAPONVAR(Weapidx, Membname)
#else
// NYI
# define POSTADDWEAPONVAR(Weapidx, Membname) do {} while (0)
#endif

// Finish a default weapon member after CON translation. If it was not
// overridden from CON itself (see example at g_weaponOverridden[]), we set
// both the weapondefaults[] entry (probably dead by now) and the live value.
#define FINISH_WEAPON_DEFAULT_X(What, i, Membname) do {  \
    if (!g_weaponOverridden[i].Membname) \
    { \
        weapondefaults[i].Membname = G_StaticToDynamic##What(weapondefaults[i].Membname); \
        POSTADDWEAPONVAR(i, Membname); \
    } \
} while (0)

#define FINISH_WEAPON_DEFAULT_TILE(i, Membname) FINISH_WEAPON_DEFAULT_X(Tile, i, Membname)
#define FINISH_WEAPON_DEFAULT_SOUND(i, Membname) FINISH_WEAPON_DEFAULT_X(Sound, i, Membname)

// Process the dynamic {tile,sound} mappings after CON has been translated.
// We cannot do this before, because the dynamic maps are not yet set up then.
void Gv_FinalizeWeaponDefaults(void)
{
    for (int i=0; i<MAX_WEAPONS; i++)
    {
        FINISH_WEAPON_DEFAULT_TILE(i, Shoots);
        FINISH_WEAPON_DEFAULT_TILE(i, Spawn);

        FINISH_WEAPON_DEFAULT_SOUND(i, InitialSound);
        FINISH_WEAPON_DEFAULT_SOUND(i, FireSound);
        FINISH_WEAPON_DEFAULT_SOUND(i, Sound2Sound);
    }
}
#undef FINISH_WEAPON_DEFAULT_SOUND
#undef FINISH_WEAPON_DEFAULT_TILE
#undef FINISH_WEAPON_DEFAULT_X
#undef POSTADDWEAPONVAR

#if !defined LUNATIC
static int32_t lastvisinc;
#endif

static void Gv_AddSystemVars(void)
{
    // only call ONCE

    for (int i=0; i<MAX_WEAPONS; i++)
    {
        ADDWEAPONVAR(i, Clip);
        ADDWEAPONVAR(i, FireDelay);
        ADDWEAPONVAR(i, FireSound);
        ADDWEAPONVAR(i, Flags);
        ADDWEAPONVAR(i, FlashColor);
        ADDWEAPONVAR(i, HoldDelay);
        ADDWEAPONVAR(i, InitialSound);
        ADDWEAPONVAR(i, Reload);
        ADDWEAPONVAR(i, Shoots);
        ADDWEAPONVAR(i, ShotsPerBurst);
        ADDWEAPONVAR(i, Sound2Sound);
        ADDWEAPONVAR(i, Sound2Time);
        ADDWEAPONVAR(i, Spawn);
        ADDWEAPONVAR(i, SpawnTime);
        ADDWEAPONVAR(i, TotalTime);
        ADDWEAPONVAR(i, WorksLike);
    }

    Gv_NewVar("GRENADE_LIFETIME",      NAM_GRENADE_LIFETIME,                    GAMEVAR_SYSTEM | GAMEVAR_PERPLAYER);
    Gv_NewVar("GRENADE_LIFETIME_VAR",  NAM_GRENADE_LIFETIME_VAR,                GAMEVAR_SYSTEM | GAMEVAR_PERPLAYER);
    Gv_NewVar("STICKYBOMB_LIFETIME",   NAM_GRENADE_LIFETIME,                    GAMEVAR_SYSTEM | GAMEVAR_PERPLAYER);
    Gv_NewVar("STICKYBOMB_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR,              GAMEVAR_SYSTEM | GAMEVAR_PERPLAYER);
    Gv_NewVar("TRIPBOMB_CONTROL",      TRIPBOMB_TRIPWIRE,                       GAMEVAR_SYSTEM | GAMEVAR_PERPLAYER);

    Gv_NewVar("ANGRANGE",              18,                                      GAMEVAR_SYSTEM | GAMEVAR_PERPLAYER);
    Gv_NewVar("AUTOAIMANGLE",          0,                                       GAMEVAR_SYSTEM | GAMEVAR_PERPLAYER);
    Gv_NewVar("COOP",                  (intptr_t)&ud.coop,                      GAMEVAR_SYSTEM | GAMEVAR_INT32PTR);
    Gv_NewVar("FFIRE",                 (intptr_t)&ud.ffire,                     GAMEVAR_SYSTEM | GAMEVAR_INT32PTR);
    Gv_NewVar("LEVEL",                 (intptr_t)&ud.level_number,              GAMEVAR_SYSTEM | GAMEVAR_INT32PTR | GAMEVAR_READONLY);
    Gv_NewVar("MARKER",                (intptr_t)&ud.marker,                    GAMEVAR_SYSTEM | GAMEVAR_INT32PTR);
    Gv_NewVar("MONSTERS_OFF",          (intptr_t)&ud.monsters_off,              GAMEVAR_SYSTEM | GAMEVAR_INT32PTR);
    Gv_NewVar("MULTIMODE",             (intptr_t)&ud.multimode,                 GAMEVAR_SYSTEM | GAMEVAR_INT32PTR);
    Gv_NewVar("RESPAWN_INVENTORY",     (intptr_t)&ud.respawn_inventory,         GAMEVAR_SYSTEM | GAMEVAR_INT32PTR);
    Gv_NewVar("RESPAWN_ITEMS",         (intptr_t)&ud.respawn_items,             GAMEVAR_SYSTEM | GAMEVAR_INT32PTR);
    Gv_NewVar("RESPAWN_MONSTERS",      (intptr_t)&ud.respawn_monsters,          GAMEVAR_SYSTEM | GAMEVAR_INT32PTR);
    Gv_NewVar("RETURN",                0,                                       GAMEVAR_SYSTEM);
    Gv_NewVar("VOLUME",                (intptr_t)&ud.volume_number,             GAMEVAR_SYSTEM | GAMEVAR_INT32PTR | GAMEVAR_READONLY);
    Gv_NewVar("WEAPON",                0,                                       GAMEVAR_SYSTEM | GAMEVAR_PERPLAYER | GAMEVAR_READONLY);
    Gv_NewVar("WORKSLIKE",             0,                                       GAMEVAR_SYSTEM | GAMEVAR_PERPLAYER | GAMEVAR_READONLY);
    Gv_NewVar("ZRANGE",                4,                                       GAMEVAR_SYSTEM | GAMEVAR_PERPLAYER);
}

#undef ADDWEAPONVAR

void Gv_Init(void)
{
#if !defined LUNATIC
    // already initialized
    if (aGameVars[0].flags)
        return;
#else
    static int32_t inited=0;
    if (inited)
        return;
    inited = 1;
#endif

    // Set up weapon defaults, g_playerWeapon[][].
    Gv_AddSystemVars();
#if !defined LUNATIC
    Gv_InitWeaponPointers();
#endif
    Gv_ResetSystemDefaults();
}

#if !defined LUNATIC
void Gv_InitWeaponPointers(void)
{
    char aszBuf[64];
    // called from game Init AND when level is loaded...

    //AddLog("Gv_InitWeaponPointers");

    for (int i=(MAX_WEAPONS-1); i>=0; i--)
    {
        Bsprintf(aszBuf, "WEAPON%d_CLIP", i);
        aplWeaponClip[i] = Gv_GetVarDataPtr(aszBuf);

        if (!aplWeaponClip[i])
        {
            I_Error("ERROR: NULL weapon!  WTF?! %s\n", aszBuf);
        }

        Bsprintf(aszBuf, "WEAPON%d_RELOAD", i);
        aplWeaponReload[i] = Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf, "WEAPON%d_FIREDELAY", i);
        aplWeaponFireDelay[i] = Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf, "WEAPON%d_TOTALTIME", i);
        aplWeaponTotalTime[i] = Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf, "WEAPON%d_HOLDDELAY", i);
        aplWeaponHoldDelay[i] = Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf, "WEAPON%d_FLAGS", i);
        aplWeaponFlags[i] = Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf, "WEAPON%d_SHOOTS", i);
        aplWeaponShoots[i] = Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf, "WEAPON%d_SPAWNTIME", i);
        aplWeaponSpawnTime[i] = Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf, "WEAPON%d_SPAWN", i);
        aplWeaponSpawn[i] = Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf, "WEAPON%d_SHOTSPERBURST", i);
        aplWeaponShotsPerBurst[i] = Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf, "WEAPON%d_WORKSLIKE", i);
        aplWeaponWorksLike[i] = Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf, "WEAPON%d_INITIALSOUND", i);
        aplWeaponInitialSound[i] = Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf, "WEAPON%d_FIRESOUND", i);
        aplWeaponFireSound[i] = Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf, "WEAPON%d_SOUND2TIME", i);
        aplWeaponSound2Time[i] = Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf, "WEAPON%d_SOUND2SOUND", i);
        aplWeaponSound2Sound[i] = Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf, "WEAPON%d_FLASHCOLOR", i);
        aplWeaponFlashColor[i] = Gv_GetVarDataPtr(aszBuf);
    }
}

void Gv_RefreshPointers(void)
{
    aGameVars[Gv_GetVarIndex("COOP")].global              = (intptr_t)&ud.coop;
    aGameVars[Gv_GetVarIndex("FFIRE")].global             = (intptr_t)&ud.ffire;
    aGameVars[Gv_GetVarIndex("LEVEL")].global             = (intptr_t)&ud.level_number;
    aGameVars[Gv_GetVarIndex("MARKER")].global            = (intptr_t)&ud.marker;
    aGameVars[Gv_GetVarIndex("MONSTERS_OFF")].global      = (intptr_t)&ud.monsters_off;
    aGameVars[Gv_GetVarIndex("MULTIMODE")].global         = (intptr_t)&ud.multimode;
    aGameVars[Gv_GetVarIndex("RESPAWN_INVENTORY")].global = (intptr_t)&ud.respawn_inventory;
    aGameVars[Gv_GetVarIndex("RESPAWN_ITEMS")].global     = (intptr_t)&ud.respawn_items;
    aGameVars[Gv_GetVarIndex("RESPAWN_MONSTERS")].global  = (intptr_t)&ud.respawn_monsters;
    aGameVars[Gv_GetVarIndex("VOLUME")].global            = (intptr_t)&ud.volume_number;
}
#endif

END_DUKE_NS
