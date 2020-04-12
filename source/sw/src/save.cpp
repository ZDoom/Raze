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

#include "network.h"
//#include "save.h"
#include "savedef.h"
#include "jsector.h"
#include "parent.h"
#include "reserve.h"

//#define FILE_TYPE 1
#include "mfile.h"

#include "weapon.h"
#include "cache.h"
#include "colormap.h"
#include "player.h"
#include "i_specialpaths.h"
#include "savegamehelp.h"
#include "z_music.h"
#include "mapinfo.h"

//void TimerFunc(task * Task);
BEGIN_SW_NS

// This cannot have a namespace declaration
#include "saveable.h"

/*
//////////////////////////////////////////////////////////////////////////////
TO DO


//////////////////////////////////////////////////////////////////////////////
*/

extern int lastUpdate;
extern char UserMapName[80];
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

#define PANEL_SAVE 1
#define ANIM_SAVE 1

extern SW_PACKET loc;
extern char LevelName[20];
extern STATE s_NotRestored[];

OrgTileListP otlist[] = {&orgwalllist, &orgwalloverlist, &orgsectorceilinglist, &orgsectorfloorlist};

int PanelSpriteToNdx(PLAYERp pp, PANEL_SPRITEp psprite)
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


PANEL_SPRITEp PanelNdxToSprite(PLAYERp pp, int ndx)
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

        assert(false);
        fp = fopen("savegame symbols missing.txt", "a");
        if (fp)
        {
            fprintf(fp,"data %p - reference variable xdim at %p\n",ptr, &xdim);
            fclose(fp);
        }
        return 1;
    }

    MWRITE(&sym, sizeof(sym), 1, fil);

    return 0;
}

static int SaveSymCodeInfo_raw(MFILE_WRITE fil, void *ptr)
{
    savedcodesym sym;

    if (Saveable_FindCodeSym(ptr, &sym))
    {
        FILE *fp;

        assert(false);
        fp = fopen("savegame symbols missing.txt", "a");
        if (fp)
        {
            fprintf(fp,"code %p - reference function SaveSymDataInfo at %p\n",ptr, SaveSymDataInfo);
            fclose(fp);
        }
        return 1;
    }

    MWRITE(&sym, sizeof(sym), 1, fil);

    return 0;
}
template <typename T>
static int SaveSymCodeInfo(MFILE_WRITE fil, T * ptr)
{
    return SaveSymCodeInfo_raw(fil, (void *)ptr);
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



bool GameInterface::SaveGame(FSaveGameNode *sv)
{
    MFILE_WRITE fil;
    int i,j;
    short ndx;
    PLAYER tp;
    PLAYERp pp;
    SECT_USERp sectu;
    USER tu;
    USERp u;
    ANIM tanim;
    ANIMp a;
    PANEL_SPRITE tpanel_sprite;
    PANEL_SPRITEp psp,cur,next;
    SECTOR_OBJECTp sop;
    int saveisshot=0;
    OrgTileP otp, next_otp;

    Saveable_Init();
	
	
    // workaround until the level info here has been transitioned.
	fil = WriteSavegameChunk("snapshot.sw");

    MWRITE(&GameVersion,sizeof(GameVersion),1,fil);

    MWRITE(&Level,sizeof(Level),1,fil);
    MWRITE(&Skill,sizeof(Skill),1,fil);

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
        assert(!saveisshot);
        saveisshot |= SaveSymDataInfo(fil, pp->remote.sop_control);
        assert(!saveisshot);
        saveisshot |= SaveSymDataInfo(fil, pp->sop_remote);
        assert(!saveisshot);
        saveisshot |= SaveSymDataInfo(fil, pp->sop);
        assert(!saveisshot);
        saveisshot |= SaveSymDataInfo(fil, pp->hi_sectp);
        assert(!saveisshot);
        saveisshot |= SaveSymDataInfo(fil, pp->lo_sectp);
        assert(!saveisshot);
        saveisshot |= SaveSymDataInfo(fil, pp->hi_sp);
        assert(!saveisshot);
        saveisshot |= SaveSymDataInfo(fil, pp->lo_sp);
        assert(!saveisshot);

        saveisshot |= SaveSymDataInfo(fil, pp->last_camera_sp);
        assert(!saveisshot);
        saveisshot |= SaveSymDataInfo(fil, pp->SpriteP);
        assert(!saveisshot);
        saveisshot |= SaveSymDataInfo(fil, pp->UnderSpriteP);
        assert(!saveisshot);

        saveisshot |= SaveSymCodeInfo(fil, pp->DoPlayerAction);
        assert(!saveisshot);

        saveisshot |= SaveSymDataInfo(fil, pp->sop_control);
        assert(!saveisshot);
        saveisshot |= SaveSymDataInfo(fil, pp->sop_riding);
        assert(!saveisshot);
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
            assert(!saveisshot);
            saveisshot |= SaveSymDataInfo(fil, psp->State);
            assert(!saveisshot);
            saveisshot |= SaveSymDataInfo(fil, psp->RetractState);
            assert(!saveisshot);
            saveisshot |= SaveSymDataInfo(fil, psp->PresentState);
            assert(!saveisshot);
            saveisshot |= SaveSymDataInfo(fil, psp->ActionState);
            assert(!saveisshot);
            saveisshot |= SaveSymDataInfo(fil, psp->RestState);
            assert(!saveisshot);
            saveisshot |= SaveSymCodeInfo(fil, psp->PanelSpriteFunc);
            assert(!saveisshot);

            for (j = 0; j < SIZ(psp->over); j++)
            {
                saveisshot |= SaveSymDataInfo(fil, psp->over[j].State);
                assert(!saveisshot);
            }

            ndx++;
        }

        // store -1 when done for player
        ndx = -1;
        MWRITE(&ndx, sizeof(ndx),1,fil);
    }
#endif

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

    //User information
    for (i = 0; i < MAXSPRITES; i++)
    {
        ndx = i;
        if (User[i])
        {
            // write header
            MWRITE(&ndx,sizeof(ndx),1,fil);

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
            assert(!saveisshot);
            saveisshot |= SaveSymDataInfo(fil, u->State);
            assert(!saveisshot);
            saveisshot |= SaveSymDataInfo(fil, u->Rot);
            assert(!saveisshot);
            saveisshot |= SaveSymDataInfo(fil, u->StateStart);
            assert(!saveisshot);
            saveisshot |= SaveSymDataInfo(fil, u->StateEnd);
            assert(!saveisshot);
            saveisshot |= SaveSymDataInfo(fil, u->StateFallOverride);
            assert(!saveisshot);
            saveisshot |= SaveSymCodeInfo(fil, u->ActorActionFunc);
            assert(!saveisshot);
            saveisshot |= SaveSymDataInfo(fil, u->ActorActionSet);
            assert(!saveisshot);
            saveisshot |= SaveSymDataInfo(fil, u->Personality);
            assert(!saveisshot);
            saveisshot |= SaveSymDataInfo(fil, u->Attrib);
            assert(!saveisshot);
            saveisshot |= SaveSymDataInfo(fil, u->sop_parent);
            assert(!saveisshot);
            saveisshot |= SaveSymDataInfo(fil, u->hi_sectp);
            assert(!saveisshot);
            saveisshot |= SaveSymDataInfo(fil, u->lo_sectp);
            assert(!saveisshot);
            saveisshot |= SaveSymDataInfo(fil, u->hi_sp);
            assert(!saveisshot);
            saveisshot |= SaveSymDataInfo(fil, u->lo_sp);
            assert(!saveisshot);
            saveisshot |= SaveSymDataInfo(fil, u->SpriteP);
            assert(!saveisshot);
            saveisshot |= SaveSymDataInfo(fil, u->PlayerP);
            assert(!saveisshot);
            saveisshot |= SaveSymDataInfo(fil, u->tgt_sp);
            assert(!saveisshot);
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
        assert(!saveisshot);
        saveisshot |= SaveSymCodeInfo(fil, sop->PostMoveAnimator);
        assert(!saveisshot);
        saveisshot |= SaveSymCodeInfo(fil, sop->Animator);
        assert(!saveisshot);
        saveisshot |= SaveSymDataInfo(fil, sop->controller);
        assert(!saveisshot);
        saveisshot |= SaveSymDataInfo(fil, sop->sp_child);
        assert(!saveisshot);
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
            assert(!saveisshot);
        }

        saveisshot |= SaveSymCodeInfo(fil, a->callback);
        assert(!saveisshot);
        saveisshot |= SaveSymDataInfo(fil, a->callbackdata);
        assert(!saveisshot);
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
    
    MWRITE(&NormalVisibility,sizeof(NormalVisibility),1,fil);
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
    {
        saveisshot |= SaveSymDataInfo(fil, curipos[i]);
        assert(!saveisshot);
    }

    // short interpolations
    MWRITE(&short_numinterpolations,sizeof(short_numinterpolations),1,fil);
    MWRITE(&short_startofdynamicinterpolations,sizeof(short_startofdynamicinterpolations),1,fil);
    MWRITE(short_oldipos,sizeof(short_oldipos),1,fil);
    MWRITE(short_bakipos,sizeof(short_bakipos),1,fil);
    for (i = short_numinterpolations - 1; i >= 0; i--)
    {
        saveisshot |= SaveSymDataInfo(fil, short_curipos[i]);
        assert(!saveisshot);
    }

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

    MWRITE(&gs,sizeof(gs),1,fil);
	for (int i = 0; i < MAXTILES; i++)
	{
		MWRITE(&picanm[i], sizeof(picanm[i]), 1, fil);
	}

    MWRITE(&LevelSecrets,sizeof(LevelSecrets),1,fil);

    MWRITE(&Bunny_Count,sizeof(Bunny_Count),1,fil);

    MWRITE(UserMapName,sizeof(UserMapName),1,fil);
    MWRITE(&GodMode,sizeof(GodMode),1,fil);

    MWRITE(&serpwasseen, sizeof(serpwasseen), 1, fil);
    MWRITE(&sumowasseen, sizeof(sumowasseen), 1, fil);
    MWRITE(&zillawasseen, sizeof(zillawasseen), 1, fil);
    MWRITE(BossSpriteNum, sizeof(BossSpriteNum), 1, fil);
    //MWRITE(&Zombies, sizeof(Zombies), 1, fil);

	if (!saveisshot)
		return FinishSavegameWrite();

    return false;
}


extern SWBOOL LoadGameOutsideMoveLoop;
extern SWBOOL InMenuLevel;

 bool GameInterface::CleanupForLoad() 
 {
     // Don't terminate until you've made sure conditions are valid for loading.
     if (InMenuLevel)
         Mus_Stop();
     else
     {
         PauseAction();
         TerminateLevel();
     }
     StopFX();
     return true;
 }

bool GameInterface::LoadGame(FSaveGameNode* sv)
{
    MFILE_READ fil;
    int i,j,saveisshot=0;
    short ndx,SpriteNum,sectnum;
    PLAYERp pp = NULL;
    USERp u;
    SECTOR_OBJECTp sop;
    SECT_USERp sectu;
    ANIMp a;
    PANEL_SPRITEp psp,next;
    OrgTileP otp;


    Saveable_Init();

	auto filr = ReadSavegameChunk("snapshot.sw");
	if (!filr.isOpen()) return false;
	fil = &filr;

    MREAD(&i,sizeof(i),1,fil);
    if (i != GameVersion)
    {
        MCLOSE_READ(fil);
        return false;
    }

    MREAD(&Level,sizeof(Level),1,fil);
    MREAD(&Skill,sizeof(Skill),1,fil);

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
        if (saveisshot) { MCLOSE_READ(fil); return false; }
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

            psp = (PANEL_SPRITEp)CallocMem(sizeof(PANEL_SPRITE), 1);
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
            if (saveisshot) { MCLOSE_READ(fil); return false; }

            for (j = 0; j < (int)SIZ(psp->over); j++)
            {
                saveisshot |= LoadSymDataInfo(fil, (void **)&psp->over[j].State);
                if (saveisshot) { MCLOSE_READ(fil); return false; }
            }

        }
    }
#endif

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

    //User information
    memset(User, 0, sizeof(User));

    MREAD(&SpriteNum, sizeof(SpriteNum),1,fil);
    while (SpriteNum != -1)
    {
        User[SpriteNum] = u = (USERp)CallocMem(sizeof(USER), 1);
        MREAD(u,sizeof(USER),1,fil);

        if (u->WallShade)
        {
            u->WallShade = (int8_t*)CallocMem(u->WallCount * sizeof(*u->WallShade), 1);
            MREAD(u->WallShade,sizeof(*u->WallShade)*u->WallCount,1,fil);
        }

        if (u->rotator)
        {
            u->rotator = (ROTATORp)CallocMem(sizeof(*u->rotator), 1);
            MREAD(u->rotator,sizeof(*u->rotator),1,fil);

            if (u->rotator->origx)
            {
                u->rotator->origx = (int*)CallocMem(u->rotator->num_walls * sizeof(*u->rotator->origx), 1);
                MREAD(u->rotator->origx,sizeof(*u->rotator->origx)*u->rotator->num_walls,1,fil);
            }
            if (u->rotator->origy)
            {
                u->rotator->origy = (int*)CallocMem(u->rotator->num_walls * sizeof(*u->rotator->origy), 1);
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
        if (saveisshot) { MCLOSE_READ(fil); return false; }

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
        if (saveisshot) { MCLOSE_READ(fil); return false; }
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
        if (saveisshot) { MCLOSE_READ(fil); return false; }
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
        if (saveisshot) { MCLOSE_READ(fil); return false; }
    }
#endif
#endif

    MREAD(&totalclock,sizeof(totalclock),1,fil);

    MREAD(&NormalVisibility,sizeof(NormalVisibility),1,fil);

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
    if (saveisshot) { MCLOSE_READ(fil); return false; }

    // short interpolations
    MREAD(&short_numinterpolations,sizeof(short_numinterpolations),1,fil);
    MREAD(&short_startofdynamicinterpolations,sizeof(short_startofdynamicinterpolations),1,fil);
    MREAD(short_oldipos,sizeof(short_oldipos),1,fil);
    MREAD(short_bakipos,sizeof(short_bakipos),1,fil);
    for (i = short_numinterpolations - 1; i >= 0; i--)
        saveisshot |= LoadSymDataInfo(fil, (void **)&short_curipos[i]);
    if (saveisshot) { MCLOSE_READ(fil); return false; }

    // parental lock
    for (i = 0; i < (int)SIZ(otlist); i++)
    {
        INITLIST(otlist[i]);

        while (TRUE)
        {
            MREAD(&ndx, sizeof(ndx),1,fil);

            if (ndx == -1)
                break;

            otp = (OrgTileP)CallocMem(sizeof(*otp), 1);
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

	MREAD(&gs,sizeof(gs),1,fil);

	for (int i = 0; i < MAXTILES; i++)
	{
		MREAD(&picanm[i], sizeof(picanm[i]), 1, fil);
	}

    MREAD(&LevelSecrets,sizeof(LevelSecrets),1,fil);

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

    screenpeek = myconnectindex;
    PlayingLevel = Level;

    Mus_ResumeSaved();
    if (snd_ambience)
        StartAmbientSound();

    TRAVERSE_CONNECT(i)
    {
        Player[i].StartColor = 0;
    }

    // this is not a new game
    NewGame = FALSE;


    DoPlayerDivePalette(Player+myconnectindex);
    DoPlayerNightVisionPalette(Player+myconnectindex);


	

    hud_size.Callback();
    LoadGameOutsideMoveLoop = TRUE;
    if (!InMenuLevel)
    {
        ready2send = 1;
    }
    else ExitLevel = TRUE;
    return true;
}

END_SW_NS
