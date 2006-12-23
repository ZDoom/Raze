//-------------------------------------------------------------------------
/*
Copyright (C) 2005 - EDuke32 team

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

static void FreeGameVars(void)
{
    // call this function as many times as needed.
    int i;
    //  AddLog("FreeGameVars");
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
    }
    iGameVarCount=0;
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
        if (aDefaultGameVars[i].szLabel)
            Bfree(aDefaultGameVars[i].szLabel);
        aGameVars[i].szLabel=NULL;
        aDefaultGameVars[i].szLabel=NULL;
        aGameVars[i].dwFlags=0;

        if (aGameVars[i].plValues)
            Bfree(aGameVars[i].plValues);
        aGameVars[i].plValues=NULL;
    }
    iGameVarCount=0;
    return;
}

int ReadGameVars(long fil)
{
    int i;
    long l;

    //     AddLog("Reading gamevars from savegame");

    FreeGameVars(); // nuke 'em from orbit, it's the only way to be sure...
    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);

    if (kdfread(&iGameVarCount,sizeof(iGameVarCount),1,fil) != 1) goto corrupt;
    for (i=0;i<iGameVarCount;i++)
    {
        if (kdfread(&(aGameVars[i]),sizeof(MATTGAMEVAR),1,fil) != 1) goto corrupt;
        aGameVars[i].szLabel=Bcalloc(MAXVARLABEL,sizeof(char));
        if (kdfread(aGameVars[i].szLabel,sizeof(char) * MAXVARLABEL, 1, fil) != 1) goto corrupt;
    }
    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);
    for (i=0;i<iGameVarCount;i++)
    {
        if (aGameVars[i].dwFlags & GAMEVAR_FLAG_PERPLAYER)
            aGameVars[i].plValues=Bcalloc(MAXPLAYERS,sizeof(long));
        else if (aGameVars[i].dwFlags & GAMEVAR_FLAG_PERACTOR)
            aGameVars[i].plValues=Bcalloc(MAXSPRITES,sizeof(long));
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
            //Bsprintf(g_szBuf,"Reading value array for %s (%d)",aGameVars[i].szLabel,sizeof(long) * MAXPLAYERS);
            //AddLog(g_szBuf);
            if (kdfread(aGameVars[i].plValues,sizeof(long) * MAXPLAYERS, 1, fil) != 1) goto corrupt;
        }
        else if (aGameVars[i].dwFlags & GAMEVAR_FLAG_PERACTOR)
        {
            //Bsprintf(g_szBuf,"Reading value array for %s (%d)",aGameVars[i].szLabel,sizeof(long) * MAXSPRITES);
            //AddLog(g_szBuf);
            if (kdfread(&aGameVars[i].plValues[0],sizeof(long), MAXSPRITES, fil) != MAXSPRITES) goto corrupt;
        }
        // else nothing 'extra...'
    }

    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);
    if (kdfread(apScriptGameEvent,sizeof(apScriptGameEvent),1,fil) != 1) goto corrupt;
    for (i=0;i<MAXGAMEEVENTS;i++)
        if (apScriptGameEvent[i])
        {
            l = (long)apScriptGameEvent[i]+(long)&script[0];
            apScriptGameEvent[i] = (long *)l;
        }

    //  Bsprintf(g_szBuf,"CP:%s %d",__FILE__,__LINE__);
    //  AddLog(g_szBuf);
    if (kdfread(&l,sizeof(l),1,fil) != 1) goto corrupt;
    if (kdfread(g_szBuf,l,1,fil) != 1) goto corrupt;
    g_szBuf[l]=0;
    AddLog(g_szBuf);

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
    long l;

    //   AddLog("Saving Game Vars to File");
    dfwrite(&iGameVarCount,sizeof(iGameVarCount),1,fil);

    for (i=0;i<iGameVarCount;i++)
    {
        dfwrite(&(aGameVars[i]),sizeof(MATTGAMEVAR),1,fil);
        dfwrite(aGameVars[i].szLabel,sizeof(char) * MAXVARLABEL, 1, fil);
    }

    //     dfwrite(&aGameVars,sizeof(aGameVars),1,fil);

    for (i=0;i<iGameVarCount;i++)
    {
        if (aGameVars[i].dwFlags & GAMEVAR_FLAG_PERPLAYER)
        {
            //Bsprintf(g_szBuf,"Writing value array for %s (%d)",aGameVars[i].szLabel,sizeof(long) * MAXPLAYERS);
            //AddLog(g_szBuf);
            dfwrite(aGameVars[i].plValues,sizeof(long) * MAXPLAYERS, 1, fil);
        }
        else if (aGameVars[i].dwFlags & GAMEVAR_FLAG_PERACTOR)
        {
            //Bsprintf(g_szBuf,"Writing value array for %s (%d)",aGameVars[i].szLabel,sizeof(long) * MAXSPRITES);
            //AddLog(g_szBuf);
            dfwrite(&aGameVars[i].plValues[0],sizeof(long), MAXSPRITES, fil);
        }
        // else nothing 'extra...'
    }

    for (i=0;i<MAXGAMEEVENTS;i++)
        if (apScriptGameEvent[i])
        {
            l = (long)apScriptGameEvent[i]-(long)&script[0];
            apScriptGameEvent[i] = (long *)l;
        }
    dfwrite(apScriptGameEvent,sizeof(apScriptGameEvent),1,fil);
    for (i=0;i<MAXGAMEEVENTS;i++)
        if (apScriptGameEvent[i])
        {
            l = (long)apScriptGameEvent[i]+(long)&script[0];
            apScriptGameEvent[i] = (long *)l;
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

            if (aGameVars[i].dwFlags & (GAMEVAR_FLAG_PLONG))
                fprintf(fp,"%ld",*((long*)aGameVars[i].lValue));
            else
                fprintf(fp,"%ld",aGameVars[i].lValue);
            if (aGameVars[i].dwFlags & (GAMEVAR_FLAG_PERPLAYER))
                fprintf(fp," GAMEVAR_FLAG_PERPLAYER");
            else if (aGameVars[i].dwFlags & (GAMEVAR_FLAG_PERACTOR))
                fprintf(fp," GAMEVAR_FLAG_PERACTOR");
            else
                fprintf(fp," %ld",aGameVars[i].dwFlags & (GAMEVAR_FLAG_USER_MASK));
            fprintf(fp," // ");
            if (aGameVars[i].dwFlags & (GAMEVAR_FLAG_SYSTEM))
                fprintf(fp," (system)");
            if (aGameVars[i].dwFlags & (GAMEVAR_FLAG_PLONG))
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
        //Bsprintf(g_szBuf,"Resetting %d: '%s' to %ld",i,aDefaultGameVars[i].szLabel,
        //      aDefaultGameVars[i].lValue
        //     );
        //AddLog(g_szBuf);
        if (aDefaultGameVars[i].szLabel != NULL)
            AddGameVar(aDefaultGameVars[i].szLabel,aDefaultGameVars[i].lValue,aDefaultGameVars[i].dwFlags);
    }
}

int AddGameVar(const char *pszLabel, long lValue, unsigned long dwFlags)
{
    int i, j;

    //Bsprintf(g_szBuf,"AddGameVar(%s, %d, %X)",pszLabel, lValue, dwFlags);
    //AddLog(g_szBuf);

    if (Bstrlen(pszLabel) > (MAXVARLABEL-1))
    {
        error++;
        initprintf("%s:%ld: error: variable name `%s' exceeds limit of %d characters.\n",compilefile,line_number,pszLabel, MAXVARLABEL);
        return 0;
    }
    for (i=0;i<iGameVarCount;i++)
    {
        if (aGameVars[i].szLabel != NULL)
        {
            if (Bstrcmp(pszLabel,aGameVars[i].szLabel) == 0)
            {
                // found it...
                if (aGameVars[i].dwFlags & GAMEVAR_FLAG_PLONG)
                {
                    //                 warning++;
                    //                 initprintf("%s:%ld: warning: Internal gamevar '%s' cannot be redefined.\n",compilefile,line_number,label+(labelcnt<<6));
                    ReportError(-1);
                    initprintf("%s:%ld: warning: cannot redefine internal gamevar `%s'.\n",compilefile,line_number,label+(labelcnt<<6));
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
            aDefaultGameVars[i].lValue=lValue;
        }
        else
        {
            if (aGameVars[i].szLabel == NULL)
                aGameVars[i].szLabel=Bcalloc(MAXVARLABEL,sizeof(char));
            Bstrcpy(aGameVars[i].szLabel,pszLabel);
            aGameVars[i].dwFlags=dwFlags;
            aGameVars[i].lValue=lValue;
            if (aDefaultGameVars[i].szLabel == NULL)
            {
                aDefaultGameVars[i].szLabel=Bcalloc(MAXVARLABEL,sizeof(char));
                Bstrcpy(aDefaultGameVars[i].szLabel,pszLabel);
            }
            aDefaultGameVars[i].dwFlags=dwFlags;
            aDefaultGameVars[i].lValue=lValue;
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
                aGameVars[i].plValues=Bcalloc(MAXPLAYERS,sizeof(long));
            for (j=0;j<MAXPLAYERS;j++)
                aGameVars[i].plValues[j]=lValue;
        }
        else if (aGameVars[i].dwFlags & GAMEVAR_FLAG_PERACTOR)
        {
            if (!aGameVars[i].plValues)
                aGameVars[i].plValues=Bcalloc(MAXSPRITES,sizeof(long));
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
            aGameVars[i].plValues[iActor]=aDefaultGameVars[i].lValue;
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

long GetGameVarID(int id, int iActor, int iPlayer)
{
    int inv=0;
    
    if (id == g_iThisActorID)
        return iActor;
    
    if (id<0 || id >= iGameVarCount)
    {
        if (id==MAXGAMEVARS)
            return(*insptr++);
            
        if (!(id&(MAXGAMEVARS<<1)))
        {
            AddLog("GetGameVarID: Invalid Game ID");
            return -1;
        }
        
        inv=1;
        id ^= (MAXGAMEVARS<<1);
    }
    
    if (aGameVars[id].dwFlags & GAMEVAR_FLAG_PERPLAYER)
    {
        // for the current player
        if (iPlayer >=0 && iPlayer < MAXPLAYERS)
        {
            //Bsprintf(g_szBuf,"GetGameVarID(%d, %d, %d) returns %ld\n",id,iActor,iPlayer, aGameVars[id].plValues[iPlayer]);
            //AddLog(g_szBuf);
            if (inv) return (-aGameVars[id].plValues[iPlayer]);
            return (aGameVars[id].plValues[iPlayer]);
        }
        
        if (inv) return (-aGameVars[id].lValue);
        return (aGameVars[id].lValue);
    }
    
    if (aGameVars[id].dwFlags & GAMEVAR_FLAG_PERACTOR)
    {
        // for the current actor
        if (iActor >= 0 && iActor <=MAXSPRITES)
        {
            if (inv) return (-aGameVars[id].plValues[iActor]);
            return (aGameVars[id].plValues[iActor]);
        }
        
        if (inv) return (-aGameVars[id].lValue);
        return (aGameVars[id].lValue);
    }
    
    if (aGameVars[id].dwFlags & GAMEVAR_FLAG_PLONG)
    {
        if (inv) return (-(*((long*)aGameVars[id].lValue)));
        return(*((long*)aGameVars[id].lValue));
    }
    
    if (inv) return (-aGameVars[id].lValue);
    return (aGameVars[id].lValue);
}

void SetGameVarID(int id, long lValue, int iActor, int iPlayer)
{
    if (id<0 || id >= iGameVarCount)
    {
        AddLog("Invalid Game ID");
        return;
    }
    //Bsprintf(g_szBuf,"SGVI: %d ('%s') to %ld for %d %d",id,aGameVars[id].szLabel,lValue,iActor,iPlayer);
    //AddLog(g_szBuf);
    if ((aGameVars[id].dwFlags & GAMEVAR_FLAG_PERPLAYER) && (iPlayer != -1))
    {
        // for the current player
        aGameVars[id].plValues[iPlayer]=lValue;
        return;
    }
    
    if ((aGameVars[id].dwFlags & GAMEVAR_FLAG_PERACTOR) && (iActor != -1))
    {
        // for the current actor
        aGameVars[id].plValues[iActor]=lValue;
        return;
    }
    
    if (aGameVars[id].dwFlags & GAMEVAR_FLAG_PLONG)
    {
        // set the value at pointer
        *((long*)aGameVars[id].lValue)=lValue;
        return;
    }

    aGameVars[id].lValue=lValue;
}

long GetGameVar(const char *szGameLabel, long lDefault, int iActor, int iPlayer)
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

static long *GetGameValuePtr(const char *szGameLabel)
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
                        AddLog("INTERNAL ERROR: NULL array !!!");
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

    //  for(i=0;i<MAXTILES;i++)
    //      projectile[i] = defaultprojectile[i];

    Bmemcpy(&projectile,&defaultprojectile,sizeof(defaultprojectile));

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

    AddGameVar("RESPAWN_MONSTERS", (long)&ud.respawn_monsters,GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);
    AddGameVar("RESPAWN_ITEMS",(long)&ud.respawn_items, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);
    AddGameVar("RESPAWN_INVENTORY",(long)&ud.respawn_inventory, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);
    AddGameVar("MONSTERS_OFF",(long)&ud.monsters_off, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);
    AddGameVar("MARKER",(long)&ud.marker, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);
    AddGameVar("FFIRE",(long)&ud.ffire, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);
    AddGameVar("LEVEL",(long)&ud.level_number, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_READONLY);
    AddGameVar("VOLUME",(long)&ud.volume_number, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_READONLY);

    AddGameVar("COOP",(long)&ud.coop, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);
    AddGameVar("MULTIMODE",(long)&ud.multimode, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);

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
    AddGameVar("myconnectindex", (long)&myconnectindex, GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("screenpeek", (long)&screenpeek, GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("currentweapon",(long)&g_currentweapon, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("gs",(long)&g_gs, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("looking_arc",(long)&g_looking_arc, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("gun_pos",(long)&g_gun_pos, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("weapon_xoffset",(long)&g_weapon_xoffset, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("weaponcount",(long)&g_kb, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("looking_angSR1",(long)&g_looking_angSR1, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("xdim",(long)&xdim, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("ydim",(long)&ydim, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("windowx1",(long)&windowx1, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("windowx2",(long)&windowx2, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("windowy1",(long)&windowy1, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("windowy2",(long)&windowy2, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("totalclock",(long)&totalclock, GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("lastvisinc",(long)&lastvisinc, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("numsectors",(long)&numsectors, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_READONLY);
    AddGameVar("numplayers",(long)&numplayers, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_READONLY);
    AddGameVar("viewingrange",(long)&viewingrange, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("yxaspect",(long)&yxaspect, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("gravitationalconstant",(long)&gc, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);
    AddGameVar("gametype_flags",(long)&gametype_flags[ud.coop], GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG);
    AddGameVar("framerate",(long)&framerate, GAMEVAR_FLAG_SYSTEM | GAMEVAR_FLAG_PLONG | GAMEVAR_FLAG_READONLY | GAMEVAR_FLAG_SYNCCHECK);
    AddGameVar("CLIPMASK0", CLIPMASK0, GAMEVAR_FLAG_SYSTEM|GAMEVAR_FLAG_READONLY);
    AddGameVar("CLIPMASK1", CLIPMASK1, GAMEVAR_FLAG_SYSTEM|GAMEVAR_FLAG_READONLY);
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

