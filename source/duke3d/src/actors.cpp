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

#define actors_c_

#include "duke3d.h"

#if KRANDDEBUG
# define ACTOR_STATIC
#else
# define ACTOR_STATIC static
#endif

#define DELETE_SPRITE_AND_CONTINUE(KX) do { A_DeleteSprite(KX); goto next_sprite; } while (0)

extern int32_t g_numEnvSoundsPlaying;
extern int32_t g_noEnemies;

int32_t otherp;

int G_SetInterpolation(int32_t *const posptr)
{
    if (g_interpolationCnt >= MAXINTERPOLATIONS)
        return 1;

    for (bssize_t i = 0; i < g_interpolationCnt; ++i)
        if (curipos[i] == posptr)
            return 0;

    curipos[g_interpolationCnt] = posptr;
    oldipos[g_interpolationCnt] = *posptr;
    g_interpolationCnt++;
    return 0;
}

void G_StopInterpolation(int32_t * const posptr)
{
    for (bssize_t i = 0; i < g_interpolationCnt; ++i)
        if (curipos[i] == posptr)
        {
            g_interpolationCnt--;
            oldipos[i] = oldipos[g_interpolationCnt];
            bakipos[i] = bakipos[g_interpolationCnt];
            curipos[i] = curipos[g_interpolationCnt];
        }
}

void G_DoInterpolations(int smoothRatio)
{
    if (g_interpolationLock++)
        return;

    int32_t odelta, ndelta = 0;

    for (bssize_t i = 0, j = 0; i < g_interpolationCnt; ++i)
    {
        odelta = ndelta;
        bakipos[i] = *curipos[i];
        ndelta = (*curipos[i]) - oldipos[i];
        if (odelta != ndelta)
            j = mulscale16(ndelta, smoothRatio);
        *curipos[i] = oldipos[i] + j;
    }
}

void G_ClearCameraView(DukePlayer_t *ps)
{
    ps->newowner = -1;
    ps->pos = ps->opos;
    ps->ang = ps->oang;

    updatesector(ps->pos.x, ps->pos.y, &ps->cursectnum);
    P_UpdateScreenPal(ps);

    for (bssize_t SPRITES_OF(STAT_ACTOR, k))
        if (sprite[k].picnum==CAMERA1)
            sprite[k].yvel = 0;
}

// Manhattan distance between wall-point and sprite.
static FORCE_INLINE int32_t G_WallSpriteDist(uwalltype const * const wal, uspritetype const * const spr)
{
    return klabs(wal->x - spr->x) + klabs(wal->y - spr->y);
}

void A_RadiusDamage(int spriteNum, int blastRadius, int dmg1, int dmg2, int dmg3, int dmg4)
{
    int32_t d, q;
    uspritetype const *const pSprite = (uspritetype *)&sprite[spriteNum];

    static const int32_t statnumList[] = {
        STAT_DEFAULT, STAT_ACTOR, STAT_STANDABLE,
        STAT_PLAYER, STAT_FALLER, STAT_ZOMBIEACTOR, STAT_MISC
    };

    // XXX: accesses to 'sectorlist' potentially unaligned
    int16_t *const sectorList = (int16_t *)tempbuf;
    const int32_t maxSectors = sizeof(tempbuf)/sizeof(int16_t);

    if (pSprite->picnum == RPG && pSprite->xrepeat < 11)
        goto SKIPWALLCHECK;

    if (pSprite->picnum != SHRINKSPARK)
    {
        int32_t sectorCount = 0;
        int32_t numSectors = 1;

        sectorList[0] = pSprite->sectnum;

        do
        {
            int const sectorNum = sectorList[sectorCount++];
            int const startWall = sector[sectorNum].wallptr;
            int const endWall   = startWall + sector[sectorNum].wallnum;
            int const w2        = wall[startWall].point2;

            // Check if "hit" 1st or 3rd wall-point. This mainly makes sense
            // for rectangular "ceiling light"-style sectors.
            if (G_WallSpriteDist((uwalltype *)&wall[startWall], pSprite) < blastRadius ||
                    G_WallSpriteDist((uwalltype *)&wall[wall[w2].point2], pSprite) < blastRadius)
            {
                if (((sector[sectorNum].ceilingz-pSprite->z)>>8) < blastRadius)
                    Sect_DamageCeilingOrFloor(0, sectorNum);
                if (((pSprite->z-sector[sectorNum].floorz)>>8) < blastRadius)
                    Sect_DamageCeilingOrFloor(1, sectorNum);
            }

            uwalltype const *pWall;
            int w = startWall;
            for (pWall=(uwalltype *)&wall[startWall]; w<endWall; w++,pWall++)
            {
                if (G_WallSpriteDist(pWall, pSprite) < blastRadius)
                {
                    int16_t damageSector = -1;
                    int const nextSector = pWall->nextsector;

                    if (nextSector >= 0)
                    {
                        int32_t dasect2;

                        for (dasect2=numSectors-1; dasect2>=0; dasect2--)
                            if (sectorList[dasect2] == nextSector)
                                break;

                        if (dasect2 < 0)
                        {
                            if (numSectors == maxSectors)
                                goto SKIPWALLCHECK;  // prevent oob access of 'sectorlist'
                            sectorList[numSectors++] = nextSector;
                        }
                    }

                    vec2_t const vect = { (((pWall->x + wall[pWall->point2].x) >> 1) + pSprite->x) >> 1,
                                          (((pWall->y + wall[pWall->point2].y) >> 1) + pSprite->y) >> 1 };

                    updatesector(vect.x, vect.y, &damageSector);

                    if (damageSector >= 0 && cansee(vect.x, vect.y, pSprite->z, damageSector,
                                                    pSprite->x, pSprite->y, pSprite->z, pSprite->sectnum))
                    {
                        vec3_t const tmpvect = { pWall->x, pWall->y, pSprite->z };
                        A_DamageWall(spriteNum, w, &tmpvect, pSprite->picnum);
                    }
                }
            }
        }
        while (sectorCount < numSectors);
    }

SKIPWALLCHECK:

    q = -ZOFFSET2 + (krand()&(ZOFFSET5-1));

    for (bssize_t stati=0; stati < ARRAY_SSIZE(statnumList); stati++)
    {
        int32_t otherSprite = headspritestat[statnumList[stati]];

        while (otherSprite >= 0)
        {
            int const         nextOther = nextspritestat[otherSprite];
            spritetype *const pOther    = &sprite[otherSprite];

            // DEFAULT, ZOMBIEACTOR, MISC
            if (stati == 0 || stati >= 5 || AFLAMABLE(pOther->picnum))
            {
                if (pSprite->picnum != SHRINKSPARK || (pOther->cstat&257))
                    if (dist(pSprite, pOther) < blastRadius)
                    {
                        if (A_CheckEnemySprite(pOther) && !cansee(pOther->x, pOther->y,pOther->z+q, pOther->sectnum, pSprite->x, pSprite->y, pSprite->z+q, pSprite->sectnum))
                            goto next_sprite;
                        A_DamageObject(otherSprite, spriteNum);
                    }
            }
            else if (pOther->extra >= 0 && (uspritetype *)pOther != pSprite &&
                     (pOther->picnum == TRIPBOMB || A_CheckEnemySprite(pOther) || pOther->picnum == QUEBALL ||
                      pOther->picnum == STRIPEBALL || (pOther->cstat & 257) || pOther->picnum == DUKELYINGDEAD))
            {
                if ((pSprite->picnum == SHRINKSPARK && pOther->picnum != SHARK &&
                     (otherSprite == pSprite->owner || pOther->xrepeat < 24)) ||
                    (pSprite->picnum == MORTER && otherSprite == pSprite->owner))
                {
                    otherSprite = nextOther;
                    continue;
                }

                if (pOther->picnum == APLAYER)
                    pOther->z -= PHEIGHT;

                d = dist(pSprite, pOther);

                if (pOther->picnum == APLAYER)
                    pOther->z += PHEIGHT;

                if (d < blastRadius && cansee(pOther->x, pOther->y, pOther->z - ZOFFSET3, pOther->sectnum, pSprite->x,
                                              pSprite->y, pSprite->z - ZOFFSET4, pSprite->sectnum))
                {
                    actor[otherSprite].ang = getangle(pOther->x-pSprite->x,pOther->y-pSprite->y);

                    if (pSprite->picnum == RPG && pOther->extra > 0)
                        actor[otherSprite].picnum = RPG;
                    else if (A_CheckSpriteFlags(spriteNum,SFLAG_PROJECTILE) && SpriteProjectile[spriteNum].workslike & PROJECTILE_RADIUS_PICNUM && pOther->extra > 0)
                        actor[otherSprite].picnum = pSprite->picnum;
                    else
                    {
                        if (pSprite->picnum == SHRINKSPARK)
                            actor[otherSprite].picnum = SHRINKSPARK;
                        else actor[otherSprite].picnum = RADIUSEXPLOSION;
                    }

                    if (pSprite->picnum != SHRINKSPARK)
                    {
                        const int32_t k = blastRadius/3;

                        if (d < k)
                        {
                            if (dmg4 == dmg3) dmg4++;
                            actor[otherSprite].extra = dmg3 + (krand()%(dmg4-dmg3));
                        }
                        else if (d < k*2)
                        {
                            if (dmg3 == dmg2) dmg3++;
                            actor[otherSprite].extra = dmg2 + (krand()%(dmg3-dmg2));
                        }
                        else if (d < blastRadius)
                        {
                            if (dmg2 == dmg1) dmg2++;
                            actor[otherSprite].extra = dmg1 + (krand()%(dmg2-dmg1));
                        }

                        if (!A_CheckSpriteFlags(otherSprite, SFLAG_NODAMAGEPUSH))
                        {
                            if (pOther->xvel < 0) pOther->xvel = 0;
                            pOther->xvel += (pSprite->extra<<2);
                        }

                        if (pOther->picnum == PODFEM1 || pOther->picnum == FEM1 || pOther->picnum == FEM2 ||
                            pOther->picnum == FEM3 || pOther->picnum == FEM4 || pOther->picnum == FEM5 ||
                            pOther->picnum == FEM6 || pOther->picnum == FEM7 || pOther->picnum == FEM8 ||
                            pOther->picnum == FEM9 || pOther->picnum == FEM10 || pOther->picnum == STATUE ||
                            pOther->picnum == STATUEFLASH || pOther->picnum == SPACEMARINE || pOther->picnum == QUEBALL ||
                            pOther->picnum == STRIPEBALL)
                            A_DamageObject(otherSprite, spriteNum);
                    }
                    else if (pSprite->extra == 0) actor[otherSprite].extra = 0;

                    if (pOther->picnum != RADIUSEXPLOSION &&
                            pSprite->owner >= 0 && sprite[pSprite->owner].statnum < MAXSTATUS)
                    {
                        if (pOther->picnum == APLAYER)
                        {
                            DukePlayer_t *pPlayer = g_player[P_GetP((uspritetype * const)pOther)].ps;

                            if (pPlayer->newowner >= 0)
                                G_ClearCameraView(pPlayer);
                        }

                        actor[otherSprite].owner = pSprite->owner;
                    }
                }
            }
next_sprite:
            otherSprite = nextOther;
        }
    }
}

// Maybe do a projectile transport via an SE7.
// <spritenum>: the projectile
// <i>: the SE7
// <fromunderp>: below->above change?
static int32_t Proj_MaybeDoTransport(int32_t spriteNum, const uspritetype * const pSEffector, int32_t fromunderp, int32_t daz)
{
    if ((totalclock & UINT8_MAX) == actor[spriteNum].lasttransport)
        return 0;

    spritetype *const        pSprite = &sprite[spriteNum];
    const uspritetype *const otherse = (uspritetype *)&sprite[pSEffector->owner];
    actor[spriteNum].lasttransport   = (totalclock & UINT8_MAX);

    pSprite->x += (otherse->x - pSEffector->x);
    pSprite->y += (otherse->y - pSEffector->y);

    // above->below
    pSprite->z = (!fromunderp) ? sector[otherse->sectnum].ceilingz - daz + sector[pSEffector->sectnum].floorz
                               : sector[otherse->sectnum].floorz - daz + sector[pSEffector->sectnum].ceilingz;
    // below->above

    actor[spriteNum].bpos = *(vec3_t *)&sprite[spriteNum];
    changespritesect(spriteNum, otherse->sectnum);

    return 1;
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
    if (zChange == 0)
        return 0;

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
            A_Spawn(spriteNum, WATERSPLASH2);
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

int32_t A_MoveSpriteClipdist(int32_t spriteNum, vec3_t const * const change, uint32_t clipType, int32_t clipDist)
{
    spritetype *const pSprite = &sprite[spriteNum];
    int const         isEnemy = A_CheckEnemySprite(pSprite);
    vec2_t const      oldPos  = *(vec2_t *)pSprite;

    if (pSprite->statnum == STAT_MISC || (isEnemy && pSprite->xrepeat < 4))
    {
        pSprite->x += change->x;
        pSprite->y += change->y;
        pSprite->z += change->z;

        if (isEnemy)
            setsprite(spriteNum, (vec3_t *)pSprite);

        return 0;
    }

    if (clipDist >= 0)
    {
        // use that value
    }
    else if (A_CheckSpriteFlags(spriteNum, SFLAG_REALCLIPDIST))
        clipDist = pSprite->clipdist<<2;
    else if (isEnemy)
    {
        if (pSprite->xrepeat > 60)
            clipDist = 1024;
        else if (pSprite->picnum == LIZMAN)
            clipDist = 292;
        else if (A_CheckSpriteFlags(spriteNum, SFLAG_BADGUY))
            clipDist = pSprite->clipdist<<2;
        else
            clipDist = 192;
    }
    else
    {
        if (pSprite->statnum == STAT_PROJECTILE && (SpriteProjectile[spriteNum].workslike & PROJECTILE_REALCLIPDIST) == 0)
            clipDist = 8;
        else
            clipDist = pSprite->clipdist<<2;
    }

    int16_t   newSectnum = pSprite->sectnum;
    int const oldSectnum = newSectnum;
    int32_t   newZ       = pSprite->z - 2 * tilesiz[pSprite->picnum].y * pSprite->yrepeat;
    int const oldZ       = pSprite->z;

    // Handle horizontal movement first.
    pSprite->z = newZ;
    int returnValue =
    clipmove((vec3_t *)pSprite, &newSectnum, change->x << 13, change->y << 13, clipDist, ZOFFSET6, ZOFFSET6, clipType);
    pSprite->z = oldZ;

    if (isEnemy)
    {
        // Handle potential stayput condition (map-provided or hard-coded).
        if (newSectnum < 0 ||
            ((actor[spriteNum].actorstayput >= 0 && actor[spriteNum].actorstayput != newSectnum) ||
             (pSprite->picnum == BOSS2 && pSprite->pal == 0 && sector[newSectnum].lotag != ST_3) ||
             ((pSprite->picnum == BOSS1 || pSprite->picnum == BOSS2) && sector[newSectnum].lotag == ST_1_ABOVE_WATER) ||
             (sector[oldSectnum].lotag != ST_1_ABOVE_WATER && sector[newSectnum].lotag == ST_1_ABOVE_WATER &&
              (pSprite->picnum == LIZMAN || (pSprite->picnum == LIZTROOP && pSprite->zvel == 0)))))
        {
            *(vec2_t *) pSprite = oldPos;

            // NOTE: in Duke3D, LIZMAN on water takes on random angle here.

            setsprite(spriteNum, (vec3_t *)pSprite);

            if (newSectnum < 0)
                newSectnum = 0;

            return 16384+newSectnum;
        }

        if ((returnValue&49152) >= 32768 && actor[spriteNum].cgg==0)
            pSprite->ang += 768;
    }

    if (newSectnum == -1)
    {
        newSectnum = pSprite->sectnum;
//        OSD_Printf("%s:%d wtf\n",__FILE__,__LINE__);
    }
    else if (newSectnum != pSprite->sectnum)
    {
        changespritesect(spriteNum, newSectnum);
        // A_GetZLimits(spritenum);
    }

    Bassert(newSectnum == pSprite->sectnum);

    int const doZUpdate = change->z ? A_CheckNeedZUpdate(spriteNum, change->z, &newZ) : 0;

    // Update sprite's z positions and (for TROR) maybe the sector number.
    if (doZUpdate)
    {
        pSprite->z = newZ;
#ifdef YAX_ENABLE
        if (doZUpdate < 0)
        {
            // If we passed a TROR no-SE7 water boundary, signal to the outside
            // that the ceiling/floor was not hit. However, this is not enough:
            // later, code checks for (retval&49152)!=49152
            // [i.e. not "was ceiling or floor hit", but "was no sprite hit"]
            // and calls G_WeaponHitCeilingOrFloor() then, so we need to set
            // actor[].flags |= SFLAG_DIDNOSE7WATER in A_CheckNeedZUpdate()
            // previously.
            // XXX: Why is this contrived data flow necessary? (If at all.)
            changespritesect(spriteNum, -doZUpdate-1);
            return 0;
        }

        if (yax_getbunch(newSectnum, (change->z>0))>=0
                && (SECTORFLD(newSectnum,stat, (change->z>0))&yax_waltosecmask(clipType))==0)
        {
            setspritez(spriteNum, (vec3_t *)pSprite);
        }
#endif
    }
    else if (change->z != 0 && returnValue == 0)
        returnValue = 16384+newSectnum;

    if (returnValue == 16384 + newSectnum)
    {
        if (pSprite->statnum == STAT_PROJECTILE)
        {
            // Projectile sector changes due to transport SEs (SE7_PROJECTILE).
            // PROJECTILE_CHSECT
            for (bssize_t SPRITES_OF(STAT_TRANSPORT, otherSpriteNum))
            {
                if (sprite[otherSpriteNum].sectnum == newSectnum)
                {
                    int const sectLotag = sector[newSectnum].lotag;

                    if (sectLotag == ST_1_ABOVE_WATER && newZ >= actor[spriteNum].floorz)
                        if (Proj_MaybeDoTransport(spriteNum, (uspritetype *)&sprite[otherSpriteNum], 0, newZ))
                            return 0;

                    if (sectLotag == ST_2_UNDERWATER && newZ <= actor[spriteNum].ceilingz)
                        if (Proj_MaybeDoTransport(spriteNum, (uspritetype *)&sprite[otherSpriteNum], 1, newZ))
                            return 0;
                }
            }
        }
    }

    return returnValue;
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
        OSD_Printf(OSD_ERROR "A_DeleteSprite(): tried to remove sprite %d in EVENT_EGS\n", spriteNum);
        return;
    }

    if (VM_HaveEvent(EVENT_KILLIT))
    {
        int32_t playerDist;
        int playerNum = A_FindPlayer(&sprite[spriteNum], &playerDist);

        if (VM_OnEventWithDist_(EVENT_KILLIT, spriteNum, playerNum, playerDist))
            return;
    }

#ifdef POLYMER
    if (actor[spriteNum].lightptr != NULL && getrendermode() == REND_POLYMER)
        A_DeleteLight(spriteNum);
#endif

    // AMBIENT_SFX_PLAYING
    if (sprite[spriteNum].picnum == MUSICANDSFX && actor[spriteNum].t_data[0] == 1)
        S_StopEnvSound(sprite[spriteNum].lotag, spriteNum);

    // NetAlloc
    if (Net_IsRelevantSprite(spriteNum))
    {
        Net_DeleteSprite(spriteNum);
        return;
    }

    deletesprite(spriteNum);
}

void A_AddToDeleteQueue(int spriteNum)
{
    if (g_deleteQueueSize == 0)
    {
        A_DeleteSprite(spriteNum);
        return;
    }

    if (SpriteDeletionQueue[g_spriteDeleteQueuePos] >= 0)
        sprite[SpriteDeletionQueue[g_spriteDeleteQueuePos]].xrepeat = 0;
    SpriteDeletionQueue[g_spriteDeleteQueuePos] = spriteNum;
    g_spriteDeleteQueuePos = (g_spriteDeleteQueuePos+1)%g_deleteQueueSize;
}

void A_SpawnMultiple(int spriteNum, int tileNum, int spawnCnt)
{
    spritetype *pSprite = &sprite[spriteNum];

    for (; spawnCnt>0; spawnCnt--)
    {
        int const j = A_InsertSprite(pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z - (krand() % (47 << 8)), tileNum, -32, 8,
                               8, krand() & 2047, 0, 0, spriteNum, 5);
        A_Spawn(-1, j);
        sprite[j].cstat = krand()&12;
    }
}

void A_DoGuts(int spriteNum, int tileNum, int spawnCnt)
{
    uspritetype const *const pSprite = (uspritetype *)&sprite[spriteNum];
    vec2_t                   repeat  = { 32, 32 };

    if (A_CheckEnemySprite(pSprite) && pSprite->xrepeat < 16)
        repeat.x = repeat.y = 8;

    int gutZ   = pSprite->z - ZOFFSET3;
    int floorz = getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y);

    if (gutZ > (floorz-ZOFFSET3))
        gutZ = floorz-ZOFFSET3;

    if (pSprite->picnum == COMMANDER)
        gutZ -= (24<<8);

    for (bssize_t j=spawnCnt; j>0; j--)
    {
        int const i = A_InsertSprite(pSprite->sectnum, pSprite->x + (krand() & 255) - 128,
                                     pSprite->y + (krand() & 255) - 128, gutZ - (krand() & 8191), tileNum, -32, repeat.x,
                                     repeat.y, krand() & 2047, 48 + (krand() & 31), -512 - (krand() & 2047), spriteNum, 5);

        if (PN(i) == JIBS2)
        {
            sprite[i].xrepeat >>= 2;
            sprite[i].yrepeat >>= 2;
        }

        sprite[i].pal = pSprite->pal;
    }
}

void A_DoGutsDir(int spriteNum, int tileNum, int spawnCnt)
{
    uspritetype const * const s = (uspritetype *)&sprite[spriteNum];
    vec2_t repeat = { 32, 32 };

    if (A_CheckEnemySprite(s) && s->xrepeat < 16)
        repeat.x = repeat.y = 8;

    int gutZ = s->z-ZOFFSET3;
    int floorZ = getflorzofslope(s->sectnum,s->x,s->y);

    if (gutZ > (floorZ-ZOFFSET3))
        gutZ = floorZ-ZOFFSET3;

    if (s->picnum == COMMANDER)
        gutZ -= (24<<8);

    for (bssize_t j=spawnCnt; j>0; j--)
    {
        int const i = A_InsertSprite(s->sectnum, s->x, s->y, gutZ, tileNum, -32, repeat.x, repeat.y, krand() & 2047,
                                     256 + (krand() & 127), -512 - (krand() & 2047), spriteNum, 5);
        sprite[i].pal = s->pal;
    }
}

LUNATIC_EXTERN int32_t G_ToggleWallInterpolation(int32_t wallNum, int32_t setInterpolation)
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

static int32_t move_rotfixed_sprite(int32_t spriteNum, int32_t pivotSpriteNum, int32_t pivotAngle)
{
    if ((ROTFIXSPR_STATNUMP(sprite[spriteNum].statnum) ||
         ((sprite[spriteNum].statnum == STAT_ACTOR || sprite[spriteNum].statnum == STAT_ZOMBIEACTOR) &&
          A_CheckSpriteFlags(spriteNum, SFLAG_ROTFIXED))) &&
        actor[spriteNum].t_data[7] == (ROTFIXSPR_MAGIC | pivotSpriteNum))
    {
        rotatepoint(zerovec, *(vec2_t *)&actor[spriteNum].t_data[8], pivotAngle & 2047, (vec2_t *)&sprite[spriteNum].x);
        sprite[spriteNum].x += sprite[pivotSpriteNum].x;
        sprite[spriteNum].y += sprite[pivotSpriteNum].y;
        return 0;
    }

    return 1;
}

void A_MoveSector(int spriteNum)
{
    // T1,T2 and T3 are used for all the sector moving stuff!!!

    int32_t           playerDist;
    spritetype *const pSprite     = &sprite[spriteNum];
    int const         playerNum   = A_FindPlayer(pSprite, &playerDist);
    int const         rotateAngle = VM_OnEventWithBoth(EVENT_MOVESECTOR, spriteNum, playerNum, playerDist, T3(spriteNum));
    int               originIdx   = T2(spriteNum);

    pSprite->x += (pSprite->xvel * (sintable[(pSprite->ang + 512) & 2047])) >> 14;
    pSprite->y += (pSprite->xvel * (sintable[pSprite->ang & 2047])) >> 14;

    int const endWall = sector[pSprite->sectnum].wallptr + sector[pSprite->sectnum].wallnum;

    for (bssize_t wallNum = sector[pSprite->sectnum].wallptr; wallNum < endWall; wallNum++)
    {
        vec2_t const origin = g_origins[originIdx];
        vec2_t result;
        rotatepoint(zerovec, origin, rotateAngle & 2047, &result);
        dragpoint(wallNum, pSprite->x + result.x, pSprite->y + result.y, 0);

        originIdx++;
    }
}

#if !defined LUNATIC
// NOTE: T5 is AC_ACTION_ID
# define LIGHTRAD_PICOFS(i) (T5(i) ? *(apScript + T5(i)) + (*(apScript + T5(i) + 2)) * AC_CURFRAME(actor[i].t_data) : 0)
#else
// startframe + viewtype*[cyclic counter]
# define LIGHTRAD_PICOFS(i) (actor[i].ac.startframe + actor[i].ac.viewtype * AC_CURFRAME(actor[i].t_data))
#endif

// this is the same crap as in game.c's tspr manipulation.  puke.
// XXX: may access tilesizy out-of-bounds by bad user code.
#define LIGHTRAD(spriteNum, s) (s->yrepeat * tilesiz[s->picnum + LIGHTRAD_PICOFS(spriteNum)].y)
#define LIGHTRAD2(spriteNum, s) ((s->yrepeat + ((rand() % s->yrepeat)>>2)) * tilesiz[s->picnum + LIGHTRAD_PICOFS(spriteNum)].y)

void G_AddGameLight(int lightRadius, int spriteNum, int zOffset, int lightRange, int lightColor, int lightPrio)
{
#ifdef POLYMER
    spritetype *s = &sprite[spriteNum];

    if (getrendermode() != REND_POLYMER || pr_lighting != 1)
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

ACTOR_STATIC void A_MaybeAwakenBadGuys(int const spriteNum)
{
    if (sprite[spriteNum].sectnum == MAXSECTORS)
        return;

    if (A_CheckSpriteFlags(spriteNum, SFLAG_WAKEUPBADGUYS))
    {
        uspritetype *const pSprite = (uspritetype *)&sprite[spriteNum];

        for (bssize_t nextSprite, SPRITES_OF_STAT_SAFE(STAT_ZOMBIEACTOR, spriteNum, nextSprite))
        {
            if (A_CheckEnemySprite(&sprite[spriteNum]))
            {
                if (sprite[spriteNum].sectnum == pSprite->sectnum
                    || sprite[spriteNum].sectnum == nextsectorneighborz(pSprite->sectnum, sector[pSprite->sectnum].floorz, 1, 1)
                    || cansee(pSprite->x, pSprite->y, pSprite->z - PHEIGHT, pSprite->sectnum, sprite[spriteNum].x, sprite[spriteNum].y,
                              sprite[spriteNum].z - PHEIGHT, sprite[spriteNum].sectnum))
                {
                    actor[spriteNum].timetosleep = 0;
                    A_PlayAlertSound(spriteNum);
                    changespritestat(spriteNum, STAT_ACTOR);

                    if (A_CheckSpriteFlags(spriteNum, SFLAG_WAKEUPBADGUYS))
                        A_MaybeAwakenBadGuys(spriteNum);
                }
            }
        }
    }
}


// sleeping monsters, etc
ACTOR_STATIC void G_MoveZombieActors(void)
{
    int spriteNum = headspritestat[STAT_ZOMBIEACTOR], canSeePlayer;

    while (spriteNum >= 0)
    {
        int const           nextSprite = nextspritestat[spriteNum];
        int32_t             playerDist;
        spritetype *const   pSprite   = &sprite[spriteNum];
        int const           playerNum = A_FindPlayer(pSprite, &playerDist);
        DukePlayer_t *const pPlayer   = g_player[playerNum].ps;

        if (sprite[pPlayer->i].extra > 0)
        {
            if (playerDist < 30000)
            {
                actor[spriteNum].timetosleep++;
                if (actor[spriteNum].timetosleep >= (playerDist>>8))
                {
                    if (pPlayer->newowner == -1 && A_CheckEnemySprite(pSprite))
                    {
                        vec3_t const p = { pPlayer->pos.x + 64 - (krand() & 127),
                                           pPlayer->pos.y + 64 - (krand() & 127),
                                           pPlayer->pos.z - (krand() % ZOFFSET5) };

                        int16_t pSectnum = pPlayer->cursectnum;

                        updatesector(p.x, p.y, &pSectnum);

                        if (pSectnum == -1)
                        {
                            spriteNum = nextSprite;
                            continue;
                        }

                        vec3_t const s = { pSprite->x + 64 - (krand() & 127),
                                           pSprite->y + 64 - (krand() & 127),
                                           pSprite->z - (krand() % (52 << 8)) };

                        int16_t sectNum = pSprite->sectnum;

                        updatesector(s.x, s.y, &sectNum);

                        if (sectNum == -1)
                        {
                            spriteNum = nextSprite;
                            continue;
                        }

                        canSeePlayer = cansee(s.x, s.y, s.z, sectNum, p.x, p.y, p.z, pSectnum);
                    }
                    else
                        canSeePlayer = cansee(pSprite->x, pSprite->y, pSprite->z - ((krand() & 31) << 8), pSprite->sectnum, pPlayer->opos.x,
                            pPlayer->opos.y, pPlayer->opos.z - ((krand() & 31) << 8), pPlayer->cursectnum);

                    if (canSeePlayer)
                    {
                        switch (DYNAMICTILEMAP(pSprite->picnum))
                        {
                            case RUBBERCAN__STATIC:
                            case EXPLODINGBARREL__STATIC:
                            case WOODENHORSE__STATIC:
                            case HORSEONSIDE__STATIC:
                            case CANWITHSOMETHING__STATIC:
                            case CANWITHSOMETHING2__STATIC:
                            case CANWITHSOMETHING3__STATIC:
                            case CANWITHSOMETHING4__STATIC:
                            case FIREBARREL__STATIC:
                            case FIREVASE__STATIC:
                            case NUKEBARREL__STATIC:
                            case NUKEBARRELDENTED__STATIC:
                            case NUKEBARRELLEAKED__STATIC:
                            case TRIPBOMB__STATIC:
                                pSprite->shade = ((sector[pSprite->sectnum].ceilingstat & 1) && A_CheckSpriteFlags(spriteNum, SFLAG_NOSHADE) == 0)
                                                ? sector[pSprite->sectnum].ceilingshade
                                                : sector[pSprite->sectnum].floorshade;
                                actor[spriteNum].timetosleep = 0;
                                changespritestat(spriteNum, STAT_STANDABLE);
                                break;

                            case RECON__STATIC:
                                CS(spriteNum) |= 257;
                            // fall-through
                            default:
                                if (A_CheckSpriteFlags(spriteNum, SFLAG_USEACTIVATOR) && sector[sprite[spriteNum].sectnum].lotag & 16384)
                                    break;

                                actor[spriteNum].timetosleep = 0;
                                A_PlayAlertSound(spriteNum);
                                changespritestat(spriteNum, STAT_ACTOR);

                                if (A_CheckSpriteFlags(spriteNum, SFLAG_WAKEUPBADGUYS))
                                    A_MaybeAwakenBadGuys(spriteNum);

                                break;
                        }
                    }
                    else
                        actor[spriteNum].timetosleep = 0;
                }
            }

            if (A_CheckEnemySprite(pSprite) && A_CheckSpriteFlags(spriteNum,SFLAG_NOSHADE) == 0)
            {
                pSprite->shade = (sector[pSprite->sectnum].ceilingstat & 1)
                                ? sector[pSprite->sectnum].ceilingshade
                                : sector[pSprite->sectnum].floorshade;
            }
        }

        spriteNum = nextSprite;
    }
}

// stupid name, but it's what the function does.
static FORCE_INLINE int G_FindExplosionInSector(int const sectNum)
{
    for (bssize_t SPRITES_OF(STAT_MISC, i))
        if (PN(i) == EXPLOSION2 && sectNum == SECT(i))
            return i;

    return -1;
}

static FORCE_INLINE void P_Nudge(int playerNum, int spriteNum, int shiftLeft)
{
    g_player[playerNum].ps->vel.x += actor[spriteNum].extra * (sintable[(actor[spriteNum].ang + 512) & 2047]) << shiftLeft;
    g_player[playerNum].ps->vel.y += actor[spriteNum].extra * (sintable[actor[spriteNum].ang & 2047]) << shiftLeft;
}

int A_IncurDamage(int const spriteNum)
{
    spritetype *const pSprite = &sprite[spriteNum];
    actor_t *const    pActor  = &actor[spriteNum];

    // dmg->picnum check: safety, since it might have been set to <0 from CON.
    if (pActor->extra < 0 || pSprite->extra < 0 || pActor->picnum < 0)
    {
        pActor->extra = -1;
        return -1;
    }

    if (pSprite->picnum == APLAYER)
    {
        if (ud.god && pActor->picnum != SHRINKSPARK)
            return -1;

        int const playerNum = P_GetP(pSprite);

        if (pActor->owner >= 0 && ud.ffire == 0 && sprite[pActor->owner].picnum == APLAYER &&
            (g_gametypeFlags[ud.coop] & GAMETYPE_PLAYERSFRIENDLY ||
             (g_gametypeFlags[ud.coop] & GAMETYPE_TDM && g_player[playerNum].ps->team == g_player[P_Get(pActor->owner)].ps->team)))
            return -1;

        pSprite->extra -= pActor->extra;

        if (pActor->owner >= 0 && pSprite->extra <= 0 && pActor->picnum != FREEZEBLAST)
        {
            int const damageOwner = pActor->owner;
            pSprite->extra        = 0;

            g_player[playerNum].ps->wackedbyactor = damageOwner;

            if (sprite[damageOwner].picnum == APLAYER && playerNum != P_Get(damageOwner))
                g_player[playerNum].ps->frag_ps = P_Get(damageOwner);

            pActor->owner = g_player[playerNum].ps->i;
        }

        switch (DYNAMICTILEMAP(pActor->picnum))
        {
            case RADIUSEXPLOSION__STATIC:
            case RPG__STATIC:
            case HYDRENT__STATIC:
            case HEAVYHBOMB__STATIC:
            case SEENINE__STATIC:
            case OOZFILTER__STATIC:
            case EXPLODINGBARREL__STATIC:
                P_Nudge(playerNum, spriteNum, 2);
                break;

            default:
                P_Nudge(playerNum, spriteNum, (A_CheckSpriteFlags(pActor->owner, SFLAG_PROJECTILE) &&
                                       (SpriteProjectile[pActor->owner].workslike & PROJECTILE_RPG))
                                      ? 2
                                      : 1);
                break;
        }

        pActor->extra = -1;
        return pActor->picnum;
    }

    if (pActor->extra == 0 && pActor->picnum == SHRINKSPARK && pSprite->xrepeat < 24)
        return -1;

    pSprite->extra -= pActor->extra;

    if (pSprite->picnum != RECON && pSprite->owner >= 0 && sprite[pSprite->owner].statnum < MAXSTATUS)
        pSprite->owner = pActor->owner;

    pActor->extra = -1;

    return pActor->picnum;
}

void A_MoveCyclers(void)
{
    for (bssize_t i=g_cyclerCnt-1; i>=0; i--)
    {
        int16_t *const pCycler     = g_cyclers[i];
        int const      sectNum     = pCycler[0];
        int            spriteShade = pCycler[2];
        int const      floorShade  = pCycler[3];
        int            sectorShade = clamp(floorShade + (sintable[pCycler[1] & 2047] >> 10), spriteShade, floorShade);

        pCycler[1] += sector[sectNum].extra;

        if (pCycler[5]) // angle 1536...
        {
            walltype *pWall = &wall[sector[sectNum].wallptr];

            for (bssize_t wallsLeft = sector[sectNum].wallnum; wallsLeft > 0; wallsLeft--, pWall++)
            {
                if (pWall->hitag != 1)
                {
                    pWall->shade = sectorShade;

                    if ((pWall->cstat&2) && pWall->nextwall >= 0)
                        wall[pWall->nextwall].shade = sectorShade;
                }
            }

            sector[sectNum].floorshade = sector[sectNum].ceilingshade = sectorShade;
        }
    }
}

void A_MoveDummyPlayers(void)
{
    int spriteNum = headspritestat[STAT_DUMMYPLAYER];

    while (spriteNum >= 0)
    {
        int const           playerNum     = P_Get(OW(spriteNum));
        DukePlayer_t *const pPlayer       = g_player[playerNum].ps;
        int const           nextSprite    = nextspritestat[spriteNum];
        int const           playerSectnum = pPlayer->cursectnum;

        if (pPlayer->on_crane >= 0 || (playerSectnum >= 0 && sector[playerSectnum].lotag != ST_1_ABOVE_WATER) || sprite[pPlayer->i].extra <= 0)
        {
            pPlayer->dummyplayersprite = -1;
            DELETE_SPRITE_AND_CONTINUE(spriteNum);
        }
        else
        {
            if (pPlayer->on_ground && pPlayer->on_warping_sector == 1 && playerSectnum >= 0 && sector[playerSectnum].lotag == ST_1_ABOVE_WATER)
            {
                CS(spriteNum) = 257;
                SZ(spriteNum) = sector[SECT(spriteNum)].ceilingz+(27<<8);
                SA(spriteNum) = pPlayer->ang;
                if (T1(spriteNum) == 8)
                    T1(spriteNum) = 0;
                else T1(spriteNum)++;
            }
            else
            {
                if (sector[SECT(spriteNum)].lotag != ST_2_UNDERWATER) SZ(spriteNum) = sector[SECT(spriteNum)].floorz;
                CS(spriteNum) = 32768;
            }
        }

        SX(spriteNum) += (pPlayer->pos.x-pPlayer->opos.x);
        SY(spriteNum) += (pPlayer->pos.y-pPlayer->opos.y);
        setsprite(spriteNum, (vec3_t *)&sprite[spriteNum]);

next_sprite:
        spriteNum = nextSprite;
    }
}


static int P_Submerge(int, int, DukePlayer_t *, int, int);
static int P_Emerge(int, int, DukePlayer_t *, int, int);
static void P_FinishWaterChange(int, DukePlayer_t *, int, int, int);

ACTOR_STATIC void G_MovePlayers(void)
{
    int spriteNum = headspritestat[STAT_PLAYER];

    while (spriteNum >= 0)
    {
        int const           nextSprite = nextspritestat[spriteNum];
        spritetype *const   pSprite    = &sprite[spriteNum];
        DukePlayer_t *const pPlayer    = g_player[P_GetP(pSprite)].ps;

        if (pSprite->owner >= 0)
        {
            if (pPlayer->newowner >= 0)  //Looking thru the camera
            {
                pSprite->x              = pPlayer->opos.x;
                pSprite->y              = pPlayer->opos.y;
                pSprite->z              = pPlayer->opos.z + PHEIGHT;
                actor[spriteNum].bpos.z = pSprite->z;
                pSprite->ang            = pPlayer->oang;

                setsprite(spriteNum, (vec3_t *)pSprite);
            }
            else
            {
                int32_t otherPlayerDist;
#ifdef YAX_ENABLE
                // TROR water submerge/emerge
                int const playerSectnum = pSprite->sectnum;
                int const sectorLotag   = sector[playerSectnum].lotag;
                int32_t   otherSector;

                if (A_CheckNoSE7Water((uspritetype const *)pSprite, playerSectnum, sectorLotag, &otherSector))
                {
                    // NOTE: Compare with G_MoveTransports().
                    pPlayer->on_warping_sector = 1;

                    if (((sectorLotag == ST_1_ABOVE_WATER) ?
                        P_Submerge(spriteNum, P_GetP(pSprite), pPlayer, playerSectnum, otherSector) :
                        P_Emerge(spriteNum, P_GetP(pSprite), pPlayer, playerSectnum, otherSector) == 1))
                        P_FinishWaterChange(spriteNum, pPlayer, sectorLotag, -1, otherSector);
                }
#endif
                if (g_netServer || ud.multimode > 1)
                    otherp = P_FindOtherPlayer(P_GetP(pSprite), &otherPlayerDist);
                else
                {
                    otherp = P_GetP(pSprite);
                    otherPlayerDist = 0;
                }

                if (G_HaveActor(sprite[spriteNum].picnum))
                    A_Execute(spriteNum, P_GetP(pSprite), otherPlayerDist);

                if (g_netServer || ud.multimode > 1)
                {
                    if (sprite[g_player[otherp].ps->i].extra > 0)
                    {
                        if (pSprite->yrepeat > 32 && sprite[g_player[otherp].ps->i].yrepeat < 32)
                        {
                            if (otherPlayerDist < 1400 && pPlayer->knee_incs == 0)
                            {
                                pPlayer->knee_incs = 1;
                                pPlayer->weapon_pos = -1;
                                pPlayer->actorsqu = g_player[otherp].ps->i;
                            }
                        }
                    }
                }

                if (ud.god)
                {
                    pSprite->extra = pPlayer->max_player_health;
                    pSprite->cstat = 257;
                    if (!WW2GI)
                        pPlayer->inv_amount[GET_JETPACK] = 1599;
                }

                if (pSprite->extra > 0)
                {
                    actor[spriteNum].owner = spriteNum;

                    if (ud.god == 0)
                        if (G_CheckForSpaceCeiling(pSprite->sectnum) || G_CheckForSpaceFloor(pSprite->sectnum))
                            P_QuickKill(pPlayer);
                }
                else
                {
                    pPlayer->pos.x = pSprite->x;
                    pPlayer->pos.y = pSprite->y;
                    pPlayer->pos.z = pSprite->z-(20<<8);

                    pPlayer->newowner = -1;

                    if (pPlayer->wackedbyactor >= 0 && sprite[pPlayer->wackedbyactor].statnum < MAXSTATUS)
                    {
                        pPlayer->ang += G_GetAngleDelta(pPlayer->ang,getangle(sprite[pPlayer->wackedbyactor].x-pPlayer->pos.x,sprite[pPlayer->wackedbyactor].y-pPlayer->pos.y))>>1;
                        pPlayer->ang &= 2047;
                    }
                }

                pSprite->ang = pPlayer->ang;
            }
        }
        else
        {
            if (pPlayer->holoduke_on == -1)
                DELETE_SPRITE_AND_CONTINUE(spriteNum);

            Bmemcpy(&actor[spriteNum].bpos, pSprite, sizeof(vec3_t));
            pSprite->cstat = 0;

            if (pSprite->xrepeat < 42)
            {
                pSprite->xrepeat += 4;
                pSprite->cstat |= 2;
            }
            else pSprite->xrepeat = 42;

            if (pSprite->yrepeat < 36)
                pSprite->yrepeat += 4;
            else
            {
                pSprite->yrepeat = 36;
                if (sector[pSprite->sectnum].lotag != ST_2_UNDERWATER)
                    A_Fall(spriteNum);
                if (pSprite->zvel == 0 && sector[pSprite->sectnum].lotag == ST_1_ABOVE_WATER)
                    pSprite->z += ZOFFSET5;
            }

            if (pSprite->extra < 8)
            {
                pSprite->xvel = 128;
                pSprite->ang = pPlayer->ang;
                pSprite->extra++;
                A_SetSprite(spriteNum,CLIPMASK0);
            }
            else
            {
                pSprite->ang = 2047-pPlayer->ang;
                setsprite(spriteNum,(vec3_t *)pSprite);
            }
        }

        pSprite->shade =
        logapproach(pSprite->shade, (sector[pSprite->sectnum].ceilingstat & 1) ? sector[pSprite->sectnum].ceilingshade
                                                                               : sector[pSprite->sectnum].floorshade);

next_sprite:
        spriteNum = nextSprite;
    }
}

ACTOR_STATIC void G_MoveFX(void)
{
    int spriteNum = headspritestat[STAT_FX];

    while (spriteNum >= 0)
    {
        spritetype *const pSprite    = &sprite[spriteNum];
        int const         nextSprite = nextspritestat[spriteNum];

        switch (DYNAMICTILEMAP(pSprite->picnum))
        {
        case RESPAWN__STATIC:
            if (pSprite->extra == 66)
            {
                /*int32_t j =*/ A_Spawn(spriteNum,SHT(spriteNum));
                //                    sprite[j].pal = sprite[i].pal;
                DELETE_SPRITE_AND_CONTINUE(spriteNum);
            }
            else if (pSprite->extra > (66-13))
                sprite[spriteNum].extra++;
            break;

        case MUSICANDSFX__STATIC:
        {
            int const           spriteHitag = pSprite->hitag;
            DukePlayer_t *const pPlayer     = g_player[screenpeek].ps;

            if (T2(spriteNum) != ud.config.SoundToggle)
            {
                // If sound playback was toggled, restart.
                T2(spriteNum) = ud.config.SoundToggle;
                T1(spriteNum) = 0;
            }

            if (pSprite->lotag >= 1000 && pSprite->lotag < 2000)
            {
                int playerDist = ldist(&sprite[pPlayer->i],pSprite);

#ifdef SPLITSCREEN_MOD_HACKS
                if (g_fakeMultiMode==2)
                {
                    // HACK for splitscreen mod
                    int32_t otherdist = ldist(&sprite[g_player[1].ps->i],pSprite);
                    playerDist = min(playerDist, otherdist);
                }
#endif

                if (playerDist < spriteHitag && T1(spriteNum) == 0)
                {
                    FX_SetReverb(pSprite->lotag - 1000);
                    T1(spriteNum) = 1;
                }
                else if (playerDist >= spriteHitag && T1(spriteNum) == 1)
                {
                    FX_SetReverb(0);
                    FX_SetReverbDelay(0);
                    T1(spriteNum) = 0;
                }
            }
            else if (pSprite->lotag < 999 && (unsigned)sector[pSprite->sectnum].lotag < 9 &&  // ST_9_SLIDING_ST_DOOR
                         ud.config.AmbienceToggle && sector[SECT(spriteNum)].floorz != sector[SECT(spriteNum)].ceilingz)
            {
                if (g_sounds[pSprite->lotag].m & SF_MSFX)
                {
                    int playerDist = dist(&sprite[pPlayer->i], pSprite);

#ifdef SPLITSCREEN_MOD_HACKS
                    if (g_fakeMultiMode==2)
                    {
                        // HACK for splitscreen mod
                        int32_t otherdist = dist(&sprite[g_player[1].ps->i],pSprite);
                        playerDist = min(playerDist, otherdist);
                    }
#endif

                    if (playerDist < spriteHitag && T1(spriteNum) == 0 && FX_VoiceAvailable(g_sounds[pSprite->lotag].pr-1))
                    {
                        // Start playing an ambience sound.

                        char om = g_sounds[pSprite->lotag].m;
                        if (g_numEnvSoundsPlaying == ud.config.NumVoices)
                        {
                            int32_t j;

                            for (SPRITES_OF(STAT_FX, j))
                                if (j != spriteNum && S_IsAmbientSFX(j) && actor[j].t_data[0] == 1 &&
                                        dist(&sprite[j], &sprite[pPlayer->i]) > playerDist)
                                {
                                    S_StopEnvSound(sprite[j].lotag,j);
                                    break;
                                }

                            if (j == -1)
                                goto next_sprite;
                        }

                        g_sounds[pSprite->lotag].m |= SF_LOOP;
                        A_PlaySound(pSprite->lotag,spriteNum);
                        g_sounds[pSprite->lotag].m = om;
                        T1(spriteNum) = 1;  // AMBIENT_SFX_PLAYING
                    }
                    else if (playerDist >= spriteHitag && T1(spriteNum) == 1)
                    {
                        // Stop playing ambience sound because we're out of its range.

                        // T1 will be reset in sounds.c: CLEAR_SOUND_T0
                        // T1 = 0;
                        S_StopEnvSound(pSprite->lotag,spriteNum);
                    }
                }

                if (g_sounds[pSprite->lotag].m & SF_GLOBAL)
                {
                    // Randomly playing global sounds (flyby of planes, screams, ...)

                    if (T5(spriteNum) > 0)
                        T5(spriteNum)--;
                    else
                    {
                        for (int TRAVERSE_CONNECT(playerNum))
                            if (playerNum == myconnectindex && g_player[playerNum].ps->cursectnum == pSprite->sectnum)
                            {
                                S_PlaySound(pSprite->lotag + (unsigned)g_globalRandom % (pSprite->hitag+1));
                                T5(spriteNum) = GAMETICSPERSEC*40 + g_globalRandom%(GAMETICSPERSEC*40);
                            }
                    }
                }
            }
            break;
        }
        }
next_sprite:
        spriteNum = nextSprite;
    }
}

ACTOR_STATIC void G_MoveFallers(void)
{
    int spriteNum = headspritestat[STAT_FALLER];

    while (spriteNum >= 0)
    {
        int const         nextSprite = nextspritestat[spriteNum];
        spritetype *const pSprite    = &sprite[spriteNum];
        int const         sectNum    = pSprite->sectnum;

        if (T1(spriteNum) == 0)
        {
            const int16_t oextra = pSprite->extra;
            int j;

            pSprite->z -= ZOFFSET2;
            T2(spriteNum) = pSprite->ang;

            if ((j = A_IncurDamage(spriteNum)) >= 0)
            {
                if (j == FIREEXT || j == RPG || j == RADIUSEXPLOSION || j == SEENINE || j == OOZFILTER)
                {
                    if (pSprite->extra <= 0)
                    {
                        T1(spriteNum) = 1;

                        for (bssize_t SPRITES_OF(STAT_FALLER, j))
                        {
                            if (sprite[j].hitag == SHT(spriteNum))
                            {
                                actor[j].t_data[0] = 1;
                                sprite[j].cstat &= (65535-64);
                                if (sprite[j].picnum == CEILINGSTEAM || sprite[j].picnum == STEAM)
                                    sprite[j].cstat |= 32768;
                            }
                        }
                    }
                }
                else
                {
                    actor[spriteNum].extra = 0;
                    pSprite->extra = oextra;
                }
            }
            pSprite->ang = T2(spriteNum);
            pSprite->z += ZOFFSET2;
        }
        else if (T1(spriteNum) == 1)
        {
            if ((int16_t)pSprite->lotag > 0)
            {
                pSprite->lotag-=3;
                if ((int16_t)pSprite->lotag <= 0)
                {
                    pSprite->xvel = (32+(krand()&63));
                    pSprite->zvel = -(1024+(krand()&1023));
                }
            }
            else
            {
                int32_t spriteGravity = g_spriteGravity;

                if (pSprite->xvel > 0)
                {
                    pSprite->xvel -= 8;
                    A_SetSprite(spriteNum,CLIPMASK0);
                }

                if (EDUKE32_PREDICT_FALSE(G_CheckForSpaceFloor(pSprite->sectnum)))
                    spriteGravity = 0;
                else if (EDUKE32_PREDICT_FALSE(G_CheckForSpaceCeiling(pSprite->sectnum)))
                    spriteGravity = g_spriteGravity / 6;

                if (pSprite->z < (sector[sectNum].floorz-ZOFFSET))
                {
                    pSprite->zvel += spriteGravity;
                    if (pSprite->zvel > 6144)
                        pSprite->zvel = 6144;
                    pSprite->z += pSprite->zvel;
                }

                if ((sector[sectNum].floorz-pSprite->z) < ZOFFSET2)
                {
                    for (bssize_t x=0; x<1+(krand()&7); ++x)
                        RANDOMSCRAP(pSprite, spriteNum);
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);
                }
            }
        }

next_sprite:
        spriteNum = nextSprite;
    }
}

ACTOR_STATIC void G_MoveStandables(void)
{
    int spriteNum = headspritestat[STAT_STANDABLE], j, switchPic;
    int32_t playerDist;

    while (spriteNum >= 0)
    {
        const int         nextSprite = nextspritestat[spriteNum];
        int32_t *const    pData      = &actor[spriteNum].t_data[0];
        spritetype *const pSprite    = &sprite[spriteNum];
        const int         sectNum    = pSprite->sectnum;

        if (sectNum < 0)
            DELETE_SPRITE_AND_CONTINUE(spriteNum);

        // Rotation-fixed sprites in rotating sectors already have bpos* updated.
        if ((pData[7]&(0xffff0000))!=ROTFIXSPR_MAGIC)
            Bmemcpy(&actor[spriteNum].bpos, pSprite, sizeof(vec3_t));

        if (PN(spriteNum) >= CRANE && PN(spriteNum) <= CRANE+3)
        {
            int32_t nextj;

            //t[0] = state
            //t[1] = checking sector number

            if (pSprite->xvel) A_GetZLimits(spriteNum);

            if (pData[0] == 0)   //Waiting to check the sector
            {
                for (SPRITES_OF_SECT_SAFE(pData[1], j, nextj))
                {
                    switch (sprite[j].statnum)
                    {
                        case STAT_ACTOR:
                        case STAT_ZOMBIEACTOR:
                        case STAT_STANDABLE:
                        case STAT_PLAYER:
                        {
                            vec3_t vect = { g_origins[pData[4]+1].x, g_origins[pData[4]+1].y, sprite[j].z };

                            pSprite->ang = getangle(vect.x-pSprite->x, vect.y-pSprite->y);
                            setsprite(j, &vect);
                            pData[0]++;
                            goto next_sprite;
                        }
                    }
                }
            }

            else if (pData[0]==1)
            {
                if (pSprite->xvel < 184)
                {
                    pSprite->picnum = CRANE+1;
                    pSprite->xvel += 8;
                }
                A_SetSprite(spriteNum,CLIPMASK0);
                if (sectNum == pData[1])
                    pData[0]++;
            }
            else if (pData[0]==2 || pData[0]==7)
            {
                pSprite->z += (1024+512);

                if (pData[0]==2)
                {
                    if (sector[sectNum].floorz - pSprite->z < (64<<8))
                        if (pSprite->picnum > CRANE) pSprite->picnum--;

                    if (sector[sectNum].floorz - pSprite->z < 4096+1024)
                        pData[0]++;
                }

                if (pData[0]==7)
                {
                    if (sector[sectNum].floorz - pSprite->z < (64<<8))
                    {
                        if (pSprite->picnum > CRANE) pSprite->picnum--;
                        else
                        {
                            if (pSprite->owner==-2)
                            {
                                int32_t p = A_FindPlayer(pSprite, NULL);
                                A_PlaySound(DUKE_GRUNT,g_player[p].ps->i);
                                if (g_player[p].ps->on_crane == spriteNum)
                                    g_player[p].ps->on_crane = -1;
                            }

                            pData[0]++;
                            pSprite->owner = -1;
                        }
                    }
                }
            }
            else if (pData[0]==3)
            {
                pSprite->picnum++;
                if (pSprite->picnum == CRANE+2)
                {
                    int32_t p = G_CheckPlayerInSector(pData[1]);

                    if (p >= 0 && g_player[p].ps->on_ground)
                    {
                        pSprite->owner = -2;
                        g_player[p].ps->on_crane = spriteNum;
                        A_PlaySound(DUKE_GRUNT,g_player[p].ps->i);
                        g_player[p].ps->ang = pSprite->ang+1024;
                    }
                    else
                    {
                        for (SPRITES_OF_SECT(pData[1], j))
                        {
                            switch (sprite[j].statnum)
                            {
                            case STAT_ACTOR:
                            case STAT_STANDABLE:
                                pSprite->owner = j;
                                break;
                            }
                        }
                    }

                    pData[0]++;//Grabbed the sprite
                    pData[2]=0;
                    goto next_sprite;
                }
            }
            else if (pData[0]==4) //Delay before going up
            {
                pData[2]++;
                if (pData[2] > 10)
                    pData[0]++;
            }
            else if (pData[0]==5 || pData[0] == 8)
            {
                if (pData[0]==8 && pSprite->picnum < (CRANE+2))
                    if ((sector[sectNum].floorz-pSprite->z) > 8192)
                        pSprite->picnum++;

                if (pSprite->z < g_origins[pData[4]+2].x)
                {
                    pData[0]++;
                    pSprite->xvel = 0;
                }
                else
                    pSprite->z -= (1024+512);
            }
            else if (pData[0]==6)
            {
                if (pSprite->xvel < 192)
                    pSprite->xvel += 8;
                pSprite->ang = getangle(g_origins[pData[4]].x - pSprite->x, g_origins[pData[4]].y - pSprite->y);
                A_SetSprite(spriteNum,CLIPMASK0);
                if (((pSprite->x-g_origins[pData[4]].x)*(pSprite->x-g_origins[pData[4]].x)+(pSprite->y-g_origins[pData[4]].y)*(pSprite->y-g_origins[pData[4]].y)) < (128*128))
                    pData[0]++;
            }

            else if (pData[0]==9)
                pData[0] = 0;

            {
                vec3_t vect;
                Bmemcpy(&vect,pSprite,sizeof(vec3_t));
                vect.z -= (34<<8);
                setsprite(g_origins[pData[4]+2].y, &vect);
            }


            if (pSprite->owner != -1)
            {
                int32_t p = A_FindPlayer(pSprite, NULL);

                if (A_IncurDamage(spriteNum) >= 0)
                {
                    if (pSprite->owner == -2)
                        if (g_player[p].ps->on_crane == spriteNum)
                            g_player[p].ps->on_crane = -1;
                    pSprite->owner = -1;
                    pSprite->picnum = CRANE;
                    goto next_sprite;
                }

                if (pSprite->owner >= 0)
                {
                    setsprite(pSprite->owner,(vec3_t *)pSprite);

                    Bmemcpy(&actor[pSprite->owner].bpos, pSprite, sizeof(vec3_t));

                    pSprite->zvel = 0;
                }
                else if (pSprite->owner == -2)
                {
                    DukePlayer_t *const ps = g_player[p].ps;

                    ps->opos.x = ps->pos.x = pSprite->x-(sintable[(ps->ang+512)&2047]>>6);
                    ps->opos.y = ps->pos.y = pSprite->y-(sintable[ps->ang&2047]>>6);
                    ps->opos.z = ps->pos.z = pSprite->z+(2<<8);

                    setsprite(ps->i, (vec3_t *)ps);
                    ps->cursectnum = sprite[ps->i].sectnum;
                }
            }

            goto next_sprite;
        }
        else if (PN(spriteNum) >= WATERFOUNTAIN && PN(spriteNum) <= WATERFOUNTAIN+3)
        {
            if (pData[0] > 0)
            {
                if (pData[0] < 20)
                {
                    pData[0]++;

                    pSprite->picnum++;

                    if (pSprite->picnum == (WATERFOUNTAIN+3))
                        pSprite->picnum = WATERFOUNTAIN+1;
                }
                else
                {
                    A_FindPlayer(pSprite,&playerDist);

                    if (playerDist > 512)
                    {
                        pData[0] = 0;
                        pSprite->picnum = WATERFOUNTAIN;
                    }
                    else pData[0] = 1;
                }
            }
            goto next_sprite;
        }
        else if (AFLAMABLE(pSprite->picnum))
        {
            if (T1(spriteNum) == 1)
            {
                if ((++T2(spriteNum)&3) > 0) goto next_sprite;

                if (pSprite->picnum == TIRE && T2(spriteNum) == 32)
                {
                    pSprite->cstat = 0;
                    j = A_Spawn(spriteNum,BLOODPOOL);
                    sprite[j].shade = 127;
                }
                else
                {
                    if (pSprite->shade < 64) pSprite->shade++;
                    else DELETE_SPRITE_AND_CONTINUE(spriteNum);
                }

                j = pSprite->xrepeat-(krand()&7);
                if (j < 10)
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);

                pSprite->xrepeat = j;

                j = pSprite->yrepeat-(krand()&7);
                if (j < 4)
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);

                pSprite->yrepeat = j;
            }
            if (pSprite->picnum == BOX)
            {
                A_Fall(spriteNum);
                actor[spriteNum].ceilingz = sector[pSprite->sectnum].ceilingz;
            }
            goto next_sprite;
        }
        else if (pSprite->picnum == TRIPBOMB)
        {
            // TIMER_CONTROL
            if (actor[spriteNum].t_data[6] == 1)
            {

                if (actor[spriteNum].t_data[7] >= 1)
                {
                    actor[spriteNum].t_data[7]--;
                }

                if (actor[spriteNum].t_data[7] <= 0)
                {
                    T3(spriteNum)=16;
                    actor[spriteNum].t_data[6]=3;
                    A_PlaySound(LASERTRIP_ARMING,spriteNum);
                }
                // we're on a timer....
            }
            if (T3(spriteNum) > 0 && actor[spriteNum].t_data[6] == 3)
            {
                T3(spriteNum)--;

                if (T3(spriteNum) == 8)
                {
                    for (j=0; j<5; j++)
                        RANDOMSCRAP(pSprite, spriteNum);

                    int const dmg = pSprite->extra;
                    A_RadiusDamage(spriteNum, g_tripbombRadius, dmg>>2, dmg>>1, dmg-(dmg>>2), dmg);

                    j = A_Spawn(spriteNum,EXPLOSION2);
                    A_PlaySound(LASERTRIP_EXPLODE,j);
                    sprite[j].ang = pSprite->ang;
                    sprite[j].xvel = 348;
                    A_SetSprite(j,CLIPMASK0);

                    for (SPRITES_OF(STAT_MISC, j))
                    {
                        if (sprite[j].picnum == LASERLINE && pSprite->hitag == sprite[j].hitag)
                            sprite[j].xrepeat = sprite[j].yrepeat = 0;
                    }

                    DELETE_SPRITE_AND_CONTINUE(spriteNum);
                }
                goto next_sprite;
            }
            else
            {
                int const oldExtra = pSprite->extra;
                int const oldAng = pSprite->ang;

                pSprite->extra = 1;
                if (A_IncurDamage(spriteNum) >= 0)
                {
                    actor[spriteNum].t_data[6] = 3;
                    T3(spriteNum) = 16;
                }
                pSprite->extra = oldExtra;
                pSprite->ang = oldAng;
            }

            switch (T1(spriteNum))
            {
            default:
            {
                int32_t playerDist;
                A_FindPlayer(pSprite, &playerDist);
                if (playerDist > 768 || T1(spriteNum) > 16) T1(spriteNum)++;
                break;
            }

            case 32:
            {
                int16_t hitSprite;
                int const oldAng = pSprite->ang;

                pSprite->ang = T6(spriteNum);

                T4(spriteNum) = pSprite->x;
                T5(spriteNum) = pSprite->y;

                pSprite->x += sintable[(T6(spriteNum)+512)&2047]>>9;
                pSprite->y += sintable[(T6(spriteNum))&2047]>>9;
                pSprite->z -= (3<<8);

                setsprite(spriteNum,(vec3_t *)pSprite);

                int hitDist = A_CheckHitSprite(spriteNum, &hitSprite);

                actor[spriteNum].lastv.x = hitDist;
                pSprite->ang = oldAng;

                //                if(lTripBombControl & TRIPBOMB_TRIPWIRE)
                if (actor[spriteNum].t_data[6] != 1)
                {
                    // we're on a trip wire
                    int16_t cursectnum;

                    while (hitDist > 0)
                    {
                        j = A_Spawn(spriteNum,LASERLINE);
                        setsprite(j,(vec3_t *)&sprite[j]);
                        sprite[j].hitag = pSprite->hitag;
                        actor[j].t_data[1] = sprite[j].z;

                        pSprite->x += sintable[(T6(spriteNum)+512)&2047]>>4;
                        pSprite->y += sintable[(T6(spriteNum))&2047]>>4;

                        if (hitDist < 1024)
                        {
                            sprite[j].xrepeat = hitDist>>5;
                            break;
                        }
                        hitDist -= 1024;

                        cursectnum = pSprite->sectnum;
                        updatesector(pSprite->x, pSprite->y, &cursectnum);
                        if (cursectnum < 0)
                            break;
                    }
                }

                T1(spriteNum)++;

                pSprite->x = T4(spriteNum);
                pSprite->y = T5(spriteNum);
                pSprite->z += (3<<8);

                setsprite(spriteNum,(vec3_t *)pSprite);
                T4(spriteNum) = T3(spriteNum) = 0;

                if (hitSprite >= 0 && actor[spriteNum].t_data[6] != 1)
                {
                    actor[spriteNum].t_data[6] = 3;
                    T3(spriteNum) = 13;
                    A_PlaySound(LASERTRIP_ARMING,spriteNum);
                }
                break;
            }

            case 33:
            {
                T2(spriteNum)++;

                T4(spriteNum) = pSprite->x;
                T5(spriteNum) = pSprite->y;

                pSprite->x += sintable[(T6(spriteNum)+512)&2047]>>9;
                pSprite->y += sintable[(T6(spriteNum))&2047]>>9;
                pSprite->z -= (3<<8);

                setsprite(spriteNum, (vec3_t *) pSprite);

                int hitDist = A_CheckHitSprite(spriteNum, NULL);

                pSprite->x = T4(spriteNum);
                pSprite->y = T5(spriteNum);
                pSprite->z += (3<<8);
                setsprite(spriteNum, (vec3_t *) pSprite);

                //                if( Actor[i].lastvx != x && lTripBombControl & TRIPBOMB_TRIPWIRE)
                if (actor[spriteNum].lastv.x != hitDist && actor[spriteNum].t_data[6] != 1)
                {
                    actor[spriteNum].t_data[6] = 3;
                    T3(spriteNum) = 13;
                    A_PlaySound(LASERTRIP_ARMING, spriteNum);
                }
                break;
            }
            }

            goto next_sprite;
        }
        else if (pSprite->picnum >= CRACK1 && pSprite->picnum <= CRACK4)
        {
            if (pSprite->hitag > 0)
            {
                pData[0] = pSprite->cstat;
                pData[1] = pSprite->ang;

                int const dmgTile = A_IncurDamage(spriteNum);

                if (dmgTile < 0)
                    goto crack_default;

                switch (DYNAMICTILEMAP(dmgTile))
                {
                    case FIREEXT__STATIC:
                    case RPG__STATIC:
                    case RADIUSEXPLOSION__STATIC:
                    case SEENINE__STATIC:
                    case OOZFILTER__STATIC:
                        for (SPRITES_OF(STAT_STANDABLE, j))
                        {
                            if (pSprite->hitag == sprite[j].hitag &&
                                (sprite[j].picnum == OOZFILTER || sprite[j].picnum == SEENINE))
                                if (sprite[j].shade != -32)
                                    sprite[j].shade = -32;
                        }

                        goto DETONATE;

crack_default:
                    default:
                        pSprite->cstat = pData[0];
                        pSprite->ang   = pData[1];
                        pSprite->extra = 0;

                        goto next_sprite;
                }
            }
            goto next_sprite;
        }
        else if (pSprite->picnum == FIREEXT)
        {
            if (A_IncurDamage(spriteNum) < 0)
                goto next_sprite;

            for (bsize_t k=0; k<16; k++)
            {
                j = A_InsertSprite(SECT(spriteNum), SX(spriteNum), SY(spriteNum), SZ(spriteNum) - (krand() % (48 << 8)),
                                   SCRAP3 + (krand() & 3), -8, 48, 48, krand() & 2047, (krand() & 63) + 64,
                                   -(krand() & 4095) - (sprite[spriteNum].zvel >> 2), spriteNum, 5);

                sprite[j].pal = 2;
            }

            j = A_Spawn(spriteNum,EXPLOSION2);
            A_PlaySound(PIPEBOMB_EXPLODE,j);
            A_PlaySound(GLASS_HEAVYBREAK,j);

            if ((int16_t)pSprite->hitag > 0)
            {
                for (SPRITES_OF(STAT_STANDABLE, j))
                {
                    // XXX: This block seems to be CODEDUP'd a lot of times.
                    if (pSprite->hitag == sprite[j].hitag && (sprite[j].picnum == OOZFILTER || sprite[j].picnum == SEENINE))
                        if (sprite[j].shade != -32)
                            sprite[j].shade = -32;
                }

                int const dmg = pSprite->extra;
                A_RadiusDamage(spriteNum, g_pipebombRadius,dmg>>2, dmg-(dmg>>1),dmg-(dmg>>2), dmg);
                j = A_Spawn(spriteNum,EXPLOSION2);
                A_PlaySound(PIPEBOMB_EXPLODE,j);

                goto DETONATE;
            }
            else
            {
                A_RadiusDamage(spriteNum,g_seenineRadius,10,15,20,25);
                DELETE_SPRITE_AND_CONTINUE(spriteNum);
            }
            goto next_sprite;
        }
        else if (pSprite->picnum == OOZFILTER || pSprite->picnum == SEENINE || pSprite->picnum == SEENINEDEAD || pSprite->picnum == SEENINEDEAD+1)
        {
            if (pSprite->shade != -32 && pSprite->shade != -33)
            {
                if (pSprite->xrepeat)
                    j = (A_IncurDamage(spriteNum) >= 0);
                else
                    j = 0;

                if (j || pSprite->shade == -31)
                {
                    if (j) pSprite->lotag = 0;

                    pData[3] = 1;

                    for (SPRITES_OF(STAT_STANDABLE, j))
                    {
                        if (pSprite->hitag == sprite[j].hitag && (sprite[j].picnum == SEENINE || sprite[j].picnum == OOZFILTER))
                            sprite[j].shade = -32;
                    }
                }
            }
            else
            {
                if (pSprite->shade == -32)
                {
                    if ((int16_t)pSprite->lotag > 0)
                    {
                        pSprite->lotag -= 3;
                        if ((int16_t)pSprite->lotag <= 0)
                            pSprite->lotag = (uint16_t)(-99);
                    }
                    else
                        pSprite->shade = -33;
                }
                else
                {
                    if (pSprite->xrepeat > 0)
                    {
                        T3(spriteNum)++;
                        if (T3(spriteNum) == 3)
                        {
                            if (pSprite->picnum == OOZFILTER)
                            {
                                T3(spriteNum) = 0;
                                goto DETONATE;
                            }

                            if (pSprite->picnum != (SEENINEDEAD+1))
                            {
                                T3(spriteNum) = 0;

                                if (pSprite->picnum == SEENINEDEAD)
                                    pSprite->picnum++;
                                else if (pSprite->picnum == SEENINE)
                                    pSprite->picnum = SEENINEDEAD;
                            }
                            else goto DETONATE;
                        }
                        goto next_sprite;
                    }

DETONATE:
                    g_earthquakeTime = 16;

                    for (SPRITES_OF(STAT_EFFECTOR, j))
                    {
                        if (pSprite->hitag == sprite[j].hitag)
                        {
                            if (sprite[j].lotag == SE_13_EXPLOSIVE)
                            {
                                if (actor[j].t_data[2] == 0)
                                    actor[j].t_data[2] = 1;
                            }
                            else if (sprite[j].lotag == SE_8_UP_OPEN_DOOR_LIGHTS)
                                actor[j].t_data[4] = 1;
                            else if (sprite[j].lotag == SE_18_INCREMENTAL_SECTOR_RISE_FALL)
                            {
                                if (actor[j].t_data[0] == 0)
                                    actor[j].t_data[0] = 1;
                            }
                            else if (sprite[j].lotag == SE_21_DROP_FLOOR)
                                actor[j].t_data[0] = 1;
                        }
                    }

                    pSprite->z -= ZOFFSET5;

                    if (pSprite->xrepeat)
                        for (bsize_t x=0; x<8; x++)
                            RANDOMSCRAP(pSprite, spriteNum);

                    if ((pData[3] == 1 && pSprite->xrepeat) || (int16_t)pSprite->lotag == -99)
                    {
                        int const newSprite = A_Spawn(spriteNum,EXPLOSION2);
                        int const dmg = pSprite->extra;

                        A_RadiusDamage(spriteNum,g_seenineRadius,dmg>>2, dmg-(dmg>>1),dmg-(dmg>>2), dmg);
                        A_PlaySound(PIPEBOMB_EXPLODE, newSprite);
                    }

                    DELETE_SPRITE_AND_CONTINUE(spriteNum);
                }
            }
            goto next_sprite;
        }
        else if (pSprite->picnum == MASTERSWITCH)
        {
            if (pSprite->yvel == 1)
            {
                if ((int16_t)--pSprite->hitag <= 0)
                {
                    G_OperateSectors(sectNum,spriteNum);

                    for (SPRITES_OF_SECT(sectNum, j))
                    {
                        if (sprite[j].statnum == STAT_EFFECTOR)
                        {
                            switch (sprite[j].lotag)
                            {
                            case SE_2_EARTHQUAKE:
                            case SE_21_DROP_FLOOR:
                            case SE_31_FLOOR_RISE_FALL:
                            case SE_32_CEILING_RISE_FALL:
                            case SE_36_PROJ_SHOOTER:
                                actor[j].t_data[0] = 1;
                                break;
                            case SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT:
                                actor[j].t_data[4] = 1;
                                break;
                            }
                        }
                        else if (sprite[j].statnum == STAT_STANDABLE)
                        {
                            switch (DYNAMICTILEMAP(sprite[j].picnum))
                            {
                            case SEENINE__STATIC:
                            case OOZFILTER__STATIC:
                                sprite[j].shade = -31;
                                break;
                            }
                        }
                    }

                    DELETE_SPRITE_AND_CONTINUE(spriteNum);
                }
            }
            goto next_sprite;
        }
        else
        {
            switchPic = pSprite->picnum;

            if (switchPic > SIDEBOLT1 && switchPic <= SIDEBOLT1 + 3)
                switchPic = SIDEBOLT1;
            else if (switchPic > BOLT1 && switchPic <= BOLT1 + 3)
                switchPic = BOLT1;

            switch (DYNAMICTILEMAP(switchPic))
            {
                case VIEWSCREEN__STATIC:
                case VIEWSCREEN2__STATIC:

                    if (pSprite->xrepeat == 0)
                        DELETE_SPRITE_AND_CONTINUE(spriteNum);

                    {
                        int32_t   playerDist;
                        int const p = A_FindPlayer(pSprite, &playerDist);
                        const DukePlayer_t *const ps = g_player[p].ps;

                        if (dist(&sprite[ps->i], pSprite) < VIEWSCREEN_ACTIVE_DISTANCE)
                        {
#if 0
                        if (sprite[i].yvel == 1)  // VIEWSCREEN_YVEL
                            g_curViewscreen = i;
#endif
                        }
                        else if (g_curViewscreen == spriteNum /*&& T1 == 1*/)
                        {
                            g_curViewscreen        = -1;
                            sprite[spriteNum].yvel = 0;  // VIEWSCREEN_YVEL
                            T1(spriteNum)          = 0;

                            for (bssize_t ii = 0; ii < VIEWSCREENFACTOR; ii++) walock[TILE_VIEWSCR - ii] = 199;
                        }
                    }

                    goto next_sprite;

                case TRASH__STATIC:

                    if (pSprite->xvel == 0)
                        pSprite->xvel = 1;
                    if (A_SetSprite(spriteNum, CLIPMASK0))
                    {
                        A_Fall(spriteNum);
                        if (krand() & 1)
                            pSprite->zvel -= 256;
                        if ((pSprite->xvel) < 48)
                            pSprite->xvel += (krand() & 3);
                    }
                    else
                        DELETE_SPRITE_AND_CONTINUE(spriteNum);
                    break;

                case SIDEBOLT1__STATIC:
                case BOLT1__STATIC:
                {
                    int32_t playerDist;
                    A_FindPlayer(pSprite, &playerDist);
                    if (playerDist > 20480)
                        goto next_sprite;

                    if (pData[3] == 0)
                        pData[3] = sector[sectNum].floorshade;

                CLEAR_THE_BOLT:
                    if (pData[2])
                    {
                        pData[2]--;
                        if (DYNAMICTILEMAP(switchPic) == BOLT1__STATIC)
                        {
                            sector[sectNum].floorshade   = 20;
                            sector[sectNum].ceilingshade = 20;
                        }
                        goto next_sprite;
                    }
                    if ((pSprite->xrepeat | pSprite->yrepeat) == 0)
                    {
                        pSprite->xrepeat = pData[0];
                        pSprite->yrepeat = pData[1];
                    }
                    else if ((krand() & 8) == 0)
                    {
                        pData[0]         = pSprite->xrepeat;
                        pData[1]         = pSprite->yrepeat;
                        pData[2]         = g_globalRandom & 4;
                        pSprite->xrepeat = pSprite->yrepeat = 0;
                        goto CLEAR_THE_BOLT;
                    }
                    pSprite->picnum++;

                    int const randomRepeat = g_globalRandom & 7;
                    pSprite->xrepeat = randomRepeat + 8;

                    if (randomRepeat & 1)
                        pSprite->cstat ^= 2;

                    if ((pSprite->picnum == (BOLT1 + 1) || pSprite->picnum == (SIDEBOLT1 + 1))
                        && (krand() & 7) == 0 && sector[sectNum].floorpicnum == HURTRAIL)
                        A_PlaySound(SHORT_CIRCUIT, spriteNum);

                    if (DYNAMICTILEMAP(switchPic) == BOLT1__STATIC)
                    {
                        if (pSprite->picnum == BOLT1 + 4)
                            pSprite->picnum = BOLT1;

                        if (pSprite->picnum & 1)
                        {
                            sector[sectNum].floorshade   = 0;
                            sector[sectNum].ceilingshade = 0;
                        }
                        else
                        {
                            sector[sectNum].floorshade   = 20;
                            sector[sectNum].ceilingshade = 20;
                        }
                    }
                    else if (pSprite->picnum == SIDEBOLT1 + 4)
                        pSprite->picnum = SIDEBOLT1;

                    goto next_sprite;
                }

                case WATERDRIP__STATIC:

                    if (pData[1])
                    {
                        if (--pData[1] == 0)
                            pSprite->cstat &= 32767;
                    }
                    else
                    {
                        A_Fall(spriteNum);
                        A_SetSprite(spriteNum, CLIPMASK0);
                        if (pSprite->xvel > 0)
                            pSprite->xvel -= 2;

                        if (pSprite->zvel == 0)
                        {
                            pSprite->cstat |= 32768;

                            if (pSprite->pal != 2 && pSprite->hitag == 0)
                                A_PlaySound(SOMETHING_DRIPPING, spriteNum);

                            if (sprite[pSprite->owner].picnum != WATERDRIP)
                            {
                                DELETE_SPRITE_AND_CONTINUE(spriteNum);
                            }
                            else
                            {
                                actor[spriteNum].bpos.z = pSprite->z = pData[0];
                                pData[1]                             = 48 + (krand() & 31);
                            }
                        }
                    }


                    goto next_sprite;

                case DOORSHOCK__STATIC:
                    pSprite->yrepeat = (klabs(sector[sectNum].ceilingz - sector[sectNum].floorz) >> 9) + 4;
                    pSprite->xrepeat = 16;
                    pSprite->z       = sector[sectNum].floorz;
                    goto next_sprite;

                case TOUCHPLATE__STATIC:
                    if (pData[1] == 1 && (int16_t)pSprite->hitag >= 0)  // Move the sector floor
                    {
                        int const floorZ = sector[sectNum].floorz;

                        if (pData[3] == 1)
                        {
                            if (floorZ >= pData[2])
                            {
                                sector[sectNum].floorz = floorZ;
                                pData[1]               = 0;
                            }
                            else
                            {
                                sector[sectNum].floorz += sector[sectNum].extra;
                                int const playerNum = G_CheckPlayerInSector(sectNum);
                                if (playerNum >= 0)
                                    g_player[playerNum].ps->pos.z += sector[sectNum].extra;
                            }
                        }
                        else
                        {
                            if (floorZ <= pSprite->z)
                            {
                                sector[sectNum].floorz = pSprite->z;
                                pData[1]               = 0;
                            }
                            else
                            {
                                int32_t p;
                                sector[sectNum].floorz -= sector[sectNum].extra;
                                p = G_CheckPlayerInSector(sectNum);
                                if (p >= 0)
                                    g_player[p].ps->pos.z -= sector[sectNum].extra;
                            }
                        }
                        goto next_sprite;
                    }

                    if (pData[5] == 1)
                        goto next_sprite;

                    {
                        int32_t p = G_CheckPlayerInSector(sectNum);

                        if (p >= 0 && (g_player[p].ps->on_ground || pSprite->ang == 512))
                        {
                            if (pData[0] == 0 && !G_CheckActivatorMotion(pSprite->lotag))
                            {
                                pData[0] = 1;
                                pData[1] = 1;
                                pData[3] = !pData[3];
                                G_OperateMasterSwitches(pSprite->lotag);
                                G_OperateActivators(pSprite->lotag, p);
                                if ((int16_t)pSprite->hitag > 0)
                                {
                                    pSprite->hitag--;
                                    if (pSprite->hitag == 0)
                                        pData[5] = 1;
                                }
                            }
                        }
                        else
                            pData[0] = 0;
                    }

                    if (pData[1] == 1)
                    {
                        for (SPRITES_OF(STAT_STANDABLE, j))
                        {
                            if (j != spriteNum && sprite[j].picnum == TOUCHPLATE && sprite[j].lotag == pSprite->lotag)
                            {
                                actor[j].t_data[1] = 1;
                                actor[j].t_data[3] = pData[3];
                            }
                        }
                    }
                    goto next_sprite;

                case CANWITHSOMETHING__STATIC:
                case CANWITHSOMETHING2__STATIC:
                case CANWITHSOMETHING3__STATIC:
                case CANWITHSOMETHING4__STATIC:
                    A_Fall(spriteNum);
                    if (A_IncurDamage(spriteNum) >= 0)
                    {
                        A_PlaySound(VENT_BUST, spriteNum);

                        for (j = 9; j >= 0; j--) RANDOMSCRAP(pSprite, spriteNum);

                        if (pSprite->lotag)
                            A_Spawn(spriteNum, pSprite->lotag);

                        DELETE_SPRITE_AND_CONTINUE(spriteNum);
                    }
                    goto next_sprite;

                case FLOORFLAME__STATIC:
                case FIREBARREL__STATIC:
                case FIREVASE__STATIC:
                case EXPLODINGBARREL__STATIC:
                case WOODENHORSE__STATIC:
                case HORSEONSIDE__STATIC:
                case NUKEBARREL__STATIC:
                case NUKEBARRELDENTED__STATIC:
                case NUKEBARRELLEAKED__STATIC:
                case TOILETWATER__STATIC:
                case RUBBERCAN__STATIC:
                case STEAM__STATIC:
                case CEILINGSTEAM__STATIC:
                case WATERBUBBLEMAKER__STATIC:
                    if (!G_HaveActor(sprite[spriteNum].picnum))
                        goto next_sprite;
                    {
                        int32_t playerDist;
                        int const playerNum = A_FindPlayer(pSprite, &playerDist);
                        A_Execute(spriteNum, playerNum, playerDist);
                    }
                    goto next_sprite;
            }
        }

    next_sprite:
        spriteNum = nextSprite;
    }
}

ACTOR_STATIC void A_DoProjectileBounce(int const spriteNum)
{
    spritetype * const pSprite = &sprite[spriteNum];
    int32_t const hitSectnum = pSprite->sectnum;
    int const firstWall  = sector[hitSectnum].wallptr;
    int const secondWall = wall[firstWall].point2;
    int const wallAngle  = getangle(wall[secondWall].x - wall[firstWall].x, wall[secondWall].y - wall[firstWall].y);
    vec3_t    vect       = { mulscale10(pSprite->xvel, sintable[(pSprite->ang + 512) & 2047]),
                                mulscale10(pSprite->xvel, sintable[pSprite->ang & 2047]), pSprite->zvel };

    int k = (pSprite->z<(actor[spriteNum].floorz + actor[spriteNum].ceilingz)>> 1) ? sector[hitSectnum].ceilingheinum
                                                                   : sector[hitSectnum].floorheinum;

    vec3_t const da = { mulscale14(k, sintable[(wallAngle)&2047]),
                        mulscale14(k, sintable[(wallAngle + 1536) & 2047]), 4096 };

    k     = vect.x * da.x + vect.y * da.y + vect.z * da.z;
    int l = da.x * da.x + da.y * da.y + da.z * da.z;

    if ((klabs(k) >> 14) < l)
    {
        k = divscale17(k, l);
        vect.x -= mulscale16(da.x, k);
        vect.y -= mulscale16(da.y, k);
        vect.z -= mulscale16(da.z, k);
    }

    pSprite->zvel = vect.z;
    pSprite->xvel = ksqrt(dmulscale8(vect.x, vect.x, vect.y, vect.y));
    pSprite->ang = getangle(vect.x, vect.y);
}

ACTOR_STATIC void P_HandleBeingSpitOn(DukePlayer_t * const ps)
{
    ps->horiz += 32;
    ps->return_to_center = 8;

    if (ps->loogcnt)
        return;

    if (!A_CheckSoundPlaying(ps->i, DUKE_LONGTERM_PAIN))
        A_PlaySound(DUKE_LONGTERM_PAIN,ps->i);

    int j = 3+(krand()&3);
    ps->numloogs = j;
    ps->loogcnt = 24*4;
    for (bssize_t x=0; x < j; x++)
    {
        ps->loogiex[x] = krand()%xdim;
        ps->loogiey[x] = krand()%ydim;
    }
}

static void A_DoProjectileEffects(int spriteNum, const vec3_t *davect, int radiusDamage)
{
    projectile_t const * const pProj = &SpriteProjectile[spriteNum];

    if (pProj->spawns >= 0)
    {
        int const newSpr = A_Spawn(spriteNum,pProj->spawns);

        if (davect)
            Bmemcpy(&sprite[newSpr],davect,sizeof(vec3_t));

        if (pProj->sxrepeat > 4)
            sprite[newSpr].xrepeat=pProj->sxrepeat;
        if (pProj->syrepeat > 4)
            sprite[newSpr].yrepeat=pProj->syrepeat;
    }

    if (pProj->isound >= 0)
        A_PlaySound(pProj->isound,spriteNum);

    if (!radiusDamage)
        return;

    spritetype *const pSprite = &sprite[spriteNum];
    pSprite->extra = Proj_GetDamage(pProj);
    int const dmg = pSprite->extra;
    A_RadiusDamage(spriteNum, pProj->hitradius, dmg >> 2, dmg >> 1, dmg - (dmg >> 2), dmg);
}

static void G_WeaponHitCeilingOrFloor(int32_t i, spritetype *s, int *j)
{
    if (actor[i].flags & SFLAG_DIDNOSE7WATER)
    {
        actor[i].flags &= ~SFLAG_DIDNOSE7WATER;
        return;
    }

    if (s->z < actor[i].ceilingz)
    {
        *j = 16384|s->sectnum;
        s->zvel = -1;
    }
    else if (s->z > actor[i].floorz + ZOFFSET2*(sector[s->sectnum].lotag == ST_1_ABOVE_WATER))
    {
        *j = 16384|s->sectnum;

        if (sector[s->sectnum].lotag != ST_1_ABOVE_WATER)
            s->zvel = 1;
    }
}

static void Proj_BounceOffWall(spritetype *s, int j)
{
    int k = getangle(
        wall[wall[j].point2].x-wall[j].x,
        wall[wall[j].point2].y-wall[j].y);
    s->ang = ((k<<1) - s->ang)&2047;
}

#define PROJ_DECAYVELOCITY(s) s->xvel >>= 1, s->zvel >>= 1

// Maybe damage a ceiling or floor as the consequence of projectile impact.
// Returns 1 if sprite <s> should be killed.
// NOTE: Compare with Proj_MaybeDamageCF2() in sector.c
static int Proj_MaybeDamageCF(uspritetype const * const s)
{
    if (s->zvel < 0)
    {
        if ((sector[s->sectnum].ceilingstat&1) && sector[s->sectnum].ceilingpal == 0)
            return 1;

        Sect_DamageCeilingOrFloor(0, s->sectnum);
    }
    else if (s->zvel > 0)
    {
        if ((sector[s->sectnum].floorstat&1) && sector[s->sectnum].floorpal == 0)
        {
            // Keep original Duke3D behavior: pass projectiles through
            // parallaxed ceilings, but NOT through such floors.
            return 0;
        }

        Sect_DamageCeilingOrFloor(1, s->sectnum);
    }

    return 0;
}

ACTOR_STATIC void Proj_MoveCustom(int const spriteNum)
{
    int const projectileMoved = SpriteProjectile[spriteNum].workslike & PROJECTILE_MOVED;
    SpriteProjectile[spriteNum].workslike |= PROJECTILE_MOVED;
    const projectile_t *const pProj   = &SpriteProjectile[spriteNum];
    spritetype *const         pSprite = &sprite[spriteNum];
    vec3_t                    davect;
    int                       otherSprite = 0;

    switch (pProj->workslike & PROJECTILE_TYPE_MASK)
    {
        case PROJECTILE_HITSCAN:
        {
            if (!G_HaveActor(sprite[spriteNum].picnum))
                return;
            int32_t   playerDist;
            int const playerNum = A_FindPlayer(pSprite, &playerDist);
            A_Execute(spriteNum, playerNum, playerDist);
            return;
        }

        case PROJECTILE_KNEE:
        case PROJECTILE_BLOOD: A_DeleteSprite(spriteNum); return;

        default:
        case PROJECTILE_RPG:
        {
            davect = *(vec3_t *) pSprite;

            VM_UpdateAnim(spriteNum, &actor[spriteNum].t_data[0]);

            if (pProj->flashcolor)
                G_AddGameLight(0, spriteNum, ((pSprite->yrepeat * tilesiz[pSprite->picnum].y) << 1), 2048, pProj->flashcolor,
                               PR_LIGHT_PRIO_LOW_GAME);

            if ((pProj->workslike & (PROJECTILE_BOUNCESOFFWALLS | PROJECTILE_EXPLODEONTIMER)) == PROJECTILE_BOUNCESOFFWALLS
                && pSprite->yvel < 1)
            {
                A_DoProjectileEffects(spriteNum, &davect, 1);
                A_DeleteSprite(spriteNum);
                return;
            }

            if (pProj->workslike & PROJECTILE_COOLEXPLOSION1 && ++pSprite->shade >= 40)
            {
                A_DeleteSprite(spriteNum);
                return;
            }

            pSprite->zvel -= pProj->drop;

            if (pProj->workslike & PROJECTILE_SPIT && pSprite->zvel < 6144)
                pSprite->zvel += g_spriteGravity - 112;

            A_GetZLimits(spriteNum);

            if (pProj->trail >= 0)
            {
                for (bssize_t cnt = 0; cnt <= pProj->tnum; cnt++)
                {
                    otherSprite = A_Spawn(spriteNum, pProj->trail);

                    sprite[otherSprite].z += (pProj->toffset << 8);

                    if (pProj->txrepeat >= 0)
                        sprite[otherSprite].xrepeat = pProj->txrepeat;

                    if (pProj->tyrepeat >= 0)
                        sprite[otherSprite].yrepeat = pProj->tyrepeat;
                }
            }

            int projMoveCnt = pProj->movecnt;
            int projVel     = pSprite->xvel;
            int projZvel    = pSprite->zvel;

            if (sector[pSprite->sectnum].lotag == ST_2_UNDERWATER)
            {
                projVel >>= 1;
                projZvel >>= 1;
            }

            uint16_t backupCstat = 0;

            if (!projectileMoved && (unsigned)pSprite->owner < MAXSPRITES)
            {
                backupCstat = sprite[pSprite->owner].cstat;
                sprite[pSprite->owner].cstat &= (uint16_t)~CSTAT_SPRITE_BLOCK_HITSCAN;
            }

            do
            {
                vec3_t tmpvect = { (projVel * (sintable[(pSprite->ang + 512) & 2047])) >> 14,
                                   (projVel * (sintable[pSprite->ang & 2047])) >> 14, projZvel };
                Bmemcpy(&davect, pSprite, sizeof(vec3_t));
                otherSprite = A_MoveSprite(spriteNum, &tmpvect, (A_CheckSpriteFlags(spriteNum, SFLAG_NOCLIP) ? 0 : CLIPMASK1));
            }
            while (!otherSprite && --projMoveCnt > 0);

            if (backupCstat)
                sprite[pSprite->owner].cstat = backupCstat;

            if (!(pProj->workslike & PROJECTILE_BOUNCESOFFWALLS) &&  // NOT_BOUNCESOFFWALLS_YVEL
                (unsigned)pSprite->yvel < MAXSPRITES
                && sprite[pSprite->yvel].sectnum != MAXSECTORS)
                if (FindDistance2D(pSprite->x - sprite[pSprite->yvel].x, pSprite->y - sprite[pSprite->yvel].y) < 256)
                    otherSprite = 49152 | pSprite->yvel;

            actor[spriteNum].movflag = otherSprite;

            if (pSprite->sectnum < 0)
            {
                A_DeleteSprite(spriteNum);
                return;
            }

            if (pProj->workslike & PROJECTILE_TIMED && pProj->range > 0)
            {
                if (++actor[spriteNum].t_data[8] > pProj->range)
                {
                    if (pProj->workslike & PROJECTILE_EXPLODEONTIMER)
                        A_DoProjectileEffects(spriteNum, &davect, 1);

                    A_DeleteSprite(spriteNum);
                    return;
                }
            }

            if ((otherSprite & 49152) != 49152 && !(pProj->workslike & PROJECTILE_BOUNCESOFFWALLS))
                G_WeaponHitCeilingOrFloor(spriteNum, pSprite, &otherSprite);

            if (pProj->workslike & PROJECTILE_WATERBUBBLES && sector[pSprite->sectnum].lotag == ST_2_UNDERWATER && rnd(140))
                A_Spawn(spriteNum, WATERBUBBLE);

            if (otherSprite != 0)
            {
                if (pProj->workslike & PROJECTILE_COOLEXPLOSION1)
                {
                    pSprite->xvel = 0;
                    pSprite->zvel = 0;
                }

                switch (otherSprite & 49152)
                {
                    case 49152:
                        otherSprite &= (MAXSPRITES - 1);

                        if (pProj->workslike & PROJECTILE_BOUNCESOFFSPRITES)
                        {
                            pSprite->yvel--;

                            int const projAngle = getangle(sprite[otherSprite].x - pSprite->x, sprite[otherSprite].y - pSprite->y)
                                                  + (sprite[otherSprite].cstat & 16 ? 0 : 512);
                            pSprite->ang = ((projAngle << 1) - pSprite->ang) & 2047;

                            if (pProj->bsound >= 0)
                                A_PlaySound(pProj->bsound, spriteNum);

                            if (pProj->workslike & PROJECTILE_LOSESVELOCITY)
                                PROJ_DECAYVELOCITY(pSprite);

                            if (!(pProj->workslike & PROJECTILE_FORCEIMPACT))
                                return;
                        }

                        A_DamageObject(otherSprite, spriteNum);

                        if (sprite[otherSprite].picnum == APLAYER)
                        {
                            int playerNum = P_Get(otherSprite);

                            A_PlaySound(PISTOL_BODYHIT, otherSprite);

                            if (pProj->workslike & PROJECTILE_SPIT)
                                P_HandleBeingSpitOn(g_player[playerNum].ps);
                        }

                        if (pProj->workslike & PROJECTILE_RPG_IMPACT)
                        {
                            actor[otherSprite].owner  = pSprite->owner;
                            actor[otherSprite].picnum = pSprite->picnum;
                            actor[otherSprite].extra += pProj->extra;

                            A_DoProjectileEffects(spriteNum, &davect, 0);

                            if (!(pProj->workslike & PROJECTILE_FORCEIMPACT))
                            {
                                A_DeleteSprite(spriteNum);
                                return;
                            }
                        }

                        if (pProj->workslike & PROJECTILE_FORCEIMPACT)
                            return;
                        break;

                    case 32768:
                        otherSprite &= (MAXWALLS - 1);

                        if (pProj->workslike & PROJECTILE_BOUNCESOFFMIRRORS
                            && (wall[otherSprite].overpicnum == MIRROR || wall[otherSprite].picnum == MIRROR))
                        {
                            Proj_BounceOffWall(pSprite, otherSprite);
                            pSprite->owner = spriteNum;
                            A_Spawn(spriteNum, TRANSPORTERSTAR);
                            return;
                        }
                        else
                        {
                            setsprite(spriteNum, &davect);
                            A_DamageWall(spriteNum, otherSprite, (vec3_t *)pSprite, pSprite->picnum);

                            if (pProj->workslike & PROJECTILE_BOUNCESOFFWALLS)
                            {
                                if (wall[otherSprite].overpicnum != MIRROR && wall[otherSprite].picnum != MIRROR)
                                    pSprite->yvel--;

                                Proj_BounceOffWall(pSprite, otherSprite);

                                if (pProj->bsound >= 0)
                                    A_PlaySound(pProj->bsound, spriteNum);

                                if (pProj->workslike & PROJECTILE_LOSESVELOCITY)
                                    PROJ_DECAYVELOCITY(pSprite);

                                return;
                            }
                        }
                        break;

                    case 16384:
                        setsprite(spriteNum, &davect);

                        if (Proj_MaybeDamageCF((uspritetype *)pSprite))
                        {
                            A_DeleteSprite(spriteNum);
                            return;
                        }

                        if (pProj->workslike & PROJECTILE_BOUNCESOFFWALLS)
                        {
                            A_DoProjectileBounce(spriteNum);
                            A_SetSprite(spriteNum, CLIPMASK1);

                            pSprite->yvel--;

                            if (pProj->bsound >= 0)
                                A_PlaySound(pProj->bsound, spriteNum);

                            if (pProj->workslike & PROJECTILE_LOSESVELOCITY)
                                PROJ_DECAYVELOCITY(pSprite);

                            return;
                        }
                        break;
                }

                A_DoProjectileEffects(spriteNum, &davect, 1);
                A_DeleteSprite(spriteNum);
                return;
            }
            return;
        }
    }
}

ACTOR_STATIC void G_MoveWeapons(void)
{
    int spriteNum = headspritestat[STAT_PROJECTILE];

    while (spriteNum >= 0)
    {
        int const         nextSprite = nextspritestat[spriteNum];
        spritetype *const pSprite    = &sprite[spriteNum];
        vec3_t            davect;

        if (pSprite->sectnum < 0)
            DELETE_SPRITE_AND_CONTINUE(spriteNum);

        actor[spriteNum].bpos = *(vec3_t *)pSprite;

        /* Custom projectiles */
        if (A_CheckSpriteFlags(spriteNum, SFLAG_PROJECTILE))
        {
            Proj_MoveCustom(spriteNum);
            goto next_sprite;
        }

        // hard coded projectiles
        switch (DYNAMICTILEMAP(pSprite->picnum))
        {
            case SHOTSPARK1__STATIC:
            {
                if (!G_HaveActor(sprite[spriteNum].picnum))
                    goto next_sprite;
                int32_t   playerDist;
                int const playerNum = A_FindPlayer(pSprite, &playerDist);
                A_Execute(spriteNum, playerNum, playerDist);
                goto next_sprite;
            }

            case RADIUSEXPLOSION__STATIC:
            case KNEE__STATIC: DELETE_SPRITE_AND_CONTINUE(spriteNum);

            case FREEZEBLAST__STATIC:
                if (pSprite->yvel < 1 || pSprite->extra < 2 || (pSprite->xvel | pSprite->zvel) == 0)
                {
                    int const newSprite       = A_Spawn(spriteNum, TRANSPORTERSTAR);
                    sprite[newSprite].pal     = 1;
                    sprite[newSprite].xrepeat = 32;
                    sprite[newSprite].yrepeat = 32;
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);
                }
                fallthrough__;
            case SHRINKSPARK__STATIC:
            case RPG__STATIC:
            case FIRELASER__STATIC:
            case SPIT__STATIC:
            case COOLEXPLOSION1__STATIC:
            {
                if (pSprite->picnum == COOLEXPLOSION1)
                    if (!S_CheckSoundPlaying(spriteNum, WIERDSHOT_FLY))
                        A_PlaySound(WIERDSHOT_FLY, spriteNum);

                int spriteXvel = pSprite->xvel;
                int spriteZvel = pSprite->zvel;

                if (pSprite->picnum == RPG && sector[pSprite->sectnum].lotag == ST_2_UNDERWATER)
                {
                    spriteXvel >>= 1;
                    spriteZvel >>= 1;
                }

                davect = *(vec3_t *) pSprite;

                A_GetZLimits(spriteNum);

                if (pSprite->picnum == RPG && actor[spriteNum].picnum != BOSS2 && pSprite->xrepeat >= 10
                    && sector[pSprite->sectnum].lotag != ST_2_UNDERWATER
                    && g_scriptVersion >= 13)
                {
                    int const newSprite = A_Spawn(spriteNum, SMALLSMOKE);
                    sprite[newSprite].z += (1 << 8);
                }

                vec3_t const tmpvect = { (spriteXvel * (sintable[(pSprite->ang + 512) & 2047])) >> 14,
                                         (spriteXvel * (sintable[pSprite->ang & 2047])) >> 14, spriteZvel };

                int moveSprite = A_MoveSprite(spriteNum, &tmpvect, (A_CheckSpriteFlags(spriteNum, SFLAG_NOCLIP) ? 0 : CLIPMASK1));

                if (pSprite->picnum == RPG && (unsigned) pSprite->yvel < MAXSPRITES)  // RPG_YVEL
                    if (FindDistance2D(pSprite->x - sprite[pSprite->yvel].x, pSprite->y - sprite[pSprite->yvel].y) < 256)
                        moveSprite = 49152 | pSprite->yvel;

                actor[spriteNum].movflag = moveSprite;

                if (pSprite->sectnum < 0)
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);

                if ((moveSprite & 49152) != 49152 && pSprite->picnum != FREEZEBLAST)
                    G_WeaponHitCeilingOrFloor(spriteNum, pSprite, &moveSprite);

                if (pSprite->picnum == FIRELASER)
                {
                    for (bssize_t k = -3; k < 2; k++)
                    {
                        int const newSprite
                            = A_InsertSprite(pSprite->sectnum, pSprite->x + ((k * sintable[(pSprite->ang + 512) & 2047]) >> 9),
                                pSprite->y + ((k * sintable[pSprite->ang & 2047]) >> 9),
                                pSprite->z + ((k * ksgn(pSprite->zvel)) * klabs(pSprite->zvel / 24)), FIRELASER, -40 + (k << 2),
                                pSprite->xrepeat, pSprite->yrepeat, 0, 0, 0, pSprite->owner, 5);

                        sprite[newSprite].cstat = 128;
                        sprite[newSprite].pal   = pSprite->pal;
                    }
                }
                else if (pSprite->picnum == SPIT)
                    if (pSprite->zvel < 6144)
                        pSprite->zvel += g_spriteGravity - 112;

                if (moveSprite != 0)
                {
                    if (pSprite->picnum == COOLEXPLOSION1)
                    {
                        if ((moveSprite & 49152) == 49152 && sprite[moveSprite & (MAXSPRITES - 1)].picnum != APLAYER)
                            goto COOLEXPLOSION;
                        pSprite->xvel = 0;
                        pSprite->zvel = 0;
                    }

                    switch (moveSprite & 49152)
                    {
                        case 49152:
                            moveSprite &= (MAXSPRITES - 1);

                            if (pSprite->picnum == FREEZEBLAST && sprite[moveSprite].pal == 1)
                                if (A_CheckEnemySprite(&sprite[moveSprite]) || sprite[moveSprite].picnum == APLAYER)
                                {
                                    int const newSprite       = A_Spawn(spriteNum, TRANSPORTERSTAR);
                                    sprite[newSprite].pal     = 1;
                                    sprite[newSprite].xrepeat = 32;
                                    sprite[newSprite].yrepeat = 32;

                                    DELETE_SPRITE_AND_CONTINUE(spriteNum);
                                }

                            A_DamageObject(moveSprite, spriteNum);

                            if (sprite[moveSprite].picnum == APLAYER)
                            {
                                int const playerNum = P_Get(moveSprite);
                                A_PlaySound(PISTOL_BODYHIT, moveSprite);

                                if (pSprite->picnum == SPIT)
                                    P_HandleBeingSpitOn(g_player[playerNum].ps);
                            }
                            break;

                        case 32768:
                            moveSprite &= (MAXWALLS - 1);

                            if (pSprite->picnum != RPG && pSprite->picnum != FREEZEBLAST && pSprite->picnum != SPIT
                                && (wall[moveSprite].overpicnum == MIRROR || wall[moveSprite].picnum == MIRROR))
                            {
                                Proj_BounceOffWall(pSprite, moveSprite);
                                pSprite->owner = spriteNum;
                                A_Spawn(spriteNum, TRANSPORTERSTAR);
                                goto next_sprite;
                            }
                            else
                            {
                                setsprite(spriteNum, &davect);
                                A_DamageWall(spriteNum, moveSprite, (vec3_t *)pSprite, pSprite->picnum);

                                if (pSprite->picnum == FREEZEBLAST)
                                {
                                    if (wall[moveSprite].overpicnum != MIRROR && wall[moveSprite].picnum != MIRROR)
                                    {
                                        pSprite->extra >>= 1;
                                        pSprite->yvel--;
                                    }

                                    Proj_BounceOffWall(pSprite, moveSprite);
                                    goto next_sprite;
                                }
                            }
                            break;

                        case 16384:
                            setsprite(spriteNum, &davect);

                            if (Proj_MaybeDamageCF((uspritetype *)pSprite))
                                DELETE_SPRITE_AND_CONTINUE(spriteNum);

                            if (pSprite->picnum == FREEZEBLAST)
                            {
                                A_DoProjectileBounce(spriteNum);
                                A_SetSprite(spriteNum, CLIPMASK1);

                                pSprite->extra >>= 1;
                                pSprite->yvel--;

                                if (pSprite->xrepeat > 8)
                                {
                                    pSprite->xrepeat -= 2;

                                    if (pSprite->yrepeat > 8)
                                        pSprite->yrepeat -= 2;
                                }

                                goto next_sprite;
                            }
                            break;
                        default: break;
                    }

                    switch (DYNAMICTILEMAP(pSprite->picnum))
                    {
                        case SPIT__STATIC:
                        case COOLEXPLOSION1__STATIC:
                        case FREEZEBLAST__STATIC:
                        case FIRELASER__STATIC: break;

                        case RPG__STATIC:
                        {
                            int const newSprite = A_Spawn(spriteNum, EXPLOSION2);
                            A_PlaySound(RPG_EXPLODE, newSprite);
                            Bmemcpy(&sprite[newSprite], &davect, sizeof(vec3_t));

                            if (pSprite->xrepeat < 10)
                            {
                                sprite[newSprite].xrepeat = 6;
                                sprite[newSprite].yrepeat = 6;
                            }
                            else if ((moveSprite & 49152) == 16384)
                            {
                                if (pSprite->zvel > 0)
                                    A_Spawn(spriteNum, EXPLOSION2BOT);
                                else
                                {
                                    sprite[newSprite].cstat |= 8;
                                    sprite[newSprite].z += (48 << 8);
                                }
                            }

                            if (pSprite->xrepeat >= 10)
                            {
                                int const x = pSprite->extra;
                                A_RadiusDamage(spriteNum, g_rpgRadius, x >> 2, x >> 1, x - (x >> 2), x);
                            }
                            else
                            {
                                int const x = pSprite->extra + (g_globalRandom & 3);
                                A_RadiusDamage(spriteNum, (g_rpgRadius >> 1), x >> 2, x >> 1, x - (x >> 2), x);
                            }
                            break;
                        }

                        case SHRINKSPARK__STATIC:
                            A_Spawn(spriteNum, SHRINKEREXPLOSION);
                            A_PlaySound(SHRINKER_HIT, spriteNum);
                            A_RadiusDamage(spriteNum, g_shrinkerRadius, 0, 0, 0, 0);
                            break;

                        default:
                        {
                            int const newSprite       = A_Spawn(spriteNum, EXPLOSION2);
                            sprite[newSprite].xrepeat = sprite[newSprite].yrepeat = pSprite->xrepeat >> 1;
                            if ((moveSprite & 49152) == 16384)
                            {
                                if (pSprite->zvel < 0)
                                {
                                    sprite[newSprite].cstat |= 8;
                                    sprite[newSprite].z += (72 << 8);
                                }
                            }
                            break;
                        }
                    }

                    if (pSprite->picnum != COOLEXPLOSION1)
                        DELETE_SPRITE_AND_CONTINUE(spriteNum);
                }

                if (pSprite->picnum == COOLEXPLOSION1)
                {
                COOLEXPLOSION:
                    pSprite->shade++;
                    if (pSprite->shade >= 40)
                        DELETE_SPRITE_AND_CONTINUE(spriteNum);
                }
                else if (pSprite->picnum == RPG && sector[pSprite->sectnum].lotag == ST_2_UNDERWATER && pSprite->xrepeat >= 10 && rnd(140))
                    A_Spawn(spriteNum, WATERBUBBLE);

                goto next_sprite;
            }
        }
    next_sprite:
        spriteNum = nextSprite;
    }
}


static int P_Submerge(int const spriteNum, int const playerNum, DukePlayer_t * const pPlayer, int const sectNum, int const otherSect)
{
    if (pPlayer->on_ground && pPlayer->pos.z >= sector[sectNum].floorz
        && (TEST_SYNC_KEY(g_player[playerNum].inputBits->bits, SK_CROUCH) || pPlayer->vel.z > 2048))
    //        if( onfloorz && sectlotag == 1 && ps->pos.z > (sector[sect].floorz-(6<<8)) )
    {
        if (screenpeek == playerNum)
        {
            FX_StopAllSounds();
            S_ClearSoundLocks();
        }

        if (sprite[pPlayer->i].extra > 0)
            A_PlaySound(DUKE_UNDERWATER, spriteNum);

        pPlayer->opos.z = pPlayer->pos.z = sector[otherSect].ceilingz;

        if (TEST_SYNC_KEY(g_player[playerNum].inputBits->bits, SK_CROUCH))
            pPlayer->vel.z += 512;

        return 1;
    }

    return 0;
}

static int P_Emerge(int const spriteNum, int const playerNum, DukePlayer_t * const pPlayer, int const sectNum, int const otherSect)
{
    // r1449-:
    if (pPlayer->pos.z < (sector[sectNum].ceilingz+1080) && pPlayer->vel.z == 0)
        // r1450+, breaks submergible slime in bobsp2:
//        if (onfloorz && sectlotag == 2 && ps->pos.z <= sector[sect].ceilingz /*&& ps->vel.z == 0*/)
    {
//        if( sprite[j].extra <= 0) break;
        if (screenpeek == playerNum)
        {
            FX_StopAllSounds();
            S_ClearSoundLocks();
        }

        A_PlaySound(DUKE_GASP, spriteNum);

        pPlayer->opos.z = pPlayer->pos.z = sector[otherSect].floorz;
        pPlayer->vel.z = 0;
//        ps->vel.z += 1024;

        pPlayer->jumping_toggle = 1;
        pPlayer->jumping_counter = 0;

        return 1;
    }

    return 0;
}

static void P_FinishWaterChange(int const playerNum, DukePlayer_t * const pPlayer, int const sectLotag, int const spriteOwner, int const newSector)
{
    pPlayer->bobpos.x = pPlayer->opos.x = pPlayer->pos.x;
    pPlayer->bobpos.y = pPlayer->opos.y = pPlayer->pos.y;

    if (spriteOwner < 0 || sprite[spriteOwner].owner != spriteOwner)
        pPlayer->transporter_hold = -2;

    pPlayer->cursectnum = newSector;
    changespritesect(playerNum, newSector);

    vec3_t vect = pPlayer->pos;
    vect.z += PHEIGHT;
    setsprite(pPlayer->i, &vect);

    P_UpdateScreenPal(pPlayer);

    if ((krand()&255) < 32)
        A_Spawn(playerNum, WATERSPLASH2);

    if (sectLotag == ST_1_ABOVE_WATER)
    {
        for (bssize_t l = 0; l < 9; l++)
            sprite[A_Spawn(pPlayer->i, WATERBUBBLE)].z += krand() & 16383;
    }
}

// Check prevention of teleportation *when alive*. For example, commanders and
// octabrains would be transported by SE7 (both water and normal) only if dead.
static int A_CheckNonTeleporting(int const spriteNum)
{
    int const tileNum = sprite[spriteNum].picnum;
    return !!(A_CheckSpriteFlags(spriteNum, SFLAG_NOTELEPORT) || tileNum == SHARK || tileNum == COMMANDER || tileNum == OCTABRAIN
              || (tileNum >= GREENSLIME && tileNum <= GREENSLIME + 7));
}

ACTOR_STATIC void G_MoveTransports(void)
{
    int spriteNum = headspritestat[STAT_TRANSPORT];

    while (spriteNum >= 0)
    {
        int const nextSprite = nextspritestat[spriteNum];

        if (OW(spriteNum) == spriteNum)
        {
            spriteNum = nextSprite;
            continue;
        }

        int const sectNum    = SECT(spriteNum);
        int const sectLotag  = sector[sectNum].lotag;
        int const onFloor    = T5(spriteNum);  // ONFLOORZ

        if (T1(spriteNum) > 0)
            T1(spriteNum)--;

        int sectSprite = headspritesect[sectNum];
        while (sectSprite >= 0)
        {
            int const nextSectSprite = nextspritesect[sectSprite];

            switch (sprite[sectSprite].statnum)
            {
                case STAT_PLAYER:
                    if (sprite[sectSprite].owner != -1)
                    {
                        int const           playerNum = P_Get(sectSprite);
                        DukePlayer_t *const pPlayer   = g_player[playerNum].ps;

                        pPlayer->on_warping_sector = 1;

                        if (pPlayer->transporter_hold == 0 && pPlayer->jumping_counter == 0)
                        {
                            if (pPlayer->on_ground && sectLotag == 0 && onFloor && pPlayer->jetpack_on == 0)
                            {
                                if (sprite[spriteNum].pal == 0)
                                {
                                    A_Spawn(spriteNum, TRANSPORTERBEAM);
                                    A_PlaySound(TELEPORTER, spriteNum);
                                }

                                for (int TRAVERSE_CONNECT(otherPlayer))
                                {
                                    if (g_player[otherPlayer].ps->cursectnum == sprite[OW(spriteNum)].sectnum)
                                    {
                                        g_player[otherPlayer].ps->frag_ps         = playerNum;
                                        sprite[g_player[otherPlayer].ps->i].extra = 0;
                                    }
                                }

                                pPlayer->ang = sprite[OW(spriteNum)].ang;

                                if (sprite[OW(spriteNum)].owner != OW(spriteNum))
                                {
                                    T1(spriteNum)                  = 13;
                                    actor[OW(spriteNum)].t_data[0] = 13;
                                    pPlayer->transporter_hold      = 13;
                                }

                                pPlayer->pos    = *(vec3_t *)&sprite[OW(spriteNum)];
                                pPlayer->pos.z -= PHEIGHT;
                                pPlayer->opos   = pPlayer->pos;
                                pPlayer->bobpos = *(vec2_t *)&pPlayer->pos;

                                changespritesect(sectSprite, sprite[OW(spriteNum)].sectnum);
                                pPlayer->cursectnum = sprite[sectSprite].sectnum;

                                if (sprite[spriteNum].pal == 0)
                                {
                                    int const newSprite = A_Spawn(OW(spriteNum), TRANSPORTERBEAM);
                                    A_PlaySound(TELEPORTER, newSprite);
                                }

                                break;
                            }

                            if (onFloor == 0 && klabs(SZ(spriteNum) - pPlayer->pos.z) < 6144)
                                if (!pPlayer->jetpack_on || TEST_SYNC_KEY(g_player[playerNum].inputBits->bits, SK_JUMP)
                                    || TEST_SYNC_KEY(g_player[playerNum].inputBits->bits, SK_CROUCH))
                                {
                                    pPlayer->pos.x += sprite[OW(spriteNum)].x - SX(spriteNum);
                                    pPlayer->pos.y += sprite[OW(spriteNum)].y - SY(spriteNum);
                                    pPlayer->pos.z = (pPlayer->jetpack_on && (TEST_SYNC_KEY(g_player[playerNum].inputBits->bits, SK_JUMP)
                                                                              || pPlayer->jetpack_on < 11))
                                                     ? sprite[OW(spriteNum)].z - 6144
                                                     : sprite[OW(spriteNum)].z + 6144;

                                    actor[pPlayer->i].bpos = pPlayer->pos;
                                    pPlayer->opos          = pPlayer->pos;
                                    pPlayer->bobpos        = *(vec2_t *)&pPlayer->pos;

                                    changespritesect(sectSprite, sprite[OW(spriteNum)].sectnum);
                                    pPlayer->cursectnum = sprite[OW(spriteNum)].sectnum;

                                    break;
                                }

                            int doWater = 0;

                            if (onFloor)
                            {
                                if (sectLotag == ST_1_ABOVE_WATER)
                                    doWater = P_Submerge(sectSprite, playerNum, pPlayer, sectNum, sprite[OW(spriteNum)].sectnum);
                                else if (sectLotag == ST_2_UNDERWATER)
                                    doWater = P_Emerge(sectSprite, playerNum, pPlayer, sectNum, sprite[OW(spriteNum)].sectnum);
                            }

                            if (doWater == 1)
                            {
                                pPlayer->pos.x += sprite[OW(spriteNum)].x - SX(spriteNum);
                                pPlayer->pos.y += sprite[OW(spriteNum)].y - SY(spriteNum);

                                P_FinishWaterChange(sectSprite, pPlayer, sectLotag, OW(spriteNum), sprite[OW(spriteNum)].sectnum);
                            }
                        }
                        else if (!(sectLotag == ST_1_ABOVE_WATER && pPlayer->on_ground == 1))
                            break;
                    }
                    break;


                ////////// Non-player teleportation //////////

                case STAT_PROJECTILE:
                // SE7_PROJECTILE, PROJECTILE_CHSECT.
                // comment out to make RPGs pass through water: (r1450 breaks this)
                //                if (sectlotag != 0) goto JBOLT;
                case STAT_ACTOR:
                    if (sprite[sectSprite].extra > 0 && A_CheckNonTeleporting(sectSprite))
                        goto JBOLT;
                    fallthrough__;
                case STAT_MISC:
                case STAT_FALLER:
                case STAT_DUMMYPLAYER:
                {
                    if ((totalclock & UINT8_MAX) != actor[sectSprite].lasttransport)
                    {
                        int const zvel    = sprite[sectSprite].zvel;
                        int const absZvel = klabs(zvel);
                        int       doWarp  = 0;

                        if (absZvel != 0)
                        {
                            if (sectLotag == ST_2_UNDERWATER && sprite[sectSprite].z < (sector[sectNum].ceilingz + absZvel) && zvel < 0)
                                doWarp = 1;
                            if (sectLotag == ST_1_ABOVE_WATER && sprite[sectSprite].z > (sector[sectNum].floorz - absZvel) && zvel > 0)
                                doWarp = 1;
                        }

                        if (sectLotag == 0 && (onFloor || klabs(sprite[sectSprite].z - SZ(spriteNum)) < 4096))
                        {
                            if (sprite[OW(spriteNum)].owner != OW(spriteNum) && onFloor && T1(spriteNum) > 0
                                && sprite[sectSprite].statnum != STAT_MISC)
                            {
                                T1(spriteNum)++;
                                goto next_sprite;
                            }
                            doWarp = 1;
                        }

                        if (doWarp)
                        {
                            if (A_CheckSpriteFlags(sectSprite, SFLAG_DECAL))
                                goto JBOLT;

                            switch (DYNAMICTILEMAP(sprite[sectSprite].picnum))
                            {
                                case TRANSPORTERSTAR__STATIC:
                                case TRANSPORTERBEAM__STATIC:
                                case TRIPBOMB__STATIC:
                                case BULLETHOLE__STATIC:
                                case WATERSPLASH2__STATIC:
                                case BURNING__STATIC:
                                case BURNING2__STATIC:
                                case FIRE__STATIC:
                                case FIRE2__STATIC:
                                case TOILETWATER__STATIC:
                                case LASERLINE__STATIC: goto JBOLT;

                                case PLAYERONWATER__STATIC:
                                    if (sectLotag == ST_2_UNDERWATER)
                                    {
                                        sprite[sectSprite].cstat &= 32768;
                                        break;
                                    }
                                // fall-through
                                default:
                                    if (sprite[sectSprite].statnum == STAT_MISC && !(sectLotag == ST_1_ABOVE_WATER || sectLotag == ST_2_UNDERWATER))
                                        break;
                                // fall-through
                                case WATERBUBBLE__STATIC:
                                    //                            if( rnd(192) && sprite[j].picnum == WATERBUBBLE)
                                    //                                break;

                                    if (sectLotag > 0)
                                    {
                                        // Water SE7 teleportation.
                                        int const osect = sprite[OW(spriteNum)].sectnum;

                                        Bassert(sectLotag == ST_1_ABOVE_WATER || sectLotag == ST_2_UNDERWATER);

                                        int const newSprite = A_Spawn(sectSprite, WATERSPLASH2);

                                        if (sectLotag == ST_1_ABOVE_WATER && sprite[sectSprite].statnum == STAT_PROJECTILE)
                                        {
                                            sprite[newSprite].xvel = sprite[sectSprite].xvel >> 1;
                                            sprite[newSprite].ang  = sprite[sectSprite].ang;
                                            A_SetSprite(newSprite, CLIPMASK0);
                                        }

                                        actor[sectSprite].lasttransport = (totalclock & UINT8_MAX);

                                        sprite[sectSprite].x += sprite[OW(spriteNum)].x - SX(spriteNum);
                                        sprite[sectSprite].y += sprite[OW(spriteNum)].y - SY(spriteNum);
                                        sprite[sectSprite].z = (sectLotag == ST_1_ABOVE_WATER) ? sector[osect].ceilingz : sector[osect].floorz;

                                        actor[sectSprite].bpos = *(vec3_t *)&sprite[sectSprite];

                                        changespritesect(sectSprite, sprite[OW(spriteNum)].sectnum);
                                    }
                                    else if (Bassert(sectLotag == 0), 1)
                                    {
                                        // Non-water SE7 teleportation.

                                        if (onFloor)
                                        {
                                            if (sprite[sectSprite].statnum == STAT_PROJECTILE
                                                || (G_CheckPlayerInSector(sectNum) == -1
                                                    && G_CheckPlayerInSector(sprite[OW(spriteNum)].sectnum) == -1))
                                            {
                                                sprite[sectSprite].x += (sprite[OW(spriteNum)].x - SX(spriteNum));
                                                sprite[sectSprite].y += (sprite[OW(spriteNum)].y - SY(spriteNum));
                                                sprite[sectSprite].z -= SZ(spriteNum) - sector[sprite[OW(spriteNum)].sectnum].floorz;

                                                sprite[sectSprite].ang = sprite[OW(spriteNum)].ang;
                                                actor[sectSprite].bpos = *(vec3_t *)&sprite[sectSprite];

                                                if (sprite[spriteNum].pal == 0)
                                                {
                                                    int newSprite = A_Spawn(spriteNum, TRANSPORTERBEAM);
                                                    A_PlaySound(TELEPORTER, newSprite);

                                                    newSprite = A_Spawn(OW(spriteNum), TRANSPORTERBEAM);
                                                    A_PlaySound(TELEPORTER, newSprite);
                                                }

                                                if (sprite[OW(spriteNum)].owner != OW(spriteNum))
                                                {
                                                    T1(spriteNum)                  = 13;
                                                    actor[OW(spriteNum)].t_data[0] = 13;
                                                }

                                                changespritesect(sectSprite, sprite[OW(spriteNum)].sectnum);
                                            }
                                        }
                                        else
                                        {
                                            sprite[sectSprite].x += (sprite[OW(spriteNum)].x - SX(spriteNum));
                                            sprite[sectSprite].y += (sprite[OW(spriteNum)].y - SY(spriteNum));
                                            sprite[sectSprite].z = sprite[OW(spriteNum)].z + 4096;

                                            actor[sectSprite].bpos = *(vec3_t *)&sprite[sectSprite];

                                            changespritesect(sectSprite, sprite[OW(spriteNum)].sectnum);
                                        }
                                    }

                                    break;
                            }  // switch (DYNAMICTILEMAP(sprite[j].picnum))
                        }      // if (doWarp)
                    }          // if (totalclock > actor[j].lasttransport)

                    break;
                }  // five cases

            }  // switch (sprite[j].statnum)
        JBOLT:
            sectSprite = nextSectSprite;
        }
    next_sprite:
        spriteNum = nextSprite;
    }
}

static int A_FindLocator(int const tag, int const sectNum)
{
    for (bssize_t SPRITES_OF(STAT_LOCATOR, spriteNum))
    {
        if ((sectNum == -1 || sectNum == SECT(spriteNum)) && tag == SLT(spriteNum))
            return spriteNum;
    }

    return -1;
}

ACTOR_STATIC void G_MoveActors(void)
{
    int spriteNum = headspritestat[STAT_ACTOR];

    while (spriteNum >= 0)
    {
        int const         nextSprite = nextspritestat[spriteNum];
        spritetype *const pSprite    = &sprite[spriteNum];
        int const         sectNum    = pSprite->sectnum;
        int32_t *const    pData      = actor[spriteNum].t_data;

        int switchPic;

        if (pSprite->xrepeat == 0 || sectNum < 0 || sectNum >= MAXSECTORS)
            DELETE_SPRITE_AND_CONTINUE(spriteNum);

        Bmemcpy(&actor[spriteNum].bpos, pSprite, sizeof(vec3_t));

        switchPic = pSprite->picnum;

        if (pSprite->picnum > GREENSLIME && pSprite->picnum <= GREENSLIME+7)
            switchPic = GREENSLIME;

        switch (DYNAMICTILEMAP(switchPic))
        {
        case DUCK__STATIC:
        case TARGET__STATIC:
            if (pSprite->cstat&32)
            {
                pData[0]++;
                if (pData[0] > 60)
                {
                    pData[0] = 0;
                    pSprite->cstat = 128+257+16;
                    pSprite->extra = 1;
                }
            }
            else
            {
                if (A_IncurDamage(spriteNum) >= 0)
                {
                    int doEffects = 1;

                    pSprite->cstat = 32+128;

                    for (bssize_t SPRITES_OF(STAT_ACTOR, actorNum))
                    {
                        if ((sprite[actorNum].lotag == pSprite->lotag && sprite[actorNum].picnum == pSprite->picnum)
                            && ((sprite[actorNum].hitag != 0) ^ ((sprite[actorNum].cstat & 32) != 0)))
                        {
                            doEffects = 0;
                            break;
                        }
                    }

                    if (doEffects == 1)
                    {
                        G_OperateActivators(pSprite->lotag, -1);
                        G_OperateForceFields(spriteNum, pSprite->lotag);
                        G_OperateMasterSwitches(pSprite->lotag);
                    }
                }
            }
            goto next_sprite;

        case RESPAWNMARKERRED__STATIC:
        case RESPAWNMARKERYELLOW__STATIC:
        case RESPAWNMARKERGREEN__STATIC:
            if (++T1(spriteNum) > g_itemRespawnTime)
                DELETE_SPRITE_AND_CONTINUE(spriteNum);

            if (T1(spriteNum) >= (g_itemRespawnTime>>1) && T1(spriteNum) < ((g_itemRespawnTime>>1)+(g_itemRespawnTime>>2)))
                PN(spriteNum) = RESPAWNMARKERYELLOW;
            else if (T1(spriteNum) > ((g_itemRespawnTime>>1)+(g_itemRespawnTime>>2)))
                PN(spriteNum) = RESPAWNMARKERGREEN;

            A_Fall(spriteNum);
            break;

        case HELECOPT__STATIC:
        case DUKECAR__STATIC:
            pSprite->z += pSprite->zvel;
            pData[0]++;

            if (pData[0] == 4)
                A_PlaySound(WAR_AMBIENCE2,spriteNum);

            if (pData[0] > (GAMETICSPERSEC*8))
            {
                g_earthquakeTime = 16;
                S_PlaySound(RPG_EXPLODE);

                for (bssize_t j  = 0; j < 32; j++)
                    RANDOMSCRAP(pSprite, spriteNum);

                DELETE_SPRITE_AND_CONTINUE(spriteNum);
            }
            else if ((pData[0]&3) == 0)
                A_Spawn(spriteNum,EXPLOSION2);

            A_SetSprite(spriteNum,CLIPMASK0);
            break;

        case RAT__STATIC:
            A_Fall(spriteNum);
            if (A_SetSprite(spriteNum, CLIPMASK0))
            {
                if ((krand()&255) < 3) A_PlaySound(RATTY,spriteNum);
                pSprite->ang += (krand()&31)-15+(sintable[(pData[0]<<8)&2047]>>11);
            }
            else
            {
                T1(spriteNum)++;
                if (T1(spriteNum) > 1)
                {
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);
                }
                else pSprite->ang = (krand()&2047);
            }
            if (pSprite->xvel < 128)
                pSprite->xvel+=2;
            pSprite->ang += (krand()&3)-6;
            break;

        case QUEBALL__STATIC:
        case STRIPEBALL__STATIC:
            if (pSprite->xvel)
            {
                for (bssize_t SPRITES_OF(STAT_DEFAULT, hitObject))
                    if (sprite[hitObject].picnum == POCKET && ldist(&sprite[hitObject],pSprite) < 52)
                        DELETE_SPRITE_AND_CONTINUE(spriteNum);

                int hitObject = clipmove((vec3_t *)pSprite, &pSprite->sectnum,
                                         (((pSprite->xvel * (sintable[(pSprite->ang + 512) & 2047])) >> 14) * TICSPERFRAME) << 11,
                                         (((pSprite->xvel * (sintable[pSprite->ang & 2047])) >> 14) * TICSPERFRAME) << 11, 24L, ZOFFSET6,
                                         ZOFFSET6, CLIPMASK1);

                if (hitObject & 49152)
                {
                    if ((hitObject & 49152) == 32768)
                    {
                        hitObject &= (MAXWALLS - 1);
                        Proj_BounceOffWall(pSprite, hitObject);
                    }
                    else if ((hitObject & 49152) == 49152)
                    {
                        hitObject &= (MAXSPRITES - 1);
                        A_DamageObject(spriteNum, hitObject);
                    }
                }

                if (--pSprite->xvel < 0)
                    pSprite->xvel = 0;

                if (pSprite->picnum == STRIPEBALL)
                {
                    pSprite->cstat = 257;
                    pSprite->cstat |= (4 & pSprite->xvel) | (8 & pSprite->xvel);
                }
            }
            else
            {
                int32_t playerDist;
                int const playerNum = A_FindPlayer(pSprite,&playerDist);
                DukePlayer_t *const pPlayer = g_player[playerNum].ps;

                // I'm 50/50 on this being either a typo or a stupid hack
                if (playerDist < 1596)
                {
                    int const angDiff = G_GetAngleDelta(pPlayer->ang,getangle(pSprite->x-pPlayer->pos.x,pSprite->y-pPlayer->pos.y));

                    if (angDiff > -64 && angDiff < 64 && TEST_SYNC_KEY(g_player[playerNum].inputBits->bits, SK_OPEN)
                        && pPlayer->toggle_key_flag == 1)
                    {
                        int ballSprite;

                        for (SPRITES_OF(STAT_ACTOR, ballSprite))
                        {
                            if (sprite[ballSprite].picnum == QUEBALL || sprite[ballSprite].picnum == STRIPEBALL)
                            {
                                int const angDiff2 = G_GetAngleDelta(
                                pPlayer->ang, getangle(sprite[ballSprite].x - pPlayer->pos.x, sprite[ballSprite].y - pPlayer->pos.y));

                                if (angDiff2 > -64 && angDiff2 < 64)
                                {
                                    int32_t ballDist;
                                    A_FindPlayer(&sprite[ballSprite], &ballDist);

                                    if (playerDist > ballDist)
                                        break;
                                }
                            }
                        }

                        if (ballSprite == -1)
                        {
                            pSprite->xvel = (pSprite->pal == 12) ? 164 : 140;
                            pSprite->ang  = pPlayer->ang;

                            pPlayer->toggle_key_flag = 2;
                        }
                    }
                }

                if (playerDist < 512 && pSprite->sectnum == pPlayer->cursectnum)
                {
                    pSprite->ang = getangle(pSprite->x-pPlayer->pos.x,pSprite->y-pPlayer->pos.y);
                    pSprite->xvel = 48;
                }
            }

            break;

        case FORCESPHERE__STATIC:
            if (pSprite->yvel == 0)
            {
                pSprite->yvel = 1;

                for (bssize_t l = 512; l < (2048 - 512); l += 128)
                {
                    for (bssize_t j = 0; j < 2048; j += 128)
                    {
                        int const newSprite        = A_Spawn(spriteNum, FORCESPHERE);
                        sprite[newSprite].cstat    = 257 + 128;
                        sprite[newSprite].clipdist = 64;
                        sprite[newSprite].ang      = j;
                        sprite[newSprite].zvel     = sintable[l & 2047] >> 5;
                        sprite[newSprite].xvel     = sintable[(l + 512) & 2047] >> 9;
                        sprite[newSprite].owner    = spriteNum;
                    }
                }
            }

            if (pData[3] > 0)
            {
                if (pSprite->zvel < 6144)
                    pSprite->zvel += 192;

                pSprite->z += pSprite->zvel;

                if (pSprite->z > sector[sectNum].floorz)
                    pSprite->z = sector[sectNum].floorz;

                if (--pData[3] == 0)
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);
            }
            else if (pData[2] > 10)
            {
                for (bssize_t SPRITES_OF(STAT_MISC, miscSprite))
                {
                    if (sprite[miscSprite].owner == spriteNum && sprite[miscSprite].picnum == FORCESPHERE)
                        actor[miscSprite].t_data[1] = 1 + (krand() & 63);
                }

                pData[3] = 64;
            }

            goto next_sprite;

        case RECON__STATIC:
        {
            int playerNum;
            DukePlayer_t *pPlayer;

            A_GetZLimits(spriteNum);

            pSprite->shade += (sector[pSprite->sectnum].ceilingstat & 1) ? (sector[pSprite->sectnum].ceilingshade - pSprite->shade) >> 1
                                                                         : (sector[pSprite->sectnum].floorshade - pSprite->shade) >> 1;

            if (pSprite->z < sector[sectNum].ceilingz + ZOFFSET5)
                pSprite->z = sector[sectNum].ceilingz + ZOFFSET5;

#if 0 //def POLYMER
            gamelights[gamelightcount&(PR_MAXLIGHTS-1)].sector = s->sectnum;
            gamelights[gamelightcount&(PR_MAXLIGHTS-1)].x = s->x;
            gamelights[gamelightcount&(PR_MAXLIGHTS-1)].y = s->y;
            gamelights[gamelightcount&(PR_MAXLIGHTS-1)].z = s->z + 10248;
            gamelights[gamelightcount&(PR_MAXLIGHTS-1)].range = 8192;

            gamelights[gamelightcount&(PR_MAXLIGHTS-1)].angle = s->ang;
            gamelights[gamelightcount&(PR_MAXLIGHTS-1)].horiz = 100;
            gamelights[gamelightcount&(PR_MAXLIGHTS-1)].radius = 256;
            gamelights[gamelightcount&(PR_MAXLIGHTS-1)].faderadius = 200;

            gamelights[gamelightcount&(PR_MAXLIGHTS-1)].color[0] = 255;
            gamelights[gamelightcount&(PR_MAXLIGHTS-1)].color[1] = 255;
            gamelights[gamelightcount&(PR_MAXLIGHTS-1)].color[2] = 255;

            gamelights[gamelightcount&(PR_MAXLIGHTS-1)].priority = PR_LIGHT_PRIO_MAX_GAME;

            if (gamelightcount < PR_MAXLIGHTS)
                gamelightcount++;
#endif

            if (!g_netServer && ud.multimode < 2)
            {
                if (g_noEnemies == 1)
                {
                    pSprite->cstat = 32768;
                    goto next_sprite;
                }
                else if (g_noEnemies == 2) pSprite->cstat = 257;
            }
            if (A_IncurDamage(spriteNum) >= 0)
            {
                if (pSprite->extra < 0 && pData[0] != -1)
                {
                    pData[0] = -1;
                    pSprite->extra = 0;
                }

                A_PlaySound(RECO_PAIN,spriteNum);
                RANDOMSCRAP(pSprite, spriteNum);
            }

            if (pData[0] == -1)
            {
                pSprite->z += 1024;
                pData[2]++;

                if ((pData[2]&3) == 0)
                    A_Spawn(spriteNum,EXPLOSION2);

                A_GetZLimits(spriteNum);
                pSprite->ang += 96;
                pSprite->xvel = 128;

                if (!A_SetSprite(spriteNum, CLIPMASK0) || pSprite->z > actor[spriteNum].floorz)
                {
                    for (bssize_t l = 0; l < 16; l++)
                        RANDOMSCRAP(pSprite, spriteNum);

                    int const newSprite = A_Spawn(spriteNum, EXPLOSION2);
                    A_PlaySound(LASERTRIP_EXPLODE, newSprite);
                    A_Spawn(spriteNum, PIGCOP);
                    P_AddKills(g_player[myconnectindex].ps, 1);
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);
                }

                goto next_sprite;
            }
            else
            {
                if (pSprite->z > actor[spriteNum].floorz-(48<<8))
                    pSprite->z = actor[spriteNum].floorz-(48<<8);
            }

            int32_t playerDist;
            playerNum = A_FindPlayer(pSprite, &playerDist);
            pPlayer   = g_player[playerNum].ps;

            int const spriteOwner = pSprite->owner;

            // 3 = findplayerz, 4 = shoot

            if (pData[0] >= 4)
            {
                if ((++pData[2] & 15) == 0)
                {
                    int const saveAng = pSprite->ang;
                    pSprite->ang      = actor[spriteNum].tempang;
                    A_PlaySound(RECO_ATTACK, spriteNum);
                    A_Shoot(spriteNum, FIRELASER);
                    pSprite->ang      = saveAng;
                }
                if (pData[2] > (GAMETICSPERSEC * 3)
                    || !cansee(pSprite->x, pSprite->y, pSprite->z - ZOFFSET2, pSprite->sectnum, pPlayer->pos.x, pPlayer->pos.y,
                               pPlayer->pos.z, pPlayer->cursectnum))
                {
                    pData[0] = 0;
                    pData[2] = 0;
                }
                else actor[spriteNum].tempang += G_GetAngleDelta(actor[spriteNum].tempang,
                                                                 getangle(pPlayer->pos.x - pSprite->x,
                                                                          pPlayer->pos.y - pSprite->y)) / 3;
            }
            else if (pData[0] == 2 || pData[0] == 3)
            {
                pData[3]      = 0;
                pSprite->xvel = (pSprite->xvel > 0) ? pSprite->xvel - 16 : 0;

                if (pData[0] == 2)
                {
                    int const zDiff = pPlayer->pos.z - pSprite->z;

                    if (klabs(zDiff) < (48 << 8))
                        pData[0] = 3;
                    else
                        pSprite->z += ksgn(pPlayer->pos.z - pSprite->z) << 10;
                }
                else
                {
                    pData[2]++;
                    if (pData[2] > (GAMETICSPERSEC*3) ||
                        !cansee(pSprite->x,pSprite->y,pSprite->z-ZOFFSET2,pSprite->sectnum, pPlayer->pos.x,pPlayer->pos.y,pPlayer->pos.z,pPlayer->cursectnum))
                    {
                        pData[0] = 1;
                        pData[2] = 0;
                    }
                    else if ((pData[2]&15) == 0)
                    {
                        A_PlaySound(RECO_ATTACK,spriteNum);
                        A_Shoot(spriteNum,FIRELASER);
                    }
                }
                pSprite->ang += G_GetAngleDelta(pSprite->ang, getangle(pPlayer->pos.x - pSprite->x, pPlayer->pos.y - pSprite->y)) >> 2;
            }

            if (pData[0] != 2 && pData[0] != 3)
            {
                int newAngle;
                int locatorDist = ldist(&sprite[spriteOwner], pSprite);
                if (locatorDist <= 1524)
                {
                    newAngle = pSprite->ang;
                    pSprite->xvel >>= 1;
                }
                else newAngle = getangle(sprite[spriteOwner].x - pSprite->x, sprite[spriteOwner].y - pSprite->y);

                if (pData[0] == 1 || pData[0] == 4) // Found a locator and going with it
                {
                    locatorDist = dist(&sprite[spriteOwner], pSprite);

                    if (locatorDist <= 1524)
                    {
                        pData[0] = (pData[0] == 1) ? 0 : 5;
                    }
                    else
                    {
                        // Control speed here
                        if (locatorDist > 1524)
                        {
                            if (pSprite->xvel < 256) pSprite->xvel += 32;
                        }
                        else
                        {
                            pSprite->xvel = (pSprite->xvel > 0) ? pSprite->xvel - 16 : 0;
                        }
                    }

                    if (pData[0] < 2) pData[2]++;

                    if (playerDist < 6144 && pData[0] < 2 && pData[2] > (GAMETICSPERSEC*4))
                    {
                        pData[0] = 2+(krand()&2);
                        pData[2] = 0;
                        actor[spriteNum].tempang = pSprite->ang;
                    }
                }

                int locatorSprite = pSprite->owner;

                if (pData[0] == 0 || pData[0] == 5)
                {
                    pData[0]       = (pData[0] == 0) ? 1 : 4;
                    pSprite->owner = A_FindLocator(pSprite->hitag, -1);
                    locatorSprite  = pSprite->owner;

                    if (locatorSprite == -1)
                    {
                        locatorSprite  = actor[spriteNum].t_data[5];
                        pSprite->hitag = locatorSprite;
                        pSprite->owner = A_FindLocator(locatorSprite, -1);
                        locatorSprite  = pSprite->owner;

                        if (locatorSprite == -1)
                            DELETE_SPRITE_AND_CONTINUE(spriteNum);
                    }
                    else pSprite->hitag++;
                }

                // RECON_T4
                pData[3] = G_GetAngleDelta(pSprite->ang,newAngle);
                pSprite->ang += pData[3]>>3;

                if (pSprite->z < sprite[locatorSprite].z - 512)
                    pSprite->z += 512;
                else if (pSprite->z > sprite[locatorSprite].z + 512)
                    pSprite->z -= 512;
                else
                    pSprite->z = sprite[locatorSprite].z;
            }

            if (!A_CheckSoundPlaying(spriteNum,RECO_ROAM))
                A_PlaySound(RECO_ROAM,spriteNum);

            A_SetSprite(spriteNum,CLIPMASK0);

            goto next_sprite;
        }

        case OOZ__STATIC:
        case OOZ2__STATIC:
        {
            A_GetZLimits(spriteNum);

            int const yrepeat = clamp((actor[spriteNum].floorz - actor[spriteNum].ceilingz) >> 9, 8, 255);
            int const xrepeat = clamp(25 - (yrepeat >> 1), 8, 48);

            pSprite->yrepeat = yrepeat;
            pSprite->xrepeat = xrepeat;
            pSprite->z       = actor[spriteNum].floorz;

            goto next_sprite;
        }

        case GREENSLIME__STATIC:
        {
            // #ifndef VOLUMEONE
            if (!g_netServer && ud.multimode < 2)
            {
                if (g_noEnemies == 1)
                {
                    pSprite->cstat = 32768;
                    goto next_sprite;
                }
                else if (g_noEnemies == 2) pSprite->cstat = 257;
            }
            // #endif

            pData[1]+=128;

            if (sector[sectNum].floorstat&1)
                DELETE_SPRITE_AND_CONTINUE(spriteNum);

            int32_t             playerDist;
            int const           playerNum = A_FindPlayer(pSprite, &playerDist);
            DukePlayer_t *const pPlayer   = g_player[playerNum].ps;

            if (playerDist > 20480)
            {
                if (++actor[spriteNum].timetosleep > SLEEPTIME)
                {
                    actor[spriteNum].timetosleep = 0;
                    changespritestat(spriteNum, STAT_ZOMBIEACTOR);
                    goto next_sprite;
                }
            }

            if (pData[0] == -5) // FROZEN
            {
                pData[3]++;
                if (pData[3] > 280)
                {
                    pSprite->pal = 0;
                    pData[0] = 0;
                    goto next_sprite;
                }
                A_Fall(spriteNum);

                pSprite->cstat  = 257;
                pSprite->picnum = GREENSLIME + 2;
                pSprite->extra  = 1;
                pSprite->pal    = 1;

                int const damageTile = A_IncurDamage(spriteNum);
                if (damageTile >= 0)
                {
                    if (damageTile == FREEZEBLAST)
                        goto next_sprite;

                    P_AddKills(pPlayer, 1);

                    for (bssize_t j = 16; j >= 0; --j)
                    {
                        int32_t newSprite = A_InsertSprite(SECT(spriteNum), SX(spriteNum), SY(spriteNum), SZ(spriteNum),
                                                           GLASSPIECES + (j % 3), -32, 36, 36, krand() & 2047, 32 + (krand() & 63),
                                                           1024 - (krand() & 1023), spriteNum, 5);
                        sprite[newSprite].pal = 1;
                    }

                    A_PlaySound(GLASS_BREAKING, spriteNum);
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);
                }
                else if (playerDist < 1024 && pPlayer->quick_kick == 0)
                {
                    int const angDiff = G_GetAngleDelta(pPlayer->ang, getangle(SX(spriteNum) - pPlayer->pos.x,
                                                                               SY(spriteNum) - pPlayer->pos.y));

                    if (angDiff > -128 && angDiff < 128)
                        pPlayer->quick_kick = 14;
                }

                goto next_sprite;
            }

            pSprite->cstat = (playerDist < 1596) ? 0 : 257;

            if (pData[0] == -4) //On the player
            {
                if (sprite[pPlayer->i].extra < 1)
                {
                    pData[0] = 0;
                    goto next_sprite;
                }

                setsprite(spriteNum,(vec3_t *)pSprite);

                pSprite->ang = pPlayer->ang;

                if ((TEST_SYNC_KEY(g_player[playerNum].inputBits->bits, SK_FIRE) || (pPlayer->quick_kick > 0)) && sprite[pPlayer->i].extra > 0)
                    if (pPlayer->quick_kick > 0 ||
                        (PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) != HANDREMOTE_WEAPON && PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) != HANDBOMB_WEAPON &&
                        PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) != TRIPBOMB_WEAPON && pPlayer->ammo_amount[pPlayer->curr_weapon] >= 0))
                    {
                        for (bssize_t x = 0; x < 8; ++x)
                        {
                            int const j
                            = A_InsertSprite(sectNum, pSprite->x, pSprite->y, pSprite->z - ZOFFSET3, SCRAP3 + (krand() & 3), -8, 48, 48,
                                             krand() & 2047, (krand() & 63) + 64, -(krand() & 4095) - (pSprite->zvel >> 2), spriteNum, 5);
                            sprite[j].pal = 6;
                        }

                        A_PlaySound(SLIM_DYING,spriteNum);
                        A_PlaySound(SQUISHED,spriteNum);

                        if ((krand()&255) < 32)
                        {
                            int const j = A_Spawn(spriteNum,BLOODPOOL);
                            sprite[j].pal = 0;
                        }

                        P_AddKills(pPlayer, 1);
                        pData[0] = -3;

                        if (pPlayer->somethingonplayer == spriteNum)
                            pPlayer->somethingonplayer = -1;

                        DELETE_SPRITE_AND_CONTINUE(spriteNum);
                    }

                pSprite->z = pPlayer->pos.z + pPlayer->pyoff - pData[2] + ZOFFSET3 + ((100 - pPlayer->horiz) << 4);

                if (pData[2] > 512)
                    pData[2] -= 128;

                if (pData[2] < 348)
                    pData[2] += 128;

                if (pPlayer->newowner >= 0)
                    G_ClearCameraView(pPlayer);

                if (pData[3] > 0)
                {
                    static const char slimeFrames[] = { 5, 5, 6, 6, 7, 7, 6, 5 };

                    pSprite->picnum = GREENSLIME + slimeFrames[pData[3]];

                    if (pData[3] == 5)
                    {
                        sprite[pPlayer->i].extra += -(5 + (krand() & 3));
                        A_PlaySound(SLIM_ATTACK, spriteNum);
                    }

                    if (pData[3] < 7)
                        pData[3]++;
                    else
                        pData[3] = 0;
                }
                else
                {
                    pSprite->picnum = GREENSLIME + 5;
                    if (rnd(32))
                        pData[3] = 1;
                }

                pSprite->xrepeat = 20 + (sintable[pData[1] & 2047] >> 13);
                pSprite->yrepeat = 15 + (sintable[pData[1] & 2047] >> 13);
                pSprite->x       = pPlayer->pos.x + (sintable[(pPlayer->ang + 512) & 2047] >> 7);
                pSprite->y       = pPlayer->pos.y + (sintable[pPlayer->ang & 2047] >> 7);

                goto next_sprite;
            }

            else if (pSprite->xvel < 64 && playerDist < 768)
            {
                if (pPlayer->somethingonplayer == -1)
                {
                    pPlayer->somethingonplayer = spriteNum;
                    if (pData[0] == 3 || pData[0] == 2)  // Falling downward
                        pData[2] = (12 << 8);
                    else
                        pData[2] = -(13 << 8);  // Climbing up player
                    pData[0]     = -4;
                }
            }

            int const damageTile = A_IncurDamage(spriteNum);
            if (damageTile >= 0)
            {
                A_PlaySound(SLIM_DYING,spriteNum);

                if (pPlayer->somethingonplayer == spriteNum)
                    pPlayer->somethingonplayer = -1;

                if (damageTile == FREEZEBLAST)
                {
                    A_PlaySound(SOMETHINGFROZE, spriteNum);
                    pData[0] = -5;
                    pData[3] = 0;
                    goto next_sprite;
                }

                P_AddKills(pPlayer, 1);

                if ((krand()&255) < 32)
                {
                    int const j = A_Spawn(spriteNum,BLOODPOOL);
                    sprite[j].pal = 0;
                }

                for (bssize_t x=0; x<8; x++)
                {
                    int const j = A_InsertSprite(sectNum, pSprite->x, pSprite->y, pSprite->z - ZOFFSET3, SCRAP3 + (krand() & 3), -8,
                                                 48, 48, krand() & 2047, (krand() & 63) + 64, -(krand() & 4095) - (pSprite->zvel >> 2),
                                                 spriteNum, 5);
                    sprite[j].pal = 6;
                }
                pData[0] = -3;
                DELETE_SPRITE_AND_CONTINUE(spriteNum);
            }
            // All weap
            if (pData[0] == -1) //Shrinking down
            {
                A_Fall(spriteNum);

                pSprite->cstat &= 65535-8;
                pSprite->picnum = GREENSLIME+4;

                //                    if(s->yrepeat > 62)
                //                      A_DoGuts(s,JIBS6,5,myconnectindex);

                if (pSprite->xrepeat > 32) pSprite->xrepeat -= krand()&7;
                if (pSprite->yrepeat > 16) pSprite->yrepeat -= krand()&7;
                else
                {
                    pSprite->xrepeat = 40;
                    pSprite->yrepeat = 16;
                    pData[5] = -1;
                    pData[0] = 0;
                }

                goto next_sprite;
            }
            else if (pData[0] != -2) A_GetZLimits(spriteNum);

            if (pData[0] == -2) //On top of somebody
            {
                A_Fall(spriteNum);
                sprite[pData[5]].xvel = 0;

                int const ang = sprite[pData[5]].ang;
                pSprite->x    = sprite[pData[5]].x + (sintable[(ang + 512) & 2047] >> 11);
                pSprite->y    = sprite[pData[5]].y + (sintable[ang & 2047] >> 11);
                pSprite->z    = sprite[pData[5]].z;

                pSprite->picnum = GREENSLIME + 2 + (g_globalRandom & 1);

                if (pSprite->yrepeat < 64)
                    pSprite->yrepeat += 2;
                else
                {
                    if (pSprite->xrepeat < 32)
                        pSprite->xrepeat += 4;
                    else
                    {
                        pData[0]   = -1;
                        playerDist = ldist(pSprite, &sprite[pData[5]]);

                        if (playerDist < 768)
                        {
                            sprite[pData[5]].xrepeat = 0;

                            // JBF 20041129: a slimer eating another enemy really ought
                            // to decrease the maximum kill count by one.
                            if (sprite[pData[5]].extra > 0)
                                g_player[myconnectindex].ps->max_actors_killed--;
                        }
                    }
                }

                goto next_sprite;
            }

            //Check randomly to see of there is an actor near
            if (rnd(32))
            {
                for (bssize_t SPRITES_OF_SECT(sectNum, j))
                {
                    if (A_CheckSpriteFlags(j, SFLAG_GREENSLIMEFOOD))
                    {
                        if (ldist(pSprite, &sprite[j]) < 768 && (klabs(pSprite->z - sprite[j].z) < 8192))  // Gulp them
                        {
                            pData[5] = j;
                            pData[0] = -2;
                            pData[1] = 0;
                            goto next_sprite;
                        }
                    }
                }
            }

            //Moving on the ground or ceiling

            if (pData[0] == 0 || pData[0] == 2)
            {
                pSprite->picnum = GREENSLIME;

                if ((krand()&511) == 0)
                    A_PlaySound(SLIM_ROAM,spriteNum);

                if (pData[0]==2)
                {
                    pSprite->zvel = 0;
                    pSprite->cstat &= (65535-8);

                    if ((sector[sectNum].ceilingstat&1) || (actor[spriteNum].ceilingz+6144) < pSprite->z)
                    {
                        pSprite->z += 2048;
                        pData[0] = 3;
                        goto next_sprite;
                    }
                }
                else
                {
                    pSprite->cstat |= 8;
                    A_Fall(spriteNum);
                }

                if (everyothertime&1) A_SetSprite(spriteNum,CLIPMASK0);

                if (pSprite->xvel > 96)
                {
                    pSprite->xvel -= 2;
                    goto next_sprite;
                }
                else
                {
                    if (pSprite->xvel < 32) pSprite->xvel += 4;
                    pSprite->xvel = 64 - (sintable[(pData[1]+512)&2047]>>9);

                    pSprite->ang += G_GetAngleDelta(pSprite->ang,
                                              getangle(pPlayer->pos.x-pSprite->x,pPlayer->pos.y-pSprite->y))>>3;
                    // TJR
                }

                pSprite->xrepeat = 36 + (sintable[(pData[1]+512)&2047]>>11);
                pSprite->yrepeat = 16 + (sintable[pData[1]&2047]>>13);

                if (rnd(4) && (sector[sectNum].ceilingstat&1) == 0 &&
                        klabs(actor[spriteNum].floorz-actor[spriteNum].ceilingz)
                        < (192<<8))
                {
                    pSprite->zvel = 0;
                    pData[0]++;
                }

            }

            if (pData[0]==1)
            {
                pSprite->picnum = GREENSLIME;
                if (pSprite->yrepeat < 40) pSprite->yrepeat+=8;
                if (pSprite->xrepeat > 8) pSprite->xrepeat-=4;
                if (pSprite->zvel > -(2048+1024))
                    pSprite->zvel -= 348;
                pSprite->z += pSprite->zvel;
                if (pSprite->z < actor[spriteNum].ceilingz+4096)
                {
                    pSprite->z = actor[spriteNum].ceilingz+4096;
                    pSprite->xvel = 0;
                    pData[0] = 2;
                }
            }

            if (pData[0]==3)
            {
                pSprite->picnum = GREENSLIME+1;

                A_Fall(spriteNum);

                if (pSprite->z > actor[spriteNum].floorz-ZOFFSET3)
                {
                    pSprite->yrepeat-=4;
                    pSprite->xrepeat+=2;
                }
                else
                {
                    if (pSprite->yrepeat < (40-4)) pSprite->yrepeat+=8;
                    if (pSprite->xrepeat > 8) pSprite->xrepeat-=4;
                }

                if (pSprite->z > actor[spriteNum].floorz-2048)
                {
                    pSprite->z = actor[spriteNum].floorz-2048;
                    pData[0] = 0;
                    pSprite->xvel = 0;
                }
            }
            goto next_sprite;
        }

        case BOUNCEMINE__STATIC:
        if (pSprite->xvel != 0)
        case MORTER__STATIC:
        {
            int const j        = A_Spawn(spriteNum, (PLUTOPAK ? FRAMEEFFECT1 : FRAMEEFFECT1_13));
            actor[j].t_data[0] = 3;
        }
        /* fall-through */
        case HEAVYHBOMB__STATIC:
        {
            int           playerNum;
            DukePlayer_t *pPlayer;
            int           detonatePlayer;

            if ((pSprite->cstat&32768))
            {
                if (--pData[2] <= 0)
                {
                    A_PlaySound(TELEPORTER, spriteNum);
                    A_Spawn(spriteNum, TRANSPORTERSTAR);
                    pSprite->cstat = 257;
                }
                goto next_sprite;
            }

            int32_t playerDist;
            playerNum = A_FindPlayer(pSprite, &playerDist);
            pPlayer   = g_player[playerNum].ps;

            if (playerDist < 1220)
                pSprite->cstat &= ~257;
            else
                pSprite->cstat |= 257;

            if (pData[3] == 0)
            {
                if (A_IncurDamage(spriteNum) >= 0)
                {
                    pData[3]       = 1;
                    pData[2]       = 0;
                    detonatePlayer = 0;
                    pSprite->xvel  = 0;
                    goto DETONATEB;
                }
            }

            if (pSprite->picnum != BOUNCEMINE)
            {
                A_Fall(spriteNum);

                if ((sector[sectNum].lotag != ST_1_ABOVE_WATER || actor[spriteNum].floorz != sector[sectNum].floorz) && pSprite->z >= actor[spriteNum].floorz-(ZOFFSET) && pSprite->yvel < 3)
                {
                    if (pSprite->yvel > 0 || (pSprite->yvel == 0 && actor[spriteNum].floorz == sector[sectNum].floorz))
                        A_PlaySound(PIPEBOMB_BOUNCE,spriteNum);
                    pSprite->zvel = -((4-pSprite->yvel)<<8);
                    if (sector[pSprite->sectnum].lotag == ST_2_UNDERWATER)
                        pSprite->zvel >>= 2;
                    pSprite->yvel++;
                }
                if (pSprite->z < actor[spriteNum].ceilingz)   // && sector[sect].lotag != ST_2_UNDERWATER )
                {
                    pSprite->z = actor[spriteNum].ceilingz+(3<<8);
                    pSprite->zvel = 0;
                }
            }

            // can't initialize this because of the goto above
            vec3_t tmpvect;
            tmpvect.x = (pSprite->xvel * (sintable[(pSprite->ang + 512) & 2047])) >> 14;
            tmpvect.y = (pSprite->xvel * (sintable[pSprite->ang & 2047])) >> 14;
            tmpvect.z = pSprite->zvel;

            int moveSprite;
            moveSprite = A_MoveSprite(spriteNum, &tmpvect, CLIPMASK0);

            actor[spriteNum].movflag = moveSprite;

            if (sector[SECT(spriteNum)].lotag == ST_1_ABOVE_WATER && pSprite->zvel == 0 && actor[spriteNum].floorz == sector[sectNum].floorz)
            {
                pSprite->z += ZOFFSET5;
                if (pData[5] == 0)
                {
                    pData[5] = 1;
                    A_Spawn(spriteNum,WATERSPLASH2);
                }
            }
            else pData[5] = 0;

            if (pData[3] == 0 && (pSprite->picnum == BOUNCEMINE || pSprite->picnum == MORTER) && (moveSprite || playerDist < 844))
            {
                pData[3] = 1;
                pData[2] = 0;
                detonatePlayer = 0;
                pSprite->xvel = 0;
                goto DETONATEB;
            }

            if (sprite[pSprite->owner].picnum == APLAYER)
                detonatePlayer = P_Get(pSprite->owner);
            else detonatePlayer = -1;

            if (pSprite->xvel > 0)
            {
                pSprite->xvel -= 5;
                if (sector[sectNum].lotag == ST_2_UNDERWATER)
                    pSprite->xvel -= 10;

                if (pSprite->xvel < 0)
                    pSprite->xvel = 0;
                if (pSprite->xvel&8) pSprite->cstat ^= 4;
            }

            if ((moveSprite&49152) == 32768)
            {
                vec3_t davect = *(vec3_t *)pSprite;
                moveSprite &= (MAXWALLS - 1);
                A_DamageWall(spriteNum, moveSprite, &davect, pSprite->picnum);
                Proj_BounceOffWall(pSprite, moveSprite);
                pSprite->xvel >>= 1;
            }

DETONATEB:
            // Pipebomb control set to timer? (see player.c)
            // TIMER_CONTROL
            if (pSprite->picnum == HEAVYHBOMB && pData[6] == 1)
            {
                if (pData[7] >= 1)
                    pData[7]--;

                if (pData[7] <= 0)
                    pData[6] = 3;
            }

            if ((detonatePlayer >= 0 && g_player[detonatePlayer].ps->hbomb_on == 0 && pData[6] == 2) || pData[3] == 1)
                pData[6] = 3;

            if (pData[6] == 3)
            {
                pData[2]++;

                if (pData[2] == 2)
                {
                    int const x      = pSprite->extra;
                    int       radius = 0;

                    switch (DYNAMICTILEMAP(pSprite->picnum))
                    {
                        case HEAVYHBOMB__STATIC: radius = g_pipebombRadius; break;
                        case MORTER__STATIC: radius     = g_morterRadius; break;
                        case BOUNCEMINE__STATIC: radius = g_bouncemineRadius; break;
                    }

                    A_RadiusDamage(spriteNum, radius, x >> 2, x >> 1, x - (x >> 2), x);

                    int const j = A_Spawn(spriteNum, EXPLOSION2);
                    A_PlaySound(PIPEBOMB_EXPLODE, j);

                    if (pSprite->zvel == 0)
                        A_Spawn(spriteNum,EXPLOSION2BOT);

                    for (bssize_t x = 0; x < 8; ++x)
                        RANDOMSCRAP(pSprite, spriteNum);
                }

                if (pSprite->yrepeat)
                {
                    pSprite->yrepeat = 0;
                    goto next_sprite;
                }

                if (pData[2] > 20)
                {
                    if (pSprite->owner != spriteNum || ud.respawn_items == 0)
                    {
                        DELETE_SPRITE_AND_CONTINUE(spriteNum);
                    }
                    else
                    {
                        pData[2] = g_itemRespawnTime;
                        A_Spawn(spriteNum,RESPAWNMARKERRED);
                        pSprite->cstat = 32768;
                        pSprite->yrepeat = 9;
                        goto next_sprite;
                    }
                }
            }
            else if (pSprite->picnum == HEAVYHBOMB && playerDist < 788 && pData[0] > 7 && pSprite->xvel == 0)
            {
                if (cansee(pSprite->x, pSprite->y, pSprite->z - ZOFFSET3, pSprite->sectnum,
                           pPlayer->pos.x, pPlayer->pos.y, pPlayer->pos.z, pPlayer->cursectnum))
                {
                    if (pPlayer->ammo_amount[HANDBOMB_WEAPON] < pPlayer->max_ammo_amount[HANDBOMB_WEAPON])
                    {
                        if ((g_gametypeFlags[ud.coop] & GAMETYPE_WEAPSTAY) && pSprite->owner == spriteNum)
                        {
                            for (bssize_t j = 0; j < pPlayer->weapreccnt; j++)
                            {
                                if (pPlayer->weaprecs[j] == pSprite->picnum)
                                    goto next_sprite;
                            }

                            if (pPlayer->weapreccnt < MAX_WEAPONS)
                                pPlayer->weaprecs[pPlayer->weapreccnt++] = pSprite->picnum;
                        }

                        P_AddAmmo(pPlayer, HANDBOMB_WEAPON, 1);
                        A_PlaySound(DUKE_GET, pPlayer->i);

                        if ((pPlayer->gotweapon & (1<<HANDBOMB_WEAPON)) == 0 || pSprite->owner == pPlayer->i)
                        {
                            int doSwitch = ((pPlayer->weaponswitch & 1) ||
                                PWEAPON(playerNum, pPlayer->curr_weapon, WorksLike) == HANDREMOTE_WEAPON);
                            P_AddWeapon(pPlayer, HANDBOMB_WEAPON, doSwitch);
                        }

                        if (sprite[pSprite->owner].picnum != APLAYER)
                            P_PalFrom(pPlayer, 32, 0, 32, 0);

                        if (pSprite->owner != spriteNum || ud.respawn_items == 0)
                        {
                            if (pSprite->owner == spriteNum && (g_gametypeFlags[ud.coop] & GAMETYPE_WEAPSTAY))
                                goto next_sprite;
                            DELETE_SPRITE_AND_CONTINUE(spriteNum);
                        }
                        else
                        {
                            pData[2] = g_itemRespawnTime;
                            A_Spawn(spriteNum, RESPAWNMARKERRED);
                            pSprite->cstat = 32768;
                        }
                    }
                }
            }

            if (pData[0] < 8)
                pData[0]++;

            goto next_sprite;
        }

        case REACTORBURNT__STATIC:
        case REACTOR2BURNT__STATIC:
            goto next_sprite;

        case REACTOR__STATIC:
        case REACTOR2__STATIC:
        {
            if (pData[4] == 1)
            {
                for (bssize_t SPRITES_OF_SECT(sectNum, j))
                {
                    switch (DYNAMICTILEMAP(sprite[j].picnum))
                    {
                    case SECTOREFFECTOR__STATIC:
                        if (sprite[j].lotag == 1)
                        {
                            sprite[j].lotag = 65535;
                            sprite[j].hitag = 65535;
                        }
                        break;
                    case REACTOR__STATIC:
                        sprite[j].picnum = REACTORBURNT;
                        break;
                    case REACTOR2__STATIC:
                        sprite[j].picnum = REACTOR2BURNT;
                        break;
                    case REACTORSPARK__STATIC:
                    case REACTOR2SPARK__STATIC:
                        sprite[j].cstat = 32768;
                        break;
                    }
                }

                goto next_sprite;
            }

            if (pData[1] >= 20)
            {
                pData[4] = 1;
                goto next_sprite;
            }

            int32_t             playerDist;
            int                 playerNum = A_FindPlayer(pSprite, &playerDist);
            DukePlayer_t *const pPlayer   = g_player[playerNum].ps;

            if (++pData[2] == 4)
                pData[2] = 0;

            if (playerDist < 4096)
            {
                if ((krand() & 255) < 16)
                {
                    if (!A_CheckSoundPlaying(pPlayer->i, DUKE_LONGTERM_PAIN))
                        A_PlaySound(DUKE_LONGTERM_PAIN, pPlayer->i);

                    A_PlaySound(SHORT_CIRCUIT, spriteNum);
                    sprite[pPlayer->i].extra--;
                    P_PalFrom(pPlayer, 32, 32, 0, 0);
                }

                pData[0] += 128;

                if (pData[3] == 0)
                    pData[3] = 1;
            }
            else pData[3] = 0;

            if (pData[1])
            {
                pData[1]++;
                pData[4]   = pSprite->z;
                pSprite->z = sector[sectNum].floorz - (krand() % (sector[sectNum].floorz - sector[sectNum].ceilingz));

                switch (pData[1])
                {
                    case 3:
                        // Turn on all of those flashing sectoreffector.
                        A_RadiusDamage(spriteNum, 4096, g_impactDamage << 2, g_impactDamage << 2, g_impactDamage << 2, g_impactDamage << 2);

                        for (bssize_t SPRITES_OF(STAT_STANDABLE, j))
                        {
                            if (sprite[j].picnum == MASTERSWITCH && sprite[j].hitag == pSprite->hitag && sprite[j].yvel == 0)
                                sprite[j].yvel = 1;
                        }
                        break;

                    case 4:
                    case 7:
                    case 10:
                    case 15:
                        for (bssize_t SPRITES_OF_SECT(sectNum, j))
                        {
                            if (j != spriteNum)
                            {
                                A_DeleteSprite(j);
                                break;
                            }
                        }

                        break;
                }

                for (bssize_t x = 0; x < 16; x++)
                    RANDOMSCRAP(pSprite, spriteNum);

                pSprite->z = pData[4];
                pData[4]   = 0;
            }
            else if (A_IncurDamage(spriteNum) >= 0)
            {
                for (bssize_t x = 0; x < 32; x++)
                    RANDOMSCRAP(pSprite, spriteNum);

                if (pSprite->extra < 0)
                    pData[1] = 1;
            }
            goto next_sprite;
        }

        case CAMERA1__STATIC:
            if (pData[0] == 0)
            {
                pData[1]+=8;
                if (g_damageCameras)
                {
                    if (A_IncurDamage(spriteNum) >= 0)
                    {
                        pData[0]       = 1;  // static
                        pSprite->cstat = 32768;

                        for (bssize_t x = 0; x < 5; x++)
                            RANDOMSCRAP(pSprite, spriteNum);

                        goto next_sprite;
                    }
                }

                if (pSprite->hitag > 0)
                {
                    if (pData[1] < pSprite->hitag)             pSprite->ang += 8;
                    else if (pData[1] < pSprite->hitag * 3)    pSprite->ang -= 8;
                    else if (pData[1] < (pSprite->hitag << 2)) pSprite->ang += 8;
                    else
                    {
                        pData[1] = 8;
                        pSprite->ang += 16;
                    }
                }
            }
            goto next_sprite;
        }

        if (!g_netServer && ud.multimode < 2 && A_CheckEnemySprite(pSprite))
        {
            if (g_noEnemies == 1)
            {
                pSprite->cstat = 32768;
                goto next_sprite;
            }
            else if (g_noEnemies == 2)
            {
                pSprite->cstat = 0;
                if (pSprite->extra)
                    pSprite->cstat = 257;
            }
        }

        if (G_HaveActor(sprite[spriteNum].picnum))
        {
            int32_t playerDist;
            int     playerNum = A_FindPlayer(pSprite, &playerDist);
            A_Execute(spriteNum, playerNum, playerDist);
        }
next_sprite:
        A_MaybeAwakenBadGuys(spriteNum);
        spriteNum = nextSprite;
    }
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
        if (pSprite->picnum > NUKEBUTTON && pSprite->picnum <= NUKEBUTTON+3)
            switchPic = NUKEBUTTON;

        if (pSprite->picnum > GLASSPIECES && pSprite->picnum <= GLASSPIECES+2)
            switchPic = GLASSPIECES;

        if (pSprite->picnum == INNERJAW+1)
            switchPic--;

        if ((pSprite->picnum == MONEY+1) || (pSprite->picnum == MAIL+1) || (pSprite->picnum == PAPER+1))
            actor[spriteNum].floorz = pSprite->z = getflorzofslope(pSprite->sectnum,pSprite->x,pSprite->y);
        else switch (DYNAMICTILEMAP(switchPic))
            {
                case APLAYER__STATIC: pSprite->cstat = 32768; goto next_sprite;
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
                    if (pData[0] == 3 * GAMETICSPERSEC)
                        goto next_sprite;

                    actor[spriteNum].bpos.z -= pSprite->z;

                    if ((++pData[0] % 9) == 0)
                    {
                        pSprite->yrepeat++;
                        pSprite->z += (tilesiz[pSprite->picnum].y * pSprite->yrepeat) >> 2;
                    }
                    else
                        pSprite->z += 16 + (krand() & 15);

                    actor[spriteNum].bpos.z += pSprite->z;
                    goto next_sprite;

                case NUKEBUTTON__STATIC:
                    //        case NUKEBUTTON+1:
                    //        case NUKEBUTTON+2:
                    //        case NUKEBUTTON+3:

                    if (pData[0])
                    {
                        pData[0]++;
                        if (pData[0] == 8)
                            pSprite->picnum = NUKEBUTTON + 1;
                        else if (pData[0] == 16)
                        {
                            pSprite->picnum = NUKEBUTTON + 2;
                            g_player[P_Get(pSprite->owner)].ps->fist_incs = 1;
                        }
                        if (g_player[P_Get(pSprite->owner)].ps->fist_incs == GAMETICSPERSEC)
                            pSprite->picnum = NUKEBUTTON + 3;
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
                fallthrough__;
            case FRAMEEFFECT1_13__STATIC:
                if (PLUTOPAK) goto next_sprite;	// JBF: ideally this should never happen...
                fallthrough__;
            case FRAMEEFFECT1__STATIC:

                if (pSprite->owner >= 0)
                {
                    pData[0]++;

                    if (pData[0] > 7)
                    {
                        DELETE_SPRITE_AND_CONTINUE(spriteNum);
                    }
                    else if (pData[0] > 4)
                        pSprite->cstat |= 512+2;
                    else if (pData[0] > 2)
                        pSprite->cstat |= 2;
                    pSprite->xoffset = sprite[pSprite->owner].xoffset;
                    pSprite->yoffset = sprite[pSprite->owner].yoffset;
                }
                goto next_sprite;
            case INNERJAW__STATIC:
            {
                //        case INNERJAW+1:
                int32_t playerDist, playerNum = A_FindPlayer(pSprite,&playerDist);

                if (playerDist < 512)
                {
                    P_PalFrom(g_player[playerNum].ps, 32, 32,0,0);
                    sprite[g_player[playerNum].ps->i].extra -= 4;
                }
            }
            fallthrough__;
            case FIRELASER__STATIC:
                if (pSprite->extra != 5)
                    pSprite->extra = 5;
                else DELETE_SPRITE_AND_CONTINUE(spriteNum);
                break;
            case TONGUE__STATIC:
                DELETE_SPRITE_AND_CONTINUE(spriteNum);

            case MONEY__STATIC:
            case MAIL__STATIC:
            case PAPER__STATIC:
            {
                pSprite->xvel = (krand()&7)+(sintable[T1(spriteNum)&2047]>>9);
                T1(spriteNum) += (krand()&63);
                if ((T1(spriteNum)&2047) > 512 && (T1(spriteNum)&2047) < 1596)
                {
                    if (sector[sectNum].lotag == ST_2_UNDERWATER)
                    {
                        if (pSprite->zvel < 64)
                            pSprite->zvel += (g_spriteGravity>>5)+(krand()&7);
                    }
                    else if (pSprite->zvel < 144)
                        pSprite->zvel += (g_spriteGravity>>5)+(krand()&7);
                }

                A_SetSprite(spriteNum, CLIPMASK0);

                if ((krand()&3) == 0)
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
                        if (sprite[j].picnum == BLOODPOOL && ldist(pSprite, &sprite[j]) < 348)
                        {
                            pSprite->pal = 2;
                            break;
                        }
                    }
                }

                break;
            }

            case JIBS1__STATIC:
            case JIBS2__STATIC:
            case JIBS3__STATIC:
            case JIBS4__STATIC:
            case JIBS5__STATIC:
            case JIBS6__STATIC:
            case HEADJIB1__STATIC:
            case ARMJIB1__STATIC:
            case LEGJIB1__STATIC:
            case LIZMANHEAD1__STATIC:
            case LIZMANARM1__STATIC:
            case LIZMANLEG1__STATIC:
            case DUKETORSO__STATIC:
            case DUKEGUN__STATIC:
            case DUKELEG__STATIC:
            {
                pSprite->xvel = (pSprite->xvel > 0) ? pSprite->xvel - 1 : 0;

                if (++pData[5] == (30*10))
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);

                if (pSprite->zvel > 1024 && pSprite->zvel < 1280)
                {
                    setsprite(spriteNum, (vec3_t *) pSprite);
                    sectNum = pSprite->sectnum;
                }

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

                        if (pSprite->picnum == DUKELEG || pSprite->picnum == DUKETORSO || pSprite->picnum == DUKEGUN)
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
                }
                else
                {
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

                    if (pSprite->picnum == JIBS6)
                    {
                        pData[1]++;

                        if ((pData[1]&3) == 0 && pData[0] < 7)
                            pData[0]++;

                        if (pData[1] > 20)
                            DELETE_SPRITE_AND_CONTINUE(spriteNum);
                    }
                    else
                    {
                        pSprite->picnum = JIBS6;
                        pData[0] = 0;
                        pData[1] = 0;
                    }
                }
                goto next_sprite;
            }

            case BLOODPOOL__STATIC:
            case PUKE__STATIC:
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
                pSprite->z          = actor[spriteNum].floorz - 1;

                DukePlayer_t *const pPlayer = g_player[playerNum].ps;

                if (pData[2] < 32)
                {
                    pData[2]++;

                    if (actor[spriteNum].picnum == TIRE)
                    {
                        if (pSprite->xrepeat < 64 && pSprite->yrepeat < 64)
                        {
                            pSprite->xrepeat += krand()&3;
                            pSprite->yrepeat += krand()&3;
                        }
                    }
                    else
                    {
                        if (pSprite->xrepeat < 32 && pSprite->yrepeat < 32)
                        {
                            pSprite->xrepeat += krand()&3;
                            pSprite->yrepeat += krand()&3;
                        }
                    }
                }

                if (playerDist < 844 && pSprite->xrepeat > 6 && pSprite->yrepeat > 6)
                {
                    if (pSprite->pal == 0 && pSprite->picnum != PUKE && (krand()&255) < 16)
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

                    pPlayer->footprintcount = (actor[spriteNum].picnum == TIRE) ? 10 : 3;
                    pPlayer->footprintpal   = pSprite->pal;
                    pPlayer->footprintshade = pSprite->shade;

                    if (pData[2] == 32)
                    {
                        pSprite->xrepeat -= 6;
                        pSprite->yrepeat -= 6;
                    }
                }
                else pData[1] = 0;
                goto next_sprite;
            }

            case BURNING__STATIC:
            case BURNING2__STATIC:
            case FECES__STATIC:
            case WATERBUBBLE__STATIC:
            case SMALLSMOKE__STATIC:
            case EXPLOSION2__STATIC:
            case SHRINKEREXPLOSION__STATIC:
            case EXPLOSION2BOT__STATIC:
            case BLOOD__STATIC:
            case LASERSITE__STATIC:
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

                if (sectNum < 0 || (sector[sectNum].floorz + 256) < pSprite->z)
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
                    //                else KILLIT(i);
                }

                goto next_sprite;

            case GLASSPIECES__STATIC:
                //        case GLASSPIECES+1:
                //        case GLASSPIECES+2:

                A_Fall(spriteNum);

                if (pSprite->zvel > 4096) pSprite->zvel = 4096;
                if (sectNum < 0)
                    DELETE_SPRITE_AND_CONTINUE(spriteNum);

                if (pSprite->z == actor[spriteNum].floorz-(ZOFFSET) && pData[0] < 3)
                {
                    pSprite->zvel = -((3-pData[0])<<8)-(krand()&511);
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

        if (PN(spriteNum) >= SCRAP6 && PN(spriteNum) <= SCRAP5+3)
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

                    if (pSprite->picnum < SCRAP6 + 8)
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
                if (pSprite->picnum == SCRAP1 && pSprite->yvel > 0 && pSprite->yvel < MAXUSERTILES)
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
        pData[3] = pSprite->hitag;

        for (bssize_t SPRITES_OF_SECT(pSprite->sectnum, j))
        {
            if (sprite[j].zvel == 0 && sprite[j].statnum != STAT_EFFECTOR && sprite[j].statnum != STAT_PROJECTILE)
            {
                actor[j].bpos.z = sprite[j].z;
                actor[j].floorz = pSector->floorz;
            }
        }

        A_CallSound(pSprite->sectnum, spriteNum);
    }
    else
    {
        int const zChange = ksgn(zDifference) * SP(spriteNum);

        pSector->floorz += zChange;

        for (bssize_t SPRITES_OF_SECT(pSprite->sectnum, j))
        {
            if (sprite[j].picnum == APLAYER && sprite[j].owner >= 0)
            {
                int const playerNum = P_Get(j);

                if (g_player[playerNum].ps->on_ground == 1)
                    g_player[playerNum].ps->pos.z += zChange;
            }

            if (sprite[j].zvel == 0 && sprite[j].statnum != STAT_EFFECTOR && sprite[j].statnum != STAT_PROJECTILE)
            {
                actor[j].bpos.z = sprite[j].z;
                sprite[j].z += zChange;
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

            if (pPlayer->cursectnum != pSprite->sectnum && (playerSectnum == -1 || playerSectnum == pSprite->sectnum))
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
static void MaybeTrainKillEnemies(int const spriteNum)
{
    int findSprite = headspritesect[sprite[OW(spriteNum)].sectnum];

    do
    {
        int const nextSprite = nextspritesect[findSprite];

        if (sprite[findSprite].extra >= 0 && sprite[findSprite].statnum == STAT_ACTOR && A_CheckEnemySprite(&sprite[findSprite]))
        {
            int16_t sectNum = sprite[findSprite].sectnum;

            updatesector(sprite[findSprite].x,sprite[findSprite].y,&sectNum);

            if (sectNum == sprite[spriteNum].sectnum || sectNum == -1)
            {
                actor[findSprite].picnum = RADIUSEXPLOSION;
                actor[findSprite].extra  = g_impactDamage << 10;
                actor[findSprite].owner  = spriteNum;
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

    while (spriteNum >= 0)
    {
        int const         nextSprite = nextspritestat[spriteNum];
        spritetype *const   pSprite    = &sprite[spriteNum];
        int32_t             playerDist;
        int                 playerNum = A_FindPlayer(pSprite, &playerDist);
        DukePlayer_t *const pPlayer   = g_player[playerNum].ps;

        if (VM_OnEventWithBoth(EVENT_MOVEEFFECTORS, spriteNum, playerNum, playerDist, 0))
        {
            spriteNum = nextSprite;
            continue;
        }

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

            if (sprite[j].lotag == UINT16_MAX)
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
                        pPlayer->ang += (l*q);
                        pPlayer->ang &= 2047;

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
                        if (sprite[p].picnum != LASERLINE)
                        {
                            if (sprite[p].picnum == APLAYER && sprite[p].owner >= 0)
                                continue;

                            sprite[p].ang += (l*q);
                            sprite[p].ang &= 2047;

                            sprite[p].z += zchange;

                            // interpolation fix
                            actor[p].bpos.x = sprite[p].x;
                            actor[p].bpos.y = sprite[p].y;

                            if (move_rotfixed_sprite(p, j, pData[2]))
                                rotatepoint(*(vec2_t *)&sprite[j], *(vec2_t *)&sprite[p], (q * l), (vec2_t *)&sprite[p].x);
                        }
                }

            }
            else if (l==0 && (pSector->floorstat&64))
            {
                // fix for jittering of sprites in halted rotating sectors
                for (bssize_t SPRITES_OF_SECT(pSprite->sectnum, p))
                {
                    // KEEPINSYNC1
                    if (sprite[p].statnum != STAT_EFFECTOR && sprite[p].statnum != STAT_PROJECTILE)
                        if (sprite[p].picnum != LASERLINE)
                        {
                            if (sprite[p].picnum == APLAYER && sprite[p].owner >= 0)
                                continue;

                            actor[p].bpos.x = sprite[p].x;
                            actor[p].bpos.y = sprite[p].y;
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
                }
            }
            else pSprite->xvel = k;

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
                    if ((pSector->floorstat&1) == 0 && (pSector->ceilingstat&1) == 0)
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
                            A_Shoot(spriteNum,RPG);
                            pSprite->ang = j;
                        }
                    }
                }

                if (pSprite->xvel <= 64 && (pSector->floorstat&1) == 0 && (pSector->ceilingstat&1) == 0)
                    S_StopEnvSound(actor[spriteNum].lastv.x,spriteNum);

                if ((pSector->floorz-pSector->ceilingz) < (108<<8))
                {
                    if (ud.noclip == 0 && pSprite->xvel >= 192)
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

                            pPlayer->ang += q;
                            pPlayer->ang &= 2047;

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
                            (sprite[j].picnum != SECTOREFFECTOR || (sprite[j].lotag == SE_49_POINT_LIGHT||sprite[j].lotag == SE_50_SPOT_LIGHT))
                            && sprite[j].picnum != LOCATORS)
                    {
                        // fix interpolation
                        if (numplayers < 2 && !g_netServer)
                        {
                            actor[j].bpos.x = sprite[j].x;
                            actor[j].bpos.y = sprite[j].y;
                        }

                        if (move_rotfixed_sprite(j, pSprite-sprite, pData[2]))
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
                    if (ud.noclip == 0 && pSprite->xvel >= 192)
                        MaybeTrainKillPlayer(pSprite, 1);

                    MaybeTrainKillEnemies(spriteNum);
                }
            }
            else
            {
                // fix for jittering of sprites in halted subways
                for (SPRITES_OF_SECT(pSprite->sectnum, j))
                {
                    // KEEPINSYNC2
                    if (sprite[j].statnum != STAT_PLAYER && sector[sprite[j].sectnum].lotag != ST_2_UNDERWATER &&
                            (sprite[j].picnum != SECTOREFFECTOR || (sprite[j].lotag == SE_49_POINT_LIGHT||sprite[j].lotag == SE_50_SPOT_LIGHT))
                            && sprite[j].picnum != LOCATORS)
                    {
                        actor[j].bpos.x = sprite[j].x;
                        actor[j].bpos.y = sprite[j].y;
                    }
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
                            if (sprite[j].picnum != SECTOREFFECTOR && sprite[j].picnum != LOCATORS)
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
                    if (ud.noclip == 0)
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
                    if ((sprite[j].picnum != SECTOREFFECTOR || sprite[j].lotag==SE_49_POINT_LIGHT || sprite[j].lotag==SE_50_SPOT_LIGHT)
                            && sprite[j].picnum != LOCATORS)
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
                    if (ud.noclip == 0)
                        MaybeTrainKillPlayer(pSprite, 1);

                    MaybeTrainKillEnemies(spriteNum);
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
                    if (sprite[sectSprite].picnum != SECTOREFFECTOR)
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

            //Flashing sector lights after reactor EXPLOSION2

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
                if (sprite[sectSprite].cstat&16 && A_CheckSpriteFlags(sectSprite,SFLAG_NOSHADE) == 0)
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
                A_Shoot(spriteNum, FIRELASER);
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
                int const saveAng = pSprite->ang;
                pSprite->ang      = getangle(pPlayer->pos.x - pSprite->x, pPlayer->pos.y - pSprite->y);
                pSprite->ang      = saveAng;
                pSprite->owner    = -1;
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
                int shadeInc = ((pSector->lotag & 0x8000) || actor[spriteNum].t_data[4]) ? -pData[3] : pData[3];

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
                    }
                }
            }
            break;
        }

        case SE_10_DOOR_AUTO_CLOSE:
            // XXX: 32791, what the hell?
            if ((pSector->lotag&0xff) == ST_27_STRETCH_BRIDGE || (pSector->floorz > pSector->ceilingz && (pSector->lotag&0xff) != ST_23_SWINGING_DOOR) || pSector->lotag == 32791)
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
                            if (GetAnimationGoal(&sector[pSprite->sectnum].ceilingz) >= 0)
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
                        if (sprite[k].owner >= 0 && clipinsidebox((vec2_t *)&sprite[k], j, pPlayer->clipdist + 40) == 1)
                        {
                            pData[5] = 8;  // Delay
                            goto next_sprite;
                        }
                    }
                }

                k = (SP(spriteNum) >> 3) * pData[3];
                pData[2] += k;
                pData[4] += k;
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
                    if ((sprite[j].cstat & 16) && (A_CheckSpriteFlags(j, SFLAG_NOSHADE) == 0))
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
                        if ((sprite[j].cstat & 16) && (A_CheckSpriteFlags(j, SFLAG_NOSHADE) == 0))
                            sprite[j].shade = (pSector->ceilingstat & 1) ? pSector->ceilingshade : pSector->floorshade;
                    }
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

                    pSector->ceilingz = (klabs(pData[0] - pSector->ceilingz) >= j)
                                      ? pSector->ceilingz + ksgn(pData[0] - pSector->ceilingz) * j
                                      : pData[0];
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
                    if (sprite[j].picnum == REACTOR || sprite[j].picnum == REACTOR2)
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
                        pPlayer->opos.z -= pPlayer->pos.z;
                        pPlayer->pos.z = sector[sprite[j].sectnum].floorz - (pSector->floorz - pPlayer->pos.z);
                        pPlayer->opos.z += pPlayer->pos.z;

                        actor[k].floorz             = sector[sprite[j].sectnum].floorz;
                        actor[k].ceilingz           = sector[sprite[j].sectnum].ceilingz;
                        *(vec2_t *)&pPlayer->opos   = *(vec2_t *)pPlayer;
                        *(vec2_t *)&pPlayer->bobpos = *(vec2_t *)pPlayer;
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

                        Bmemcpy(&actor[k].bpos, &sprite[k], sizeof(vec2_t));

                        actor[k].bpos.z -= sprite[k].z;
                        sprite[k].z = sector[sprite[j].sectnum].floorz - (pSector->floorz - sprite[k].z);
                        actor[k].bpos.z += sprite[k].z;

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

                        for (bssize_t SPRITES_OF_SECT(pSprite->sectnum, sectSprite))
                        {
                            if (sprite[sectSprite].picnum == APLAYER && sprite[sectSprite].owner >= 0 && g_player[P_Get(sectSprite)].ps->on_ground == 1)
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

                        for (bssize_t SPRITES_OF_SECT(pSprite->sectnum, sectSprite))
                        {
                            if (sprite[sectSprite].picnum == APLAYER && sprite[sectSprite].owner >= 0 &&g_player[P_Get(sectSprite)].ps->on_ground == 1)
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
                        if (wall[j].overpicnum == BIGFORCE)
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
                            case BLOODPOOL__STATIC:
                            case PUKE__STATIC:
                            case FOOTPRINTS__STATIC:
                            case FOOTPRINTS2__STATIC:
                            case FOOTPRINTS3__STATIC:
                            case FOOTPRINTS4__STATIC:
                            case BULLETHOLE__STATIC:
                            case BLOODSPLAT1__STATIC:
                            case BLOODSPLAT2__STATIC:
                            case BLOODSPLAT3__STATIC:
                            case BLOODSPLAT4__STATIC: sprite[sectSprite].xrepeat = sprite[sectSprite].yrepeat = 0; continue;

                            case LASERLINE__STATIC: continue;
                        }
                    // fall-through
                    case STAT_STANDABLE:
                        if (sprite[sectSprite].picnum == TRIPBOMB)
                            break;
                    // else, fall-through
                    case STAT_ACTOR:
                    case STAT_DEFAULT:
                        if (sprite[sectSprite].picnum == BOLT1
                            || sprite[sectSprite].picnum == BOLT1 + 1
                            || sprite[sectSprite].picnum == BOLT1 + 2
                            || sprite[sectSprite].picnum == BOLT1 + 3
                            || sprite[sectSprite].picnum == SIDEBOLT1
                            || sprite[sectSprite].picnum == SIDEBOLT1 + 1
                            || sprite[sectSprite].picnum == SIDEBOLT1 + 2
                            || sprite[sectSprite].picnum == SIDEBOLT1 + 3
                            || A_CheckSwitchTile(sectSprite))
                            break;

                        if (!(sprite[sectSprite].picnum >= CRANE && sprite[sectSprite].picnum <= CRANE + 3))
                        {
                            if (sprite[sectSprite].z > actor[sectSprite].floorz - ZOFFSET2)
                            {
                                actor[sectSprite].bpos.x = sprite[sectSprite].x;
                                actor[sectSprite].bpos.y = sprite[sectSprite].y;

                                sprite[sectSprite].x += vect.x >> 2;
                                sprite[sectSprite].y += vect.y >> 2;

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
            pSector->floorxpanning += SP(spriteNum)>>7;

            break;
        }

        case SE_35:
            if (pSector->ceilingz > pSprite->z)
            {
                for (j = 0; j < 8; j++)
                {
                    pSprite->ang += krand()&511;
                    k = A_Spawn(spriteNum, SMALLSMOKE);
                    sprite[k].xvel = 96+(krand()&127);
                    A_SetSprite(k, CLIPMASK0);
                    setsprite(k, (vec3_t *) &sprite[k]);
                    if (rnd(16))
                        A_Spawn(spriteNum, EXPLOSION2);
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
            else if (pSector->ceilingz <= pData[3])
                pSprite->shade = 1;

            if (pSprite->shade)
            {
                pSector->ceilingz += SP(spriteNum)<<4;
                if (pSector->ceilingz > pSector->floorz)
                    pSector->ceilingz = pSector->floorz;
            }
            else
            {
                pSector->ceilingz   -= SP(spriteNum)<<4;
                if (pSector->ceilingz < pData[3])
                    pSector->ceilingz = pData[3];
            }

            break;

        case SE_26:
        {
            int32_t p, nextj;

            pSprite->xvel = pSector->extra != 0 ? pSector->extra : 32;
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
                if (sprite[j].statnum != STAT_EFFECTOR && sprite[j].statnum != STAT_PLAYER)
                {
                    actor[j].bpos.x = sprite[j].x;
                    actor[j].bpos.y = sprite[j].y;

                    sprite[j].x += l;
                    sprite[j].y += x;

                    sprite[j].z += pSprite->zvel;
                    setsprite(j,(vec3_t *)&sprite[j]);
                }
            }

            for (TRAVERSE_CONNECT(p))
            {
                DukePlayer_t *const ps = g_player[p].ps;

                if (sprite[ps->i].sectnum == pSprite->sectnum && ps->on_ground)
                {
                    ps->fric.x += l<<5;
                    ps->fric.y += x<<5;
                    ps->pos.z += pSprite->zvel;
                }
            }
            A_MoveSector(spriteNum);
            setsprite(spriteNum,(vec3_t *)pSprite);

            break;
        }

        case SE_27_DEMO_CAM:
        {
            if (ud.recstat == 0 || !ud.democams) break;

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
                T2(spriteNum) = 64 + (krand()&511);
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
                        if (sprite[j].picnum == NATURALLIGHTNING && sprite[j].hitag == pSprite->hitag)
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
                        if (sprite[j].picnum == NATURALLIGHTNING && sprite[j].hitag == pSprite->hitag)
                        {
                            if (rnd(32) && (T3(spriteNum)&1))
                            {
                                int32_t p;
                                DukePlayer_t *ps;

                                sprite[j].cstat &= 32767;
                                A_Spawn(j,SMALLSMOKE);

                                p = A_FindPlayer(pSprite, NULL);
                                ps = g_player[p].ps;

                                x = ldist(&sprite[ps->i], &sprite[j]);
                                if (x < 768)
                                {
                                    if (!A_CheckSoundPlaying(ps->i,DUKE_LONGTERM_PAIN))
                                        A_PlaySound(DUKE_LONGTERM_PAIN,ps->i);
                                    A_PlaySound(SHORT_CIRCUIT,ps->i);
                                    sprite[ps->i].extra -= 8+(krand()&7);

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

                if (pData[3] > 0)
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
            if (g_earthquakeTime > 0 && (krand()&7) == 0)
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
                k = A_Spawn(spriteNum,EXPLOSION2);
                sprite[k].xrepeat = sprite[k].yrepeat = 2+(krand()&7);
                sprite[k].z = pSector->floorz-(krand()%x);
                sprite[k].ang += 256-(krand()%511);
                sprite[k].xvel = krand()&127;
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
                k = A_Spawn(spriteNum,EXPLOSION2);
                sprite[k].xrepeat = sprite[k].yrepeat = 2+(krand()&3);
                sprite[k].z = pSector->floorz-(krand()%x);
                sprite[k].ang += 256-(krand()%511);
                sprite[k].xvel = krand()&127;
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
    int32_t i;

    for (SPRITES_OF(STAT_LIGHT, i))
    {
        switch (sprite[i].lotag)
        {
#ifdef POLYMER
        case SE_49_POINT_LIGHT:
        {
            if (!A_CheckSpriteFlags(i, SFLAG_NOLIGHT) && getrendermode() == REND_POLYMER &&
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
            if (!A_CheckSpriteFlags(i, SFLAG_NOLIGHT) && getrendermode() == REND_POLYMER &&
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
#endif // POLYMER
        }
    }
}

#ifdef POLYMER
static void A_DoLight(int spriteNum)
{
    spritetype *const pSprite = &sprite[spriteNum];
    int savedFires = 0;

    if (((sector[pSprite->sectnum].floorz - sector[pSprite->sectnum].ceilingz) < 16) || pSprite->z > sector[pSprite->sectnum].floorz || pSprite->z > actor[spriteNum].floorz ||
        (pSprite->picnum != SECTOREFFECTOR && ((pSprite->cstat & 32768) || pSprite->yrepeat < 4)) ||
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
                        goto POOP;

                    G_AddGameLight(0, spriteNum, (pSprite->yrepeat*tilesiz[pSprite->picnum].y)<<1, 512-ii*128,
                        ii==0 ? (172+(200<<8)+(104<<16)) : 216+(52<<8)+(20<<16), PR_LIGHT_PRIO_LOW);

                POOP:
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
    if (sprite[spriteNum].extra > 0)
    {
        if ((VM_OnEventWithReturn(EVENT_RECOGSOUND, spriteNum, screenpeek, 0)) != 0)
            return;

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
            case BOSS1__STATIC:
            case BOSS1STAYPUT__STATIC:     S_PlaySound(BOS1_RECOG); break;
            case BOSS2__STATIC:            S_PlaySound((sprite[spriteNum].pal != 0) ? BOS2_RECOG : WHIPYOURASS); break;
            case BOSS3__STATIC:            S_PlaySound((sprite[spriteNum].pal != 0) ? BOS3_RECOG : RIPHEADNECK); break;
            case BOSS4__STATIC:
            case BOSS4STAYPUT__STATIC:     S_PlaySound((sprite[spriteNum].pal != 0) ? BOS4_RECOG : BOSS4_FIRSTSEE); break;
            case GREENSLIME__STATIC:       A_PlaySound(SLIM_RECOG, spriteNum); break;
        }
    }
}

int A_CheckSwitchTile(int spriteNum)
{
    // picnum 0 would oob in the switch below,

    if (PN(spriteNum) <= 0)
        return 0;

    // MULTISWITCH has 4 states so deal with it separately,
    // ACCESSSWITCH and ACCESSSWITCH2 are only active in one state so deal with
    // them separately.

    if ((PN(spriteNum) >= MULTISWITCH && PN(spriteNum) <= MULTISWITCH + 3) || (PN(spriteNum) == ACCESSSWITCH || PN(spriteNum) == ACCESSSWITCH2))
        return 1;

    // Loop to catch both states of switches.
    for (bssize_t j=1; j>=0; j--)
    {
        switch (DYNAMICTILEMAP(PN(spriteNum)-j))
        {
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
    if (Numsprites && getrendermode() == REND_POLYMER)
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

static void G_DoEventGame(int const nEventID)
{
    if (VM_HaveEvent(nEventID))
    {
        int statNum = 0;

        do
        {
            int spriteNum = headspritestat[statNum++];

            while (spriteNum >= 0)
            {
                int const nextSprite = nextspritestat[spriteNum];

                if (A_CheckSpriteFlags(spriteNum, SFLAG_NOEVENTCODE))
                {
                    spriteNum = nextSprite;
                    continue;
                }

                int32_t   playerDist;
                int const playerNum = A_FindPlayer(&sprite[spriteNum], &playerDist);
                VM_OnEventWithDist_(nEventID, spriteNum, playerNum, playerDist);

                spriteNum = nextSprite;
            }
        }
        while (statNum < MAXSTATUS);
    }

    int spriteNum = headspritestat[0];

    while (spriteNum >= 0)
    {
        int const nextSprite = nextspritestat[spriteNum];

        if (sprite[spriteNum].xrepeat == 0)
            A_DeleteSprite(spriteNum);

        spriteNum = nextSprite;
    }
}

void G_MoveWorld(void)
{
    extern double g_moveActorsTime;

    VM_OnEvent(EVENT_PREWORLD, -1, -1);

    G_DoEventGame(EVENT_PREGAME);

    G_MoveZombieActors();     //ST 2
    G_MoveWeapons();          //ST 4
    G_MoveTransports();       //ST 9

    G_MovePlayers();          //ST 10
    G_MoveFallers();          //ST 12
    G_MoveMisc();             //ST 5

    double t = gethiticks();

    G_MoveActors();           //ST 1

    g_moveActorsTime = (1-0.033)*g_moveActorsTime + 0.033*(gethiticks()-t);

    // XXX: Has to be before effectors, in particular movers?
    // TODO: lights in moving sectors ought to be interpolated
    G_DoEffectorLights();
    G_MoveEffectors();        //ST 3
    G_MoveStandables();       //ST 6


    VM_OnEvent(EVENT_WORLD, -1, -1);

    G_DoEventGame(EVENT_GAME);

    G_RefreshLights();
    G_DoSectorAnimations();
    G_MoveFX();               //ST 11
}
