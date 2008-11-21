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

int g_numSyncBytes = 1;
char g_szfirstSyncMsg[MAXSYNCBYTES][60];
int g_foundSyncError = 0;

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

char Net_PlayerSync(void)
{
    short i;
    unsigned short crc = 0;
    DukePlayer_t *pp;

    TRAVERSE_CONNECT(i)
    {
        pp = g_player[i].ps;
        updatecrc(crc, pp->posx & 255);
        updatecrc(crc, pp->posy & 255);
        updatecrc(crc, pp->posz & 255);
        updatecrc(crc, pp->ang & 255);
    }

    return ((char) crc & 255);
}

char Net_PlayerSync2(void)
{
    int i;
    int j, nextj;
    unsigned short crc = 0;
    DukePlayer_t *pp;
    spritetype *spr;

    TRAVERSE_CONNECT(i)
    {
        pp = g_player[i].ps;

        updatecrc(crc, pp->horiz & 255);
        updatecrc(crc, sprite[pp->i].extra & 255);
        updatecrc(crc, pp->bobcounter & 255);
    }

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_PLAYER], j, nextj)
    {
        spr = &sprite[j];
        updatecrc(crc, (spr->x) & 255);
        updatecrc(crc, (spr->y) & 255);
        updatecrc(crc, (spr->z) & 255);
        updatecrc(crc, (spr->ang) & 255);
    }

    return ((char) crc & 255);
}

char Net_ActorSync(void)
{
    unsigned short crc = 0;
    int j, nextj;
    spritetype *spr;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_ACTOR], j, nextj)
    {
        spr = &sprite[j];
        updatecrc(crc, (spr->x) & 255);
        updatecrc(crc, (spr->y) & 255);
        updatecrc(crc, (spr->z) & 255);
        updatecrc(crc, (spr->lotag) & 255);
        updatecrc(crc, (spr->hitag) & 255);
        updatecrc(crc, (spr->ang) & 255);
    }

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_ZOMBIEACTOR], j, nextj)
    {
        spr = &sprite[j];
        updatecrc(crc, (spr->x) & 255);
        updatecrc(crc, (spr->y) & 255);
        updatecrc(crc, (spr->z) & 255);
        updatecrc(crc, (spr->lotag) & 255);
        updatecrc(crc, (spr->hitag) & 255);
        updatecrc(crc, (spr->ang) & 255);
    }

    return ((char) crc & 255);
}

char Net_WeaponSync(void)
{
    unsigned short crc = 0;
    int j, nextj;
    spritetype *spr;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_PROJECTILE], j, nextj)
    {
        spr = &sprite[j];
        updatecrc(crc, (spr->x) & 255);
        updatecrc(crc, (spr->y) & 255);
        updatecrc(crc, (spr->z) & 255);
        updatecrc(crc, (spr->ang) & 255);
    }

    return ((char) crc & 255);
}

char Net_MapSync(void)
{
    unsigned short crc = 0;
    int j, nextj;
    spritetype *spr;
    walltype *wal;
    sectortype *sect;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_EFFECTOR], j, nextj)
    {
        spr = &sprite[j];
        updatecrc(crc, (spr->x) & 255);
        updatecrc(crc, (spr->y) & 255);
        updatecrc(crc, (spr->z) & 255);
        updatecrc(crc, (spr->ang) & 255);
        updatecrc(crc, (spr->lotag) & 255);
        updatecrc(crc, (spr->hitag) & 255);
    }

    for (j=numwalls;j>=0;j--)
    {
        wal = &wall[j];
        updatecrc(crc, (wal->x) & 255);
        updatecrc(crc, (wal->y) & 255);
    }

    for (j=numsectors;j>=0;j--)
    {
        sect = &sector[j];
        updatecrc(crc, (sect->floorz) & 255);
        updatecrc(crc, (sect->ceilingz) & 255);
    }

    return ((char) crc & 255);
}

char Net_RandomSync(void)
{
    unsigned short crc = 0;

    updatecrc(crc, randomseed & 255);
    updatecrc(crc, (randomseed >> 8) & 255);
    updatecrc(crc, g_globalRandom & 255);
    updatecrc(crc, (g_globalRandom >> 8) & 255);

    if (g_numSyncBytes == 1)
    {
        updatecrc(crc,Net_PlayerSync() & 255);
        updatecrc(crc,Net_PlayerSync2() & 255);
        updatecrc(crc,Net_WeaponSync() & 255);
        updatecrc(crc,Net_ActorSync() & 255);
    }

    return ((char) crc & 255);
}

char *SyncNames[] =
{
    "Net_CheckRandomSync",
    "Net_CheckPlayerSync",
    "Net_CheckPlayerSync2",
    "Net_CheckWeaponSync",
    "Net_CheckActorSync",
    "Net_CheckMapSync",
    NULL
};

static char(*SyncFunc[MAXSYNCBYTES + 1])(void) =
{
    Net_RandomSync,
    Net_PlayerSync,
    Net_PlayerSync2,
    Net_WeaponSync,
    Net_ActorSync,
    Net_MapSync,
    NULL
};

void Net_GetSyncStat(void)
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


void Net_DisplaySyncMsg(void)
{
    int i, j;
    static unsigned int moveCount = 0;
    extern unsigned int g_moveThingsCount;

//    if (!SyncPrintMode)
//        return;

    if (numplayers < 2)
        return;

    for (i = 0; i < g_numSyncBytes; i++)
    {
        // syncstat is NON 0 - out of sync
        if (syncstat[i] != 0)
        {
            if (g_numSyncBytes > 1)
            {
                sprintf(tempbuf, "Out Of Sync - %s", SyncNames[i]);
                printext256(4L, 100L + (i * 8), 31, 1, tempbuf, 0);
            }

            if (!g_foundSyncError && g_szfirstSyncMsg[i][0] == '\0')
            {
                // g_foundSyncError one so test all of them and then never test again
                g_foundSyncError = TRUE;

                // save off loop count
                moveCount = g_moveThingsCount;

                for (j = 0; j < g_numSyncBytes; j++)
                {
                    if (syncstat[j] != 0 && g_szfirstSyncMsg[j][0] == '\0')
                    {
                        sprintf(tempbuf, "Out Of Sync - %s", SyncNames[j]);
                        strcpy(g_szfirstSyncMsg[j], tempbuf);
                    }
                }
            }
        }
    }

    // print out the g_szfirstSyncMsg message you got
    for (i = 0; i < g_numSyncBytes; i++)
    {
        if (g_szfirstSyncMsg[i][0] != '\0')
        {
            if (g_numSyncBytes > 1)
            {
                sprintf(tempbuf, "FIRST %s", g_szfirstSyncMsg[i]);
                printext256(4L, 44L + (i * 8), 31, 1, tempbuf, 0);
                sprintf(tempbuf, "moveCount %d",moveCount);
                printext256(4L, 52L + (i * 8), 31, 1, tempbuf, 0);
            }
            else
            {
                printext256(4L,100L,31,0,"Out Of Sync - Please restart game",0);
            }
        }
    }

//    if (syncstate != 0)
//        printext256(68L, 92L, 1, 31, "Missed Network packet!", 0);
}


void  Net_AddSyncInfoToPacket(int *j)
{
    int sb;
    int count = 0;

    // sync testing
    while (g_player[myconnectindex].syncvalhead != syncvaltail && count++ < 4)
    {
        for (sb = 0; sb < g_numSyncBytes; sb++)
            packbuf[(*j)++] = g_player[myconnectindex].syncval[syncvaltail & (SYNCFIFOSIZ - 1)][sb];

        syncvaltail++;
    }
}

void Net_GetSyncInfoFromPacket(char *packbuf, int packbufleng, int *j, int otherconnectindex)
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

    TRAVERSE_CONNECT(i)
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
        for (sb = 0; sb < g_numSyncBytes; sb++)
        {
            ppo->syncval[ppo->syncvalhead & (SYNCFIFOSIZ - 1)][sb] = packbuf[(*j)++];
        }
        ppo->syncvalhead++;
    }

    // update syncstat
    // if any of the syncstat vars is non-0 then there is a problem
    TRAVERSE_CONNECT(i)
    {
        if (g_player[i].syncvalhead == syncvaltottail)
            return;
    }

    //for (sb = 0; sb < g_numSyncBytes; sb++)
    //    syncstat[sb] = 0;

    while (1)
    {
        for (i = connectpoint2[connecthead]; i >= 0; i = connectpoint2[i])
        {
            for (sb = 0; sb < g_numSyncBytes; sb++)
            {
                if (g_player[i].syncval[syncvaltottail & (SYNCFIFOSIZ - 1)][sb] != g_player[connecthead].syncval[syncvaltottail & (SYNCFIFOSIZ - 1)][sb])
                {
                    syncstat[sb] = 1;
                }
            }
        }

        syncvaltottail++;

        TRAVERSE_CONNECT(i)
        {
            if (g_player[i].syncvalhead == syncvaltottail)
                return;
        }
    }
}


