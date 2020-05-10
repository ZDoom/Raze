//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#include "ns.h"	// Must come before everything else!

#define actors_c_

#include "duke3d.h"

BEGIN_DUKE_NS

#if KRANDDEBUG
# define ACTOR_STATIC
#else
# define ACTOR_STATIC static
#endif

#define DELETE_SPRITE_AND_CONTINUE(KX) do { A_DeleteSprite(KX); goto next_sprite; } while (0)

void G_ClearCameraView(DukePlayer_t *ps)
{
    ps->newowner = -1;
    ps->pos = ps->opos;
    ps->q16ang = ps->oq16ang;

    updatesector(ps->pos.x, ps->pos.y, &ps->cursectnum);
    P_UpdateScreenPal(ps);

    for (bssize_t SPRITES_OF(STAT_ACTOR, k))
        if (sprite[k].picnum==TILE_CAMERA1)
            sprite[k].yvel = 0;
}

void hitradius(short i, int  r, int  hp1, int  hp2, int  hp3, int  hp4);

void A_RadiusDamage(int spriteNum, int blastRadius, int dmg1, int dmg2, int dmg3, int dmg4)
{
    hitradius(spriteNum, blastRadius, dmg1, dmg2, dmg3, dmg4);
}


int movesprite(short spritenum, int xchange, int ychange, int zchange, unsigned int cliptype);

int32_t A_MoveSprite(int32_t spriteNum, vec3_t const * const change, uint32_t clipType)
{

    return movesprite(spriteNum, change->x, change->y, change->z, clipType);
}

int32_t block_deletesprite = 0;

#ifdef POLYMER
static void A_DeleteLight(int32_t s)
{
    if (actor[s].lightId >= 0)
        polymer_deletelight(actor[s].lightId);
    actor[s].lightId = -1;
    actor[s].lightptr = NULL;
}

void G_Polymer_UnInit(void)
{
    int32_t i;

    for (i=0; i<MAXSPRITES; i++)
        A_DeleteLight(i);
}
#endif

// deletesprite() game wrapper
void A_DeleteSprite(int spriteNum)
{
    if (EDUKE32_PREDICT_FALSE(block_deletesprite))
    {
        Printf(TEXTCOLOR_RED "A_DeleteSprite(): tried to remove sprite %d in EVENT_EGS\n", spriteNum);
        return;
    }

#ifdef POLYMER
    if (actor[spriteNum].lightptr != NULL && videoGetRenderMode() == REND_POLYMER)
        A_DeleteLight(spriteNum);
#endif

    // AMBIENT_SFX_PLAYING
    if (sprite[spriteNum].picnum == TILE_MUSICANDSFX && actor[spriteNum].t_data[0] == 1)
        S_StopEnvSound(sprite[spriteNum].lotag, spriteNum);

    // NetAlloc
    //if (Net_IsRelevantSprite(spriteNum))
    //{
    //    Net_DeleteSprite(spriteNum);
    //    return;
    //}

    deletesprite(spriteNum);
}

void insertspriteq(int i);

void A_AddToDeleteQueue(int spriteNum)
{
    insertspriteq(spriteNum);
}

void A_SpawnMultiple(int spriteNum, int tileNum, int spawnCnt)
{
    spritetype *pSprite = &sprite[spriteNum];

    for (; spawnCnt>0; spawnCnt--)
    {
        int32_t const r1 = krand2(), r2 = krand2();
        int const j = A_InsertSprite(pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z - (r2 % (47 << 8)), tileNum, -32, 8,
                               8, r1 & 2047, 0, 0, spriteNum, 5);
        //A_Spawn(-1, j);
        sprite[j].cstat = krand2()&12;
    }
}

void guts(spritetype* s, short gtype, short n, short p);
void gutsdir(spritetype* s, short gtype, short n, short p);

void A_DoGuts(int spriteNum, int tileNum, int spawnCnt)
{
    guts(&sprite[spriteNum], tileNum, spawnCnt, 0);
}

void A_DoGutsDir(int spriteNum, int tileNum, int spawnCnt)
{
    gutsdir(&sprite[spriteNum], tileNum, spawnCnt, 0);
}

static int32_t G_ToggleWallInterpolation(int32_t wallNum, int32_t setInterpolation)
{
    if (setInterpolation)
    {
        return G_SetInterpolation(&wall[wallNum].x) || G_SetInterpolation(&wall[wallNum].y);
    }
    else
    {
        G_StopInterpolation(&wall[wallNum].x);
        G_StopInterpolation(&wall[wallNum].y);
        return 0;
    }
}

void Sect_ToggleInterpolation(int sectNum, int setInterpolation)
{
    for (bssize_t j = sector[sectNum].wallptr, endwall = sector[sectNum].wallptr + sector[sectNum].wallnum; j < endwall; j++)
    {
        G_ToggleWallInterpolation(j, setInterpolation);

        int const nextWall = wall[j].nextwall;

        if (nextWall >= 0)
        {
            G_ToggleWallInterpolation(nextWall, setInterpolation);
            G_ToggleWallInterpolation(wall[nextWall].point2, setInterpolation);
        }
    }
}

void ms(short i);

void A_MoveSector(int spriteNum)
{
    ms(spriteNum);
}

// NOTE: T5 is AC_ACTION_ID
# define LIGHTRAD_PICOFS(i) (T5(i) ? *(apScript + T5(i)) + (*(apScript + T5(i) + 2)) * AC_CURFRAME(actor[i].t_data) : 0)

// this is the same crap as in game.c's tspr manipulation.  puke.
// XXX: may access tilesizy out-of-bounds by bad user code.
#define LIGHTRAD(spriteNum, s) (s->yrepeat * tilesiz[s->picnum + LIGHTRAD_PICOFS(spriteNum)].y)
#define LIGHTRAD2(spriteNum, s) ((s->yrepeat + ((rand() % s->yrepeat)>>2)) * tilesiz[s->picnum + LIGHTRAD_PICOFS(spriteNum)].y)

void G_AddGameLight(int lightRadius, int spriteNum, int zOffset, int lightRange, int lightColor, int lightPrio)
{
#ifdef POLYMER
    spritetype *s = &sprite[spriteNum];

    if (videoGetRenderMode() != REND_POLYMER || pr_lighting != 1)
        return;

    if (actor[spriteNum].lightptr == NULL)
    {
#pragma pack(push, 1)
        _prlight mylight;
#pragma pack(pop)
        Bmemset(&mylight, 0, sizeof(mylight));

        mylight.sector = s->sectnum;
        mylight.x = s->x;
        mylight.y = s->y;
        mylight.z = s->z - zOffset;
        mylight.color[0] = lightColor & 255;
        mylight.color[1] = (lightColor >> 8) & 255;
        mylight.color[2] = (lightColor >> 16) & 255;
        mylight.radius = lightRadius;
        actor[spriteNum].lightmaxrange = mylight.range = lightRange;

        mylight.priority = lightPrio;
        mylight.tilenum = 0;

        mylight.publicflags.emitshadow = 1;
        mylight.publicflags.negative = 0;

        actor[spriteNum].lightId = polymer_addlight(&mylight);
        if (actor[spriteNum].lightId >= 0)
            actor[spriteNum].lightptr = &prlights[actor[spriteNum].lightId];
        return;
    }

    s->z -= zOffset;

    if (lightRange<actor[spriteNum].lightmaxrange>> 1)
        actor[spriteNum].lightmaxrange = 0;

    if (lightRange > actor[spriteNum].lightmaxrange || lightPrio != actor[spriteNum].lightptr->priority ||
        Bmemcmp(&sprite[spriteNum], actor[spriteNum].lightptr, sizeof(int32_t) * 3))
    {
        if (lightRange > actor[spriteNum].lightmaxrange)
            actor[spriteNum].lightmaxrange = lightRange;

        Bmemcpy(actor[spriteNum].lightptr, &sprite[spriteNum], sizeof(int32_t) * 3);
        actor[spriteNum].lightptr->sector = s->sectnum;
        actor[spriteNum].lightptr->flags.invalidate = 1;
    }

    actor[spriteNum].lightptr->priority = lightPrio;
    actor[spriteNum].lightptr->range = lightRange;
    actor[spriteNum].lightptr->color[0] = lightColor & 255;
    actor[spriteNum].lightptr->color[1] = (lightColor >> 8) & 255;
    actor[spriteNum].lightptr->color[2] = (lightColor >> 16) & 255;

    s->z += zOffset;

#else
    UNREFERENCED_PARAMETER(lightRadius);
    UNREFERENCED_PARAMETER(spriteNum);
    UNREFERENCED_PARAMETER(zOffset);
    UNREFERENCED_PARAMETER(lightRange);
    UNREFERENCED_PARAMETER(lightColor);
    UNREFERENCED_PARAMETER(lightPrio);
#endif
}

int g_canSeePlayer = 0;

int G_WakeUp(spritetype *const pSprite, int const playerNum)
{
    DukePlayer_t *const pPlayer = g_player[playerNum].ps;
    if (!pPlayer->make_noise)
        return 0;
    int const radius = pPlayer->noise_radius;

    if (pSprite->pal == 30 || pSprite->pal == 32 || pSprite->pal == 33 || (RRRA && pSprite->pal == 8))
        return 0;

    return (pPlayer->noise_x - radius < pSprite->x && pPlayer->noise_x + radius > pSprite->x
        && pPlayer->noise_y - radius < pSprite->y && pPlayer->noise_y + radius > pSprite->y);
}


// sleeping monsters, etc

// stupid name, but it's what the function does.
int ifhitsectors(int sectnum);

static FORCE_INLINE int G_FindExplosionInSector(int const sectNum)
{
    return ifhitsectors(sectNum);
}

int ifhitbyweapon(int s);
int A_IncurDamage(int const spriteNum)
{
    return ifhitbyweapon(spriteNum);
}


 int A_FindLocator(int const tag, int const sectNum)
{
    for (bssize_t SPRITES_OF(STAT_LOCATOR, spriteNum))
    {
        if ((sectNum == -1 || sectNum == SECT(spriteNum)) && tag == SLT(spriteNum))
            return spriteNum;
    }

    return -1;
}



static void G_DoEffectorLights(void)  // STATNUM 14
{
#ifdef POLYMER
	int32_t i;

    for (SPRITES_OF(STAT_LIGHT, i))
    {
        switch (sprite[i].lotag)
        {
        case SE_49_POINT_LIGHT:
        {
            if (!A_CheckSpriteFlags(i, SFLAG_NOLIGHT) && videoGetRenderMode() == REND_POLYMER &&
                    !(A_CheckSpriteFlags(i, SFLAG_USEACTIVATOR) && sector[sprite[i].sectnum].lotag & 16384))
            {
                if (actor[i].lightptr == NULL)
                {
#pragma pack(push,1)
                    _prlight mylight;
#pragma pack(pop)
                    mylight.sector = SECT(i);
                    Bmemcpy(&mylight, &sprite[i], sizeof(int32_t) * 3);
                    mylight.range = SHT(i);
                    mylight.color[0] = sprite[i].xvel;
                    mylight.color[1] = sprite[i].yvel;
                    mylight.color[2] = sprite[i].zvel;
                    mylight.radius = 0;
                    mylight.angle = SA(i);
                    mylight.horiz = SH(i);
                    mylight.minshade = sprite[i].xoffset;
                    mylight.maxshade = sprite[i].yoffset;
                    mylight.tilenum = 0;
                    mylight.publicflags.emitshadow = 0;
                    mylight.publicflags.negative = !!(CS(i) & 128);

                    if (CS(i) & 2)
                    {
                        if (CS(i) & 512)
                            mylight.priority = PR_LIGHT_PRIO_LOW;
                        else
                            mylight.priority = PR_LIGHT_PRIO_HIGH;
                    }
                    else
                        mylight.priority = PR_LIGHT_PRIO_MAX;

                    actor[i].lightId = polymer_addlight(&mylight);
                    if (actor[i].lightId >= 0)
                        actor[i].lightptr = &prlights[actor[i].lightId];
                    break;
                }

                if (Bmemcmp(&sprite[i], actor[i].lightptr, sizeof(int32_t) * 3))
                {
                    Bmemcpy(actor[i].lightptr, &sprite[i], sizeof(int32_t) * 3);
                    actor[i].lightptr->sector = sprite[i].sectnum;
                    actor[i].lightptr->flags.invalidate = 1;
                }
                if (SHT(i) != actor[i].lightptr->range)
                {
                    actor[i].lightptr->range = SHT(i);
                    actor[i].lightptr->flags.invalidate = 1;
                }
                if ((sprite[i].xvel != actor[i].lightptr->color[0]) ||
                        (sprite[i].yvel != actor[i].lightptr->color[1]) ||
                        (sprite[i].zvel != actor[i].lightptr->color[2]))
                {
                    actor[i].lightptr->color[0] = sprite[i].xvel;
                    actor[i].lightptr->color[1] = sprite[i].yvel;
                    actor[i].lightptr->color[2] = sprite[i].zvel;
                }
                if ((int)!!(CS(i) & 128) != actor[i].lightptr->publicflags.negative) {
                    actor[i].lightptr->publicflags.negative = !!(CS(i) & 128);
                }
            }
            break;
        }
        case SE_50_SPOT_LIGHT:
        {
            if (!A_CheckSpriteFlags(i, SFLAG_NOLIGHT) && videoGetRenderMode() == REND_POLYMER &&
                    !(A_CheckSpriteFlags(i, SFLAG_USEACTIVATOR) && sector[sprite[i].sectnum].lotag & 16384))
            {
                if (actor[i].lightptr == NULL)
                {
#pragma pack(push,1)
                    _prlight mylight;
#pragma pack(pop)

                    mylight.sector = SECT(i);
                    Bmemcpy(&mylight, &sprite[i], sizeof(int32_t) * 3);
                    mylight.range = SHT(i);
                    mylight.color[0] = sprite[i].xvel;
                    mylight.color[1] = sprite[i].yvel;
                    mylight.color[2] = sprite[i].zvel;
                    mylight.radius = (256-(SS(i)+128))<<1;
                    mylight.faderadius = (int16_t)(mylight.radius * 0.75f);
                    mylight.angle = SA(i);
                    mylight.horiz = SH(i);
                    mylight.minshade = sprite[i].xoffset;
                    mylight.maxshade = sprite[i].yoffset;
                    mylight.tilenum = actor[i].picnum;
                    mylight.publicflags.emitshadow = !(CS(i) & 64);
                    mylight.publicflags.negative = !!(CS(i) & 128);

                    if (CS(i) & 2)
                    {
                        if (CS(i) & 512)
                            mylight.priority = PR_LIGHT_PRIO_LOW;
                        else
                            mylight.priority = PR_LIGHT_PRIO_HIGH;
                    }
                    else
                        mylight.priority = PR_LIGHT_PRIO_MAX;

                    actor[i].lightId = polymer_addlight(&mylight);
                    if (actor[i].lightId >= 0)
                    {
                        actor[i].lightptr = &prlights[actor[i].lightId];

                        // Hack in case polymer_addlight tweaked the horiz value
                        if (actor[i].lightptr->horiz != SH(i))
                            SH(i) = actor[i].lightptr->horiz;
                    }
                    break;
                }

                if (Bmemcmp(&sprite[i], actor[i].lightptr, sizeof(int32_t) * 3))
                {
                    Bmemcpy(actor[i].lightptr, &sprite[i], sizeof(int32_t) * 3);
                    actor[i].lightptr->sector = sprite[i].sectnum;
                    actor[i].lightptr->flags.invalidate = 1;
                }
                if (SHT(i) != actor[i].lightptr->range)
                {
                    actor[i].lightptr->range = SHT(i);
                    actor[i].lightptr->flags.invalidate = 1;
                }
                if ((sprite[i].xvel != actor[i].lightptr->color[0]) ||
                        (sprite[i].yvel != actor[i].lightptr->color[1]) ||
                        (sprite[i].zvel != actor[i].lightptr->color[2]))
                {
                    actor[i].lightptr->color[0] = sprite[i].xvel;
                    actor[i].lightptr->color[1] = sprite[i].yvel;
                    actor[i].lightptr->color[2] = sprite[i].zvel;
                }
                if (((256-(SS(i)+128))<<1) != actor[i].lightptr->radius)
                {
                    actor[i].lightptr->radius = (256-(SS(i)+128))<<1;
                    actor[i].lightptr->faderadius = (int16_t)(actor[i].lightptr->radius * 0.75f);
                    actor[i].lightptr->flags.invalidate = 1;
                }
                if (SA(i) != actor[i].lightptr->angle)
                {
                    actor[i].lightptr->angle = SA(i);
                    actor[i].lightptr->flags.invalidate = 1;
                }
                if (SH(i) != actor[i].lightptr->horiz)
                {
                    actor[i].lightptr->horiz = SH(i);
                    actor[i].lightptr->flags.invalidate = 1;
                }
                if ((int)!(CS(i) & 64) != actor[i].lightptr->publicflags.emitshadow) {
                    actor[i].lightptr->publicflags.emitshadow = !(CS(i) & 64);
                }
                if ((int)!!(CS(i) & 128) != actor[i].lightptr->publicflags.negative) {
                    actor[i].lightptr->publicflags.negative = !!(CS(i) & 128);
                }
                actor[i].lightptr->tilenum = actor[i].picnum;
            }

            break;
        }
        }
    }
#endif // POLYMER
}

#ifdef POLYMER
static void A_DoLight(int spriteNum)
{
    spritetype *const pSprite = &sprite[spriteNum];
    int savedFires = 0;

    if (((sector[pSprite->sectnum].floorz - sector[pSprite->sectnum].ceilingz) < 16) || pSprite->z > sector[pSprite->sectnum].floorz || pSprite->z > actor[spriteNum].floorz ||
        (pSprite->picnum != TILE_SECTOREFFECTOR && ((pSprite->cstat & 32768) || pSprite->yrepeat < 4)) ||
        A_CheckSpriteFlags(spriteNum, SFLAG_NOLIGHT) || (A_CheckSpriteFlags(spriteNum, SFLAG_USEACTIVATOR) && sector[pSprite->sectnum].lotag & 16384))
    {
        if (actor[spriteNum].lightptr != NULL)
            A_DeleteLight(spriteNum);
    }
    else
    {
        if (actor[spriteNum].lightptr != NULL && actor[spriteNum].lightcount)
        {
            if (!(--actor[spriteNum].lightcount))
                A_DeleteLight(spriteNum);
        }

        if (pr_lighting != 1)
            return;

        for (bsize_t ii=0; ii<2; ii++)
        {
            if (pSprite->picnum <= 0)  // oob safety
                break;

            switch (DYNAMICTILEMAP(pSprite->picnum-1+ii))
            {
            case DIPSWITCH__STATIC:
            case DIPSWITCH2__STATIC:
            case DIPSWITCH3__STATIC:
            case PULLSWITCH__STATIC:
            case SLOTDOOR__STATIC:
            case LIGHTSWITCH__STATIC:
            case SPACELIGHTSWITCH__STATIC:
            case SPACEDOORSWITCH__STATIC:
            case FRANKENSTINESWITCH__STATIC:
            case POWERSWITCH1__STATIC:
            case LOCKSWITCH1__STATIC:
            case POWERSWITCH2__STATIC:
            case TECHSWITCH__STATIC:
            case ACCESSSWITCH__STATIC:
            case ACCESSSWITCH2__STATIC:
                {
                    if ((pSprite->cstat & 32768) || A_CheckSpriteFlags(spriteNum, SFLAG_NOLIGHT))
                    {
                        if (actor[spriteNum].lightptr != NULL)
                            A_DeleteLight(spriteNum);
                        break;
                    }

                    vec2_t const d = { sintable[(pSprite->ang+512)&2047]>>7, sintable[(pSprite->ang)&2047]>>7 };

                    pSprite->x += d.x;
                    pSprite->y += d.y;

                    int16_t sectnum = pSprite->sectnum;
                    updatesector(pSprite->x, pSprite->y, &sectnum);

                    if ((unsigned) sectnum >= MAXSECTORS || pSprite->z > sector[sectnum].floorz || pSprite->z < sector[sectnum].ceilingz)
                        goto TILE_POOP;

                    G_AddGameLight(0, spriteNum, (pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1, 512-ii*128,
                        ii==0 ? (172+(200<<8)+(104<<16)) : 216+(52<<8)+(20<<16), PR_LIGHT_PRIO_LOW);

                TILE_POOP:
                    pSprite->x -= d.x;
                    pSprite->y -= d.y;
                }
                break;
            }
        }

        switch (DYNAMICTILEMAP(pSprite->picnum))
        {
        case ATOMICHEALTH__STATIC:
            G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), LIGHTRAD2(spriteNum, pSprite), 128+(128<<8)+(255<<16),PR_LIGHT_PRIO_HIGH_GAME);
            break;

        case FIRE__STATIC:
        case FIRE2__STATIC:
        case BURNING__STATIC:
        case BURNING2__STATIC:
            {
                uint32_t color;
                int32_t jj;

                static int32_t savedfires[32][4];  // sectnum x y z

                /*
                if (Actor[i].floorz - Actor[i].ceilingz < 128) break;
                if (s->z > Actor[i].floorz+2048) break;
                */

                switch (pSprite->pal)
                {
                case 1: color = 128+(128<<8)+(255<<16); break;
                case 2: color = 255+(48<<8)+(48<<16); break;
                case 8: color = 48+(255<<8)+(48<<16); break;
                default: color = 240+(160<<8)+(80<<16); break;
                }

                for (jj=savedFires-1; jj>=0; jj--)
                    if (savedfires[jj][0]==pSprite->sectnum && savedfires[jj][1]==(pSprite->x>>3) &&
                        savedfires[jj][2]==(pSprite->y>>3) && savedfires[jj][3]==(pSprite->z>>7))
                        break;

                if (jj==-1 && savedFires<32)
                {
                    jj = savedFires;
                    G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), LIGHTRAD2(spriteNum, pSprite), color, PR_LIGHT_PRIO_HIGH_GAME);
                    savedfires[jj][0] = pSprite->sectnum;
                    savedfires[jj][1] = pSprite->x>>3;
                    savedfires[jj][2] = pSprite->y>>3;
                    savedfires[jj][3] = pSprite->z>>7;
                    savedFires++;
                }
            }
            break;

        case OOZFILTER__STATIC:
            if (pSprite->xrepeat > 4)
                G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), 4096, 176+(252<<8)+(120<<16),PR_LIGHT_PRIO_HIGH_GAME);
            break;
        case FLOORFLAME__STATIC:
        case FIREBARREL__STATIC:
        case FIREVASE__STATIC:
            G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<2), LIGHTRAD2(spriteNum, pSprite)>>1, 255+(95<<8),PR_LIGHT_PRIO_HIGH_GAME);
            break;

        case EXPLOSION2__STATIC:
            if (!actor[spriteNum].lightcount)
            {
                // XXX: This block gets CODEDUP'd too much.
                int32_t x = ((sintable[(pSprite->ang+512)&2047])>>6);
                int32_t y = ((sintable[(pSprite->ang)&2047])>>6);

                pSprite->x -= x;
                pSprite->y -= y;

                G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), LIGHTRAD(spriteNum, pSprite), 240+(160<<8)+(80<<16),
                    pSprite->yrepeat > 32 ? PR_LIGHT_PRIO_HIGH_GAME : PR_LIGHT_PRIO_LOW_GAME);

                pSprite->x += x;
                pSprite->y += y;
            }
            break;
        case FORCERIPPLE__STATIC:
        case TRANSPORTERBEAM__STATIC:
            G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), LIGHTRAD(spriteNum, pSprite), 80+(80<<8)+(255<<16),PR_LIGHT_PRIO_LOW_GAME);
            break;
        case GROWSPARK__STATIC:
            {
                int32_t x = ((sintable[(pSprite->ang+512)&2047])>>6);
                int32_t y = ((sintable[(pSprite->ang)&2047])>>6);

                pSprite->x -= x;
                pSprite->y -= y;

                G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), 1024, 216+(52<<8)+(20<<16),PR_LIGHT_PRIO_HIGH_GAME);

                pSprite->x += x;
                pSprite->y += y;
            }
            break;
        case SHRINKEREXPLOSION__STATIC:
            {
                int32_t x = ((sintable[(pSprite->ang+512)&2047])>>6);
                int32_t y = ((sintable[(pSprite->ang)&2047])>>6);

                pSprite->x -= x;
                pSprite->y -= y;

                G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), 2048, 176+(252<<8)+(120<<16),PR_LIGHT_PRIO_HIGH_GAME);

                pSprite->x += x;
                pSprite->y += y;
            }
            break;
        case FREEZEBLAST__STATIC:
            G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), LIGHTRAD(spriteNum, pSprite)<<2, 72+(88<<8)+(140<<16),PR_LIGHT_PRIO_HIGH_GAME);
            break;
        case COOLEXPLOSION1__STATIC:
            G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), LIGHTRAD(spriteNum, pSprite)<<2, 128+(0<<8)+(255<<16),PR_LIGHT_PRIO_HIGH_GAME);
            break;
        case SHRINKSPARK__STATIC:
            G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), LIGHTRAD(spriteNum, pSprite), 176+(252<<8)+(120<<16),PR_LIGHT_PRIO_HIGH_GAME);
            break;
        case FIRELASER__STATIC:
            if (pSprite->statnum == STAT_PROJECTILE)
                G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), 64 * pSprite->yrepeat, 255+(95<<8),PR_LIGHT_PRIO_LOW_GAME);
            break;
        case RPG__STATIC:
            G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), 128 * pSprite->yrepeat, 255+(95<<8),PR_LIGHT_PRIO_LOW_GAME);
            break;
        case SHOTSPARK1__STATIC:
            if (actor[spriteNum].t_data[2] == 0) // check for first frame of action
            {
                int32_t x = ((sintable[(pSprite->ang+512)&2047])>>7);
                int32_t y = ((sintable[(pSprite->ang)&2047])>>7);

                pSprite->x -= x;
                pSprite->y -= y;

                G_AddGameLight(0, spriteNum, ((pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1), 8 * pSprite->yrepeat, 240+(160<<8)+(80<<16),PR_LIGHT_PRIO_LOW_GAME);
                actor[spriteNum].lightcount = 1;

                pSprite->x += x;
                pSprite->y += y;
            }
            break;
        }
    }
}
#endif // POLYMER

void A_PlayAlertSound(int spriteNum)
{
    if (DEER)
        return;
    if (RR)
    {
        if (sprite[spriteNum].extra > 0)
        {
            switch (DYNAMICTILEMAP(PN(spriteNum)))
            {
                case COOT__STATICRR: if (!RRRA || (krand2()&3) == 2) A_PlaySound(PRED_RECOG, spriteNum); break;
                case LTH__STATICRR: break;
                case BILLYCOCK__STATICRR:
                case BILLYRAY__STATICRR:
                case BRAYSNIPER__STATICRR: A_PlaySound(PIG_RECOG, spriteNum); break;
                case DOGRUN__STATICRR:
                case HULK__STATICRR:
                case HEN__STATICRR:
                case DRONE__STATICRR:
                case PIG__STATICRR:
                case RECON__STATICRR:
                case MINION__STATICRR:
                case COW__STATICRR:
                case VIXEN__STATICRR:
                case RABBIT__STATICRR: break;
            }
        }
        return;
    }
    if (sprite[spriteNum].extra > 0)
    {
        switch (DYNAMICTILEMAP(PN(spriteNum)))
        {
            case LIZTROOPONTOILET__STATIC:
            case LIZTROOPJUSTSIT__STATIC:
            case LIZTROOPSHOOT__STATIC:
            case LIZTROOPJETPACK__STATIC:
            case LIZTROOPDUCKING__STATIC:
            case LIZTROOPRUNNING__STATIC:
            case LIZTROOP__STATIC:         A_PlaySound(PRED_RECOG, spriteNum); break;
            case LIZMAN__STATIC:
            case LIZMANSPITTING__STATIC:
            case LIZMANFEEDING__STATIC:
            case LIZMANJUMP__STATIC:       A_PlaySound(CAPT_RECOG, spriteNum); break;
            case PIGCOP__STATIC:
            case PIGCOPDIVE__STATIC:       A_PlaySound(PIG_RECOG, spriteNum); break;
            case RECON__STATIC:            A_PlaySound(RECO_RECOG, spriteNum); break;
            case DRONE__STATIC:            A_PlaySound(DRON_RECOG, spriteNum); break;
            case COMMANDER__STATIC:
            case COMMANDERSTAYPUT__STATIC: A_PlaySound(COMM_RECOG, spriteNum); break;
            case ORGANTIC__STATIC:         A_PlaySound(TURR_RECOG, spriteNum); break;
            case OCTABRAIN__STATIC:
            case OCTABRAINSTAYPUT__STATIC: A_PlaySound(OCTA_RECOG, spriteNum); break;
            case BOSS1__STATIC:            S_PlaySound(BOS1_RECOG); break;
            case BOSS2__STATIC:            S_PlaySound((sprite[spriteNum].pal == 1) ? BOS2_RECOG : WHIPYOURASS); break;
            case BOSS3__STATIC:            S_PlaySound((sprite[spriteNum].pal == 1) ? BOS3_RECOG : RIPHEADNECK); break;
            case BOSS4__STATIC:
            case BOSS4STAYPUT__STATIC:     if (sprite[spriteNum].pal == 1) S_PlaySound(BOS4_RECOG); S_PlaySound(BOSS4_FIRSTSEE); break;
            case GREENSLIME__STATIC:       A_PlaySound(SLIM_RECOG, spriteNum); break;
        }
    }
}

int A_CheckSwitchTile(int spriteNum)
{
    // picnum 0 would oob in the switch below,

    if (PN(spriteNum) <= 0)
        return 0;

    // TILE_MULTISWITCH has 4 states so deal with it separately,
    // TILE_ACCESSSWITCH and TILE_ACCESSSWITCH2 are only active in one state so deal with
    // them separately.

    if ((PN(spriteNum) >= TILE_MULTISWITCH && PN(spriteNum) <= TILE_MULTISWITCH + 3) || (PN(spriteNum) == TILE_ACCESSSWITCH || PN(spriteNum) == TILE_ACCESSSWITCH2))
        return 1;

    if (RRRA && PN(spriteNum) >= TILE_MULTISWITCH2 && PN(spriteNum) <= TILE_MULTISWITCH2 + 3)
        return 1;

    // Loop to catch both states of switches.
    for (bssize_t j=1; j>=0; j--)
    {
        switch (DYNAMICTILEMAP(PN(spriteNum)-j))
        {
        case RRTILE8464__STATICRR:
            if (RRRA) return 1;
            break;
        case NUKEBUTTON__STATIC:
            if (RR) return 1;
            break;
        case HANDPRINTSWITCH__STATIC:
        case ALIENSWITCH__STATIC:
        case MULTISWITCH__STATIC:
        case PULLSWITCH__STATIC:
        case HANDSWITCH__STATIC:
        case SLOTDOOR__STATIC:
        case LIGHTSWITCH__STATIC:
        case SPACELIGHTSWITCH__STATIC:
        case SPACEDOORSWITCH__STATIC:
        case FRANKENSTINESWITCH__STATIC:
        case LIGHTSWITCH2__STATIC:
        case POWERSWITCH1__STATIC:
        case LOCKSWITCH1__STATIC:
        case POWERSWITCH2__STATIC:
        case DIPSWITCH__STATIC:
        case DIPSWITCH2__STATIC:
        case TECHSWITCH__STATIC:
        case DIPSWITCH3__STATIC:
            return 1;
        }
    }

    return 0;
}

void G_RefreshLights(void)
{
#ifdef POLYMER
    if (Numsprites && videoGetRenderMode() == REND_POLYMER)
    {
        int statNum = 0;

        do
        {
            int spriteNum = headspritestat[statNum++];

            while (spriteNum >= 0)
            {
                A_DoLight(spriteNum);
                spriteNum = nextspritestat[spriteNum];
            }
        }
        while (statNum < MAXSTATUS);
    }
#endif
}

void movefta_d(void);
void movefallers_d();
void movestandables_d();
void moveweapons_d();
void movetransports_d(void);
void moveactors_d();
void moveexplosions_d();
void moveeffectors_d();

void movefta_r(void);
void moveplayers();
void movefx();
void movefallers_r();
void movestandables_r();
void moveweapons_r();
void movetransports_r(void);
void moveactors_r();
void thunder();
void moveexplosions_r();
void moveeffectors_r();

void doanimations(void);

void G_MoveWorld_d(void)
{
    extern double g_moveActorsTime, g_moveWorldTime;
    const double worldTime = timerGetHiTicks();

    movefta_d();     //ST 2
    moveweapons_d();          //ST 4
    movetransports_d();       //ST 9

    moveplayers();          //ST 10
    movefallers_d();          //ST 12
    moveexplosions_d();             //ST 5

    const double actorsTime = timerGetHiTicks();

    moveactors_d();           //ST 1

    g_moveActorsTime = (1-0.033)*g_moveActorsTime + 0.033*(timerGetHiTicks()-actorsTime);

    // XXX: Has to be before effectors, in particular movers?
    // TODO: lights in moving sectors ought to be interpolated
    G_DoEffectorLights();
    moveeffectors_d();        //ST 3
    movestandables_d();       //ST 6

    G_RefreshLights();
    doanimations();
    movefx();               //ST 11

    g_moveWorldTime = (1-0.033)*g_moveWorldTime + 0.033*(timerGetHiTicks()-worldTime);
}

void G_MoveWorld_r(void)
{
    extern double g_moveActorsTime, g_moveWorldTime;
    const double worldTime = timerGetHiTicks();

    if (!DEER)
    {
        movefta_r();     //ST 2
        moveweapons_r();          //ST 4
        movetransports_r();       //ST 9
    }

    moveplayers();          //ST 10
    movefallers_r();          //ST 12
    if (!DEER)
        moveexplosions_r();             //ST 5

    const double actorsTime = timerGetHiTicks();

    moveactors_r();           //ST 1

    g_moveActorsTime = (1 - 0.033) * g_moveActorsTime + 0.033 * (timerGetHiTicks() - actorsTime);

    if (DEER)
    {
        sub_56EA8();
        ghtarget_move();
        gharrow_move();
        ghdeploy_move();
        sub_519E8(ud.level_number);
        sub_5524C();
    }

    // XXX: Has to be before effectors, in particular movers?
    // TODO: lights in moving sectors ought to be interpolated
    G_DoEffectorLights();
    if (!DEER)
    {
        moveeffectors_r();        //ST 3
        movestandables_r();       //ST 6
    }

    G_RefreshLights();
    doanimations();
    if (!DEER)
        movefx();               //ST 11

    if (numplayers < 2 && g_thunderOn)
        thunder();

    g_moveWorldTime = (1 - 0.033) * g_moveWorldTime + 0.033 * (timerGetHiTicks() - worldTime);
}

void G_MoveWorld(void)
{
    if (!isRR()) G_MoveWorld_d();
    else G_MoveWorld_r();
}

END_DUKE_NS

