//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#include "duke3d.h"
#include "gamevars.h"
#include "gamedef.h"
#include "osd.h"

#define _gamevars_c_
#include "gamestructures.c"

extern int32_t OSD_errors;

void Gv_RefreshPointers(void);
extern void G_FreeMapState(int32_t mapnum);
static void Gv_Free(void) /* called from Gv_ReadSave() and Gv_ResetVars() */
{
    // call this function as many times as needed.
    int32_t i=(MAXGAMEVARS-1);
    //  AddLog("Gv_Free");
    for (; i>=0; i--)
    {
        if ((aGameVars[i].dwFlags & GAMEVAR_USER_MASK) && aGameVars[i].val.plValues)
        {
            Bfree(aGameVars[i].val.plValues);
            aGameVars[i].val.plValues=NULL;
        }

        aGameVars[i].dwFlags |= GAMEVAR_RESET;

        if (i >= MAXGAMEARRAYS)
            continue;

        if (aGameArrays[i].plValues)
            Bfree(aGameArrays[i].plValues);

        aGameArrays[i].plValues=NULL;
        aGameArrays[i].bReset=1;
    }
    g_gameVarCount=g_gameArrayCount=0;
    hash_init(&h_gamevars);
    hash_init(&h_arrays);
    return;
}

static void Gv_Clear(void)
{
    // only call this function ONCE...
    int32_t i=(MAXGAMEVARS-1);

    //AddLog("Gv_Clear");

    for (; i>=0; i--)
    {
        if (aGameVars[i].szLabel)
            Bfree(aGameVars[i].szLabel);
        aGameVars[i].szLabel=NULL;
        aGameVars[i].dwFlags=0;

        if ((aGameVars[i].dwFlags & GAMEVAR_USER_MASK) && aGameVars[i].val.plValues)
        {
            Bfree(aGameVars[i].val.plValues);
            aGameVars[i].val.plValues=NULL;
        }
        aGameVars[i].val.lValue=0;
        aGameVars[i].dwFlags |= GAMEVAR_RESET;
        if (i >= MAXGAMEARRAYS)
            continue;
        if (aGameArrays[i].szLabel)
            Bfree(aGameArrays[i].szLabel);
        aGameArrays[i].szLabel=NULL;

        if (aGameArrays[i].plValues)
            Bfree(aGameArrays[i].plValues);
        aGameArrays[i].plValues=NULL;
        aGameArrays[i].bReset=1;
    }
    g_gameVarCount=g_gameArrayCount=0;
    hash_init(&h_gamevars);
    hash_init(&h_arrays);
    return;
}

int32_t Gv_ReadSave(int32_t fil, int32_t newbehav)
{
    int32_t i, j;
    intptr_t l;
    char savedstate[MAXVOLUMES*MAXLEVELS];
    char tbuf[12];

    if (newbehav)
    {
        if (kread(fil, tbuf, 12)!=12) goto corrupt;
        if (Bmemcmp(tbuf, "BEG: EDuke32", 12)) { OSD_Printf("BEG ERR\n"); return 2; }
    }

    Bmemset(&savedstate,0,sizeof(savedstate));

    //     AddLog("Reading gamevars from savegame");

    Gv_Free(); // nuke 'em from orbit, it's the only way to be sure...

    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);

    if (kdfread(&g_gameVarCount,sizeof(g_gameVarCount),1,fil) != 1) goto corrupt;
    for (i=0; i<g_gameVarCount; i++)
    {
        if (kdfread(&(aGameVars[i]),sizeof(gamevar_t),1,fil) != 1) goto corrupt;
        aGameVars[i].szLabel=Bcalloc(MAXVARLABEL,sizeof(uint8_t));
        if (kdfread(aGameVars[i].szLabel,sizeof(uint8_t) * MAXVARLABEL, 1, fil) != 1) goto corrupt;
        hash_add(&h_gamevars, aGameVars[i].szLabel,i, 1);

        if (aGameVars[i].dwFlags & GAMEVAR_PERPLAYER)
        {
            aGameVars[i].val.plValues=Bcalloc(MAXPLAYERS,sizeof(intptr_t));
            if (kdfread(aGameVars[i].val.plValues,sizeof(intptr_t) * MAXPLAYERS, 1, fil) != 1) goto corrupt;
        }
        else if (aGameVars[i].dwFlags & GAMEVAR_PERACTOR)
        {
            aGameVars[i].val.plValues=Bcalloc(MAXSPRITES,sizeof(intptr_t));
            if (kdfread(&aGameVars[i].val.plValues[0],sizeof(intptr_t), MAXSPRITES, fil) != MAXSPRITES) goto corrupt;
        }
    }
    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);

    Gv_InitWeaponPointers();

    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);
    Gv_RefreshPointers();

    if (kdfread(&g_gameArrayCount,sizeof(g_gameArrayCount),1,fil) != 1) goto corrupt;
    for (i=0; i<g_gameArrayCount; i++)
    {
        if (kdfread(&(aGameArrays[i]),sizeof(gamearray_t),1,fil) != 1) goto corrupt;
        aGameArrays[i].szLabel=Bcalloc(MAXARRAYLABEL,sizeof(uint8_t));
        if (kdfread(aGameArrays[i].szLabel,sizeof(uint8_t) * MAXARRAYLABEL, 1, fil) != 1) goto corrupt;
        hash_add(&h_arrays, aGameArrays[i].szLabel, i, 1);

        aGameArrays[i].plValues=Bcalloc(aGameArrays[i].size,sizeof(intptr_t));
        if (kdfread(aGameArrays[i].plValues,sizeof(intptr_t) * aGameArrays[i].size, 1, fil) < 1) goto corrupt;
    }

    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);
    if (kdfread(apScriptGameEvent,sizeof(apScriptGameEvent),1,fil) != 1) goto corrupt;
    for (i=0; i<MAXGAMEEVENTS; i++)
        if (apScriptGameEvent[i])
        {
            l = (intptr_t)apScriptGameEvent[i]+(intptr_t)&script[0];
            apScriptGameEvent[i] = (intptr_t *)l;
        }

    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);

    if (kdfread(&savedstate[0],sizeof(savedstate),1,fil) != 1) goto corrupt;

    for (i=0; i<(MAXVOLUMES*MAXLEVELS); i++)
    {
        if (savedstate[i])
        {
            if (MapInfo[i].savedstate == NULL)
                MapInfo[i].savedstate = Bcalloc(1,sizeof(mapstate_t));
            if (kdfread(MapInfo[i].savedstate,sizeof(mapstate_t),1,fil) != sizeof(mapstate_t)) goto corrupt;
            for (j=0; j<g_gameVarCount; j++)
            {
                if (aGameVars[j].dwFlags & GAMEVAR_NORESET) continue;
                if (aGameVars[j].dwFlags & GAMEVAR_PERPLAYER)
                {
//                    if (!MapInfo[i].savedstate->vars[j])
                    MapInfo[i].savedstate->vars[j] = Bcalloc(MAXPLAYERS,sizeof(intptr_t));
                    if (kdfread(&MapInfo[i].savedstate->vars[j][0],sizeof(intptr_t) * MAXPLAYERS, 1, fil) != 1) goto corrupt;
                }
                else if (aGameVars[j].dwFlags & GAMEVAR_PERACTOR)
                {
//                    if (!MapInfo[i].savedstate->vars[j])
                    MapInfo[i].savedstate->vars[j] = Bcalloc(MAXSPRITES,sizeof(intptr_t));
                    if (kdfread(&MapInfo[i].savedstate->vars[j][0],sizeof(intptr_t), MAXSPRITES, fil) != MAXSPRITES) goto corrupt;
                }
            }
        }
        else if (MapInfo[i].savedstate)
        {
            G_FreeMapState(i);
        }
    }

    if (!newbehav)
    {
        if (kdfread(&l,sizeof(l),1,fil) != 1) goto corrupt;
        if (kdfread(g_szBuf,l,1,fil) != 1) goto corrupt;
        g_szBuf[l]=0;
        OSD_Printf("%s\n",g_szBuf);
    }
    else
    {
        if (kread(fil, tbuf, 12)!=12) goto corrupt;
        if (Bmemcmp(tbuf, "EOF: EDuke32", 12)) { OSD_Printf("EOF ERR\n"); return 2; }
    }

#if 0
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
#endif
    return(0);
corrupt:
    return(1);
}

void Gv_WriteSave(FILE *fil, int32_t newbehav)
{
    int32_t i, j;
    intptr_t l;
    char savedstate[MAXVOLUMES*MAXLEVELS];

    Bmemset(&savedstate,0,sizeof(savedstate));

    //   AddLog("Saving Game Vars to File");
    if (newbehav)
        fwrite("BEG: EDuke32", 12, 1, fil);

    dfwrite(&g_gameVarCount,sizeof(g_gameVarCount),1,fil);

    for (i=0; i<g_gameVarCount; i++)
    {
        dfwrite(&(aGameVars[i]),sizeof(gamevar_t),1,fil);
        dfwrite(aGameVars[i].szLabel,sizeof(uint8_t) * MAXVARLABEL, 1, fil);

        if (aGameVars[i].dwFlags & GAMEVAR_PERPLAYER)
        {
            //Bsprintf(g_szBuf,"Writing value array for %s (%d)",aGameVars[i].szLabel,sizeof(int32_t) * MAXPLAYERS);
            //AddLog(g_szBuf);
            dfwrite(aGameVars[i].val.plValues,sizeof(intptr_t) * MAXPLAYERS, 1, fil);
        }
        else if (aGameVars[i].dwFlags & GAMEVAR_PERACTOR)
        {
            //Bsprintf(g_szBuf,"Writing value array for %s (%d)",aGameVars[i].szLabel,sizeof(int32_t) * MAXSPRITES);
            //AddLog(g_szBuf);
            dfwrite(&aGameVars[i].val.plValues[0],sizeof(intptr_t), MAXSPRITES, fil);
        }
    }

    dfwrite(&g_gameArrayCount,sizeof(g_gameArrayCount),1,fil);

    for (i=0; i<g_gameArrayCount; i++)
    {
        dfwrite(&(aGameArrays[i]),sizeof(gamearray_t),1,fil);
        dfwrite(aGameArrays[i].szLabel,sizeof(uint8_t) * MAXARRAYLABEL, 1, fil);
        dfwrite(aGameArrays[i].plValues,sizeof(intptr_t) * aGameArrays[i].size, 1, fil);
    }

    for (i=0; i<MAXGAMEEVENTS; i++)
        if (apScriptGameEvent[i])
        {
            l = (intptr_t)apScriptGameEvent[i]-(intptr_t)&script[0];
            apScriptGameEvent[i] = (intptr_t *)l;
        }
    dfwrite(apScriptGameEvent,sizeof(apScriptGameEvent),1,fil);
    for (i=0; i<MAXGAMEEVENTS; i++)
        if (apScriptGameEvent[i])
        {
            l = (intptr_t)apScriptGameEvent[i]+(intptr_t)&script[0];
            apScriptGameEvent[i] = (intptr_t *)l;
        }

    for (i=0; i<(MAXVOLUMES*MAXLEVELS); i++)
        if (MapInfo[i].savedstate != NULL)
            savedstate[i] = 1;

    dfwrite(&savedstate[0],sizeof(savedstate),1,fil);

    for (i=0; i<(MAXVOLUMES*MAXLEVELS); i++)
        if (MapInfo[i].savedstate)
        {
            dfwrite(MapInfo[i].savedstate,sizeof(mapstate_t),1,fil);
            for (j=0; j<g_gameVarCount; j++)
            {
                if (aGameVars[j].dwFlags & GAMEVAR_NORESET) continue;
                if (aGameVars[j].dwFlags & GAMEVAR_PERPLAYER)
                {
                    dfwrite(&MapInfo[i].savedstate->vars[j][0],sizeof(intptr_t) * MAXPLAYERS, 1, fil);
                }
                else if (aGameVars[j].dwFlags & GAMEVAR_PERACTOR)
                {
                    dfwrite(&MapInfo[i].savedstate->vars[j][0],sizeof(intptr_t), MAXSPRITES, fil);
                }
            }
        }

    if (!newbehav)
    {
        Bsprintf(g_szBuf,"EOF: EDuke32");
        l=Bstrlen(g_szBuf);
        dfwrite(&l,sizeof(l),1,fil);
        dfwrite(g_szBuf,l,1,fil);
    }
    else
        fwrite("EOF: EDuke32", 12, 1, fil);
}

void Gv_DumpValues(void)
{
    int32_t i;

    OSD_Printf("// Current Game Definitions\n\n");

    for (i=0; i<g_gameVarCount; i++)
    {
        if (aGameVars[i].dwFlags & (GAMEVAR_SECRET))
            continue; // do nothing...

        OSD_Printf("gamevar %s ",aGameVars[i].szLabel);

        if (aGameVars[i].dwFlags & (GAMEVAR_INTPTR))
            OSD_Printf("%d",*((int32_t *)aGameVars[i].val.lValue));
        else if (aGameVars[i].dwFlags & (GAMEVAR_SHORTPTR))
            OSD_Printf("%d",*((int16_t *)aGameVars[i].val.lValue));
        else if (aGameVars[i].dwFlags & (GAMEVAR_CHARPTR))
            OSD_Printf("%d",*((char *)aGameVars[i].val.lValue));
        else
            OSD_Printf("%" PRIdPTR "",aGameVars[i].val.lValue);

        if (aGameVars[i].dwFlags & (GAMEVAR_PERPLAYER))
            OSD_Printf(" GAMEVAR_PERPLAYER");
        else if (aGameVars[i].dwFlags & (GAMEVAR_PERACTOR))
            OSD_Printf(" GAMEVAR_PERACTOR");
        else
            OSD_Printf(" %d",aGameVars[i].dwFlags/* & (GAMEVAR_USER_MASK)*/);

        OSD_Printf(" // ");
        if (aGameVars[i].dwFlags & (GAMEVAR_SYSTEM))
            OSD_Printf(" (system)");
        if (aGameVars[i].dwFlags & (GAMEVAR_INTPTR|GAMEVAR_SHORTPTR|GAMEVAR_CHARPTR))
            OSD_Printf(" (pointer)");
        if (aGameVars[i].dwFlags & (GAMEVAR_READONLY))
            OSD_Printf(" (read only)");
        if (aGameVars[i].dwFlags & (GAMEVAR_SPECIAL))
            OSD_Printf(" (special)");
        OSD_Printf("\n");
    }
    OSD_Printf("\n// end of game definitions\n");
}

void Gv_ResetVars(void) /* this is called during a new game and nowhere else */
{
    int32_t i;

    Gv_Free();
    OSD_errors=0;

    for (i=0; i<MAXGAMEVARS; i++)
    {
        if (aGameVars[i].szLabel != NULL)
            Gv_NewVar(aGameVars[i].szLabel,
                      aGameVars[i].dwFlags & GAMEVAR_NODEFAULT ? aGameVars[i].val.lValue : aGameVars[i].lDefault,
                      aGameVars[i].dwFlags);
    }

    for (i=0; i<MAXGAMEARRAYS; i++)
    {
        if (aGameArrays[i].szLabel != NULL && aGameArrays[i].bReset)
            Gv_NewArray(aGameArrays[i].szLabel,aGameArrays[i].size);
    }
}

int32_t Gv_NewArray(const char *pszLabel, int32_t asize)
{
    int32_t i;

    if (g_gameArrayCount >= MAXGAMEARRAYS)
    {
        g_numCompilerErrors++;
        C_ReportError(-1);
        initprintf("%s:%d: error: too many arrays!\n",g_szScriptFileName,g_lineNumber);
        return 0;
    }

    if (Bstrlen(pszLabel) > (MAXARRAYLABEL-1))
    {
        g_numCompilerErrors++;
        C_ReportError(-1);
        initprintf("%s:%d: error: array name `%s' exceeds limit of %d characters.\n",g_szScriptFileName,g_lineNumber,pszLabel, MAXARRAYLABEL);
        return 0;
    }
    i = hash_find(&h_arrays,pszLabel);
    if (i >=0 && !aGameArrays[i].bReset)
    {
        // found it it's a duplicate in error
        g_numCompilerWarnings++;
        C_ReportError(WARNING_DUPLICATEDEFINITION);
        return 0;
    }

    i = g_gameArrayCount;

    if (aGameArrays[i].szLabel == NULL)
        aGameArrays[i].szLabel=Bcalloc(MAXVARLABEL,sizeof(uint8_t));
    if (aGameArrays[i].szLabel != pszLabel)
        Bstrcpy(aGameArrays[i].szLabel,pszLabel);
    aGameArrays[i].plValues=Bcalloc(asize,sizeof(intptr_t));
    aGameArrays[i].size=asize;
    aGameArrays[i].bReset=0;
    g_gameArrayCount++;
    hash_add(&h_arrays, aGameArrays[i].szLabel, i, 1);
    return 1;
}

int32_t Gv_NewVar(const char *pszLabel, int32_t lValue, uint32_t dwFlags)
{
    int32_t i, j;

    //Bsprintf(g_szBuf,"Gv_NewVar(%s, %d, %X)",pszLabel, lValue, dwFlags);
    //AddLog(g_szBuf);

    if (g_gameVarCount >= MAXGAMEVARS)
    {
        g_numCompilerErrors++;
        C_ReportError(-1);
        initprintf("%s:%d: error: too many gamevars!\n",g_szScriptFileName,g_lineNumber);
        return 0;
    }

    if (Bstrlen(pszLabel) > (MAXVARLABEL-1))
    {
        g_numCompilerErrors++;
        C_ReportError(-1);
        initprintf("%s:%d: error: variable name `%s' exceeds limit of %d characters.\n",g_szScriptFileName,g_lineNumber,pszLabel, MAXVARLABEL);
        return 0;
    }

    i = hash_find(&h_gamevars,pszLabel);

    if (i >= 0 && !(aGameVars[i].dwFlags & GAMEVAR_RESET))
    {
        // found it...
        if (aGameVars[i].dwFlags & (GAMEVAR_INTPTR|GAMEVAR_SHORTPTR|GAMEVAR_CHARPTR))
        {
            //  			   warning++;
            //  			   initprintf("%s:%d: warning: Internal gamevar '%s' cannot be redefined.\n",g_szScriptFileName,g_lineNumber,label+(g_numLabels<<6));
            C_ReportError(-1);
            initprintf("%s:%d: warning: cannot redefine internal gamevar `%s'.\n",g_szScriptFileName,g_lineNumber,label+(g_numLabels<<6));
            return 0;
        }
        else if (!(aGameVars[i].dwFlags & GAMEVAR_DEFAULT) && !(aGameVars[i].dwFlags & GAMEVAR_SYSTEM))
        {
            // it's a duplicate in error
            g_numCompilerWarnings++;
            C_ReportError(WARNING_DUPLICATEDEFINITION);
            return 0;
        }
    }

    if (i == -1)
        i = g_gameVarCount;

    // Set values
    if ((aGameVars[i].dwFlags & GAMEVAR_SYSTEM) == 0)
    {
        if (aGameVars[i].szLabel == NULL)
            aGameVars[i].szLabel=Bcalloc(MAXVARLABEL,sizeof(uint8_t));
        if (aGameVars[i].szLabel != pszLabel)
            Bstrcpy(aGameVars[i].szLabel,pszLabel);
        aGameVars[i].dwFlags=dwFlags;

        if (aGameVars[i].dwFlags & GAMEVAR_USER_MASK)
        {
            // only free if not system
            if (aGameVars[i].val.plValues)
                Bfree(aGameVars[i].val.plValues);
            aGameVars[i].val.plValues=NULL;
        }
    }

    // if existing is system, they only get to change default value....
    aGameVars[i].lDefault = lValue;
    aGameVars[i].dwFlags &= ~GAMEVAR_RESET;

    if (i == g_gameVarCount)
    {
        // we're adding a new one.
        hash_add(&h_gamevars, aGameVars[i].szLabel, g_gameVarCount++, 0);
    }

    if (aGameVars[i].dwFlags & GAMEVAR_PERPLAYER)
    {
        if (!aGameVars[i].val.plValues)
            aGameVars[i].val.plValues=Bcalloc(MAXPLAYERS,sizeof(intptr_t));
        for (j=MAXPLAYERS-1; j>=0; j--)
            aGameVars[i].val.plValues[j]=lValue;
    }
    else if (aGameVars[i].dwFlags & GAMEVAR_PERACTOR)
    {
        if (!aGameVars[i].val.plValues)
            aGameVars[i].val.plValues=Bcalloc(MAXSPRITES,sizeof(intptr_t));
        for (j=MAXSPRITES-1; j>=0; j--)
            aGameVars[i].val.plValues[j]=lValue;
    }
    else aGameVars[i].val.lValue = lValue;

    return 1;
}

void __fastcall A_ResetVars(register int32_t iActor)
{
    register int32_t i=(MAXGAMEVARS-1);
    do
    {
        if ((aGameVars[i].dwFlags & (GAMEVAR_PERACTOR|GAMEVAR_NODEFAULT)) == GAMEVAR_PERACTOR)
            aGameVars[i].val.plValues[iActor]=aGameVars[i].lDefault;
    }
    while (i--);
}

static int32_t Gv_GetVarIndex(const char *szGameLabel)
{
    int32_t i = hash_find(&h_gamevars,szGameLabel);
    if (i == -1)
    {
        OSD_Printf(OSD_ERROR "Gv_GetVarDataPtr(): INTERNAL ERROR: couldn't find gamevar %s!\n",szGameLabel);
        return 0;
    }
    return i;
}

int32_t __fastcall Gv_GetVar(register int32_t id, register int32_t iActor, register int32_t iPlayer)
{
    if (id == g_iThisActorID)
        return iActor;

    if (id == MAXGAMEVARS)
        return(*insptr++);

    {
        register intptr_t negateResult = id&(MAXGAMEVARS<<1);

        if (id >= g_gameVarCount)
        {
            if (id&(MAXGAMEVARS<<2)) // array
            {
                register int32_t index=Gv_GetVar(*insptr++,iActor,iPlayer);

                id &= (MAXGAMEVARS-1);// ~((MAXGAMEVARS<<2)|(MAXGAMEVARS<<1));

                if (index >= aGameArrays[id].size || index < 0)
                {
                    OSD_Printf(CON_ERROR "Gv_GetVar(): invalid array index (%s[%d])\n",g_errorLineNum,keyw[g_tw],aGameArrays[id].szLabel,index);
                    return -1;
                }

                return ((aGameArrays[id].plValues[index] ^ -negateResult) + negateResult);
            }

            if (id&(MAXGAMEVARS<<3)) // struct shortcut vars
            {
                register int32_t index=Gv_GetVar(*insptr++, iActor, iPlayer);

                switch ((id&(MAXGAMEVARS-1)) - g_iSpriteVarID)
                {
                case 0: //if (id == g_iSpriteVarID)
                {
                    int32_t parm2 = 0, label = *insptr++;

                    /*OSD_Printf("%d %d %d\n",__LINE__,index,label);*/
                    if (ActorLabels[label].flags & LABEL_HASPARM2)
                        parm2 = Gv_GetVar(*insptr++, iActor, iPlayer);

                    return ((VM_AccessSpriteX(index, label, parm2) ^ -negateResult) + negateResult);
                }
                case 3: //else if (id == g_iPlayerVarID)
                {
                    int32_t parm2 = 0, label = *insptr++;

                    if (PlayerLabels[label].flags & LABEL_HASPARM2)
                        parm2 = Gv_GetVar(*insptr++, iActor, iPlayer);

                    if (index == vm.g_i) index = vm.g_p;
                    return ((VM_AccessPlayerX(index, label, parm2) ^ -negateResult) + negateResult);
                }
                case 4: //else if (id == g_iActorVarID)
                    return ((Gv_GetVar(*insptr++, index, iPlayer) ^ -negateResult) + negateResult);
                case 1: //else if (id == g_iSectorVarID)
                    if (index == vm.g_i) index = sprite[vm.g_i].sectnum;
                    return ((VM_AccessSectorX(index, *insptr++) ^ -negateResult) + negateResult);
                case 2: //else if (id == g_iWallVarID)
                    return ((VM_AccessWallX(index, *insptr++) ^ -negateResult) + negateResult);
                default:
                    OSD_Printf(CON_ERROR "Gv_GetVar(): WTF?\n",g_errorLineNum,keyw[g_tw]);
                    return -1;
                }
            }

            id &= (MAXGAMEVARS-1);

            if (!negateResult)
            {
                OSD_Printf(CON_ERROR "Gv_GetVar(): invalid gamevar ID (%d)\n",g_errorLineNum,keyw[g_tw],id);
                return -1;
            }
        }

        switch (aGameVars[id].dwFlags &
                (GAMEVAR_USER_MASK|GAMEVAR_INTPTR|GAMEVAR_SHORTPTR|GAMEVAR_CHARPTR))
        {
        default:
            return ((aGameVars[id].val.lValue ^ -negateResult) + negateResult);
        case GAMEVAR_PERPLAYER:
            if (iPlayer < 0 || iPlayer >= MAXPLAYERS) goto bad_id;
            return ((aGameVars[id].val.plValues[iPlayer] ^ -negateResult) + negateResult);
        case GAMEVAR_PERACTOR:
            if (iActor < 0 || iActor >= MAXSPRITES) goto bad_id;
            return ((aGameVars[id].val.plValues[iActor] ^ -negateResult) + negateResult);
        case GAMEVAR_INTPTR:
            return (((*((int32_t *)aGameVars[id].val.lValue)) ^ -negateResult) + negateResult);
        case GAMEVAR_SHORTPTR:
            return (((*((int16_t *)aGameVars[id].val.lValue)) ^ -negateResult) + negateResult);
        case GAMEVAR_CHARPTR:
            return (((*((char *)aGameVars[id].val.lValue)) ^ -negateResult) + negateResult);
        }
    }
bad_id:
    OSD_Printf(CON_ERROR "Gv_GetVar(): invalid sprite/player ID %d/%d\n",g_errorLineNum,keyw[g_tw],iActor,iPlayer);
    return -1;
}

void __fastcall Gv_SetVar(register int32_t id, register int32_t lValue, register int32_t iActor, register int32_t iPlayer)
{
    if (id<0 || id >= g_gameVarCount) goto badvarid;

    //Bsprintf(g_szBuf,"SGVI: %d ('%s') to %d for %d %d",id,aGameVars[id].szLabel,lValue,iActor,iPlayer);
    //AddLog(g_szBuf);

    switch (aGameVars[id].dwFlags &
            (GAMEVAR_USER_MASK|GAMEVAR_INTPTR|GAMEVAR_SHORTPTR|GAMEVAR_CHARPTR))
    {
    default:
        aGameVars[id].val.lValue=lValue;
        return;
    case GAMEVAR_PERPLAYER:
        if (iPlayer < 0 || iPlayer > MAXPLAYERS-1) goto badplayer;
        // for the current player
        aGameVars[id].val.plValues[iPlayer]=lValue;
        return;
    case GAMEVAR_PERACTOR:
        if (iActor < 0 || iActor > MAXSPRITES-1) goto badactor;
        aGameVars[id].val.plValues[iActor]=lValue;
        return;
    case GAMEVAR_INTPTR:
        *((int32_t *)aGameVars[id].val.lValue)=(int32_t)lValue;
        return;
    case GAMEVAR_SHORTPTR:
        *((int16_t *)aGameVars[id].val.lValue)=(int16_t)lValue;
        return;
    case GAMEVAR_CHARPTR:
        *((uint8_t *)aGameVars[id].val.lValue)=(uint8_t)lValue;
        return;
    }

badvarid:
    OSD_Printf(CON_ERROR "Gv_SetVar(): invalid gamevar (%d) from sprite %d (%d), player %d\n",
               g_errorLineNum,keyw[g_tw],id,vm.g_i,sprite[vm.g_i].picnum,vm.g_p);
    return;

badplayer:
    OSD_Printf(CON_ERROR "Gv_SetVar(): invalid player (%d) for gamevar %s from sprite %d, player %d\n",
               g_errorLineNum,keyw[g_tw],iPlayer,aGameVars[id].szLabel,vm.g_i,vm.g_p);
    return;

badactor:
    OSD_Printf(CON_ERROR "Gv_SetVar(): invalid actor (%d) for gamevar %s from sprite %d (%d), player %d\n",
               g_errorLineNum,keyw[g_tw],iActor,aGameVars[id].szLabel,vm.g_i,sprite[vm.g_i].picnum,vm.g_p);
    return;
}

int32_t __fastcall Gv_GetVarX(register int32_t id)
{
    if (id == g_iThisActorID)
        return vm.g_i;

    if (id == MAXGAMEVARS)
        return(*insptr++);

    {
        register intptr_t negateResult = id&(MAXGAMEVARS<<1);

        if (id >= g_gameVarCount)
        {
            if (id&(MAXGAMEVARS<<2)) // array
            {
                register int32_t index=Gv_GetVarX(*insptr++);

                id &= (MAXGAMEVARS-1);// ~((MAXGAMEVARS<<2)|(MAXGAMEVARS<<1));

                if (index >= aGameArrays[id].size || index < 0)
                {
                    OSD_Printf(CON_ERROR "Gv_GetVar(): invalid array index (%s[%d])\n",g_errorLineNum,keyw[g_tw],aGameArrays[id].szLabel,index);
                    return -1;
                }
                return ((aGameArrays[id].plValues[index] ^ -negateResult) + negateResult);
            }

            if (id&(MAXGAMEVARS<<3)) // struct shortcut vars
            {
                int32_t index=Gv_GetVarX(*insptr++);

                switch ((id&(MAXGAMEVARS-1)) - g_iSpriteVarID)
                {
                case 0: //if (id == g_iSpriteVarID)
                {
                    register int32_t parm2 = 0, label = *insptr++;

                    /*OSD_Printf("%d %d %d\n",__LINE__,index,label);*/
                    if (ActorLabels[label].flags & LABEL_HASPARM2)
                        parm2 = Gv_GetVarX(*insptr++);

                    return ((VM_AccessSpriteX(index, label, parm2) ^ -negateResult) + negateResult);
                }
                case 3: //else if (id == g_iPlayerVarID)
                {
                    register int32_t parm2 = 0, label = *insptr++;

                    if (PlayerLabels[label].flags & LABEL_HASPARM2)
                        parm2 = Gv_GetVarX(*insptr++);

                    if (index == vm.g_i) index = vm.g_p;
                    return ((VM_AccessPlayerX(index, label, parm2) ^ -negateResult) + negateResult);
                }
                case 4: //else if (id == g_iActorVarID)
                    return ((Gv_GetVar(*insptr++, index, vm.g_p) ^ -negateResult) + negateResult);
                case 1: //else if (id == g_iSectorVarID)
                    if (index == vm.g_i) index = sprite[vm.g_i].sectnum;
                    return ((VM_AccessSectorX(index, *insptr++) ^ -negateResult) + negateResult);
                case 2: //else if (id == g_iWallVarID)
                    return ((VM_AccessWallX(index, *insptr++) ^ -negateResult) + negateResult);
                default:
                    OSD_Printf(CON_ERROR "Gv_GetVar(): WTF?\n",g_errorLineNum,keyw[g_tw]);
                    return -1;
                }
            }

            id &= (MAXGAMEVARS-1);

            if (!negateResult)
            {
                OSD_Printf(CON_ERROR "Gv_GetVar(): invalid gamevar ID (%d)\n",g_errorLineNum,keyw[g_tw],id);
                return -1;
            }
        }

        switch (aGameVars[id].dwFlags &
                (GAMEVAR_USER_MASK|GAMEVAR_INTPTR|GAMEVAR_SHORTPTR|GAMEVAR_CHARPTR))
        {
        default:
            return ((aGameVars[id].val.lValue ^ -negateResult) + negateResult);
        case GAMEVAR_PERPLAYER:
            return ((aGameVars[id].val.plValues[vm.g_p] ^ -negateResult) + negateResult);
        case GAMEVAR_PERACTOR:
            return ((aGameVars[id].val.plValues[vm.g_i] ^ -negateResult) + negateResult);
        case GAMEVAR_INTPTR:
            return (((*((int32_t *)aGameVars[id].val.lValue)) ^ -negateResult) + negateResult);
        case GAMEVAR_SHORTPTR:
            return (((*((int16_t *)aGameVars[id].val.lValue)) ^ -negateResult) + negateResult);
        case GAMEVAR_CHARPTR:
            return (((*((uint8_t *)aGameVars[id].val.lValue)) ^ -negateResult) + negateResult);
        }
    }
}

void __fastcall Gv_SetVarX(register int32_t id, register int32_t lValue)
{
    switch (aGameVars[id].dwFlags &
            (GAMEVAR_USER_MASK|GAMEVAR_INTPTR|GAMEVAR_SHORTPTR|GAMEVAR_CHARPTR))
    {
    default:
        aGameVars[id].val.lValue=lValue;
        return;
    case GAMEVAR_PERPLAYER:
        if (vm.g_p < 0 || vm.g_p > MAXPLAYERS-1) goto badplayer;
        aGameVars[id].val.plValues[vm.g_p]=lValue;
        return;
    case GAMEVAR_PERACTOR:
        if (vm.g_i < 0 || vm.g_i > MAXSPRITES-1) goto badactor;
        aGameVars[id].val.plValues[vm.g_i]=lValue;
        return;
    case GAMEVAR_INTPTR:
        *((int32_t *)aGameVars[id].val.lValue)=(int32_t)lValue;
        return;
    case GAMEVAR_SHORTPTR:
        *((int16_t *)aGameVars[id].val.lValue)=(int16_t)lValue;
        return;
    case GAMEVAR_CHARPTR:
        *((uint8_t *)aGameVars[id].val.lValue)=(uint8_t)lValue;
        return;
    }

badplayer:
    OSD_Printf(CON_ERROR "Gv_SetVar(): invalid player (%d) for gamevar %s\n",
               g_errorLineNum,keyw[g_tw],vm.g_p,aGameVars[id].szLabel);
    return;

badactor:
    OSD_Printf(CON_ERROR "Gv_SetVar(): invalid actor (%d) for gamevar %s\n",
               g_errorLineNum,keyw[g_tw],vm.g_i,aGameVars[id].szLabel);
    return;
}

int32_t Gv_GetVarByLabel(const char *szGameLabel, int32_t lDefault, int32_t iActor, int32_t iPlayer)
{
    int32_t i = hash_find(&h_gamevars,szGameLabel);

    if (i < 0)
        return lDefault;

    return Gv_GetVar(i, iActor, iPlayer);
}

static intptr_t *Gv_GetVarDataPtr(const char *szGameLabel)
{
    int32_t i = hash_find(&h_gamevars,szGameLabel);

    if (i < 0)
        return NULL;

    if (aGameVars[i].dwFlags & (GAMEVAR_PERACTOR | GAMEVAR_PERPLAYER))
    {
        if (!aGameVars[i].val.plValues)
            OSD_Printf(CON_ERROR "Gv_GetVarDataPtr(): INTERNAL ERROR: NULL array !!!\n",g_errorLineNum,keyw[g_tw]);
        return aGameVars[i].val.plValues;
    }

    return &(aGameVars[i].val.lValue);
}

void Gv_ResetSystemDefaults(void)
{
    // call many times...

    int32_t i,j;
    char aszBuf[64];

    //AddLog("ResetWeaponDefaults");

    for (j=MAXPLAYERS-1; j>=0; j--)
    {
        for (i=MAX_WEAPONS-1; i>=0; i--)
        {
            Bsprintf(aszBuf,"WEAPON%d_CLIP",i);
            aplWeaponClip[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_RELOAD",i);
            aplWeaponReload[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",i);
            aplWeaponFireDelay[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",i);
            aplWeaponTotalTime[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",i);
            aplWeaponHoldDelay[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_FLAGS",i);
            aplWeaponFlags[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_SHOOTS",i);
            aplWeaponShoots[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",i);
            aplWeaponSpawnTime[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_SPAWN",i);
            aplWeaponSpawn[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",i);
            aplWeaponShotsPerBurst[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",i);
            aplWeaponWorksLike[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",i);
            aplWeaponInitialSound[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",i);
            aplWeaponFireSound[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",i);
            aplWeaponSound2Time[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",i);
            aplWeaponSound2Sound[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",i);
            aplWeaponReloadSound1[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",i);
            aplWeaponReloadSound2[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_SELECTSOUND",i);
            aplWeaponSelectSound[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_FLASHCOLOR",i);
            aplWeaponFlashColor[i][j]=Gv_GetVarByLabel(aszBuf,0, -1, j);
        }
    }

    g_iReturnVarID=Gv_GetVarIndex("RETURN");
    g_iWeaponVarID=Gv_GetVarIndex("WEAPON");
    g_iWorksLikeVarID=Gv_GetVarIndex("WORKSLIKE");
    g_iZRangeVarID=Gv_GetVarIndex("ZRANGE");
    g_iAngRangeVarID=Gv_GetVarIndex("ANGRANGE");
    g_iAimAngleVarID=Gv_GetVarIndex("AUTOAIMANGLE");
    g_iLoTagID=Gv_GetVarIndex("LOTAG");
    g_iHiTagID=Gv_GetVarIndex("HITAG");
    g_iTextureID=Gv_GetVarIndex("TEXTURE");
    g_iThisActorID=Gv_GetVarIndex("THISACTOR");

    g_iSpriteVarID=Gv_GetVarIndex("sprite");
    g_iSectorVarID=Gv_GetVarIndex("sector");
    g_iWallVarID=Gv_GetVarIndex("wall");
    g_iPlayerVarID=Gv_GetVarIndex("player");
    g_iActorVarID=Gv_GetVarIndex("actorvar");

    Bmemcpy(&ProjectileData,&DefaultProjectileData,sizeof(ProjectileData));

    //AddLog("EOF:ResetWeaponDefaults");
}

static void Gv_AddSystemVars(void)
{
    // only call ONCE
    char aszBuf[64];

    //AddLog("Gv_AddSystemVars");

    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",KNEE_WEAPON);
    Gv_NewVar(aszBuf, KNEE_WEAPON, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",KNEE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",KNEE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",KNEE_WEAPON);
    Gv_NewVar(aszBuf, 7, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",KNEE_WEAPON);
    Gv_NewVar(aszBuf, 14, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",KNEE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",KNEE_WEAPON);
    Gv_NewVar(aszBuf, WEAPON_NOVISIBLE | WEAPON_RANDOMRESTART | WEAPON_AUTOMATIC, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",KNEE_WEAPON);
    Gv_NewVar(aszBuf, KNEE, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",KNEE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",KNEE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",KNEE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",KNEE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",KNEE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",KNEE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",KNEE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",KNEE_WEAPON);
    Gv_NewVar(aszBuf, EJECT_CLIP, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",KNEE_WEAPON);
    Gv_NewVar(aszBuf, INSERT_CLIP, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SELECTSOUND",KNEE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLASHCOLOR",KNEE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);


    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",PISTOL_WEAPON);
    Gv_NewVar(aszBuf, PISTOL_WEAPON, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",PISTOL_WEAPON);
    Gv_NewVar(aszBuf, NAM?20:12, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",PISTOL_WEAPON);
    Gv_NewVar(aszBuf, NAM?50:27, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",PISTOL_WEAPON);
    Gv_NewVar(aszBuf, 2, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",PISTOL_WEAPON);
    Gv_NewVar(aszBuf, 5, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",PISTOL_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",PISTOL_WEAPON);
    Gv_NewVar(aszBuf, NAM?WEAPON_HOLSTER_CLEARS_CLIP:0 | WEAPON_RELOAD_TIMING, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",PISTOL_WEAPON);
    Gv_NewVar(aszBuf, SHOTSPARK1, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",PISTOL_WEAPON);
    Gv_NewVar(aszBuf, 2, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",PISTOL_WEAPON);
    Gv_NewVar(aszBuf, SHELL, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",PISTOL_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",PISTOL_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",PISTOL_WEAPON);
    Gv_NewVar(aszBuf, PISTOL_FIRE, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",PISTOL_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",PISTOL_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",PISTOL_WEAPON);
    Gv_NewVar(aszBuf, EJECT_CLIP, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",PISTOL_WEAPON);
    Gv_NewVar(aszBuf, INSERT_CLIP, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SELECTSOUND",PISTOL_WEAPON);
    Gv_NewVar(aszBuf, INSERT_CLIP, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLASHCOLOR",PISTOL_WEAPON);
    Gv_NewVar(aszBuf, 255+(95<<8), GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",SHOTGUN_WEAPON);
    Gv_NewVar(aszBuf, SHOTGUN_WEAPON, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",SHOTGUN_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",SHOTGUN_WEAPON);
    Gv_NewVar(aszBuf, 13, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",SHOTGUN_WEAPON);
    Gv_NewVar(aszBuf, 4, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",SHOTGUN_WEAPON);
    Gv_NewVar(aszBuf, 30, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",SHOTGUN_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",SHOTGUN_WEAPON);
    Gv_NewVar(aszBuf, WEAPON_CHECKATRELOAD, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",SHOTGUN_WEAPON);
    Gv_NewVar(aszBuf, SHOTGUN, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",SHOTGUN_WEAPON);
    Gv_NewVar(aszBuf, 24, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",SHOTGUN_WEAPON);
    Gv_NewVar(aszBuf, SHOTGUNSHELL, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",SHOTGUN_WEAPON);
    Gv_NewVar(aszBuf, 7, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",SHOTGUN_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",SHOTGUN_WEAPON);
    Gv_NewVar(aszBuf, SHOTGUN_FIRE, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",SHOTGUN_WEAPON);
    Gv_NewVar(aszBuf, 15, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",SHOTGUN_WEAPON);
    Gv_NewVar(aszBuf, SHOTGUN_COCK, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",SHOTGUN_WEAPON);
    Gv_NewVar(aszBuf, EJECT_CLIP, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",SHOTGUN_WEAPON);
    Gv_NewVar(aszBuf, INSERT_CLIP, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SELECTSOUND",SHOTGUN_WEAPON);
    Gv_NewVar(aszBuf, SHOTGUN_COCK, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLASHCOLOR",SHOTGUN_WEAPON);
    Gv_NewVar(aszBuf, 255+(95<<8), GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",CHAINGUN_WEAPON);
    Gv_NewVar(aszBuf, CHAINGUN_WEAPON, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",CHAINGUN_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",CHAINGUN_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",CHAINGUN_WEAPON);
    Gv_NewVar(aszBuf, 3, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",CHAINGUN_WEAPON);
    Gv_NewVar(aszBuf, 12, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",CHAINGUN_WEAPON);
    Gv_NewVar(aszBuf, 3, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",CHAINGUN_WEAPON);
    Gv_NewVar(aszBuf, WEAPON_AUTOMATIC | WEAPON_FIREEVERYTHIRD | WEAPON_AMMOPERSHOT | WEAPON_SPAWNTYPE3 | WEAPON_RESET, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",CHAINGUN_WEAPON);
    Gv_NewVar(aszBuf, CHAINGUN, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",CHAINGUN_WEAPON);
    Gv_NewVar(aszBuf, 1, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",CHAINGUN_WEAPON);
    Gv_NewVar(aszBuf, SHELL, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",CHAINGUN_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",CHAINGUN_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",CHAINGUN_WEAPON);
    Gv_NewVar(aszBuf, CHAINGUN_FIRE, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",CHAINGUN_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",CHAINGUN_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",CHAINGUN_WEAPON);
    Gv_NewVar(aszBuf, EJECT_CLIP, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",CHAINGUN_WEAPON);
    Gv_NewVar(aszBuf, INSERT_CLIP, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SELECTSOUND",CHAINGUN_WEAPON);
    Gv_NewVar(aszBuf, SELECT_WEAPON, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLASHCOLOR",CHAINGUN_WEAPON);
    Gv_NewVar(aszBuf, 255+(95<<8), GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",RPG_WEAPON);
    Gv_NewVar(aszBuf, RPG_WEAPON, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",RPG_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",RPG_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",RPG_WEAPON);
    Gv_NewVar(aszBuf, 4, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",RPG_WEAPON);
    Gv_NewVar(aszBuf, 20, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",RPG_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",RPG_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",RPG_WEAPON);
    Gv_NewVar(aszBuf, RPG, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",RPG_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",RPG_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",RPG_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",RPG_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",RPG_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",RPG_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",RPG_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",RPG_WEAPON);
    Gv_NewVar(aszBuf, EJECT_CLIP, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",RPG_WEAPON);
    Gv_NewVar(aszBuf, INSERT_CLIP, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SELECTSOUND",RPG_WEAPON);
    Gv_NewVar(aszBuf, SELECT_WEAPON, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLASHCOLOR",RPG_WEAPON);
    Gv_NewVar(aszBuf, 255+(95<<8), GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",HANDBOMB_WEAPON);
    Gv_NewVar(aszBuf, HANDBOMB_WEAPON, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",HANDBOMB_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",HANDBOMB_WEAPON);
    Gv_NewVar(aszBuf, 30, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",HANDBOMB_WEAPON);
    Gv_NewVar(aszBuf, 6, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",HANDBOMB_WEAPON);
    Gv_NewVar(aszBuf, 19, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",HANDBOMB_WEAPON);
    Gv_NewVar(aszBuf, 12, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",HANDBOMB_WEAPON);
    Gv_NewVar(aszBuf, WEAPON_THROWIT, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",HANDBOMB_WEAPON);
    Gv_NewVar(aszBuf, HEAVYHBOMB, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",HANDBOMB_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",HANDBOMB_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",HANDBOMB_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",HANDBOMB_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",HANDBOMB_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",HANDBOMB_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",HANDBOMB_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",HANDBOMB_WEAPON);
    Gv_NewVar(aszBuf, EJECT_CLIP, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",HANDBOMB_WEAPON);
    Gv_NewVar(aszBuf, INSERT_CLIP, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SELECTSOUND",HANDBOMB_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLASHCOLOR",HANDBOMB_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",SHRINKER_WEAPON);
    Gv_NewVar(aszBuf, SHRINKER_WEAPON, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",SHRINKER_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",SHRINKER_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",SHRINKER_WEAPON);
    Gv_NewVar(aszBuf, 10, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",SHRINKER_WEAPON);
    Gv_NewVar(aszBuf, NAM?30:12, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",SHRINKER_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",SHRINKER_WEAPON);
    Gv_NewVar(aszBuf, WEAPON_GLOWS, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",SHRINKER_WEAPON);
    Gv_NewVar(aszBuf, SHRINKER, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",SHRINKER_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",SHRINKER_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",SHRINKER_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",SHRINKER_WEAPON);
    Gv_NewVar(aszBuf, SHRINKER_FIRE, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",SHRINKER_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",SHRINKER_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",SHRINKER_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",SHRINKER_WEAPON);
    Gv_NewVar(aszBuf, EJECT_CLIP, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",SHRINKER_WEAPON);
    Gv_NewVar(aszBuf, INSERT_CLIP, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SELECTSOUND",SHRINKER_WEAPON);
    Gv_NewVar(aszBuf, SELECT_WEAPON, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLASHCOLOR",SHRINKER_WEAPON);
    Gv_NewVar(aszBuf, 128+(255<<8)+(128<<16), GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",DEVISTATOR_WEAPON);
    Gv_NewVar(aszBuf, DEVISTATOR_WEAPON, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",DEVISTATOR_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",DEVISTATOR_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",DEVISTATOR_WEAPON);
    Gv_NewVar(aszBuf, 3, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",DEVISTATOR_WEAPON);
    Gv_NewVar(aszBuf, 6, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",DEVISTATOR_WEAPON);
    Gv_NewVar(aszBuf, 5, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",DEVISTATOR_WEAPON);
    Gv_NewVar(aszBuf, WEAPON_FIREEVERYOTHER | WEAPON_AMMOPERSHOT, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",DEVISTATOR_WEAPON);
    Gv_NewVar(aszBuf, RPG, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",DEVISTATOR_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",DEVISTATOR_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",DEVISTATOR_WEAPON);
    Gv_NewVar(aszBuf, 2, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",DEVISTATOR_WEAPON);
    Gv_NewVar(aszBuf, CAT_FIRE, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",DEVISTATOR_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",DEVISTATOR_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",DEVISTATOR_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",DEVISTATOR_WEAPON);
    Gv_NewVar(aszBuf, EJECT_CLIP, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",DEVISTATOR_WEAPON);
    Gv_NewVar(aszBuf, INSERT_CLIP, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SELECTSOUND",DEVISTATOR_WEAPON);
    Gv_NewVar(aszBuf, SELECT_WEAPON, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLASHCOLOR",DEVISTATOR_WEAPON);
    Gv_NewVar(aszBuf, 255+(95<<8), GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",TRIPBOMB_WEAPON);
    Gv_NewVar(aszBuf, TRIPBOMB_WEAPON, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",TRIPBOMB_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",TRIPBOMB_WEAPON);
    Gv_NewVar(aszBuf, 16, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",TRIPBOMB_WEAPON);
    Gv_NewVar(aszBuf, 3, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",TRIPBOMB_WEAPON);
    Gv_NewVar(aszBuf, 16, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",TRIPBOMB_WEAPON);
    Gv_NewVar(aszBuf, 7, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",TRIPBOMB_WEAPON);
    Gv_NewVar(aszBuf, WEAPON_NOVISIBLE | WEAPON_STANDSTILL | WEAPON_CHECKATRELOAD, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",TRIPBOMB_WEAPON);
    Gv_NewVar(aszBuf, HANDHOLDINGLASER, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",TRIPBOMB_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",TRIPBOMB_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",TRIPBOMB_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",TRIPBOMB_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",TRIPBOMB_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",TRIPBOMB_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",TRIPBOMB_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",TRIPBOMB_WEAPON);
    Gv_NewVar(aszBuf, EJECT_CLIP, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",TRIPBOMB_WEAPON);
    Gv_NewVar(aszBuf, INSERT_CLIP, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SELECTSOUND",TRIPBOMB_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLASHCOLOR",TRIPBOMB_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",FREEZE_WEAPON);
    Gv_NewVar(aszBuf, FREEZE_WEAPON, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",FREEZE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",FREEZE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",FREEZE_WEAPON);
    Gv_NewVar(aszBuf, 3, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",FREEZE_WEAPON);
    Gv_NewVar(aszBuf, 5, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",FREEZE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",FREEZE_WEAPON);
    Gv_NewVar(aszBuf, WEAPON_RESET, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",FREEZE_WEAPON);
    Gv_NewVar(aszBuf, FREEZEBLAST, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",FREEZE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",FREEZE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",FREEZE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",FREEZE_WEAPON);
    Gv_NewVar(aszBuf, CAT_FIRE, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",FREEZE_WEAPON);
    Gv_NewVar(aszBuf, CAT_FIRE, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",FREEZE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",FREEZE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",FREEZE_WEAPON);
    Gv_NewVar(aszBuf, EJECT_CLIP, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",FREEZE_WEAPON);
    Gv_NewVar(aszBuf, INSERT_CLIP, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SELECTSOUND",FREEZE_WEAPON);
    Gv_NewVar(aszBuf, SELECT_WEAPON, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLASHCOLOR",FREEZE_WEAPON);
    Gv_NewVar(aszBuf, 128+(128<<8)+(255<<16), GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",HANDREMOTE_WEAPON);
    Gv_NewVar(aszBuf, HANDREMOTE_WEAPON, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",HANDREMOTE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",HANDREMOTE_WEAPON);
    Gv_NewVar(aszBuf, 10, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",HANDREMOTE_WEAPON);
    Gv_NewVar(aszBuf, 2, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",HANDREMOTE_WEAPON);
    Gv_NewVar(aszBuf, 10, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",HANDREMOTE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",HANDREMOTE_WEAPON);
    Gv_NewVar(aszBuf, WEAPON_BOMB_TRIGGER | WEAPON_NOVISIBLE, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",HANDREMOTE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",HANDREMOTE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",HANDREMOTE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",HANDREMOTE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",HANDREMOTE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",HANDREMOTE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",HANDREMOTE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",HANDREMOTE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",HANDREMOTE_WEAPON);
    Gv_NewVar(aszBuf, EJECT_CLIP, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",HANDREMOTE_WEAPON);
    Gv_NewVar(aszBuf, INSERT_CLIP, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SELECTSOUND",HANDREMOTE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLASHCOLOR",HANDREMOTE_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);

    ///////////////////////////////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",GROW_WEAPON);
    Gv_NewVar(aszBuf, GROW_WEAPON, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",GROW_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",GROW_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",GROW_WEAPON);
    Gv_NewVar(aszBuf, 3, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",GROW_WEAPON);
    Gv_NewVar(aszBuf, NAM?30:5, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",GROW_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",GROW_WEAPON);
    Gv_NewVar(aszBuf, WEAPON_GLOWS, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",GROW_WEAPON);
    Gv_NewVar(aszBuf, GROWSPARK, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",GROW_WEAPON);
    Gv_NewVar(aszBuf, NAM?2:0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",GROW_WEAPON);
    Gv_NewVar(aszBuf, NAM?SHELL:0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",GROW_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",GROW_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",GROW_WEAPON);
    Gv_NewVar(aszBuf, NAM?0:EXPANDERSHOOT, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",GROW_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",GROW_WEAPON);
    Gv_NewVar(aszBuf, 0, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",GROW_WEAPON);
    Gv_NewVar(aszBuf, EJECT_CLIP, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",GROW_WEAPON);
    Gv_NewVar(aszBuf, INSERT_CLIP, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SELECTSOUND",GROW_WEAPON);
    Gv_NewVar(aszBuf, SELECT_WEAPON, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLASHCOLOR",GROW_WEAPON);
    Gv_NewVar(aszBuf, 255+(95<<8), GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);

    Gv_NewVar("GRENADE_LIFETIME", NAM_GRENADE_LIFETIME, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Gv_NewVar("GRENADE_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);

    Gv_NewVar("STICKYBOMB_LIFETIME", NAM_GRENADE_LIFETIME, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Gv_NewVar("STICKYBOMB_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);

    Gv_NewVar("TRIPBOMB_CONTROL", TRIPBOMB_TRIPWIRE, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);
    Gv_NewVar("PIPEBOMB_CONTROL", NAM?PIPEBOMB_TIMER:PIPEBOMB_REMOTE, GAMEVAR_PERPLAYER | GAMEVAR_SYSTEM);

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
    Gv_NewVar("sprite", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    Gv_NewVar("sector", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    Gv_NewVar("wall", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    Gv_NewVar("player", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);
    Gv_NewVar("actorvar", -1, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SPECIAL);

    Gv_NewVar("myconnectindex", (intptr_t)&myconnectindex, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_SYNCCHECK);
    Gv_NewVar("screenpeek", (intptr_t)&screenpeek, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_SYNCCHECK);
    Gv_NewVar("currentweapon",(intptr_t)&g_currentweapon, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_SYNCCHECK);
    Gv_NewVar("gs",(intptr_t)&g_gs, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_SYNCCHECK);
    Gv_NewVar("looking_arc",(intptr_t)&g_looking_arc, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_SYNCCHECK);
    Gv_NewVar("gun_pos",(intptr_t)&g_gun_pos, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_SYNCCHECK);
    Gv_NewVar("weapon_xoffset",(intptr_t)&g_weapon_xoffset, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_SYNCCHECK);
    Gv_NewVar("weaponcount",(intptr_t)&g_kb, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_SYNCCHECK);
    Gv_NewVar("looking_angSR1",(intptr_t)&g_looking_angSR1, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_SYNCCHECK);
    Gv_NewVar("xdim",(intptr_t)&xdim, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY | GAMEVAR_SYNCCHECK);
    Gv_NewVar("ydim",(intptr_t)&ydim, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY | GAMEVAR_SYNCCHECK);
    Gv_NewVar("windowx1",(intptr_t)&windowx1, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY | GAMEVAR_SYNCCHECK);
    Gv_NewVar("windowx2",(intptr_t)&windowx2, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY | GAMEVAR_SYNCCHECK);
    Gv_NewVar("windowy1",(intptr_t)&windowy1, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY | GAMEVAR_SYNCCHECK);
    Gv_NewVar("windowy2",(intptr_t)&windowy2, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY | GAMEVAR_SYNCCHECK);
    Gv_NewVar("totalclock",(intptr_t)&totalclock, GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_READONLY | GAMEVAR_SYNCCHECK);
    Gv_NewVar("lastvisinc",(intptr_t)&lastvisinc, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("numsectors",(intptr_t)&numsectors, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR | GAMEVAR_READONLY);

    Gv_NewVar("current_menu",(intptr_t)&g_currentMenu, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);
    Gv_NewVar("numplayers",(intptr_t)&numplayers, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY);
    Gv_NewVar("viewingrange",(intptr_t)&viewingrange, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY | GAMEVAR_SYNCCHECK);
    Gv_NewVar("yxaspect",(intptr_t)&yxaspect, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY | GAMEVAR_SYNCCHECK);
    Gv_NewVar("gravitationalconstant",(intptr_t)&g_spriteGravity, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("gametype_flags",(intptr_t)&GametypeFlags[ud.coop], GAMEVAR_SYSTEM | GAMEVAR_INTPTR);
    Gv_NewVar("framerate",(intptr_t)&g_currentFrameRate, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_READONLY | GAMEVAR_SYNCCHECK);
    Gv_NewVar("CLIPMASK0", CLIPMASK0, GAMEVAR_SYSTEM|GAMEVAR_READONLY);
    Gv_NewVar("CLIPMASK1", CLIPMASK1, GAMEVAR_SYSTEM|GAMEVAR_READONLY);

    Gv_NewVar("camerax",(intptr_t)&ud.camera.x, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("cameray",(intptr_t)&ud.camera.y, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("cameraz",(intptr_t)&ud.camera.z, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("cameraang",(intptr_t)&ud.cameraang, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("camerahoriz",(intptr_t)&ud.camerahoriz, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("camerasect",(intptr_t)&ud.camerasect, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("cameradist",(intptr_t)&g_cameraDistance, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("cameraclock",(intptr_t)&g_cameraClock, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_SYNCCHECK);

    Gv_NewVar("myx",(intptr_t)&my.x, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("myy",(intptr_t)&my.y, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("myz",(intptr_t)&my.z, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("omyx",(intptr_t)&omy.x, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("omyy",(intptr_t)&omy.y, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("omyz",(intptr_t)&omy.z, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("myvelx",(intptr_t)&myvel.x, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("myvely",(intptr_t)&myvel.y, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("myvelz",(intptr_t)&myvel.z, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_SYNCCHECK);

    Gv_NewVar("myhoriz",(intptr_t)&myhoriz, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("myhorizoff",(intptr_t)&myhorizoff, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("omyhoriz",(intptr_t)&omyhoriz, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("omyhorizoff",(intptr_t)&omyhorizoff, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("myang",(intptr_t)&myang, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("omyang",(intptr_t)&omyang, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("mycursectnum",(intptr_t)&mycursectnum, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("myjumpingcounter",(intptr_t)&myjumpingcounter, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR | GAMEVAR_SYNCCHECK);

    Gv_NewVar("myjumpingtoggle",(intptr_t)&myjumpingtoggle, GAMEVAR_SYSTEM | GAMEVAR_CHARPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("myonground",(intptr_t)&myonground, GAMEVAR_SYSTEM | GAMEVAR_CHARPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("myhardlanding",(intptr_t)&myhardlanding, GAMEVAR_SYSTEM | GAMEVAR_CHARPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("myreturntocenter",(intptr_t)&myreturntocenter, GAMEVAR_SYSTEM | GAMEVAR_CHARPTR | GAMEVAR_SYNCCHECK);

    Gv_NewVar("display_mirror",(intptr_t)&display_mirror, GAMEVAR_SYSTEM | GAMEVAR_CHARPTR | GAMEVAR_SYNCCHECK);
    Gv_NewVar("randomseed",(intptr_t)&randomseed, GAMEVAR_SYSTEM | GAMEVAR_INTPTR);

    Gv_NewVar("NUMWALLS",(intptr_t)&numwalls, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR | GAMEVAR_READONLY);
    Gv_NewVar("NUMSECTORS",(intptr_t)&numsectors, GAMEVAR_SYSTEM | GAMEVAR_SHORTPTR | GAMEVAR_READONLY);

    Gv_NewVar("lastsavepos",(intptr_t)&g_lastSaveSlot, GAMEVAR_SYSTEM | GAMEVAR_INTPTR | GAMEVAR_SYNCCHECK);
#ifdef POLYMOST
    Gv_NewVar("rendmode",(intptr_t)&rendmode, GAMEVAR_READONLY | GAMEVAR_INTPTR | GAMEVAR_SYSTEM | GAMEVAR_SYNCCHECK);
#else
    Gv_NewVar("rendmode", 0, GAMEVAR_READONLY | GAMEVAR_SYSTEM | GAMEVAR_SYNCCHECK);
#endif
}

void Gv_Init(void)
{
    // only call ONCE

    //  initprintf("Initializing game variables\n");
    //AddLog("Gv_Init");

    Gv_Clear();
    Gv_AddSystemVars();
    Gv_InitWeaponPointers();
    Gv_ResetSystemDefaults();
}

void Gv_InitWeaponPointers(void)
{
    int32_t i;
    char aszBuf[64];
    // called from game Init AND when level is loaded...

    //AddLog("Gv_InitWeaponPointers");

    for (i=(MAX_WEAPONS-1); i>=0; i--)
    {
        Bsprintf(aszBuf,"WEAPON%d_CLIP",i);
        aplWeaponClip[i]=Gv_GetVarDataPtr(aszBuf);
        if (!aplWeaponClip[i])
        {
            initprintf("ERROR: NULL weapon!  WTF?!\n");
            // exit(0);
            G_Shutdown();
        }
        Bsprintf(aszBuf,"WEAPON%d_RELOAD",i);
        aplWeaponReload[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",i);
        aplWeaponFireDelay[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",i);
        aplWeaponTotalTime[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",i);
        aplWeaponHoldDelay[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_FLAGS",i);
        aplWeaponFlags[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SHOOTS",i);
        aplWeaponShoots[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",i);
        aplWeaponSpawnTime[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SPAWN",i);
        aplWeaponSpawn[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",i);
        aplWeaponShotsPerBurst[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",i);
        aplWeaponWorksLike[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",i);
        aplWeaponInitialSound[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",i);
        aplWeaponFireSound[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",i);
        aplWeaponSound2Time[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",i);
        aplWeaponSound2Sound[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",i);
        aplWeaponReloadSound1[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",i);
        aplWeaponReloadSound2[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SELECTSOUND",i);
        aplWeaponSelectSound[i]=Gv_GetVarDataPtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_FLASHCOLOR",i);
        aplWeaponFlashColor[i]=Gv_GetVarDataPtr(aszBuf);
    }
}

void Gv_RefreshPointers(void)
{
    aGameVars[Gv_GetVarIndex("RESPAWN_MONSTERS")].val.lValue = (intptr_t)&ud.respawn_monsters;
    aGameVars[Gv_GetVarIndex("RESPAWN_ITEMS")].val.lValue = (intptr_t)&ud.respawn_items;
    aGameVars[Gv_GetVarIndex("RESPAWN_INVENTORY")].val.lValue = (intptr_t)&ud.respawn_inventory;
    aGameVars[Gv_GetVarIndex("MONSTERS_OFF")].val.lValue = (intptr_t)&ud.monsters_off;
    aGameVars[Gv_GetVarIndex("MARKER")].val.lValue = (intptr_t)&ud.marker;
    aGameVars[Gv_GetVarIndex("FFIRE")].val.lValue = (intptr_t)&ud.ffire;
    aGameVars[Gv_GetVarIndex("LEVEL")].val.lValue = (intptr_t)&ud.level_number;
    aGameVars[Gv_GetVarIndex("VOLUME")].val.lValue = (intptr_t)&ud.volume_number;

    aGameVars[Gv_GetVarIndex("COOP")].val.lValue = (intptr_t)&ud.coop;
    aGameVars[Gv_GetVarIndex("MULTIMODE")].val.lValue = (intptr_t)&ud.multimode;

    aGameVars[Gv_GetVarIndex("myconnectindex")].val.lValue = (intptr_t)&myconnectindex;
    aGameVars[Gv_GetVarIndex("screenpeek")].val.lValue = (intptr_t)&screenpeek;
    aGameVars[Gv_GetVarIndex("currentweapon")].val.lValue = (intptr_t)&g_currentweapon;
    aGameVars[Gv_GetVarIndex("gs")].val.lValue = (intptr_t)&g_gs;
    aGameVars[Gv_GetVarIndex("looking_arc")].val.lValue = (intptr_t)&g_looking_arc;
    aGameVars[Gv_GetVarIndex("gun_pos")].val.lValue = (intptr_t)&g_gun_pos;
    aGameVars[Gv_GetVarIndex("weapon_xoffset")].val.lValue = (intptr_t)&g_weapon_xoffset;
    aGameVars[Gv_GetVarIndex("weaponcount")].val.lValue = (intptr_t)&g_kb;
    aGameVars[Gv_GetVarIndex("looking_angSR1")].val.lValue = (intptr_t)&g_looking_angSR1;
    aGameVars[Gv_GetVarIndex("xdim")].val.lValue = (intptr_t)&xdim;
    aGameVars[Gv_GetVarIndex("ydim")].val.lValue = (intptr_t)&ydim;
    aGameVars[Gv_GetVarIndex("windowx1")].val.lValue = (intptr_t)&windowx1;
    aGameVars[Gv_GetVarIndex("windowx2")].val.lValue = (intptr_t)&windowx2;
    aGameVars[Gv_GetVarIndex("windowy1")].val.lValue = (intptr_t)&windowy1;
    aGameVars[Gv_GetVarIndex("windowy2")].val.lValue = (intptr_t)&windowy2;
    aGameVars[Gv_GetVarIndex("totalclock")].val.lValue = (intptr_t)&totalclock;
    aGameVars[Gv_GetVarIndex("lastvisinc")].val.lValue = (intptr_t)&lastvisinc;
    aGameVars[Gv_GetVarIndex("numsectors")].val.lValue = (intptr_t)&numsectors;
    aGameVars[Gv_GetVarIndex("numplayers")].val.lValue = (intptr_t)&numplayers;
    aGameVars[Gv_GetVarIndex("current_menu")].val.lValue = (intptr_t)&g_currentMenu;
    aGameVars[Gv_GetVarIndex("viewingrange")].val.lValue = (intptr_t)&viewingrange;
    aGameVars[Gv_GetVarIndex("yxaspect")].val.lValue = (intptr_t)&yxaspect;
    aGameVars[Gv_GetVarIndex("gravitationalconstant")].val.lValue = (intptr_t)&g_spriteGravity;
    aGameVars[Gv_GetVarIndex("gametype_flags")].val.lValue = (intptr_t)&GametypeFlags[ud.coop];
    aGameVars[Gv_GetVarIndex("framerate")].val.lValue = (intptr_t)&g_currentFrameRate;

    aGameVars[Gv_GetVarIndex("camerax")].val.lValue = (intptr_t)&ud.camera.x;
    aGameVars[Gv_GetVarIndex("cameray")].val.lValue = (intptr_t)&ud.camera.y;
    aGameVars[Gv_GetVarIndex("cameraz")].val.lValue = (intptr_t)&ud.camera.z;
    aGameVars[Gv_GetVarIndex("cameraang")].val.lValue = (intptr_t)&ud.cameraang;
    aGameVars[Gv_GetVarIndex("camerahoriz")].val.lValue = (intptr_t)&ud.camerahoriz;
    aGameVars[Gv_GetVarIndex("camerasect")].val.lValue = (intptr_t)&ud.camerasect;
    aGameVars[Gv_GetVarIndex("cameradist")].val.lValue = (intptr_t)&g_cameraDistance;
    aGameVars[Gv_GetVarIndex("cameraclock")].val.lValue = (intptr_t)&g_cameraClock;

    aGameVars[Gv_GetVarIndex("myx")].val.lValue = (intptr_t)&my.x;
    aGameVars[Gv_GetVarIndex("myy")].val.lValue = (intptr_t)&my.y;
    aGameVars[Gv_GetVarIndex("myz")].val.lValue = (intptr_t)&my.z;
    aGameVars[Gv_GetVarIndex("omyx")].val.lValue = (intptr_t)&omy.x;
    aGameVars[Gv_GetVarIndex("omyy")].val.lValue = (intptr_t)&omy.y;
    aGameVars[Gv_GetVarIndex("omyz")].val.lValue = (intptr_t)&omy.z;
    aGameVars[Gv_GetVarIndex("myvelx")].val.lValue = (intptr_t)&myvel.x;
    aGameVars[Gv_GetVarIndex("myvely")].val.lValue = (intptr_t)&myvel.y;
    aGameVars[Gv_GetVarIndex("myvelz")].val.lValue = (intptr_t)&myvel.z;

    aGameVars[Gv_GetVarIndex("myhoriz")].val.lValue = (intptr_t)&myhoriz;
    aGameVars[Gv_GetVarIndex("myhorizoff")].val.lValue = (intptr_t)&myhorizoff;
    aGameVars[Gv_GetVarIndex("omyhoriz")].val.lValue = (intptr_t)&omyhoriz;
    aGameVars[Gv_GetVarIndex("omyhorizoff")].val.lValue = (intptr_t)&omyhorizoff;
    aGameVars[Gv_GetVarIndex("myang")].val.lValue = (intptr_t)&myang;
    aGameVars[Gv_GetVarIndex("omyang")].val.lValue = (intptr_t)&omyang;
    aGameVars[Gv_GetVarIndex("mycursectnum")].val.lValue = (intptr_t)&mycursectnum;
    aGameVars[Gv_GetVarIndex("myjumpingcounter")].val.lValue = (intptr_t)&myjumpingcounter;

    aGameVars[Gv_GetVarIndex("myjumpingtoggle")].val.lValue = (intptr_t)&myjumpingtoggle;
    aGameVars[Gv_GetVarIndex("myonground")].val.lValue = (intptr_t)&myonground;
    aGameVars[Gv_GetVarIndex("myhardlanding")].val.lValue = (intptr_t)&myhardlanding;
    aGameVars[Gv_GetVarIndex("myreturntocenter")].val.lValue = (intptr_t)&myreturntocenter;

    aGameVars[Gv_GetVarIndex("display_mirror")].val.lValue = (intptr_t)&display_mirror;
    aGameVars[Gv_GetVarIndex("randomseed")].val.lValue = (intptr_t)&randomseed;

    aGameVars[Gv_GetVarIndex("NUMWALLS")].val.lValue = (intptr_t)&numwalls;
    aGameVars[Gv_GetVarIndex("NUMSECTORS")].val.lValue = (intptr_t)&numsectors;

    aGameVars[Gv_GetVarIndex("lastsavepos")].val.lValue = (intptr_t)&g_lastSaveSlot;
#ifdef POLYMOST
    aGameVars[Gv_GetVarIndex("rendmode")].val.lValue = (intptr_t)&rendmode;
#endif
}
