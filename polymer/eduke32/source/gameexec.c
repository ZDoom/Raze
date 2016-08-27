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


#include "duke3d.h"
#include <time.h>
#include <math.h>  // sqrt

#include "scriplib.h"
#include "savegame.h"
#include "osdcmds.h"
#include "menus.h"
#include "input.h"
#include "anim.h"

#ifdef LUNATIC
# include "lunatic_game.h"
#endif

#if KRANDDEBUG
# define GAMEEXEC_INLINE
# define GAMEEXEC_STATIC
#else
# define GAMEEXEC_INLINE inline
# define GAMEEXEC_STATIC static
#endif

vmstate_t vm;

#if !defined LUNATIC
enum vmflags_t {
    VM_RETURN       = 0x00000001,
    VM_KILL         = 0x00000002,
    VM_NOEXECUTE    = 0x00000004,
};

int32_t g_tw;
int32_t g_errorLineNum;
int32_t g_currentEventExec = -1;

intptr_t const *insptr;

int32_t g_iReturnVarID    = -1;  // var ID of "RETURN"
int32_t g_iWeaponVarID    = -1;  // var ID of "WEAPON"
int32_t g_iWorksLikeVarID = -1;  // var ID of "WORKSLIKE"
int32_t g_iZRangeVarID    = -1;  // var ID of "ZRANGE"
int32_t g_iAngRangeVarID  = -1;  // var ID of "ANGRANGE"
int32_t g_iAimAngleVarID  = -1;  // var ID of "AUTOAIMANGLE"
int32_t g_iLoTagID        = -1;  // var ID of "LOTAG"
int32_t g_iHiTagID        = -1;  // var ID of "HITAG"
int32_t g_iTextureID      = -1;  // var ID of "TEXTURE"
int32_t g_iThisActorID    = -1;  // var ID of "THISACTOR"
int32_t g_iStructVarIDs   = -1;

GAMEEXEC_STATIC void VM_Execute(int loop);

# include "gamestructures.c"
#endif

#define VM_CONDITIONAL(xxx)                                                                                            \
    {                                                                                                                  \
        if ((xxx) || ((insptr = (intptr_t *)*(insptr + 1)) && (((*insptr) & VM_INSTMASK) == CON_ELSE)))                \
        {                                                                                                              \
            insptr += 2;                                                                                               \
            VM_Execute(0);                                                                                             \
        }                                                                                                              \
    }

#if !defined LUNATIC
void VM_ScriptInfo(intptr_t const *ptr, int range)
{
    if (!script)
        return;

    if (ptr)
    {
        initprintf("\n");

        for (intptr_t const *p = max(ptr - (range>>1), script), *p_end = min(ptr + (range>>1), script + g_scriptSize); p < p_end; p++)
        {
            initprintf("%5d: %3d: ", (int32_t) (p - script), (int32_t) (p - ptr));

            if (*p >> 12 && (*p & VM_INSTMASK) < CON_END)
                initprintf("%5d %s\n", (int32_t) (*p >> 12), keyw[*p & VM_INSTMASK]);
            else
                initprintf("%d\n", (int32_t) *p);
        }

        initprintf("\n");
    }

    if (ptr == insptr)
    {
        if (vm.g_i)
            initprintf("current actor: %d (%d)\n", vm.g_i, TrackerCast(vm.g_sp->picnum));

        initprintf("g_errorLineNum: %d, g_tw: %d\n", g_errorLineNum, g_tw);
    }
}
#endif

static void VM_DeleteSprite(int nSprite, int nPlayer)
{
    if (EDUKE32_PREDICT_FALSE((unsigned) nSprite >= MAXSPRITES))
        return;

    // if player was set to squish, first stop that...
    if (EDUKE32_PREDICT_FALSE(nPlayer >= 0 && g_player[nPlayer].ps->actorsqu == nSprite))
        g_player[nPlayer].ps->actorsqu = -1;

    A_DeleteSprite(nSprite);
}

intptr_t apScriptGameEvent[MAXGAMEEVENTS];

// May recurse, e.g. through EVENT_XXX -> ... -> EVENT_KILLIT
#ifdef LUNATIC
FORCE_INLINE int32_t VM_EventCommon_(int nEventID, int nSprite, int nPlayer, int nDist, int32_t nReturn)
{
    const double t = gethiticks();
    int32_t ret = El_CallEvent(&g_ElState, nEventID, nSprite, nPlayer, nDist, &nReturn);

    // NOTE: the run times are those of the called event plus any events
    // called by it, *not* "self" time.
    g_eventTotalMs[nEventID] += gethiticks()-t;
    g_eventCalls[nEventID]++;

    if (ret == 1)
        VM_DeleteSprite(nSprite, nPlayer);

    return nReturn;
}
#else
FORCE_INLINE int32_t VM_EventCommon_(int const nEventID, int const nSprite, int const nPlayer,
                                     int const nDist, int32_t nReturn)
{
    // this is initialized first thing because iActor, iPlayer, lDist, etc are already right there on the stack
    // from the function call
    const vmstate_t tempvm = { nSprite, nPlayer, nDist, &actor[(unsigned)nSprite].t_data[0],
                               &sprite[(unsigned)nSprite], g_player[nPlayer].ps, 0 };

    // since we're targeting C99 and C++ now, we can interweave these to avoid
    // having to load addresses for things twice
    // for example, because we are loading backupReturnVar with the value of
    // aGameVars[g_iReturnVarID].lValue, the compiler can avoid having to
    // reload the address of aGameVars[g_iReturnVarID].lValue in order to
    // set it to the value of iReturn (...which should still be on the stack!)

    int const backupReturnVar = aGameVars[g_iReturnVarID].nValue;
    aGameVars[g_iReturnVarID].nValue = nReturn;

    int const backupEventExec = g_currentEventExec;
    g_currentEventExec = nEventID;

    intptr_t const *oinsptr = insptr;
    insptr = script + apScriptGameEvent[nEventID];

    const vmstate_t vm_backup = vm;
    vm = tempvm;

    // check tempvm instead of vm... this way, we are not actually loading
    // FROM vm anywhere until VM_Execute() is called
    if ((unsigned)tempvm.g_i >= MAXSPRITES)
    {
        static spritetype dummy_sprite;
        static int32_t dummy_t[ARRAY_SIZE(actor[0].t_data)];

        vm.g_sp = &dummy_sprite;
        vm.g_t = dummy_t;
    }

    if ((unsigned)nPlayer >= (unsigned)playerswhenstarted)
        vm.g_pp = g_player[0].ps;

    VM_Execute(1);

    if (vm.g_flags & VM_KILL)
        VM_DeleteSprite(vm.g_i, vm.g_p);

    // this needs to happen after VM_DeleteSprite() because VM_DeleteSprite()
    // can trigger additional events

    vm                               = vm_backup;
    insptr                           = oinsptr;
    g_currentEventExec               = backupEventExec;
    nReturn                          = aGameVars[g_iReturnVarID].nValue;
    aGameVars[g_iReturnVarID].nValue = backupReturnVar;

    return nReturn;
}
#endif

// the idea here is that the compiler inlines the call to VM_EventCommon_() and gives us a set of full functions
// which are not only optimized further based on lDist or iReturn (or both) having values known at compile time,
// but are called faster due to having less parameters

int32_t VM_OnEventWithBoth_(int nEventID, int nSprite, int nPlayer, int nDist, int32_t nReturn)
{
    return VM_EventCommon_(nEventID, nSprite, nPlayer, nDist, nReturn);
}

int32_t VM_OnEventWithReturn_(int nEventID, int nSprite, int nPlayer, int32_t nReturn)
{
    return VM_EventCommon_(nEventID, nSprite, nPlayer, -1, nReturn);
}

int32_t VM_OnEventWithDist_(int nEventID, int nSprite, int nPlayer, int nDist)
{
    return VM_EventCommon_(nEventID, nSprite, nPlayer, nDist, 0);
}

int32_t VM_OnEvent_(int nEventID, int nSprite, int nPlayer)
{
    return VM_EventCommon_(nEventID, nSprite, nPlayer, -1, 0);
}

static int32_t VM_CheckSquished(void)
{
    sectortype const * const sc = &sector[vm.g_sp->sectnum];

    if (sc->lotag == ST_23_SWINGING_DOOR ||
        (sc->lotag == ST_1_ABOVE_WATER && !A_CheckNoSE7Water(vm.g_sp, vm.g_sp->sectnum, sc->lotag, NULL)) ||
        (vm.g_sp->picnum == APLAYER && ud.noclip))
        return 0;

    int32_t fz=sc->floorz, cz=sc->ceilingz;
#ifdef YAX_ENABLE
    int16_t cb, fb;

    yax_getbunches(vm.g_sp->sectnum, &cb, &fb);

    if (cb >= 0 && (sc->ceilingstat&512)==0)  // if ceiling non-blocking...
        cz -= (32<<8);  // unconditionally don't squish... yax_getneighborsect is slowish :/
    if (fb >= 0 && (sc->floorstat&512)==0)
        fz += (32<<8);
#endif

    if (vm.g_sp->pal == 1 ?
        (fz - cz >= (32<<8) || (sc->lotag&32768)) :
        (fz - cz >= (12<<8)))
    return 0;

    P_DoQuote(QUOTE_SQUISHED, vm.g_pp);

    if (A_CheckEnemySprite(vm.g_sp))
        vm.g_sp->xvel = 0;

    if (EDUKE32_PREDICT_FALSE(vm.g_sp->pal == 1)) // frozen
    {
        actor[vm.g_i].picnum = SHOTSPARK1;
        actor[vm.g_i].extra = 1;
        return 0;
    }

    return 1;
}

#if !defined LUNATIC
GAMEEXEC_STATIC GAMEEXEC_INLINE void P_ForceAngle(DukePlayer_t *p)
{
    int const nAngle = 128-(krand()&255);

    p->horiz += 64;
    p->return_to_center = 9;
    p->look_ang = p->rotscrnang = nAngle>>1;
}
#endif

int32_t A_Dodge(spritetype *pSprite)
{
    vec2_t const m    = *(vec2_t *)pSprite;
    vec2_t const msin = { sintable[(pSprite->ang + 512) & 2047], sintable[pSprite->ang & 2047] };

    if (A_CheckEnemySprite(pSprite) && pSprite->extra <= 0)  // hack
        return 0;

    for (int nexti, SPRITES_OF_STAT_SAFE(STAT_PROJECTILE, i, nexti)) //weapons list
    {
        if (OW == i)
            continue;

        vec2_t const b = { SX - m.x, SY - m.y };
        vec2_t const v = { sintable[(SA + 512) & 2047], sintable[SA & 2047] };

        if (((msin.x * b.x) + (msin.y * b.y) >= 0) && ((v.x * b.x) + (v.y * b.y) < 0))
        {
            if (klabs((v.x * b.y) - (v.y * b.x)) < 65536 << 6)
            {
                pSprite->ang -= 512+(krand()&1024);
                return 1;
            }
        }
    }

    return 0;
}

int32_t A_GetFurthestAngle(int32_t iActor, int32_t angs)
{
    spritetype *const s = &sprite[iActor];

    if (s->picnum != APLAYER && (AC_COUNT(actor[iActor].t_data)&63) > 2)
        return s->ang + 1024;

    int32_t furthest_angle = 0, greatestd = INT32_MIN;
    int const angincs = tabledivide32_noinline(2048, angs);
    hitdata_t hit;

    for (int32_t j=s->ang; j<(2048+s->ang); j+=angincs)
    {
        s->z -= (8<<8);
        hitscan((const vec3_t *)s, s->sectnum,
                sintable[(j+512)&2047],
                sintable[j&2047], 0,
                &hit, CLIPMASK1);
        s->z += (8<<8);

        int const d = klabs(hit.pos.x-s->x) + klabs(hit.pos.y-s->y);

        if (d > greatestd)
        {
            greatestd = d;
            furthest_angle = j;
        }
    }

    return furthest_angle&2047;
}

int32_t A_FurthestVisiblePoint(int32_t nSprite, uspritetype * const pSprite, int32_t *dax, int32_t *day)
{
    if (AC_COUNT(actor[nSprite].t_data)&63)
        return -1;

    const uspritetype *const pnSprite = (uspritetype *)&sprite[nSprite];

    hitdata_t hit;
    int const angincs = 128;
//    ((!g_netServer && ud.multimode < 2) && ud.player_skill < 3) ? 2048 / 2 : tabledivide32_noinline(2048, 1 + (krand() & 1));

    for (int j = pSprite->ang; j < (2048 + pSprite->ang); j += (angincs /*-(krand()&511)*/))
    {
        pSprite->z -= ZOFFSET2;
        hitscan((const vec3_t *)pSprite, pSprite->sectnum, sintable[(j + 512) & 2047], sintable[j & 2047],
                16384 - (krand() & 32767), &hit, CLIPMASK1);
        pSprite->z += ZOFFSET2;

        if (hit.sect < 0)
            continue;

        int const d  = FindDistance2D(hit.pos.x - pSprite->x, hit.pos.y - pSprite->y);
        int const da = FindDistance2D(hit.pos.x - pnSprite->x, hit.pos.y - pnSprite->y);

        if (d < da)
        {
            if (cansee(hit.pos.x, hit.pos.y, hit.pos.z, hit.sect,
                        pnSprite->x, pnSprite->y, pnSprite->z - ZOFFSET2, pnSprite->sectnum))
            {
                *dax = hit.pos.x;
                *day = hit.pos.y;
                return hit.sect;
            }
        }
    }

    return -1;
}

static void VM_GetZRange(int32_t iActor, int32_t *ceilhit, int32_t *florhit, int walldist)
{
    spritetype *const s = &sprite[iActor];
    int const ocstat = s->cstat;

    s->cstat = 0;
    s->z -= ZOFFSET;

    getzrange((vec3_t *)s, s->sectnum,
              &actor[iActor].ceilingz, ceilhit,
              &actor[iActor].floorz, florhit,
              walldist, CLIPMASK0);

    s->z += ZOFFSET;
    s->cstat = ocstat;
}

void A_GetZLimits(int32_t nSprite)
{
    spritetype *const pSprite = &sprite[nSprite];
    int32_t           ceilhit, florhit;

    VM_GetZRange(nSprite, &ceilhit, &florhit, (pSprite->statnum == STAT_PROJECTILE) ? 4 : 127);
    actor[nSprite].flags &= ~SFLAG_NOFLOORSHADOW;

    if ((florhit&49152) == 49152 && (sprite[florhit&(MAXSPRITES-1)].cstat&48) == 0)
    {
        uspritetype const * const hitspr = (uspritetype *)&sprite[florhit&(MAXSPRITES-1)];

        florhit &= (MAXSPRITES-1);

        // If a non-projectile would fall onto non-frozen enemy OR an enemy onto a player...
        if ((A_CheckEnemySprite(hitspr) && hitspr->pal != 1 && pSprite->statnum != STAT_PROJECTILE)
                || (hitspr->picnum == APLAYER && A_CheckEnemySprite(pSprite)))
        {
            actor[nSprite].flags |= SFLAG_NOFLOORSHADOW;  // No shadows on actors
            pSprite->xvel = -256;  // SLIDE_ABOVE_ENEMY
            A_SetSprite(nSprite, CLIPMASK0);
        }
        else if (pSprite->statnum == STAT_PROJECTILE && hitspr->picnum == APLAYER && pSprite->owner==florhit)
        {
            actor[nSprite].ceilingz = sector[pSprite->sectnum].ceilingz;
            actor[nSprite].floorz   = sector[pSprite->sectnum].floorz;
        }
    }
}

void A_Fall(int nSprite)
{
    spritetype *const pSprite = &sprite[nSprite];
    int nGravity = g_spriteGravity;

    if (EDUKE32_PREDICT_FALSE(G_CheckForSpaceFloor(pSprite->sectnum)))
        nGravity = 0;
    else if (sector[pSprite->sectnum].lotag == ST_2_UNDERWATER || EDUKE32_PREDICT_FALSE(G_CheckForSpaceCeiling(pSprite->sectnum)))
        nGravity = g_spriteGravity/6;

    if (pSprite->statnum == STAT_ACTOR || pSprite->statnum == STAT_PLAYER || pSprite->statnum == STAT_ZOMBIEACTOR || pSprite->statnum == STAT_STANDABLE)
    {
        int32_t ceilhit, florhit;
        VM_GetZRange(nSprite, &ceilhit, &florhit, 127);
    }
    else
    {
        actor[nSprite].ceilingz = sector[pSprite->sectnum].ceilingz;
        actor[nSprite].floorz   = sector[pSprite->sectnum].floorz;
    }

#ifdef YAX_ENABLE
    int fbunch = (sector[pSprite->sectnum].floorstat&512) ? -1 : yax_getbunch(pSprite->sectnum, YAX_FLOOR);
#endif

    if (pSprite->z < actor[nSprite].floorz-ZOFFSET
#ifdef YAX_ENABLE
            || fbunch >= 0
#endif
       )
    {
        if (sector[pSprite->sectnum].lotag == ST_2_UNDERWATER && pSprite->zvel > 3122)
            pSprite->zvel = 3144;
        pSprite->z += pSprite->zvel = min(6144, pSprite->zvel+nGravity);
    }

#ifdef YAX_ENABLE
    if (fbunch >= 0)
        setspritez(nSprite, (vec3_t *)pSprite);
    else
#endif
        if (pSprite->z >= actor[nSprite].floorz-ZOFFSET)
        {
            pSprite->z = actor[nSprite].floorz-ZOFFSET;
            pSprite->zvel = 0;
        }
}

int G_GetAngleDelta(int a, int na)
{
    a &= 2047;
    na &= 2047;

    if (klabs(a-na) < 1024)
    {
//        OSD_Printf("G_GetAngleDelta() returning %d\n",na-a);
        return na-a;
    }

    if (na > 1024) na -= 2048;
    if (a > 1024) a -= 2048;

//    OSD_Printf("G_GetAngleDelta() returning %d\n",na-a);
    return na-a;
}

GAMEEXEC_STATIC void VM_AlterAng(int32_t movflags)
{
    int const nElapsedTics = (AC_COUNT(vm.g_t))&31;

#if !defined LUNATIC
    const intptr_t *moveptr;
    if (EDUKE32_PREDICT_FALSE((unsigned)AC_MOVE_ID(vm.g_t) >= (unsigned)g_scriptSize-1))

    {
        AC_MOVE_ID(vm.g_t) = 0;
        OSD_Printf(OSD_ERROR "bad moveptr for actor %d (%d)!\n", vm.g_i, TrackerCast(vm.g_sp->picnum));
        return;
    }

    moveptr = script + AC_MOVE_ID(vm.g_t);

    vm.g_sp->xvel += (moveptr[0] - vm.g_sp->xvel)/5;
    if (vm.g_sp->zvel < 648)
        vm.g_sp->zvel += ((moveptr[1]<<4) - vm.g_sp->zvel)/5;
#else
    vm.g_sp->xvel += (actor[vm.g_i].mv.hvel - vm.g_sp->xvel)/5;
    if (vm.g_sp->zvel < 648)
        vm.g_sp->zvel += ((actor[vm.g_i].mv.vvel<<4) - vm.g_sp->zvel)/5;
#endif

    if (A_CheckEnemySprite(vm.g_sp) && vm.g_sp->extra <= 0) // hack
        return;

    if (movflags&seekplayer)
    {
        int       nAngDiff;
        int       nGoalAng;
        int const nAngle    = vm.g_sp->ang;
        int32_t   nHoloDuke = vm.g_pp->holoduke_on;

        // NOTE: looks like 'owner' is set to target sprite ID...

        if (nHoloDuke >= 0 && cansee(sprite[nHoloDuke].x, sprite[nHoloDuke].y, sprite[nHoloDuke].z, sprite[nHoloDuke].sectnum,
                                     vm.g_sp->x, vm.g_sp->y, vm.g_sp->z, vm.g_sp->sectnum))
            vm.g_sp->owner = nHoloDuke;
        else vm.g_sp->owner = vm.g_pp->i;

        nGoalAng = (sprite[vm.g_sp->owner].picnum == APLAYER)
                   ? getangle(actor[vm.g_i].lastvx - vm.g_sp->x, actor[vm.g_i].lastvy - vm.g_sp->y)
                   : getangle(sprite[vm.g_sp->owner].x - vm.g_sp->x, sprite[vm.g_sp->owner].y - vm.g_sp->y);

        if (vm.g_sp->xvel && vm.g_sp->picnum != DRONE)
        {
            nAngDiff = G_GetAngleDelta(nAngle,nGoalAng);

            if (nElapsedTics < 2)
            {
                if (klabs(nAngDiff) < 256)
                {
                    int const j = 128-(krand()&256);
                    vm.g_sp->ang += j;
                    if (A_GetHitscanRange(vm.g_i) < 844)
                        vm.g_sp->ang -= j;
                }
            }
            else if (nElapsedTics > 18 && nElapsedTics < GAMETICSPERSEC) // choose
            {
                if (klabs(nAngDiff>>2) < 128) vm.g_sp->ang = nGoalAng;
                else vm.g_sp->ang += nAngDiff>>2;
            }
        }
        else vm.g_sp->ang = nGoalAng;
    }

    if (nElapsedTics < 1)
    {
        if (movflags&furthestdir)
        {
            vm.g_sp->ang = A_GetFurthestAngle(vm.g_i, 2);
            vm.g_sp->owner = vm.g_pp->i;
        }

        if (movflags&fleeenemy)
            vm.g_sp->ang = A_GetFurthestAngle(vm.g_i, 2);
    }
}

static inline void VM_AddAngle(int nShift, int nGoalAng)
{
    int nAngDiff = G_GetAngleDelta(vm.g_sp->ang, nGoalAng) >> nShift;

    if ((nAngDiff > -8 && nAngDiff < 0) || (nAngDiff < 8 && nAngDiff > 0))
        nAngDiff <<= 1;

    vm.g_sp->ang += nAngDiff;
}

static inline void VM_FacePlayer(int nShift)
{
    VM_AddAngle(nShift, (vm.g_pp->newowner >= 0) ? getangle(vm.g_pp->opos.x - vm.g_sp->x, vm.g_pp->opos.y - vm.g_sp->y)
                                                 : getangle(vm.g_pp->pos.x - vm.g_sp->x, vm.g_pp->pos.y - vm.g_sp->y));
}

////////// TROR get*zofslope //////////
// These rather belong into the engine.

static int32_t VM_GetCeilZOfSlope(void)
{
    vec2_t const vect     = *(vec2_t *)vm.g_sp;
    int const    sectnum  = vm.g_sp->sectnum;

#ifdef YAX_ENABLE
    if ((sector[sectnum].ceilingstat&512)==0)
    {
        int const nsect = yax_getneighborsect(vect.x, vect.y, sectnum, YAX_CEILING);
        if (nsect >= 0)
            return getceilzofslope(nsect, vect.x, vect.y);
    }
#endif
    return getceilzofslope(sectnum, vect.x, vect.y);
}

static int32_t VM_GetFlorZOfSlope(void)
{
    vec2_t const vect    = *(vec2_t *)vm.g_sp;
    int const    sectnum = vm.g_sp->sectnum;

#ifdef YAX_ENABLE
    if ((sector[sectnum].floorstat&512)==0)
    {
        int const nsect = yax_getneighborsect(vect.x, vect.y, sectnum, YAX_FLOOR);
        if (nsect >= 0)
            return getflorzofslope(nsect, vect.x, vect.y);
    }
#endif
    return getflorzofslope(sectnum, vect.x, vect.y);
}

////////////////////

static int32_t A_GetWaterZOffset(int spritenum);

GAMEEXEC_STATIC void VM_Move(void)
{
#if !defined LUNATIC
    const intptr_t *moveptr;
#endif
    // NOTE: commented out condition is dead since r3159 (making hi/lotag unsigned).
    // XXX: Does it break anything? Where are movflags with all bits set created?
    const uint16_t *movflagsptr = &AC_MOVFLAGS(vm.g_sp, &actor[vm.g_i]);
    int const movflags = /*(*movflagsptr==-1) ? 0 :*/ *movflagsptr;
    int const deadflag = (A_CheckEnemySprite(vm.g_sp) && vm.g_sp->extra <= 0);

    AC_COUNT(vm.g_t)++;

    if (AC_MOVE_ID(vm.g_t) == 0 || movflags == 0)
    {
        if (deadflag || (actor[vm.g_i].bpos.x != vm.g_sp->x) || (actor[vm.g_i].bpos.y != vm.g_sp->y))
        {
            actor[vm.g_i].bpos.x = vm.g_sp->x;
            actor[vm.g_i].bpos.y = vm.g_sp->y;
            setsprite(vm.g_i, (vec3_t *)vm.g_sp);
        }
        return;
    }

    if (deadflag)
        goto dead;

    if (movflags&face_player)
        VM_FacePlayer(2);

    if (movflags&spin)
        vm.g_sp->ang += sintable[((AC_COUNT(vm.g_t)<<3)&2047)]>>6;

    if (movflags&face_player_slow)
        VM_FacePlayer(4);

    if ((movflags&jumptoplayer_bits) == jumptoplayer_bits)
    {
        if (AC_COUNT(vm.g_t) < 16)
            vm.g_sp->zvel -= (sintable[(512+(AC_COUNT(vm.g_t)<<4))&2047]>>5);
    }

    if (movflags&face_player_smart)
    {
        vec2_t const vect = { vm.g_pp->pos.x + (vm.g_pp->vel.x / 768),
                              vm.g_pp->pos.y + (vm.g_pp->vel.y / 768) };
        VM_AddAngle(2, getangle(vect.x - vm.g_sp->x, vect.y - vm.g_sp->y));
    }

dead:
#if !defined LUNATIC
    if (EDUKE32_PREDICT_FALSE((unsigned)AC_MOVE_ID(vm.g_t) >= (unsigned)g_scriptSize-1))
    {
        AC_MOVE_ID(vm.g_t) = 0;
        OSD_Printf(OSD_ERROR "clearing bad moveptr for actor %d (%d)\n", vm.g_i, TrackerCast(vm.g_sp->picnum));
        return;
    }

    moveptr = script + AC_MOVE_ID(vm.g_t);

    if (movflags&geth) vm.g_sp->xvel += ((moveptr[0])-vm.g_sp->xvel)>>1;
    if (movflags&getv) vm.g_sp->zvel += ((moveptr[1]<<4)-vm.g_sp->zvel)>>1;
#else
    if (movflags&geth) vm.g_sp->xvel += (actor[vm.g_i].mv.hvel - vm.g_sp->xvel)>>1;
    if (movflags&getv) vm.g_sp->zvel += (16*actor[vm.g_i].mv.vvel - vm.g_sp->zvel)>>1;
#endif

    if (movflags&dodgebullet && !deadflag)
        A_Dodge(vm.g_sp);

    if (vm.g_sp->picnum != APLAYER)
        VM_AlterAng(movflags);

    if (vm.g_sp->xvel > -6 && vm.g_sp->xvel < 6)
        vm.g_sp->xvel = 0;

    int badguyp = A_CheckEnemySprite(vm.g_sp);

    if (vm.g_sp->xvel || vm.g_sp->zvel)
    {
        int nXvel    = vm.g_sp->xvel;
        int nAngDiff = vm.g_sp->ang;

        if (badguyp && vm.g_sp->picnum != ROTATEGUN)
        {
            if ((vm.g_sp->picnum == DRONE || vm.g_sp->picnum == COMMANDER) && vm.g_sp->extra > 0)
            {
                if (vm.g_sp->picnum == COMMANDER)
                {
                    int32_t nSectorZ;
                    // NOTE: COMMANDER updates both actor[].floorz and
                    // .ceilingz regardless of its zvel.
                    actor[vm.g_i].floorz = nSectorZ = VM_GetFlorZOfSlope();
                    if (vm.g_sp->z > nSectorZ-(8<<8))
                    {
                        vm.g_sp->z = nSectorZ-(8<<8);
                        vm.g_sp->zvel = 0;
                    }

                    actor[vm.g_i].ceilingz = nSectorZ = VM_GetCeilZOfSlope();
                    if (vm.g_sp->z < nSectorZ+(80<<8))
                    {
                        vm.g_sp->z = nSectorZ+(80<<8);
                        vm.g_sp->zvel = 0;
                    }
                }
                else
                {
                    int32_t nSectorZ;
                    // The DRONE updates either .floorz or .ceilingz, not both.
                    if (vm.g_sp->zvel > 0)
                    {
                        actor[vm.g_i].floorz = nSectorZ = VM_GetFlorZOfSlope();
                        if (vm.g_sp->z > nSectorZ-(30<<8))
                            vm.g_sp->z = nSectorZ-(30<<8);
                    }
                    else
                    {
                        actor[vm.g_i].ceilingz = nSectorZ = VM_GetCeilZOfSlope();
                        if (vm.g_sp->z < nSectorZ+(50<<8))
                        {
                            vm.g_sp->z = nSectorZ+(50<<8);
                            vm.g_sp->zvel = 0;
                        }
                    }
                }
            }
            else if (vm.g_sp->picnum != ORGANTIC)
            {
                // All other actors besides ORGANTIC don't update .floorz or
                // .ceilingz here.
                if (vm.g_sp->zvel > 0)
                {
                    if (vm.g_sp->z > actor[vm.g_i].floorz)
                        vm.g_sp->z = actor[vm.g_i].floorz;
                    vm.g_sp->z += A_GetWaterZOffset(vm.g_i);
                }
                else if (vm.g_sp->zvel < 0)
                {
                    int const l = VM_GetCeilZOfSlope();

                    if (vm.g_sp->z < l+(66<<8))
                    {
                        vm.g_sp->z = l+(66<<8);
                        vm.g_sp->zvel >>= 1;
                    }
                }
            }

            if (vm.g_x < 960 && vm.g_sp->xrepeat > 16)
            {
                nXvel = -(1024 - vm.g_x);
                nAngDiff = getangle(vm.g_pp->pos.x - vm.g_sp->x, vm.g_pp->pos.y - vm.g_sp->y);

                if (vm.g_x < 512)
                {
                    vm.g_pp->vel.x = 0;
                    vm.g_pp->vel.y = 0;
                }
                else
                {
                    vm.g_pp->vel.x = mulscale16(vm.g_pp->vel.x, vm.g_pp->runspeed - 0x2000);
                    vm.g_pp->vel.y = mulscale16(vm.g_pp->vel.y, vm.g_pp->runspeed - 0x2000);
                }
            }
            else if (vm.g_sp->picnum != DRONE && vm.g_sp->picnum != SHARK && vm.g_sp->picnum != COMMANDER)
            {
                if (vm.g_pp->actorsqu == vm.g_i)
                    return;

                if (!A_CheckSpriteFlags(vm.g_i, SFLAG_SMOOTHMOVE))
                {
                    if (AC_COUNT(vm.g_t) & 1)
                        return;
                    nXvel <<= 1;
                }
            }
        }
        else if (vm.g_sp->picnum == APLAYER)
            if (vm.g_sp->z < actor[vm.g_i].ceilingz+(32<<8))
                vm.g_sp->z = actor[vm.g_i].ceilingz+(32<<8);

        vec3_t const vect = { (nXvel * (sintable[(nAngDiff + 512) & 2047])) >> 14,
                              (nXvel * (sintable[nAngDiff & 2047])) >> 14, vm.g_sp->zvel };

        actor[vm.g_i].movflag = A_MoveSprite(vm.g_i, &vect, (A_CheckSpriteFlags(vm.g_i, SFLAG_NOCLIP) ? 0 : CLIPMASK0));
    }

    if (!badguyp)
        return;

    vm.g_sp->shade += (sector[vm.g_sp->sectnum].ceilingstat & 1) ? (sector[vm.g_sp->sectnum].ceilingshade - vm.g_sp->shade) >> 1
                                                                 : (sector[vm.g_sp->sectnum].floorshade - vm.g_sp->shade) >> 1;
}

static void P_AddWeaponMaybeSwitch(DukePlayer_t *ps, int nWeapon)
{
    if ((ps->weaponswitch & (1|4)) == (1|4))
    {
        int const nPlayer      = P_Get(ps->i);
        int       new_wchoice  = -1;
        int       curr_wchoice = -1;

        for (int i=0; i<=FREEZE_WEAPON && (new_wchoice < 0 || curr_wchoice < 0); i++)
        {
            int w = g_player[nPlayer].wchoice[i];

            if (w == KNEE_WEAPON)
                w = FREEZE_WEAPON;
            else w--;

            if (w == ps->curr_weapon)
                curr_wchoice = i;
            if (w == nWeapon)
                new_wchoice = i;
        }

        P_AddWeapon(ps, nWeapon, (new_wchoice < curr_wchoice));
    }
    else
    {
        P_AddWeapon(ps, nWeapon, (ps->weaponswitch & 1));
    }
}

#if defined LUNATIC
void P_AddWeaponMaybeSwitchI(int32_t snum, int32_t weap)
{
    P_AddWeaponMaybeSwitch(g_player[snum].ps, weap);
}
#else
static void P_AddWeaponAmmoCommon(DukePlayer_t *ps, int nWeapon, int nAmount)
{
    P_AddAmmo(nWeapon, ps, nAmount);

    if (PWEAPON(vm.g_p, ps->curr_weapon, WorksLike) == KNEE_WEAPON && (ps->gotweapon & (1 << nWeapon)))
        P_AddWeaponMaybeSwitch(ps, nWeapon);
}

static int VM_AddWeapon(DukePlayer_t *ps, int nWeapon, int nAmount)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)nWeapon >= MAX_WEAPONS))
    {
        CON_ERRPRINTF("Invalid weapon ID %d\n", nWeapon);
        return 1;
    }

    if ((ps->gotweapon & (1 << nWeapon)) == 0)
    {
        P_AddWeaponMaybeSwitch(ps, nWeapon);
    }
    else if (ps->ammo_amount[nWeapon] >= ps->max_ammo_amount[nWeapon])
    {
        vm.g_flags |= VM_NOEXECUTE;
        return 2;
    }

    P_AddWeaponAmmoCommon(ps, nWeapon, nAmount);

    return 0;
}
#endif

static int32_t A_GetVerticalVel(actor_t const * const pActor)
{
#ifdef LUNATIC
    return pActor->mv.vvel;
#else
    int32_t moveScriptOfs = AC_MOVE_ID(pActor->t_data);

    return ((unsigned) moveScriptOfs < (unsigned) g_scriptSize - 1) ? script[moveScriptOfs + 1] : 0;
#endif
}

static int32_t A_GetWaterZOffset(int nSprite)
{
    uspritetype const *const pSprite = (uspritetype *)&sprite[nSprite];
    actor_t const *const     pActor  = &actor[nSprite];

    if (sector[pSprite->sectnum].lotag == ST_1_ABOVE_WATER)
    {
        if (A_CheckSpriteFlags(nSprite, SFLAG_NOWATERDIP))
            return 0;

        // fix for flying/jumping monsters getting stuck in water
        if ((AC_MOVFLAGS(pSprite, pActor) & jumptoplayer_only) || (G_HaveActor(pSprite->picnum) && A_GetVerticalVel(pActor) != 0))
            return 0;

        return ACTOR_ONWATER_ADDZ;
    }

    return 0;
}

static void VM_Fall(int nSprite, spritetype *pSprite)
{
    int nGravity = g_spriteGravity;

    pSprite->xoffset = pSprite->yoffset = 0;

    if (sector[pSprite->sectnum].lotag == ST_2_UNDERWATER || EDUKE32_PREDICT_FALSE(G_CheckForSpaceCeiling(pSprite->sectnum)))
        nGravity = g_spriteGravity/6;
    else if (EDUKE32_PREDICT_FALSE(G_CheckForSpaceFloor(pSprite->sectnum)))
        nGravity = 0;

    if (!actor[nSprite].cgg-- || (sector[pSprite->sectnum].floorstat&2))
    {
        A_GetZLimits(nSprite);
        actor[nSprite].cgg = 3;
    }

    if (pSprite->z < actor[nSprite].floorz-ZOFFSET)
    {
        // Free fall.
        pSprite->zvel = min(pSprite->zvel+nGravity, ACTOR_MAXFALLINGZVEL);
        int32_t z = pSprite->z + pSprite->zvel;

#ifdef YAX_ENABLE
        if (yax_getbunch(pSprite->sectnum, YAX_FLOOR) >= 0 &&
                (sector[pSprite->sectnum].floorstat&512)==0)
            setspritez(nSprite, (vec3_t *)pSprite);
        else
#endif
            if (z > actor[nSprite].floorz - ZOFFSET)
                z = actor[nSprite].floorz - ZOFFSET;

        pSprite->z = z;
        return;
    }

    // Preliminary new z position of the actor.
    int32_t z = actor[nSprite].floorz - ZOFFSET;

    if (A_CheckEnemySprite(pSprite) || (pSprite->picnum == APLAYER && pSprite->owner >= 0))
    {
        if (pSprite->zvel > 3084 && pSprite->extra <= 1)
        {
            // I'm guessing this DRONE check is from a beta version of the game
            // where they crashed into the ground when killed
            if (!(pSprite->picnum == APLAYER && pSprite->extra > 0) && pSprite->pal != 1 && pSprite->picnum != DRONE)
            {
                A_DoGuts(nSprite,JIBS6,15);
                A_PlaySound(SQUISHED,nSprite);
                A_Spawn(nSprite,BLOODPOOL);
            }

            actor[nSprite].picnum = SHOTSPARK1;
            actor[nSprite].extra = 1;
            pSprite->zvel = 0;
        }
        else if (pSprite->zvel > 2048 && sector[pSprite->sectnum].lotag != ST_1_ABOVE_WATER)
        {
            int16_t newsect = pSprite->sectnum;

            pushmove((vec3_t *)pSprite, &newsect, 128, 4<<8, 4<<8, CLIPMASK0);
            if ((unsigned)newsect < MAXSECTORS)
                changespritesect(nSprite, newsect);

            A_PlaySound(THUD, nSprite);
        }
    }

    if (sector[pSprite->sectnum].lotag == ST_1_ABOVE_WATER)
    {
        pSprite->z = z + A_GetWaterZOffset(nSprite);
        return;
    }

    pSprite->z = z;
    pSprite->zvel = 0;
}

static int32_t VM_ResetPlayer(int nPlayer, int32_t g_flags, int32_t nFlags)
{
    //AddLog("resetplayer");
    if (!g_netServer && ud.multimode < 2 && !(nFlags & 2))
    {
        if (g_lastSaveSlot >= 0 && ud.recstat != 2 && !(nFlags & 1))
        {
            M_OpenMenu(nPlayer);
            KB_ClearKeyDown(sc_Space);
            I_AdvanceTriggerClear();
            M_ChangeMenu(MENU_RESETPLAYER);
        }
        else g_player[nPlayer].ps->gm = MODE_RESTART;
#if !defined LUNATIC
        g_flags |= VM_NOEXECUTE;
#endif
    }
    else
    {
        if (nPlayer == myconnectindex)
        {
            CAMERADIST = 0;
            CAMERACLOCK = totalclock;
        }

        if (g_fakeMultiMode)
            P_ResetPlayer(nPlayer);
#ifndef NETCODE_DISABLE
        if (g_netServer)
        {
            P_ResetPlayer(nPlayer);
            Net_SpawnPlayer(nPlayer);
        }
#endif
    }

    P_UpdateScreenPal(g_player[nPlayer].ps);
    //AddLog("EOF: resetplayer");

    return g_flags;
}

void G_GetTimeDate(int32_t *vals)
{
    time_t rawtime;
    time(&rawtime);
    struct tm *ti = localtime(&rawtime);

    // initprintf("Time&date: %s\n",asctime (ti));

    vals[0] = ti->tm_sec;
    vals[1] = ti->tm_min;
    vals[2] = ti->tm_hour;
    vals[3] = ti->tm_mday;
    vals[4] = ti->tm_mon;
    vals[5] = ti->tm_year+1900;
    vals[6] = ti->tm_wday;
    vals[7] = ti->tm_yday;
}

int G_StartTrack(int nLevel)
{
    if ((unsigned)nLevel < MAXLEVELS)
    {
        int nTrackNum = MAXLEVELS*ud.volume_number + nLevel;

        if (MapInfo[nTrackNum].musicfn != NULL)
        {
            // Only set g_musicIndex on success.
            g_musicIndex = nTrackNum;
            S_PlayMusic(MapInfo[nTrackNum].musicfn);

            return 0;
        }
    }

    return 1;
}

LUNATIC_EXTERN void G_ShowView(vec3_t vec, int32_t a, int32_t horiz, int32_t sect,
                               int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t unbiasedp)
{
    if (g_screenCapture)
        return;

    if (offscreenrendering)
    {
        clearview(0);
        return;
    }

    if (x1 > x2) swaplong(&x1,&x2);
    if (y1 > y2) swaplong(&y1,&y2);

    if (!unbiasedp)
    {
        // The showview command has a rounding bias towards zero,
        // e.g. floor((319*1680)/320) == 1674
        x1 = scale(x1,xdim,320);
        y1 = scale(y1,ydim,200);
        x2 = scale(x2,xdim,320);
        y2 = scale(y2,ydim,200);
    }
    else
    {
        // This will map the maximum 320-based coordinate to the
        // maximum real screen coordinate:
        // floor((319*1679)/319) == 1679
        x1 = scale(x1,xdim-1,319);
        y1 = scale(y1,ydim-1,199);
        x2 = scale(x2,xdim-1,319);
        y2 = scale(y2,ydim-1,199);
    }

    horiz = clamp(horiz, HORIZ_MIN, HORIZ_MAX);

#ifdef USE_OPENGL
    int const oprojhacks = glprojectionhacks;
    glprojectionhacks = 0;
#endif
    int const onewaspect = newaspect_enable;
    newaspect_enable = r_usenewaspect;
    setaspect_new_use_dimen = 1;
    setview(x1,y1,x2,y2);
    setaspect_new_use_dimen = 0;
    newaspect_enable = onewaspect;

    int const smoothratio = calc_smoothratio(totalclock, ototalclock);
    G_DoInterpolations(smoothratio);
    G_HandleMirror(vec.x, vec.y, vec.z, a, horiz, smoothratio);
#ifdef POLYMER
    if (getrendermode() == REND_POLYMER)
        polymer_setanimatesprites(G_DoSpriteAnimations, vec.x,vec.y,a,smoothratio);
#endif
    yax_preparedrawrooms();
    drawrooms(vec.x,vec.y,vec.z,a,horiz,sect);
    yax_drawrooms(G_DoSpriteAnimations, sect, 0, smoothratio);

    display_mirror = 2;
    G_DoSpriteAnimations(vec.x,vec.y,a,smoothratio);
    display_mirror = 0;
    drawmasks();
    G_RestoreInterpolations();
    G_UpdateScreenArea();
#ifdef USE_OPENGL
    glprojectionhacks = oprojhacks;
#endif
}

#if !defined LUNATIC
GAMEEXEC_STATIC void VM_Execute(int loop)
{
    int tw = *insptr;
    DukePlayer_t * const ps = vm.g_pp;

    // jump directly into the loop, saving us from the checks during the first iteration
    goto skip_check;

    while (loop)
    {
        if (vm.g_flags & (VM_RETURN | VM_KILL | VM_NOEXECUTE))
            break;

        tw = *insptr;

skip_check:
        //      Bsprintf(g_szBuf,"Parsing: %d",*insptr);
        //      AddLog(g_szBuf);

        g_errorLineNum = tw>>12;
        g_tw = tw &= VM_INSTMASK;

        if (tw == CON_LEFTBRACE)
        {
            insptr++, loop++;
            continue;
        }
        else if (tw == CON_RIGHTBRACE)
        {
            insptr++, loop--;
            continue;
        }
        else if (tw == CON_ELSE)
        {
            insptr = (intptr_t *) *(insptr+1);
            continue;
        }
        else if (tw == CON_STATE)
        {
            intptr_t const * const tempscrptr = insptr + 2;
            insptr = (intptr_t *)*(insptr + 1);
            VM_Execute(1);
            insptr = tempscrptr;
            continue;
        }

        switch (tw)
        {
        case CON_IFVARE:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            VM_CONDITIONAL(tw == *insptr);
            continue;

        case CON_REDEFINEQUOTE:
            insptr++;
            {
                int const nDestQuote = *insptr++;
                int const nRedef = *insptr++;
                if (EDUKE32_PREDICT_FALSE((ScriptQuotes[nDestQuote] == NULL || ScriptQuoteRedefinitions[nRedef] == NULL)))
                {
                    CON_ERRPRINTF("%d %d null quote\n", nDestQuote,nRedef);
                    break;
                }
                Bstrcpy(ScriptQuotes[nDestQuote],ScriptQuoteRedefinitions[nRedef]);
                continue;
            }

        case CON_GETTHISPROJECTILE:
            insptr++;
            {
                tw = *insptr++;
                int const nSprite  = (tw != g_iThisActorID) ? Gv_GetVarX(tw) : vm.g_i;
                int const nLabel   = *insptr++;

                Gv_SetVarX(*insptr++, VM_GetActiveProjectile(nSprite, nLabel));
                continue;
            }

        case CON_SETTHISPROJECTILE:
            insptr++;
            {
                tw = *insptr++;
                int const nSprite = (tw != g_iThisActorID) ? Gv_GetVarX(tw) : vm.g_i;
                int const nLabel = *insptr++;

                VM_SetActiveProjectile(nSprite, nLabel, Gv_GetVarX(*insptr++));
                continue;
            }

        case CON_IFRND:
            VM_CONDITIONAL(rnd(*(++insptr)));
            continue;

        case CON_IFCANSHOOTTARGET:
        {
            if (vm.g_x > 1024)
            {
                int16_t temphit;

                if ((tw = A_CheckHitSprite(vm.g_i, &temphit)) == (1 << 30))
                {
                    VM_CONDITIONAL(1);
                    continue;
                }

                int nClip = 768;
                int nAngDiff = 16;

                if (A_CheckEnemySprite(vm.g_sp) && vm.g_sp->xrepeat > 56)
                {
                    nClip = 3084;
                    nAngDiff = 48;
                }

#define CHECK(x) if (x >= 0 && sprite[x].picnum == vm.g_sp->picnum) { VM_CONDITIONAL(0); continue; }
#define CHECK2(x) do { vm.g_sp->ang += x; tw = A_CheckHitSprite(vm.g_i, &temphit); vm.g_sp->ang -= x; } while(0)

                if (tw > nClip)
                {
                    CHECK(temphit);
                    CHECK2(nAngDiff);

                    if (tw > nClip)
                    {
                        CHECK(temphit);
                        CHECK2(-nAngDiff);

                        if (tw > 768)
                        {
                            CHECK(temphit);
                            VM_CONDITIONAL(1);
                            continue;
                        }
                    }
                }
            }
            VM_CONDITIONAL(1);
        }
        continue;

        case CON_IFCANSEETARGET:
            tw = cansee(vm.g_sp->x, vm.g_sp->y, vm.g_sp->z-((krand()&41)<<8),
                               vm.g_sp->sectnum, ps->pos.x, ps->pos.y,
                               ps->pos.z/*-((krand()&41)<<8)*/, sprite[ps->i].sectnum);
            VM_CONDITIONAL(tw);
            if (tw) actor[vm.g_i].timetosleep = SLEEPTIME;
        continue;

        case CON_IFACTORNOTSTAYPUT:
            VM_CONDITIONAL(actor[vm.g_i].actorstayput == -1);
            continue;

        case CON_IFCANSEE:
        {
            uspritetype *s = (uspritetype *)&sprite[ps->i];

            // select sprite for monster to target
            // if holoduke is on, let them target holoduke first.
            //
            if (ps->holoduke_on >= 0)
            {
                s = (uspritetype *)&sprite[ps->holoduke_on];
                tw = cansee(vm.g_sp->x,vm.g_sp->y,vm.g_sp->z-(krand()&((32<<8)-1)),vm.g_sp->sectnum,
                           s->x,s->y,s->z,s->sectnum);

                if (tw == 0)
                {
                    // they can't see player's holoduke
                    // check for player...
                    s = (uspritetype *)&sprite[ps->i];
                }
            }

            // can they see player, (or player's holoduke)
            tw = cansee(vm.g_sp->x,vm.g_sp->y,vm.g_sp->z-(krand()&((47<<8))),vm.g_sp->sectnum,
                       s->x,s->y,s->z-(24<<8),s->sectnum);

            if (tw == 0)
            {
                // search around for target player

                // also modifies 'target' x&y if found..

                tw = 1;
                if (A_FurthestVisiblePoint(vm.g_i,s,&actor[vm.g_i].lastvx,&actor[vm.g_i].lastvy) == -1)
                    tw = 0;
            }
            else
            {
                // else, they did see it.
                // save where we were looking...
                actor[vm.g_i].lastvx = s->x;
                actor[vm.g_i].lastvy = s->y;
            }

            if (tw && (vm.g_sp->statnum == STAT_ACTOR || vm.g_sp->statnum == STAT_STANDABLE))
                actor[vm.g_i].timetosleep = SLEEPTIME;

            VM_CONDITIONAL(tw);
            continue;
        }

        case CON_IFHITWEAPON:
            VM_CONDITIONAL(A_IncurDamage(vm.g_i) >= 0);
            continue;

        case CON_IFSQUISHED:
            VM_CONDITIONAL(VM_CheckSquished());
            continue;

        case CON_IFDEAD:
            VM_CONDITIONAL(vm.g_sp->extra <= 0);
            continue;

        case CON_AI:
            insptr++;
            //Following changed to use pointersizes
            AC_AI_ID(vm.g_t) = *insptr++; // Ai

            AC_ACTION_ID(vm.g_t) = *(script + AC_AI_ID(vm.g_t));  // Action

            // NOTE: "if" check added in r1155. It used to be a pointer though.
            if (AC_AI_ID(vm.g_t))
                AC_MOVE_ID(vm.g_t) = *(script + AC_AI_ID(vm.g_t) + 1);  // move

            vm.g_sp->hitag = *(script + AC_AI_ID(vm.g_t) + 2);  // move flags

            AC_COUNT(vm.g_t) = AC_ACTION_COUNT(vm.g_t) = AC_CURFRAME(vm.g_t) = 0;

            if (!A_CheckEnemySprite(vm.g_sp) || vm.g_sp->extra > 0) // hack
                if (vm.g_sp->hitag&random_angle)
                    vm.g_sp->ang = krand()&2047;
            continue;

        case CON_ACTION:
            insptr++;
            AC_ACTION_COUNT(vm.g_t) = AC_CURFRAME(vm.g_t) = 0;
            AC_ACTION_ID(vm.g_t) = *insptr++;
            continue;

        case CON_IFPLAYERSL:
            VM_CONDITIONAL(numplayers < *(++insptr));
            continue;

        case CON_IFPDISTL:
            VM_CONDITIONAL(vm.g_x < *(++insptr));
            if (vm.g_x > MAXSLEEPDIST && actor[vm.g_i].timetosleep == 0)
                actor[vm.g_i].timetosleep = SLEEPTIME;
            continue;

        case CON_IFPDISTG:
            VM_CONDITIONAL(vm.g_x > *(++insptr));
            if (vm.g_x > MAXSLEEPDIST && actor[vm.g_i].timetosleep == 0)
                actor[vm.g_i].timetosleep = SLEEPTIME;
            continue;

        case CON_ADDSTRENGTH:
            insptr++;
            vm.g_sp->extra += *insptr++;
            continue;

        case CON_STRENGTH:
            insptr++;
            vm.g_sp->extra = *insptr++;
            continue;

        case CON_IFGOTWEAPONCE:
            insptr++;

            if ((GametypeFlags[ud.coop]&GAMETYPE_WEAPSTAY) && (g_netServer || ud.multimode > 1))
            {
                if (*insptr == 0)
                {
                    int j = 0;
                    for (; j < ps->weapreccnt; j++)
                        if (ps->weaprecs[j] == vm.g_sp->picnum)
                            break;

                    VM_CONDITIONAL(j < ps->weapreccnt && vm.g_sp->owner == vm.g_i);
                    continue;
                }
                else if (ps->weapreccnt < MAX_WEAPONS)
                {
                    ps->weaprecs[ps->weapreccnt++] = vm.g_sp->picnum;
                    VM_CONDITIONAL(vm.g_sp->owner == vm.g_i);
                    continue;
                }
            }
            VM_CONDITIONAL(0);
            continue;

        case CON_GETLASTPAL:
            insptr++;
            if (vm.g_sp->picnum == APLAYER)
                vm.g_sp->pal = g_player[P_GetP(vm.g_sp)].ps->palookup;
            else
            {
                if (vm.g_sp->pal == 1 && vm.g_sp->extra == 0) // hack for frozen
                    vm.g_sp->extra++;
                vm.g_sp->pal = actor[vm.g_i].tempang;
            }
            actor[vm.g_i].tempang = 0;
            continue;

        case CON_TOSSWEAPON:
            insptr++;
            // NOTE: assumes that current actor is APLAYER
            P_DropWeapon(P_GetP(vm.g_sp));
            continue;

        case CON_MIKESND:
            insptr++;
            if (EDUKE32_PREDICT_FALSE(((unsigned)vm.g_sp->yvel >= MAXSOUNDS)))
            {
                CON_ERRPRINTF("Invalid sound %d\n", TrackerCast(vm.g_sp->yvel));
                continue;
            }
            if (!S_CheckSoundPlaying(vm.g_i,vm.g_sp->yvel))
                A_PlaySound(vm.g_sp->yvel,vm.g_i);
            continue;

        case CON_PKICK:
            insptr++;

            if ((g_netServer || ud.multimode > 1) && vm.g_sp->picnum == APLAYER)
            {
                if (g_player[otherp].ps->quick_kick == 0)
                    g_player[otherp].ps->quick_kick = 14;
            }
            else if (vm.g_sp->picnum != APLAYER && ps->quick_kick == 0)
                ps->quick_kick = 14;
            continue;

        case CON_SIZETO:
            insptr++;

            tw = (*insptr++ - vm.g_sp->xrepeat)<<1;
            vm.g_sp->xrepeat += ksgn(tw);

            if ((vm.g_sp->picnum == APLAYER && vm.g_sp->yrepeat < 36) || *insptr < vm.g_sp->yrepeat ||
                    ((vm.g_sp->yrepeat*(tilesiz[vm.g_sp->picnum].y+8))<<2) < (actor[vm.g_i].floorz - actor[vm.g_i].ceilingz))
            {
                tw = ((*insptr)-vm.g_sp->yrepeat)<<1;
                if (klabs(tw)) vm.g_sp->yrepeat += ksgn(tw);
            }

            insptr++;

            continue;

        case CON_SIZEAT:
            insptr++;
            vm.g_sp->xrepeat = (uint8_t) *insptr++;
            vm.g_sp->yrepeat = (uint8_t) *insptr++;
            continue;

        case CON_SHOOT:
            insptr++;
            A_Shoot(vm.g_i,*insptr++);
            continue;

        case CON_SOUNDONCE:
            if (EDUKE32_PREDICT_FALSE((unsigned)*(++insptr) >= MAXSOUNDS))
            {
                CON_ERRPRINTF("Invalid sound %d\n", (int32_t)*insptr++);
                continue;
            }

            if (!S_CheckSoundPlaying(vm.g_i, *insptr++))
                A_PlaySound(*(insptr-1),vm.g_i);

            continue;

        case CON_IFACTORSOUND:
            insptr++;
            {
                int const i = Gv_GetVarX(*insptr++), j = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)j >= MAXSOUNDS))
                {
                    CON_ERRPRINTF("Invalid sound %d\n", j);
                    insptr++;
                    continue;
                }

                insptr--;
                VM_CONDITIONAL(A_CheckSoundPlaying(i, j));
            }
            continue;

        case CON_IFSOUND:
            if (EDUKE32_PREDICT_FALSE((unsigned)*(++insptr) >= MAXSOUNDS))
            {
                CON_ERRPRINTF("Invalid sound %d\n", (int32_t)*insptr);
                insptr++;
                continue;
            }
            VM_CONDITIONAL(S_CheckSoundPlaying(vm.g_i,*insptr));
            //    VM_DoConditional(SoundOwner[*insptr][0].ow == vm.g_i);
            continue;

        case CON_STOPSOUND:
            if (EDUKE32_PREDICT_FALSE((unsigned)*(++insptr) >= MAXSOUNDS))
            {
                CON_ERRPRINTF("Invalid sound %d\n", (int32_t)*insptr);
                insptr++;
                continue;
            }
            if (S_CheckSoundPlaying(vm.g_i,*insptr))
                S_StopSound((int16_t)*insptr);
            insptr++;
            continue;

        case CON_STOPACTORSOUND:
            insptr++;
            {
                int const nSprite = Gv_GetVarX(*insptr++), nSound = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)nSound >= MAXSOUNDS))
                {
                    CON_ERRPRINTF("Invalid sound %d\n", nSound);
                    continue;
                }

                if (A_CheckSoundPlaying(nSprite, nSound))
                    S_StopEnvSound(nSound, nSprite);

                continue;
            }

        case CON_SETACTORSOUNDPITCH:
            insptr++;
            {
                int const nSprite = Gv_GetVarX(*insptr++);
                int const nSound  = Gv_GetVarX(*insptr++);
                int const nPitch  = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)nSound>=MAXSOUNDS))
                {
                    CON_ERRPRINTF("Invalid sound %d\n", nSound);
                    continue;
                }

                S_ChangeSoundPitch(nSound, nSprite, nPitch);

                continue;
            }

        case CON_GLOBALSOUND:
            if (EDUKE32_PREDICT_FALSE((unsigned)*(++insptr) >= MAXSOUNDS))
            {
                CON_ERRPRINTF("Invalid sound %d\n", (int32_t)*insptr);
                insptr++;
                continue;
            }
            if (vm.g_p == screenpeek || (GametypeFlags[ud.coop]&GAMETYPE_COOPSOUND)
#ifdef SPLITSCREEN_MOD_HACKS
                || (g_fakeMultiMode==2)
#endif
                )
                A_PlaySound(*insptr,g_player[screenpeek].ps->i);
            insptr++;
            continue;

        case CON_SOUND:
            if (EDUKE32_PREDICT_FALSE((unsigned)*(++insptr) >= MAXSOUNDS))
            {
                CON_ERRPRINTF("Invalid sound %d\n", (int32_t)*insptr);
                insptr++;
                continue;
            }
            A_PlaySound(*insptr++,vm.g_i);
            continue;

        case CON_TIP:
            insptr++;
            ps->tipincs = GAMETICSPERSEC;
            continue;

        case CON_FALL:
            insptr++;
            VM_Fall(vm.g_i, vm.g_sp);
            continue;

        case CON_RETURN:
            vm.g_flags |= VM_RETURN;
        case CON_ENDA:
        case CON_BREAK:
        case CON_ENDS:
            return;
        case CON_NULLOP:
            insptr++;
            continue;

        case CON_ADDAMMO:
            insptr++;
            {
                int const nWeapon = *insptr++;
                int const nAmount = *insptr++;

                if (EDUKE32_PREDICT_FALSE((unsigned)nWeapon >= MAX_WEAPONS))
                {
                    CON_ERRPRINTF("Invalid weapon ID %d\n", nWeapon);
                    break;
                }

                if (ps->ammo_amount[nWeapon] >= ps->max_ammo_amount[nWeapon])
                {
                    vm.g_flags |= VM_NOEXECUTE;
                    return;
                }

                P_AddWeaponAmmoCommon(ps, nWeapon, nAmount);

                continue;
            }

        case CON_MONEY:
            insptr++;
            A_SpawnMultiple(vm.g_i, MONEY, *insptr++);
            continue;

        case CON_MAIL:
            insptr++;
            A_SpawnMultiple(vm.g_i, MAIL, *insptr++);
            continue;

        case CON_SLEEPTIME:
            insptr++;
            actor[vm.g_i].timetosleep = (int16_t)*insptr++;
            continue;

        case CON_PAPER:
            insptr++;
            A_SpawnMultiple(vm.g_i, PAPER, *insptr++);
            continue;

        case CON_ADDKILLS:
            insptr++;
            ps->actors_killed += *insptr++;
            actor[vm.g_i].actorstayput = -1;
            continue;

        case CON_LOTSOFGLASS:
            insptr++;
            A_SpawnGlass(vm.g_i,*insptr++);
            continue;

        case CON_KILLIT:
            insptr++;
            vm.g_flags |= VM_KILL;
            return;

        case CON_ADDWEAPON:
            insptr++;
            {
                int const nWeapon = *insptr++;
                VM_AddWeapon(ps, nWeapon, *insptr++);
                continue;
            }

        case CON_DEBUG:
            insptr++;
            initprintf("%" PRIdPTR "\n",*insptr++);
            continue;

        case CON_ENDOFGAME:
        case CON_ENDOFLEVEL:
            insptr++;
            ps->timebeforeexit = *insptr++;
            ps->customexitsound = -1;
            ud.eog = 1;
            continue;

        case CON_ADDPHEALTH:
            insptr++;

            {
                if (ps->newowner >= 0)
                    G_ClearCameraView(ps);

                int nHealth = sprite[ps->i].extra;

                if (vm.g_sp->picnum != ATOMICHEALTH)
                {
                    if (nHealth > ps->max_player_health && *insptr > 0)
                    {
                        insptr++;
                        continue;
                    }
                    else
                    {
                        if (nHealth > 0)
                            nHealth += *insptr;
                        if (nHealth > ps->max_player_health && *insptr > 0)
                            nHealth = ps->max_player_health;
                    }
                }
                else
                {
                    if (nHealth > 0)
                        nHealth += *insptr;
                    if (nHealth > (ps->max_player_health<<1))
                        nHealth = (ps->max_player_health<<1);
                }

                if (nHealth < 0) nHealth = 0;

                if (ud.god == 0)
                {
                    if (*insptr > 0)
                    {
                        if ((nHealth - *insptr) < (ps->max_player_health>>2) &&
                                nHealth >= (ps->max_player_health>>2))
                            A_PlaySound(DUKE_GOTHEALTHATLOW,ps->i);

                        ps->last_extra = nHealth;
                    }

                    sprite[ps->i].extra = nHealth;
                }
            }

            insptr++;
            continue;

        case CON_MOVE:
            insptr++;
            AC_COUNT(vm.g_t) = 0;
            AC_MOVE_ID(vm.g_t) = *insptr++;
            vm.g_sp->hitag = *insptr++;
            if (A_CheckEnemySprite(vm.g_sp) && vm.g_sp->extra <= 0) // hack
                continue;
            if (vm.g_sp->hitag&random_angle)
                vm.g_sp->ang = krand()&2047;
            continue;

        case CON_ADDWEAPONVAR:
            insptr++;
            {
                int const nWeapon = Gv_GetVarX(*insptr++);
                VM_AddWeapon(ps, nWeapon, Gv_GetVarX(*insptr++));
                continue;
            }

        case CON_SETASPECT:
            insptr++;
            {
                int const nRange = Gv_GetVarX(*insptr++);
                setaspect(nRange, Gv_GetVarX(*insptr++));
                break;
            }

        case CON_SSP:
            insptr++;
            {
                int const nSprite = Gv_GetVarX(*insptr++);
                int const nClipType = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)nSprite >= MAXSPRITES))
                {
                    CON_ERRPRINTF("Invalid sprite %d\n", nSprite);
                    break;
                }
                A_SetSprite(nSprite, nClipType);
                break;
            }

        case CON_ACTIVATEBYSECTOR:
            insptr++;
            {
                int const nSector = Gv_GetVarX(*insptr++);
                int const nSprite = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)nSector >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector %d\n", nSector);
                    break;
                }
                G_ActivateBySector(nSector, nSprite);
                break;
            }

        case CON_OPERATESECTORS:
            insptr++;
            {
                int const nSector = Gv_GetVarX(*insptr++);
                int const nSprite = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)nSector >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector %d\n", nSector);
                    break;
                }
                G_OperateSectors(nSector, nSprite);
                break;
            }

        case CON_OPERATEACTIVATORS:
            insptr++;
            {
                int const nTag = Gv_GetVarX(*insptr++);
                int const nPlayer = (*insptr++ == g_iThisActorID) ? vm.g_p : Gv_GetVarX(*(insptr-1));

                if (EDUKE32_PREDICT_FALSE((unsigned)nPlayer >= (unsigned)playerswhenstarted))
                {
                    CON_ERRPRINTF("Invalid player %d\n", nPlayer);
                    break;
                }
                G_OperateActivators(nTag, nPlayer);
                break;
            }


        case CON_CANSEESPR:
            insptr++;
            {
                int const nSprite1 = Gv_GetVarX(*insptr++);
                int const nSprite2 = Gv_GetVarX(*insptr++);
                int nResult = 0;

                if (EDUKE32_PREDICT_FALSE((unsigned)nSprite1 >= MAXSPRITES || (unsigned)nSprite2 >= MAXSPRITES))
                    CON_ERRPRINTF("Invalid sprite %d\n", (unsigned)nSprite1 >= MAXSPRITES ? nSprite1 : nSprite2);
                else
                {
                    nResult = cansee(sprite[nSprite1].x, sprite[nSprite1].y, sprite[nSprite1].z, sprite[nSprite1].sectnum,
                                 sprite[nSprite2].x, sprite[nSprite2].y, sprite[nSprite2].z, sprite[nSprite2].sectnum);
                }

                Gv_SetVarX(*insptr++, nResult);
                continue;
            }

        case CON_OPERATERESPAWNS:
            insptr++;
            G_OperateRespawns(Gv_GetVarX(*insptr++));
            continue;

        case CON_OPERATEMASTERSWITCHES:
            insptr++;
            G_OperateMasterSwitches(Gv_GetVarX(*insptr++));
            continue;

        case CON_CHECKACTIVATORMOTION:
            insptr++;
            aGameVars[g_iReturnVarID].nValue = G_CheckActivatorMotion(Gv_GetVarX(*insptr++));
            continue;

        case CON_INSERTSPRITEQ:
            insptr++;
            A_AddToDeleteQueue(vm.g_i);
            continue;

        case CON_QSTRLEN:
            insptr++;
            {
                int const nGameVar = *insptr++;
                int const nQuote   = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE(ScriptQuotes[nQuote] == NULL))
                {
                    CON_ERRPRINTF("null quote %d\n", nQuote);
                    Gv_SetVarX(nGameVar, -1);
                    continue;
                }

                Gv_SetVarX(nGameVar, Bstrlen(ScriptQuotes[nQuote]));
                continue;
            }

        case CON_QSTRDIM:
            insptr++;
            {
                vec2_t dim = { 0, 0, };

                int const w = *insptr++;
                int const h = *insptr++;

                int32_t params[16];

                Gv_GetManyVars(16, params);

                int const tilenum = params[0], x = params[1], y = params[2], z = params[3];
                int const blockangle = params[4], q = params[5];
                int const orientation = params[6] & (ROTATESPRITE_MAX-1);
                int const xspace = params[7], yline = params[8], xbetween = params[9];
                int const ybetween = params[10], f = params[11];
                int const x1 = params[12], y1 = params[13], x2 = params[14], y2 = params[15];

                if (EDUKE32_PREDICT_FALSE(tilenum < 0 || tilenum+255 >= MAXTILES))
                    CON_ERRPRINTF("invalid base tilenum %d\n", tilenum);
                else if (EDUKE32_PREDICT_FALSE((unsigned)q >= MAXQUOTES || ScriptQuotes[q] == NULL))
                    CON_ERRPRINTF("invalid quote ID %d\n", q);
                else
                    dim = G_ScreenTextSize(tilenum, x, y, z, blockangle, ScriptQuotes[q], 2 | orientation,
                                           xspace, yline, xbetween, ybetween, f, x1, y1, x2, y2);

                Gv_SetVarX(w, dim.x);
                Gv_SetVarX(h, dim.y);
                continue;
            }

        case CON_HEADSPRITESTAT:
            insptr++;
            {
                int const nGameVar  = *insptr++;
                int const nSprite = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)nSprite > MAXSTATUS))
                {
                    CON_ERRPRINTF("invalid status list %d\n", nSprite);
                    continue;
                }

                Gv_SetVarX(nGameVar,headspritestat[nSprite]);
                continue;
            }

        case CON_PREVSPRITESTAT:
            insptr++;
            {
                int const nGameVar  = *insptr++;
                int const nSprite = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)nSprite >= MAXSPRITES))
                {
                    CON_ERRPRINTF("invalid sprite ID %d\n", nSprite);
                    continue;
                }

                Gv_SetVarX(nGameVar, prevspritestat[nSprite]);
                continue;
            }

        case CON_NEXTSPRITESTAT:
            insptr++;
            {
                int const nGameVar  = *insptr++;
                int const nSprite = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)nSprite >= MAXSPRITES))
                {
                    CON_ERRPRINTF("invalid sprite ID %d\n", nSprite);
                    continue;
                }

                Gv_SetVarX(nGameVar, nextspritestat[nSprite]);
                continue;
            }

        case CON_HEADSPRITESECT:
            insptr++;
            {
                int const nGameVar = *insptr++,
                          nSprite = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)nSprite >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("invalid sector %d\n", nSprite);
                    continue;
                }

                Gv_SetVarX(nGameVar, headspritesect[nSprite]);
                continue;
            }

        case CON_PREVSPRITESECT:
            insptr++;
            {
                int const nGameVar = *insptr++;
                int const nSprite = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)nSprite >= MAXSPRITES))
                {
                    CON_ERRPRINTF("invalid sprite ID %d\n", nSprite);
                    continue;
                }

                Gv_SetVarX(nGameVar, prevspritesect[nSprite]);
                continue;
            }

        case CON_NEXTSPRITESECT:
            insptr++;
            {
                int const nGameVar = *insptr++;
                int const nSprite = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)nSprite >= MAXSPRITES))
                {
                    CON_ERRPRINTF("invalid sprite ID %d\n", nSprite);
                    continue;
                }

                Gv_SetVarX(nGameVar, nextspritesect[nSprite]);
                continue;
            }

        case CON_GETKEYNAME:
            insptr++;
            {
                int const nQuoteIndex = Gv_GetVarX(*insptr++);
                int const nGameFunc = Gv_GetVarX(*insptr++);
                int const nFuncPos = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)nQuoteIndex >= MAXQUOTES || ScriptQuotes[nQuoteIndex] == NULL))
                {
                    CON_ERRPRINTF("invalid quote ID %d\n", nQuoteIndex);
                    continue;
                }
                else if (EDUKE32_PREDICT_FALSE((unsigned)nGameFunc >= NUMGAMEFUNCTIONS))
                {
                    CON_ERRPRINTF("invalid function %d\n", nGameFunc);
                    continue;
                }
                else
                {
                    if (nFuncPos < 2)
                        Bstrcpy(tempbuf, KB_ScanCodeToString(ud.config.KeyboardKeys[nGameFunc][nFuncPos]));
                    else
                    {
                        Bstrcpy(tempbuf, KB_ScanCodeToString(ud.config.KeyboardKeys[nGameFunc][0]));

                        if (!*tempbuf)
                            Bstrcpy(tempbuf, KB_ScanCodeToString(ud.config.KeyboardKeys[nGameFunc][1]));
                    }
                }

                if (*tempbuf)
                    Bstrcpy(ScriptQuotes[nQuoteIndex], tempbuf);

                continue;
            }

        case CON_QSUBSTR:
            insptr++;
            {
                int32_t params[4];

                Gv_GetManyVars(4, params);

                const int nDestQuote = params[0], nSrcQuote = params[1];

                if (EDUKE32_PREDICT_FALSE((unsigned)nDestQuote>=MAXQUOTES || ScriptQuotes[nDestQuote] == NULL))
                {
                    CON_ERRPRINTF("invalid quote ID %d\n", nDestQuote);
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE((unsigned)nSrcQuote>=MAXQUOTES || ScriptQuotes[nSrcQuote] == NULL))
                {
                    CON_ERRPRINTF("invalid quote ID %d\n", nSrcQuote);
                    continue;
                }

                int nPos = params[2], nLength = params[3];

                if (EDUKE32_PREDICT_FALSE((unsigned)nPos >= MAXQUOTELEN))
                {
                    CON_ERRPRINTF("invalid start position %d\n", nPos);
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE(nLength < 0))
                {
                    CON_ERRPRINTF("invalid length %d\n", nLength);
                    continue;
                }

                char *pDestQuote = ScriptQuotes[nDestQuote];
                char const * pSrcQuote = ScriptQuotes[nSrcQuote];

                while (*pSrcQuote && nPos--) pSrcQuote++;
                while ((*pDestQuote = *pSrcQuote) && nLength--)
                {
                    pDestQuote++;
                    pSrcQuote++;
                }
                *pDestQuote = 0;

                continue;
            }

        case CON_GETPNAME:
        case CON_QSTRNCAT:
        case CON_QSTRCAT:
        case CON_QSTRCPY:
        case CON_QGETSYSSTR:
        case CON_CHANGESPRITESECT:
            insptr++;
            {
                int32_t i = Gv_GetVarX(*insptr++), j;
                if (tw == CON_GETPNAME && *insptr == g_iThisActorID)
                {
                    j = vm.g_p;
                    insptr++;
                }
                else j = Gv_GetVarX(*insptr++);

                switch (tw)
                {
                case CON_GETPNAME:
                    if (EDUKE32_PREDICT_FALSE((unsigned)i>=MAXQUOTES || ScriptQuotes[i] == NULL))
                    {
                        CON_ERRPRINTF("invalid quote ID %d\n", i);
                        break;
                    }
                    if (g_player[j].user_name[0])
                        Bstrcpy(ScriptQuotes[i],g_player[j].user_name);
                    else Bsprintf(ScriptQuotes[i],"%d",j);
                    break;
                case CON_QGETSYSSTR:
                    if (EDUKE32_PREDICT_FALSE((unsigned)i>=MAXQUOTES || ScriptQuotes[i] == NULL))
                    {
                        CON_ERRPRINTF("invalid quote ID %d\n", i);
                        break;
                    }
                    switch (j)
                    {
                    case STR_MAPNAME:
                    case STR_MAPFILENAME:
                    {
                        int32_t idx = ud.volume_number*MAXLEVELS + ud.level_number;
                        const char *src;

                        if (EDUKE32_PREDICT_FALSE((unsigned)idx >= ARRAY_SIZE(MapInfo)))
                        {
                            CON_ERRPRINTF("out of bounds map number (vol=%d, lev=%d)\n",
                                          ud.volume_number, ud.level_number);
                            break;
                        }

                        src = j==STR_MAPNAME ? MapInfo[idx].name : MapInfo[idx].filename;
                        if (EDUKE32_PREDICT_FALSE(src == NULL))
                        {
                            CON_ERRPRINTF("attempted access to %s of non-existent map (vol=%d, lev=%d)",
                                          j==STR_MAPNAME ? "name" : "file name",
                                          ud.volume_number, ud.level_number);
                            break;
                        }

                        Bstrcpy(ScriptQuotes[i], j==STR_MAPNAME ? MapInfo[idx].name : MapInfo[idx].filename);
                        break;
                    }
                    case STR_PLAYERNAME:
                        if (EDUKE32_PREDICT_FALSE((unsigned)vm.g_p >= (unsigned)playerswhenstarted))
                        {
                            CON_ERRPRINTF("Invalid player ID %d\n", vm.g_p);
                            break;
                        }
                        Bstrcpy(ScriptQuotes[i],g_player[vm.g_p].user_name);
                        break;
                    case STR_VERSION:
                        Bsprintf(tempbuf,HEAD2 " %s",s_buildRev);
                        Bstrcpy(ScriptQuotes[i],tempbuf);
                        break;
                    case STR_GAMETYPE:
                        Bstrcpy(ScriptQuotes[i],GametypeNames[ud.coop]);
                        break;
                    case STR_VOLUMENAME:
                        if (EDUKE32_PREDICT_FALSE((unsigned)ud.volume_number >= MAXVOLUMES))
                        {
                            CON_ERRPRINTF("invalid volume (%d)\n", ud.volume_number);
                            break;
                        }
                        Bstrcpy(ScriptQuotes[i],EpisodeNames[ud.volume_number]);
                        break;
                    case STR_YOURTIME:
                        Bstrcpy(ScriptQuotes[i],G_PrintYourTime());
                        break;
                    case STR_PARTIME:
                        Bstrcpy(ScriptQuotes[i],G_PrintParTime());
                        break;
                    case STR_DESIGNERTIME:
                        Bstrcpy(ScriptQuotes[i],G_PrintDesignerTime());
                        break;
                    case STR_BESTTIME:
                        Bstrcpy(ScriptQuotes[i],G_PrintBestTime());
                        break;
                    default:
                        CON_ERRPRINTF("unknown str ID %d %d\n", i,j);
                    }
                    break;
                case CON_QSTRCAT:
                    if (EDUKE32_PREDICT_FALSE(ScriptQuotes[i] == NULL || ScriptQuotes[j] == NULL)) goto nullquote;
                    Bstrncat(ScriptQuotes[i],ScriptQuotes[j],(MAXQUOTELEN-1)-Bstrlen(ScriptQuotes[i]));
                    break;
                case CON_QSTRNCAT:
                    if (EDUKE32_PREDICT_FALSE(ScriptQuotes[i] == NULL || ScriptQuotes[j] == NULL)) goto nullquote;
                    Bstrncat(ScriptQuotes[i],ScriptQuotes[j],Gv_GetVarX(*insptr++));
                    break;
                case CON_QSTRCPY:
                    if (EDUKE32_PREDICT_FALSE(ScriptQuotes[i] == NULL || ScriptQuotes[j] == NULL)) goto nullquote;
                    if (i != j)
                        Bstrcpy(ScriptQuotes[i],ScriptQuotes[j]);
                    break;
                case CON_CHANGESPRITESECT:
                    if (EDUKE32_PREDICT_FALSE((unsigned)i >= MAXSPRITES))
                    {
                        CON_ERRPRINTF("Invalid sprite %d\n", i);
                        break;
                    }
                    else if (EDUKE32_PREDICT_FALSE((unsigned)j >= (unsigned)numsectors))
                    {
                        CON_ERRPRINTF("Invalid sector %d\n", j);
                        break;
                    }
                    changespritesect(i,j);
                    break;
                default:
nullquote:
                    CON_ERRPRINTF("null quote %d\n", ScriptQuotes[i] ? j : i);
                    break;
                }
                continue;
            }

        case CON_CHANGESPRITESTAT:
            insptr++;
            {
                int32_t const nSprite = Gv_GetVarX(*insptr++);
                int32_t nStatnum = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)nSprite >= MAXSPRITES))
                {
                    CON_ERRPRINTF("Invalid sprite: %d\n", nSprite);
                    continue;
                }
                if (EDUKE32_PREDICT_FALSE((unsigned)nStatnum >= MAXSTATUS))
                {
                    CON_ERRPRINTF("Invalid statnum: %d\n", nStatnum);
                    continue;
                }
                if (sprite[nSprite].statnum == nStatnum)
                    continue;

                /* initialize actor data when changing to an actor statnum because there's usually
                garbage left over from being handled as a hard coded object */

                if (sprite[nSprite].statnum > STAT_ZOMBIEACTOR && (nStatnum == STAT_ACTOR || nStatnum == STAT_ZOMBIEACTOR))
                {
                    actor_t * const a = &actor[nSprite];

                    a->lastvx = 0;
                    a->lastvy = 0;
                    a->timetosleep = 0;
                    a->cgg = 0;
                    a->movflag = 0;
                    a->tempang = 0;
                    a->dispicnum = 0;
                    Bmemset(&a->t_data, 0, sizeof a->t_data);
                    a->flags = 0;
                    sprite[nSprite].hitag = 0;

                    if (G_HaveActor(sprite[nSprite].picnum))
                    {
                        const intptr_t *actorptr = g_tile[sprite[nSprite].picnum].execPtr;
                        // offsets
                        AC_ACTION_ID(a->t_data) = actorptr[1];
                        AC_MOVE_ID(a->t_data) = actorptr[2];
                        AC_MOVFLAGS(&sprite[nSprite], &actor[nSprite]) = actorptr[3];  // ai bits (movflags)
                    }
                }

                changespritestat(nSprite, nStatnum);
                continue;
            }

        case CON_STARTLEVEL:
            insptr++; // skip command
            {
                // from 'level' cheat in game.c (about line 6250)
                int const nVolume = Gv_GetVarX(*insptr++);
                int const nLevel = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)nVolume >= MAXVOLUMES))
                {
                    CON_ERRPRINTF("invalid volume (%d)\n", nVolume);
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE((unsigned)nLevel >= MAXLEVELS))
                {
                    CON_ERRPRINTF("invalid level (%d)\n", nLevel);
                    continue;
                }

                ud.m_volume_number = ud.volume_number = nVolume;
                ud.m_level_number = ud.level_number = nLevel;
                //if (numplayers > 1 && g_netServer)
                //    Net_NewGame(volnume,levnume);
                //else
                {
                    g_player[myconnectindex].ps->gm |= MODE_EOL;
                    ud.display_bonus_screen = 0;
                } // MODE_RESTART;

                continue;
            }

        case CON_MYOSX:
        case CON_MYOSPALX:
        case CON_MYOS:
        case CON_MYOSPAL:
            insptr++;
            {
                int32_t values[5];
                Gv_GetManyVars(5, values);

                vec2_t const pos = *(vec2_t *)values;
                int const tilenum = values[2];
                int const shade = values[3];
                int const orientation = values[4];

                switch (tw)
                {
                    case CON_MYOS:
                        VM_DrawTile(pos.x, pos.y, tilenum, shade, orientation);
                        break;
                    case CON_MYOSPAL:
                        VM_DrawTilePal(pos.x, pos.y, tilenum, shade, orientation, Gv_GetVarX(*insptr++));
                        break;
                    case CON_MYOSX:
                        VM_DrawTileSmall(pos.x, pos.y, tilenum, shade, orientation);
                        break;
                    case CON_MYOSPALX:
                        VM_DrawTilePalSmall(pos.x, pos.y, tilenum, shade, orientation, Gv_GetVarX(*insptr++));
                        break;
                }
                continue;
            }

        case CON_SWITCH:
            insptr++;
            {
                // command format:
                // variable ID to check
                // script offset to 'end'
                // count of case statements
                // script offset to default case (null if none)
                // For each case: value, ptr to code
                int32_t const lValue = Gv_GetVarX(*insptr++), lEnd = *insptr++, lCases = *insptr++;
                intptr_t const * const lpDefault = insptr++;
                intptr_t const * const lpCases = insptr;
                int32_t left = 0, right = lCases - 1;
                insptr += lCases << 1;

                do
                {
                    int const lCheckCase = (left + right) >> 1;

                    if (lpCases[lCheckCase << 1] > lValue)
                        right = lCheckCase - 1;
                    else if (lpCases[lCheckCase << 1] < lValue)
                        left = lCheckCase + 1;
                    else if (lpCases[lCheckCase << 1] == lValue)
                    {
                        // fake a 2-d Array
                        insptr = (intptr_t *)(lpCases[(lCheckCase << 1) + 1] + &script[0]);
                        VM_Execute(1);
                        goto matched;
                    }

                    if (right - left < 0)
                        break;
                }
                while (1);

                if (*lpDefault)
                {
                    insptr = (intptr_t *)(*lpDefault + &script[0]);
                    VM_Execute(1);
                }

            matched:
                insptr = (intptr_t *)(lEnd + (intptr_t)&script[0]);

                continue;
            }

        case CON_ENDSWITCH:
            insptr++;
        case CON_ENDEVENT:
            return;

        case CON_DISPLAYRAND:
            insptr++;
            Gv_SetVarX(*insptr++, system_15bit_rand());
            continue;

        case CON_DRAGPOINT:
            insptr++;
            {
                int const nWall = Gv_GetVarX(*insptr++);
                vec2_t n;

                Gv_GetManyVars(2, (int32_t *)&n);

                if (EDUKE32_PREDICT_FALSE((unsigned)nWall >= (unsigned)numwalls))
                {
                    CON_ERRPRINTF("Invalid wall %d\n", nWall);
                    continue;
                }

                dragpoint(nWall, n.x, n.y, 0);
                continue;
            }

        case CON_LDIST:
            insptr++;
            {
                int const out = *insptr++;
                vec2_t in;

                Gv_GetManyVars(2, (int32_t *) &in);

                if (EDUKE32_PREDICT_FALSE((unsigned)in.x >= MAXSPRITES || (unsigned)in.y >= MAXSPRITES))
                {
                    CON_ERRPRINTF("invalid sprite %d %d\n", in.x, in.y);
                    continue;
                }

                Gv_SetVarX(out, ldist(&sprite[in.x], &sprite[in.y]));
                continue;
            }

        case CON_DIST:
            insptr++;
            {
                int const out = *insptr++;
                vec2_t in;

                Gv_GetManyVars(2, (int32_t *) &in);

                if (EDUKE32_PREDICT_FALSE((unsigned)in.x >= MAXSPRITES || (unsigned)in.y >= MAXSPRITES))
                {
                    CON_ERRPRINTF("invalid sprite %d %d\n", in.x, in.y);
                    continue;
                }

                Gv_SetVarX(out, dist(&sprite[in.x], &sprite[in.y]));
                continue;
            }

        case CON_GETANGLE:
            insptr++;
            {
                int const out = *insptr++;
                vec2_t in;

                Gv_GetManyVars(2, (int32_t *)&in);
                Gv_SetVarX(out, getangle(in.x, in.y));
                continue;
            }

        case CON_GETINCANGLE:
            insptr++;
            {
                int const out = *insptr++;
                vec2_t in;

                Gv_GetManyVars(2, (int32_t *)&in);
                Gv_SetVarX(out, G_GetAngleDelta(in.x, in.y));
                continue;
            }

        case CON_MULSCALE:
            insptr++;
            {
                int const out = *insptr++;
                vec3_t in;

                Gv_GetManyVars(3, (int32_t *)&in);
                Gv_SetVarX(out, mulscale(in.x, in.y, in.z));
                continue;
            }

        case CON_INITTIMER:
            insptr++;
            G_InitTimer(Gv_GetVarX(*insptr++));
            continue;

        case CON_NEXTSECTORNEIGHBORZ:
            insptr++;
            {
                int32_t params[4];
                Gv_GetManyVars(4, params);
                aGameVars[g_iReturnVarID].nValue = nextsectorneighborz(params[0], params[1], params[2], params[3]);
            }
            continue;

        case CON_MOVESECTOR:
            insptr++;
            A_MoveSector(Gv_GetVarX(*insptr++));
            continue;

        case CON_TIME:
            insptr += 2;
            continue;

        case CON_ESPAWNVAR:
        case CON_EQSPAWNVAR:
        case CON_QSPAWNVAR:
            insptr++;
            {
                int const nPicnum = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)vm.g_sp->sectnum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector %d\n", TrackerCast(vm.g_sp->sectnum));
                    continue;
                }

                int const nSprite = A_Spawn(vm.g_i, nPicnum);

                switch (tw)
                {
                    case CON_EQSPAWNVAR:
                        if (nSprite != -1)
                            A_AddToDeleteQueue(nSprite);
                    case CON_ESPAWNVAR:
                        aGameVars[g_iReturnVarID].nValue = nSprite;
                        break;
                    case CON_QSPAWNVAR:
                        if (nSprite != -1)
                            A_AddToDeleteQueue(nSprite);
                        break;
                }
                continue;
            }

        case CON_ESPAWN:
        case CON_EQSPAWN:
        case CON_QSPAWN:
            insptr++;

            {
                if (EDUKE32_PREDICT_FALSE((unsigned)vm.g_sp->sectnum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector %d\n", TrackerCast(vm.g_sp->sectnum));
                    insptr++;
                    continue;
                }

                int const nSprite = A_Spawn(vm.g_i,*insptr++);

                switch (tw)
                {
                    case CON_EQSPAWN:
                        if (nSprite != -1)
                            A_AddToDeleteQueue(nSprite);
                    case CON_ESPAWN:
                        aGameVars[g_iReturnVarID].nValue = nSprite;
                        break;
                    case CON_QSPAWN:
                        if (nSprite != -1)
                            A_AddToDeleteQueue(nSprite);
                        break;
                }
            }
            continue;

        case CON_ESHOOT:
        case CON_EZSHOOT:
        case CON_ZSHOOT:
            insptr++;
            {
                // NOTE: (int16_t) cast because we want to exclude that
                // SHOOT_HARDCODED_ZVEL is passed.
                int const zvel = (tw == CON_ESHOOT) ?
                    SHOOT_HARDCODED_ZVEL : (int16_t)Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)vm.g_sp->sectnum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector %d\n", TrackerCast(vm.g_sp->sectnum));
                    insptr++;
                    continue;
                }

                int const nSprite = A_ShootWithZvel(vm.g_i,*insptr++,zvel);

                if (tw != CON_ZSHOOT)
                    aGameVars[g_iReturnVarID].nValue = nSprite;
            }
            continue;

        case CON_SHOOTVAR:
        case CON_ESHOOTVAR:
            insptr++;
            {
                int j = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)vm.g_sp->sectnum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector %d\n", TrackerCast(vm.g_sp->sectnum));
                    continue;
                }

                j = A_Shoot(vm.g_i, j);

                if (tw == CON_ESHOOTVAR)
                    aGameVars[g_iReturnVarID].nValue = j;

                continue;
            }

        case CON_EZSHOOTVAR:
        case CON_ZSHOOTVAR:
            insptr++;
            {
                int const zvel = (int16_t)Gv_GetVarX(*insptr++);
                int j = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)vm.g_sp->sectnum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector %d\n", TrackerCast(vm.g_sp->sectnum));
                    continue;
                }

                j = A_ShootWithZvel(vm.g_i, j, zvel);

                if (tw == CON_EZSHOOTVAR)
                    aGameVars[g_iReturnVarID].nValue = j;

                continue;
            }

        case CON_CMENU:
            insptr++;
            M_ChangeMenu(Gv_GetVarX(*insptr++));
            continue;

        case CON_SOUNDVAR:
        case CON_STOPSOUNDVAR:
        case CON_SOUNDONCEVAR:
        case CON_GLOBALSOUNDVAR:
        case CON_SCREENSOUND:
            insptr++;
            {
                int const nSound = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)nSound>=MAXSOUNDS))
                {
                    CON_ERRPRINTF("Invalid sound %d\n", nSound);
                    continue;
                }

                switch (tw)
                {
                    case CON_SOUNDONCEVAR: // falls through to CON_SOUNDVAR
                        if (!S_CheckSoundPlaying(vm.g_i, nSound))
                    case CON_SOUNDVAR:
                        A_PlaySound((int16_t)nSound, vm.g_i);
                        continue;
                    case CON_GLOBALSOUNDVAR:
                        A_PlaySound((int16_t)nSound, g_player[screenpeek].ps->i);
                        continue;
                    case CON_STOPSOUNDVAR:
                        if (S_CheckSoundPlaying(vm.g_i, nSound))
                            S_StopSound((int16_t)nSound);
                        continue;
                    case CON_SCREENSOUND:
                        A_PlaySound(nSound, -1);
                        continue;
                }
            }
            continue;

        case CON_STARTCUTSCENE:
        case CON_IFCUTSCENE:
            insptr++;
            {
                int const nQuote = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)nQuote >= MAXQUOTES || ScriptQuotes[nQuote] == NULL))
                {
                    CON_ERRPRINTF("invalid quote ID %d for anim!\n", nQuote);
                    continue;
                }

                if (tw == CON_IFCUTSCENE)
                {
                    insptr--;
                    VM_CONDITIONAL(g_animPtr == Anim_Find(ScriptQuotes[nQuote]));
                    continue;
                }

                tw = ps->palette;
                Anim_Play(ScriptQuotes[nQuote]);
                P_SetGamePalette(ps, tw, 2 + 16);
                continue;
            }
            continue;

        case CON_GUNIQHUDID:
            insptr++;
            {
                tw = Gv_GetVarX(*insptr++);
                if (EDUKE32_PREDICT_FALSE((unsigned)tw >= MAXUNIQHUDID - 1))
                    CON_ERRPRINTF("Invalid ID %d\n", tw);
                else
                    guniqhudid = tw;

                continue;
            }

        case CON_SAVEGAMEVAR:
        case CON_READGAMEVAR:
        {
            int32_t nValue = 0;
            insptr++;
            if (ud.config.scripthandle < 0)
            {
                insptr++;
                continue;
            }
            switch (tw)
            {
                case CON_SAVEGAMEVAR:
                    nValue = Gv_GetVarX(*insptr);
                    SCRIPT_PutNumber(ud.config.scripthandle, "Gamevars", aGameVars[*insptr++].szLabel, nValue, FALSE, FALSE);
                    break;
                case CON_READGAMEVAR:
                    SCRIPT_GetNumber(ud.config.scripthandle, "Gamevars", aGameVars[*insptr].szLabel, &nValue);
                    Gv_SetVarX(*insptr++, nValue);
                    break;
            }
            continue;
        }

        case CON_SHOWVIEW:
        case CON_SHOWVIEWUNBIASED:
            insptr++;
            {
                vec3_t vec;
                Gv_GetManyVars(3, (int32_t *)&vec);

                int32_t params[3];
                Gv_GetManyVars(3, params);

                vec2_t scrn[2];
                Gv_GetManyVars(4, (int32_t *)scrn);

                if (EDUKE32_PREDICT_FALSE(scrn[0].x < 0 || scrn[0].y < 0 || scrn[1].x >= 320 || scrn[1].y >= 200))
                {
                    CON_ERRPRINTF("incorrect coordinates\n");
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE((unsigned)params[2] >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector %d\n", params[2]);
                    continue;
                }

                G_ShowView(vec, params[0], params[1], params[2], scrn[0].x, scrn[0].y, scrn[1].x, scrn[1].y, (tw != CON_SHOWVIEW));

                continue;
            }

        case CON_ROTATESPRITEA:
        case CON_ROTATESPRITE16:
        case CON_ROTATESPRITE:
            insptr++;
            {
                int32_t params[8];

                Gv_GetManyVars(8, params);

                vec3_t    pos         = *(vec3_t *)params;
                int const ang         = params[3];
                int const tilenum     = params[4];
                int const shade       = params[5];
                int const pal         = params[6];
                int       orientation = params[7] & (ROTATESPRITE_MAX - 1);

                int32_t alpha = (tw == CON_ROTATESPRITEA) ? Gv_GetVarX(*insptr++) : 0;

                vec2_t scrn[2];
                Gv_GetManyVars(4, (int32_t *) scrn);

                if (tw != CON_ROTATESPRITE16 && !(orientation&ROTATESPRITE_FULL16))
                {
                    pos.x <<= 16;
                    pos.y <<= 16;
                }

                if (EDUKE32_PREDICT_FALSE((unsigned) tilenum >= MAXTILES))
                {
                    CON_ERRPRINTF("invalid tilenum %d\n", tilenum);
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE(pos.x < -(320<<16) || pos.x >= (640<<16) || pos.y < -(200<<16) || pos.y >= (400<<16)))
                {
                    CON_ERRPRINTF("invalid coordinates: %d, %d\n", pos.x, pos.y);
                    continue;
                }

                int32_t blendidx = 0;

                NEG_ALPHA_TO_BLEND(alpha, blendidx, orientation);

                rotatesprite_(pos.x, pos.y, pos.z, ang, tilenum, shade, pal, 2 | orientation, alpha, blendidx,
                              scrn[0].x, scrn[0].y, scrn[1].x, scrn[1].y);
                continue;
            }

        case CON_GAMETEXT:
        case CON_GAMETEXTZ:
            insptr++;
            {
                int32_t params[11];
                Gv_GetManyVars(11, params);

                int const    tilenum     = params[0];
                vec2_t const pos         = { params[1], params[2] };
                int const    nQuote      = params[3];
                int const    shade       = params[4];
                int const    pal         = params[5];
                int const    orientation = params[6] & (ROTATESPRITE_MAX - 1);
                vec2_t const bound1      = *(vec2_t *)&params[7];
                vec2_t const bound2      = *(vec2_t *)&params[9];
                int32_t      z           = (tw == CON_GAMETEXTZ) ? Gv_GetVarX(*insptr++) : 65536;

                if (EDUKE32_PREDICT_FALSE(tilenum < 0 || tilenum + 255 >= MAXTILES))
                {
                    CON_ERRPRINTF("invalid base tilenum %d\n", tilenum);
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE((unsigned)nQuote >= MAXQUOTES || ScriptQuotes[nQuote] == NULL))
                {
                    CON_ERRPRINTF("invalid quote ID %d\n", nQuote);
                    continue;
                }

                G_PrintGameText(0, tilenum, pos.x >> 1, pos.y, ScriptQuotes[nQuote], shade, pal, orientation, bound1.x, bound1.y, bound2.x, bound2.y, z, 0);
                continue;
            }

        case CON_DIGITALNUMBER:
        case CON_DIGITALNUMBERZ:
            insptr++;
            {
                int32_t params[11];
                Gv_GetManyVars(11, params);

                int const    tilenum     = params[0];
                vec2_t const pos         = { params[1], params[2] };
                int const    q           = params[3];
                int const    shade       = params[4];
                int const    pal         = params[5];
                int const    orientation = params[6] & (ROTATESPRITE_MAX - 1);
                vec2_t const bound1      = *(vec2_t *)&params[7];
                vec2_t const bound2      = *(vec2_t *)&params[9];
                int32_t      nZoom       = (tw == CON_DIGITALNUMBERZ) ? Gv_GetVarX(*insptr++) : 65536;

                // NOTE: '-' not taken into account, but we have rotatesprite() bound check now anyway
                if (EDUKE32_PREDICT_FALSE(tilenum < 0 || tilenum+9 >= MAXTILES))
                {
                    CON_ERRPRINTF("invalid base tilenum %d\n", tilenum);
                    continue;
                }

                G_DrawTXDigiNumZ(tilenum, pos.x, pos.y, q, shade, pal, orientation, bound1.x, bound1.y, bound2.x, bound2.y, nZoom);
                continue;
            }

        case CON_MINITEXT:
            insptr++;
            {
                int32_t params[5];
                Gv_GetManyVars(5, params);

                vec2_t const pos = { params[0], params[1] };
                int const nQuote = params[2];
                int const shade = params[3], pal = params[4];

                if (EDUKE32_PREDICT_FALSE((unsigned)nQuote >= MAXQUOTES || ScriptQuotes[nQuote] == NULL))
                {
                    CON_ERRPRINTF("invalid quote ID %d\n", nQuote);
                    continue;
                }

                minitextshade(pos.x, pos.y, ScriptQuotes[nQuote], shade, pal, 2+8+16);
                continue;
            }

        case CON_SCREENTEXT:
            insptr++;
            {
                int32_t params[20];
                Gv_GetManyVars(20, params);

                int const    tilenum     = params[0];
                vec3_t const v           = *(vec3_t *)&params[1];
                int const    blockangle  = params[4];
                int const    charangle   = params[5];
                int const    nQuote      = params[6];
                int const    shade       = params[7];
                int const    pal         = params[8];
                int const    orientation = params[9] & (ROTATESPRITE_MAX - 1);
                int const    alpha       = params[10];
                vec2_t const spacing     = { params[11], params[12] };
                vec2_t const between     = { params[13], params[14] };
                int const    f           = params[15];
                vec2_t const scrn[2]     = { *(vec2_t *)&params[16], *(vec2_t *)&params[18] };

                if (EDUKE32_PREDICT_FALSE(tilenum < 0 || tilenum+255 >= MAXTILES))
                {
                    CON_ERRPRINTF("invalid base tilenum %d\n", tilenum);
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE((unsigned)nQuote >= MAXQUOTES || ScriptQuotes[nQuote] == NULL))
                {
                    CON_ERRPRINTF("invalid quote ID %d\n", nQuote);
                    continue;
                }

                G_ScreenText(tilenum, v.x, v.y, v.z, blockangle, charangle, ScriptQuotes[nQuote], shade, pal, 2 | orientation,
                             alpha, spacing.x, spacing.y, between.x, between.y, f, scrn[0].x, scrn[0].y, scrn[1].x, scrn[1].y);
                continue;
            }

        case CON_ANGOFF:
            insptr++;
            spriteext[vm.g_i].angoff=*insptr++;
            continue;

        case CON_GETZRANGE:
            insptr++;
            {
                vec3_t vect;
                Gv_GetManyVars(3, (int32_t *)&vect);

                int const sectnum    = Gv_GetVarX(*insptr++);
                int const ceilzvar   = *insptr++;
                int const ceilhitvar = *insptr++;
                int const florzvar   = *insptr++;
                int const florhitvar = *insptr++;
                int const walldist   = Gv_GetVarX(*insptr++);
                int const clipmask   = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)sectnum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector %d\n", sectnum);
                    continue;
                }

                int32_t ceilz, ceilhit, florz, florhit;

                getzrange(&vect, sectnum, &ceilz, &ceilhit, &florz, &florhit, walldist, clipmask);
                Gv_SetVarX(ceilzvar, ceilz);
                Gv_SetVarX(ceilhitvar, ceilhit);
                Gv_SetVarX(florzvar, florz);
                Gv_SetVarX(florhitvar, florhit);

                continue;
            }

        case CON_SECTSETINTERPOLATION:
        case CON_SECTCLEARINTERPOLATION:
            insptr++;
            {
                int const sectnum = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)sectnum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector %d\n", sectnum);
                    continue;
                }

                if (tw==CON_SECTSETINTERPOLATION)
                    Sect_SetInterpolation(sectnum);
                else
                    Sect_ClearInterpolation(sectnum);

                continue;
            }

        case CON_CALCHYPOTENUSE:
            insptr++;
            {
                int32_t retvar=*insptr++;
                vec2_t da;
                Gv_GetManyVars(2, (int32_t *)&da);
                int64_t const hypsq = (int64_t)da.x*da.x + (int64_t)da.y*da.y;

                if (hypsq > (int64_t)INT32_MAX)
                    Gv_SetVarX(retvar, (int32_t)sqrt((double)hypsq));
                else
                    Gv_SetVarX(retvar, ksqrt((uint32_t)hypsq));

                continue;
            }

        case CON_LINEINTERSECT:
        case CON_RAYINTERSECT:
            insptr++;
            {
                vec3_t vec[2];
                Gv_GetManyVars(6, (int32_t *)vec);

                vec2_t vec2[2];
                Gv_GetManyVars(4, (int32_t *)vec2);

                int const intxvar = *insptr++;
                int const intyvar = *insptr++;
                int const intzvar = *insptr++;
                int const retvar  = *insptr++;
                vec3_t    in;
                int       ret = (tw == CON_LINEINTERSECT)
                          ? lintersect(vec[0].x, vec[0].y, vec[0].z, vec[1].x, vec[1].y, vec[1].z, vec2[0].x, vec2[0].y,
                                       vec2[1].x, vec2[1].y, &in.x, &in.y, &in.z)
                          : rayintersect(vec[0].x, vec[0].y, vec[0].z, vec[1].x, vec[1].y, vec[1].z, vec2[0].x, vec2[0].y,
                                         vec2[1].x, vec2[1].y, &in.x, &in.y, &in.z);

                Gv_SetVarX(retvar, ret);

                if (ret)
                {
                    Gv_SetVarX(intxvar, in.x);
                    Gv_SetVarX(intyvar, in.y);
                    Gv_SetVarX(intzvar, in.z);
                }

                continue;
            }

        case CON_CLIPMOVE:
        case CON_CLIPMOVENOSLIDE:
            insptr++;
            {
                typedef struct {
                    int32_t w, f, c;
                } vec3dist_t;

                int const retvar = *insptr++;
                int const xvar   = *insptr++;
                int const yvar   = *insptr++;

                insptr -= 2;

                vec3_t vec3;
                Gv_GetManyVars(3, (int32_t *)&vec3);

                int const sectnumvar = *insptr++;

                vec2_t vec2;
                Gv_GetManyVars(2, (int32_t *)&vec2);

                vec3dist_t dist;
                Gv_GetManyVars(3, (int32_t *)&dist);

                int const clipmask = Gv_GetVarX(*insptr++);
                int16_t   sectnum  = Gv_GetVarX(sectnumvar);

                if (EDUKE32_PREDICT_FALSE((unsigned)sectnum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector %d\n", sectnum);
                    Gv_SetVarX(retvar, 0);
                    continue;
                }

                Gv_SetVarX(retvar, clipmovex(&vec3, &sectnum, vec2.x, vec2.y, dist.w, dist.f, dist.c, clipmask, (tw == CON_CLIPMOVENOSLIDE)));
                Gv_SetVarX(sectnumvar, sectnum);
                Gv_SetVarX(xvar, vec3.x);
                Gv_SetVarX(yvar, vec3.y);

                continue;
            }

        case CON_HITSCAN:
            insptr++;
            {
                vec3_t vect;
                Gv_GetManyVars(3, (int32_t *)&vect);

                int const sectnum = Gv_GetVarX(*insptr++);

                vec3_t v;
                Gv_GetManyVars(3, (int32_t *) &v);

                int const hitsectvar   = *insptr++;
                int const hitwallvar   = *insptr++;
                int const hitspritevar = *insptr++;
                int const hitxvar      = *insptr++;
                int const hityvar      = *insptr++;
                int const hitzvar      = *insptr++;
                int const cliptype     = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)sectnum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector %d\n", sectnum);
                    continue;
                }

                hitdata_t hit;
                hitscan((const vec3_t *)&vect, sectnum, v.x, v.y, v.z, &hit, cliptype);

                Gv_SetVarX(hitsectvar, hit.sect);
                Gv_SetVarX(hitwallvar, hit.wall);
                Gv_SetVarX(hitspritevar, hit.sprite);
                Gv_SetVarX(hitxvar, hit.pos.x);
                Gv_SetVarX(hityvar, hit.pos.y);
                Gv_SetVarX(hitzvar, hit.pos.z);
                continue;
            }

        case CON_CANSEE:
            insptr++;
            {
                vec3_t vec1;
                Gv_GetManyVars(3, (int32_t *) &vec1);

                int const sect1 = Gv_GetVarX(*insptr++);

                vec3_t vec2;
                Gv_GetManyVars(3, (int32_t *) &vec2);

                int const sect2 = Gv_GetVarX(*insptr++), rvar = *insptr++;

                if (EDUKE32_PREDICT_FALSE((unsigned)sect1 >= (unsigned)numsectors || (unsigned)sect2 >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector\n");
                    Gv_SetVarX(rvar, 0);
                }

                Gv_SetVarX(rvar, cansee(vec1.x, vec1.y, vec1.z, sect1, vec2.x, vec2.y, vec2.z, sect2));
                continue;
            }

        case CON_ROTATEPOINT:
            insptr++;
            {
                vec2_t point[2];
                Gv_GetManyVars(4, (int32_t *)point);

                int const angle = Gv_GetVarX(*insptr++);
                int const x2var = *insptr++;
                int const y2var = *insptr++;
                vec2_t    result;

                rotatepoint(point[0], point[1], angle, &result);

                Gv_SetVarX(x2var, result.x);
                Gv_SetVarX(y2var, result.y);
                continue;
            }

        case CON_NEARTAG:
            insptr++;
            {
                //             neartag(int32_t x, int32_t y, int32_t z, short sectnum, short ang,  //Starting position & angle
                //                     short *neartagsector,   //Returns near sector if sector[].tag != 0
                //                     short *neartagwall,     //Returns near wall if wall[].tag != 0
                //                     short *neartagsprite,   //Returns near sprite if sprite[].tag != 0
                //                     int32_t *neartaghitdist,   //Returns actual distance to object (scale: 1024=largest grid size)
                //                     int32_t neartagrange,      //Choose maximum distance to scan (scale: 1024=largest grid size)
                //                     char tagsearch)         //1-lotag only, 2-hitag only, 3-lotag&hitag

                vec3_t point;
                Gv_GetManyVars(3, (int32_t *)&point);
                int const sectnum           = Gv_GetVarX(*insptr++);
                int const ang               = Gv_GetVarX(*insptr++);
                int const neartagsectorvar  = *insptr++;
                int const neartagwallvar    = *insptr++;
                int const neartagspritevar  = *insptr++;
                int const neartaghitdistvar = *insptr++;
                int const neartagrange      = Gv_GetVarX(*insptr++);
                int const tagsearch         = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)sectnum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector %d\n", sectnum);
                    continue;
                }

                int16_t neartagsector, neartagwall, neartagsprite;
                int32_t neartaghitdist;

                neartag(point.x, point.y, point.z, sectnum, ang, &neartagsector, &neartagwall, &neartagsprite,
                        &neartaghitdist, neartagrange, tagsearch, NULL);

                Gv_SetVarX(neartagsectorvar, neartagsector);
                Gv_SetVarX(neartagwallvar, neartagwall);
                Gv_SetVarX(neartagspritevar, neartagsprite);
                Gv_SetVarX(neartaghitdistvar, neartaghitdist);
                continue;
            }

        case CON_GETTIMEDATE:
            insptr++;
            {
                int32_t i, vals[8];

                G_GetTimeDate(vals);

                for (i=0; i<8; i++)
                    Gv_SetVarX(*insptr++, vals[i]);

                continue;
            }

        case CON_MOVESPRITE:
            insptr++;
            {
                int const spritenum = Gv_GetVarX(*insptr++);
                vec3_t vect;

                Gv_GetManyVars(3, (int32_t *)&vect);

                int const cliptype = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)spritenum >= MAXSPRITES))
                {
                    CON_ERRPRINTF("invalid sprite ID %d\n", spritenum);
                    insptr++;
                    continue;
                }

                Gv_SetVarX(*insptr++, A_MoveSprite(spritenum, &vect, cliptype));
                continue;
            }

        case CON_SETSPRITE:
            insptr++;
            {
                int const spritenum = Gv_GetVarX(*insptr++);
                vec3_t vect;

                Gv_GetManyVars(3, (int32_t *)&vect);

                if (EDUKE32_PREDICT_FALSE((unsigned)spritenum >= MAXSPRITES))
                {
                    CON_ERRPRINTF("invalid sprite ID %d\n", spritenum);
                    continue;
                }
                setsprite(spritenum, &vect);
                continue;
            }

        case CON_GETFLORZOFSLOPE:
        case CON_GETCEILZOFSLOPE:
            insptr++;
            {
                int const sectnum = Gv_GetVarX(*insptr++);
                vec2_t    vect;
                Gv_GetManyVars(2, (int32_t *)&vect);

                if (EDUKE32_PREDICT_FALSE((unsigned)sectnum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector %d\n", sectnum);
                    insptr++;
                    continue;
                }

                Gv_SetVarX(*insptr++, (tw == CON_GETFLORZOFSLOPE) ? getflorzofslope(sectnum, vect.x, vect.y) :
                                                                    getceilzofslope(sectnum, vect.x, vect.y));

                continue;
            }

        case CON_UPDATESECTOR:
            insptr++;
            {
                vec2_t vect = { 0, 0 };
                Gv_GetManyVars(2, (int32_t *)&vect);
                int const var     = *insptr++;
                int16_t   sectnum = sprite[vm.g_i].sectnum;

                updatesector(vect.x, vect.y, &sectnum);

                Gv_SetVarX(var, sectnum);
                continue;
            }

        case CON_UPDATESECTORZ:
            insptr++;
            {
                vec3_t vect = { 0, 0, 0 };
                Gv_GetManyVars(3, (int32_t *)&vect);
                int const var     = *insptr++;
                int16_t   sectnum = sprite[vm.g_i].sectnum;

                updatesectorz(vect.x, vect.y, vect.z, &sectnum);

                Gv_SetVarX(var, sectnum);
                continue;
            }

        case CON_SPAWN:
            insptr++;
            if ((unsigned)vm.g_sp->sectnum >= MAXSECTORS)
            {
                CON_ERRPRINTF("Invalid sector %d\n", TrackerCast(vm.g_sp->sectnum));
                insptr++;
                continue;
            }
            A_Spawn(vm.g_i,*insptr++);
            continue;

        case CON_IFWASWEAPON:
            insptr++;
            VM_CONDITIONAL(actor[vm.g_i].picnum == *insptr);
            continue;

        case CON_IFAI:
            insptr++;
            VM_CONDITIONAL(AC_AI_ID(vm.g_t) == *insptr);
            continue;

        case CON_IFACTION:
            insptr++;
            VM_CONDITIONAL(AC_ACTION_ID(vm.g_t) == *insptr);
            continue;

        case CON_IFACTIONCOUNT:
            insptr++;
            VM_CONDITIONAL(AC_ACTION_COUNT(vm.g_t) >= *insptr);
            continue;

        case CON_RESETACTIONCOUNT:
            insptr++;
            AC_ACTION_COUNT(vm.g_t) = 0;
            continue;

        case CON_DEBRIS:
            insptr++;
            {
                int32_t dnum = *insptr++;
                int32_t s, l, j;

                if ((unsigned)vm.g_sp->sectnum < MAXSECTORS)
                    for (j=(*insptr)-1; j>=0; j--)
                    {
                        if (vm.g_sp->picnum == BLIMP && dnum == SCRAP1)
                            s = 0;
                        else s = (krand()%3);

                        l = A_InsertSprite(vm.g_sp->sectnum,
                                           vm.g_sp->x+(krand()&255)-128,vm.g_sp->y+(krand()&255)-128,vm.g_sp->z-(8<<8)-(krand()&8191),
                                           dnum+s,vm.g_sp->shade,32+(krand()&15),32+(krand()&15),
                                           krand()&2047,(krand()&127)+32,
                                           -(krand()&2047),vm.g_i,5);
                        if (vm.g_sp->picnum == BLIMP && dnum == SCRAP1)
                            sprite[l].yvel = BlimpSpawnSprites[j%14];
                        else sprite[l].yvel = -1;
                        sprite[l].pal = vm.g_sp->pal;
                    }
                insptr++;
            }
            continue;

        case CON_COUNT:
            insptr++;
            AC_COUNT(vm.g_t) = (int16_t) *insptr++;
            continue;

        case CON_CSTATOR:
            insptr++;
            vm.g_sp->cstat |= (int16_t) *insptr++;
            continue;

        case CON_CLIPDIST:
            insptr++;
            vm.g_sp->clipdist = (int16_t) *insptr++;
            continue;

        case CON_CSTAT:
            insptr++;
            vm.g_sp->cstat = (int16_t) *insptr++;
            continue;

        case CON_SAVENN:
        case CON_SAVE:
            insptr++;
            {
                g_lastSaveSlot = *insptr++;

                if ((unsigned)g_lastSaveSlot >= MAXSAVEGAMES)
                    continue;

                if (tw == CON_SAVE || ud.savegame[g_lastSaveSlot][0] == 0)
                {
                    time_t     curtime = time(NULL);
                    struct tm *timeptr = localtime(&curtime);

                    Bsnprintf(ud.savegame[g_lastSaveSlot], sizeof(ud.savegame[g_lastSaveSlot]),
                              "Auto %.4d%.2d%.2d %.2d%.2d%.2d\n", timeptr->tm_year + 1900, timeptr->tm_mon + 1, timeptr->tm_mday,
                              timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);
                }

                OSD_Printf("Saving to slot %d\n",g_lastSaveSlot);

                KB_FlushKeyboardQueue();

                g_screenCapture = 1;
                G_DrawRooms(myconnectindex,65536);
                g_screenCapture = 0;

                G_SavePlayerMaybeMulti(g_lastSaveSlot);

                continue;
            }

        case CON_QUAKE:
            insptr++;
            g_earthquakeTime = Gv_GetVarX(*insptr++);
            A_PlaySound(EARTHQUAKE,g_player[screenpeek].ps->i);
            continue;

        case CON_IFMOVE:
            insptr++;
            VM_CONDITIONAL(AC_MOVE_ID(vm.g_t) == *insptr);
            continue;

        case CON_RESETPLAYER:
            insptr++;
            vm.g_flags = VM_ResetPlayer(vm.g_p, vm.g_flags, 0);
            continue;

        case CON_RESETPLAYERFLAGS:
            insptr++;
            vm.g_flags = VM_ResetPlayer(vm.g_p, vm.g_flags, Gv_GetVarX(*insptr++));
            continue;

        case CON_IFONWATER:
            VM_CONDITIONAL(sector[vm.g_sp->sectnum].lotag == ST_1_ABOVE_WATER &&
                           klabs(vm.g_sp->z - sector[vm.g_sp->sectnum].floorz) < (32 << 8));
            continue;

        case CON_IFINWATER:
            VM_CONDITIONAL(sector[vm.g_sp->sectnum].lotag == ST_2_UNDERWATER);
            continue;

        case CON_IFCOUNT:
            insptr++;
            VM_CONDITIONAL(AC_COUNT(vm.g_t) >= *insptr);
            continue;

        case CON_IFACTOR:
            insptr++;
            VM_CONDITIONAL(vm.g_sp->picnum == *insptr);
            continue;

        case CON_RESETCOUNT:
            insptr++;
            AC_COUNT(vm.g_t) = 0;
            continue;

        case CON_ADDINVENTORY:
        {
            insptr += 2;

            int const item = *(insptr-1);

            switch (item)
            {
            case GET_STEROIDS:
            case GET_SCUBA:
            case GET_HOLODUKE:
            case GET_JETPACK:
            case GET_HEATS:
            case GET_FIRSTAID:
            case GET_BOOTS:
                ps->inven_icon = inv_to_icon[item];
                ps->inv_amount[item] = *insptr;
                break;

            case GET_SHIELD:
                ps->inv_amount[GET_SHIELD] = min(ps->inv_amount[GET_SHIELD] + *insptr, ps->max_shield_amount);
                break;

            case GET_ACCESS:
                switch (vm.g_sp->pal)
                {
                case  0:
                    ps->got_access |= 1;
                    break;
                case 21:
                    ps->got_access |= 2;
                    break;
                case 23:
                    ps->got_access |= 4;
                    break;
                }
                break;

            default:
                CON_ERRPRINTF("Invalid inventory ID %d\n", item);
                break;
            }
            insptr++;
            continue;
        }

        case CON_HITRADIUSVAR:
            insptr++;
            {
                int32_t params[5];
                Gv_GetManyVars(5, params);
                A_RadiusDamage(vm.g_i, params[0], params[1], params[2], params[3], params[4]);
            }
            continue;

        case CON_HITRADIUS:
            A_RadiusDamage(vm.g_i,*(insptr+1),*(insptr+2),*(insptr+3),*(insptr+4),*(insptr+5));
            insptr += 6;
            continue;

        case CON_IFP:
        {
            int const l    = *(++insptr);
            int       j    = 0;
            int const s    = sprite[ps->i].xvel;
            int const bits = g_player[vm.g_p].sync->bits;

            if (((l & pducking) && ps->on_ground && TEST_SYNC_KEY(bits, SK_CROUCH)) ||
                ((l & pfalling) && ps->jumping_counter == 0 && !ps->on_ground && ps->vel.z > 2048) ||
                ((l & pjumping) && ps->jumping_counter > 348) ||
                ((l & pstanding) && s >= 0 && s < 8) ||
                ((l & pwalking) && s >= 8 && !TEST_SYNC_KEY(bits, SK_RUN)) ||
                ((l & prunning) && s >= 8 && TEST_SYNC_KEY(bits, SK_RUN)) ||
                ((l & phigher) && ps->pos.z < (vm.g_sp->z - (48 << 8))) ||
                ((l & pwalkingback) && s <= -8 && !TEST_SYNC_KEY(bits, SK_RUN)) ||
                ((l & prunningback) && s <= -8 && TEST_SYNC_KEY(bits, SK_RUN)) ||
                ((l & pkicking) && (ps->quick_kick > 0 || (PWEAPON(vm.g_p, ps->curr_weapon, WorksLike) == KNEE_WEAPON &&
                                                           ps->kickback_pic > 0))) ||
                ((l & pshrunk) && sprite[ps->i].xrepeat < 32) ||
                ((l & pjetpack) && ps->jetpack_on) ||
                ((l & ponsteroids) && ps->inv_amount[GET_STEROIDS] > 0 && ps->inv_amount[GET_STEROIDS] < 400) ||
                ((l & ponground) && ps->on_ground) ||
                ((l & palive) && sprite[ps->i].xrepeat > 32 && sprite[ps->i].extra > 0 && ps->timebeforeexit == 0) ||
                ((l & pdead) && sprite[ps->i].extra <= 0))
                j = 1;
            else if ((l & pfacing))
            {
                if (vm.g_sp->picnum == APLAYER && (g_netServer || ud.multimode > 1))
                    j = G_GetAngleDelta(g_player[otherp].ps->ang, getangle(ps->pos.x - g_player[otherp].ps->pos.x,
                                                                           ps->pos.y - g_player[otherp].ps->pos.y));
                else
                    j = G_GetAngleDelta(ps->ang, getangle(vm.g_sp->x - ps->pos.x, vm.g_sp->y - ps->pos.y));

                j = (j > -128 && j < 128);
            }
            VM_CONDITIONAL(j);
        }
        continue;

        case CON_IFSTRENGTH:
            insptr++;
            VM_CONDITIONAL(vm.g_sp->extra <= *insptr);
            continue;

        case CON_GUTS:
            A_DoGuts(vm.g_i,*(insptr+1),*(insptr+2));
            insptr += 3;
            continue;

        case CON_IFSPAWNEDBY:
            insptr++;
            VM_CONDITIONAL(actor[vm.g_i].picnum == *insptr);
            continue;

        case CON_WACKPLAYER:
            insptr++;
            P_ForceAngle(ps);
            continue;

        case CON_FLASH:
            insptr++;
            sprite[vm.g_i].shade = -127;
            ps->visibility = -127;
            continue;

        case CON_SAVEMAPSTATE:
            G_SaveMapState();
            insptr++;
            continue;

        case CON_LOADMAPSTATE:
            G_RestoreMapState();
            insptr++;
            continue;

        case CON_CLEARMAPSTATE:
            insptr++;
            {
                int const j = Gv_GetVarX(*insptr++);
                if (EDUKE32_PREDICT_FALSE((unsigned)j >= MAXVOLUMES*MAXLEVELS))
                {
                    CON_ERRPRINTF("Invalid map number: %d\n", j);
                    continue;
                }

                G_FreeMapState(j);
            }
            continue;

        case CON_STOPALLSOUNDS:
            insptr++;
            if (screenpeek == vm.g_p)
                FX_StopAllSounds();
            continue;

        case CON_IFGAPZL:
            insptr++;
            VM_CONDITIONAL(((actor[vm.g_i].floorz - actor[vm.g_i].ceilingz) >> 8) < *insptr);
            continue;

        case CON_IFHITSPACE:
            VM_CONDITIONAL(TEST_SYNC_KEY(g_player[vm.g_p].sync->bits, SK_OPEN));
            continue;

        case CON_IFOUTSIDE:
            VM_CONDITIONAL(sector[vm.g_sp->sectnum].ceilingstat&1);
            continue;

        case CON_IFMULTIPLAYER:
            VM_CONDITIONAL((g_netServer || g_netClient || ud.multimode > 1));
            continue;

        case CON_IFCLIENT:
            VM_CONDITIONAL(g_netClient != NULL);
            continue;

        case CON_IFSERVER:
            VM_CONDITIONAL(g_netServer != NULL);
            continue;

        case CON_OPERATE:
            insptr++;
            if (sector[vm.g_sp->sectnum].lotag == 0)
            {
                int16_t neartagsector, neartagwall, neartagsprite;
                int32_t neartaghitdist;

                neartag(vm.g_sp->x,vm.g_sp->y,vm.g_sp->z-(32<<8),vm.g_sp->sectnum,vm.g_sp->ang,
                        &neartagsector,&neartagwall,&neartagsprite,&neartaghitdist, 768, 4+1, NULL);

                if (neartagsector >= 0 && isanearoperator(sector[neartagsector].lotag))
                    if ((sector[neartagsector].lotag&0xff) == ST_23_SWINGING_DOOR || sector[neartagsector].floorz == sector[neartagsector].ceilingz)
                        if ((sector[neartagsector].lotag&(16384|32768)) == 0)
                        {
                            int32_t j;

                            for (SPRITES_OF_SECT(neartagsector, j))
                                if (sprite[j].picnum == ACTIVATOR)
                                    break;

                            if (j == -1)
                                G_OperateSectors(neartagsector,vm.g_i);
                        }
            }
            continue;

        case CON_IFINSPACE:
            VM_CONDITIONAL(G_CheckForSpaceCeiling(vm.g_sp->sectnum));
            continue;

        case CON_SPRITEPAL:
            insptr++;
            if (vm.g_sp->picnum != APLAYER)
                actor[vm.g_i].tempang = vm.g_sp->pal;
            vm.g_sp->pal = *insptr++;
            continue;

        case CON_CACTOR:
            insptr++;
            vm.g_sp->picnum = *insptr++;
            continue;

        case CON_IFBULLETNEAR:
            VM_CONDITIONAL(A_Dodge(vm.g_sp) == 1);
            continue;

        case CON_IFRESPAWN:
            if (A_CheckEnemySprite(vm.g_sp)) VM_CONDITIONAL(ud.respawn_monsters)
            else if (A_CheckInventorySprite(vm.g_sp)) VM_CONDITIONAL(ud.respawn_inventory)
            else VM_CONDITIONAL(ud.respawn_items)
            continue;

        case CON_IFFLOORDISTL:
            insptr++;
            VM_CONDITIONAL((actor[vm.g_i].floorz - vm.g_sp->z) <= ((*insptr)<<8));
            continue;

        case CON_IFCEILINGDISTL:
            insptr++;
            VM_CONDITIONAL((vm.g_sp->z - actor[vm.g_i].ceilingz) <= ((*insptr)<<8));
            continue;

        case CON_PALFROM:
            insptr++;
            if (EDUKE32_PREDICT_FALSE((unsigned)vm.g_p >= (unsigned)playerswhenstarted))
            {
                CON_ERRPRINTF("invalid player ID %d\n", vm.g_p);
                insptr += 4;
            }
            else
            {
                palette_t const pal = { (uint8_t) * (insptr + 1), (uint8_t) * (insptr + 2), (uint8_t) * (insptr + 3),
                                        (uint8_t) * (insptr) };
                insptr += 4;
                P_PalFrom(ps, pal.f, pal.r, pal.g, pal.b);
            }
            continue;

        case CON_SECTOROFWALL:
            insptr++;
            tw = *insptr++;
            Gv_SetVarX(tw, sectorofwall(Gv_GetVarX(*insptr++)));
            continue;

        case CON_QSPRINTF:
            insptr++;
            {
                int const nDestQuote = Gv_GetVarX(*insptr++);
                int const nSrcQuote  = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE(ScriptQuotes[nSrcQuote] == NULL || ScriptQuotes[nDestQuote] == NULL))
                {
                    CON_ERRPRINTF("null quote %d\n", ScriptQuotes[nSrcQuote] ? nDestQuote : nSrcQuote);

                    while ((*insptr & VM_INSTMASK) != CON_NULLOP)
                        Gv_GetVarX(*insptr++);

                    insptr++; // skip the NOP
                    continue;
                }

                int32_t   arg[32], i = 0, j = 0, k = 0, numargs;
                int const nQuoteLen = Bstrlen(ScriptQuotes[nSrcQuote]);
                char      tempbuf[MAXQUOTELEN];

                while ((*insptr & VM_INSTMASK) != CON_NULLOP && i < 32)
                    arg[i++] = Gv_GetVarX(*insptr++);

                numargs = i;

                insptr++; // skip the NOP

                i = 0;

                do
                {
                    while (k < nQuoteLen && j < MAXQUOTELEN && ScriptQuotes[nSrcQuote][k] != '%')
                        tempbuf[j++] = ScriptQuotes[nSrcQuote][k++];

                    if (ScriptQuotes[nSrcQuote][k] == '%')
                    {
                        k++;
                        switch (ScriptQuotes[nSrcQuote][k])
                        {
                        case 'l':
                            if (ScriptQuotes[nSrcQuote][k+1] != 'd')
                            {
                                // write the % and l
                                tempbuf[j++] = ScriptQuotes[nSrcQuote][k-1];
                                tempbuf[j++] = ScriptQuotes[nSrcQuote][k++];
                                break;
                            }
                            k++;
                        case 'd':
                        {
                            if (i >= numargs)
                                goto finish_qsprintf;

                            char buf[16];
                            Bsprintf(buf, "%d", arg[i++]);

                            int const ii = Bstrlen(buf);
                            Bmemcpy(&tempbuf[j], buf, ii);
                            j += ii;
                            k++;
                        }
                        break;

                        case 's':
                        {
                            if (i >= numargs)
                                goto finish_qsprintf;

                            int const ii = Bstrlen(ScriptQuotes[arg[i]]);

                            Bmemcpy(&tempbuf[j], ScriptQuotes[arg[i]], ii);
                            j += ii;
                            i++;
                            k++;
                        }
                        break;

                        default:
                            tempbuf[j++] = ScriptQuotes[nSrcQuote][k-1];
                            break;
                        }
                    }
                }
                while (k < nQuoteLen && j < MAXQUOTELEN);
finish_qsprintf:
                tempbuf[j] = '\0';
                Bstrncpyz(ScriptQuotes[nDestQuote], tempbuf, MAXQUOTELEN);
                continue;
            }

        case CON_ADDLOG:
        {
            insptr++;

            OSD_Printf(OSDTEXT_GREEN "CONLOG: L=%d\n",g_errorLineNum);
            continue;
        }

        case CON_ADDLOGVAR:
            insptr++;
            {
                int32_t m=1;
                char szBuf[256];
                int32_t lVarID = *insptr;

                if ((lVarID >= g_gameVarCount) || lVarID < 0)
                {
                    if (*insptr==MAXGAMEVARS) // addlogvar for a constant?  Har.
                        insptr++;
                    //                else if (*insptr > g_gameVarCount && (*insptr < (MAXGAMEVARS<<1)+MAXGAMEVARS+1+MAXGAMEARRAYS))
                    else if (*insptr&(MAXGAMEVARS<<2))
                    {
                        int32_t index;

                        lVarID ^= (MAXGAMEVARS<<2);

                        if (lVarID&(MAXGAMEVARS<<1))
                        {
                            m = -m;
                            lVarID ^= (MAXGAMEVARS<<1);
                        }

                        insptr++;

                        index=Gv_GetVarX(*insptr++);
                        if (EDUKE32_PREDICT_TRUE((unsigned)index < (unsigned)aGameArrays[lVarID].size))
                        {
                            OSD_Printf(OSDTEXT_GREEN "%s: L=%d %s[%d] =%d\n", keyw[g_tw], g_errorLineNum,
                                       aGameArrays[lVarID].szLabel, index,
                                       (int32_t)(m*Gv_GetGameArrayValue(lVarID, index)));
                            continue;
                        }
                        else
                        {
                            CON_ERRPRINTF("invalid array index\n");
                            continue;
                        }
                    }
                    else if (*insptr&(MAXGAMEVARS<<3))
                    {
                        //                    FIXME FIXME FIXME
                        if ((lVarID & (MAXGAMEVARS-1)) == g_iStructVarIDs + STRUCT_ACTORVAR)
                        {
                            intptr_t const *oinsptr = insptr++;
                            int32_t index = Gv_GetVarX(*insptr++);
                            insptr = oinsptr;
                            if (EDUKE32_PREDICT_FALSE((unsigned)index >= MAXSPRITES-1))
                            {
                                CON_ERRPRINTF("invalid array index\n");
                                Gv_GetVarX(*insptr++);
                                continue;
                            }
                            OSD_Printf(OSDTEXT_GREEN "%s: L=%d %d %d\n",keyw[g_tw],g_errorLineNum,index,Gv_GetVar(*insptr++,index,vm.g_p));
                            continue;
                        }
                    }
                    else if (EDUKE32_PREDICT_TRUE(*insptr&(MAXGAMEVARS<<1)))
                    {
                        m = -m;
                        lVarID ^= (MAXGAMEVARS<<1);
                    }
                    else
                    {
                        // invalid varID
                        insptr++;
                        CON_ERRPRINTF("invalid variable\n");
                        continue;  // out of switch
                    }
                }
                Bsprintf(szBuf,"CONLOGVAR: L=%d %s ",g_errorLineNum, aGameVars[lVarID].szLabel);
                strcpy(g_szBuf,szBuf);

                if (aGameVars[lVarID].nFlags & GAMEVAR_READONLY)
                {
                    Bsprintf(szBuf," (read-only)");
                    strcat(g_szBuf,szBuf);
                }
                if (aGameVars[lVarID].nFlags & GAMEVAR_PERPLAYER)
                {
                    Bsprintf(szBuf," (Per Player. Player=%d)",vm.g_p);
                }
                else if (aGameVars[lVarID].nFlags & GAMEVAR_PERACTOR)
                {
                    Bsprintf(szBuf," (Per Actor. Actor=%d)",vm.g_i);
                }
                else
                {
                    Bsprintf(szBuf," (Global)");
                }
                Bstrcat(g_szBuf,szBuf);
                Bsprintf(szBuf," =%d\n", Gv_GetVarX(lVarID)*m);
                Bstrcat(g_szBuf,szBuf);
                OSD_Printf(OSDTEXT_GREEN "%s",g_szBuf);
                insptr++;
                continue;
            }

        case CON_SETSECTOR:
            insptr++;
            {
                tw = *insptr++;

                int const              nLabel  = *insptr++;
                int const              lVar2   = *insptr++;
                register int32_t const nSector = (tw != g_iThisActorID) ? Gv_GetVarX(tw) : sprite[vm.g_i].sectnum;
                register int32_t const nSet    = Gv_GetVarX(lVar2);

                VM_SetSector(nSector, nLabel, nSet);
                continue;
            }

        case CON_GETSECTOR:
            insptr++;
            {
                tw = *insptr++;

                int const              nLabel  = *insptr++;
                int const              lVar2   = *insptr++;
                register int32_t const nSector = (tw != g_iThisActorID) ? Gv_GetVarX(tw) : sprite[vm.g_i].sectnum;

                Gv_SetVarX(lVar2, VM_GetSector(nSector, nLabel));
                continue;
            }

        case CON_SQRT:
            insptr++;
            {
                // syntax sqrt <invar> <outvar>
                int const sqrtval = ksqrt((uint32_t)Gv_GetVarX(*insptr++));
                Gv_SetVarX(*insptr++, sqrtval);
                continue;
            }

        case CON_FINDNEARACTOR:
        case CON_FINDNEARSPRITE:
        case CON_FINDNEARACTOR3D:
        case CON_FINDNEARSPRITE3D:
            insptr++;
            {
                // syntax findnearactorvar <type> <maxdist> <getvar>
                // gets the sprite ID of the nearest actor within max dist
                // that is of <type> into <getvar>
                // -1 for none found
                // <type> <maxdist> <varid>
                int const nType    = *insptr++;
                int const nMaxDist = *insptr++;
                int const nGameVar = *insptr++;
                int       lFound   = -1;
                int       nStatnum = MAXSTATUS - 1;
                int       nSprite;

                if (tw == CON_FINDNEARACTOR || tw == CON_FINDNEARACTOR3D)
                    nStatnum = 1;

                if (tw==CON_FINDNEARSPRITE3D || tw==CON_FINDNEARACTOR3D)
                {
                    do
                    {
                        nSprite=headspritestat[nStatnum];    // all sprites
                        while (nSprite>=0)
                        {
                            if (sprite[nSprite].picnum == nType && nSprite != vm.g_i && dist(&sprite[vm.g_i], &sprite[nSprite]) < nMaxDist)
                            {
                                lFound=nSprite;
                                nSprite = MAXSPRITES;
                                break;
                            }
                            nSprite = nextspritestat[nSprite];
                        }
                        if (nSprite == MAXSPRITES || tw == CON_FINDNEARACTOR3D)
                            break;
                    }
                    while (nStatnum--);
                    Gv_SetVarX(nGameVar, lFound);
                    continue;
                }

                do
                {
                    nSprite=headspritestat[nStatnum];    // all sprites
                    while (nSprite>=0)
                    {
                        if (sprite[nSprite].picnum == nType && nSprite != vm.g_i && ldist(&sprite[vm.g_i], &sprite[nSprite]) < nMaxDist)
                        {
                            lFound=nSprite;
                            nSprite = MAXSPRITES;
                            break;
                        }
                        nSprite = nextspritestat[nSprite];
                    }

                    if (nSprite == MAXSPRITES || tw == CON_FINDNEARACTOR)
                        break;
                }
                while (nStatnum--);
                Gv_SetVarX(nGameVar, lFound);
                continue;
            }

        case CON_FINDNEARACTORVAR:
        case CON_FINDNEARSPRITEVAR:
        case CON_FINDNEARACTOR3DVAR:
        case CON_FINDNEARSPRITE3DVAR:
            insptr++;
            {
                // syntax findnearactorvar <type> <maxdistvar> <getvar>
                // gets the sprite ID of the nearest actor within max dist
                // that is of <type> into <getvar>
                // -1 for none found
                // <type> <maxdistvarid> <varid>
                int const nPicnum  = *insptr++;
                int const nMaxDist = Gv_GetVarX(*insptr++);
                int const nGameVar = *insptr++;
                int       lFound   = -1;
                int       nStatnum = 1;
                int       nSprite;

                if (tw == CON_FINDNEARSPRITEVAR || tw == CON_FINDNEARSPRITE3DVAR)
                    nStatnum = MAXSTATUS-1;

                if (tw==CON_FINDNEARACTOR3DVAR || tw==CON_FINDNEARSPRITE3DVAR)
                {
                    do
                    {
                        nSprite=headspritestat[nStatnum];    // all sprites

                        while (nSprite >= 0)
                        {
                            if (sprite[nSprite].picnum == nPicnum && nSprite != vm.g_i && dist(&sprite[vm.g_i], &sprite[nSprite]) < nMaxDist)
                            {
                                lFound=nSprite;
                                nSprite = MAXSPRITES;
                                break;
                            }
                            nSprite = nextspritestat[nSprite];
                        }
                        if (nSprite == MAXSPRITES || tw==CON_FINDNEARACTOR3DVAR)
                            break;
                    }
                    while (nStatnum--);
                    Gv_SetVarX(nGameVar, lFound);
                    continue;
                }

                do
                {
                    nSprite=headspritestat[nStatnum];    // all sprites

                    while (nSprite >= 0)
                    {
                        if (sprite[nSprite].picnum == nPicnum && nSprite != vm.g_i && ldist(&sprite[vm.g_i], &sprite[nSprite]) < nMaxDist)
                        {
                            lFound=nSprite;
                            nSprite = MAXSPRITES;
                            break;
                        }
                        nSprite = nextspritestat[nSprite];
                    }

                    if (nSprite == MAXSPRITES || tw==CON_FINDNEARACTORVAR)
                        break;
                }
                while (nStatnum--);
                Gv_SetVarX(nGameVar, lFound);
                continue;
            }

        case CON_FINDNEARACTORZVAR:
        case CON_FINDNEARSPRITEZVAR:
            insptr++;
            {
                // syntax findnearactorvar <type> <maxdistvar> <getvar>
                // gets the sprite ID of the nearest actor within max dist
                // that is of <type> into <getvar>
                // -1 for none found
                // <type> <maxdistvarid> <varid>
                int const lType     = *insptr++;
                int const lMaxDist  = Gv_GetVarX(*insptr++);
                int const lMaxZDist = Gv_GetVarX(*insptr++);
                int const nGameVar  = *insptr++;
                int       lFound    = -1;
                int       nStatnum  = MAXSTATUS-1;

                do
                {
                    int nSprite = headspritestat[tw == CON_FINDNEARACTORZVAR ? 1 : nStatnum];  // all sprites

                    if (nSprite == -1)
                        continue;
                    do
                    {
                        if (sprite[nSprite].picnum == lType && nSprite != vm.g_i)
                        {
                            if (ldist(&sprite[vm.g_i], &sprite[nSprite]) < lMaxDist)
                            {
                                if (klabs(sprite[vm.g_i].z-sprite[nSprite].z) < lMaxZDist)
                                {
                                    lFound  = nSprite;
                                    nSprite = MAXSPRITES;
                                    break;
                                }
                            }
                        }
                        nSprite = nextspritestat[nSprite];
                    }
                    while (nSprite>=0);
                    if (tw==CON_FINDNEARACTORZVAR || nSprite == MAXSPRITES)
                        break;
                }
                while (nStatnum--);
                Gv_SetVarX(nGameVar, lFound);

                continue;
            }

        case CON_FINDNEARACTORZ:
        case CON_FINDNEARSPRITEZ:
            insptr++;
            {
                // syntax findnearactorvar <type> <maxdist> <getvar>
                // gets the sprite ID of the nearest actor within max dist
                // that is of <type> into <getvar>
                // -1 for none found
                // <type> <maxdist> <varid>
                int const lType     = *insptr++;
                int const lMaxDist  = *insptr++;
                int const lMaxZDist = *insptr++;
                int const nGameVar  = *insptr++;
                int       lFound    = -1;
                int       nStatnum  = MAXSTATUS - 1;

                do
                {
                    int nSprite = headspritestat[tw == CON_FINDNEARACTORZ ? 1 : nStatnum];  // all sprites

                    if (nSprite == -1)
                        continue;
                    do
                    {
                        if (sprite[nSprite].picnum == lType && nSprite != vm.g_i)
                        {
                            if (ldist(&sprite[vm.g_i], &sprite[nSprite]) < lMaxDist)
                            {
                                if (klabs(sprite[vm.g_i].z-sprite[nSprite].z) < lMaxZDist)
                                {
                                    lFound=nSprite;
                                    nSprite = MAXSPRITES;
                                    break;
                                }
                            }
                        }
                        nSprite = nextspritestat[nSprite];
                    }
                    while (nSprite>=0);

                    if (tw==CON_FINDNEARACTORZ || nSprite == MAXSPRITES)
                        break;
                }
                while (nStatnum--);
                Gv_SetVarX(nGameVar, lFound);
                continue;
            }

        case CON_FINDPLAYER:
            insptr++;
            aGameVars[g_iReturnVarID].nValue = A_FindPlayer(&sprite[vm.g_i], &tw);
            Gv_SetVarX(*insptr++, tw);
            continue;

        case CON_FINDOTHERPLAYER:
            insptr++;
            aGameVars[g_iReturnVarID].nValue = P_FindOtherPlayer(vm.g_p,&tw);
            Gv_SetVarX(*insptr++, tw);
            continue;

        case CON_SETPLAYER:
            insptr++;
            {
                tw = *insptr++;

                int const lLabelID = *insptr++;
                int const lParm2   = (PlayerLabels[lLabelID].flags & LABEL_HASPARM2) ? Gv_GetVarX(*insptr++) : 0;
                int const lVar2    = *insptr++;
                int const iPlayer  = (tw != g_iThisActorID) ? Gv_GetVarX(tw) : vm.g_p;
                int const iSet     = Gv_GetVarX(lVar2);

                VM_SetPlayer(iPlayer, lLabelID, lParm2, iSet);
                continue;
            }

        case CON_GETPLAYER:
            insptr++;
            {
                tw = *insptr++;

                int const lLabelID = *insptr++;
                int const lParm2   = (PlayerLabels[lLabelID].flags & LABEL_HASPARM2) ? Gv_GetVarX(*insptr++) : 0;
                int const lVar2    = *insptr++;
                int const iPlayer  = (tw != g_iThisActorID) ? Gv_GetVarX(tw) : vm.g_p;

                Gv_SetVarX(lVar2, VM_GetPlayer(iPlayer, lLabelID, lParm2));
                continue;
            }

        case CON_GETINPUT:
            insptr++;
            {
                tw = *insptr++;

                int const lLabelID = *insptr++;
                int const lVar2    = *insptr++;
                int const iPlayer  = (tw != g_iThisActorID) ? Gv_GetVarX(tw) : vm.g_p;

                Gv_SetVarX(lVar2, VM_GetPlayerInput(iPlayer, lLabelID));
                continue;
            }

        case CON_SETINPUT:
            insptr++;
            {
                tw = *insptr++;

                int const lLabelID = *insptr++;
                int const lVar2    = *insptr++;
                int const iPlayer  = (tw != g_iThisActorID) ? Gv_GetVarX(tw) : vm.g_p;
                int const iSet     = Gv_GetVarX(lVar2);

                VM_SetPlayerInput(iPlayer, lLabelID, iSet);
                continue;
            }

        case CON_GETUSERDEF:
            insptr++;
            {
                tw = *insptr++;

                int const lVar2 = *insptr++;

                Gv_SetVarX(lVar2, VM_GetUserdef(tw));
                continue;
            }

        case CON_SETUSERDEF:
            insptr++;
            {
                tw = *insptr++;

                int const lVar2 = *insptr++;
                int const iSet  = Gv_GetVarX(lVar2);

                VM_SetUserdef(tw, iSet);
                continue;
            }

        case CON_GETPROJECTILE:
            insptr++;
            {
                tw = Gv_GetVarX(*insptr++);

                int const lLabelID = *insptr++;
                int const lVar2    = *insptr++;

                Gv_SetVarX(lVar2, VM_GetProjectile(tw, lLabelID));
                continue;
            }

        case CON_SETPROJECTILE:
            insptr++;
            {
                tw = Gv_GetVarX(*insptr++);

                int const lLabelID = *insptr++;
                int const lVar2    = *insptr++;
                int const iSet     = Gv_GetVarX(lVar2);

                VM_SetProjectile(tw, lLabelID, iSet);
                continue;
            }

        case CON_SETWALL:
            insptr++;
            {
                tw = *insptr++;

                int const lLabelID = *insptr++;
                int const lVar2    = *insptr++;
                int const iWall    = Gv_GetVarX(tw);
                int const iSet     = Gv_GetVarX(lVar2);

                VM_SetWall(iWall, lLabelID, iSet);
                continue;
            }

        case CON_GETWALL:
            insptr++;
            {
                tw = *insptr++;

                int const lLabelID = *insptr++;
                int const lVar2    = *insptr++;
                int const iWall    = Gv_GetVarX(tw);

                Gv_SetVarX(lVar2, VM_GetWall(iWall, lLabelID));
                continue;
            }

        case CON_SETACTORVAR:
        case CON_GETACTORVAR:
            insptr++;
            {
                int const lSprite = Gv_GetVarX(*insptr++);
                int const lVar1   = *insptr++;
                int const lVar2   = *insptr++;

                if (EDUKE32_PREDICT_FALSE((unsigned)lSprite >= MAXSPRITES))
                {
                    CON_ERRPRINTF("invalid sprite ID %d\n", lSprite);

                    if (lVar1 == MAXGAMEVARS || lVar1 & ((MAXGAMEVARS << 2) | (MAXGAMEVARS << 3)))
                        insptr++;

                    if (lVar2 == MAXGAMEVARS || lVar2 & ((MAXGAMEVARS << 2) | (MAXGAMEVARS << 3)))
                        insptr++;
                    continue;
                }

                if (tw == CON_SETACTORVAR)
                    Gv_SetVar(lVar1, Gv_GetVarX(lVar2), lSprite, vm.g_p);
                else
                    Gv_SetVarX(lVar2, Gv_GetVar(lVar1, lSprite, vm.g_p));

                continue;
            }

        case CON_SETPLAYERVAR:
        case CON_GETPLAYERVAR:
            insptr++;
            {
                int const iPlayer = (*insptr++ != g_iThisActorID) ? Gv_GetVarX(*(insptr-1)) : vm.g_p;
                int const lVar1   = *insptr++;
                int const lVar2   = *insptr++;

                if (EDUKE32_PREDICT_FALSE((unsigned)iPlayer >= (unsigned)playerswhenstarted))
                {
                    CON_ERRPRINTF("invalid player ID %d\n", iPlayer);

                    if (lVar1 == MAXGAMEVARS || lVar1 & ((MAXGAMEVARS << 2) | (MAXGAMEVARS << 3)))
                        insptr++;

                    if (lVar2 == MAXGAMEVARS || lVar2 & ((MAXGAMEVARS << 2) | (MAXGAMEVARS << 3)))
                        insptr++;

                    continue;
                }

                if (tw == CON_SETPLAYERVAR)
                    Gv_SetVar(lVar1, Gv_GetVarX(lVar2), vm.g_i, iPlayer);
                else
                    Gv_SetVarX(lVar2, Gv_GetVar(lVar1, vm.g_i, iPlayer));

                continue;
            }

        case CON_SETACTOR:
            insptr++;
            {
                tw = *insptr++;

                int const lLabelID = *insptr++;
                int const lParm2   = (ActorLabels[lLabelID].flags & LABEL_HASPARM2) ? Gv_GetVarX(*insptr++) : 0;
                int const lVar2    = *insptr++;
                int const iActor   = (tw != g_iThisActorID) ? Gv_GetVarX(tw) : vm.g_i;
                int const iSet     = Gv_GetVarX(lVar2);

                VM_SetSprite(iActor, lLabelID, lParm2, iSet);
                continue;
            }

        case CON_GETACTOR:
            insptr++;
            {
                tw = *insptr++;

                int const lLabelID = *insptr++;
                int const lParm2   = (ActorLabels[lLabelID].flags & LABEL_HASPARM2) ? Gv_GetVarX(*insptr++) : 0;
                int const lVar2    = *insptr++;
                int const iActor   = (tw != g_iThisActorID) ? Gv_GetVarX(tw) : vm.g_i;

                Gv_SetVarX(lVar2, VM_GetSprite(iActor, lLabelID, lParm2));
                continue;
            }

        case CON_SETTSPR:
            insptr++;
            {
                tw = *insptr++;

                int const lLabelID = *insptr++;
                int const lVar2    = *insptr++;
                int const iActor   = (tw != g_iThisActorID) ? Gv_GetVarX(tw) : vm.g_i;
                int const iSet     = Gv_GetVarX(lVar2);

                VM_SetTsprite(iActor, lLabelID, iSet);
                continue;
            }

        case CON_GETTSPR:
            insptr++;
            {
                tw = *insptr++;

                int const lLabelID = *insptr++;
                int const lVar2    = *insptr++;
                int const iActor   = (tw != g_iThisActorID) ? Gv_GetVarX(tw) : vm.g_i;

                Gv_SetVarX(lVar2, VM_GetTsprite(iActor, lLabelID));
                continue;
            }

        case CON_GETANGLETOTARGET:
            insptr++;
            // Actor[vm.g_i].lastvx and lastvy are last known location of target.
            Gv_SetVarX(*insptr++, getangle(actor[vm.g_i].lastvx-vm.g_sp->x,actor[vm.g_i].lastvy-vm.g_sp->y));
            continue;

        case CON_ANGOFFVAR:
            insptr++;
            spriteext[vm.g_i].angoff = Gv_GetVarX(*insptr++);
            continue;

        case CON_LOCKPLAYER:
            insptr++;
            ps->transporter_hold = Gv_GetVarX(*insptr++);
            continue;

        case CON_CHECKAVAILWEAPON:
            insptr++;
            tw = (*insptr != g_iThisActorID) ? Gv_GetVarX(*insptr) : vm.g_p;
            insptr++;

            if (EDUKE32_PREDICT_FALSE((unsigned)tw >= (unsigned)playerswhenstarted))
            {
                CON_ERRPRINTF("Invalid player ID %d\n", tw);
                continue;
            }

            P_CheckWeapon(g_player[tw].ps);
            continue;

        case CON_CHECKAVAILINVEN:
            insptr++;
            tw = (*insptr != g_iThisActorID) ? Gv_GetVarX(*insptr) : vm.g_p;
            insptr++;

            if (EDUKE32_PREDICT_FALSE((unsigned)tw >= (unsigned)playerswhenstarted))
            {
                CON_ERRPRINTF("Invalid player ID %d\n", tw);
                continue;
            }

            P_SelectNextInvItem(g_player[tw].ps);
            continue;

        case CON_GETPLAYERANGLE:
            insptr++;
            Gv_SetVarX(*insptr++, ps->ang);
            continue;

        case CON_GETACTORANGLE:
            insptr++;
            Gv_SetVarX(*insptr++, vm.g_sp->ang);
            continue;

        case CON_SETPLAYERANGLE:
            insptr++;
            ps->ang = Gv_GetVarX(*insptr++) & 2047;
            continue;

        case CON_SETACTORANGLE:
            insptr++;
            vm.g_sp->ang = Gv_GetVarX(*insptr++) & 2047;
            continue;

        case CON_SETVAR:
            insptr++;
            if ((aGameVars[*insptr].nFlags & (GAMEVAR_USER_MASK | GAMEVAR_PTR_MASK)) == 0)
                aGameVars[*insptr].nValue = *(insptr + 1);
            else
                Gv_SetVarX(*insptr, *(insptr + 1));
            insptr += 2;
            continue;

        case CON_KLABS:
            if ((aGameVars[*(insptr + 1)].nFlags & (GAMEVAR_USER_MASK | GAMEVAR_PTR_MASK)) == 0)
                aGameVars[*(insptr + 1)].nValue = klabs(aGameVars[*(insptr + 1)].nValue);
            else
                Gv_SetVarX(*(insptr + 1), klabs(Gv_GetVarX(*(insptr + 1))));
            insptr += 2;
            continue;

        case CON_SETARRAY:
            insptr++;
            {
                tw = *insptr++;

                int const index = Gv_GetVarX(*insptr++);
                int const value = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)tw >= (unsigned)g_gameArrayCount || (unsigned)index >= (unsigned)aGameArrays[tw].size))
                {
                    OSD_Printf(OSD_ERROR "Gv_SetVar(): tried to set invalid array ID (%d) or index out of bounds from sprite %d (%d), player %d\n",
                        tw,vm.g_i,TrackerCast(sprite[vm.g_i].picnum),vm.g_p);
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE(aGameArrays[tw].nFlags & GAMEARRAY_READONLY))
                {
                    OSD_Printf("Tried to set on read-only array `%s'", aGameArrays[tw].szLabel);
                    continue;
                }

                aGameArrays[tw].pValues[index]=value;
                continue;
            }

        case CON_WRITEARRAYTOFILE:
        case CON_READARRAYFROMFILE:
            insptr++;
            {
                int const nArray    = *insptr++;
                int const qFilename = *insptr++;

                if (EDUKE32_PREDICT_FALSE(ScriptQuotes[qFilename] == NULL))
                {
                    CON_ERRPRINTF("null quote %d\n", qFilename);
                    continue;
                }

                if (tw == CON_READARRAYFROMFILE)
                {
                    int32_t kfil = kopen4loadfrommod(ScriptQuotes[qFilename], 0);

                    if (kfil < 0)
                        continue;

                    int32_t nElements = kfilelength(kfil) / sizeof(int32_t);

                    if (nElements == 0)
                    {
                        Baligned_free(aGameArrays[nArray].pValues);
                        aGameArrays[nArray].pValues = NULL;
                        aGameArrays[nArray].size = nElements;
                    }
                    else if (nElements > 0)
                    {
                        int const numbytes = nElements * sizeof(int32_t);
#ifdef BITNESS64
                        int32_t *pArray = (int32_t *)Xcalloc(nElements, sizeof(int32_t));
                        kread(kfil, pArray, numbytes);
#endif
                        Baligned_free(aGameArrays[nArray].pValues);
                        aGameArrays[nArray].pValues = (intptr_t *)Xaligned_alloc(ACTOR_VAR_ALIGNMENT, nElements * GAR_ELTSZ);
                        aGameArrays[nArray].size = nElements;
#ifdef BITNESS64
                        for (int i = 0; i < nElements; i++)
                            aGameArrays[nArray].pValues[i] = pArray[i];  // int32_t --> int64_t
                        Bfree(pArray);
#else
                        kread(kfil, aGameArrays[nArray].pValues, numbytes);
#endif
                    }

                    kclose(kfil);
                    continue;
                }

                char temp[BMAX_PATH];

                if (EDUKE32_PREDICT_FALSE(G_ModDirSnprintf(temp, sizeof(temp), "%s", ScriptQuotes[qFilename])))
                {
                    CON_ERRPRINTF("file name too long\n");
                    continue;
                }

                FILE *const fil = fopen(temp, "wb");

                if (EDUKE32_PREDICT_FALSE(fil == NULL))
                {
                    CON_ERRPRINTF("couldn't open file \"%s\"\n", temp);
                    continue;
                }

                int const  nSize  = aGameArrays[nArray].size;
                int *const pArray = (int32_t *)Xmalloc(sizeof(int32_t) * nSize);

                for (int k = 0; k < nSize; k++)
                    pArray[k] = Gv_GetGameArrayValue(nArray, k);

                fwrite(pArray, 1, sizeof(int32_t) * nSize, fil);
                Bfree(pArray);
                fclose(fil);

                continue;
            }

        case CON_GETARRAYSIZE:
            insptr++;
            tw = *insptr++;
            Gv_SetVarX(*insptr++,(aGameArrays[tw].nFlags & GAMEARRAY_VARSIZE) ?
                       Gv_GetVarX(aGameArrays[tw].size) : aGameArrays[tw].size);
            continue;

        case CON_RESIZEARRAY:
            insptr++;
            {
                tw = *insptr++;

                int const newSize = Gv_GetVarX(*insptr++);
                int const oldSize = aGameArrays[tw].size;

                if (newSize >= 0 && newSize != oldSize)
                {
//                    OSD_Printf(OSDTEXT_GREEN "CON_RESIZEARRAY: resizing array %s from %d to %d\n",
//                               aGameArrays[j].szLabel, aGameArrays[j].size, newSize);

                    intptr_t * const tmpar = oldSize != 0 ? (intptr_t *)Xmalloc(GAR_ELTSZ * oldSize) : NULL;

                    if (oldSize != 0)
                        memcpy(tmpar, aGameArrays[tw].pValues, GAR_ELTSZ * oldSize);

                    Baligned_free(aGameArrays[tw].pValues);

                    aGameArrays[tw].pValues = newSize != 0 ? (intptr_t *)Xaligned_alloc(ACTOR_VAR_ALIGNMENT, GAR_ELTSZ * newSize) : NULL;
                    aGameArrays[tw].size = newSize;

                    if (oldSize != 0)
                        memcpy(aGameArrays[tw].pValues, tmpar, GAR_ELTSZ * min(oldSize, newSize));

                    if (newSize > oldSize)
                        memset(&aGameArrays[tw].pValues[oldSize], 0, GAR_ELTSZ * (newSize - oldSize));
                }
                continue;
            }

        case CON_COPY:
            insptr++;
            {
                int const aSrc       = *insptr++;
                int       nSrcIndex  = Gv_GetVarX(*insptr++);  //, vm.g_i, vm.g_p);
                int const aDest      = *insptr++;
                int       nDestIndex = Gv_GetVarX(*insptr++);
                int       nElements  = Gv_GetVarX(*insptr++);

                tw = 0;

                if (EDUKE32_PREDICT_FALSE((unsigned)aSrc>=(unsigned)g_gameArrayCount))
                {
                    CON_ERRPRINTF("Invalid array %d!", aSrc);
                    tw = 1;
                }

                if (EDUKE32_PREDICT_FALSE((unsigned)aDest>=(unsigned)g_gameArrayCount))
                {
                    CON_ERRPRINTF("Invalid array %d!", aDest);
                    tw = 1;
                }

                if (EDUKE32_PREDICT_FALSE(aGameArrays[aDest].nFlags & GAMEARRAY_READONLY))
                {
                    CON_ERRPRINTF("Array %d is read-only!", aDest);
                    tw = 1;
                }

                if (EDUKE32_PREDICT_FALSE(tw)) continue; // dirty replacement for VMFLAG_ERROR

                int const nSrcSize =
                (aGameArrays[aSrc].nFlags & GAMEARRAY_VARSIZE) ? Gv_GetVarX(aGameArrays[aSrc].size) : aGameArrays[aSrc].size;
                int const nDestSize =
                (aGameArrays[aDest].nFlags & GAMEARRAY_VARSIZE) ? Gv_GetVarX(aGameArrays[aSrc].size) : aGameArrays[aDest].size;

                if (EDUKE32_PREDICT_FALSE(nSrcIndex > nSrcSize || nDestIndex > nDestSize))
                    continue;

                if ((nSrcIndex + nElements) > nSrcSize)
                    nElements = nSrcSize - nSrcIndex;

                if ((nDestIndex + nElements) > nDestSize)
                    nElements = nDestSize - nDestIndex;

                // Switch depending on the source array type.
                switch (aGameArrays[aSrc].nFlags & GAMEARRAY_TYPE_MASK)
                {
                case 0:
                    // CON array to CON array.
                    if (EDUKE32_PREDICT_FALSE(aGameArrays[aSrc].nFlags & GAMEARRAY_STRIDE2))
                    {
                        for (; nElements>0; --nElements)
                            (aGameArrays[aDest].pValues)[nDestIndex++] = ((int32_t *)aGameArrays[aSrc].pValues)[nSrcIndex+=2];
                        break;
                    }
                    Bmemcpy(aGameArrays[aDest].pValues+nDestIndex, aGameArrays[aSrc].pValues+nSrcIndex, nElements*GAR_ELTSZ);
                    break;
                case GAMEARRAY_OFINT:
                    // From int32-sized array. Note that the CON array element
                    // type is intptr_t, so it is different-sized on 64-bit
                    // archs, but same-sized on 32-bit ones.
                    if (EDUKE32_PREDICT_FALSE(aGameArrays[aSrc].nFlags & GAMEARRAY_STRIDE2))
                    {
                        for (; nElements>0; --nElements)
                            (aGameArrays[aDest].pValues)[nDestIndex++] = ((int32_t *)aGameArrays[aSrc].pValues)[nSrcIndex+=2];
                        break;
                    }
                    for (; nElements>0; --nElements)
                        (aGameArrays[aDest].pValues)[nDestIndex++] = ((int32_t *)aGameArrays[aSrc].pValues)[nSrcIndex++];
                    break;
                case GAMEARRAY_OFSHORT:
                    // From int16_t array. Always different-sized.
                    if (EDUKE32_PREDICT_FALSE(aGameArrays[aSrc].nFlags & GAMEARRAY_STRIDE2))
                    {
                        for (; nElements>0; --nElements)
                            (aGameArrays[aDest].pValues)[nDestIndex++] = ((int16_t *)aGameArrays[aSrc].pValues)[nSrcIndex+=2];
                        break;
                    }
                    for (; nElements>0; --nElements)
                        (aGameArrays[aDest].pValues)[nDestIndex++] = ((int16_t *)aGameArrays[aSrc].pValues)[nSrcIndex++];
                    break;
                case GAMEARRAY_OFCHAR:
                    // From char array. Always different-sized.
                    if (EDUKE32_PREDICT_FALSE(aGameArrays[aSrc].nFlags & GAMEARRAY_STRIDE2))
                    {
                        for (; nElements>0; --nElements)
                            (aGameArrays[aDest].pValues)[nDestIndex++] = ((uint8_t *)aGameArrays[aSrc].pValues)[nSrcIndex+=2];
                        break;
                    }
                    for (; nElements>0; --nElements)
                        (aGameArrays[aDest].pValues)[nDestIndex++] = ((uint8_t *)aGameArrays[aSrc].pValues)[nSrcIndex++];
                    break;
                }
                continue;
            }

        case CON_RANDVAR:
            insptr++;
            Gv_SetVarX(*insptr, mulscale16(krand(), *(insptr + 1) + 1));
            insptr += 2;
            continue;

        case CON_DISPLAYRANDVAR:
            insptr++;
            Gv_SetVarX(*insptr, mulscale15(system_15bit_rand(), *(insptr + 1) + 1));
            insptr += 2;
            continue;

        case CON_CLAMP:
            insptr++;
            {
                tw = *insptr++;
                int const min = Gv_GetVarX(*insptr++);
                Gv_SetVarX(tw, clamp2(Gv_GetVarX(tw), min, Gv_GetVarX(*insptr++)));
            }
            continue;

        case CON_INV:
            if ((aGameVars[*(insptr + 1)].nFlags & (GAMEVAR_USER_MASK | GAMEVAR_PTR_MASK)) == 0)
                aGameVars[*(insptr + 1)].nValue = -aGameVars[*(insptr + 1)].nValue;
            else
                Gv_SetVarX(*(insptr + 1), -Gv_GetVarX(*(insptr + 1)));
            insptr += 2;
            continue;

        case CON_MULVAR:
            insptr++;
            Gv_MulVar(*insptr, *(insptr + 1));
            insptr += 2;
            continue;

        case CON_DIVVAR:
            insptr++;
            if (EDUKE32_PREDICT_FALSE(*(insptr + 1) == 0))
            {
                CON_ERRPRINTF("divide by zero!\n");
                insptr += 2;
                continue;
            }
            Gv_DivVar(*insptr, *(insptr + 1));
            insptr += 2;
            continue;

        case CON_MODVAR:
            insptr++;
            if (EDUKE32_PREDICT_FALSE(*(insptr + 1) == 0))
            {
                CON_ERRPRINTF("mod by zero!\n");
                insptr += 2;
                continue;
            }

            Gv_ModVar(*insptr, *(insptr + 1));
            insptr += 2;
            continue;

        case CON_ANDVAR:
            insptr++;
            Gv_AndVar(*insptr, *(insptr + 1));
            insptr += 2;
            continue;

        case CON_ORVAR:
            insptr++;
            Gv_OrVar(*insptr, *(insptr + 1));
            insptr += 2;
            continue;

        case CON_XORVAR:
            insptr++;
            Gv_XorVar(*insptr, *(insptr + 1));
            insptr += 2;
            continue;

        case CON_SETVARVAR:
            insptr++;
            {
                tw = *insptr++;
                int const gv = Gv_GetVarX(*insptr++);

                if ((aGameVars[tw].nFlags & (GAMEVAR_USER_MASK | GAMEVAR_PTR_MASK)) == 0)
                    aGameVars[tw].nValue = gv;
                else
                    Gv_SetVarX(tw, gv);
            }
            continue;

        case CON_RANDVARVAR:
            insptr++;
            tw = *insptr++;
            Gv_SetVarX(tw, mulscale16(krand(), Gv_GetVarX(*insptr++) + 1));
            continue;

        case CON_DISPLAYRANDVARVAR:
            insptr++;
            tw = *insptr++;
            Gv_SetVarX(tw, mulscale15(system_15bit_rand(), Gv_GetVarX(*insptr++) + 1));
            continue;

        case CON_GMAXAMMO:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            if (EDUKE32_PREDICT_FALSE((unsigned)tw >= MAX_WEAPONS))
            {
                CON_ERRPRINTF("Invalid weapon ID %d\n", tw);
                insptr++;
                continue;
            }
            Gv_SetVarX(*insptr++, ps->max_ammo_amount[tw]);
            continue;

        case CON_SMAXAMMO:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            if (EDUKE32_PREDICT_FALSE((unsigned)tw >= MAX_WEAPONS))
            {
                CON_ERRPRINTF("Invalid weapon ID %d\n", tw);
                insptr++;
                continue;
            }
            ps->max_ammo_amount[tw] = Gv_GetVarX(*insptr++);
            continue;

        case CON_MULVARVAR:
            insptr++;
            tw = *insptr++;
            Gv_MulVar(tw, Gv_GetVarX(*insptr++));
            continue;

        case CON_DIVVARVAR:
            insptr++;
            {
                tw = *insptr++;
                int const l2 = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE(!l2))
                {
                    CON_ERRPRINTF("divide by zero!\n");
                    continue;
                }

                Gv_DivVar(tw, l2);
                continue;
            }

        case CON_MODVARVAR:
            insptr++;
            {
                tw=*insptr++;
                int const l2 = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE(!l2))
                {
                    CON_ERRPRINTF("mod by zero!\n");
                    continue;
                }

                Gv_ModVar(tw, l2);
                continue;
            }

        case CON_ANDVARVAR:
            insptr++;
            tw = *insptr++;
            Gv_AndVar(tw, Gv_GetVarX(*insptr++));
            continue;

        case CON_XORVARVAR:
            insptr++;
            tw = *insptr++;
            Gv_XorVar(tw, Gv_GetVarX(*insptr++));
            continue;

        case CON_ORVARVAR:
            insptr++;
            tw = *insptr++;
            Gv_OrVar(tw, Gv_GetVarX(*insptr++));
            continue;

        case CON_SUBVAR:
            insptr++;
            Gv_SubVar(*insptr, *(insptr+1));
            insptr += 2;
            continue;

        case CON_SUBVARVAR:
            insptr++;
            tw = *insptr++;
            Gv_SubVar(tw, Gv_GetVarX(*insptr++));
            continue;

        case CON_ADDVAR:
            insptr++;
            Gv_AddVar(*insptr, *(insptr+1));
            insptr += 2;
            continue;

        case CON_SHIFTVARL:
            insptr++;
            if ((aGameVars[*insptr].nFlags & (GAMEVAR_USER_MASK|GAMEVAR_PTR_MASK)) == 0)
            {
                aGameVars[*insptr].nValue <<= *(insptr+1);
                insptr += 2;
                continue;
            }
            Gv_SetVarX(*insptr, Gv_GetVarX(*insptr) << *(insptr+1));
            insptr += 2;
            continue;

        case CON_SHIFTVARR:
            insptr++;
            if ((aGameVars[*insptr].nFlags & (GAMEVAR_USER_MASK|GAMEVAR_PTR_MASK)) == 0)
            {
                aGameVars[*insptr].nValue >>= *(insptr+1);
                insptr += 2;
                continue;
            }
            Gv_SetVarX(*insptr, Gv_GetVarX(*insptr) >> *(insptr+1));
            insptr += 2;
            continue;

        case CON_SHIFTVARVARL:
            insptr++;
            tw = *insptr++;
            Gv_SetVarX(tw, Gv_GetVarX(tw) << Gv_GetVarX(*insptr++));
            continue;

        case CON_SHIFTVARVARR:
            insptr++;
            tw = *insptr++;
            Gv_SetVarX(tw, Gv_GetVarX(tw) >> Gv_GetVarX(*insptr++));
            continue;

        case CON_SIN:
            insptr++;
            tw = *insptr++;
            Gv_SetVarX(tw, sintable[Gv_GetVarX(*insptr++)&2047]);
            continue;

        case CON_COS:
            insptr++;
            tw = *insptr++;
            Gv_SetVarX(tw, sintable[(Gv_GetVarX(*insptr++)+512)&2047]);
            continue;

        case CON_ADDVARVAR:
            insptr++;
            tw = *insptr++;
            Gv_AddVar(tw, Gv_GetVarX(*insptr++));
            continue;

        case CON_SPGETLOTAG:
            insptr++;
            aGameVars[g_iLoTagID].nValue = vm.g_sp->lotag;
            continue;

        case CON_SPGETHITAG:
            insptr++;
            aGameVars[g_iHiTagID].nValue = vm.g_sp->hitag;
            continue;

        case CON_SECTGETLOTAG:
            insptr++;
            aGameVars[g_iLoTagID].nValue = sector[vm.g_sp->sectnum].lotag;
            continue;

        case CON_SECTGETHITAG:
            insptr++;
            aGameVars[g_iHiTagID].nValue = sector[vm.g_sp->sectnum].hitag;
            continue;

        case CON_GETTEXTUREFLOOR:
            insptr++;
            aGameVars[g_iTextureID].nValue = sector[vm.g_sp->sectnum].floorpicnum;
            continue;

        case CON_STARTTRACK:
        case CON_STARTTRACKVAR:
            insptr++;
            {
                int const level = (tw == CON_STARTTRACK) ? *(insptr++) : Gv_GetVarX(*(insptr++));

                if (EDUKE32_PREDICT_FALSE(G_StartTrack(level)))
                    CON_ERRPRINTF("invalid level %d or null music for volume %d level %d\n",
                                  level, ud.volume_number, level);
            }
            continue;

        case CON_SETMUSICPOSITION:
            insptr++;
            S_SetMusicPosition(Gv_GetVarX(*insptr++));
            continue;

        case CON_GETMUSICPOSITION:
            insptr++;
            Gv_SetVarX(*insptr++, S_GetMusicPosition());
            continue;

        case CON_ACTIVATECHEAT:
            insptr++;
            tw = Gv_GetVarX(*(insptr++));
            if (EDUKE32_PREDICT_FALSE(numplayers != 1 || !(g_player[myconnectindex].ps->gm & MODE_GAME)))
            {
                CON_ERRPRINTF("not in a single-player game.\n");
                continue;
            }
            osdcmd_cheatsinfo_stat.cheatnum = tw;
            continue;

        case CON_SETGAMEPALETTE:
            insptr++;
            P_SetGamePalette(ps, Gv_GetVarX(*(insptr++)), 2+16);
            continue;

        case CON_GETTEXTURECEILING:
            insptr++;
            aGameVars[g_iTextureID].nValue = sector[vm.g_sp->sectnum].ceilingpicnum;
            continue;

        case CON_IFVARVARAND:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            tw &= Gv_GetVarX(*insptr++);
            insptr--;
            VM_CONDITIONAL(tw);
            continue;

        case CON_IFVARVAROR:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            tw |= Gv_GetVarX(*insptr++);
            insptr--;
            VM_CONDITIONAL(tw);
            continue;

        case CON_IFVARVARXOR:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            tw ^= Gv_GetVarX(*insptr++);
            insptr--;
            VM_CONDITIONAL(tw);
            continue;

        case CON_IFVARVAREITHER:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            tw = (Gv_GetVarX(*insptr++) || tw);
            insptr--;
            VM_CONDITIONAL(tw);
            continue;

        case CON_IFVARVARBOTH:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            tw = (Gv_GetVarX(*insptr++) && tw);
            insptr--;
            VM_CONDITIONAL(tw);
            continue;

        case CON_IFVARVARN:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            tw = (tw != Gv_GetVarX(*insptr++));
            insptr--;
            VM_CONDITIONAL(tw);
            continue;

        case CON_IFVARVARE:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            tw = (tw == Gv_GetVarX(*insptr++));
            insptr--;
            VM_CONDITIONAL(tw);
            continue;

        case CON_IFVARVARG:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            tw = (tw > Gv_GetVarX(*insptr++));
            insptr--;
            VM_CONDITIONAL(tw);
            continue;

        case CON_IFVARVARGE:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            tw = (tw >= Gv_GetVarX(*insptr++));
            insptr--;
            VM_CONDITIONAL(tw);
            continue;

        case CON_IFVARVARL:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            tw = (tw < Gv_GetVarX(*insptr++));
            insptr--;
            VM_CONDITIONAL(tw);
            continue;

        case CON_IFVARVARLE:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            tw = (tw <= Gv_GetVarX(*insptr++));
            insptr--;
            VM_CONDITIONAL(tw);
            continue;

        case CON_IFVARN:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            VM_CONDITIONAL(tw != *insptr);
            continue;

        case CON_WHILEVARN:
        {
            intptr_t const *const savedinsptr = insptr + 2;
            do
            {
                insptr = savedinsptr;
                tw = (Gv_GetVarX(*(insptr - 1)) != *insptr);
                VM_CONDITIONAL(tw);
            }
            while (tw);
            continue;
        }

        case CON_WHILEVARL:
        {
            intptr_t const *const savedinsptr = insptr + 2;
            do
            {
                insptr = savedinsptr;
                tw = (Gv_GetVarX(*(insptr - 1)) < *insptr);
                VM_CONDITIONAL(tw);
            } while (tw);
            continue;
        }

        case CON_WHILEVARVARN:
        {
            intptr_t const *const savedinsptr = insptr + 2;
            do
            {
                insptr = savedinsptr;
                tw = Gv_GetVarX(*(insptr - 1));
                tw = (tw != Gv_GetVarX(*insptr++));
                insptr--;
                VM_CONDITIONAL(tw);
            }
            while (tw);
            continue;
        }

        case CON_WHILEVARVARL:
        {
            intptr_t const *const savedinsptr = insptr + 2;
            do
            {
                insptr = savedinsptr;
                tw = Gv_GetVarX(*(insptr - 1));
                tw = (tw < Gv_GetVarX(*insptr++));
                insptr--;
                VM_CONDITIONAL(tw);
            } while (tw);
            continue;
        }

        case CON_FOR:  // special-purpose iteration
            insptr++;
            {
                int const             var   = *insptr++;
                int const             how   = *insptr++;
                int const             parm2 = how <= ITER_DRAWNSPRITES ? 0 : Gv_GetVarX(*insptr++);
                intptr_t const *const end   = insptr + *insptr;
                intptr_t const *const beg   = ++insptr;

                switch (how)
                {
                case ITER_ALLSPRITES:
                    for (int jj=0; jj<MAXSPRITES; ++jj)
                    {
                        if (sprite[jj].statnum == MAXSTATUS)
                            continue;

                        Gv_SetVarX(var, jj);
                        insptr = beg;
                        VM_Execute(0);
                    }
                    break;
                case ITER_ALLSECTORS:
                    for (int jj=0; jj<numsectors; ++jj)
                    {
                        Gv_SetVarX(var, jj);
                        insptr = beg;
                        VM_Execute(0);
                    }
                    break;
                case ITER_ALLWALLS:
                    for (int jj=0; jj<numwalls; ++jj)
                    {
                        Gv_SetVarX(var, jj);
                        insptr = beg;
                        VM_Execute(0);
                    }
                    break;
                case ITER_ACTIVELIGHTS:
#ifdef POLYMER
                    for (int jj=0; jj<PR_MAXLIGHTS; ++jj)
                    {
                        if (!prlights[jj].flags.active)
                            continue;

                        Gv_SetVarX(var, jj);
                        insptr = beg;
                        VM_Execute(0);
                    }
#endif
                    break;

                case ITER_DRAWNSPRITES:
                {
/*
                    uspritetype lastSpriteBackup;
                    uspritetype *const lastSpritePtr = (uspritetype *) &sprite[MAXSPRITES-1];
*/

                    // Back up sprite MAXSPRITES-1.
/*
                    Bmemcpy(&lastSpriteBackup, lastSpritePtr, sizeof(uspritetype));
*/

                    for (int ii=0; ii<spritesortcnt; ii++)
                    {
/*
                        Bmemcpy(lastSpritePtr, &tsprite[ii], sizeof(uspritetype));
*/
                        Gv_SetVarX(var, ii);
                        insptr = beg;
                        VM_Execute(0);

                        // Copy over potentially altered tsprite.
/*
                        Bmemcpy(&tsprite[ii], lastSpritePtr, sizeof(uspritetype));
*/
                    }

                    // Restore sprite MAXSPRITES-1.
/*
                    Bmemcpy(lastSpritePtr, &lastSpriteBackup, sizeof(uspritetype));
*/
                    break;
                }

                case ITER_SPRITESOFSECTOR:
                    if ((unsigned)parm2 >= MAXSECTORS) goto badindex;
                    for (int jj=headspritesect[parm2]; jj>=0; jj=nextspritesect[jj])
                    {
                        Gv_SetVarX(var, jj);
                        insptr = beg;
                        VM_Execute(0);
                    }
                    break;
                case ITER_SPRITESOFSTATUS:
                    if ((unsigned) parm2 >= MAXSTATUS) goto badindex;
                    for (int jj=headspritestat[parm2]; jj>=0; jj=nextspritestat[jj])
                    {
                        Gv_SetVarX(var, jj);
                        insptr = beg;
                        VM_Execute(0);
                    }
                    break;
                case ITER_WALLSOFSECTOR:
                    if ((unsigned) parm2 >= MAXSECTORS) goto badindex;
                    for (int jj=sector[parm2].wallptr, endwall=jj+sector[parm2].wallnum-1;
                    jj<=endwall; jj++)
                    {
                        Gv_SetVarX(var, jj);
                        insptr = beg;
                        VM_Execute(0);
                    }
                    break;
                case ITER_LOOPOFWALL:
                    if ((unsigned) parm2 >= (unsigned)numwalls) goto badindex;
                    {
                        int jj = parm2;
                        do
                        {
                            Gv_SetVarX(var, jj);
                            insptr = beg;
                            VM_Execute(0);
                            jj = wall[jj].point2;
                        } while (jj != parm2);
                    }
                    break;
                case ITER_RANGE:
                    for (int jj=0; jj<parm2; jj++)
                    {
                        Gv_SetVarX(var, jj);
                        insptr = beg;
                        VM_Execute(0);
                    }
                    break;
                default:
                    CON_ERRPRINTF("Unknown iteration type %d!", how);
                    continue;
                badindex:
                    OSD_Printf(OSD_ERROR "Line %d, %s %s: index %d out of range!\n", g_errorLineNum, keyw[g_tw],
                        iter_tokens[how].token, parm2);
                    continue;
                }
                insptr = end;
            }
            continue;

        case CON_IFVARAND:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            VM_CONDITIONAL(tw & *insptr);
            continue;

        case CON_IFVAROR:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            VM_CONDITIONAL(tw | *insptr);
            continue;

        case CON_IFVARXOR:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            VM_CONDITIONAL(tw ^ *insptr);
            continue;

        case CON_IFVAREITHER:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            VM_CONDITIONAL(tw || *insptr);
            continue;

        case CON_IFVARBOTH:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            VM_CONDITIONAL(tw && *insptr);
            continue;

        case CON_IFVARG:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            VM_CONDITIONAL(tw > *insptr);
            continue;

        case CON_IFVARGE:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            VM_CONDITIONAL(tw >= *insptr);
            continue;

        case CON_IFVARL:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            VM_CONDITIONAL(tw < *insptr);
            continue;

        case CON_IFVARLE:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            VM_CONDITIONAL(tw <= *insptr);
            continue;

        case CON_IFPHEALTHL:
            insptr++;
            VM_CONDITIONAL(sprite[ps->i].extra < *insptr);
            continue;

        case CON_IFPINVENTORY:
            insptr++;

            switch (*insptr++)
            {
                case GET_STEROIDS: tw = (ps->inv_amount[GET_STEROIDS] != *insptr); break;
                case GET_SHIELD:   tw = (ps->inv_amount[GET_SHIELD] != ps->max_shield_amount); break;
                case GET_SCUBA:    tw = (ps->inv_amount[GET_SCUBA] != *insptr); break;
                case GET_HOLODUKE: tw = (ps->inv_amount[GET_HOLODUKE] != *insptr); break;
                case GET_JETPACK:  tw = (ps->inv_amount[GET_JETPACK] != *insptr); break;
                case GET_ACCESS:
                    switch (vm.g_sp->pal)
                    {
                        case 0:  tw = (ps->got_access & 1); break;
                        case 21: tw = (ps->got_access & 2); break;
                        case 23: tw = (ps->got_access & 4); break;
                    }
                    break;
                case GET_HEATS:    tw = (ps->inv_amount[GET_HEATS] != *insptr); break;
                case GET_FIRSTAID: tw = (ps->inv_amount[GET_FIRSTAID] != *insptr); break;
                case GET_BOOTS:    tw = (ps->inv_amount[GET_BOOTS] != *insptr); break;
                default: tw = 0; CON_ERRPRINTF("invalid inventory ID: %d\n", (int32_t) * (insptr - 1));
            }

            VM_CONDITIONAL(tw);
            continue;

        case CON_PSTOMP:
            insptr++;
            if (ps->knee_incs == 0 && sprite[ps->i].xrepeat >= 40)
                if (cansee(vm.g_sp->x, vm.g_sp->y, vm.g_sp->z - (4 << 8), vm.g_sp->sectnum, ps->pos.x,
                           ps->pos.y, ps->pos.z + ZOFFSET2, sprite[ps->i].sectnum))
                {
                    int32_t j = playerswhenstarted - 1;

                    for (; j >= 0; j--)
                    {
                        if (g_player[j].ps->actorsqu == vm.g_i)
                            break;
                    }

                    if (j == -1)
                    {
                        ps->knee_incs = 1;
                        if (ps->weapon_pos == 0)
                            ps->weapon_pos = -1;
                        ps->actorsqu = vm.g_i;
                    }
                }

            continue;

        case CON_IFAWAYFROMWALL:
        {
            int16_t s1 = vm.g_sp->sectnum;
            tw = 0;

#define IFAWAYDIST 108

            updatesector(vm.g_sp->x + IFAWAYDIST, vm.g_sp->y + IFAWAYDIST, &s1);
            if (s1 == vm.g_sp->sectnum)
            {
                updatesector(vm.g_sp->x - IFAWAYDIST, vm.g_sp->y - IFAWAYDIST, &s1);
                if (s1 == vm.g_sp->sectnum)
                {
                    updatesector(vm.g_sp->x + IFAWAYDIST, vm.g_sp->y - IFAWAYDIST, &s1);
                    if (s1 == vm.g_sp->sectnum)
                    {
                        updatesector(vm.g_sp->x - IFAWAYDIST, vm.g_sp->y + IFAWAYDIST, &s1);
                        if (s1 == vm.g_sp->sectnum)
                            tw = 1;
                    }
                }
            }

            VM_CONDITIONAL(tw);

#undef IFAWAYDIST

        }
        continue;

        case CON_QUOTE:
            insptr++;

            if (EDUKE32_PREDICT_FALSE((unsigned)(*insptr) >= MAXQUOTES) || ScriptQuotes[*insptr] == NULL)
            {
                CON_ERRPRINTF("invalid quote ID %d\n", (int32_t)(*insptr));
                insptr++;
                continue;
            }

            if (EDUKE32_PREDICT_FALSE((unsigned)vm.g_p >= MAXPLAYERS))
            {
                CON_ERRPRINTF("bad player for quote %d: (%d)\n", (int32_t)*insptr,vm.g_p);
                insptr++;
                continue;
            }

            P_DoQuote(*(insptr++)|MAXQUOTES,ps);
            continue;

        case CON_USERQUOTE:
            insptr++;
            tw = Gv_GetVarX(*insptr++);

            if (EDUKE32_PREDICT_FALSE((unsigned)tw >= MAXQUOTES || ScriptQuotes[tw] == NULL))
            {
                CON_ERRPRINTF("invalid quote ID %d\n", tw);
                continue;
            }

            G_AddUserQuote(ScriptQuotes[tw]);
            continue;

        case CON_ECHO:
            insptr++;
            tw = Gv_GetVarX(*insptr++);

            if (EDUKE32_PREDICT_FALSE((unsigned)tw >= MAXQUOTES || ScriptQuotes[tw] == NULL))
            {
                CON_ERRPRINTF("invalid quote ID %d\n", tw);
                continue;
            }

            OSD_Printf("%s\n", ScriptQuotes[tw]);
            continue;

        case CON_IFINOUTERSPACE:
            VM_CONDITIONAL(G_CheckForSpaceFloor(vm.g_sp->sectnum));
            continue;

        case CON_IFNOTMOVING:
            VM_CONDITIONAL((actor[vm.g_i].movflag&49152) > 16384);
            continue;

        case CON_RESPAWNHITAG:
            insptr++;
            switch (DYNAMICTILEMAP(vm.g_sp->picnum))
            {
            case FEM1__STATIC:
            case FEM2__STATIC:
            case FEM3__STATIC:
            case FEM4__STATIC:
            case FEM5__STATIC:
            case FEM6__STATIC:
            case FEM7__STATIC:
            case FEM8__STATIC:
            case FEM9__STATIC:
            case FEM10__STATIC:
            case PODFEM1__STATIC:
            case NAKED1__STATIC:
            case STATUE__STATIC:
                if (vm.g_sp->yvel)
                    G_OperateRespawns(vm.g_sp->yvel);
                break;
            default:
//                if (vm.g_sp->hitag >= 0)
                    G_OperateRespawns(vm.g_sp->hitag);
                break;
            }
            continue;

        case CON_IFSPRITEPAL:
            insptr++;
            VM_CONDITIONAL(vm.g_sp->pal == *insptr);
            continue;

        case CON_IFANGDIFFL:
            insptr++;
            tw = klabs(G_GetAngleDelta(ps->ang, vm.g_sp->ang));
            VM_CONDITIONAL(tw <= *insptr);
            continue;

        case CON_IFNOSOUNDS:
            VM_CONDITIONAL(!A_CheckAnySoundPlaying(vm.g_i));
            continue;

        case CON_SPRITEFLAGS:
            insptr++;
            actor[vm.g_i].flags = Gv_GetVarX(*insptr++);
            continue;

        case CON_GETTICKS:
            insptr++;
            Gv_SetVarX(*insptr++, getticks());
            continue;

        case CON_GETCURRADDRESS:
            insptr++;
            tw = *insptr++;
            Gv_SetVarX(tw, (intptr_t)(insptr - script));
            continue;

        case CON_JUMP:  // XXX XXX XXX
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            insptr = (intptr_t *)(tw + script);
            continue;

        default:
            VM_ScriptInfo(insptr, 64);

            G_GameExit("An error has occurred in the EDuke32 virtual machine.\n\n"
                       "If you are an end user, please e-mail the file eduke32.log\n"
                       "along with links to any mods you're using to terminx@gmail.com.\n\n"
                       "If you are a mod developer, please attach all of your CON files\n"
                       "along with instructions on how to reproduce this error.\n\n"
                       "Thank you!");
            break;
        }
    }
}

// NORECURSE
void A_LoadActor(int32_t iActor)
{
    vm.g_i  = iActor;           // Sprite ID
    vm.g_sp = &sprite[iActor];  // Pointer to sprite structure

    if (g_tile[vm.g_sp->picnum].loadPtr == NULL)
        return;

    vm.g_t  = &actor[iActor].t_data[0];  // Sprite's 'extra' data
    vm.g_p  = -1;                        // Player ID
    vm.g_x  = -1;                        // Distance
    vm.g_pp = g_player[0].ps;

    vm.g_flags &= ~(VM_RETURN | VM_KILL | VM_NOEXECUTE);

    if ((unsigned)vm.g_sp->sectnum >= MAXSECTORS)
    {
        A_DeleteSprite(vm.g_i);
        return;
    }

    insptr = g_tile[vm.g_sp->picnum].loadPtr;
    VM_Execute(1);
    insptr = NULL;

    if (vm.g_flags & VM_KILL)
        A_DeleteSprite(vm.g_i);
}
#endif

// NORECURSE
void A_Execute(int32_t iActor, int32_t iPlayer, int32_t lDist)
{
    vmstate_t tempvm = { iActor, iPlayer, lDist, &actor[iActor].t_data[0], &sprite[iActor], g_player[iPlayer].ps, 0 };
    vm = tempvm;

#ifdef LUNATIC
    int32_t killit=0;
#else
    intptr_t actionofs, *actionptr;
#endif

/*
    if (g_netClient && A_CheckSpriteFlags(iActor, SFLAG_NULL))
    {
        A_DeleteSprite(iActor);
        return;
    }
*/

    if (g_netServer || g_netClient)
        randomseed = ticrandomseed;

    if (EDUKE32_PREDICT_FALSE((unsigned)vm.g_sp->sectnum >= MAXSECTORS))
    {
        if (A_CheckEnemySprite(vm.g_sp))
            vm.g_pp->actors_killed++;

        A_DeleteSprite(vm.g_i);
        return;
    }

#if !defined LUNATIC
    actionofs = AC_ACTION_ID(vm.g_t);
    actionptr = (actionofs != 0 && actionofs + 4u < (unsigned)g_scriptSize) ? &script[actionofs] : NULL;

    if (actionptr != NULL)
#endif
    {
#if !defined LUNATIC
        int const action_frames = actionptr[1];
        int const action_incval = actionptr[3];
        int const action_delay = actionptr[4];
#else
        int const action_frames = actor[vm.g_i].ac.numframes;
        int const action_incval = actor[vm.g_i].ac.incval;
        int const action_delay = actor[vm.g_i].ac.delay;
#endif
        uint16_t *actionticsptr = &AC_ACTIONTICS(vm.g_sp, &actor[vm.g_i]);
        *actionticsptr += TICSPERFRAME;

        if (*actionticsptr > action_delay)
        {
            *actionticsptr = 0;
            AC_ACTION_COUNT(vm.g_t)++;
            AC_CURFRAME(vm.g_t) += action_incval;
        }

        if (klabs(AC_CURFRAME(vm.g_t)) >= klabs(action_frames * action_incval))
            AC_CURFRAME(vm.g_t) = 0;
    }

#ifdef LUNATIC
    int const picnum = vm.g_sp->picnum;

    if (L_IsInitialized(&g_ElState) && El_HaveActor(picnum))
    {
        double t = gethiticks();

        killit = (El_CallActor(&g_ElState, picnum, iActor, iPlayer, lDist)==1);

        t = gethiticks()-t;
        g_actorTotalMs[picnum] += t;
        g_actorMinMs[picnum] = min(g_actorMinMs[picnum], t);
        g_actorMaxMs[picnum] = max(g_actorMaxMs[picnum], t);
        g_actorCalls[picnum]++;
    }
#else
    insptr = 4 + (g_tile[vm.g_sp->picnum].execPtr);
    VM_Execute(1);
    insptr = NULL;
#endif

#ifdef LUNATIC
    if (killit)
#else
    if (vm.g_flags & VM_KILL)
#endif
    {
        VM_DeleteSprite(iActor, iPlayer);
        return;
    }

    VM_Move();

    if (vm.g_sp->statnum != STAT_ACTOR)
    {
        if (vm.g_sp->statnum == STAT_STANDABLE)
        {
            switch (DYNAMICTILEMAP(vm.g_sp->picnum))
            {
                case RUBBERCAN__STATIC:
                case EXPLODINGBARREL__STATIC:
                case WOODENHORSE__STATIC:
                case HORSEONSIDE__STATIC:
                case CANWITHSOMETHING__STATIC:
                case FIREBARREL__STATIC:
                case NUKEBARREL__STATIC:
                case NUKEBARRELDENTED__STATIC:
                case NUKEBARRELLEAKED__STATIC:
                case TRIPBOMB__STATIC:
                case EGG__STATIC:
                    if (actor[vm.g_i].timetosleep > 1)
                        actor[vm.g_i].timetosleep--;
                    else if (actor[vm.g_i].timetosleep == 1)
                        changespritestat(vm.g_i, STAT_ZOMBIEACTOR);
                default: break;
            }
        }
        return;
    }

    if (A_CheckEnemySprite(vm.g_sp))
    {
        if (vm.g_sp->xrepeat > 60 || (ud.respawn_monsters == 1 && vm.g_sp->extra <= 0))
            return;
    }
    else if (EDUKE32_PREDICT_FALSE(ud.respawn_items == 1 && (vm.g_sp->cstat & 32768)))
        return;

    if (A_CheckSpriteFlags(vm.g_i, SFLAG_USEACTIVATOR) && sector[vm.g_sp->sectnum].lotag & 16384)
        changespritestat(vm.g_i, STAT_ZOMBIEACTOR);
    else if (actor[vm.g_i].timetosleep > 1)
        actor[vm.g_i].timetosleep--;
    else if (actor[vm.g_i].timetosleep == 1)
    {
        // hack for 1.3D fire sprites
        if (EDUKE32_PREDICT_FALSE(g_scriptVersion == 13 && (vm.g_sp->picnum == FIRE || vm.g_sp->picnum == FIRE2)))
            return;
        changespritestat(vm.g_i, STAT_ZOMBIEACTOR);
    }
}

void G_SaveMapState(void)
{
    int32_t levelnum = ud.volume_number*MAXLEVELS+ud.level_number;
    map_t *mapinfo = &MapInfo[levelnum];

    if (mapinfo->savedstate == NULL)
    {
        mapinfo->savedstate = (mapstate_t *) Xaligned_alloc(16, sizeof(mapstate_t));
        Bmemset(mapinfo->savedstate, 0, sizeof(mapstate_t));
    }

    mapstate_t *save = mapinfo->savedstate;

    if (save == NULL)
        return;

    Bmemcpy(&save->numwalls,&numwalls,sizeof(numwalls));
    Bmemcpy(&save->wall[0],&wall[0],sizeof(walltype)*MAXWALLS);
    Bmemcpy(&save->numsectors,&numsectors,sizeof(numsectors));
    Bmemcpy(&save->sector[0],&sector[0],sizeof(sectortype)*MAXSECTORS);
    Bmemcpy(&save->sprite[0],&sprite[0],sizeof(spritetype)*MAXSPRITES);

    // If we're in EVENT_ANIMATESPRITES, we'll be saving pointer values to disk :-/
#if !defined LUNATIC
    if (g_currentEventExec == EVENT_ANIMATESPRITES)
        initprintf("Line %d: savemapstate called from EVENT_ANIMATESPRITES. WHY?\n", g_errorLineNum);
#endif
    Bmemcpy(&save->spriteext[0],&spriteext[0],sizeof(spriteext_t)*MAXSPRITES);

    save->numsprites = Numsprites;
    save->tailspritefree = tailspritefree;
    Bmemcpy(&save->headspritesect[0],&headspritesect[0],sizeof(headspritesect));
    Bmemcpy(&save->prevspritesect[0],&prevspritesect[0],sizeof(prevspritesect));
    Bmemcpy(&save->nextspritesect[0],&nextspritesect[0],sizeof(nextspritesect));
    Bmemcpy(&save->headspritestat[0],&headspritestat[0],sizeof(headspritestat));
    Bmemcpy(&save->prevspritestat[0],&prevspritestat[0],sizeof(prevspritestat));
    Bmemcpy(&save->nextspritestat[0],&nextspritestat[0],sizeof(nextspritestat));
#ifdef YAX_ENABLE
    Bmemcpy(&save->numyaxbunches, &numyaxbunches, sizeof(numyaxbunches));
# if !defined NEW_MAP_FORMAT
    Bmemcpy(save->yax_bunchnum, yax_bunchnum, sizeof(yax_bunchnum));
    Bmemcpy(save->yax_nextwall, yax_nextwall, sizeof(yax_nextwall));
# endif
#endif
    Bmemcpy(&save->actor[0],&actor[0],sizeof(actor_t)*MAXSPRITES);

    Bmemcpy(&save->g_numCyclers,&g_numCyclers,sizeof(g_numCyclers));
    Bmemcpy(&save->cyclers[0],&cyclers[0],sizeof(cyclers));
    Bmemcpy(&save->g_playerSpawnPoints[0],&g_playerSpawnPoints[0],sizeof(g_playerSpawnPoints));
    Bmemcpy(&save->g_numAnimWalls,&g_numAnimWalls,sizeof(g_numAnimWalls));
    Bmemcpy(&save->SpriteDeletionQueue[0],&SpriteDeletionQueue[0],sizeof(SpriteDeletionQueue));
    Bmemcpy(&save->g_spriteDeleteQueuePos,&g_spriteDeleteQueuePos,sizeof(g_spriteDeleteQueuePos));
    Bmemcpy(&save->animwall[0],&animwall[0],sizeof(animwall));
    Bmemcpy(&save->origins[0],&g_origins[0],sizeof(g_origins));
    Bmemcpy(&save->g_mirrorWall[0],&g_mirrorWall[0],sizeof(g_mirrorWall));
    Bmemcpy(&save->g_mirrorSector[0],&g_mirrorSector[0],sizeof(g_mirrorSector));
    Bmemcpy(&save->g_mirrorCount,&g_mirrorCount,sizeof(g_mirrorCount));
    Bmemcpy(&save->show2dsector[0],&show2dsector[0],sizeof(show2dsector));
    Bmemcpy(&save->g_numClouds,&g_numClouds,sizeof(g_numClouds));
    Bmemcpy(&save->clouds[0],&clouds[0],sizeof(clouds));
    Bmemcpy(&save->cloudx,&cloudx,sizeof(cloudx));
    Bmemcpy(&save->cloudy,&cloudy,sizeof(cloudy));
    Bmemcpy(&save->pskyidx,&g_pskyidx,sizeof(g_pskyidx));
    Bmemcpy(&save->animategoal[0],&animategoal[0],sizeof(animategoal));
    Bmemcpy(&save->animatevel[0],&animatevel[0],sizeof(animatevel));
    Bmemcpy(&save->g_animateCount,&g_animateCount,sizeof(g_animateCount));
    Bmemcpy(&save->animatesect[0],&animatesect[0],sizeof(animatesect));

    G_Util_PtrToIdx(animateptr, g_animateCount, sector, P2I_FWD);
    Bmemcpy(&save->animateptr[0],&animateptr[0],sizeof(animateptr));
    G_Util_PtrToIdx(animateptr, g_animateCount, sector, P2I_BACK);

    {
        EDUKE32_STATIC_ASSERT(sizeof(save->animateptr) == sizeof(animateptr));
    }

    Bmemcpy(&save->g_numPlayerSprites,&g_numPlayerSprites,sizeof(g_numPlayerSprites));
    Bmemcpy(&save->g_earthquakeTime,&g_earthquakeTime,sizeof(g_earthquakeTime));
    Bmemcpy(&save->lockclock,&lockclock,sizeof(lockclock));
    Bmemcpy(&save->randomseed,&randomseed,sizeof(randomseed));
    Bmemcpy(&save->g_globalRandom,&g_globalRandom,sizeof(g_globalRandom));

#if !defined LUNATIC
    for (int i=g_gameVarCount-1; i>=0; i--)
    {
        if (aGameVars[i].nFlags & GAMEVAR_NORESET) continue;
        if (aGameVars[i].nFlags & GAMEVAR_PERPLAYER)
        {
            if (!save->vars[i])
                save->vars[i] = (intptr_t *)Xaligned_alloc(16, MAXPLAYERS * sizeof(intptr_t));
            Bmemcpy(&save->vars[i][0],&aGameVars[i].pValues[0],sizeof(intptr_t) * MAXPLAYERS);
        }
        else if (aGameVars[i].nFlags & GAMEVAR_PERACTOR)
        {
            if (!save->vars[i])
                save->vars[i] = (intptr_t *)Xaligned_alloc(16, MAXSPRITES * sizeof(intptr_t));
            Bmemcpy(&save->vars[i][0],&aGameVars[i].pValues[0],sizeof(intptr_t) * MAXSPRITES);
        }
        else save->vars[i] = (intptr_t *)aGameVars[i].nValue;
    }
#else
    int32_t slen;
    const char *svcode = El_SerializeGamevars(&slen, levelnum);

    if (slen < 0)
    {
        El_OnError("ERROR: savemapstate: serialization failed!");
    }
    else
    {
        char *savecode = Xstrdup(svcode);
        Bfree(save->savecode);
        save->savecode = savecode;
    }
#endif
    ototalclock = totalclock;
}

void G_RestoreMapState(void)
{
    int32_t levelnum = ud.volume_number*MAXLEVELS+ud.level_number;
    mapstate_t *save = MapInfo[levelnum].savedstate;

    if (save != NULL)
    {
        int32_t i, x;
        char phealth[MAXPLAYERS];

        for (i=0; i<playerswhenstarted; i++)
            phealth[i] = sprite[g_player[i].ps->i].extra;

        pub = NUMPAGES;
        pus = NUMPAGES;
        G_UpdateScreenArea();

        Bmemcpy(&numwalls,&save->numwalls,sizeof(numwalls));
        Bmemcpy(&wall[0],&save->wall[0],sizeof(walltype)*MAXWALLS);
        Bmemcpy(&numsectors,&save->numsectors,sizeof(numsectors));
        Bmemcpy(&sector[0],&save->sector[0],sizeof(sectortype)*MAXSECTORS);
        Bmemcpy(&sprite[0],&save->sprite[0],sizeof(spritetype)*MAXSPRITES);
        Bmemcpy(&spriteext[0],&save->spriteext[0],sizeof(spriteext_t)*MAXSPRITES);

        // If we're restoring from EVENT_ANIMATESPRITES, all spriteext[].tspr
        // will be overwritten, so NULL them.
#if !defined LUNATIC
        if (g_currentEventExec == EVENT_ANIMATESPRITES)
        {
            initprintf("Line %d: loadmapstate called from EVENT_ANIMATESPRITES. WHY?\n",g_errorLineNum);
            for (i=0; i<MAXSPRITES; i++)
                spriteext[i].tspr = NULL;
        }
#endif
        Numsprites = save->numsprites;
        tailspritefree = save->tailspritefree;
        Bmemcpy(&headspritesect[0],&save->headspritesect[0],sizeof(headspritesect));
        Bmemcpy(&prevspritesect[0],&save->prevspritesect[0],sizeof(prevspritesect));
        Bmemcpy(&nextspritesect[0],&save->nextspritesect[0],sizeof(nextspritesect));
        Bmemcpy(&headspritestat[0],&save->headspritestat[0],sizeof(headspritestat));
        Bmemcpy(&prevspritestat[0],&save->prevspritestat[0],sizeof(prevspritestat));
        Bmemcpy(&nextspritestat[0],&save->nextspritestat[0],sizeof(nextspritestat));
#ifdef YAX_ENABLE
        Bmemcpy(&numyaxbunches, &save->numyaxbunches, sizeof(numyaxbunches));
# if !defined NEW_MAP_FORMAT
        Bmemcpy(yax_bunchnum, save->yax_bunchnum, sizeof(yax_bunchnum));
        Bmemcpy(yax_nextwall, save->yax_nextwall, sizeof(yax_nextwall));
# endif
#endif
        Bmemcpy(&actor[0],&save->actor[0],sizeof(actor_t)*MAXSPRITES);

        Bmemcpy(&g_numCyclers,&save->g_numCyclers,sizeof(g_numCyclers));
        Bmemcpy(&cyclers[0],&save->cyclers[0],sizeof(cyclers));
        Bmemcpy(&g_playerSpawnPoints[0],&save->g_playerSpawnPoints[0],sizeof(g_playerSpawnPoints));
        Bmemcpy(&g_numAnimWalls,&save->g_numAnimWalls,sizeof(g_numAnimWalls));
        Bmemcpy(&SpriteDeletionQueue[0],&save->SpriteDeletionQueue[0],sizeof(SpriteDeletionQueue));
        Bmemcpy(&g_spriteDeleteQueuePos,&save->g_spriteDeleteQueuePos,sizeof(g_spriteDeleteQueuePos));
        Bmemcpy(&animwall[0],&save->animwall[0],sizeof(animwall));
        Bmemcpy(&g_origins[0],&save->origins[0],sizeof(g_origins));
        Bmemcpy(&g_mirrorWall[0],&save->g_mirrorWall[0],sizeof(g_mirrorWall));
        Bmemcpy(&g_mirrorSector[0],&save->g_mirrorSector[0],sizeof(g_mirrorSector));
        Bmemcpy(&g_mirrorCount,&save->g_mirrorCount,sizeof(g_mirrorCount));
        Bmemcpy(&show2dsector[0],&save->show2dsector[0],sizeof(show2dsector));
        Bmemcpy(&g_numClouds,&save->g_numClouds,sizeof(g_numClouds));
        Bmemcpy(&clouds[0],&save->clouds[0],sizeof(clouds));
        Bmemcpy(&cloudx,&save->cloudx,sizeof(cloudx));
        Bmemcpy(&cloudy,&save->cloudy,sizeof(cloudy));
        Bmemcpy(&g_pskyidx,&save->pskyidx,sizeof(g_pskyidx));
        Bmemcpy(&animategoal[0],&save->animategoal[0],sizeof(animategoal));
        Bmemcpy(&animatevel[0],&save->animatevel[0],sizeof(animatevel));
        Bmemcpy(&g_animateCount,&save->g_animateCount,sizeof(g_animateCount));
        Bmemcpy(&animatesect[0],&save->animatesect[0],sizeof(animatesect));

        Bmemcpy(&animateptr[0],&save->animateptr[0],sizeof(animateptr));
        G_Util_PtrToIdx(animateptr, g_animateCount, sector, P2I_BACK);

        Bmemcpy(&g_numPlayerSprites,&save->g_numPlayerSprites,sizeof(g_numPlayerSprites));
        Bmemcpy(&g_earthquakeTime,&save->g_earthquakeTime,sizeof(g_earthquakeTime));
        Bmemcpy(&lockclock,&save->lockclock,sizeof(lockclock));
        Bmemcpy(&randomseed,&save->randomseed,sizeof(randomseed));
        Bmemcpy(&g_globalRandom,&save->g_globalRandom,sizeof(g_globalRandom));
#if !defined LUNATIC
        for (i=g_gameVarCount-1; i>=0; i--)
        {
            if (aGameVars[i].nFlags & GAMEVAR_NORESET) continue;
            if (aGameVars[i].nFlags & GAMEVAR_PERPLAYER)
            {
                if (!save->vars[i]) continue;
                Bmemcpy(&aGameVars[i].pValues[0],&save->vars[i][0],sizeof(intptr_t) * MAXPLAYERS);
            }
            else if (aGameVars[i].nFlags & GAMEVAR_PERACTOR)
            {
                if (!save->vars[i]) continue;
                Bmemcpy(&aGameVars[i].pValues[0],&save->vars[i][0],sizeof(intptr_t) * MAXSPRITES);
            }
            else aGameVars[i].nValue = (intptr_t)save->vars[i];
        }

        Gv_RefreshPointers();
#else
        if (save->savecode)
        {
            El_RestoreGamevars(save->savecode);
        }
#endif
        // Update g_player[].ps->i (sprite indices of players) to be consistent
        // with just loaded sprites.
        // Otherwise, crashes may ensue: e.g. WGR2 SVN r391, map spiderden:
        // - walk forward (to door leading to other level "Shadowpine Forest")
        // - in new level, walk backward to get back to the Spider Den
        // - walk backward to the door leading to Shadowpine Forest --> crash.
        for (SPRITES_OF(STAT_PLAYER, i))
        {
            int32_t snum = P_Get(i);
            Bassert((unsigned)snum < MAXPLAYERS);
            g_player[snum].ps->i = i;
        }

        for (i=0; i<playerswhenstarted; i++)
            sprite[g_player[i].ps->i].extra = phealth[i];

        if (g_player[myconnectindex].ps->over_shoulder_on != 0)
        {
            CAMERADIST = 0;
            CAMERACLOCK = 0;
            g_player[myconnectindex].ps->over_shoulder_on = 1;
        }

        screenpeek = myconnectindex;

        if (ud.lockout)
        {
            for (x=g_numAnimWalls-1; x>=0; x--)
                switch (DYNAMICTILEMAP(wall[animwall[x].wallnum].picnum))
                {
                case FEMPIC1__STATIC:
                    wall[animwall[x].wallnum].picnum = BLANKSCREEN;
                    break;
                case FEMPIC2__STATIC:
                case FEMPIC3__STATIC:
                    wall[animwall[x].wallnum].picnum = SCREENBREAK6;
                    break;
                }
        }
#if 0
        else
        {
            for (x=g_numAnimWalls-1; x>=0; x--)
                if (wall[animwall[x].wallnum].extra >= 0)
                    wall[animwall[x].wallnum].picnum = wall[animwall[x].wallnum].extra;
        }
#endif
#ifdef YAX_ENABLE
        sv_postyaxload();
#endif
        G_ResetInterpolations();

        Net_ResetPrediction();

        G_ClearFIFO();
        G_ResetTimers(0);
    }
}

#ifdef LUNATIC
void VM_FallSprite(int32_t i)
{
    VM_Fall(i, &sprite[i]);
}

int32_t VM_ResetPlayer2(int32_t snum, int32_t flags)
{
    return VM_ResetPlayer(snum, 0, flags);
}

int32_t VM_CheckSquished2(int32_t i, int32_t snum)
{
    vm.g_i = i;
    vm.g_sp = &sprite[i];
    vm.g_p = snum;
    vm.g_pp = g_player[snum].ps;

    return VM_CheckSquished();
}
#endif

// MYOS* CON commands.
LUNATIC_EXTERN void VM_DrawTileGeneric(int32_t x, int32_t y, int32_t zoom, int32_t tilenum,
    int32_t shade, int32_t orientation, int32_t p)
{
    int32_t a = 0;

    orientation &= (ROTATESPRITE_MAX-1);

    if (orientation&4)
        a = 1024;

    if (!(orientation&ROTATESPRITE_FULL16))
    {
        x<<=16;
        y<<=16;
    }

    rotatesprite_win(x, y, zoom, a, tilenum, shade, p, 2|orientation);
}

#if !defined LUNATIC
void VM_DrawTile(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation)
{
    DukePlayer_t *ps = g_player[screenpeek].ps;
    int32_t p = ps->cursectnum >= 0 ? sector[ps->cursectnum].floorpal : 0;

    VM_DrawTileGeneric(x, y, 65536, tilenum, shade, orientation, p);
}

void VM_DrawTilePal(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation, int32_t p)
{
    VM_DrawTileGeneric(x, y, 65536, tilenum, shade, orientation, p);
}

void VM_DrawTileSmall(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation)
{
    DukePlayer_t *ps = g_player[screenpeek].ps;
    int32_t p = ps->cursectnum >= 0 ? sector[ps->cursectnum].floorpal : 0;

    VM_DrawTileGeneric(x, y, 32768, tilenum, shade, orientation, p);
}

void VM_DrawTilePalSmall(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation, int32_t p)
{
    VM_DrawTileGeneric(x, y, 32768, tilenum, shade, orientation, p);
}
#endif
