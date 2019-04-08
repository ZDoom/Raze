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

#include "build.h"

#include "names2.h"
#include "game.h"
#include "tags.h"
#include "common_game.h"
#include "break.h"
#include "quake.h"
#include "pal.h"
#include "cache.h"
#include "sounds.h"
#include "network.h"

// Run the game with the -CACHEPRINT option and redirect to a file.
// It will save out the tile and sound number every time one caches.
//
// sw -map $bullet -cacheprint > foofile
extern SWBOOL PreCaching;

// player weaponry, item usage, etc. Precache every time.
short Player_SCTable[] =
{
    1,2,3,4,5,6,7,8,9,10, // weapons
    11,12,14,16,18,20,
    22,23,24,25,26,27,28,30,
    31,32,33,34,35,40,145,291,445,362,269,
    158,476, // underwater
    47,359, // cloaking
    48,50, // dead head
    196,52,53,54,55, // splash & getting items
    56,57,58,73,74,410, // bodies hitting ground
    484,442, // teleport & respawn
    417,418, // healing
    238,239,240,241,242,243,244, // bring weapons up
    181,182,183,184,187,216, // explosions
    272,273,274,275,486, // nuke associated sounds
    276,277,278,279,477, // chem bomb
    280,281,282,283,284,288,289,290,450, // various player sounds
    295, // armor hit
    312, // sword clank
    324,325,209, // unlocking sound
    395,396,411, // gibs
    175, // drip
    435,436,311,221,220,227, // breakage
    246,247,248,249,250,251,252,253,254,255,256, // common player talk
    257,258,259,260,261,262,263,264,266,267,438,439,440,441,
    326, // ancient chinese secret
    330,331,332,333,334,335,336,337,338,339,340, // player kill talk
    341,342,343,344,345,346,347,348,
    376,377,378,379,380,381,382,383,384,385,386,387, // more asst. talking
    370,443 // repair talk
};

// Actor specific sound tables. Cache only if the actor appears on the map.
// Exceptions would include ghosts, which are spawned by coolies, and
// rippers, which are spawned by the serpent boss.
short Coolie_SCTable[] =
{
    75,76,77,78,79
};

short Ghost_SCTable[] =
{
    80,81,82,83,84,85,86,87,213
};

short Ninja_SCTable[] =
{
    88,89,90,91,92,93,94,412,
    319, // firing sound
    430 // death by sword
};

short Ripper_SCTable[] =
{
    95,96,97,98,99,100,431
};

short Ripper2_SCTable[] =
{
    313,314,315,316,317,318,431
};

short Head_SCTable[] =
{
    115,116,117,118 // accursed heads
};

short Hornet_SCTable[] =
{
    119,120,121,122
};

short Guardian_SCTable[] =
{
    101,102,103,104,105,106,107
};

short Serpent_SCTable[] =
{
    123,124,125,126,127,128,129,130,131 // serpent boss
};

short Sumo_SCTable[] =
{
    320,321,322,323 // sumo boss
};

short Bunny_SCTable[] =
{
    424,425,426,427,428
};

short Toilet_SCTable[] =
{
    388,389,390,391,392,393,488,489,490 // anime girl on toilet
}; // I suspect some of these are no longer in use

short Trash_SCTable[] =
{
    416 // I heard the trash can was an actor, so here is is
};

short Pachinko_SCTable[] =
{
    419,420,421,422,423
};

void PreCacheSoundList(short table[], int num);
void PreCacheTable(short table[], int num);
void PreCacheGhost(void);

void
SetupPreCache(void)
{
    if (PreCaching)
    {
        precache();

        PreCacheSoundList(Player_SCTable, SIZ(Player_SCTable));

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
    PreCacheSoundList(Ripper_SCTable, SIZ(Ripper_SCTable));
    PreCacheRange(1580, 1644);
}

void PreCacheRipper2(void)
{
    PreCacheSoundList(Ripper2_SCTable, SIZ(Ripper2_SCTable));
    PreCacheRange(4320, 4427);
}

void PreCacheCoolie(void)
{
    PreCacheGhost();
    PreCacheSoundList(Coolie_SCTable, SIZ(Coolie_SCTable));
    PreCacheRange(1400, 1440);
    PreCacheRange(4260, 4276); // coolie explode
}

void PreCacheGhost(void)
{
    PreCacheSoundList(Ghost_SCTable, SIZ(Ghost_SCTable));
    PreCacheRange(4277, 4312);
}

void PreCacheSerpent(void)
{
    PreCacheSoundList(Serpent_SCTable, SIZ(Serpent_SCTable));
    PreCacheRange(960, 1016);
    PreCacheRange(1300, 1314);
}

void PreCacheGuardian(void)
{
    PreCacheSoundList(Guardian_SCTable, SIZ(Guardian_SCTable));
    PreCacheRange(1469,1497);
}

void PreCacheNinja(void)
{
    PreCacheSoundList(Ninja_SCTable, SIZ(Ninja_SCTable));
    PreCacheRange(4096, 4239);
}

void PreCacheNinjaGirl(void)
{
    //PreCacheSoundList(NinjaGirl_SCTable, SIZ(NinjaGirl_SCTable));
    PreCacheRange(5162, 5260);
}

void PreCacheSumo(void)
{
    PreCacheSoundList(Sumo_SCTable, SIZ(Sumo_SCTable));
    PreCacheRange(4490, 4544);
}

void PreCacheZilla(void)
{
    PreCacheSoundList(Sumo_SCTable, SIZ(Sumo_SCTable));
    PreCacheRange(4490, 4544);
}

void PreCacheEel(void)
{
    PreCacheRange(4430, 4479);
}

void PreCacheToiletGirl(void)
{
    PreCacheSoundList(Toilet_SCTable, SIZ(Toilet_SCTable));
    PreCacheRange(5023, 5027);
}

void PreCacheWashGirl(void)
{
    PreCacheSoundList(Toilet_SCTable, SIZ(Toilet_SCTable));
    PreCacheRange(5032, 5035);
}

void PreCacheCarGirl(void)
{
    PreCacheSoundList(Toilet_SCTable, SIZ(Toilet_SCTable));
    PreCacheRange(4594,4597);
}

void PreCacheMechanicGirl(void)
{
    PreCacheSoundList(Toilet_SCTable, SIZ(Toilet_SCTable));
    PreCacheRange(4590,4593);
}

void PreCacheSailorGirl(void)
{
    PreCacheSoundList(Toilet_SCTable, SIZ(Toilet_SCTable));
    PreCacheRange(4600,4602);
}

void PreCachePruneGirl(void)
{
    PreCacheSoundList(Toilet_SCTable, SIZ(Toilet_SCTable));
    PreCacheRange(4604,4604);
}

void PreCacheTrash(void)
{
    PreCacheSoundList(Trash_SCTable, SIZ(Trash_SCTable));
    PreCacheRange(2540, 2546);
}

void PreCacheBunny(void)
{
    PreCacheSoundList(Bunny_SCTable, SIZ(Bunny_SCTable));
    PreCacheRange(4550, 4584);
}

void PreCacheSkel(void)
{
    PreCacheRange(1320, 1396);
}

void PreCacheHornet(void)
{
    PreCacheSoundList(Hornet_SCTable, SIZ(Hornet_SCTable));
    PreCacheRange(800, 811);
}

void PreCacheSkull(void)
{
    PreCacheSoundList(Head_SCTable, SIZ(Head_SCTable));
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
    PreCacheSoundList(Pachinko_SCTable, SIZ(Pachinko_SCTable));
}

void PreCacheSoundList(short table[], int num)
{
    short j;

    for (j = 0; j < num; j++)
    {
        CacheSound(table[j], CACHE_SOUND_PRECACHE);
        AnimateCacheCursor();
    }
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

void PreCacheAmbient(void)
{
    int i,nexti;
    int num;
    SPRITEp sp;
    extern AMB_INFO ambarray[];

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_AMBIENT], i, nexti)
    {
        sp = &sprite[i];

        num = sp->lotag;
        num = ambarray[num].diginame;

        CacheSound(num, CACHE_SOUND_PRECACHE);
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


void PreCacheSoundSpot(void)
{
    int i,nexti;
    int num;
    SPRITEp sp;

    TRAVERSE_SPRITE_STAT(headspritestat[STAT_SOUND_SPOT], i, nexti)
    {
        sp = &sprite[i];

        num = SP_TAG13(sp); // tag4 is copied to tag13
        if (num > 0 && num < DIGI_MAX)
            CacheSound(num, CACHE_SOUND_PRECACHE);

        num = SP_TAG5(sp);
        if (num > 0 && num < DIGI_MAX)
            CacheSound(num, CACHE_SOUND_PRECACHE);

        num = SP_TAG6(sp);
        if (num > 0 && num < DIGI_MAX)
            CacheSound(num, CACHE_SOUND_PRECACHE);

        CacheSound(num, CACHE_SOUND_PRECACHE);
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

    PreCacheAmbient();
    PreCacheSoundSpot();
    PreCacheActor();
    PreCacheOverride();

    for (i = 0; i < MAXTILES; i++)
    {
        if ((TEST(gotpic[i>>3], 1<<(i&7))) && (!waloff[i]))
        {
            tileLoad(i);
            cnt++;
            if (!(cnt&7))
            {
                AnimateCacheCursor();
                handleevents();
                getpackets();
            }
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


