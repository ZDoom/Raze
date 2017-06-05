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

#include "duke3d.h"
#include "menus.h"

#define gamevars_c_

#ifdef LUNATIC
int32_t g_noResetVars;
LUNATIC_CB void (*A_ResetVars)(int32_t spriteNum);
#else

gamevar_t   aGameVars[MAXGAMEVARS];
gamearray_t aGameArrays[MAXGAMEARRAYS];
int32_t     g_gameVarCount   = 0;
int32_t     g_gameArrayCount = 0;

// pointers to weapon gamevar data
intptr_t *aplWeaponClip[MAX_WEAPONS];           // number of items in magazine
intptr_t *aplWeaponReload[MAX_WEAPONS];         // delay to reload (include fire)
intptr_t *aplWeaponFireDelay[MAX_WEAPONS];      // delay to fire
intptr_t *aplWeaponHoldDelay[MAX_WEAPONS];      // delay after release fire button to fire (0 for none)
intptr_t *aplWeaponTotalTime[MAX_WEAPONS];      // The total time the weapon is cycling before next fire.
intptr_t *aplWeaponFlags[MAX_WEAPONS];          // Flags for weapon
intptr_t *aplWeaponShoots[MAX_WEAPONS];         // what the weapon shoots
intptr_t *aplWeaponSpawnTime[MAX_WEAPONS];      // the frame at which to spawn an item
intptr_t *aplWeaponSpawn[MAX_WEAPONS];          // the item to spawn
intptr_t *aplWeaponShotsPerBurst[MAX_WEAPONS];  // number of shots per 'burst' (one ammo per 'burst')
intptr_t *aplWeaponWorksLike[MAX_WEAPONS];      // What original the weapon works like
intptr_t *aplWeaponInitialSound[MAX_WEAPONS];   // Sound made when weapon starts firing. zero for no sound
intptr_t *aplWeaponFireSound[MAX_WEAPONS];      // Sound made when firing (each time for automatic)
intptr_t *aplWeaponSound2Time[MAX_WEAPONS];     // Alternate sound time
intptr_t *aplWeaponSound2Sound[MAX_WEAPONS];    // Alternate sound sound ID
intptr_t *aplWeaponReloadSound1[MAX_WEAPONS];   // Sound of magazine being removed
intptr_t *aplWeaponReloadSound2[MAX_WEAPONS];   // Sound of magazine being inserted
intptr_t *aplWeaponSelectSound[MAX_WEAPONS];    // Sound of weapon being selected
intptr_t *aplWeaponFlashColor[MAX_WEAPONS];     // Muzzle flash color

# include "gamestructures.cpp"

// Frees the memory for the *values* of game variables and arrays. Resets their
// counts to zero. Call this function as many times as needed.
//
// Returns: old g_gameVarCount | (g_gameArrayCount<<16).
static int Gv_Free(void)
{
    for (bssize_t i=0; i<g_gameVarCount; ++i)
    {
        if (aGameVars[i].flags & GAMEVAR_USER_MASK)
            ALIGNED_FREE_AND_NULL(aGameVars[i].pValues);

        aGameVars[i].flags |= GAMEVAR_RESET;
    }

    for (bssize_t i=0; i<g_gameArrayCount; ++i)
    {
        if (aGameArrays[i].flags & GAMEARRAY_NORMAL)
            ALIGNED_FREE_AND_NULL(aGameArrays[i].pValues);

        aGameArrays[i].flags |= GAMEARRAY_RESET;
    }

    EDUKE32_STATIC_ASSERT(MAXGAMEVARS < 32768);
    int const varCount = g_gameVarCount | (g_gameArrayCount << 16);
    g_gameVarCount = g_gameArrayCount = 0;

    hash_init(&h_gamevars);
    hash_init(&h_arrays);

    return varCount;
}

// Calls Gv_Free() and in addition frees the labels of all game variables and
// arrays.
// Only call this function ONCE...
static void Gv_Clear(void)
{
    int       gameVarCount   = Gv_Free();
    int const gameArrayCount = gameVarCount >> 16;
    gameVarCount &= 65535;

    // Now, only do work that Gv_Free() hasn't done.
    for (bssize_t i=0; i<gameVarCount; ++i)
        DO_FREE_AND_NULL(aGameVars[i].szLabel);

    for (bssize_t i=0; i<gameArrayCount; i++)
        DO_FREE_AND_NULL(aGameArrays[i].szLabel);
}

int Gv_ReadSave(int32_t kFile)
{
    char savedstate[MAXVOLUMES*MAXLEVELS];
    char tbuf[12];

    if (kread(kFile, tbuf, 12)!=12) goto corrupt;
    if (Bmemcmp(tbuf, "BEG: EDuke32", 12)) { OSD_Printf("BEG ERR\n"); return 2; }

    Bmemset(&savedstate,0,sizeof(savedstate));

    //     AddLog("Reading gamevars from savegame");

    Gv_Free(); // nuke 'em from orbit, it's the only way to be sure...

    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);

    if (kdfread(&g_gameVarCount,sizeof(g_gameVarCount),1,kFile) != 1) goto corrupt;
    for (bssize_t i=0; i<g_gameVarCount; i++)
    {
        char *const olabel = aGameVars[i].szLabel;

        if (kdfread(&aGameVars[i], sizeof(gamevar_t), 1, kFile) != 1)
            goto corrupt;

        if (olabel == NULL)
            aGameVars[i].szLabel = (char *)Xmalloc(MAXVARLABEL * sizeof(uint8_t));
        else
            aGameVars[i].szLabel = olabel;

        if (kdfread(aGameVars[i].szLabel, MAXVARLABEL, 1, kFile) != 1)
            goto corrupt;
        hash_add(&h_gamevars, aGameVars[i].szLabel,i, 1);

        if (aGameVars[i].flags & GAMEVAR_PERPLAYER)
        {
            aGameVars[i].pValues = (intptr_t*)Xaligned_alloc(PLAYER_VAR_ALIGNMENT, MAXPLAYERS * sizeof(intptr_t));
            if (kdfread(aGameVars[i].pValues,sizeof(intptr_t) * MAXPLAYERS, 1, kFile) != 1) goto corrupt;
        }
        else if (aGameVars[i].flags & GAMEVAR_PERACTOR)
        {
            aGameVars[i].pValues = (intptr_t*)Xaligned_alloc(ACTOR_VAR_ALIGNMENT, MAXSPRITES * sizeof(intptr_t));
            if (kdfread(&aGameVars[i].pValues[0],sizeof(intptr_t), MAXSPRITES, kFile) != MAXSPRITES) goto corrupt;
        }
    }
    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);

    Gv_InitWeaponPointers();

    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);
    Gv_RefreshPointers();

    if (kdfread(&g_gameArrayCount,sizeof(g_gameArrayCount),1,kFile) != 1) goto corrupt;
    for (bssize_t i=0; i<g_gameArrayCount; i++)
    {
        if (aGameArrays[i].flags&GAMEARRAY_READONLY)
            continue;

        char *const olabel = aGameArrays[i].szLabel;

        // read for .size and .dwFlags (the rest are pointers):
        if (kdfread(&aGameArrays[i], sizeof(gamearray_t), 1, kFile) != 1)
            goto corrupt;

        if (olabel == NULL)
            aGameArrays[i].szLabel = (char *)Xmalloc(MAXARRAYLABEL * sizeof(uint8_t));
        else
            aGameArrays[i].szLabel = olabel;

        if (kdfread(aGameArrays[i].szLabel,sizeof(uint8_t) * MAXARRAYLABEL, 1, kFile) != 1)
            goto corrupt;
        hash_add(&h_arrays, aGameArrays[i].szLabel, i, 1);

        intptr_t const asize = aGameArrays[i].size;
        if (asize != 0)
        {
            aGameArrays[i].pValues = (intptr_t *)Xaligned_alloc(ACTOR_VAR_ALIGNMENT, asize * GAR_ELTSZ);
            if (kdfread(aGameArrays[i].pValues, GAR_ELTSZ * aGameArrays[i].size, 1, kFile) < 1) goto corrupt;
        }
        else
            aGameArrays[i].pValues = NULL;
    }

    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);
    if (kdfread(apScriptEvents,sizeof(apScriptEvents),1,kFile) != 1) goto corrupt;

    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);

    if (kdfread(&savedstate[0],sizeof(savedstate),1,kFile) != 1) goto corrupt;

    for (bssize_t i=0; i<(MAXVOLUMES*MAXLEVELS); i++)
    {
        if (savedstate[i])
        {
            if (g_mapInfo[i].savedstate == NULL)
                g_mapInfo[i].savedstate = (mapstate_t *)Xaligned_alloc(16, sizeof(mapstate_t));
            if (kdfread(g_mapInfo[i].savedstate,sizeof(mapstate_t),1,kFile) != sizeof(mapstate_t)) goto corrupt;
            for (bssize_t j=0; j<g_gameVarCount; j++)
            {
                if (aGameVars[j].flags & GAMEVAR_NORESET) continue;
                if (aGameVars[j].flags & GAMEVAR_PERPLAYER)
                {
//                    if (!g_mapInfo[i].savedstate->vars[j])
                    g_mapInfo[i].savedstate->vars[j] = (intptr_t *)Xaligned_alloc(PLAYER_VAR_ALIGNMENT, MAXPLAYERS * sizeof(intptr_t));
                    if (kdfread(&g_mapInfo[i].savedstate->vars[j][0],sizeof(intptr_t) * MAXPLAYERS, 1, kFile) != 1) goto corrupt;
                }
                else if (aGameVars[j].flags & GAMEVAR_PERACTOR)
                {
//                    if (!g_mapInfo[i].savedstate->vars[j])
                    g_mapInfo[i].savedstate->vars[j] = (intptr_t *)Xaligned_alloc(ACTOR_VAR_ALIGNMENT, MAXSPRITES * sizeof(intptr_t));
                    if (kdfread(&g_mapInfo[i].savedstate->vars[j][0],sizeof(intptr_t), MAXSPRITES, kFile) != MAXSPRITES) goto corrupt;
                }
            }
        }
        else
        {
            G_FreeMapState(i);
        }
    }

    if (kread(kFile, tbuf, 12)!=12) goto corrupt;
    if (Bmemcmp(tbuf, "EOF: EDuke32", 12)) { OSD_Printf("EOF ERR\n"); return 2; }

# if 0
    {
        FILE *fp;
        AddLog("Dumping Vars...");
        fp=fopen("xxx.txt","w");
        if (fp)
        {
            Gv_DumpValues(fp);
            fclose(fp);
        }
        AddLog("Done Dumping...");
    }
# endif
    return 0;
corrupt:
    return 1;
}

void Gv_WriteSave(FILE *fil)
{
    char savedstate[MAXVOLUMES*MAXLEVELS];

    Bmemset(&savedstate,0,sizeof(savedstate));

    //   AddLog("Saving Game Vars to File");
    fwrite("BEG: EDuke32", 12, 1, fil);

    dfwrite(&g_gameVarCount,sizeof(g_gameVarCount),1,fil);

    for (bssize_t i=0; i<g_gameVarCount; i++)
    {
        dfwrite(&(aGameVars[i]),sizeof(gamevar_t),1,fil);
        dfwrite(aGameVars[i].szLabel,sizeof(uint8_t) * MAXVARLABEL, 1, fil);

        if (aGameVars[i].flags & GAMEVAR_PERPLAYER)
        {
            //Bsprintf(g_szBuf,"Writing value array for %s (%d)",aGameVars[i].szLabel,sizeof(int32_t) * MAXPLAYERS);
            //AddLog(g_szBuf);
            dfwrite(aGameVars[i].pValues,sizeof(intptr_t) * MAXPLAYERS, 1, fil);
        }
        else if (aGameVars[i].flags & GAMEVAR_PERACTOR)
        {
            //Bsprintf(g_szBuf,"Writing value array for %s (%d)",aGameVars[i].szLabel,sizeof(int32_t) * MAXSPRITES);
            //AddLog(g_szBuf);
            dfwrite(&aGameVars[i].pValues[0],sizeof(intptr_t), MAXSPRITES, fil);
        }
    }

    dfwrite(&g_gameArrayCount,sizeof(g_gameArrayCount),1,fil);

    for (bssize_t i=0; i<g_gameArrayCount; i++)
    {
        if (aGameArrays[i].flags&GAMEARRAY_READONLY)
            continue;

        // write for .size and .dwFlags (the rest are pointers):
        dfwrite(&aGameArrays[i],sizeof(gamearray_t),1,fil);

        dfwrite(aGameArrays[i].szLabel,sizeof(uint8_t) * MAXARRAYLABEL, 1, fil);
        dfwrite(aGameArrays[i].pValues, GAR_ELTSZ * aGameArrays[i].size, 1, fil);
    }

    dfwrite(apScriptEvents,sizeof(apScriptEvents),1,fil);

    for (bssize_t i=0; i<(MAXVOLUMES*MAXLEVELS); i++)
        if (g_mapInfo[i].savedstate != NULL)
            savedstate[i] = 1;

    dfwrite(&savedstate[0],sizeof(savedstate),1,fil);

    for (bssize_t i=0; i<(MAXVOLUMES*MAXLEVELS); i++)
        if (g_mapInfo[i].savedstate)
        {
            dfwrite(g_mapInfo[i].savedstate,sizeof(mapstate_t),1,fil);
            for (bssize_t j=0; j<g_gameVarCount; j++)
            {
                if (aGameVars[j].flags & GAMEVAR_NORESET) continue;
                if (aGameVars[j].flags & GAMEVAR_PERPLAYER)
                {
                    dfwrite(&g_mapInfo[i].savedstate->vars[j][0],sizeof(intptr_t) * MAXPLAYERS, 1, fil);
                }
                else if (aGameVars[j].flags & GAMEVAR_PERACTOR)
                {
                    dfwrite(&g_mapInfo[i].savedstate->vars[j][0],sizeof(intptr_t), MAXSPRITES, fil);
                }
            }
        }

    fwrite("EOF: EDuke32", 12, 1, fil);
}

void Gv_DumpValues(void)
{
    buildprint("// Current Game Definitions\n\n");

    for (bssize_t i=0; i<g_gameVarCount; i++)
    {
        buildprint("gamevar ", aGameVars[i].szLabel, " ");

        if (aGameVars[i].flags & (GAMEVAR_INTPTR))
            buildprint(*(int32_t *)aGameVars[i].global);
        else if (aGameVars[i].flags & (GAMEVAR_SHORTPTR))
            buildprint(*(int16_t *)aGameVars[i].global);
        else if (aGameVars[i].flags & (GAMEVAR_CHARPTR))
            buildprint(*(int8_t *)aGameVars[i].global);
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

    osd->log.errors = 0;

    for (bssize_t i=0; i<MAXGAMEVARS; i++)
    {
        if (aGameVars[i].szLabel != NULL)
            Gv_NewVar(aGameVars[i].szLabel,
                      aGameVars[i].flags & GAMEVAR_NODEFAULT ? aGameVars[i].global : aGameVars[i].defaultValue,
                      aGameVars[i].flags);
    }

    for (bssize_t i=0; i<MAXGAMEARRAYS; i++)
    {
        if (aGameArrays[i].szLabel != NULL && (aGameArrays[i].flags & GAMEARRAY_RESET))
            Gv_NewArray(aGameArrays[i].szLabel,aGameArrays[i].pValues,aGameArrays[i].size,aGameArrays[i].flags);
    }
}

int32_t Gv_NewArray(const char *pszLabel, void *pArray, intptr_t arraySize, uint32_t nFlags)
{
    int32_t i;

    if (EDUKE32_PREDICT_FALSE(g_gameArrayCount >= MAXGAMEARRAYS))
    {
        g_errorCnt++;
        C_ReportError(-1);
        initprintf("%s:%d: error: too many arrays!\n",g_scriptFileName,g_lineNumber);
        return 0;
    }

    if (EDUKE32_PREDICT_FALSE(Bstrlen(pszLabel) > (MAXARRAYLABEL-1)))
    {
        g_errorCnt++;
        C_ReportError(-1);
        initprintf("%s:%d: error: array name `%s' exceeds limit of %d characters.\n",g_scriptFileName,g_lineNumber,pszLabel, MAXARRAYLABEL);
        return 0;
    }
    i = hash_find(&h_arrays,pszLabel);

    if (EDUKE32_PREDICT_FALSE(i >=0 && !(aGameArrays[i].flags & GAMEARRAY_RESET)))
    {
        // found it it's a duplicate in error

        g_warningCnt++;

        if (aGameArrays[i].flags&GAMEARRAY_TYPE_MASK)
        {
            C_ReportError(-1);
            initprintf("ignored redefining system array `%s'.", pszLabel);
        }
        else
            C_ReportError(WARNING_DUPLICATEDEFINITION);

        return 0;
    }

    i = g_gameArrayCount;

    if (aGameArrays[i].szLabel == NULL)
        aGameArrays[i].szLabel = (char *)Xcalloc(MAXVARLABEL,sizeof(uint8_t));

    if (aGameArrays[i].szLabel != pszLabel)
        Bstrcpy(aGameArrays[i].szLabel,pszLabel);

    if (!(nFlags & GAMEARRAY_TYPE_MASK))
    {
        Baligned_free(aGameArrays[i].pValues);
        if (arraySize != 0)
        {
            aGameArrays[i].pValues = (intptr_t *)Xaligned_alloc(ACTOR_VAR_ALIGNMENT, arraySize * GAR_ELTSZ);
            Bmemset(aGameArrays[i].pValues, 0, arraySize * GAR_ELTSZ);
        }
        else
            aGameArrays[i].pValues = NULL;
    }
    else
        aGameArrays[i].pValues=(intptr_t *)pArray;

    aGameArrays[i].size   = arraySize;
    aGameArrays[i].flags = nFlags & ~GAMEARRAY_RESET;

    g_gameArrayCount++;
    hash_add(&h_arrays, aGameArrays[i].szLabel, i, 1);

    return 1;
}

int32_t Gv_NewVar(const char *pszLabel, intptr_t lValue, uint32_t dwFlags)
{
    //Bsprintf(g_szBuf,"Gv_NewVar(%s, %d, %X)",pszLabel, lValue, dwFlags);
    //AddLog(g_szBuf);

    if (EDUKE32_PREDICT_FALSE(g_gameVarCount >= MAXGAMEVARS))
    {
        g_errorCnt++;
        C_ReportError(-1);
        initprintf("%s:%d: error: too many gamevars!\n",g_scriptFileName,g_lineNumber);
        return 0;
    }

    if (EDUKE32_PREDICT_FALSE(Bstrlen(pszLabel) > (MAXVARLABEL-1)))
    {
        g_errorCnt++;
        C_ReportError(-1);
        initprintf("%s:%d: error: variable name `%s' exceeds limit of %d characters.\n",g_scriptFileName,g_lineNumber,pszLabel, MAXVARLABEL);
        return 0;
    }

    int gV = hash_find(&h_gamevars,pszLabel);

    if (gV >= 0 && !(aGameVars[gV].flags & GAMEVAR_RESET))
    {
        // found it...
        if (EDUKE32_PREDICT_FALSE(aGameVars[gV].flags & (GAMEVAR_PTR_MASK)))
        {
            C_ReportError(-1);
            initprintf("%s:%d: warning: cannot redefine internal gamevar `%s'.\n",g_scriptFileName,g_lineNumber,label+(g_labelCnt<<6));
            return 0;
        }
        else if (EDUKE32_PREDICT_FALSE(!(aGameVars[gV].flags & GAMEVAR_SYSTEM)))
        {
            // it's a duplicate in error
            g_warningCnt++;
            C_ReportError(WARNING_DUPLICATEDEFINITION);
            return 0;
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

    return 1;
}

static int Gv_GetVarIndex(const char *szGameLabel)
{
    int const gameVar = hash_find(&h_gamevars,szGameLabel);

    if (EDUKE32_PREDICT_FALSE(gameVar == -1))
    {
        OSD_Printf(OSD_ERROR "Gv_GetVarIndex(): INTERNAL ERROR: couldn't find gamevar %s!\n",szGameLabel);
        return 0;
    }

    return gameVar;
}

int __fastcall Gv_GetArrayValue(int const id, int index)
{
    if (aGameArrays[id].flags & GAMEARRAY_STRIDE2)
        index <<= 1;

    int returnValue = -1;

    switch (aGameArrays[id].flags & GAMEARRAY_TYPE_MASK)
    {
        case 0:                 returnValue = (aGameArrays[id].pValues)[index]; break;
        case GAMEARRAY_OFINT:   returnValue = ((int32_t *)aGameArrays[id].pValues)[index]; break;
        case GAMEARRAY_OFSHORT: returnValue = ((int16_t *)aGameArrays[id].pValues)[index]; break;
        case GAMEARRAY_OFCHAR:  returnValue = ((uint8_t *)aGameArrays[id].pValues)[index]; break;
    }

    return returnValue;
}

int __fastcall Gv_GetVar(int gameVar, int spriteNum, int playerNum)
{
    if (gameVar == g_thisActorVarID)
        return spriteNum;

    if (gameVar == MAXGAMEVARS)
        return *insptr++;

    int invertResult = !!(gameVar & (MAXGAMEVARS << 1));

    if (EDUKE32_PREDICT_FALSE((gameVar & ~(MAXGAMEVARS << 1)) >= g_gameVarCount))
        goto special;

    gameVar &= (MAXGAMEVARS - 1);

    int returnValue, varFlags;
    varFlags = aGameVars[gameVar].flags & (GAMEVAR_USER_MASK | GAMEVAR_PTR_MASK);

    if (varFlags == GAMEVAR_PERACTOR)
    {
        if (EDUKE32_PREDICT_FALSE((unsigned) spriteNum >= MAXSPRITES)) goto badindex;
        returnValue = aGameVars[gameVar].pValues[spriteNum];
    }
    else if (!varFlags) returnValue = aGameVars[gameVar].global;
    else if (varFlags == GAMEVAR_PERPLAYER)
    {
        if (EDUKE32_PREDICT_FALSE((unsigned) playerNum >= MAXPLAYERS))
        {
            spriteNum = playerNum;
            goto badindex;
        }
        returnValue = aGameVars[gameVar].pValues[playerNum];
    }
    else switch (varFlags)
    {
        case GAMEVAR_INTPTR: returnValue   = *(int32_t *)aGameVars[gameVar].global; break;
        case GAMEVAR_SHORTPTR: returnValue = *(int16_t *)aGameVars[gameVar].global; break;
        case GAMEVAR_CHARPTR: returnValue  = *(char *)aGameVars[gameVar].global; break;
        default: EDUKE32_UNREACHABLE_SECTION(returnValue = 0; break);
    }

    return (returnValue ^ -invertResult) + invertResult;

special:
    if (gameVar & (MAXGAMEVARS << 2))  // array
    {
        gameVar &= (MAXGAMEVARS - 1);  // ~((MAXGAMEVARS<<2)|(MAXGAMEVARS<<1));

        int const arrayIndex = Gv_GetVar(*insptr++, spriteNum, playerNum);

        if (EDUKE32_PREDICT_FALSE((unsigned)arrayIndex >= (unsigned)aGameArrays[gameVar].size))
        {
            spriteNum = arrayIndex;
            goto badarrayindex;
        }

        returnValue = Gv_GetArrayValue(gameVar, arrayIndex);
    }
    else if (gameVar&(MAXGAMEVARS<<3)) // struct shortcut vars
    {
        int arrayIndexVar = *insptr++;
        int arrayIndex = Gv_GetVar(arrayIndexVar, spriteNum, playerNum);

        gameVar &= (MAXGAMEVARS - 1);

        switch (gameVar - g_structVarIDs)
        {
        case STRUCT_SPRITE:
        {
            int const label = *insptr++;

            arrayIndexVar = (EDUKE32_PREDICT_FALSE(ActorLabels[label].flags & LABEL_HASPARM2)) ?
                        Gv_GetVar(*insptr++, spriteNum, playerNum) : 0;

            if (EDUKE32_PREDICT_FALSE((unsigned) arrayIndex >= MAXSPRITES))
            {
                spriteNum = arrayIndex;
                goto badindex;
            }

            returnValue = VM_GetSprite(arrayIndex, label, arrayIndexVar);
            break;
        }
        case STRUCT_TSPR:
        {
            int const label = *insptr++;

            if (EDUKE32_PREDICT_FALSE((unsigned) arrayIndex >= MAXSPRITES))
            {
                spriteNum = arrayIndex;
                goto badindex;
            }

            returnValue = VM_GetTsprite(arrayIndex, label);
            break;
        }
        case STRUCT_THISPROJECTILE:
        {
            int const label = *insptr++;

            if (EDUKE32_PREDICT_FALSE((unsigned) arrayIndex >= MAXSPRITES))
            {
                spriteNum = arrayIndex;
                goto badindex;
            }

            returnValue = VM_GetActiveProjectile(arrayIndex, label);
            break;
        }

        case STRUCT_PROJECTILE:
        {
            int const label = *insptr++;

            if (EDUKE32_PREDICT_FALSE((unsigned) arrayIndex >= MAXTILES))
            {
                spriteNum = arrayIndex;
                goto badindex;
            }

            returnValue = VM_GetProjectile(arrayIndex, label);
            break;
        }
        case STRUCT_TILEDATA:
        {
            int const label = *insptr++;

            if (EDUKE32_PREDICT_FALSE((unsigned) arrayIndex >= MAXTILES))
            {
                spriteNum = arrayIndex;
                goto badindex;
            }

            returnValue = VM_GetTileData(arrayIndex, label);
            break;
        }

        case STRUCT_PALDATA:
        {
            int const label = *insptr++;

            if (EDUKE32_PREDICT_FALSE((unsigned) arrayIndex >= MAXPALOOKUPS))
            {
                spriteNum = arrayIndex;
                goto badindex;
            }

            returnValue = VM_GetPalData(arrayIndex, label);
            break;
        }

        case STRUCT_PLAYER:
        {
            int const label = *insptr++;

            if (arrayIndexVar == g_thisActorVarID) arrayIndex = vm.playerNum;

            arrayIndexVar = (EDUKE32_PREDICT_FALSE(PlayerLabels[label].flags & LABEL_HASPARM2)) ?
                Gv_GetVar(*insptr++, spriteNum, playerNum) : 0;

            if (EDUKE32_PREDICT_FALSE((unsigned) arrayIndex >= MAXPLAYERS))
            {
                spriteNum = arrayIndex;
                goto badindex;
            }

            returnValue = VM_GetPlayer(arrayIndex, label, arrayIndexVar);
            break;
        }
        case STRUCT_INPUT:
        {
            int const label = *insptr++;

            if (arrayIndexVar == g_thisActorVarID) arrayIndex = vm.playerNum;

            if (EDUKE32_PREDICT_FALSE((unsigned) arrayIndex >= MAXPLAYERS))
            {
                spriteNum = arrayIndex;
                goto badindex;
            }

            returnValue = VM_GetPlayerInput(arrayIndex, label);
            break;
        }

        case STRUCT_ACTORVAR:
        case STRUCT_PLAYERVAR:
            returnValue = Gv_GetVar(*insptr++, arrayIndex, playerNum);
            break;

        case STRUCT_SECTOR:
            if (arrayIndexVar == g_thisActorVarID) arrayIndex = sprite[vm.spriteNum].sectnum;
            if (EDUKE32_PREDICT_FALSE((unsigned) arrayIndex >= MAXSECTORS))
            {
                spriteNum = arrayIndex;
                insptr++;
                goto badindex;
            }
            returnValue = VM_GetSector(arrayIndex, *insptr++);
            break;

        case STRUCT_WALL:
            if (EDUKE32_PREDICT_FALSE((unsigned) arrayIndex >= MAXWALLS))
            {
                spriteNum = arrayIndex;
                insptr++;
                goto badindex;
            }
            returnValue = VM_GetWall(arrayIndex, *insptr++);
            break;

        case STRUCT_USERDEF:
            returnValue = VM_GetUserdef(*insptr++);
            break;

        default:
            EDUKE32_UNREACHABLE_SECTION(return -1);
        }
    }
    else
    {
        CON_ERRPRINTF("Gv_GetVar(): invalid gamevar ID (%d)\n", gameVar);
        return -1;
    }

    return (returnValue ^ -invertResult) + invertResult;

badarrayindex:
    CON_ERRPRINTF("Gv_GetVar(): invalid array index (%s[%d])\n", aGameArrays[gameVar].szLabel,spriteNum);
    return -1;

badindex:
    CON_ERRPRINTF("Gv_GetVar(): invalid index %d for \"%s\"\n", spriteNum, aGameVars[gameVar].szLabel);
    return -1;
}

void __fastcall Gv_SetVar(int const gameVar, int const newValue, int const spriteNum, int const playerNum)
{
    int const varFlags = aGameVars[gameVar].flags & (GAMEVAR_USER_MASK|GAMEVAR_PTR_MASK);

    if (EDUKE32_PREDICT_FALSE((unsigned)gameVar >= (unsigned)g_gameVarCount)) goto badvarid;

    if (!varFlags) aGameVars[gameVar].global=newValue;
    else if (varFlags == GAMEVAR_PERPLAYER)
    {
        if (EDUKE32_PREDICT_FALSE((unsigned) playerNum > MAXPLAYERS-1)) goto badindex;
        // for the current player
        aGameVars[gameVar].pValues[playerNum]=newValue;
    }
    else if (varFlags == GAMEVAR_PERACTOR)
    {
        if (EDUKE32_PREDICT_FALSE((unsigned) spriteNum > MAXSPRITES-1)) goto badindex;
        aGameVars[gameVar].pValues[spriteNum]=newValue;
    }
    else
    {
        switch (varFlags)
        {
            case GAMEVAR_INTPTR: *((int32_t *)aGameVars[gameVar].global)   = (int32_t)newValue; break;
            case GAMEVAR_SHORTPTR: *((int16_t *)aGameVars[gameVar].global) = (int16_t)newValue; break;
            case GAMEVAR_CHARPTR: *((uint8_t *)aGameVars[gameVar].global)  = (uint8_t)newValue; break;
        }
    }
    return;

badvarid:
    CON_ERRPRINTF("Gv_SetVar(): invalid gamevar (%d) from sprite %d (%d), player %d\n",
                  gameVar,vm.spriteNum,TrackerCast(sprite[vm.spriteNum].picnum),vm.playerNum);
    return;

badindex:
    CON_ERRPRINTF("Gv_SetVar(): invalid index (%d) for gamevar %s from sprite %d, player %d\n",
               aGameVars[gameVar].flags & GAMEVAR_PERACTOR ? spriteNum : playerNum,
               aGameVars[gameVar].szLabel,vm.spriteNum,vm.playerNum);
}

enum
{
    GVX_BADVARID = 0,
    GVX_BADPLAYER,
    GVX_BADSPRITE,
    GVX_BADSECTOR,
    GVX_BADWALL,
    GVX_BADINDEX,
    GVX_BADTILE,
    GVX_BADPAL,
};

static const char *gvxerrs[] = {
    "Gv_GetVarX(): invalid gamevar ID",
    "Gv_GetVarX(): invalid player ID",
    "Gv_GetVarX(): invalid sprite ID",
    "Gv_GetVarX(): invalid sector ID",
    "Gv_GetVarX(): invalid wall ID",
    "Gv_GetVarX(): invalid array index",
    "Gv_GetVarX(): invalid tile ID",
    "Gv_GetVarX(): invalid pal ID",
};

int __fastcall Gv_GetSpecialVarX(int gameVar)
{
    int returnValue = -1;

    if (gameVar & (MAXGAMEVARS << 2))  // array
    {
        int const arrayIndex = Gv_GetVarX(*insptr++);

        gameVar &= (MAXGAMEVARS - 1);  // ~((MAXGAMEVARS<<2)|(MAXGAMEVARS<<1));

        int const arraySiz
        = (aGameArrays[gameVar].flags & GAMEARRAY_VARSIZE) ? Gv_GetVarX(aGameArrays[gameVar].size) : aGameArrays[gameVar].size;

        if (EDUKE32_PREDICT_FALSE((unsigned) arrayIndex >= (unsigned) arraySiz))
        {
            CON_ERRPRINTF("%s %s[%d]\n", gvxerrs[GVX_BADINDEX], aGameArrays[gameVar].szLabel, arrayIndex);
            return -1;
        }

        returnValue = Gv_GetArrayValue(gameVar, arrayIndex);
    }
    else if (gameVar & (MAXGAMEVARS << 3))  // struct shortcut vars
    {
        int arrayIndexVar = *insptr++;
        int arrayIndex    = Gv_GetVarX(arrayIndexVar);

        switch ((gameVar & (MAXGAMEVARS - 1)) - g_structVarIDs)
        {
            case STRUCT_SPRITE:
            {
                int const labelNum = *insptr++;

                arrayIndexVar = (EDUKE32_PREDICT_FALSE(ActorLabels[labelNum].flags & LABEL_HASPARM2)) ? Gv_GetVarX(*insptr++) : 0;

                if (EDUKE32_PREDICT_FALSE((unsigned) arrayIndex >= MAXSPRITES))
                {
                    gameVar = arrayIndex;
                    returnValue = GVX_BADSPRITE;
                    goto badindex;
                }

                returnValue = VM_GetSprite(arrayIndex, labelNum, arrayIndexVar);
                break;
            }
            case STRUCT_TSPR:
            {
                int const labelNum = *insptr++;

                if (EDUKE32_PREDICT_FALSE((unsigned) arrayIndex >= MAXSPRITES))
                {
                    gameVar = arrayIndex;
                    returnValue = GVX_BADSPRITE;
                    goto badindex;
                }

                returnValue = VM_GetTsprite(arrayIndex, labelNum);
                break;
            }
            case STRUCT_THISPROJECTILE:
            {
                int const labelNum = *insptr++;

                if (EDUKE32_PREDICT_FALSE((unsigned) arrayIndex >= MAXSPRITES))
                {
                    gameVar = arrayIndex;
                    returnValue = GVX_BADSPRITE;
                    goto badindex;
                }

                returnValue = VM_GetActiveProjectile(arrayIndex, labelNum);
                break;
            }

            case STRUCT_PROJECTILE:
            {
                int const labelNum = *insptr++;

                if (EDUKE32_PREDICT_FALSE((unsigned) arrayIndex >= MAXTILES))
                {
                    gameVar = arrayIndex;
                    returnValue = GVX_BADTILE;
                    goto badindex;
                }

                returnValue = VM_GetProjectile(arrayIndex, labelNum);
                break;
            }
            case STRUCT_TILEDATA:
            {
                int const labelNum = *insptr++;

                if (EDUKE32_PREDICT_FALSE((unsigned) arrayIndex >= MAXTILES))
                {
                    gameVar = arrayIndex;
                    returnValue = GVX_BADTILE;
                    goto badindex;
                }

                returnValue = VM_GetTileData(arrayIndex, labelNum);
                break;
            }

            case STRUCT_PALDATA:
            {
                int const labelNum = *insptr++;

                if (EDUKE32_PREDICT_FALSE((unsigned) arrayIndex >= MAXPALOOKUPS))
                {
                    gameVar = arrayIndex;
                    returnValue = GVX_BADPAL;
                    goto badindex;
                }

                returnValue = VM_GetPalData(arrayIndex, labelNum);
                break;
            }

            case STRUCT_PLAYER:
            {
                int const labelNum = *insptr++;

                if (arrayIndexVar == g_thisActorVarID)
                    arrayIndex = vm.playerNum;

                arrayIndexVar = (EDUKE32_PREDICT_FALSE(PlayerLabels[labelNum].flags & LABEL_HASPARM2)) ?
                    Gv_GetVarX(*insptr++) : 0;

                if (EDUKE32_PREDICT_FALSE((unsigned) arrayIndex >= MAXPLAYERS))
                {
                    gameVar = arrayIndex;
                    returnValue = GVX_BADPLAYER;
                    goto badindex;
                }

                returnValue = VM_GetPlayer(arrayIndex, labelNum, arrayIndexVar);
                break;
            }
            case STRUCT_INPUT:
            {
                int const labelNum = *insptr++;

                if (arrayIndexVar == g_thisActorVarID)
                    arrayIndex = vm.playerNum;

                if (EDUKE32_PREDICT_FALSE((unsigned) arrayIndex >= MAXPLAYERS))
                {
                    gameVar = arrayIndex;
                    returnValue = GVX_BADPLAYER;
                    goto badindex;
                }

                returnValue = VM_GetPlayerInput(arrayIndex, labelNum);
                break;
            }

            case STRUCT_ACTORVAR:
            case STRUCT_PLAYERVAR:
                returnValue = Gv_GetVar(*insptr++, arrayIndex, vm.playerNum);
                break;

            case STRUCT_SECTOR:
                if (arrayIndexVar == g_thisActorVarID)
                    arrayIndex = sprite[vm.spriteNum].sectnum;

                if (EDUKE32_PREDICT_FALSE((unsigned) arrayIndex >= MAXSECTORS))
                {
                    gameVar = arrayIndex;
                    insptr++;
                    returnValue = GVX_BADSECTOR;
                    goto badindex;
                }
                returnValue = VM_GetSector(arrayIndex, *insptr++);
                break;

            case STRUCT_WALL:
                if (EDUKE32_PREDICT_FALSE((unsigned) arrayIndex >= MAXWALLS))
                {
                    gameVar = arrayIndex;
                    insptr++;
                    returnValue = GVX_BADWALL;
                    goto badindex;
                }
                returnValue = VM_GetWall(arrayIndex, *insptr++);
                break;

            case STRUCT_USERDEF:
                returnValue = VM_GetUserdef(*insptr++);
                break;

            default: EDUKE32_UNREACHABLE_SECTION(return -1);
        }
    }

    return returnValue;

badindex:
    CON_ERRPRINTF("%s %d\n", gvxerrs[returnValue], gameVar);
    return -1;
}

int __fastcall Gv_GetVarX(int gameVar)
{
    if (gameVar == g_thisActorVarID)
        return vm.spriteNum;

    if (gameVar == MAXGAMEVARS)
        return *insptr++;

    int const invertResult = !!(gameVar & (MAXGAMEVARS << 1));
    int       returnValue  = -1;

    if (EDUKE32_PREDICT_FALSE(gameVar >= g_gameVarCount && invertResult == 0))
        returnValue = Gv_GetSpecialVarX(gameVar);
    else
    {
        gameVar &= MAXGAMEVARS-1;

        int const varFlags = aGameVars[gameVar].flags & (GAMEVAR_USER_MASK|GAMEVAR_PTR_MASK);

        if (!varFlags) returnValue = aGameVars[gameVar].global;
        else if (varFlags == GAMEVAR_PERPLAYER)
        {
            if (EDUKE32_PREDICT_FALSE((unsigned) vm.playerNum >= MAXPLAYERS))
                goto perr;
            returnValue = aGameVars[gameVar].pValues[vm.playerNum];
        }
        else if (varFlags == GAMEVAR_PERACTOR)
            returnValue = aGameVars[gameVar].pValues[vm.spriteNum];
        else switch (varFlags)
        {
            case GAMEVAR_INTPTR: returnValue   = (*((int32_t *)aGameVars[gameVar].global)); break;
            case GAMEVAR_SHORTPTR: returnValue = (*((int16_t *)aGameVars[gameVar].global)); break;
            case GAMEVAR_CHARPTR: returnValue  = (*((uint8_t *)aGameVars[gameVar].global)); break;
        }
    }

    return (returnValue ^ -invertResult) + invertResult;

perr:
    CON_ERRPRINTF("%s %d\n", gvxerrs[GVX_BADPLAYER], vm.playerNum);
    return -1;
}

void __fastcall Gv_GetManyVars(int const count, int32_t * const rv)
{
    for (bssize_t j = 0; j < count; ++j)
    {
        int gameVar = *insptr++;

        if (gameVar == g_thisActorVarID)
        {
            rv[j] = vm.spriteNum;
            continue;
        }

        if (gameVar == MAXGAMEVARS)
        {
            rv[j] = *insptr++;
            continue;
        }

        int const invertResult = !!(gameVar & (MAXGAMEVARS << 1));

        if (EDUKE32_PREDICT_FALSE(gameVar >= g_gameVarCount && invertResult == 0))
        {
            rv[j] = Gv_GetSpecialVarX(gameVar);
            continue;
        }

        gameVar &= MAXGAMEVARS - 1;

        int const varFlags = aGameVars[gameVar].flags & (GAMEVAR_USER_MASK | GAMEVAR_PTR_MASK);
        int       value    = aGameVars[gameVar].global;

        if (varFlags == GAMEVAR_PERPLAYER)
        {
            if (EDUKE32_PREDICT_FALSE((unsigned)vm.playerNum >= MAXPLAYERS))
                goto perr;
            value = aGameVars[gameVar].pValues[vm.playerNum];
        }
        else if (varFlags == GAMEVAR_PERACTOR)
            value = aGameVars[gameVar].pValues[vm.spriteNum];
        else
        {
            switch (varFlags)
            {
                case GAMEVAR_INTPTR: value   = (*((int32_t *)aGameVars[gameVar].global)); break;
                case GAMEVAR_SHORTPTR: value = (*((int16_t *)aGameVars[gameVar].global)); break;
                case GAMEVAR_CHARPTR: value  = (*((uint8_t *)aGameVars[gameVar].global)); break;
            }
        }

        rv[j] = (value ^ -invertResult) + invertResult;
        continue;

    perr:
        CON_ERRPRINTF("%s %d\n", gvxerrs[GVX_BADPLAYER], vm.playerNum);
    }
}

void __fastcall Gv_SetVarX(int const gameVar, int const newValue)
{
    int const varFlags = aGameVars[gameVar].flags & (GAMEVAR_USER_MASK|GAMEVAR_PTR_MASK);

    if (!varFlags) aGameVars[gameVar].global = newValue;
    else if (varFlags == GAMEVAR_PERPLAYER)
    {
        if (EDUKE32_PREDICT_FALSE((unsigned)vm.playerNum >= MAXPLAYERS)) goto badindex;
        aGameVars[gameVar].pValues[vm.playerNum] = newValue;
    }
    else if (varFlags == GAMEVAR_PERACTOR)
    {
        if (EDUKE32_PREDICT_FALSE((unsigned)vm.spriteNum >= MAXSPRITES)) goto badindex;
        aGameVars[gameVar].pValues[vm.spriteNum] = newValue;
    }
    else switch (varFlags)
    {
        case GAMEVAR_INTPTR: *((int32_t *)aGameVars[gameVar].global)   = (int32_t)newValue; break;
        case GAMEVAR_SHORTPTR: *((int16_t *)aGameVars[gameVar].global) = (int16_t)newValue; break;
        case GAMEVAR_CHARPTR: *((uint8_t *)aGameVars[gameVar].global)  = (uint8_t)newValue; break;
    }

    return;

badindex:
    CON_ERRPRINTF("Gv_SetVar(): invalid index (%d) for gamevar %s\n",
               aGameVars[gameVar].flags & GAMEVAR_PERACTOR ? vm.spriteNum : vm.playerNum,
               aGameVars[gameVar].szLabel);
}

int Gv_GetVarByLabel(const char *szGameLabel, int const defaultValue, int const spriteNum, int const playerNum)
{
    int const gameVar = hash_find(&h_gamevars, szGameLabel);
    return EDUKE32_PREDICT_FALSE(gameVar < 0) ? defaultValue : Gv_GetVar(gameVar, spriteNum, playerNum);
}

static intptr_t *Gv_GetVarDataPtr(const char *szGameLabel)
{
    int const gameVar = hash_find(&h_gamevars, szGameLabel);

    if (EDUKE32_PREDICT_FALSE(gameVar < 0))
        return NULL;

    if (aGameVars[gameVar].flags & (GAMEVAR_PERACTOR | GAMEVAR_PERPLAYER))
    {
        if (EDUKE32_PREDICT_FALSE(!aGameVars[gameVar].pValues))
            CON_ERRPRINTF("Gv_GetVarDataPtr(): INTERNAL ERROR: NULL array !!!\n");
        return aGameVars[gameVar].pValues;
    }

    return &(aGameVars[gameVar].global);
}
#endif  // !defined LUNATIC

void Gv_ResetSystemDefaults(void)
{
    // call many times...
#if !defined LUNATIC
    char aszBuf[64];

    //AddLog("ResetWeaponDefaults");

    for (int weaponNum = 0; weaponNum < MAX_WEAPONS; ++weaponNum)
    {
        for (bssize_t playerNum = 0; playerNum < MAXPLAYERS; ++playerNum)
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
            Bsprintf(aszBuf, "WEAPON%d_RELOADSOUND1", weaponNum);
            aplWeaponReloadSound1[weaponNum][playerNum] = Gv_GetVarByLabel(aszBuf, 0, -1, playerNum);
            Bsprintf(aszBuf, "WEAPON%d_RELOADSOUND2", weaponNum);
            aplWeaponReloadSound2[weaponNum][playerNum] = Gv_GetVarByLabel(aszBuf, 0, -1, playerNum);
            Bsprintf(aszBuf, "WEAPON%d_SELECTSOUND", weaponNum);
            aplWeaponSelectSound[weaponNum][playerNum] = Gv_GetVarByLabel(aszBuf, 0, -1, playerNum);
            Bsprintf(aszBuf, "WEAPON%d_FLASHCOLOR", weaponNum);
            aplWeaponFlashColor[weaponNum][playerNum] = Gv_GetVarByLabel(aszBuf, 0, -1, playerNum);
        }
    }

    g_returnVarID    = Gv_GetVarIndex("RETURN");
    g_weaponVarID    = Gv_GetVarIndex("WEAPON");
    g_worksLikeVarID = Gv_GetVarIndex("WORKSLIKE");
    g_zRangeVarID    = Gv_GetVarIndex("ZRANGE");
    g_angRangeVarID  = Gv_GetVarIndex("ANGRANGE");
    g_aimAngleVarID  = Gv_GetVarIndex("AUTOAIMANGLE");
    g_lotagVarID     = Gv_GetVarIndex("LOTAG");
    g_hitagVarID     = Gv_GetVarIndex("HITAG");
    g_textureVarID   = Gv_GetVarIndex("TEXTURE");
    g_thisActorVarID = Gv_GetVarIndex("THISACTOR");
    g_structVarIDs   = Gv_GetVarIndex("sprite");
#endif

    for (bssize_t weaponNum = 0; weaponNum <= MAXTILES - 1; weaponNum++)
        if (g_tile[weaponNum].defproj)
            *g_tile[weaponNum].proj = *g_tile[weaponNum].defproj;

    //AddLog("EOF:ResetWeaponDefaults");
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
        ReloadSound1, ReloadSound2, SelectSound, FlashColor
    */

    {
        KNEE_WEAPON, 0, 0, 7, 14, 0,
        WEAPON_NOVISIBLE | WEAPON_RANDOMRESTART | WEAPON_AUTOMATIC,
        KNEE__STATIC, 0, 0, 0, 0, 0, 0,
        0, EJECT_CLIP__STATIC, INSERT_CLIP__STATIC, 0, 0
    },

    {
        PISTOL_WEAPON, 12, 27, 2, 5, 0,
        WEAPON_RELOAD_TIMING,
        SHOTSPARK1__STATIC, 2, SHELL__STATIC, 0, 0, PISTOL_FIRE__STATIC, 0, 0,
        EJECT_CLIP__STATIC, INSERT_CLIP__STATIC, INSERT_CLIP__STATIC, 255+(95<<8)
    },

    {
        SHOTGUN_WEAPON, 0, 13, 4, 30, 0,
        WEAPON_CHECKATRELOAD,
        SHOTGUN__STATIC, 24, SHOTGUNSHELL__STATIC, 7, 0, SHOTGUN_FIRE__STATIC, 15, SHOTGUN_COCK__STATIC,
        EJECT_CLIP__STATIC, INSERT_CLIP__STATIC, SHOTGUN_COCK__STATIC, 255+(95<<8)
    },

    {
        CHAINGUN_WEAPON, 0, 0, 3, 12, 3,
        WEAPON_AUTOMATIC | WEAPON_FIREEVERYTHIRD | WEAPON_AMMOPERSHOT | WEAPON_SPAWNTYPE3 | WEAPON_RESET,
        CHAINGUN__STATIC, 1, SHELL__STATIC, 0, 0, CHAINGUN_FIRE__STATIC, 0, 0,
        EJECT_CLIP__STATIC, INSERT_CLIP__STATIC, SELECT_WEAPON__STATIC, 255+(95<<8)
    },

    {
        RPG_WEAPON, 0, 0, 4, 20, 0,
        0,
        RPG__STATIC, 0, 0, 0, 0, 0, 0, 0,
        EJECT_CLIP__STATIC, INSERT_CLIP__STATIC, SELECT_WEAPON__STATIC, 255+(95<<8)
    },

    {
        HANDBOMB_WEAPON, 0, 30, 6, 19, 12,
        WEAPON_THROWIT,
        HEAVYHBOMB__STATIC, 0, 0, 0, 0, 0, 0,
        0, EJECT_CLIP__STATIC, INSERT_CLIP__STATIC, 0, 0
    },

    {
        SHRINKER_WEAPON, 0, 0, 10, 12, 0,
        WEAPON_GLOWS,
        SHRINKER__STATIC, 0, 0, 0, SHRINKER_FIRE__STATIC, 0, 0, 0,
        EJECT_CLIP__STATIC, INSERT_CLIP__STATIC, SELECT_WEAPON__STATIC, 176+(252<<8)+(120<<16)
    },

    {
        DEVISTATOR_WEAPON, 0, 0, 3, 6, 5,
        WEAPON_FIREEVERYOTHER | WEAPON_AMMOPERSHOT,
        RPG__STATIC, 0, 0, 2, CAT_FIRE__STATIC, 0, 0, 0,
        EJECT_CLIP__STATIC, INSERT_CLIP__STATIC, SELECT_WEAPON__STATIC, 255+(95<<8)
    },

    {
        TRIPBOMB_WEAPON, 0, 16, 3, 16, 7,
        WEAPON_NOVISIBLE | WEAPON_STANDSTILL | WEAPON_CHECKATRELOAD,
        HANDHOLDINGLASER__STATIC, 0, 0, 0, 0, 0, 0,
        0, EJECT_CLIP__STATIC, INSERT_CLIP__STATIC, 0, 0
    },

    {
        FREEZE_WEAPON, 0, 0, 3, 5, 0,
        WEAPON_RESET,
        FREEZEBLAST__STATIC, 0, 0, 0, CAT_FIRE__STATIC, CAT_FIRE__STATIC, 0, 0,
        EJECT_CLIP__STATIC, INSERT_CLIP__STATIC, SELECT_WEAPON__STATIC, 72+(88<<8)+(140<<16)
    },

    {
        HANDREMOTE_WEAPON, 0, 10, 2, 10, 0,
        WEAPON_BOMB_TRIGGER | WEAPON_NOVISIBLE,
        0, 0, 0, 0, 0, 0, 0,
        0, EJECT_CLIP__STATIC, INSERT_CLIP__STATIC, 0, 0
    },

    {
        GROW_WEAPON, 0, 0, 3, 5, 0,
        WEAPON_GLOWS,
        GROWSPARK__STATIC, 0, 0, 0, 0, EXPANDERSHOOT__STATIC, 0, 0,
        EJECT_CLIP__STATIC, INSERT_CLIP__STATIC, SELECT_WEAPON__STATIC, 216+(52<<8)+(20<<16)
    },
};

// KEEPINSYNC with what is contained above
// XXX: ugly
static int32_t G_StaticToDynamicTile(int32_t const tile)
{
    switch (tile)
    {
    case CHAINGUN__STATIC: return CHAINGUN;
    case FREEZEBLAST__STATIC: return FREEZEBLAST;
    case GROWSPARK__STATIC: return GROWSPARK;
    case HANDHOLDINGLASER__STATIC: return HANDHOLDINGLASER;
    case HEAVYHBOMB__STATIC: return HEAVYHBOMB;
    case KNEE__STATIC: return KNEE;
    case RPG__STATIC: return RPG;
    case SHELL__STATIC: return SHELL;
    case SHOTGUNSHELL__STATIC: return SHOTGUNSHELL;
    case SHOTGUN__STATIC: return SHOTGUN;
    case SHOTSPARK1__STATIC: return SHOTSPARK1;
    case SHRINKER__STATIC: return SHRINKER;
    default: return tile;
    }
}

static int32_t G_StaticToDynamicSound(int32_t const sound)
{
    switch (sound)
    {
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
    Bsprintf(aszBuf, "WEAPON%d_" #Membname, Weapidx); \
    Bstrupr(aszBuf); \
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
    for (bssize_t i=0; i<MAX_WEAPONS; i++)
    {
        FINISH_WEAPON_DEFAULT_TILE(i, Shoots);
        FINISH_WEAPON_DEFAULT_TILE(i, Spawn);

        FINISH_WEAPON_DEFAULT_SOUND(i, InitialSound);
        FINISH_WEAPON_DEFAULT_SOUND(i, FireSound);
        FINISH_WEAPON_DEFAULT_SOUND(i, ReloadSound1);
        FINISH_WEAPON_DEFAULT_SOUND(i, Sound2Sound);
        FINISH_WEAPON_DEFAULT_SOUND(i, ReloadSound2);
        FINISH_WEAPON_DEFAULT_SOUND(i, SelectSound);
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
#if !defined LUNATIC
    char aszBuf[64];
#endif

    if (NAM_WW2GI)
    {
        weapondefaults[PISTOL_WEAPON].Clip = 20;
        weapondefaults[PISTOL_WEAPON].Reload = 50;
        weapondefaults[PISTOL_WEAPON].Flags |= WEAPON_HOLSTER_CLEARS_CLIP;

        weapondefaults[SHRINKER_WEAPON].TotalTime = 30;

        weapondefaults[GROW_WEAPON].TotalTime = 30;
        weapondefaults[GROW_WEAPON].SpawnTime = 2;
        weapondefaults[GROW_WEAPON].Spawn = SHELL;
        weapondefaults[GROW_WEAPON].FireSound = 0;
    }

    for (int i=0; i<MAX_WEAPONS; i++)
    {
        ADDWEAPONVAR(i, WorksLike);
        ADDWEAPONVAR(i, Clip);
        ADDWEAPONVAR(i, Reload);
        ADDWEAPONVAR(i, FireDelay);
        ADDWEAPONVAR(i, TotalTime);
        ADDWEAPONVAR(i, HoldDelay);
        ADDWEAPONVAR(i, Flags);
        ADDWEAPONVAR(i, Shoots);
        ADDWEAPONVAR(i, SpawnTime);
        ADDWEAPONVAR(i, Spawn);
        ADDWEAPONVAR(i, ShotsPerBurst);
        ADDWEAPONVAR(i, InitialSound);
        ADDWEAPONVAR(i, FireSound);
        ADDWEAPONVAR(i, Sound2Time);
        ADDWEAPONVAR(i, Sound2Sound);
        ADDWEAPONVAR(i, ReloadSound1);
        ADDWEAPONVAR(i, ReloadSound2);
        ADDWEAPONVAR(i, SelectSound);
        ADDWEAPONVAR(i, FlashColor);
    }
#ifdef LUNATIC
    for (int i=0; i<MAXPLAYERS; i++)
    {
        DukePlayer_t *ps = g_player[i].ps;

        ps->pipebombControl = NAM_WW2GI ? PIPEBOMB_TIMER : PIPEBOMB_REMOTE;
        ps->pipebombLifetime = NAM_GRENADE_LIFETIME;
        ps->pipebombLifetimeVar = NAM_GRENADE_LIFETIME_VAR;

        ps->tripbombControl = TRIPBOMB_TRIPWIRE;
        ps->tripbombLifetime = NAM_GRENADE_LIFETIME;
        ps->tripbombLifetimeVar = NAM_GRENADE_LIFETIME_VAR;
    }
#else
    Gv_NewVar("GRENADE_LIFETIME", NAM_GRENADE_LIFETIME, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Gv_NewVar("GRENADE_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);

    Gv_NewVar("STICKYBOMB_LIFETIME", NAM_GRENADE_LIFETIME, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Gv_NewVar("STICKYBOMB_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);

    Gv_NewVar("TRIPBOMB_CONTROL", TRIPBOMB_TRIPWIRE, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Gv_NewVar("PIPEBOMB_CONTROL", NAM_WW2GI ? PIPEBOMB_TIMER : PIPEBOMB_REMOTE, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);

    Gv_NewVar("RESPAWN_MONSTERS", (intptr_t)&ud.respawn_monsters,GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("RESPAWN_ITEMS",(intptr_t)&ud.respawn_items, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("RESPAWN_INVENTORY",(intptr_t)&ud.respawn_inventory, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("MONSTERS_OFF",(intptr_t)&ud.monsters_off, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("MARKER",(intptr_t)&ud.marker, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("FFIRE",(intptr_t)&ud.ffire, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("LEVEL",(intptr_t)&ud.level_number, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);
    Gv_NewVar("VOLUME",(intptr_t)&ud.volume_number, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);

    Gv_NewVar("COOP",(intptr_t)&ud.coop, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("MULTIMODE",(intptr_t)&ud.multimode, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);

    Gv_NewVar("WEAPON", 0, GAMEVAR_PERPLAYER | GAMEVAR_READONLY | GAMEVAR_SYSTEM);
    Gv_NewVar("WORKSLIKE", 0, GAMEVAR_PERPLAYER | GAMEVAR_READONLY | GAMEVAR_SYSTEM);
    Gv_NewVar("RETURN", 0, GAMEVAR_SYSTEM);
    Gv_NewVar("ZRANGE", 4, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Gv_NewVar("ANGRANGE", 18, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Gv_NewVar("AUTOAIMANGLE", 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Gv_NewVar("LOTAG", 0, GAMEVAR_SYSTEM);
    Gv_NewVar("HITAG", 0, GAMEVAR_SYSTEM);
    Gv_NewVar("TEXTURE", 0, GAMEVAR_SYSTEM);
    Gv_NewVar("THISACTOR", 0, GAMEVAR_READONLY | GAMEVAR_SYSTEM);

    // special vars for struct access
    // KEEPINSYNC gamedef.h: enum QuickStructureAccess_t
    Gv_NewVar("sprite", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    Gv_NewVar("sector", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    Gv_NewVar("wall", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    Gv_NewVar("player", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    Gv_NewVar("actorvar", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    Gv_NewVar("playervar", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    Gv_NewVar("tspr", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    Gv_NewVar("projectile", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    Gv_NewVar("thisprojectile", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    Gv_NewVar("userdef", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    Gv_NewVar("input", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    Gv_NewVar("tiledata", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    Gv_NewVar("paldata", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);

    Gv_NewVar("myconnectindex", (intptr_t)&myconnectindex, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("screenpeek", (intptr_t)&screenpeek, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("currentweapon",(intptr_t)&hudweap.cur, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("gs",(intptr_t)&hudweap.shade, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("looking_arc",(intptr_t)&hudweap.lookhoriz, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("gun_pos",(intptr_t)&hudweap.gunposy, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("weapon_xoffset",(intptr_t)&hudweap.gunposx, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("weaponcount",(intptr_t)&hudweap.count, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("looking_angSR1",(intptr_t)&hudweap.lookhalfang, GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
    Gv_NewVar("xdim",(intptr_t)&xdim, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("ydim",(intptr_t)&ydim, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("windowx1",(intptr_t)&windowxy1.x, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("windowx2",(intptr_t)&windowxy2.x, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("windowy1",(intptr_t)&windowxy1.y, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("windowy2",(intptr_t)&windowxy2.y, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("totalclock",(intptr_t)&totalclock, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY);
    Gv_NewVar("lastvisinc",(intptr_t)&lastvisinc, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("numsectors",(intptr_t)&numsectors, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR | GAMEVAR_READONLY);

    Gv_NewVar("current_menu",(intptr_t)&g_currentMenu, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);
    Gv_NewVar("numplayers",(intptr_t)&numplayers, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);
    Gv_NewVar("viewingrange",(intptr_t)&viewingrange, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);
    Gv_NewVar("yxaspect",(intptr_t)&yxaspect, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);
    Gv_NewVar("gravitationalconstant",(intptr_t)&g_spriteGravity, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("gametype_flags",(intptr_t)&g_gametypeFlags[ud.coop], GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("framerate",(intptr_t)&g_frameRate, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);
    Gv_NewVar("CLIPMASK0", CLIPMASK0, GAMEVAR_SYSTEM|GAMEVAR_READONLY);
    Gv_NewVar("CLIPMASK1", CLIPMASK1, GAMEVAR_SYSTEM|GAMEVAR_READONLY);

    Gv_NewVar("camerax",(intptr_t)&ud.camerapos.x, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("cameray",(intptr_t)&ud.camerapos.y, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("cameraz",(intptr_t)&ud.camerapos.z, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("cameraang",(intptr_t)&ud.cameraang, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR);
    Gv_NewVar("camerahoriz",(intptr_t)&ud.camerahoriz, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR);
    Gv_NewVar("camerasect",(intptr_t)&ud.camerasect, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR);
    Gv_NewVar("cameradist",(intptr_t)&g_cameraDistance, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("cameraclock",(intptr_t)&g_cameraClock, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);

    Gv_NewVar("display_mirror",(intptr_t)&display_mirror, GAMEVAR_SYSTEM | GAMEVAR_CHARPTR);
    Gv_NewVar("randomseed",(intptr_t)&randomseed, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);

    Gv_NewVar("NUMWALLS",(intptr_t)&numwalls, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR | GAMEVAR_READONLY);
    Gv_NewVar("NUMSECTORS",(intptr_t)&numsectors, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR | GAMEVAR_READONLY);
    Gv_NewVar("Numsprites",(intptr_t)&Numsprites, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);

    Gv_NewVar("lastsavepos",(intptr_t)&g_lastSaveSlot, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
# ifdef USE_OPENGL
    Gv_NewVar("rendmode",(intptr_t)&rendmode, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM);
# else
    Gv_NewVar("rendmode", 0, GAMEVAR_READONLY | GAMEVAR_SYSTEM);
# endif

    // SYSTEM_GAMEARRAY
    Gv_NewArray("tilesizx", (void *)&tilesiz[0].x, MAXTILES, GAMEARRAY_STRIDE2|GAMEARRAY_READONLY|GAMEARRAY_OFINT);
    Gv_NewArray("tilesizy", (void *)&tilesiz[0].y, MAXTILES, GAMEARRAY_STRIDE2|GAMEARRAY_READONLY|GAMEARRAY_OFINT);
#endif
}

#undef ADDWEAPONVAR

void Gv_Init(void)
{
#if !defined LUNATIC
    // already initialized
    if (aGameVars[0].flags)
        return;

    Gv_Clear();
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
            initprintf("ERROR: NULL weapon!  WTF?! %s\n", aszBuf);
            // Bexit(0);
            G_Shutdown();
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
        Bsprintf(aszBuf, "WEAPON%d_RELOADSOUND1", i);
        aplWeaponReloadSound1[i] = Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf, "WEAPON%d_RELOADSOUND2", i);
        aplWeaponReloadSound2[i] = Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf, "WEAPON%d_SELECTSOUND", i);
        aplWeaponSelectSound[i] = Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf, "WEAPON%d_FLASHCOLOR", i);
        aplWeaponFlashColor[i] = Gv_GetVarDataPtr(aszBuf);
    }
}

void Gv_RefreshPointers(void)
{
    aGameVars[Gv_GetVarIndex("RESPAWN_MONSTERS")].global  = (intptr_t)&ud.respawn_monsters;
    aGameVars[Gv_GetVarIndex("RESPAWN_ITEMS")].global     = (intptr_t)&ud.respawn_items;
    aGameVars[Gv_GetVarIndex("RESPAWN_INVENTORY")].global = (intptr_t)&ud.respawn_inventory;
    aGameVars[Gv_GetVarIndex("MONSTERS_OFF")].global      = (intptr_t)&ud.monsters_off;
    aGameVars[Gv_GetVarIndex("MARKER")].global            = (intptr_t)&ud.marker;
    aGameVars[Gv_GetVarIndex("FFIRE")].global             = (intptr_t)&ud.ffire;
    aGameVars[Gv_GetVarIndex("LEVEL")].global             = (intptr_t)&ud.level_number;
    aGameVars[Gv_GetVarIndex("VOLUME")].global            = (intptr_t)&ud.volume_number;

    aGameVars[Gv_GetVarIndex("COOP")].global      = (intptr_t)&ud.coop;
    aGameVars[Gv_GetVarIndex("MULTIMODE")].global = (intptr_t)&ud.multimode;

    aGameVars[Gv_GetVarIndex("myconnectindex")].global = (intptr_t)&myconnectindex;
    aGameVars[Gv_GetVarIndex("screenpeek")].global     = (intptr_t)&screenpeek;
    aGameVars[Gv_GetVarIndex("currentweapon")].global  = (intptr_t)&hudweap.cur;
    aGameVars[Gv_GetVarIndex("gs")].global             = (intptr_t)&hudweap.shade;
    aGameVars[Gv_GetVarIndex("looking_arc")].global    = (intptr_t)&hudweap.lookhoriz;
    aGameVars[Gv_GetVarIndex("gun_pos")].global        = (intptr_t)&hudweap.gunposy;
    aGameVars[Gv_GetVarIndex("weapon_xoffset")].global = (intptr_t)&hudweap.gunposx;
    aGameVars[Gv_GetVarIndex("weaponcount")].global    = (intptr_t)&hudweap.count;
    aGameVars[Gv_GetVarIndex("looking_angSR1")].global = (intptr_t)&hudweap.lookhalfang;
    aGameVars[Gv_GetVarIndex("xdim")].global           = (intptr_t)&xdim;
    aGameVars[Gv_GetVarIndex("ydim")].global           = (intptr_t)&ydim;
    aGameVars[Gv_GetVarIndex("windowx1")].global       = (intptr_t)&windowxy1.x;
    aGameVars[Gv_GetVarIndex("windowx2")].global       = (intptr_t)&windowxy2.x;
    aGameVars[Gv_GetVarIndex("windowy1")].global       = (intptr_t)&windowxy1.y;
    aGameVars[Gv_GetVarIndex("windowy2")].global       = (intptr_t)&windowxy2.y;
    aGameVars[Gv_GetVarIndex("totalclock")].global     = (intptr_t)&totalclock;
    aGameVars[Gv_GetVarIndex("lastvisinc")].global     = (intptr_t)&lastvisinc;
    aGameVars[Gv_GetVarIndex("numsectors")].global     = (intptr_t)&numsectors;
    aGameVars[Gv_GetVarIndex("numplayers")].global     = (intptr_t)&numplayers;
    aGameVars[Gv_GetVarIndex("current_menu")].global   = (intptr_t)&g_currentMenu;
    aGameVars[Gv_GetVarIndex("viewingrange")].global   = (intptr_t)&viewingrange;
    aGameVars[Gv_GetVarIndex("yxaspect")].global       = (intptr_t)&yxaspect;
    aGameVars[Gv_GetVarIndex("gametype_flags")].global = (intptr_t)&g_gametypeFlags[ud.coop];
    aGameVars[Gv_GetVarIndex("framerate")].global      = (intptr_t)&g_frameRate;

    aGameVars[Gv_GetVarIndex("gravitationalconstant")].global = (intptr_t)&g_spriteGravity;

    aGameVars[Gv_GetVarIndex("camerax")].global     = (intptr_t)&ud.camerapos.x;
    aGameVars[Gv_GetVarIndex("cameray")].global     = (intptr_t)&ud.camerapos.y;
    aGameVars[Gv_GetVarIndex("cameraz")].global     = (intptr_t)&ud.camerapos.z;
    aGameVars[Gv_GetVarIndex("cameraang")].global   = (intptr_t)&ud.cameraang;
    aGameVars[Gv_GetVarIndex("camerahoriz")].global = (intptr_t)&ud.camerahoriz;
    aGameVars[Gv_GetVarIndex("camerasect")].global  = (intptr_t)&ud.camerasect;
    aGameVars[Gv_GetVarIndex("cameradist")].global  = (intptr_t)&g_cameraDistance;
    aGameVars[Gv_GetVarIndex("cameraclock")].global = (intptr_t)&g_cameraClock;

    aGameVars[Gv_GetVarIndex("display_mirror")].global = (intptr_t)&display_mirror;
    aGameVars[Gv_GetVarIndex("randomseed")].global = (intptr_t)&randomseed;

    aGameVars[Gv_GetVarIndex("NUMWALLS")].global = (intptr_t)&numwalls;
    aGameVars[Gv_GetVarIndex("NUMSECTORS")].global = (intptr_t)&numsectors;
    aGameVars[Gv_GetVarIndex("Numsprites")].global = (intptr_t)&Numsprites;

    aGameVars[Gv_GetVarIndex("lastsavepos")].global = (intptr_t)&g_lastSaveSlot;
# ifdef USE_OPENGL
    aGameVars[Gv_GetVarIndex("rendmode")].global = (intptr_t)&rendmode;
# endif
}
#endif
