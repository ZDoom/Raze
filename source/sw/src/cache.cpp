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

/****************************************
Removed sounds that were causing "Could
not load" error messages.
****************************************/
#include "ns.h"

#include "build.h"

#include "names2.h"
#include "game.h"
#include "tags.h"
#include "break.h"
#include "quake.h"
#include "pal.h"
#include "misc.h"
#include "sounds.h"
#include "network.h"

BEGIN_SW_NS

// Run the game with the -CACHEPRINT option and redirect to a file.
// It will save out the tile and sound number every time one caches.
//
// sw -map $bullet -cacheprint > foofile
extern SWBOOL PreCaching;

void PreCacheTable(short table[], int num);
void PreCacheGhost(void);

void
SetupPreCache(void)
{
    if (PreCaching)
    {
        precache();

       
        // actors cache ranges are called from SpriteSetup
        // only caches the actor if its on the level

        // weapons
        PreCacheRange(2000, 2227);
        PreCacheRange(4090, 4093);
        // Explosions
        PreCacheRange(3072, 3225);
        // ninja player character
        PreCacheRange(1024, 1175);
        // console
        PreCacheRange(2380, 2409);
        PreCacheRange(3600, 3645);
        PreCacheRange(2434, 2435);
        // common
        PreCacheRange(204, 208);
        // message font
        PreCacheRange(4608, 4701);
        // gibs
        PreCacheRange(1150,1568);
        PreCacheRange(1685,1690);
        PreCacheRange(900,944);
        PreCacheRange(1670,1681);
        // blood
        PreCacheRange(1710,1715);
        PreCacheRange(2410,2425);
        PreCacheRange(389,389); // blood puddle by itself in art file
        PreCacheRange(2500,2503);
        // shrap
        PreCacheRange(3840,3911);
        PreCacheRange(3924,3947);
        PreCacheRange(1397,1398);
        // water *** animated tiles, can be deleted now ***
        // PreCacheRange(780,794);
        // switches
        PreCacheRange(561,584);
        PreCacheRange(551,552);
        PreCacheRange(1846,1847);
        PreCacheRange(1850,1859);
        // bullet smoke
        PreCacheRange(1748,1753);
        // small blue font
        PreCacheRange(2930,3023);
        // gas can
        PreCacheRange(3038,3042);
        // lava *** animated tiles, can be deleted now ***
        // PreCacheRange(175,182);
        // gas clouds & teleport effect
        PreCacheRange(3240,3277);
        // nuke mushroom cloud
        PreCacheRange(3280,3300);
        // blood drops
        PreCacheRange(1718,1721);
        // smoke
        PreCacheRange(3948,3968);
        // footprints
        PreCacheRange(2490,2492);
        // player fists
        PreCacheRange(4070,4077);
        PreCacheRange(4050,4051);
        PreCacheRange(4090,4093);
        // fish actor
        PreCacheRange(3760,3771);
        PreCacheRange(3780,3795);
        // coins
        PreCacheRange(2531,2533);
        // respawn markers & console keys
        PreCacheRange(2440,2467);
        // light/torch sprites
        PreCacheRange(537,548);
        PreCacheRange(521,528);
        PreCacheRange(512,515);
        PreCacheRange(396,399);
        PreCacheRange(443,446);
        // bubbles
        PreCacheRange(716,720);
        // bullet splashes
        PreCacheRange(772,776);
    }
}

void PreCacheRipper(void)
{
    PreCacheRange(1580, 1644);
}

void PreCacheRipper2(void)
{
    PreCacheRange(4320, 4427);
}

void PreCacheCoolie(void)
{
    PreCacheGhost();
    PreCacheRange(1400, 1440);
    PreCacheRange(4260, 4276); // coolie explode
}

void PreCacheGhost(void)
{
    PreCacheRange(4277, 4312);
}

void PreCacheSerpent(void)
{
    PreCacheRange(960, 1016);
    PreCacheRange(1300, 1314);
}

void PreCacheGuardian(void)
{
    PreCacheRange(1469,1497);
}

void PreCacheNinja(void)
{
    PreCacheRange(4096, 4239);
}

void PreCacheNinjaGirl(void)
{
    PreCacheRange(5162, 5260);
}

void PreCacheSumo(void)
{
    PreCacheRange(4490, 4544);
}

void PreCacheZilla(void)
{
    PreCacheRange(4490, 4544);
}

void PreCacheEel(void)
{
    PreCacheRange(4430, 4479);
}

void PreCacheToiletGirl(void)
{
    PreCacheRange(5023, 5027);
}

void PreCacheWashGirl(void)
{
    PreCacheRange(5032, 5035);
}

void PreCacheCarGirl(void)
{
    PreCacheRange(4594,4597);
}

void PreCacheMechanicGirl(void)
{
    PreCacheRange(4590,4593);
}

void PreCacheSailorGirl(void)
{
    PreCacheRange(4600,4602);
}

void PreCachePruneGirl(void)
{
    PreCacheRange(4604,4604);
}

void PreCacheTrash(void)
{
    PreCacheRange(2540, 2546);
}

void PreCacheBunny(void)
{
    PreCacheRange(4550, 4584);
}

void PreCacheSkel(void)
{
    PreCacheRange(1320, 1396);
}

void PreCacheHornet(void)
{
    PreCacheRange(800, 811);
}

void PreCacheSkull(void)
{
    PreCacheRange(820, 854);
}

void PreCacheBetty(void)
{
    PreCacheRange(817, 819);
}

void PreCachePachinko(void)
{
    PreCacheRange(618,623);
    PreCacheRange(618,623);
    PreCacheRange(4768,4790);
    PreCacheRange(4792,4814);
    PreCacheRange(4816,4838);
    PreCacheRange(4840,4863);
}

void
PreCacheTable(short table[], int num)
{
    short j;

    for (j = 0; j < num; j++)
    {
        SET(gotpic[table[j]>>3], 1<<(table[j]&7));
    }
}

void
PreCacheRange(short start_pic, short end_pic)
{
    short j;

    for (j = start_pic; j <= end_pic; j++)
    {
        SET(gotpic[j>>3], 1<<(j&7));
    }
}

void PreCacheOverride(void)
{
    int i,nexti;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_CEILING_FLOOR_PIC_OVERRIDE], i, nexti)
    {
        ASSERT(SPRITE_TAG2(i) >= 0 && SPRITE_TAG2(i) <= MAXTILES);
        SET_GOTPIC(SPRITE_TAG2(i));
    }
}

void
PreCacheActor(void)
{
    int i;
    short pic;

    for (i=0; i < MAXSPRITES; i++)
    {
        if (sprite[i].statnum >= MAXSTATUS)
            continue;

        if (User[i])
            pic = User[i]->ID;
        else
            pic = sprite[i].picnum;

        switch (pic)
        {
        case COOLIE_RUN_R0:
            PreCacheCoolie();
            break;

        case NINJA_RUN_R0:
        case NINJA_CRAWL_R0:
            PreCacheNinja();
            break;

        case GORO_RUN_R0:
            PreCacheGuardian();
            break;

        case 1441:
        case COOLG_RUN_R0:
            PreCacheGhost();
            break;

        case EEL_RUN_R0:
            PreCacheEel();
            break;

        case SUMO_RUN_R0:
            PreCacheZilla();
            break;

        case ZILLA_RUN_R0:
            PreCacheSumo();
            break;

        case TOILETGIRL_R0:
            PreCacheToiletGirl();
            break;

        case WASHGIRL_R0:
            PreCacheWashGirl();
            break;

        case CARGIRL_R0:
            PreCacheCarGirl();
            break;

        case MECHANICGIRL_R0:
            PreCacheMechanicGirl();
            break;

        case SAILORGIRL_R0:
            PreCacheSailorGirl();
            break;

        case PRUNEGIRL_R0:
            PreCachePruneGirl();
            break;

        case TRASHCAN:
            PreCacheTrash();
            break;

        case BUNNY_RUN_R0:
            PreCacheBunny();
            break;

        case RIPPER_RUN_R0:
            PreCacheRipper();
            break;

        case RIPPER2_RUN_R0:
            PreCacheRipper2();
            break;

        case SERP_RUN_R0:
            PreCacheSerpent();
            break;

        case LAVA_RUN_R0:
            break;

        case SKEL_RUN_R0:
            PreCacheSkel();
            break;

        case HORNET_RUN_R0:
            PreCacheHornet();
            break;

        case SKULL_R0:
            PreCacheSkull();
            break;

        case BETTY_R0:
            PreCacheBetty();
            break;

        case GIRLNINJA_RUN_R0:
            PreCacheNinjaGirl();
            break;

        case 623:   // Pachinko win light
        case PACHINKO1:
        case PACHINKO2:
        case PACHINKO3:
        case PACHINKO4:
            PreCachePachinko();
            break;
        }
    }
}


void DoTheCache(void)
{
    extern char CacheLastLevel[32],LevelName[20];
    int i, cnt=0;

    PreCacheActor();
    PreCacheOverride();

    for (i = 0; i < MAXTILES; i++)
    {
        if ((TEST(gotpic[i>>3], 1<<(i&7))) && (!tilePtr(i)))
        {
            // Without palettes this is rather useless...
            if (r_precache) PrecacheHardwareTextures(i);
            cnt++;
        }
    }

    memset(gotpic,0,sizeof(gotpic));
    strcpy(CacheLastLevel, LevelName);
}

void
precache(void)
{
    int i;
    short j;
    SECTORp sectp;
    WALLp wp;
    SPRITEp sp;

    memset(gotpic,0,sizeof(gotpic));

    for (sectp = sector; sectp < &sector[numsectors]; sectp++)
    {
        j = sectp->ceilingpicnum;

        SET(gotpic[j>>3], 1<<(j&7));

        if (TEST(picanm[j].sf, PICANM_ANIMTYPE_MASK) >> PICANM_ANIMTYPE_SHIFT)
        {
            for (i = 1; i <= picanm[j].num; i++)
            {
                SET(gotpic[(j+i)>>3], 1<<((j+i)&7));
            }
        }

        j = sectp->floorpicnum;

        SET(gotpic[j>>3], 1<<(j&7));

        if (TEST(picanm[j].sf, PICANM_ANIMTYPE_MASK) >> PICANM_ANIMTYPE_SHIFT)
        {
            for (i = 1; i <= picanm[j].num; i++)
            {
                SET(gotpic[(j+i)>>3], 1<<((j+i)&7));
            }
        }

    }

    for (wp = wall; wp < &wall[numwalls]; wp++)
    {
        j = wp->picnum;

        SET(gotpic[j>>3], 1<<(j&7));

        if (TEST(picanm[j].sf, PICANM_ANIMTYPE_MASK) >> PICANM_ANIMTYPE_SHIFT)
        {
            for (i = 1; i <= picanm[j].num; i++)
            {
                SET(gotpic[(j+i)>>3], 1<<((j+i)&7));
            }
        }

        if (wp->overpicnum > 0 && wp->overpicnum < MAXTILES)
        {
            j = wp->overpicnum;
            SET(gotpic[j>>3], 1<<(j&7));

            if (TEST(picanm[j].sf, PICANM_ANIMTYPE_MASK) >> PICANM_ANIMTYPE_SHIFT)
            {
                for (i = 1; i <= picanm[j].num; i++)
                {
                    SET(gotpic[(j+i)>>3], 1<<((j+i)&7));
                }
            }

        }
    }

    for (sp = sprite; sp < &sprite[MAXSPRITES]; sp++)
    {
        if (sp->statnum < MAXSTATUS)
        {
            j = sp->picnum;

            SET(gotpic[j>>3], 1<<(j&7));
        }
    }
}


END_SW_NS
