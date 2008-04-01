//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment
Copyright (C) 2000, 2003 - Matt Saettler (EDuke Enhancements)
Copyright (C) 2004, 2007 - EDuke32 developers

This file is part of EDuke32

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
#include "gamedef.h"
#include "osd.h"

extern int g_i,g_p;

static void ResetPointerVars(void);

static void FreeGameVars(void)
{
    // call this function as many times as needed.
    int i;
    //  AddLog("FreeGameVars");
    for (i=0;i<MAXGAMEVARS;i++)
    {
//        aGameVars[i].lValue=0;
//        if (aGameVars[i].szLabel)
        //Bfree(aGameVars[i].szLabel);
//        aGameVars[i].szLabel=NULL;
//        aGameVars[i].dwFlags=0;

        if (aGameVars[i].plValues)
            Bfree(aGameVars[i].plValues);
        aGameVars[i].plValues=NULL;
        aGameVars[i].bReset=1;
    }
    iGameVarCount=0;
    for (i=0;i<MAXGAMEARRAYS;i++)
    {
        if (aGameArrays[i].plValues)
            Bfree(aGameArrays[i].plValues);
        aGameArrays[i].plValues=NULL;
        aGameArrays[i].bReset=1;
    }
    iGameArrayCount=0;
    return;
}

static void ClearGameVars(void)
{
    // only call this function ONCE...
    int i;

    //AddLog("ClearGameVars");

    for (i=0;i<MAXGAMEVARS;i++)
    {
        aGameVars[i].lValue=0;
        if (aGameVars[i].szLabel)
            Bfree(aGameVars[i].szLabel);
        aGameVars[i].szLabel=NULL;
        aGameVars[i].dwFlags=0;

        if (aGameVars[i].plValues)
            Bfree(aGameVars[i].plValues);
        aGameVars[i].plValues=NULL;
        aGameVars[i].bReset=1;
    }
    iGameVarCount=0;
    for (i=0;i<MAXGAMEARRAYS;i++)
    {
        if (aGameArrays[i].szLabel)
            Bfree(aGameArrays[i].szLabel);
        aGameArrays[i].szLabel=NULL;

        if (aGameArrays[i].plValues)
            Bfree(aGameArrays[i].plValues);
        aGameArrays[i].plValues=NULL;
        aGameArrays[i].bReset=1;
    }
    iGameArrayCount=0;
    return;
}

int ReadGameVars(int fil)
{
    int i;
    int l;

    //     AddLog("Reading gamevars from savegame");

    FreeGameVars(); // nuke 'em from orbit, it's the only way to be sure...
    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);

    if (kdfread(&iGameVarCount,sizeof(iGameVarCount),1,fil) != 1) goto corrupt;
    for (i=0;i<iGameVarCount;i++)
    {
        if (kdfread(&(aGameVars[i]),sizeof(gamevar_t),1,fil) != 1) goto corrupt;
        aGameVars[i].szLabel=Bcalloc(MAXVARLABEL,sizeof(char));
        if (kdfread(aGameVars[i].szLabel,sizeof(char) * MAXVARLABEL, 1, fil) != 1) goto corrupt;
    }
    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);
    for (i=0;i<iGameVarCount;i++)
    {
        if (aGameVars[i].dwFlags & GAMEVAR_FLAG_PERPLAYER)
            aGameVars[i].plValues=Bcalloc(MAXPLAYERS,sizeof(int));
        else if (aGameVars[i].dwFlags & GAMEVAR_FLAG_PERACTOR)
            aGameVars[i].plValues=Bcalloc(MAXSPRITES,sizeof(int));
        else
            // else nothing 'extra...'
            aGameVars[i].plValues=NULL;
    }

    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);
    InitGameVarPointers();

    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);
    for (i=0;i<iGameVarCount;i++)
    {
        if (aGameVars[i].dwFlags & GAMEVAR_FLAG_PERPLAYER)
        {
            //Bsprintf(g_szBuf,"Reading value array for %s (%d)",aGameVars[i].szLabel,sizeof(int) * MAXPLAYERS);
            //AddLog(g_szBuf);
            if (kdfread(aGameVars[i].plValues,sizeof(int) * MAXPLAYERS, 1, fil) != 1) goto corrupt;
        }
        else if (aGameVars[i].dwFlags & GAMEVAR_FLAG_PERACTOR)
        {
            //Bsprintf(g_szBuf,"Reading value array for %s (%d)",aGameVars[i].szLabel,sizeof(int) * MAXSPRITES);
            //AddLog(g_szBuf);
            if (kdfread(&aGameVars[i].plValues[0],sizeof(int), MAXSPRITES, fil) != MAXSPRITES) goto corrupt;
        }
        // else nothing 'extra...'
    }

    ResetPointerVars();

    if (kdfread(&iGameArrayCount,sizeof(iGameArrayCount),1,fil) != 1) goto corrupt;
    for (i=0;i<iGameArrayCount;i++)
    {
        if (kdfread(&(aGameArrays[i]),sizeof(gamearray_t),1,fil) != 1) goto corrupt;
        aGameArrays[i].szLabel=Bcalloc(MAXARRAYLABEL,sizeof(char));
        if (kdfread(aGameArrays[i].szLabel,sizeof(char) * MAXARRAYLABEL, 1, fil) != 1) goto corrupt;
    }
    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);
    for (i=0;i<iGameArrayCount;i++)
    {
        aGameArrays[i].plValues=Bcalloc(aGameArrays[i].size,sizeof(int));
    }

    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);
    for (i=0;i<iGameArrayCount;i++)
    {
        //Bsprintf(g_szBuf,"Reading value array for %s (%d)",aGameVars[i].szLabel,sizeof(int) * MAXPLAYERS);
        //AddLog(g_szBuf);
        if (kdfread(aGameArrays[i].plValues,sizeof(int) * aGameArrays[i].size, 1, fil) != 1) goto corrupt;
    }

    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);
    if (kdfread(apScriptGameEvent,sizeof(apScriptGameEvent),1,fil) != 1) goto corrupt;
    for (i=0;i<MAXGAMEEVENTS;i++)
        if (apScriptGameEvent[i])
        {
            l = (int)apScriptGameEvent[i]+(int)&script[0];
            apScriptGameEvent[i] = (int *)l;
        }

    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);
    if (kdfread(&l,sizeof(l),1,fil) != 1) goto corrupt;
    if (kdfread(g_szBuf,l,1,fil) != 1) goto corrupt;
    g_szBuf[l]=0;
    OSD_Printf("%s\n",g_szBuf);

#if 0
    {
        FILE *fp;
        AddLog("Dumping Vars...");
        fp=fopen("xxx.txt","w");
        if (fp)
        {
            DumpGameVars(fp);
            fclose(fp);
        }
        AddLog("Done Dumping...");
    }
#endif
    return(0);
corrupt:
    return(1);
}

void SaveGameVars(FILE *fil)
{
    int i;
    int l;

    //   AddLog("Saving Game Vars to File");
    dfwrite(&iGameVarCount,sizeof(iGameVarCount),1,fil);

    for (i=0;i<iGameVarCount;i++)
    {
        dfwrite(&(aGameVars[i]),sizeof(gamevar_t),1,fil);
        dfwrite(aGameVars[i].szLabel,sizeof(char) * MAXVARLABEL, 1, fil);
    }

    //     dfwrite(&aGameVars,sizeof(aGameVars),1,fil);

    for (i=0;i<iGameVarCount;i++)
    {
        if (aGameVars[i].dwFlags & GAMEVAR_FLAG_PERPLAYER)
        {
            //Bsprintf(g_szBuf,"Writing value array for %s (%d)",aGameVars[i].szLabel,sizeof(int) * MAXPLAYERS);
            //AddLog(g_szBuf);
            dfwrite(aGameVars[i].plValues,sizeof(int) * MAXPLAYERS, 1, fil);
        }
        else if (aGameVars[i].dwFlags & GAMEVAR_FLAG_PERACTOR)
        {
            //Bsprintf(g_szBuf,"Writing value array for %s (%d)",aGameVars[i].szLabel,sizeof(int) * MAXSPRITES);
            //AddLog(g_szBuf);
            dfwrite(&aGameVars[i].plValues[0],sizeof(int), MAXSPRITES, fil);
        }
        // else nothing 'extra...'
    }

    dfwrite(&iGameArrayCount,sizeof(iGameArrayCount),1,fil);

    for (i=0;i<iGameArrayCount;i++)
    {
        dfwrite(&(aGameArrays[i]),sizeof(gamearray_t),1,fil);
        dfwrite(aGameArrays[i].szLabel,sizeof(char) * MAXARRAYLABEL, 1, fil);
    }

    //     dfwrite(&aGameVars,sizeof(aGameVars),1,fil);

    for (i=0;i<iGameArrayCount;i++)
    {
            dfwrite(aGameArrays[i].plValues,sizeof(int) * aGameArrays[i].size, 1, fil);
    }

    for (i=0;i<MAXGAMEEVENTS;i++)
        if (apScriptGameEvent[i])
        {
            l = (int)apScriptGameEvent[i]-(int)&script[0];
            apScriptGameEvent[i] = (int *)l;
        }
    dfwrite(apScriptGameEvent,sizeof(apScriptGameEvent),1,fil);
    for (i=0;i<MAXGAMEEVENTS;i++)
        if (apScriptGameEvent[i])
        {
            l = (int)apScriptGameEvent[i]+(int)&script[0];
            apScriptGameEvent[i] = (int *)l;
        }

    Bsprintf(g_szBuf,"EOF: EDuke32");
    l=strlen(g_szBuf);
    dfwrite(&l,sizeof(l),1,fil);
    dfwrite(g_szBuf,l,1,fil);
}

void DumpGameVars(FILE *fp)
{
    int i;
    if (!fp)
    {
        return;
    }
    fprintf(fp,"// Current Game Definitions\n\n");
    for (i=0;i<iGameVarCount;i++)
    {
        if (aGameVars[i].dwFlags & (GAMEVAR_FLAG_SECRET))
        {
            continue; // do nothing...
        }
        else
        {
            fprintf(fp,"gamevar %s ",aGameVars[i].szLabel);

            if (aGameVars[i].dwFlags & (GAMEVAR_FLAG_INTPTR))
                fprintf(fp,"%d",*((int*)aGameVars[i].lValue));
            else if (aGameVars[i].dwFlags & (GAMEVAR_FLAG_SHORTPTR))
                fprintf(fp,"%d",*((short*)aGameVars[i].lValue));
            else if (aGameVars[i].dwFlags & (GAMEVAR_FLAG_CHARPTR))
                fprintf(fp,"%d",*((char*)aGameVars[i].lValue));
            else
                fprintf(fp,"%d",aGameVars[i].lValue);
            if (aGameVars[i].dwFlags & (GAMEVAR_FLAG_PERPLAYER))
                fprintf(fp," GAMEVAR_FLAG_PERPLAYER");
            else if (aGameVars[i].dwFlags & (GAMEVAR_FLAG_PERACTOR))
                fprintf(fp," GAMEVAR_FLAG_PERACTOR");
            else
                fprintf(fp," %d",aGameVars[i].dwFlags & (GAMEVAR_FLAG_USER_MASK));
            fprintf(fp," // ");
            if (aGameVars[i].dwFlags & (GAMEVAR_FLAG_SYSTEM))
                fprintf(fp," (system)");
            if (aGameVars[i].dwFlags & (GAMEVAR_FLAG_INTPTR|GAMEVAR_FLAG_SHORTPTR|GAMEVAR_FLAG_CHARPTR))
                fprintf(fp," (pointer)");
            if (aGameVars[i].dwFlags & (GAMEVAR_FLAG_READONLY))
                fprintf(fp," (read only)");
            fprintf(fp,"\n");
        }
    }
    fprintf(fp,"\n// end of game definitions\n");
}

void ResetGameVars(void)
{
    int i;

    //AddLog("Reset Game Vars");
    FreeGameVars();

    for (i=0;i<MAXGAMEVARS;i++)
    {
        //Bsprintf(g_szBuf,"Resetting %d: '%s' to %d",i,aDefaultGameVars[i].szLabel,
        //      aDefaultGameVars[i].lValue
        //     );
        //AddLog(g_szBuf);
        if (aGameVars[i].szLabel != NULL && aGameVars[i].bReset)
            AddGameVar(aGameVars[i].szLabel,aGameVars[i].lDefault,aGameVars[i].dwFlags);
    }

    for (i=0;i<MAXGAMEARRAYS;i++)
    {
        //Bsprintf(g_szBuf,"Resetting %d: '%s' to %d",i,aDefaultGameVars[i].szLabel,
        //      aDefaultGameVars[i].lValue
        //     );
        //AddLog(g_szBuf);
        if (aGameArrays[i].szLabel != NULL && aGameArrays[i].bReset)
            AddGameArray(aGameArrays[i].szLabel,aGameArrays[i].size);
    }
}

int AddGameArray(const char *pszLabel, int asize)
{
    int i;

    if (Bstrlen(pszLabel) > (MAXARRAYLABEL-1))
    {
        error++;
        ReportError(-1);
        initprintf("%s:%d: error: array name `%s' exceeds limit of %d characters.\n",compilefile,line_number,pszLabel, MAXARRAYLABEL);
        return 0;
    }
    for (i=0;i<iGameArrayCount;i++)
    {
        if (aGameVars[i].szLabel != NULL && !aGameArrays[i].bReset)
        {
            if (Bstrcmp(pszLabel,aGameArrays[i].szLabel) == 0)
            {
                // found it it's a duplicate in error
                warning++;
                ReportError(WARNING_DUPLICATEDEFINITION);
                return 0;

            }
        }
    }
    if (i < MAXGAMEARRAYS)
    {
        if (aGameArrays[i].szLabel == NULL)
            aGameArrays[i].szLabel=Bcalloc(MAXVARLABEL,sizeof(char));
        if (aGameArrays[i].szLabel != pszLabel)
            Bstrcpy(aGameArrays[i].szLabel,pszLabel);
        aGameArrays[i].plValues=Bcalloc(asize,sizeof(int));
        aGameArrays[i].size=asize;
        aGameVars[i].bReset=0;
        iGameArrayCount++;
        return 1;
    }
    return 0;
}

int AddGameVar(const char *pszLabel, int lValue, unsigned int dwFlags)
{
    int i, j;

    //Bsprintf(g_szBuf,"AddGameVar(%s, %d, %X)",pszLabel, lValue, dwFlags);
    //AddLog(g_szBuf);

    if (Bstrlen(pszLabel) > (MAXVARLABEL-1))
    {
        error++;
        ReportError(-1);
        initprintf("%s:%d: error: variable name `%s' exceeds limit of %d characters.\n",compilefile,line_number,pszLabel, MAXVARLABEL);
        return 0;
    }
    for (i=0;i<iGameVarCount;i++)
    {
        if (aGameVars[i].szLabel != NULL && !aGameVars[i].bReset)
        {
            if (Bstrcmp(pszLabel,aGameVars[i].szLabel) == 0)
            {
                // found it...
                if (aGameVars[i].dwFlags & (GAMEVAR_FLAG_INTPTR|GAMEVAR_FLAG_SHORTPTR|GAMEVAR_FLAG_CHARPTR))
                {
                    //                 warning++;
                    //                 initprintf("%s:%d: warning: Internal gamevar '%s' cannot be redefined.\n",compilefile,line_number,label+(labelcnt<<6));
                    ReportError(-1);
                    initprintf("%s:%d: warning: cannot redefine internal gamevar `%s'.\n",compilefile,line_number,label+(labelcnt<<6));
                    return 0;
                }
                else if ((aGameVars[i].dwFlags & GAMEVAR_FLAG_DEFAULT) || (aGameVars[i].dwFlags & GAMEVAR_FLAG_SYSTEM))
                {
                    //Bsprintf(g_szBuf,"Replacing %s at %d",pszLabel,i);
                    //AddLog(g_szBuf);
                    //b=1;
                    // it's OK to replace
                    break;
                }
                else
                {
                    // it's a duplicate in error
                    warning++;
                    ReportError(WARNING_DUPLICATEDEFINITION);
                    return 0;
                }
            }
        }
    }
    if (i < MAXGAMEVARS)
    {
        // Set values
        if (aGameVars[i].dwFlags & GAMEVAR_FLAG_SYSTEM)
        {
            //if(b)
            //{
            //Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
            //AddLog(g_szBuf);
            //}
            // if existing is system, they only get to change default value....
            aGameVars[i].lValue=lValue;
            aGameVars[i].lDefault=lValue;
            aGameVars[i].bReset=0;
        }
        else
        {
            if (aGameVars[i].szLabel == NULL)
                aGameVars[i].szLabel=Bcalloc(MAXVARLABEL,sizeof(char));
            if (aGameVars[i].szLabel != pszLabel)
                Bstrcpy(aGameVars[i].szLabel,pszLabel);
            aGameVars[i].dwFlags=dwFlags;
            aGameVars[i].lValue=lValue;
            aGameVars[i].lDefault=lValue;
            aGameVars[i].bReset=0;
        }

        if (i==iGameVarCount)
        {
            // we're adding a new one.
            iGameVarCount++;
        }
        if (aGameVars[i].plValues && !(aGameVars[i].dwFlags & GAMEVAR_FLAG_SYSTEM))
        {
            // only free if not system
            Bfree(aGameVars[i].plValues);
            aGameVars[i].plValues=NULL;
        }
        if (aGameVars[i].dwFlags & GAMEVAR_FLAG_PERPLAYER)
        {
            if (!aGameVars[i].plValues)
                aGameVars[i].plValues=Bcalloc(MAXPLAYERS,sizeof(int));
            for (j=0;j<MAXPLAYERS;j++)
                aGameVars[i].plValues[j]=lValue;
        }
        else if (aGameVars[i].dwFlags & GAMEVAR_FLAG_PERACTOR)
        {
            if (!aGameVars[i].plValues)
                aGameVars[i].plValues=Bcalloc(MAXSPRITES,sizeof(int));
            for (j=0;j<MAXSPRITES;j++)
                aGameVars[i].plValues[j]=lValue;
        }
        return 1;
    }
    else
    {
        // no room to add...
        return 0;
    }
}

void ResetActorGameVars(int iActor)
{
    int i;
    //    OSD_Printf("resetting vars for actor %d\n",iActor);
    for (i=0;i<MAXGAMEVARS;i++)
        if ((aGameVars[i].dwFlags & GAMEVAR_FLAG_PERACTOR) && !(aGameVars[i].dwFlags & GAMEVAR_FLAG_NODEFAULT))
        {
            //            OSD_Printf("reset %s (%d) to %s (%d)\n",aGameVars[i].szLabel,aGameVars[i].plValues[iActor],aDefaultGameVars[i].szLabel,aDefaultGameVars[i].lValue);
            aGameVars[i].plValues[iActor]=aGameVars[i].lDefault;
        }
}

static int GetGameID(const char *szGameLabel)
{
    int i;

    for (i=0;i<iGameVarCount;i++)
    {
        if (aGameVars[i].szLabel != NULL)
        {
            if (Bstrcmp(szGameLabel, aGameVars[i].szLabel) == 0)
            {
                return i;
            }
        }
    }
    return -1;
}

int GetGameVarID(int id, int iActor, int iPlayer)
{
    int inv = 0;

    if (id == g_iThisActorID)
        return iActor;

    if (id<0 || id >= iGameVarCount)
    {
        if (id==MAXGAMEVARS)
        {
//            OSD_Printf("GetGameVarID(): reading gamevar constant\n");
            return(*insptr++);
        }
        if (id < MAXGAMEVARS+1+MAXGAMEARRAYS)
        {
            int index=0;
//            OSD_Printf("GetGameVarID(): reading from array\n");
            index=GetGameVarID(*insptr++,iActor,iPlayer);
            if ((index < aGameArrays[id-MAXGAMEVARS-1].size)&&(index>=0))
                inv =aGameArrays[id-MAXGAMEVARS-1].plValues[index];
            else
            {
                OSD_Printf("GetGameVarID(): invalid array index (%s[%d])\n",aGameArrays[id-MAXGAMEVARS-1].szLabel,index);
                return -1;
            }
            return(inv);
        }
        if (!(id&(MAXGAMEVARS<<1)))
        {
            OSD_Printf("GetGameVarID(): invalid gamevar ID (%d)\n",id);
            return -1;
        }

        inv = 1;
        id ^= (MAXGAMEVARS<<1);
    }

    if (aGameVars[id].dwFlags & GAMEVAR_FLAG_PERPLAYER)
    {
        // for the current player
        if (iPlayer >= 0 && iPlayer < MAXPLAYERS)
        {
            //Bsprintf(g_szBuf,"GetGameVarID(%d, %d, %d) returns %d\n",id,iActor,iPlayer, aGameVars[id].plValues[iPlayer]);
            //AddLog(g_szBuf);
            if (inv) return(-aGameVars[id].plValues[iPlayer]);
            return (aGameVars[id].plValues[iPlayer]);
        }

        if (inv) return(-aGameVars[id].lValue);
        return (aGameVars[id].lValue);
    }

    if (aGameVars[id].dwFlags & GAMEVAR_FLAG_PERACTOR)
    {
        // for the current actor
        if (iActor >= 0 && iActor <= MAXSPRITES)
        {
            if (inv) return(-aGameVars[id].plValues[iActor]);
            return (aGameVars[id].plValues[iActor]);
        }

        if (inv) return(-aGameVars[id].lValue);
        return (aGameVars[id].lValue);
    }

    if (aGameVars[id].dwFlags & GAMEVAR_FLAG_INTPTR)
    {
        if (inv) return(-(*((int*)aGameVars[id].lValue)));
        return(*((int*)aGameVars[id].lValue));
    }

    if (aGameVars[id].dwFlags & GAMEVAR_FLAG_SHORTPTR)
    {
        if (inv) return(-(*((short*)aGameVars[id].lValue)));
        return(*((short*)aGameVars[id].lValue));
    }

    if (aGameVars[id].dwFlags & GAMEVAR_FLAG_CHARPTR)
    {
        if (inv) return(-(*((char*)aGameVars[id].lValue)));
        return(*((char*)aGameVars[id].lValue));
    }

    if (inv) return(-aGameVars[id].lValue);
    return (aGameVars[id].lValue);
}

void SetGameArrayID(int id,int index, int lValue)
{
    if (id<0 || id >= iGameArrayCount || !((index < aGameArrays[id].size)&&(index>=0)))
    {
        OSD_Printf("SetGameVarID(): tried to set invalid array ID (%d) or index out of bounds from sprite %d (%d), player %d\n",id,g_i,sprite[g_i].picnum,g_p);
        return;
    }
    aGameArrays[id].plValues[index]=lValue;
}

void SetGameVarID(int id, int lValue, int iActor, int iPlayer)
{
    if (id<0 || id >= iGameVarCount)
    {
        OSD_Printf("SetGameVarID(): tried to set invalid gamevar ID (%d) from sprite %d (%d), player %d\n",id,g_i,sprite[g_i].picnum,g_p);
        return;
    }
    //Bsprintf(g_szBuf,"SGVI: %d ('%s') to %d for %d %d",id,aGameVars[id].szLabel,lValue,iActor,iPlayer);
    //AddLog(g_szBuf);
    if (aGameVars[id].dwFlags & GAMEVAR_FLAG_PERPLAYER)
    {
        if (iPlayer < 0 || iPlayer > MAXPLAYERS-1)
        {
            OSD_Printf("SetGameVarID(): invalid player (%d) for per-player gamevar %s from sprite %d (%d), player %d\n",iPlayer,aGameVars[id].szLabel,g_i,sprite[g_i].picnum,g_p);
            return;
        }
        // for the current player
        aGameVars[id].plValues[iPlayer]=lValue;
        return;
    }

    if (aGameVars[id].dwFlags & GAMEVAR_FLAG_PERACTOR)
    {
        if (iActor < 0 || iActor > MAXSPRITES-1)
        {
            OSD_Printf("SetGameVarID(): invalid sprite (%d) for per-actor gamevar %s from sprite %d (%d), player %d\n",iActor,aGameVars[id].szLabel,g_i,sprite[g_i].picnum,g_p);
            return;
        }
        // for the current actor
        aGameVars[id].plValues[iActor]=lValue;
        return;
    }

    if (aGameVars[id].dwFlags & GAMEVAR_FLAG_INTPTR)
    {
        // set the value at pointer
        *((int*)aGameVars[id].lValue)=(int)lValue;
        return;
    }

    if (aGameVars[id].dwFlags & GAMEVAR_FLAG_SHORTPTR)
    {
        // set the value at pointer
        *((short*)aGameVars[id].lValue)=(short)lValue;
        return;
    }

    if (aGameVars[id].dwFlags & GAMEVAR_FLAG_CHARPTR)
    {
        // set the value at pointer
        *((char*)aGameVars[id].lValue)=(char)lValue;
        return;
    }

    aGameVars[id].lValue=lValue;
}

int GetGameVar(const char *szGameLabel, int lDefault, int iActor, int iPlayer)
{
    int i=0;
    for (;i<iGameVarCount;i++)
    {
        if (aGameVars[i].szLabel != NULL)
        {
            if (Bstrcmp(szGameLabel, aGameVars[i].szLabel) == 0)
            {
                return GetGameVarID(i, iActor, iPlayer);
            }
        }
    }
    return lDefault;
}

static intptr_t *GetGameValuePtr(const char *szGameLabel)
{
    int i;
    for (i=0;i<iGameVarCount;i++)
    {
        if (aGameVars[i].szLabel != NULL)
        {
            if (Bstrcmp(szGameLabel, aGameVars[i].szLabel) == 0)
            {
                if (aGameVars[i].dwFlags & (GAMEVAR_FLAG_PERACTOR | GAMEVAR_FLAG_PERPLAYER))
                {
                    if (!aGameVars[i].plValues)
                    {
                        OSD_Printf("GetGameValuePtr(): INTERNAL ERROR: NULL array !!!\n");
                    }
                    return aGameVars[i].plValues;
                }
                return &(aGameVars[i].lValue);
            }
        }
    }
    //Bsprintf(g_szBuf,"Could not find value '%s'\n",szGameLabel);
    //AddLog(g_szBuf);
    return NULL;
}

void ResetSystemDefaults(void)
{
    // call many times...

    int i,j;
    char aszBuf[64];

    //AddLog("ResetWeaponDefaults");

    for (j=0;j<MAXPLAYERS;j++)
    {
        for (i=0;i<MAX_WEAPONS;i++)
        {
            Bsprintf(aszBuf,"WEAPON%d_CLIP",i);
            aplWeaponClip[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_RELOAD",i);
            aplWeaponReload[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",i);
            aplWeaponFireDelay[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",i);
            aplWeaponTotalTime[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",i);
            aplWeaponHoldDelay[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_FLAGS",i);
            aplWeaponFlags[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_SHOOTS",i);
            aplWeaponShoots[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",i);
            aplWeaponSpawnTime[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_SPAWN",i);
            aplWeaponSpawn[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",i);
            aplWeaponShotsPerBurst[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",i);
            aplWeaponWorksLike[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",i);
            aplWeaponInitialSound[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",i);
            aplWeaponFireSound[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",i);
            aplWeaponSound2Time[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",i);
            aplWeaponSound2Sound[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",i);
            aplWeaponReloadSound1[i][j]=GetGameVar(aszBuf,0, -1, j);
            Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",i);
            aplWeaponReloadSound2[i][j]=GetGameVar(aszBuf,0, -1, j);
        }
    }

    g_iReturnVarID=GetGameID("RETURN");
    g_iWeaponVarID=GetGameID("WEAPON");
    g_iWorksLikeVarID=GetGameID("WORKSLIKE");
    g_iZRangeVarID=GetGameID("ZRANGE");
    g_iAngRangeVarID=GetGameID("ANGRANGE");
    g_iAimAngleVarID=GetGameID("AUTOAIMANGLE");
    g_iLoTagID=GetGameID("LOTAG");
    g_iHiTagID=GetGameID("HITAG");
    g_iTextureID=GetGameID("TEXTURE");
    g_iThisActorID=GetGameID("THISACTOR");

    Bmemcpy(&projectile,&defaultprojectile,sizeof(projectile));

    //AddLog("EOF:ResetWeaponDefaults");
}

static void AddSystemVars()
{
    // only call ONCE
    char aszBuf[64];

    //AddLog("AddSystemVars");

    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",KNEE_WEAPON);
    AddGameVar(aszBuf, KNEE_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",KNEE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",KNEE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",KNEE_WEAPON);
    AddGameVar(aszBuf, 7, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",KNEE_WEAPON);
    AddGameVar(aszBuf, 14, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",KNEE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",KNEE_WEAPON);
    AddGameVar(aszBuf, WEAPON_FLAG_NOVISIBLE | WEAPON_FLAG_RANDOMRESTART | WEAPON_FLAG_AUTOMATIC, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",KNEE_WEAPON);
    AddGameVar(aszBuf, KNEE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",KNEE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",KNEE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",KNEE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",KNEE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",KNEE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",KNEE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",KNEE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",KNEE_WEAPON);
    AddGameVar(aszBuf, EJECT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",KNEE_WEAPON);
    AddGameVar(aszBuf, INSERT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",PISTOL_WEAPON);
    AddGameVar(aszBuf, PISTOL_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",PISTOL_WEAPON);
    AddGameVar(aszBuf, NAM?20:12, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",PISTOL_WEAPON);
    AddGameVar(aszBuf, NAM?50:27, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",PISTOL_WEAPON);
    AddGameVar(aszBuf, 2, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",PISTOL_WEAPON);
    AddGameVar(aszBuf, 5, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",PISTOL_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",PISTOL_WEAPON);
    AddGameVar(aszBuf, NAM?WEAPON_FLAG_HOLSTER_CLEARS_CLIP:0 | WEAPON_FLAG_RELOAD_TIMING, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",PISTOL_WEAPON);
    AddGameVar(aszBuf, SHOTSPARK1, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",PISTOL_WEAPON);
    AddGameVar(aszBuf, 2, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",PISTOL_WEAPON);
    AddGameVar(aszBuf, SHELL, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",PISTOL_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",PISTOL_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",PISTOL_WEAPON);
    AddGameVar(aszBuf, PISTOL_FIRE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",PISTOL_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",PISTOL_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",PISTOL_WEAPON);
    AddGameVar(aszBuf, EJECT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",PISTOL_WEAPON);
    AddGameVar(aszBuf, INSERT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, SHOTGUN_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, 13, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, 4, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, 30, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, WEAPON_FLAG_CHECKATRELOAD, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, SHOTGUN, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, 24, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, SHOTGUNSHELL, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, 7, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, SHOTGUN_FIRE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, 15, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, SHOTGUN_COCK, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, EJECT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",SHOTGUN_WEAPON);
    AddGameVar(aszBuf, INSERT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, CHAINGUN_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, 3, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, 12, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, 3, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, WEAPON_FLAG_AUTOMATIC | WEAPON_FLAG_FIREEVERYTHIRD | WEAPON_FLAG_AMMOPERSHOT | WEAPON_FLAG_SPAWNTYPE3 | WEAPON_FLAG_RESET, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, CHAINGUN, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, 1, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, SHELL, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, CHAINGUN_FIRE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, EJECT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",CHAINGUN_WEAPON);
    AddGameVar(aszBuf, INSERT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",RPG_WEAPON);
    AddGameVar(aszBuf, RPG_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",RPG_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",RPG_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",RPG_WEAPON);
    AddGameVar(aszBuf, 4, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",RPG_WEAPON);
    AddGameVar(aszBuf, 20, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",RPG_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",RPG_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",RPG_WEAPON);
    AddGameVar(aszBuf, RPG, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",RPG_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",RPG_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",RPG_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",RPG_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",RPG_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",RPG_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",RPG_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",RPG_WEAPON);
    AddGameVar(aszBuf, EJECT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",RPG_WEAPON);
    AddGameVar(aszBuf, INSERT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, HANDBOMB_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, 30, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, 6, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, 19, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, 12, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, WEAPON_FLAG_THROWIT, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, HEAVYHBOMB, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, EJECT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",HANDBOMB_WEAPON);
    AddGameVar(aszBuf, INSERT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",SHRINKER_WEAPON);
    AddGameVar(aszBuf, SHRINKER_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",SHRINKER_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",SHRINKER_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",SHRINKER_WEAPON);
    AddGameVar(aszBuf, 10, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",SHRINKER_WEAPON);
    AddGameVar(aszBuf, NAM?30:12, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",SHRINKER_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",SHRINKER_WEAPON);
    AddGameVar(aszBuf, WEAPON_FLAG_GLOWS, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",SHRINKER_WEAPON);
    AddGameVar(aszBuf, SHRINKER, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",SHRINKER_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",SHRINKER_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",SHRINKER_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",SHRINKER_WEAPON);
    AddGameVar(aszBuf, SHRINKER_FIRE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",SHRINKER_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",SHRINKER_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",SHRINKER_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",SHRINKER_WEAPON);
    AddGameVar(aszBuf, EJECT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",SHRINKER_WEAPON);
    AddGameVar(aszBuf, INSERT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, DEVISTATOR_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, 3, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, 6, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, 5, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, WEAPON_FLAG_FIREEVERYOTHER | WEAPON_FLAG_AMMOPERSHOT, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, RPG, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, 2, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, CAT_FIRE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, EJECT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",DEVISTATOR_WEAPON);
    AddGameVar(aszBuf, INSERT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, TRIPBOMB_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, 16, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, 3, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, 16, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, 7, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, WEAPON_FLAG_STANDSTILL | WEAPON_FLAG_CHECKATRELOAD, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, HANDHOLDINGLASER, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, EJECT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",TRIPBOMB_WEAPON);
    AddGameVar(aszBuf, INSERT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",FREEZE_WEAPON);
    AddGameVar(aszBuf, FREEZE_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",FREEZE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",FREEZE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",FREEZE_WEAPON);
    AddGameVar(aszBuf, 3, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",FREEZE_WEAPON);
    AddGameVar(aszBuf, 5, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",FREEZE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",FREEZE_WEAPON);
    AddGameVar(aszBuf, WEAPON_FLAG_RESET, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",FREEZE_WEAPON);
    AddGameVar(aszBuf, FREEZEBLAST, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",FREEZE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",FREEZE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",FREEZE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",FREEZE_WEAPON);
    AddGameVar(aszBuf, CAT_FIRE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",FREEZE_WEAPON);
    AddGameVar(aszBuf, CAT_FIRE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",FREEZE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",FREEZE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",FREEZE_WEAPON);
    AddGameVar(aszBuf, EJECT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",FREEZE_WEAPON);
    AddGameVar(aszBuf, INSERT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    /////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, HANDREMOTE_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 10, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 2, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 10, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, WEAPON_FLAG_BOMB_TRIGGER | WEAPON_FLAG_NOVISIBLE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, EJECT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",HANDREMOTE_WEAPON);
    AddGameVar(aszBuf, INSERT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    ///////////////////////////////////////////////////////
    Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",GROW_WEAPON);
    AddGameVar(aszBuf, GROW_WEAPON, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_CLIP",GROW_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOAD",GROW_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",GROW_WEAPON);
    AddGameVar(aszBuf, 3, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",GROW_WEAPON);
    AddGameVar(aszBuf, NAM?30:5, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",GROW_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FLAGS",GROW_WEAPON);
    AddGameVar(aszBuf, WEAPON_FLAG_GLOWS, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOOTS",GROW_WEAPON);
    AddGameVar(aszBuf, GROWSPARK, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",GROW_WEAPON);
    AddGameVar(aszBuf, NAM?2:0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SPAWN",GROW_WEAPON);
    AddGameVar(aszBuf, NAM?SHELL:0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",GROW_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",GROW_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",GROW_WEAPON);
    AddGameVar(aszBuf, NAM?0:EXPANDERSHOOT, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",GROW_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",GROW_WEAPON);
    AddGameVar(aszBuf, 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",GROW_WEAPON);
    AddGameVar(aszBuf, EJECT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",GROW_WEAPON);
    AddGameVar(aszBuf, INSERT_CLIP, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    AddGameVar("GRENADE_LIFETIME", NAM_GRENADE_LIFETIME, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    AddGameVar("GRENADE_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    AddGameVar("STICKYBOMB_LIFETIME", NAM_GRENADE_LIFETIME, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    AddGameVar("STICKYBOMB_LIFETIME_VAR", NAM_GRENADE_LIFETIME_VAR, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    AddGameVar("TRIPBOMB_CONTROL", TRIPBOMB_TRIPWIRE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    AddGameVar("PIPEBOMB_CONTROL", NAM?PIPEBOMB_TIMER:PIPEBOMB_REMOTE, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);

    AddGameVar("RESPAWN_MONSTERS", (int)&ud.respawn_monsters,GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR);
    AddGameVar("RESPAWN_ITEMS",(int)&ud.respawn_items, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR);
    AddGameVar("RESPAWN_INVENTORY",(int)&ud.respawn_inventory, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR);
    AddGameVar("MONSTERS_OFF",(int)&ud.monsters_off, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR);
    AddGameVar("MARKER",(int)&ud.marker, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR);
    AddGameVar("FFIRE",(int)&ud.ffire, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR);
    AddGameVar("LEVEL",(int)&ud.level_number, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_READONLY);
    AddGameVar("VOLUME",(int)&ud.volume_number, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_READONLY);

    AddGameVar("COOP",(int)&ud.coop, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR);
    AddGameVar("MULTIMODE",(int)&ud.multimode, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR);

    AddGameVar("WEAPON", 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYSTEM);
    AddGameVar("WORKSLIKE", 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYSTEM);
    AddGameVar("RETURN", 0, GAMEVAR_FLAG_SYSTEM);
    AddGameVar("ZRANGE", 4, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    AddGameVar("ANGRANGE", 18, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    AddGameVar("AUTOAIMANGLE", 0, GAMEVAR_FLAG_PERPLAYER | GAMEVAR_FLAG_SYSTEM);
    AddGameVar("LOTAG", 0, GAMEVAR_FLAG_SYSTEM);
    AddGameVar("HITAG", 0, GAMEVAR_FLAG_SYSTEM);
    AddGameVar("TEXTURE", 0, GAMEVAR_FLAG_SYSTEM);
    AddGameVar("THISACTOR", 0, GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYSTEM);
    AddGameVar("myconnectindex", (int)&myconnectindex, GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("screenpeek", (int)&screenpeek, GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("currentweapon",(int)&g_currentweapon, GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("gs",(int)&g_gs, GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("looking_arc",(int)&g_looking_arc, GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("gun_pos",(int)&g_gun_pos, GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("weapon_xoffset",(int)&g_weapon_xoffset, GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("weaponcount",(int)&g_kb, GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("looking_angSR1",(int)&g_looking_angSR1, GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("xdim",(int)&xdim, GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("ydim",(int)&ydim, GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("windowx1",(int)&windowx1, GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("windowx2",(int)&windowx2, GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("windowy1",(int)&windowy1, GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("windowy2",(int)&windowy2, GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("totalclock",(int)&totalclock, GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("lastvisinc",(int)&lastvisinc, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("numsectors",(int)&numsectors, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_READONLY);
    AddGameVar("numplayers",(int)&numplayers, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_READONLY);
    AddGameVar("viewingrange",(int)&viewingrange, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("yxaspect",(int)&yxaspect, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("gravitationalconstant",(int)&gc, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR);
    AddGameVar("gametype_flags",(int)&gametype_flags[ud.coop], GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR);
    AddGameVar("framerate",(int)&framerate, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("CLIPMASK0", CLIPMASK0, GAMEVAR_FLAG_SYSTEM|GAMEVAR_FLAG_READONLY);
    AddGameVar("CLIPMASK1", CLIPMASK1, GAMEVAR_FLAG_SYSTEM|GAMEVAR_FLAG_READONLY);

    AddGameVar("camerax",(int)&ud.camerax, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("cameray",(int)&ud.cameray, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("cameraz",(int)&ud.cameraz, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("cameraang",(int)&ud.cameraang, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SHORTPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("camerahoriz",(int)&ud.camerahoriz, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SHORTPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("camerasect",(int)&ud.camerasect, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SHORTPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("cameradist",(int)&cameradist, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("cameraclock",(int)&cameraclock, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYNCCHECK);

    AddGameVar("myx",(int)&myx, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("myy",(int)&myy, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("myz",(int)&myz, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("omyx",(int)&omyx, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("omyy",(int)&omyy, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("omyz",(int)&omyz, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("myxvel",(int)&myxvel, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("myyvel",(int)&myyvel, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("myzvel",(int)&myzvel, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR | GAMEVAR_FLAG_SYNCCHECK);

    AddGameVar("myhoriz",(int)&myhoriz, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SHORTPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("myhorizoff",(int)&myhorizoff, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SHORTPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("omyhoriz",(int)&omyhoriz, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SHORTPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("omyhorizoff",(int)&omyhorizoff, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SHORTPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("myang",(int)&myang, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SHORTPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("omyang",(int)&omyang, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SHORTPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("mycursectnum",(int)&mycursectnum, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SHORTPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("myjumpingcounter",(int)&myjumpingcounter, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SHORTPTR | GAMEVAR_FLAG_SYNCCHECK);

    AddGameVar("myjumpingtoggle",(int)&myjumpingtoggle, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_CHARPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("myonground",(int)&myonground, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_CHARPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("myhardlanding",(int)&myhardlanding, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_CHARPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("myreturntocenter",(int)&myreturntocenter, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_CHARPTR | GAMEVAR_FLAG_SYNCCHECK);

    AddGameVar("display_mirror",(int)&display_mirror, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_CHARPTR | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("randomseed",(int)&randomseed, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_INTPTR);
}

void InitGameVars(void)
{
    // only call ONCE

    //  initprintf("Initializing game variables\n");
    //AddLog("InitGameVars");

    ClearGameVars();
    AddSystemVars();
    InitGameVarPointers();
    ResetSystemDefaults();
}

void InitGameVarPointers(void)
{
    int i;
    char aszBuf[64];
    // called from game Init AND when level is loaded...

    //AddLog("InitGameVarPointers");

    for (i=0;i<MAX_WEAPONS;i++)
    {
        Bsprintf(aszBuf,"WEAPON%d_CLIP",i);
        aplWeaponClip[i]=GetGameValuePtr(aszBuf);
        if (!aplWeaponClip[i])
        {
            initprintf("ERROR: NULL Weapon\n");
            exit(0);
        }
        Bsprintf(aszBuf,"WEAPON%d_RELOAD",i);
        aplWeaponReload[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_FIREDELAY",i);
        aplWeaponFireDelay[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_TOTALTIME",i);
        aplWeaponTotalTime[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_HOLDDELAY",i);
        aplWeaponHoldDelay[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_FLAGS",i);
        aplWeaponFlags[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SHOOTS",i);
        aplWeaponShoots[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SPAWNTIME",i);
        aplWeaponSpawnTime[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SPAWN",i);
        aplWeaponSpawn[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SHOTSPERBURST",i);
        aplWeaponShotsPerBurst[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_WORKSLIKE",i);
        aplWeaponWorksLike[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_INITIALSOUND",i);
        aplWeaponInitialSound[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_FIRESOUND",i);
        aplWeaponFireSound[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SOUND2TIME",i);
        aplWeaponSound2Time[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_SOUND2SOUND",i);
        aplWeaponSound2Sound[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND1",i);
        aplWeaponReloadSound1[i]=GetGameValuePtr(aszBuf);
        Bsprintf(aszBuf,"WEAPON%d_RELOADSOUND2",i);
        aplWeaponReloadSound2[i]=GetGameValuePtr(aszBuf);
    }
}

static void ResetPointerVars(void)
{
    aGameVars[GetGameID("RESPAWN_MONSTERS")].lValue = (int)&ud.respawn_monsters;
    aGameVars[GetGameID("RESPAWN_ITEMS")].lValue = (int)&ud.respawn_items;
    aGameVars[GetGameID("RESPAWN_INVENTORY")].lValue = (int)&ud.respawn_inventory;
    aGameVars[GetGameID("MONSTERS_OFF")].lValue = (int)&ud.monsters_off;
    aGameVars[GetGameID("MARKER")].lValue = (int)&ud.marker;
    aGameVars[GetGameID("FFIRE")].lValue = (int)&ud.ffire;
    aGameVars[GetGameID("LEVEL")].lValue = (int)&ud.level_number;
    aGameVars[GetGameID("VOLUME")].lValue = (int)&ud.volume_number;

    aGameVars[GetGameID("COOP")].lValue = (int)&ud.coop;
    aGameVars[GetGameID("MULTIMODE")].lValue = (int)&ud.multimode;

    aGameVars[GetGameID("myconnectindex")].lValue = (int)&myconnectindex;
    aGameVars[GetGameID("screenpeek")].lValue = (int)&screenpeek;
    aGameVars[GetGameID("currentweapon")].lValue = (int)&g_currentweapon;
    aGameVars[GetGameID("gs")].lValue = (int)&g_gs;
    aGameVars[GetGameID("looking_arc")].lValue = (int)&g_looking_arc;
    aGameVars[GetGameID("gun_pos")].lValue = (int)&g_gun_pos;
    aGameVars[GetGameID("weapon_xoffset")].lValue = (int)&g_weapon_xoffset;
    aGameVars[GetGameID("weaponcount")].lValue = (int)&g_kb;
    aGameVars[GetGameID("looking_angSR1")].lValue = (int)&g_looking_angSR1;
    aGameVars[GetGameID("xdim")].lValue = (int)&xdim;
    aGameVars[GetGameID("ydim")].lValue = (int)&ydim;
    aGameVars[GetGameID("windowx1")].lValue = (int)&windowx1;
    aGameVars[GetGameID("windowx2")].lValue = (int)&windowx2;
    aGameVars[GetGameID("windowy1")].lValue = (int)&windowy1;
    aGameVars[GetGameID("windowy2")].lValue = (int)&windowy2;
    aGameVars[GetGameID("totalclock")].lValue = (int)&totalclock;
    aGameVars[GetGameID("lastvisinc")].lValue = (int)&lastvisinc;
    aGameVars[GetGameID("numsectors")].lValue = (int)&numsectors;
    aGameVars[GetGameID("numplayers")].lValue = (int)&numplayers;
    aGameVars[GetGameID("viewingrange")].lValue = (int)&viewingrange;
    aGameVars[GetGameID("yxaspect")].lValue = (int)&yxaspect;
    aGameVars[GetGameID("gravitationalconstant")].lValue = (int)&gc;
    aGameVars[GetGameID("gametype_flags")].lValue = (int)&gametype_flags[ud.coop];
    aGameVars[GetGameID("framerate")].lValue = (int)&framerate;

    aGameVars[GetGameID("camerax")].lValue = (int)&ud.camerax;
    aGameVars[GetGameID("cameray")].lValue = (int)&ud.cameray;
    aGameVars[GetGameID("cameraz")].lValue = (int)&ud.cameraz;
    aGameVars[GetGameID("cameraang")].lValue = (int)&ud.cameraang;
    aGameVars[GetGameID("camerahoriz")].lValue = (int)&ud.camerahoriz;
    aGameVars[GetGameID("camerasect")].lValue = (int)&ud.camerasect;
    aGameVars[GetGameID("cameradist")].lValue = (int)&cameradist;
    aGameVars[GetGameID("cameraclock")].lValue = (int)&cameraclock;

    aGameVars[GetGameID("myx")].lValue = (int)&myx;
    aGameVars[GetGameID("myy")].lValue = (int)&myy;
    aGameVars[GetGameID("myz")].lValue = (int)&myz;
    aGameVars[GetGameID("omyx")].lValue = (int)&omyx;
    aGameVars[GetGameID("omyy")].lValue = (int)&omyy;
    aGameVars[GetGameID("omyz")].lValue = (int)&omyz;
    aGameVars[GetGameID("myxvel")].lValue = (int)&myxvel;
    aGameVars[GetGameID("myyvel")].lValue = (int)&myyvel;
    aGameVars[GetGameID("myzvel")].lValue = (int)&myzvel;

    aGameVars[GetGameID("myhoriz")].lValue = (int)&myhoriz;
    aGameVars[GetGameID("myhorizoff")].lValue = (int)&myhorizoff;
    aGameVars[GetGameID("omyhoriz")].lValue = (int)&omyhoriz;
    aGameVars[GetGameID("omyhorizoff")].lValue = (int)&omyhorizoff;
    aGameVars[GetGameID("myang")].lValue = (int)&myang;
    aGameVars[GetGameID("omyang")].lValue = (int)&omyang;
    aGameVars[GetGameID("mycursectnum")].lValue = (int)&mycursectnum;
    aGameVars[GetGameID("myjumpingcounter")].lValue = (int)&myjumpingcounter;

    aGameVars[GetGameID("myjumpingtoggle")].lValue = (int)&myjumpingtoggle;
    aGameVars[GetGameID("myonground")].lValue = (int)&myonground;
    aGameVars[GetGameID("myhardlanding")].lValue = (int)&myhardlanding;
    aGameVars[GetGameID("myreturntocenter")].lValue = (int)&myreturntocenter;

    aGameVars[GetGameID("display_mirror")].lValue = (int)&display_mirror;
    aGameVars[GetGameID("randomseed")].lValue = (int)&randomseed;
}
