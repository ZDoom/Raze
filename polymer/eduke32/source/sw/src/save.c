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

#define MAIN
#define QUIET
#include "build.h"

#include "keys.h"
#include "names2.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "lists.h"
#include "interp.h"

#include "net.h"
//#include "save.h"
#include "savedef.h"
#include "jsector.h"
#include "parent.h"
#include "reserve.h"

//#define FILE_TYPE 1
#include "mfile.h"

#include "fx_man.h"
#include "music.h"

#include "weapon.h"
#include "cache.h"
#include "colormap.h"
#include "player.h"

#include "saveable.h"

//void TimerFunc(task * Task);

/*
//////////////////////////////////////////////////////////////////////////////
TO DO


//////////////////////////////////////////////////////////////////////////////
*/

extern int lastUpdate;
extern uint8_t RedBookSong[40];
extern char UserMapName[80];
extern char LevelSong[16];
extern char SaveGameDescr[10][80];
extern int PlayClock;
extern short TotalKillable;
extern short LevelSecrets;
extern short Bunny_Count;
extern SWBOOL NewGame;
extern char CacheLastLevel[];
extern short PlayingLevel;
extern int GodMode;
extern int GameVersion;
//extern short Zombies;

extern SWBOOL serpwasseen;
extern SWBOOL sumowasseen;
extern SWBOOL zillawasseen;
extern short BossSpriteNum[3];

void ScreenTileLock(void);
void ScreenTileUnLock(void);

int ScreenSaveSetup(PLAYERp pp);
void ScreenSave(MFILE_WRITE fout);

int ScreenLoadSaveSetup(PLAYERp pp);
void ScreenLoad(MFILE_READ fin);

#define PANEL_SAVE 1
#define ANIM_SAVE 1

extern SW_PACKET loc;
extern char LevelName[20];
extern STATE s_NotRestored[];

OrgTileListP otlist[] = {&orgwalllist, &orgwalloverlist, &orgsectorceilinglist, &orgsectorfloorlist};

int
PanelSpriteToNdx(PLAYERp pp, PANEL_SPRITEp psprite)
{
    short ndx = 0;
    PANEL_SPRITEp psp=NULL, next=NULL;

    TRAVERSE(&pp->PanelSpriteList, psp, next)
    {
        if (psp == psprite)
            return ndx;

        ndx++;
    }

    // special case for pointing to the list head
    if ((LIST)psprite == (LIST)&pp->PanelSpriteList)
        return 9999;

    return -1;
}


PANEL_SPRITEp
PanelNdxToSprite(PLAYERp pp, int ndx)
{
    short count = 0;
    PANEL_SPRITEp psp, next;

    if (ndx == -1)
        return NULL;

    if (ndx == 9999)
        return (PANEL_SPRITEp)&pp->PanelSpriteList;

    TRAVERSE(&pp->PanelSpriteList, psp, next)
    {
        if (count == ndx)
            return psp;

        count++;
    }

    return NULL;
}

int SaveSymDataInfo(MFILE_WRITE fil, void *ptr)
{
    saveddatasym sym;

    if (Saveable_FindDataSym(ptr, &sym))
    {
        FILE *fp;

        fp = fopen("savegame symbols missing.txt", "a");
        if (fp)
        {
            fprintf(fp,"data %p\n",ptr);
            fclose(fp);
        }
        return 1;
    }

    MWRITE(&sym, sizeof(sym), 1, fil);

    return 0;
}
int SaveSymCodeInfo(MFILE_WRITE fil, void *ptr)
{
    savedcodesym sym;

    if (Saveable_FindCodeSym(ptr, &sym))
    {
        FILE *fp;

        fp = fopen("savegame symbols missing.txt", "a");
        if (fp)
        {
            fprintf(fp,"code %p\n",ptr);
            fclose(fp);
        }
        return 1;
    }

    MWRITE(&sym, sizeof(sym), 1, fil);

    return 0;
}

int LoadSymDataInfo(MFILE_READ fil, void **ptr)
{
    saveddatasym sym;

    MREAD(&sym, sizeof(sym), 1, fil);

    return Saveable_RestoreDataSym(&sym, ptr);
}
int LoadSymCodeInfo(MFILE_READ fil, void **ptr)
{
    savedcodesym sym;

    MREAD(&sym, sizeof(sym), 1, fil);

    return Saveable_RestoreCodeSym(&sym, ptr);
}


int SaveGame(short save_num)
{
    MFILE_WRITE fil;
    int i,j;
    short ndx;
    SPRITE tsp;
    SPRITEp sp;
    PLAYER tp;
    PLAYERp pp;
    SECT_USERp sectu;
    USER tu;
    USERp u;
    ANIM tanim;
    ANIMp a;
    int8_t code;
    uint8_t data_code;
    int16_t data_ndx;
    PANEL_SPRITE tpanel_sprite;
    PANEL_SPRITEp psp,cur,next;
    SECTOR_OBJECTp sop;
    char game_name[80];
    int cnt = 0, saveisshot=0;
    OrgTileP otp, next_otp;

    Saveable_Init();

    sprintf(game_name,"game%d.sav",save_num);
    if ((fil = MOPEN_WRITE(game_name)) == MOPEN_WRITE_ERR)
        return -1;

    MWRITE(&GameVersion,sizeof(GameVersion),1,fil);

    MWRITE(SaveGameDescr[save_num],sizeof(SaveGameDescr[save_num]),1,fil);

    MWRITE(&Level,sizeof(Level),1,fil);
    MWRITE(&Skill,sizeof(Skill),1,fil);

    ScreenSaveSetup(&Player[myconnectindex]);

    ScreenSave(fil);

    ScreenTileUnLock();

    MWRITE(&numplayers,sizeof(numplayers),1,fil);
    MWRITE(&myconnectindex,sizeof(myconnectindex),1,fil);
    MWRITE(&connecthead,sizeof(connecthead),1,fil);
    MWRITE(connectpoint2,sizeof(connectpoint2),1,fil);

    //save players info
    pp = &tp;
    for (i = 0; i < numplayers; i++)
    {
        memcpy(&tp, &Player[i], sizeof(PLAYER));

        // this does not point to global data - this is allocated link list based
        // save this inside the structure
#if PANEL_SAVE
        pp->CurWpn = (PANEL_SPRITEp)(intptr_t)PanelSpriteToNdx(&Player[i], pp->CurWpn);
        for (ndx = 0; ndx < MAX_WEAPONS; ndx++)
            pp->Wpn[ndx] = (PANEL_SPRITEp)(intptr_t)PanelSpriteToNdx(&Player[i], pp->Wpn[ndx]);
        pp->Chops = (PANEL_SPRITEp)(intptr_t)PanelSpriteToNdx(&Player[i], pp->Chops);
        for (ndx = 0; ndx < MAX_INVENTORY; ndx++)
            pp->InventorySprite[ndx] = (PANEL_SPRITEp)(intptr_t)PanelSpriteToNdx(&Player[i], pp->InventorySprite[ndx]);
        pp->InventorySelectionBox = (PANEL_SPRITEp)(intptr_t)PanelSpriteToNdx(&Player[i], pp->InventorySelectionBox);
        pp->MiniBarHealthBox = (PANEL_SPRITEp)(intptr_t)PanelSpriteToNdx(&Player[i], pp->MiniBarHealthBox);
        pp->MiniBarAmmo = (PANEL_SPRITEp)(intptr_t)PanelSpriteToNdx(&Player[i], pp->MiniBarAmmo);
        for (ndx = 0; ndx < (short)SIZ(pp->MiniBarHealthBoxDigit); ndx++)
            pp->MiniBarHealthBoxDigit[ndx] = (PANEL_SPRITEp)(intptr_t)PanelSpriteToNdx(&Player[i], pp->MiniBarHealthBoxDigit[ndx]);
        for (ndx = 0; ndx < (short)SIZ(pp->MiniBarAmmoDigit); ndx++)
            pp->MiniBarAmmoDigit[ndx] = (PANEL_SPRITEp)(intptr_t)PanelSpriteToNdx(&Player[i], pp->MiniBarAmmoDigit[ndx]);
#endif

        MWRITE(&tp, sizeof(PLAYER),1,fil);

        //////

        saveisshot |= SaveSymDataInfo(fil, pp->remote_sprite);
        saveisshot |= SaveSymDataInfo(fil, pp->remote.sop_control);
        saveisshot |= SaveSymDataInfo(fil, pp->sop_remote);
        saveisshot |= SaveSymDataInfo(fil, pp->sop);
        saveisshot |= SaveSymDataInfo(fil, pp->hi_sectp);
        saveisshot |= SaveSymDataInfo(fil, pp->lo_sectp);
        saveisshot |= SaveSymDataInfo(fil, pp->hi_sp);
        saveisshot |= SaveSymDataInfo(fil, pp->lo_sp);

        saveisshot |= SaveSymDataInfo(fil, pp->last_camera_sp);
        saveisshot |= SaveSymDataInfo(fil, pp->SpriteP);
        saveisshot |= SaveSymDataInfo(fil, pp->UnderSpriteP);

        saveisshot |= SaveSymCodeInfo(fil, pp->DoPlayerAction);

        saveisshot |= SaveSymDataInfo(fil, pp->sop_control);
        saveisshot |= SaveSymDataInfo(fil, pp->sop_riding);
    }

#if PANEL_SAVE
    // local copy
    psp = &tpanel_sprite;
    for (i = 0; i < numplayers; i++)
    {
        unsigned j;
        pp = &Player[i];
        ndx = 0;

        TRAVERSE(&pp->PanelSpriteList, cur, next)
        {
            // this is a HEADER
            MWRITE(&ndx, sizeof(ndx),1,fil);

            memcpy(psp, cur, sizeof(PANEL_SPRITE));

            // Panel Sprite - save in structure
            psp->sibling = (PANEL_SPRITEp)(intptr_t)PanelSpriteToNdx(pp, cur->sibling);
            MWRITE(psp, sizeof(PANEL_SPRITE),1,fil);

            saveisshot |= SaveSymDataInfo(fil, psp->PlayerP);
            saveisshot |= SaveSymDataInfo(fil, psp->State);
            saveisshot |= SaveSymDataInfo(fil, psp->RetractState);
            saveisshot |= SaveSymDataInfo(fil, psp->PresentState);
            saveisshot |= SaveSymDataInfo(fil, psp->ActionState);
            saveisshot |= SaveSymDataInfo(fil, psp->RestState);
            saveisshot |= SaveSymCodeInfo(fil, psp->PanelSpriteFunc);

            for (j = 0; j < SIZ(psp->over); j++)
            {
                saveisshot |= SaveSymDataInfo(fil, psp->over[j].State);
            }

            ndx++;
        }

        // store -1 when done for player
        ndx = -1;
        MWRITE(&ndx, sizeof(ndx),1,fil);
    }
#endif

    MWRITE(&numsectors,sizeof(numsectors),1,fil);
    MWRITE(sector,sizeof(SECTOR), numsectors, fil);

    //Sector User information
    for (i = 0; i < numsectors; i++)
    {
        sectu = SectUser[i];
        ndx = i;
        if (sectu)
        {
            // write header
            MWRITE(&ndx,sizeof(ndx),1,fil);

            MWRITE(sectu,sizeof(SECT_USER),1,fil);
        }
        else
        {
            // write trailer
            ndx = -1;
            MWRITE(&ndx,sizeof(ndx),1,fil);
        }
    }

    MWRITE(&numwalls,sizeof(numwalls),1,fil);
    MWRITE(wall,sizeof(WALL),numwalls,fil);

    for (i = 0; i < MAXSPRITES; i++)
    {
        if (sprite[i].statnum != MAXSTATUS)
        {
            MWRITE(&i,sizeof(i),1,fil);

            MWRITE(&sprite[i],sizeof(SPRITE),1,fil);
        }
    }
    i = -1;
    MWRITE(&i,sizeof(i),1,fil);

    MWRITE(headspritesect,sizeof(headspritesect),1,fil);
    MWRITE(prevspritesect,sizeof(prevspritesect),1,fil);
    MWRITE(nextspritesect,sizeof(nextspritesect),1,fil);
    MWRITE(headspritestat,sizeof(headspritestat),1,fil);
    MWRITE(prevspritestat,sizeof(prevspritestat),1,fil);
    MWRITE(nextspritestat,sizeof(nextspritestat),1,fil);

    //User information
    for (i = 0; i < MAXSPRITES; i++)
    {
        ndx = i;
        if (User[i])
        {
            // write header
            MWRITE(&ndx,sizeof(ndx),1,fil);

            sp = &sprite[i];
            memcpy(&tu, User[i], sizeof(USER));
            u = &tu;

            MWRITE(u,sizeof(USER),1,fil);

            if (u->WallShade)
            {
                MWRITE(u->WallShade,sizeof(*u->WallShade)*u->WallCount,1,fil);
            }

            if (u->rotator)
            {
                MWRITE(u->rotator,sizeof(*u->rotator),1,fil);
                if (u->rotator->origx)
                    MWRITE(u->rotator->origx,sizeof(*u->rotator->origx)*u->rotator->num_walls,1,fil);
                if (u->rotator->origy)
                    MWRITE(u->rotator->origy,sizeof(*u->rotator->origy)*u->rotator->num_walls,1,fil);
            }

            saveisshot |= SaveSymDataInfo(fil, u->WallP);
            saveisshot |= SaveSymDataInfo(fil, u->State);
            saveisshot |= SaveSymDataInfo(fil, u->Rot);
            saveisshot |= SaveSymDataInfo(fil, u->StateStart);
            saveisshot |= SaveSymDataInfo(fil, u->StateEnd);
            saveisshot |= SaveSymDataInfo(fil, u->StateFallOverride);
            saveisshot |= SaveSymCodeInfo(fil, u->ActorActionFunc);
            saveisshot |= SaveSymDataInfo(fil, u->ActorActionSet);
            saveisshot |= SaveSymDataInfo(fil, u->Personality);
            saveisshot |= SaveSymDataInfo(fil, u->Attrib);
            saveisshot |= SaveSymDataInfo(fil, u->sop_parent);
            saveisshot |= SaveSymDataInfo(fil, u->hi_sectp);
            saveisshot |= SaveSymDataInfo(fil, u->lo_sectp);
            saveisshot |= SaveSymDataInfo(fil, u->hi_sp);
            saveisshot |= SaveSymDataInfo(fil, u->lo_sp);
            saveisshot |= SaveSymDataInfo(fil, u->SpriteP);
            saveisshot |= SaveSymDataInfo(fil, u->PlayerP);
            saveisshot |= SaveSymDataInfo(fil, u->tgt_sp);
        }
    }
    ndx = -1;
    MWRITE(&ndx,sizeof(ndx),1,fil);

    //
    // Sector object
    //

    MWRITE(SectorObject, sizeof(SectorObject),1,fil);

    for (ndx = 0; ndx < (short)SIZ(SectorObject); ndx++)
    {
        sop = &SectorObject[ndx];

        saveisshot |= SaveSymCodeInfo(fil, sop->PreMoveAnimator);
        saveisshot |= SaveSymCodeInfo(fil, sop->PostMoveAnimator);
        saveisshot |= SaveSymCodeInfo(fil, sop->Animator);
        saveisshot |= SaveSymDataInfo(fil, sop->controller);
        saveisshot |= SaveSymDataInfo(fil, sop->sp_child);
    }


    MWRITE(SineWaveFloor, sizeof(SineWaveFloor),1,fil);
    MWRITE(SineWall, sizeof(SineWall),1,fil);
    MWRITE(SpringBoard, sizeof(SpringBoard),1,fil);
    //MWRITE(Rotate, sizeof(Rotate),1,fil);
    //MWRITE(DoorAutoClose, sizeof(DoorAutoClose),1,fil);
    MWRITE(&x_min_bound, sizeof(x_min_bound),1,fil);
    MWRITE(&y_min_bound, sizeof(y_min_bound),1,fil);
    MWRITE(&x_max_bound, sizeof(x_max_bound),1,fil);
    MWRITE(&y_max_bound, sizeof(y_max_bound),1,fil);


    MWRITE(Track, sizeof(Track),1,fil);
    for (i = 0; i < MAX_TRACKS; i++)
    {
        ASSERT(Track[i].TrackPoint);
        if (Track[i].NumPoints == 0)
            MWRITE(Track[i].TrackPoint, sizeof(TRACK_POINT),1,fil);
        else
            MWRITE(Track[i].TrackPoint, Track[i].NumPoints * sizeof(TRACK_POINT),1,fil);
    }

    MWRITE(&vel,sizeof(vel),1,fil);
    MWRITE(&svel,sizeof(svel),1,fil);
    MWRITE(&angvel,sizeof(angvel),1,fil);

    MWRITE(&loc,sizeof(loc),1,fil);
    //MWRITE(&oloc,sizeof(oloc),1,fil);
    //MWRITE(&fsync,sizeof(fsync),1,fil);

    MWRITE(LevelName,sizeof(LevelName),1,fil);
    MWRITE(&screenpeek,sizeof(screenpeek),1,fil);
    MWRITE(&totalsynctics,sizeof(totalsynctics),1,fil);

    // do all sector manipulation structures

#if ANIM_SAVE
#if 1
    MWRITE(&AnimCnt,sizeof(AnimCnt),1,fil);

    for (i = 0, a = &tanim; i < AnimCnt; i++)
    {
        intptr_t offset;
        memcpy(a,&Anim[i],sizeof(ANIM));

        // maintain compatibility with sinking boat which points to user data
        for (j=0; j<MAXSPRITES; j++)
        {
            if (User[j])
            {
                uint8_t* bp = (uint8_t*)User[j];

                if ((uint8_t*)a->ptr >= bp && (uint8_t*)a->ptr < bp + sizeof(USER))
                {
                    offset = (intptr_t)((uint8_t*)a->ptr - bp); // offset from user data
                    a->ptr = (int *)-2;
                    break;
                }
            }
        }

        if ((intptr_t)a->ptr != -2)
        {
            for (j=0; j<numsectors; j++)
            {
                if (SectUser[j])
                {
                    uint8_t* bp = (uint8_t*)SectUser[j];

                    if ((uint8_t*)a->ptr >= bp && (uint8_t*)a->ptr < bp + sizeof(SECT_USER))
                    {
                        offset = (intptr_t)((uint8_t*)a->ptr - bp); // offset from user data
                        a->ptr = (int *)-3;
                        break;
                    }
                }
            }
        }
        MWRITE(a,sizeof(ANIM),1,fil);

        if ((intptr_t)a->ptr == -2 || (intptr_t)a->ptr == -3)
        {
            MWRITE(&j, sizeof(j),1,fil);
            MWRITE(&offset, sizeof(offset),1,fil);
        }
        else
        {
            saveisshot |= SaveSymDataInfo(fil, a->ptr);
        }

        saveisshot |= SaveSymCodeInfo(fil, a->callback);
        saveisshot |= SaveSymDataInfo(fil, a->callbackdata);
    }

#else
    ndx = 0;
    for (i = AnimCnt - 1, a = &tanim; i >= 0; i--)
    {
        // write header
        MWRITE(&ndx,sizeof(ndx),1,fil);

        memcpy(a,&Anim[i],sizeof(ANIM));
        MWRITE(a,sizeof(ANIM),1,fil);

        saveisshot |= SaveSymDataInfo(fil, a->ptr);
        saveisshot |= SaveSymCodeInfo(fil, a->callback);
        saveisshot |= SaveSymDataInfo(fil, a->callbackdata);

        ndx++;
    }

    // write trailer
    ndx = -1;
    MWRITE(&ndx,sizeof(ndx),1,fil);
#endif
#endif

    MWRITE(&totalclock,sizeof(totalclock),1,fil);
    MWRITE(&numframes,sizeof(numframes),1,fil);
    MWRITE(&randomseed,sizeof(randomseed),1,fil);
    MWRITE(&numshades,sizeof(numshades),1,fil);

    MWRITE(&NormalVisibility,sizeof(NormalVisibility),1,fil);
    MWRITE(&g_visibility,sizeof(g_visibility),1,fil);
    MWRITE(&parallaxtype,sizeof(parallaxtype),1,fil);
    MWRITE(&parallaxyoffs_override,sizeof(parallaxyoffs_override),1,fil);
    MWRITE(&parallaxyscale_override,sizeof(parallaxyscale_override),1,fil);
    MWRITE(&pskybits_override,sizeof(pskybits_override),1,fil);

    MWRITE(&BorderInfo,sizeof(BorderInfo),1,fil);
    MWRITE(&MoveSkip2,sizeof(MoveSkip2),1,fil);
    MWRITE(&MoveSkip4,sizeof(MoveSkip4),1,fil);
    MWRITE(&MoveSkip8,sizeof(MoveSkip8),1,fil);

    // long interpolations
    MWRITE(&numinterpolations,sizeof(numinterpolations),1,fil);
    MWRITE(&startofdynamicinterpolations,sizeof(startofdynamicinterpolations),1,fil);
    MWRITE(oldipos,sizeof(oldipos),1,fil);
    MWRITE(bakipos,sizeof(bakipos),1,fil);
    for (i = numinterpolations - 1; i >= 0; i--)
        saveisshot |= SaveSymDataInfo(fil, curipos[i]);

    // short interpolations
    MWRITE(&short_numinterpolations,sizeof(short_numinterpolations),1,fil);
    MWRITE(&short_startofdynamicinterpolations,sizeof(short_startofdynamicinterpolations),1,fil);
    MWRITE(short_oldipos,sizeof(short_oldipos),1,fil);
    MWRITE(short_bakipos,sizeof(short_bakipos),1,fil);
    for (i = short_numinterpolations - 1; i >= 0; i--)
        saveisshot |= SaveSymDataInfo(fil, short_curipos[i]);


    // parental lock
    for (i = 0; i < (int)SIZ(otlist); i++)
    {
        ndx = 0;
        TRAVERSE(otlist[i], otp, next_otp)
        {
            MWRITE(&ndx,sizeof(ndx),1,fil);
            MWRITE(&otp,sizeof(*otp),1,fil);
            ndx++;
        }
        ndx = -1;
        MWRITE(&ndx, sizeof(ndx),1,fil);
    }

    // mirror
    MWRITE(mirror,sizeof(mirror),1,fil);
    MWRITE(&mirrorcnt,sizeof(mirrorcnt),1,fil);
    MWRITE(&mirrorinview,sizeof(mirrorinview),1,fil);

    // queue
    MWRITE(&StarQueueHead,sizeof(StarQueueHead),1,fil);
    MWRITE(StarQueue,sizeof(StarQueue),1,fil);
    MWRITE(&HoleQueueHead,sizeof(HoleQueueHead),1,fil);
    MWRITE(HoleQueue,sizeof(HoleQueue),1,fil);
    MWRITE(&WallBloodQueueHead,sizeof(WallBloodQueueHead),1,fil);
    MWRITE(WallBloodQueue,sizeof(WallBloodQueue),1,fil);
    MWRITE(&FloorBloodQueueHead,sizeof(FloorBloodQueueHead),1,fil);
    MWRITE(FloorBloodQueue,sizeof(FloorBloodQueue),1,fil);
    MWRITE(&GenericQueueHead,sizeof(GenericQueueHead),1,fil);
    MWRITE(GenericQueue,sizeof(GenericQueue),1,fil);
    MWRITE(&LoWangsQueueHead,sizeof(LoWangsQueueHead),1,fil);
    MWRITE(LoWangsQueue,sizeof(LoWangsQueue),1,fil);

    MWRITE(&PlayClock,sizeof(PlayClock),1,fil);
    MWRITE(&TotalKillable,sizeof(TotalKillable),1,fil);

    // game settings
    MWRITE(&gNet,sizeof(gNet),1,fil);

    MWRITE(LevelSong,sizeof(LevelSong),1,fil);

    MWRITE(palette,sizeof(palette),1,fil);
    MWRITE(palette_data,sizeof(palette_data),1,fil);
    MWRITE(&gs,sizeof(gs),1,fil);
    MWRITE(picanm,sizeof(picanm),1,fil);

    MWRITE(&LevelSecrets,sizeof(LevelSecrets),1,fil);

    MWRITE(show2dwall,sizeof(show2dwall),1,fil);
    MWRITE(show2dsprite,sizeof(show2dsprite),1,fil);
    MWRITE(show2dsector,sizeof(show2dsector),1,fil);

    MWRITE(&Bunny_Count,sizeof(Bunny_Count),1,fil);

    MWRITE(UserMapName,sizeof(UserMapName),1,fil);
    MWRITE(&GodMode,sizeof(GodMode),1,fil);

    MWRITE(&serpwasseen, sizeof(serpwasseen), 1, fil);
    MWRITE(&sumowasseen, sizeof(sumowasseen), 1, fil);
    MWRITE(&zillawasseen, sizeof(zillawasseen), 1, fil);
    MWRITE(BossSpriteNum, sizeof(BossSpriteNum), 1, fil);
    //MWRITE(&Zombies, sizeof(Zombies), 1, fil);

    MCLOSE_WRITE(fil);

    ////DSPRINTF(ds, "done saving");
    //MONO_PRINT(ds);

    if (saveisshot)
        CON_Message("There was a problem saving. See \"Save Help\" section of release notes.");

    return saveisshot ? -1 : 0;
}

int LoadGameFullHeader(short save_num, char *descr, short *level, short *skill)
{
    MFILE_READ fil;
    char game_name[80];
    short tile;
    int ver;

    sprintf(game_name,"game%d.sav",save_num);
    if ((fil = MOPEN_READ(game_name)) == MOPEN_READ_ERR)
        return -1;

    MREAD(&ver,sizeof(ver),1,fil);
    if (ver != GameVersion)
    {
        MCLOSE_READ(fil);
        return -1;
    }

    MREAD(descr, sizeof(SaveGameDescr[0]), 1,fil);

    MREAD(level,sizeof(*level),1,fil);
    MREAD(skill,sizeof(*skill),1,fil);

    tile = ScreenLoadSaveSetup(Player + myconnectindex);
    ScreenLoad(fil);

    MCLOSE_READ(fil);

    return tile;
}

void LoadGameDescr(short save_num, char *descr)
{
    MFILE_READ fil;
    char game_name[80];
    short tile;
    int ver;

    sprintf(game_name,"game%d.sav",save_num);
    if ((fil = MOPEN_READ(game_name)) == MOPEN_READ_ERR)
        return;

    MREAD(&ver,sizeof(ver),1,fil);
    if (ver != GameVersion)
    {
        MCLOSE_READ(fil);
        return;
    }

    MREAD(descr, sizeof(SaveGameDescr[0]),1,fil);

    MCLOSE_READ(fil);
}


int LoadGame(short save_num)
{
    MFILE_READ fil;
    int i,j,saveisshot=0;
    short ndx,SpriteNum,sectnum;
    PLAYERp pp = NULL;
    SPRITEp sp;
    USERp u;
    SECTOR_OBJECTp sop;
    SECT_USERp sectu;
    int8_t code;
    ANIMp a;
    uint8_t data_code;
    int16_t data_ndx;
    PANEL_SPRITEp psp,next,cur;
    PANEL_SPRITE tpanel_sprite;
    char game_name[80];
    OrgTileP otp, next_otp;

    int RotNdx;
    int StateStartNdx;
    int StateNdx;
    int StateEndNdx;
    extern SWBOOL InMenuLevel;

    Saveable_Init();

    sprintf(game_name,"game%d.sav",save_num);
    if ((fil = MOPEN_READ(game_name)) == MOPEN_READ_ERR)
        return -1;

    MREAD(&i,sizeof(i),1,fil);
    if (i != GameVersion)
    {
        MCLOSE_READ(fil);
        return -1;
    }

    // Don't terminate until you've made sure conditions are valid for loading.
    if (InMenuLevel)
        StopSong();
    else
        TerminateLevel();
    Terminate3DSounds();

    Terminate3DSounds();

    MREAD(SaveGameDescr[save_num], sizeof(SaveGameDescr[save_num]),1,fil);

    MREAD(&Level,sizeof(Level),1,fil);
    MREAD(&Skill,sizeof(Skill),1,fil);

    ScreenLoadSaveSetup(Player + myconnectindex);
    ScreenLoad(fil);
    ScreenTileUnLock();

    MREAD(&numplayers, sizeof(numplayers),1,fil);
    MREAD(&myconnectindex,sizeof(myconnectindex),1,fil);
    MREAD(&connecthead,sizeof(connecthead),1,fil);
    MREAD(connectpoint2,sizeof(connectpoint2),1,fil);

    //save players
    //MREAD(Player,sizeof(PLAYER), numplayers,fil);

    //save players info
    for (i = 0; i < numplayers; i++)
    {
        pp = &Player[i];

        MREAD(pp, sizeof(*pp), 1, fil);

        saveisshot |= LoadSymDataInfo(fil, (void **)&pp->remote_sprite);
        saveisshot |= LoadSymDataInfo(fil, (void **)&pp->remote.sop_control);
        saveisshot |= LoadSymDataInfo(fil, (void **)&pp->sop_remote);
        saveisshot |= LoadSymDataInfo(fil, (void **)&pp->sop);

        saveisshot |= LoadSymDataInfo(fil, (void **)&pp->hi_sectp);
        saveisshot |= LoadSymDataInfo(fil, (void **)&pp->lo_sectp);

        saveisshot |= LoadSymDataInfo(fil, (void **)&pp->hi_sp);
        saveisshot |= LoadSymDataInfo(fil, (void **)&pp->lo_sp);

        saveisshot |= LoadSymDataInfo(fil, (void **)&pp->last_camera_sp);
        saveisshot |= LoadSymDataInfo(fil, (void **)&pp->SpriteP);
        saveisshot |= LoadSymDataInfo(fil, (void **)&pp->UnderSpriteP);
        saveisshot |= LoadSymCodeInfo(fil, (void **)&pp->DoPlayerAction);
        saveisshot |= LoadSymDataInfo(fil, (void **)&pp->sop_control);
        saveisshot |= LoadSymDataInfo(fil, (void **)&pp->sop_riding);
        if (saveisshot) { MCLOSE_READ(fil); return -1; }
    }


#if PANEL_SAVE
    for (i = 0; i < numplayers; i++)
    {
        int j;
        pp = &Player[i];

        INITLIST(&pp->PanelSpriteList);

        while (TRUE)
        {
            MREAD(&ndx, sizeof(ndx),1,fil);

            if (ndx == -1)
                break;

            psp = CallocMem(sizeof(PANEL_SPRITE), 1);
            ASSERT(psp);

            MREAD(psp, sizeof(PANEL_SPRITE),1,fil);
            INSERT_TAIL(&pp->PanelSpriteList,psp);

            saveisshot |= LoadSymDataInfo(fil, (void **)&psp->PlayerP);
            saveisshot |= LoadSymDataInfo(fil, (void **)&psp->State);
            saveisshot |= LoadSymDataInfo(fil, (void **)&psp->RetractState);
            saveisshot |= LoadSymDataInfo(fil, (void **)&psp->PresentState);
            saveisshot |= LoadSymDataInfo(fil, (void **)&psp->ActionState);
            saveisshot |= LoadSymDataInfo(fil, (void **)&psp->RestState);
            saveisshot |= LoadSymCodeInfo(fil, (void **)&psp->PanelSpriteFunc);
            if (saveisshot) { MCLOSE_READ(fil); return -1; }

            for (j = 0; j < (int)SIZ(psp->over); j++)
            {
                saveisshot |= LoadSymDataInfo(fil, (void **)&psp->over[j].State);
                if (saveisshot) { MCLOSE_READ(fil); return -1; }
            }

        }
    }
#endif

    MREAD(&numsectors,sizeof(numsectors),1,fil);
    MREAD(sector,sizeof(SECTOR),numsectors,fil);

    //Sector User information
    for (i = 0; i < numsectors; i++)
    {
        MREAD(&sectnum,sizeof(sectnum),1,fil);
        if (sectnum != -1)
        {
            SectUser[sectnum] = sectu = (SECT_USERp)CallocMem(sizeof(SECT_USER), 1);
            MREAD(sectu,sizeof(SECT_USER),1,fil);
        }
    }

    MREAD(&numwalls,sizeof(numwalls),1,fil);
    MREAD(wall,sizeof(WALL),numwalls,fil);

    //Store all sprites to preserve indeces
    MREAD(&i, sizeof(i),1,fil);
    while (i != -1)
    {
        MREAD(&sprite[i], sizeof(SPRITE),1,fil);
        MREAD(&i, sizeof(i),1,fil);
    }

    MREAD(headspritesect,sizeof(headspritesect),1,fil);
    MREAD(prevspritesect,sizeof(prevspritesect),1,fil);
    MREAD(nextspritesect,sizeof(nextspritesect),1,fil);
    MREAD(headspritestat,sizeof(headspritestat),1,fil);
    MREAD(prevspritestat,sizeof(prevspritestat),1,fil);
    MREAD(nextspritestat,sizeof(nextspritestat),1,fil);

    //User information
    memset(User, 0, sizeof(User));

    MREAD(&SpriteNum, sizeof(SpriteNum),1,fil);
    while (SpriteNum != -1)
    {
        sp = &sprite[SpriteNum];
        User[SpriteNum] = u = (USERp)CallocMem(sizeof(USER), 1);
        MREAD(u,sizeof(USER),1,fil);

        if (u->WallShade)
        {
            u->WallShade = CallocMem(u->WallCount * sizeof(*u->WallShade), 1);
            MREAD(u->WallShade,sizeof(*u->WallShade)*u->WallCount,1,fil);
        }

        if (u->rotator)
        {
            u->rotator = CallocMem(sizeof(*u->rotator), 1);
            MREAD(u->rotator,sizeof(*u->rotator),1,fil);

            if (u->rotator->origx)
            {
                u->rotator->origx = CallocMem(u->rotator->num_walls * sizeof(*u->rotator->origx), 1);
                MREAD(u->rotator->origx,sizeof(*u->rotator->origx)*u->rotator->num_walls,1,fil);
            }
            if (u->rotator->origy)
            {
                u->rotator->origy = CallocMem(u->rotator->num_walls * sizeof(*u->rotator->origy), 1);
                MREAD(u->rotator->origy,sizeof(*u->rotator->origy)*u->rotator->num_walls,1,fil);
            }
        }

        saveisshot |= LoadSymDataInfo(fil, (void **)&u->WallP);
        saveisshot |= LoadSymDataInfo(fil, (void **)&u->State);
        saveisshot |= LoadSymDataInfo(fil, (void **)&u->Rot);
        saveisshot |= LoadSymDataInfo(fil, (void **)&u->StateStart);
        saveisshot |= LoadSymDataInfo(fil, (void **)&u->StateEnd);
        saveisshot |= LoadSymDataInfo(fil, (void **)&u->StateFallOverride);
        saveisshot |= LoadSymCodeInfo(fil, (void **)&u->ActorActionFunc);
        saveisshot |= LoadSymDataInfo(fil, (void **)&u->ActorActionSet);
        saveisshot |= LoadSymDataInfo(fil, (void **)&u->Personality);
        saveisshot |= LoadSymDataInfo(fil, (void **)&u->Attrib);
        saveisshot |= LoadSymDataInfo(fil, (void **)&u->sop_parent);
        saveisshot |= LoadSymDataInfo(fil, (void **)&u->hi_sectp);
        saveisshot |= LoadSymDataInfo(fil, (void **)&u->lo_sectp);
        saveisshot |= LoadSymDataInfo(fil, (void **)&u->hi_sp);
        saveisshot |= LoadSymDataInfo(fil, (void **)&u->lo_sp);
        saveisshot |= LoadSymDataInfo(fil, (void **)&u->SpriteP);
        saveisshot |= LoadSymDataInfo(fil, (void **)&u->PlayerP);
        saveisshot |= LoadSymDataInfo(fil, (void **)&u->tgt_sp);
        if (saveisshot) { MCLOSE_READ(fil); return -1; }

        MREAD(&SpriteNum,sizeof(SpriteNum),1,fil);
    }

    MREAD(SectorObject, sizeof(SectorObject),1,fil);

    for (ndx = 0; ndx < (short)SIZ(SectorObject); ndx++)
    {
        sop = &SectorObject[ndx];

        saveisshot |= LoadSymCodeInfo(fil, (void **)&sop->PreMoveAnimator);
        saveisshot |= LoadSymCodeInfo(fil, (void **)&sop->PostMoveAnimator);
        saveisshot |= LoadSymCodeInfo(fil, (void **)&sop->Animator);
        saveisshot |= LoadSymDataInfo(fil, (void **)&sop->controller);
        saveisshot |= LoadSymDataInfo(fil, (void **)&sop->sp_child);
        if (saveisshot) { MCLOSE_READ(fil); return -1; }
    }

    MREAD(SineWaveFloor, sizeof(SineWaveFloor),1,fil);
    MREAD(SineWall, sizeof(SineWall),1,fil);
    MREAD(SpringBoard, sizeof(SpringBoard),1,fil);
    //MREAD(Rotate, sizeof(Rotate),1,fil);
    //MREAD(DoorAutoClose, sizeof(DoorAutoClose),1,fil);
    MREAD(&x_min_bound, sizeof(x_min_bound),1,fil);
    MREAD(&y_min_bound, sizeof(y_min_bound),1,fil);
    MREAD(&x_max_bound, sizeof(x_max_bound),1,fil);
    MREAD(&y_max_bound, sizeof(y_max_bound),1,fil);

    MREAD(Track, sizeof(Track),1,fil);
    for (i = 0; i < MAX_TRACKS; i++)
    {
        if (Track[i].NumPoints == 0)
        {
            Track[i].TrackPoint = (TRACK_POINTp)CallocMem(sizeof(TRACK_POINT), 1);
            MREAD(Track[i].TrackPoint, sizeof(TRACK_POINT),1,fil);
        }
        else
        {
            Track[i].TrackPoint = (TRACK_POINTp)CallocMem(Track[i].NumPoints * sizeof(TRACK_POINT), 1);
            MREAD(Track[i].TrackPoint, Track[i].NumPoints * sizeof(TRACK_POINT),1,fil);
        }
    }

    MREAD(&vel,sizeof(vel),1,fil);
    MREAD(&svel,sizeof(svel),1,fil);
    MREAD(&angvel,sizeof(angvel),1,fil);

    MREAD(&loc,sizeof(loc),1,fil);

    MREAD(LevelName,sizeof(LevelName),1,fil);
    MREAD(&screenpeek,sizeof(screenpeek),1,fil);
    MREAD(&totalsynctics,sizeof(totalsynctics),1,fil);  // same as kens lockclock

    // do all sector manipulation structures

#if ANIM_SAVE
#if 1
    MREAD(&AnimCnt,sizeof(AnimCnt),1,fil);

    for (i = 0; i < AnimCnt; i++)
    {
        a = &Anim[i];
        MREAD(a,sizeof(ANIM),1,fil);

        if ((intptr_t)a->ptr == -2)
        {
            // maintain compatibility with sinking boat which points to user data
            int offset;
            MREAD(&j, sizeof(j),1,fil);
            MREAD(&offset, sizeof(offset),1,fil);
            a->ptr = (int *)(((char *)User[j]) + offset);
        }
        else if ((intptr_t)a->ptr == -3)
        {
            // maintain compatibility with sinking boat which points to user data
            int offset;
            MREAD(&j, sizeof(j),1,fil);
            MREAD(&offset, sizeof(offset),1,fil);
            a->ptr = (int *)(((char *)SectUser[j]) + offset);
        }
        else
        {
            saveisshot |= LoadSymDataInfo(fil, (void **)&a->ptr);
        }

        saveisshot |= LoadSymCodeInfo(fil, (void **)&a->callback);
        saveisshot |= LoadSymDataInfo(fil, (void **)&a->callbackdata);
        if (saveisshot) { MCLOSE_READ(fil); return -1; }
    }
#else
    AnimCnt = 0;
    for (i = MAXANIM - 1; i >= 0; i--)
    {
        a = &Anim[i];

        MREAD(&ndx,sizeof(ndx),1,fil);

        if (ndx == -1)
            break;

        AnimCnt++;

        MREAD(a,sizeof(ANIM),1,fil);

        saveisshot |= LoadSymDataInfo(fil, (void **)&a->ptr);
        saveisshot |= LoadSymCodeInfo(fil, (void **)&a->callback);
        saveisshot |= LoadSymDataInfo(fil, (void **)&a->callbackdata);
        if (saveisshot) { MCLOSE_READ(fil); return -1; }
    }
#endif
#endif

    MREAD(&totalclock,sizeof(totalclock),1,fil);
    MREAD(&numframes,sizeof(numframes),1,fil);
    MREAD(&randomseed,sizeof(randomseed),1,fil);
    MREAD(&numshades,sizeof(numshades),1,fil);

    MREAD(&NormalVisibility,sizeof(NormalVisibility),1,fil);
    MREAD(&g_visibility,sizeof(g_visibility),1,fil);
    MREAD(&parallaxtype,sizeof(parallaxtype),1,fil);
    MREAD(&parallaxyoffs_override,sizeof(parallaxyoffs_override),1,fil);
    MREAD(&parallaxyscale_override,sizeof(parallaxyscale_override),1,fil);
    MREAD(&pskybits_override,sizeof(pskybits_override),1,fil);

    MREAD(&BorderInfo,sizeof(BorderInfo),1,fil);
    MREAD(&MoveSkip2,sizeof(MoveSkip2),1,fil);
    MREAD(&MoveSkip4,sizeof(MoveSkip4),1,fil);
    MREAD(&MoveSkip8,sizeof(MoveSkip8),1,fil);

    // long interpolations
    MREAD(&numinterpolations,sizeof(numinterpolations),1,fil);
    MREAD(&startofdynamicinterpolations,sizeof(startofdynamicinterpolations),1,fil);
    MREAD(oldipos,sizeof(oldipos),1,fil);
    MREAD(bakipos,sizeof(bakipos),1,fil);
    for (i = numinterpolations - 1; i >= 0; i--)
        saveisshot |= LoadSymDataInfo(fil, (void **)&curipos[i]);
    if (saveisshot) { MCLOSE_READ(fil); return -1; }

    // short interpolations
    MREAD(&short_numinterpolations,sizeof(short_numinterpolations),1,fil);
    MREAD(&short_startofdynamicinterpolations,sizeof(short_startofdynamicinterpolations),1,fil);
    MREAD(short_oldipos,sizeof(short_oldipos),1,fil);
    MREAD(short_bakipos,sizeof(short_bakipos),1,fil);
    for (i = short_numinterpolations - 1; i >= 0; i--)
        saveisshot |= LoadSymDataInfo(fil, (void **)&short_curipos[i]);
    if (saveisshot) { MCLOSE_READ(fil); return -1; }

    // parental lock
    for (i = 0; i < (int)SIZ(otlist); i++)
    {
        INITLIST(otlist[i]);

        while (TRUE)
        {
            MREAD(&ndx, sizeof(ndx),1,fil);

            if (ndx == -1)
                break;

            otp = CallocMem(sizeof(*otp), 1);
            ASSERT(otp);

            MREAD(otp, sizeof(*otp),1,fil);
            INSERT_TAIL(otlist[i],otp);
        }
    }

    // mirror
    MREAD(mirror,sizeof(mirror),1,fil);
    MREAD(&mirrorcnt,sizeof(mirrorcnt),1,fil);
    MREAD(&mirrorinview,sizeof(mirrorinview),1,fil);

    // queue
    MREAD(&StarQueueHead,sizeof(StarQueueHead),1,fil);
    MREAD(StarQueue,sizeof(StarQueue),1,fil);
    MREAD(&HoleQueueHead,sizeof(HoleQueueHead),1,fil);
    MREAD(HoleQueue,sizeof(HoleQueue),1,fil);
    MREAD(&WallBloodQueueHead,sizeof(WallBloodQueueHead),1,fil);
    MREAD(WallBloodQueue,sizeof(WallBloodQueue),1,fil);
    MREAD(&FloorBloodQueueHead,sizeof(FloorBloodQueueHead),1,fil);
    MREAD(FloorBloodQueue,sizeof(FloorBloodQueue),1,fil);
    MREAD(&GenericQueueHead,sizeof(GenericQueueHead),1,fil);
    MREAD(GenericQueue,sizeof(GenericQueue),1,fil);
    MREAD(&LoWangsQueueHead,sizeof(LoWangsQueueHead),1,fil);
    MREAD(LoWangsQueue,sizeof(LoWangsQueue),1,fil);

    // init timing vars before PlayClock is read
    MREAD(&PlayClock,sizeof(PlayClock),1,fil);
    MREAD(&TotalKillable,sizeof(TotalKillable),1,fil);

    // game settings
    MREAD(&gNet,sizeof(gNet),1,fil);

    MREAD(LevelSong,sizeof(LevelSong),1,fil);

    MREAD(palette,sizeof(palette),1,fil);
    MREAD(palette_data,sizeof(palette_data),1,fil);

    {
        SWBOOL AmbBak = gs.Ambient;
        SWBOOL MusicBak = gs.MusicOn;
        SWBOOL FxBak = gs.FxOn;
        short SndVolBak = gs.SoundVolume;
        short MusVolBak = gs.MusicVolume;
        MREAD(&gs,sizeof(gs),1,fil);
        gs.MusicOn = MusicBak;
        gs.FxOn = FxBak;
        gs.Ambient = AmbBak;
        gs.SoundVolume = SndVolBak;
        gs.MusicVolume = MusVolBak;
    }


    //COVERsetbrightness(gs.Brightness,(char *)palette_data);

    MREAD(picanm,sizeof(picanm),1,fil);

    MREAD(&LevelSecrets,sizeof(LevelSecrets),1,fil);

    MREAD(show2dwall,sizeof(show2dwall),1,fil);
    MREAD(show2dsprite,sizeof(show2dsprite),1,fil);
    MREAD(show2dsector,sizeof(show2dsector),1,fil);

    MREAD(&Bunny_Count,sizeof(Bunny_Count),1,fil);

    MREAD(UserMapName,sizeof(UserMapName),1,fil);
    MREAD(&GodMode,sizeof(GodMode),1,fil);

    MREAD(&serpwasseen, sizeof(serpwasseen), 1, fil);
    MREAD(&sumowasseen, sizeof(sumowasseen), 1, fil);
    MREAD(&zillawasseen, sizeof(zillawasseen), 1, fil);
    MREAD(BossSpriteNum, sizeof(BossSpriteNum), 1, fil);
    //MREAD(&Zombies, sizeof(Zombies), 1, fil);

    MCLOSE_READ(fil);


    //!!IMPORTANT - this POST stuff will not work here now becaus it does actual reads


    //
    // POST processing of info MREAD in
    //

#if PANEL_SAVE
    for (i = 0; i < numplayers; i++)
    {
        pp = &Player[i];
        TRAVERSE(&pp->PanelSpriteList, psp, next)
        {
            // dont need to set Next and Prev this was done
            // when sprites were inserted

            // sibling is the only PanelSprite (malloced ptr) in the PanelSprite struct
            psp->sibling = PanelNdxToSprite(pp, (int)(intptr_t)psp->sibling);
        }
    }
#endif

    if (Bstrcasecmp(CacheLastLevel, LevelName) != 0)
    {
        SetupPreCache();
        DoTheCache();
    }

    // what is this for? don't remember
    totalclock = totalsynctics;
    ototalclock = totalsynctics;

    // this is ok - just duplicating sector list with pointers
    for (sop = SectorObject; sop < &SectorObject[SIZ(SectorObject)]; sop++)
    {
        for (i = 0; i < sop->num_sectors; i++)
            sop->sectp[i] = &sector[sop->sector[i]];
    }

    //!!Again this will not work here
    //restore players info
    for (i = 0; i < numplayers; i++)
    {
#if PANEL_SAVE
        pp->CurWpn = PanelNdxToSprite(pp, (int)(intptr_t)pp->CurWpn);

        for (ndx = 0; ndx < MAX_WEAPONS; ndx++)
            pp->Wpn[ndx] = PanelNdxToSprite(pp, (int)(intptr_t)pp->Wpn[ndx]);

        for (ndx = 0; ndx < MAX_INVENTORY; ndx++)
            pp->InventorySprite[ndx] = PanelNdxToSprite(pp, (int)(intptr_t)pp->InventorySprite[ndx]);

        pp->Chops = PanelNdxToSprite(pp, (int)(intptr_t)pp->Chops);
        pp->InventorySelectionBox = PanelNdxToSprite(pp, (int)(intptr_t)pp->InventorySelectionBox);
        pp->MiniBarHealthBox = PanelNdxToSprite(pp, (int)(intptr_t)pp->MiniBarHealthBox);
        pp->MiniBarAmmo = PanelNdxToSprite(pp, (int)(intptr_t)pp->MiniBarAmmo);

        for (ndx = 0; ndx < (short)SIZ(pp->MiniBarHealthBoxDigit); ndx++)
            pp->MiniBarHealthBoxDigit[ndx] = PanelNdxToSprite(pp, (int)(intptr_t)pp->MiniBarHealthBoxDigit[ndx]);

        for (ndx = 0; ndx < (short)SIZ(pp->MiniBarAmmoDigit); ndx++)
            pp->MiniBarAmmoDigit[ndx] = PanelNdxToSprite(pp, (int)(intptr_t)pp->MiniBarAmmoDigit[ndx]);

#endif
    }

    {
        int SavePlayClock = PlayClock;
        InitTimingVars();
        PlayClock = SavePlayClock;
    }
    InitNetVars();

    SetupAspectRatio();
    SetRedrawScreen(Player + myconnectindex);

    COVERsetbrightness(gs.Brightness,&palette_data[0][0]);

    screenpeek = myconnectindex;
    PlayingLevel = Level;

    PlaySong(LevelSong, RedBookSong[Level], TRUE, TRUE);
    if (gs.Ambient)
        StartAmbientSound();
    FX_SetVolume(gs.SoundVolume);
    SetSongVolume(gs.MusicVolume);

    TRAVERSE_CONNECT(i)
    {
        Player[i].PlayerTalking = FALSE;
        Player[i].TalkVocnum = -1;
        Player[i].TalkVocHandle = -1;
        Player[i].StartColor = 0;
    }

    // this is not a new game
    NewGame = FALSE;


    DoPlayerDivePalette(Player+myconnectindex);
    DoPlayerNightVisionPalette(Player+myconnectindex);

    return 0;
}

void
ScreenSave(MFILE_WRITE fout)
{
    // int num;
    MWRITE((void *)waloff[SAVE_SCREEN_TILE], SAVE_SCREEN_XSIZE * SAVE_SCREEN_YSIZE, 1, fout);
    // ASSERT(num == 1);
}

void
ScreenLoad(MFILE_READ fin)
{
    int num;

    setviewtotile(SAVE_SCREEN_TILE, SAVE_SCREEN_YSIZE, SAVE_SCREEN_XSIZE);

    num = MREAD((void *)waloff[SAVE_SCREEN_TILE], SAVE_SCREEN_XSIZE * SAVE_SCREEN_YSIZE, 1, fin);

    setviewback();
}

