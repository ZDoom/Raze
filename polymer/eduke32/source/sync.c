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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

int NumSyncBytes = 1;
char sync_first[MAXSYNCBYTES][60];
int sync_found = 0;

static int crctable[256];
#define updatecrc(dcrc,xz) (dcrc = (crctable[((dcrc)>>8)^((xz)&255)]^((dcrc)<<8)))

void initsynccrc(void)
{
    int i, j, k, a;

    for (j=0;j<256;j++)     //Calculate CRC table
    {
        k = (j<<8); a = 0;
        for (i=7;i>=0;i--)
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

char PlayerSync(void)
{
    short i;
    unsigned short crc = 0;
    player_struct *pp;

    for (i = connecthead; i >= 0; i = connectpoint2[i])
    {
        pp = g_player[i].ps;
        updatecrc(crc, pp->posx & 255);
        updatecrc(crc, pp->posy & 255);
        updatecrc(crc, pp->posz & 255);
        updatecrc(crc, pp->ang & 255);
    }

    return ((char) crc & 255);
}

char PlayerSync2(void)
{
    short i;
    unsigned short crc = 0;
    player_struct *pp;

    for (i = connecthead; i >= 0; i = connectpoint2[i])
    {
        pp = g_player[i].ps;

        updatecrc(crc, pp->horiz & 255);
        updatecrc(crc, sprite[pp->i].extra & 255);
        updatecrc(crc, pp->bobcounter & 255);
    }

    return ((char) crc & 255);
}
/*
char SOSync(void)
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

    return ((char) crc & 255);
}


char EnemySync(void)
{
    unsigned short crc = 0;
    short j, nextj;
    spritetype *spr;
    extern char DemoTmpName[];

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_ENEMY], j, nextj)
    {
        spr = &sprite[j];
        updatecrc(crc, (spr->x) & 255);
        updatecrc(crc, (spr->y) & 255);
        updatecrc(crc, (spr->z) & 255);
        updatecrc(crc, (spr->ang) & 255);
    }

    return ((char) crc & 255);
}

char MissileSync(void)
{
    unsigned short crc = 0;
    short j, nextj;
    spritetype *spr;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_MISSILE], j, nextj)
    {
        spr = &sprite[j];
        updatecrc(crc, (spr->x) & 255);
        updatecrc(crc, (spr->y) & 255);
        updatecrc(crc, (spr->z) & 255);
        updatecrc(crc, (spr->ang) & 255);
    }

    return ((char) crc & 255);
}

char MissileSkip4Sync(void)
{
    unsigned short crc = 0;
    short j, nextj;
    spritetype *spr;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_MISSILE_SKIP4], j, nextj)
    {
        spr = &sprite[j];
        updatecrc(crc, (spr->x) & 255);
        updatecrc(crc, (spr->y) & 255);
        updatecrc(crc, (spr->z) & 255);
        updatecrc(crc, (spr->ang) & 255);
    }

    return ((char) crc & 255);
}

char ShrapSync(void)
{
    unsigned short crc = 0;
    short j, nextj;
    spritetype *spr;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_SHRAP], j, nextj)
    {
        spr = &sprite[j];
        updatecrc(crc, (spr->x) & 255);
        updatecrc(crc, (spr->y) & 255);
        updatecrc(crc, (spr->z) & 255);
        updatecrc(crc, (spr->ang) & 255);
    }

    return ((char) crc & 255);
}

char MiscSync(void)
{
    unsigned short crc = 0;
    short j, nextj;
    spritetype *spr;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_MISC], j, nextj)
    {
        spr = &sprite[j];
        updatecrc(crc, (spr->x) & 255);
        updatecrc(crc, (spr->y) & 255);
        updatecrc(crc, (spr->z) & 255);
        updatecrc(crc, (spr->ang) & 255);
    }

    return ((char) crc & 255);
}
*/
char RandomSync(void)
{
    unsigned short crc = 0;

    updatecrc(crc, randomseed & 255);
    updatecrc(crc, (randomseed >> 8) & 255);

    if (NumSyncBytes == 1)
    {
        updatecrc(crc,PlayerSync() & 255);
        updatecrc(crc,PlayerSync2() & 255);
//        updatecrc(crc,WeaponSync() & 255);
    }

    return ((char) crc & 255);
}

/*
    movefta();              //ST 2
    moveweapons();          //ST 4
    movetransports();       //ST 9

    moveplayers();          //ST 10
    movefallers();          //ST 12
    moveexplosions();       //ST 5

    moveactors();           //ST 1
    moveeffectors();        //ST 3

    movestandables();       //ST 6
    movefx();               //ST 11
*/

char *SyncNames[] =
{
    "RandomSync",
    "PlayerSync",
    "PlayerSync2",
    /*    "FTASync",
        "WeaponSync",
        "TransportSync",
        "FallerSync",
        "ExplosionSync",
        "ActorSync",
        "EffectorSync",
        "StandableSync",
        "FXSync", */
    NULL
};

static char(*SyncFunc[MAXSYNCBYTES + 1])(void) =
{
    RandomSync,
    PlayerSync,
    PlayerSync2,
    /*    FTASync,
        WeaponSync,
        TransportSync,
        FallerSync,
        ExplosionSync,
        ActorSync,
        EffectorSync,
        StandableSync,
        FXSync, */
    NULL
};

void getsyncstat(void)
{
    int i;
    playerdata_t *pp = &g_player[myconnectindex];
    unsigned int val;
    static unsigned int count;

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


void SyncStatMessage(void)
{
    int i, j;
    static unsigned int MoveCount = 0;
    extern unsigned int MoveThingsCount;

//    if (!SyncPrintMode)
//        return;

    if (numplayers <= 1)
        return;

    for (i = 0; i < NumSyncBytes; i++)
    {
        // syncstat is NON 0 - out of sync
        if (syncstat[i] != 0)
        {
            if (NumSyncBytes > 1)
            {
                sprintf(tempbuf, "GAME OUT OF SYNC - %s", SyncNames[i]);
                printext256(68L, 68L + (i * 8), 1, 31, tempbuf, 0);
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
                        sprintf(tempbuf, "OUT OF SYNC - %s", SyncNames[j]);
                        strcpy(sync_first[j], tempbuf);
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
                sprintf(tempbuf, "FIRST %s", sync_first[i]);
                printext256(50L, 0L, 1, 31, tempbuf, 0);
                sprintf(tempbuf, "MoveCount %d",MoveCount);
                printext256(50L, 10L, 1, 31, tempbuf, 0);
            }
            else
            {
//                short w,h;
                // production out of sync error

//                sprintf(tempbuf,"GAME OUT OF SYNC!");
//                MNU_MeasureString(tempbuf, &w, &h);
                //              MNU_DrawString(TEXT_TEST_COL(w), 20, tempbuf, 0, 19);

//                sprintf(tempbuf,"Restart the game.");
                //            MNU_MeasureString(tempbuf, &w, &h);
                //          MNU_DrawString(TEXT_TEST_COL(w), 30, tempbuf, 0, 19);
                printext256(4L,130L,31,0,"Out Of Sync - Please restart game",0);
                printext256(4L,138L,31,0,"RUN DN3DHELP.EXE for information.",0);
            }
        }
    }

//    if (syncstate != 0)
//        printext256(68L, 92L, 1, 31, "Missed Network packet!", 0);
}


void  AddSyncInfoToPacket(int *j)
{
    int sb;
    int count = 0;

    // sync testing
    while (g_player[myconnectindex].syncvalhead != syncvaltail && count++ < 4)
    {
        for (sb = 0; sb < NumSyncBytes; sb++)
            packbuf[(*j)++] = g_player[myconnectindex].syncval[syncvaltail & (SYNCFIFOSIZ - 1)][sb];

        syncvaltail++;
    }
}

void GetSyncInfoFromPacket(char *packbuf, int packbufleng, int *j, int otherconnectindex)
{
    int sb, i;
    extern int syncvaltail, syncvaltottail;
    playerdata_t *ppo = &g_player[otherconnectindex];
    char found = 0;

    // have had problems with this routine crashing when players quit
    // games.

    // if ready2send is not set then don't try to get sync info

    if (!ready2send)
        return;

    // Suspect that its trying to traverse the connect list
    // for a player that does not exist.  This tries to take care of that

    for (i=connecthead;i>=0;i=connectpoint2[i])
    {
        if (otherconnectindex == i)
            found = 1;
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
    for (i=connecthead;i>=0;i=connectpoint2[i])
    {
        if (g_player[i].syncvalhead == syncvaltottail)
            return;
    }

    //for (sb = 0; sb < NumSyncBytes; sb++)
    //    syncstat[sb] = 0;

    while (1)
    {
        for (i = connectpoint2[connecthead]; i >= 0; i = connectpoint2[i])
        {
            for (sb = 0; sb < NumSyncBytes; sb++)
            {
                if (g_player[i].syncval[syncvaltottail & (SYNCFIFOSIZ - 1)][sb] != g_player[connecthead].syncval[syncvaltottail & (SYNCFIFOSIZ - 1)][sb])
                {
                    syncstat[sb] = 1;
                }
            }
        }

        syncvaltottail++;

        for (i=connecthead;i>=0;i=connectpoint2[i])
        {
            if (g_player[i].syncvalhead == syncvaltottail)
                return;
        }
    }
}


