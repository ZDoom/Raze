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

#include "compat.h"
#include "colmatch.h"

#include "duke3d.h"

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

int32_t g_returnVarID    = -1;  // var ID of "RETURN"
int32_t g_weaponVarID    = -1;  // var ID of "WEAPON"
int32_t g_worksLikeVarID = -1;  // var ID of "WORKSLIKE"
int32_t g_zRangeVarID    = -1;  // var ID of "ZRANGE"
int32_t g_angRangeVarID  = -1;  // var ID of "ANGRANGE"
int32_t g_aimAngleVarID  = -1;  // var ID of "AUTOAIMANGLE"
int32_t g_lotagVarID     = -1;  // var ID of "LOTAG"
int32_t g_hitagVarID     = -1;  // var ID of "HITAG"
int32_t g_textureVarID   = -1;  // var ID of "TEXTURE"
int32_t g_thisActorVarID = -1;  // var ID of "THISACTOR"
int32_t g_structVarIDs   = -1;

GAMEEXEC_STATIC void VM_Execute(int loop);

# include "gamestructures.cpp"
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
    if (!apScript || (!vm.pSprite && !vm.pPlayer && g_currentEventExec == -1))
        return;

    if (ptr)
    {
        initprintf("\n");

        for (intptr_t const *pScript = max(ptr - (range >> 1), apScript),
                            *p_end   = min(ptr + (range >> 1), apScript + g_scriptSize);
             pScript < p_end; pScript++)
        {
            initprintf("%5d: %3d: ", (int32_t) (pScript - apScript), (int32_t) (pScript - ptr));

            if (*pScript >> 12 && (*pScript & VM_INSTMASK) < CON_END)
                initprintf("%5d %s\n", (int32_t) (*pScript >> 12), VM_GetKeywordForID(*pScript & VM_INSTMASK));
            else
                initprintf("%d\n", (int32_t) *pScript);
        }

        initprintf("\n");
    }

    if (ptr == insptr)
    {
        if (vm.spriteNum)
            initprintf("current actor: %d (%d)\n", vm.spriteNum, vm.pUSprite->picnum);

        initprintf("g_errorLineNum: %d, g_tw: %d\n", g_errorLineNum, g_tw);
    }
}
#endif

static void VM_DeleteSprite(int const spriteNum, int const playerNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned) spriteNum >= MAXSPRITES))
        return;

    // if player was set to squish, first stop that...
    if (EDUKE32_PREDICT_FALSE(playerNum >= 0 && g_player[playerNum].ps->actorsqu == spriteNum))
        g_player[playerNum].ps->actorsqu = -1;

    A_DeleteSprite(spriteNum);
}

intptr_t apScriptEvents[MAXEVENTS];

// May recurse, e.g. through EVENT_XXX -> ... -> EVENT_KILLIT
#ifdef LUNATIC
static FORCE_INLINE int32_t VM_EventCommon_(int eventNum, int spriteNum, int playerNum, int playerDist, int32_t returnValue)
{
    const double t = gethiticks();
    int32_t ret = El_CallEvent(&g_ElState, eventNum, spriteNum, playerNum, playerDist, &returnValue);

    // NOTE: the run times are those of the called event plus any events
    // called by it, *not* "self" time.
    g_eventTotalMs[eventNum] += gethiticks()-t;
    g_eventCalls[eventNum]++;

    if (ret == 1)
        VM_DeleteSprite(spriteNum, playerNum);

    return returnValue;
}
#else
// do not inline
static void VM_DummySprite(void)
{
    static uspritetype dummy_sprite;
    static actor_t dummy;

    vm.pUSprite = &dummy_sprite;
    vm.pActor = &dummy;
    vm.pData = &dummy.t_data[0];
}

static FORCE_INLINE int32_t VM_EventCommon_(int const eventNum, int const spriteNum, int const playerNum,
                                     int const playerDist, int32_t returnValue)
{
    const vmstate_t tempvm = { spriteNum, playerNum, playerDist, 0, NULL, NULL, g_player[playerNum].ps, NULL };

    int const backupReturnVar = aGameVars[g_returnVarID].global;
    int const backupEventExec = g_currentEventExec;

    aGameVars[g_returnVarID].global = returnValue;
    g_currentEventExec              = eventNum;

    intptr_t const *oinsptr   = insptr;
    const vmstate_t vm_backup = vm;

    insptr = apScript + apScriptEvents[eventNum];
    vm     = tempvm;

    // check tempvm instead of vm... this way, we are not actually loading
    // FROM vm anywhere until VM_Execute() is called
    if (EDUKE32_PREDICT_FALSE((unsigned) tempvm.spriteNum >= MAXSPRITES))
        VM_DummySprite();
    else
    {
        vm.pSprite = &sprite[spriteNum];
        vm.pActor  = &actor[spriteNum];
        vm.pData   = &actor[spriteNum].t_data[0];
    }

    if ((unsigned)playerNum >= (unsigned)g_mostConcurrentPlayers)
        vm.pPlayer = g_player[0].ps;

    VM_Execute(1);

    if (vm.flags & VM_KILL)
        VM_DeleteSprite(vm.spriteNum, vm.playerNum);

    // this needs to happen after VM_DeleteSprite() because VM_DeleteSprite()
    // can trigger additional events

    vm                 = vm_backup;
    insptr             = oinsptr;
    g_currentEventExec = backupEventExec;
    returnValue        = aGameVars[g_returnVarID].global;

    aGameVars[g_returnVarID].global  = backupReturnVar;

    return returnValue;
}
#endif

// the idea here is that the compiler inlines the call to VM_EventCommon_() and gives us a set of full functions
// which are not only optimized further based on lDist or iReturn (or both) having values known at compile time,
// but are called faster due to having less parameters

int32_t VM_OnEventWithBoth_(int const nEventID, int const spriteNum, int const playerNum, int const nDist, int32_t const nReturn)
{
    return VM_EventCommon_(nEventID, spriteNum, playerNum, nDist, nReturn);
}

int32_t VM_OnEventWithReturn_(int const nEventID, int const spriteNum, int const playerNum, int32_t const nReturn)
{
    return VM_EventCommon_(nEventID, spriteNum, playerNum, -1, nReturn);
}

int32_t VM_OnEventWithDist_(int const nEventID, int const spriteNum, int const playerNum, int const nDist)
{
    return VM_EventCommon_(nEventID, spriteNum, playerNum, nDist, 0);
}

int32_t VM_OnEvent_(int const nEventID, int const spriteNum, int const playerNum)
{
    return VM_EventCommon_(nEventID, spriteNum, playerNum, -1, 0);
}

static int32_t VM_CheckSquished(void)
{
    usectortype const * const pSector = (usectortype *)&sector[vm.pSprite->sectnum];

    if (pSector->lotag == ST_23_SWINGING_DOOR ||
        (pSector->lotag == ST_1_ABOVE_WATER &&
         !A_CheckNoSE7Water((uspritetype const *)vm.pSprite, vm.pSprite->sectnum, pSector->lotag, NULL)) ||
        (vm.pSprite->picnum == APLAYER && ud.noclip))
        return 0;

    int32_t floorZ = pSector->floorz;
    int32_t ceilZ  = pSector->ceilingz;
#ifdef YAX_ENABLE
    int16_t cb, fb;

    yax_getbunches(vm.pSprite->sectnum, &cb, &fb);

    if (cb >= 0 && (pSector->ceilingstat&512)==0)  // if ceiling non-blocking...
        ceilZ -= ZOFFSET5;  // unconditionally don't squish... yax_getneighborsect is slowish :/
    if (fb >= 0 && (pSector->floorstat&512)==0)
        floorZ += ZOFFSET5;
#endif

    if (vm.pSprite->pal == 1 ?
        (floorZ - ceilZ >= ZOFFSET5 || (pSector->lotag&32768)) :
        (floorZ - ceilZ >= ZOFFSET4))
    return 0;

    P_DoQuote(QUOTE_SQUISHED, vm.pPlayer);

    if (A_CheckEnemySprite(vm.pSprite))
        vm.pSprite->xvel = 0;

    if (EDUKE32_PREDICT_FALSE(vm.pSprite->pal == 1)) // frozen
    {
        vm.pActor->picnum = SHOTSPARK1;
        vm.pActor->extra  = 1;
        return 0;
    }

    return 1;
}

#if !defined LUNATIC
GAMEEXEC_STATIC GAMEEXEC_INLINE void P_ForceAngle(DukePlayer_t *pPlayer)
{
    int const nAngle = 128-(krand()&255);

    pPlayer->horiz           += 64;
    pPlayer->return_to_center = 9;
    pPlayer->rotscrnang       = nAngle >> 1;
    pPlayer->look_ang         = pPlayer->rotscrnang;
}
#endif

// wow, this function sucks
#ifdef __cplusplus
extern "C"
#endif
int32_t A_Dodge(spritetype * const);
int32_t A_Dodge(spritetype * const pSprite)
{
    if (A_CheckEnemySprite(pSprite) && pSprite->extra <= 0)  // hack
        return 0;

    vec2_t const msin = { sintable[(pSprite->ang + 512) & 2047], sintable[pSprite->ang & 2047] };

    for (native_t nexti, SPRITES_OF_STAT_SAFE(STAT_PROJECTILE, i, nexti)) //weapons list
    {
        if (OW(i) == i)
            continue;

        vec2_t const b = { SX(i) - pSprite->x, SY(i) - pSprite->y };
        vec2_t const v = { sintable[(SA(i) + 512) & 2047], sintable[SA(i) & 2047] };

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

int32_t A_GetFurthestAngle(int const spriteNum, int const angDiv)
{
    uspritetype *const pSprite = (uspritetype *)&sprite[spriteNum];

    if (pSprite->picnum != APLAYER && (AC_COUNT(actor[spriteNum].t_data)&63) > 2)
        return pSprite->ang + 1024;

    int32_t   furthestAngle = 0;
    int32_t   greatestDist  = INT32_MIN;
    int const angIncs       = tabledivide32_noinline(2048, angDiv);
    hitdata_t hit;

    for (native_t j = pSprite->ang; j < (2048 + pSprite->ang); j += angIncs)
    {
        pSprite->z -= ZOFFSET3;
        hitscan((const vec3_t *)pSprite, pSprite->sectnum,
                sintable[(j+512)&2047],
                sintable[j&2047], 0,
                &hit, CLIPMASK1);
        pSprite->z += ZOFFSET3;

        int const hitDist = klabs(hit.pos.x-pSprite->x) + klabs(hit.pos.y-pSprite->y);

        if (hitDist > greatestDist)
        {
            greatestDist = hitDist;
            furthestAngle = j;
        }
    }

    return furthestAngle&2047;
}

int A_FurthestVisiblePoint(int const spriteNum, uspritetype * const ts, vec2_t * const vect)
{
    if (AC_COUNT(actor[spriteNum].t_data)&63)
        return -1;

    const uspritetype *const pnSprite = (uspritetype *)&sprite[spriteNum];

    hitdata_t hit;
    int const angincs = 128;
//    ((!g_netServer && ud.multimode < 2) && ud.player_skill < 3) ? 2048 / 2 : tabledivide32_noinline(2048, 1 + (krand() & 1));

    for (native_t j = ts->ang; j < (2048 + ts->ang); j += (angincs /*-(krand()&511)*/))
    {
        ts->z -= ZOFFSET2;
        hitscan((const vec3_t *)ts, ts->sectnum, sintable[(j + 512) & 2047], sintable[j & 2047],
                16384 - (krand() & 32767), &hit, CLIPMASK1);
        ts->z += ZOFFSET2;

        if (hit.sect < 0)
            continue;

        int const d  = FindDistance2D(hit.pos.x - ts->x, hit.pos.y - ts->y);
        int const da = FindDistance2D(hit.pos.x - pnSprite->x, hit.pos.y - pnSprite->y);

        if (d < da)
        {
            if (cansee(hit.pos.x, hit.pos.y, hit.pos.z, hit.sect,
                        pnSprite->x, pnSprite->y, pnSprite->z - ZOFFSET2, pnSprite->sectnum))
            {
                vect->x = hit.pos.x;
                vect->y = hit.pos.y;
                return hit.sect;
            }
        }
    }

    return -1;
}

static void VM_GetZRange(int const spriteNum, int32_t * const ceilhit, int32_t * const florhit, int const wallDist)
{
    uspritetype *const pSprite = (uspritetype *)&sprite[spriteNum];
    int const          ocstat  = pSprite->cstat;

    pSprite->cstat = 0;
    pSprite->z -= ZOFFSET;

    getzrange((vec3_t *)pSprite, pSprite->sectnum,
              &actor[spriteNum].ceilingz, ceilhit,
              &actor[spriteNum].floorz, florhit,
              wallDist, CLIPMASK0);

    pSprite->z += ZOFFSET;
    pSprite->cstat = ocstat;
}

void A_GetZLimits(int const spriteNum)
{
    spritetype *const pSprite = &sprite[spriteNum];
    int32_t           ceilhit, florhit;

    VM_GetZRange(spriteNum, &ceilhit, &florhit, (pSprite->statnum == STAT_PROJECTILE) ? 4 : 127);
    actor[spriteNum].flags &= ~SFLAG_NOFLOORSHADOW;

    if ((florhit&49152) == 49152 && (sprite[florhit&(MAXSPRITES-1)].cstat&48) == 0)
    {
        uspritetype const * const hitspr = (uspritetype *)&sprite[florhit&(MAXSPRITES-1)];

        florhit &= (MAXSPRITES-1);

        // If a non-projectile would fall onto non-frozen enemy OR an enemy onto a player...
        if ((A_CheckEnemySprite(hitspr) && hitspr->pal != 1 && pSprite->statnum != STAT_PROJECTILE)
                || (hitspr->picnum == APLAYER && A_CheckEnemySprite(pSprite)))
        {
            actor[spriteNum].flags |= SFLAG_NOFLOORSHADOW;  // No shadows on actors
            pSprite->xvel = -256;  // SLIDE_ABOVE_ENEMY
            A_SetSprite(spriteNum, CLIPMASK0);
        }
        else if (pSprite->statnum == STAT_PROJECTILE && hitspr->picnum == APLAYER && pSprite->owner==florhit)
        {
            actor[spriteNum].ceilingz = sector[pSprite->sectnum].ceilingz;
            actor[spriteNum].floorz   = sector[pSprite->sectnum].floorz;
        }
    }
}

void A_Fall(int const spriteNum)
{
    spritetype *const pSprite = &sprite[spriteNum];
    int spriteGravity = g_spriteGravity;

    if (EDUKE32_PREDICT_FALSE(G_CheckForSpaceFloor(pSprite->sectnum)))
        spriteGravity = 0;
    else if (sector[pSprite->sectnum].lotag == ST_2_UNDERWATER || EDUKE32_PREDICT_FALSE(G_CheckForSpaceCeiling(pSprite->sectnum)))
        spriteGravity = g_spriteGravity/6;

    if (pSprite->statnum == STAT_ACTOR || pSprite->statnum == STAT_PLAYER || pSprite->statnum == STAT_ZOMBIEACTOR || pSprite->statnum == STAT_STANDABLE)
    {
        int32_t ceilhit, florhit;
        VM_GetZRange(spriteNum, &ceilhit, &florhit, 127);
    }
    else
    {
        actor[spriteNum].ceilingz = sector[pSprite->sectnum].ceilingz;
        actor[spriteNum].floorz   = sector[pSprite->sectnum].floorz;
    }

#ifdef YAX_ENABLE
    int fbunch = (sector[pSprite->sectnum].floorstat&512) ? -1 : yax_getbunch(pSprite->sectnum, YAX_FLOOR);
#endif

    if (pSprite->z < actor[spriteNum].floorz-ZOFFSET
#ifdef YAX_ENABLE
            || fbunch >= 0
#endif
       )
    {
        if (sector[pSprite->sectnum].lotag == ST_2_UNDERWATER && pSprite->zvel > 3122)
            pSprite->zvel = 3144;
        pSprite->z += pSprite->zvel = min(6144, pSprite->zvel+spriteGravity);
    }

#ifdef YAX_ENABLE
    if (fbunch >= 0)
        setspritez(spriteNum, (vec3_t *)pSprite);
    else
#endif
        if (pSprite->z >= actor[spriteNum].floorz-ZOFFSET)
        {
            pSprite->z = actor[spriteNum].floorz-ZOFFSET;
            pSprite->zvel = 0;
        }
}

int32_t __fastcall G_GetAngleDelta(int32_t currAngle, int32_t newAngle)
{
    currAngle &= 2047;
    newAngle &= 2047;

    if (klabs(currAngle-newAngle) < 1024)
    {
//        OSD_Printf("G_GetAngleDelta() returning %d\n",na-a);
        return newAngle-currAngle;
    }

    if (newAngle > 1024) newAngle -= 2048;
    if (currAngle > 1024) currAngle -= 2048;

//    OSD_Printf("G_GetAngleDelta() returning %d\n",na-a);
    return newAngle-currAngle;
}

GAMEEXEC_STATIC void VM_AlterAng(int32_t const moveFlags)
{
    int const elapsedTics = (AC_COUNT(vm.pData))&31;

#if !defined LUNATIC
    const intptr_t *moveptr;
    if (EDUKE32_PREDICT_FALSE((unsigned)AC_MOVE_ID(vm.pData) >= (unsigned)g_scriptSize-1))

    {
        AC_MOVE_ID(vm.pData) = 0;
        OSD_Printf(OSD_ERROR "bad moveptr for actor %d (%d)!\n", vm.spriteNum, vm.pUSprite->picnum);
        return;
    }

    moveptr = apScript + AC_MOVE_ID(vm.pData);

    vm.pSprite->xvel += (moveptr[0] - vm.pSprite->xvel)/5;
    if (vm.pSprite->zvel < 648)
        vm.pSprite->zvel += ((moveptr[1]<<4) - vm.pSprite->zvel)/5;
#else
    vm.pSprite->xvel += (vm.pActor->mv.hvel - vm.pSprite->xvel)/5;
    if (vm.pSprite->zvel < 648)
        vm.pSprite->zvel += ((vm.pActor->mv.vvel<<4) - vm.pSprite->zvel)/5;
#endif

    if (A_CheckEnemySprite(vm.pSprite) && vm.pSprite->extra <= 0) // hack
        return;

    if (moveFlags&seekplayer)
    {
        int const spriteAngle    = vm.pSprite->ang;
        int const holoDukeSprite = vm.pPlayer->holoduke_on;

        // NOTE: looks like 'owner' is set to target sprite ID...

        vm.pSprite->owner
        = (holoDukeSprite >= 0 && cansee(sprite[holoDukeSprite].x, sprite[holoDukeSprite].y, sprite[holoDukeSprite].z,
                                         sprite[holoDukeSprite].sectnum, vm.pSprite->x, vm.pSprite->y, vm.pSprite->z, vm.pSprite->sectnum))
          ? holoDukeSprite
          : vm.pPlayer->i;

        int const goalAng = (sprite[vm.pSprite->owner].picnum == APLAYER)
                  ? getangle(vm.pActor->lastv.x - vm.pSprite->x, vm.pActor->lastv.y - vm.pSprite->y)
                  : getangle(sprite[vm.pSprite->owner].x - vm.pSprite->x, sprite[vm.pSprite->owner].y - vm.pSprite->y);

        if (vm.pSprite->xvel && vm.pSprite->picnum != DRONE)
        {
            int const angDiff = G_GetAngleDelta(spriteAngle, goalAng);

            if (elapsedTics < 2)
            {
                if (klabs(angDiff) < 256)
                {
                    int const angInc = 128-(krand()&256);
                    vm.pSprite->ang += angInc;
                    if (A_GetHitscanRange(vm.spriteNum) < 844)
                        vm.pSprite->ang -= angInc;
                }
            }
            else if (elapsedTics > 18 && elapsedTics < GAMETICSPERSEC) // choose
            {
                if (klabs(angDiff>>2) < 128) vm.pSprite->ang = goalAng;
                else vm.pSprite->ang += angDiff>>2;
            }
        }
        else vm.pSprite->ang = goalAng;
    }

    if (elapsedTics < 1)
    {
        if (moveFlags&furthestdir)
        {
            vm.pSprite->ang = A_GetFurthestAngle(vm.spriteNum, 2);
            vm.pSprite->owner = vm.pPlayer->i;
        }

        if (moveFlags&fleeenemy)
            vm.pSprite->ang = A_GetFurthestAngle(vm.spriteNum, 2);
    }
}

static inline void VM_AddAngle(int const shift, int const goalAng)
{
    int angDiff = G_GetAngleDelta(vm.pSprite->ang, goalAng) >> shift;

    if ((angDiff > -8 && angDiff < 0) || (angDiff < 8 && angDiff > 0))
        angDiff <<= 1;

    vm.pSprite->ang += angDiff;
}

static inline void VM_FacePlayer(int const shift)
{
    VM_AddAngle(shift, (vm.pPlayer->newowner >= 0) ? getangle(vm.pPlayer->opos.x - vm.pSprite->x, vm.pPlayer->opos.y - vm.pSprite->y)
                                                 : getangle(vm.pPlayer->pos.x - vm.pSprite->x, vm.pPlayer->pos.y - vm.pSprite->y));
}

////////// TROR get*zofslope //////////
// These rather belong into the engine.

static int32_t VM_GetCeilZOfSlope(void)
{
    vec2_t const vect     = *(vec2_t *)vm.pSprite;
    int const    sectnum  = vm.pSprite->sectnum;

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
    vec2_t const vect    = *(vec2_t *)vm.pSprite;
    int const    sectnum = vm.pSprite->sectnum;

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
    // NOTE: commented out condition is dead since r3159 (making hi/lotag unsigned).
    // XXX: Does it break anything? Where are movflags with all bits set created?
    const uint16_t * const movflagsptr = &AC_MOVFLAGS(vm.pSprite, &actor[vm.spriteNum]);
    int const movflags = /*(*movflagsptr==-1) ? 0 :*/ *movflagsptr;
    int const deadflag = (A_CheckEnemySprite(vm.pSprite) && vm.pSprite->extra <= 0);

    AC_COUNT(vm.pData)++;

    if (AC_MOVE_ID(vm.pData) == 0 || movflags == 0)
    {
        if (deadflag || (vm.pActor->bpos.x != vm.pSprite->x) || (vm.pActor->bpos.y != vm.pSprite->y))
        {
            vm.pActor->bpos.x = vm.pSprite->x;
            vm.pActor->bpos.y = vm.pSprite->y;
            setsprite(vm.spriteNum, (vec3_t *)vm.pSprite);
        }
        return;
    }

    if (deadflag)
        goto dead;

    if (movflags&face_player)
        VM_FacePlayer(2);

    if (movflags&spin)
        vm.pSprite->ang += sintable[((AC_COUNT(vm.pData)<<3)&2047)]>>6;

    if (movflags&face_player_slow)
        VM_FacePlayer(4);

    if ((movflags&jumptoplayer_bits) == jumptoplayer_bits)
    {
        if (AC_COUNT(vm.pData) < 16)
            vm.pSprite->zvel -= (sintable[(512+(AC_COUNT(vm.pData)<<4))&2047]>>5);
    }

    if (movflags&face_player_smart)
    {
        vec2_t const vect = { vm.pPlayer->pos.x + (vm.pPlayer->vel.x / 768),
                              vm.pPlayer->pos.y + (vm.pPlayer->vel.y / 768) };
        VM_AddAngle(2, getangle(vect.x - vm.pSprite->x, vect.y - vm.pSprite->y));
    }

dead:
#if !defined LUNATIC
    if (EDUKE32_PREDICT_FALSE((unsigned)AC_MOVE_ID(vm.pData) >= (unsigned)g_scriptSize-1))
    {
        AC_MOVE_ID(vm.pData) = 0;
        OSD_Printf(OSD_ERROR "clearing bad moveptr for actor %d (%d)\n", vm.spriteNum, vm.pUSprite->picnum);
        return;
    }

    intptr_t const * const moveptr = apScript + AC_MOVE_ID(vm.pData);

    if (movflags&geth) vm.pSprite->xvel += ((moveptr[0])-vm.pSprite->xvel)>>1;
    if (movflags&getv) vm.pSprite->zvel += ((moveptr[1]<<4)-vm.pSprite->zvel)>>1;
#else
    if (movflags&geth) vm.pSprite->xvel += (vm.pActor->mv.hvel - vm.pSprite->xvel)>>1;
    if (movflags&getv) vm.pSprite->zvel += (16*vm.pActor->mv.vvel - vm.pSprite->zvel)>>1;
#endif

    if (movflags&dodgebullet && !deadflag)
        A_Dodge(vm.pSprite);

    if (vm.pSprite->picnum != APLAYER)
        VM_AlterAng(movflags);

    if (vm.pSprite->xvel > -6 && vm.pSprite->xvel < 6)
        vm.pSprite->xvel = 0;

    int badguyp = A_CheckEnemySprite(vm.pSprite);

    if (vm.pSprite->xvel || vm.pSprite->zvel)
    {
        int spriteXvel = vm.pSprite->xvel;
        int angDiff    = vm.pSprite->ang;

        if (badguyp && vm.pSprite->picnum != ROTATEGUN)
        {
            if ((vm.pSprite->picnum == DRONE || vm.pSprite->picnum == COMMANDER) && vm.pSprite->extra > 0)
            {
                if (vm.pSprite->picnum == COMMANDER)
                {
                    int32_t nSectorZ;
                    // NOTE: COMMANDER updates both actor[].floorz and
                    // .ceilingz regardless of its zvel.
                    vm.pActor->floorz = nSectorZ = VM_GetFlorZOfSlope();
                    if (vm.pSprite->z > nSectorZ-ZOFFSET3)
                    {
                        vm.pSprite->z = nSectorZ-ZOFFSET3;
                        vm.pSprite->zvel = 0;
                    }

                    vm.pActor->ceilingz = nSectorZ = VM_GetCeilZOfSlope();
                    if (vm.pSprite->z < nSectorZ+(80<<8))
                    {
                        vm.pSprite->z = nSectorZ+(80<<8);
                        vm.pSprite->zvel = 0;
                    }
                }
                else
                {
                    int32_t nSectorZ;
                    // The DRONE updates either .floorz or .ceilingz, not both.
                    if (vm.pSprite->zvel > 0)
                    {
                        vm.pActor->floorz = nSectorZ = VM_GetFlorZOfSlope();
                        if (vm.pSprite->z > nSectorZ-(30<<8))
                            vm.pSprite->z = nSectorZ-(30<<8);
                    }
                    else
                    {
                        vm.pActor->ceilingz = nSectorZ = VM_GetCeilZOfSlope();
                        if (vm.pSprite->z < nSectorZ+(50<<8))
                        {
                            vm.pSprite->z = nSectorZ+(50<<8);
                            vm.pSprite->zvel = 0;
                        }
                    }
                }
            }
            else if (vm.pSprite->picnum != ORGANTIC)
            {
                // All other actors besides ORGANTIC don't update .floorz or
                // .ceilingz here.
                if (vm.pSprite->zvel > 0)
                {
                    if (vm.pSprite->z > vm.pActor->floorz)
                        vm.pSprite->z = vm.pActor->floorz;
                    vm.pSprite->z += A_GetWaterZOffset(vm.spriteNum);
                }
                else if (vm.pSprite->zvel < 0)
                {
                    int const l = VM_GetCeilZOfSlope();

                    if (vm.pSprite->z < l+(66<<8))
                    {
                        vm.pSprite->z = l+(66<<8);
                        vm.pSprite->zvel >>= 1;
                    }
                }
            }

            if (vm.playerDist < 960 && vm.pSprite->xrepeat > 16)
            {
                spriteXvel = -(1024 - vm.playerDist);
                angDiff = getangle(vm.pPlayer->pos.x - vm.pSprite->x, vm.pPlayer->pos.y - vm.pSprite->y);

                if (vm.playerDist < 512)
                {
                    vm.pPlayer->vel.x = 0;
                    vm.pPlayer->vel.y = 0;
                }
                else
                {
                    vm.pPlayer->vel.x = mulscale16(vm.pPlayer->vel.x, vm.pPlayer->runspeed - 0x2000);
                    vm.pPlayer->vel.y = mulscale16(vm.pPlayer->vel.y, vm.pPlayer->runspeed - 0x2000);
                }
            }
            else if (vm.pSprite->picnum != DRONE && vm.pSprite->picnum != SHARK && vm.pSprite->picnum != COMMANDER)
            {
                if (vm.pPlayer->actorsqu == vm.spriteNum)
                    return;

                if (!A_CheckSpriteFlags(vm.spriteNum, SFLAG_SMOOTHMOVE))
                {
                    if (AC_COUNT(vm.pData) & 1)
                        return;
                    spriteXvel <<= 1;
                }
            }
        }
        else if (vm.pSprite->picnum == APLAYER)
            if (vm.pSprite->z < vm.pActor->ceilingz+ZOFFSET5)
                vm.pSprite->z = vm.pActor->ceilingz+ZOFFSET5;

        vec3_t const vect = { (spriteXvel * (sintable[(angDiff + 512) & 2047])) >> 14,
                              (spriteXvel * (sintable[angDiff & 2047])) >> 14, vm.pSprite->zvel };

        vm.pActor->movflag = A_MoveSprite(vm.spriteNum, &vect, (A_CheckSpriteFlags(vm.spriteNum, SFLAG_NOCLIP) ? 0 : CLIPMASK0));
    }

    if (!badguyp)
        return;

    vm.pSprite->shade += (sector[vm.pSprite->sectnum].ceilingstat & 1) ? (sector[vm.pSprite->sectnum].ceilingshade - vm.pSprite->shade) >> 1
                                                                 : (sector[vm.pSprite->sectnum].floorshade - vm.pSprite->shade) >> 1;
}

static void P_AddWeaponMaybeSwitch(DukePlayer_t * const ps, int const weaponNum)
{
    if ((ps->weaponswitch & (1|4)) == (1|4))
    {
        int const playerNum    = P_Get(ps->i);
        int       new_wchoice  = -1;
        int       curr_wchoice = -1;

        for (native_t i=0; i<=FREEZE_WEAPON && (new_wchoice < 0 || curr_wchoice < 0); i++)
        {
            int w = g_player[playerNum].wchoice[i];

            if (w == KNEE_WEAPON)
                w = FREEZE_WEAPON;
            else w--;

            if (w == ps->curr_weapon)
                curr_wchoice = i;
            if (w == weaponNum)
                new_wchoice = i;
        }

        P_AddWeapon(ps, weaponNum, (new_wchoice < curr_wchoice));
    }
    else
    {
        P_AddWeapon(ps, weaponNum, (ps->weaponswitch & 1));
    }
}

#if defined LUNATIC
void P_AddWeaponMaybeSwitchI(int32_t snum, int32_t weap)
{
    P_AddWeaponMaybeSwitch(g_player[snum].ps, weap);
}
#else
static void P_AddWeaponAmmoCommon(DukePlayer_t * const pPlayer, int const weaponNum, int const nAmount)
{
    P_AddAmmo(pPlayer, weaponNum, nAmount);

    if (PWEAPON(vm.playerNum, pPlayer->curr_weapon, WorksLike) == KNEE_WEAPON && (pPlayer->gotweapon & (1 << weaponNum)))
        P_AddWeaponMaybeSwitch(pPlayer, weaponNum);
}

static void VM_AddWeapon(DukePlayer_t * const pPlayer, int const weaponNum, int const nAmount)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)weaponNum >= MAX_WEAPONS))
    {
        CON_ERRPRINTF("invalid weapon %d\n", weaponNum);
        return;
    }

    if ((pPlayer->gotweapon & (1 << weaponNum)) == 0)
    {
        P_AddWeaponMaybeSwitch(pPlayer, weaponNum);
    }
    else if (pPlayer->ammo_amount[weaponNum] >= pPlayer->max_ammo_amount[weaponNum])
    {
        vm.flags |= VM_NOEXECUTE;
        return;
    }

    P_AddWeaponAmmoCommon(pPlayer, weaponNum, nAmount);
}

static void VM_AddAmmo(DukePlayer_t * const pPlayer, int const weaponNum, int const nAmount)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)weaponNum >= MAX_WEAPONS))
    {
        CON_ERRPRINTF("invalid weapon %d\n", weaponNum);
        return;
    }

    if (pPlayer->ammo_amount[weaponNum] >= pPlayer->max_ammo_amount[weaponNum])
    {
        vm.flags |= VM_NOEXECUTE;
        return;
    }

    P_AddWeaponAmmoCommon(pPlayer, weaponNum, nAmount);
}

static void VM_AddInventory(DukePlayer_t * const pPlayer, int const itemNum, int const nAmount)
{
    switch (itemNum)
    {
    case GET_STEROIDS:
    case GET_SCUBA:
    case GET_HOLODUKE:
    case GET_JETPACK:
    case GET_HEATS:
    case GET_FIRSTAID:
    case GET_BOOTS:
        pPlayer->inven_icon = inv_to_icon[itemNum];
        pPlayer->inv_amount[itemNum] = nAmount;
        break;

    case GET_SHIELD:
    {
        int16_t & shield_amount = pPlayer->inv_amount[GET_SHIELD];
        shield_amount = min(shield_amount + nAmount, pPlayer->max_shield_amount);
        break;
    }

    case GET_ACCESS:
        switch (vm.pSprite->pal)
        {
        case  0:
            pPlayer->got_access |= 1;
            break;
        case 21:
            pPlayer->got_access |= 2;
            break;
        case 23:
            pPlayer->got_access |= 4;
            break;
        }
        break;

    default:
        CON_ERRPRINTF("invalid inventory item %d\n", itemNum);
        break;
    }
}
#endif

static int32_t A_GetVerticalVel(actor_t const * const pActor)
{
#ifdef LUNATIC
    return pActor->mv.vvel;
#else
    int32_t moveScriptOfs = AC_MOVE_ID(pActor->t_data);

    return ((unsigned) moveScriptOfs < (unsigned) g_scriptSize - 1) ? apScript[moveScriptOfs + 1] : 0;
#endif
}

static int32_t A_GetWaterZOffset(int const spriteNum)
{
    uspritetype const *const pSprite = (uspritetype *)&sprite[spriteNum];
    actor_t const *const     pActor  = &actor[spriteNum];

    if (sector[pSprite->sectnum].lotag == ST_1_ABOVE_WATER)
    {
        if (A_CheckSpriteFlags(spriteNum, SFLAG_NOWATERDIP))
            return 0;

        // fix for flying/jumping monsters getting stuck in water
        if ((AC_MOVFLAGS(pSprite, pActor) & jumptoplayer_only) || (G_HaveActor(pSprite->picnum) && A_GetVerticalVel(pActor) != 0))
            return 0;

        return ACTOR_ONWATER_ADDZ;
    }

    return 0;
}

static void VM_Fall(int const spriteNum, spritetype * const pSprite)
{
    int spriteGravity = g_spriteGravity;

    pSprite->xoffset = pSprite->yoffset = 0;

    if (sector[pSprite->sectnum].lotag == ST_2_UNDERWATER || EDUKE32_PREDICT_FALSE(G_CheckForSpaceCeiling(pSprite->sectnum)))
        spriteGravity = g_spriteGravity/6;
    else if (EDUKE32_PREDICT_FALSE(G_CheckForSpaceFloor(pSprite->sectnum)))
        spriteGravity = 0;

    if (!actor[spriteNum].cgg-- || (sector[pSprite->sectnum].floorstat&2))
    {
        A_GetZLimits(spriteNum);
        actor[spriteNum].cgg = 3;
    }

    if (pSprite->z < actor[spriteNum].floorz-ZOFFSET)
    {
        // Free fall.
        pSprite->zvel = min(pSprite->zvel+spriteGravity, ACTOR_MAXFALLINGZVEL);
        int newZ = pSprite->z + pSprite->zvel;

#ifdef YAX_ENABLE
        if (yax_getbunch(pSprite->sectnum, YAX_FLOOR) >= 0 &&
                (sector[pSprite->sectnum].floorstat&512)==0)
            setspritez(spriteNum, (vec3_t *)pSprite);
        else
#endif
            if (newZ > actor[spriteNum].floorz - ZOFFSET)
                newZ = actor[spriteNum].floorz - ZOFFSET;

        pSprite->z = newZ;
        return;
    }

    // Preliminary new z position of the actor.
    int newZ = actor[spriteNum].floorz - ZOFFSET;

    if (A_CheckEnemySprite(pSprite) || (pSprite->picnum == APLAYER && pSprite->owner >= 0))
    {
        if (pSprite->zvel > 3084 && pSprite->extra <= 1)
        {
            // I'm guessing this DRONE check is from a beta version of the game
            // where they crashed into the ground when killed
            if (!(pSprite->picnum == APLAYER && pSprite->extra > 0) && pSprite->pal != 1 && pSprite->picnum != DRONE)
            {
                A_DoGuts(spriteNum,JIBS6,15);
                A_PlaySound(SQUISHED,spriteNum);
                A_Spawn(spriteNum,BLOODPOOL);
            }

            actor[spriteNum].picnum = SHOTSPARK1;
            actor[spriteNum].extra = 1;
            pSprite->zvel = 0;
        }
        else if (pSprite->zvel > 2048 && sector[pSprite->sectnum].lotag != ST_1_ABOVE_WATER)
        {
            int16_t newsect = pSprite->sectnum;

            pushmove((vec3_t *)pSprite, &newsect, 128, 4<<8, 4<<8, CLIPMASK0);
            if ((unsigned)newsect < MAXSECTORS)
                changespritesect(spriteNum, newsect);

            A_PlaySound(THUD, spriteNum);
        }
    }

    if (sector[pSprite->sectnum].lotag == ST_1_ABOVE_WATER)
    {
        pSprite->z = newZ + A_GetWaterZOffset(spriteNum);
        return;
    }

    pSprite->z = newZ;
    pSprite->zvel = 0;
}

static int32_t VM_ResetPlayer(int const playerNum, int32_t vmFlags, int32_t const resetFlags)
{
    //AddLog("resetplayer");
    if (!g_netServer && ud.multimode < 2 && !(resetFlags & 2))
    {
        if (g_quickload && g_quickload->isValid() && ud.recstat != 2 && !(resetFlags & 1))
        {
            Menu_Open(playerNum);
            KB_ClearKeyDown(sc_Space);
            I_AdvanceTriggerClear();
            Menu_Change(MENU_RESETPLAYER);
        }
        else g_player[playerNum].ps->gm = MODE_RESTART;
#if !defined LUNATIC
        vmFlags |= VM_NOEXECUTE;
#endif
    }
    else
    {
        if (playerNum == myconnectindex)
        {
            CAMERADIST = 0;
            CAMERACLOCK = totalclock;
        }

        if (g_fakeMultiMode)
            P_ResetPlayer(playerNum);
#ifndef NETCODE_DISABLE
        if (g_netServer)
        {
            P_ResetPlayer(playerNum);
            Net_SpawnPlayer(playerNum);
        }
#endif
    }

    P_UpdateScreenPal(g_player[playerNum].ps);
    //AddLog("EOF: resetplayer");

    return vmFlags;
}

void G_GetTimeDate(int32_t * const pValues)
{
    time_t timeStruct;
    time(&timeStruct);
    struct tm *pTime = localtime(&timeStruct);

    // initprintf("Time&date: %s\n",asctime (ti));

    pValues[0] = pTime->tm_sec;
    pValues[1] = pTime->tm_min;
    pValues[2] = pTime->tm_hour;
    pValues[3] = pTime->tm_mday;
    pValues[4] = pTime->tm_mon;
    pValues[5] = pTime->tm_year+1900;
    pValues[6] = pTime->tm_wday;
    pValues[7] = pTime->tm_yday;
}

static int G_StartTrackSlot(int const volumeNum, int const levelNum)
{
    if ((unsigned)volumeNum <= MAXVOLUMES && (unsigned)levelNum < MAXLEVELS)
    {
        int trackNum = MAXLEVELS*volumeNum + levelNum;

        if (g_mapInfo[trackNum].musicfn != NULL)
        {
            // Only set g_musicIndex on success.
            g_musicIndex = trackNum;
            S_PlayMusic(g_mapInfo[trackNum].musicfn);

            return 0;
        }
    }

    return 1;
}

#ifndef LUNATIC
static void G_StartTrackSlotWrap(int const volumeNum, int const levelNum)
{
    if (EDUKE32_PREDICT_FALSE(G_StartTrackSlot(volumeNum, levelNum)))
        CON_ERRPRINTF("invalid level %d or null music for volume %d level %d\n",
                      levelNum, volumeNum, levelNum);
}
#else
int G_StartTrack(int const levelNum)
{
    return G_StartTrackSlot(ud.volume_number, levelNum);
}
#endif

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
}

void Screen_Play(void)
{
    int32_t running = 1;

    I_ClearAllInput();

    do
    {
        G_HandleAsync();

        clearallviews(0);
        if (VM_OnEventWithReturn(EVENT_SCREEN, g_player[screenpeek].ps->i, screenpeek, I_CheckAllInput()))
            running = 0;

        nextpage();

        I_ClearAllInput();

        ototalclock = totalclock + 1; // pause game like ANMs
    } while (running);
}

#if !defined LUNATIC
GAMEEXEC_STATIC void VM_Execute(int loop)
{
    int32_t tw = *insptr;
    DukePlayer_t * const pPlayer = vm.pPlayer;

    // jump directly into the loop, saving us from the checks during the first iteration
    goto skip_check;

    while (loop)
    {
        if (vm.flags & (VM_RETURN | VM_KILL | VM_NOEXECUTE))
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
                int const strIndex  = *insptr++;
                int const XstrIndex = *insptr++;

                if (EDUKE32_PREDICT_FALSE((apStrings[strIndex] == NULL || apXStrings[XstrIndex] == NULL)))
                {
                    CON_ERRPRINTF("invalid source quote %d or destination quote %d\n", XstrIndex, strIndex);
                    break;
                }
                Bstrcpy(apStrings[strIndex], apXStrings[XstrIndex]);
                continue;
            }

        case CON_GETTHISPROJECTILE:
            insptr++;
            {
                tw = *insptr++;
                int const spriteNum = (tw != g_thisActorVarID) ? Gv_GetVarX(tw) : vm.spriteNum;
                int const labelNum  = *insptr++;

                Gv_SetVarX(*insptr++, VM_GetActiveProjectile(spriteNum, labelNum));
                continue;
            }

        case CON_SETTHISPROJECTILE:
            insptr++;
            {
                tw = *insptr++;
                int const spriteNum = (tw != g_thisActorVarID) ? Gv_GetVarX(tw) : vm.spriteNum;
                int const labelNum  = *insptr++;

                VM_SetActiveProjectile(spriteNum, labelNum, Gv_GetVarX(*insptr++));
                continue;
            }

        case CON_IFRND:
            VM_CONDITIONAL(rnd(*(++insptr)));
            continue;

        case CON_IFCANSHOOTTARGET:
        {
            if (vm.playerDist > 1024)
            {
                int16_t temphit;

                if ((tw = A_CheckHitSprite(vm.spriteNum, &temphit)) == (1 << 30))
                {
                    VM_CONDITIONAL(1);
                    continue;
                }

                int dist    = 768;
                int angDiff = 16;

                if (A_CheckEnemySprite(vm.pSprite) && vm.pSprite->xrepeat > 56)
                {
                    dist    = 3084;
                    angDiff = 48;
                }

#define CHECK(x)                                                                                                                           \
    if (x >= 0 && sprite[x].picnum == vm.pSprite->picnum)                                                                                  \
    {                                                                                                                                      \
        VM_CONDITIONAL(0);                                                                                                                 \
        continue;                                                                                                                          \
    }
#define CHECK2(x)                                                                                                                          \
    do                                                                                                                                     \
    {                                                                                                                                      \
        vm.pSprite->ang += x;                                                                                                              \
        tw = A_CheckHitSprite(vm.spriteNum, &temphit);                                                                                     \
        vm.pSprite->ang -= x;                                                                                                              \
    } while (0)

                if (tw > dist)
                {
                    CHECK(temphit);
                    CHECK2(angDiff);

                    if (tw > dist)
                    {
                        CHECK(temphit);
                        CHECK2(-angDiff);

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
            tw = cansee(vm.pSprite->x, vm.pSprite->y, vm.pSprite->z-((krand()&41)<<8),
                               vm.pSprite->sectnum, pPlayer->pos.x, pPlayer->pos.y,
                               pPlayer->pos.z/*-((krand()&41)<<8)*/, sprite[pPlayer->i].sectnum);
            VM_CONDITIONAL(tw);
            if (tw) vm.pActor->timetosleep = SLEEPTIME;
        continue;

        case CON_IFACTORNOTSTAYPUT:
            VM_CONDITIONAL(vm.pActor->actorstayput == -1);
            continue;

        case CON_IFCANSEE:
        {
            uspritetype *pSprite = (uspritetype *)&sprite[pPlayer->i];

            // select sprite for monster to target
            // if holoduke is on, let them target holoduke first.
            //
            if (pPlayer->holoduke_on >= 0)
            {
                pSprite = (uspritetype *)&sprite[pPlayer->holoduke_on];
                tw = cansee(vm.pSprite->x,vm.pSprite->y,vm.pSprite->z-(krand()&(ZOFFSET5-1)),vm.pSprite->sectnum,
                           pSprite->x,pSprite->y,pSprite->z,pSprite->sectnum);

                if (tw == 0)
                {
                    // they can't see player's holoduke
                    // check for player...
                    pSprite = (uspritetype *)&sprite[pPlayer->i];
                }
            }

            // can they see player, (or player's holoduke)
            tw = cansee(vm.pSprite->x,vm.pSprite->y,vm.pSprite->z-(krand()&((47<<8))),vm.pSprite->sectnum,
                       pSprite->x,pSprite->y,pSprite->z-(24<<8),pSprite->sectnum);

            if (tw == 0)
            {
                // search around for target player

                // also modifies 'target' x&y if found..

                tw = 1;
                if (A_FurthestVisiblePoint(vm.spriteNum,pSprite,&vm.pActor->lastv) == -1)
                    tw = 0;
            }
            else
            {
                // else, they did see it.
                // save where we were looking...
                vm.pActor->lastv.x = pSprite->x;
                vm.pActor->lastv.y = pSprite->y;
            }

            if (tw && (vm.pSprite->statnum == STAT_ACTOR || vm.pSprite->statnum == STAT_STANDABLE))
                vm.pActor->timetosleep = SLEEPTIME;

            VM_CONDITIONAL(tw);
            continue;
        }

        case CON_IFHITWEAPON:
            VM_CONDITIONAL(A_IncurDamage(vm.spriteNum) >= 0);
            continue;

        case CON_IFSQUISHED:
            VM_CONDITIONAL(VM_CheckSquished());
            continue;

        case CON_IFDEAD:
            VM_CONDITIONAL(vm.pSprite->extra <= 0);
            continue;

        case CON_AI:
            insptr++;
            //Following changed to use pointersizes
            AC_AI_ID(vm.pData)       = *insptr++;                     // Ai
            AC_ACTION_ID(vm.pData)   = *(apScript + AC_AI_ID(vm.pData));  // Action

            // NOTE: "if" check added in r1155. It used to be a pointer though.
            if (AC_AI_ID(vm.pData))
                AC_MOVE_ID(vm.pData) = *(apScript + AC_AI_ID(vm.pData) + 1);  // move

            vm.pSprite->hitag         = *(apScript + AC_AI_ID(vm.pData) + 2);  // move flags

            AC_COUNT(vm.pData)        = 0;
            AC_ACTION_COUNT(vm.pData) = 0;
            AC_CURFRAME(vm.pData)     = 0;

            if (!A_CheckEnemySprite(vm.pSprite) || vm.pSprite->extra > 0) // hack
                if (vm.pSprite->hitag&random_angle)
                    vm.pSprite->ang = krand()&2047;
            continue;

        case CON_ACTION:
            insptr++;
            AC_ACTION_COUNT(vm.pData) = 0;
            AC_CURFRAME(vm.pData)     = 0;
            AC_ACTION_ID(vm.pData)    = *insptr++;
            continue;

        case CON_IFPLAYERSL:
            VM_CONDITIONAL(numplayers < *(++insptr));
            continue;

        case CON_IFPDISTL:
            VM_CONDITIONAL(vm.playerDist < *(++insptr));
            if (vm.playerDist > MAXSLEEPDIST && vm.pActor->timetosleep == 0)
                vm.pActor->timetosleep = SLEEPTIME;
            continue;

        case CON_IFPDISTG:
            VM_CONDITIONAL(vm.playerDist > *(++insptr));
            if (vm.playerDist > MAXSLEEPDIST && vm.pActor->timetosleep == 0)
                vm.pActor->timetosleep = SLEEPTIME;
            continue;

        case CON_ADDSTRENGTH:
            insptr++;
            vm.pSprite->extra += *insptr++;
            continue;

        case CON_STRENGTH:
            insptr++;
            vm.pSprite->extra = *insptr++;
            continue;

        case CON_IFGOTWEAPONCE:
            insptr++;

            if ((g_gametypeFlags[ud.coop]&GAMETYPE_WEAPSTAY) && (g_netServer || ud.multimode > 1))
            {
                if (*insptr == 0)
                {
                    int j = 0;
                    for (; j < pPlayer->weapreccnt; ++j)
                        if (pPlayer->weaprecs[j] == vm.pSprite->picnum)
                            break;

                    VM_CONDITIONAL(j < pPlayer->weapreccnt && vm.pSprite->owner == vm.spriteNum);
                    continue;
                }
                else if (pPlayer->weapreccnt < MAX_WEAPONS)
                {
                    pPlayer->weaprecs[pPlayer->weapreccnt++] = vm.pSprite->picnum;
                    VM_CONDITIONAL(vm.pSprite->owner == vm.spriteNum);
                    continue;
                }
            }
            VM_CONDITIONAL(0);
            continue;

        case CON_GETLASTPAL:
            insptr++;
            if (vm.pSprite->picnum == APLAYER)
                vm.pSprite->pal = g_player[P_GetP(vm.pSprite)].ps->palookup;
            else
            {
                if (vm.pSprite->pal == 1 && vm.pSprite->extra == 0) // hack for frozen
                    vm.pSprite->extra++;
                vm.pSprite->pal = vm.pActor->tempang;
            }
            vm.pActor->tempang = 0;
            continue;

        case CON_TOSSWEAPON:
            insptr++;
            // NOTE: assumes that current actor is APLAYER
            P_DropWeapon(P_GetP(vm.pSprite));
            continue;

        case CON_MIKESND:
            insptr++;
            if (EDUKE32_PREDICT_FALSE(((unsigned)vm.pSprite->yvel >= MAXSOUNDS)))
            {
                CON_ERRPRINTF("invalid sound %d\n", vm.pUSprite->yvel);
                continue;
            }
            if (!S_CheckSoundPlaying(vm.spriteNum,vm.pSprite->yvel))
                A_PlaySound(vm.pSprite->yvel,vm.spriteNum);
            continue;

        case CON_PKICK:
            insptr++;

            if ((g_netServer || ud.multimode > 1) && vm.pSprite->picnum == APLAYER)
            {
                if (g_player[otherp].ps->quick_kick == 0)
                    g_player[otherp].ps->quick_kick = 14;
            }
            else if (vm.pSprite->picnum != APLAYER && pPlayer->quick_kick == 0)
                pPlayer->quick_kick = 14;
            continue;

        case CON_SIZETO:
            insptr++;

            tw = (*insptr++ - vm.pSprite->xrepeat)<<1;
            vm.pSprite->xrepeat += ksgn(tw);

            if ((vm.pSprite->picnum == APLAYER && vm.pSprite->yrepeat < 36) || *insptr < vm.pSprite->yrepeat ||
                    ((vm.pSprite->yrepeat*(tilesiz[vm.pSprite->picnum].y+8))<<2) < (vm.pActor->floorz - vm.pActor->ceilingz))
            {
                tw = ((*insptr)-vm.pSprite->yrepeat)<<1;
                if (klabs(tw)) vm.pSprite->yrepeat += ksgn(tw);
            }

            insptr++;

            continue;

        case CON_SIZEAT:
            insptr++;
            vm.pSprite->xrepeat = (uint8_t)*insptr++;
            vm.pSprite->yrepeat = (uint8_t)*insptr++;
            continue;

        case CON_SOUNDONCE:
            if (EDUKE32_PREDICT_FALSE((unsigned)*(++insptr) >= MAXSOUNDS))
            {
                CON_ERRPRINTF("invalid sound %d\n", (int32_t)*insptr++);
                continue;
            }

            if (!S_CheckSoundPlaying(vm.spriteNum, *insptr++))
                A_PlaySound(*(insptr-1),vm.spriteNum);

            continue;

        case CON_IFACTORSOUND:
            insptr++;
            {
                int const spriteNum = Gv_GetVarX(*insptr++);
                int const soundNum  = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)soundNum >= MAXSOUNDS))
                {
                    CON_ERRPRINTF("invalid sound %d\n", soundNum);
                    insptr++;
                    continue;
                }

                insptr--;
                VM_CONDITIONAL(A_CheckSoundPlaying(spriteNum, soundNum));
            }
            continue;

        case CON_IFSOUND:
            if (EDUKE32_PREDICT_FALSE((unsigned)*(++insptr) >= MAXSOUNDS))
            {
                CON_ERRPRINTF("invalid sound %d\n", (int32_t)*insptr);
                insptr++;
                continue;
            }
            VM_CONDITIONAL(S_CheckSoundPlaying(vm.spriteNum,*insptr));
            //    VM_DoConditional(SoundOwner[*insptr][0].ow == vm.spriteNum);
            continue;

        case CON_STOPSOUND:
            if (EDUKE32_PREDICT_FALSE((unsigned)*(++insptr) >= MAXSOUNDS))
            {
                CON_ERRPRINTF("invalid sound %d\n", (int32_t)*insptr);
                insptr++;
                continue;
            }
            if (S_CheckSoundPlaying(vm.spriteNum,*insptr))
                S_StopSound((int16_t)*insptr);
            insptr++;
            continue;

        case CON_STOPACTORSOUND:
            insptr++;
            {
                int const spriteNum = Gv_GetVarX(*insptr++);
                int const soundNum  = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)soundNum >= MAXSOUNDS))
                {
                    CON_ERRPRINTF("invalid sound %d\n", soundNum);
                    continue;
                }

                if (A_CheckSoundPlaying(spriteNum, soundNum))
                    S_StopEnvSound(soundNum, spriteNum);

                continue;
            }

        case CON_ACTORSOUND:
            insptr++;
            {
                int const spriteNum = (*insptr++ != g_thisActorVarID) ? Gv_GetVarX(*(insptr - 1)) : vm.spriteNum;
                int const soundNum  = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)soundNum >= MAXSOUNDS))
                {
                    CON_ERRPRINTF("invalid sound %d\n", soundNum);
                    continue;
                }

                A_PlaySound(soundNum, spriteNum);

                continue;
            }

        case CON_SETACTORSOUNDPITCH:
            insptr++;
            {
                int const spriteNum = Gv_GetVarX(*insptr++);
                int const soundNum  = Gv_GetVarX(*insptr++);
                int const newPitch  = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)soundNum>=MAXSOUNDS))
                {
                    CON_ERRPRINTF("invalid sound %d\n", soundNum);
                    continue;
                }

                S_ChangeSoundPitch(soundNum, spriteNum, newPitch);

                continue;
            }

        case CON_GLOBALSOUND:
            if (EDUKE32_PREDICT_FALSE((unsigned)*(++insptr) >= MAXSOUNDS))
            {
                CON_ERRPRINTF("invalid sound %d\n", (int32_t)*insptr);
                insptr++;
                continue;
            }
            if (vm.playerNum == screenpeek || (g_gametypeFlags[ud.coop]&GAMETYPE_COOPSOUND)
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
                CON_ERRPRINTF("invalid sound %d\n", (int32_t)*insptr);
                insptr++;
                continue;
            }
            A_PlaySound(*insptr++,vm.spriteNum);
            continue;

        case CON_TIP:
            insptr++;
            pPlayer->tipincs = GAMETICSPERSEC;
            continue;

        case CON_FALL:
            insptr++;
            VM_Fall(vm.spriteNum, vm.pSprite);
            continue;

        case CON_RETURN:
            vm.flags |= VM_RETURN;
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
                int const weaponNum = *insptr++;
                int const addAmount = *insptr++;

                VM_AddAmmo(pPlayer, weaponNum, addAmount);

                continue;
            }

        case CON_MONEY:
            insptr++;
            A_SpawnMultiple(vm.spriteNum, MONEY, *insptr++);
            continue;

        case CON_MAIL:
            insptr++;
            A_SpawnMultiple(vm.spriteNum, MAIL, *insptr++);
            continue;

        case CON_SLEEPTIME:
            insptr++;
            vm.pActor->timetosleep = (int16_t)*insptr++;
            continue;

        case CON_PAPER:
            insptr++;
            A_SpawnMultiple(vm.spriteNum, PAPER, *insptr++);
            continue;

        case CON_ADDKILLS:
            insptr++;
            P_AddKills(pPlayer, *insptr++);
            vm.pActor->actorstayput = -1;
            continue;

        case CON_LOTSOFGLASS:
            insptr++;
            A_SpawnGlass(vm.spriteNum,*insptr++);
            continue;

        case CON_KILLIT:
            insptr++;
            vm.flags |= VM_KILL;
            return;

        case CON_ADDWEAPON:
            insptr++;
            {
                int const weaponNum = *insptr++;
                VM_AddWeapon(pPlayer, weaponNum, *insptr++);
                continue;
            }

        case CON_DEBUG:
            insptr++;
            buildprint(*insptr++, "\n");
            continue;

        case CON_ENDOFGAME:
        case CON_ENDOFLEVEL:
            insptr++;
            pPlayer->timebeforeexit = *insptr++;
            pPlayer->customexitsound = -1;
            ud.eog = 1;
            continue;

        case CON_ADDPHEALTH:
            insptr++;

            {
                if (pPlayer->newowner >= 0)
                    G_ClearCameraView(pPlayer);

                int newHealth = sprite[pPlayer->i].extra;

                if (vm.pSprite->picnum != ATOMICHEALTH)
                {
                    if (newHealth > pPlayer->max_player_health && *insptr > 0)
                    {
                        insptr++;
                        continue;
                    }
                    else
                    {
                        if (newHealth > 0)
                            newHealth += *insptr;
                        if (newHealth > pPlayer->max_player_health && *insptr > 0)
                            newHealth = pPlayer->max_player_health;
                    }
                }
                else
                {
                    if (newHealth > 0)
                        newHealth += *insptr;
                    if (newHealth > (pPlayer->max_player_health<<1))
                        newHealth = (pPlayer->max_player_health<<1);
                }

                if (newHealth < 0) newHealth = 0;

                if (ud.god == 0)
                {
                    if (*insptr > 0)
                    {
                        if ((newHealth - *insptr) < (pPlayer->max_player_health>>2) &&
                                newHealth >= (pPlayer->max_player_health>>2))
                            A_PlaySound(DUKE_GOTHEALTHATLOW,pPlayer->i);

                        pPlayer->last_extra = newHealth;
                    }

                    sprite[pPlayer->i].extra = newHealth;
                }
            }

            insptr++;
            continue;

        case CON_MOVE:
            insptr++;
            AC_COUNT(vm.pData) = 0;
            AC_MOVE_ID(vm.pData) = *insptr++;
            vm.pSprite->hitag = *insptr++;
            if (A_CheckEnemySprite(vm.pSprite) && vm.pSprite->extra <= 0) // hack
                continue;
            if (vm.pSprite->hitag&random_angle)
                vm.pSprite->ang = krand()&2047;
            continue;

        case CON_ADDWEAPONVAR:
            insptr++;
            {
                int const weaponNum = Gv_GetVarX(*insptr++);
                VM_AddWeapon(pPlayer, weaponNum, Gv_GetVarX(*insptr++));
                continue;
            }

        case CON_SETASPECT:
            insptr++;
            {
                int const xRange = Gv_GetVarX(*insptr++);
                setaspect(xRange, Gv_GetVarX(*insptr++));
                break;
            }

        case CON_SSP:
            insptr++;
            {
                int const spriteNum = Gv_GetVarX(*insptr++);
                int const clipType  = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)spriteNum >= MAXSPRITES))
                {
                    CON_ERRPRINTF("invalid sprite %d\n", spriteNum);
                    break;
                }
                A_SetSprite(spriteNum, clipType);
                break;
            }

        case CON_ACTIVATEBYSECTOR:
            insptr++;
            {
                int const sectNum   = Gv_GetVarX(*insptr++);
                int const spriteNum = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)sectNum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("invalid sector %d\n", sectNum);
                    break;
                }
                G_ActivateBySector(sectNum, spriteNum);
                break;
            }

        case CON_OPERATESECTORS:
            insptr++;
            {
                int const sectNum   = Gv_GetVarX(*insptr++);
                int const spriteNum = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)sectNum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("invalid sector %d\n", sectNum);
                    break;
                }
                G_OperateSectors(sectNum, spriteNum);
                break;
            }

        case CON_OPERATEACTIVATORS:
            insptr++;
            {
                int const nTag      = Gv_GetVarX(*insptr++);
                int const playerNum = (*insptr++ == g_thisActorVarID) ? vm.playerNum : Gv_GetVarX(*(insptr-1));

                if (EDUKE32_PREDICT_FALSE((unsigned)playerNum >= (unsigned)g_mostConcurrentPlayers))
                {
                    CON_ERRPRINTF("invalid player %d\n", playerNum);
                    break;
                }
                G_OperateActivators(nTag, playerNum);
                break;
            }


        case CON_CANSEESPR:
            insptr++;
            {
                int const nSprite1 = Gv_GetVarX(*insptr++);
                int const nSprite2 = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)nSprite1 >= MAXSPRITES || (unsigned)nSprite2 >= MAXSPRITES))
                {
                    CON_ERRPRINTF("invalid sprite %d\n", (unsigned)nSprite1 >= MAXSPRITES ? nSprite1 : nSprite2);
                    insptr++;
                    continue;
                }

                int const nResult = cansee(sprite[nSprite1].x, sprite[nSprite1].y, sprite[nSprite1].z, sprite[nSprite1].sectnum,
                                sprite[nSprite2].x, sprite[nSprite2].y, sprite[nSprite2].z, sprite[nSprite2].sectnum);

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
            aGameVars[g_returnVarID].global = G_CheckActivatorMotion(Gv_GetVarX(*insptr++));
            continue;

        case CON_INSERTSPRITEQ:
            insptr++;
            A_AddToDeleteQueue(vm.spriteNum);
            continue;

        case CON_QSTRLEN:
            insptr++;
            {
                int const gameVar    = *insptr++;
                int const quoteNum   = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE(apStrings[quoteNum] == NULL))
                {
                    CON_ERRPRINTF("null quote %d\n", quoteNum);
                    Gv_SetVarX(gameVar, -1);
                    continue;
                }

                Gv_SetVarX(gameVar, Bstrlen(apStrings[quoteNum]));
                continue;
            }

        case CON_QSTRDIM:
            insptr++;
            {
                int const widthVar  = *insptr++;
                int const heightVar = *insptr++;

                struct {
                    int32_t tileNum;
                    vec3_t  vect;
                    int32_t blockAngle, quoteNum, orientation;
                    vec2_t  offset, between;
                    int32_t f;
                    vec2_t  bound[2];
                } v;
                Gv_FillWithVars(v);

                if (EDUKE32_PREDICT_FALSE(v.tileNum < 0 || v.tileNum + 127 >= MAXTILES))
                    CON_ERRPRINTF("invalid base tilenum %d\n", v.tileNum);
                else if (EDUKE32_PREDICT_FALSE((unsigned)v.quoteNum >= MAXQUOTES || apStrings[v.quoteNum] == NULL))
                    CON_ERRPRINTF("invalid quote %d\n", v.quoteNum);
                else
                {
                    vec2_t dim = G_ScreenTextSize(v.tileNum, v.vect.x, v.vect.y, v.vect.z, v.blockAngle, apStrings[v.quoteNum],
                                                  2 | v.orientation, v.offset.x, v.offset.y, v.between.x, v.between.y, v.f,
                                                  v.bound[0].x, v.bound[0].y, v.bound[1].x, v.bound[1].y);

                    Gv_SetVarX(widthVar, dim.x);
                    Gv_SetVarX(heightVar, dim.y);
                }
                continue;
            }

        case CON_HEADSPRITESTAT:
            insptr++;
            {
                int const gameVar   = *insptr++;
                int const statNum = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)statNum > MAXSTATUS))
                {
                    CON_ERRPRINTF("invalid status list %d\n", statNum);
                    continue;
                }

                Gv_SetVarX(gameVar, headspritestat[statNum]);
                continue;
            }

        case CON_PREVSPRITESTAT:
            insptr++;
            {
                int const gameVar   = *insptr++;
                int const spriteNum = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)spriteNum >= MAXSPRITES))
                {
                    CON_ERRPRINTF("invalid sprite %d\n", spriteNum);
                    continue;
                }

                Gv_SetVarX(gameVar, prevspritestat[spriteNum]);
                continue;
            }

        case CON_NEXTSPRITESTAT:
            insptr++;
            {
                int const gameVar   = *insptr++;
                int const spriteNum = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)spriteNum >= MAXSPRITES))
                {
                    CON_ERRPRINTF("invalid sprite %d\n", spriteNum);
                    continue;
                }

                Gv_SetVarX(gameVar, nextspritestat[spriteNum]);
                continue;
            }

        case CON_HEADSPRITESECT:
            insptr++;
            {
                int const gameVar   = *insptr++;
                int const sectNum = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)sectNum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("invalid sector %d\n", sectNum);
                    continue;
                }

                Gv_SetVarX(gameVar, headspritesect[sectNum]);
                continue;
            }

        case CON_PREVSPRITESECT:
            insptr++;
            {
                int const gameVar   = *insptr++;
                int const spriteNum = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)spriteNum >= MAXSPRITES))
                {
                    CON_ERRPRINTF("invalid sprite %d\n", spriteNum);
                    continue;
                }

                Gv_SetVarX(gameVar, prevspritesect[spriteNum]);
                continue;
            }

        case CON_NEXTSPRITESECT:
            insptr++;
            {
                int const gameVar   = *insptr++;
                int const spriteNum = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)spriteNum >= MAXSPRITES))
                {
                    CON_ERRPRINTF("invalid sprite %d\n", spriteNum);
                    continue;
                }

                Gv_SetVarX(gameVar, nextspritesect[spriteNum]);
                continue;
            }

        case CON_GETKEYNAME:
            insptr++;
            {
                int const quoteIndex = Gv_GetVarX(*insptr++);
                int const gameFunc   = Gv_GetVarX(*insptr++);
                int const funcPos    = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)quoteIndex >= MAXQUOTES || apStrings[quoteIndex] == NULL))
                {
                    CON_ERRPRINTF("invalid quote %d\n", quoteIndex);
                    continue;
                }
                else if (EDUKE32_PREDICT_FALSE((unsigned)gameFunc >= NUMGAMEFUNCTIONS))
                {
                    CON_ERRPRINTF("invalid function %d\n", gameFunc);
                    continue;
                }
                else
                {
                    if (funcPos < 2)
                        Bstrcpy(tempbuf, KB_ScanCodeToString(ud.config.KeyboardKeys[gameFunc][funcPos]));
                    else
                    {
                        Bstrcpy(tempbuf, KB_ScanCodeToString(ud.config.KeyboardKeys[gameFunc][0]));

                        if (!*tempbuf)
                            Bstrcpy(tempbuf, KB_ScanCodeToString(ud.config.KeyboardKeys[gameFunc][1]));
                    }
                }

                if (*tempbuf)
                    Bstrcpy(apStrings[quoteIndex], tempbuf);

                continue;
            }

        case CON_QSUBSTR:
            insptr++;
            {
                struct {
                    int32_t outputQuote, inputQuote, quotePos, quoteLength;
                } v;
                Gv_FillWithVars(v);

                if (EDUKE32_PREDICT_FALSE((unsigned)v.outputQuote >= MAXQUOTES || apStrings[v.outputQuote] == NULL
                                          || (unsigned)v.inputQuote >= MAXQUOTES || apStrings[v.inputQuote] == NULL))
                {
                    CON_ERRPRINTF("invalid quote %d\n", apStrings[v.outputQuote] ? v.inputQuote : v.outputQuote);
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE((unsigned)v.quotePos >= MAXQUOTELEN))
                {
                    CON_ERRPRINTF("invalid position %d\n", v.quotePos);
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE(v.quoteLength < 0))
                {
                    CON_ERRPRINTF("invalid length %d\n", v.quoteLength);
                    continue;
                }

                char *      pOutput = apStrings[v.outputQuote];
                char const *pInput  = apStrings[v.inputQuote];

                while (*pInput && v.quotePos--) pInput++;
                while ((*pOutput = *pInput) && v.quoteLength--)
                {
                    pOutput++;
                    pInput++;
                }
                *pOutput = '\0';

                continue;
            }

        case CON_GETPNAME:
        case CON_QSTRNCAT:
        case CON_QSTRCAT:
        case CON_QSTRCPY:
        case CON_QGETSYSSTR:
            insptr++;
            {
                int32_t i = Gv_GetVarX(*insptr++), j;
                if (tw == CON_GETPNAME && *insptr == g_thisActorVarID)
                {
                    j = vm.playerNum;
                    insptr++;
                }
                else j = Gv_GetVarX(*insptr++);

                switch (tw)
                {
                case CON_GETPNAME:
                    if (EDUKE32_PREDICT_FALSE((unsigned)i>=MAXQUOTES || apStrings[i] == NULL))
                    {
                        CON_ERRPRINTF("invalid quote %d\n", i);
                        break;
                    }
                    if (g_player[j].user_name[0])
                        Bstrcpy(apStrings[i],g_player[j].user_name);
                    else Bsprintf(apStrings[i],"%d",j);
                    break;
                case CON_QGETSYSSTR:
                    if (EDUKE32_PREDICT_FALSE((unsigned)i>=MAXQUOTES || apStrings[i] == NULL))
                    {
                        CON_ERRPRINTF("invalid quote %d\n", i);
                        break;
                    }
                    switch (j)
                    {
                    case STR_MAPNAME:
                    case STR_MAPFILENAME:
                    {
                        int32_t levelNum = ud.volume_number*MAXLEVELS + ud.level_number;
                        const char *pName;

                        if (EDUKE32_PREDICT_FALSE((unsigned)levelNum >= ARRAY_SIZE(g_mapInfo)))
                        {
                            CON_ERRPRINTF("out of bounds map number (vol=%d, lev=%d)\n",
                                          ud.volume_number, ud.level_number);
                            break;
                        }

                        pName = j == STR_MAPNAME ? g_mapInfo[levelNum].name : g_mapInfo[levelNum].filename;

                        if (EDUKE32_PREDICT_FALSE(pName == NULL))
                        {
                            CON_ERRPRINTF("attempted access to %s of non-existent map (vol=%d, lev=%d)",
                                          j==STR_MAPNAME ? "name" : "file name",
                                          ud.volume_number, ud.level_number);
                            break;
                        }

                        Bstrcpy(apStrings[i], j==STR_MAPNAME ? g_mapInfo[levelNum].name : g_mapInfo[levelNum].filename);
                        break;
                    }
                    case STR_PLAYERNAME:
                        if (EDUKE32_PREDICT_FALSE((unsigned)vm.playerNum >= (unsigned)g_mostConcurrentPlayers))
                        {
                            CON_ERRPRINTF("invalid player %d\n", vm.playerNum);
                            break;
                        }
                        Bstrcpy(apStrings[i],g_player[vm.playerNum].user_name);
                        break;
                    case STR_VERSION:
                        Bsprintf(tempbuf,HEAD2 " %s",s_buildRev);
                        Bstrcpy(apStrings[i],tempbuf);
                        break;
                    case STR_GAMETYPE:
                        Bstrcpy(apStrings[i],g_gametypeNames[ud.coop]);
                        break;
                    case STR_VOLUMENAME:
                        if (EDUKE32_PREDICT_FALSE((unsigned)ud.volume_number >= MAXVOLUMES))
                        {
                            CON_ERRPRINTF("invalid volume %d\n", ud.volume_number);
                            break;
                        }
                        Bstrcpy(apStrings[i],g_volumeNames[ud.volume_number]);
                        break;
                    case STR_YOURTIME:
                        Bstrcpy(apStrings[i],G_PrintYourTime());
                        break;
                    case STR_PARTIME:
                        Bstrcpy(apStrings[i],G_PrintParTime());
                        break;
                    case STR_DESIGNERTIME:
                        Bstrcpy(apStrings[i],G_PrintDesignerTime());
                        break;
                    case STR_BESTTIME:
                        Bstrcpy(apStrings[i],G_PrintBestTime());
                        break;
                    default:
                        CON_ERRPRINTF("invalid string index %d or %d\n", i, j);
                    }
                    break;
                case CON_QSTRCAT:
                    if (EDUKE32_PREDICT_FALSE(apStrings[i] == NULL || apStrings[j] == NULL)) goto nullquote;
                    Bstrncat(apStrings[i],apStrings[j],(MAXQUOTELEN-1)-Bstrlen(apStrings[i]));
                    break;
                case CON_QSTRNCAT:
                    if (EDUKE32_PREDICT_FALSE(apStrings[i] == NULL || apStrings[j] == NULL)) goto nullquote;
                    Bstrncat(apStrings[i],apStrings[j],Gv_GetVarX(*insptr++));
                    break;
                case CON_QSTRCPY:
                    if (EDUKE32_PREDICT_FALSE(apStrings[i] == NULL || apStrings[j] == NULL)) goto nullquote;
                    if (i != j)
                        Bstrcpy(apStrings[i],apStrings[j]);
                    break;
                default:
nullquote:
                    CON_ERRPRINTF("invalid quote %d\n", apStrings[i] ? j : i);
                    break;
                }
                continue;
            }

        case CON_CHANGESPRITESECT:
            insptr++;
            {
                int32_t const spriteNum = Gv_GetVarX(*insptr++);
                int32_t       sectNum   = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)spriteNum >= MAXSPRITES || (unsigned) sectNum >= MAXSECTORS))
                {
                    CON_ERRPRINTF("invalid parameters: %d, %d\n", spriteNum, sectNum);
                    continue;
                }

                if (sprite[spriteNum].sectnum == sectNum)
                    continue;

                changespritesect(spriteNum, sectNum);
                continue;
            }

        case CON_CHANGESPRITESTAT:
            insptr++;
            {
                int32_t const spriteNum = Gv_GetVarX(*insptr++);
                int32_t       statNum   = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned) spriteNum >= MAXSPRITES || (unsigned) statNum >= MAXSECTORS))
                {
                    CON_ERRPRINTF("invalid parameters: %d, %d\n", spriteNum, statNum);
                    continue;
                }

                if (sprite[spriteNum].statnum == statNum)
                    continue;

                /* initialize actor data when changing to an actor statnum because there's usually
                garbage left over from being handled as a hard coded object */

                if (sprite[spriteNum].statnum > STAT_ZOMBIEACTOR && (statNum == STAT_ACTOR || statNum == STAT_ZOMBIEACTOR))
                {
                    actor_t * const pActor = &actor[spriteNum];

                    Bmemset(&pActor->t_data,  0, sizeof pActor->t_data);
                    pActor->lastv.x          = 0;
                    pActor->lastv.y          = 0;
                    pActor->timetosleep     = 0;
                    pActor->cgg             = 0;
                    pActor->movflag         = 0;
                    pActor->tempang         = 0;
                    pActor->dispicnum       = 0;
                    pActor->flags           = 0;
                    sprite[spriteNum].hitag = 0;

                    if (G_HaveActor(sprite[spriteNum].picnum))
                    {
                        const intptr_t *actorptr = g_tile[sprite[spriteNum].picnum].execPtr;
                        // offsets
                        AC_ACTION_ID(pActor->t_data) = actorptr[1];
                        AC_MOVE_ID(pActor->t_data) = actorptr[2];
                        AC_MOVFLAGS(&sprite[spriteNum], &actor[spriteNum]) = actorptr[3];  // ai bits (movflags)
                    }
                }

                changespritestat(spriteNum, statNum);
                continue;
            }

        case CON_STARTLEVEL:
            insptr++; // skip command
            {
                // from 'level' cheat in game.c (about line 6250)
                int const volumeNum = Gv_GetVarX(*insptr++);
                int const levelNum  = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)volumeNum >= MAXVOLUMES || (unsigned) levelNum >= MAXLEVELS))
                {
                    CON_ERRPRINTF("invalid parameters: %d, %d\n", volumeNum, levelNum);
                    continue;
                }

                ud.m_volume_number = ud.volume_number = volumeNum;
                ud.m_level_number = ud.level_number = levelNum;
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
                struct {
                    vec2_t pos;
                    int32_t tilenum, shade, orientation;
                } v;
                Gv_FillWithVars(v);

                switch (tw)
                {
                    case CON_MYOS:
                        VM_DrawTile(v.pos.x, v.pos.y, v.tilenum, v.shade, v.orientation);
                        break;
                    case CON_MYOSPAL:
                        VM_DrawTilePal(v.pos.x, v.pos.y, v.tilenum, v.shade, v.orientation, Gv_GetVarX(*insptr++));
                        break;
                    case CON_MYOSX:
                        VM_DrawTileSmall(v.pos.x, v.pos.y, v.tilenum, v.shade, v.orientation);
                        break;
                    case CON_MYOSPALX:
                        VM_DrawTilePalSmall(v.pos.x, v.pos.y, v.tilenum, v.shade, v.orientation, Gv_GetVarX(*insptr++));
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
                int32_t const lValue = Gv_GetVarX(*insptr++);
                int32_t const lEnd   = *insptr++;
                int32_t const lCases = *insptr++;

                intptr_t const *const lpDefault = insptr++;
                intptr_t const *const lpCases   = insptr;

                int left  = 0;
                int right = lCases - 1;

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
                        insptr = (intptr_t *)(lpCases[(lCheckCase << 1) + 1] + &apScript[0]);
                        VM_Execute(1);
                        goto matched;
                    }

                    if (right - left < 0)
                        break;
                }
                while (1);

                if (*lpDefault)
                {
                    insptr = (intptr_t *)(*lpDefault + &apScript[0]);
                    VM_Execute(1);
                }

            matched:
                insptr = (intptr_t *)(lEnd + (intptr_t)&apScript[0]);

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
                int const wallNum = Gv_GetVarX(*insptr++);
                vec2_t n;
                Gv_FillWithVars(n);

                if (EDUKE32_PREDICT_FALSE((unsigned)wallNum >= (unsigned)numwalls))
                {
                    CON_ERRPRINTF("invalid wall %d\n", wallNum);
                    continue;
                }

                dragpoint(wallNum, n.x, n.y, 0);
                continue;
            }

        case CON_LDIST:
        case CON_DIST:
            insptr++;
            {
                int const out = *insptr++;
                vec2_t in;
                Gv_FillWithVars(in);

                if (EDUKE32_PREDICT_FALSE((unsigned)in.x >= MAXSPRITES || (unsigned)in.y >= MAXSPRITES))
                {
                    CON_ERRPRINTF("invalid sprite %d, %d\n", in.x, in.y);
                    continue;
                }

                Gv_SetVarX(out, (tw == CON_LDIST ? ldist : dist)(&sprite[in.x], &sprite[in.y]));
                continue;
            }

        case CON_GETANGLE:
        case CON_GETINCANGLE:
            insptr++;
            {
                int const out = *insptr++;
                vec2_t in;
                Gv_FillWithVars(in);
                Gv_SetVarX(out, (tw == CON_GETANGLE ? getangle : G_GetAngleDelta)(in.x, in.y));
                continue;
            }

        case CON_MULSCALE:
        case CON_DIVSCALE:
            insptr++;
            {
                int const out = *insptr++;
                vec3_t in;
                Gv_FillWithVars(in);

                if (tw == CON_MULSCALE)
                    Gv_SetVarX(out, mulscale(in.x, in.y, in.z));
                else
                    Gv_SetVarX(out, divscale(in.x, in.y, in.z));

                continue;
            }

        case CON_SCALEVAR:
            insptr++;
            {
                int const out = *insptr++;
                vec3_t in;
                Gv_FillWithVars(in);
                Gv_SetVarX(out, scale(in.x, in.y, in.z));
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
                Gv_FillWithVars(params);
                aGameVars[g_returnVarID].global = nextsectorneighborz(params[0], params[1], params[2], params[3]);
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
                int const tileNum = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)vm.pSprite->sectnum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("invalid sector %d\n", vm.pUSprite->sectnum);
                    continue;
                }

                int const spriteNum = A_Spawn(vm.spriteNum, tileNum);

                switch (tw)
                {
                    case CON_EQSPAWNVAR:
                        if (spriteNum != -1)
                            A_AddToDeleteQueue(spriteNum);
                        fallthrough__;
                    case CON_ESPAWNVAR:
                        aGameVars[g_returnVarID].global = spriteNum;
                        break;
                    case CON_QSPAWNVAR:
                        if (spriteNum != -1)
                            A_AddToDeleteQueue(spriteNum);
                        break;
                }
                continue;
            }

        case CON_SHOOTVAR:
        case CON_ESHOOTVAR:
            insptr++;
            {
                int j = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)vm.pSprite->sectnum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("invalid sector %d\n", vm.pUSprite->sectnum);
                    continue;
                }

                j = A_Shoot(vm.spriteNum, j);

                if (tw == CON_ESHOOTVAR)
                    aGameVars[g_returnVarID].global = j;

                continue;
            }

        case CON_EZSHOOTVAR:
        case CON_ZSHOOTVAR:
            insptr++;
            {
                int const zvel = (int16_t)Gv_GetVarX(*insptr++);
                int j = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)vm.pSprite->sectnum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("invalid sector %d\n", vm.pUSprite->sectnum);
                    continue;
                }

                j = A_ShootWithZvel(vm.spriteNum, j, zvel);

                if (tw == CON_EZSHOOTVAR)
                    aGameVars[g_returnVarID].global = j;

                continue;
            }

        case CON_CMENU:
            insptr++;
            Menu_Change(Gv_GetVarX(*insptr++));
            continue;

        case CON_SOUNDVAR:
        case CON_STOPSOUNDVAR:
        case CON_SOUNDONCEVAR:
        case CON_GLOBALSOUNDVAR:
        case CON_SCREENSOUND:
            insptr++;
            {
                int const soundNum = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)soundNum>=MAXSOUNDS))
                {
                    CON_ERRPRINTF("invalid sound %d\n", soundNum);
                    continue;
                }

                switch (tw)
                {
                    case CON_SOUNDONCEVAR: // falls through to CON_SOUNDVAR
                        if (!S_CheckSoundPlaying(vm.spriteNum, soundNum))
                    case CON_SOUNDVAR:
                        A_PlaySound((int16_t)soundNum, vm.spriteNum);
                        continue;
                    case CON_GLOBALSOUNDVAR:
                        A_PlaySound((int16_t)soundNum, g_player[screenpeek].ps->i);
                        continue;
                    case CON_STOPSOUNDVAR:
                        if (S_CheckSoundPlaying(vm.spriteNum, soundNum))
                            S_StopSound((int16_t)soundNum);
                        continue;
                    case CON_SCREENSOUND:
                        A_PlaySound(soundNum, -1);
                        continue;
                }
            }
            continue;

        case CON_STARTCUTSCENE:
        case CON_IFCUTSCENE:
            insptr++;
            {
                int const nQuote = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)nQuote >= MAXQUOTES || apStrings[nQuote] == NULL))
                {
                    CON_ERRPRINTF("invalid quote %d for anim!\n", nQuote);
                    continue;
                }

                if (tw == CON_IFCUTSCENE)
                {
                    insptr--;
                    VM_CONDITIONAL(g_animPtr == Anim_Find(apStrings[nQuote]));
                    continue;
                }

                tw = pPlayer->palette;
                I_ClearAllInput();
                Anim_Play(apStrings[nQuote]);
                P_SetGamePalette(pPlayer, tw, 2 + 16);
                continue;
            }

        case CON_STARTSCREEN:
            insptr++;
            I_ClearAllInput();
            Screen_Play();
            continue;

        case CON_GUNIQHUDID:
            insptr++;
            {
                tw = Gv_GetVarX(*insptr++);
                if (EDUKE32_PREDICT_FALSE((unsigned)tw >= MAXUNIQHUDID - 1))
                    CON_ERRPRINTF("invalid value %d\n", tw);
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
                struct {
                    vec3_t vec;
                    int32_t params[3];
                    vec2_t scrn[2];
                } v;
                Gv_FillWithVars(v);

                if (EDUKE32_PREDICT_FALSE(v.scrn[0].x < 0 || v.scrn[0].y < 0 || v.scrn[1].x >= 320 || v.scrn[1].y >= 200))
                {
                    CON_ERRPRINTF("incorrect coordinates\n");
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE((unsigned)v.params[2] >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("invalid sector %d\n", v.params[2]);
                    continue;
                }

                G_ShowView(v.vec, v.params[0], v.params[1], v.params[2], v.scrn[0].x, v.scrn[0].y, v.scrn[1].x, v.scrn[1].y, (tw != CON_SHOWVIEW));

                continue;
            }

        case CON_ROTATESPRITEA:
        case CON_ROTATESPRITE16:
        case CON_ROTATESPRITE:
            insptr++;
            {
                struct {
                    vec3_t  pos;
                    int32_t ang, tilenum, shade, pal, orientation;
                } v;
                Gv_FillWithVars(v);

                int32_t alpha = (tw == CON_ROTATESPRITEA) ? Gv_GetVarX(*insptr++) : 0;

                vec2_t bound[2];
                Gv_FillWithVars(bound);

                if (tw != CON_ROTATESPRITE16 && !(v.orientation&ROTATESPRITE_FULL16))
                {
                    v.pos.x <<= 16;
                    v.pos.y <<= 16;
                }

                if (EDUKE32_PREDICT_FALSE((unsigned) v.tilenum >= MAXTILES))
                {
                    CON_ERRPRINTF("invalid tilenum %d\n", v.tilenum);
                    continue;
                }

                int32_t blendidx = 0;

                NEG_ALPHA_TO_BLEND(alpha, blendidx, v.orientation);

                rotatesprite_(v.pos.x, v.pos.y, v.pos.z, v.ang, v.tilenum, v.shade, v.pal, 2 | (v.orientation & (ROTATESPRITE_MAX - 1)),
                              alpha, blendidx, bound[0].x, bound[0].y, bound[1].x, bound[1].y);
                continue;
            }

        case CON_GAMETEXT:
        case CON_GAMETEXTZ:
            insptr++;
            {
                struct {
                    int32_t tilenum;
                    vec2_t  pos;
                    int32_t nQuote, shade, pal, orientation;
                    vec2_t  bound[2];
                } v;
                Gv_FillWithVars(v);

                int32_t const z = (tw == CON_GAMETEXTZ) ? Gv_GetVarX(*insptr++) : 65536;

                if (EDUKE32_PREDICT_FALSE(v.tilenum < 0 || v.tilenum + 127 >= MAXTILES))
                {
                    CON_ERRPRINTF("invalid base tilenum %d\n", v.tilenum);
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE((unsigned)v.nQuote >= MAXQUOTES || apStrings[v.nQuote] == NULL))
                {
                    CON_ERRPRINTF("invalid quote %d\n", v.nQuote);
                    continue;
                }

                G_PrintGameText(v.tilenum, v.pos.x >> 1, v.pos.y, apStrings[v.nQuote], v.shade, v.pal, v.orientation & (ROTATESPRITE_MAX - 1),
                                v.bound[0].x, v.bound[0].y, v.bound[1].x, v.bound[1].y, z, 0);
                continue;
            }

        case CON_DIGITALNUMBER:
        case CON_DIGITALNUMBERZ:
            insptr++;
            {
                struct {
                    int32_t tilenum;
                    vec2_t  pos;
                    int32_t nQuote, shade, pal, orientation;
                    vec2_t  bound[2];
                } v;
                Gv_FillWithVars(v);

                int32_t const nZoom = (tw == CON_DIGITALNUMBERZ) ? Gv_GetVarX(*insptr++) : 65536;

                // NOTE: '-' not taken into account, but we have rotatesprite() bound check now anyway
                if (EDUKE32_PREDICT_FALSE(v.tilenum < 0 || v.tilenum+9 >= MAXTILES))
                {
                    CON_ERRPRINTF("invalid base tilenum %d\n", v.tilenum);
                    continue;
                }

                G_DrawTXDigiNumZ(v.tilenum, v.pos.x, v.pos.y, v.nQuote, v.shade, v.pal, v.orientation & (ROTATESPRITE_MAX - 1),
                                 v.bound[0].x, v.bound[0].y, v.bound[1].x, v.bound[1].y, nZoom);
                continue;
            }

        case CON_MINITEXT:
            insptr++;
            {
                struct {
                    vec2_t  pos;
                    int32_t nQuote, shade, pal;
                } v;
                Gv_FillWithVars(v);

                if (EDUKE32_PREDICT_FALSE((unsigned)v.nQuote >= MAXQUOTES || apStrings[v.nQuote] == NULL))
                {
                    CON_ERRPRINTF("invalid quote %d\n", v.nQuote);
                    continue;
                }

                minitextshade(v.pos.x, v.pos.y, apStrings[v.nQuote], v.shade, v.pal, 2+8+16);
                continue;
            }

        case CON_SCREENTEXT:
            insptr++;
            {
                struct {
                    int32_t tilenum;
                    vec3_t  v;
                    int32_t blockangle, charangle, nQuote, shade, pal, orientation, alpha;
                    vec2_t  spacing, between;
                    int32_t nFlags;
                    vec2_t  bound[2];
                } v;
                Gv_FillWithVars(v);

                if (EDUKE32_PREDICT_FALSE(v.tilenum < 0 || v.tilenum+127 >= MAXTILES))
                {
                    CON_ERRPRINTF("invalid base tilenum %d\n", v.tilenum);
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE((unsigned)v.nQuote >= MAXQUOTES || apStrings[v.nQuote] == NULL))
                {
                    CON_ERRPRINTF("invalid quote %d\n", v.nQuote);
                    continue;
                }

                G_ScreenText(v.tilenum, v.v.x, v.v.y, v.v.z, v.blockangle, v.charangle, apStrings[v.nQuote], v.shade, v.pal, 2 | (v.orientation & (ROTATESPRITE_MAX - 1)),
                             v.alpha, v.spacing.x, v.spacing.y, v.between.x, v.between.y, v.nFlags, v.bound[0].x, v.bound[0].y, v.bound[1].x, v.bound[1].y);
                continue;
            }

        case CON_ANGOFF:
            insptr++;
            spriteext[vm.spriteNum].angoff=*insptr++;
            continue;

        case CON_GETZRANGE:
            insptr++;
            {
                struct {
                    vec3_t  vect;
                    int32_t sectNum;
                } v;
                Gv_FillWithVars(v);

                int const ceilzvar   = *insptr++;
                int const ceilhitvar = *insptr++;
                int const florzvar   = *insptr++;
                int const florhitvar = *insptr++;

                struct {
                    int32_t walldist, clipmask;
                } v2;
                Gv_FillWithVars(v2);

                if (EDUKE32_PREDICT_FALSE((unsigned)v.sectNum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("invalid sector %d\n", v.sectNum);
                    continue;
                }

                int32_t ceilz, ceilhit, florz, florhit;

                getzrange(&v.vect, v.sectNum, &ceilz, &ceilhit, &florz, &florhit, v2.walldist, v2.clipmask);
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
                    CON_ERRPRINTF("invalid sector %d\n", sectnum);
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
                int32_t returnVar = *insptr++;
                vec2_t  da;
                Gv_FillWithVars(da);
                int64_t const hypsq = (int64_t)da.x*da.x + (int64_t)da.y*da.y;

                Gv_SetVarX(returnVar, (hypsq > (int64_t) INT32_MAX)
                                      ? (int32_t)sqrt((double)hypsq)
                                      : ksqrt((uint32_t)hypsq));
                continue;
            }

        case CON_LINEINTERSECT:
        case CON_RAYINTERSECT:
            insptr++;
            {
                struct {
                    vec3_t vec[2];
                    vec2_t vec2[2];
                } v;
                Gv_FillWithVars(v);

                int const intxvar = *insptr++;
                int const intyvar = *insptr++;
                int const intzvar = *insptr++;
                int const retvar  = *insptr++;
                vec3_t    in;

                int ret = ((tw == CON_LINEINTERSECT) ? lintersect : rayintersect)(v.vec[0].x, v.vec[0].y, v.vec[0].z, v.vec[1].x,
                                                                                  v.vec[1].y, v.vec[1].z, v.vec2[0].x, v.vec2[0].y,
                                                                                  v.vec2[1].x, v.vec2[1].y, &in.x, &in.y, &in.z);

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

                int const returnVar    = *insptr++;
                int const xReturn      = *insptr++;
                int const yReturn      = *insptr++;

                insptr -= 2;

                typedef struct {
                    vec3_t vec3;
                    int32_t sectNum32;
                    vec2_t vec2;
                    vec3dist_t dist;
                    int32_t clipMask;
                } clipmoveparams_t;

                int32_t const sectReturn = insptr[offsetof(clipmoveparams_t, sectNum32)/sizeof(int32_t)];

                clipmoveparams_t v;
                Gv_FillWithVars(v);
                int16_t sectNum = v.sectNum32;

                if (EDUKE32_PREDICT_FALSE((unsigned)sectNum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("invalid sector %d\n", sectNum);
                    Gv_SetVarX(returnVar, 0);
                    continue;
                }

                Gv_SetVarX(returnVar,
                    clipmovex(&v.vec3, &sectNum, v.vec2.x, v.vec2.y, v.dist.w, v.dist.f, v.dist.c, v.clipMask, (tw == CON_CLIPMOVENOSLIDE)));
                Gv_SetVarX(sectReturn, v.sectNum32);
                Gv_SetVarX(xReturn, v.vec3.x);
                Gv_SetVarX(yReturn, v.vec3.y);

                continue;
            }

        case CON_HITSCAN:
            insptr++;
            {
                struct {
                    vec3_t origin;
                    int32_t sectnum;
                    vec3_t vect;
                } v;
                Gv_FillWithVars(v);

                int const sectReturn   = *insptr++;
                int const wallReturn   = *insptr++;
                int const spriteReturn = *insptr++;
                int const xReturn      = *insptr++;
                int const yReturn      = *insptr++;
                int const zReturn      = *insptr++;
                int const clipType     = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)v.sectnum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("invalid sector %d\n", v.sectnum);
                    continue;
                }

                hitdata_t hit;
                hitscan(&v.origin, v.sectnum, v.vect.x, v.vect.y, v.vect.z, &hit, clipType);

                Gv_SetVarX(sectReturn, hit.sect);
                Gv_SetVarX(wallReturn, hit.wall);
                Gv_SetVarX(spriteReturn, hit.sprite);
                Gv_SetVarX(xReturn, hit.pos.x);
                Gv_SetVarX(yReturn, hit.pos.y);
                Gv_SetVarX(zReturn, hit.pos.z);
                continue;
            }

        case CON_CANSEE:
            insptr++;
            {
                struct {
                    vec3_t  vec1;
                    int32_t firstSector;
                    vec3_t  vec2;
                    int32_t secondSector;
                } v;
                Gv_FillWithVars(v);

                int const returnVar = *insptr++;

                if (EDUKE32_PREDICT_FALSE((unsigned)v.firstSector >= (unsigned)numsectors ||
                                          (unsigned)v.secondSector >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("invalid sector %d\n", (unsigned)v.firstSector >= (unsigned)numsectors ? v.firstSector : v.secondSector);
                    Gv_SetVarX(returnVar, 0);
                }

                Gv_SetVarX(returnVar, cansee(v.vec1.x, v.vec1.y, v.vec1.z, v.firstSector, v.vec2.x, v.vec2.y, v.vec2.z, v.secondSector));
                continue;
            }

        case CON_ROTATEPOINT:
            insptr++;
            {
                struct {
                    vec2_t point[2];
                    int32_t angle;
                } v;
                Gv_FillWithVars(v);

                int const xReturn = *insptr++;
                int const yReturn = *insptr++;
                vec2_t    result;

                rotatepoint(v.point[0], v.point[1], v.angle, &result);

                Gv_SetVarX(xReturn, result.x);
                Gv_SetVarX(yReturn, result.y);
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

                struct {
                    vec3_t  point;
                    int32_t sectNum, nAngle;
                } v;
                Gv_FillWithVars(v);

                int const sectReturn   = *insptr++;
                int const wallReturn   = *insptr++;
                int const spriteReturn = *insptr++;
                int const distReturn   = *insptr++;

                struct {
                    int32_t tagRange, tagSearch;
                } v2;
                Gv_FillWithVars(v2);

                if (EDUKE32_PREDICT_FALSE((unsigned)v.sectNum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("Invalid sector %d\n", v.sectNum);
                    continue;
                }

                int16_t neartagsector, neartagwall, neartagsprite;
                int32_t neartaghitdist;

                neartag(v.point.x, v.point.y, v.point.z, v.sectNum, v.nAngle, &neartagsector, &neartagwall, &neartagsprite,
                        &neartaghitdist, v2.tagRange, v2.tagSearch, NULL);

                Gv_SetVarX(sectReturn, neartagsector);
                Gv_SetVarX(wallReturn, neartagwall);
                Gv_SetVarX(spriteReturn, neartagsprite);
                Gv_SetVarX(distReturn, neartaghitdist);
                continue;
            }

        case CON_GETTIMEDATE:
            insptr++;
            {
                int32_t values[8];
                G_GetTimeDate(values);

                for (native_t i = 0; i < 8; i++)
                    Gv_SetVarX(*insptr++, values[i]);

                continue;
            }

        case CON_MOVESPRITE:
            insptr++;
            {
                struct {
                    int32_t spriteNum;
                    vec3_t  vect;
                    int32_t clipType;
                } v;
                Gv_FillWithVars(v);

                if (EDUKE32_PREDICT_FALSE((unsigned)v.spriteNum >= MAXSPRITES))
                {
                    CON_ERRPRINTF("invalid sprite %d\n", v.spriteNum);
                    insptr++;
                    continue;
                }

                Gv_SetVarX(*insptr++, A_MoveSprite(v.spriteNum, &v.vect, v.clipType));
                continue;
            }

        case CON_SETSPRITE:
            insptr++;
            {
                struct {
                    int32_t spriteNum;
                    vec3_t  vect;
                } v;
                Gv_FillWithVars(v);

                if (EDUKE32_PREDICT_FALSE((unsigned)v.spriteNum >= MAXSPRITES))
                {
                    CON_ERRPRINTF("invalid sprite %d\n", v.spriteNum);
                    continue;
                }
                setsprite(v.spriteNum, &v.vect);
                continue;
            }

        case CON_GETFLORZOFSLOPE:
        case CON_GETCEILZOFSLOPE:
            insptr++;
            {
                struct {
                    int32_t sectNum;
                    vec2_t  vect;
                } v;
                Gv_FillWithVars(v);

                if (EDUKE32_PREDICT_FALSE((unsigned)v.sectNum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("invalid sector %d\n", v.sectNum);
                    insptr++;
                    continue;
                }
                Gv_SetVarX(*insptr++, (tw == CON_GETFLORZOFSLOPE ? getflorzofslope : getceilzofslope)(v.sectNum, v.vect.x, v.vect.y));
                continue;
            }

        case CON_UPDATESECTOR:
            insptr++;
            {
                vec2_t vect         = { 0, 0 };
                Gv_FillWithVars(vect);

                int const returnVar = *insptr++;
                int16_t   sectNum   = sprite[vm.spriteNum].sectnum;

                updatesector(vect.x, vect.y, &sectNum);
                Gv_SetVarX(returnVar, sectNum);
                continue;
            }

        case CON_UPDATESECTORZ:
            insptr++;
            {
                vec3_t vect         = { 0, 0, 0 };
                Gv_FillWithVars(vect);

                int const returnVar = *insptr++;
                int16_t   sectNum   = sprite[vm.spriteNum].sectnum;

                updatesectorz(vect.x, vect.y, vect.z, &sectNum);
                Gv_SetVarX(returnVar, sectNum);
                continue;
            }

        case CON_SPAWN:
            insptr++;
            if ((unsigned)vm.pSprite->sectnum >= MAXSECTORS)
            {
                CON_ERRPRINTF("invalid sector %d\n", vm.pUSprite->sectnum);
                insptr++;
                continue;
            }
            A_Spawn(vm.spriteNum,*insptr++);
            continue;

        case CON_IFWASWEAPON:
        case CON_IFSPAWNEDBY:
            insptr++;
            VM_CONDITIONAL(vm.pActor->picnum == *insptr);
            continue;

        case CON_IFAI:
            insptr++;
            VM_CONDITIONAL(AC_AI_ID(vm.pData) == *insptr);
            continue;

        case CON_IFACTION:
            insptr++;
            VM_CONDITIONAL(AC_ACTION_ID(vm.pData) == *insptr);
            continue;

        case CON_IFACTIONCOUNT:
            insptr++;
            VM_CONDITIONAL(AC_ACTION_COUNT(vm.pData) >= *insptr);
            continue;

        case CON_RESETACTIONCOUNT:
            insptr++;
            AC_ACTION_COUNT(vm.pData) = 0;
            continue;

        case CON_DEBRIS:
            insptr++;
            {
                int debrisTile = *insptr++;

                if ((unsigned)vm.pSprite->sectnum < MAXSECTORS)
                    for (native_t cnt = (*insptr) - 1; cnt >= 0; cnt--)
                    {
                        int const tileOffset = (vm.pSprite->picnum == BLIMP && debrisTile == SCRAP1) ? 0 : (krand() % 3);

                        int const spriteNum =
                        A_InsertSprite(vm.pSprite->sectnum, vm.pSprite->x + (krand() & 255) - 128,
                                       vm.pSprite->y + (krand() & 255) - 128, vm.pSprite->z - (8 << 8) - (krand() & 8191),
                                       debrisTile + tileOffset, vm.pSprite->shade, 32 + (krand() & 15), 32 + (krand() & 15),
                                       krand() & 2047, (krand() & 127) + 32, -(krand() & 2047), vm.spriteNum, 5);

                        sprite[spriteNum].yvel =
                        (vm.pSprite->picnum == BLIMP && debrisTile == SCRAP1) ? g_blimpSpawnItems[cnt % 14] : -1;
                        sprite[spriteNum].pal      = vm.pSprite->pal;
                    }
                insptr++;
            }
            continue;

        case CON_COUNT:
            insptr++;
            AC_COUNT(vm.pData) = (int16_t) *insptr++;
            continue;

        case CON_CSTATOR:
            insptr++;
            vm.pSprite->cstat |= (int16_t) *insptr++;
            continue;

        case CON_CLIPDIST:
            insptr++;
            vm.pSprite->clipdist = (int16_t) *insptr++;
            continue;

        case CON_CSTAT:
            insptr++;
            vm.pSprite->cstat = (int16_t) *insptr++;
            continue;

        case CON_SAVENN:
        case CON_SAVE:
            insptr++;
            {
                int32_t const requestedSlot = *insptr++;

                if ((unsigned)requestedSlot >= 10)
                    continue;

                // check if we need to make a new file
                if (strcmp(g_lastautosave.path, g_lastusersave.path) == 0 ||
                    requestedSlot != g_lastAutoSaveArbitraryID)
                {
                    g_lastautosave.reset();
                }

                g_lastAutoSaveArbitraryID = requestedSlot;

                if (tw == CON_SAVE || g_lastautosave.name[0] == 0)
                {
                    time_t     timeStruct = time(NULL);
                    struct tm *pTime      = localtime(&timeStruct);

                    strftime(g_lastautosave.name, sizeof(g_lastautosave.name),
                        "%d %b %Y %I:%M%p", pTime);
                }

                g_saveRequested = true;

                continue;
            }

        case CON_QUAKE:
            insptr++;
            g_earthquakeTime = Gv_GetVarX(*insptr++);
            A_PlaySound(EARTHQUAKE,g_player[screenpeek].ps->i);
            continue;

        case CON_IFMOVE:
            insptr++;
            VM_CONDITIONAL(AC_MOVE_ID(vm.pData) == *insptr);
            continue;

        case CON_RESETPLAYER:
            insptr++;
            vm.flags = VM_ResetPlayer(vm.playerNum, vm.flags, 0);
            continue;

        case CON_RESETPLAYERFLAGS:
            insptr++;
            vm.flags = VM_ResetPlayer(vm.playerNum, vm.flags, Gv_GetVarX(*insptr++));
            continue;

        case CON_IFONWATER:
            VM_CONDITIONAL(sector[vm.pSprite->sectnum].lotag == ST_1_ABOVE_WATER &&
                           klabs(vm.pSprite->z - sector[vm.pSprite->sectnum].floorz) < ZOFFSET5);
            continue;

        case CON_IFINWATER:
            VM_CONDITIONAL(sector[vm.pSprite->sectnum].lotag == ST_2_UNDERWATER);
            continue;

        case CON_IFCOUNT:
            insptr++;
            VM_CONDITIONAL(AC_COUNT(vm.pData) >= *insptr);
            continue;

        case CON_IFACTOR:
            insptr++;
            VM_CONDITIONAL(vm.pSprite->picnum == *insptr);
            continue;

        case CON_RESETCOUNT:
            insptr++;
            AC_COUNT(vm.pData) = 0;
            continue;

        case CON_ADDINVENTORY:
            insptr += 2;

            VM_AddInventory(pPlayer, *(insptr-1), *insptr);

            insptr++;
            continue;

        case CON_HITRADIUSVAR:
            insptr++;
            {
                int32_t params[5];
                Gv_FillWithVars(params);
                A_RadiusDamage(vm.spriteNum, params[0], params[1], params[2], params[3], params[4]);
            }
            continue;

        case CON_HITRADIUS:
            A_RadiusDamage(vm.spriteNum,*(insptr+1),*(insptr+2),*(insptr+3),*(insptr+4),*(insptr+5));
            insptr += 6;
            continue;

        case CON_IFP:
        {
            int const moveFlags  = *(++insptr);
            int       nResult    = 0;
            int const playerXVel = sprite[pPlayer->i].xvel;
            int const syncBits   = g_player[vm.playerNum].inputBits->bits;

            if (((moveFlags & pducking) && pPlayer->on_ground && TEST_SYNC_KEY(syncBits, SK_CROUCH)) ||
                ((moveFlags & pfalling) && pPlayer->jumping_counter == 0 && !pPlayer->on_ground && pPlayer->vel.z > 2048) ||
                ((moveFlags & pjumping) && pPlayer->jumping_counter > 348) ||
                ((moveFlags & pstanding) && playerXVel >= 0 && playerXVel < 8) ||
                ((moveFlags & pwalking) && playerXVel >= 8 && !TEST_SYNC_KEY(syncBits, SK_RUN)) ||
                ((moveFlags & prunning) && playerXVel >= 8 && TEST_SYNC_KEY(syncBits, SK_RUN)) ||
                ((moveFlags & phigher) && pPlayer->pos.z < (vm.pSprite->z - (48 << 8))) ||
                ((moveFlags & pwalkingback) && playerXVel <= -8 && !TEST_SYNC_KEY(syncBits, SK_RUN)) ||
                ((moveFlags & prunningback) && playerXVel <= -8 && TEST_SYNC_KEY(syncBits, SK_RUN)) ||
                ((moveFlags & pkicking) && (pPlayer->quick_kick > 0 || (PWEAPON(vm.playerNum, pPlayer->curr_weapon, WorksLike) == KNEE_WEAPON &&
                                                           pPlayer->kickback_pic > 0))) ||
                ((moveFlags & pshrunk) && sprite[pPlayer->i].xrepeat < 32) ||
                ((moveFlags & pjetpack) && pPlayer->jetpack_on) ||
                ((moveFlags & ponsteroids) && pPlayer->inv_amount[GET_STEROIDS] > 0 && pPlayer->inv_amount[GET_STEROIDS] < 400) ||
                ((moveFlags & ponground) && pPlayer->on_ground) ||
                ((moveFlags & palive) && sprite[pPlayer->i].xrepeat > 32 && sprite[pPlayer->i].extra > 0 && pPlayer->timebeforeexit == 0) ||
                ((moveFlags & pdead) && sprite[pPlayer->i].extra <= 0))
                nResult = 1;
            else if ((moveFlags & pfacing))
            {
                nResult =
                (vm.pSprite->picnum == APLAYER && (g_netServer || ud.multimode > 1))
                ? G_GetAngleDelta(g_player[otherp].ps->ang, getangle(pPlayer->pos.x - g_player[otherp].ps->pos.x,
                                                                     pPlayer->pos.y - g_player[otherp].ps->pos.y))
                : G_GetAngleDelta(pPlayer->ang, getangle(vm.pSprite->x - pPlayer->pos.x, vm.pSprite->y - pPlayer->pos.y));

                nResult = (nResult > -128 && nResult < 128);
            }
            VM_CONDITIONAL(nResult);
        }
        continue;

        case CON_IFSTRENGTH:
            insptr++;
            VM_CONDITIONAL(vm.pSprite->extra <= *insptr);
            continue;

        case CON_GUTS:
            A_DoGuts(vm.spriteNum,*(insptr+1),*(insptr+2));
            insptr += 3;
            continue;

        case CON_WACKPLAYER:
            insptr++;
            P_ForceAngle(pPlayer);
            continue;

        case CON_FLASH:
            insptr++;
            sprite[vm.spriteNum].shade = -127;
            pPlayer->visibility = -127;
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
                int const levelNum = Gv_GetVarX(*insptr++);
                if (EDUKE32_PREDICT_FALSE((unsigned)levelNum >= MAXVOLUMES*MAXLEVELS))
                {
                    CON_ERRPRINTF("invalid map number %d\n", levelNum);
                    continue;
                }

                G_FreeMapState(levelNum);
            }
            continue;

        case CON_STOPALLSOUNDS:
            insptr++;
            if (screenpeek == vm.playerNum)
                FX_StopAllSounds();
            continue;

        case CON_STOPALLMUSIC:
            insptr++;
            S_StopMusic();
            continue;

        case CON_IFGAPZL:
            insptr++;
            VM_CONDITIONAL(((vm.pActor->floorz - vm.pActor->ceilingz) >> 8) < *insptr);
            continue;

        case CON_IFHITSPACE:
            VM_CONDITIONAL(TEST_SYNC_KEY(g_player[vm.playerNum].inputBits->bits, SK_OPEN));
            continue;

        case CON_IFOUTSIDE:
            VM_CONDITIONAL(sector[vm.pSprite->sectnum].ceilingstat&1);
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

        case CON_IFPLAYBACKON:
            VM_CONDITIONAL(0);
            continue;

        case CON_OPERATE:
            insptr++;
            if (sector[vm.pSprite->sectnum].lotag == 0)
            {
                int16_t foundSect, foundWall, foundSprite;
                int32_t foundDist;

                neartag(vm.pSprite->x,vm.pSprite->y,vm.pSprite->z-ZOFFSET5,vm.pSprite->sectnum,vm.pSprite->ang,
                        &foundSect,&foundWall,&foundSprite,&foundDist, 768, 4+1, NULL);

                if (foundSect >= 0 && isanearoperator(sector[foundSect].lotag))
                    if ((sector[foundSect].lotag&0xff) == ST_23_SWINGING_DOOR || sector[foundSect].floorz == sector[foundSect].ceilingz)
                        if ((sector[foundSect].lotag&(16384|32768)) == 0)
                        {
                            int32_t j;

                            for (SPRITES_OF_SECT(foundSect, j))
                                if (sprite[j].picnum == ACTIVATOR)
                                    break;

                            if (j == -1)
                                G_OperateSectors(foundSect,vm.spriteNum);
                        }
            }
            continue;

        case CON_IFINSPACE:
            VM_CONDITIONAL(G_CheckForSpaceCeiling(vm.pSprite->sectnum));
            continue;

        case CON_SPRITEPAL:
            insptr++;
            if (vm.pSprite->picnum != APLAYER)
                vm.pActor->tempang = vm.pSprite->pal;
            vm.pSprite->pal = *insptr++;
            continue;

        case CON_CACTOR:
            insptr++;
            vm.pSprite->picnum = *insptr++;
            continue;

        case CON_IFBULLETNEAR:
            VM_CONDITIONAL(A_Dodge(vm.pSprite) == 1);
            continue;

        case CON_IFRESPAWN:
            if (A_CheckEnemySprite(vm.pSprite)) VM_CONDITIONAL(ud.respawn_monsters)
            else if (A_CheckInventorySprite(vm.pSprite)) VM_CONDITIONAL(ud.respawn_inventory)
            else VM_CONDITIONAL(ud.respawn_items)
            continue;

        case CON_IFFLOORDISTL:
            insptr++;
            VM_CONDITIONAL((vm.pActor->floorz - vm.pSprite->z) <= ((*insptr)<<8));
            continue;

        case CON_IFCEILINGDISTL:
            insptr++;
            VM_CONDITIONAL((vm.pSprite->z - vm.pActor->ceilingz) <= ((*insptr)<<8));
            continue;

        case CON_PALFROM:
            insptr++;
            if (EDUKE32_PREDICT_FALSE((unsigned)vm.playerNum >= (unsigned)g_mostConcurrentPlayers))
            {
                CON_ERRPRINTF("invalid player %d\n", vm.playerNum);
                insptr += 4;
            }
            else
            {
                palette_t const pal = { (uint8_t) * (insptr + 1), (uint8_t) * (insptr + 2), (uint8_t) * (insptr + 3),
                                        (uint8_t) * (insptr) };
                insptr += 4;
                P_PalFrom(pPlayer, pal.f, pal.r, pal.g, pal.b);
            }
            continue;

        case CON_SCREENPAL:
            insptr++;
            {
                int32_t params[4];
                Gv_FillWithVars(params);
                setpalettefade(params[0], params[1], params[2], params[3]);
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
                int const outputQuote = Gv_GetVarX(*insptr++);
                int const inputQuote  = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE(apStrings[inputQuote] == NULL || apStrings[outputQuote] == NULL))
                {
                    CON_ERRPRINTF("null quote %d\n", apStrings[inputQuote] ? outputQuote : inputQuote);

                    while ((*insptr & VM_INSTMASK) != CON_NULLOP)
                        Gv_GetVarX(*insptr++);

                    insptr++; // skip the NOP
                    continue;
                }

                int32_t   arg[32];
                int const quoteLen = Bstrlen(apStrings[inputQuote]);
                int       inputPos = 0;
                char      outBuf[MAXQUOTELEN];
                int       outBufPos = 0;
                int       argIdx    = 0;

                while ((*insptr & VM_INSTMASK) != CON_NULLOP && argIdx < 32)
                    arg[argIdx++] = Gv_GetVarX(*insptr++);

                int numArgs = argIdx;

                insptr++; // skip the NOP

                argIdx = 0;

                do
                {
                    while (inputPos < quoteLen && outBufPos < MAXQUOTELEN && apStrings[inputQuote][inputPos] != '%')
                        outBuf[outBufPos++] = apStrings[inputQuote][inputPos++];

                    if (apStrings[inputQuote][inputPos] == '%')
                    {
                        inputPos++;
                        switch (apStrings[inputQuote][inputPos])
                        {
                        case 'l':
                            if (apStrings[inputQuote][inputPos+1] != 'd')
                            {
                                // write the % and l
                                outBuf[outBufPos++] = apStrings[inputQuote][inputPos-1];
                                outBuf[outBufPos++] = apStrings[inputQuote][inputPos++];
                                break;
                            }
                            inputPos++;
                            fallthrough__;
                        case 'd':
                        {
                            if (argIdx >= numArgs)
                                goto finish_qsprintf;

                            char buf[16];
                            Bsprintf(buf, "%d", arg[argIdx++]);

                            int const bufLen = Bstrlen(buf);
                            Bmemcpy(&outBuf[outBufPos], buf, bufLen);
                            outBufPos += bufLen;
                            inputPos++;
                        }
                        break;

                        case 's':
                        {
                            if (argIdx >= numArgs)
                                goto finish_qsprintf;

                            int const argLen = Bstrlen(apStrings[arg[argIdx]]);

                            Bmemcpy(&outBuf[outBufPos], apStrings[arg[argIdx]], argLen);
                            outBufPos += argLen;
                            argIdx++;
                            inputPos++;
                        }
                        break;

                        default:
                            outBuf[outBufPos++] = apStrings[inputQuote][inputPos-1];
                            break;
                        }
                    }
                }
                while (inputPos < quoteLen && outBufPos < MAXQUOTELEN);
finish_qsprintf:
                outBuf[outBufPos] = '\0';
                Bstrncpyz(apStrings[outputQuote], outBuf, MAXQUOTELEN);
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
                            OSD_Printf(OSDTEXT_GREEN "CONLOGVAR: L=%d %s[%d] =%d\n", g_errorLineNum,
                                       aGameArrays[lVarID].szLabel, index,
                                       (int32_t)(m*Gv_GetArrayValue(lVarID, index)));
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
                        if ((lVarID & (MAXGAMEVARS-1)) == g_structVarIDs + STRUCT_ACTORVAR)
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
                            OSD_Printf(OSDTEXT_GREEN "CONLOGVAR: L=%d %d %d\n",g_errorLineNum,index,Gv_GetVar(*insptr++,index,vm.playerNum));
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
                Bsprintf(tempbuf,"CONLOGVAR: L=%d %s ",g_errorLineNum, aGameVars[lVarID].szLabel);

                if (aGameVars[lVarID].flags & GAMEVAR_READONLY)
                {
                    Bsprintf(szBuf," (read-only)");
                    Bstrcat(tempbuf,szBuf);
                }
                if (aGameVars[lVarID].flags & GAMEVAR_PERPLAYER)
                {
                    Bsprintf(szBuf," (Per Player. Player=%d)",vm.playerNum);
                }
                else if (aGameVars[lVarID].flags & GAMEVAR_PERACTOR)
                {
                    Bsprintf(szBuf," (Per Actor. Actor=%d)",vm.spriteNum);
                }
                else
                {
                    Bsprintf(szBuf," (Global)");
                }
                Bstrcat(tempbuf, szBuf);
                Bsprintf(szBuf, " =%d\n", Gv_GetVarX(lVarID) * m);
                Bstrcat(tempbuf, szBuf);
                OSD_Printf(OSDTEXT_GREEN "%s", tempbuf);
                insptr++;
                continue;
            }

        case CON_SETSECTOR:
            insptr++;
            {
                int const sectNum  = (*insptr++ != g_thisActorVarID) ? Gv_GetVarX(*(insptr - 1)) : sprite[vm.spriteNum].sectnum;
                int const labelNum = *insptr++;

                VM_SetSector(sectNum, labelNum, Gv_GetVarX(*insptr++));
                continue;
            }

        case CON_GETSECTOR:
            insptr++;
            {
                int const sectNum  = (*insptr++ != g_thisActorVarID) ? Gv_GetVarX(*(insptr - 1)) : sprite[vm.spriteNum].sectnum;
                int const labelNum = *insptr++;

                Gv_SetVarX(*insptr++, VM_GetSector(sectNum, labelNum));
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
                int const findPicnum  = *insptr++;
                int const maxDist     = Gv_GetVarX(*insptr++);
                int const returnVar   = *insptr++;
                int       foundSprite = -1;
                int       findStatnum = 1;
                int       spriteNum;

                if (tw == CON_FINDNEARSPRITEVAR || tw == CON_FINDNEARSPRITE3DVAR)
                    findStatnum = MAXSTATUS-1;

                if (tw==CON_FINDNEARACTOR3DVAR || tw==CON_FINDNEARSPRITE3DVAR)
                {
                    do
                    {
                        spriteNum=headspritestat[findStatnum];    // all sprites

                        while (spriteNum >= 0)
                        {
                            if (sprite[spriteNum].picnum == findPicnum && spriteNum != vm.spriteNum && dist(&sprite[vm.spriteNum], &sprite[spriteNum]) < maxDist)
                            {
                                foundSprite=spriteNum;
                                spriteNum = MAXSPRITES;
                                break;
                            }
                            spriteNum = nextspritestat[spriteNum];
                        }
                        if (spriteNum == MAXSPRITES || tw==CON_FINDNEARACTOR3DVAR)
                            break;
                    }
                    while (findStatnum--);
                    Gv_SetVarX(returnVar, foundSprite);
                    continue;
                }

                do
                {
                    spriteNum=headspritestat[findStatnum];    // all sprites

                    while (spriteNum >= 0)
                    {
                        if (sprite[spriteNum].picnum == findPicnum && spriteNum != vm.spriteNum && ldist(&sprite[vm.spriteNum], &sprite[spriteNum]) < maxDist)
                        {
                            foundSprite=spriteNum;
                            spriteNum = MAXSPRITES;
                            break;
                        }
                        spriteNum = nextspritestat[spriteNum];
                    }

                    if (spriteNum == MAXSPRITES || tw==CON_FINDNEARACTORVAR)
                        break;
                }
                while (findStatnum--);
                Gv_SetVarX(returnVar, foundSprite);
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
                int const findPicnum  = *insptr++;
                int const maxDist     = Gv_GetVarX(*insptr++);
                int const maxZDist    = Gv_GetVarX(*insptr++);
                int const returnVar   = *insptr++;
                int       foundSprite = -1;
                int       findStatnum = MAXSTATUS - 1;

                do
                {
                    int spriteNum = headspritestat[tw == CON_FINDNEARACTORZVAR ? 1 : findStatnum];  // all sprites

                    if (spriteNum == -1)
                        continue;
                    do
                    {
                        if (sprite[spriteNum].picnum == findPicnum && spriteNum != vm.spriteNum)
                        {
                            if (ldist(&sprite[vm.spriteNum], &sprite[spriteNum]) < maxDist)
                            {
                                if (klabs(sprite[vm.spriteNum].z-sprite[spriteNum].z) < maxZDist)
                                {
                                    foundSprite  = spriteNum;
                                    spriteNum = MAXSPRITES;
                                    break;
                                }
                            }
                        }
                        spriteNum = nextspritestat[spriteNum];
                    }
                    while (spriteNum>=0);
                    if (tw==CON_FINDNEARACTORZVAR || spriteNum == MAXSPRITES)
                        break;
                }
                while (findStatnum--);
                Gv_SetVarX(returnVar, foundSprite);

                continue;
            }

        case CON_FINDPLAYER:
            insptr++;
            aGameVars[g_returnVarID].global = A_FindPlayer(&sprite[vm.spriteNum], &tw);
            Gv_SetVarX(*insptr++, tw);
            continue;

        case CON_FINDOTHERPLAYER:
            insptr++;
            aGameVars[g_returnVarID].global = P_FindOtherPlayer(vm.playerNum,&tw);
            Gv_SetVarX(*insptr++, tw);
            continue;

        case CON_SETPLAYER:
            insptr++;
            {
                int const playerNum = (*insptr++ != g_thisActorVarID) ? Gv_GetVarX(*(insptr - 1)) : vm.playerNum;
                int const labelNum  = *insptr++;
                int const lParm2    = (PlayerLabels[labelNum].flags & LABEL_HASPARM2) ? Gv_GetVarX(*insptr++) : 0;

                VM_SetPlayer(playerNum, labelNum, lParm2, Gv_GetVarX(*insptr++));
                continue;
            }

        case CON_GETPLAYER:
            insptr++;
            {
                int const playerNum = (*insptr++ != g_thisActorVarID) ? Gv_GetVarX(*(insptr - 1)) : vm.playerNum;
                int const labelNum  = *insptr++;
                int const lParm2    = (PlayerLabels[labelNum].flags & LABEL_HASPARM2) ? Gv_GetVarX(*insptr++) : 0;

                Gv_SetVarX(*insptr++, VM_GetPlayer(playerNum, labelNum, lParm2));
                continue;
            }

        case CON_GETINPUT:
            insptr++;
            {
                int const playerNum = (*insptr++ != g_thisActorVarID) ? Gv_GetVarX(*(insptr - 1)) : vm.playerNum;
                int const labelNum  = *insptr++;

                Gv_SetVarX(*insptr++, VM_GetPlayerInput(playerNum, labelNum));
                continue;
            }

        case CON_SETINPUT:
            insptr++;
            {
                int const playerNum  = (*insptr++ != g_thisActorVarID) ? Gv_GetVarX(*(insptr - 1)) : vm.playerNum;
                int const labelNum = *insptr++;

                VM_SetPlayerInput(playerNum, labelNum, Gv_GetVarX(*insptr++));
                continue;
            }

        case CON_GETUSERDEF:
            insptr++;
            {
                tw = *insptr++;
                Gv_SetVarX(*insptr++, VM_GetUserdef(tw));
                continue;
            }

        case CON_SETUSERDEF:
            insptr++;
            {
                tw = *insptr++;
                VM_SetUserdef(tw, Gv_GetVarX(*insptr++));
                continue;
            }

        case CON_GETPROJECTILE:
            insptr++;
            {
                tw = Gv_GetVarX(*insptr++);
                int const labelNum = *insptr++;
                Gv_SetVarX(*insptr++, VM_GetProjectile(tw, labelNum));
                continue;
            }

        case CON_SETPROJECTILE:
            insptr++;
            {
                tw = Gv_GetVarX(*insptr++);
                int const labelNum = *insptr++;
                VM_SetProjectile(tw, labelNum, Gv_GetVarX(*insptr++));
                continue;
            }

        case CON_SETWALL:
            insptr++;
            {
                tw = *insptr++;

                int const wallNum  = Gv_GetVarX(tw);
                int const labelNum = *insptr++;

                VM_SetWall(wallNum, labelNum, Gv_GetVarX(*insptr++));
                continue;
            }

        case CON_GETWALL:
            insptr++;
            {
                tw = *insptr++;

                int const wallNum  = Gv_GetVarX(tw);
                int const labelNum = *insptr++;

                Gv_SetVarX(*insptr++, VM_GetWall(wallNum, labelNum));
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
                    CON_ERRPRINTF("invalid sprite %d\n", lSprite);

                    if (lVar1 == MAXGAMEVARS || lVar1 & ((MAXGAMEVARS << 2) | (MAXGAMEVARS << 3)))
                        insptr++;

                    if (lVar2 == MAXGAMEVARS || lVar2 & ((MAXGAMEVARS << 2) | (MAXGAMEVARS << 3)))
                        insptr++;

                    continue;
                }

                if (tw == CON_SETACTORVAR)
                    Gv_SetVar(lVar1, Gv_GetVarX(lVar2), lSprite, vm.playerNum);
                else
                    Gv_SetVarX(lVar2, Gv_GetVar(lVar1, lSprite, vm.playerNum));

                continue;
            }

        case CON_SETPLAYERVAR:
        case CON_GETPLAYERVAR:
            insptr++;
            {
                int const playerNum = (*insptr++ != g_thisActorVarID) ? Gv_GetVarX(*(insptr - 1)) : vm.playerNum;
                int const lVar1     = *insptr++;
                int const lVar2     = *insptr++;

                if (EDUKE32_PREDICT_FALSE((unsigned)playerNum >= (unsigned)g_mostConcurrentPlayers))
                {
                    CON_ERRPRINTF("invalid player %d\n", playerNum);

                    if (lVar1 == MAXGAMEVARS || lVar1 & ((MAXGAMEVARS << 2) | (MAXGAMEVARS << 3)))
                        insptr++;

                    if (lVar2 == MAXGAMEVARS || lVar2 & ((MAXGAMEVARS << 2) | (MAXGAMEVARS << 3)))
                        insptr++;

                    continue;
                }

                if (tw == CON_SETPLAYERVAR)
                    Gv_SetVar(lVar1, Gv_GetVarX(lVar2), vm.spriteNum, playerNum);
                else
                    Gv_SetVarX(lVar2, Gv_GetVar(lVar1, vm.spriteNum, playerNum));

                continue;
            }

        case CON_SETACTOR:
            insptr++;
            {
                int const spriteNum = (*insptr++ != g_thisActorVarID) ? Gv_GetVarX(*(insptr - 1)) : vm.spriteNum;
                int const labelNum  = *insptr++;
                int const lParm2    = (ActorLabels[labelNum].flags & LABEL_HASPARM2) ? Gv_GetVarX(*insptr++) : 0;

                VM_SetSprite(spriteNum, labelNum, lParm2, Gv_GetVarX(*insptr++));
                continue;
            }

        case CON_GETACTOR:
            insptr++;
            {
                int const spriteNum = (*insptr++ != g_thisActorVarID) ? Gv_GetVarX(*(insptr - 1)) : vm.spriteNum;
                int const labelNum  = *insptr++;
                int const lParm2    = (ActorLabels[labelNum].flags & LABEL_HASPARM2) ? Gv_GetVarX(*insptr++) : 0;

                Gv_SetVarX(*insptr++, VM_GetSprite(spriteNum, labelNum, lParm2));
                continue;
            }

        case CON_SETTSPR:
            insptr++;
            {
                int const spriteNum = (*insptr++ != g_thisActorVarID) ? Gv_GetVarX(*(insptr - 1)) : vm.spriteNum;
                int const labelNum  = *insptr++;

                VM_SetTsprite(spriteNum, labelNum, Gv_GetVarX(*insptr++));
                continue;
            }

        case CON_GETTSPR:
            insptr++;
            {
                int const spriteNum = (*insptr++ != g_thisActorVarID) ? Gv_GetVarX(*(insptr - 1)) : vm.spriteNum;
                int const labelNum  = *insptr++;

                Gv_SetVarX(*insptr++, VM_GetTsprite(spriteNum, labelNum));
                continue;
            }

        case CON_GETANGLETOTARGET:
            insptr++;
            // vm.pActor->lastvx and lastvy are last known location of target.
            Gv_SetVarX(*insptr++, getangle(vm.pActor->lastv.x-vm.pSprite->x,vm.pActor->lastv.y-vm.pSprite->y));
            continue;

        case CON_ANGOFFVAR:
            insptr++;
            spriteext[vm.spriteNum].angoff = Gv_GetVarX(*insptr++);
            continue;

        case CON_LOCKPLAYER:
            insptr++;
            pPlayer->transporter_hold = Gv_GetVarX(*insptr++);
            continue;

        case CON_CHECKAVAILWEAPON:
        case CON_CHECKAVAILINVEN:
        {
            insptr++;
            int const playerNum = (*insptr++ != g_thisActorVarID) ? Gv_GetVarX(*insptr) : vm.playerNum;

            if (EDUKE32_PREDICT_FALSE((unsigned)playerNum >= (unsigned)g_mostConcurrentPlayers))
            {
                CON_ERRPRINTF("invalid player %d\n", (int)playerNum);
                continue;
            }

            if (tw == CON_CHECKAVAILWEAPON)
                P_CheckWeapon(g_player[playerNum].ps);
            else
                P_SelectNextInvItem(g_player[playerNum].ps);

            continue;
        }

        case CON_GETPLAYERANGLE:
            insptr++;
            Gv_SetVarX(*insptr++, pPlayer->ang);
            continue;

        case CON_GETACTORANGLE:
            insptr++;
            Gv_SetVarX(*insptr++, vm.pSprite->ang);
            continue;

        case CON_SETPLAYERANGLE:
            insptr++;
            pPlayer->ang = Gv_GetVarX(*insptr++) & 2047;
            continue;

        case CON_SETACTORANGLE:
            insptr++;
            vm.pSprite->ang = Gv_GetVarX(*insptr++) & 2047;
            continue;

        case CON_SETVAR:
            insptr++;
            if ((aGameVars[*insptr].flags & (GAMEVAR_USER_MASK | GAMEVAR_PTR_MASK)) == 0)
                aGameVars[*insptr].global = *(insptr + 1);
            else
                Gv_SetVarX(*insptr, *(insptr + 1));
            insptr += 2;
            continue;

        case CON_KLABS:
            if ((aGameVars[*(insptr + 1)].flags & (GAMEVAR_USER_MASK | GAMEVAR_PTR_MASK)) == 0)
                aGameVars[*(insptr + 1)].global = klabs(aGameVars[*(insptr + 1)].global);
            else
                Gv_SetVarX(*(insptr + 1), klabs(Gv_GetVarX(*(insptr + 1))));
            insptr += 2;
            continue;

        case CON_SETARRAY:
            insptr++;
            {
                tw = *insptr++;

                int const arrayIndex = Gv_GetVarX(*insptr++);
                int const newValue   = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE((unsigned)tw >= (unsigned)g_gameArrayCount ||
                                          (unsigned)arrayIndex >= (unsigned)aGameArrays[tw].size))
                {
                    OSD_Printf(OSD_ERROR "Gv_SetVar(): tried to set invalid array %d or index out of bounds from "
                                         "sprite %d (%d), player %d\n",
                               tw, vm.spriteNum, TrackerCast(sprite[vm.spriteNum].picnum), vm.playerNum);
                    continue;
                }

                if (EDUKE32_PREDICT_FALSE(aGameArrays[tw].flags & GAMEARRAY_READONLY))
                {
                    OSD_Printf("Tried to set on read-only array `%s'", aGameArrays[tw].szLabel);
                    continue;
                }

                switch (aGameArrays[tw].flags & GAMEARRAY_TYPE_MASK)
                {
                    case 0:                              aGameArrays[tw].pValues[arrayIndex]  = newValue; break;
                    case GAMEARRAY_INT16:  ((int16_t *)  aGameArrays[tw].pValues)[arrayIndex] = newValue; break;
                    case GAMEARRAY_INT8:   ((int8_t *)   aGameArrays[tw].pValues)[arrayIndex] = newValue; break;
                    case GAMEARRAY_UINT16: ((uint16_t *) aGameArrays[tw].pValues)[arrayIndex] = newValue; break;
                    case GAMEARRAY_UINT8:  ((int8_t *)   aGameArrays[tw].pValues)[arrayIndex] = newValue; break;
                    case GAMEARRAY_BITMAP:
                        if (newValue)
                            ((uint8_t *)aGameArrays[tw].pValues)[arrayIndex >> 3] |= (1 << (arrayIndex & 7));
                        else
                            ((uint8_t *)aGameArrays[tw].pValues)[arrayIndex >> 3] &= ~(1 << (arrayIndex & 7));
                        break;
                }

                continue;
            }

        case CON_READARRAYFROMFILE:
            insptr++;
            {
                int const arrayNum      = *insptr++;
                int const quoteFilename = *insptr++;

                if (EDUKE32_PREDICT_FALSE(apStrings[quoteFilename] == NULL))
                {
                    CON_ERRPRINTF("null quote %d\n", quoteFilename);
                    continue;
                }

                int kFile = kopen4loadfrommod(apStrings[quoteFilename], 0);

                if (kFile < 0)
                    continue;

                size_t const filelength = kfilelength(kFile);
                size_t const numElements = Gv_GetArrayCountFromFile(arrayNum, filelength);

                if (numElements > 0)
                {
                    size_t const newBytes = Gv_GetArrayAllocSizeForCount(arrayNum, numElements);
                    size_t const readBytes = min(newBytes, filelength);
                    size_t const oldBytes = Gv_GetArrayAllocSize(arrayNum);

                    intptr_t *& pValues = aGameArrays[arrayNum].pValues;

                    if (newBytes != oldBytes)
                    {
                        Baligned_free(pValues);
                        pValues = (intptr_t *) Xaligned_alloc(ARRAY_ALIGNMENT, newBytes);
                    }

                    aGameArrays[arrayNum].size = numElements;

                    uintptr_t const flags = aGameArrays[arrayNum].flags;

                    switch (flags & GAMEARRAY_SIZE_MASK)
                    {
                    case 0:
#ifdef BITNESS64
                    {
                        void * const pArray = Xcalloc(1, newBytes);

                        kread(kFile, pArray, readBytes);

                        if (flags & GAMEARRAY_UNSIGNED)
                        {
                            for (unative_t i = 0; i < numElements; ++i)
                                pValues[i] = ((uint32_t *)pArray)[i];
                        }
                        else
                        {
                            for (unative_t i = 0; i < numElements; ++i)
                                pValues[i] = ((int32_t *)pArray)[i];
                        }

                        Bfree(pArray);
                        break;
                    }
#endif
                    default:
                        memset((char *)pValues + readBytes, 0, newBytes - readBytes);
                        kread(kFile, pValues, readBytes);
                        break;
                    }
                }

                kclose(kFile);
                continue;
            }

        case CON_WRITEARRAYTOFILE:
            insptr++;
            {
                int const arrayNum      = *insptr++;
                int const quoteFilename = *insptr++;

                if (EDUKE32_PREDICT_FALSE(apStrings[quoteFilename] == NULL))
                {
                    CON_ERRPRINTF("null quote %d\n", quoteFilename);
                    continue;
                }

                char temp[BMAX_PATH];

                if (EDUKE32_PREDICT_FALSE(G_ModDirSnprintf(temp, sizeof(temp), "%s", apStrings[quoteFilename])))
                {
                    CON_ERRPRINTF("file name too long\n");
                    continue;
                }

                FILE *const fil = Bfopen(temp, "wb");

                if (EDUKE32_PREDICT_FALSE(fil == NULL))
                {
                    CON_ERRPRINTF("couldn't open file \"%s\"\n", temp);
                    continue;
                }

                switch (aGameArrays[arrayNum].flags & GAMEARRAY_SIZE_MASK)
                {
                case 0:
#ifdef BITNESS64
                {
                    size_t const numElements = aGameArrays[arrayNum].size;
                    size_t const numDiskBytes = numElements * sizeof(int32_t);
                    int32_t *const pArray = (int32_t *)Xmalloc(numDiskBytes);

                    for (unative_t k = 0; k < numElements; ++k)
                        pArray[k] = Gv_GetArrayValue(arrayNum, k);

                    Bfwrite(pArray, 1, numDiskBytes, fil);
                    Bfree(pArray);
                    break;
                }
#endif
                default:
                    Bfwrite(aGameArrays[arrayNum].pValues, 1, Gv_GetArrayAllocSize(arrayNum), fil);
                    break;
                }

                Bfclose(fil);

                continue;
            }

        case CON_GETARRAYSIZE:
            insptr++;
            tw = *insptr++;
            Gv_SetVarX(*insptr++, (aGameArrays[tw].flags & GAMEARRAY_VARSIZE) ? Gv_GetVarX(aGameArrays[tw].size)
                                                                               : aGameArrays[tw].size);
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

                    int const eltSize = Gv_GetArrayElementSize(tw);
                    intptr_t * const pArray = oldSize != 0 ? (intptr_t *)Xmalloc(eltSize * oldSize) : NULL;

                    if (oldSize != 0)
                        Bmemcpy(pArray, aGameArrays[tw].pValues, eltSize * oldSize);

                    Baligned_free(aGameArrays[tw].pValues);

                    aGameArrays[tw].pValues = newSize != 0 ? (intptr_t *)Xaligned_alloc(ARRAY_ALIGNMENT, eltSize * newSize) : NULL;
                    aGameArrays[tw].size = newSize;

                    if (oldSize != 0)
                        Bmemcpy(aGameArrays[tw].pValues, pArray, eltSize * min(oldSize, newSize));

                    if (newSize > oldSize)
                        Bmemset(&aGameArrays[tw].pValues[oldSize], 0, eltSize * (newSize - oldSize));

                    Bfree(pArray);
                }
                continue;
            }

        case CON_COPY:
            insptr++;
            {
                int const srcArray       = *insptr++;
                int       srcArrayIndex  = Gv_GetVarX(*insptr++);  //, vm.spriteNum, vm.playerNum);
                int const destArray      = *insptr++;
                int       destArrayIndex = Gv_GetVarX(*insptr++);
                int       numElements    = Gv_GetVarX(*insptr++);

                int const srcArraySize = (aGameArrays[srcArray].flags & GAMEARRAY_VARSIZE)
                                         ? Gv_GetVarX(aGameArrays[srcArray].size)
                                         : aGameArrays[srcArray].size;

                int const destArraySize = (aGameArrays[destArray].flags & GAMEARRAY_VARSIZE)
                                          ? Gv_GetVarX(aGameArrays[srcArray].size)
                                          : aGameArrays[destArray].size;

                if (EDUKE32_PREDICT_FALSE(srcArrayIndex > srcArraySize || destArrayIndex > destArraySize))
                    continue;

                if ((srcArrayIndex + numElements) > srcArraySize)
                    numElements = srcArraySize - srcArrayIndex;

                if ((destArrayIndex + numElements) > destArraySize)
                    numElements = destArraySize - destArrayIndex;

                // Switch depending on the source array type.

                int const srcInc = 1 << (int)!!(EDUKE32_PREDICT_FALSE(aGameArrays[srcArray].flags & GAMEARRAY_STRIDE2));
                int const destInc = 1 << (int)!!(EDUKE32_PREDICT_FALSE(aGameArrays[destArray].flags & GAMEARRAY_STRIDE2));

                // matching array types and no STRIDE2 flag
                if ((aGameArrays[srcArray].flags & GAMEARRAY_SIZE_MASK) == (aGameArrays[destArray].flags & GAMEARRAY_SIZE_MASK) && (srcInc & destInc) == 1)
                {
                    Bmemcpy(aGameArrays[destArray].pValues + destArrayIndex, aGameArrays[srcArray].pValues + srcArrayIndex,
                        numElements * Gv_GetArrayElementSize(srcArray));
                }
                else switch (aGameArrays[destArray].flags & GAMEARRAY_TYPE_MASK)
                {
                    case 0:
                        for (; numElements > 0; --numElements)
                            aGameArrays[destArray].pValues[destArrayIndex += destInc] = Gv_GetArrayValue(srcArray, srcArrayIndex += srcInc);
                        break;
                    case GAMEARRAY_INT16:
                        for (; numElements > 0; --numElements)
                            ((int16_t *) aGameArrays[destArray].pValues)[destArrayIndex += destInc] = Gv_GetArrayValue(srcArray, srcArrayIndex += srcInc);
                        break;
                    case GAMEARRAY_INT8:
                        for (; numElements > 0; --numElements)
                            ((int8_t *) aGameArrays[destArray].pValues)[destArrayIndex += destInc] = Gv_GetArrayValue(srcArray, srcArrayIndex += srcInc);
                        break;
                    case GAMEARRAY_UINT16:
                        for (; numElements > 0; --numElements)
                            ((uint16_t *) aGameArrays[destArray].pValues)[destArrayIndex += destInc] = Gv_GetArrayValue(srcArray, srcArrayIndex += srcInc);
                        break;
                    case GAMEARRAY_UINT8:
                        for (; numElements > 0; --numElements)
                            ((uint8_t *) aGameArrays[destArray].pValues)[destArrayIndex += destInc] = Gv_GetArrayValue(srcArray, srcArrayIndex += srcInc);
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

        case CON_GETCLOSESTCOL:
            insptr++;
            {
                tw = *insptr++;
                int32_t const rgb = Gv_GetVarX(*insptr++);
                Gv_SetVarX(tw, getclosestcol_lim(rgb & 0xFF, (rgb >> 8) & 0xFF, (rgb >> 16) & 0xFF, Gv_GetVarX(*insptr++)));
            }
            continue;

        case CON_DRAWLINE256:
            insptr++;
            {
                struct {
                    vec2_t pos[2];
                    int32_t index;
                } v;

                Gv_FillWithVars(v);

                drawline256(v.pos[0].x, v.pos[0].y, v.pos[1].x, v.pos[1].y, v.index);
            }
            continue;

        case CON_DRAWLINERGB:
            insptr++;
            {
                struct {
                    vec2_t pos[2];
                    int32_t index, rgb;
                } v;

                Gv_FillWithVars(v);

                palette_t const p = { (uint8_t)(v.rgb & 0xFF), (uint8_t)((v.rgb >> 8) & 0xFF), (uint8_t)((v.rgb >> 16) & 0xFF), (uint8_t)v.index };

                drawlinergb(v.pos[0].x, v.pos[0].y, v.pos[1].x, v.pos[1].y, p);
            }
            continue;

        case CON_INV:
            if ((aGameVars[*(insptr + 1)].flags & (GAMEVAR_USER_MASK | GAMEVAR_PTR_MASK)) == 0)
                aGameVars[*(insptr + 1)].global = -aGameVars[*(insptr + 1)].global;
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
                CON_CRITICALERRPRINTF("divide by zero!\n");
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
                CON_CRITICALERRPRINTF("mod by zero!\n");
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
                int const nValue = Gv_GetVarX(*insptr++);

                if ((aGameVars[tw].flags & (GAMEVAR_USER_MASK | GAMEVAR_PTR_MASK)) == 0)
                    aGameVars[tw].global = nValue;
                else
                    Gv_SetVarX(tw, nValue);
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
                CON_ERRPRINTF("invalid weapon %d\n", tw);
                insptr++;
                continue;
            }
            Gv_SetVarX(*insptr++, pPlayer->max_ammo_amount[tw]);
            continue;

        case CON_SMAXAMMO:
            insptr++;
            tw = Gv_GetVarX(*insptr++);
            if (EDUKE32_PREDICT_FALSE((unsigned)tw >= MAX_WEAPONS))
            {
                CON_ERRPRINTF("invalid weapon %d\n", tw);
                insptr++;
                continue;
            }
            pPlayer->max_ammo_amount[tw] = Gv_GetVarX(*insptr++);
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

                int const nValue = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE(!nValue))
                {
                    CON_CRITICALERRPRINTF("divide by zero!\n");
                    continue;
                }

                Gv_DivVar(tw, nValue);
                continue;
            }

        case CON_MODVARVAR:
            insptr++;
            {
                tw = *insptr++;

                int const nValue = Gv_GetVarX(*insptr++);

                if (EDUKE32_PREDICT_FALSE(!nValue))
                {
                    CON_CRITICALERRPRINTF("mod by zero!\n");
                    continue;
                }

                Gv_ModVar(tw, nValue);
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
            Gv_ShiftVarL(*insptr, *(insptr+1));
            insptr += 2;
            continue;

        case CON_SHIFTVARR:
            insptr++;
            Gv_ShiftVarR(*insptr, *(insptr+1));
            insptr += 2;
            continue;

        case CON_SHIFTVARVARL:
            insptr++;
            tw = *insptr++;
            Gv_ShiftVarL(tw, Gv_GetVarX(*insptr++));
            continue;

        case CON_SHIFTVARVARR:
            insptr++;
            tw = *insptr++;
            Gv_ShiftVarR(tw, Gv_GetVarX(*insptr++));
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
            aGameVars[g_lotagVarID].global = vm.pSprite->lotag;
            continue;

        case CON_SPGETHITAG:
            insptr++;
            aGameVars[g_hitagVarID].global = vm.pSprite->hitag;
            continue;

        case CON_SECTGETLOTAG:
            insptr++;
            aGameVars[g_lotagVarID].global = sector[vm.pSprite->sectnum].lotag;
            continue;

        case CON_SECTGETHITAG:
            insptr++;
            aGameVars[g_hitagVarID].global = sector[vm.pSprite->sectnum].hitag;
            continue;

        case CON_GETTEXTUREFLOOR:
            insptr++;
            aGameVars[g_textureVarID].global = sector[vm.pSprite->sectnum].floorpicnum;
            continue;

        case CON_STARTTRACK:
            insptr++;
            G_StartTrackSlotWrap(ud.volume_number, *(insptr++));
            continue;

        case CON_STARTTRACKVAR:
            insptr++;
            G_StartTrackSlotWrap(ud.volume_number, Gv_GetVarX(*(insptr++)));
            continue;

        case CON_STARTTRACKSLOT:
            insptr++;
            {
                int const volumeNum = Gv_GetVarX(*(insptr++));
                int const levelNum = Gv_GetVarX(*(insptr++));
                G_StartTrackSlotWrap(volumeNum == -1 ? MAXVOLUMES : volumeNum, levelNum);
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
            P_SetGamePalette(pPlayer, Gv_GetVarX(*(insptr++)), 2+16);
            continue;

        case CON_GETTEXTURECEILING:
            insptr++;
            aGameVars[g_textureVarID].global = sector[vm.pSprite->sectnum].ceilingpicnum;
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
                tw     = (Gv_GetVarX(*(insptr - 1)) != *insptr);
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
                tw     = (Gv_GetVarX(*(insptr - 1)) < *insptr);
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
                tw     = Gv_GetVarX(*(insptr - 1));
                tw     = (tw != Gv_GetVarX(*insptr++));
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
                tw     = Gv_GetVarX(*(insptr - 1));
                tw     = (tw < Gv_GetVarX(*insptr++));
                insptr--;
                VM_CONDITIONAL(tw);
            } while (tw);
            continue;
        }

        case CON_FOR:  // special-purpose iteration
            insptr++;
            {
                int const returnVar = *insptr++;
                int const iterType  = *insptr++;
                int const nIndex    = iterType <= ITER_DRAWNSPRITES ? 0 : Gv_GetVarX(*insptr++);

                intptr_t const *const pEnd  = insptr + *insptr;
                intptr_t const *const pNext = ++insptr;

                switch (iterType)
                {
                case ITER_ALLSPRITES:
                    for (native_t statNum=0; statNum<MAXSTATUS; ++statNum)
                    {
                        for (native_t jj=headspritestat[statNum]; jj>=0;)
                        {
                            int const kk=nextspritestat[jj];
                            Gv_SetVarX(returnVar, jj);
                            insptr = pNext;
                            VM_Execute(0);
                            jj=kk;
                        }
                    }
                    break;
                case ITER_ALLSECTORS:
                    for (native_t jj=0; jj<numsectors; ++jj)
                    {
                        Gv_SetVarX(returnVar, jj);
                        insptr = pNext;
                        VM_Execute(0);
                    }
                    break;
                case ITER_ALLWALLS:
                    for (native_t jj=0; jj<numwalls; ++jj)
                    {
                        Gv_SetVarX(returnVar, jj);
                        insptr = pNext;
                        VM_Execute(0);
                    }
                    break;
                case ITER_ACTIVELIGHTS:
#ifdef POLYMER
                    for (native_t jj=0; jj<PR_MAXLIGHTS; ++jj)
                    {
                        if (!prlights[jj].flags.active)
                            continue;

                        Gv_SetVarX(returnVar, jj);
                        insptr = pNext;
                        VM_Execute(0);
                    }
#endif
                    break;

                case ITER_DRAWNSPRITES:
                {
                    for (native_t ii=0; ii<spritesortcnt; ii++)
                    {
                        Gv_SetVarX(returnVar, ii);
                        insptr = pNext;
                        VM_Execute(0);
                    }
                    break;
                }

                case ITER_SPRITESOFSECTOR:
                    if ((unsigned)nIndex >= MAXSECTORS) goto badindex;
                    for (native_t jj=headspritesect[nIndex]; jj>=0;)
                    {
                        int const kk=nextspritesect[jj];
                        Gv_SetVarX(returnVar, jj);
                        insptr = pNext;
                        VM_Execute(0);
                        jj=kk;
                    }
                    break;
                case ITER_SPRITESOFSTATUS:
                    if ((unsigned) nIndex >= MAXSTATUS) goto badindex;
                    for (native_t jj=headspritestat[nIndex]; jj>=0;)
                    {
                        int const kk=nextspritestat[jj];
                        Gv_SetVarX(returnVar, jj);
                        insptr = pNext;
                        VM_Execute(0);
                        jj=kk;
                    }
                    break;
                case ITER_WALLSOFSECTOR:
                    if ((unsigned) nIndex >= MAXSECTORS) goto badindex;
                    for (native_t jj=sector[nIndex].wallptr, endwall=jj+sector[nIndex].wallnum-1;
                    jj<=endwall; jj++)
                    {
                        Gv_SetVarX(returnVar, jj);
                        insptr = pNext;
                        VM_Execute(0);
                    }
                    break;
                case ITER_LOOPOFWALL:
                    if ((unsigned) nIndex >= (unsigned)numwalls) goto badindex;
                    {
                        int jj = nIndex;
                        do
                        {
                            Gv_SetVarX(returnVar, jj);
                            insptr = pNext;
                            VM_Execute(0);
                            jj = wall[jj].point2;
                        } while (jj != nIndex);
                    }
                    break;
                case ITER_RANGE:
                    for (native_t jj=0; jj<nIndex; jj++)
                    {
                        Gv_SetVarX(returnVar, jj);
                        insptr = pNext;
                        VM_Execute(0);
                    }
                    break;
                default:
                    CON_ERRPRINTF("invalid iterator type %d", iterType);
                    continue;
                badindex:
                    OSD_Printf(OSD_ERROR "Line %d, for %s: index %d out of range!\n", g_errorLineNum,
                        iter_tokens[iterType].token, nIndex);
                    continue;
                }
                insptr = pEnd;
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
            VM_CONDITIONAL(sprite[pPlayer->i].extra < *insptr);
            continue;

        case CON_IFPINVENTORY:
            insptr++;

            switch (*insptr++)
            {
                case GET_STEROIDS:
                case GET_SHIELD:
                case GET_SCUBA:
                case GET_HOLODUKE:
                case GET_HEATS:
                case GET_FIRSTAID:
                case GET_BOOTS:
                case GET_JETPACK:
                    tw = (pPlayer->inv_amount[*(insptr - 1)] != *insptr);
                    break;

                case GET_ACCESS:
                    switch (vm.pSprite->pal)
                    {
                        case 0:  tw = (pPlayer->got_access & 1); break;
                        case 21: tw = (pPlayer->got_access & 2); break;
                        case 23: tw = (pPlayer->got_access & 4); break;
                    }
                    break;
                default: tw = 0; CON_ERRPRINTF("invalid inventory item %d\n", (int32_t) * (insptr - 1));
            }

            VM_CONDITIONAL(tw);
            continue;

        case CON_PSTOMP:
            insptr++;
            if (pPlayer->knee_incs == 0 && sprite[pPlayer->i].xrepeat >= 40)
                if (cansee(vm.pSprite->x, vm.pSprite->y, vm.pSprite->z - ZOFFSET6, vm.pSprite->sectnum, pPlayer->pos.x,
                           pPlayer->pos.y, pPlayer->pos.z + ZOFFSET2, sprite[pPlayer->i].sectnum))
                {
                    int numPlayers = g_mostConcurrentPlayers - 1;

                    for (; numPlayers >= 0; --numPlayers)
                    {
                        if (g_player[numPlayers].ps->actorsqu == vm.spriteNum)
                            break;
                    }

                    if (numPlayers == -1)
                    {
                        if (pPlayer->weapon_pos == 0)
                            pPlayer->weapon_pos = -1;

                        pPlayer->actorsqu  = vm.spriteNum;
                        pPlayer->knee_incs = 1;
                    }
                }
            continue;

        case CON_IFAWAYFROMWALL:
        {
            int16_t otherSectnum = vm.pSprite->sectnum;
            tw = 0;

#define IFAWAYDIST 108

            updatesector(vm.pSprite->x + IFAWAYDIST, vm.pSprite->y + IFAWAYDIST, &otherSectnum);
            if (otherSectnum == vm.pSprite->sectnum)
            {
                updatesector(vm.pSprite->x - IFAWAYDIST, vm.pSprite->y - IFAWAYDIST, &otherSectnum);
                if (otherSectnum == vm.pSprite->sectnum)
                {
                    updatesector(vm.pSprite->x + IFAWAYDIST, vm.pSprite->y - IFAWAYDIST, &otherSectnum);
                    if (otherSectnum == vm.pSprite->sectnum)
                    {
                        updatesector(vm.pSprite->x - IFAWAYDIST, vm.pSprite->y + IFAWAYDIST, &otherSectnum);
                        if (otherSectnum == vm.pSprite->sectnum)
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

            if (EDUKE32_PREDICT_FALSE((unsigned)(*insptr) >= MAXQUOTES) || apStrings[*insptr] == NULL)
            {
                CON_ERRPRINTF("invalid quote %d\n", (int32_t)(*insptr));
                insptr++;
                continue;
            }

            if (EDUKE32_PREDICT_FALSE((unsigned)vm.playerNum >= MAXPLAYERS))
            {
                CON_ERRPRINTF("invalid player %d\n", vm.playerNum);
                insptr++;
                continue;
            }

            P_DoQuote(*(insptr++) | MAXQUOTES, pPlayer);
            continue;

        case CON_USERQUOTE:
            insptr++;
            tw = Gv_GetVarX(*insptr++);

            if (EDUKE32_PREDICT_FALSE((unsigned)tw >= MAXQUOTES || apStrings[tw] == NULL))
            {
                CON_ERRPRINTF("invalid quote %d\n", tw);
                continue;
            }

            G_AddUserQuote(apStrings[tw]);
            continue;

        case CON_ECHO:
            insptr++;
            tw = Gv_GetVarX(*insptr++);

            if (EDUKE32_PREDICT_FALSE((unsigned)tw >= MAXQUOTES || apStrings[tw] == NULL))
            {
                CON_ERRPRINTF("invalid quote %d\n", tw);
                continue;
            }

            OSD_Printf("%s\n", apStrings[tw]);
            continue;

        case CON_IFINOUTERSPACE:
            VM_CONDITIONAL(G_CheckForSpaceFloor(vm.pSprite->sectnum));
            continue;

        case CON_IFNOTMOVING:
            VM_CONDITIONAL((vm.pActor->movflag&49152) > 16384);
            continue;

        case CON_RESPAWNHITAG:
            insptr++;
            switch (DYNAMICTILEMAP(vm.pSprite->picnum))
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
                    if (vm.pSprite->yvel)
                        G_OperateRespawns(vm.pSprite->yvel);
                    break;
                default:
                    //                if (vm.pSprite->hitag >= 0)
                    G_OperateRespawns(vm.pSprite->hitag);
                    break;
            }
            continue;

        case CON_IFSPRITEPAL:
            insptr++;
            VM_CONDITIONAL(vm.pSprite->pal == *insptr);
            continue;

        case CON_IFANGDIFFL:
            insptr++;
            tw = klabs(G_GetAngleDelta(pPlayer->ang, vm.pSprite->ang));
            VM_CONDITIONAL(tw <= *insptr);
            continue;

        case CON_IFNOSOUNDS:
            VM_CONDITIONAL(!A_CheckAnySoundPlaying(vm.spriteNum));
            continue;

        case CON_SPRITEFLAGS:
            insptr++;
            vm.pActor->flags = Gv_GetVarX(*insptr++);
            continue;

        case CON_GETTICKS:
            insptr++;
            Gv_SetVarX(*insptr++, getticks());
            continue;

        case CON_GETCURRADDRESS:
            insptr++;
            tw = *insptr++;
            Gv_SetVarX(tw, (intptr_t)(insptr - apScript));
            continue;

        case CON_JUMP:  // XXX XXX XXX
            insptr++;
            tw     = Gv_GetVarX(*insptr++);
            insptr = (intptr_t *)(tw + apScript);
            continue;

        default:  // you aren't supposed to be here!
            VM_ScriptInfo(insptr, 64);
            G_GameExit("An error has occurred in the " APPNAME " virtual machine.\n\n"
                       "If you are an end user, please e-mail the file " APPBASENAME ".log\n"
                       "along with links to any mods you're using to richard@voidpoint.com.\n\n"
                       "If you are a developer, please attach all of your script files\n"
                       "along with instructions on how to reproduce this error.\n\n"
                       "Thank you!");
            break;
        }
    }
}

// NORECURSE
void A_LoadActor(int32_t spriteNum)
{
    vm.spriteNum = spriteNum;           // Sprite ID
    vm.pSprite   = &sprite[spriteNum];  // Pointer to sprite structure
    vm.pActor    = &actor[spriteNum];

    if (g_tile[vm.pSprite->picnum].loadPtr == NULL)
        return;

    vm.pData      = &actor[spriteNum].t_data[0];  // Sprite's 'extra' data
    vm.playerNum  = -1;                           // Player ID
    vm.playerDist = -1;                           // Distance
    vm.pPlayer    = g_player[0].ps;

    vm.flags &= ~(VM_RETURN | VM_KILL | VM_NOEXECUTE);

    if ((unsigned)vm.pSprite->sectnum >= MAXSECTORS)
    {
        A_DeleteSprite(vm.spriteNum);
        return;
    }

    insptr = g_tile[vm.pSprite->picnum].loadPtr;
    VM_Execute(1);
    insptr = NULL;

    if (vm.flags & VM_KILL)
        A_DeleteSprite(vm.spriteNum);
}
#endif

void VM_UpdateAnim(int spriteNum, int32_t *pData)
{
#if !defined LUNATIC
    size_t const actionofs = AC_ACTION_ID(pData);
    intptr_t const *actionptr = (actionofs != 0 && actionofs + (ACTION_PARAM_COUNT-1) < (unsigned) g_scriptSize) ? &apScript[actionofs] : NULL;

    if (actionptr != NULL)
#endif
    {
#if !defined LUNATIC
        int const action_frames = actionptr[ACTION_NUMFRAMES];
        int const action_incval = actionptr[ACTION_INCVAL];
        int const action_delay  = actionptr[ACTION_DELAY];
#else
        int const action_frames = actor[spriteNum].ac.numframes;
        int const action_incval = actor[spriteNum].ac.incval;
        int const action_delay  = actor[spriteNum].ac.delay;
#endif
        uint16_t *actionticsptr = &AC_ACTIONTICS(&sprite[spriteNum], &actor[spriteNum]);
        *actionticsptr += TICSPERFRAME;

        if (*actionticsptr > action_delay)
        {
            *actionticsptr = 0;
            AC_ACTION_COUNT(pData)++;
            AC_CURFRAME(pData) += action_incval;
        }

        if (klabs(AC_CURFRAME(pData)) >= klabs(action_frames * action_incval))
            AC_CURFRAME(pData) = 0;
    }
}

// NORECURSE
void A_Execute(int spriteNum, int playerNum, int playerDist)
{
    vmstate_t tempvm = {
        spriteNum, playerNum, playerDist, 0, &sprite[spriteNum], &actor[spriteNum].t_data[0], g_player[playerNum].ps, &actor[spriteNum]
    };
    vm = tempvm;

#ifdef LUNATIC
    int32_t killit=0;
#endif

/*
    if (g_netClient && A_CheckSpriteFlags(spriteNum, SFLAG_NULL))
    {
        A_DeleteSprite(spriteNum);
        return;
    }
*/

    if (g_netClient) // [75] The server should not overwrite its own randomseed
        randomseed = ticrandomseed;

    if (EDUKE32_PREDICT_FALSE((unsigned)vm.pSprite->sectnum >= MAXSECTORS))
    {
        if (A_CheckEnemySprite(vm.pSprite))
            P_AddKills(vm.pPlayer, 1);

        A_DeleteSprite(vm.spriteNum);
        return;
    }

    VM_UpdateAnim(vm.spriteNum, vm.pData);

#ifdef LUNATIC
    int const picnum = vm.pSprite->picnum;

    if (L_IsInitialized(&g_ElState) && El_HaveActor(picnum))
    {
        double t = gethiticks();

        killit = (El_CallActor(&g_ElState, picnum, spriteNum, playerNum, playerDist)==1);

        t = gethiticks()-t;
        g_actorTotalMs[picnum] += t;
        g_actorMinMs[picnum] = min(g_actorMinMs[picnum], t);
        g_actorMaxMs[picnum] = max(g_actorMaxMs[picnum], t);
        g_actorCalls[picnum]++;
    }
#else
    insptr = 4 + (g_tile[vm.pSprite->picnum].execPtr);
    VM_Execute(1);
    insptr = NULL;
#endif

#ifdef LUNATIC
    if (killit)
#else
    if (vm.flags & VM_KILL)
#endif
    {
        VM_DeleteSprite(spriteNum, playerNum);
        return;
    }

    VM_Move();

    if (vm.pSprite->statnum != STAT_ACTOR)
    {
        if (vm.pSprite->statnum == STAT_STANDABLE)
        {
            switch (DYNAMICTILEMAP(vm.pSprite->picnum))
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
                    if (vm.pActor->timetosleep > 1)
                        vm.pActor->timetosleep--;
                    else if (vm.pActor->timetosleep == 1)
                        changespritestat(vm.spriteNum, STAT_ZOMBIEACTOR);
                default: break;
            }
        }
        return;
    }

    if (A_CheckEnemySprite(vm.pSprite))
    {
        if (vm.pSprite->xrepeat > 60 || (ud.respawn_monsters == 1 && vm.pSprite->extra <= 0))
            return;
    }
    else if (EDUKE32_PREDICT_FALSE(ud.respawn_items == 1 && (vm.pSprite->cstat & 32768)))
        return;

    if (A_CheckSpriteFlags(vm.spriteNum, SFLAG_USEACTIVATOR) && sector[vm.pSprite->sectnum].lotag & 16384)
        changespritestat(vm.spriteNum, STAT_ZOMBIEACTOR);
    else if (vm.pActor->timetosleep > 1)
        vm.pActor->timetosleep--;
    else if (vm.pActor->timetosleep == 1)
    {
        // hack for 1.3D fire sprites
        if (EDUKE32_PREDICT_FALSE(g_scriptVersion == 13 && (vm.pSprite->picnum == FIRE || vm.pSprite->picnum == FIRE2)))
            return;
        changespritestat(vm.spriteNum, STAT_ZOMBIEACTOR);
    }
}

void G_SaveMapState(void)
{
    int const    levelNum = ud.volume_number * MAXLEVELS + ud.level_number;
    map_t *const pMapInfo = &g_mapInfo[levelNum];

    if (pMapInfo->savedstate == NULL)
    {
        pMapInfo->savedstate = (mapstate_t *) Xaligned_alloc(ACTOR_VAR_ALIGNMENT, sizeof(mapstate_t));
        Bmemset(pMapInfo->savedstate, 0, sizeof(mapstate_t));
    }

    mapstate_t *save = pMapInfo->savedstate;

    if (save == NULL)
        return;

    save->numwalls = numwalls;
    Bmemcpy(&save->wall[0],&wall[0],sizeof(walltype)*MAXWALLS);
    save->numsectors = numsectors;
    Bmemcpy(&save->sector[0],&sector[0],sizeof(sectortype)*MAXSECTORS);
    Bmemcpy(&save->sprite[0],&sprite[0],sizeof(spritetype)*MAXSPRITES);

    // If we're in EVENT_ANIMATESPRITES, we'll be saving pointer values to disk :-/
#if !defined LUNATIC
    if (g_currentEventExec == EVENT_ANIMATESPRITES)
        initprintf("Line %d: savemapstate called from EVENT_ANIMATESPRITES. WHY?\n", g_errorLineNum);
#endif
    Bmemcpy(&save->spriteext[0],&spriteext[0],sizeof(spriteext_t)*MAXSPRITES);
#ifndef NEW_MAP_FORMAT
    Bmemcpy(&save->wallext[0],&wallext[0],sizeof(wallext_t)*MAXWALLS);
#endif

    save->numsprites = Numsprites;
    save->tailspritefree = tailspritefree;
    Bmemcpy(&save->headspritesect[0],&headspritesect[0],sizeof(headspritesect));
    Bmemcpy(&save->prevspritesect[0],&prevspritesect[0],sizeof(prevspritesect));
    Bmemcpy(&save->nextspritesect[0],&nextspritesect[0],sizeof(nextspritesect));
    Bmemcpy(&save->headspritestat[0],&headspritestat[0],sizeof(headspritestat));
    Bmemcpy(&save->prevspritestat[0],&prevspritestat[0],sizeof(prevspritestat));
    Bmemcpy(&save->nextspritestat[0],&nextspritestat[0],sizeof(nextspritestat));
#ifdef YAX_ENABLE
    save->numyaxbunches = numyaxbunches;
# if !defined NEW_MAP_FORMAT
    Bmemcpy(save->yax_bunchnum, yax_bunchnum, sizeof(yax_bunchnum));
    Bmemcpy(save->yax_nextwall, yax_nextwall, sizeof(yax_nextwall));
# endif
#endif
    Bmemcpy(&save->actor[0],&actor[0],sizeof(actor_t)*MAXSPRITES);

    save->g_cyclerCnt = g_cyclerCnt;
    Bmemcpy(&save->g_cyclers[0],&g_cyclers[0],sizeof(g_cyclers));
    Bmemcpy(&save->g_playerSpawnPoints[0],&g_playerSpawnPoints[0],sizeof(g_playerSpawnPoints));
    save->g_animWallCnt = g_animWallCnt;
    Bmemcpy(&save->SpriteDeletionQueue[0],&SpriteDeletionQueue[0],sizeof(SpriteDeletionQueue));
    save->g_spriteDeleteQueuePos = g_spriteDeleteQueuePos;
    Bmemcpy(&save->animwall[0],&animwall[0],sizeof(animwall));
    Bmemcpy(&save->origins[0],&g_origins[0],sizeof(g_origins));
    Bmemcpy(&save->g_mirrorWall[0],&g_mirrorWall[0],sizeof(g_mirrorWall));
    Bmemcpy(&save->g_mirrorSector[0],&g_mirrorSector[0],sizeof(g_mirrorSector));
    save->g_mirrorCount = g_mirrorCount;
    Bmemcpy(&save->show2dsector[0],&show2dsector[0],sizeof(show2dsector));
    save->g_cloudCnt = g_cloudCnt;
    Bmemcpy(&save->g_cloudSect[0],&g_cloudSect[0],sizeof(g_cloudSect));
    save->g_cloudX = g_cloudX;
    save->g_cloudY = g_cloudY;
    save->pskyidx = g_pskyidx;
    Bmemcpy(&save->g_animateGoal[0],&g_animateGoal[0],sizeof(g_animateGoal));
    Bmemcpy(&save->g_animateVel[0],&g_animateVel[0],sizeof(g_animateVel));
    save->g_animateCnt = g_animateCnt;
    Bmemcpy(&save->g_animateSect[0],&g_animateSect[0],sizeof(g_animateSect));

    G_Util_PtrToIdx(g_animatePtr, g_animateCnt, sector, P2I_FWD);
    Bmemcpy(&save->g_animatePtr[0],&g_animatePtr[0],sizeof(g_animatePtr));
    G_Util_PtrToIdx(g_animatePtr, g_animateCnt, sector, P2I_BACK);

    {
        EDUKE32_STATIC_ASSERT(sizeof(save->g_animatePtr) == sizeof(g_animatePtr));
    }

    save->g_playerSpawnCnt = g_playerSpawnCnt;
    save->g_earthquakeTime = g_earthquakeTime;
    save->lockclock        = lockclock;
    save->randomseed       = randomseed;
    save->g_globalRandom   = g_globalRandom;

#if !defined LUNATIC
    for (native_t i=g_gameVarCount-1; i>=0; i--)
    {
        if (aGameVars[i].flags & GAMEVAR_NORESET) continue;
        if (aGameVars[i].flags & GAMEVAR_PERPLAYER)
        {
            if (!save->vars[i])
                save->vars[i] = (intptr_t *)Xaligned_alloc(PLAYER_VAR_ALIGNMENT, MAXPLAYERS * sizeof(intptr_t));
            Bmemcpy(&save->vars[i][0], aGameVars[i].pValues, sizeof(intptr_t) * MAXPLAYERS);
        }
        else if (aGameVars[i].flags & GAMEVAR_PERACTOR)
        {
            if (!save->vars[i])
                save->vars[i] = (intptr_t *)Xaligned_alloc(ACTOR_VAR_ALIGNMENT, MAXSPRITES * sizeof(intptr_t));
            Bmemcpy(&save->vars[i][0], aGameVars[i].pValues, sizeof(intptr_t) * MAXSPRITES);
        }
        else save->vars[i] = (intptr_t *)aGameVars[i].global;
    }

    for (native_t i=g_gameArrayCount-1; i>=0; i--)
    {
        if ((aGameArrays[i].flags & GAMEARRAY_RESTORE) == 0)
            continue;

        save->arraysiz[i] = aGameArrays[i].size;
        Baligned_free(save->arrays[i]);
        save->arrays[i] = (intptr_t *)Xaligned_alloc(ARRAY_ALIGNMENT, Gv_GetArrayAllocSize(i));
        Bmemcpy(&save->arrays[i][0], aGameArrays[i].pValues, Gv_GetArrayAllocSize(i));
    }
#else
    int32_t slen;
    const char *svcode = El_SerializeGamevars(&slen, levelNum);

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
    int const   levelNum    = ud.volume_number * MAXLEVELS + ud.level_number;
    mapstate_t *pSavedState = g_mapInfo[levelNum].savedstate;

    if (pSavedState != NULL)
    {
        int playerHealth[MAXPLAYERS];

        for (native_t i=0; i<g_mostConcurrentPlayers; i++)
            playerHealth[i] = sprite[g_player[i].ps->i].extra;

        pub = NUMPAGES;
        pus = NUMPAGES;
        G_UpdateScreenArea();

        numwalls = pSavedState->numwalls;
        Bmemcpy(&wall[0],&pSavedState->wall[0],sizeof(walltype)*MAXWALLS);
#ifndef NEW_MAP_FORMAT
        Bmemcpy(&wallext[0],&pSavedState->wallext[0],sizeof(wallext_t)*MAXWALLS);
#endif
        numsectors = pSavedState->numsectors;
        Bmemcpy(&sector[0],&pSavedState->sector[0],sizeof(sectortype)*MAXSECTORS);
        Bmemcpy(&sprite[0],&pSavedState->sprite[0],sizeof(spritetype)*MAXSPRITES);
        Bmemcpy(&spriteext[0],&pSavedState->spriteext[0],sizeof(spriteext_t)*MAXSPRITES);

        // If we're restoring from EVENT_ANIMATESPRITES, all spriteext[].tspr
        // will be overwritten, so NULL them.
#if !defined LUNATIC
        if (g_currentEventExec == EVENT_ANIMATESPRITES)
        {
            initprintf("Line %d: loadmapstate called from EVENT_ANIMATESPRITES. WHY?\n",g_errorLineNum);
            for (native_t i=0; i<MAXSPRITES; i++)
                spriteext[i].tspr = NULL;
        }
#endif
        Numsprites = pSavedState->numsprites;
        tailspritefree = pSavedState->tailspritefree;
        Bmemcpy(&headspritesect[0],&pSavedState->headspritesect[0],sizeof(headspritesect));
        Bmemcpy(&prevspritesect[0],&pSavedState->prevspritesect[0],sizeof(prevspritesect));
        Bmemcpy(&nextspritesect[0],&pSavedState->nextspritesect[0],sizeof(nextspritesect));
        Bmemcpy(&headspritestat[0],&pSavedState->headspritestat[0],sizeof(headspritestat));
        Bmemcpy(&prevspritestat[0],&pSavedState->prevspritestat[0],sizeof(prevspritestat));
        Bmemcpy(&nextspritestat[0],&pSavedState->nextspritestat[0],sizeof(nextspritestat));
#ifdef YAX_ENABLE
        numyaxbunches = pSavedState->numyaxbunches;
# if !defined NEW_MAP_FORMAT
        Bmemcpy(yax_bunchnum, pSavedState->yax_bunchnum, sizeof(yax_bunchnum));
        Bmemcpy(yax_nextwall, pSavedState->yax_nextwall, sizeof(yax_nextwall));
# endif
#endif
        Bmemcpy(&actor[0],&pSavedState->actor[0],sizeof(actor_t)*MAXSPRITES);

        g_cyclerCnt = pSavedState->g_cyclerCnt;
        Bmemcpy(&g_cyclers[0],&pSavedState->g_cyclers[0],sizeof(g_cyclers));
        Bmemcpy(&g_playerSpawnPoints[0],&pSavedState->g_playerSpawnPoints[0],sizeof(g_playerSpawnPoints));
        g_animWallCnt = pSavedState->g_animWallCnt;
        Bmemcpy(&SpriteDeletionQueue[0],&pSavedState->SpriteDeletionQueue[0],sizeof(SpriteDeletionQueue));
        g_spriteDeleteQueuePos = pSavedState->g_spriteDeleteQueuePos;
        Bmemcpy(&animwall[0],&pSavedState->animwall[0],sizeof(animwall));
        Bmemcpy(&g_origins[0],&pSavedState->origins[0],sizeof(g_origins));
        Bmemcpy(&g_mirrorWall[0],&pSavedState->g_mirrorWall[0],sizeof(g_mirrorWall));
        Bmemcpy(&g_mirrorSector[0],&pSavedState->g_mirrorSector[0],sizeof(g_mirrorSector));
        g_mirrorCount = pSavedState->g_mirrorCount;
        Bmemcpy(&show2dsector[0],&pSavedState->show2dsector[0],sizeof(show2dsector));
        g_cloudCnt = pSavedState->g_cloudCnt;
        Bmemcpy(&g_cloudSect[0],&pSavedState->g_cloudSect[0],sizeof(g_cloudSect));
        g_cloudX = pSavedState->g_cloudX;
        g_cloudY = pSavedState->g_cloudY;
        g_pskyidx = pSavedState->pskyidx;
        Bmemcpy(&g_animateGoal[0],&pSavedState->g_animateGoal[0],sizeof(g_animateGoal));
        Bmemcpy(&g_animateVel[0],&pSavedState->g_animateVel[0],sizeof(g_animateVel));
        g_animateCnt = pSavedState->g_animateCnt;
        Bmemcpy(&g_animateSect[0],&pSavedState->g_animateSect[0],sizeof(g_animateSect));

        Bmemcpy(&g_animatePtr[0],&pSavedState->g_animatePtr[0],sizeof(g_animatePtr));
        G_Util_PtrToIdx(g_animatePtr, g_animateCnt, sector, P2I_BACK);

        g_playerSpawnCnt = pSavedState->g_playerSpawnCnt;
        g_earthquakeTime = pSavedState->g_earthquakeTime;
        lockclock = pSavedState->lockclock;
        randomseed = pSavedState->randomseed;
        g_globalRandom = pSavedState->g_globalRandom;

#if !defined LUNATIC
        for (native_t i=g_gameVarCount-1; i>=0; i--)
        {
            if (aGameVars[i].flags & GAMEVAR_NORESET) continue;
            if (aGameVars[i].flags & GAMEVAR_PERPLAYER)
            {
                if (!pSavedState->vars[i]) continue;
                Bmemcpy(aGameVars[i].pValues, pSavedState->vars[i], sizeof(intptr_t) * MAXPLAYERS);
            }
            else if (aGameVars[i].flags & GAMEVAR_PERACTOR)
            {
                if (!pSavedState->vars[i]) continue;
                Bmemcpy(aGameVars[i].pValues, pSavedState->vars[i], sizeof(intptr_t) * MAXSPRITES);
            }
            else aGameVars[i].global = (intptr_t)pSavedState->vars[i];
        }

        for (native_t i=g_gameArrayCount-1; i>=0; i--)
        {
            if ((aGameArrays[i].flags & GAMEARRAY_RESTORE) == 0)
                continue;

            aGameArrays[i].size = pSavedState->arraysiz[i];
            Baligned_free(aGameArrays[i].pValues);
            aGameArrays[i].pValues = (intptr_t *) Xaligned_alloc(ARRAY_ALIGNMENT, Gv_GetArrayAllocSize(i));

            Bmemcpy(aGameArrays[i].pValues, pSavedState->arrays[i], Gv_GetArrayAllocSize(i));
        }

        Gv_RefreshPointers();
#else
        if (pSavedState->savecode)
        {
            El_RestoreGamevars(pSavedState->savecode);
        }
#endif
        // Update g_player[].ps->i (sprite indices of players) to be consistent
        // with just loaded sprites.
        // Otherwise, crashes may ensue: e.g. WGR2 SVN r391, map spiderden:
        // - walk forward (to door leading to other level "Shadowpine Forest")
        // - in new level, walk backward to get back to the Spider Den
        // - walk backward to the door leading to Shadowpine Forest --> crash.
        for (native_t SPRITES_OF(STAT_PLAYER, i))
        {
            int32_t snum = P_Get(i);
            Bassert((unsigned)snum < MAXPLAYERS);
            g_player[snum].ps->i = i;
        }

        for (native_t i=0; i<g_mostConcurrentPlayers; i++)
            sprite[g_player[i].ps->i].extra = playerHealth[i];

        if (g_player[myconnectindex].ps->over_shoulder_on != 0)
        {
            CAMERADIST = 0;
            CAMERACLOCK = 0;
            g_player[myconnectindex].ps->over_shoulder_on = 1;
        }

        screenpeek = myconnectindex;

        if (ud.lockout)
        {
            for (native_t x=g_animWallCnt-1; x>=0; x--)
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
            for (native_t x=g_numAnimWalls-1; x>=0; x--)
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
    vm.spriteNum = i;
    vm.pSprite   = &sprite[i];
    vm.playerNum = snum;
    vm.pPlayer   = g_player[snum].ps;

    return VM_CheckSquished();
}
#endif

// MYOS* CON commands.
LUNATIC_EXTERN void VM_DrawTileGeneric(int32_t x, int32_t y, int32_t zoom, int32_t tilenum, int32_t shade,
                                       int32_t orientation, int32_t p)
{
    orientation &= (ROTATESPRITE_MAX-1);

    int const rotAngle = (orientation&4) ? 1024 : 0;

    if (!(orientation&ROTATESPRITE_FULL16))
    {
        x<<=16;
        y<<=16;
    }

    rotatesprite_win(x, y, zoom, rotAngle, tilenum, shade, p, 2|orientation);
}

#if !defined LUNATIC
void VM_DrawTile(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation)
{
    DukePlayer_t *pPlayer = g_player[screenpeek].ps;
    int32_t       tilePal = pPlayer->cursectnum >= 0 ? sector[pPlayer->cursectnum].floorpal : 0;

    VM_DrawTileGeneric(x, y, 65536, tilenum, shade, orientation, tilePal);
}

void VM_DrawTilePal(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation, int32_t p)
{
    VM_DrawTileGeneric(x, y, 65536, tilenum, shade, orientation, p);
}

void VM_DrawTileSmall(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation)
{
    DukePlayer_t *const pPlayer = g_player[screenpeek].ps;
    int32_t             tilePal = pPlayer->cursectnum >= 0 ? sector[pPlayer->cursectnum].floorpal : 0;

    VM_DrawTileGeneric(x, y, 32768, tilenum, shade, orientation, tilePal);
}

void VM_DrawTilePalSmall(int32_t x, int32_t y, int32_t tilenum, int32_t shade, int32_t orientation, int32_t p)
{
    VM_DrawTileGeneric(x, y, 32768, tilenum, shade, orientation, p);
}
#endif
