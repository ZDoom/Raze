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
#include "build.h"

#include "keys.h"
#include "game.h"
#include "tags.h"
#include "names2.h"
#include "net.h"
#include "menus.h"

SWBOOL SyncPrintMode = TRUE;
short NumSyncBytes = 1;
char sync_first[MAXSYNCBYTES][60];
int sync_found = FALSE;

static int crctable[256];
#define updatecrc(dcrc,xz) (dcrc = (crctable[((dcrc)>>8)^((xz)&255)]^((dcrc)<<8)))

void initsynccrc(void)
{
    int i, j, k, a;

    for (j=0; j<256; j++)   //Calculate CRC table
    {
        k = (j<<8); a = 0;
        for (i=7; i>=0; i--)
        {
            if (((k^a)&0x8000) > 0)
                a = ((a<<1)&65535) ^ 0x1021;   //0x1021 = genpoly
            else
                a = ((a<<1)&65535);
            k = ((k<<1)&65535);
        }
        crctable[j] = (a&65535);
    }
}

#if SYNC_TEST
uint8_t
PlayerSync(void)
{
    short i, j;
    unsigned short crc = 0;
    PLAYERp pp;

    for (i = connecthead; i >= 0; i = connectpoint2[i])
    {
        pp = Player + i;
        updatecrc(crc, pp->posx & 255);
        updatecrc(crc, pp->posy & 255);
        updatecrc(crc, pp->posz & 255);
        updatecrc(crc, pp->pang & 255);
    }

    return (uint8_t) crc & 255;
}

uint8_t
PlayerSync2(void)
{
    short i, j;
    unsigned short crc = 0;
    PLAYERp pp;

    for (i = connecthead; i >= 0; i = connectpoint2[i])
    {
        pp = Player + i;

        updatecrc(crc, pp->horiz & 255);
        updatecrc(crc, User[pp->PlayerSprite]->Health & 255);
        updatecrc(crc, pp->bcnt & 255);
    }

    return (uint8_t) crc & 255;
}

uint8_t
SOSync(void)
{
    unsigned short crc = 0;
    SECTOR_OBJECTp sop;

    for (sop = SectorObject; sop < &SectorObject[MAX_SECTOR_OBJECTS]; sop++)
    {
        // if (sop->xmid == MAXLONG)
        // continue;

        updatecrc(crc, (sop->xmid) & 255);
        updatecrc(crc, (sop->ymid) & 255);
        updatecrc(crc, (sop->zmid) & 255);
        updatecrc(crc, (sop->vel) & 255);
        updatecrc(crc, (sop->ang) & 255);
        updatecrc(crc, (sop->ang_moving) & 255);
        updatecrc(crc, (sop->spin_ang) & 255);
    }

    return (uint8_t) crc & 255;
}


uint8_t
EnemySync(void)
{
    unsigned short crc = 0;
    short j, nextj;
    SPRITEp spr;
    extern char DemoTmpName[];

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_ENEMY], j, nextj)
    {
        spr = &sprite[j];
        updatecrc(crc, (spr->x) & 255);
        updatecrc(crc, (spr->y) & 255);
        updatecrc(crc, (spr->z) & 255);
        updatecrc(crc, (spr->ang) & 255);
    }

#if 0
    //DSPRINTF(ds, "Demo Tmp Name %s", DemoTmpName);
    MONO_PRINT(ds);

    {
        if (Once < 1 && DemoTmpName[0] != '\0')
        {
            FILE *fout;

            Once++;
            fout = fopen(DemoTmpName, "wb");

            //DSPRINTF(ds, "Demo Tmp Name %s", DemoTmpName);
            MONO_PRINT(ds);

            TRAVERSE_SPRITE_STAT(headspritestat[STAT_ENEMY], j, nextj)
            {
                spr = &sprite[j];

                fprintf(fout, "num %d, spr->x %d, spr->y %d, spr->z %d, spr->ang %d, spr->picnum %d\n", j, spr->x, spr->y, spr->z, spr->ang, spr->picnum);
            }
            fclose(fout);
        }
    }
#endif

    return (uint8_t) crc & 255;
}

uint8_t
MissileSync(void)
{
    unsigned short crc = 0;
    short j, nextj;
    SPRITEp spr;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_MISSILE], j, nextj)
    {
        spr = &sprite[j];
        updatecrc(crc, (spr->x) & 255);
        updatecrc(crc, (spr->y) & 255);
        updatecrc(crc, (spr->z) & 255);
        updatecrc(crc, (spr->ang) & 255);
    }

    return (uint8_t) crc & 255;
}

uint8_t
MissileSkip4Sync(void)
{
    unsigned short crc = 0;
    short j, nextj;
    SPRITEp spr;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_MISSILE_SKIP4], j, nextj)
    {
        spr = &sprite[j];
        updatecrc(crc, (spr->x) & 255);
        updatecrc(crc, (spr->y) & 255);
        updatecrc(crc, (spr->z) & 255);
        updatecrc(crc, (spr->ang) & 255);
    }

    return (uint8_t) crc & 255;
}

uint8_t
ShrapSync(void)
{
    unsigned short crc = 0;
    short j, nextj;
    SPRITEp spr;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_SHRAP], j, nextj)
    {
        spr = &sprite[j];
        updatecrc(crc, (spr->x) & 255);
        updatecrc(crc, (spr->y) & 255);
        updatecrc(crc, (spr->z) & 255);
        updatecrc(crc, (spr->ang) & 255);
    }

    return (uint8_t) crc & 255;
}

uint8_t
MiscSync(void)
{
    unsigned short crc = 0;
    short j, nextj;
    SPRITEp spr;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_MISC], j, nextj)
    {
        spr = &sprite[j];
        updatecrc(crc, (spr->x) & 255);
        updatecrc(crc, (spr->y) & 255);
        updatecrc(crc, (spr->z) & 255);
        updatecrc(crc, (spr->ang) & 255);
    }

    return (uint8_t) crc & 255;
}

uint8_t
RandomSync(void)
{
    unsigned short crc = 0;

    updatecrc(crc, randomseed & 255);
    updatecrc(crc, (randomseed >> 8) & 255);

    if (NumSyncBytes == 1)
    {
        updatecrc(crc,PlayerSync() & 255);
        updatecrc(crc,PlayerSync2() & 255);
        updatecrc(crc,MissileSync() & 255);
    }

    return (uint8_t) crc & 255;
}

/*
#define STAT_SKIP2_START    2
#define STAT_ENEMY          2
#define STAT_DEAD_ACTOR     3 //misc actor stuff - dead guys etc
#define STAT_MISSILE        4
#define STAT_SKIP2_END      4

#define STAT_SKIP4_START    5
#define STAT_ITEM           5
#define STAT_SKIP4          6
#define STAT_MISSILE_SKIP4  7
#define STAT_ENEMY_SKIP4    8
#define STAT_SKIP4_END      8
*/

char *SyncNames[] =
{
    "RandomSync",
    "PlayerSync",
    "PlayerSync2",
    "SOSync",
    "EnemySync",
    "MissileSync",
    "ShrapSync",
    "MiscSync",
    "MissileSkip4Sync",
    NULL
};

static uint8_t(*SyncFunc[MAXSYNCBYTES + 1]) (void) =
{
    RandomSync,
    PlayerSync,
    PlayerSync2,
    SOSync,
    EnemySync,
    MissileSync,
    ShrapSync,
    MiscSync,
    MissileSkip4Sync,
    NULL
};

void
getsyncstat(void)
{
    int i;
    PLAYERp pp = Player + myconnectindex;
    unsigned int val;
    static unsigned int count;
    extern int syncvaltail, syncvaltottail;

    if (!CommEnabled)
        return;

    if (numplayers < 2)
        return;

    for (i = 0; SyncFunc[i]; i++)
    {
        pp->syncval[pp->syncvalhead & (SYNCFIFOSIZ - 1)][i] = (*SyncFunc[i])();
    }

    val = pp->syncval[pp->syncvalhead & (SYNCFIFOSIZ - 1)][0];
    count += val;

    pp->syncvalhead++;
}

////////////////////////////////////////////////////////////////////////
//
// Sync Message print
//
////////////////////////////////////////////////////////////////////////


void
SyncStatMessage(void)
{
    int i, j, count = 0;
    static unsigned int MoveCount = 0;
    extern unsigned int MoveThingsCount;

    if (!CommEnabled)
        return;

    if (!SyncPrintMode)
        return;

    if (numplayers <= 1)
        return;

    for (i = 0; i < NumSyncBytes; i++)
    {
        // syncstat is NON 0 - out of sync
        if (syncstat[i] != 0)
        {
            if (NumSyncBytes > 1)
            {
                sprintf(ds, "GAME OUT OF SYNC - %s", SyncNames[i]);
                printext256(68L, 68L + (i * 8), 1, 31, ds, 0);
            }

            if (!sync_found && sync_first[i][0] == '\0')
            {
                // sync_found one so test all of them and then never test again
                sync_found = TRUE;

                // save off loop count
                MoveCount = MoveThingsCount;

                for (j = 0; j < NumSyncBytes; j++)
                {
                    if (syncstat[j] != 0 && sync_first[j][0] == '\0')
                    {
                        sprintf(ds, "OUT OF SYNC - %s", SyncNames[j]);
                        strcpy(sync_first[j], ds);
                    }
                }
            }
        }
    }

    // print out the sync_first message you got
    for (i = 0; i < NumSyncBytes; i++)
    {
        if (sync_first[i][0] != '\0')
        {
            if (NumSyncBytes > 1)
            {
                sprintf(ds, "FIRST %s", sync_first[i]);
                printext256(50L, 0L, 1, 31, ds, 0);
                sprintf(ds, "MoveCount %u",MoveCount);
                printext256(50L, 10L, 1, 31, ds, 0);
            }
            else
            {
                short w,h;
                // production out of sync error

                sprintf(ds,"GAME OUT OF SYNC!");
                MNU_MeasureString(ds, &w, &h);
                MNU_DrawString(TEXT_TEST_COL(w), 20, ds, 0, 19);

                sprintf(ds,"Restart the game.");
                MNU_MeasureString(ds, &w, &h);
                MNU_DrawString(TEXT_TEST_COL(w), 30, ds, 0, 19);
            }
        }
    }

    if (syncstate != 0)
        printext256(68L, 92L, 1, 31, "Missed Network packet!", 0);
}


void
GetSyncInfoFromPacket(char *packbuf, int packbufleng, int *j, int otherconnectindex)
{
    int sb, i;
    extern int syncvaltail, syncvaltottail;
    PLAYERp ppo = &Player[otherconnectindex];
    SWBOOL found = FALSE;

    // have had problems with this routine crashing when players quit
    // games.

    // if ready2send is not set then don't try to get sync info

    if (!ready2send)
        return;

    // Suspect that its trying to traverse the connect list
    // for a player that does not exist.  This tries to take care of that

    TRAVERSE_CONNECT(i)
    {
        if (otherconnectindex == i)
            found = TRUE;
    }

    if (!found)
        return;

    // sync testing
    //while ((*j) != packbufleng) // changed this on Kens suggestion
    while ((*j) < packbufleng)
    {
        for (sb = 0; sb < NumSyncBytes; sb++)
        {
            ppo->syncval[ppo->syncvalhead & (SYNCFIFOSIZ - 1)][sb] = packbuf[(*j)++];
        }
        ppo->syncvalhead++;
    }

    // update syncstat
    // if any of the syncstat vars is non-0 then there is a problem
    TRAVERSE_CONNECT(i)
    {
        if (Player[i].syncvalhead == syncvaltottail)
            return;
    }

    //for (sb = 0; sb < NumSyncBytes; sb++)
    //    syncstat[sb] = 0;

    while (TRUE)
    {
        for (i = connectpoint2[connecthead]; i >= 0; i = connectpoint2[i])
        {
            for (sb = 0; sb < NumSyncBytes; sb++)
            {
                if (Player[i].syncval[syncvaltottail & (SYNCFIFOSIZ - 1)][sb] != Player[connecthead].syncval[syncvaltottail & (SYNCFIFOSIZ - 1)][sb])
                {
                    syncstat[sb] = 1;
                }
            }
        }

        syncvaltottail++;

        TRAVERSE_CONNECT(i)
        {
            if (Player[i].syncvalhead == syncvaltottail)
                return;
        }
    }
}


////////////////////////////////////////////////////////////////////////
//
// Demo Sync recording and testing
//
////////////////////////////////////////////////////////////////////////

extern FILE *DemoSyncFile;

void
demosync_record(void)
{
    int i;
    uint8_t sync_val;

    for (i = 0; SyncFunc[i]; i++)
    {
        sync_val = (*SyncFunc[i])();
        fwrite(&sync_val, sizeof(sync_val), 1, DemoSyncFile);
    }
}

void
demosync_test(int cnt)
{
    int i;
    uint8_t sync_val;

    for (i = 0; SyncFunc[i]; i++)
    {
        fread(&sync_val, sizeof(sync_val), 1, DemoSyncFile);

        if (sync_val != (*SyncFunc[i])())
        {
            TerminateLevel();
            TerminateGame();
            printf("Demo out of sync - Sync Byte Number %d - Iteration %d.", i, cnt);
            exit(0);
        }
    }
}



/*
getsyncbyte()
    {
    int i, j;
    char ch;
    SPRITEp spr;
    PLAYERp pp;
    USERp u;

    ch = (char) (randomseed & 255);

    for (i = connecthead; i >= 0; i = connectpoint2[i])
        {
        pp = Player + i;
        u = User[pp->SpriteP - sprite];
        ch ^= (pp->posx ^ pp->posy ^ pp->posz ^ pp->pang ^ pp->horiz ^ u->Health);
        }

    for (j = headspritestat[STAT_ENEMY]; j >= 0; j = nextspritestat[j])
        {
        spr = &sprite[j];
        ch ^= spr->x ^ spr->y ^ spr->z ^ spr->ang;
        }

    return (ch);
    }
*/
#endif

