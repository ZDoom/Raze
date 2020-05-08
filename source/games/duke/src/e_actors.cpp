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

// Manhattan distance between wall-point and sprite.
static FORCE_INLINE int32_t G_WallSpriteDist(uwalltype const * const wal, uspritetype const * const spr)
{
    return klabs(wal->x - spr->x) + klabs(wal->y - spr->y);
}

void hitradius(short i, int  r, int  hp1, int  hp2, int  hp3, int  hp4);

void A_RadiusDamage(int spriteNum, int blastRadius, int dmg1, int dmg2, int dmg3, int dmg4)
{
    hitradius(spriteNum, blastRadius, dmg1, dmg2, dmg3, dmg4);
}

// Check whether sprite <s> is on/in a non-SE7 water sector.
// <othersectptr>: if not NULL, the sector on the other side.
int A_CheckNoSE7Water(uspritetype const * const pSprite, int sectNum, int sectLotag, int32_t *pOther)
{
    if (sectLotag == ST_1_ABOVE_WATER || sectLotag == ST_2_UNDERWATER)
    {
        int const otherSect =
        yax_getneighborsect(pSprite->x, pSprite->y, sectNum, sectLotag == ST_1_ABOVE_WATER ? YAX_FLOOR : YAX_CEILING);
        int const otherLotag = (sectLotag == ST_1_ABOVE_WATER) ? ST_2_UNDERWATER : ST_1_ABOVE_WATER;

        // If submerging, the lower sector MUST have lotag 2.
        // If emerging, the upper sector MUST have lotag 1.
        // This way, the x/y coordinates where above/below water
        // changes can happen are the same.
        if (otherSect >= 0 && sector[otherSect].lotag == otherLotag)
        {
            if (pOther)
                *pOther = otherSect;
            return 1;
        }
    }

    return 0;
}

// Check whether to do a z position update of sprite <spritenum>.
// Returns:
//  0 if no.
//  1 if yes, but stayed inside [actor[].ceilingz+1, actor[].floorz].
// <0 if yes, but passed a TROR no-SE7 water boundary. -returnvalue-1 is the
//       other-side sector number.
static int32_t A_CheckNeedZUpdate(int32_t spriteNum, int32_t zChange, int32_t *pZcoord)
{
    uspritetype const *const pSprite = (uspritetype *)&sprite[spriteNum];
    int const                newZ    = pSprite->z + (zChange >> 1);

    *pZcoord = newZ;

    if (newZ > actor[spriteNum].ceilingz && newZ <= actor[spriteNum].floorz)
        return 1;

#ifdef YAX_ENABLE
    int const sectNum   = pSprite->sectnum;
    int const sectLotag = sector[sectNum].lotag;
    int32_t   otherSect;

    // Non-SE7 water.
    // PROJECTILE_CHSECT
    if ((zChange < 0 && sectLotag == ST_2_UNDERWATER) || (zChange > 0 && sectLotag == ST_1_ABOVE_WATER))
    {
        if (A_CheckNoSE7Water(pSprite, sprite[spriteNum].sectnum, sectLotag, &otherSect))
        {
            A_Spawn(spriteNum, TILE_WATERSPLASH2);
            // NOTE: Don't tweak its z position afterwards like with
            // SE7-induced projectile teleportation. It doesn't look good
            // with TROR water.

            actor[spriteNum].flags |= SFLAG_DIDNOSE7WATER;
            return -otherSect-1;
        }
    }
#endif

    return 0;
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


static void Proj_BounceOffWall(spritetype *s, int j)
{
    int k = getangle(
        wall[wall[j].point2].x-wall[j].x,
        wall[wall[j].point2].y-wall[j].y);
    s->ang = ((k<<1) - s->ang)&2047;
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


ACTOR_STATIC void G_MoveMisc(void)  // STATNUM 5
{
    int spriteNum = headspritestat[STAT_MISC];

    while (spriteNum >= 0)
    {
        int const         nextSprite = nextspritestat[spriteNum];
        int32_t           playerDist;
        int32_t *const    pData   = actor[spriteNum].t_data;
        spritetype *const pSprite = &sprite[spriteNum];
        int           sectNum = pSprite->sectnum;  // XXX: not const
        int           switchPic;

        if (sectNum < 0 || pSprite->xrepeat == 0)
            DELETE_SPRITE_AND_CONTINUE(spriteNum);

        Bmemcpy(&actor[spriteNum].bpos, pSprite, sizeof(vec3_t));

        switchPic = pSprite->picnum;

        if (!RR && pSprite->picnum > TILE_NUKEBUTTON && pSprite->picnum <= TILE_NUKEBUTTON+3)
            switchPic = TILE_NUKEBUTTON;

        if (pSprite->picnum > TILE_GLASSPIECES && pSprite->picnum <= TILE_GLASSPIECES+2)
            switchPic = TILE_GLASSPIECES;

        if (pSprite->picnum == TILE_INNERJAW+1)
            switchPic--;

        if ((pSprite->picnum == TILE_MONEY+1) || (!RR && (pSprite->picnum == TILE_MAIL+1 || pSprite->picnum == TILE_PAPER+1)))
        {
            actor[spriteNum].floorz = pSprite->z = getflorzofslope(pSprite->sectnum,pSprite->x,pSprite->y);
            if (RR && sector[pSprite->sectnum].lotag == 800)
                DELETE_SPRITE_AND_CONTINUE(spriteNum);
        }
        else switch (DYNAMICTILEMAP(switchPic))
            {
            //case APLAYER__STATIC: pSprite->cstat = 32768; goto next_sprite;

            case SHOTGUNSPRITE__STATIC:
                if (!RR) break;
                if (sector[pSprite->sectnum].lotag == 800 && pSprite->z >= sector[pSprite->sectnum].floorz - ZOFFSET3)
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);
                break;

            case NEON1__STATIC:
            case NEON2__STATIC:
            case NEON3__STATIC:
            case NEON4__STATIC:
            case NEON5__STATIC:
            case NEON6__STATIC:
                pSprite->shade = ((tabledivide32_noinline(g_globalRandom, pSprite->lotag + 1) & 31) > 4) ? -127 : 127;
                goto next_sprite;

            case BLOODSPLAT1__STATIC:
            case BLOODSPLAT2__STATIC:
            case BLOODSPLAT3__STATIC:
            case BLOODSPLAT4__STATIC:
                if (pData[0] == 7 * GAMETICSPERSEC)
                    goto next_sprite;

                pSprite->z += 16 + (krand2() & 15);

                if ((++pData[0] % 9) == 0)
                    pSprite->yrepeat++;
                

                goto next_sprite;

            case NUKEBUTTON__STATIC:
                //        case TILE_NUKEBUTTON+1:
                //        case TILE_NUKEBUTTON+2:
                //        case TILE_NUKEBUTTON+3:

                if (RR) break;

                if (pData[0])
                {
                    pData[0]++;
                    if (pData[0] == 8)
                        pSprite->picnum = TILE_NUKEBUTTON + 1;
                    else if (pData[0] == 16)
                    {
                        pSprite->picnum = TILE_NUKEBUTTON + 2;
                        g_player[P_Get(pSprite->owner)].ps->fist_incs = 1;
                    }
                    if (g_player[P_Get(pSprite->owner)].ps->fist_incs == GAMETICSPERSEC)
                        pSprite->picnum = TILE_NUKEBUTTON + 3;
                }
                goto next_sprite;

            case FORCESPHERE__STATIC:
                {
                    int forceRepeat = pSprite->xrepeat;
                    if (pData[1] > 0)
                    {
                        pData[1]--;
                        if (pData[1] == 0)
                            DELETE_SPRITE_AND_CONTINUE(spriteNum);
                    }
                    if (actor[pSprite->owner].t_data[1] == 0)
                    {
                        if (pData[0] < 64)
                        {
                            pData[0]++;
                            forceRepeat += 3;
                        }
                    }
                    else if (pData[0] > 64)
                    {
                        pData[0]--;
                        forceRepeat -= 3;
                    }

                    *(vec3_t *)pSprite = *(vec3_t *)&sprite[pSprite->owner];
                    pSprite->ang      += actor[pSprite->owner].t_data[0];

                    forceRepeat        = clamp2(forceRepeat, 1, 64);
                    pSprite->xrepeat   = forceRepeat;
                    pSprite->yrepeat   = forceRepeat;
                    pSprite->shade     = (forceRepeat >> 1) - 48;

                    for (bsize_t j = pData[0]; j > 0; j--)
                        A_SetSprite(spriteNum, CLIPMASK0);
                    goto next_sprite;
                }

            case MUD__STATICRR:
                pData[0]++;
                if (pData[0] == 1)
                {
                    if (sector[sectNum].floorpicnum != TILE_RRTILE3073)
                        DELETE_SPRITE_AND_CONTINUE(spriteNum);
                    if (!S_CheckSoundPlaying(spriteNum,ITEM_SPLASH))
                        A_PlaySound(ITEM_SPLASH,spriteNum);
                }
                if (pData[0] == 3)
                {
                    pData[0] = 0;
                    pData[1]++;  // WATERSPLASH_T2
                }
                if (pData[1] == 5)
                    A_DeleteSprite(spriteNum);
                goto next_sprite;

            case WATERSPLASH2__STATIC:
                pData[0]++;
                if (pData[0] == 1)
                {
                    if (sector[sectNum].lotag != ST_1_ABOVE_WATER && sector[sectNum].lotag != ST_2_UNDERWATER)
                        DELETE_SPRITE_AND_CONTINUE(spriteNum);
                    /*
                    else
                    {
                        l = getflorzofslope(sect,s->x,s->y)-s->z;
                        if( l > ZOFFSET2 ) KILLIT(i);
                    }
                    else
                    */
                    if (!S_CheckSoundPlaying(spriteNum,ITEM_SPLASH))
                        A_PlaySound(ITEM_SPLASH,spriteNum);
                }
                if (pData[0] == 3)
                {
                    pData[0] = 0;
                    pData[1]++;  // WATERSPLASH_T2
                }
                if (pData[1] == 5)
                    A_DeleteSprite(spriteNum);
                goto next_sprite;
            case FRAMEEFFECT1__STATIC:

                if (pSprite->owner >= 0)
                {
                    pData[0]++;

                    if (pData[0] > 7)
                    {
                        DELETE_SPRITE_AND_CONTINUE(spriteNum);
                    }
                    else if (pData[0] > 4)
                        pSprite->cstat |= 512 + 2;
                    else if (pData[0] > 2)
                        pSprite->cstat |= 2;
                    pSprite->xoffset = sprite[pSprite->owner].xoffset;
                    pSprite->yoffset = sprite[pSprite->owner].yoffset;
                }
                goto next_sprite;
            case INNERJAW__STATIC:
            {
                //        case TILE_INNERJAW+1:
                int32_t playerDist, playerNum = A_FindPlayer(pSprite,&playerDist);

                if (playerDist < 512)
                {
                    P_PalFrom(g_player[playerNum].ps, 32, 32,0,0);
                    sprite[g_player[playerNum].ps->i].extra -= 4;
                }
            }
            fallthrough__;
            case COOLEXPLOSION1__STATIC:
                if (!RR && switchPic == TILE_COOLEXPLOSION1)
                    break;
                fallthrough__;
            case OWHIP__STATICRR:
            case UWHIP__STATICRR:
            case FIRELASER__STATIC:
                if (pSprite->extra != 999)
                    pSprite->extra = 999;
                else DELETE_SPRITE_AND_CONTINUE(spriteNum);
                break;
            case TONGUE__STATIC:
                DELETE_SPRITE_AND_CONTINUE(spriteNum);

            case MONEY__STATIC:
            case MAIL__STATIC:
            case PAPER__STATIC:
            {
                if (RR && (switchPic == TILE_MAIL || switchPic == TILE_PAPER))
                    break;
                pSprite->xvel = (krand2()&7)+(sintable[T1(spriteNum)&2047]>>9);
                T1(spriteNum) += (krand2()&63);
                if ((T1(spriteNum)&2047) > 512 && (T1(spriteNum)&2047) < 1596)
                {
                    if (sector[sectNum].lotag == ST_2_UNDERWATER)
                    {
                        if (pSprite->zvel < 64)
                            pSprite->zvel += (g_spriteGravity>>5)+(krand2()&7);
                    }
                    else if (pSprite->zvel < 144)
                        pSprite->zvel += (g_spriteGravity>>5)+(krand2()&7);
                }

                A_SetSprite(spriteNum, CLIPMASK0);

                if ((krand2()&3) == 0)
                    setsprite(spriteNum, (vec3_t *) pSprite);

                if (pSprite->sectnum == -1)
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);

                int const floorZ = getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y);

                if (pSprite->z > floorZ)
                {
                    pSprite->z = floorZ;
                    A_AddToDeleteQueue(spriteNum);
                    PN(spriteNum)++;

                    for (bssize_t SPRITES_OF(STAT_MISC, j))
                    {
                        if (sprite[j].picnum == TILE_BLOODPOOL && ldist(pSprite, &sprite[j]) < 348)
                        {
                            pSprite->pal = 2;
                            break;
                        }
                    }
                }

                
                if (RR && sector[pSprite->sectnum].lotag == 800 && pSprite->z >= sector[pSprite->sectnum].floorz - ZOFFSET3)
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);

                break;
            }

            case HEADJIB1__STATIC:
            case ARMJIB1__STATIC:
            case LEGJIB1__STATIC:
            case LIZMANHEAD1__STATIC:
            case LIZMANARM1__STATIC:
            case LIZMANLEG1__STATIC:
                if (RR) break;
                goto jib_code;
            case RRTILE2460__STATICRR:
            case RRTILE2465__STATICRR:
            case BIKEJIBA__STATICRR:
            case BIKEJIBB__STATICRR:
            case BIKEJIBC__STATICRR:
            case BIKERJIBA__STATICRR:
            case BIKERJIBB__STATICRR:
            case BIKERJIBC__STATICRR:
            case BIKERJIBD__STATICRR:
            case CHEERJIBA__STATICRR:
            case CHEERJIBB__STATICRR:
            case CHEERJIBC__STATICRR:
            case CHEERJIBD__STATICRR:
            case FBOATJIBA__STATICRR:
            case FBOATJIBB__STATICRR:
            case RABBITJIBA__STATICRR:
            case RABBITJIBB__STATICRR:
            case RABBITJIBC__STATICRR:
            case MAMAJIBA__STATICRR:
            case MAMAJIBB__STATICRR:
                if (!RRRA) break;
                goto jib_code;
            case JIBS1__STATIC:
            case JIBS2__STATIC:
            case JIBS3__STATIC:
            case JIBS4__STATIC:
            case JIBS5__STATIC:
            case JIBS6__STATIC:
            case DUKETORSO__STATIC:
            case DUKEGUN__STATIC:
            case DUKELEG__STATIC:
            case BILLYJIBA__STATICRR:
            case BILLYJIBB__STATICRR:
            case HULKJIBA__STATICRR:
            case HULKJIBB__STATICRR:
            case HULKJIBC__STATICRR:
            case MINJIBA__STATICRR:
            case MINJIBB__STATICRR:
            case MINJIBC__STATICRR:
            case COOTJIBA__STATICRR:
            case COOTJIBB__STATICRR:
            case COOTJIBC__STATICRR:
jib_code:
            {
                pSprite->xvel = (pSprite->xvel > 0) ? pSprite->xvel - 1 : 0;

                if (!RR)
                {
                    if (pData[5] < (30*10))
                        pData[5]++;
                    else
                        DELETE_SPRITE_AND_CONTINUE(spriteNum);
                }

                if (pSprite->zvel > 1024 && pSprite->zvel < 1280)
                {
                    setsprite(spriteNum, (vec3_t *) pSprite);
                    sectNum = pSprite->sectnum;
                }

                if (RR)
                    setsprite(spriteNum, (vec3_t * ) pSprite);

                int32_t floorZ, ceilZ;
                getzsofslope(sectNum, pSprite->x, pSprite->y, &ceilZ, &floorZ);

                if (ceilZ == floorZ || sectNum < 0 || sectNum >= MAXSECTORS)
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);

                if (pSprite->z < floorZ-(2<<8))
                {
                    if (pData[1] < 2) pData[1]++;
                    else if (sector[sectNum].lotag != ST_2_UNDERWATER)
                    {
                        pData[1] = 0;

                        if (pSprite->picnum == TILE_DUKELEG || pSprite->picnum == TILE_DUKETORSO || pSprite->picnum == TILE_DUKEGUN)
                        {
                            pData[0] = (pData[0] > 6) ? 0 : pData[0] + 1;
                        }
                        else
                        {
                            pData[0] = (pData[0] > 2) ? 0 : pData[0] + 1;
                        }
                    }

                    if (pSprite->zvel < 6144)
                    {
                        if (sector[sectNum].lotag == ST_2_UNDERWATER)
                        {
                            if (pSprite->zvel < 1024)
                                pSprite->zvel += 48;
                            else pSprite->zvel = 1024;
                        }
                        else pSprite->zvel += g_spriteGravity-50;
                    }

                    pSprite->x += (pSprite->xvel*sintable[(pSprite->ang+512)&2047])>>14;
                    pSprite->y += (pSprite->xvel*sintable[pSprite->ang&2047])>>14;
                    pSprite->z += pSprite->zvel;

                    if (RR && pSprite->z >= sector[pSprite->sectnum].floorz)
                        DELETE_SPRITE_AND_CONTINUE(spriteNum);
                }
                else
                {
                    if (RRRA || (pSprite->picnum == TILE_RRTILE2465 || pSprite->picnum == TILE_RRTILE2560))
                        DELETE_SPRITE_AND_CONTINUE(spriteNum);
                    if (pData[2] == 0)
                    {
                        if (pSprite->sectnum == -1)
                            DELETE_SPRITE_AND_CONTINUE(spriteNum);

                        if ((sector[pSprite->sectnum].floorstat&2))
                            DELETE_SPRITE_AND_CONTINUE(spriteNum);

                        pData[2]++;
                    }

                    floorZ        = getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y);
                    pSprite->z    = floorZ - (2 << 8);
                    pSprite->xvel = 0;

                    if (pSprite->picnum == TILE_JIBS6)
                    {
                        pData[1]++;

                        if ((pData[1]&3) == 0 && pData[0] < 7)
                            pData[0]++;

                        if (pData[1] > 20)
                            DELETE_SPRITE_AND_CONTINUE(spriteNum);
                    }
                    else
                    {
                        pSprite->picnum = TILE_JIBS6;
                        pData[0] = 0;
                        pData[1] = 0;
                    }
                }

                if (RR && sector[pSprite->sectnum].lotag == 800 && pSprite->z >= sector[pSprite->sectnum].floorz - ZOFFSET3)
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);
                goto next_sprite;
            }

            case PUKE__STATIC:
                if (RR) break;
                fallthrough__;
            case BLOODPOOL__STATIC:
            {
                if (pData[0] == 0)
                {
                    pData[0] = 1;
                    if (sector[sectNum].floorstat&2)
                    {
                        DELETE_SPRITE_AND_CONTINUE(spriteNum);
                    }
                    else A_AddToDeleteQueue(spriteNum);
                }

                A_Fall(spriteNum);

                int32_t   playerDist;
                int const playerNum = A_FindPlayer(pSprite, &playerDist);
                pSprite->z          = actor[spriteNum].floorz - ZOFFSET;

                DukePlayer_t *const pPlayer = g_player[playerNum].ps;

                if (pData[2] < 32)
                {
                    pData[2]++;

                    if (actor[spriteNum].picnum == TILE_TIRE)
                    {
                        if (pSprite->xrepeat < 64 && pSprite->yrepeat < 64)
                        {
                            pSprite->xrepeat += krand2()&3;
                            pSprite->yrepeat += krand2()&3;
                        }
                    }
                    else
                    {
                        if (pSprite->xrepeat < 32 && pSprite->yrepeat < 32)
                        {
                            pSprite->xrepeat += krand2()&3;
                            pSprite->yrepeat += krand2()&3;
                        }
                    }
                }

                if (playerDist < 844 && pSprite->xrepeat > 6 && pSprite->yrepeat > 6)
                {
                    if (pSprite->pal == 0 && (krand2()&255) < 16 && (RR || pSprite->picnum != TILE_PUKE))
                    {
                        if (pPlayer->inv_amount[GET_BOOTS] > 0)
                            pPlayer->inv_amount[GET_BOOTS]--;
                        else
                        {
                            if (!A_CheckSoundPlaying(pPlayer->i,DUKE_LONGTERM_PAIN))
                                A_PlaySound(DUKE_LONGTERM_PAIN,pPlayer->i);

                            sprite[pPlayer->i].extra --;

                            P_PalFrom(pPlayer, 32, 16,0,0);
                        }
                    }

                    if (pData[1] == 1) goto next_sprite;

                    pData[1] = 1;

                    pPlayer->footprintcount = (actor[spriteNum].picnum == TILE_TIRE) ? 10 : 3;
                    pPlayer->footprintpal   = pSprite->pal;
                    pPlayer->footprintshade = pSprite->shade;

                    if (pData[2] == 32)
                    {
                        pSprite->xrepeat -= 6;
                        pSprite->yrepeat -= 6;
                    }
                }
                else pData[1] = 0;

                if (RR && sector[pSprite->sectnum].lotag == 800 && pSprite->z >= sector[pSprite->sectnum].floorz - ZOFFSET3)
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);
                goto next_sprite;
            }

            case BURNING2__STATIC:
            case FECES__STATIC:
            case SHRINKEREXPLOSION__STATIC:
            case EXPLOSION2BOT__STATIC:
            case LASERSITE__STATIC:
                if (RR) break;
                fallthrough__;
            case BURNING__STATIC:
            case WATERBUBBLE__STATIC:
            case SMALLSMOKE__STATIC:
            case EXPLOSION2__STATIC:
            case EXPLOSION3__STATICRR:
            case BLOOD__STATIC:
            case FORCERIPPLE__STATIC:
            case TRANSPORTERSTAR__STATIC:
            case TRANSPORTERBEAM__STATIC:
            {
                if (!G_HaveActor(sprite[spriteNum].picnum))
                    goto next_sprite;
                int const playerNum = A_FindPlayer(pSprite, &playerDist);
                A_Execute(spriteNum, playerNum, playerDist);
                goto next_sprite;
            }


            case SHELL__STATIC:
            case SHOTGUNSHELL__STATIC:

                A_SetSprite(spriteNum,CLIPMASK0);

                if (sectNum < 0 || (!RR && (sector[sectNum].floorz + (24<<8)) < pSprite->z))
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);

                if (sector[sectNum].lotag == ST_2_UNDERWATER)
                {
                    pData[1]++;
                    if (pData[1] > 8)
                    {
                        pData[1] = 0;
                        pData[0]++;
                        pData[0] &= 3;
                    }
                    if (pSprite->zvel < 128) pSprite->zvel += (g_spriteGravity/13); // 8
                    else pSprite->zvel -= 64;
                    if (pSprite->xvel > 0)
                        pSprite->xvel -= 4;
                    else pSprite->xvel = 0;
                }
                else
                {
                    pData[1]++;
                    if (pData[1] > 3)
                    {
                        pData[1] = 0;
                        pData[0]++;
                        pData[0] &= 3;
                    }
                    if (pSprite->zvel < 512) pSprite->zvel += (g_spriteGravity/3); // 52;
                    if (pSprite->xvel > 0)
                        pSprite->xvel --;
                    else
                        DELETE_SPRITE_AND_CONTINUE(spriteNum);
                }

                goto next_sprite;

            case GLASSPIECES__STATIC:
            case POPCORN__STATICRR:
                //        case TILE_GLASSPIECES+1:
                //        case TILE_GLASSPIECES+2:

                A_Fall(spriteNum);

                if (pSprite->zvel > 4096) pSprite->zvel = 4096;
                if (sectNum < 0)
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);

                if (pSprite->z == actor[spriteNum].floorz-(ZOFFSET) && pData[0] < 3)
                {
                    pSprite->zvel = -((3-pData[0])<<8)-(krand2()&511);
                    if (sector[sectNum].lotag == ST_2_UNDERWATER)
                        pSprite->zvel >>= 1;
                    pSprite->xrepeat >>= 1;
                    pSprite->yrepeat >>= 1;
                    if (rnd(96))
                        setsprite(spriteNum,(vec3_t *)pSprite);
                    pData[0]++;//Number of bounces
                }
                else if (pData[0] == 3)
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);

                if (pSprite->xvel > 0)
                {
                    pSprite->xvel -= 2;
                    pSprite->cstat = ((pSprite->xvel&3)<<2);
                }
                else pSprite->xvel = 0;

                A_SetSprite(spriteNum,CLIPMASK0);

                goto next_sprite;
            }

        if (PN(spriteNum) >= TILE_SCRAP6 && PN(spriteNum) <= TILE_SCRAP5+3)
        {
            if (pSprite->xvel > 0)
                pSprite->xvel--;
            else pSprite->xvel = 0;

            if (pSprite->zvel > 1024 && pSprite->zvel < 1280)
            {
                setsprite(spriteNum,(vec3_t *)pSprite);
                sectNum = pSprite->sectnum;
            }

            if (pSprite->z < sector[sectNum].floorz-(2<<8))
            {
                if (pData[1] < 1) pData[1]++;
                else
                {
                    pData[1] = 0;

                    if (pSprite->picnum < TILE_SCRAP6 + 8)
                        pData[0] = (pData[0] > 6) ? 0 : pData[0] + 1;
                    else
                        pData[0] = (pData[0] > 2) ? 0 : pData[0] + 1;
                }
                if (pSprite->zvel < 4096)
                    pSprite->zvel += g_spriteGravity - 50;
                pSprite->x += (pSprite->xvel*sintable[(pSprite->ang+512)&2047])>>14;
                pSprite->y += (pSprite->xvel*sintable[pSprite->ang&2047])>>14;
                pSprite->z += pSprite->zvel;
            }
            else
            {
                if (pSprite->picnum == TILE_SCRAP1 && pSprite->yvel > 0 && pSprite->yvel < MAXUSERTILES)
                {
                    int32_t j = A_Spawn(spriteNum, pSprite->yvel);

                    setsprite(j,(vec3_t *)pSprite);
                    A_GetZLimits(j);
                    sprite[j].hitag = sprite[j].lotag = 0;
                }

                DELETE_SPRITE_AND_CONTINUE(spriteNum);
            }
            goto next_sprite;
        }

next_sprite:
        spriteNum = nextSprite;
    }
}


// i: SE spritenum
static void HandleSE31(int spriteNum, int setFloorZ, int spriteZ, int SEdir, int zDifference)
{
    const spritetype *pSprite = &sprite[spriteNum];
    sectortype *const pSector = &sector[sprite[spriteNum].sectnum];
    int32_t *const    pData   = actor[spriteNum].t_data;

    if (klabs(pSector->floorz - spriteZ) < SP(spriteNum))
    {
        if (setFloorZ)
            pSector->floorz = spriteZ;

        pData[2] = SEdir;
        pData[0] = 0;
        if (!RR)
            pData[3] = pSprite->hitag;

        A_CallSound(pSprite->sectnum, spriteNum);
    }
    else
    {
        int const zChange = ksgn(zDifference) * SP(spriteNum);

        pSector->floorz += zChange;

        for (bssize_t SPRITES_OF_SECT(pSprite->sectnum, j))
        {
            if (sprite[j].picnum == TILE_APLAYER && sprite[j].owner >= 0)
            {
                int const playerNum = P_Get(j);

                if (g_player[playerNum].ps->on_ground == 1)
                    g_player[playerNum].ps->pos.z += zChange;
            }

            if (sprite[j].zvel == 0 && sprite[j].statnum != STAT_EFFECTOR && (RR || sprite[j].statnum != STAT_PROJECTILE))
            {
                sprite[j].z += zChange;
                actor[j].bpos.z = sprite[j].z;
                actor[j].floorz = pSector->floorz;
            }
        }
    }
}

// s: SE sprite
static void MaybeTrainKillPlayer(const spritetype *pSprite, int const setOPos)
{
    for (bssize_t TRAVERSE_CONNECT(playerNum))
    {
        DukePlayer_t *const pPlayer = g_player[playerNum].ps;

        if (sprite[pPlayer->i].extra > 0)
        {
            int16_t playerSectnum = pPlayer->cursectnum;

            updatesector(pPlayer->pos.x, pPlayer->pos.y, &playerSectnum);

            if ((playerSectnum == -1 && !ud.clipping) || (pPlayer->cursectnum != pSprite->sectnum && playerSectnum == pSprite->sectnum))
            {
                *(vec2_t *)pPlayer = *(vec2_t const *)pSprite;

                if (setOPos)
                    *(vec2_t *)&pPlayer->opos = *(vec2_t *)pPlayer;

                pPlayer->cursectnum = pSprite->sectnum;

                setsprite(pPlayer->i, (vec3_t const *)pSprite);
                P_QuickKill(pPlayer);
            }
        }
    }
}

// i: SE spritenum
static void MaybeTrainKillEnemies(int const spriteNum, int const gutSpawnCnt)
{
    int findSprite = headspritesect[sprite[OW(spriteNum)].sectnum];

    do
    {
        int const nextSprite = nextspritesect[findSprite];

        if (sprite[findSprite].statnum == STAT_ACTOR && A_CheckEnemySprite(&sprite[findSprite]))
        {
            int16_t sectNum = sprite[findSprite].sectnum;

            updatesector(sprite[findSprite].x,sprite[findSprite].y,&sectNum);

            if (sprite[findSprite].extra >= 0 && sectNum == sprite[spriteNum].sectnum)
            {
                A_DoGutsDir(findSprite, TILE_JIBS6, gutSpawnCnt);
                A_PlaySound(SQUISHED, findSprite);
                A_DeleteSprite(findSprite);
            }
        }

        findSprite = nextSprite;
    }
    while (findSprite >= 0);
}

ACTOR_STATIC void G_MoveEffectors(void)   //STATNUM 3
{
    int32_t q = 0, j, k, l, m, x;
    int spriteNum = headspritestat[STAT_EFFECTOR];

    for (native_t TRAVERSE_CONNECT(playerNum))
    {
        vec2_t & fric = g_player[playerNum].ps->fric;
        fric.x = fric.y = 0;
    }

    while (spriteNum >= 0)
    {
        int const         nextSprite = nextspritestat[spriteNum];
        spritetype *const   pSprite    = &sprite[spriteNum];
        int32_t             playerDist;
        int                 playerNum = A_FindPlayer(pSprite, &playerDist);
        DukePlayer_t *const pPlayer   = g_player[playerNum].ps;

        sectortype *const pSector     = &sector[pSprite->sectnum];
        int const         spriteLotag = pSprite->lotag;
        int const         spriteHitag = pSprite->hitag;
        int32_t *const    pData       = &actor[spriteNum].t_data[0];

        switch (spriteLotag)
        {
        case SE_0_ROTATING_SECTOR:
        {
            int32_t zchange = 0;

            j = pSprite->owner;

            if ((uint16_t)sprite[j].lotag == UINT16_MAX)
                DELETE_SPRITE_AND_CONTINUE(spriteNum);

            q = pSector->extra>>3;
            l = 0;

            if (pSector->lotag == ST_30_ROTATE_RISE_BRIDGE)
            {
                q >>= 2;

                if (sprite[spriteNum].extra == 1)
                {
                    if (actor[spriteNum].tempang < 256)
                    {
                        actor[spriteNum].tempang += 4;
                        if (actor[spriteNum].tempang >= 256)
                            A_CallSound(pSprite->sectnum,spriteNum);
                        if (pSprite->clipdist) l = 1;
                        else l = -1;
                    }
                    else actor[spriteNum].tempang = 256;

                    if (pSector->floorz > pSprite->z)   //z's are touching
                    {
                        pSector->floorz -= 512;
                        zchange = -512;
                        if (pSector->floorz < pSprite->z)
                            pSector->floorz = pSprite->z;
                    }
                    else if (pSector->floorz < pSprite->z)   //z's are touching
                    {
                        pSector->floorz += 512;
                        zchange = 512;
                        if (pSector->floorz > pSprite->z)
                            pSector->floorz = pSprite->z;
                    }
                }
                else if (sprite[spriteNum].extra == 3)
                {
                    if (actor[spriteNum].tempang > 0)
                    {
                        actor[spriteNum].tempang -= 4;
                        if (actor[spriteNum].tempang <= 0)
                            A_CallSound(pSprite->sectnum,spriteNum);
                        if (pSprite->clipdist) l = -1;
                        else l = 1;
                    }
                    else actor[spriteNum].tempang = 0;

                    if (pSector->floorz > T4(spriteNum))   //z's are touching
                    {
                        pSector->floorz -= 512;
                        zchange = -512;
                        if (pSector->floorz < T4(spriteNum))
                            pSector->floorz = T4(spriteNum);
                    }
                    else if (pSector->floorz < T4(spriteNum))   //z's are touching
                    {
                        pSector->floorz += 512;
                        zchange = 512;
                        if (pSector->floorz > T4(spriteNum))
                            pSector->floorz = T4(spriteNum);
                    }
                }
            }
            else
            {
                if (actor[j].t_data[0] == 0) break;
                if (actor[j].t_data[0] == 2) DELETE_SPRITE_AND_CONTINUE(spriteNum);

                l = (sprite[j].ang > 1024) ? -1 : 1;

                if (pData[3] == 0)
                    pData[3] = ldist(pSprite,&sprite[j]);
                pSprite->xvel = pData[3];
                pSprite->x = sprite[j].x;
                pSprite->y = sprite[j].y;
            }

            pSprite->ang += (l*q);
            pData[2] += (l*q);

            if (l && (pSector->floorstat&64))
            {
                for (TRAVERSE_CONNECT(playerNum))
                {
                    DukePlayer_t *const pPlayer = g_player[playerNum].ps;

                    if (pPlayer->cursectnum == pSprite->sectnum && pPlayer->on_ground == 1)
                    {
                        pPlayer->q16ang += fix16_from_int(l*q);
                        pPlayer->q16ang &= 0x7FFFFFF;

                        pPlayer->pos.z += zchange;

                        vec2_t r;
                        rotatepoint(*(vec2_t *)&sprite[j],*(vec2_t *)&pPlayer->pos,(q*l),&r);

                        pPlayer->bobpos.x += r.x-pPlayer->pos.x;
                        pPlayer->bobpos.y += r.y-pPlayer->pos.y;

                        *(vec2_t *)&pPlayer->pos = r;

                        if (sprite[pPlayer->i].extra <= 0)
                            *(vec2_t *)&sprite[pPlayer->i] = r;
                    }
                }

                for (bssize_t SPRITES_OF_SECT(pSprite->sectnum, p))
                {
                    // KEEPINSYNC1
                    if (sprite[p].statnum != STAT_EFFECTOR && sprite[p].statnum != STAT_PROJECTILE)
                        if (RR || sprite[p].picnum != TILE_LASERLINE)
                        {
                            if (sprite[p].picnum == TILE_APLAYER && sprite[p].owner >= 0)
                                continue;

                            sprite[p].ang += (l*q);
                            sprite[p].ang &= 2047;

                            sprite[p].z += zchange;

                            rotatepoint(*(vec2_t *)&sprite[j], *(vec2_t *)&sprite[p], (q * l), (vec2_t *)&sprite[p].x);
                        }
                }

            }

            A_MoveSector(spriteNum);
        }
        break;

        case SE_1_PIVOT: //Nothing for now used as the pivot
            if (pSprite->owner == -1) //Init
            {
                pSprite->owner = spriteNum;

                for (SPRITES_OF(STAT_EFFECTOR, j))
                {
                    if (sprite[j].lotag == SE_19_EXPLOSION_LOWERS_CEILING && sprite[j].hitag == spriteHitag)
                    {
                        pData[0] = 0;
                        break;
                    }
                }
            }
            break;

        case SE_6_SUBWAY:
            k = pSector->extra;

            if (pData[4] > 0)
            {
                pData[4]--;
                if (pData[4] >= (k-(k>>3)))
                    pSprite->xvel -= (k>>5);
                if (pData[4] > ((k>>1)-1) && pData[4] < (k-(k>>3)))
                    pSprite->xvel = 0;
                if (pData[4] < (k>>1))
                    pSprite->xvel += (k>>5);
                if (pData[4] < ((k>>1)-(k>>3)))
                {
                    pData[4] = 0;
                    pSprite->xvel = k;
                    if (RR && (!RRRA || g_lastLevel) && g_hulkSpawn)
                    {
                        g_hulkSpawn--;
                        int newSprite = A_Spawn(spriteNum, TILE_HULK);
                        sprite[newSprite].z = sector[sprite[newSprite].sectnum].ceilingz;
                        sprite[newSprite].pal = 33;
                        if (!g_hulkSpawn)
                        {
                            newSprite = A_InsertSprite(pSprite->sectnum, pSprite->x, pSprite->y,
                                sector[pSprite->sectnum].ceilingz + 119428, TILE_RRTILE3677, -8, 16, 16, 0, 0, 0, spriteNum, STAT_MISC);
                            sprite[newSprite].cstat = 514;
                            sprite[newSprite].pal = 7;
                            sprite[newSprite].xrepeat = 80;
                            sprite[newSprite].yrepeat = 255;
                            newSprite = A_Spawn(spriteNum, TILE_RRTILE296);
                            sprite[newSprite].cstat = 0;
                            sprite[newSprite].cstat |= 32768;
                            sprite[newSprite].z = sector[pSprite->sectnum].floorz - 6144;
                            DELETE_SPRITE_AND_CONTINUE(spriteNum);
                        }
                    }
                }
            }
            else
            {
                pSprite->xvel = k;
                if (RR)
                {
                    int otherSprite = headspritesect[pSprite->sectnum];
                    while (otherSprite >= 0)
                    {
                        int const nextOtherSprite = nextspritesect[otherSprite];
                        if (sprite[otherSprite].picnum == TILE_UFOBEAM)
                            if (g_ufoSpawn)
                                if (++g_ufoCnt == 64)
                        {
                            g_ufoCnt = 0;
                            g_ufoSpawn--;
                            int ufoTile = TILE_UFO1;
                            switch (krand2()&3)
                            {
                                case 0:
                                    ufoTile = TILE_UFO1;
                                    break;
                                case 1:
                                    ufoTile = TILE_UFO2;
                                    break;
                                case 2:
                                    ufoTile = TILE_UFO3;
                                    break;
                                case 3:
                                    ufoTile = TILE_UFO4;
                                    break;
                            }
                            if (RRRA)
                                ufoTile = TILE_UFO1;
                            int const newSprite = A_Spawn(spriteNum, ufoTile);
                            sprite[newSprite].z = sector[sprite[newSprite].sectnum].ceilingz;
                        }
                        otherSprite = nextOtherSprite;
                    }
                }
            }

            for (SPRITES_OF(STAT_EFFECTOR, j))
            {
                if (sprite[j].lotag == SE_14_SUBWAY_CAR && spriteHitag == sprite[j].hitag && actor[j].t_data[0] == pData[0])
                {
                    sprite[j].xvel = pSprite->xvel;
                    //                        if( t[4] == 1 )
                    {
                        if (actor[j].t_data[5] == 0)
                            actor[j].t_data[5] = dist(&sprite[j],pSprite);
                        x = ksgn(dist(&sprite[j],pSprite)-actor[j].t_data[5]);
                        if (sprite[j].extra)
                            x = -x;
                        pSprite->xvel += x;
                    }
                    actor[j].t_data[4] = pData[4];
                }
            }
            x = 0;  // XXX: This assignment is dead?
            fallthrough__;

        case SE_14_SUBWAY_CAR:
            if (pSprite->owner==-1)
                pSprite->owner = A_FindLocator((int16_t)pData[3],(int16_t)pData[0]);

            if (pSprite->owner == -1)
            {
                // debugging subway cars (mapping-wise) is freakin annoying
                // let's at least have a helpful message...
                Bsprintf(tempbuf,"Could not find any locators in sector %d"
                         " for SE# 6 or 14 with hitag %d.\n", (int)pData[0], (int)pData[3]);
                G_GameExit(tempbuf);
            }

            j = ldist(&sprite[pSprite->owner],pSprite);

            if (j < 1024L)
            {
                if (spriteLotag==SE_6_SUBWAY)
                    if (sprite[pSprite->owner].hitag&1)
                        pData[4]=pSector->extra; //Slow it down
                pData[3]++;
                pSprite->owner = A_FindLocator(pData[3],pData[0]);
                if (pSprite->owner==-1)
                {
                    pData[3]=0;
                    pSprite->owner = A_FindLocator(0,pData[0]);
                }
            }

            if (pSprite->xvel)
            {
#ifdef YAX_ENABLE
                int32_t firstrun = 1;
#endif
                x = getangle(sprite[pSprite->owner].x-pSprite->x,sprite[pSprite->owner].y-pSprite->y);
                q = G_GetAngleDelta(pSprite->ang,x)>>3;

                pData[2] += q;
                pSprite->ang += q;

                if (pSprite->xvel == pSector->extra)
                {
                    if (RR)
                    {
                        if (!S_CheckSoundPlaying(spriteNum,actor[spriteNum].lastv.x))
                            A_PlaySound(actor[spriteNum].lastv.x,spriteNum);
                    }
                    if (!RR && (pSector->floorstat&1) == 0 && (pSector->ceilingstat&1) == 0)
                    {
                        if (!S_CheckSoundPlaying(spriteNum,actor[spriteNum].lastv.x))
                            A_PlaySound(actor[spriteNum].lastv.x,spriteNum);
                    }
                    else if (ud.monsters_off == 0 && pSector->floorpal == 0 && (pSector->floorstat&1) && rnd(8))
                    {
                        if (playerDist < 20480)
                        {
                            j = pSprite->ang;
                            pSprite->ang = getangle(pSprite->x-g_player[playerNum].ps->pos.x,pSprite->y-g_player[playerNum].ps->pos.y);
                            A_Shoot(spriteNum,TILE_RPG);
                            pSprite->ang = j;
                        }
                    }
                }

                if (pSprite->xvel <= 64 && (RR || ((pSector->floorstat&1) == 0 && (pSector->ceilingstat&1) == 0)))
                    S_StopEnvSound(actor[spriteNum].lastv.x,spriteNum);

                if ((pSector->floorz-pSector->ceilingz) < (108<<8))
                {
                    if (ud.clipping == 0 && pSprite->xvel >= 192)
                        MaybeTrainKillPlayer(pSprite, 0);
                }

                m = (pSprite->xvel*sintable[(pSprite->ang+512)&2047])>>14;
                x = (pSprite->xvel*sintable[pSprite->ang&2047])>>14;

                for (TRAVERSE_CONNECT(playerNum))
                {
                    DukePlayer_t *const pPlayer = g_player[playerNum].ps;

                    // might happen when squished into void space
                    if (pPlayer->cursectnum < 0)
                        break;

                    if (sector[pPlayer->cursectnum].lotag != ST_2_UNDERWATER)
                    {
                        if (g_playerSpawnPoints[playerNum].sect == pSprite->sectnum)
                        {
                            g_playerSpawnPoints[playerNum].pos.x += m;
                            g_playerSpawnPoints[playerNum].pos.y += x;
                        }

                        if (pSprite->sectnum == sprite[pPlayer->i].sectnum
#ifdef YAX_ENABLE
                                || (pData[9]>=0 && pData[9] == sprite[pPlayer->i].sectnum)
#endif
                            )
                        {
                            rotatepoint(*(vec2_t *)pSprite, *(vec2_t *)&pPlayer->pos, q, (vec2_t *)&pPlayer->pos);

                            pPlayer->pos.x += m;
                            pPlayer->pos.y += x;

                            pPlayer->bobpos.x += m;
                            pPlayer->bobpos.y += x;

                            pPlayer->q16ang += fix16_from_int(q);
                            pPlayer->q16ang &= 0x7FFFFFF;

                            if (g_netServer || numplayers > 1)
                            {
                                pPlayer->opos.x = pPlayer->pos.x;
                                pPlayer->opos.y = pPlayer->pos.y;
                            }
                            if (sprite[pPlayer->i].extra <= 0)
                            {
                                sprite[pPlayer->i].x = pPlayer->pos.x;
                                sprite[pPlayer->i].y = pPlayer->pos.y;
                            }
                        }
                    }
                }

                // NOTE: special loop handling
                j = headspritesect[pSprite->sectnum];
                while (j >= 0)
                {
                    // KEEPINSYNC2
                    // XXX: underwater check?
                    if (sprite[j].statnum != STAT_PLAYER && sector[sprite[j].sectnum].lotag != ST_2_UNDERWATER &&
                            (sprite[j].picnum != TILE_SECTOREFFECTOR || (sprite[j].lotag == SE_49_POINT_LIGHT||sprite[j].lotag == SE_50_SPOT_LIGHT))
                            && sprite[j].picnum != TILE_LOCATORS)
                    {
                        rotatepoint(*(vec2_t *)pSprite,*(vec2_t *)&sprite[j],q,(vec2_t *)&sprite[j].x);

                        sprite[j].x+= m;
                        sprite[j].y+= x;

                        sprite[j].ang+=q;

                        if (g_netServer || numplayers > 1)
                        {
                            actor[j].bpos.x = sprite[j].x;
                            actor[j].bpos.y = sprite[j].y;
                        }
                    }
                    j = nextspritesect[j];
#ifdef YAX_ENABLE
                    if (j < 0)
                    {
                        if (pData[9]>=0 && firstrun)
                        {
                            firstrun = 0;
                            j = headspritesect[pData[9]];
                        }
                    }
#endif
                }

                A_MoveSector(spriteNum);
                setsprite(spriteNum,(vec3_t *)pSprite);

                if ((pSector->floorz-pSector->ceilingz) < (108<<8))
                {
                    if (ud.clipping == 0 && pSprite->xvel >= 192)
                        MaybeTrainKillPlayer(pSprite, 1);

                    MaybeTrainKillEnemies(spriteNum, 72);
                }
            }

            break;

        case SE_30_TWO_WAY_TRAIN:
            if (pSprite->owner == -1)
            {
                pData[3] = !pData[3];
                pSprite->owner = A_FindLocator(pData[3],pData[0]);
            }
            else
            {

                if (pData[4] == 1) // Starting to go
                {
                    if (ldist(&sprite[pSprite->owner],pSprite) < (2048-128))
                        pData[4] = 2;
                    else
                    {
                        if (pSprite->xvel == 0)
                            G_OperateActivators(pSprite->hitag+(!pData[3]),-1);
                        if (pSprite->xvel < 256)
                            pSprite->xvel += 16;
                    }
                }
                if (pData[4] == 2)
                {
                    l = FindDistance2D(sprite[pSprite->owner].x-pSprite->x,sprite[pSprite->owner].y-pSprite->y);

                    if (l <= 128)
                        pSprite->xvel = 0;

                    if (pSprite->xvel > 0)
                        pSprite->xvel -= 16;
                    else
                    {
                        pSprite->xvel = 0;
                        G_OperateActivators(pSprite->hitag+(int16_t)pData[3],-1);
                        pSprite->owner = -1;
                        pSprite->ang += 1024;
                        pData[4] = 0;
                        G_OperateForceFields(spriteNum,pSprite->hitag);

                        for (SPRITES_OF_SECT(pSprite->sectnum, j))
                        {
                            if (sprite[j].picnum != TILE_SECTOREFFECTOR && sprite[j].picnum != TILE_LOCATORS)
                            {
                                actor[j].bpos.x = sprite[j].x;
                                actor[j].bpos.y = sprite[j].y;
                            }
                        }

                    }
                }
            }

            if (pSprite->xvel)
            {
                l = (pSprite->xvel*sintable[(pSprite->ang+512)&2047])>>14;
                x = (pSprite->xvel*sintable[pSprite->ang&2047])>>14;

                if ((pSector->floorz-pSector->ceilingz) < (108<<8))
                    if (ud.clipping == 0)
                        MaybeTrainKillPlayer(pSprite, 0);

                for (int TRAVERSE_CONNECT(playerNum))
                {
                    DukePlayer_t *const pPlayer = g_player[playerNum].ps;

                    if (sprite[pPlayer->i].sectnum == pSprite->sectnum)
                    {
                        pPlayer->pos.x += l;
                        pPlayer->pos.y += x;

                        if (g_netServer || numplayers > 1)
                        {
                            pPlayer->opos.x = pPlayer->pos.x;
                            pPlayer->opos.y = pPlayer->pos.y;
                        }

                        pPlayer->bobpos.x += l;
                        pPlayer->bobpos.y += x;
                    }

                    if (g_playerSpawnPoints[playerNum].sect == pSprite->sectnum)
                    {
                        g_playerSpawnPoints[playerNum].pos.x += l;
                        g_playerSpawnPoints[playerNum].pos.y += x;
                    }
                }

                for (SPRITES_OF_SECT(pSprite->sectnum, j))
                {
                    // TODO: replace some checks for SE 49/50 with statnum LIGHT instead?
                    if ((sprite[j].picnum != TILE_SECTOREFFECTOR || sprite[j].lotag==SE_49_POINT_LIGHT || sprite[j].lotag==SE_50_SPOT_LIGHT)
                            && sprite[j].picnum != TILE_LOCATORS)
                    {
                        if (numplayers < 2 && !g_netServer)
                        {
                            actor[j].bpos.x = sprite[j].x;
                            actor[j].bpos.y = sprite[j].y;
                        }

                        sprite[j].x += l;
                        sprite[j].y += x;

                        if (g_netServer || numplayers > 1)
                        {
                            actor[j].bpos.x = sprite[j].x;
                            actor[j].bpos.y = sprite[j].y;
                        }
                    }
                }

                A_MoveSector(spriteNum);
                setsprite(spriteNum,(vec3_t *)pSprite);

                if (pSector->floorz-pSector->ceilingz < (108<<8))
                {
                    if (ud.clipping == 0)
                        MaybeTrainKillPlayer(pSprite, 1);

                    MaybeTrainKillEnemies(spriteNum, 24);
                }
            }

            break;


        case SE_2_EARTHQUAKE://Quakes
            if (pData[4] > 0 && pData[0] == 0)
            {
                if (pData[4] < spriteHitag)
                    pData[4]++;
                else pData[0] = 1;
            }

            if (pData[0] > 0)
            {
                pData[0]++;

                pSprite->xvel = 3;

                if (pData[0] > 96)
                {
                    pData[0] = -1; //Stop the quake
                    pData[4] = -1;
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);
                }
                else
                {
                    if ((pData[0]&31) ==  8)
                    {
                        g_earthquakeTime = 48;
                        A_PlaySound(EARTHQUAKE,g_player[screenpeek].ps->i);
                    }

                    pSector->floorheinum = (klabs(pSector->floorheinum - pData[5]) < 8)
                                           ? pData[5]
                                           : pSector->floorheinum + (ksgn(pData[5] - pSector->floorheinum) << 4);
                }

                vec2_t const vect = { (pSprite->xvel * sintable[(pSprite->ang + 512) & 2047]) >> 14,
                                      (pSprite->xvel * sintable[pSprite->ang & 2047]) >> 14 };

                for (TRAVERSE_CONNECT(playerNum))
                {
                    DukePlayer_t *const pPlayer = g_player[playerNum].ps;

                    if (pPlayer->cursectnum == pSprite->sectnum && pPlayer->on_ground)
                    {
                        pPlayer->pos.x += vect.x;
                        pPlayer->pos.y += vect.y;

                        pPlayer->bobpos.x += vect.x;
                        pPlayer->bobpos.y += vect.y;
                    }
                }

                for (bssize_t nextSprite, SPRITES_OF_SECT_SAFE(pSprite->sectnum, sectSprite, nextSprite))
                {
                    if (sprite[sectSprite].picnum != TILE_SECTOREFFECTOR)
                    {
                        sprite[sectSprite].x+=vect.x;
                        sprite[sectSprite].y+=vect.y;
                        setsprite(sectSprite,(vec3_t *)&sprite[sectSprite]);
                    }
                }

                A_MoveSector(spriteNum);
                setsprite(spriteNum,(vec3_t *)pSprite);
            }
            break;

            //Flashing sector lights after reactor TILE_EXPLOSION2

        case SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT:
        {
            if (pData[4] == 0) break;

            //    if(t[5] > 0) { t[5]--; break; }

            if ((tabledivide32_noinline(g_globalRandom, spriteHitag+1)&31) < 4 && !pData[2])
            {
                //       t[5] = 4+(g_globalRandom&7);
                pSector->ceilingpal = pSprite->owner >> 8;
                pSector->floorpal   = pSprite->owner & 0xff;
                pData[0]            = pSprite->shade + (g_globalRandom & 15);
            }
            else
            {
                //       t[5] = 4+(g_globalRandom&3);
                pSector->ceilingpal = pSprite->pal;
                pSector->floorpal   = pSprite->pal;
                pData[0]            = pData[3];
            }

            pSector->ceilingshade = pData[0];
            pSector->floorshade   = pData[0];

            walltype *pWall = &wall[pSector->wallptr];

            for (x=pSector->wallnum; x > 0; x--,pWall++)
            {
                if (pWall->hitag != 1)
                {
                    pWall->shade = pData[0];

                    if ((pWall->cstat & 2) && pWall->nextwall >= 0)
                        wall[pWall->nextwall].shade = pWall->shade;
                }
            }

            break;
        }

        case SE_4_RANDOM_LIGHTS:
        {
            // See A_Spawn():
            //  s->owner: original ((ceilingpal<<8) | floorpal)
            //  t[2]: original floor shade
            //  t[3]: max wall shade
            int lightFlag;

            if ((tabledivide32_noinline(g_globalRandom, spriteHitag+1)&31) < 4)
            {
                pData[1]            = pSprite->shade + (g_globalRandom & 15);  // Got really bright
                pData[0]            = pSprite->shade + (g_globalRandom & 15);
                pSector->ceilingpal = pSprite->owner >> 8;
                pSector->floorpal   = pSprite->owner & 0xff;
                lightFlag           = 1;
            }
            else
            {
                pData[1] = pData[2];
                pData[0] = pData[3];

                pSector->ceilingpal = pSprite->pal;
                pSector->floorpal   = pSprite->pal;

                lightFlag = 0;
            }

            pSector->floorshade = pData[1];
            pSector->ceilingshade = pData[1];

            walltype *pWall = &wall[pSector->wallptr];

            for (x=pSector->wallnum; x > 0; x--,pWall++)
            {
                if (lightFlag) pWall->pal = (pSprite->owner&0xff);
                else pWall->pal = pSprite->pal;

                if (pWall->hitag != 1)
                {
                    pWall->shade = pData[0];
                    if ((pWall->cstat&2) && pWall->nextwall >= 0)
                        wall[pWall->nextwall].shade = pWall->shade;
                }
            }

            for (bssize_t SPRITES_OF_SECT(SECT(spriteNum), sectSprite))
            {
                if (sprite[sectSprite].cstat&16)
                    sprite[sectSprite].shade = (pSector->ceilingstat & 1) ? pSector->ceilingshade : pSector->floorshade;
            }

            if (pData[4])
                DELETE_SPRITE_AND_CONTINUE(spriteNum);

            break;
        }

            //BOSS
        case SE_5:
        {
            if (playerDist < 8192)
            {
                int const saveAng = pSprite->ang;
                pSprite->ang      = getangle(pSprite->x - pPlayer->pos.x, pSprite->y - pPlayer->pos.y);
                A_Shoot(spriteNum, TILE_FIRELASER);
                pSprite->ang      = saveAng;
            }

            if (pSprite->owner==-1) //Start search
            {
                pData[4]               = 0;
                int closestLocatorDist = INT32_MAX;
                int closestLocator     = pSprite->owner;

                //Find the shortest dist
                do
                {
                    pSprite->owner = A_FindLocator((int16_t)pData[4], -1);  // t[0] hold sectnum

                    if (pSprite->owner == -1)
                        break;

                    int const locatorDist = ldist(&sprite[pPlayer->i],&sprite[pSprite->owner]);

                    if (closestLocatorDist > locatorDist)
                    {
                        closestLocator     = pSprite->owner;
                        closestLocatorDist = locatorDist;
                    }

                    pData[4]++;
                }
                while (1);

                pSprite->owner = closestLocator;
                pSprite->zvel  = ksgn(sprite[closestLocator].z - pSprite->z) << 4;
            }

            if (ldist(&sprite[pSprite->owner],pSprite) < 1024)
            {
                pSprite->owner = -1;
                goto next_sprite;
            }
            else pSprite->xvel=256;

            int const angInc = G_GetAngleDelta(pSprite->ang, getangle(sprite[pSprite->owner].x-pSprite->x,
                                                                      sprite[pSprite->owner].y-pSprite->y))>>3;
            pSprite->ang += angInc;

            if (rnd(32))
            {
                pData[2] += angInc;
                pSector->ceilingshade = 127;
            }
            else
            {
                pData[2] += G_GetAngleDelta(pData[2] + 512, getangle(pPlayer->pos.x - pSprite->x, pPlayer->pos.y - pSprite->y)) >> 2;
                pSector->ceilingshade = 0;
            }

            if (A_IncurDamage(spriteNum) >= 0)
            {
                if (++pData[3] == 5)
                {
                    pSprite->zvel += 1024;
                    P_DoQuote(QUOTE_WASTED, g_player[myconnectindex].ps);
                }
            }

            pSprite->z                += pSprite->zvel;
            pSector->ceilingz         += pSprite->zvel;
            sector[pData[0]].ceilingz += pSprite->zvel;

            A_MoveSector(spriteNum);
            setsprite(spriteNum, (vec3_t *)pSprite);
            break;
        }

        case SE_8_UP_OPEN_DOOR_LIGHTS:
        case SE_9_DOWN_OPEN_DOOR_LIGHTS:
        {

            // work only if its moving

            int animGoal = -1;

            if (actor[spriteNum].t_data[4])
            {
                if (++actor[spriteNum].t_data[4] > 8)
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);

                animGoal = 1;
            }
            else animGoal = GetAnimationGoal(&pSector->ceilingz);

            if (animGoal >= 0)
            {
                int shadeInc = ((pSector->lotag & 0x8000u) || actor[spriteNum].t_data[4]) ? -pData[3] : pData[3];

                if (spriteLotag == SE_9_DOWN_OPEN_DOOR_LIGHTS)
                    shadeInc = -shadeInc;

                for (bssize_t SPRITES_OF(STAT_EFFECTOR, sectorEffector))
                {
                    if (sprite[sectorEffector].lotag == spriteLotag && sprite[sectorEffector].hitag == spriteHitag)
                    {
                        int const sectNum = sprite[sectorEffector].sectnum;
                        int const spriteShade = sprite[sectorEffector].shade;

                        walltype *pWall = &wall[sector[sectNum].wallptr];

                        for (bsize_t l=sector[sectNum].wallnum; l>0; l--, pWall++)
                        {
                            if (pWall->hitag == 1)
                                continue;

                            pWall->shade += shadeInc;

                            if (pWall->shade < spriteShade)
                                pWall->shade = spriteShade;
                            else if (pWall->shade > actor[sectorEffector].t_data[2])
                                pWall->shade = actor[sectorEffector].t_data[2];

                            if (pWall->nextwall >= 0 && wall[pWall->nextwall].hitag != 1)
                                wall[pWall->nextwall].shade = pWall->shade;
                        }

                        sector[sectNum].floorshade   += shadeInc;
                        sector[sectNum].ceilingshade += shadeInc;

                        if (sector[sectNum].floorshade < spriteShade)
                            sector[sectNum].floorshade = spriteShade;
                        else if (sector[sectNum].floorshade > actor[sectorEffector].t_data[0])
                            sector[sectNum].floorshade = actor[sectorEffector].t_data[0];

                        if (sector[sectNum].ceilingshade < spriteShade)
                            sector[sectNum].ceilingshade = spriteShade;
                        else if (sector[sectNum].ceilingshade > actor[sectorEffector].t_data[1])
                            sector[sectNum].ceilingshade = actor[sectorEffector].t_data[1];

                        if (RR && sector[sectNum].hitag == 1)
                            sector[sectNum].ceilingshade = actor[sectorEffector].t_data[1];
                    }
                }
            }
            break;
        }

        case SE_10_DOOR_AUTO_CLOSE:
            // XXX: 32791, what the hell?
            if ((pSector->lotag&0xff) == ST_27_STRETCH_BRIDGE || (pSector->floorz > pSector->ceilingz && (pSector->lotag&0xff) != ST_23_SWINGING_DOOR) || pSector->lotag == (int16_t)32791u)
            {
                j = 1;

                if ((pSector->lotag&0xff) != ST_27_STRETCH_BRIDGE)
                    for (bssize_t TRAVERSE_CONNECT(playerNum))
                        if (pSector->lotag != ST_30_ROTATE_RISE_BRIDGE && pSector->lotag != ST_31_TWO_WAY_TRAIN && pSector->lotag != 0
                            && pSprite->sectnum == sprite[g_player[playerNum].ps->i].sectnum)
                            j = 0;

                if (j == 1)
                {
                    if (pData[0] > spriteHitag)
                        switch (sector[pSprite->sectnum].lotag)
                        {
                        case ST_20_CEILING_DOOR:
                        case ST_21_FLOOR_DOOR:
                        case ST_22_SPLITTING_DOOR:
                        case ST_26_SPLITTING_ST_DOOR:
                            if (!RR && GetAnimationGoal(&sector[pSprite->sectnum].ceilingz) >= 0)
                                break;
                            fallthrough__;
                        default:
                            G_ActivateBySector(pSprite->sectnum,spriteNum);
                            pData[0] = 0;
                            break;
                        }
                    else pData[0]++;
                }
            }
            else pData[0]=0;
            break;

        case SE_11_SWINGING_DOOR: //Swingdoor

            if (pData[5] > 0)
            {
                pData[5]--;
                break;
            }

            if (pData[4])
            {
                int const endWall = pSector->wallptr+pSector->wallnum;

                l = (SP(spriteNum) >> 3) * pData[3];
                for (j=pSector->wallptr; j<endWall; j++)
                {
                    for (SPRITES_OF(STAT_ACTOR, k))
                    {
                        if (sprite[k].extra > 0 && A_CheckEnemySprite(&sprite[k])
                                && clipinsidebox((vec2_t *)&sprite[k], j, 256) == 1)
                            goto next_sprite;
                    }
                    for (SPRITES_OF(STAT_PLAYER, k))
                    {
                        if (sprite[k].owner >= 0 && clipinsidebox((vec2_t *)&sprite[k], j, 144) == 1)
                        {
                            pData[5] = 8;  // Delay
                            pData[2] -= l;
                            pData[4] -= l;
                            A_MoveSector(spriteNum);
                            setsprite(spriteNum, (vec3_t *)pSprite);
                            goto next_sprite;
                        }
                    }
                }

                pData[2] += l;
                pData[4] += l;
                A_MoveSector(spriteNum);
                setsprite(spriteNum, (vec3_t *)pSprite);

                if (pData[4] <= -511 || pData[4] >= 512)
                {
                    pData[4] = 0;
                    pData[2] &= 0xffffff00;
                    A_MoveSector(spriteNum);
                    setsprite(spriteNum, (vec3_t *) pSprite);
                    break;
                }
            }
            break;

        case SE_12_LIGHT_SWITCH:
            if (pData[0] == 3 || pData[3] == 1)   //Lights going off
            {
                pSector->floorpal   = 0;
                pSector->ceilingpal = 0;

                walltype *pWall = &wall[pSector->wallptr];

                for (j = pSector->wallnum; j > 0; j--, pWall++)
                {
                    if (pWall->hitag != 1)
                    {
                        pWall->shade = pData[1];
                        pWall->pal   = 0;
                    }
                }

                pSector->floorshade   = pData[1];
                pSector->ceilingshade = pData[2];
                pData[0]              = 0;

                for (SPRITES_OF_SECT(SECT(spriteNum), j))
                {
                    if (sprite[j].cstat & 16)
                        sprite[j].shade = (pSector->ceilingstat & 1) ? pSector->ceilingshade : pSector->floorshade;
                }

                if (pData[3] == 1)
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);
            }

            if (pData[0] == 1)   //Lights flickering on
            {
                if (pSector->floorshade > pSprite->shade)
                {
                    pSector->floorpal   = pSprite->pal;
                    pSector->ceilingpal = pSprite->pal;

                    pSector->floorshade   -= 2;
                    pSector->ceilingshade -= 2;

                    walltype *pWall = &wall[pSector->wallptr];
                    for (j = pSector->wallnum; j > 0; j--, pWall++)
                    {
                        if (pWall->hitag != 1)
                        {
                            pWall->pal = pSprite->pal;
                            pWall->shade -= 2;
                        }
                    }
                }
                else pData[0] = 2;

                for (SPRITES_OF_SECT(SECT(spriteNum), j))
                {
                    if (sprite[j].cstat&16)
                    {
                        if (sprite[j].cstat & 16)
                            sprite[j].shade = (pSector->ceilingstat & 1) ? pSector->ceilingshade : pSector->floorshade;
                    }
                }
            }
            break;

        case 47:
            if (!RRRA)
                break;
            if (pData[0] == 3 || pData[3] == 1)   //Lights going off
            {
                pSector->floorpal   = 0;
                pSector->ceilingpal = 0;

                walltype *pWall = &wall[pSector->wallptr];

                for (j = pSector->wallnum; j > 0; j--, pWall++)
                {
                    if (pWall->hitag != 1)
                    {
                        pWall->shade = pData[1];
                        pWall->pal   = 0;
                    }
                }

                pSector->floorshade   = pData[1];
                pSector->ceilingshade = pData[2];
                pData[0]              = 0;

                for (SPRITES_OF_SECT(SECT(spriteNum), j))
                {
                    if (sprite[j].cstat & 16)
                        sprite[j].shade = (pSector->ceilingstat & 1) ? pSector->ceilingshade : pSector->floorshade;
                }

                if (pData[3] == 1)
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);
            }

            if (pData[0] == 1)   //Lights flickering on
            {
                if (pSector->floorshade > pSprite->shade)
                {
                    pSector->floorpal   = pSprite->pal;

                    pSector->floorshade   -= 2;

                    walltype *pWall = &wall[pSector->wallptr];
                    for (j = pSector->wallnum; j > 0; j--, pWall++)
                    {
                        if (pWall->hitag != 1)
                        {
                            pWall->pal = pSprite->pal;
                            pWall->shade -= 2;
                        }
                    }
                }
                else pData[0] = 2;

                for (SPRITES_OF_SECT(SECT(spriteNum), j))
                {
                    if (sprite[j].cstat&16)
                    {
                        if (sprite[j].cstat & 16)
                            sprite[j].shade = (pSector->ceilingstat & 1) ? pSector->ceilingshade : pSector->floorshade;
                    }
                }
            }
            break;

        case 48:
            if (!RRRA)
                break;
            if (pData[0] == 3 || pData[3] == 1)   //Lights going off
            {
                pSector->floorpal   = 0;
                pSector->ceilingpal = 0;

                walltype *pWall = &wall[pSector->wallptr];

                for (j = pSector->wallnum; j > 0; j--, pWall++)
                {
                    if (pWall->hitag != 1)
                    {
                        pWall->shade = pData[1];
                        pWall->pal   = 0;
                    }
                }

                pSector->floorshade   = pData[1];
                pSector->ceilingshade = pData[2];
                pData[0]              = 0;

                for (SPRITES_OF_SECT(SECT(spriteNum), j))
                {
                    if (sprite[j].cstat & 16)
                        sprite[j].shade = (pSector->ceilingstat & 1) ? pSector->ceilingshade : pSector->floorshade;
                }

                if (pData[3] == 1)
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);
            }

            if (pData[0] == 1)   //Lights flickering on
            {
                if (pSector->ceilingshade > pSprite->shade)
                {
                    pSector->ceilingpal = pSprite->pal;

                    pSector->ceilingshade -= 2;

                    walltype *pWall = &wall[pSector->wallptr];
                    for (j = pSector->wallnum; j > 0; j--, pWall++)
                    {
                        if (pWall->hitag != 1)
                        {
                            pWall->pal = pSprite->pal;
                            pWall->shade -= 2;
                        }
                    }
                }
                else pData[0] = 2;

                for (SPRITES_OF_SECT(SECT(spriteNum), j))
                {
                    if (sprite[j].cstat&16)
                        sprite[j].shade = (pSector->ceilingstat & 1) ? pSector->ceilingshade : pSector->floorshade;
                }
            }
            break;


        case SE_13_EXPLOSIVE:
            if (pData[2])
            {
                // t[0]: ceiling z
                // t[1]: floor z
                // s->owner: 1 if affect ceiling, 0 if affect floor
                // t[3]: 1 if ceiling was parallaxed at premap, 0 else

                j = (SP(spriteNum)<<5)|1;

                if (pSprite->ang == 512)
                {
                    if (pSprite->owner)
                    {
                        pSector->ceilingz = (klabs(pData[0] - pSector->ceilingz) >= j)
                                            ? pSector->ceilingz + ksgn(pData[0] - pSector->ceilingz) * j
                                            : pData[0];
                    }
                    else
                    {
                        pSector->floorz = (klabs(pData[1] - pSector->floorz) >= j)
                                          ? pSector->floorz + ksgn(pData[1] - pSector->floorz) * j
                                          : pData[1];
                    }
                }
                else
                {
                    pSector->floorz = (klabs(pData[1] - pSector->floorz) >= j)
                                      ? pSector->floorz + ksgn(pData[1] - pSector->floorz) * j
                                      : pData[1];

                    pSector->ceilingz = /*(klabs(pData[0] - pSector->ceilingz) >= j)
                                      ? pSector->ceilingz + ksgn(pData[0] - pSector->ceilingz) * j
                                      : */pData[0];
                }
#ifdef YAX_ENABLE
                if (pSprite->ang == 512)
                {
                    int16_t cf=!pSprite->owner, bn=yax_getbunch(pSector-sector, cf);
                    int32_t jj, daz=SECTORFLD(pSector-sector,z, cf);

                    if (bn >= 0)
                    {
                        for (SECTORS_OF_BUNCH(bn, cf, jj))
                        {
                            SECTORFLD(jj,z, cf) = daz;
                            SECTORFLD(jj,stat, cf) &= ~(128+256 + 512+2048);
                        }
                        for (SECTORS_OF_BUNCH(bn, !cf, jj))
                        {
                            SECTORFLD(jj,z, !cf) = daz;
                            SECTORFLD(jj,stat, !cf) &= ~(128+256 + 512+2048);
                        }
                    }
                }
#endif
                if (pData[3] == 1)
                {
                    //Change the shades

                    pData[3]++;
                    pSector->ceilingstat ^= 1;

                    if (pSprite->ang == 512)
                    {
                        walltype *pWall = &wall[pSector->wallptr];

                        for (j = pSector->wallnum; j > 0; j--, pWall++)
                            pWall->shade = pSprite->shade;

                        pSector->floorshade = pSprite->shade;

                        if (g_player[0].ps->one_parallax_sectnum >= 0)
                        {
                            pSector->ceilingpicnum = sector[g_player[0].ps->one_parallax_sectnum].ceilingpicnum;
                            pSector->ceilingshade  = sector[g_player[0].ps->one_parallax_sectnum].ceilingshade;
                        }
                    }
                }

                if (++pData[2] > 256)
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);
            }

            if (pData[2] == 4 && pSprite->ang != 512)
                for (x=0; x<7; x++) RANDOMSCRAP(pSprite, spriteNum);
            break;


        case SE_15_SLIDING_DOOR:

            if (pData[4])
            {
                pSprite->xvel = 16;

                if (pData[4] == 1) //Opening
                {
                    if (pData[3] >= (SP(spriteNum)>>3))
                    {
                        pData[4] = 0; //Turn off the sliders
                        A_CallSound(pSprite->sectnum,spriteNum);
                        break;
                    }
                    pData[3]++;
                }
                else if (pData[4] == 2)
                {
                    if (pData[3]<1)
                    {
                        pData[4] = 0;
                        A_CallSound(pSprite->sectnum,spriteNum);
                        break;
                    }
                    pData[3]--;
                }

                A_MoveSector(spriteNum);
                setsprite(spriteNum,(vec3_t *)pSprite);
            }
            break;

        case SE_16_REACTOR: //Reactor

            pData[2]+=32;

            if (pSector->floorz < pSector->ceilingz)
                pSprite->shade = 0;
            else if (pSector->ceilingz < pData[3])
            {
                //The following code check to see if
                //there is any other sprites in the sector.
                //If there isn't, then kill this sectoreffector
                //itself.....

                for (SPRITES_OF_SECT(pSprite->sectnum, j))
                {
                    if (sprite[j].picnum == TILE_REACTOR || sprite[j].picnum == TILE_REACTOR2)
                        break;
                }

                if (j == -1)
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);

                pSprite->shade = 1;
            }

            pSector->ceilingz = (pSprite->shade)
                                ? pSector->ceilingz + 1024
                                : pSector->ceilingz - 512;

            A_MoveSector(spriteNum);
            setsprite(spriteNum,(vec3_t *)pSprite);

            break;

        case SE_17_WARP_ELEVATOR:
        {
            int32_t nextk;

            q = pData[0]*(SP(spriteNum)<<2);

            pSector->ceilingz += q;
            pSector->floorz += q;

            for (SPRITES_OF_SECT(pSprite->sectnum, j))
            {
                if (sprite[j].statnum == STAT_PLAYER && sprite[j].owner >= 0)
                {
                    int const           warpPlayer = P_Get(j);
                    DukePlayer_t *const pPlayer    = g_player[warpPlayer].ps;

                    if (numplayers < 2 && !g_netServer)
                        pPlayer->opos.z = pPlayer->pos.z;

                    pPlayer->pos.z += q;
                    pPlayer->truefz += q;
                    pPlayer->truecz += q;

                    if (g_netServer || numplayers > 1)
                        pPlayer->opos.z = pPlayer->pos.z;
                }

                if (sprite[j].statnum != STAT_EFFECTOR)
                {
                    actor[j].bpos.z = sprite[j].z;
                    sprite[j].z += q;
                }

                actor[j].floorz   = pSector->floorz;
                actor[j].ceilingz = pSector->ceilingz;
            }

            if (pData[0]) //If in motion
            {
                if (klabs(pSector->floorz-pData[2]) <= SP(spriteNum))
                {
                    G_ActivateWarpElevators(spriteNum,0);
                    break;
                }

                // If we still see the opening, we can't yet teleport.
                if (pData[0]==-1)
                {
                    if (pSector->floorz > pData[3])
                        break;
                }
                else if (pSector->ceilingz < pData[4]) break;

                if (pData[1] == 0) break;
                pData[1] = 0;

                for (SPRITES_OF(STAT_EFFECTOR, j))
                {
                    if (spriteNum != j && sprite[j].lotag == SE_17_WARP_ELEVATOR)
                        if (pSector->hitag-pData[0] == sector[sprite[j].sectnum].hitag
                                && spriteHitag == sprite[j].hitag)
                            break;
                }

                if (j == -1) break;

                for (SPRITES_OF_SECT_SAFE(pSprite->sectnum, k, nextk))
                {
                    if (sprite[k].statnum == STAT_PLAYER && sprite[k].owner >= 0)
                    {
                        int const           warpPlayer = P_Get(k);
                        DukePlayer_t *const pPlayer    = g_player[warpPlayer].ps;

                        pPlayer->pos.x += sprite[j].x - pSprite->x;
                        pPlayer->pos.y += sprite[j].y - pSprite->y;
                        pPlayer->pos.z = sector[sprite[j].sectnum].floorz - (pSector->floorz - pPlayer->pos.z);
                        
                        actor[k].floorz             = sector[sprite[j].sectnum].floorz;
                        actor[k].ceilingz           = sector[sprite[j].sectnum].ceilingz;
                        *(vec2_t *)&pPlayer->opos   = *(vec2_t *)pPlayer;
                        *(vec2_t *)&pPlayer->bobpos = *(vec2_t *)pPlayer;
                        pPlayer->opos.z             = pPlayer->pos.z;
                        pPlayer->truefz             = actor[k].floorz;
                        pPlayer->truecz             = actor[k].ceilingz;
                        pPlayer->bobcounter         = 0;

                        changespritesect(k, sprite[j].sectnum);
                        pPlayer->cursectnum = sprite[j].sectnum;
                    }
                    else if (sprite[k].statnum != STAT_EFFECTOR)
                    {
                        sprite[k].x += sprite[j].x-pSprite->x;
                        sprite[k].y += sprite[j].y-pSprite->y;
                        sprite[k].z = sector[sprite[j].sectnum].floorz - (pSector->floorz - sprite[k].z);

                        Bmemcpy(&actor[k].bpos, &sprite[k], sizeof(vec3_t));

                        changespritesect(k,sprite[j].sectnum);
                        setsprite(k,(vec3_t *)&sprite[k]);

                        actor[k].floorz   = sector[sprite[j].sectnum].floorz;
                        actor[k].ceilingz = sector[sprite[j].sectnum].ceilingz;
                    }
                }
            }
            break;
        }

        case SE_18_INCREMENTAL_SECTOR_RISE_FALL:
            if (pData[0])
            {
                if (pSprite->pal)
                {
                    if (pSprite->ang == 512)
                    {
                        pSector->ceilingz -= pSector->extra;
                        if (pSector->ceilingz <= pData[1])
                        {
                            pSector->ceilingz = pData[1];
                            DELETE_SPRITE_AND_CONTINUE(spriteNum);
                        }
                    }
                    else
                    {
                        pSector->floorz += pSector->extra;

                        if (!RR)
                            for (bssize_t SPRITES_OF_SECT(pSprite->sectnum, sectSprite))
                            {
                                if (sprite[sectSprite].picnum == TILE_APLAYER && sprite[sectSprite].owner >= 0 && g_player[P_Get(sectSprite)].ps->on_ground == 1)
                                    g_player[P_Get(sectSprite)].ps->pos.z += pSector->extra;

                                if (sprite[sectSprite].zvel == 0 && sprite[sectSprite].statnum != STAT_EFFECTOR && sprite[sectSprite].statnum != STAT_PROJECTILE)
                                {
                                    actor[sectSprite].bpos.z = sprite[sectSprite].z += pSector->extra;
                                    actor[sectSprite].floorz = pSector->floorz;
                                }
                            }

                        if (pSector->floorz >= pData[1])
                        {
                            pSector->floorz = pData[1];
                            DELETE_SPRITE_AND_CONTINUE(spriteNum);
                        }
                    }
                }
                else
                {
                    if (pSprite->ang == 512)
                    {
                        pSector->ceilingz += pSector->extra;
                        if (pSector->ceilingz >= pSprite->z)
                        {
                            pSector->ceilingz = pSprite->z;
                            DELETE_SPRITE_AND_CONTINUE(spriteNum);
                        }
                    }
                    else
                    {
                        pSector->floorz -= pSector->extra;

                        if (!RR)
                            for (bssize_t SPRITES_OF_SECT(pSprite->sectnum, sectSprite))
                            {
                                if (sprite[sectSprite].picnum == TILE_APLAYER && sprite[sectSprite].owner >= 0 &&g_player[P_Get(sectSprite)].ps->on_ground == 1)
                                    g_player[P_Get(sectSprite)].ps->pos.z -= pSector->extra;

                                if (sprite[sectSprite].zvel == 0 && sprite[sectSprite].statnum != STAT_EFFECTOR && sprite[sectSprite].statnum != STAT_PROJECTILE)
                                {
                                    actor[sectSprite].bpos.z = sprite[sectSprite].z -= pSector->extra;
                                    actor[sectSprite].floorz = pSector->floorz;
                                }
                            }

                        if (pSector->floorz <= pSprite->z)
                        {
                            pSector->floorz = pSprite->z;
                            DELETE_SPRITE_AND_CONTINUE(spriteNum);
                        }
                    }
                }

                if (++pData[2] >= pSprite->hitag)
                {
                    pData[2] = 0;
                    pData[0] = 0;
                }
            }
            break;

        case SE_19_EXPLOSION_LOWERS_CEILING: //Battlestar galactia shields

            if (pData[0])
            {
                if (pData[0] == 1)
                {
                    pData[0]++;
                    x = pSector->wallptr;
                    q = x+pSector->wallnum;

                    for (j=x; j<q; j++)
                    {
                        if (wall[j].overpicnum == TILE_BIGFORCE)
                        {
                            wall[j].cstat &= (128+32+8+4+2);
                            wall[j].overpicnum = 0;

                            if (wall[j].nextwall >= 0)
                            {
                                wall[wall[j].nextwall].overpicnum = 0;
                                wall[wall[j].nextwall].cstat &= (128+32+8+4+2);
                            }
                        }
                    }
                }

                if (pSector->ceilingz < pSector->floorz)
                    pSector->ceilingz += SP(spriteNum);
                else
                {
                    pSector->ceilingz = pSector->floorz;

                    for (SPRITES_OF(STAT_EFFECTOR, j))
                    {
                        if (sprite[j].lotag == SE_0_ROTATING_SECTOR && sprite[j].hitag==spriteHitag)
                        {
                            sectortype *const pSector     = &sector[sprite[j].sectnum];
                            int const         ownerSector = sprite[sprite[j].owner].sectnum;

                            pSector->ceilingpal   = sector[ownerSector].floorpal;
                            pSector->floorpal     = pSector->ceilingpal;
                            pSector->ceilingshade = sector[ownerSector].floorshade;
                            pSector->floorshade   = pSector->ceilingshade;

                            actor[sprite[j].owner].t_data[0] = 2;
                        }
                    }

                    DELETE_SPRITE_AND_CONTINUE(spriteNum);
                }
            }
            else //Not hit yet
            {
                if (G_FindExplosionInSector(pSprite->sectnum) >= 0)
                {
                    P_DoQuote(QUOTE_UNLOCKED, g_player[myconnectindex].ps);

                    for (SPRITES_OF(STAT_EFFECTOR, l))
                    {
                        switch (sprite[l].lotag & 0x7fff)
                        {
                        case SE_0_ROTATING_SECTOR:
                            if (sprite[l].hitag == spriteHitag)
                            {
                                int const spriteOwner = sprite[l].owner;
                                int const sectNum     = sprite[l].sectnum;

                                sector[sectNum].ceilingshade = sprite[spriteOwner].shade;
                                sector[sectNum].floorshade   = sector[sectNum].ceilingshade;
                                sector[sectNum].ceilingpal   = sprite[spriteOwner].pal;
                                sector[sectNum].floorpal     = sector[sectNum].ceilingpal;
                            }
                            break;

                        case SE_1_PIVOT:
                        case SE_12_LIGHT_SWITCH:
//                        case SE_18_INCREMENTAL_SECTOR_RISE_FALL:
                        case SE_19_EXPLOSION_LOWERS_CEILING:
                            if (spriteHitag == sprite[l].hitag)
                                if (actor[l].t_data[0] == 0)
                                {
                                    actor[l].t_data[0] = 1;  // Shut them all on
                                    sprite[l].owner    = spriteNum;
                                }

                            break;
                        }
                    }
                }
            }

            break;

        case SE_20_STRETCH_BRIDGE: //Extend-o-bridge
            if (pData[0] == 0) break;
            pSprite->xvel = (pData[0] == 1) ? 8 : -8;

            if (pSprite->xvel)   //Moving
            {
                vec2_t const vect = { (pSprite->xvel * sintable[(pSprite->ang + 512) & 2047]) >> 14,
                                      (pSprite->xvel * sintable[pSprite->ang & 2047]) >> 14 };

                pData[3] += pSprite->xvel;

                pSprite->x += vect.x;
                pSprite->y += vect.y;

                if (pData[3] <= 0 || (pData[3] >> 6) >= (SP(spriteNum) >> 6))
                {
                    pSprite->x -= vect.x;
                    pSprite->y -= vect.y;
                    pData[0] = 0;
                    A_CallSound(pSprite->sectnum, spriteNum);
                    break;
                }

                for (bssize_t nextSprite, SPRITES_OF_SECT_SAFE(pSprite->sectnum, sectSprite, nextSprite))
                {
                    if (sprite[sectSprite].statnum != STAT_EFFECTOR && sprite[sectSprite].zvel == 0)
                    {
                        sprite[sectSprite].x += vect.x;
                        sprite[sectSprite].y += vect.y;

                        setsprite(sectSprite, (vec3_t *)&sprite[sectSprite]);

                        if (sector[sprite[sectSprite].sectnum].floorstat & 2 && sprite[sectSprite].statnum == STAT_ZOMBIEACTOR)
                            A_Fall(sectSprite);
                    }
                }

                dragpoint((int16_t)pData[1], wall[pData[1]].x + vect.x, wall[pData[1]].y + vect.y, 0);
                dragpoint((int16_t)pData[2], wall[pData[2]].x + vect.x, wall[pData[2]].y + vect.y, 0);

                for (bssize_t TRAVERSE_CONNECT(playerNum))
                {
                    DukePlayer_t *const pPlayer = g_player[playerNum].ps;

                    if (pPlayer->cursectnum == pSprite->sectnum && pPlayer->on_ground)
                    {
                        pPlayer->pos.x += vect.x;
                        pPlayer->pos.y += vect.y;

                        pPlayer->opos.x = pPlayer->pos.x;
                        pPlayer->opos.y = pPlayer->pos.y;

                        pPlayer->pos.z += PHEIGHT;
                        setsprite(pPlayer->i, (vec3_t *)pPlayer);
                        pPlayer->pos.z -= PHEIGHT;
                    }
                }

                pSector->floorxpanning -= vect.x >> 3;
                pSector->floorypanning -= vect.y >> 3;

                pSector->ceilingxpanning -= vect.x >> 3;
                pSector->ceilingypanning -= vect.y >> 3;
            }

            break;

        case SE_21_DROP_FLOOR: // Cascading effect
        {
            if (pData[0] == 0) break;

            int32_t *zptr = (pSprite->ang == 1536) ? &pSector->ceilingz : &pSector->floorz;

            if (pData[0] == 1)   //Decide if the s->sectnum should go up or down
            {
                pSprite->zvel = ksgn(pSprite->z-*zptr) * (SP(spriteNum)<<4);
                pData[0]++;
            }

            if (pSector->extra == 0)
            {
                *zptr += pSprite->zvel;

                if (klabs(*zptr-pSprite->z) < 1024)
                {
                    *zptr = pSprite->z;
                    DELETE_SPRITE_AND_CONTINUE(spriteNum); //All done   // SE_21_KILLIT, see sector.c
                }
            }
            else pSector->extra--;
            break;
        }

        case SE_22_TEETH_DOOR:
            if (pData[1])
            {
                if (GetAnimationGoal(&sector[pData[0]].ceilingz) >= 0)
                    pSector->ceilingz += pSector->extra*9;
                else pData[1] = 0;
            }
            break;

        case 156:
            if (!RRRA) break;
            fallthrough__;
        case SE_24_CONVEYOR:
        case SE_34:
        {
            if (pData[4])
                break;

            vec2_t const vect = { (SP(spriteNum) * sintable[(pSprite->ang + 512) & 2047]) >> 18,
                                  (SP(spriteNum) * sintable[pSprite->ang & 2047]) >> 18 };

            k = 0;

            for (bssize_t nextSprite, SPRITES_OF_SECT_SAFE(pSprite->sectnum, sectSprite, nextSprite))
            {
                if (sprite[sectSprite].zvel < 0)
                    continue;

                switch (sprite[sectSprite].statnum)
                {
                    case STAT_MISC:
                        switch (DYNAMICTILEMAP(sprite[sectSprite].picnum))
                        {
                            case PUKE__STATIC:
                            case FOOTPRINTS4__STATIC:
                            case BLOODSPLAT1__STATIC:
                            case BLOODSPLAT2__STATIC:
                            case BLOODSPLAT3__STATIC:
                            case BLOODSPLAT4__STATIC:
                                if (RR) break;
                                fallthrough__;
                            case BULLETHOLE__STATIC:
                                if (RR && sprite[sectSprite].picnum == TILE_BULLETHOLE) continue;
                                fallthrough__;
                            case BLOODPOOL__STATIC:
                            case FOOTPRINTS__STATIC:
                            case FOOTPRINTS2__STATIC:
                            case FOOTPRINTS3__STATIC: sprite[sectSprite].xrepeat = sprite[sectSprite].yrepeat = 0; continue;

                            case LASERLINE__STATIC: if (RR) break; continue;
                        }
                        fallthrough__;
                    case STAT_STANDABLE:
                        if (!RR && sprite[sectSprite].picnum == TILE_TRIPBOMB)
                            break;
                        fallthrough__;
                    case STAT_ACTOR:
                    case STAT_DEFAULT:
                        if (sprite[sectSprite].picnum == TILE_BOLT1
                            || sprite[sectSprite].picnum == TILE_BOLT1 + 1
                            || sprite[sectSprite].picnum == TILE_BOLT1 + 2
                            || sprite[sectSprite].picnum == TILE_BOLT1 + 3
                            || (!RR && (sprite[sectSprite].picnum == TILE_SIDEBOLT1
                            || sprite[sectSprite].picnum == TILE_SIDEBOLT1 + 1
                            || sprite[sectSprite].picnum == TILE_SIDEBOLT1 + 2
                            || sprite[sectSprite].picnum == TILE_SIDEBOLT1 + 3))
                            || A_CheckSwitchTile(sectSprite))
                            break;

                        if (!(sprite[sectSprite].picnum >= TILE_CRANE && sprite[sectSprite].picnum <= TILE_CRANE + 3))
                        {
                            if (sprite[sectSprite].z > actor[sectSprite].floorz - ZOFFSET2)
                            {
                                actor[sectSprite].bpos.x = sprite[sectSprite].x;
                                actor[sectSprite].bpos.y = sprite[sectSprite].y;

                                sprite[sectSprite].x += vect.x >> (RR ? 1 : 2);
                                sprite[sectSprite].y += vect.y >> (RR ? 1 : 2);

                                setsprite(sectSprite, (vec3_t *)&sprite[sectSprite]);

                                if (sector[sprite[sectSprite].sectnum].floorstat & 2)
                                    if (sprite[sectSprite].statnum == STAT_ZOMBIEACTOR)
                                        A_Fall(sectSprite);
                            }
                        }
                        break;
                }
            }

            for (bssize_t TRAVERSE_CONNECT(playerNum))
            {
                DukePlayer_t *const pPlayer = g_player[playerNum].ps;

                if (pPlayer->cursectnum == pSprite->sectnum && pPlayer->on_ground)
                {
                    if (klabs(pPlayer->pos.z - pPlayer->truefz) < PHEIGHT + (9 << 8))
                    {
                        pPlayer->fric.x += vect.x << 3;
                        pPlayer->fric.y += vect.y << 3;
                    }
                }
            }
            if (!RRRA || spriteLotag != 156)
                pSector->floorxpanning += SP(spriteNum)>>7;

            break;
        }

        case SE_35:
            if (pSector->ceilingz > pSprite->z)
            {
                for (j = 0; j < 8; j++)
                {
                    pSprite->ang += krand2()&511;
                    k = A_Spawn(spriteNum, TILE_SMALLSMOKE);
                    sprite[k].xvel = 96+(krand2()&127);
                    A_SetSprite(k, CLIPMASK0);
                    setsprite(k, (vec3_t *) &sprite[k]);
                    if (rnd(16))
                        A_Spawn(spriteNum, TILE_EXPLOSION2);
                }

            }
            switch (pData[0])
            {
            case 0:
                pSector->ceilingz += pSprite->yvel;
                if (pSector->ceilingz > pSector->floorz)
                    pSector->floorz = pSector->ceilingz;
                if (pSector->ceilingz > pSprite->z+ZOFFSET5)
                    pData[0]++;
                break;
            case 1:
                pSector->ceilingz-=(pSprite->yvel<<2);
                if (pSector->ceilingz < pData[4])
                {
                    pSector->ceilingz = pData[4];
                    pData[0] = 0;
                }
                break;
            }
            break;

        case SE_25_PISTON: //PISTONS
            if (pData[4] == 0) break;

            if (pSector->floorz <= pSector->ceilingz)
                pSprite->shade = 0;
            else if (pSector->ceilingz <= pData[RR?4:3])
                pSprite->shade = 1;

            if (pSprite->shade)
            {
                pSector->ceilingz += SP(spriteNum)<<4;
                if (pSector->ceilingz > pSector->floorz)
                {
                    pSector->ceilingz = pSector->floorz;
                    if (RRRA && g_pistonSound)
                        A_PlaySound(371, spriteNum);
                }
            }
            else
            {
                pSector->ceilingz   -= SP(spriteNum)<<4;
                if (pSector->ceilingz < pData[RR?4:3])
                {
                    pSector->ceilingz = pData[RR?4:3];
                    if (RRRA && g_pistonSound)
                        A_PlaySound(167, spriteNum);
                }
            }

            break;

        case SE_26:
        {
            int32_t p, nextj;

            pSprite->xvel = 32;
            l = (pSprite->xvel*sintable[(pSprite->ang+512)&2047])>>14;
            x = (pSprite->xvel*sintable[pSprite->ang&2047])>>14;

            pSprite->shade++;
            if (pSprite->shade > 7)
            {
                pSprite->x = pData[3];
                pSprite->y = pData[4];
                pSector->floorz -= ((pSprite->zvel*pSprite->shade)-pSprite->zvel);
                pSprite->shade = 0;
            }
            else
                pSector->floorz += pSprite->zvel;

            for (SPRITES_OF_SECT_SAFE(pSprite->sectnum, j, nextj))
            {
                if (sprite[j].statnum != STAT_EFFECTOR && sprite[j].statnum != STAT_PLAYER )
                {
                    actor[j].bpos.x = sprite[j].x;
                    actor[j].bpos.y = sprite[j].y;

                    sprite[j].x += l;
                    sprite[j].y += x;
                    sprite[j].z += pSprite->zvel;

                    setsprite(j, (vec3_t *)&sprite[j]);
                }
            }

            for (TRAVERSE_CONNECT(p))
            {
                DukePlayer_t *const pPlayer = g_player[p].ps;

                if (pSprite->sectnum == sprite[pPlayer->i].sectnum && pPlayer->on_ground)
                {
                    pPlayer->fric.x += l << 5;
                    pPlayer->fric.y += x << 5;
                    pPlayer->pos.z += pSprite->zvel;
                }
            }

            A_MoveSector(spriteNum);
            setsprite(spriteNum,(vec3_t *)pSprite);

            break;
        }

        case SE_27_DEMO_CAM:
        {
            if (ud.recstat == 0 || !cl_democams) break;

            actor[spriteNum].tempang = pSprite->ang;

            int const p = A_FindPlayer(pSprite,&x);
            DukePlayer_t * const ps = g_player[p].ps;

            if (sprite[ps->i].extra > 0 && myconnectindex == screenpeek)
            {
                if (pData[0] < 0)
                {
                    ud.camerasprite = spriteNum;
                    pData[0]++;
                }
                else if (ud.recstat == 2 && ps->newowner == -1)
                {
                    if (cansee(pSprite->x,pSprite->y,pSprite->z,SECT(spriteNum),ps->pos.x,ps->pos.y,ps->pos.z,ps->cursectnum))
                    {
                        if (x < (int32_t)((unsigned)spriteHitag))
                        {
                            ud.camerasprite = spriteNum;
                            pData[0] = 999;
                            pSprite->ang += G_GetAngleDelta(pSprite->ang,getangle(ps->pos.x-pSprite->x,ps->pos.y-pSprite->y))>>3;
                            SP(spriteNum) = 100+((pSprite->z-ps->pos.z)/257);

                        }
                        else if (pData[0] == 999)
                        {
                            if (ud.camerasprite == spriteNum)
                                pData[0] = 0;
                            else pData[0] = -10;
                            ud.camerasprite = spriteNum;

                        }
                    }
                    else
                    {
                        pSprite->ang = getangle(ps->pos.x-pSprite->x,ps->pos.y-pSprite->y);

                        if (pData[0] == 999)
                        {
                            if (ud.camerasprite == spriteNum)
                                pData[0] = 0;
                            else pData[0] = -20;
                            ud.camerasprite = spriteNum;
                        }
                    }
                }
            }
            break;
        }

        case SE_28_LIGHTNING:
        {
            if (RR)
                break;
            if (pData[5] > 0)
            {
                pData[5]--;
                break;
            }

            if (T1(spriteNum) == 0)
            {
                A_FindPlayer(pSprite,&x);
                if (x > 15500)
                    break;
                T1(spriteNum) = 1;
                T2(spriteNum) = 64 + (krand2()&511);
                T3(spriteNum) = 0;
            }
            else
            {
                T3(spriteNum)++;
                if (T3(spriteNum) > T2(spriteNum))
                {
                    T1(spriteNum) = 0;
                    g_player[screenpeek].ps->visibility = ud.const_visibility;
                    break;
                }
                else if (T3(spriteNum) == (T2(spriteNum)>>1))
                    A_PlaySound(THUNDER,spriteNum);
                else if (T3(spriteNum) == (T2(spriteNum)>>3))
                    A_PlaySound(LIGHTNING_SLAP,spriteNum);
                else if (T3(spriteNum) == (T2(spriteNum)>>2))
                {
                    for (SPRITES_OF(STAT_DEFAULT, j))
                        if (sprite[j].picnum == TILE_NATURALLIGHTNING && sprite[j].hitag == pSprite->hitag)
                            sprite[j].cstat |= 32768;
                }
                else if (T3(spriteNum) > (T2(spriteNum)>>3) && T3(spriteNum) < (T2(spriteNum)>>2))
                {
                    if (cansee(pSprite->x,pSprite->y,pSprite->z,pSprite->sectnum,g_player[screenpeek].ps->pos.x,g_player[screenpeek].ps->pos.y,g_player[screenpeek].ps->pos.z,g_player[screenpeek].ps->cursectnum))
                        j = 1;
                    else j = 0;

                    if (rnd(192) && (T3(spriteNum)&1))
                    {
                        if (j)
                            g_player[screenpeek].ps->visibility = 0;
                    }
                    else if (j)
                        g_player[screenpeek].ps->visibility = ud.const_visibility;

                    for (SPRITES_OF(STAT_DEFAULT, j))
                    {
                        if (sprite[j].picnum == TILE_NATURALLIGHTNING && sprite[j].hitag == pSprite->hitag)
                        {
                            if (rnd(32) && (T3(spriteNum)&1))
                            {
                                int32_t p;
                                DukePlayer_t *ps;

                                sprite[j].cstat &= 32767;
                                A_Spawn(j,TILE_SMALLSMOKE);

                                p = A_FindPlayer(pSprite, NULL);
                                ps = g_player[p].ps;

                                x = ldist(&sprite[ps->i], &sprite[j]);
                                if (x < 768)
                                {
                                    if (!A_CheckSoundPlaying(ps->i,DUKE_LONGTERM_PAIN))
                                        A_PlaySound(DUKE_LONGTERM_PAIN,ps->i);
                                    A_PlaySound(SHORT_CIRCUIT,ps->i);
                                    sprite[ps->i].extra -= 8+(krand2()&7);

                                    P_PalFrom(ps, 32, 16,0,0);
                                }
                                break;
                            }
                            else sprite[j].cstat |= 32768;
                        }
                    }
                }
            }
            break;
        }

        case SE_29_WAVES:
            pSprite->hitag += 64;
            l = mulscale12((int32_t)pSprite->yvel,sintable[pSprite->hitag&2047]);
            pSector->floorz = pSprite->z + l;
            break;

        case SE_31_FLOOR_RISE_FALL: // True Drop Floor
            if (pData[0] == 1)
            {
                // Choose dir

                if (!RR && pData[3] > 0)
                {
                    pData[3]--;
                    break;
                }

                if (pData[2] == 1) // Retract
                {
                    if (SA(spriteNum) != 1536)
                        HandleSE31(spriteNum, 1, pSprite->z, 0, pSprite->z-pSector->floorz);
                    else
                        HandleSE31(spriteNum, 1, pData[1], 0, pData[1]-pSector->floorz);

                    Yax_SetBunchZs(pSector-sector, YAX_FLOOR, pSector->floorz);

                    break;
                }

                if ((pSprite->ang&2047) == 1536)
                    HandleSE31(spriteNum, 0, pSprite->z, 1, pSprite->z-pSector->floorz);
                else
                    HandleSE31(spriteNum, 0, pData[1], 1, pData[1]-pSprite->z);

                Yax_SetBunchZs(pSector-sector, YAX_FLOOR, pSector->floorz);
            }
            break;

        case SE_32_CEILING_RISE_FALL: // True Drop Ceiling
            if (pData[0] == 1)
            {
                // Choose dir

                if (pData[2] == 1) // Retract
                {
                    if (SA(spriteNum) != 1536)
                    {
                        if (klabs(pSector->ceilingz - pSprite->z) < (SP(spriteNum)<<1))
                        {
                            pSector->ceilingz = pSprite->z;
                            A_CallSound(pSprite->sectnum,spriteNum);
                            pData[2] = 0;
                            pData[0] = 0;
                        }
                        else pSector->ceilingz += ksgn(pSprite->z-pSector->ceilingz)*SP(spriteNum);
                    }
                    else
                    {
                        if (klabs(pSector->ceilingz - pData[1]) < (SP(spriteNum)<<1))
                        {
                            pSector->ceilingz = pData[1];
                            A_CallSound(pSprite->sectnum,spriteNum);
                            pData[2] = 0;
                            pData[0] = 0;
                        }
                        else pSector->ceilingz += ksgn(pData[1]-pSector->ceilingz)*SP(spriteNum);
                    }

                    Yax_SetBunchZs(pSector-sector, YAX_CEILING, pSector->ceilingz);

                    break;
                }

                if ((pSprite->ang&2047) == 1536)
                {
                    if (klabs(pSector->ceilingz-pSprite->z) < (SP(spriteNum)<<1))
                    {
                        pData[0] = 0;
                        pData[2] = !pData[2];
                        A_CallSound(pSprite->sectnum,spriteNum);
                        pSector->ceilingz = pSprite->z;
                    }
                    else pSector->ceilingz += ksgn(pSprite->z-pSector->ceilingz)*SP(spriteNum);
                }
                else
                {
                    if (klabs(pSector->ceilingz-pData[1]) < (SP(spriteNum)<<1))
                    {
                        pData[0] = 0;
                        pData[2] = !pData[2];
                        A_CallSound(pSprite->sectnum,spriteNum);
                    }
                    else pSector->ceilingz -= ksgn(pSprite->z-pData[1])*SP(spriteNum);
                }

                Yax_SetBunchZs(pSector-sector, YAX_CEILING, pSector->ceilingz);
            }
            break;

        case SE_33_QUAKE_DEBRIS:
            if (g_earthquakeTime > 0 && (krand2()&7) == 0)
                RANDOMSCRAP(pSprite, spriteNum);
            break;

        case SE_36_PROJ_SHOOTER:
            if (pData[0])
            {
                if (pData[0] == 1)
                    A_Shoot(spriteNum,pSector->extra);
                else if (pData[0] == GAMETICSPERSEC*5)
                    pData[0] = 0;
                pData[0]++;
            }
            break;

        case 128: //SE to control glass breakage
            {
                walltype *pWall = &wall[pData[2]];

                if (pWall->cstat|32)
                {
                    pWall->cstat &= (255-32);
                    pWall->cstat |= 16;
                    if (pWall->nextwall >= 0)
                    {
                        wall[pWall->nextwall].cstat &= (255-32);
                        wall[pWall->nextwall].cstat |= 16;
                    }
                }
                else break;

                pWall->overpicnum++;
                if (pWall->nextwall >= 0)
                    wall[pWall->nextwall].overpicnum++;

                if (pData[0] < pData[1]) pData[0]++;
                else
                {
                    pWall->cstat &= (128+32+8+4+2);
                    if (pWall->nextwall >= 0)
                        wall[pWall->nextwall].cstat &= (128+32+8+4+2);
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);
                }
            }
            break;

        case SE_130:
            if (pData[0] > 80)
            {
                DELETE_SPRITE_AND_CONTINUE(spriteNum);
            }
            else pData[0]++;

            x = pSector->floorz-pSector->ceilingz;

            if (rnd(64))
            {
                k = A_Spawn(spriteNum,TILE_EXPLOSION2);
                sprite[k].xrepeat = sprite[k].yrepeat = 2+(krand2()&7);
                sprite[k].z = pSector->floorz-(krand2()%x);
                sprite[k].ang += 256-(krand2()%511);
                sprite[k].xvel = krand2()&127;
                A_SetSprite(k,CLIPMASK0);
            }
            break;

        case SE_131:
            if (pData[0] > 40)
            {
                DELETE_SPRITE_AND_CONTINUE(spriteNum);
            }
            else pData[0]++;

            x = pSector->floorz-pSector->ceilingz;

            if (rnd(32))
            {
                k = A_Spawn(spriteNum,TILE_EXPLOSION2);
                sprite[k].xrepeat = sprite[k].yrepeat = 2+(krand2()&3);
                sprite[k].z = pSector->floorz-(krand2()%x);
                sprite[k].ang += 256-(krand2()%511);
                sprite[k].xvel = krand2()&127;
                A_SetSprite(k,CLIPMASK0);
            }
            break;

        case SE_49_POINT_LIGHT:
        case SE_50_SPOT_LIGHT:
            changespritestat(spriteNum, STAT_LIGHT);
            break;
        }
next_sprite:
        spriteNum = nextSprite;
    }

    //Sloped sin-wave floors!
    for (SPRITES_OF(STAT_EFFECTOR, spriteNum))
    {
        const spritetype *s = &sprite[spriteNum];

        if (s->lotag == SE_29_WAVES)
        {
            usectortype const *const sc = (usectortype *)&sector[s->sectnum];

            if (sc->wallnum == 4)
            {
                walltype *const pWall = &wall[sc->wallptr+2];
                if (pWall->nextsector >= 0)
                    alignflorslope(s->sectnum, pWall->x,pWall->y, sector[pWall->nextsector].floorz);
            }
        }
    }
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

void movefta_r(void);
void moveplayers();
void movefx();
void movefallers_r();
void movestandables_r();
void moveweapons_r();
void movetransports_r(void);
void moveactors_r();
void thunder();

void G_MoveWorld_d(void)
{
    extern double g_moveActorsTime, g_moveWorldTime;
    const double worldTime = timerGetHiTicks();

    movefta_d();     //ST 2
    moveweapons_d();          //ST 4
    movetransports_d();       //ST 9

    moveplayers();          //ST 10
    movefallers_d();          //ST 12
    G_MoveMisc();             //ST 5

    const double actorsTime = timerGetHiTicks();

    moveactors_d();           //ST 1

    g_moveActorsTime = (1-0.033)*g_moveActorsTime + 0.033*(timerGetHiTicks()-actorsTime);

    // XXX: Has to be before effectors, in particular movers?
    // TODO: lights in moving sectors ought to be interpolated
    G_DoEffectorLights();
    G_MoveEffectors();        //ST 3
    movestandables_d();       //ST 6

    G_RefreshLights();
    G_DoSectorAnimations();
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
        G_MoveMisc();             //ST 5

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
        G_MoveEffectors();        //ST 3
        movestandables_r();       //ST 6
    }

    G_RefreshLights();
    G_DoSectorAnimations();
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

