//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

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

#include "duke3d.h"
#include "actors.h"
#include "gamedef.h"
#include "gameexec.h"

#if KRANDDEBUG
# define ACTOR_STATIC
#else
# define ACTOR_STATIC static
#endif

#define KILLIT(KX) do { A_DeleteSprite(KX); goto BOLT; } while (0)

extern int32_t g_numEnvSoundsPlaying;
extern int32_t g_noEnemies;

int32_t otherp;

int32_t G_SetInterpolation(int32_t * const posptr)
{
    if (g_numInterpolations >= MAXINTERPOLATIONS)
        return 1;

    for (int i = 0; i < g_numInterpolations; ++i)
        if (curipos[i] == posptr)
            return 0;

    curipos[g_numInterpolations] = posptr;
    oldipos[g_numInterpolations] = *posptr;
    g_numInterpolations++;
    return 0;
}

void G_StopInterpolation(int32_t * const posptr)
{
    for (int i = startofdynamicinterpolations; i < g_numInterpolations; ++i)
        if (curipos[i] == posptr)
        {
            g_numInterpolations--;
            oldipos[i] = oldipos[g_numInterpolations];
            bakipos[i] = bakipos[g_numInterpolations];
            curipos[i] = curipos[g_numInterpolations];
        }
}

void G_DoInterpolations(int32_t smoothratio)       //Stick at beginning of drawscreen
{
    if (g_interpolationLock++)
        return;

    int32_t odelta, ndelta = 0;

    for (int i = 0, j = 0; i < g_numInterpolations; ++i)
    {
        odelta = ndelta;
        bakipos[i] = *curipos[i];
        ndelta = (*curipos[i]) - oldipos[i];
        if (odelta != ndelta)
            j = mulscale16(ndelta, smoothratio);
        *curipos[i] = oldipos[i] + j;
    }
}

void G_ClearCameraView(DukePlayer_t *ps)
{
    int32_t k;

    ps->newowner = -1;
    ps->pos = ps->opos;
    ps->ang = ps->oang;

    updatesector(ps->pos.x, ps->pos.y, &ps->cursectnum);
    P_UpdateScreenPal(ps);

    for (SPRITES_OF(STAT_ACTOR, k))
        if (sprite[k].picnum==CAMERA1)
            sprite[k].yvel = 0;
}

// Manhattan distance between wall-point and sprite.
static inline int32_t G_WallSpriteDist(const twalltype *wal, const spritetype *spr)
{
    return klabs(wal->x - spr->x) + klabs(wal->y - spr->y);
}

void A_RadiusDamage(int32_t i, int32_t r, int32_t hp1, int32_t hp2, int32_t hp3, int32_t hp4)
{
    int32_t d, q, stati;
    const spritetype *const s = &sprite[i];

    static const int32_t statlist[] = {
        STAT_DEFAULT, STAT_ACTOR, STAT_STANDABLE,
        STAT_PLAYER, STAT_FALLER, STAT_ZOMBIEACTOR, STAT_MISC
    };

    // XXX: accesses to 'sectorlist' potentially unaligned
    int16_t *const sectorlist = (int16_t *)tempbuf;
    const int32_t maxsects = sizeof(tempbuf)/sizeof(int16_t);

    if (s->picnum == RPG && s->xrepeat < 11)
        goto SKIPWALLCHECK;

    if (s->picnum != SHRINKSPARK)
    {
        int32_t sectcnt = 0;
        int32_t sectend = 1;

        sectorlist[0] = s->sectnum;

        do
        {
            const twalltype *wal;
            const int32_t dasect = sectorlist[sectcnt++];
            const int32_t startwall = sector[dasect].wallptr;
            const int32_t endwall = startwall+sector[dasect].wallnum;

            int32_t w;
            const int32_t w2 = wall[startwall].point2;

            // Check if "hit" 1st or 3rd wall-point. This mainly makes sense
            // for rectangular "ceiling light"-style sectors.
            if (G_WallSpriteDist((twalltype *)&wall[startwall], s) < r ||
                    G_WallSpriteDist((twalltype *)&wall[wall[w2].point2], s) < r)
            {
                if (((sector[dasect].ceilingz-s->z)>>8) < r)
                    Sect_DamageCeilingOrFloor(0, dasect);
                if (((s->z-sector[dasect].floorz)>>8) < r)
                    Sect_DamageCeilingOrFloor(1, dasect);
            }

            for (w=startwall,wal=(twalltype *)&wall[startwall]; w<endwall; w++,wal++)
                if (G_WallSpriteDist(wal, s) < r)
                {
                    int16_t sect = -1;
                    const int32_t nextsect = wal->nextsector;
                    int32_t x1, y1;

                    if (nextsect >= 0)
                    {
                        int32_t dasect2;
                        for (dasect2=sectend-1; dasect2>=0; dasect2--)
                            if (sectorlist[dasect2] == nextsect)
                                break;

                        if (dasect2 < 0)
                        {
                            if (sectend == maxsects)
                                goto SKIPWALLCHECK;  // prevent oob access of 'sectorlist'
                            sectorlist[sectend++] = nextsect;
                        }
                    }

                    x1 = (((wal->x+wall[wal->point2].x)>>1)+s->x)>>1;
                    y1 = (((wal->y+wall[wal->point2].y)>>1)+s->y)>>1;

                    updatesector(x1,y1,&sect);

                    if (sect >= 0 && cansee(x1,y1,s->z,sect,s->x,s->y,s->z,s->sectnum))
                    {
                        vec3_t tmpvect = { wal->x, wal->y, s->z };
                        A_DamageWall(i, w, &tmpvect, s->picnum);
                    }
                }
        }
        while (sectcnt < sectend);
    }

SKIPWALLCHECK:

    q = -(16<<8) + (krand()&((32<<8)-1));

    for (stati=0; stati < ARRAY_SSIZE(statlist); stati++)
    {
        int32_t j = headspritestat[statlist[stati]];

        while (j >= 0)
        {
            const int32_t nextj = nextspritestat[j];
            spritetype *const sj = &sprite[j];

            // DEFAULT, ZOMBIEACTOR, MISC
            if (stati == 0 || stati >= 5 || AFLAMABLE(sj->picnum))
            {
                if (s->picnum != SHRINKSPARK || (sj->cstat&257))
                    if (dist(s, sj) < r)
                    {
                        if (A_CheckEnemySprite(sj) && !cansee(sj->x, sj->y,sj->z+q, sj->sectnum, s->x, s->y, s->z+q, s->sectnum))
                            goto BOLT;
                        A_DamageObject(j, i);
                    }
            }
            else if (sj->extra >= 0 && sj != s && (sj->picnum == TRIPBOMB || A_CheckEnemySprite(sj) || sj->picnum == QUEBALL || sj->picnum == STRIPEBALL || (sj->cstat&257) || sj->picnum == DUKELYINGDEAD))
            {
                if (s->picnum == SHRINKSPARK && sj->picnum != SHARK && (j == s->owner || sj->xrepeat < 24))
                {
                    j = nextj;
                    continue;
                }
                if (s->picnum == MORTER && j == s->owner)
                {
                    j = nextj;
                    continue;
                }

                if (sj->picnum == APLAYER) sj->z -= PHEIGHT;
                d = dist(s, sj);
                if (sj->picnum == APLAYER) sj->z += PHEIGHT;

                if (d < r && cansee(sj->x, sj->y, sj->z-(8<<8), sj->sectnum, s->x, s->y, s->z-(12<<8), s->sectnum))
                {
                    actor[j].ang = getangle(sj->x-s->x,sj->y-s->y);

                    if (s->picnum == RPG && sj->extra > 0)
                        actor[j].picnum = RPG;
                    else if (A_CheckSpriteFlags(i,SFLAG_PROJECTILE) && SpriteProjectile[i].workslike & PROJECTILE_RADIUS_PICNUM && sj->extra > 0)
                        actor[j].picnum = s->picnum;
                    else
                    {
                        if (s->picnum == SHRINKSPARK)
                            actor[j].picnum = SHRINKSPARK;
                        else actor[j].picnum = RADIUSEXPLOSION;
                    }

                    if (s->picnum != SHRINKSPARK)
                    {
                        const int32_t k = r/3;

                        if (d < k)
                        {
                            if (hp4 == hp3) hp4++;
                            actor[j].extra = hp3 + (krand()%(hp4-hp3));
                        }
                        else if (d < k*2)
                        {
                            if (hp3 == hp2) hp3++;
                            actor[j].extra = hp2 + (krand()%(hp3-hp2));
                        }
                        else if (d < r)
                        {
                            if (hp2 == hp1) hp2++;
                            actor[j].extra = hp1 + (krand()%(hp2-hp1));
                        }

                        if (!A_CheckSpriteFlags(j, SFLAG_NODAMAGEPUSH))
                        {
                            if (sj->xvel < 0) sj->xvel = 0;
                            sj->xvel += (s->extra<<2);
                        }

                        if (sj->picnum == PODFEM1 || sj->picnum == FEM1 ||
                                sj->picnum == FEM2 || sj->picnum == FEM3 ||
                                sj->picnum == FEM4 || sj->picnum == FEM5 ||
                                sj->picnum == FEM6 || sj->picnum == FEM7 ||
                                sj->picnum == FEM8 || sj->picnum == FEM9 ||
                                sj->picnum == FEM10 || sj->picnum == STATUE ||
                                sj->picnum == STATUEFLASH || sj->picnum == SPACEMARINE || sj->picnum == QUEBALL || sj->picnum == STRIPEBALL)
                            A_DamageObject(j, i);
                    }
                    else if (s->extra == 0) actor[j].extra = 0;

                    if (sj->picnum != RADIUSEXPLOSION &&
                            s->owner >= 0 && sprite[s->owner].statnum < MAXSTATUS)
                    {
                        if (sj->picnum == APLAYER)
                        {
                            DukePlayer_t *ps = g_player[P_GetP(sj)].ps;

                            if (ps->newowner >= 0)
                                G_ClearCameraView(ps);
                        }

                        actor[j].owner = s->owner;
                    }
                }
            }
BOLT:
            j = nextj;
        }
    }
}

// Maybe do a projectile transport via an SE7.
// <spritenum>: the projectile
// <i>: the SE7
// <fromunderp>: below->above change?
static int32_t Proj_MaybeDoTransport(int32_t spritenum, const tspritetype * const effector, int32_t fromunderp, int32_t daz)
{
    if (totalclock <= actor[spritenum].lasttransport)
        return 0;

    spritetype *const spr = &sprite[spritenum];
    const tspritetype *const otherse = (tspritetype *)&sprite[effector->owner];

    actor[spritenum].lasttransport = totalclock + (TICSPERFRAME<<2);

    spr->x += (otherse->x - effector->x);
    spr->y += (otherse->y - effector->y);

                             // above->below
    spr->z = (!fromunderp) ? sector[otherse->sectnum].ceilingz - daz + sector[effector->sectnum].floorz :
                             sector[otherse->sectnum].floorz - daz + sector[effector->sectnum].ceilingz;
                             // below->above

    actor[spritenum].bpos = *(vec3_t *)&sprite[spritenum];
    changespritesect(spritenum, otherse->sectnum);

    return 1;
}

// Check whether sprite <s> is on/in a non-SE7 water sector.
// <othersectptr>: if not NULL, the sector on the other side.
static int32_t A_CheckNoSE7Water(const spritetype *s, int32_t sectnum, int32_t slotag, int32_t *othersectptr)
{
    if (slotag==ST_1_ABOVE_WATER || slotag==ST_2_UNDERWATER)
    {
        int32_t othersect = yax_getneighborsect(
            s->x, s->y, sectnum, slotag==ST_1_ABOVE_WATER ? YAX_FLOOR : YAX_CEILING);

        int32_t othertag = (slotag==ST_1_ABOVE_WATER) ?
            ST_2_UNDERWATER : ST_1_ABOVE_WATER;

        // If submerging, the lower sector MUST have lotag 2.
        // If emerging, the upper sector MUST have lotag 1.
        // This way, the x/y coordinates where above/below water
        // changes can happen are the same.
        if (othersect >= 0 && sector[othersect].lotag==othertag)
        {
            if (othersectptr)
                *othersectptr = othersect;
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
static int32_t A_CheckNeedZUpdate(int32_t spritenum, int32_t changez, int32_t *dazptr)
{
    const spritetype *spr = &sprite[spritenum];
    const int32_t daz = spr->z + (changez>>1);

    *dazptr = daz;

    if (changez == 0)
        return 0;

    if (daz > actor[spritenum].ceilingz && daz <= actor[spritenum].floorz)
        return 1;

#ifdef YAX_ENABLE
    {
        const int32_t psect=spr->sectnum, slotag=sector[psect].lotag;
        int32_t othersect;

        // Non-SE7 water.
        // PROJECTILE_CHSECT
        if ((changez < 0 && slotag==ST_2_UNDERWATER) || (changez > 0 && slotag==ST_1_ABOVE_WATER))
            if (A_CheckNoSE7Water(spr, sprite[spritenum].sectnum, slotag, &othersect))
            {
                A_Spawn(spritenum, WATERSPLASH2);
                // NOTE: Don't tweak its z position afterwards like with
                // SE7-induced projectile teleportation. It doesn't look good
                // with TROR water.

                actor[spritenum].flags |= SFLAG_DIDNOSE7WATER;
                return -othersect-1;
            }
    }
#endif

    return 0;
}

int32_t A_MoveSpriteClipdist(int32_t spritenum, const vec3_t *change, uint32_t cliptype, int32_t clipdist)
{
    spritetype *const spr = &sprite[spritenum];
    const int32_t badguy = A_CheckEnemySprite(spr);
    const int32_t oldx = spr->x, oldy = spr->y;

    if (spr->statnum == STAT_MISC || (badguy && spr->xrepeat < 4))
    {
        spr->x += change->x;
        spr->y += change->y;
        spr->z += change->z;

        if (badguy)
            setsprite(spritenum, (vec3_t *)spr);

        return 0;
    }

    if (clipdist >= 0)
    {
        // use that value
    }
    else if (badguy)
    {
        if (spr->xrepeat > 60)
            clipdist = 1024;
        else if (spr->picnum == LIZMAN)
            clipdist = 292;
        else if (A_CheckSpriteTileFlags(spr->picnum, SFLAG_BADGUY))
            clipdist = spr->clipdist<<2;
        else
            clipdist = 192;
    }
    else
    {
        if (spr->statnum == STAT_PROJECTILE && (SpriteProjectile[spritenum].workslike & PROJECTILE_REALCLIPDIST) == 0)
            clipdist = 8;
        else
            clipdist = spr->clipdist<<2;
    }

    int16_t dasectnum = spr->sectnum;
    const int16_t osectnum = dasectnum;
    int32_t daz = spr->z - 2*tilesiz[spr->picnum].y*spr->yrepeat;
    const int32_t oldz = spr->z;

    // Handle horizontal movement first.
    spr->z = daz;
    int32_t retval = clipmove((vec3_t *)spr, &dasectnum,
                              change->x<<13, change->y<<13,
                              clipdist, 4<<8, 4<<8, cliptype);
    spr->z = oldz;

    if (badguy)
    {
        // Handle potential stayput condition (map-provided or hard-coded).
        if (dasectnum < 0 ||
                ((actor[spritenum].actorstayput >= 0 && actor[spritenum].actorstayput != dasectnum) ||
                 (spr->picnum == BOSS2 && spr->pal == 0 && sector[dasectnum].lotag != ST_3) ||
                 ((spr->picnum == BOSS1 || spr->picnum == BOSS2) && sector[dasectnum].lotag == ST_1_ABOVE_WATER)
                 || (sector[osectnum].lotag != ST_1_ABOVE_WATER && sector[dasectnum].lotag == ST_1_ABOVE_WATER &&
                     (spr->picnum == LIZMAN || (spr->picnum == LIZTROOP && spr->zvel == 0)))
                )
            )
        {
            spr->x = oldx;
            spr->y = oldy;

            // NOTE: in Duke3D, LIZMAN on water takes on random angle here.

            setsprite(spritenum, (vec3_t *)spr);

            if (dasectnum < 0)
                dasectnum = 0;

            return 16384+dasectnum;
        }

        if ((retval&49152) >= 32768 && actor[spritenum].cgg==0)
            spr->ang += 768;
    }

    if (dasectnum == -1)
    {
        dasectnum = spr->sectnum;
//        OSD_Printf("%s:%d wtf\n",__FILE__,__LINE__);
    }
    else if (dasectnum != spr->sectnum)
    {
        changespritesect(spritenum, dasectnum);
        // A_GetZLimits(spritenum);
    }

    Bassert(dasectnum == spr->sectnum);

    int dozupdate = A_CheckNeedZUpdate(spritenum, change->z, &daz);

    // Update sprite's z positions and (for TROR) maybe the sector number.
    if (dozupdate)
    {
        spr->z = daz;
#ifdef YAX_ENABLE
        if (dozupdate < 0)
        {
            // If we passed a TROR no-SE7 water boundary, signal to the outside
            // that the ceiling/floor was not hit. However, this is not enough:
            // later, code checks for (retval&49152)!=49152
            // [i.e. not "was ceiling or floor hit", but "was no sprite hit"]
            // and calls G_WeaponHitCeilingOrFloor() then, so we need to set
            // actor[].flags |= SFLAG_DIDNOSE7WATER in A_CheckNeedZUpdate()
            // previously.
            // XXX: Why is this contrived data flow necessary? (If at all.)
            changespritesect(spritenum, -dozupdate-1);
            return 0;
        }

        if (yax_getbunch(dasectnum, (change->z>0))>=0
                && (SECTORFLD(dasectnum,stat, (change->z>0))&yax_waltosecmask(cliptype))==0)
        {
            setspritez(spritenum, (vec3_t *)spr);
        }
#endif
    }
    else if (change->z != 0 && retval == 0)
        retval = 16384+dasectnum;

    if (retval == 16384+dasectnum)
        if (spr->statnum == STAT_PROJECTILE)
        {
            int32_t i;

            // Projectile sector changes due to transport SEs (SE7_PROJECTILE).
            // PROJECTILE_CHSECT
            for (SPRITES_OF(STAT_TRANSPORT, i))
                if (sprite[i].sectnum == dasectnum)
                {
                    const int32_t lotag = sector[dasectnum].lotag;

                    if (lotag == ST_1_ABOVE_WATER)
                        if (daz >= actor[spritenum].floorz)
                            if (Proj_MaybeDoTransport(spritenum, (tspritetype *)&sprite[i], 0, daz))
                                return 0;

                    if (lotag == ST_2_UNDERWATER)
                        if (daz <= actor[spritenum].ceilingz)
                            if (Proj_MaybeDoTransport(spritenum, (tspritetype *)&sprite[i], 1, daz))
                                return 0;
                }
        }

    return retval;
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
void A_DeleteSprite(int32_t s)
{
    if (EDUKE32_PREDICT_FALSE(block_deletesprite))
    {
        OSD_Printf(OSD_ERROR "A_DeleteSprite(): tried to remove sprite %d in EVENT_EGS\n", s);
        return;
    }

    if (VM_HaveEvent(EVENT_KILLIT))
    {
        int32_t p, pl = A_FindPlayer(&sprite[s], &p);

        if (VM_OnEventWithDist_(EVENT_KILLIT, s, pl, p))
            return;
    }

#ifdef POLYMER
    if (actor[s].lightptr != NULL && getrendermode() == REND_POLYMER)
        A_DeleteLight(s);
#endif

    // AMBIENT_SFX_PLAYING
    if (sprite[s].picnum == MUSICANDSFX && actor[s].t_data[0] == 1)
        S_StopEnvSound(sprite[s].lotag, s);

    // NetAlloc
    if (Net_IsRelevantSprite(s))
    {
        Net_DeleteSprite(s);
        return;
    }

    deletesprite(s);
}

void A_AddToDeleteQueue(int32_t i)
{
    if (g_spriteDeleteQueueSize == 0)
    {
        A_DeleteSprite(i);
        return;
    }

    if (SpriteDeletionQueue[g_spriteDeleteQueuePos] >= 0)
        sprite[SpriteDeletionQueue[g_spriteDeleteQueuePos]].xrepeat = 0;
    SpriteDeletionQueue[g_spriteDeleteQueuePos] = i;
    g_spriteDeleteQueuePos = (g_spriteDeleteQueuePos+1)%g_spriteDeleteQueueSize;
}

void A_SpawnMultiple(int32_t sp, int32_t pic, int32_t n)
{
    int32_t j;
    spritetype *s = &sprite[sp];

    for (; n>0; n--)
    {
        j = A_InsertSprite(s->sectnum,s->x,s->y,s->z-(krand()%(47<<8)),pic,-32,8,8,krand()&2047,0,0,sp,5);
        A_Spawn(-1, j);
        sprite[j].cstat = krand()&12;
    }
}

void A_DoGuts(int32_t sp, int32_t gtype, int32_t n)
{
    int32_t gutz,floorz;
    int32_t i,a,j,sx = 32,sy = 32;

    const spritetype *const s = &sprite[sp];

    if (A_CheckEnemySprite(s) && s->xrepeat < 16)
        sx = sy = 8;

    gutz = s->z-(8<<8);
    floorz = getflorzofslope(s->sectnum,s->x,s->y);

    if (gutz > (floorz-(8<<8)))
        gutz = floorz-(8<<8);

    if (s->picnum == COMMANDER)
        gutz -= (24<<8);

    for (j=n; j>0; j--)
    {
        a = krand()&2047;
        i = A_InsertSprite(s->sectnum,s->x+(krand()&255)-128,s->y+(krand()&255)-128,gutz-(krand()&8191),gtype,-32,sx,sy,a,48+(krand()&31),-512-(krand()&2047),sp,5);
        if (PN == JIBS2)
        {
            sprite[i].xrepeat >>= 2;
            sprite[i].yrepeat >>= 2;
        }

        sprite[i].pal = s->pal;
    }
}

void A_DoGutsDir(int32_t sp, int32_t gtype, int32_t n)
{
    int32_t gutz,floorz;
    int32_t i,a,j,sx = 32,sy = 32;
    const spritetype *const s = &sprite[sp];

    if (A_CheckEnemySprite(s) && s->xrepeat < 16)
        sx = sy = 8;

    gutz = s->z-(8<<8);
    floorz = getflorzofslope(s->sectnum,s->x,s->y);

    if (gutz > (floorz-(8<<8)))
        gutz = floorz-(8<<8);

    if (s->picnum == COMMANDER)
        gutz -= (24<<8);

    for (j=n; j>0; j--)
    {
        a = krand()&2047;
        i = A_InsertSprite(s->sectnum,s->x,s->y,gutz,gtype,-32,sx,sy,a,256+(krand()&127),-512-(krand()&2047),sp,5);
        sprite[i].pal = s->pal;
    }
}

LUNATIC_EXTERN int32_t G_ToggleWallInterpolation(int32_t w, int32_t doset)
{
    if (doset)
    {
        return G_SetInterpolation(&wall[w].x)
            || G_SetInterpolation(&wall[w].y);
    }
    else
    {
        G_StopInterpolation(&wall[w].x);
        G_StopInterpolation(&wall[w].y);
        return 0;
    }
}

void Sect_ToggleInterpolation(int sectnum, int doset)
{
    int k, j = sector[sectnum].wallptr;
    const int endwall = sector[sectnum].wallptr + sector[sectnum].wallnum;

    for (; j<endwall; j++)
    {
        G_ToggleWallInterpolation(j, doset);

        k = wall[j].nextwall;
        if (k >= 0)
        {
            G_ToggleWallInterpolation(k, doset);
            G_ToggleWallInterpolation(wall[k].point2, doset);
        }
    }
}

static int32_t move_rotfixed_sprite(int32_t j, int32_t pivotspr, int32_t daang)
{
    if ((ROTFIXSPR_STATNUMP(sprite[j].statnum) ||
         ((sprite[j].statnum == STAT_ACTOR || sprite[j].statnum == STAT_ZOMBIEACTOR) &&
          A_CheckSpriteTileFlags(sprite[j].picnum, SFLAG_ROTFIXED))) &&
        actor[j].t_data[7] == (ROTFIXSPR_MAGIC | pivotspr))
    {
        rotatepoint(zerovec, *(vec2_t *)&actor[j].t_data[8], daang & 2047, (vec2_t *)&sprite[j].x);
        sprite[j].x += sprite[pivotspr].x;
        sprite[j].y += sprite[pivotspr].y;
        return 0;
    }

    return 1;
}

static void A_MoveSector(int i)
{
    // T1,T2 and T3 are used for all the sector moving stuff!!!
    spritetype * const s = &sprite[i];
    int j = T2;
    int const k = T3;

    s->x += (s->xvel * (sintable[(s->ang + 512) & 2047])) >> 14;
    s->y += (s->xvel * (sintable[s->ang & 2047])) >> 14;

    const int endwall = sector[s->sectnum].wallptr + sector[s->sectnum].wallnum;

    for (i = sector[s->sectnum].wallptr; i < endwall; i++)
    {
        vec2_t const v = { msx[j], msy[j] };
        vec2_t t;
        rotatepoint(zerovec, v, k & 2047, &t);
        dragpoint(i, s->x + t.x, s->y + t.y, 0);

        j++;
    }
}

#if !defined LUNATIC
// NOTE: T5 is AC_ACTION_ID
# define LIGHTRAD_PICOFS (T5 ? *(script + T5) + (*(script + T5 + 2)) * AC_CURFRAME(actor[i].t_data) : 0)
#else
// startframe + viewtype*[cyclic counter]
# define LIGHTRAD_PICOFS (actor[i].ac.startframe + actor[i].ac.viewtype * AC_CURFRAME(actor[i].t_data))
#endif

// this is the same crap as in game.c's tspr manipulation.  puke.
// XXX: may access tilesizy out-of-bounds by bad user code.
#define LIGHTRAD (s->yrepeat * tilesiz[s->picnum + LIGHTRAD_PICOFS].y)
#define LIGHTRAD2 (((s->yrepeat) + (rand() % (s->yrepeat >> 2))) * tilesiz[s->picnum + LIGHTRAD_PICOFS].y)

void G_AddGameLight(int32_t radius, int32_t srcsprite, int32_t zoffset, int32_t range, int32_t color, int32_t priority)
{
#ifdef POLYMER
    spritetype *s = &sprite[srcsprite];

    if (getrendermode() != REND_POLYMER)
        return;

    if (actor[srcsprite].lightptr == NULL)
    {
#pragma pack(push, 1)
        _prlight mylight;
#pragma pack(pop)
        Bmemset(&mylight, 0, sizeof(mylight));

        mylight.sector = s->sectnum;
        mylight.x = s->x;
        mylight.y = s->y;
        mylight.z = s->z - zoffset;
        mylight.color[0] = color & 255;
        mylight.color[1] = (color >> 8) & 255;
        mylight.color[2] = (color >> 16) & 255;
        mylight.radius = radius;
        actor[srcsprite].lightmaxrange = mylight.range = range;

        mylight.priority = priority;
        mylight.tilenum = 0;

        mylight.publicflags.emitshadow = 1;
        mylight.publicflags.negative = 0;

        actor[srcsprite].lightId = polymer_addlight(&mylight);
        if (actor[srcsprite].lightId >= 0)
            actor[srcsprite].lightptr = &prlights[actor[srcsprite].lightId];
        return;
    }

    s->z -= zoffset;

    if (range<actor[srcsprite].lightmaxrange>> 1)
        actor[srcsprite].lightmaxrange = 0;

    if (range > actor[srcsprite].lightmaxrange || priority != actor[srcsprite].lightptr->priority ||
        Bmemcmp(&sprite[srcsprite], actor[srcsprite].lightptr, sizeof(int32_t) * 3))
    {
        if (range > actor[srcsprite].lightmaxrange)
            actor[srcsprite].lightmaxrange = range;

        Bmemcpy(actor[srcsprite].lightptr, &sprite[srcsprite], sizeof(int32_t) * 3);
        actor[srcsprite].lightptr->sector = s->sectnum;
        actor[srcsprite].lightptr->flags.invalidate = 1;
    }

    actor[srcsprite].lightptr->priority = priority;
    actor[srcsprite].lightptr->range = range;
    actor[srcsprite].lightptr->color[0] = color & 255;
    actor[srcsprite].lightptr->color[1] = (color >> 8) & 255;
    actor[srcsprite].lightptr->color[2] = (color >> 16) & 255;

    s->z += zoffset;

#else
    UNREFERENCED_PARAMETER(radius);
    UNREFERENCED_PARAMETER(srcsprite);
    UNREFERENCED_PARAMETER(zoffset);
    UNREFERENCED_PARAMETER(range);
    UNREFERENCED_PARAMETER(color);
    UNREFERENCED_PARAMETER(priority);
#endif
}

// sleeping monsters, etc
ACTOR_STATIC void G_MoveZombieActors(void)
{
    int32_t i = headspritestat[STAT_ZOMBIEACTOR], j;

    while (i >= 0)
    {
        const int32_t nexti = nextspritestat[i];

        int32_t x;
        spritetype *const s = &sprite[i];
        const int32_t p = A_FindPlayer(s,&x);

        int16_t ssect = s->sectnum;
        int16_t psect = s->sectnum;

        if (sprite[g_player[p].ps->i].extra > 0)
        {
            if (x < 30000)
            {
                actor[i].timetosleep++;
                if (actor[i].timetosleep >= (x>>8))
                {
                    if (A_CheckEnemySprite(s))
                    {
                        const int32_t px = g_player[p].ps->opos.x+64-(krand()&127);
                        const int32_t py = g_player[p].ps->opos.y+64-(krand()&127);
                        int32_t sx, sy;

                        updatesector(px,py,&psect);
                        if (psect == -1)
                        {
                            i = nexti;
                            continue;
                        }

                        sx = s->x+64-(krand()&127);
                        sy = s->y+64-(krand()&127);
                        updatesector(px,py,&ssect);
                        if (ssect == -1)
                        {
                            i = nexti;
                            continue;
                        }

                        j = cansee(sx,sy,s->z-(krand()%(52<<8)),s->sectnum, px,py,
                                   g_player[p].ps->opos.z-(krand()%(32<<8)),g_player[p].ps->cursectnum);
                    }
                    else
                        j = cansee(s->x,s->y,s->z-((krand()&31)<<8),s->sectnum, g_player[p].ps->opos.x,g_player[p].ps->opos.y,
                                   g_player[p].ps->opos.z-((krand()&31)<<8), g_player[p].ps->cursectnum);

                    if (j)
                    {
                        switch (DYNAMICTILEMAP(s->picnum))
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
                            if (sector[s->sectnum].ceilingstat&1 && A_CheckSpriteFlags(i,SFLAG_NOSHADE) == 0)
                                s->shade = sector[s->sectnum].ceilingshade;
                            else s->shade = sector[s->sectnum].floorshade;

                            actor[i].timetosleep = 0;
                            changespritestat(i, STAT_STANDABLE);
                            break;

                        case RECON__STATIC:
                            CS |= 257;
                            // fall-through
                        default:
                            if (A_CheckSpriteFlags(i, SFLAG_USEACTIVATOR) && sector[sprite[i].sectnum].lotag & 16384)
                                break;
                            actor[i].timetosleep = 0;
                            A_PlayAlertSound(i);
                            changespritestat(i, STAT_ACTOR);
                            break;
                        }
                    }
                    else actor[i].timetosleep = 0;
                }
            }

            if (A_CheckEnemySprite(s) && A_CheckSpriteFlags(i,SFLAG_NOSHADE) == 0)
            {
                if (sector[s->sectnum].ceilingstat&1)
                    s->shade = sector[s->sectnum].ceilingshade;
                else s->shade = sector[s->sectnum].floorshade;
            }
        }

        i = nexti;
    }
}

// stupid name, but it's what the function does.
FORCE_INLINE int G_FindExplosionInSector(int sectnum)
{
    for (int SPRITES_OF(STAT_MISC, i))
        if (PN == EXPLOSION2 && sectnum == SECT)
            return i;

    return -1;
}

FORCE_INLINE void P_Nudge(int p, int sn, int shl)
{
    g_player[p].ps->vel.x += actor[sn].extra * (sintable[(actor[sn].ang + 512) & 2047]) << shl;
    g_player[p].ps->vel.y += actor[sn].extra * (sintable[actor[sn].ang & 2047]) << shl;
}

int32_t A_IncurDamage(int32_t sn)
{
    spritetype *const targ = &sprite[sn];
    actor_t *const dmg = &actor[sn];

    // dmg->picnum check: safety, since it might have been set to <0 from CON.
    if (dmg->extra < 0 || targ->extra < 0 || dmg->picnum < 0)
    {
        dmg->extra = -1;
        return -1;
    }

    if (targ->picnum == APLAYER)
    {
        const int p = P_GetP(targ);

        if (ud.god && dmg->picnum != SHRINKSPARK)
            return -1;

        if (dmg->owner >= 0 && ud.ffire == 0 && sprite[dmg->owner].picnum == APLAYER &&
            (GametypeFlags[ud.coop] & GAMETYPE_PLAYERSFRIENDLY ||
             (GametypeFlags[ud.coop] & GAMETYPE_TDM && g_player[p].ps->team == g_player[P_Get(dmg->owner)].ps->team)))
            return -1;

        targ->extra -= dmg->extra;

        if (dmg->owner >= 0 && targ->extra <= 0 && dmg->picnum != FREEZEBLAST)
        {
            const int32_t ow = dmg->owner;

            targ->extra = 0;

            g_player[p].ps->wackedbyactor = ow;

            if (sprite[ow].picnum == APLAYER && p != P_Get(ow))
                g_player[p].ps->frag_ps = P_Get(ow);

            dmg->owner = g_player[p].ps->i;
        }

        switch (DYNAMICTILEMAP(dmg->picnum))
        {
            case RADIUSEXPLOSION__STATIC:
            case RPG__STATIC:
            case HYDRENT__STATIC:
            case HEAVYHBOMB__STATIC:
            case SEENINE__STATIC:
            case OOZFILTER__STATIC:
            case EXPLODINGBARREL__STATIC:
                P_Nudge(p, sn, 2);
                break;

            default:
                if (A_CheckSpriteTileFlags(dmg->picnum, SFLAG_PROJECTILE) &&
                    (SpriteProjectile[sn].workslike & PROJECTILE_RPG))
                    P_Nudge(p, sn, 2);
                else
                    P_Nudge(p, sn, 1);
                break;
        }

        dmg->extra = -1;
        return dmg->picnum;
    }

    if (dmg->extra == 0 && dmg->picnum == SHRINKSPARK && targ->xrepeat < 24)
        return -1;

    targ->extra -= dmg->extra;

    if (targ->picnum != RECON && targ->owner >= 0 && sprite[targ->owner].statnum < MAXSTATUS)
        targ->owner = dmg->owner;

    dmg->extra = -1;

    return dmg->picnum;
}

void A_MoveCyclers(void)
{
    int32_t i;

    for (i=g_numCyclers-1; i>=0; i--)
    {
        int16_t *const c = cyclers[i];
        const int32_t sect = c[0];
        const int32_t t = c[3];
        int32_t j = t + (sintable[c[1]&2047]>>10);
        int32_t cshade = c[2];

        if (j < cshade)
            j = cshade;
        else if (j > t)
            j = t;

        c[1] += sector[sect].extra;

        if (c[5])
        {
            walltype *wal = &wall[sector[sect].wallptr];
            int32_t x;

            for (x = sector[sect].wallnum; x>0; x--,wal++)
            {
                if (wal->hitag != 1)
                {
                    wal->shade = j;

                    if ((wal->cstat&2) && wal->nextwall >= 0)
                        wall[wal->nextwall].shade = j;
                }
            }

            sector[sect].floorshade = sector[sect].ceilingshade = j;
        }
    }
}

void A_MoveDummyPlayers(void)
{
    int32_t i = headspritestat[STAT_DUMMYPLAYER];

    while (i >= 0)
    {
        const int32_t p = P_Get(OW);
        DukePlayer_t *const ps = g_player[p].ps;

        const int32_t nexti = nextspritestat[i];
        const int32_t psectnum = ps->cursectnum;

        if (ps->on_crane >= 0 || (psectnum >= 0 && sector[psectnum].lotag != ST_1_ABOVE_WATER) || sprite[ps->i].extra <= 0)
        {
            ps->dummyplayersprite = -1;
            KILLIT(i);
        }
        else
        {
            if (ps->on_ground && ps->on_warping_sector == 1 && psectnum >= 0 && sector[psectnum].lotag == ST_1_ABOVE_WATER)
            {
                CS = 257;
                SZ = sector[SECT].ceilingz+(27<<8);
                SA = ps->ang;
                if (T1 == 8)
                    T1 = 0;
                else T1++;
            }
            else
            {
                if (sector[SECT].lotag != ST_2_UNDERWATER) SZ = sector[SECT].floorz;
                CS = 32768;
            }
        }

        SX += (ps->pos.x-ps->opos.x);
        SY += (ps->pos.y-ps->opos.y);
        setsprite(i, (vec3_t *)&sprite[i]);

BOLT:
        i = nexti;
    }
}


static int32_t P_Submerge(int32_t j, int32_t p, DukePlayer_t *ps, int32_t sect, int32_t othersect);
static int32_t P_Emerge(int32_t j, int32_t p, DukePlayer_t *ps, int32_t sect, int32_t othersect);
static void P_FinishWaterChange(int32_t j, DukePlayer_t *ps, int32_t sectlotag, int32_t ow, int32_t newsectnum);

ACTOR_STATIC void G_MovePlayers(void)
{
    int32_t i = headspritestat[STAT_PLAYER];

    while (i >= 0)
    {
        const int32_t nexti = nextspritestat[i];

        spritetype *const s = &sprite[i];
        DukePlayer_t *const p = g_player[P_GetP(s)].ps;

        if (s->owner >= 0)
        {
            if (p->newowner >= 0)  //Looking thru the camera
            {
                s->x = p->opos.x;
                s->y = p->opos.y;
                actor[i].bpos.z = s->z = p->opos.z+PHEIGHT;
                s->ang = p->oang;
                setsprite(i,(vec3_t *)s);
            }
            else
            {
                int32_t otherx;
#ifdef YAX_ENABLE
                // TROR water submerge/emerge
                const int32_t psect=s->sectnum, slotag=sector[psect].lotag;
                int32_t othersect;

                if (A_CheckNoSE7Water(s, psect, slotag, &othersect))
                {
                    int32_t k = 0;

                    // NOTE: Compare with G_MoveTransports().
                    p->on_warping_sector = 1;

                    if (slotag==ST_1_ABOVE_WATER)
                        k = P_Submerge(i, P_GetP(s), p, psect, othersect);
                    else
                        k = P_Emerge(i, P_GetP(s), p, psect, othersect);

                    if (k == 1)
                        P_FinishWaterChange(i, p, slotag, -1, othersect);
                }
#endif
                if (g_netServer || ud.multimode > 1)
                    otherp = P_FindOtherPlayer(P_GetP(s), &otherx);
                else
                {
                    otherp = P_GetP(s);
                    otherx = 0;
                }

                if (G_HaveActor(sprite[i].picnum))
                    A_Execute(i, P_GetP(s), otherx);

                if (g_netServer || ud.multimode > 1)
                    if (sprite[g_player[otherp].ps->i].extra > 0)
                    {
                        if (s->yrepeat > 32 && sprite[g_player[otherp].ps->i].yrepeat < 32)
                        {
                            if (otherx < 1400 && p->knee_incs == 0)
                            {
                                p->knee_incs = 1;
                                p->weapon_pos = -1;
                                p->actorsqu = g_player[otherp].ps->i;
                            }
                        }
                    }

                if (ud.god)
                {
                    s->extra = p->max_player_health;
                    s->cstat = 257;
                    p->inv_amount[GET_JETPACK] = 1599;
                }

                if (s->extra > 0)
                {
                    actor[i].owner = i;

                    if (ud.god == 0)
                        if (G_CheckForSpaceCeiling(s->sectnum) || G_CheckForSpaceFloor(s->sectnum))
                            P_QuickKill(p);
                }
                else
                {
                    p->pos.x = s->x;
                    p->pos.y = s->y;
                    p->pos.z = s->z-(20<<8);

                    p->newowner = -1;

                    if (p->wackedbyactor >= 0 && sprite[p->wackedbyactor].statnum < MAXSTATUS)
                    {
                        p->ang += G_GetAngleDelta(p->ang,getangle(sprite[p->wackedbyactor].x-p->pos.x,sprite[p->wackedbyactor].y-p->pos.y))>>1;
                        p->ang &= 2047;
                    }
                }

                s->ang = p->ang;
            }
        }
        else
        {
            if (p->holoduke_on == -1)
                KILLIT(i);

            Bmemcpy(&actor[i].bpos, s, sizeof(vec3_t));
            s->cstat = 0;

            if (s->xrepeat < 42)
            {
                s->xrepeat += 4;
                s->cstat |= 2;
            }
            else s->xrepeat = 42;

            if (s->yrepeat < 36)
                s->yrepeat += 4;
            else
            {
                s->yrepeat = 36;
                if (sector[s->sectnum].lotag != ST_2_UNDERWATER)
                    A_Fall(i);
                if (s->zvel == 0 && sector[s->sectnum].lotag == ST_1_ABOVE_WATER)
                    s->z += (32<<8);
            }

            if (s->extra < 8)
            {
                s->xvel = 128;
                s->ang = p->ang;
                s->extra++;
                A_SetSprite(i,CLIPMASK0);
            }
            else
            {
                s->ang = 2047-p->ang;
                setsprite(i,(vec3_t *)s);
            }
        }

        if (sector[s->sectnum].ceilingstat&1)
            s->shade = logapproach(s->shade, sector[s->sectnum].ceilingshade);
        else
            s->shade = logapproach(s->shade, sector[s->sectnum].floorshade);

BOLT:
        i = nexti;
    }
}

ACTOR_STATIC void G_MoveFX(void)
{
    int32_t i = headspritestat[STAT_FX];

    while (i >= 0)
    {
        spritetype *const s = &sprite[i];
        const int32_t nexti = nextspritestat[i];

        switch (DYNAMICTILEMAP(s->picnum))
        {
        case RESPAWN__STATIC:
            if (sprite[i].extra == 66)
            {
                /*int32_t j =*/ A_Spawn(i,SHT);
                //                    sprite[j].pal = sprite[i].pal;
                KILLIT(i);
            }
            else if (sprite[i].extra > (66-13))
                sprite[i].extra++;
            break;

        case MUSICANDSFX__STATIC:
        {
            const int32_t ht = s->hitag;
            DukePlayer_t *const peekps = g_player[screenpeek].ps;

            if (T2 != ud.config.SoundToggle)
            {
                // If sound playback was toggled, restart.
                T2 = ud.config.SoundToggle;
                T1 = 0;
            }

            if (s->lotag >= 1000 && s->lotag < 2000)
            {
                int32_t x = ldist(&sprite[peekps->i],s);

#ifdef SPLITSCREEN_MOD_HACKS
                if (g_fakeMultiMode==2)
                {
                    // HACK for splitscreen mod
                    int32_t otherdist = ldist(&sprite[g_player[1].ps->i],s);
                    x = min(x, otherdist);
                }
#endif

                if (x < ht && T1 == 0)
                {
                    FX_SetReverb(s->lotag - 1000);
                    T1 = 1;
                }
                if (x >= ht && T1 == 1)
                {
                    FX_SetReverb(0);
                    FX_SetReverbDelay(0);
                    T1 = 0;
                }
            }
            else if (s->lotag < 999 && (unsigned)sector[s->sectnum].lotag < 9 &&  // ST_9_SLIDING_ST_DOOR
                         ud.config.AmbienceToggle && sector[SECT].floorz != sector[SECT].ceilingz)
            {
                if (g_sounds[s->lotag].m & SF_MSFX)
                {
                    int32_t x = dist(&sprite[peekps->i],s);

#ifdef SPLITSCREEN_MOD_HACKS
                    if (g_fakeMultiMode==2)
                    {
                        // HACK for splitscreen mod
                        int32_t otherdist = dist(&sprite[g_player[1].ps->i],s);
                        x = min(x, otherdist);
                    }
#endif

                    if (x < ht && T1 == 0 && FX_VoiceAvailable(g_sounds[s->lotag].pr-1))
                    {
                        // Start playing an ambience sound.

                        char om = g_sounds[s->lotag].m;
                        if (g_numEnvSoundsPlaying == ud.config.NumVoices)
                        {
                            int32_t j;

                            for (SPRITES_OF(STAT_FX, j))
                                if (j != i && S_IsAmbientSFX(j) && actor[j].t_data[0] == 1 &&
                                        dist(&sprite[j], &sprite[peekps->i]) > x)
                                {
                                    S_StopEnvSound(sprite[j].lotag,j);
                                    break;
                                }

                            if (j == -1)
                                goto BOLT;
                        }

                        g_sounds[s->lotag].m |= SF_LOOP;
                        A_PlaySound(s->lotag,i);
                        g_sounds[s->lotag].m = om;
                        T1 = 1;  // AMBIENT_SFX_PLAYING
                    }
                    else if (x >= ht && T1 == 1)
                    {
                        // Stop playing ambience sound because we're out of its range.

                        // T1 will be reset in sounds.c: CLEAR_SOUND_T0
                        // T1 = 0;
                        S_StopEnvSound(s->lotag,i);
                    }
                }

                if (g_sounds[s->lotag].m & SF_GLOBAL)
                {
                    // Randomly playing global sounds (flyby of planes, screams, ...)

                    if (T5 > 0)
                        T5--;
                    else
                    {
                        int32_t p;
                        for (TRAVERSE_CONNECT(p))
                            if (p == myconnectindex && g_player[p].ps->cursectnum == s->sectnum)
                            {
                                S_PlaySound(s->lotag + (unsigned)g_globalRandom % (s->hitag+1));
                                T5 = GAMETICSPERSEC*40 + g_globalRandom%(GAMETICSPERSEC*40);
                            }
                    }
                }
            }
            break;
        }
        }
BOLT:
        i = nexti;
    }
}

ACTOR_STATIC void G_MoveFallers(void)
{
    int32_t i = headspritestat[STAT_FALLER];

    while (i >= 0)
    {
        const int32_t nexti = nextspritestat[i];
        spritetype *const s = &sprite[i];

        const int32_t sect = s->sectnum;

        if (T1 == 0)
        {
            int32_t j;
            const int32_t oextra = s->extra;

            s->z -= (16<<8);
            T2 = s->ang;
            if ((j = A_IncurDamage(i)) >= 0)
            {
                if (j == FIREEXT || j == RPG || j == RADIUSEXPLOSION || j == SEENINE || j == OOZFILTER)
                {
                    if (s->extra <= 0)
                    {
                        T1 = 1;

                        for (SPRITES_OF(STAT_FALLER, j))
                        {
                            if (sprite[j].hitag == SHT)
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
                    actor[i].extra = 0;
                    s->extra = oextra;
                }
            }
            s->ang = T2;
            s->z += (16<<8);
        }
        else if (T1 == 1)
        {
            if ((int16_t)s->lotag > 0)
            {
                s->lotag-=3;
                if ((int16_t)s->lotag <= 0)
                {
                    s->xvel = (32+(krand()&63));
                    s->zvel = -(1024+(krand()&1023));
                }
            }
            else
            {
                int32_t x;

                if (s->xvel > 0)
                {
                    s->xvel -= 8;
                    A_SetSprite(i,CLIPMASK0);
                }

                if (EDUKE32_PREDICT_FALSE(G_CheckForSpaceFloor(s->sectnum)))
                    x = 0;
                else
                {
                    if (EDUKE32_PREDICT_FALSE(G_CheckForSpaceCeiling(s->sectnum)))
                        x = g_spriteGravity/6;
                    else
                        x = g_spriteGravity;
                }

                if (s->z < (sector[sect].floorz-ZOFFSET))
                {
                    s->zvel += x;
                    if (s->zvel > 6144)
                        s->zvel = 6144;
                    s->z += s->zvel;
                }
                if ((sector[sect].floorz-s->z) < (16<<8))
                {
                    int32_t j = 1+(krand()&7);
                    for (x=0; x<j; x++) RANDOMSCRAP;
                    KILLIT(i);
                }
            }
        }

BOLT:
        i = nexti;
    }
}

ACTOR_STATIC void G_MoveStandables(void)
{
    int32_t i = headspritestat[STAT_STANDABLE], j, switchpicnum;
    int32_t l=0, x;

    while (i >= 0)
    {
        const int32_t nexti = nextspritestat[i];

        int32_t *const t = &actor[i].t_data[0];
        spritetype *const s = &sprite[i];
        const int32_t sect = s->sectnum;

        if (sect < 0)
            KILLIT(i);

        // Rotation-fixed sprites in rotating sectors already have bpos* updated.
        if ((t[7]&(0xffff0000))!=ROTFIXSPR_MAGIC)
            Bmemcpy(&actor[i].bpos, s, sizeof(vec3_t));

        if (PN >= CRANE && PN <= CRANE+3)
        {
            int32_t nextj;

            //t[0] = state
            //t[1] = checking sector number

            if (s->xvel) A_GetZLimits(i);

            if (t[0] == 0)   //Waiting to check the sector
            {
                for (SPRITES_OF_SECT_SAFE(t[1], j, nextj))
                {
                    switch (sprite[j].statnum)
                    {
                    case STAT_ACTOR:
                    case STAT_ZOMBIEACTOR:
                    case STAT_STANDABLE:
                    case STAT_PLAYER:
                    {
                        vec3_t vect = { msx[t[4]+1], msy[t[4]+1], sprite[j].z };

                        s->ang = getangle(vect.x-s->x, vect.y-s->y);
                        setsprite(j, &vect);
                        t[0]++;
                        goto BOLT;
                    }
                    }
                }
            }

            else if (t[0]==1)
            {
                if (s->xvel < 184)
                {
                    s->picnum = CRANE+1;
                    s->xvel += 8;
                }
                A_SetSprite(i,CLIPMASK0);
                if (sect == t[1])
                    t[0]++;
            }
            else if (t[0]==2 || t[0]==7)
            {
                s->z += (1024+512);

                if (t[0]==2)
                {
                    if (sector[sect].floorz - s->z < (64<<8))
                        if (s->picnum > CRANE) s->picnum--;

                    if (sector[sect].floorz - s->z < 4096+1024)
                        t[0]++;
                }

                if (t[0]==7)
                {
                    if (sector[sect].floorz - s->z < (64<<8))
                    {
                        if (s->picnum > CRANE) s->picnum--;
                        else
                        {
                            if (s->owner==-2)
                            {
                                int32_t p = A_FindPlayer(s, NULL);
                                A_PlaySound(DUKE_GRUNT,g_player[p].ps->i);
                                if (g_player[p].ps->on_crane == i)
                                    g_player[p].ps->on_crane = -1;
                            }

                            t[0]++;
                            s->owner = -1;
                        }
                    }
                }
            }
            else if (t[0]==3)
            {
                s->picnum++;
                if (s->picnum == CRANE+2)
                {
                    int32_t p = G_CheckPlayerInSector(t[1]);

                    if (p >= 0 && g_player[p].ps->on_ground)
                    {
                        s->owner = -2;
                        g_player[p].ps->on_crane = i;
                        A_PlaySound(DUKE_GRUNT,g_player[p].ps->i);
                        g_player[p].ps->ang = s->ang+1024;
                    }
                    else
                    {
                        for (SPRITES_OF_SECT(t[1], j))
                        {
                            switch (sprite[j].statnum)
                            {
                            case STAT_ACTOR:
                            case STAT_STANDABLE:
                                s->owner = j;
                                break;
                            }
                        }
                    }

                    t[0]++;//Grabbed the sprite
                    t[2]=0;
                    goto BOLT;
                }
            }
            else if (t[0]==4) //Delay before going up
            {
                t[2]++;
                if (t[2] > 10)
                    t[0]++;
            }
            else if (t[0]==5 || t[0] == 8)
            {
                if (t[0]==8 && s->picnum < (CRANE+2))
                    if ((sector[sect].floorz-s->z) > 8192)
                        s->picnum++;

                if (s->z < msx[t[4]+2])
                {
                    t[0]++;
                    s->xvel = 0;
                }
                else
                    s->z -= (1024+512);
            }
            else if (t[0]==6)
            {
                if (s->xvel < 192)
                    s->xvel += 8;
                s->ang = getangle(msx[t[4]]-s->x,msy[t[4]]-s->y);
                A_SetSprite(i,CLIPMASK0);
                if (((s->x-msx[t[4]])*(s->x-msx[t[4]])+(s->y-msy[t[4]])*(s->y-msy[t[4]])) < (128*128))
                    t[0]++;
            }

            else if (t[0]==9)
                t[0] = 0;

            {
                vec3_t vect;
                Bmemcpy(&vect,s,sizeof(vec3_t));
                vect.z -= (34<<8);
                setsprite(msy[t[4]+2],&vect);
            }


            if (s->owner != -1)
            {
                int32_t p = A_FindPlayer(s, NULL);

                if (A_IncurDamage(i) >= 0)
                {
                    if (s->owner == -2)
                        if (g_player[p].ps->on_crane == i)
                            g_player[p].ps->on_crane = -1;
                    s->owner = -1;
                    s->picnum = CRANE;
                    goto BOLT;
                }

                if (s->owner >= 0)
                {
                    setsprite(s->owner,(vec3_t *)s);

                    Bmemcpy(&actor[s->owner].bpos, s, sizeof(vec3_t));

                    s->zvel = 0;
                }
                else if (s->owner == -2)
                {
                    DukePlayer_t *const ps = g_player[p].ps;

                    ps->opos.x = ps->pos.x = s->x-(sintable[(ps->ang+512)&2047]>>6);
                    ps->opos.y = ps->pos.y = s->y-(sintable[ps->ang&2047]>>6);
                    ps->opos.z = ps->pos.z = s->z+(2<<8);

                    setsprite(ps->i, (vec3_t *)ps);
                    ps->cursectnum = sprite[ps->i].sectnum;
                }
            }

            goto BOLT;
        }

        if (PN >= WATERFOUNTAIN && PN <= WATERFOUNTAIN+3)
        {
            if (t[0] > 0)
            {
                if (t[0] < 20)
                {
                    t[0]++;

                    s->picnum++;

                    if (s->picnum == (WATERFOUNTAIN+3))
                        s->picnum = WATERFOUNTAIN+1;
                }
                else
                {
                    A_FindPlayer(s,&x);

                    if (x > 512)
                    {
                        t[0] = 0;
                        s->picnum = WATERFOUNTAIN;
                    }
                    else t[0] = 1;
                }
            }
            goto BOLT;
        }

        if (AFLAMABLE(s->picnum))
        {
            if (T1 == 1)
            {
                T2++;
                if ((T2&3) > 0) goto BOLT;

                if (s->picnum == TIRE && T2 == 32)
                {
                    s->cstat = 0;
                    j = A_Spawn(i,BLOODPOOL);
                    sprite[j].shade = 127;
                }
                else
                {
                    if (s->shade < 64) s->shade++;
                    else KILLIT(i);
                }

                j = s->xrepeat-(krand()&7);
                if (j < 10)
                    KILLIT(i);

                s->xrepeat = j;

                j = s->yrepeat-(krand()&7);
                if (j < 4)
                    KILLIT(i);

                s->yrepeat = j;
            }
            if (s->picnum == BOX)
            {
                A_Fall(i);
                actor[i].ceilingz = sector[s->sectnum].ceilingz;
            }
            goto BOLT;
        }

        if (s->picnum == TRIPBOMB)
        {
            // TIMER_CONTROL
            if (actor[i].t_data[6] == 1)
            {

                if (actor[i].t_data[7] >= 1)
                {
                    actor[i].t_data[7]--;
                }

                if (actor[i].t_data[7] <= 0)
                {
                    T3=16;
                    actor[i].t_data[6]=3;
                    A_PlaySound(LASERTRIP_ARMING,i);
                }
                // we're on a timer....
            }
            if (T3 > 0 && actor[i].t_data[6] == 3)
            {
                T3--;

                if (T3 == 8)
                {
                    for (j=0; j<5; j++) RANDOMSCRAP;
                    x = s->extra;
                    A_RadiusDamage(i, g_tripbombBlastRadius, x>>2,x>>1,x-(x>>2),x);

                    j = A_Spawn(i,EXPLOSION2);
                    A_PlaySound(LASERTRIP_EXPLODE,j);
                    sprite[j].ang = s->ang;
                    sprite[j].xvel = 348;
                    A_SetSprite(j,CLIPMASK0);

                    for (SPRITES_OF(STAT_MISC, j))
                    {
                        if (sprite[j].picnum == LASERLINE && s->hitag == sprite[j].hitag)
                            sprite[j].xrepeat = sprite[j].yrepeat = 0;
                    }

                    KILLIT(i);
                }
                goto BOLT;
            }
            else
            {
                const int32_t oextra = s->extra;
                s->extra = 1;
                l = s->ang;
                if (A_IncurDamage(i) >= 0)
                {
                    actor[i].t_data[6] = 3;
                    T3 = 16;
                }
                s->extra = oextra;
                s->ang = l;
            }

            switch (T1)
            {
            default:
                A_FindPlayer(s,&x);
                if (x > 768 || T1 > 16) T1++;
                break;

            case 32:
            {
                int16_t m;

                l = s->ang;
                s->ang = T6;

                T4 = s->x;
                T5 = s->y;

                s->x += sintable[(T6+512)&2047]>>9;
                s->y += sintable[(T6)&2047]>>9;
                s->z -= (3<<8);

                setsprite(i,(vec3_t *)s);

                x = A_CheckHitSprite(i, &m);

                actor[i].lastvx = x;

                s->ang = l;

                //                if(lTripBombControl & TRIPBOMB_TRIPWIRE)
                if (actor[i].t_data[6] != 1)
                {
                    // we're on a trip wire
                    int16_t cursectnum;

                    while (x > 0)
                    {
                        j = A_Spawn(i,LASERLINE);
                        setsprite(j,(vec3_t *)&sprite[j]);
                        sprite[j].hitag = s->hitag;
                        actor[j].t_data[1] = sprite[j].z;

                        s->x += sintable[(T6+512)&2047]>>4;
                        s->y += sintable[(T6)&2047]>>4;

                        if (x < 1024)
                        {
                            sprite[j].xrepeat = x>>5;
                            break;
                        }
                        x -= 1024;

                        cursectnum = s->sectnum;
                        updatesector(s->x, s->y, &cursectnum);
                        if (cursectnum < 0)
                            break;
                    }
                }

                T1++;

                s->x = T4;
                s->y = T5;
                s->z += (3<<8);

                setsprite(i,(vec3_t *)s);
                T4 = T3 = 0;

                if (m >= 0 && actor[i].t_data[6] != 1)
                {
                    actor[i].t_data[6] = 3;
                    T3 = 13;
                    A_PlaySound(LASERTRIP_ARMING,i);
                }
                break;
            }

            case 33:
                T2++;

                T4 = s->x;
                T5 = s->y;

                s->x += sintable[(T6+512)&2047]>>9;
                s->y += sintable[(T6)&2047]>>9;
                s->z -= (3<<8);

                setsprite(i,(vec3_t *)s);

                x = A_CheckHitSprite(i, NULL);

                s->x = T4;
                s->y = T5;
                s->z += (3<<8);
                setsprite(i,(vec3_t *)s);

//                if( Actor[i].lastvx != x && lTripBombControl & TRIPBOMB_TRIPWIRE)
                if (actor[i].lastvx != x && actor[i].t_data[6] != 1)
                {
                    actor[i].t_data[6] = 3;
                    T3 = 13;
                    A_PlaySound(LASERTRIP_ARMING,i);
                }
                break;
            }

            goto BOLT;
        }

        if (s->picnum >= CRACK1 && s->picnum <= CRACK4)
        {
            if (s->hitag > 0)
            {
                int32_t k;

                t[0] = s->cstat;
                t[1] = s->ang;

                k = A_IncurDamage(i);
                if (k < 0)
                    goto crack_default;

                switch (DYNAMICTILEMAP(k))
                {
                case FIREEXT__STATIC:
                case RPG__STATIC:
                case RADIUSEXPLOSION__STATIC:
                case SEENINE__STATIC:
                case OOZFILTER__STATIC:
                    for (SPRITES_OF(STAT_STANDABLE, j))
                    {
                        if (s->hitag == sprite[j].hitag && (sprite[j].picnum == OOZFILTER || sprite[j].picnum == SEENINE))
                            if (sprite[j].shade != -32)
                                sprite[j].shade = -32;
                    }

                    goto DETONATE;

                crack_default:
                default:
                    s->cstat = t[0];
                    s->ang = t[1];
                    s->extra = 0;

                    goto BOLT;
                }
            }
            goto BOLT;
        }

        if (s->picnum == FIREEXT)
        {
            int32_t k;

            if (A_IncurDamage(i) < 0)
                goto BOLT;

            for (k=0; k<16; k++)
            {
                j = A_InsertSprite(SECT,SX,SY,SZ-(krand()%(48<<8)),SCRAP3+(krand()&3),-8,48,48,krand()&2047,(krand()&63)+64,-(krand()&4095)-(sprite[i].zvel>>2),i,5);
                sprite[j].pal = 2;
            }

            j = A_Spawn(i,EXPLOSION2);
            A_PlaySound(PIPEBOMB_EXPLODE,j);
            A_PlaySound(GLASS_HEAVYBREAK,j);

            if ((int16_t)s->hitag > 0)
            {
                for (SPRITES_OF(STAT_STANDABLE, j))
                {
                    // XXX: This block seems to be CODEDUP'd a lot of times.
                    if (s->hitag == sprite[j].hitag && (sprite[j].picnum == OOZFILTER || sprite[j].picnum == SEENINE))
                        if (sprite[j].shade != -32)
                            sprite[j].shade = -32;
                }

                x = s->extra;
                A_RadiusDamage(i, g_pipebombBlastRadius,x>>2, x-(x>>1),x-(x>>2), x);
                j = A_Spawn(i,EXPLOSION2);
                A_PlaySound(PIPEBOMB_EXPLODE,j);

                goto DETONATE;
            }
            else
            {
                A_RadiusDamage(i,g_seenineBlastRadius,10,15,20,25);
                KILLIT(i);
            }
            goto BOLT;
        }

        if (s->picnum == OOZFILTER || s->picnum == SEENINE || s->picnum == SEENINEDEAD || s->picnum == SEENINEDEAD+1)
        {
            if (s->shade != -32 && s->shade != -33)
            {
                if (s->xrepeat)
                    j = (A_IncurDamage(i) >= 0);
                else
                    j = 0;

                if (j || s->shade == -31)
                {
                    if (j) s->lotag = 0;

                    t[3] = 1;

                    for (SPRITES_OF(STAT_STANDABLE, j))
                    {
                        if (s->hitag == sprite[j].hitag && (sprite[j].picnum == SEENINE || sprite[j].picnum == OOZFILTER))
                            sprite[j].shade = -32;
                    }
                }
            }
            else
            {
                if (s->shade == -32)
                {
                    if ((int16_t)s->lotag > 0)
                    {
                        s->lotag -= 3;
                        if ((int16_t)s->lotag <= 0)
                            s->lotag = (uint16_t)(-99);
                    }
                    else
                        s->shade = -33;
                }
                else
                {
                    if (s->xrepeat > 0)
                    {
                        T3++;
                        if (T3 == 3)
                        {
                            if (s->picnum == OOZFILTER)
                            {
                                T3 = 0;
                                goto DETONATE;
                            }

                            if (s->picnum != (SEENINEDEAD+1))
                            {
                                T3 = 0;

                                if (s->picnum == SEENINEDEAD)
                                    s->picnum++;
                                else if (s->picnum == SEENINE)
                                    s->picnum = SEENINEDEAD;
                            }
                            else goto DETONATE;
                        }
                        goto BOLT;
                    }

DETONATE:
                    g_earthquakeTime = 16;

                    for (SPRITES_OF(STAT_EFFECTOR, j))
                    {
                        if (s->hitag == sprite[j].hitag)
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

                    s->z -= (32<<8);

                    if (s->xrepeat)
                        for (x=0; x<8; x++) RANDOMSCRAP;

                    if ((t[3] == 1 && s->xrepeat) || (int16_t)s->lotag == -99)
                    {
                        int32_t j = A_Spawn(i,EXPLOSION2);
                        x = s->extra;
                        A_RadiusDamage(i,g_seenineBlastRadius,x>>2, x-(x>>1),x-(x>>2), x);
                        A_PlaySound(PIPEBOMB_EXPLODE,j);
                    }

                    KILLIT(i);
                }
            }
            goto BOLT;
        }

        if (s->picnum == MASTERSWITCH)
        {
            if (s->yvel == 1)
            {
                s->hitag--;
                if ((int16_t)s->hitag <= 0)
                {
                    G_OperateSectors(sect,i);

                    for (SPRITES_OF_SECT(sect, j))
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

                    KILLIT(i);
                }
            }
            goto BOLT;
        }

        switchpicnum = s->picnum;

        if (switchpicnum > SIDEBOLT1 && switchpicnum <= SIDEBOLT1+3)
            switchpicnum = SIDEBOLT1;
        else if (switchpicnum > BOLT1 && switchpicnum <= BOLT1+3)
            switchpicnum = BOLT1;

        switch (DYNAMICTILEMAP(switchpicnum))
        {
        case VIEWSCREEN__STATIC:
        case VIEWSCREEN2__STATIC:

            if (s->xrepeat == 0)
                KILLIT(i);

            {
                const int32_t p = A_FindPlayer(s, &x);
                const DukePlayer_t *const ps = g_player[p].ps;

                if (dist(&sprite[ps->i], s) < VIEWSCREEN_ACTIVE_DISTANCE)
                {
#if 0
                    if (sprite[i].yvel == 1)  // VIEWSCREEN_YVEL
                        g_curViewscreen = i;
#endif
                }
                else if (g_curViewscreen == i /*&& T1 == 1*/)
                {
                    g_curViewscreen = -1;
                    sprite[i].yvel = 0;  // VIEWSCREEN_YVEL
                    T1 = 0;

                    for (int ii=0; ii < VIEWSCREENFACTOR; ii++)
                        walock[TILE_VIEWSCR-ii] = 199;
                }
            }

            goto BOLT;

        case TRASH__STATIC:

            if (s->xvel == 0) s->xvel = 1;
            if (A_SetSprite(i, CLIPMASK0))
            {
                A_Fall(i);
                if (krand()&1) s->zvel -= 256;
                if ((s->xvel) < 48)
                    s->xvel += (krand()&3);
            }
            else KILLIT(i);
            break;

        case SIDEBOLT1__STATIC:
            //        case SIDEBOLT1+1:
            //        case SIDEBOLT1+2:
            //        case SIDEBOLT1+3:
            A_FindPlayer(s, &x);
            if (x > 20480) goto BOLT;

CLEAR_THE_BOLT2:
            if (t[2])
            {
                t[2]--;
                goto BOLT;
            }
            if ((s->xrepeat|s->yrepeat) == 0)
            {
                s->xrepeat=t[0];
                s->yrepeat=t[1];
            }
            if ((krand()&8) == 0)
            {
                t[0]=s->xrepeat;
                t[1]=s->yrepeat;
                t[2] = g_globalRandom&4;
                s->xrepeat=s->yrepeat=0;
                goto CLEAR_THE_BOLT2;
            }
            s->picnum++;

            // NOTE: Um, this 'l' was assigned to last at the beginning of this function.
            // SIDEBOLT1 never gets translucent as a consequence, unlike BOLT1.
            if (l&1) s->cstat ^= 2;

            if ((krand()&1) && sector[sect].floorpicnum == HURTRAIL)
                A_PlaySound(SHORT_CIRCUIT,i);

            if (s->picnum == SIDEBOLT1+4) s->picnum = SIDEBOLT1;

            goto BOLT;

        case BOLT1__STATIC:
            //        case BOLT1+1:
            //        case BOLT1+2:
            //        case BOLT1+3:
            A_FindPlayer(s, &x);
            if (x > 20480) goto BOLT;

            if (t[3] == 0)
                t[3]=sector[sect].floorshade;

CLEAR_THE_BOLT:
            if (t[2])
            {
                t[2]--;
                sector[sect].floorshade = 20;
                sector[sect].ceilingshade = 20;
                goto BOLT;
            }
            if ((s->xrepeat|s->yrepeat) == 0)
            {
                s->xrepeat=t[0];
                s->yrepeat=t[1];
            }
            else if ((krand()&8) == 0)
            {
                t[0]=s->xrepeat;
                t[1]=s->yrepeat;
                t[2] = g_globalRandom&4;
                s->xrepeat=s->yrepeat=0;
                goto CLEAR_THE_BOLT;
            }
            s->picnum++;

            l = g_globalRandom&7;
            s->xrepeat=l+8;

            if (l&1) s->cstat ^= 2;

            if (s->picnum == (BOLT1+1) && (krand()&7) == 0 && sector[sect].floorpicnum == HURTRAIL)
                A_PlaySound(SHORT_CIRCUIT,i);

            if (s->picnum==BOLT1+4) s->picnum=BOLT1;

            if (s->picnum&1)
            {
                sector[sect].floorshade = 0;
                sector[sect].ceilingshade = 0;
            }
            else
            {
                sector[sect].floorshade = 20;
                sector[sect].ceilingshade = 20;
            }
            goto BOLT;

        case WATERDRIP__STATIC:

            if (t[1])
            {
                if (--t[1] == 0)
                    s->cstat &= 32767;
            }
            else
            {
                A_Fall(i);
                A_SetSprite(i,CLIPMASK0);
                if (s->xvel > 0) s->xvel -= 2;

                if (s->zvel == 0)
                {
                    s->cstat |= 32768;

                    if (s->pal != 2 && s->hitag == 0)
                        A_PlaySound(SOMETHING_DRIPPING,i);

                    if (sprite[s->owner].picnum != WATERDRIP)
                    {
                        KILLIT(i);
                    }
                    else
                    {
                        actor[i].bpos.z = s->z = t[0];
                        t[1] = 48+(krand()&31);
                    }
                }
            }


            goto BOLT;

        case DOORSHOCK__STATIC:
            j = klabs(sector[sect].ceilingz-sector[sect].floorz)>>9;
            s->yrepeat = j+4;
            s->xrepeat = 16;
            s->z = sector[sect].floorz;
            goto BOLT;

        case TOUCHPLATE__STATIC:
            if (t[1] == 1 && (int16_t)s->hitag >= 0)  //Move the sector floor
            {
                x = sector[sect].floorz;

                if (t[3] == 1)
                {
                    if (x >= t[2])
                    {
                        sector[sect].floorz = x;
                        t[1] = 0;
                    }
                    else
                    {
                        int32_t p;
                        sector[sect].floorz += sector[sect].extra;
                        p = G_CheckPlayerInSector(sect);
                        if (p >= 0) g_player[p].ps->pos.z += sector[sect].extra;
                    }
                }
                else
                {
                    if (x <= s->z)
                    {
                        sector[sect].floorz = s->z;
                        t[1] = 0;
                    }
                    else
                    {
                        int32_t p;
                        sector[sect].floorz -= sector[sect].extra;
                        p = G_CheckPlayerInSector(sect);
                        if (p >= 0)
                            g_player[p].ps->pos.z -= sector[sect].extra;
                    }
                }
                goto BOLT;
            }

            if (t[5] == 1) goto BOLT;

            {
                int32_t p = G_CheckPlayerInSector(sect);

                if (p >= 0 &&
                    (g_player[p].ps->on_ground || s->ang == 512))
                {
                    if (t[0] == 0 && !G_CheckActivatorMotion(s->lotag))
                    {
                        t[0] = 1;
                        t[1] = 1;
                        t[3] = !t[3];
                        G_OperateMasterSwitches(s->lotag);
                        G_OperateActivators(s->lotag,p);
                        if ((int16_t)s->hitag > 0)
                        {
                            s->hitag--;
                            if (s->hitag == 0) t[5] = 1;
                        }
                    }
                }
                else t[0] = 0;
            }

            if (t[1] == 1)
            {
                for (SPRITES_OF(STAT_STANDABLE, j))
                {
                    if (j != i && sprite[j].picnum == TOUCHPLATE && sprite[j].lotag == s->lotag)
                    {
                        actor[j].t_data[1] = 1;
                        actor[j].t_data[3] = t[3];
                    }
                }
            }
            goto BOLT;

        case CANWITHSOMETHING__STATIC:
        case CANWITHSOMETHING2__STATIC:
        case CANWITHSOMETHING3__STATIC:
        case CANWITHSOMETHING4__STATIC:
            A_Fall(i);
            if (A_IncurDamage(i) >= 0)
            {
                A_PlaySound(VENT_BUST,i);

                for (j=9; j>=0; j--)
                    RANDOMSCRAP;

                if (s->lotag) A_Spawn(i,s->lotag);

                KILLIT(i);
            }
            goto BOLT;

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
            if (!G_HaveActor(sprite[i].picnum))
                goto BOLT;
            {
                int32_t p = A_FindPlayer(s, &x);
                A_Execute(i,p,x);
            }
            goto BOLT;
        }

BOLT:
        i = nexti;
    }
}

ACTOR_STATIC void A_DoProjectileBounce(int32_t i)
{
    int32_t dax, day, daz = 4096;
    spritetype *s = &sprite[i];
    int32_t hitsect = s->sectnum;
    int32_t k = sector[hitsect].wallptr;
    int32_t l = wall[k].point2;

    int32_t xvect = mulscale10(s->xvel,sintable[(s->ang+512)&2047]);
    int32_t yvect = mulscale10(s->xvel,sintable[s->ang&2047]);
    int32_t zvect = s->zvel;

    int32_t daang = getangle(wall[l].x-wall[k].x,wall[l].y-wall[k].y);

    if (s->z < (actor[i].floorz+actor[i].ceilingz)>>1)
        k = sector[hitsect].ceilingheinum;
    else
        k = sector[hitsect].floorheinum;

    dax = mulscale14(k,sintable[(daang)&2047]);
    day = mulscale14(k,sintable[(daang+1536)&2047]);

    k = xvect*dax+yvect*day+zvect*daz;
    l = dax*dax+day*day+daz*daz;
    if ((klabs(k)>>14) < l)
    {
        k = divscale17(k,l);
        xvect -= mulscale16(dax,k);
        yvect -= mulscale16(day,k);
        zvect -= mulscale16(daz,k);
    }

    s->zvel = zvect;
    s->xvel = ksqrt(dmulscale8(xvect,xvect,yvect,yvect));
    s->ang = getangle(xvect,yvect);
}

ACTOR_STATIC void P_HandleBeingSpitOn(DukePlayer_t *ps)
{
    ps->horiz += 32;
    ps->return_to_center = 8;

    if (ps->loogcnt == 0)
    {
        int32_t j, x;

        if (!A_CheckSoundPlaying(ps->i, DUKE_LONGTERM_PAIN))
            A_PlaySound(DUKE_LONGTERM_PAIN,ps->i);

        j = 3+(krand()&3);
        ps->numloogs = j;
        ps->loogcnt = 24*4;
        for (x=0; x < j; x++)
        {
            ps->loogiex[x] = krand()%xdim;
            ps->loogiey[x] = krand()%ydim;
        }
    }
}

static void A_DoProjectileEffects(int32_t i, const vec3_t *davect, int32_t do_radius_damage)
{
    const projectile_t *proj = &SpriteProjectile[i];

    if (proj->spawns >= 0)
    {
        int32_t k = A_Spawn(i,proj->spawns);

        if (davect)
            Bmemcpy(&sprite[k],davect,sizeof(vec3_t));

        if (proj->sxrepeat > 4)
            sprite[k].xrepeat=proj->sxrepeat;
        if (proj->syrepeat > 4)
            sprite[k].yrepeat=proj->syrepeat;
    }

    if (proj->isound >= 0)
        A_PlaySound(proj->isound,i);

    if (do_radius_damage)
    {
        spritetype *const s = &sprite[i];
        int32_t x;

        s->extra=proj->extra;

        if (proj->extra_rand > 0)
            s->extra += (krand()&proj->extra_rand);

        x = s->extra;
        A_RadiusDamage(i,proj->hitradius, x>>2,x>>1,x-(x>>2),x);
    }
}

static void G_WeaponHitCeilingOrFloor(int32_t i, spritetype *s, int32_t *j)
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
    else if (s->z > actor[i].floorz + (16<<8)*(sector[s->sectnum].lotag == ST_1_ABOVE_WATER))
    {
        *j = 16384|s->sectnum;

        if (sector[s->sectnum].lotag != ST_1_ABOVE_WATER)
            s->zvel = 1;
    }
}

static void Proj_BounceOffWall(spritetype *s, int32_t j)
{
    int32_t k = getangle(
        wall[wall[j].point2].x-wall[j].x,
        wall[wall[j].point2].y-wall[j].y);
    s->ang = ((k<<1) - s->ang)&2047;
}

#define PROJ_DECAYVELOCITY(s) s->xvel >>= 1, s->zvel >>= 1

// Maybe damage a ceiling or floor as the consequence of projectile impact.
// Returns 1 if sprite <s> should be killed.
// NOTE: Compare with Proj_MaybeDamageCF2() in sector.c
static int32_t Proj_MaybeDamageCF(const spritetype *s)
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

ACTOR_STATIC void Proj_MoveCustom(int32_t i)
{
    const projectile_t *const proj = &SpriteProjectile[i];
    spritetype *const s = &sprite[i];
    vec3_t davect;
    int32_t j=0;

    if (proj->pal >= 0)
        s->pal = proj->pal;

    switch (proj->workslike & PROJECTILE_TYPE_MASK)
    {
    case PROJECTILE_HITSCAN:
        if (!G_HaveActor(sprite[i].picnum))
            return;
        {
            int32_t x, p = A_FindPlayer(s, &x);
            A_Execute(i, p, x);
        }
        return;

    case PROJECTILE_KNEE:
    case PROJECTILE_BLOOD:
        A_DeleteSprite(i);
        return;

    default:
    case PROJECTILE_RPG:
        Bmemcpy(&davect, s, sizeof(vec3_t));

        if (proj->flashcolor)
            G_AddGameLight(0, i, ((s->yrepeat*tilesiz[s->picnum].y)<<1), 2048, proj->flashcolor, PR_LIGHT_PRIO_LOW_GAME);

        if (proj->workslike & PROJECTILE_BOUNCESOFFWALLS && s->yvel < 1)
        {
            A_DoProjectileEffects(i, NULL, 1);
            A_DeleteSprite(i);
            return;
        }

        if (proj->workslike & PROJECTILE_COOLEXPLOSION1 && ++s->shade >= 40)
        {
            A_DeleteSprite(i);
            return;
        }

        s->zvel -= proj->drop;

        if (proj->workslike & PROJECTILE_SPIT && s->zvel < 6144)
            s->zvel += g_spriteGravity-112;

        A_GetZLimits(i);

        if (proj->trail >= 0)
        {
            int32_t cnt;

            for (cnt=0; cnt<=proj->tnum; cnt++)
            {
                j = A_Spawn(i, proj->trail);

                sprite[j].z += (proj->toffset<<8);

                if (proj->txrepeat >= 0)
                    sprite[j].xrepeat=proj->txrepeat;

                if (proj->tyrepeat >= 0)
                    sprite[j].yrepeat=proj->tyrepeat;
            }
        }

        {
            int32_t cnt = proj->movecnt;
            int32_t k = s->xvel;
            int32_t ll = s->zvel;

            if (sector[s->sectnum].lotag == ST_2_UNDERWATER)
            {
                k >>= 1;
                ll >>= 1;
            }

            do
            {
                vec3_t tmpvect;
                Bmemcpy(&davect, s, sizeof(vec3_t));

                tmpvect.x = (k*(sintable[(s->ang+512)&2047]))>>14;
                tmpvect.y = (k*(sintable[s->ang&2047]))>>14;
                tmpvect.z = ll;

                j = A_MoveSprite(i, &tmpvect, CLIPMASK1);
            } while (!j && --cnt > 0);
        }

        if (!(proj->workslike & PROJECTILE_BOUNCESOFFWALLS) &&  // NOT_BOUNCESOFFWALLS_YVEL
            (unsigned)s->yvel < MAXSPRITES && sprite[s->yvel].sectnum != MAXSECTORS)
            if (FindDistance2D(s->x-sprite[s->yvel].x, s->y-sprite[s->yvel].y) < 256)
                j = 49152|s->yvel;

        actor[i].movflag = j;

        if (s->sectnum < 0)
        {
            A_DeleteSprite(i);
            return;
        }

        if (proj->workslike & PROJECTILE_TIMED && proj->range > 0)
        {
            if (++actor[i].t_data[8] > proj->range)
            {
                if (proj->workslike & PROJECTILE_EXPLODEONTIMER)
                    A_DoProjectileEffects(i, &davect, 1);

                A_DeleteSprite(i);
                return;
            }
        }

        if ((j&49152) != 49152 && !(proj->workslike & PROJECTILE_BOUNCESOFFWALLS))
            G_WeaponHitCeilingOrFloor(i, s, &j);

        if (proj->workslike & PROJECTILE_WATERBUBBLES && sector[s->sectnum].lotag == ST_2_UNDERWATER && rnd(140))
            A_Spawn(i, WATERBUBBLE);

        if (j != 0)
        {
            int32_t k;

            if (proj->workslike & PROJECTILE_COOLEXPLOSION1)
            {
                s->xvel = 0;
                s->zvel = 0;
            }

            switch (j&49152)
            {
            case 49152:
                j &= (MAXSPRITES-1);

                if (proj->workslike & PROJECTILE_BOUNCESOFFSPRITES)
                {
                    s->yvel--;

                    k = getangle(sprite[j].x-s->x, sprite[j].y-s->y)+(sprite[j].cstat&16 ? 0 : 512);
                    s->ang = ((k<<1) - s->ang)&2047;

                    if (proj->bsound >= 0)
                        A_PlaySound(proj->bsound, i);

                    if (proj->workslike & PROJECTILE_LOSESVELOCITY)
                        PROJ_DECAYVELOCITY(s);

                    if (!(proj->workslike & PROJECTILE_FORCEIMPACT))
                        return;
                }

                A_DamageObject(j, i);

                if (sprite[j].picnum == APLAYER)
                {
                    int32_t p = P_Get(j);

                    A_PlaySound(PISTOL_BODYHIT, j);

                    if (proj->workslike & PROJECTILE_SPIT)
                        P_HandleBeingSpitOn(g_player[p].ps);
                }

                if (proj->workslike & PROJECTILE_RPG_IMPACT)
                {
                    actor[j].owner = s->owner;
                    actor[j].picnum = s->picnum;
                    actor[j].extra += proj->extra;

                    A_DoProjectileEffects(i, &davect, 0);

                    if (!(proj->workslike & PROJECTILE_FORCEIMPACT))
                    {
                        A_DeleteSprite(i);
                        return;
                    }
                }

                if (proj->workslike & PROJECTILE_FORCEIMPACT)
                    return;
                break;

            case 32768:
                j &= (MAXWALLS-1);

                if (proj->workslike & PROJECTILE_BOUNCESOFFMIRRORS &&
                    (wall[j].overpicnum == MIRROR || wall[j].picnum == MIRROR))
                {
                    Proj_BounceOffWall(s, j);
                    s->owner = i;
                    A_Spawn(i, TRANSPORTERSTAR);
                    return;
                }
                else
                {
                    setsprite(i, &davect);
                    A_DamageWall(i, j, (vec3_t *) s, s->picnum);

                    if (proj->workslike & PROJECTILE_BOUNCESOFFWALLS)
                    {
                        if (wall[j].overpicnum != MIRROR && wall[j].picnum != MIRROR)
                            s->yvel--;

                        Proj_BounceOffWall(s, j);

                        if (proj->bsound >= 0)
                            A_PlaySound(proj->bsound, i);

                        if (proj->workslike & PROJECTILE_LOSESVELOCITY)
                            PROJ_DECAYVELOCITY(s);

                        return;
                    }
                }
                break;

            case 16384:
                setsprite(i, &davect);

                if (Proj_MaybeDamageCF(s))
                {
                    A_DeleteSprite(i);
                    return;
                }

                if (proj->workslike & PROJECTILE_BOUNCESOFFWALLS)
                {
                    A_DoProjectileBounce(i);
                    A_SetSprite(i, CLIPMASK1);

                    s->yvel--;

                    if (proj->bsound >= 0)
                        A_PlaySound(proj->bsound, i);

                    if (proj->workslike & PROJECTILE_LOSESVELOCITY)
                        PROJ_DECAYVELOCITY(s);

                    return;
                }
                break;
            }

            A_DoProjectileEffects(i, &davect, 1);
            A_DeleteSprite(i);
            return;
        }
        return;
    }
}

ACTOR_STATIC void G_MoveWeapons(void)
{
    int32_t i = headspritestat[STAT_PROJECTILE], j=0, k;
    int32_t x, ll;

    while (i >= 0)
    {
        const int32_t nexti = nextspritestat[i];
        spritetype *const s = &sprite[i];
        vec3_t davect;

        if (s->sectnum < 0)
            KILLIT(i);

        Bmemcpy(&actor[i].bpos, s, sizeof(vec3_t));

        /* Custom projectiles */
        if (A_CheckSpriteFlags(i, SFLAG_PROJECTILE))
        {
            Proj_MoveCustom(i);
            goto BOLT;
        }

        // hard coded projectiles
        switch (DYNAMICTILEMAP(s->picnum))
        {
        case RADIUSEXPLOSION__STATIC:
        case KNEE__STATIC:
            KILLIT(i);

        case FREEZEBLAST__STATIC:
            if (s->yvel < 1 || s->extra < 2 || (s->xvel|s->zvel) == 0)
            {
                j = A_Spawn(i,TRANSPORTERSTAR);
                sprite[j].pal = 1;
                sprite[j].xrepeat = 32;
                sprite[j].yrepeat = 32;
                KILLIT(i);
            }
        case SHRINKSPARK__STATIC:
        case RPG__STATIC:
        case FIRELASER__STATIC:
        case SPIT__STATIC:
        case COOLEXPLOSION1__STATIC:

            if (s->picnum == COOLEXPLOSION1)
                if (!S_CheckSoundPlaying(i,WIERDSHOT_FLY))
                    A_PlaySound(WIERDSHOT_FLY,i);

            k = s->xvel;
            ll = s->zvel;

            if (s->picnum == RPG && sector[s->sectnum].lotag == ST_2_UNDERWATER)
            {
                k >>= 1;
                ll >>= 1;
            }

            Bmemcpy(&davect,s,sizeof(vec3_t));

            A_GetZLimits(i);

            if (s->picnum == RPG && actor[i].picnum != BOSS2 && s->xrepeat >= 10 &&
                    sector[s->sectnum].lotag != ST_2_UNDERWATER && g_scriptVersion >= 13)
            {
                j = A_Spawn(i,SMALLSMOKE);
                sprite[j].z += (1<<8);
            }

            {
                vec3_t tmpvect;

                tmpvect.x = (k*(sintable[(s->ang+512)&2047]))>>14;
                tmpvect.y = (k*(sintable[s->ang&2047]))>>14;
                tmpvect.z = ll;
                j = A_MoveSprite(i,&tmpvect, CLIPMASK1);
            }


            if (s->picnum == RPG && (unsigned)s->yvel < MAXSPRITES)  // RPG_YVEL
                if (FindDistance2D(s->x-sprite[s->yvel].x,s->y-sprite[s->yvel].y) < 256)
                    j = 49152|s->yvel;

            actor[i].movflag = j;

            if (s->sectnum < 0)
                KILLIT(i);

            if ((j&49152) != 49152 && s->picnum != FREEZEBLAST)
                G_WeaponHitCeilingOrFloor(i, s, &j);

            if (s->picnum == FIRELASER)
            {
                for (k=-3; k<2; k++)
                {
                    x = A_InsertSprite(s->sectnum,
                                       s->x+((k*sintable[(s->ang+512)&2047])>>9),
                                       s->y+((k*sintable[s->ang&2047])>>9),
                                       s->z+((k*ksgn(s->zvel))*klabs(s->zvel/24)),FIRELASER,-40+(k<<2),
                                       s->xrepeat,s->yrepeat,0,0,0,s->owner,5);

                    sprite[x].cstat = 128;
                    sprite[x].pal = s->pal;
                }
            }
            else if (s->picnum == SPIT)
                if (s->zvel < 6144)
                    s->zvel += g_spriteGravity-112;

            if (j != 0)
            {
                if (s->picnum == COOLEXPLOSION1)
                {
                    if ((j&49152) == 49152 && sprite[j&(MAXSPRITES-1)].picnum != APLAYER)
                        goto COOLEXPLOSION;
                    s->xvel = 0;
                    s->zvel = 0;
                }

                switch (j&49152)
                {
                case 49152:
                    j &= (MAXSPRITES-1);

                    if (s->picnum == FREEZEBLAST && sprite[j].pal == 1)
                        if (A_CheckEnemySprite(&sprite[j]) || sprite[j].picnum == APLAYER)
                        {
                            j = A_Spawn(i, TRANSPORTERSTAR);
                            sprite[j].pal = 1;
                            sprite[j].xrepeat = 32;
                            sprite[j].yrepeat = 32;

                            KILLIT(i);
                        }

                        A_DamageObject(j, i);

                        if (sprite[j].picnum == APLAYER)
                        {
                            int32_t p = P_Get(j);
                            A_PlaySound(PISTOL_BODYHIT, j);

                            if (s->picnum == SPIT)
                                P_HandleBeingSpitOn(g_player[p].ps);
                        }
                        break;

                case 32768:
                    j &= (MAXWALLS-1);

                    if (s->picnum != RPG && s->picnum != FREEZEBLAST && s->picnum != SPIT &&
                        (wall[j].overpicnum == MIRROR || wall[j].picnum == MIRROR))
                    {
                        Proj_BounceOffWall(s, j);
                        s->owner = i;
                        A_Spawn(i, TRANSPORTERSTAR);
                        goto BOLT;
                    }
                    else
                    {
                        setsprite(i, &davect);
                        A_DamageWall(i, j, (vec3_t *) s, s->picnum);

                        if (s->picnum == FREEZEBLAST)
                        {
                            if (wall[j].overpicnum != MIRROR && wall[j].picnum != MIRROR)
                            {
                                s->extra >>= 1;
                                s->yvel--;
                            }

                            Proj_BounceOffWall(s, j);
                            goto BOLT;
                        }
                    }
                    break;

                case 16384:
                    setsprite(i, &davect);

                    if (Proj_MaybeDamageCF(s))
                        KILLIT(i);

                    if (s->picnum == FREEZEBLAST)
                    {
                        A_DoProjectileBounce(i);
                        A_SetSprite(i, CLIPMASK1);

                        s->extra >>= 1;
                        s->yvel--;

                        if (s->xrepeat > 8)
                        {
                            s->xrepeat -= 2;

                            if (s->yrepeat > 8)
                                s->yrepeat -= 2;
                        }

                        goto BOLT;
                    }
                    break;
                default:
                    break;
                }

                switch (DYNAMICTILEMAP(s->picnum))
                {
                case SPIT__STATIC:
                case COOLEXPLOSION1__STATIC:
                case FREEZEBLAST__STATIC:
                case FIRELASER__STATIC:
                    break;

                case RPG__STATIC:
                    k = A_Spawn(i, EXPLOSION2);
                    A_PlaySound(RPG_EXPLODE, k);
                    Bmemcpy(&sprite[k], &davect, sizeof(vec3_t));

                    if (s->xrepeat < 10)
                    {
                        sprite[k].xrepeat = 6;
                        sprite[k].yrepeat = 6;
                    }
                    else if ((j&49152) == 16384)
                    {
                        if (s->zvel > 0)
                            A_Spawn(i, EXPLOSION2BOT);
                        else
                        {
                            sprite[k].cstat |= 8;
                            sprite[k].z += (48<<8);
                        }
                    }

                    if (s->xrepeat >= 10)
                    {
                        x = s->extra;
                        A_RadiusDamage(i, g_rpgBlastRadius, x>>2, x>>1, x-(x>>2), x);
                    }
                    else
                    {
                        x = s->extra+(g_globalRandom&3);
                        A_RadiusDamage(i, (g_rpgBlastRadius>>1), x>>2, x>>1, x-(x>>2), x);
                    }
                    break;

                case SHRINKSPARK__STATIC:
                    A_Spawn(i, SHRINKEREXPLOSION);
                    A_PlaySound(SHRINKER_HIT, i);
                    A_RadiusDamage(i, g_shrinkerBlastRadius, 0, 0, 0, 0);
                    break;

                default:
                    k = A_Spawn(i, EXPLOSION2);
                    sprite[k].xrepeat = sprite[k].yrepeat = s->xrepeat>>1;
                    if ((j&49152) == 16384)
                    {
                        if (s->zvel < 0)
                        {
                            sprite[k].cstat |= 8;
                            sprite[k].z += (72<<8);
                        }
                    }
                    break;
                }

                if (s->picnum != COOLEXPLOSION1)
                    KILLIT(i);
            }

            if (s->picnum == COOLEXPLOSION1)
            {
COOLEXPLOSION:
                s->shade++;
                if (s->shade >= 40)
                    KILLIT(i);
            }
            else if (s->picnum == RPG && sector[s->sectnum].lotag == ST_2_UNDERWATER && s->xrepeat >= 10 && rnd(140))
                A_Spawn(i,WATERBUBBLE);

            goto BOLT;

        case SHOTSPARK1__STATIC:
            if (!G_HaveActor(sprite[i].picnum))
                goto BOLT;
            {
                int32_t p = A_FindPlayer(s,&x);
                A_Execute(i,p,x);
            }
            goto BOLT;
        }
BOLT:
        i = nexti;
    }
}


static int32_t P_Submerge(int32_t j, int32_t p, DukePlayer_t *ps, int32_t sect, int32_t othersect)
{
    if (ps->on_ground &&
        ps->pos.z >= sector[sect].floorz &&
        (TEST_SYNC_KEY(g_player[p].sync->bits, SK_CROUCH) || ps->vel.z > 2048))
//        if( onfloorz && sectlotag == 1 && ps->pos.z > (sector[sect].floorz-(6<<8)) )
    {
        if (screenpeek == p)
        {
            FX_StopAllSounds();
            S_ClearSoundLocks();
        }

        if (sprite[ps->i].extra > 0)
            A_PlaySound(DUKE_UNDERWATER, j);

        ps->opos.z = ps->pos.z = sector[othersect].ceilingz;
//        ps->vel.x = 4096-(krand()&8192);
//        ps->vel.y = 4096-(krand()&8192);

        if (TEST_SYNC_KEY(g_player[p].sync->bits, SK_CROUCH))
            ps->vel.z += 512;

        return 1;
    }

    return 0;
}

static int32_t P_Emerge(int32_t j, int32_t p, DukePlayer_t *ps, int32_t sect, int32_t othersect)
{
    // r1449-:
    if (ps->pos.z < (sector[sect].ceilingz+1080) && ps->vel.z == 0)
        // r1450+, breaks submergible slime in bobsp2:
//        if (onfloorz && sectlotag == 2 && ps->pos.z <= sector[sect].ceilingz /*&& ps->vel.z == 0*/)
    {
//        if( sprite[j].extra <= 0) break;
        if (screenpeek == p)
        {
            FX_StopAllSounds();
            S_ClearSoundLocks();
        }

        A_PlaySound(DUKE_GASP, j);

        ps->opos.z = ps->pos.z = sector[othersect].floorz;
        ps->vel.z = 0;
//        ps->vel.z += 1024;

        ps->jumping_toggle = 1;
        ps->jumping_counter = 0;

        return 1;
    }

    return 0;
}

static void P_FinishWaterChange(int32_t j, DukePlayer_t *ps, int32_t sectlotag, int32_t ow, int32_t newsectnum)
{
    int32_t l;
    vec3_t vect;

    ps->bobpos.x = ps->opos.x = ps->pos.x;
    ps->bobpos.y = ps->opos.y = ps->pos.y;

    if (ow < 0 || sprite[ow].owner != ow)
        ps->transporter_hold = -2;

    ps->cursectnum = newsectnum;
    changespritesect(j, newsectnum);

    vect.x = ps->pos.x;
    vect.y = ps->pos.y;
    vect.z = ps->pos.z+PHEIGHT;
    setsprite(ps->i, &vect);

    P_UpdateScreenPal(ps);

    if ((krand()&255) < 32)
        A_Spawn(j, WATERSPLASH2);

    if (sectlotag == ST_1_ABOVE_WATER)
        for (l = 0; l < 9; l++)
        {
            int32_t q = A_Spawn(ps->i,WATERBUBBLE);
            sprite[q].z += krand()&16383;
        }
}

// Check prevention of teleportation *when alive*. For example, commanders and
// octabrains would be transported by SE7 (both water and normal) only if dead.
static int32_t A_CheckNonTeleporting(int32_t s)
{
    int32_t pic = sprite[s].picnum;

    if (A_CheckSpriteFlags(s, SFLAG_NOTELEPORT)) return 1;

    return (pic == SHARK || pic == COMMANDER || pic == OCTABRAIN
                || (pic >= GREENSLIME && pic <= GREENSLIME+7));
}

ACTOR_STATIC void G_MoveTransports(void)
{
    int32_t i = headspritestat[STAT_TRANSPORT];

    while (i >= 0)
    {
        const int32_t sect = SECT;
        const int32_t sectlotag = sector[sect].lotag;

        const int32_t nexti = nextspritestat[i];
        int32_t j, k;

        const int32_t onfloorz = T5;  // ONFLOORZ

        if (OW == i)
        {
            i = nexti;
            continue;
        }

        if (T1 > 0) T1--;

        j = headspritesect[sect];
        while (j >= 0)
        {
            const int32_t nextj = nextspritesect[j];

            switch (sprite[j].statnum)
            {
            case STAT_PLAYER:
                if (sprite[j].owner != -1)
                {
                    const int32_t p = P_Get(j);
                    DukePlayer_t *const ps = g_player[p].ps;

                    ps->on_warping_sector = 1;

                    if (ps->transporter_hold == 0 && ps->jumping_counter == 0)
                    {
                        if (ps->on_ground && sectlotag == 0 && onfloorz && ps->jetpack_on == 0)
                        {
                            if (sprite[i].pal == 0)
                            {
                                A_Spawn(i,TRANSPORTERBEAM);
                                A_PlaySound(TELEPORTER,i);
                            }

                            for (TRAVERSE_CONNECT(k))
                                if (g_player[k].ps->cursectnum == sprite[OW].sectnum)
                                {
                                    g_player[k].ps->frag_ps = p;
                                    sprite[g_player[k].ps->i].extra = 0;
                                }

                            ps->ang = sprite[OW].ang;

                            if (sprite[OW].owner != OW)
                            {
                                T1 = 13;
                                actor[OW].t_data[0] = 13;
                                ps->transporter_hold = 13;
                            }

                            ps->bobpos.x = ps->opos.x = ps->pos.x = sprite[OW].x;
                            ps->bobpos.y = ps->opos.y = ps->pos.y = sprite[OW].y;
                            ps->opos.z = ps->pos.z = sprite[OW].z-PHEIGHT;

                            changespritesect(j,sprite[OW].sectnum);
                            ps->cursectnum = sprite[j].sectnum;

                            if (sprite[i].pal == 0)
                            {
                                k = A_Spawn(OW,TRANSPORTERBEAM);
                                A_PlaySound(TELEPORTER,k);
                            }

                            break;
                        }
                    }
                    else if (!(sectlotag == ST_1_ABOVE_WATER && ps->on_ground == 1)) break;

                    if (onfloorz == 0 && klabs(SZ-ps->pos.z) < 6144)
                        if (!ps->jetpack_on || TEST_SYNC_KEY(g_player[p].sync->bits, SK_JUMP) ||
                                TEST_SYNC_KEY(g_player[p].sync->bits, SK_CROUCH))
                        {
                            ps->bobpos.x = ps->opos.x = ps->pos.x += sprite[OW].x-SX;
                            ps->bobpos.y = ps->opos.y = ps->pos.y += sprite[OW].y-SY;

                            if (ps->jetpack_on && (TEST_SYNC_KEY(g_player[p].sync->bits, SK_JUMP) || ps->jetpack_on < 11))
                                ps->pos.z = sprite[OW].z-6144;
                            else ps->pos.z = sprite[OW].z+6144;
                            ps->opos.z = ps->pos.z;

                            actor[ps->i].bpos.x = ps->pos.x;
                            actor[ps->i].bpos.y = ps->pos.y;
                            actor[ps->i].bpos.z = ps->pos.z;

                            changespritesect(j,sprite[OW].sectnum);
                            ps->cursectnum = sprite[OW].sectnum;

                            break;
                        }

                    k = 0;
                    if (onfloorz)
                    {
                        if (sectlotag==ST_1_ABOVE_WATER)
                            k = P_Submerge(j, p, ps, sect, sprite[OW].sectnum);
                        else if (sectlotag==ST_2_UNDERWATER)
                            k = P_Emerge(j, p, ps, sect, sprite[OW].sectnum);
                    }

                    if (k == 1)
                    {
                        ps->pos.x += sprite[OW].x-SX;
                        ps->pos.y += sprite[OW].y-SY;

                        P_FinishWaterChange(j, ps, sectlotag, OW, sprite[OW].sectnum);
                    }
                }
                break;


            ////////// Non-player teleportation //////////

            case STAT_PROJECTILE:
                // SE7_PROJECTILE, PROJECTILE_CHSECT.
                // comment out to make RPGs pass through water: (r1450 breaks this)
//                if (sectlotag != 0) goto JBOLT;
            case STAT_ACTOR:
                if (sprite[j].extra > 0 && A_CheckNonTeleporting(j))
                    goto JBOLT;
            case STAT_MISC:
            case STAT_FALLER:
            case STAT_DUMMYPLAYER:
            {
                if (totalclock > actor[j].lasttransport)
                {
                    const int32_t zvel = sprite[j].zvel;
                    const int32_t ll = klabs(zvel);
                    int32_t warpspriteto = 0;

                    if (ll != 0)
                    {
                        if (sectlotag == ST_2_UNDERWATER && sprite[j].z < (sector[sect].ceilingz+ll) && zvel < 0)
                            warpspriteto = 1;
                        if (sectlotag == ST_1_ABOVE_WATER && sprite[j].z > (sector[sect].floorz-ll) && zvel > 0)
                            warpspriteto = 1;
                    }

                    if (sectlotag == 0 && (onfloorz || klabs(sprite[j].z-SZ) < 4096))
                    {
                        if (sprite[OW].owner != OW && onfloorz && T1 > 0 && sprite[j].statnum != STAT_MISC)
                        {
                            T1++;
                            goto BOLT;
                        }
                        warpspriteto = 1;
                    }

                    if (warpspriteto)
                    {
                        if (A_CheckSpriteFlags(j,SFLAG_DECAL))
                            goto JBOLT;

                        switch (DYNAMICTILEMAP(sprite[j].picnum))
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
                        case LASERLINE__STATIC:
                            goto JBOLT;

                        case PLAYERONWATER__STATIC:
                            if (sectlotag == ST_2_UNDERWATER)
                            {
                                sprite[j].cstat &= 32768;
                                break;
                            }
                            // fall-through
                        default:
                            if (sprite[j].statnum == STAT_MISC && !(sectlotag == ST_1_ABOVE_WATER || sectlotag == ST_2_UNDERWATER))
                                break;
                            // fall-through
                        case WATERBUBBLE__STATIC:
//                            if( rnd(192) && sprite[j].picnum == WATERBUBBLE)
//                                break;

                            if (sectlotag > 0)
                            {
                                // Water SE7 teleportation.
                                const int32_t osect = sprite[OW].sectnum;

                                Bassert(sectlotag==ST_1_ABOVE_WATER || sectlotag==ST_2_UNDERWATER);

                                k = A_Spawn(j,WATERSPLASH2);
                                if (sectlotag == ST_1_ABOVE_WATER && sprite[j].statnum == STAT_PROJECTILE)
                                {
                                    sprite[k].xvel = sprite[j].xvel>>1;
                                    sprite[k].ang = sprite[j].ang;
                                    A_SetSprite(k,CLIPMASK0);
                                }

                                //
                                actor[j].lasttransport = totalclock + (TICSPERFRAME<<2);

                                sprite[j].x += (sprite[OW].x-SX);
                                sprite[j].y += (sprite[OW].y-SY);
                                sprite[j].z = sectlotag==ST_1_ABOVE_WATER ?
                                    sector[osect].ceilingz : sector[osect].floorz;

                                Bmemcpy(&actor[j].bpos, &sprite[j], sizeof(vec3_t));

                                changespritesect(j, sprite[OW].sectnum);
                            }
                            else if (Bassert(sectlotag==0), 1)
                            {
                                // Non-water SE7 teleportation.

                                if (onfloorz)
                                {
                                    if (sprite[j].statnum == STAT_PROJECTILE ||
                                            (G_CheckPlayerInSector(sect) == -1 && G_CheckPlayerInSector(sprite[OW].sectnum) == -1))
                                    {
                                        sprite[j].x += (sprite[OW].x-SX);
                                        sprite[j].y += (sprite[OW].y-SY);
                                        sprite[j].z -= SZ - sector[sprite[OW].sectnum].floorz;
                                        sprite[j].ang = sprite[OW].ang;

                                        Bmemcpy(&actor[j].bpos, &sprite[j], sizeof(vec3_t));

                                        if (sprite[i].pal == 0)
                                        {
                                            k = A_Spawn(i,TRANSPORTERBEAM);
                                            A_PlaySound(TELEPORTER,k);

                                            k = A_Spawn(OW,TRANSPORTERBEAM);
                                            A_PlaySound(TELEPORTER,k);
                                        }

                                        if (sprite[OW].owner != OW)
                                        {
                                            T1 = 13;
                                            actor[OW].t_data[0] = 13;
                                        }

                                        changespritesect(j,sprite[OW].sectnum);
                                    }
                                }
                                else
                                {
                                    sprite[j].x += (sprite[OW].x-SX);
                                    sprite[j].y += (sprite[OW].y-SY);
                                    sprite[j].z = sprite[OW].z+4096;

                                    Bmemcpy(&actor[j].bpos, &sprite[j], sizeof(vec3_t));

                                    changespritesect(j,sprite[OW].sectnum);
                                }
                            }

                            break;
                        }  // switch (DYNAMICTILEMAP(sprite[j].picnum))
                    }  // if (warpspriteto)
                }  // if (totalclock > actor[j].lasttransport)

                break;
            }  // five cases

            }  // switch (sprite[j].statnum)
JBOLT:
            j = nextj;
        }
BOLT:
        i = nexti;
    }
}

static int16_t A_FindLocator(int32_t n, int32_t sn)
{
    int32_t i;

    for (SPRITES_OF(STAT_LOCATOR, i))
    {
        if ((sn == -1 || sn == SECT) && n == SLT)
            return i;
    }

    return -1;
}

ACTOR_STATIC void G_MoveActors(void)
{
    int32_t x, m, l;

    int32_t j;
    int32_t i = headspritestat[STAT_ACTOR];

    while (i >= 0)
    {
        const int32_t nexti = nextspritestat[i];

        spritetype *const s = &sprite[i];
        const int32_t sect = s->sectnum;

        int32_t switchpicnum;
        int32_t *const t = actor[i].t_data;

        if (s->xrepeat == 0 || sect < 0 || sect >= MAXSECTORS)
            KILLIT(i);

        Bmemcpy(&actor[i].bpos, s, sizeof(vec3_t));

        switchpicnum = s->picnum;

        if (s->picnum > GREENSLIME && s->picnum <= GREENSLIME+7)
            switchpicnum = GREENSLIME;

        switch (DYNAMICTILEMAP(switchpicnum))
        {
        case DUCK__STATIC:
        case TARGET__STATIC:
            if (s->cstat&32)
            {
                t[0]++;
                if (t[0] > 60)
                {
                    t[0] = 0;
                    s->cstat = 128+257+16;
                    s->extra = 1;
                }
            }
            else
            {
                if (A_IncurDamage(i) >= 0)
                {
                    int32_t k = 1;

                    s->cstat = 32+128;

                    for (SPRITES_OF(STAT_ACTOR, j))
                    {
                        if (sprite[j].lotag == s->lotag && sprite[j].picnum == s->picnum)
                        {
                            if ((sprite[j].hitag!=0) ^ ((sprite[j].cstat&32)!=0))
                            {
                                k = 0;
                                break;
                            }
                        }
                    }

                    if (k == 1)
                    {
                        G_OperateActivators(s->lotag,-1);
                        G_OperateForceFields(i,s->lotag);
                        G_OperateMasterSwitches(s->lotag);
                    }
                }
            }
            goto BOLT;

        case RESPAWNMARKERRED__STATIC:
        case RESPAWNMARKERYELLOW__STATIC:
        case RESPAWNMARKERGREEN__STATIC:
            if (++T1 > g_itemRespawnTime)
                KILLIT(i);

            if (T1 >= (g_itemRespawnTime>>1) && T1 < ((g_itemRespawnTime>>1)+(g_itemRespawnTime>>2)))
                PN = RESPAWNMARKERYELLOW;
            else if (T1 > ((g_itemRespawnTime>>1)+(g_itemRespawnTime>>2)))
                PN = RESPAWNMARKERGREEN;

            A_Fall(i);
            break;

        case HELECOPT__STATIC:
        case DUKECAR__STATIC:
            s->z += s->zvel;
            t[0]++;

            if (t[0] == 4) A_PlaySound(WAR_AMBIENCE2,i);

            if (t[0] > (GAMETICSPERSEC*8))
            {
                S_PlaySound(RPG_EXPLODE);
                for (j=0; j<32; j++) RANDOMSCRAP;
                g_earthquakeTime = 16;
                KILLIT(i);
            }
            else if ((t[0]&3) == 0)
                A_Spawn(i,EXPLOSION2);
            A_SetSprite(i,CLIPMASK0);
            break;

        case RAT__STATIC:
            A_Fall(i);
            if (A_SetSprite(i, CLIPMASK0))
            {
                if ((krand()&255) < 3) A_PlaySound(RATTY,i);
                s->ang += (krand()&31)-15+(sintable[(t[0]<<8)&2047]>>11);
            }
            else
            {
                T1++;
                if (T1 > 1)
                {
                    KILLIT(i);
                }
                else s->ang = (krand()&2047);
            }
            if (s->xvel < 128)
                s->xvel+=2;
            s->ang += (krand()&3)-6;
            break;

        case QUEBALL__STATIC:
        case STRIPEBALL__STATIC:
            if (s->xvel)
            {
                for (SPRITES_OF(STAT_DEFAULT, j))
                    if (sprite[j].picnum == POCKET && ldist(&sprite[j],s) < 52)
                        KILLIT(i);

                j = clipmove((vec3_t *)s,&s->sectnum,
                             (((s->xvel*(sintable[(s->ang+512)&2047]))>>14)*TICSPERFRAME)<<11,
                             (((s->xvel*(sintable[s->ang&2047]))>>14)*TICSPERFRAME)<<11,
                             24L,(4<<8),(4<<8),CLIPMASK1);

                if (j&49152)
                {
                    if ((j&49152) == 32768)
                    {
                        j &= (MAXWALLS-1);
                        Proj_BounceOffWall(s, j);
                    }
                    else if ((j&49152) == 49152)
                    {
                        j &= (MAXSPRITES-1);
                        A_DamageObject(i,j);
                    }
                }

                s->xvel--;
                if (s->xvel < 0) s->xvel = 0;

                if (s->picnum == STRIPEBALL)
                {
                    s->cstat = 257;
                    s->cstat |= (4 & s->xvel) | (8 & s->xvel);
                }
            }
            else
            {
                const int32_t p = A_FindPlayer(s,&x);
                DukePlayer_t *const ps = g_player[p].ps;

                if (x < 1596)
//                    if (s->pal == 12)
                {
                    j = G_GetAngleDelta(ps->ang,getangle(s->x-ps->pos.x,s->y-ps->pos.y));

                    if (j > -64 && j < 64 && TEST_SYNC_KEY(g_player[p].sync->bits, SK_OPEN))
                        if (ps->toggle_key_flag == 1)
                        {
                            int32_t a;

                            for (SPRITES_OF(STAT_ACTOR, a))
                            {
                                if (sprite[a].picnum == QUEBALL || sprite[a].picnum == STRIPEBALL)
                                {
                                    j = G_GetAngleDelta(ps->ang,getangle(sprite[a].x-ps->pos.x,sprite[a].y-ps->pos.y));
                                    if (j > -64 && j < 64)
                                    {
                                        A_FindPlayer(&sprite[a],&l);
                                        if (x > l) break;
                                    }
                                }
                            }

                            if (a == -1)
                            {
                                if (s->pal == 12)
                                    s->xvel = 164;
                                else s->xvel = 140;
                                s->ang = ps->ang;
                                ps->toggle_key_flag = 2;
                            }
                        }
                }

                if (x < 512 && s->sectnum == ps->cursectnum)
                {
                    s->ang = getangle(s->x-ps->pos.x,s->y-ps->pos.y);
                    s->xvel = 48;
                }
            }

            break;

        case FORCESPHERE__STATIC:
            if (s->yvel == 0)
            {
                s->yvel = 1;

                for (l=512; l<(2048-512); l+= 128)
                    for (j=0; j<2048; j += 128)
                    {
                        int32_t k = A_Spawn(i,FORCESPHERE);
                        sprite[k].cstat = 257+128;
                        sprite[k].clipdist = 64;
                        sprite[k].ang = j;
                        sprite[k].zvel = sintable[l&2047]>>5;
                        sprite[k].xvel = sintable[(l+512)&2047]>>9;
                        sprite[k].owner = i;
                    }
            }

            if (t[3] > 0)
            {
                if (s->zvel < 6144)
                    s->zvel += 192;
                s->z += s->zvel;
                if (s->z > sector[sect].floorz)
                    s->z = sector[sect].floorz;
                t[3]--;
                if (t[3] == 0)
                    KILLIT(i);
            }
            else if (t[2] > 10)
            {
                for (SPRITES_OF(STAT_MISC, j))
                    if (sprite[j].owner == i && sprite[j].picnum == FORCESPHERE)
                        actor[j].t_data[1] = 1+(krand()&63);

                t[3] = 64;
            }

            goto BOLT;

        case RECON__STATIC:
        {
            int32_t p;
            DukePlayer_t *ps;

            A_GetZLimits(i);

            if (sector[s->sectnum].ceilingstat&1)
                s->shade += (sector[s->sectnum].ceilingshade-s->shade)>>1;
            else s->shade += (sector[s->sectnum].floorshade-s->shade)>>1;

            if (s->z < sector[sect].ceilingz+(32<<8))
                s->z = sector[sect].ceilingz+(32<<8);

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
                    s->cstat = 32768;
                    goto BOLT;
                }
                else if (g_noEnemies == 2) s->cstat = 257;
            }
            if (A_IncurDamage(i) >= 0)
            {
                if (s->extra < 0 && t[0] != -1)
                {
                    t[0] = -1;
                    s->extra = 0;
                }

                A_PlaySound(RECO_PAIN,i);
                RANDOMSCRAP;
            }

            if (t[0] == -1)
            {
                s->z += 1024;
                t[2]++;

                if ((t[2]&3) == 0)
                    A_Spawn(i,EXPLOSION2);

                A_GetZLimits(i);
                s->ang += 96;
                s->xvel = 128;
                j = A_SetSprite(i,CLIPMASK0);

                if (!j || s->z > actor[i].floorz)
                {
                    for (l=0; l<16; l++)
                        RANDOMSCRAP;
                    j = A_Spawn(i,EXPLOSION2);
                    A_PlaySound(LASERTRIP_EXPLODE,j);
                    A_Spawn(i,PIGCOP);
                    g_player[myconnectindex].ps->actors_killed++;
                    KILLIT(i);
                }
                goto BOLT;
            }
            else
            {
                if (s->z > actor[i].floorz-(48<<8))
                    s->z = actor[i].floorz-(48<<8);
            }

            p = A_FindPlayer(s,&x);
            ps = g_player[p].ps;

            j = s->owner;

            // 3 = findplayerz, 4 = shoot

            if (t[0] >= 4)
            {
                t[2]++;
                if ((t[2]&15) == 0)
                {
                    int32_t a = s->ang;
                    s->ang = actor[i].tempang;
                    A_PlaySound(RECO_ATTACK,i);
                    A_Shoot(i,FIRELASER);
                    s->ang = a;
                }
                if (t[2] > (GAMETICSPERSEC*3) || !cansee(s->x,s->y,s->z-(16<<8),s->sectnum, ps->pos.x,ps->pos.y,ps->pos.z,ps->cursectnum))
                {
                    t[0] = 0;
                    t[2] = 0;
                }
                else actor[i].tempang +=
                        G_GetAngleDelta(actor[i].tempang,getangle(ps->pos.x-s->x,ps->pos.y-s->y))/3;
            }
            else if (t[0] == 2 || t[0] == 3)
            {
                t[3] = 0;
                if (s->xvel > 0) s->xvel -= 16;
                else s->xvel = 0;

                if (t[0] == 2)
                {
                    l = ps->pos.z-s->z;
                    if (klabs(l) < (48<<8)) t[0] = 3;
                    else s->z += ksgn(ps->pos.z-s->z)<<10;
                }
                else
                {
                    t[2]++;
                    if (t[2] > (GAMETICSPERSEC*3) || !cansee(s->x,s->y,s->z-(16<<8),s->sectnum, ps->pos.x,ps->pos.y,ps->pos.z,ps->cursectnum))
                    {
                        t[0] = 1;
                        t[2] = 0;
                    }
                    else if ((t[2]&15) == 0)
                    {
                        A_PlaySound(RECO_ATTACK,i);
                        A_Shoot(i,FIRELASER);
                    }
                }
                s->ang += G_GetAngleDelta(s->ang,getangle(ps->pos.x-s->x,ps->pos.y-s->y))>>2;
            }

            if (t[0] != 2 && t[0] != 3)
            {
                int32_t a;
                l = ldist(&sprite[j],s);
                if (l <= 1524)
                {
                    a = s->ang;
                    s->xvel >>= 1;
                }
                else a = getangle(sprite[j].x-s->x,sprite[j].y-s->y);

                if (t[0] == 1 || t[0] == 4) // Found a locator and going with it
                {
                    l = dist(&sprite[j],s);

                    if (l <= 1524)
                    {
                        if (t[0] == 1) t[0] = 0;
                        else t[0] = 5;
                    }
                    else
                    {
                        // Control speed here
                        if (l > 1524)
                        {
                            if (s->xvel < 256) s->xvel += 32;
                        }
                        else
                        {
                            if (s->xvel > 0) s->xvel -= 16;
                            else s->xvel = 0;
                        }
                    }

                    if (t[0] < 2) t[2]++;

                    if (x < 6144 && t[0] < 2 && t[2] > (GAMETICSPERSEC*4))
                    {
                        t[0] = 2+(krand()&2);
                        t[2] = 0;
                        actor[i].tempang = s->ang;
                    }
                }

                if (t[0] == 0 || t[0] == 5)
                {
                    if (t[0] == 0)
                        t[0] = 1;
                    else t[0] = 4;

                    j = s->owner = A_FindLocator(s->hitag,-1);
                    if (j == -1)
                    {
                        s->hitag = j = actor[i].t_data[5];
                        s->owner = A_FindLocator(j,-1);
                        j = s->owner;
                        if (j == -1)
                            KILLIT(i);
                    }
                    else s->hitag++;
                }

                // RECON_T4
                t[3] = G_GetAngleDelta(s->ang,a);
                s->ang += t[3]>>3;

                if (s->z < sprite[j].z-512)
                    s->z += 512;
                else if (s->z > sprite[j].z+512)
                    s->z -= 512;
                else s->z = sprite[j].z;
            }

            if (!A_CheckSoundPlaying(i,RECO_ROAM))
                A_PlaySound(RECO_ROAM,i);

            A_SetSprite(i,CLIPMASK0);

            goto BOLT;
        }

        case OOZ__STATIC:
        case OOZ2__STATIC:
            A_GetZLimits(i);

            j = (actor[i].floorz-actor[i].ceilingz)>>9;
            if (j > 255) j = 255;

            x = 25-(j>>1);
            if (x < 8) x = 8;
            else if (x > 48) x = 48;

            s->yrepeat = j;
            s->xrepeat = x;
            s->z = actor[i].floorz;

            goto BOLT;

        case GREENSLIME__STATIC:
        {
            int32_t p;
            DukePlayer_t *ps;

            // #ifndef VOLUMEONE
            if (!g_netServer && ud.multimode < 2)
            {
                if (g_noEnemies == 1)
                {
                    s->cstat = 32768;
                    goto BOLT;
                }
                else if (g_noEnemies == 2) s->cstat = 257;
            }
            // #endif

            t[1]+=128;

            if (sector[sect].floorstat&1)
                KILLIT(i);

            p = A_FindPlayer(s,&x);
            ps = g_player[p].ps;

            if (x > 20480)
            {
                actor[i].timetosleep++;
                if (actor[i].timetosleep > SLEEPTIME)
                {
                    actor[i].timetosleep = 0;
                    changespritestat(i, STAT_ZOMBIEACTOR);
                    goto BOLT;
                }
            }

            if (t[0] == -5) // FROZEN
            {
                t[3]++;
                if (t[3] > 280)
                {
                    s->pal = 0;
                    t[0] = 0;
                    goto BOLT;
                }
                A_Fall(i);
                s->cstat = 257;
                s->picnum = GREENSLIME+2;
                s->extra = 1;
                s->pal = 1;
                if ((j = A_IncurDamage(i)) >= 0)
                {
                    if (j == FREEZEBLAST) goto BOLT;
                    for (j=16; j >= 0 ; j--)
                    {
                        int32_t k = A_InsertSprite(SECT,SX,SY,SZ,GLASSPIECES+(j%3),-32,36,36,krand()&2047,32+(krand()&63),1024-(krand()&1023),i,5);
                        sprite[k].pal = 1;
                    }
                    A_PlaySound(GLASS_BREAKING,i);
                    KILLIT(i);
                }
                else if (x < 1024 && ps->quick_kick == 0)
                {
                    j = G_GetAngleDelta(ps->ang,getangle(SX-ps->pos.x,SY-ps->pos.y));
                    if (j > -128 && j < 128)
                        ps->quick_kick = 14;
                }

                goto BOLT;
            }

            if (x < 1596)
                s->cstat = 0;
            else s->cstat = 257;

            if (t[0] == -4) //On the player
            {
                if (sprite[ps->i].extra < 1)
                {
                    t[0] = 0;
                    goto BOLT;
                }

                setsprite(i,(vec3_t *)s);

                s->ang = ps->ang;

                if ((TEST_SYNC_KEY(g_player[p].sync->bits, SK_FIRE) || (ps->quick_kick > 0)) && sprite[ps->i].extra > 0)
                    if (ps->quick_kick > 0 ||
                        (PWEAPON(p, ps->curr_weapon, WorksLike) != HANDREMOTE_WEAPON && PWEAPON(p, ps->curr_weapon, WorksLike) != HANDBOMB_WEAPON && 
                        PWEAPON(p, ps->curr_weapon, WorksLike) != TRIPBOMB_WEAPON && ps->ammo_amount[ps->curr_weapon] >= 0))
                    {
                        for (x=0; x<8; x++)
                        {
                            j = A_InsertSprite(sect,s->x,s->y,s->z-(8<<8),SCRAP3+(krand()&3),-8,48,48,krand()&2047,(krand()&63)+64,-(krand()&4095)-(s->zvel>>2),i,5);
                            sprite[j].pal = 6;
                        }

                        A_PlaySound(SLIM_DYING,i);
                        A_PlaySound(SQUISHED,i);
                        if ((krand()&255) < 32)
                        {
                            j = A_Spawn(i,BLOODPOOL);
                            sprite[j].pal = 0;
                        }
                        ps->actors_killed ++;
                        t[0] = -3;
                        if (ps->somethingonplayer == i)
                            ps->somethingonplayer = -1;
                        KILLIT(i);
                    }

                s->z = ps->pos.z+ps->pyoff-t[2]+(8<<8);

                s->z += (100-ps->horiz)<<4;

                if (t[2] > 512)
                    t[2] -= 128;

                if (t[2] < 348)
                    t[2] += 128;

                if (ps->newowner >= 0)
                    G_ClearCameraView(ps);

                if (t[3]>0)
                {
                    static const char frames[] = {5,5,6,6,7,7,6,5};

                    s->picnum = GREENSLIME+frames[t[3]];

                    if (t[3] == 5)
                    {
                        sprite[ps->i].extra += -(5+(krand()&3));
                        A_PlaySound(SLIM_ATTACK,i);
                    }

                    if (t[3] < 7) t[3]++;
                    else t[3] = 0;

                }
                else
                {
                    s->picnum = GREENSLIME+5;
                    if (rnd(32))
                        t[3] = 1;
                }

                s->xrepeat = 20+(sintable[t[1]&2047]>>13);
                s->yrepeat = 15+(sintable[t[1]&2047]>>13);

                s->x = ps->pos.x + (sintable[(ps->ang+512)&2047]>>7);
                s->y = ps->pos.y + (sintable[ps->ang&2047]>>7);

                goto BOLT;
            }

            else if (s->xvel < 64 && x < 768)
            {
                if (ps->somethingonplayer == -1)
                {
                    ps->somethingonplayer = i;
                    if (t[0] == 3 || t[0] == 2) //Falling downward
                        t[2] = (12<<8);
                    else t[2] = -(13<<8); //Climbing up duke
                    t[0] = -4;
                }
            }

            if ((j = A_IncurDamage(i)) >= 0)
            {
                A_PlaySound(SLIM_DYING,i);

                ps->actors_killed ++;
                if (ps->somethingonplayer == i)
                    ps->somethingonplayer = -1;

                if (j == FREEZEBLAST)
                {
                    A_PlaySound(SOMETHINGFROZE,i);
                    t[0] = -5 ;
                    t[3] = 0 ;
                    goto BOLT;
                }

                if ((krand()&255) < 32)
                {
                    j = A_Spawn(i,BLOODPOOL);
                    sprite[j].pal = 0;
                }

                for (x=0; x<8; x++)
                {
                    j = A_InsertSprite(sect,s->x,s->y,s->z-(8<<8),SCRAP3+(krand()&3),-8,48,48,krand()&2047,(krand()&63)+64,-(krand()&4095)-(s->zvel>>2),i,5);
                    sprite[j].pal = 6;
                }
                t[0] = -3;
                KILLIT(i);
            }
            // All weap
            if (t[0] == -1) //Shrinking down
            {
                A_Fall(i);

                s->cstat &= 65535-8;
                s->picnum = GREENSLIME+4;

                //                    if(s->yrepeat > 62)
                //                      A_DoGuts(s,JIBS6,5,myconnectindex);

                if (s->xrepeat > 32) s->xrepeat -= krand()&7;
                if (s->yrepeat > 16) s->yrepeat -= krand()&7;
                else
                {
                    s->xrepeat = 40;
                    s->yrepeat = 16;
                    t[5] = -1;
                    t[0] = 0;
                }

                goto BOLT;
            }
            else if (t[0] != -2) A_GetZLimits(i);

            if (t[0] == -2) //On top of somebody
            {
                A_Fall(i);
                sprite[t[5]].xvel = 0;

                l = sprite[t[5]].ang;

                s->z = sprite[t[5]].z;
                s->x = sprite[t[5]].x+(sintable[(l+512)&2047]>>11);
                s->y = sprite[t[5]].y+(sintable[l&2047]>>11);

                s->picnum =  GREENSLIME+2+(g_globalRandom&1);

                if (s->yrepeat < 64) s->yrepeat+=2;
                else
                {
                    if (s->xrepeat < 32) s->xrepeat += 4;
                    else
                    {
                        t[0] = -1;
                        x = ldist(s,&sprite[t[5]]);
                        if (x < 768)
                        {
                            sprite[t[5]].xrepeat = 0;

                            // JBF 20041129: a slimer eating another enemy really ought
                            // to decrease the maximum kill count by one.
                            if (sprite[t[5]].extra > 0)
                                g_player[myconnectindex].ps->max_actors_killed--;
                        }
                    }
                }

                goto BOLT;
            }

            //Check randomly to see of there is an actor near
            if (rnd(32))
            {
                for (SPRITES_OF_SECT(sect, j))
                {
                    switch (DYNAMICTILEMAP(sprite[j].picnum))
                    {
                    case LIZTROOP__STATIC:
                    case LIZMAN__STATIC:
                    case PIGCOP__STATIC:
                    case NEWBEAST__STATIC:
                        if (ldist(s,&sprite[j]) < 768 && (klabs(s->z-sprite[j].z)<8192))   //Gulp them
                        {
                            t[5] = j;
                            t[0] = -2;
                            t[1] = 0;
                            goto BOLT;
                        }
                    }
                }
            }

            //Moving on the ground or ceiling

            if (t[0] == 0 || t[0] == 2)
            {
                s->picnum = GREENSLIME;

                if ((krand()&511) == 0)
                    A_PlaySound(SLIM_ROAM,i);

                if (t[0]==2)
                {
                    s->zvel = 0;
                    s->cstat &= (65535-8);

                    if ((sector[sect].ceilingstat&1) || (actor[i].ceilingz+6144) < s->z)
                    {
                        s->z += 2048;
                        t[0] = 3;
                        goto BOLT;
                    }
                }
                else
                {
                    s->cstat |= 8;
                    A_Fall(i);
                }

                if (everyothertime&1) A_SetSprite(i,CLIPMASK0);

                if (s->xvel > 96)
                {
                    s->xvel -= 2;
                    goto BOLT;
                }
                else
                {
                    if (s->xvel < 32) s->xvel += 4;
                    s->xvel = 64 - (sintable[(t[1]+512)&2047]>>9);

                    s->ang += G_GetAngleDelta(s->ang,
                                              getangle(ps->pos.x-s->x,ps->pos.y-s->y))>>3;
                    // TJR
                }

                s->xrepeat = 36 + (sintable[(t[1]+512)&2047]>>11);
                s->yrepeat = 16 + (sintable[t[1]&2047]>>13);

                if (rnd(4) && (sector[sect].ceilingstat&1) == 0 &&
                        klabs(actor[i].floorz-actor[i].ceilingz)
                        < (192<<8))
                {
                    s->zvel = 0;
                    t[0]++;
                }

            }

            if (t[0]==1)
            {
                s->picnum = GREENSLIME;
                if (s->yrepeat < 40) s->yrepeat+=8;
                if (s->xrepeat > 8) s->xrepeat-=4;
                if (s->zvel > -(2048+1024))
                    s->zvel -= 348;
                s->z += s->zvel;
                if (s->z < actor[i].ceilingz+4096)
                {
                    s->z = actor[i].ceilingz+4096;
                    s->xvel = 0;
                    t[0] = 2;
                }
            }

            if (t[0]==3)
            {
                s->picnum = GREENSLIME+1;

                A_Fall(i);

                if (s->z > actor[i].floorz-(8<<8))
                {
                    s->yrepeat-=4;
                    s->xrepeat+=2;
                }
                else
                {
                    if (s->yrepeat < (40-4)) s->yrepeat+=8;
                    if (s->xrepeat > 8) s->xrepeat-=4;
                }

                if (s->z > actor[i].floorz-2048)
                {
                    s->z = actor[i].floorz-2048;
                    t[0] = 0;
                    s->xvel = 0;
                }
            }
            goto BOLT;
        }

        case BOUNCEMINE__STATIC:
        if (s->xvel != 0)
        case MORTER__STATIC:
        {
            j = A_Spawn(i,(PLUTOPAK?FRAMEEFFECT1:FRAMEEFFECT1_13));
            actor[j].t_data[0] = 3;
        }
            /* fall-through */
        case HEAVYHBOMB__STATIC:
        {
            int32_t p;
            DukePlayer_t *ps;

            if ((s->cstat&32768))
            {
                t[2]--;
                if (t[2] <= 0)
                {
                    A_PlaySound(TELEPORTER,i);
                    A_Spawn(i,TRANSPORTERSTAR);
                    s->cstat = 257;
                }
                goto BOLT;
            }

            p = A_FindPlayer(s,&x);
            ps = g_player[p].ps;

            if (x < 1220) s->cstat &= ~257;
            else s->cstat |= 257;

            if (t[3] == 0)
            {
                if (A_IncurDamage(i) >= 0)
                {
                    t[3] = 1;
                    t[2] = 0;
                    l = 0;
                    s->xvel = 0;
                    goto DETONATEB;
                }
            }

            if (s->picnum != BOUNCEMINE)
            {
                A_Fall(i);

                if ((sector[sect].lotag != ST_1_ABOVE_WATER || actor[i].floorz != sector[sect].floorz) && s->z >= actor[i].floorz-(ZOFFSET) && s->yvel < 3)
                {
                    if (s->yvel > 0 || (s->yvel == 0 && actor[i].floorz == sector[sect].floorz))
                        A_PlaySound(PIPEBOMB_BOUNCE,i);
                    s->zvel = -((4-s->yvel)<<8);
                    if (sector[s->sectnum].lotag == ST_2_UNDERWATER)
                        s->zvel >>= 2;
                    s->yvel++;
                }
                if (s->z < actor[i].ceilingz)   // && sector[sect].lotag != ST_2_UNDERWATER )
                {
                    s->z = actor[i].ceilingz+(3<<8);
                    s->zvel = 0;
                }
            }

            {
                vec3_t tmpvect;

                tmpvect.x = (s->xvel*(sintable[(s->ang+512)&2047]))>>14;
                tmpvect.y = (s->xvel*(sintable[s->ang&2047]))>>14;
                tmpvect.z = s->zvel;
                j = A_MoveSprite(i,&tmpvect,CLIPMASK0);
            }

            actor[i].movflag = j;

            if (sector[SECT].lotag == ST_1_ABOVE_WATER && s->zvel == 0 && actor[i].floorz == sector[sect].floorz)
            {
                s->z += (32<<8);
                if (t[5] == 0)
                {
                    t[5] = 1;
                    A_Spawn(i,WATERSPLASH2);
                }
            }
            else t[5] = 0;

            if (t[3] == 0 && (s->picnum == BOUNCEMINE || s->picnum == MORTER) && (j || x < 844))
            {
                t[3] = 1;
                t[2] = 0;
                l = 0;
                s->xvel = 0;
                goto DETONATEB;
            }

            if (sprite[s->owner].picnum == APLAYER)
                l = P_Get(s->owner);
            else l = -1;

            if (s->xvel > 0)
            {
                s->xvel -= 5;
                if (sector[sect].lotag == ST_2_UNDERWATER)
                    s->xvel -= 10;

                if (s->xvel < 0)
                    s->xvel = 0;
                if (s->xvel&8) s->cstat ^= 4;
            }

            if ((j&49152) == 32768)
            {
                vec3_t davect;

                j &= (MAXWALLS-1);

                Bmemcpy(&davect, s, sizeof(vec3_t));
                A_DamageWall(i,j,&davect,s->picnum);

                Proj_BounceOffWall(s, j);
                s->xvel >>= 1;
            }

DETONATEB:
            // Pipebomb control set to timer? (see player.c)
            // TIMER_CONTROL
            if (s->picnum == HEAVYHBOMB && t[6] == 1)
            {
                /*                if(s->extra >= 1)
                                {
                                    s->extra--;
                                }

                                if(s->extra <= 0)
                                    s->lotag=911;
                */

                if (t[7] >= 1)
                    t[7]--;

                if (t[7] <= 0)
                    t[6] = 3;
            }

            if ((l >= 0 && g_player[l].ps->hbomb_on == 0 && t[6] == 2) || t[3] == 1)
                t[6] = 3;

            if (t[6] == 3)
            {
                t[2]++;

                if (t[2] == 2)
                {
                    int32_t j;

                    x = s->extra;
                    m = 0;
                    switch (DYNAMICTILEMAP(s->picnum))
                    {
                    case HEAVYHBOMB__STATIC:
                        m = g_pipebombBlastRadius;
                        break;
                    case MORTER__STATIC:
                        m = g_morterBlastRadius;
                        break;
                    case BOUNCEMINE__STATIC:
                        m = g_bouncemineBlastRadius;
                        break;
                    }

                    A_RadiusDamage(i, m,x>>2,x>>1,x-(x>>2),x);
                    j = A_Spawn(i,EXPLOSION2);
                    A_PlaySound(PIPEBOMB_EXPLODE,j);
                    if (s->zvel == 0)
                        A_Spawn(i,EXPLOSION2BOT);
                    for (x=0; x<8; x++)
                        RANDOMSCRAP;
                }

                if (s->yrepeat)
                {
                    s->yrepeat = 0;
                    goto BOLT;
                }

                if (t[2] > 20)
                {
                    if (s->owner != i || ud.respawn_items == 0)
                    {
                        KILLIT(i);
                    }
                    else
                    {
                        t[2] = g_itemRespawnTime;
                        A_Spawn(i,RESPAWNMARKERRED);
                        s->cstat = 32768;
                        s->yrepeat = 9;
                        goto BOLT;
                    }
                }
            }
            else if (s->picnum == HEAVYHBOMB && x < 788 && t[0] > 7 && s->xvel == 0)
                if (cansee(s->x,s->y,s->z-(8<<8),s->sectnum,ps->pos.x,ps->pos.y,ps->pos.z,ps->cursectnum))
                    if (ps->ammo_amount[HANDBOMB_WEAPON] < ps->max_ammo_amount[HANDBOMB_WEAPON])
                    {
                        if ((GametypeFlags[ud.coop] & GAMETYPE_WEAPSTAY) && s->owner == i)
                        {
                            for (j=0; j<ps->weapreccnt; j++)
                                if (ps->weaprecs[j] == s->picnum)
                                    goto BOLT;

                            if (ps->weapreccnt < MAX_WEAPONS)
                                ps->weaprecs[ps->weapreccnt++] = s->picnum;
                        }

                        P_AddAmmo(HANDBOMB_WEAPON,ps,1);
                        A_PlaySound(DUKE_GET,ps->i);

                        if ((ps->gotweapon & (1<<HANDBOMB_WEAPON)) == 0 || s->owner == ps->i)
                        {
                            int32_t doswitch = ((ps->weaponswitch & 1) ||
                                                PWEAPON(p, ps->curr_weapon, WorksLike) == HANDREMOTE_WEAPON);
                            P_AddWeapon(ps, HANDBOMB_WEAPON, doswitch);
                        }

                        if (sprite[s->owner].picnum != APLAYER)
                            P_PalFrom(ps, 32, 0,32,0);

                        if (s->owner != i || ud.respawn_items == 0)
                        {
                            if (s->owner == i && (GametypeFlags[ud.coop] & GAMETYPE_WEAPSTAY))
                                goto BOLT;
                            KILLIT(i);
                        }
                        else
                        {
                            t[2] = g_itemRespawnTime;
                            A_Spawn(i,RESPAWNMARKERRED);
                            s->cstat = 32768;
                        }
                    }

            if (t[0] < 8) t[0]++;
            goto BOLT;
        }

        case REACTORBURNT__STATIC:
        case REACTOR2BURNT__STATIC:
            goto BOLT;

        case REACTOR__STATIC:
        case REACTOR2__STATIC:
        {
            int32_t p;
            DukePlayer_t *ps;

            if (t[4] == 1)
            {
                for (SPRITES_OF_SECT(sect, j))
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

                goto BOLT;
            }

            if (t[1] >= 20)
            {
                t[4] = 1;
                goto BOLT;
            }

            p = A_FindPlayer(s,&x);
            ps = g_player[p].ps;

            t[2]++;
            if (t[2] == 4) t[2]=0;

            if (x < 4096)
            {
                if ((krand()&255) < 16)
                {
                    if (!A_CheckSoundPlaying(ps->i, DUKE_LONGTERM_PAIN))
                        A_PlaySound(DUKE_LONGTERM_PAIN,ps->i);

                    A_PlaySound(SHORT_CIRCUIT,i);

                    sprite[ps->i].extra --;

                    P_PalFrom(ps, 32, 32,0,0);
                }
                t[0] += 128;
                if (t[3] == 0)
                    t[3] = 1;
            }
            else t[3] = 0;

            if (t[1])
            {
                t[1]++;

                t[4] = s->z;
                s->z = sector[sect].floorz-(krand()%(sector[sect].floorz-sector[sect].ceilingz));

                switch (t[1])
                {
                case 3:
                    //Turn on all of those flashing sectoreffector.
                    A_RadiusDamage(i, 4096,
                                   g_impactDamage<<2,
                                   g_impactDamage<<2,
                                   g_impactDamage<<2,
                                   g_impactDamage<<2);

                    for (SPRITES_OF(STAT_STANDABLE, j))
                    {
                        if (sprite[j].picnum == MASTERSWITCH)
                            if (sprite[j].hitag == s->hitag)
                                if (sprite[j].yvel == 0)
                                    sprite[j].yvel = 1;
                    }
                    break;

                case 4:
                case 7:
                case 10:
                case 15:
                    for (SPRITES_OF_SECT(sect, j))
                        if (j != i)
                        {
                            A_DeleteSprite(j);
                            break;
                        }

                    break;
                }

                for (x=0; x<16; x++)
                    RANDOMSCRAP;

                s->z = t[4];
                t[4] = 0;
            }
            else
            {
                if (A_IncurDamage(i) >= 0)
                {
                    for (x=0; x<32; x++)
                        RANDOMSCRAP;
                    if (s->extra < 0)
                        t[1] = 1;
                }
            }
            goto BOLT;
        }

        case CAMERA1__STATIC:

            if (t[0] == 0)
            {
                t[1]+=8;
                if (g_damageCameras)
                {
                    if (A_IncurDamage(i) >= 0)
                    {
                        t[0] = 1; // static
                        s->cstat = 32768;
                        for (x=0; x<5; x++) RANDOMSCRAP;
                        goto BOLT;
                    }
                }

                if (s->hitag > 0)
                {
                    if (t[1] < s->hitag)
                        s->ang+=8;
                    else if (t[1] < s->hitag*3)
                        s->ang-=8;
                    else if (t[1] < (s->hitag<<2))
                        s->ang+=8;
                    else
                    {
                        t[1]=8;
                        s->ang+=16;
                    }
                }
            }
            goto BOLT;
        }

        if (!g_netServer && ud.multimode < 2 && A_CheckEnemySprite(s))
        {
            if (g_noEnemies == 1)
            {
                s->cstat = 32768;
                goto BOLT;
            }
            else if (g_noEnemies == 2)
            {
                s->cstat = 0;
                if (s->extra)
                    s->cstat = 257;
            }
        }

        if (G_HaveActor(sprite[i].picnum))
        {
            int32_t p = A_FindPlayer(s,&x);
            A_Execute(i,p,x);
        }
BOLT:
        i = nexti;
    }
}

ACTOR_STATIC void G_MoveMisc(void)  // STATNUM 5
{
    int32_t i = headspritestat[STAT_MISC];

    while (i >= 0)
    {
        const int32_t nexti = nextspritestat[i];

        int32_t l, x;
        int32_t *const t = actor[i].t_data;

        spritetype *const s = &sprite[i];
        int32_t sect = s->sectnum;  // XXX: not const
        int32_t switchpicnum;

        if (sect < 0 || s->xrepeat == 0)
            KILLIT(i);

        Bmemcpy(&actor[i].bpos, s, sizeof(vec3_t));

        switchpicnum = s->picnum;
        if (s->picnum > NUKEBUTTON && s->picnum <= NUKEBUTTON+3)
            switchpicnum = NUKEBUTTON;

        if (s->picnum > GLASSPIECES && s->picnum <= GLASSPIECES+2)
            switchpicnum = GLASSPIECES;

        if (s->picnum == INNERJAW+1)
            switchpicnum--;

        if ((s->picnum == MONEY+1) || (s->picnum == MAIL+1) || (s->picnum == PAPER+1))
            actor[i].floorz = s->z = getflorzofslope(s->sectnum,s->x,s->y);
        else switch (DYNAMICTILEMAP(switchpicnum))
            {
            case APLAYER__STATIC:
                s->cstat = 32768;
                goto BOLT;
            case NEON1__STATIC:
            case NEON2__STATIC:
            case NEON3__STATIC:
            case NEON4__STATIC:
            case NEON5__STATIC:
            case NEON6__STATIC:

                if ((tabledivide32_noinline(g_globalRandom, s->lotag+1)&31) > 4)
                    s->shade = -127;
                else s->shade = 127;
                goto BOLT;

            case BLOODSPLAT1__STATIC:
            case BLOODSPLAT2__STATIC:
            case BLOODSPLAT3__STATIC:
            case BLOODSPLAT4__STATIC:

                if (t[0] == 7*GAMETICSPERSEC) goto BOLT;
                s->z += 16+(krand()&15);
                t[0]++;
                if ((t[0]%9) == 0) s->yrepeat++;
                goto BOLT;

            case NUKEBUTTON__STATIC:
                //        case NUKEBUTTON+1:
                //        case NUKEBUTTON+2:
                //        case NUKEBUTTON+3:

                if (t[0])
                {
                    t[0]++;
                    if (t[0] == 8) s->picnum = NUKEBUTTON+1;
                    else if (t[0] == 16)
                    {
                        s->picnum = NUKEBUTTON+2;
                        g_player[P_Get(s->owner)].ps->fist_incs = 1;
                    }
                    if (g_player[P_Get(s->owner)].ps->fist_incs == GAMETICSPERSEC)
                        s->picnum = NUKEBUTTON+3;
                }
                goto BOLT;

            case FORCESPHERE__STATIC:
            {
                int32_t j;

                l = s->xrepeat;
                if (t[1] > 0)
                {
                    t[1]--;
                    if (t[1] == 0)
                        KILLIT(i);
                }
                if (actor[s->owner].t_data[1] == 0)
                {
                    if (t[0] < 64)
                    {
                        t[0]++;
                        l += 3;
                    }
                }
                else if (t[0] > 64)
                {
                    t[0]--;
                    l -= 3;
                }

                s->x = sprite[s->owner].x;
                s->y = sprite[s->owner].y;
                s->z = sprite[s->owner].z;
                s->ang += actor[s->owner].t_data[0];

                if (l > 64) l = 64;
                else if (l < 1) l = 1;

                s->xrepeat = l;
                s->yrepeat = l;
                s->shade = (l>>1)-48;

                for (j=t[0]; j > 0; j--)
                    A_SetSprite(i,CLIPMASK0);
                goto BOLT;
            }

            case WATERSPLASH2__STATIC:
                t[0]++;
                if (t[0] == 1)
                {
                    if (sector[sect].lotag != ST_1_ABOVE_WATER && sector[sect].lotag != ST_2_UNDERWATER)
                        KILLIT(i);
                    /*
                    else
                    {
                        l = getflorzofslope(sect,s->x,s->y)-s->z;
                        if( l > (16<<8) ) KILLIT(i);
                    }
                    else
                    */
                    if (!S_CheckSoundPlaying(i,ITEM_SPLASH))
                        A_PlaySound(ITEM_SPLASH,i);
                }
                if (t[0] == 3)
                {
                    t[0] = 0;
                    t[1]++;  // WATERSPLASH_T2
                }
                if (t[1] == 5)
                    A_DeleteSprite(i);
                goto BOLT;

            case FRAMEEFFECT1_13__STATIC:
                if (PLUTOPAK) goto BOLT;	// JBF: ideally this should never happen...
            case FRAMEEFFECT1__STATIC:

                if (s->owner >= 0)
                {
                    t[0]++;

                    if (t[0] > 7)
                    {
                        KILLIT(i);
                    }
                    else if (t[0] > 4)
                        s->cstat |= 512+2;
                    else if (t[0] > 2)
                        s->cstat |= 2;
                    s->xoffset = sprite[s->owner].xoffset;
                    s->yoffset = sprite[s->owner].yoffset;
                }
                goto BOLT;
            case INNERJAW__STATIC:
            {
                //        case INNERJAW+1:

                int32_t p = A_FindPlayer(s,&x);
                if (x < 512)
                {
                    P_PalFrom(g_player[p].ps, 32, 32,0,0);
                    sprite[g_player[p].ps->i].extra -= 4;
                }
            }

            case FIRELASER__STATIC:
                if (s->extra != 5)
                    s->extra = 5;
                else KILLIT(i);
                break;
            case TONGUE__STATIC:
                KILLIT(i);

            case MONEY__STATIC:
            case MAIL__STATIC:
            case PAPER__STATIC:

                s->xvel = (krand()&7)+(sintable[T1&2047]>>9);
                T1 += (krand()&63);
                if ((T1&2047) > 512 && (T1&2047) < 1596)
                {
                    if (sector[sect].lotag == ST_2_UNDERWATER)
                    {
                        if (s->zvel < 64)
                            s->zvel += (g_spriteGravity>>5)+(krand()&7);
                    }
                    else if (s->zvel < 144)
                        s->zvel += (g_spriteGravity>>5)+(krand()&7);
                }

                A_SetSprite(i,CLIPMASK0);

                if ((krand()&3) == 0)
                    setsprite(i,(vec3_t *)s);

                if (s->sectnum == -1)
                    KILLIT(i);
                l = getflorzofslope(s->sectnum,s->x,s->y);

                if (s->z > l)
                {
                    int32_t j;

                    s->z = l;

                    A_AddToDeleteQueue(i);
                    PN ++;

                    for (SPRITES_OF(STAT_MISC, j))
                        if (sprite[j].picnum == BLOODPOOL)
                            if (ldist(s,&sprite[j]) < 348)
                            {
                                s->pal = 2;
                                break;
                            }
                }

                break;

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

                if (s->xvel > 0) s->xvel--;
                else s->xvel = 0;

                if (++t[5] == (30*10))
                    KILLIT(i);

                if (s->zvel > 1024 && s->zvel < 1280)
                {
                    setsprite(i,(vec3_t *)s);
                    sect = s->sectnum;
                }

                getzsofslope(sect,s->x,s->y,&x,&l);
                if (x == l || sect < 0 || sect >= MAXSECTORS) KILLIT(i);

                if (s->z < l-(2<<8))
                {
                    if (t[1] < 2) t[1]++;
                    else if (sector[sect].lotag != ST_2_UNDERWATER)
                    {
                        t[1] = 0;
                        if (s->picnum == DUKELEG || s->picnum == DUKETORSO || s->picnum == DUKEGUN)
                        {
                            if (t[0] > 6) t[0] = 0;
                            else t[0]++;
                        }
                        else
                        {
                            if (t[0] > 2)
                                t[0] = 0;
                            else t[0]++;
                        }
                    }

                    if (s->zvel < 6144)
                    {
                        if (sector[sect].lotag == ST_2_UNDERWATER)
                        {
                            if (s->zvel < 1024)
                                s->zvel += 48;
                            else s->zvel = 1024;
                        }
                        else s->zvel += g_spriteGravity-50;
                    }

                    s->x += (s->xvel*sintable[(s->ang+512)&2047])>>14;
                    s->y += (s->xvel*sintable[s->ang&2047])>>14;
                    s->z += s->zvel;

                }
                else
                {
                    if (t[2] == 0)
                    {
                        if (s->sectnum == -1)
                            KILLIT(i);

                        if ((sector[s->sectnum].floorstat&2))
                            KILLIT(i);

                        t[2]++;
                    }
                    l = getflorzofslope(s->sectnum,s->x,s->y);

                    s->z = l-(2<<8);
                    s->xvel = 0;

                    if (s->picnum == JIBS6)
                    {
                        t[1]++;
                        if ((t[1]&3) == 0 && t[0] < 7)
                            t[0]++;
                        if (t[1] > 20)
                            KILLIT(i);
                    }
                    else
                    {
                        s->picnum = JIBS6;
                        t[0] = 0;
                        t[1] = 0;
                    }

                }
                goto BOLT;

            case BLOODPOOL__STATIC:
            case PUKE__STATIC:
            {
                int32_t p;
                DukePlayer_t *ps;

                if (t[0] == 0)
                {
                    t[0] = 1;
                    if (sector[sect].floorstat&2)
                    {
                        KILLIT(i);
                    }
                    else A_AddToDeleteQueue(i);
                }

                A_Fall(i);

                p = A_FindPlayer(s,&x);
                ps = g_player[p].ps;

                s->z = actor[i].floorz-(ZOFFSET);

                if (t[2] < 32)
                {
                    t[2]++;
                    if (actor[i].picnum == TIRE)
                    {
                        if (s->xrepeat < 64 && s->yrepeat < 64)
                        {
                            s->xrepeat += krand()&3;
                            s->yrepeat += krand()&3;
                        }
                    }
                    else
                    {
                        if (s->xrepeat < 32 && s->yrepeat < 32)
                        {
                            s->xrepeat += krand()&3;
                            s->yrepeat += krand()&3;
                        }
                    }
                }

                if (x < 844 && s->xrepeat > 6 && s->yrepeat > 6)
                {
                    if (s->pal == 0 && s->picnum != PUKE && (krand()&255) < 16)
                    {
                        if (ps->inv_amount[GET_BOOTS] > 0)
                            ps->inv_amount[GET_BOOTS]--;
                        else
                        {
                            if (!A_CheckSoundPlaying(ps->i,DUKE_LONGTERM_PAIN))
                                A_PlaySound(DUKE_LONGTERM_PAIN,ps->i);
                            sprite[ps->i].extra --;

                            P_PalFrom(ps, 32, 16,0,0);
                        }
                    }

                    if (t[1] == 1) goto BOLT;
                    t[1] = 1;

                    if (actor[i].picnum == TIRE)
                        ps->footprintcount = 10;
                    else ps->footprintcount = 3;

                    ps->footprintpal = s->pal;
                    ps->footprintshade = s->shade;

                    if (t[2] == 32)
                    {
                        s->xrepeat -= 6;
                        s->yrepeat -= 6;
                    }
                }
                else t[1] = 0;
                goto BOLT;
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
                if (!G_HaveActor(sprite[i].picnum))
                    goto BOLT;
                {
                    int32_t p = A_FindPlayer(s,&x);
                    A_Execute(i,p,x);
                }
                goto BOLT;
            }

            case SHELL__STATIC:
            case SHOTGUNSHELL__STATIC:

                A_SetSprite(i,CLIPMASK0);

                if (sect < 0 || (sector[sect].floorz + 256) < s->z)
                    KILLIT(i);

                if (sector[sect].lotag == ST_2_UNDERWATER)
                {
                    t[1]++;
                    if (t[1] > 8)
                    {
                        t[1] = 0;
                        t[0]++;
                        t[0] &= 3;
                    }
                    if (s->zvel < 128) s->zvel += (g_spriteGravity/13); // 8
                    else s->zvel -= 64;
                    if (s->xvel > 0)
                        s->xvel -= 4;
                    else s->xvel = 0;
                }
                else
                {
                    t[1]++;
                    if (t[1] > 3)
                    {
                        t[1] = 0;
                        t[0]++;
                        t[0] &= 3;
                    }
                    if (s->zvel < 512) s->zvel += (g_spriteGravity/3); // 52;
                    if (s->xvel > 0)
                        s->xvel --;
                    //                else KILLIT(i);
                }

                goto BOLT;

            case GLASSPIECES__STATIC:
                //        case GLASSPIECES+1:
                //        case GLASSPIECES+2:

                A_Fall(i);

                if (s->zvel > 4096) s->zvel = 4096;
                if (sect < 0)
                    KILLIT(i);

                if (s->z == actor[i].floorz-(ZOFFSET) && t[0] < 3)
                {
                    s->zvel = -((3-t[0])<<8)-(krand()&511);
                    if (sector[sect].lotag == ST_2_UNDERWATER)
                        s->zvel >>= 1;
                    s->xrepeat >>= 1;
                    s->yrepeat >>= 1;
                    if (rnd(96))
                        setsprite(i,(vec3_t *)s);
                    t[0]++;//Number of bounces
                }
                else if (t[0] == 3)
                    KILLIT(i);

                if (s->xvel > 0)
                {
                    s->xvel -= 2;
                    s->cstat = ((s->xvel&3)<<2);
                }
                else s->xvel = 0;

                A_SetSprite(i,CLIPMASK0);

                goto BOLT;
            }

        if (PN >= SCRAP6 && PN <= SCRAP5+3)
        {
            if (s->xvel > 0)
                s->xvel--;
            else s->xvel = 0;

            if (s->zvel > 1024 && s->zvel < 1280)
            {
                setsprite(i,(vec3_t *)s);
                sect = s->sectnum;
            }

            if (s->z < sector[sect].floorz-(2<<8))
            {
                if (t[1] < 1) t[1]++;
                else
                {
                    t[1] = 0;

                    if (s->picnum < SCRAP6+8)
                    {
                        if (t[0] > 6)
                            t[0] = 0;
                        else t[0]++;
                    }
                    else
                    {
                        if (t[0] > 2)
                            t[0] = 0;
                        else t[0]++;
                    }
                }
                if (s->zvel < 4096) s->zvel += g_spriteGravity-50;
                s->x += (s->xvel*sintable[(s->ang+512)&2047])>>14;
                s->y += (s->xvel*sintable[s->ang&2047])>>14;
                s->z += s->zvel;
            }
            else
            {
                if (s->picnum == SCRAP1 && s->yvel > 0 && s->yvel < MAXUSERTILES)
                {
                    int32_t j = A_Spawn(i, s->yvel);

                    setsprite(j,(vec3_t *)s);
                    A_GetZLimits(j);
                    sprite[j].hitag = sprite[j].lotag = 0;
                }

                KILLIT(i);
            }
            goto BOLT;
        }

BOLT:
        i = nexti;
    }
}


// i: SE spritenum
static void HandleSE31(int32_t i, int32_t setfloorzp, int32_t zref, int32_t t2val, int32_t movesignexp)
{
    const spritetype *s = &sprite[i];
    sectortype *const sc = &sector[sprite[i].sectnum];
    int32_t *const t = actor[i].t_data;

    if (klabs(sc->floorz - zref) < SP)
    {
        if (setfloorzp)
            sc->floorz = zref;

        t[2] = t2val;
        t[0] = 0;
        t[3] = s->hitag;
        A_CallSound(s->sectnum,i);
    }
    else
    {
        int32_t j;
        int32_t l = ksgn(movesignexp)*SP;

        sc->floorz += l;

        for (SPRITES_OF_SECT(s->sectnum, j))
        {
            if (sprite[j].picnum == APLAYER && sprite[j].owner >= 0)
                if (g_player[P_Get(j)].ps->on_ground == 1)
                    g_player[P_Get(j)].ps->pos.z += l;

            if (sprite[j].zvel == 0 && sprite[j].statnum != STAT_EFFECTOR && sprite[j].statnum != STAT_PROJECTILE)
            {
                actor[j].bpos.z = sprite[j].z += l;
                actor[j].floorz = sc->floorz;
            }
        }
    }
}

// s: SE sprite
static void MaybeTrainKillPlayer(const spritetype *s, int32_t dosetopos)
{
    int32_t p;

    for (TRAVERSE_CONNECT(p))
    {
        DukePlayer_t *const ps = g_player[p].ps;

        if (sprite[ps->i].extra > 0)
        {
            int16_t k = ps->cursectnum;

            updatesector(ps->pos.x,ps->pos.y,&k);
            if ((k == -1 && ud.noclip == 0) || (k == s->sectnum && ps->cursectnum != s->sectnum))
            {
                ps->pos.x = s->x;
                ps->pos.y = s->y;

                if (dosetopos)
                {
                    ps->opos.x = ps->pos.x;
                    ps->opos.y = ps->pos.y;
                }

                ps->cursectnum = s->sectnum;

                setsprite(ps->i,(vec3_t *)s);
                P_QuickKill(ps);
            }
        }
    }
}

// i: SE spritenum
static void MaybeTrainKillEnemies(int32_t i, int32_t numguts)
{
    int32_t j = headspritesect[sprite[OW].sectnum];

    while (j >= 0)
    {
        const int32_t nextj = nextspritesect[j];

        if (sprite[j].extra >= 0 && sprite[j].statnum == STAT_ACTOR && A_CheckEnemySprite(&sprite[j]))
        {
            int16_t k = sprite[j].sectnum;

            updatesector(sprite[j].x,sprite[j].y,&k);
            if (k == sprite[i].sectnum)
            {
                A_DoGutsDir(j,JIBS6,numguts);
                A_PlaySound(SQUISHED,j);
                A_DeleteSprite(j);
            }
        }

        j = nextj;
    }
}

ACTOR_STATIC void G_MoveEffectors(void)   //STATNUM 3
{
    walltype *wal;
    int32_t q=0, j, k, l, m, x;
    int32_t i = headspritestat[STAT_EFFECTOR];

    while (i >= 0)
    {
        const int32_t nexti = nextspritestat[i];
        spritetype *const s = &sprite[i];

        sectortype *const sc = &sector[s->sectnum];
        const int32_t st = s->lotag;
        const int32_t sh = s->hitag;

        int32_t *const t = &actor[i].t_data[0];

        switch (st)
        {
        case SE_0_ROTATING_SECTOR:
        {
            int32_t zchange = 0;

            j = s->owner;

            if (sprite[j].lotag == UINT16_MAX)
                KILLIT(i);

            q = sc->extra>>3;
            l = 0;

            if (sc->lotag == ST_30_ROTATE_RISE_BRIDGE)
            {
                q >>= 2;

                if (sprite[i].extra == 1)
                {
                    if (actor[i].tempang < 256)
                    {
                        actor[i].tempang += 4;
                        if (actor[i].tempang >= 256)
                            A_CallSound(s->sectnum,i);
                        if (s->clipdist) l = 1;
                        else l = -1;
                    }
                    else actor[i].tempang = 256;

                    if (sc->floorz > s->z)   //z's are touching
                    {
                        sc->floorz -= 512;
                        zchange = -512;
                        if (sc->floorz < s->z)
                            sc->floorz = s->z;
                    }
                    else if (sc->floorz < s->z)   //z's are touching
                    {
                        sc->floorz += 512;
                        zchange = 512;
                        if (sc->floorz > s->z)
                            sc->floorz = s->z;
                    }
                }
                else if (sprite[i].extra == 3)
                {
                    if (actor[i].tempang > 0)
                    {
                        actor[i].tempang -= 4;
                        if (actor[i].tempang <= 0)
                            A_CallSound(s->sectnum,i);
                        if (s->clipdist) l = -1;
                        else l = 1;
                    }
                    else actor[i].tempang = 0;

                    if (sc->floorz > T4)   //z's are touching
                    {
                        sc->floorz -= 512;
                        zchange = -512;
                        if (sc->floorz < T4)
                            sc->floorz = T4;
                    }
                    else if (sc->floorz < T4)   //z's are touching
                    {
                        sc->floorz += 512;
                        zchange = 512;
                        if (sc->floorz > T4)
                            sc->floorz = T4;
                    }
                }
            }
            else
            {
                if (actor[j].t_data[0] == 0) break;
                if (actor[j].t_data[0] == 2) KILLIT(i);

                if (sprite[j].ang > 1024)
                    l = -1;
                else l = 1;
                if (t[3] == 0)
                    t[3] = ldist(s,&sprite[j]);
                s->xvel = t[3];
                s->x = sprite[j].x;
                s->y = sprite[j].y;
            }

            s->ang += (l*q);
            t[2] += (l*q);

            if (l && (sc->floorstat&64))
            {
                int32_t p;

                for (TRAVERSE_CONNECT(p))
                {
                    DukePlayer_t *const ps = g_player[p].ps;

                    if (ps->cursectnum == s->sectnum && ps->on_ground == 1)
                    {

                        ps->ang += (l*q);
                        ps->ang &= 2047;

                        ps->pos.z += zchange;

                        vec2_t r;
                        rotatepoint(*(vec2_t *)&sprite[j],*(vec2_t *)&ps->pos,(q*l),&r);

                        ps->bobpos.x += r.x-ps->pos.x;
                        ps->bobpos.y += r.y-ps->pos.y;

                        *(vec2_t *)&ps->pos = r;

                        if (sprite[ps->i].extra <= 0)
                            *(vec2_t *)&sprite[ps->i] = r;
                    }
                }

                for (SPRITES_OF_SECT(s->sectnum, p))
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

                            if (move_rotfixed_sprite(p, j, t[2]))
                                rotatepoint(*(vec2_t *)&sprite[j], *(vec2_t *)&sprite[p], (q * l), (vec2_t *)&sprite[p].x);
                        }
                }

            }
            else if (l==0 && (sc->floorstat&64))
            {
                int32_t p;

                // fix for jittering of sprites in halted rotating sectors
                for (SPRITES_OF_SECT(s->sectnum, p))
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

            A_MoveSector(i);
        }
        break;

        case SE_1_PIVOT: //Nothing for now used as the pivot
            if (s->owner == -1) //Init
            {
                s->owner = i;

                for (SPRITES_OF(STAT_EFFECTOR, j))
                {
                    if (sprite[j].lotag == SE_19_EXPLOSION_LOWERS_CEILING && sprite[j].hitag == sh)
                    {
                        t[0] = 0;
                        break;
                    }
                }
            }
            break;

        case 6:
            k = sc->extra;

            if (t[4] > 0)
            {
                t[4]--;
                if (t[4] >= (k-(k>>3)))
                    s->xvel -= (k>>5);
                if (t[4] > ((k>>1)-1) && t[4] < (k-(k>>3)))
                    s->xvel = 0;
                if (t[4] < (k>>1))
                    s->xvel += (k>>5);
                if (t[4] < ((k>>1)-(k>>3)))
                {
                    t[4] = 0;
                    s->xvel = k;
                }
            }
            else s->xvel = k;

            for (SPRITES_OF(STAT_EFFECTOR, j))
            {
                if (sprite[j].lotag == SE_14_SUBWAY_CAR && sh == sprite[j].hitag && actor[j].t_data[0] == t[0])
                {
                    sprite[j].xvel = s->xvel;
                    //                        if( t[4] == 1 )
                    {
                        if (actor[j].t_data[5] == 0)
                            actor[j].t_data[5] = dist(&sprite[j],s);
                        x = ksgn(dist(&sprite[j],s)-actor[j].t_data[5]);
                        if (sprite[j].extra)
                            x = -x;
                        s->xvel += x;
                    }
                    actor[j].t_data[4] = t[4];
                }
            }
            x = 0;  // XXX: This assignment is dead?


        case SE_14_SUBWAY_CAR:
            if (s->owner==-1)
                s->owner = A_FindLocator((int16_t)t[3],(int16_t)t[0]);

            if (s->owner == -1)
            {
                // debugging subway cars (mapping-wise) is freakin annoying
                // let's at least have a helpful message...
                Bsprintf(tempbuf,"Could not find any locators in sector %d"
                         " for SE# 6 or 14 with hitag %d.\n", (int)t[0], (int)t[3]);
                G_GameExit(tempbuf);
            }

            j = ldist(&sprite[s->owner],s);

            if (j < 1024L)
            {
                if (st==SE_6_SUBWAY)
                    if (sprite[s->owner].hitag&1)
                        t[4]=sc->extra; //Slow it down
                t[3]++;
                s->owner = A_FindLocator(t[3],t[0]);
                if (s->owner==-1)
                {
                    t[3]=0;
                    s->owner = A_FindLocator(0,t[0]);
                }
            }

            if (s->xvel)
            {
                int32_t p;
#ifdef YAX_ENABLE
                int32_t firstrun = 1;
#endif
                x = getangle(sprite[s->owner].x-s->x,sprite[s->owner].y-s->y);
                q = G_GetAngleDelta(s->ang,x)>>3;

                t[2] += q;
                s->ang += q;

                if (s->xvel == sc->extra)
                {
                    if ((sc->floorstat&1) == 0 && (sc->ceilingstat&1) == 0)
                    {
                        if (!S_CheckSoundPlaying(i,actor[i].lastvx))
                            A_PlaySound(actor[i].lastvx,i);
                    }
                    else if (ud.monsters_off == 0 && sc->floorpal == 0 && (sc->floorstat&1) && rnd(8))
                    {
                        p = A_FindPlayer(s,&x);
                        if (x < 20480)
                        {
                            j = s->ang;
                            s->ang = getangle(s->x-g_player[p].ps->pos.x,s->y-g_player[p].ps->pos.y);
                            A_Shoot(i,RPG);
                            s->ang = j;
                        }
                    }
                }

                if (s->xvel <= 64 && (sc->floorstat&1) == 0 && (sc->ceilingstat&1) == 0)
                    S_StopEnvSound(actor[i].lastvx,i);

                if ((sc->floorz-sc->ceilingz) < (108<<8))
                {
                    if (ud.noclip == 0 && s->xvel >= 192)
                        MaybeTrainKillPlayer(s, 0);
                }

                m = (s->xvel*sintable[(s->ang+512)&2047])>>14;
                x = (s->xvel*sintable[s->ang&2047])>>14;

                for (TRAVERSE_CONNECT(p))
                {
                    DukePlayer_t *const ps = g_player[p].ps;

                    if (ps->cursectnum < 0)
                    {
                        // might happen when squished into void space
//                        initprintf("cursectnum < 0!\n");
                        break;
                    }

                    if (sector[ps->cursectnum].lotag != ST_2_UNDERWATER)
                    {
                        if (g_playerSpawnPoints[p].sect == s->sectnum)
                        {
                            g_playerSpawnPoints[p].pos.x += m;
                            g_playerSpawnPoints[p].pos.y += x;
                        }

                        if (s->sectnum == sprite[ps->i].sectnum
#ifdef YAX_ENABLE
                                || (t[9]>=0 && t[9] == sprite[ps->i].sectnum)
#endif
                            )
                        {
                            rotatepoint(*(vec2_t *)s, *(vec2_t *)&ps->pos, q, (vec2_t *)&ps->pos);

                            ps->pos.x += m;
                            ps->pos.y += x;

                            ps->bobpos.x += m;
                            ps->bobpos.y += x;

                            ps->ang += q;
                            ps->ang &= 2047;

                            if (g_netServer || numplayers > 1)
                            {
                                ps->opos.x = ps->pos.x;
                                ps->opos.y = ps->pos.y;
                            }
                            if (sprite[ps->i].extra <= 0)
                            {
                                sprite[ps->i].x = ps->pos.x;
                                sprite[ps->i].y = ps->pos.y;
                            }
                        }
                    }
                }

                // NOTE: special loop handling
                j = headspritesect[s->sectnum];
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

                        if (move_rotfixed_sprite(j, s-sprite, t[2]))
                            rotatepoint(*(vec2_t *)s,*(vec2_t *)&sprite[j],q,(vec2_t *)&sprite[j].x);

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
                        if (t[9]>=0 && firstrun)
                        {
                            firstrun = 0;
                            j = headspritesect[t[9]];
                        }
                    }
#endif
                }

                A_MoveSector(i);
                setsprite(i,(vec3_t *)s);

                if ((sc->floorz-sc->ceilingz) < (108<<8))
                {
                    if (ud.noclip == 0 && s->xvel >= 192)
                        MaybeTrainKillPlayer(s, 1);

                    MaybeTrainKillEnemies(i, 72);
                }
            }
            else
            {
                // fix for jittering of sprites in halted subways
                for (SPRITES_OF_SECT(s->sectnum, j))
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
            if (s->owner == -1)
            {
                t[3] = !t[3];
                s->owner = A_FindLocator(t[3],t[0]);
            }
            else
            {

                if (t[4] == 1) // Starting to go
                {
                    if (ldist(&sprite[s->owner],s) < (2048-128))
                        t[4] = 2;
                    else
                    {
                        if (s->xvel == 0)
                            G_OperateActivators(s->hitag+(!t[3]),-1);
                        if (s->xvel < 256)
                            s->xvel += 16;
                    }
                }
                if (t[4] == 2)
                {
                    l = FindDistance2D(sprite[s->owner].x-s->x,sprite[s->owner].y-s->y);

                    if (l <= 128)
                        s->xvel = 0;

                    if (s->xvel > 0)
                        s->xvel -= 16;
                    else
                    {
                        s->xvel = 0;
                        G_OperateActivators(s->hitag+(int16_t)t[3],-1);
                        s->owner = -1;
                        s->ang += 1024;
                        t[4] = 0;
                        G_OperateForceFields(i,s->hitag);

                        for (SPRITES_OF_SECT(s->sectnum, j))
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

            if (s->xvel)
            {
                int32_t p;

                l = (s->xvel*sintable[(s->ang+512)&2047])>>14;
                x = (s->xvel*sintable[s->ang&2047])>>14;

                if ((sc->floorz-sc->ceilingz) < (108<<8))
                    if (ud.noclip == 0)
                        MaybeTrainKillPlayer(s, 0);

                for (TRAVERSE_CONNECT(p))
                {
                    DukePlayer_t *const ps = g_player[p].ps;

                    if (sprite[ps->i].sectnum == s->sectnum)
                    {
                        ps->pos.x += l;
                        ps->pos.y += x;

                        if (g_netServer || numplayers > 1)
                        {
                            ps->opos.x = ps->pos.x;
                            ps->opos.y = ps->pos.y;
                        }

                        ps->bobpos.x += l;
                        ps->bobpos.y += x;
                    }

                    if (g_playerSpawnPoints[p].sect == s->sectnum)
                    {
                        g_playerSpawnPoints[p].pos.x += l;
                        g_playerSpawnPoints[p].pos.y += x;
                    }
                }

                for (SPRITES_OF_SECT(s->sectnum, j))
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

                A_MoveSector(i);
                setsprite(i,(vec3_t *)s);

                if (sc->floorz-sc->ceilingz < (108<<8))
                {
                    if (ud.noclip == 0)
                        MaybeTrainKillPlayer(s, 1);

                    MaybeTrainKillEnemies(i, 24);
                }
            }

            break;


        case SE_2_EARTHQUAKE://Quakes
            if (t[4] > 0 && t[0] == 0)
            {
                if (t[4] < sh)
                    t[4]++;
                else t[0] = 1;
            }

            if (t[0] > 0)
            {
                int32_t p, nextj;

                t[0]++;

                s->xvel = 3;

                if (t[0] > 96)
                {
                    t[0] = -1; //Stop the quake
                    t[4] = -1;
                    KILLIT(i);
                }
                else
                {
                    if ((t[0]&31) ==  8)
                    {
                        g_earthquakeTime = 48;
                        A_PlaySound(EARTHQUAKE,g_player[screenpeek].ps->i);
                    }

                    if (klabs(sc->floorheinum-t[5]) < 8)
                        sc->floorheinum = t[5];
                    else sc->floorheinum += (ksgn(t[5]-sc->floorheinum)<<4);
                }

                m = (s->xvel*sintable[(s->ang+512)&2047])>>14;
                x = (s->xvel*sintable[s->ang&2047])>>14;


                for (TRAVERSE_CONNECT(p))
                {
                    DukePlayer_t *const ps = g_player[p].ps;

                    if (ps->cursectnum == s->sectnum && ps->on_ground)
                    {
                        ps->pos.x += m;
                        ps->pos.y += x;

                        ps->bobpos.x += m;
                        ps->bobpos.y += x;
                    }
                }

                for (SPRITES_OF_SECT_SAFE(s->sectnum, j, nextj))
                {
                    if (sprite[j].picnum != SECTOREFFECTOR)
                    {
                        sprite[j].x+=m;
                        sprite[j].y+=x;
                        setsprite(j,(vec3_t *)&sprite[j]);
                    }
                }

                A_MoveSector(i);
                setsprite(i,(vec3_t *)s);
            }
            break;

            //Flashing sector lights after reactor EXPLOSION2

        case SE_3_RANDOM_LIGHTS_AFTER_SHOT_OUT:
        {
            if (t[4] == 0) break;

            //    if(t[5] > 0) { t[5]--; break; }

            if ((tabledivide32_noinline(g_globalRandom, sh+1)&31) < 4 && !t[2])
            {
                //       t[5] = 4+(g_globalRandom&7);
                sc->ceilingpal = s->owner>>8;
                sc->floorpal = s->owner&0xff;
                t[0] = s->shade + (g_globalRandom&15);
            }
            else
            {
                //       t[5] = 4+(g_globalRandom&3);
                sc->ceilingpal = s->pal;
                sc->floorpal = s->pal;
                t[0] = t[3];
            }

            sc->ceilingshade = t[0];
            sc->floorshade = t[0];

            wal = &wall[sc->wallptr];

            for (x=sc->wallnum; x > 0; x--,wal++)
            {
                if (wal->hitag != 1)
                {
                    wal->shade = t[0];
                    if ((wal->cstat&2) && wal->nextwall >= 0)
                    {
                        wall[wal->nextwall].shade = wal->shade;
                    }
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

            if ((tabledivide32_noinline(g_globalRandom, sh+1)&31) < 4)
            {
                t[1] = s->shade + (g_globalRandom&15);//Got really bright
                t[0] = s->shade + (g_globalRandom&15);
                sc->ceilingpal = s->owner>>8;
                sc->floorpal = s->owner&0xff;
                j = 1;
            }
            else
            {
                t[1] = t[2];
                t[0] = t[3];

                sc->ceilingpal = s->pal;
                sc->floorpal = s->pal;

                j = 0;
            }

            sc->floorshade = t[1];
            sc->ceilingshade = t[1];

            wal = &wall[sc->wallptr];

            for (x=sc->wallnum; x > 0; x--,wal++)
            {
                if (j) wal->pal = (s->owner&0xff);
                else wal->pal = s->pal;

                if (wal->hitag != 1)
                {
                    wal->shade = t[0];
                    if ((wal->cstat&2) && wal->nextwall >= 0)
                        wall[wal->nextwall].shade = wal->shade;
                }
            }

            for (SPRITES_OF_SECT(SECT, j))
            {
                if (sprite[j].cstat&16 && A_CheckSpriteFlags(j,SFLAG_NOSHADE) == 0)
                {
                    if (sc->ceilingstat&1)
                        sprite[j].shade = sc->ceilingshade;
                    else sprite[j].shade = sc->floorshade;
                }
            }

            if (t[4])
                KILLIT(i);

            break;
        }

            //BOSS
        case SE_5:
        {
            const int32_t p = A_FindPlayer(s,&x);
            DukePlayer_t *const ps = g_player[p].ps;

            if (x < 8192)
            {
                j = s->ang;
                s->ang = getangle(s->x-ps->pos.x,s->y-ps->pos.y);
                A_Shoot(i,FIRELASER);
                s->ang = j;
            }

            if (s->owner==-1) //Start search
            {
                t[4]=0;
                l = INT32_MAX;
                while (1) //Find the shortest dist
                {
                    s->owner = A_FindLocator((int16_t)t[4],-1); //t[0] hold sectnum

                    if (s->owner==-1) break;

                    m = ldist(&sprite[ps->i],&sprite[s->owner]);

                    if (l > m)
                    {
                        q = s->owner;
                        l = m;
                    }

                    t[4]++;
                }

                s->owner = q;
                s->zvel = ksgn(sprite[q].z-s->z)<<4;
            }

            if (ldist(&sprite[s->owner],s) < 1024)
            {
                int16_t ta;
                ta = s->ang;
                s->ang = getangle(ps->pos.x-s->x,ps->pos.y-s->y);
                s->ang = ta;
                s->owner = -1;
                goto BOLT;

            }
            else s->xvel=256;

            x = getangle(sprite[s->owner].x-s->x,sprite[s->owner].y-s->y);
            q = G_GetAngleDelta(s->ang,x)>>3;
            s->ang += q;

            if (rnd(32))
            {
                t[2]+=q;
                sc->ceilingshade = 127;
            }
            else
            {
                t[2] += G_GetAngleDelta(t[2]+512,getangle(ps->pos.x-s->x,ps->pos.y-s->y))>>2;
                sc->ceilingshade = 0;
            }

            if (A_IncurDamage(i) >= 0)
            {
                if (++t[3] == 5)
                {
                    s->zvel += 1024;
                    P_DoQuote(QUOTE_WASTED, g_player[myconnectindex].ps);
                }
            }

            s->z += s->zvel;
            sc->ceilingz += s->zvel;
            sector[t[0]].ceilingz += s->zvel;
            A_MoveSector(i);
            setsprite(i,(vec3_t *)s);
            break;
        }

        case SE_8_UP_OPEN_DOOR_LIGHTS:
        case SE_9_DOWN_OPEN_DOOR_LIGHTS:

            // work only if its moving

            j = -1;

            if (actor[i].t_data[4])
            {
                actor[i].t_data[4]++;
                if (actor[i].t_data[4] > 8)
                    KILLIT(i);
                j = 1;
            }
            else j = GetAnimationGoal(&sc->ceilingz);

            if (j >= 0)
            {
                if ((sc->lotag&0x8000) || actor[i].t_data[4])
                    x = -t[3];
                else
                    x = t[3];

                if (st == SE_9_DOWN_OPEN_DOOR_LIGHTS)
                    x = -x;

                for (SPRITES_OF(STAT_EFFECTOR, j))
                {
                    if (sprite[j].lotag == st && sprite[j].hitag == sh)
                    {
                        int32_t sn = sprite[j].sectnum;

                        m = sprite[j].shade;

                        wal = &wall[sector[sn].wallptr];

                        for (l=sector[sn].wallnum; l>0; l--,wal++)
                        {
                            if (wal->hitag != 1)
                            {
                                wal->shade+=x;

                                if (wal->shade < m)
                                    wal->shade = m;
                                else if (wal->shade > actor[j].t_data[2])
                                    wal->shade = actor[j].t_data[2];

                                if (wal->nextwall >= 0)
                                    if (wall[wal->nextwall].hitag != 1)
                                        wall[wal->nextwall].shade = wal->shade;
                            }
                        }

                        sector[sn].floorshade   += x;
                        sector[sn].ceilingshade += x;

                        if (sector[sn].floorshade < m)
                            sector[sn].floorshade = m;
                        else if (sector[sn].floorshade > actor[j].t_data[0])
                            sector[sn].floorshade = actor[j].t_data[0];

                        if (sector[sn].ceilingshade < m)
                            sector[sn].ceilingshade = m;
                        else if (sector[sn].ceilingshade > actor[j].t_data[1])
                            sector[sn].ceilingshade = actor[j].t_data[1];

                    }
                }
            }
            break;

        case SE_10_DOOR_AUTO_CLOSE:
            // XXX: 32791, what the hell?
            if ((sc->lotag&0xff) == ST_27_STRETCH_BRIDGE || (sc->floorz > sc->ceilingz && (sc->lotag&0xff) != ST_23_SWINGING_DOOR) || sc->lotag == 32791)
            {
                int32_t p;

                j = 1;

                if ((sc->lotag&0xff) != ST_27_STRETCH_BRIDGE)
                    for (TRAVERSE_CONNECT(p))
                        if (sc->lotag != ST_30_ROTATE_RISE_BRIDGE && sc->lotag != ST_31_TWO_WAY_TRAIN && sc->lotag != 0)
                            if (s->sectnum == sprite[g_player[p].ps->i].sectnum)
                                j = 0;

                if (j == 1)
                {
                    if (t[0] > sh)
                        switch (sector[s->sectnum].lotag)
                        {
                        case ST_20_CEILING_DOOR:
                        case ST_21_FLOOR_DOOR:
                        case ST_22_SPLITTING_DOOR:
                        case ST_26_SPLITTING_ST_DOOR:
                            if (GetAnimationGoal(&sector[s->sectnum].ceilingz) >= 0)
                                break;
                        default:
                            G_ActivateBySector(s->sectnum,i);
                            t[0] = 0;
                            break;
                        }
                    else t[0]++;
                }
            }
            else t[0]=0;
            break;

        case SE_11_SWINGING_DOOR: //Swingdoor

            if (t[5] > 0)
            {
                t[5]--;
                break;
            }

            if (t[4])
            {
                int32_t endwall = sc->wallptr+sc->wallnum;

                for (j=sc->wallptr; j<endwall; j++)
                {
                    for (SPRITES_OF(STAT_ACTOR, k))
                    {
                        if (sprite[k].extra > 0 && A_CheckEnemySprite(&sprite[k])
                                && clipinsidebox(sprite[k].x, sprite[k].y, j, 256) == 1)
                            goto BOLT;
                    }

                    for (SPRITES_OF(STAT_PLAYER, k))
                    {
                        if (sprite[k].owner >= 0 && clipinsidebox(sprite[k].x, sprite[k].y, j, 144) == 1)
                        {
                            t[5] = 8; // Delay
                            k = (SP>>3)*t[3];
                            t[2] -= k;
                            t[4] -= k;
                            A_MoveSector(i);
                            setsprite(i,(vec3_t *)s);
                            goto BOLT;
                        }
                    }
                }

                k = (SP>>3)*t[3];
                t[2]+=k;
                t[4]+=k;
                A_MoveSector(i);
                setsprite(i,(vec3_t *)s);

                if (t[4] <= -511 || t[4] >= 512)
                {
                    t[4] = 0;
                    t[2] &= 0xffffff00;
                    A_MoveSector(i);
                    setsprite(i,(vec3_t *)s);
                    break;
                }
            }
            break;

        case SE_12_LIGHT_SWITCH:
            if (t[0] == 3 || t[3] == 1)   //Lights going off
            {
                sc->floorpal = 0;
                sc->ceilingpal = 0;

                wal = &wall[sc->wallptr];
                for (j = sc->wallnum; j > 0; j--, wal++)
                    if (wal->hitag != 1)
                    {
                        wal->shade = t[1];
                        wal->pal = 0;
                    }

                sc->floorshade = t[1];
                sc->ceilingshade = t[2];
                t[0]=0;

                for (SPRITES_OF_SECT(SECT, j))
                {
                    if (sprite[j].cstat&16 && A_CheckSpriteFlags(j,SFLAG_NOSHADE) == 0)
                    {
                        if (sc->ceilingstat&1)
                            sprite[j].shade = sc->ceilingshade;
                        else sprite[j].shade = sc->floorshade;
                    }
                }

                if (t[3] == 1)
                    KILLIT(i);
            }
            if (t[0] == 1)   //Lights flickering on
            {
                if (sc->floorshade > s->shade)
                {
                    sc->floorpal = s->pal;
                    sc->ceilingpal = s->pal;

                    sc->floorshade -= 2;
                    sc->ceilingshade -= 2;

                    wal = &wall[sc->wallptr];
                    for (j=sc->wallnum; j>0; j--,wal++)
                        if (wal->hitag != 1)
                        {
                            wal->pal = s->pal;
                            wal->shade -= 2;
                        }
                }
                else t[0] = 2;

                for (SPRITES_OF_SECT(SECT, j))
                {
                    if (sprite[j].cstat&16)
                    {
                        if (sc->ceilingstat&1 && A_CheckSpriteFlags(j,SFLAG_NOSHADE) == 0)
                            sprite[j].shade = sc->ceilingshade;
                        else sprite[j].shade = sc->floorshade;
                    }
                }
            }
            break;


        case SE_13_EXPLOSIVE:
            if (t[2])
            {
                // t[0]: ceiling z
                // t[1]: floor z
                // s->owner: 1 if affect ceiling, 0 if affect floor
                // t[3]: 1 if ceiling was parallaxed at premap, 0 else

                j = (SP<<5)|1;

                if (s->ang == 512)
                {
                    if (s->owner)
                    {
                        if (klabs(t[0]-sc->ceilingz) >= j)
                            sc->ceilingz += ksgn(t[0]-sc->ceilingz)*j;
                        else sc->ceilingz = t[0];
                    }
                    else
                    {
                        if (klabs(t[1]-sc->floorz) >= j)
                            sc->floorz += ksgn(t[1]-sc->floorz)*j;
                        else sc->floorz = t[1];
                    }
                }
                else
                {
                    if (klabs(t[1]-sc->floorz) >= j)
                        sc->floorz += ksgn(t[1]-sc->floorz)*j;
                    else sc->floorz = t[1];
                    if (klabs(t[0]-sc->ceilingz) >= j)
                        sc->ceilingz += ksgn(t[0]-sc->ceilingz)*j;
                    sc->ceilingz = t[0];
                }
#ifdef YAX_ENABLE
                if (s->ang == 512)
                {
                    int16_t cf=!s->owner, bn=yax_getbunch(sc-sector, cf);
                    int32_t jj, daz=SECTORFLD(sc-sector,z, cf);

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
                if (t[3] == 1)
                {
                    //Change the shades

                    t[3]++;
                    sc->ceilingstat ^= 1;

                    if (s->ang == 512)
                    {
                        wal = &wall[sc->wallptr];
                        for (j=sc->wallnum; j>0; j--,wal++)
                            wal->shade = s->shade;

                        sc->floorshade = s->shade;

                        if (g_player[0].ps->one_parallax_sectnum >= 0)
                        {
                            sc->ceilingpicnum =
                                sector[g_player[0].ps->one_parallax_sectnum].ceilingpicnum;
                            sc->ceilingshade  =
                                sector[g_player[0].ps->one_parallax_sectnum].ceilingshade;
                        }
                    }
                }
                t[2]++;
                if (t[2] > 256)
                    KILLIT(i);
            }


            if (t[2] == 4 && s->ang != 512)
                for (x=0; x<7; x++) RANDOMSCRAP;
            break;


        case SE_15_SLIDING_DOOR:

            if (t[4])
            {
                s->xvel = 16;

                if (t[4] == 1) //Opening
                {
                    if (t[3] >= (SP>>3))
                    {
                        t[4] = 0; //Turn off the sliders
                        A_CallSound(s->sectnum,i);
                        break;
                    }
                    t[3]++;
                }
                else if (t[4] == 2)
                {
                    if (t[3]<1)
                    {
                        t[4] = 0;
                        A_CallSound(s->sectnum,i);
                        break;
                    }
                    t[3]--;
                }

                A_MoveSector(i);
                setsprite(i,(vec3_t *)s);
            }
            break;

        case SE_16_REACTOR: //Reactor

            t[2]+=32;
            if (sc->floorz<sc->ceilingz) s->shade=0;

            else if (sc->ceilingz < t[3])
            {

                //The following code check to see if
                //there is any other sprites in the sector.
                //If there isn't, then kill this sectoreffector
                //itself.....

                for (SPRITES_OF_SECT(s->sectnum, j))
                {
                    if (sprite[j].picnum == REACTOR || sprite[j].picnum == REACTOR2)
                        break;
                }

                if (j == -1)
                    KILLIT(i);

                s->shade = 1;
            }

            if (s->shade) sc->ceilingz+=1024;
            else sc->ceilingz-=512;

            A_MoveSector(i);
            setsprite(i,(vec3_t *)s);

            break;

        case SE_17_WARP_ELEVATOR:
        {
            int32_t nextk;

            q = t[0]*(SP<<2);

            sc->ceilingz += q;
            sc->floorz += q;

            for (SPRITES_OF_SECT(s->sectnum, j))
            {
                if (sprite[j].statnum == STAT_PLAYER && sprite[j].owner >= 0)
                {
                    const int32_t p = P_Get(j);
                    DukePlayer_t *const ps = g_player[p].ps;

                    if (numplayers < 2 && !g_netServer)
                        ps->opos.z = ps->pos.z;
                    ps->pos.z += q;
                    ps->truefz += q;
                    ps->truecz += q;
                    if (g_netServer || numplayers > 1)
                        ps->opos.z = ps->pos.z;
                }
                if (sprite[j].statnum != STAT_EFFECTOR)
                {
                    actor[j].bpos.z = sprite[j].z;
                    sprite[j].z += q;
                }

                actor[j].floorz = sc->floorz;
                actor[j].ceilingz = sc->ceilingz;
            }

            if (t[0]) //If in motion
            {
                if (klabs(sc->floorz-t[2]) <= SP)
                {
                    G_ActivateWarpElevators(i,0);
                    break;
                }

                // If we still see the opening, we can't yet teleport.
                if (t[0]==-1)
                {
                    if (sc->floorz > t[3])
                        break;
                }
                else if (sc->ceilingz < t[4]) break;

                if (t[1] == 0) break;
                t[1] = 0;

                for (SPRITES_OF(STAT_EFFECTOR, j))
                {
                    if (i != j && sprite[j].lotag == SE_17_WARP_ELEVATOR)
                        if (sc->hitag-t[0] == sector[sprite[j].sectnum].hitag
                                && sh == sprite[j].hitag)
                            break;
                }

                if (j == -1) break;

                for (SPRITES_OF_SECT_SAFE(s->sectnum, k, nextk))
                {
                    if (sprite[k].statnum == STAT_PLAYER && sprite[k].owner >= 0)
                    {
                        const int32_t p = P_Get(k);
                        DukePlayer_t *const ps = g_player[p].ps;

                        ps->pos.x += sprite[j].x-s->x;
                        ps->pos.y += sprite[j].y-s->y;
                        ps->pos.z = sector[sprite[j].sectnum].floorz-(sc->floorz-ps->pos.z);

                        actor[k].floorz = sector[sprite[j].sectnum].floorz;
                        actor[k].ceilingz = sector[sprite[j].sectnum].ceilingz;

                        ps->bobpos.x = ps->opos.x = ps->pos.x;
                        ps->bobpos.y = ps->opos.y = ps->pos.y;
                        ps->opos.z = ps->pos.z;

                        ps->truefz = actor[k].floorz;
                        ps->truecz = actor[k].ceilingz;
                        ps->bobcounter = 0;

                        changespritesect(k,sprite[j].sectnum);
                        ps->cursectnum = sprite[j].sectnum;
                    }
                    else if (sprite[k].statnum != STAT_EFFECTOR)
                    {
                        sprite[k].x += sprite[j].x-s->x;
                        sprite[k].y += sprite[j].y-s->y;
                        sprite[k].z = sector[sprite[j].sectnum].floorz-
                                      (sc->floorz-sprite[k].z);

                        Bmemcpy(&actor[k].bpos, &sprite[k], sizeof(vec3_t));

                        changespritesect(k,sprite[j].sectnum);
                        setsprite(k,(vec3_t *)&sprite[k]);

                        actor[k].floorz = sector[sprite[j].sectnum].floorz;
                        actor[k].ceilingz = sector[sprite[j].sectnum].ceilingz;
                    }
                }
            }
            break;
        }

        case SE_18_INCREMENTAL_SECTOR_RISE_FALL:
            if (t[0])
            {
                if (s->pal)
                {
                    if (s->ang == 512)
                    {
                        sc->ceilingz -= sc->extra;
                        if (sc->ceilingz <= t[1])
                        {
                            sc->ceilingz = t[1];
                            KILLIT(i);
                        }
                    }
                    else
                    {
                        sc->floorz += sc->extra;

                        for (SPRITES_OF_SECT(s->sectnum, j))
                        {
                            if (sprite[j].picnum == APLAYER && sprite[j].owner >= 0)
                                if (g_player[P_Get(j)].ps->on_ground == 1)
                                    g_player[P_Get(j)].ps->pos.z += sc->extra;

                            if (sprite[j].zvel == 0 && sprite[j].statnum != STAT_EFFECTOR && sprite[j].statnum != STAT_PROJECTILE)
                            {
                                actor[j].bpos.z = sprite[j].z += sc->extra;
                                actor[j].floorz = sc->floorz;
                            }
                        }

                        if (sc->floorz >= t[1])
                        {
                            sc->floorz = t[1];
                            KILLIT(i);
                        }
                    }
                }
                else
                {
                    if (s->ang == 512)
                    {
                        sc->ceilingz += sc->extra;
                        if (sc->ceilingz >= s->z)
                        {
                            sc->ceilingz = s->z;
                            KILLIT(i);
                        }
                    }
                    else
                    {
                        sc->floorz -= sc->extra;

                        for (SPRITES_OF_SECT(s->sectnum, j))
                        {
                            if (sprite[j].picnum == APLAYER && sprite[j].owner >= 0)
                                if (g_player[P_Get(j)].ps->on_ground == 1)
                                    g_player[P_Get(j)].ps->pos.z -= sc->extra;

                            if (sprite[j].zvel == 0 && sprite[j].statnum != STAT_EFFECTOR && sprite[j].statnum != STAT_PROJECTILE)
                            {
                                actor[j].bpos.z = sprite[j].z -= sc->extra;
                                actor[j].floorz = sc->floorz;
                            }
                        }

                        if (sc->floorz <= s->z)
                        {
                            sc->floorz = s->z;
                            KILLIT(i);
                        }
                    }
                }

                t[2]++;
                if (t[2] >= s->hitag)
                {
                    t[2] = 0;
                    t[0] = 0;
                }
            }
            break;

        case SE_19_EXPLOSION_LOWERS_CEILING: //Battlestar galactia shields

            if (t[0])
            {
                if (t[0] == 1)
                {
                    t[0]++;
                    x = sc->wallptr;
                    q = x+sc->wallnum;
                    for (j=x; j<q; j++)
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

                if (sc->ceilingz < sc->floorz)
                    sc->ceilingz += SP;
                else
                {
                    sc->ceilingz = sc->floorz;

                    for (SPRITES_OF(STAT_EFFECTOR, j))
                    {
                        if (sprite[j].lotag == SE_0_ROTATING_SECTOR && sprite[j].hitag==sh)
                        {
                            sectortype *const sec = &sector[sprite[j].sectnum];

                            q = sprite[sprite[j].owner].sectnum;
                            sec->floorpal = sec->ceilingpal = sector[q].floorpal;
                            sec->floorshade = sec->ceilingshade = sector[q].floorshade;

                            actor[sprite[j].owner].t_data[0] = 2;
                        }
                    }

                    KILLIT(i);
                }
            }
            else //Not hit yet
            {
                if (G_FindExplosionInSector(s->sectnum) >= 0)
                {
                    P_DoQuote(QUOTE_UNLOCKED, g_player[myconnectindex].ps);

                    for (SPRITES_OF(STAT_EFFECTOR, l))
                    {
                        x = sprite[l].lotag&0x7fff;
                        switch (x)
                        {
                        case SE_0_ROTATING_SECTOR:
                            if (sprite[l].hitag == sh)
                            {
                                int32_t ow = sprite[l].owner;
                                q = sprite[l].sectnum;
                                sector[q].floorshade = sector[q].ceilingshade = sprite[ow].shade;
                                sector[q].floorpal = sector[q].ceilingpal = sprite[ow].pal;
                            }
                            break;

                        case SE_1_PIVOT:
                        case SE_12_LIGHT_SWITCH:
//                        case SE_18_INCREMENTAL_SECTOR_RISE_FALL:
                        case SE_19_EXPLOSION_LOWERS_CEILING:
                            if (sh == sprite[l].hitag)
                                if (actor[l].t_data[0] == 0)
                                {
                                    actor[l].t_data[0] = 1; //Shut them all on
                                    sprite[l].owner = i;
                                }

                            break;
                        }
                    }
                }
            }

            break;

        case SE_20_STRETCH_BRIDGE: //Extend-o-bridge
            if (t[0] == 0) break;
            if (t[0] == 1) s->xvel = 8;
            else s->xvel = -8;

            if (s->xvel)   //Moving
            {
                int32_t p, nextj;

                x = (s->xvel*sintable[(s->ang+512)&2047])>>14;
                l = (s->xvel*sintable[s->ang&2047])>>14;

                t[3] += s->xvel;

                s->x += x;
                s->y += l;

                if (t[3] <= 0 || (t[3]>>6) >= (SP>>6))
                {
                    s->x -= x;
                    s->y -= l;
                    t[0] = 0;
                    A_CallSound(s->sectnum,i);
                    break;
                }

                for (SPRITES_OF_SECT_SAFE(s->sectnum, j, nextj))
                {
                    if (sprite[j].statnum != STAT_EFFECTOR && sprite[j].zvel == 0)
                    {
                        sprite[j].x += x;
                        sprite[j].y += l;
                        setsprite(j,(vec3_t *)&sprite[j]);
                        if (sector[sprite[j].sectnum].floorstat&2)
                            if (sprite[j].statnum == STAT_ZOMBIEACTOR)
                                A_Fall(j);
                    }
                }

                dragpoint((int16_t)t[1],wall[t[1]].x+x,wall[t[1]].y+l,0);
                dragpoint((int16_t)t[2],wall[t[2]].x+x,wall[t[2]].y+l,0);

                for (TRAVERSE_CONNECT(p))
                {
                    DukePlayer_t *const ps = g_player[p].ps;

                    if (ps->cursectnum == s->sectnum && ps->on_ground)
                    {
                        ps->pos.x += x;
                        ps->pos.y += l;

                        ps->opos.x = ps->pos.x;
                        ps->opos.y = ps->pos.y;

                        ps->pos.z += PHEIGHT;
                        setsprite(ps->i,(vec3_t *)ps);
                        ps->pos.z -= PHEIGHT;
                    }
                }

                sc->floorxpanning-=x>>3;
                sc->floorypanning-=l>>3;

                sc->ceilingxpanning-=x>>3;
                sc->ceilingypanning-=l>>3;
            }

            break;

        case SE_21_DROP_FLOOR: // Cascading effect
        {
            int32_t *zptr;

            if (t[0] == 0) break;

            if (s->ang == 1536)
                zptr = &sc->ceilingz;
            else
                zptr = &sc->floorz;

            if (t[0] == 1)   //Decide if the s->sectnum should go up or down
            {
                s->zvel = ksgn(s->z-*zptr) * (SP<<4);
                t[0]++;
            }

            if (sc->extra == 0)
            {
                *zptr += s->zvel;

                if (klabs(*zptr-s->z) < 1024)
                {
                    *zptr = s->z;
                    KILLIT(i); //All done   // SE_21_KILLIT, see sector.c
                }
            }
            else sc->extra--;
            break;
        }

        case SE_22_TEETH_DOOR:
            if (t[1])
            {
                if (GetAnimationGoal(&sector[t[0]].ceilingz) >= 0)
                    sc->ceilingz += sc->extra*9;
                else t[1] = 0;
            }
            break;

        case SE_24_CONVEYOR:
        case SE_34:
        {
            int32_t p, nextj;

            if (t[4])
                break;

            x = (SP*sintable[(s->ang+512)&2047])>>18;
            l = (SP*sintable[s->ang&2047])>>18;

            k = 0;

            for (SPRITES_OF_SECT_SAFE(s->sectnum, j, nextj))
            {
                if (sprite[j].zvel >= 0)
                    switch (sprite[j].statnum)
                    {
                    case STAT_MISC:
                        switch (DYNAMICTILEMAP(sprite[j].picnum))
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
                        case BLOODSPLAT4__STATIC:
                            sprite[j].xrepeat = sprite[j].yrepeat = 0;
                            continue;

                        case LASERLINE__STATIC:
                            continue;
                        }
                        // fall-through
                    case STAT_STANDABLE:
                        if (sprite[j].picnum == TRIPBOMB)
                            break;
                        // else, fall-through
                    case STAT_ACTOR:
                    case STAT_DEFAULT:
                        if (
                            sprite[j].picnum == BOLT1 ||
                            sprite[j].picnum == BOLT1+1 ||
                            sprite[j].picnum == BOLT1+2 ||
                            sprite[j].picnum == BOLT1+3 ||
                            sprite[j].picnum == SIDEBOLT1 ||
                            sprite[j].picnum == SIDEBOLT1+1 ||
                            sprite[j].picnum == SIDEBOLT1+2 ||
                            sprite[j].picnum == SIDEBOLT1+3 ||
                            A_CheckSwitchTile(j)
                        )
                            break;

                        if (!(sprite[j].picnum >= CRANE && sprite[j].picnum <= CRANE+3))
                        {
                            if (sprite[j].z > actor[j].floorz-(16<<8))
                            {
                                actor[j].bpos.x = sprite[j].x;
                                actor[j].bpos.y = sprite[j].y;

                                sprite[j].x += x>>2;
                                sprite[j].y += l>>2;

                                setsprite(j,(vec3_t *)&sprite[j]);

                                if (sector[sprite[j].sectnum].floorstat&2)
                                    if (sprite[j].statnum == STAT_ZOMBIEACTOR)
                                        A_Fall(j);
                            }
                        }
                        break;
                    }
            }

            for (TRAVERSE_CONNECT(p))
            {
                DukePlayer_t *const ps = g_player[p].ps;

                if (ps->cursectnum == s->sectnum && ps->on_ground)
                {
                    if (klabs(ps->pos.z-ps->truefz) < PHEIGHT+(9<<8))
                    {
                        ps->fric.x += x<<3;
                        ps->fric.y += l<<3;
                    }
                }
            }
            sc->floorxpanning += SP>>7;

            break;
        }

        case SE_35:
            if (sc->ceilingz > s->z)
                for (j = 0; j < 8; j++)
                {
                    s->ang += krand()&511;
                    k = A_Spawn(i,SMALLSMOKE);
                    sprite[k].xvel = 96+(krand()&127);
                    A_SetSprite(k,CLIPMASK0);
                    setsprite(k,(vec3_t *)&sprite[k]);
                    if (rnd(16))
                        A_Spawn(i,EXPLOSION2);
                }

            switch (t[0])
            {
            case 0:
                sc->ceilingz += s->yvel;
                if (sc->ceilingz > sc->floorz)
                    sc->floorz = sc->ceilingz;
                if (sc->ceilingz > s->z+(32<<8))
                    t[0]++;
                break;
            case 1:
                sc->ceilingz-=(s->yvel<<2);
                if (sc->ceilingz < t[4])
                {
                    sc->ceilingz = t[4];
                    t[0] = 0;
                }
                break;
            }
            break;

        case SE_25_PISTON: //PISTONS
            if (t[4] == 0) break;

            if (sc->floorz <= sc->ceilingz)
                s->shade = 0;
            else if (sc->ceilingz <= t[3])
                s->shade = 1;

            if (s->shade)
            {
                sc->ceilingz += SP<<4;
                if (sc->ceilingz > sc->floorz)
                    sc->ceilingz = sc->floorz;
            }
            else
            {
                sc->ceilingz   -= SP<<4;
                if (sc->ceilingz < t[3])
                    sc->ceilingz = t[3];
            }

            break;

        case SE_26:
        {
            int32_t p, nextj;

            s->xvel = 32;
            l = (s->xvel*sintable[(s->ang+512)&2047])>>14;
            x = (s->xvel*sintable[s->ang&2047])>>14;

            s->shade++;
            if (s->shade > 7)
            {
                s->x = t[3];
                s->y = t[4];
                sc->floorz -= ((s->zvel*s->shade)-s->zvel);
                s->shade = 0;
            }
            else
                sc->floorz += s->zvel;

            for (SPRITES_OF_SECT_SAFE(s->sectnum, j, nextj))
            {
                if (sprite[j].statnum != STAT_EFFECTOR && sprite[j].statnum != STAT_PLAYER)
                {
                    actor[j].bpos.x = sprite[j].x;
                    actor[j].bpos.y = sprite[j].y;

                    sprite[j].x += l;
                    sprite[j].y += x;

                    sprite[j].z += s->zvel;
                    setsprite(j,(vec3_t *)&sprite[j]);
                }
            }

            for (TRAVERSE_CONNECT(p))
            {
                DukePlayer_t *const ps = g_player[p].ps;

                if (sprite[ps->i].sectnum == s->sectnum && ps->on_ground)
                {
                    ps->fric.x += l<<5;
                    ps->fric.y += x<<5;
                    ps->pos.z += s->zvel;
                }
            }
            A_MoveSector(i);
            setsprite(i,(vec3_t *)s);

            break;
        }

        case SE_27_DEMO_CAM:
        {
            int32_t p;
            DukePlayer_t *ps;

            if (ud.recstat == 0 || !ud.democams) break;

            actor[i].tempang = s->ang;

            p = A_FindPlayer(s,&x);
            ps = g_player[p].ps;

            if (sprite[ps->i].extra > 0 && myconnectindex == screenpeek)
            {
                if (t[0] < 0)
                {
                    ud.camerasprite = i;
                    t[0]++;
                }
                else if (ud.recstat == 2 && ps->newowner == -1)
                {
                    if (cansee(s->x,s->y,s->z,SECT,ps->pos.x,ps->pos.y,ps->pos.z,ps->cursectnum))
                    {
                        if (x < (int32_t)((unsigned)sh))
                        {
                            ud.camerasprite = i;
                            t[0] = 999;
                            s->ang += G_GetAngleDelta(s->ang,getangle(ps->pos.x-s->x,ps->pos.y-s->y))>>3;
                            SP = 100+((s->z-ps->pos.z)/257);

                        }
                        else if (t[0] == 999)
                        {
                            if (ud.camerasprite == i)
                                t[0] = 0;
                            else t[0] = -10;
                            ud.camerasprite = i;

                        }
                    }
                    else
                    {
                        s->ang = getangle(ps->pos.x-s->x,ps->pos.y-s->y);

                        if (t[0] == 999)
                        {
                            if (ud.camerasprite == i)
                                t[0] = 0;
                            else t[0] = -20;
                            ud.camerasprite = i;
                        }
                    }
                }
            }
            break;
        }

        case SE_28_LIGHTNING:
        {
            if (t[5] > 0)
            {
                t[5]--;
                break;
            }

            if (T1 == 0)
            {
                A_FindPlayer(s,&x);
                if (x > 15500)
                    break;
                T1 = 1;
                T2 = 64 + (krand()&511);
                T3 = 0;
            }
            else
            {
                T3++;
                if (T3 > T2)
                {
                    T1 = 0;
                    g_player[screenpeek].ps->visibility = ud.const_visibility;
                    break;
                }
                else if (T3 == (T2>>1))
                    A_PlaySound(THUNDER,i);
                else if (T3 == (T2>>3))
                    A_PlaySound(LIGHTNING_SLAP,i);
                else if (T3 == (T2>>2))
                {
                    for (SPRITES_OF(STAT_DEFAULT, j))
                        if (sprite[j].picnum == NATURALLIGHTNING && sprite[j].hitag == s->hitag)
                            sprite[j].cstat |= 32768;
                }
                else if (T3 > (T2>>3) && T3 < (T2>>2))
                {
                    if (cansee(s->x,s->y,s->z,s->sectnum,g_player[screenpeek].ps->pos.x,g_player[screenpeek].ps->pos.y,g_player[screenpeek].ps->pos.z,g_player[screenpeek].ps->cursectnum))
                        j = 1;
                    else j = 0;

                    if (rnd(192) && (T3&1))
                    {
                        if (j)
                            g_player[screenpeek].ps->visibility = 0;
                    }
                    else if (j)
                        g_player[screenpeek].ps->visibility = ud.const_visibility;

                    for (SPRITES_OF(STAT_DEFAULT, j))
                    {
                        if (sprite[j].picnum == NATURALLIGHTNING && sprite[j].hitag == s->hitag)
                        {
                            if (rnd(32) && (T3&1))
                            {
                                int32_t p;
                                DukePlayer_t *ps;

                                sprite[j].cstat &= 32767;
                                A_Spawn(j,SMALLSMOKE);

                                p = A_FindPlayer(s, NULL);
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
            s->hitag += 64;
            l = mulscale12((int32_t)s->yvel,sintable[s->hitag&2047]);
            sc->floorz = s->z + l;
            break;

        case SE_31_FLOOR_RISE_FALL: // True Drop Floor
            if (t[0] == 1)
            {
                // Choose dir

                if (t[3] > 0)
                {
                    t[3]--;
                    break;
                }

                if (t[2] == 1) // Retract
                {
                    if (SA != 1536)
                        HandleSE31(i, 1, s->z, 0, s->z-sc->floorz);
                    else
                        HandleSE31(i, 1, t[1], 0, t[1]-sc->floorz);

                    Yax_SetBunchZs(sc-sector, YAX_FLOOR, sc->floorz);

                    break;
                }

                if ((s->ang&2047) == 1536)
                    HandleSE31(i, 0, s->z, 1, s->z-sc->floorz);
                else
                    HandleSE31(i, 0, t[1], 1, t[1]-s->z);

                Yax_SetBunchZs(sc-sector, YAX_FLOOR, sc->floorz);
            }
            break;

        case SE_32_CEILING_RISE_FALL: // True Drop Ceiling
            if (t[0] == 1)
            {
                // Choose dir

                if (t[2] == 1) // Retract
                {
                    if (SA != 1536)
                    {
                        if (klabs(sc->ceilingz - s->z) < (SP<<1))
                        {
                            sc->ceilingz = s->z;
                            A_CallSound(s->sectnum,i);
                            t[2] = 0;
                            t[0] = 0;
                        }
                        else sc->ceilingz += ksgn(s->z-sc->ceilingz)*SP;
                    }
                    else
                    {
                        if (klabs(sc->ceilingz - t[1]) < (SP<<1))
                        {
                            sc->ceilingz = t[1];
                            A_CallSound(s->sectnum,i);
                            t[2] = 0;
                            t[0] = 0;
                        }
                        else sc->ceilingz += ksgn(t[1]-sc->ceilingz)*SP;
                    }

                    Yax_SetBunchZs(sc-sector, YAX_CEILING, sc->ceilingz);

                    break;
                }

                if ((s->ang&2047) == 1536)
                {
                    if (klabs(sc->ceilingz-s->z) < (SP<<1))
                    {
                        t[0] = 0;
                        t[2] = !t[2];
                        A_CallSound(s->sectnum,i);
                        sc->ceilingz = s->z;
                    }
                    else sc->ceilingz += ksgn(s->z-sc->ceilingz)*SP;
                }
                else
                {
                    if (klabs(sc->ceilingz-t[1]) < (SP<<1))
                    {
                        t[0] = 0;
                        t[2] = !t[2];
                        A_CallSound(s->sectnum,i);
                    }
                    else sc->ceilingz -= ksgn(s->z-t[1])*SP;
                }

                Yax_SetBunchZs(sc-sector, YAX_CEILING, sc->ceilingz);
            }
            break;

        case SE_33_QUAKE_DEBRIS:
            if (g_earthquakeTime > 0 && (krand()&7) == 0)
                RANDOMSCRAP;
            break;

        case SE_36_PROJ_SHOOTER:
            if (t[0])
            {
                if (t[0] == 1)
                    A_Shoot(i,sc->extra);
                else if (t[0] == GAMETICSPERSEC*5)
                    t[0] = 0;
                t[0]++;
            }
            break;

        case 128: //SE to control glass breakage
            wal = &wall[t[2]];

            if (wal->cstat|32)
            {
                wal->cstat &= (255-32);
                wal->cstat |= 16;
                if (wal->nextwall >= 0)
                {
                    wall[wal->nextwall].cstat &= (255-32);
                    wall[wal->nextwall].cstat |= 16;
                }
            }
            else break;

            wal->overpicnum++;
            if (wal->nextwall >= 0)
                wall[wal->nextwall].overpicnum++;

            if (t[0] < t[1]) t[0]++;
            else
            {
                wal->cstat &= (128+32+8+4+2);
                if (wal->nextwall >= 0)
                    wall[wal->nextwall].cstat &= (128+32+8+4+2);
                KILLIT(i);
            }
            break;

        case SE_130:
            if (t[0] > 80)
            {
                KILLIT(i);
            }
            else t[0]++;

            x = sc->floorz-sc->ceilingz;

            if (rnd(64))
            {
                k = A_Spawn(i,EXPLOSION2);
                sprite[k].xrepeat = sprite[k].yrepeat = 2+(krand()&7);
                sprite[k].z = sc->floorz-(krand()%x);
                sprite[k].ang += 256-(krand()%511);
                sprite[k].xvel = krand()&127;
                A_SetSprite(k,CLIPMASK0);
            }
            break;

        case SE_131:
            if (t[0] > 40)
            {
                KILLIT(i);
            }
            else t[0]++;

            x = sc->floorz-sc->ceilingz;

            if (rnd(32))
            {
                k = A_Spawn(i,EXPLOSION2);
                sprite[k].xrepeat = sprite[k].yrepeat = 2+(krand()&3);
                sprite[k].z = sc->floorz-(krand()%x);
                sprite[k].ang += 256-(krand()%511);
                sprite[k].xvel = krand()&127;
                A_SetSprite(k,CLIPMASK0);
            }
            break;

        case SE_49_POINT_LIGHT:
        case SE_50_SPOT_LIGHT:
            changespritestat(i, STAT_LIGHT);
            break;
        }
BOLT:
        i = nexti;
    }

    //Sloped sin-wave floors!
    for (SPRITES_OF(STAT_EFFECTOR, i))
    {
        const spritetype *s = &sprite[i];

        if (s->lotag == SE_29_WAVES)
        {
            sectortype *const sc = &sector[s->sectnum];

            if (sc->wallnum == 4)
            {
                wal = &wall[sc->wallptr+2];
                if (wal->nextsector >= 0)
                    alignflorslope(s->sectnum, wal->x,wal->y, sector[wal->nextsector].floorz);
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
                    mylight.sector = SECT;
                    Bmemcpy(&mylight, &sprite[i], sizeof(int32_t) * 3);
                    mylight.range = SHT;
                    mylight.color[0] = sprite[i].xvel;
                    mylight.color[1] = sprite[i].yvel;
                    mylight.color[2] = sprite[i].zvel;
                    mylight.radius = 0;
                    mylight.angle = SA;
                    mylight.horiz = SH;
                    mylight.minshade = sprite[i].xoffset;
                    mylight.maxshade = sprite[i].yoffset;
                    mylight.tilenum = 0;
                    mylight.publicflags.emitshadow = 0;
                    mylight.publicflags.negative = !!(CS & 128);

                    if (CS & 2)
                    {
                        if (CS & 512)
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
                if (SHT != actor[i].lightptr->range)
                {
                    actor[i].lightptr->range = SHT;
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
                if ((int)!!(CS & 128) != actor[i].lightptr->publicflags.negative) {
                    actor[i].lightptr->publicflags.negative = !!(CS & 128);
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

                    mylight.sector = SECT;
                    Bmemcpy(&mylight, &sprite[i], sizeof(int32_t) * 3);
                    mylight.range = SHT;
                    mylight.color[0] = sprite[i].xvel;
                    mylight.color[1] = sprite[i].yvel;
                    mylight.color[2] = sprite[i].zvel;
                    mylight.radius = (256-(SS+128))<<1;
                    mylight.faderadius = (int16_t)(mylight.radius * 0.75f);
                    mylight.angle = SA;
                    mylight.horiz = SH;
                    mylight.minshade = sprite[i].xoffset;
                    mylight.maxshade = sprite[i].yoffset;
                    mylight.tilenum = actor[i].picnum;
                    mylight.publicflags.emitshadow = !(CS & 64);
                    mylight.publicflags.negative = !!(CS & 128);

                    if (CS & 2)
                    {
                        if (CS & 512)
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
                        if (actor[i].lightptr->horiz != SH)
                            SH = actor[i].lightptr->horiz;
                    }
                    break;
                }

                if (Bmemcmp(&sprite[i], actor[i].lightptr, sizeof(int32_t) * 3))
                {
                    Bmemcpy(actor[i].lightptr, &sprite[i], sizeof(int32_t) * 3);
                    actor[i].lightptr->sector = sprite[i].sectnum;
                    actor[i].lightptr->flags.invalidate = 1;
                }
                if (SHT != actor[i].lightptr->range)
                {
                    actor[i].lightptr->range = SHT;
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
                if (((256-(SS+128))<<1) != actor[i].lightptr->radius)
                {
                    actor[i].lightptr->radius = (256-(SS+128))<<1;
                    actor[i].lightptr->faderadius = (int16_t)(actor[i].lightptr->radius * 0.75f);
                    actor[i].lightptr->flags.invalidate = 1;
                }
                if (SA != actor[i].lightptr->angle)
                {
                    actor[i].lightptr->angle = SA;
                    actor[i].lightptr->flags.invalidate = 1;
                }
                if (SH != actor[i].lightptr->horiz)
                {
                    actor[i].lightptr->horiz = SH;
                    actor[i].lightptr->flags.invalidate = 1;
                }
                if ((int)!(CS & 64) != actor[i].lightptr->publicflags.emitshadow) {
                    actor[i].lightptr->publicflags.emitshadow = !(CS & 64);
                }
                if ((int)!!(CS & 128) != actor[i].lightptr->publicflags.negative) {
                    actor[i].lightptr->publicflags.negative = !!(CS & 128);
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
static void A_DoLight(int32_t i)
{
    spritetype *const s = &sprite[i];
    int32_t numsavedfires = 0;

    if ((sprite[i].picnum != SECTOREFFECTOR && ((s->cstat & 32768) || s->yrepeat < 4)) || A_CheckSpriteFlags(i, SFLAG_NOLIGHT) ||
        (A_CheckSpriteFlags(i, SFLAG_USEACTIVATOR) && sector[sprite[i].sectnum].lotag & 16384))
    {
        if (actor[i].lightptr != NULL)
            A_DeleteLight(i);
    }
    else
    {
        int32_t ii;

        if (actor[i].lightptr != NULL && actor[i].lightcount)
        {
            if (!(--actor[i].lightcount))
                A_DeleteLight(i);
        }

        for (ii=0; ii<2; ii++)
        {
            if (sprite[i].picnum <= 0)  // oob safety
                break;

            switch (DYNAMICTILEMAP(sprite[i].picnum-1+ii))
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
                    int32_t dx = sintable[(s->ang+512)&2047];
                    int32_t dy = sintable[(s->ang)&2047];

                    if ((s->cstat & 32768) || A_CheckSpriteFlags(i, SFLAG_NOLIGHT))
                    {
                        if (actor[i].lightptr != NULL)
                            A_DeleteLight(i);
                        break;
                    }

                    s->x += dx>>7;
                    s->y += dy>>7;

                    G_AddGameLight(0, i, ((s->yrepeat*tilesiz[s->picnum].y)<<1), 1024-ii*256,
                        ii==0 ? (48+(255<<8)+(48<<16)) : 255+(48<<8)+(48<<16), PR_LIGHT_PRIO_LOW);

                    s->x -= dx>>7;
                    s->y -= dy>>7;
                }
                break;
            }
        }

        switch (DYNAMICTILEMAP(sprite[i].picnum))
        {
        case ATOMICHEALTH__STATIC:
            G_AddGameLight(0, i, ((s->yrepeat*tilesiz[s->picnum].y)<<1), LIGHTRAD2 * 3, 128+(128<<8)+(255<<16),PR_LIGHT_PRIO_HIGH_GAME);
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

                switch (s->pal)
                {
                case 1: color = 128+(128<<8)+(255<<16); break;
                case 2: color = 255+(48<<8)+(48<<16); break;
                case 8: color = 48+(255<<8)+(48<<16); break;
                default: color = 255+(95<<8); break;
                }

                for (jj=numsavedfires-1; jj>=0; jj--)
                    if (savedfires[jj][0]==s->sectnum && savedfires[jj][1]==(s->x>>3) &&
                        savedfires[jj][2]==(s->y>>3) && savedfires[jj][3]==(s->z>>7))
                        break;

                if (jj==-1 && numsavedfires<32)
                {
                    jj = numsavedfires;
                    G_AddGameLight(0, i, ((s->yrepeat*tilesiz[s->picnum].y)<<1), LIGHTRAD2, color, PR_LIGHT_PRIO_HIGH_GAME);
                    savedfires[jj][0] = s->sectnum;
                    savedfires[jj][1] = s->x>>3;
                    savedfires[jj][2] = s->y>>3;
                    savedfires[jj][3] = s->z>>7;
                    numsavedfires++;
                }
            }
            break;

        case OOZFILTER__STATIC:
            if (s->xrepeat > 4)
                G_AddGameLight(0, i, ((s->yrepeat*tilesiz[s->picnum].y)<<1), 4096, 128+(255<<8)+(128<<16),PR_LIGHT_PRIO_HIGH_GAME);
            break;
        case FLOORFLAME__STATIC:
        case FIREBARREL__STATIC:
        case FIREVASE__STATIC:
            G_AddGameLight(0, i, ((s->yrepeat*tilesiz[s->picnum].y)<<1), LIGHTRAD, 255+(95<<8),PR_LIGHT_PRIO_HIGH_GAME);
            break;

        case EXPLOSION2__STATIC:
            if (!actor[i].lightcount)
            {
                // XXX: This block gets CODEDUP'd too much.
                int32_t x = ((sintable[(s->ang+512)&2047])>>6);
                int32_t y = ((sintable[(s->ang)&2047])>>6);

                s->x -= x;
                s->y -= y;

                G_AddGameLight(0, i, ((s->yrepeat*tilesiz[s->picnum].y)<<1), LIGHTRAD, 255+(95<<8),
                    s->yrepeat > 32 ? PR_LIGHT_PRIO_HIGH_GAME : PR_LIGHT_PRIO_LOW_GAME);

                s->x += x;
                s->y += y;
            }
            break;
        case FORCERIPPLE__STATIC:
        case TRANSPORTERBEAM__STATIC:
            G_AddGameLight(0, i, ((s->yrepeat*tilesiz[s->picnum].y)<<1), LIGHTRAD, 80+(80<<8)+(255<<16),PR_LIGHT_PRIO_LOW_GAME);
            break;
        case GROWSPARK__STATIC:
            {
                int32_t x = ((sintable[(s->ang+512)&2047])>>6);
                int32_t y = ((sintable[(s->ang)&2047])>>6);

                s->x -= x;
                s->y -= y;

                G_AddGameLight(0, i, ((s->yrepeat*tilesiz[s->picnum].y)<<1), 2048, 255+(95<<8),PR_LIGHT_PRIO_HIGH_GAME);

                s->x += x;
                s->y += y;
            }
            break;
        case SHRINKEREXPLOSION__STATIC:
            {
                int32_t x = ((sintable[(s->ang+512)&2047])>>6);
                int32_t y = ((sintable[(s->ang)&2047])>>6);

                s->x -= x;
                s->y -= y;

                G_AddGameLight(0, i, ((s->yrepeat*tilesiz[s->picnum].y)<<1), 2048, 128+(255<<8)+(128<<16),PR_LIGHT_PRIO_HIGH_GAME);

                s->x += x;
                s->y += y;
            }
            break;
        case FREEZEBLAST__STATIC:
            G_AddGameLight(0, i, ((s->yrepeat*tilesiz[s->picnum].y)<<1), LIGHTRAD<<2, 128+(128<<8)+(255<<16),PR_LIGHT_PRIO_HIGH_GAME);
            break;
        case COOLEXPLOSION1__STATIC:
            G_AddGameLight(0, i, ((s->yrepeat*tilesiz[s->picnum].y)<<1), LIGHTRAD<<2, 128+(0<<8)+(255<<16),PR_LIGHT_PRIO_HIGH_GAME);
            break;
        case SHRINKSPARK__STATIC:
            G_AddGameLight(0, i, ((s->yrepeat*tilesiz[s->picnum].y)<<1), LIGHTRAD, 128+(255<<8)+(128<<16),PR_LIGHT_PRIO_HIGH_GAME);
            break;
        case FIRELASER__STATIC:
            G_AddGameLight(0, i, ((s->yrepeat*tilesiz[s->picnum].y)<<1), 64 * s->yrepeat, 255+(95<<8),PR_LIGHT_PRIO_LOW_GAME);
            break;
        case RPG__STATIC:
            G_AddGameLight(0, i, ((s->yrepeat*tilesiz[s->picnum].y)<<1), 128 * s->yrepeat, 255+(95<<8),PR_LIGHT_PRIO_LOW_GAME);
            break;
        case SHOTSPARK1__STATIC:
            if (actor[i].t_data[2] == 0) // check for first frame of action
            {
                int32_t x = ((sintable[(s->ang+512)&2047])>>7);
                int32_t y = ((sintable[(s->ang)&2047])>>7);

                s->x -= x;
                s->y -= y;

                G_AddGameLight(0, i, ((s->yrepeat*tilesiz[s->picnum].y)<<1), 16 * s->yrepeat, 255+(95<<8),PR_LIGHT_PRIO_LOW_GAME);
                actor[i].lightcount = 1;

                s->x += x;
                s->y += y;
            }
            break;
        }
    }
}
#endif // POLYMER

void A_PlayAlertSound(int32_t i)
{
    if (sprite[i].extra > 0)
        switch (DYNAMICTILEMAP(PN))
        {
        case LIZTROOPONTOILET__STATIC:
        case LIZTROOPJUSTSIT__STATIC:
        case LIZTROOPSHOOT__STATIC:
        case LIZTROOPJETPACK__STATIC:
        case LIZTROOPDUCKING__STATIC:
        case LIZTROOPRUNNING__STATIC:
        case LIZTROOP__STATIC:
            A_PlaySound(PRED_RECOG,i);
            break;
        case LIZMAN__STATIC:
        case LIZMANSPITTING__STATIC:
        case LIZMANFEEDING__STATIC:
        case LIZMANJUMP__STATIC:
            A_PlaySound(CAPT_RECOG,i);
            break;
        case PIGCOP__STATIC:
        case PIGCOPDIVE__STATIC:
            A_PlaySound(PIG_RECOG,i);
            break;
        case RECON__STATIC:
            A_PlaySound(RECO_RECOG,i);
            break;
        case DRONE__STATIC:
            A_PlaySound(DRON_RECOG,i);
            break;
        case COMMANDER__STATIC:
        case COMMANDERSTAYPUT__STATIC:
            A_PlaySound(COMM_RECOG,i);
            break;
        case ORGANTIC__STATIC:
            A_PlaySound(TURR_RECOG,i);
            break;
        case OCTABRAIN__STATIC:
        case OCTABRAINSTAYPUT__STATIC:
            A_PlaySound(OCTA_RECOG,i);
            break;
        case BOSS1__STATIC:
        case BOSS1STAYPUT__STATIC:
            S_PlaySound(BOS1_RECOG);
            break;
        case BOSS2__STATIC:
            if (sprite[i].pal != 0)
                S_PlaySound(BOS2_RECOG);
            else S_PlaySound(WHIPYOURASS);
            break;
        case BOSS3__STATIC:
            if (sprite[i].pal != 0)
                S_PlaySound(BOS3_RECOG);
            else S_PlaySound(RIPHEADNECK);
            break;
        case BOSS4__STATIC:
        case BOSS4STAYPUT__STATIC:
            if (sprite[i].pal != 0)
                S_PlaySound(BOS4_RECOG);
            else S_PlaySound(BOSS4_FIRSTSEE);
            break;
        case GREENSLIME__STATIC:
            A_PlaySound(SLIM_RECOG,i);
            break;
        }
}

int32_t A_CheckSwitchTile(int32_t i)
{
    int32_t j;

    if (PN <= 0)  // picnum 0 would oob in the switch below
        return 0;

    // MULTISWITCH has 4 states so deal with it separately.
    if (PN >= MULTISWITCH && PN <= MULTISWITCH+3)
        return 1;

    // ACCESSSWITCH and ACCESSSWITCH2 are only active in one state so deal with
    // them separately.
    if (PN == ACCESSSWITCH || PN == ACCESSSWITCH2)
        return 1;

    // Loop to catch both states of switches.
    for (j=1; j>=0; j--)
    {
        switch (DYNAMICTILEMAP(PN-j))
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

void G_MoveWorld(void)
{
    extern double g_moveActorsTime;

    VM_OnEvent(EVENT_PREWORLD, -1, -1);

    if (EDUKE32_PREDICT_FALSE(VM_HaveEvent(EVENT_PREGAME)))
    {
        int32_t i, j, k = 0, p, pl;

        do
        {
            i = headspritestat[k++];

            while (i >= 0)
            {
                j = nextspritestat[i];

                if (A_CheckSpriteFlags(i, SFLAG_NOEVENTCODE))
                {
                    i = j;
                    continue;
                }

                pl = A_FindPlayer(&sprite[i], &p);
                VM_OnEventWithDist_(EVENT_PREGAME, i, pl, p);

                i = j;
            }
        } while (k < MAXSTATUS);
    }

    G_MoveZombieActors();     //ST 2
    G_MoveWeapons();          //ST 4
    G_MoveTransports();       //ST 9

    G_MovePlayers();          //ST 10
    G_MoveFallers();          //ST 12
    G_MoveMisc();             //ST 5

    {
        double t = gethiticks();

        G_MoveActors();           //ST 1

        g_moveActorsTime = (1-0.033)*g_moveActorsTime + 0.033*(gethiticks()-t);
    }

    // XXX: Has to be before effectors, in particular movers?
    // TODO: lights in moving sectors ought to be interpolated
    G_DoEffectorLights();

    G_MoveEffectors();        //ST 3

    G_MoveStandables();       //ST 6


    VM_OnEvent(EVENT_WORLD, -1, -1);

    if (EDUKE32_PREDICT_FALSE(VM_HaveEvent(EVENT_GAME)))
    {
        int32_t i, j, k = 0, p, pl;

        do
        {
            i = headspritestat[k++];

            while (i >= 0)
            {

                if (A_CheckSpriteFlags(i, SFLAG_NOEVENTCODE))
                {
                    i = nextspritestat[i];
                    continue;
                }

                j = nextspritestat[i];

                pl = A_FindPlayer(&sprite[i], &p);
                VM_OnEventWithDist_(EVENT_GAME, i, pl, p);

                i = j;
            }
        } while (k < MAXSTATUS);
    }

#ifdef POLYMER
    if (getrendermode() == REND_POLYMER)
    {
        int32_t i, k = 0;

        do
        {
            i = headspritestat[k++];

            while (i >= 0)
            {
                A_DoLight(i);
                i = nextspritestat[i];
            }
        } while (k < MAXSTATUS);
    }
#endif

    G_DoSectorAnimations();
    G_MoveFX();               //ST 11
}
