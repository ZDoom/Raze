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
#include "compat.h"

#include <time.h>
#include <stdlib.h>
#include <math.h>  // sqrt

#include "build.h"

#include "duke3d.h"
#include "gamedef.h"
#include "gameexec.h"
#include "scriplib.h"
#include "savegame.h"
#include "premap.h"
#include "osdcmds.h"
#include "osd.h"
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

int32_t g_iReturnVarID = -1;     // var ID of "RETURN"
int32_t g_iWeaponVarID = -1;     // var ID of "WEAPON"
int32_t g_iWorksLikeVarID = -1;  // var ID of "WORKSLIKE"
int32_t g_iZRangeVarID = -1;     // var ID of "ZRANGE"
int32_t g_iAngRangeVarID = -1;   // var ID of "ANGRANGE"
int32_t g_iAimAngleVarID = -1;   // var ID of "AUTOAIMANGLE"
int32_t g_iLoTagID = -1;         // var ID of "LOTAG"
int32_t g_iHiTagID = -1;         // var ID of "HITAG"
int32_t g_iTextureID = -1;       // var ID of "TEXTURE"
int32_t g_iThisActorID = -1;     // var ID of "THISACTOR"
int32_t g_iSpriteVarID = -1;
int32_t g_iSectorVarID = -1;
int32_t g_iWallVarID = -1;
int32_t g_iPlayerVarID = -1;
int32_t g_iActorVarID = -1;

GAMEEXEC_STATIC void VM_Execute(int32_t loop);

# include "gamestructures.c"
#endif

#define VM_INSTMASK 0xfff

#define VM_CONDITIONAL(xxx)                                                                                            \
    {                                                                                                                  \
        if ((xxx) || ((insptr = (intptr_t *)*(insptr + 1)) && (((*insptr) & VM_INSTMASK) == CON_ELSE)))                \
        {                                                                                                              \
            insptr += 2;                                                                                               \
            VM_Execute(0);                                                                                             \
        }                                                                                                              \
    }

void VM_ScriptInfo(void)
{
#if !defined LUNATIC
    if (!script)
        return;

    if (insptr)
    {
        initprintf("\n");

        for (intptr_t const *p = insptr - 32; p < insptr + 32; p++)
        {
            if ((int32_t)(p - script) >= g_scriptSize)
                break;

            initprintf("%5d: %3d: ", (int32_t) (p - script), (int32_t) (p - insptr));

            if (*p >> 12 && (*p & VM_INSTMASK) < CON_END)
                initprintf("%5d %s\n", (int32_t) (*p >> 12), keyw[*p & VM_INSTMASK]);
            else
                initprintf("%d\n", (int32_t) *p);
        }

        initprintf("\n");
    }

    if (vm.g_i)
        initprintf("current actor: %d (%d)\n", vm.g_i, TrackerCast(vm.g_sp->picnum));

    initprintf("g_errorLineNum: %d, g_tw: %d\n", g_errorLineNum, g_tw);
#endif
}

static void VM_DeleteSprite(int32_t iActor, int32_t iPlayer)
{
    if (EDUKE32_PREDICT_FALSE((unsigned) iActor >= MAXSPRITES))
        return;

    // if player was set to squish, first stop that...
    if (EDUKE32_PREDICT_FALSE(iPlayer >= 0 && g_player[iPlayer].ps->actorsqu == iActor))
        g_player[iPlayer].ps->actorsqu = -1;

    A_DeleteSprite(iActor);
}

intptr_t *apScriptGameEvent[MAXGAMEEVENTS];

// May recurse, e.g. through EVENT_XXX -> ... -> EVENT_KILLIT
#ifdef LUNATIC
FORCE_INLINE int32_t VM_EventCommon_(int32_t iEventID, int32_t iActor, int32_t iPlayer, int32_t lDist, int32_t iReturn)
{
    const double t = gethiticks();
    int32_t ret = El_CallEvent(&g_ElState, iEventID, iActor, iPlayer, lDist, &iReturn);

    // NOTE: the run times are those of the called event plus any events
    // called by it, *not* "self" time.
    g_eventTotalMs[iEventID] += gethiticks()-t;
    g_eventCalls[iEventID]++;

    if (ret == 1)
        VM_DeleteSprite(iActor, iPlayer);

    return iReturn;
}
#else
FORCE_INLINE int32_t VM_EventCommon_(const int32_t iEventID, const int32_t iActor, const int32_t iPlayer,
                                     const int32_t lDist, int32_t iReturn)
{
    // this is initialized first thing because iActor, iPlayer, lDist, etc are already right there on the stack
    // from the function call
    const vmstate_t tempvm = { iActor, iPlayer, lDist, &actor[(unsigned)iActor].t_data[0],
                               &sprite[(unsigned)iActor], g_player[iPlayer].ps, 0 };

    // since we're targeting C99 and C++ now, we can interweave these to avoid
    // having to load addresses for things twice
    // for example, because we are loading backupReturnVar with the value of
    // aGameVars[g_iReturnVarID].val.lValue, the compiler can avoid having to
    // reload the address of aGameVars[g_iReturnVarID].val.lValue in order to
    // set it to the value of iReturn (...which should still be on the stack!)

    const int32_t backupReturnVar = aGameVars[g_iReturnVarID].val.lValue;
    aGameVars[g_iReturnVarID].val.lValue = iReturn;

    const int32_t backupEventExec = g_currentEventExec;
    g_currentEventExec = iEventID;

    intptr_t const *oinsptr = insptr;
    insptr = apScriptGameEvent[iEventID];

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

    if ((unsigned)iPlayer >= (unsigned)playerswhenstarted)
        vm.g_pp = g_player[0].ps;

    VM_Execute(1);

    if (vm.g_flags & VM_KILL)
        VM_DeleteSprite(vm.g_i, vm.g_p);

    // this needs to happen after VM_DeleteSprite() because VM_DeleteSprite()
    // can trigger additional events
    vm = vm_backup;
    insptr = oinsptr;
    g_currentEventExec = backupEventExec;
    iReturn = aGameVars[g_iReturnVarID].val.lValue;
    aGameVars[g_iReturnVarID].val.lValue = backupReturnVar;

    return iReturn;
}
#endif

// the idea here is that the compiler inlines the call to VM_EventCommon_() and gives us a set of full functions
// which are not only optimized further based on lDist or iReturn (or both) having values known at compile time,
// but are called faster due to having less parameters

int32_t VM_OnEventWithBoth_(int32_t iEventID, int32_t iActor, int32_t iPlayer, int32_t lDist, int32_t iReturn)
{
    return VM_EventCommon_(iEventID, iActor, iPlayer, lDist, iReturn);
}

int32_t VM_OnEventWithReturn_(int32_t iEventID, int32_t iActor, int32_t iPlayer, int32_t iReturn)
{
    return VM_EventCommon_(iEventID, iActor, iPlayer, -1, iReturn);
}

int32_t VM_OnEventWithDist_(int32_t iEventID, int32_t iActor, int32_t iPlayer, int32_t lDist)
{
    return VM_EventCommon_(iEventID, iActor, iPlayer, lDist, 0);
}

int32_t VM_OnEvent_(int32_t iEventID, int32_t iActor, int32_t iPlayer)
{
    return VM_EventCommon_(iEventID, iActor, iPlayer, -1, 0);
}

static int32_t VM_CheckSquished(void)
{
    sectortype const * const sc = &sector[vm.g_sp->sectnum];

    if (sc->lotag == ST_23_SWINGING_DOOR || EDUKE32_PREDICT_FALSE(vm.g_sp->picnum == APLAYER && ud.noclip))
        return 0;

    {
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
    }
    
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
    int32_t n = 128-(krand()&255);

    p->horiz += 64;
    p->return_to_center = 9;
    p->look_ang = p->rotscrnang = n>>1;
}
#endif

int32_t A_Dodge(spritetype *s)
{
    const int32_t mx = s->x, my = s->y;
    const int32_t mxvect = sintable[(s->ang+512)&2047];
    const int32_t myvect = sintable[s->ang&2047];

    if (A_CheckEnemySprite(s) && s->extra <= 0) // hack
        return 0;

    for (int32_t i=headspritestat[STAT_PROJECTILE]; i>=0; i=nextspritestat[i]) //weapons list
    {
        if (OW == i)
            continue;

        int32_t bx = SX-mx;
        int32_t by = SY-my;
        int32_t bxvect = sintable[(SA+512)&2047];
        int32_t byvect = sintable[SA&2047];

        if (mxvect*bx + myvect*by >= 0 && bxvect*bx + byvect*by < 0)
        {
            if (klabs(bxvect*by - byvect*bx) < 65536<<6)
            {
                s->ang -= 512+(krand()&1024);
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
    const int32_t angincs = tabledivide32_noinline(2048, angs);
    hitdata_t hit;

    for (int32_t j=s->ang; j<(2048+s->ang); j+=angincs)
    {
        s->z -= (8<<8);
        hitscan((const vec3_t *)s, s->sectnum,
                sintable[(j+512)&2047],
                sintable[j&2047], 0,
                &hit, CLIPMASK1);
        s->z += (8<<8);

        const int32_t d = klabs(hit.pos.x-s->x) + klabs(hit.pos.y-s->y);

        if (d > greatestd)
        {
            greatestd = d;
            furthest_angle = j;
        }
    }

    return furthest_angle&2047;
}

int32_t A_FurthestVisiblePoint(int32_t iActor, tspritetype * const ts, int32_t *dax, int32_t *day)
{
    if (AC_COUNT(actor[iActor].t_data)&63)
        return -1;

    const spritetype *const s = &sprite[iActor];

    const int32_t angincs =
        ((!g_netServer && ud.multimode < 2) && ud.player_skill < 3) ? 2048/2 :
        tabledivide32_noinline(2048, 1+(krand()&1));

    hitdata_t hit;

    for (int32_t j=ts->ang; j<(2048+ts->ang); j+=(angincs-(krand()&511)))
    {
        ts->z -= (16<<8);
        hitscan((const vec3_t *)ts, ts->sectnum,
                sintable[(j+512)&2047],
                sintable[j&2047], 16384-(krand()&32767),
                &hit, CLIPMASK1);

        ts->z += (16<<8);

        const int32_t d = klabs(hit.pos.x-ts->x)+klabs(hit.pos.y-ts->y);
        const int32_t da = klabs(hit.pos.x-s->x)+klabs(hit.pos.y-s->y);

        if (d < da && hit.sect > -1)
            if (cansee(hit.pos.x,hit.pos.y,hit.pos.z,
                       hit.sect,s->x,s->y,s->z-(16<<8),s->sectnum))
            {
                *dax = hit.pos.x;
                *day = hit.pos.y;
                return hit.sect;
            }
    }

    return -1;
}

static void VM_GetZRange(int32_t iActor, int32_t *ceilhit, int32_t *florhit, int walldist)
{
    spritetype *const s = &sprite[iActor];
    const int32_t ocstat = s->cstat;

    s->cstat = 0;
    s->z -= ZOFFSET;

    getzrange((vec3_t *)s, s->sectnum,
              &actor[iActor].ceilingz, ceilhit,
              &actor[iActor].floorz, florhit,
              walldist, CLIPMASK0);

    s->z += ZOFFSET;
    s->cstat = ocstat;
}

void A_GetZLimits(int32_t iActor)
{
    spritetype *const s = &sprite[iActor];

    int32_t ceilhit, florhit;
    const int walldist = (s->statnum == STAT_PROJECTILE) ? 4 : 127;

    VM_GetZRange(iActor, &ceilhit, &florhit, walldist);

    actor[iActor].flags &= ~SFLAG_NOFLOORSHADOW;

    if ((florhit&49152) == 49152 && (sprite[florhit&(MAXSPRITES-1)].cstat&48) == 0)
    {
        const spritetype *hitspr = &sprite[florhit&(MAXSPRITES-1)];

        florhit &= (MAXSPRITES-1);

        // If a non-projectile would fall onto non-frozen enemy OR an enemy onto a player...
        if ((A_CheckEnemySprite(hitspr) && hitspr->pal != 1 && s->statnum != STAT_PROJECTILE)
                || (hitspr->picnum == APLAYER && A_CheckEnemySprite(s)))
        {
            actor[iActor].flags |= SFLAG_NOFLOORSHADOW;  // No shadows on actors
            s->xvel = -256;
            A_SetSprite(iActor, CLIPMASK0);
        }
        else if (s->statnum == STAT_PROJECTILE && hitspr->picnum == APLAYER && s->owner==florhit)
        {
            actor[iActor].ceilingz = sector[s->sectnum].ceilingz;
            actor[iActor].floorz   = sector[s->sectnum].floorz;
        }
    }
}

void A_Fall(int32_t iActor)
{
    spritetype *const s = &sprite[iActor];
    int c = g_spriteGravity;

    if (EDUKE32_PREDICT_FALSE(G_CheckForSpaceFloor(s->sectnum)))
        c = 0;
    else if (sector[s->sectnum].lotag == ST_2_UNDERWATER || EDUKE32_PREDICT_FALSE(G_CheckForSpaceCeiling(s->sectnum)))
        c = g_spriteGravity/6;

    if (s->statnum == STAT_ACTOR || s->statnum == STAT_PLAYER || s->statnum == STAT_ZOMBIEACTOR || s->statnum == STAT_STANDABLE)
    {
        int32_t ceilhit, florhit;
        VM_GetZRange(iActor, &ceilhit, &florhit, 127);
    }
    else
    {
        actor[iActor].ceilingz = sector[s->sectnum].ceilingz;
        actor[iActor].floorz   = sector[s->sectnum].floorz;
    }

#ifdef YAX_ENABLE
    int16_t fbunch = (sector[s->sectnum].floorstat&512) ? -1 : yax_getbunch(s->sectnum, YAX_FLOOR);
#endif

    if (s->z < actor[iActor].floorz-ZOFFSET
#ifdef YAX_ENABLE
            || fbunch >= 0
#endif
       )
    {
        if (sector[s->sectnum].lotag == ST_2_UNDERWATER && s->zvel > 3122)
            s->zvel = 3144;
        s->z += s->zvel = min(6144, s->zvel+c);
    }

#ifdef YAX_ENABLE
    if (fbunch >= 0)
        setspritez(iActor, (vec3_t *)s);
    else
#endif
        if (s->z >= actor[iActor].floorz-ZOFFSET)
        {
            s->z = actor[iActor].floorz-ZOFFSET;
            s->zvel = 0;
        }
}

int32_t G_GetAngleDelta(int32_t a,int32_t na)
{
    a &= 2047;
    na &= 2047;

    if (klabs(a-na) < 1024)
    {
//        OSD_Printf("G_GetAngleDelta() returning %d\n",na-a);
        return (na-a);
    }

    if (na > 1024) na -= 2048;
    if (a > 1024) a -= 2048;

//    OSD_Printf("G_GetAngleDelta() returning %d\n",na-a);
    return (na-a);
}

GAMEEXEC_STATIC void VM_AlterAng(int32_t movflags)
{
    const int32_t ticselapsed = (AC_COUNT(vm.g_t))&31;

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
        int32_t aang = vm.g_sp->ang, angdif, goalang;
        int32_t j = vm.g_pp->holoduke_on;

        // NOTE: looks like 'owner' is set to target sprite ID...

        if (j >= 0 && cansee(sprite[j].x,sprite[j].y,sprite[j].z,sprite[j].sectnum,vm.g_sp->x,vm.g_sp->y,vm.g_sp->z,vm.g_sp->sectnum))
            vm.g_sp->owner = j;
        else vm.g_sp->owner = vm.g_pp->i;

        if (sprite[vm.g_sp->owner].picnum == APLAYER)
            goalang = getangle(actor[vm.g_i].lastvx-vm.g_sp->x,actor[vm.g_i].lastvy-vm.g_sp->y);
        else
            goalang = getangle(sprite[vm.g_sp->owner].x-vm.g_sp->x,sprite[vm.g_sp->owner].y-vm.g_sp->y);

        if (vm.g_sp->xvel && vm.g_sp->picnum != DRONE)
        {
            angdif = G_GetAngleDelta(aang,goalang);

            if (ticselapsed < 2)
            {
                if (klabs(angdif) < 256)
                {
                    j = 128-(krand()&256);
                    vm.g_sp->ang += j;
                    if (A_GetHitscanRange(vm.g_i) < 844)
                        vm.g_sp->ang -= j;
                }
            }
            else if (ticselapsed > 18 && ticselapsed < GAMETICSPERSEC) // choose
            {
                if (klabs(angdif>>2) < 128) vm.g_sp->ang = goalang;
                else vm.g_sp->ang += angdif>>2;
            }
        }
        else vm.g_sp->ang = goalang;
    }

    if (ticselapsed < 1)
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

static inline void VM_AddAngle(int32_t shr, int32_t goalang)
{
    int32_t angdif = G_GetAngleDelta(vm.g_sp->ang,goalang)>>shr;

    if ((angdif > -8 && angdif < 0) || (angdif < 8 && angdif > 0))
        angdif *= 2;

    vm.g_sp->ang += angdif;
}

static void VM_FacePlayer(int32_t shr)
{
    int32_t goalang;

    if (vm.g_pp->newowner >= 0)
        goalang = getangle(vm.g_pp->opos.x-vm.g_sp->x, vm.g_pp->opos.y-vm.g_sp->y);
    else
        goalang = getangle(vm.g_pp->pos.x-vm.g_sp->x, vm.g_pp->pos.y-vm.g_sp->y);

    VM_AddAngle(shr, goalang);
}

////////// TROR get*zofslope //////////
// These rather belong into the engine.

static int32_t VM_GetCeilZOfSlope(void)
{
    const int dax = vm.g_sp->x, day = vm.g_sp->y;
    const int sectnum = vm.g_sp->sectnum;

#ifdef YAX_ENABLE
    if ((sector[sectnum].ceilingstat&512)==0)
    {
        int32_t nsect = yax_getneighborsect(dax, day, sectnum, YAX_CEILING);
        if (nsect >= 0)
            return getceilzofslope(nsect, dax, day);
    }
#endif
    return getceilzofslope(sectnum, dax, day);
}

static int32_t VM_GetFlorZOfSlope(void)
{
    const int dax = vm.g_sp->x, day = vm.g_sp->y;
    const int sectnum = vm.g_sp->sectnum;

#ifdef YAX_ENABLE
    if ((sector[sectnum].floorstat&512)==0)
    {
        int32_t nsect = yax_getneighborsect(dax, day, sectnum, YAX_FLOOR);
        if (nsect >= 0)
            return getflorzofslope(nsect, dax, day);
    }
#endif
    return getflorzofslope(sectnum, dax, day);
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
    const int32_t movflags = /*(*movflagsptr==-1) ? 0 :*/ *movflagsptr;
    const int32_t deadflag = (A_CheckEnemySprite(vm.g_sp) && vm.g_sp->extra <= 0);

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
        int32_t newx = vm.g_pp->pos.x + (vm.g_pp->vel.x/768);
        int32_t newy = vm.g_pp->pos.y + (vm.g_pp->vel.y/768);
        int32_t goalang = getangle(newx-vm.g_sp->x,newy-vm.g_sp->y);
        VM_AddAngle(2, goalang);
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
        int32_t daxvel = vm.g_sp->xvel;
        int32_t angdif = vm.g_sp->ang;

        if (badguyp && vm.g_sp->picnum != ROTATEGUN)
        {
            if ((vm.g_sp->picnum == DRONE || vm.g_sp->picnum == COMMANDER) && vm.g_sp->extra > 0)
            {
                if (vm.g_sp->picnum == COMMANDER)
                {
                    int32_t l;
                    // NOTE: COMMANDER updates both actor[].floorz and
                    // .ceilingz regardless of its zvel.
                    actor[vm.g_i].floorz = l = VM_GetFlorZOfSlope();
                    if (vm.g_sp->z > l-(8<<8))
                    {
                        vm.g_sp->z = l-(8<<8);
                        vm.g_sp->zvel = 0;
                    }

                    actor[vm.g_i].ceilingz = l = VM_GetCeilZOfSlope();
                    if (vm.g_sp->z < l+(80<<8))
                    {
                        vm.g_sp->z = l+(80<<8);
                        vm.g_sp->zvel = 0;
                    }
                }
                else
                {
                    int32_t l;
                    // The DRONE updates either .floorz or .ceilingz, not both.
                    if (vm.g_sp->zvel > 0)
                    {
                        actor[vm.g_i].floorz = l = VM_GetFlorZOfSlope();
                        if (vm.g_sp->z > l-(30<<8))
                            vm.g_sp->z = l-(30<<8);
                    }
                    else
                    {
                        actor[vm.g_i].ceilingz = l = VM_GetCeilZOfSlope();
                        if (vm.g_sp->z < l+(50<<8))
                        {
                            vm.g_sp->z = l+(50<<8);
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
                    const int32_t l = VM_GetCeilZOfSlope();

                    if (vm.g_sp->z < l+(66<<8))
                    {
                        vm.g_sp->z = l+(66<<8);
                        vm.g_sp->zvel >>= 1;
                    }
                }
            }

            if (vm.g_x < 960 && vm.g_sp->xrepeat > 16)
            {
                daxvel = -(1024 - vm.g_x);
                angdif = getangle(vm.g_pp->pos.x - vm.g_sp->x, vm.g_pp->pos.y - vm.g_sp->y);

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
                    daxvel <<= 1;
                }
            }
        }
        else if (vm.g_sp->picnum == APLAYER)
            if (vm.g_sp->z < actor[vm.g_i].ceilingz+(32<<8))
                vm.g_sp->z = actor[vm.g_i].ceilingz+(32<<8);

        vec3_t tmpvect = { (daxvel * (sintable[(angdif + 512) & 2047])) >> 14,
                           (daxvel * (sintable[angdif & 2047])) >> 14, vm.g_sp->zvel };

        actor[vm.g_i].movflag =
            A_MoveSprite(vm.g_i, &tmpvect, (A_CheckSpriteFlags(vm.g_i, SFLAG_NOCLIP) ? 0 : CLIPMASK0));
    }

    if (!badguyp)
        return;

    vm.g_sp->shade += (sector[vm.g_sp->sectnum].ceilingstat & 1) ?
                      (sector[vm.g_sp->sectnum].ceilingshade - vm.g_sp->shade) >> 1 :
                      (sector[vm.g_sp->sectnum].floorshade - vm.g_sp->shade) >> 1;
}

static void P_AddWeaponMaybeSwitch(DukePlayer_t *ps, int32_t weap)
{
    if ((ps->weaponswitch & (1|4)) == (1|4))
    {
        const int32_t snum = P_Get(ps->i);
        int32_t i, new_wchoice = -1, curr_wchoice = -1;

        for (i=0; i<=FREEZE_WEAPON && (new_wchoice < 0 || curr_wchoice < 0); i++)
        {
            int32_t w = g_player[snum].wchoice[i];

            if (w == KNEE_WEAPON)
                w = FREEZE_WEAPON;
            else w--;

            if (w == ps->curr_weapon)
                curr_wchoice = i;
            if (w == weap)
                new_wchoice = i;
        }

        P_AddWeapon(ps, weap, (new_wchoice < curr_wchoice));
    }
    else
    {
        P_AddWeapon(ps, weap, (ps->weaponswitch & 1));
    }
}

#if defined LUNATIC
void P_AddWeaponMaybeSwitchI(int32_t snum, int32_t weap)
{
    P_AddWeaponMaybeSwitch(g_player[snum].ps, weap);
}
#else
static void P_AddWeaponAmmoCommon(DukePlayer_t *ps, int32_t weap, int32_t amount)
{
    P_AddAmmo(weap, ps, amount);

    if (PWEAPON(vm.g_p, ps->curr_weapon, WorksLike) == KNEE_WEAPON && (ps->gotweapon & (1 << weap)))
        P_AddWeaponMaybeSwitch(ps, weap);
}

static int32_t VM_AddWeapon(int32_t weap, int32_t amount, DukePlayer_t *ps)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)weap >= MAX_WEAPONS))
    {
        CON_ERRPRINTF("Invalid weapon ID %d\n", weap);
        return 1;
    }

    if ((ps->gotweapon & (1 << weap)) == 0)
    {
        P_AddWeaponMaybeSwitch(ps, weap);
    }
    else if (ps->ammo_amount[weap] >= ps->max_ammo_amount[weap])
    {
        vm.g_flags |= VM_NOEXECUTE;
        return 2;
    }

    P_AddWeaponAmmoCommon(ps, weap, amount);

    return 0;
}
#endif

static int32_t A_GetVerticalVel(const actor_t *ac)
{
#ifdef LUNATIC
    return ac->mv.vvel;
#else
    int32_t moveScriptOfs = AC_MOVE_ID(ac->t_data);

    if ((unsigned)moveScriptOfs < (unsigned)g_scriptSize-1)
        return script[moveScriptOfs + 1];
    else
        return 0;
#endif
}

static int32_t A_GetWaterZOffset(int spritenum)
{
    const spritetype *const sp = &sprite[spritenum];
    const actor_t *const ac = &actor[spritenum];

    if (sector[sp->sectnum].lotag == ST_1_ABOVE_WATER)
    {
        if (A_CheckSpriteFlags(spritenum, SFLAG_NOWATERDIP))
            return 0;

        // fix for flying/jumping monsters getting stuck in water
        if ((AC_MOVFLAGS(sp, ac) & jumptoplayer_only) ||
            (G_HaveActor(sp->picnum) && A_GetVerticalVel(ac) != 0))
            return 0;

        return ACTOR_ONWATER_ADDZ;
    }

    return 0;
}

static void VM_Fall(int32_t g_i, spritetype *g_sp)
{
    int32_t grav = g_spriteGravity;

    g_sp->xoffset = g_sp->yoffset = 0;

    if (sector[g_sp->sectnum].lotag == ST_2_UNDERWATER || EDUKE32_PREDICT_FALSE(G_CheckForSpaceCeiling(g_sp->sectnum)))
        grav = g_spriteGravity/6;
    else if (EDUKE32_PREDICT_FALSE(G_CheckForSpaceFloor(g_sp->sectnum)))
        grav = 0;

    if (!actor[g_i].cgg-- || (sector[g_sp->sectnum].floorstat&2))
    {
        A_GetZLimits(g_i);
        actor[g_i].cgg = 3;
    }

    if (g_sp->z < actor[g_i].floorz-ZOFFSET)
    {
        // Free fall.
        g_sp->zvel = min(g_sp->zvel+grav, ACTOR_MAXFALLINGZVEL);
        int32_t z = g_sp->z + g_sp->zvel;

#ifdef YAX_ENABLE
        if (yax_getbunch(g_sp->sectnum, YAX_FLOOR) >= 0 &&
                (sector[g_sp->sectnum].floorstat&512)==0)
            setspritez(g_i, (vec3_t *)g_sp);
        else
#endif
            if (z > actor[g_i].floorz - ZOFFSET)
                z = actor[g_i].floorz - ZOFFSET;

        g_sp->z = z;
        return;
    }

    // Preliminary new z position of the actor.
    int32_t z = actor[g_i].floorz - ZOFFSET;

    if (A_CheckEnemySprite(g_sp) || (g_sp->picnum == APLAYER && g_sp->owner >= 0))
    {
        if (g_sp->zvel > 3084 && g_sp->extra <= 1)
        {
            // I'm guessing this DRONE check is from a beta version of the game
            // where they crashed into the ground when killed
            if (!(g_sp->picnum == APLAYER && g_sp->extra > 0) && g_sp->pal != 1 && g_sp->picnum != DRONE)
            {
                A_DoGuts(g_i,JIBS6,15);
                A_PlaySound(SQUISHED,g_i);
                A_Spawn(g_i,BLOODPOOL);
            }

            actor[g_i].picnum = SHOTSPARK1;
            actor[g_i].extra = 1;
            g_sp->zvel = 0;
        }
        else if (g_sp->zvel > 2048 && sector[g_sp->sectnum].lotag != ST_1_ABOVE_WATER)
        {
            int16_t newsect = g_sp->sectnum;

            pushmove((vec3_t *)g_sp, &newsect, 128, 4<<8, 4<<8, CLIPMASK0);
            if ((unsigned)newsect < MAXSECTORS)
                changespritesect(g_i, newsect);

            A_PlaySound(THUD, g_i);
        }
    }

    if (sector[g_sp->sectnum].lotag == ST_1_ABOVE_WATER)
    {
        g_sp->z = z + A_GetWaterZOffset(g_i);
        return;
    }

    g_sp->z = z;
    g_sp->zvel = 0;
}

static int32_t VM_ResetPlayer(int32_t g_p, int32_t g_flags)
{
    //AddLog("resetplayer");
    if (!g_netServer && ud.multimode < 2)
    {
        if (g_lastSaveSlot >= 0 && ud.recstat != 2)
        {
            M_OpenMenu(g_p);
            KB_ClearKeyDown(sc_Space);
            I_AdvanceTriggerClear();
            M_ChangeMenu(MENU_RESETPLAYER);
        }
        else g_player[g_p].ps->gm = MODE_RESTART;
#if !defined LUNATIC
        g_flags |= VM_NOEXECUTE;
#endif
    }
    else
    {
        if (g_p == myconnectindex)
        {
            CAMERADIST = 0;
            CAMERACLOCK = totalclock;
        }

        if (g_fakeMultiMode)
            P_ResetPlayer(g_p);
#ifndef NETCODE_DISABLE
        if (g_netServer)
        {
            P_ResetPlayer(g_p);
            Net_SpawnPlayer(g_p);
        }
#endif
    }

    P_UpdateScreenPal(g_player[g_p].ps);
    //AddLog("EOF: resetplayer");

    return g_flags;
}

void G_GetTimeDate(int32_t *vals)
{
    time_t rawtime;
    struct tm *ti;

    time(&rawtime);
    ti=localtime(&rawtime);
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

int32_t G_StartTrack(int32_t level)
{
    if ((unsigned)level < MAXLEVELS)
    {
        int32_t musicIndex = MAXLEVELS*ud.volume_number + level;

        if (MapInfo[musicIndex].musicfn != NULL)
        {
            // Only set g_musicIndex on success.
            g_musicIndex = musicIndex;
            S_PlayMusic(MapInfo[musicIndex].musicfn);

            return 0;
        }
    }

    return 1;
}

LUNATIC_EXTERN void G_ShowView(int32_t x, int32_t y, int32_t z, int32_t a, int32_t horiz, int32_t sect,
                               int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t unbiasedp)
{
    int32_t smoothratio = calc_smoothratio(totalclock, ototalclock);
#ifdef USE_OPENGL
    int32_t oprojhacks;
#endif

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
    oprojhacks = glprojectionhacks;
    glprojectionhacks = 0;
#endif
    {
        int32_t o = newaspect_enable;
        newaspect_enable = r_usenewaspect;
        setaspect_new_use_dimen = 1;

        setview(x1,y1,x2,y2);

        setaspect_new_use_dimen = 0;
        newaspect_enable = o;
    }

    G_DoInterpolations(smoothratio);

    G_HandleMirror(x, y, z, a, horiz, smoothratio);
#ifdef POLYMER
    if (getrendermode() == REND_POLYMER)
        polymer_setanimatesprites(G_DoSpriteAnimations, x,y,a,smoothratio);
#endif
    yax_preparedrawrooms();
    drawrooms(x,y,z,a,horiz,sect);
    yax_drawrooms(G_DoSpriteAnimations, sect, 0, smoothratio);

    display_mirror = 2;
    G_DoSpriteAnimations(x,y,a,smoothratio);
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
            insptr++;
            loop++;
            continue;
        }
        else if (tw == CON_RIGHTBRACE)
        {
            insptr++;
            loop--;
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
                int32_t q = *insptr++, i = *insptr++;
                if (EDUKE32_PREDICT_FALSE((ScriptQuotes[q] == NULL || ScriptQuoteRedefinitions[i] == NULL)))
                {
                    CON_ERRPRINTF("%d %d null quote\n", q,i);
                    break;
                }
                Bstrcpy(ScriptQuotes[q],ScriptQuoteRedefinitions[i]);
                continue;
            }

        case CON_GETTHISPROJECTILE:
        case CON_SETTHISPROJECTILE:
            insptr++;
            {
                // syntax [gs]etplayer[<var>].x <VAR>
                // <varid> <xxxid> <varid>
                int32_t lVar1=*insptr++, lLabelID=*insptr++, lVar2=*insptr++;

                VM_AccessActiveProjectile(tw==CON_SETTHISPROJECTILE,lVar1,lLabelID,lVar2);
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

                int32_t sclip = 768, angdif = 16;

                if (A_CheckEnemySprite(vm.g_sp) && vm.g_sp->xrepeat > 56)
                {
                    sclip = 3084;
                    angdif = 48;
                }

#define CHECK(x) if (x >= 0 && sprite[x].picnum == vm.g_sp->picnum) { VM_CONDITIONAL(0); continue; }
#define CHECK2(x) do { vm.g_sp->ang += x; tw = A_CheckHitSprite(vm.g_i, &temphit); vm.g_sp->ang -= x; } while(0)

                if (tw > sclip)
                {
                    CHECK(temphit);
                    CHECK2(angdif);

                    if (tw > sclip)
                    {
                        CHECK(temphit);
                        CHECK2(-angdif);

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
            tspritetype *s = (tspritetype *)&sprite[ps->i];

            // select sprite for monster to target
            // if holoduke is on, let them target holoduke first.
            //
            if (ps->holoduke_on >= 0)
            {
                s = (tspritetype *)&sprite[ps->holoduke_on];
                tw = cansee(vm.g_sp->x,vm.g_sp->y,vm.g_sp->z-(krand()&((32<<8)-1)),vm.g_sp->sectnum,
                           s->x,s->y,s->z,s->sectnum);

                if (tw == 0)
                {
                    // they can't see player's holoduke
                    // check for player...
                    s = (tspritetype *)&sprite[ps->i];
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
                    int32_t j = 0;
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
            if (!S_CheckSoundPlaying(vm.g_i,*insptr++))
                A_PlaySound(*(insptr-1),vm.g_i);
            continue;

        case CON_IFACTORSOUND:
            insptr++;
            {
                int32_t i = Gv_GetVarX(*insptr++), j = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)j >= MAXSOUNDS))
                {
                    CON_ERRPRINTF("Invalid sound %d\n", j);
                    insptr++;
                    continue;
                }
                insptr--;
                VM_CONDITIONAL(A_CheckSoundPlaying(i,j));
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
                int32_t i = Gv_GetVarX(*insptr++), j = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)j>=MAXSOUNDS))
                {
                    CON_ERRPRINTF("Invalid sound %d\n", j);
                    continue;
                }

                if (A_CheckSoundPlaying(i,j))
                    S_StopEnvSound(j,i);

                continue;
            }

        case CON_SETACTORSOUNDPITCH:
            insptr++;
            {
                int32_t i = Gv_GetVarX(*insptr++), j = Gv_GetVarX(*insptr++), pitchoffset = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)j>=MAXSOUNDS))
                {
                    CON_ERRPRINTF("Invalid sound %d\n", j);
                    continue;
                }

                S_ChangeSoundPitch(j,i,pitchoffset);

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
                int32_t weap=*insptr++, amount=*insptr++;

                if (EDUKE32_PREDICT_FALSE((unsigned)weap >= MAX_WEAPONS))
                {
                    CON_ERRPRINTF("Invalid weapon ID %d\n", weap);
                    break;
                }

                if (ps->ammo_amount[weap] >= ps->max_ammo_amount[weap])
                {
                    vm.g_flags |= VM_NOEXECUTE;
                    return;
                }

                P_AddWeaponAmmoCommon(ps, weap, amount);

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
                int32_t weap=*insptr++, amount=*insptr++;
                VM_AddWeapon(weap, amount, ps);

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
                int32_t j;

                if (ps->newowner >= 0)
                    G_ClearCameraView(ps);

                j = sprite[ps->i].extra;

                if (vm.g_sp->picnum != ATOMICHEALTH)
                {
                    if (j > ps->max_player_health && *insptr > 0)
                    {
                        insptr++;
                        continue;
                    }
                    else
                    {
                        if (j > 0)
                            j += *insptr;
                        if (j > ps->max_player_health && *insptr > 0)
                            j = ps->max_player_health;
                    }
                }
                else
                {
                    if (j > 0)
                        j += *insptr;
                    if (j > (ps->max_player_health<<1))
                        j = (ps->max_player_health<<1);
                }

                if (j < 0) j = 0;

                if (ud.god == 0)
                {
                    if (*insptr > 0)
                    {
                        if ((j - *insptr) < (ps->max_player_health>>2) &&
                                j >= (ps->max_player_health>>2))
                            A_PlaySound(DUKE_GOTHEALTHATLOW,ps->i);

                        ps->last_extra = j;
                    }

                    sprite[ps->i].extra = j;
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
                int32_t weap=Gv_GetVarX(*insptr++), amount=Gv_GetVarX(*insptr++);
                VM_AddWeapon(weap, amount, ps);
                continue;
            }

        case CON_ACTIVATEBYSECTOR:
        case CON_OPERATESECTORS:
        case CON_OPERATEACTIVATORS:
        case CON_SETASPECT:
        case CON_SSP:
            insptr++;
            {
                int32_t var1 = Gv_GetVarX(*insptr++), var2;
                if (tw == CON_OPERATEACTIVATORS && *insptr == g_iThisActorID)
                {
                    var2 = vm.g_p;
                    insptr++;
                }
                else var2 = Gv_GetVarX(*insptr++);

                switch (tw)
                {
                case CON_ACTIVATEBYSECTOR:
                    if (EDUKE32_PREDICT_FALSE((unsigned)var1 >= (unsigned)numsectors))
                    {
                        CON_ERRPRINTF("Invalid sector %d\n", var1);
                        break;
                    }
                    G_ActivateBySector(var1, var2);
                    break;
                case CON_OPERATESECTORS:
                    if (EDUKE32_PREDICT_FALSE((unsigned)var1 >= (unsigned)numsectors))
                    {
                        CON_ERRPRINTF("Invalid sector %d\n", var1);
                        break;
                    }
                    G_OperateSectors(var1, var2);
                    break;
                case CON_OPERATEACTIVATORS:
                    if (EDUKE32_PREDICT_FALSE((unsigned)var2>=(unsigned)playerswhenstarted))
                    {
                        CON_ERRPRINTF("Invalid player %d\n", var2);
                        break;
                    }
                    G_OperateActivators(var1, var2);
                    break;
                case CON_SETASPECT:
                    setaspect(var1, var2);
                    break;
                case CON_SSP:
                    if (EDUKE32_PREDICT_FALSE((unsigned)var1 >= MAXSPRITES))
                    {
                        CON_ERRPRINTF("Invalid sprite %d\n", var1);
                        break;
                    }
                    A_SetSprite(var1, var2);
                    break;
                }
                continue;
            }

        case CON_CANSEESPR:
            insptr++;
            {
                int32_t lVar1 = Gv_GetVarX(*insptr++), lVar2 = Gv_GetVarX(*insptr++), res;

                if (EDUKE32_PREDICT_FALSE((unsigned)lVar1 >= MAXSPRITES || (unsigned)lVar2 >= MAXSPRITES))
                {
                    CON_ERRPRINTF("Invalid sprite %d\n", (unsigned)lVar1 >= MAXSPRITES ? lVar1 : lVar2);
                    res=0;
                }
                else res=cansee(sprite[lVar1].x,sprite[lVar1].y,sprite[lVar1].z,sprite[lVar1].sectnum,
                                    sprite[lVar2].x,sprite[lVar2].y,sprite[lVar2].z,sprite[lVar2].sectnum);

                Gv_SetVarX(*insptr++, res);
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
            aGameVars[g_iReturnVarID].val.lValue = G_CheckActivatorMotion(Gv_GetVarX(*insptr++));
            continue;

        case CON_INSERTSPRITEQ:
            insptr++;
            A_AddToDeleteQueue(vm.g_i);
            continue;

        case CON_QSTRLEN:
            insptr++;
            {
                int32_t i=*insptr++;
                int32_t j=Gv_GetVarX(*insptr++);
                if (EDUKE32_PREDICT_FALSE(ScriptQuotes[j] == NULL))
                {
                    CON_ERRPRINTF("null quote %d\n", j);
                    Gv_SetVarX(i,-1);
                    continue;
                }
                Gv_SetVarX(i,Bstrlen(ScriptQuotes[j]));
                continue;
            }

        case CON_QSTRDIM:
            insptr++;
            {
                vec2_t dim = { 0, 0, };

                int32_t w=*insptr++;
                int32_t h=*insptr++;

                int32_t tilenum = Gv_GetVarX(*insptr++);
                int32_t x=Gv_GetVarX(*insptr++), y=Gv_GetVarX(*insptr++), z = Gv_GetVarX(*insptr++);
                int32_t blockangle=Gv_GetVarX(*insptr++);
                int32_t q=Gv_GetVarX(*insptr++);
                int32_t orientation=Gv_GetVarX(*insptr++);
                int32_t xspace=Gv_GetVarX(*insptr++), yline=Gv_GetVarX(*insptr++);
                int32_t xbetween=Gv_GetVarX(*insptr++), ybetween=Gv_GetVarX(*insptr++);
                int32_t f=Gv_GetVarX(*insptr++);
                int32_t x1=Gv_GetVarX(*insptr++), y1=Gv_GetVarX(*insptr++);
                int32_t x2=Gv_GetVarX(*insptr++), y2=Gv_GetVarX(*insptr++);

                orientation &= (ROTATESPRITE_MAX-1);

                if (EDUKE32_PREDICT_FALSE(tilenum < 0 || tilenum+255 >= MAXTILES))
                    CON_ERRPRINTF("invalid base tilenum %d\n", tilenum);
                else if (EDUKE32_PREDICT_FALSE((unsigned)q >= MAXQUOTES || ScriptQuotes[q] == NULL))
                    CON_ERRPRINTF("invalid quote ID %d\n", q);
                else
                    dim = G_ScreenTextSize(tilenum,x,y,z,blockangle,ScriptQuotes[q],2|orientation,xspace,yline,xbetween,ybetween,f,x1,y1,x2,y2);

                Gv_SetVarX(w,dim.x);
                Gv_SetVarX(h,dim.y);
                continue;
            }

        case CON_HEADSPRITESTAT:
            insptr++;
            {
                int32_t i=*insptr++;
                int32_t j=Gv_GetVarX(*insptr++);
                if (EDUKE32_PREDICT_FALSE((unsigned)j > MAXSTATUS))
                {
                    CON_ERRPRINTF("invalid status list %d\n", j);
                    continue;
                }
                Gv_SetVarX(i,headspritestat[j]);
                continue;
            }

        case CON_PREVSPRITESTAT:
            insptr++;
            {
                int32_t i=*insptr++;
                int32_t j=Gv_GetVarX(*insptr++);
                if (EDUKE32_PREDICT_FALSE((unsigned)j >= MAXSPRITES))
                {
                    CON_ERRPRINTF("invalid sprite ID %d\n", j);
                    continue;
                }
                Gv_SetVarX(i,prevspritestat[j]);
                continue;
            }

        case CON_NEXTSPRITESTAT:
            insptr++;
            {
                int32_t i=*insptr++;
                int32_t j=Gv_GetVarX(*insptr++);
                if (EDUKE32_PREDICT_FALSE((unsigned)j >= MAXSPRITES))
                {
                    CON_ERRPRINTF("invalid sprite ID %d\n", j);
                    continue;
                }
                Gv_SetVarX(i,nextspritestat[j]);
                continue;
            }

        case CON_HEADSPRITESECT:
            insptr++;
            {
                int32_t i=*insptr++;
                int32_t j=Gv_GetVarX(*insptr++);
                if (EDUKE32_PREDICT_FALSE((unsigned)j >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("invalid sector %d\n", j);
                    continue;
                }
                Gv_SetVarX(i,headspritesect[j]);
                continue;
            }

        case CON_PREVSPRITESECT:
            insptr++;
            {
                int32_t i=*insptr++;
                int32_t j=Gv_GetVarX(*insptr++);
                if (EDUKE32_PREDICT_FALSE((unsigned)j >= MAXSPRITES))
                {
                    CON_ERRPRINTF("invalid sprite ID %d\n", j);
                    continue;
                }
                Gv_SetVarX(i,prevspritesect[j]);
                continue;
            }

        case CON_NEXTSPRITESECT:
            insptr++;
            {
                int32_t i=*insptr++;
                int32_t j=Gv_GetVarX(*insptr++);
                if (EDUKE32_PREDICT_FALSE((unsigned)j >= MAXSPRITES))
                {
                    CON_ERRPRINTF("invalid sprite ID %d\n", j);
                    continue;
                }
                Gv_SetVarX(i,nextspritesect[j]);
                continue;
            }

        case CON_GETKEYNAME:
            insptr++;
            {
                int32_t i = Gv_GetVarX(*insptr++),
                        f = Gv_GetVarX(*insptr++);
                int32_t j = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)i >= MAXQUOTES || ScriptQuotes[i] == NULL))
                {
                    CON_ERRPRINTF("invalid quote ID %d\n", i);
                    continue;
                }
                else if (EDUKE32_PREDICT_FALSE((unsigned)f >= NUMGAMEFUNCTIONS))
                {
                    CON_ERRPRINTF("invalid function %d\n", f);
                    continue;
                }
                else
                {
                    if (j < 2)
                        Bstrcpy(tempbuf,KB_ScanCodeToString(ud.config.KeyboardKeys[f][j]));
                    else
                    {
                        Bstrcpy(tempbuf,KB_ScanCodeToString(ud.config.KeyboardKeys[f][0]));

                        if (!*tempbuf)
                            Bstrcpy(tempbuf,KB_ScanCodeToString(ud.config.KeyboardKeys[f][1]));
                    }
                }

                if (*tempbuf)
                    Bstrcpy(ScriptQuotes[i],tempbuf);

                continue;
            }

        case CON_QSUBSTR:
            insptr++;
            {
                int32_t q1 = Gv_GetVarX(*insptr++);
                int32_t q2 = Gv_GetVarX(*insptr++);
                int32_t st = Gv_GetVarX(*insptr++);
                int32_t ln = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)q1>=MAXQUOTES || ScriptQuotes[q1] == NULL))
                {
                    CON_ERRPRINTF("invalid quote ID %d\n", q1);
                    continue;
                }
                if (EDUKE32_PREDICT_FALSE((unsigned)q2>=MAXQUOTES || ScriptQuotes[q2] == NULL))
                {
                    CON_ERRPRINTF("invalid quote ID %d\n", q2);
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE((unsigned)st >= MAXQUOTELEN))
                {
                    CON_ERRPRINTF("invalid start position %d\n", st);
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE(ln < 0))
                {
                    CON_ERRPRINTF("invalid length %d\n", ln);
                    continue;
                }

                char *s1 = ScriptQuotes[q1];
                char *s2 = ScriptQuotes[q2];

                while (*s2 && st--) s2++;
                while ((*s1 = *s2) && ln--)
                {
                    s1++;
                    s2++;
                }
                *s1 = 0;

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
                int32_t i = Gv_GetVarX(*insptr++);
                int32_t j = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)i >= MAXSPRITES))
                {
                    CON_ERRPRINTF("Invalid sprite: %d\n", i);
                    continue;
                }
                if (EDUKE32_PREDICT_FALSE((unsigned)j >= MAXSTATUS))
                {
                    CON_ERRPRINTF("Invalid statnum: %d\n", j);
                    continue;
                }
                if (sprite[i].statnum == j)
                    continue;

                /* initialize actor data when changing to an actor statnum because there's usually
                garbage left over from being handled as a hard coded object */

                if (sprite[i].statnum > STAT_ZOMBIEACTOR && (j == STAT_ACTOR || j == STAT_ZOMBIEACTOR))
                {
                    actor_t * const a = &actor[i];

                    a->lastvx = 0;
                    a->lastvy = 0;
                    a->timetosleep = 0;
                    a->cgg = 0;
                    a->movflag = 0;
                    a->tempang = 0;
                    a->dispicnum = 0;
                    T1=T2=T3=T4=T5=T6=T7=T8=T9=0;
                    a->flags = 0;
                    sprite[i].hitag = 0;

                    if (G_HaveActor(sprite[i].picnum))
                    {
                        const intptr_t *actorptr = g_tile[sprite[i].picnum].execPtr;
                        // offsets
                        AC_ACTION_ID(a->t_data) = actorptr[1];
                        AC_MOVE_ID(a->t_data) = actorptr[2];
                        AC_MOVFLAGS(&sprite[i], &actor[i]) = actorptr[3];  // ai bits (movflags)
                    }
                }

                changespritestat(i,j);
                continue;
            }

        case CON_STARTLEVEL:
            insptr++; // skip command
            {
                // from 'level' cheat in game.c (about line 6250)
                int32_t volnume=Gv_GetVarX(*insptr++), levnume=Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)volnume >= MAXVOLUMES))
                {
                    CON_ERRPRINTF("invalid volume (%d)\n", volnume);
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE((unsigned)levnume >= MAXLEVELS))
                {
                    CON_ERRPRINTF("invalid level (%d)\n", levnume);
                    continue;
                }

                ud.m_volume_number = ud.volume_number = volnume;
                ud.m_level_number = ud.level_number = levnume;
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
                int32_t x=Gv_GetVarX(*insptr++), y=Gv_GetVarX(*insptr++), tilenum=Gv_GetVarX(*insptr++);
                int32_t shade=Gv_GetVarX(*insptr++), orientation=Gv_GetVarX(*insptr++);

                switch (tw)
                {
                case CON_MYOS:
                    G_DrawTile(x,y,tilenum,shade,orientation);
                    break;
                case CON_MYOSPAL:
                {
                    int32_t pal=Gv_GetVarX(*insptr++);
                    G_DrawTilePal(x,y,tilenum,shade,orientation,pal);
                    break;
                }
                case CON_MYOSX:
                    G_DrawTileSmall(x,y,tilenum,shade,orientation);
                    break;
                case CON_MYOSPALX:
                {
                    int32_t pal=Gv_GetVarX(*insptr++);
                    G_DrawTilePalSmall(x,y,tilenum,shade,orientation,pal);
                    break;
                }
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
                int32_t lValue = Gv_GetVarX(*insptr++), lEnd = *insptr++, lCases = *insptr++;
                intptr_t const *lpDefault = insptr++, *lpCases = insptr;
                int32_t lCheckCase, left = 0, right = lCases - 1;
                insptr += lCases << 1;

                do
                {
                    lCheckCase = (left + right) >> 1;

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
        case CON_ENDEVENT:
            insptr++;
            return;

        case CON_DISPLAYRAND:
            insptr++;
            Gv_SetVarX(*insptr++, system_15bit_rand());
            continue;

        case CON_DRAGPOINT:
            insptr++;
            {
                int32_t wallnum = Gv_GetVarX(*insptr++), newx = Gv_GetVarX(*insptr++), newy = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)wallnum >= (unsigned)numwalls))
                {
                    CON_ERRPRINTF("Invalid wall %d\n", wallnum);
                    continue;
                }

                dragpoint(wallnum,newx,newy,0);
                continue;
            }

        case CON_LDIST:
            insptr++;
            {
                int32_t distvar = *insptr++, xvar = Gv_GetVarX(*insptr++), yvar = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)xvar >= MAXSPRITES || (unsigned)yvar >= MAXSPRITES))
                {
                    CON_ERRPRINTF("invalid sprite %d %d\n", xvar, yvar);
                    continue;
                }

                Gv_SetVarX(distvar, ldist(&sprite[xvar],&sprite[yvar]));
                continue;
            }

        case CON_DIST:
            insptr++;
            {
                int32_t distvar = *insptr++, xvar = Gv_GetVarX(*insptr++), yvar = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)xvar >= MAXSPRITES || (unsigned)yvar >= MAXSPRITES))
                {
                    CON_ERRPRINTF("invalid sprite %d %d\n", xvar, yvar);
                    continue;
                }

                Gv_SetVarX(distvar, dist(&sprite[xvar],&sprite[yvar]));
                continue;
            }

        case CON_GETANGLE:
            insptr++;
            {
                int32_t angvar = *insptr++;
                int32_t xvar = Gv_GetVarX(*insptr++);
                int32_t yvar = Gv_GetVarX(*insptr++);

                Gv_SetVarX(angvar, getangle(xvar,yvar));
                continue;
            }

        case CON_GETINCANGLE:
            insptr++;
            {
                int32_t angvar = *insptr++;
                int32_t xvar = Gv_GetVarX(*insptr++);
                int32_t yvar = Gv_GetVarX(*insptr++);

                Gv_SetVarX(angvar, G_GetAngleDelta(xvar,yvar));
                continue;
            }

        case CON_MULSCALE:
            insptr++;
            {
                int32_t var1 = *insptr++, var2 = Gv_GetVarX(*insptr++);
                int32_t var3 = Gv_GetVarX(*insptr++), var4 = Gv_GetVarX(*insptr++);

                Gv_SetVarX(var1, mulscale(var2, var3, var4));
                continue;
            }

        case CON_INITTIMER:
            insptr++;
            G_InitTimer(Gv_GetVarX(*insptr++));
            continue;

        case CON_TIME:
            insptr += 2;
            continue;

        case CON_ESPAWNVAR:
        case CON_EQSPAWNVAR:
        case CON_QSPAWNVAR:
            insptr++;
            {
                int32_t lIn=Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)vm.g_sp->sectnum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector %d\n", TrackerCast(vm.g_sp->sectnum));
                    continue;
                }
                int32_t j = A_Spawn(vm.g_i, lIn);

                switch (tw)
                {
                case CON_EQSPAWNVAR:
                    if (j != -1)
                        A_AddToDeleteQueue(j);
                case CON_ESPAWNVAR:
                    aGameVars[g_iReturnVarID].val.lValue = j;
                    break;

                case CON_QSPAWNVAR:
                    if (j != -1)
                        A_AddToDeleteQueue(j);
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

                int32_t j = A_Spawn(vm.g_i,*insptr++);

                switch (tw)
                {
                case CON_EQSPAWN:
                    if (j != -1)
                        A_AddToDeleteQueue(j);
                case CON_ESPAWN:
                    aGameVars[g_iReturnVarID].val.lValue = j;
                    break;
                case CON_QSPAWN:
                    if (j != -1)
                        A_AddToDeleteQueue(j);
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
                const int32_t zvel = (tw == CON_ESHOOT) ?
                    SHOOT_HARDCODED_ZVEL : (int16_t)Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)vm.g_sp->sectnum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector %d\n", TrackerCast(vm.g_sp->sectnum));
                    insptr++;
                    continue;
                }

                int32_t j = A_ShootWithZvel(vm.g_i,*insptr++,zvel);

                if (tw != CON_ZSHOOT)
                    aGameVars[g_iReturnVarID].val.lValue = j;
            }
            continue;

        case CON_SHOOTVAR:
        case CON_ESHOOTVAR:
            insptr++;
            {
                int32_t j=Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)vm.g_sp->sectnum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector %d\n", TrackerCast(vm.g_sp->sectnum));
                    continue;
                }

                j = A_Shoot(vm.g_i, j);
                if (tw == CON_ESHOOTVAR)
                    aGameVars[g_iReturnVarID].val.lValue = j;
                continue;
            }

        case CON_EZSHOOTVAR:
        case CON_ZSHOOTVAR:
            insptr++;
            {
                const int32_t zvel = (int16_t)Gv_GetVarX(*insptr++);
                int32_t j=Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)vm.g_sp->sectnum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector %d\n", TrackerCast(vm.g_sp->sectnum));
                    continue;
                }

                j = A_ShootWithZvel(vm.g_i, j, zvel);
                if (tw == CON_EZSHOOTVAR)
                    aGameVars[g_iReturnVarID].val.lValue = j;
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
                int32_t j=Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)j>=MAXSOUNDS))
                {
                    CON_ERRPRINTF("Invalid sound %d\n", j);
                    continue;
                }

                switch (tw)
                {
                    case CON_SOUNDONCEVAR: // falls through to CON_SOUNDVAR
                        if (!S_CheckSoundPlaying(vm.g_i, j))
                    case CON_SOUNDVAR:
                        A_PlaySound((int16_t)j, vm.g_i);
                        continue;
                    case CON_GLOBALSOUNDVAR:
                        A_PlaySound((int16_t)j, g_player[screenpeek].ps->i);
                        continue;
                    case CON_STOPSOUNDVAR:
                        if (S_CheckSoundPlaying(vm.g_i, j))
                            S_StopSound((int16_t)j);
                        continue;
                    case CON_SCREENSOUND:
                        A_PlaySound(j, -1);
                        continue;
                }
            }
            continue;

        case CON_CUTSCENE:
        case CON_IFCUTSCENE:
            insptr++;
            {
                int32_t j = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)j >= MAXQUOTES || ScriptQuotes[j] == NULL))
                {
                    CON_ERRPRINTF("invalid quote ID %d for anim!\n", j);
                    continue;
                }

                if (tw == CON_IFCUTSCENE)
                {
                    VM_CONDITIONAL(g_animPtr == G_FindAnim(ScriptQuotes[j]));
                    continue;
                }

                tw = ps->palette;
                G_PlayAnim(ScriptQuotes[j]);
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
            int32_t i=0;
            insptr++;
            if (ud.config.scripthandle < 0)
            {
                insptr++;
                continue;
            }
            switch (tw)
            {
            case CON_SAVEGAMEVAR:
                i=Gv_GetVarX(*insptr);
                SCRIPT_PutNumber(ud.config.scripthandle, "Gamevars",aGameVars[*insptr++].szLabel,i,FALSE,FALSE);
                break;
            case CON_READGAMEVAR:
                SCRIPT_GetNumber(ud.config.scripthandle, "Gamevars",aGameVars[*insptr].szLabel,&i);
                Gv_SetVarX(*insptr++, i);
                break;
            }
            continue;
        }

        case CON_SHOWVIEW:
        case CON_SHOWVIEWUNBIASED:
            insptr++;
            {
                int32_t x=Gv_GetVarX(*insptr++);
                int32_t y=Gv_GetVarX(*insptr++);
                int32_t z=Gv_GetVarX(*insptr++);
                int32_t a=Gv_GetVarX(*insptr++);
                int32_t horiz=Gv_GetVarX(*insptr++);
                int32_t sect=Gv_GetVarX(*insptr++);
                int32_t x1=Gv_GetVarX(*insptr++);
                int32_t y1=Gv_GetVarX(*insptr++);
                int32_t x2=Gv_GetVarX(*insptr++);
                int32_t y2=Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE(x1 < 0 || y1 < 0 || x2 >= 320 || y2 >= 200))
                {
                    CON_ERRPRINTF("incorrect coordinates\n");
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE((unsigned)sect >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector %d\n", sect);
                    continue;
                }

                G_ShowView(x, y, z, a, horiz, sect, x1, y1, x2, y2, (tw != CON_SHOWVIEW));

                continue;
            }

        case CON_ROTATESPRITEA:
        case CON_ROTATESPRITE16:
        case CON_ROTATESPRITE:
            insptr++;
            {
                int32_t x=Gv_GetVarX(*insptr++),   y=Gv_GetVarX(*insptr++),           z=Gv_GetVarX(*insptr++);
                int32_t a=Gv_GetVarX(*insptr++),   tilenum=Gv_GetVarX(*insptr++),     shade=Gv_GetVarX(*insptr++);
                int32_t pal=Gv_GetVarX(*insptr++), orientation=Gv_GetVarX(*insptr++);
                int32_t alpha = (tw == CON_ROTATESPRITEA) ? Gv_GetVarX(*insptr++) : 0;
                int32_t x1=Gv_GetVarX(*insptr++),  y1=Gv_GetVarX(*insptr++);
                int32_t x2=Gv_GetVarX(*insptr++),  y2=Gv_GetVarX(*insptr++);

                int32_t blendidx = 0;

                if (tw != CON_ROTATESPRITE16 && !(orientation&ROTATESPRITE_FULL16))
                {
                    x<<=16;
                    y<<=16;
                }

                if (EDUKE32_PREDICT_FALSE((unsigned)tilenum >= MAXTILES))
                {
                    CON_ERRPRINTF("invalid tilenum %d\n", tilenum);
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE(x < -(320<<16) || x >= (640<<16) || y < -(200<<16) || y >= (400<<16)))
                {
                    CON_ERRPRINTF("invalid coordinates: %d, %d\n", x, y);
                    continue;
                }

                orientation &= (ROTATESPRITE_MAX-1);

                NEG_ALPHA_TO_BLEND(alpha, blendidx, orientation);

                rotatesprite_(x,y,z,a,tilenum,shade,pal,2|orientation,alpha,blendidx,x1,y1,x2,y2);
                continue;
            }

        case CON_GAMETEXT:
        case CON_GAMETEXTZ:
            insptr++;
            {
                int32_t tilenum = Gv_GetVarX(*insptr++);
                int32_t x=Gv_GetVarX(*insptr++), y=Gv_GetVarX(*insptr++), q=Gv_GetVarX(*insptr++);
                int32_t shade=Gv_GetVarX(*insptr++), pal=Gv_GetVarX(*insptr++);
                int32_t orientation=Gv_GetVarX(*insptr++);
                int32_t x1=Gv_GetVarX(*insptr++), y1=Gv_GetVarX(*insptr++);
                int32_t x2=Gv_GetVarX(*insptr++), y2=Gv_GetVarX(*insptr++);
                int32_t z = (tw == CON_GAMETEXTZ) ? Gv_GetVarX(*insptr++) : 65536;

                if (EDUKE32_PREDICT_FALSE(tilenum < 0 || tilenum+255 >= MAXTILES))
                {
                    CON_ERRPRINTF("invalid base tilenum %d\n", tilenum);
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE((unsigned)q >= MAXQUOTES || ScriptQuotes[q] == NULL))
                {
                    CON_ERRPRINTF("invalid quote ID %d\n", q);
                    continue;
                }

                orientation &= (ROTATESPRITE_MAX-1);

                G_PrintGameText(0,tilenum,x>>1,y,ScriptQuotes[q],shade,pal,orientation,x1,y1,x2,y2,z,0);
                continue;
            }

        case CON_DIGITALNUMBER:
        case CON_DIGITALNUMBERZ:
            insptr++;
            {
                int32_t tilenum = Gv_GetVarX(*insptr++);
                int32_t x=Gv_GetVarX(*insptr++), y=Gv_GetVarX(*insptr++), q=Gv_GetVarX(*insptr++);
                int32_t shade=Gv_GetVarX(*insptr++), pal=Gv_GetVarX(*insptr++);
                int32_t orientation=Gv_GetVarX(*insptr++);
                int32_t x1=Gv_GetVarX(*insptr++), y1=Gv_GetVarX(*insptr++);
                int32_t x2=Gv_GetVarX(*insptr++), y2=Gv_GetVarX(*insptr++);
                int32_t z = (tw == CON_DIGITALNUMBERZ) ? Gv_GetVarX(*insptr++) : 65536;

                // NOTE: '-' not taken into account, but we have rotatesprite() bound check now anyway
                if (EDUKE32_PREDICT_FALSE(tilenum < 0 || tilenum+9 >= MAXTILES))
                {
                    CON_ERRPRINTF("invalid base tilenum %d\n", tilenum);
                    continue;
                }

                G_DrawTXDigiNumZ(tilenum,x,y,q,shade,pal,orientation,x1,y1,x2,y2,z);
                continue;
            }

        case CON_MINITEXT:
            insptr++;
            {
                int32_t x=Gv_GetVarX(*insptr++), y=Gv_GetVarX(*insptr++), q=Gv_GetVarX(*insptr++);
                int32_t shade=Gv_GetVarX(*insptr++), pal=Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)q >= MAXQUOTES || ScriptQuotes[q] == NULL))
                {
                    CON_ERRPRINTF("invalid quote ID %d\n", q);
                    continue;
                }

                minitextshade(x,y,ScriptQuotes[q],shade,pal, 2+8+16);
                continue;
            }

        case CON_SCREENTEXT:
            insptr++;
            {
                int32_t tilenum = Gv_GetVarX(*insptr++);
                int32_t x=Gv_GetVarX(*insptr++), y=Gv_GetVarX(*insptr++), z = Gv_GetVarX(*insptr++);
                int32_t blockangle=Gv_GetVarX(*insptr++), charangle=Gv_GetVarX(*insptr++);
                int32_t q=Gv_GetVarX(*insptr++);
                int32_t shade=Gv_GetVarX(*insptr++), pal=Gv_GetVarX(*insptr++);
                int32_t orientation=Gv_GetVarX(*insptr++);
                int32_t alpha=Gv_GetVarX(*insptr++);
                int32_t xspace=Gv_GetVarX(*insptr++), yline=Gv_GetVarX(*insptr++);
                int32_t xbetween=Gv_GetVarX(*insptr++), ybetween=Gv_GetVarX(*insptr++);
                int32_t f=Gv_GetVarX(*insptr++);
                int32_t x1=Gv_GetVarX(*insptr++), y1=Gv_GetVarX(*insptr++);
                int32_t x2=Gv_GetVarX(*insptr++), y2=Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE(tilenum < 0 || tilenum+255 >= MAXTILES))
                {
                    CON_ERRPRINTF("invalid base tilenum %d\n", tilenum);
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE((unsigned)q >= MAXQUOTES || ScriptQuotes[q] == NULL))
                {
                    CON_ERRPRINTF("invalid quote ID %d\n", q);
                    continue;
                }

                orientation &= (ROTATESPRITE_MAX-1);

                G_ScreenText(tilenum,x,y,z,blockangle,charangle,ScriptQuotes[q],shade,pal,2|orientation,alpha,xspace,yline,xbetween,ybetween,f,x1,y1,x2,y2);
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

                vect.x = Gv_GetVarX(*insptr++);
                vect.y = Gv_GetVarX(*insptr++);
                vect.z = Gv_GetVarX(*insptr++);

                int32_t sectnum = Gv_GetVarX(*insptr++);
                int32_t ceilzvar = *insptr++, ceilhitvar = *insptr++, florzvar = *insptr++, florhitvar = *insptr++;
                int32_t walldist = Gv_GetVarX(*insptr++), clipmask = Gv_GetVarX(*insptr++);
                int32_t ceilz, ceilhit, florz, florhit;

                if (EDUKE32_PREDICT_FALSE((unsigned)sectnum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector %d\n", sectnum);
                    continue;
                }

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
                int32_t sectnum = Gv_GetVarX(*insptr++);

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
                int64_t dax=Gv_GetVarX(*insptr++), day=Gv_GetVarX(*insptr++);
                int64_t hypsq = dax*dax + day*day;

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
                int32_t x1=Gv_GetVarX(*insptr++), y1=Gv_GetVarX(*insptr++), z1=Gv_GetVarX(*insptr++);
                int32_t x2=Gv_GetVarX(*insptr++), y2=Gv_GetVarX(*insptr++), z2=Gv_GetVarX(*insptr++);
                int32_t x3=Gv_GetVarX(*insptr++), y3=Gv_GetVarX(*insptr++), x4=Gv_GetVarX(*insptr++), y4=Gv_GetVarX(*insptr++);
                int32_t intxvar=*insptr++, intyvar=*insptr++, intzvar=*insptr++, retvar=*insptr++;
                int32_t intx, inty, intz, ret;

                if (tw==CON_LINEINTERSECT)
                    ret = lintersect(x1, y1, z1, x2, y2, z2, x3, y3, x4, y4, &intx, &inty, &intz);
                else
                    ret = rayintersect(x1, y1, z1, x2, y2, z2, x3, y3, x4, y4, &intx, &inty, &intz);

                Gv_SetVarX(retvar, ret);
                if (ret)
                {
                    Gv_SetVarX(intxvar, intx);
                    Gv_SetVarX(intyvar, inty);
                    Gv_SetVarX(intzvar, intz);
                }

                continue;
            }

        case CON_CLIPMOVE:
        case CON_CLIPMOVENOSLIDE:
            insptr++;
            {
                int32_t retvar=*insptr++, xvar=*insptr++, yvar=*insptr++, z=Gv_GetVarX(*insptr++), sectnumvar=*insptr++;
                int32_t xvect=Gv_GetVarX(*insptr++), yvect=Gv_GetVarX(*insptr++);
                int32_t walldist=Gv_GetVarX(*insptr++), floordist=Gv_GetVarX(*insptr++), ceildist=Gv_GetVarX(*insptr++);
                int32_t clipmask=Gv_GetVarX(*insptr++);
                int16_t sectnum;

                vec3_t vect;
                vect.x = Gv_GetVarX(xvar);
                vect.y = Gv_GetVarX(yvar);
                vect.z = z;
                sectnum = Gv_GetVarX(sectnumvar);

                if (EDUKE32_PREDICT_FALSE((unsigned)sectnum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector %d\n", sectnum);
                    Gv_SetVarX(retvar, 0);
                    continue;
                }

                Gv_SetVarX(retvar, clipmovex(&vect, &sectnum, xvect, yvect, walldist, floordist, ceildist,
                                             clipmask, (tw==CON_CLIPMOVENOSLIDE)));
                Gv_SetVarX(sectnumvar, sectnum);
                Gv_SetVarX(xvar, vect.x);
                Gv_SetVarX(yvar, vect.y);

                continue;
            }

        case CON_HITSCAN:
            insptr++;
            {
                vec3_t vect;

                vect.x = Gv_GetVarX(*insptr++);
                vect.y = Gv_GetVarX(*insptr++);
                vect.z = Gv_GetVarX(*insptr++);

                int32_t sectnum = Gv_GetVarX(*insptr++);
                int32_t vx = Gv_GetVarX(*insptr++), vy = Gv_GetVarX(*insptr++), vz = Gv_GetVarX(*insptr++);
                int32_t hitsectvar = *insptr++, hitwallvar = *insptr++, hitspritevar = *insptr++;
                int32_t hitxvar = *insptr++, hityvar = *insptr++, hitzvar = *insptr++, cliptype = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)sectnum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector %d\n", sectnum);
                    continue;
                }

                hitdata_t hit;
                hitscan((const vec3_t *)&vect, sectnum, vx, vy, vz, &hit, cliptype);

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
                int32_t x1=Gv_GetVarX(*insptr++), y1=Gv_GetVarX(*insptr++), z1=Gv_GetVarX(*insptr++);
                int32_t sect1=Gv_GetVarX(*insptr++);
                int32_t x2=Gv_GetVarX(*insptr++), y2=Gv_GetVarX(*insptr++), z2=Gv_GetVarX(*insptr++);
                int32_t sect2=Gv_GetVarX(*insptr++), rvar=*insptr++;

                if (EDUKE32_PREDICT_FALSE((unsigned)sect1 >= (unsigned)numsectors || (unsigned)sect2 >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector\n");
                    Gv_SetVarX(rvar, 0);
                }

                Gv_SetVarX(rvar, cansee(x1,y1,z1,sect1,x2,y2,z2,sect2));
                continue;
            }

        case CON_ROTATEPOINT:
            insptr++;
            {
                int32_t xpivot=Gv_GetVarX(*insptr++), ypivot=Gv_GetVarX(*insptr++);
                int32_t x=Gv_GetVarX(*insptr++), y=Gv_GetVarX(*insptr++), daang=Gv_GetVarX(*insptr++);
                int32_t x2var=*insptr++, y2var=*insptr++;
                int32_t x2, y2;

                rotatepoint(xpivot,ypivot,x,y,daang,&x2,&y2);
                Gv_SetVarX(x2var, x2);
                Gv_SetVarX(y2var, y2);
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

                int32_t x=Gv_GetVarX(*insptr++), y=Gv_GetVarX(*insptr++), z=Gv_GetVarX(*insptr++);
                int32_t sectnum=Gv_GetVarX(*insptr++), ang=Gv_GetVarX(*insptr++);
                int32_t neartagsectorvar=*insptr++, neartagwallvar=*insptr++, neartagspritevar=*insptr++, neartaghitdistvar=*insptr++;
                int32_t neartagrange=Gv_GetVarX(*insptr++), tagsearch=Gv_GetVarX(*insptr++);

                int16_t neartagsector, neartagwall, neartagsprite;
                int32_t neartaghitdist;

                if (EDUKE32_PREDICT_FALSE((unsigned)sectnum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector %d\n", sectnum);
                    continue;
                }
                neartag(x, y, z, sectnum, ang, &neartagsector, &neartagwall, &neartagsprite,
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
        case CON_SETSPRITE:
            insptr++;
            {
                int32_t spritenum = Gv_GetVarX(*insptr++);
                vec3_t davector;

                davector.x = Gv_GetVarX(*insptr++);
                davector.y = Gv_GetVarX(*insptr++);
                davector.z = Gv_GetVarX(*insptr++);

                if (tw == CON_SETSPRITE)
                {
                    if (EDUKE32_PREDICT_FALSE((unsigned)spritenum >= MAXSPRITES))
                    {
                        CON_ERRPRINTF("invalid sprite ID %d\n", spritenum);
                        continue;
                    }
                    setsprite(spritenum, &davector);
                    continue;
                }

                {
                    int32_t cliptype = Gv_GetVarX(*insptr++);

                    if (EDUKE32_PREDICT_FALSE((unsigned)spritenum >= MAXSPRITES))
                    {
                        CON_ERRPRINTF("invalid sprite ID %d\n", spritenum);
                        insptr++;
                        continue;
                    }
                    Gv_SetVarX(*insptr++, A_MoveSprite(spritenum, &davector, cliptype));
                    continue;
                }
            }

        case CON_GETFLORZOFSLOPE:
        case CON_GETCEILZOFSLOPE:
            insptr++;
            {
                int32_t sectnum = Gv_GetVarX(*insptr++), x = Gv_GetVarX(*insptr++), y = Gv_GetVarX(*insptr++);
                if (EDUKE32_PREDICT_FALSE((unsigned)sectnum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector %d\n", sectnum);
                    insptr++;
                    continue;
                }

                if (tw == CON_GETFLORZOFSLOPE)
                {
                    Gv_SetVarX(*insptr++, getflorzofslope(sectnum,x,y));
                    continue;
                }
                Gv_SetVarX(*insptr++, getceilzofslope(sectnum,x,y));
                continue;
            }

        case CON_UPDATESECTOR:
        case CON_UPDATESECTORZ:
            insptr++;
            {
                int32_t x=Gv_GetVarX(*insptr++), y=Gv_GetVarX(*insptr++);
                int32_t z=(tw==CON_UPDATESECTORZ)?Gv_GetVarX(*insptr++):0;
                int32_t var=*insptr++;
                int16_t w=sprite[vm.g_i].sectnum;

                if (tw==CON_UPDATESECTOR) updatesector(x,y,&w);
                else updatesectorz(x,y,z,&w);

                Gv_SetVarX(var, w);
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
                    time_t curtime = time(NULL);
                    struct tm *timeptr = localtime(&curtime);
                    Bsnprintf(ud.savegame[g_lastSaveSlot], sizeof(ud.savegame[g_lastSaveSlot]), "Auto %.4d%.2d%.2d %.2d%.2d%.2d\n",
                    timeptr->tm_year + 1900, timeptr->tm_mon, timeptr->tm_mday,
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
        {
            insptr++;
            vm.g_flags = VM_ResetPlayer(vm.g_p, vm.g_flags);
        }
        continue;

        case CON_IFONWATER:
            VM_CONDITIONAL(sector[vm.g_sp->sectnum].lotag == ST_1_ABOVE_WATER && klabs(vm.g_sp->z-sector[vm.g_sp->sectnum].floorz) < (32<<8));
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
            switch (*(insptr-1))
            {
            case GET_STEROIDS:
                ps->inv_amount[GET_STEROIDS] = *insptr;
                ps->inven_icon = ICON_STEROIDS;
                break;

            case GET_SHIELD:
                ps->inv_amount[GET_SHIELD] += *insptr;// 100;
                if (ps->inv_amount[GET_SHIELD] > ps->max_shield_amount)
                    ps->inv_amount[GET_SHIELD] = ps->max_shield_amount;
                break;

            case GET_SCUBA:
                ps->inv_amount[GET_SCUBA] = *insptr;// 1600;
                ps->inven_icon = ICON_SCUBA;
                break;

            case GET_HOLODUKE:
                ps->inv_amount[GET_HOLODUKE] = *insptr;// 1600;
                ps->inven_icon = ICON_HOLODUKE;
                break;

            case GET_JETPACK:
                ps->inv_amount[GET_JETPACK] = *insptr;// 1600;
                ps->inven_icon = ICON_JETPACK;
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

            case GET_HEATS:
                ps->inv_amount[GET_HEATS] = *insptr;
                ps->inven_icon = ICON_HEATS;
                break;

            case GET_FIRSTAID:
                ps->inven_icon = ICON_FIRSTAID;
                ps->inv_amount[GET_FIRSTAID] = *insptr;
                break;

            case GET_BOOTS:
                ps->inven_icon = ICON_BOOTS;
                ps->inv_amount[GET_BOOTS] = *insptr;
                break;
            default:
                CON_ERRPRINTF("Invalid inventory ID %d\n", (int32_t)*(insptr-1));
                break;
            }
            insptr++;
            continue;
        }

        case CON_HITRADIUSVAR:
            insptr++;
            {
                int32_t v1=Gv_GetVarX(*insptr++),v2=Gv_GetVarX(*insptr++),v3=Gv_GetVarX(*insptr++);
                int32_t v4=Gv_GetVarX(*insptr++),v5=Gv_GetVarX(*insptr++);
                A_RadiusDamage(vm.g_i,v1,v2,v3,v4,v5);
            }
            continue;

        case CON_HITRADIUS:
            A_RadiusDamage(vm.g_i,*(insptr+1),*(insptr+2),*(insptr+3),*(insptr+4),*(insptr+5));
            insptr += 6;
            continue;

        case CON_IFP:
        {
            int32_t l = *(++insptr);
            int32_t j = 0;
            int32_t s = sprite[ps->i].xvel;

            if ((l&8) && ps->on_ground && TEST_SYNC_KEY(g_player[vm.g_p].sync->bits, SK_CROUCH))
                j = 1;
            else if ((l&16) && ps->jumping_counter == 0 && !ps->on_ground &&
                     ps->vel.z > 2048)
                j = 1;
            else if ((l&32) && ps->jumping_counter > 348)
                j = 1;
            else if ((l&1) && s >= 0 && s < 8)
                j = 1;
            else if ((l&2) && s >= 8 && !TEST_SYNC_KEY(g_player[vm.g_p].sync->bits, SK_RUN))
                j = 1;
            else if ((l&4) && s >= 8 && TEST_SYNC_KEY(g_player[vm.g_p].sync->bits, SK_RUN))
                j = 1;
            else if ((l&64) && ps->pos.z < (vm.g_sp->z-(48<<8)))
                j = 1;
            else if ((l&128) && s <= -8 && !TEST_SYNC_KEY(g_player[vm.g_p].sync->bits, SK_RUN))
                j = 1;
            else if ((l&256) && s <= -8 && TEST_SYNC_KEY(g_player[vm.g_p].sync->bits, SK_RUN))
                j = 1;
            else if ((l&512) && (ps->quick_kick > 0 || (PWEAPON(vm.g_p, ps->curr_weapon, WorksLike) == KNEE_WEAPON && ps->kickback_pic > 0)))
                j = 1;
            else if ((l&1024) && sprite[ps->i].xrepeat < 32)
                j = 1;
            else if ((l&2048) && ps->jetpack_on)
                j = 1;
            else if ((l&4096) && ps->inv_amount[GET_STEROIDS] > 0 && ps->inv_amount[GET_STEROIDS] < 400)
                j = 1;
            else if ((l&8192) && ps->on_ground)
                j = 1;
            else if ((l&16384) && sprite[ps->i].xrepeat > 32 && sprite[ps->i].extra > 0 && ps->timebeforeexit == 0)
                j = 1;
            else if ((l&32768) && sprite[ps->i].extra <= 0)
                j = 1;
            else if ((l&65536L))
            {
                if (vm.g_sp->picnum == APLAYER && (g_netServer || ud.multimode > 1))
                    j = G_GetAngleDelta(g_player[otherp].ps->ang,getangle(ps->pos.x-g_player[otherp].ps->pos.x,ps->pos.y-g_player[otherp].ps->pos.y));
                else
                    j = G_GetAngleDelta(ps->ang,getangle(vm.g_sp->x-ps->pos.x,vm.g_sp->y-ps->pos.y));

                if (j > -128 && j < 128)
                    j = 1;
                else
                    j = 0;
            }
            VM_CONDITIONAL((intptr_t) j);
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
                int32_t j = Gv_GetVarX(*insptr++);
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
                uint8_t f=*insptr++, r=*insptr++, g=*insptr++, b=*insptr++;

                P_PalFrom(ps, f, r,g,b);
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
                int32_t dq = Gv_GetVarX(*insptr++), sq = Gv_GetVarX(*insptr++);
                if (EDUKE32_PREDICT_FALSE(ScriptQuotes[sq] == NULL || ScriptQuotes[dq] == NULL))
                {
                    CON_ERRPRINTF("null quote %d\n", ScriptQuotes[sq] ? dq : sq);

                    while ((*insptr & VM_INSTMASK) != CON_NULLOP)
                        Gv_GetVarX(*insptr++);

                    insptr++; // skip the NOP
                    continue;
                }

                {
                    int32_t arg[32], i = 0, j = 0, k = 0, numargs;
                    int32_t len = Bstrlen(ScriptQuotes[sq]);
                    char tempbuf[MAXQUOTELEN];

                    while ((*insptr & VM_INSTMASK) != CON_NULLOP && i < 32)
                        arg[i++] = Gv_GetVarX(*insptr++);
                    numargs = i;

                    insptr++; // skip the NOP

                    i = 0;

                    do
                    {
                        while (k < len && j < MAXQUOTELEN && ScriptQuotes[sq][k] != '%')
                            tempbuf[j++] = ScriptQuotes[sq][k++];

                        if (ScriptQuotes[sq][k] == '%')
                        {
                            k++;
                            switch (ScriptQuotes[sq][k])
                            {
                            case 'l':
                                if (ScriptQuotes[sq][k+1] != 'd')
                                {
                                    // write the % and l
                                    tempbuf[j++] = ScriptQuotes[sq][k-1];
                                    tempbuf[j++] = ScriptQuotes[sq][k++];
                                    break;
                                }
                                k++;
                            case 'd':
                            {
                                char buf[16];
                                int32_t ii;

                                if (i >= numargs)
                                    goto finish_qsprintf;
                                Bsprintf(buf, "%d", arg[i++]);

                                ii = Bstrlen(buf);
                                Bmemcpy(&tempbuf[j], buf, ii);
                                j += ii;
                                k++;
                            }
                            break;

                            case 's':
                            {
                                int32_t ii;

                                if (i >= numargs)
                                    goto finish_qsprintf;
                                ii = Bstrlen(ScriptQuotes[arg[i]]);

                                Bmemcpy(&tempbuf[j], ScriptQuotes[arg[i]], ii);
                                j += ii;
                                i++;
                                k++;
                            }
                            break;

                            default:
                                tempbuf[j++] = ScriptQuotes[sq][k-1];
                                break;
                            }
                        }
                    }
                    while (k < len && j < MAXQUOTELEN);
finish_qsprintf:
                    tempbuf[j] = '\0';
                    Bstrncpyz(ScriptQuotes[dq], tempbuf, MAXQUOTELEN);
                    continue;
                }
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
                                       (int32_t)(m*aGameArrays[lVarID].plValues[index]));
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
                        if ((lVarID & (MAXGAMEVARS-1)) == g_iActorVarID)
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

                if (aGameVars[lVarID].dwFlags & GAMEVAR_READONLY)
                {
                    Bsprintf(szBuf," (read-only)");
                    strcat(g_szBuf,szBuf);
                }
                if (aGameVars[lVarID].dwFlags & GAMEVAR_PERPLAYER)
                {
                    Bsprintf(szBuf," (Per Player. Player=%d)",vm.g_p);
                }
                else if (aGameVars[lVarID].dwFlags & GAMEVAR_PERACTOR)
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
        case CON_GETSECTOR:
            insptr++;
            {
                // syntax [gs]etsector[<var>].x <VAR>
                // <varid> <xxxid> <varid>
                int32_t lVar1=*insptr++, lLabelID=*insptr++, lVar2=*insptr++;

                VM_AccessSector(tw==CON_SETSECTOR, lVar1, lLabelID, lVar2);
                continue;
            }

        case CON_SQRT:
            insptr++;
            {
                // syntax sqrt <invar> <outvar>
                int32_t lInVarID=*insptr++, lOutVarID=*insptr++;

                Gv_SetVarX(lOutVarID, ksqrt((uint32_t)Gv_GetVarX(lInVarID)));
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
                int32_t lType=*insptr++, lMaxDist=*insptr++, lVarID=*insptr++;
                int32_t lFound=-1, j, k = MAXSTATUS-1;

                if (tw == CON_FINDNEARACTOR || tw == CON_FINDNEARACTOR3D)
                    k = 1;

                if (tw==CON_FINDNEARSPRITE3D || tw==CON_FINDNEARACTOR3D)
                {
                    do
                    {
                        j=headspritestat[k];    // all sprites
                        while (j>=0)
                        {
                            if (sprite[j].picnum == lType && j != vm.g_i && dist(&sprite[vm.g_i], &sprite[j]) < lMaxDist)
                            {
                                lFound=j;
                                j = MAXSPRITES;
                                break;
                            }
                            j = nextspritestat[j];
                        }
                        if (j == MAXSPRITES || tw == CON_FINDNEARACTOR3D)
                            break;
                    }
                    while (k--);
                    Gv_SetVarX(lVarID, lFound);
                    continue;
                }

                do
                {
                    j=headspritestat[k];    // all sprites
                    while (j>=0)
                    {
                        if (sprite[j].picnum == lType && j != vm.g_i && ldist(&sprite[vm.g_i], &sprite[j]) < lMaxDist)
                        {
                            lFound=j;
                            j = MAXSPRITES;
                            break;
                        }
                        j = nextspritestat[j];
                    }

                    if (j == MAXSPRITES || tw == CON_FINDNEARACTOR)
                        break;
                }
                while (k--);
                Gv_SetVarX(lVarID, lFound);
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
                int32_t lType=*insptr++, lMaxDist=Gv_GetVarX(*insptr++), lVarID=*insptr++;
                int32_t lFound=-1, j, k = 1;

                if (tw == CON_FINDNEARSPRITEVAR || tw == CON_FINDNEARSPRITE3DVAR)
                    k = MAXSTATUS-1;

                if (tw==CON_FINDNEARACTOR3DVAR || tw==CON_FINDNEARSPRITE3DVAR)
                {
                    do
                    {
                        j=headspritestat[k];    // all sprites

                        while (j >= 0)
                        {
                            if (sprite[j].picnum == lType && j != vm.g_i && dist(&sprite[vm.g_i], &sprite[j]) < lMaxDist)
                            {
                                lFound=j;
                                j = MAXSPRITES;
                                break;
                            }
                            j = nextspritestat[j];
                        }
                        if (j == MAXSPRITES || tw==CON_FINDNEARACTOR3DVAR)
                            break;
                    }
                    while (k--);
                    Gv_SetVarX(lVarID, lFound);
                    continue;
                }

                do
                {
                    j=headspritestat[k];    // all sprites

                    while (j >= 0)
                    {
                        if (sprite[j].picnum == lType && j != vm.g_i && ldist(&sprite[vm.g_i], &sprite[j]) < lMaxDist)
                        {
                            lFound=j;
                            j = MAXSPRITES;
                            break;
                        }
                        j = nextspritestat[j];
                    }

                    if (j == MAXSPRITES || tw==CON_FINDNEARACTORVAR)
                        break;
                }
                while (k--);
                Gv_SetVarX(lVarID, lFound);
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
                int32_t lType=*insptr++, lMaxDist=Gv_GetVarX(*insptr++);
                int32_t lMaxZDist=Gv_GetVarX(*insptr++);
                int32_t lVarID=*insptr++, lFound=-1, lTemp, lTemp2, j, k=MAXSTATUS-1;
                do
                {
                    j=headspritestat[tw==CON_FINDNEARACTORZVAR?1:k];    // all sprites
                    if (j == -1) continue;
                    do
                    {
                        if (sprite[j].picnum == lType && j != vm.g_i)
                        {
                            lTemp=ldist(&sprite[vm.g_i], &sprite[j]);
                            if (lTemp < lMaxDist)
                            {
                                lTemp2=klabs(sprite[vm.g_i].z-sprite[j].z);
                                if (lTemp2 < lMaxZDist)
                                {
                                    lFound=j;
                                    j = MAXSPRITES;
                                    break;
                                }
                            }
                        }
                        j = nextspritestat[j];
                    }
                    while (j>=0);
                    if (tw==CON_FINDNEARACTORZVAR || j == MAXSPRITES)
                        break;
                }
                while (k--);
                Gv_SetVarX(lVarID, lFound);

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
                int32_t lType=*insptr++, lMaxDist=*insptr++, lMaxZDist=*insptr++, lVarID=*insptr++;
                int32_t lTemp, lTemp2, lFound=-1, j, k=MAXSTATUS-1;
                do
                {
                    j=headspritestat[tw==CON_FINDNEARACTORZ?1:k];    // all sprites
                    if (j == -1) continue;
                    do
                    {
                        if (sprite[j].picnum == lType && j != vm.g_i)
                        {
                            lTemp=ldist(&sprite[vm.g_i], &sprite[j]);
                            if (lTemp < lMaxDist)
                            {
                                lTemp2=klabs(sprite[vm.g_i].z-sprite[j].z);
                                if (lTemp2 < lMaxZDist)
                                {
                                    lFound=j;
                                    j = MAXSPRITES;
                                    break;
                                }
                            }
                        }
                        j = nextspritestat[j];
                    }
                    while (j>=0);

                    if (tw==CON_FINDNEARACTORZ || j == MAXSPRITES)
                        break;
                }
                while (k--);
                Gv_SetVarX(lVarID, lFound);
                continue;
            }

        case CON_FINDPLAYER:
            insptr++;
            aGameVars[g_iReturnVarID].val.lValue = A_FindPlayer(&sprite[vm.g_i], &tw);
            Gv_SetVarX(*insptr++, tw);
            continue;

        case CON_FINDOTHERPLAYER:
            insptr++;
            aGameVars[g_iReturnVarID].val.lValue = P_FindOtherPlayer(vm.g_p,&tw);
            Gv_SetVarX(*insptr++, tw);
            continue;

        case CON_SETPLAYER:
            insptr++;
            {
                tw=*insptr++;
                int32_t lLabelID=*insptr++;
                int32_t lParm2 = (PlayerLabels[lLabelID].flags & LABEL_HASPARM2) ? Gv_GetVarX(*insptr++) : 0;
                VM_SetPlayer(tw, lLabelID, *insptr++, lParm2);
                continue;
            }

        case CON_GETPLAYER:
            insptr++;
            {
                tw=*insptr++;
                int32_t lLabelID=*insptr++;
                int32_t lParm2 = (PlayerLabels[lLabelID].flags & LABEL_HASPARM2) ? Gv_GetVarX(*insptr++) : 0;
                VM_GetPlayer(tw, lLabelID, *insptr++, lParm2);
                continue;
            }

        case CON_GETINPUT:
            insptr++;
            {
                tw=*insptr++;
                int32_t lLabelID=*insptr++, lVar2=*insptr++;
                VM_AccessPlayerInput(0, tw, lLabelID, lVar2);
                continue;
            }

        case CON_SETINPUT:
            insptr++;
            {
                tw=*insptr++;
                int32_t lLabelID=*insptr++, lVar2=*insptr++;
                VM_AccessPlayerInput(1, tw, lLabelID, lVar2);
                continue;
            }

        case CON_GETUSERDEF:
            insptr++;
            {
                tw=*insptr++;
                int32_t lVar2=*insptr++;
                VM_AccessUserdef(0, tw, lVar2);
                continue;
            }

        case CON_SETUSERDEF:
            insptr++;
            {
                tw=*insptr++;
                int32_t lVar2=*insptr++;
                VM_AccessUserdef(1, tw, lVar2);
                continue;
            }

        case CON_GETPROJECTILE:
            insptr++;
            {
                tw = Gv_GetVarX(*insptr++);
                int32_t lLabelID = *insptr++, lVar2 = *insptr++;
                VM_AccessProjectile(0, tw, lLabelID, lVar2);
                continue;
            }

        case CON_SETPROJECTILE:
            insptr++;
            {
                tw = Gv_GetVarX(*insptr++);
                int32_t lLabelID = *insptr++, lVar2 = *insptr++;
                VM_AccessProjectile(1, tw, lLabelID, lVar2);
                continue;
            }

        case CON_SETWALL:
            insptr++;
            {
                tw=*insptr++;
                int32_t lLabelID=*insptr++, lVar2=*insptr++;
                VM_AccessWall(1, tw, lLabelID, lVar2);
                continue;
            }

        case CON_GETWALL:
            insptr++;
            {
                tw=*insptr++;
                int32_t lLabelID=*insptr++, lVar2=*insptr++;
                VM_AccessWall(0, tw, lLabelID, lVar2);
                continue;
            }

        case CON_SETACTORVAR:
        case CON_GETACTORVAR:
            insptr++;
            {
                // syntax [gs]etactorvar[<var>].<varx> <VAR>
                // gets the value of the per-actor variable varx into VAR
                // <var> <varx> <VAR>
                int32_t lSprite=Gv_GetVarX(*insptr++), lVar1=*insptr++;
                int32_t lVar2=*insptr++;

                if (EDUKE32_PREDICT_FALSE((unsigned)lSprite >= MAXSPRITES))
                {
                    CON_ERRPRINTF("invalid sprite ID %d\n", lSprite);
                    if (lVar1 == MAXGAMEVARS || lVar1 & ((MAXGAMEVARS<<2)|(MAXGAMEVARS<<3))) insptr++;
                    if (lVar2 == MAXGAMEVARS || lVar2 & ((MAXGAMEVARS<<2)|(MAXGAMEVARS<<3))) insptr++;
                    continue;
                }

                if (tw == CON_SETACTORVAR)
                {
                    Gv_SetVar(lVar1, Gv_GetVarX(lVar2), lSprite, vm.g_p);
                    continue;
                }
                Gv_SetVarX(lVar2, Gv_GetVar(lVar1, lSprite, vm.g_p));
                continue;
            }

        case CON_SETPLAYERVAR:
        case CON_GETPLAYERVAR:
            insptr++;
            {
                int32_t iPlayer = (*insptr != g_iThisActorID) ? Gv_GetVarX(*insptr) : vm.g_p;

                insptr++;

                int32_t lVar1 = *insptr++, lVar2 = *insptr++;

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
                // syntax [gs]etactor[<var>].x <VAR>
                // <varid> <xxxid> <varid>

                int32_t lVar1 = *insptr++, lLabelID = *insptr++;
                int32_t lParm2 = (ActorLabels[lLabelID].flags & LABEL_HASPARM2) ? Gv_GetVarX(*insptr++) : 0;

                VM_SetSprite(lVar1, lLabelID, *insptr++, lParm2);
                continue;
            }

        case CON_GETACTOR:
            insptr++;
            {
                // syntax [gs]etactor[<var>].x <VAR>
                // <varid> <xxxid> <varid>

                int32_t lVar1=*insptr++, lLabelID=*insptr++;
                int32_t lParm2 = (ActorLabels[lLabelID].flags & LABEL_HASPARM2) ? Gv_GetVarX(*insptr++) : 0;

                VM_GetSprite(lVar1, lLabelID, *insptr++, lParm2);
                continue;
            }

        case CON_SETTSPR:
        case CON_GETTSPR:
            insptr++;
            {
                // syntax [gs]etactor[<var>].x <VAR>
                // <varid> <xxxid> <varid>

                int32_t lVar1=*insptr++, lLabelID=*insptr++, lVar2=*insptr++;

                VM_AccessTsprite(tw==CON_SETTSPR, lVar1, lLabelID, lVar2);
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
            if ((aGameVars[*insptr].dwFlags & (GAMEVAR_USER_MASK | GAMEVAR_PTR_MASK)) == 0)
                aGameVars[*insptr].val.lValue = *(insptr + 1);
            else
                Gv_SetVarX(*insptr, *(insptr + 1));
            insptr += 2;
            continue;

        case CON_SETARRAY:
            insptr++;
            {
                tw=*insptr++;
                int32_t index = Gv_GetVarX(*insptr++);
                int32_t value = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)tw >= (unsigned)g_gameArrayCount || (unsigned)index >= (unsigned)aGameArrays[tw].size))
                {
                    OSD_Printf(OSD_ERROR "Gv_SetVar(): tried to set invalid array ID (%d) or index out of bounds from sprite %d (%d), player %d\n",
                        tw,vm.g_i,TrackerCast(sprite[vm.g_i].picnum),vm.g_p);
                    continue;
                }
                if (EDUKE32_PREDICT_FALSE(aGameArrays[tw].dwFlags & GAMEARRAY_READONLY))
                {
                    OSD_Printf("Tried to set on read-only array `%s'", aGameArrays[tw].szLabel);
                    continue;
                }
                aGameArrays[tw].plValues[index]=value;
                continue;
            }
        case CON_WRITEARRAYTOFILE:
        case CON_READARRAYFROMFILE:
            insptr++;
            {
                const int32_t j=*insptr++;
                const int q = *insptr++;

                if (EDUKE32_PREDICT_FALSE(ScriptQuotes[q] == NULL))
                {
                    CON_ERRPRINTF("null quote %d\n", q);
                    continue;
                }

                if (tw == CON_READARRAYFROMFILE)
                {
                    int32_t fil = kopen4loadfrommod(ScriptQuotes[q], 0);

                    if (fil < 0)
                        continue;

                    int32_t numelts = kfilelength(fil) / sizeof(int32_t);

                    // NOTE: LunaCON is stricter: if the file has no
                    // elements, resize the array to size zero.
                    if (numelts > 0)
                    {
                        /*OSD_Printf(OSDTEXT_GREEN "CON_RESIZEARRAY: resizing array %s from %d to %d\n",
                            aGameArrays[j].szLabel, aGameArrays[j].size, numelts);*/
                        int32_t numbytes = numelts * sizeof(int32_t);
#ifdef BITNESS64
                        int32_t *tmpar = (int32_t *)Xmalloc(numbytes);
                        kread(fil, tmpar, numbytes);
#endif
                        Baligned_free(aGameArrays[j].plValues);
                        aGameArrays[j].plValues = (intptr_t *)Xaligned_alloc(ACTOR_VAR_ALIGNMENT, numelts * GAR_ELTSZ);
                        aGameArrays[j].size = numelts;
#ifdef BITNESS64
                        for (int32_t i = 0; i < numelts; i++)
                            aGameArrays[j].plValues[i] = tmpar[i];  // int32_t --> int64_t
                        Bfree(tmpar);
#else
                        kread(fil, aGameArrays[j].plValues, numbytes);
#endif
                    }

                    kclose(fil);
                    continue;
                }

                char temp[BMAX_PATH];

                if (EDUKE32_PREDICT_FALSE(G_ModDirSnprintf(temp, sizeof(temp), "%s", ScriptQuotes[q])))
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

                const int32_t n = aGameArrays[j].size;
#ifdef BITNESS64
                int32_t *const array = (int32_t *)Xmalloc(sizeof(int32_t) * n);
                for (int32_t k = 0; k < n; k++) array[k] = aGameArrays[j].plValues[k];
#else
                int32_t *const array = (int32_t *)aGameArrays[j].plValues;
#endif
                fwrite(array, 1, sizeof(int32_t) * n, fil);
#ifdef BITNESS64
                Bfree(array);
#endif
                fclose(fil);

                continue;
            }

        case CON_GETARRAYSIZE:
            insptr++;
            tw = *insptr++;
            Gv_SetVarX(*insptr++,(aGameArrays[tw].dwFlags & GAMEARRAY_VARSIZE) ?
                       Gv_GetVarX(aGameArrays[tw].size) : aGameArrays[tw].size);
            continue;

        case CON_RESIZEARRAY:
            insptr++;
            {
                tw=*insptr++;
                int32_t asize = Gv_GetVarX(*insptr++);

                if (asize > 0)
                {
                    /*OSD_Printf(OSDTEXT_GREEN "CON_RESIZEARRAY: resizing array %s from %d to %d\n", aGameArrays[j].szLabel, aGameArrays[j].size, asize);*/
                    Baligned_free(aGameArrays[tw].plValues);
                    aGameArrays[tw].plValues = (intptr_t *)Xaligned_alloc(ACTOR_VAR_ALIGNMENT, GAR_ELTSZ * asize);
                    aGameArrays[tw].size = asize;
                }
                continue;
            }

        case CON_COPY:
            insptr++;
            {
                int32_t si=*insptr++;
                int32_t sidx = Gv_GetVarX(*insptr++); //, vm.g_i, vm.g_p);
                int32_t di=*insptr++;
                int32_t didx = Gv_GetVarX(*insptr++);
                int32_t numelts = Gv_GetVarX(*insptr++);

                tw = 0;

                if (EDUKE32_PREDICT_FALSE((unsigned)si>=(unsigned)g_gameArrayCount))
                {
                    CON_ERRPRINTF("Invalid array %d!", si);
                    tw = 1;
                }
                if (EDUKE32_PREDICT_FALSE((unsigned)di>=(unsigned)g_gameArrayCount))
                {
                    CON_ERRPRINTF("Invalid array %d!", di);
                    tw = 1;
                }
                if (EDUKE32_PREDICT_FALSE(aGameArrays[di].dwFlags & GAMEARRAY_READONLY))
                {
                    CON_ERRPRINTF("Array %d is read-only!", di);
                    tw = 1;
                }

                if (EDUKE32_PREDICT_FALSE(tw)) continue; // dirty replacement for VMFLAG_ERROR

                int32_t ssiz = (aGameArrays[si].dwFlags&GAMEARRAY_VARSIZE) ?
                       Gv_GetVarX(aGameArrays[si].size) : aGameArrays[si].size;
                int32_t dsiz = (aGameArrays[di].dwFlags&GAMEARRAY_VARSIZE) ?
                       Gv_GetVarX(aGameArrays[si].size) : aGameArrays[di].size;

                if (EDUKE32_PREDICT_FALSE(sidx > ssiz || didx > dsiz)) continue;
                if ((sidx+numelts) > ssiz) numelts = ssiz-sidx;
                if ((didx+numelts) > dsiz) numelts = dsiz-didx;

                // Switch depending on the source array type.
                switch (aGameArrays[si].dwFlags & GAMEARRAY_TYPE_MASK)
                {
                case 0:
                    // CON array to CON array.
                    Bmemcpy(aGameArrays[di].plValues+didx, aGameArrays[si].plValues+sidx, numelts*GAR_ELTSZ);
                    break;
                case GAMEARRAY_OFINT:
                    // From int32-sized array. Note that the CON array element
                    // type is intptr_t, so it is different-sized on 64-bit
                    // archs, but same-sized on 32-bit ones.
                    for (; numelts>0; numelts--)
                        (aGameArrays[di].plValues)[didx++] = ((int32_t *)aGameArrays[si].plValues)[sidx++];
                    break;
                case GAMEARRAY_OFSHORT:
                    // From int16_t array. Always different-sized.
                    for (; numelts>0; numelts--)
                        (aGameArrays[di].plValues)[didx++] = ((int16_t *)aGameArrays[si].plValues)[sidx++];
                    break;
                case GAMEARRAY_OFCHAR:
                    // From char array. Always different-sized.
                    for (; numelts>0; numelts--)
                        (aGameArrays[di].plValues)[didx++] = ((uint8_t *)aGameArrays[si].plValues)[sidx++];
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

        case CON_INV:
            if ((aGameVars[*(insptr + 1)].dwFlags & (GAMEVAR_USER_MASK | GAMEVAR_PTR_MASK)) == 0)
                aGameVars[*(insptr + 1)].val.lValue = -aGameVars[*(insptr + 1)].val.lValue;
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
                int32_t const gv = Gv_GetVarX(*insptr++);

                if ((aGameVars[tw].dwFlags & (GAMEVAR_USER_MASK | GAMEVAR_PTR_MASK)) == 0)
                    aGameVars[tw].val.lValue = gv;
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
                tw=*insptr++;
                int32_t const l2=Gv_GetVarX(*insptr++);

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
                int32_t const l2=Gv_GetVarX(*insptr++);

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
            if ((aGameVars[*insptr].dwFlags & (GAMEVAR_USER_MASK|GAMEVAR_PTR_MASK)) == 0)
            {
                aGameVars[*insptr].val.lValue <<= *(insptr+1);
                insptr += 2;
                continue;
            }
            Gv_SetVarX(*insptr, Gv_GetVarX(*insptr) << *(insptr+1));
            insptr += 2;
            continue;

        case CON_SHIFTVARR:
            insptr++;
            if ((aGameVars[*insptr].dwFlags & (GAMEVAR_USER_MASK|GAMEVAR_PTR_MASK)) == 0)
            {
                aGameVars[*insptr].val.lValue >>= *(insptr+1);
                insptr += 2;
                continue;
            }
            Gv_SetVarX(*insptr, Gv_GetVarX(*insptr) >> *(insptr+1));
            insptr += 2;
            continue;

        case CON_SIN:
            insptr++;
            Gv_SetVarX(*insptr, sintable[Gv_GetVarX(*(insptr+1))&2047]);
            insptr += 2;
            continue;

        case CON_COS:
            insptr++;
            Gv_SetVarX(*insptr, sintable[(Gv_GetVarX(*(insptr+1))+512)&2047]);
            insptr += 2;
            continue;

        case CON_ADDVARVAR:
            insptr++;
            tw = *insptr++;
            Gv_AddVar(tw, Gv_GetVarX(*insptr++));
            continue;

        case CON_SPGETLOTAG:
            insptr++;
            aGameVars[g_iLoTagID].val.lValue = vm.g_sp->lotag;
            continue;

        case CON_SPGETHITAG:
            insptr++;
            aGameVars[g_iHiTagID].val.lValue = vm.g_sp->hitag;
            continue;

        case CON_SECTGETLOTAG:
            insptr++;
            aGameVars[g_iLoTagID].val.lValue = sector[vm.g_sp->sectnum].lotag;
            continue;

        case CON_SECTGETHITAG:
            insptr++;
            aGameVars[g_iHiTagID].val.lValue = sector[vm.g_sp->sectnum].hitag;
            continue;

        case CON_GETTEXTUREFLOOR:
            insptr++;
            aGameVars[g_iTextureID].val.lValue = sector[vm.g_sp->sectnum].floorpicnum;
            continue;

        case CON_STARTTRACK:
        case CON_STARTTRACKVAR:
            insptr++;
            {
                int32_t const level = (tw == CON_STARTTRACK) ? *(insptr++) :
                    Gv_GetVarX(*(insptr++));

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
            aGameVars[g_iTextureID].val.lValue = sector[vm.g_sp->sectnum].ceilingpicnum;
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

        case CON_IFVARVARL:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            tw = (tw < Gv_GetVarX(*insptr++));
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

        case CON_IFVARG:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            VM_CONDITIONAL(tw > *insptr);
            continue;

        case CON_IFVARL:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            VM_CONDITIONAL(tw < *insptr);
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
                case GET_SHIELD: tw = (ps->inv_amount[GET_SHIELD] != ps->max_shield_amount); break;
                case GET_SCUBA: tw = (ps->inv_amount[GET_SCUBA] != *insptr); break;
                case GET_HOLODUKE: tw = (ps->inv_amount[GET_HOLODUKE] != *insptr); break;
                case GET_JETPACK: tw = (ps->inv_amount[GET_JETPACK] != *insptr); break;
                case GET_ACCESS:
                    switch (vm.g_sp->pal)
                    {
                        case 0: tw = (ps->got_access & 1); break;
                        case 21: tw = (ps->got_access & 2); break;
                        case 23: tw = (ps->got_access & 4); break;
                    }
                    break;
                case GET_HEATS: tw = (ps->inv_amount[GET_HEATS] != *insptr); break;
                case GET_FIRSTAID: tw = (ps->inv_amount[GET_FIRSTAID] != *insptr); break;
                case GET_BOOTS: tw = (ps->inv_amount[GET_BOOTS] != *insptr); break;
                default: tw = 0; CON_ERRPRINTF("invalid inventory ID: %d\n", (int32_t) * (insptr - 1));
            }

            VM_CONDITIONAL(tw);
            continue;

        case CON_PSTOMP:
            insptr++;
            if (ps->knee_incs == 0 && sprite[ps->i].xrepeat >= 40)
                if (cansee(vm.g_sp->x, vm.g_sp->y, vm.g_sp->z - (4 << 8), vm.g_sp->sectnum, ps->pos.x,
                           ps->pos.y, ps->pos.z + (16 << 8), sprite[ps->i].sectnum))
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
            VM_ScriptInfo();

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
    vm.g_i = iActor;                    // Sprite ID
    vm.g_sp = &sprite[iActor];          // Pointer to sprite structure

    if (g_tile[vm.g_sp->picnum].loadPtr == NULL)
        return;

    vm.g_t = &actor[iActor].t_data[0];  // Sprite's 'extra' data
    vm.g_p = -1;                        // Player ID
    vm.g_x = -1;                        // Distance
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
        const int32_t action_frames = actionptr[1];
        const int32_t action_incval = actionptr[3];
        const int32_t action_delay = actionptr[4];
#else
        const int32_t action_frames = actor[vm.g_i].ac.numframes;
        const int32_t action_incval = actor[vm.g_i].ac.incval;
        const int32_t action_delay = actor[vm.g_i].ac.delay;
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
    const int32_t picnum = vm.g_sp->picnum;

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
    Bmemcpy(&save->msx[0],&msx[0],sizeof(msx));
    Bmemcpy(&save->msy[0],&msy[0],sizeof(msy));
    Bmemcpy(&save->g_mirrorWall[0],&g_mirrorWall[0],sizeof(g_mirrorWall));
    Bmemcpy(&save->g_mirrorSector[0],&g_mirrorSector[0],sizeof(g_mirrorSector));
    Bmemcpy(&save->g_mirrorCount,&g_mirrorCount,sizeof(g_mirrorCount));
    Bmemcpy(&save->show2dsector[0],&show2dsector[0],sizeof(show2dsector));
    Bmemcpy(&save->g_numClouds,&g_numClouds,sizeof(g_numClouds));
    Bmemcpy(&save->clouds[0],&clouds[0],sizeof(clouds));
    Bmemcpy(&save->cloudx[0],&cloudx[0],sizeof(cloudx));
    Bmemcpy(&save->cloudy[0],&cloudy[0],sizeof(cloudy));
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
        if (aGameVars[i].dwFlags & GAMEVAR_NORESET) continue;
        if (aGameVars[i].dwFlags & GAMEVAR_PERPLAYER)
        {
            if (!save->vars[i])
                save->vars[i] = (intptr_t *)Xaligned_alloc(16, MAXPLAYERS * sizeof(intptr_t));
            Bmemcpy(&save->vars[i][0],&aGameVars[i].val.plValues[0],sizeof(intptr_t) * MAXPLAYERS);
        }
        else if (aGameVars[i].dwFlags & GAMEVAR_PERACTOR)
        {
            if (!save->vars[i])
                save->vars[i] = (intptr_t *)Xaligned_alloc(16, MAXSPRITES * sizeof(intptr_t));
            Bmemcpy(&save->vars[i][0],&aGameVars[i].val.plValues[0],sizeof(intptr_t) * MAXSPRITES);
        }
        else save->vars[i] = (intptr_t *)aGameVars[i].val.lValue;
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
        Bmemcpy(&msx[0],&save->msx[0],sizeof(msx));
        Bmemcpy(&msy[0],&save->msy[0],sizeof(msy));
        Bmemcpy(&g_mirrorWall[0],&save->g_mirrorWall[0],sizeof(g_mirrorWall));
        Bmemcpy(&g_mirrorSector[0],&save->g_mirrorSector[0],sizeof(g_mirrorSector));
        Bmemcpy(&g_mirrorCount,&save->g_mirrorCount,sizeof(g_mirrorCount));
        Bmemcpy(&show2dsector[0],&save->show2dsector[0],sizeof(show2dsector));
        Bmemcpy(&g_numClouds,&save->g_numClouds,sizeof(g_numClouds));
        Bmemcpy(&clouds[0],&save->clouds[0],sizeof(clouds));
        Bmemcpy(&cloudx[0],&save->cloudx[0],sizeof(cloudx));
        Bmemcpy(&cloudy[0],&save->cloudy[0],sizeof(cloudy));
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
            if (aGameVars[i].dwFlags & GAMEVAR_NORESET) continue;
            if (aGameVars[i].dwFlags & GAMEVAR_PERPLAYER)
            {
                if (!save->vars[i]) continue;
                Bmemcpy(&aGameVars[i].val.plValues[0],&save->vars[i][0],sizeof(intptr_t) * MAXPLAYERS);
            }
            else if (aGameVars[i].dwFlags & GAMEVAR_PERACTOR)
            {
                if (!save->vars[i]) continue;
                Bmemcpy(&aGameVars[i].val.plValues[0],&save->vars[i][0],sizeof(intptr_t) * MAXSPRITES);
            }
            else aGameVars[i].val.lValue = (intptr_t)save->vars[i];
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

int32_t VM_ResetPlayer2(int32_t snum)
{
    return VM_ResetPlayer(snum, 0);
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
