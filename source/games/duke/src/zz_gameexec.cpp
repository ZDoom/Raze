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

#include "concmd.h"
#include "compat.h"

#include "duke3d.h"

#include "anim.h"

#include "menus.h"
#include "osdcmds.h"
#include "savegame.h"
#include "gamecvars.h"
#include "version.h"

#include "debugbreak.h"

BEGIN_DUKE_NS

#if KRANDDEBUG
# define GAMEEXEC_INLINE
# define GAMEEXEC_STATIC
#else
# define GAMEEXEC_INLINE inline
# define GAMEEXEC_STATIC static
#endif

vmstate_t vm;

enum vmflags_t
{
    VM_RETURN       = 0x00000001,
    VM_KILL         = 0x00000002,
    VM_NOEXECUTE    = 0x00000004,
    VM_SAFEDELETE   = 0x00000008
};

int32_t g_tw;
int32_t g_currentEvent = -1;
int32_t g_errorLineNum;

intptr_t const *insptr;

int32_t g_returnVarID    = -1;  // var ID of "RETURN"
int32_t g_weaponVarID    = -1;  // var ID of "WEAPON"
int32_t g_worksLikeVarID = -1;  // var ID of "WORKSLIKE"
int32_t g_zRangeVarID    = -1;  // var ID of "ZRANGE"
int32_t g_angRangeVarID  = -1;  // var ID of "ANGRANGE"
int32_t g_aimAngleVarID  = -1;  // var ID of "AUTOAIMANGLE"

// for timing events and actors
uint32_t g_actorCalls[MAXTILES];
double g_actorTotalMs[MAXTILES], g_actorMinMs[MAXTILES], g_actorMaxMs[MAXTILES];

GAMEEXEC_STATIC void VM_Execute(native_t loop);

#define VM_CONDITIONAL(xxx)                                                                                            \
    {                                                                                                                  \
        if ((xxx) || ((insptr = (intptr_t *)*(insptr + 1)) && (((*insptr) & VM_INSTMASK) == concmd_else)))                \
        {                                                                                                              \
            insptr += 2;                                                                                               \
            VM_Execute(0);                                                                                             \
        }                                                                                                              \
    }

void VM_ScriptInfo(intptr_t const *ptr, int range)
{
    if (!apScript || (!vm.pSprite && !vm.pPlayer))
        return;

    if (ptr)
    {
        Printf("\n");

        for (auto pScript = max<intptr_t const *>(ptr - (range >> 1), apScript),
                  p_end   = min<intptr_t const *>(ptr + (range >> 1), apScript + g_scriptSize);
             pScript < p_end;
             ++pScript)
        {
            Printf("%5d: %3d: ", (int32_t)(pScript - apScript), (int32_t)(pScript - ptr));

            if (*pScript >> 12 && (*pScript & VM_INSTMASK) < CON_END)
                Printf("%5d %s\n", (int32_t)(*pScript >> 12), VM_GetKeywordForID(*pScript & VM_INSTMASK));
            else
                Printf("%d\n", (int32_t)*pScript);
        }

        Printf("\n");
    }

    if (ptr == insptr)
    {
        if (vm.pUSprite)
            Printf("current actor: %d (%d)\n", vm.spriteNum, vm.pUSprite->picnum);

        Printf("g_errorLineNum: %d, g_tw: %d\n", g_errorLineNum, g_tw);
    }
}

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
static uspritetype dummy_sprite;
static actor_t     dummy_actor;

static inline void VM_DummySprite(void)
{
    vm.pUSprite = &dummy_sprite;
    vm.pActor   = &dummy_actor;
    vm.pData    = &dummy_actor.t_data[0];
}

// verification that the event actually exists happens elsewhere
static FORCE_INLINE int32_t VM_EventInlineInternal__(int const eventNum, int const spriteNum, int const playerNum,
                                                       int const playerDist = -1, int32_t returnValue = 0)
{
    vmstate_t const newVMstate = { spriteNum, playerNum, playerDist, 0,
                                   &sprite[spriteNum&(MAXSPRITES-1)],
                                   &actor[spriteNum&(MAXSPRITES-1)].t_data[0],
                                   g_player[playerNum&(MAXPLAYERS-1)].ps,
                                   &actor[spriteNum&(MAXSPRITES-1)] };

    auto &globalReturn = aGameVars[g_returnVarID].global;

    struct
    {
        vmstate_t vm;
        intptr_t globalReturn;
        int eventNum;
        intptr_t const *insptr;
    } const saved = { vm, globalReturn, g_currentEvent, insptr };

    vm = newVMstate;
    g_currentEvent = eventNum;
    insptr = apScript + apScriptEvents[eventNum];
    globalReturn = returnValue;

    double const t = timerGetHiTicks();

    if ((unsigned)spriteNum >= MAXSPRITES)
        VM_DummySprite();

    if ((unsigned)playerNum >= (unsigned)g_mostConcurrentPlayers)
        vm.pPlayer = g_player[0].ps;

    VM_Execute(true);

    if (vm.flags & VM_KILL)
        VM_DeleteSprite(vm.spriteNum, vm.playerNum);

    // restoring these needs to happen after VM_DeleteSprite() due to event recursion
    returnValue = globalReturn;

    vm             = saved.vm;
    globalReturn   = saved.globalReturn;
    g_currentEvent = saved.eventNum;
    insptr         = saved.insptr;

    return returnValue;
}

// the idea here is that the compiler inlines the call to VM_EventInlineInternal__() and gives us a set of
// functions which are optimized further based on distance/return having values known at compile time

int32_t VM_ExecuteEvent(int const nEventID, int const spriteNum, int const playerNum, int const nDist, int32_t const nReturn)
{
    return VM_EventInlineInternal__(nEventID, spriteNum, playerNum, nDist, nReturn);
}

int32_t VM_ExecuteEvent(int const nEventID, int const spriteNum, int const playerNum, int const nDist)
{
    return VM_EventInlineInternal__(nEventID, spriteNum, playerNum, nDist);
}

int32_t VM_ExecuteEvent(int const nEventID, int const spriteNum, int const playerNum)
{
    return VM_EventInlineInternal__(nEventID, spriteNum, playerNum);
}

int32_t VM_ExecuteEventWithValue(int const nEventID, int const spriteNum, int const playerNum, int32_t const nReturn)
{
    return VM_EventInlineInternal__(nEventID, spriteNum, playerNum, -1, nReturn);
}

bool ifsquished(int i, int p);

static int32_t VM_CheckSquished(void)
{
    return ifsquished(vm.spriteNum, vm.playerNum);
}

GAMEEXEC_STATIC GAMEEXEC_INLINE void P_ForceAngle(DukePlayer_t *pPlayer)
{
    int const nAngle = 128-(krand2()&255);

    pPlayer->q16horiz           += F16(64);
    pPlayer->return_to_center = 9;
    pPlayer->rotscrnang       = nAngle >> 1;
    pPlayer->look_ang         = pPlayer->rotscrnang;
}

// wow, this function sucks
int32_t A_Dodge(spritetype * const);
int32_t A_Dodge(spritetype * const pSprite)
{
    if (DEER)
    {
        pSprite->ang += ((rrdh_random() & 255) - 127) * 2;
        pSprite->ang &= 2047;
        return 0;
    }
    vec2_t const msin = { sintable[(pSprite->ang + 512) & 2047], sintable[pSprite->ang & 2047] };

    for (native_t nexti, SPRITES_OF_STAT_SAFE(STAT_PROJECTILE, i, nexti)) //weapons list
    {
        if (OW(i) == i || SECT(i) != pSprite->sectnum)
            continue;

        vec2_t const b = { SX(i) - pSprite->x, SY(i) - pSprite->y };
        vec2_t const v = { sintable[(SA(i) + 512) & 2047], sintable[SA(i) & 2047] };

        if (((msin.x * b.x) + (msin.y * b.y) >= 0) && ((v.x * b.x) + (v.y * b.y) < 0))
        {
            if (klabs((v.x * b.y) - (v.y * b.x)) < 65536 << 6)
            {
                pSprite->ang -= 512+(krand2()&1024);
                return 1;
            }
        }
    }

    return 0;
}

int32_t A_GetFurthestAngle(int const spriteNum, int const angDiv)
{
    uspritetype *const pSprite = (uspritetype *)&sprite[spriteNum];

    if (pSprite->picnum != TILE_APLAYER && (AC_COUNT(actor[spriteNum].t_data)&63) > 2)
        return pSprite->ang + 1024;

    int32_t   furthestAngle = 0;
    int32_t   greatestDist  = INT32_MIN;
    int const angIncs       = tabledivide32_noinline(2048, angDiv);
    hitdata_t hit;

    for (native_t j = pSprite->ang; j < (2048 + pSprite->ang); j += angIncs)
    {
        pSprite->z -= ZOFFSET3;
        hitscan((const vec3_t *)pSprite, pSprite->sectnum, sintable[(j + 512) & 2047], sintable[j & 2047], 0, &hit, CLIPMASK1);
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
    int const angincs = ((!g_netServer && ud.multimode < 2) && ud.player_skill < 3) ? 2048 / 2 : tabledivide32_noinline(2048, 1 + (krand2() & 1));

    for (native_t j = ts->ang; j < (2048 + ts->ang); j += (angincs-(krand2()&511)))
    {
        ts->z -= ZOFFSET2;
        hitscan((const vec3_t *)ts, ts->sectnum, sintable[(j + 512) & 2047], sintable[j & 2047], 16384 - (krand2() & 32767), &hit, CLIPMASK1);
        ts->z += ZOFFSET2;

        if (hit.sect < 0)
            continue;

        int const d  = klabs(hit.pos.x - ts->x) + klabs(hit.pos.y - ts->y);
        int const da = klabs(hit.pos.x - pnSprite->x) + klabs(hit.pos.y - pnSprite->y);

        if (d < da)
        {
            if (cansee(hit.pos.x, hit.pos.y, hit.pos.z, hit.sect, pnSprite->x, pnSprite->y, pnSprite->z - ZOFFSET2, pnSprite->sectnum))
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
    vec3_t const tempVect = {
        pSprite->x, pSprite->y, pSprite->z - ZOFFSET
    };
    getzrange(&tempVect, pSprite->sectnum, &actor[spriteNum].ceilingz, ceilhit, &actor[spriteNum].floorz, florhit, wallDist, CLIPMASK0);
}

void A_GetZLimits(int const spriteNum)
{
    spritetype *const pSprite = &sprite[spriteNum];
    int32_t           ceilhit, florhit;

    if (pSprite->statnum == STAT_PLAYER || pSprite->statnum == STAT_STANDABLE || pSprite->statnum == STAT_ZOMBIEACTOR
        || pSprite->statnum == STAT_ACTOR || pSprite->statnum == STAT_PROJECTILE)
    {
        VM_GetZRange(spriteNum, &ceilhit, &florhit, (pSprite->statnum == STAT_PROJECTILE) ? 4 : 127);
        actor[spriteNum].flags &= ~SFLAG_NOFLOORSHADOW;

        if ((florhit&49152) == 49152 && (sprite[florhit&(MAXSPRITES-1)].cstat&48) == 0)
        {
            uspritetype const * const hitspr = (uspritetype *)&sprite[florhit&(MAXSPRITES-1)];

            florhit &= (MAXSPRITES-1);

            // If a non-projectile would fall onto non-frozen enemy OR an enemy onto a player...
            if ((A_CheckEnemySprite(hitspr) && hitspr->pal != 1 && pSprite->statnum != STAT_PROJECTILE)
                    || (hitspr->picnum == TILE_APLAYER && A_CheckEnemySprite(pSprite)))
            {
                actor[spriteNum].flags |= SFLAG_NOFLOORSHADOW;  // No shadows on actors
                pSprite->xvel = -256;  // SLIDE_ABOVE_ENEMY
                A_SetSprite(spriteNum, CLIPMASK0);
            }
            else if (pSprite->statnum == STAT_PROJECTILE && hitspr->picnum == TILE_APLAYER && pSprite->owner==florhit)
            {
                actor[spriteNum].ceilingz = sector[pSprite->sectnum].ceilingz;
                actor[spriteNum].floorz   = sector[pSprite->sectnum].floorz;
            }
        }
    }
    else
    {
        actor[spriteNum].ceilingz = sector[pSprite->sectnum].ceilingz;
        actor[spriteNum].floorz   = sector[pSprite->sectnum].floorz;
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
    
    if (RRRA && spriteGravity == g_spriteGravity)
    {
        if (pSprite->picnum == TILE_BIKERB || pSprite->picnum == TILE_CHEERB)
            spriteGravity >>= 2;
        else if (pSprite->picnum == TILE_BIKERBV2)
            spriteGravity >>= 3;
    }

    if (pSprite->statnum == STAT_ACTOR || pSprite->statnum == STAT_PLAYER || pSprite->statnum == STAT_ZOMBIEACTOR
        || pSprite->statnum == STAT_STANDABLE)
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
        if (pSprite->zvel < 6144)
            pSprite->zvel += spriteGravity;
        else pSprite->zvel = 6144;
        pSprite->z += pSprite->zvel;
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
//        Printf("G_GetAngleDelta() returning %d\n",na-a);
        return newAngle-currAngle;
    }

    if (newAngle > 1024)
        newAngle -= 2048;
    if (currAngle > 1024)
        currAngle -= 2048;

//    Printf("G_GetAngleDelta() returning %d\n",na-a);
    return newAngle-currAngle;
}

GAMEEXEC_STATIC void VM_AlterAng(int32_t const moveFlags)
{
    int const elapsedTics = (AC_COUNT(vm.pData))&31;

    const intptr_t *moveptr;
    if (EDUKE32_PREDICT_FALSE((unsigned)AC_MOVE_ID(vm.pData) >= (unsigned)g_scriptSize-1))

    {
        AC_MOVE_ID(vm.pData) = 0;
        Printf(TEXTCOLOR_RED "bad moveptr for actor %d (%d)!\n", vm.spriteNum, vm.pUSprite->picnum);
        return;
    }

    moveptr = apScript + AC_MOVE_ID(vm.pData);

    vm.pSprite->xvel += (moveptr[0] - vm.pSprite->xvel)/5;
    if (vm.pSprite->zvel < 648)
        vm.pSprite->zvel += ((moveptr[1]<<4) - vm.pSprite->zvel)/5;

    if (RRRA && (moveFlags&windang))
        vm.pSprite->ang = g_windDir;
    else if (moveFlags&seekplayer)
    {
        int const spriteAngle    = vm.pSprite->ang;
        int const holoDukeSprite = vm.pPlayer->holoduke_on;

        // NOTE: looks like 'owner' is set to target sprite ID...

        vm.pSprite->owner = (!RR && holoDukeSprite >= 0
                             && cansee(sprite[holoDukeSprite].x, sprite[holoDukeSprite].y, sprite[holoDukeSprite].z, sprite[holoDukeSprite].sectnum,
                                       vm.pSprite->x, vm.pSprite->y, vm.pSprite->z, vm.pSprite->sectnum))
          ? holoDukeSprite
          : vm.pPlayer->i;

        int const goalAng = (sprite[vm.pSprite->owner].picnum == TILE_APLAYER)
                  ? getangle(vm.pActor->lastv.x - vm.pSprite->x, vm.pActor->lastv.y - vm.pSprite->y)
                  : getangle(sprite[vm.pSprite->owner].x - vm.pSprite->x, sprite[vm.pSprite->owner].y - vm.pSprite->y);

        if (vm.pSprite->xvel && vm.pSprite->picnum != TILE_DRONE)
        {
            int const angDiff = G_GetAngleDelta(spriteAngle, goalAng);

            if (elapsedTics < 2)
            {
                if (klabs(angDiff) < 256)
                {
                    int const angInc = 128-(krand2()&256);
                    vm.pSprite->ang += angInc;
                    if (A_GetHitscanRange(vm.spriteNum) < 844)
                        vm.pSprite->ang -= angInc;
                }
            }
            else if (elapsedTics > 18 && elapsedTics < GAMETICSPERSEC) // choose
            {
                if (klabs(angDiff >> 2) < 128)
                    vm.pSprite->ang = goalAng;
                else
                    vm.pSprite->ang += angDiff >> 2;
            }
        }
        else
            vm.pSprite->ang = goalAng;
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

    if (angDiff > -8 && angDiff < 0)
        angDiff = 0;

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
    auto const movflagsptr = &AC_MOVFLAGS(vm.pSprite, &actor[vm.spriteNum]);
    // NOTE: test against -1 commented out and later revived in source history
    // XXX: Does its presence/absence break anything? Where are movflags with all bits set created?
    int const movflags = (*movflagsptr == (std::remove_pointer<decltype(movflagsptr)>::type)-1) ? 0 : *movflagsptr;
    
    AC_COUNT(vm.pData)++;

    if (movflags&face_player)
        VM_FacePlayer(2);

    if (movflags&spin)
        vm.pSprite->ang += sintable[((AC_COUNT(vm.pData)<<3)&2047)]>>6;

    if (movflags&face_player_slow)
    {
        int const goalAng = (vm.pPlayer->newowner >= 0) ? getangle(vm.pPlayer->opos.x - vm.pSprite->x, vm.pPlayer->opos.y - vm.pSprite->y)
                                                 : getangle(vm.pPlayer->pos.x - vm.pSprite->x, vm.pPlayer->pos.y - vm.pSprite->y);

        vm.pSprite->ang += ksgn(G_GetAngleDelta(vm.pSprite->ang, goalAng)) << 5;
    }

    if (RRRA && (movflags&antifaceplayerslow))
    {
        int goalAng = (vm.pPlayer->newowner >= 0) ? getangle(vm.pPlayer->opos.x - vm.pSprite->x, vm.pPlayer->opos.y - vm.pSprite->y)
                                                 : getangle(vm.pPlayer->pos.x - vm.pSprite->x, vm.pPlayer->pos.y - vm.pSprite->y);
        goalAng = (goalAng+1024)&2047;

        vm.pSprite->ang += ksgn(G_GetAngleDelta(vm.pSprite->ang, goalAng)) << 5;
    }

    if ((movflags&jumptoplayer_bits) == jumptoplayer_bits)
    {
        if (AC_COUNT(vm.pData) < 16)
            vm.pSprite->zvel -= (!DEER && RRRA && vm.pSprite->picnum == TILE_CHEER) ? (sintable[(512+(AC_COUNT(vm.pData)<<4))&2047]/40)
            : (sintable[(512+(AC_COUNT(vm.pData)<<4))&2047]>>5);
    }

    if (movflags&face_player_smart)
    {
        vec2_t const vect = { vm.pPlayer->pos.x + (vm.pPlayer->vel.x / 768), vm.pPlayer->pos.y + (vm.pPlayer->vel.y / 768) };
        VM_AddAngle(2, getangle(vect.x - vm.pSprite->x, vect.y - vm.pSprite->y));
    }

    if (RRRA && (vm.pSprite->picnum == TILE_RABBIT || vm.pSprite->picnum == TILE_MAMA))
    {
        if(movflags&jumptoplayer_only)
        {
            if (AC_COUNT(vm.pData) < 8)
                vm.pSprite->zvel -= sintable[(512+(AC_COUNT(vm.pData)<<4))&2047]/(vm.pSprite->picnum == TILE_RABBIT ? 30 : 35);
        }
        if(movflags&justjump2)
        {
            if (AC_COUNT(vm.pData) < 8)
                vm.pSprite->zvel -= sintable[(512+(AC_COUNT(vm.pData)<<4))&2047]/(vm.pSprite->picnum == TILE_RABBIT ? 24 : 28);
        }
    }

    if (RRRA && (movflags&windang))
    {
        if (AC_COUNT(vm.pData) < 8)
            vm.pSprite->zvel -= sintable[(512+(AC_COUNT(vm.pData)<<4))&2047]/24;
    }
    
    if (AC_MOVE_ID(vm.pData) == 0 || movflags == 0)
    {
        if ((A_CheckEnemySprite(vm.pSprite) && vm.pSprite->extra <= 0) || (vm.pActor->bpos.x != vm.pSprite->x) || (vm.pActor->bpos.y != vm.pSprite->y))
        {
            vm.pActor->bpos.x = vm.pSprite->x;
            vm.pActor->bpos.y = vm.pSprite->y;
            setsprite(vm.spriteNum, (vec3_t *)vm.pSprite);
        }
        if (RR && A_CheckEnemySprite(vm.pSprite) && vm.pSprite->extra <= 0)
        {
            vm.pSprite->shade += (sector[vm.pSprite->sectnum].ceilingstat & 1) ? ((g_shadedSector[vm.pSprite->sectnum] == 1 ? 16 : sector[vm.pSprite->sectnum].ceilingshade) - vm.pSprite->shade) >> 1
                                                                 : (sector[vm.pSprite->sectnum].floorshade - vm.pSprite->shade) >> 1;
        }
        return;
    }

    if (EDUKE32_PREDICT_FALSE((unsigned)AC_MOVE_ID(vm.pData) >= (unsigned)g_scriptSize-1))
    {
        AC_MOVE_ID(vm.pData) = 0;
        Printf(TEXTCOLOR_RED "clearing bad moveptr for actor %d (%d)\n", vm.spriteNum, vm.pUSprite->picnum);
        return;
    }

    intptr_t const * const moveptr = apScript + AC_MOVE_ID(vm.pData);

    if (movflags & geth)
        vm.pSprite->xvel += ((moveptr[0]) - vm.pSprite->xvel) >> 1;
    if (movflags & getv)
        vm.pSprite->zvel += ((moveptr[1] << 4) - vm.pSprite->zvel) >> 1;

    if (movflags&dodgebullet)
        A_Dodge(vm.pSprite);

    if (vm.pSprite->picnum != TILE_APLAYER)
        VM_AlterAng(movflags);

    if (vm.pSprite->xvel > -6 && vm.pSprite->xvel < 6)
        vm.pSprite->xvel = 0;

    int badguyp = A_CheckEnemySprite(vm.pSprite);

    if (vm.pSprite->xvel || vm.pSprite->zvel)
    {
        int spriteXvel = vm.pSprite->xvel;
        int angDiff    = vm.pSprite->ang;

        if (badguyp && (vm.pSprite->picnum != TILE_ROTATEGUN || RR))
        {
            if ((vm.pSprite->picnum == TILE_DRONE || (!RR && vm.pSprite->picnum == TILE_COMMANDER)) && vm.pSprite->extra > 0)
            {
                if (!RR && vm.pSprite->picnum == TILE_COMMANDER)
                {
                    int32_t nSectorZ;
                    // NOTE: TILE_COMMANDER updates both actor[].floorz and
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
                    // The TILE_DRONE updates either .floorz or .ceilingz, not both.
                    if (vm.pSprite->zvel > 0)
                    {
                        vm.pActor->floorz = nSectorZ = VM_GetFlorZOfSlope();
                        int const zDiff = RRRA ? (28<<8) : (30<<8);
                        if (vm.pSprite->z > nSectorZ-zDiff)
                            vm.pSprite->z = nSectorZ-zDiff;
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
            else if (vm.pSprite->picnum != TILE_ORGANTIC || RR)
            {
                // All other actors besides TILE_ORGANTIC don't update .floorz or
                // .ceilingz here.
                if (vm.pSprite->zvel > 0)
                {
                    if (vm.pSprite->z > vm.pActor->floorz)
                        vm.pSprite->z = vm.pActor->floorz;
                    //vm.pSprite->z += A_GetWaterZOffset(vm.spriteNum);
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
            else if (vm.pSprite->picnum != TILE_DRONE && vm.pSprite->picnum != TILE_SHARK
                && ((!RR && vm.pSprite->picnum != TILE_COMMANDER)
                    || (RR && vm.pSprite->picnum != TILE_UFO1)
                    || (RR && !RRRA && vm.pSprite->picnum != TILE_UFO2 && vm.pSprite->picnum != TILE_UFO3 && vm.pSprite->picnum != TILE_UFO4 && vm.pSprite->picnum != TILE_UFO5)))
            {
                if (vm.pPlayer->actorsqu == vm.spriteNum)
                    return;

                if (vm.pActor->bpos.z != vm.pSprite->z || (!g_netServer && ud.multimode < 2 && ud.player_skill < 2))
                {
                    if (AC_COUNT(vm.pData)&1) return;
                    spriteXvel <<= 1;
                }
                else
                {
                    if (AC_COUNT(vm.pData)&3) return;
                    spriteXvel <<= 2;
                }
            }
        }
        else if (vm.pSprite->picnum == TILE_APLAYER)
            if (vm.pSprite->z < vm.pActor->ceilingz+ZOFFSET5)
                vm.pSprite->z = vm.pActor->ceilingz+ZOFFSET5;

        if (RRRA)
        {
            if (sector[vm.pSprite->sectnum].lotag != ST_1_ABOVE_WATER)
            {
                switch (DYNAMICTILEMAP(vm.pSprite->picnum))
                {
                case MINIONBOAT__STATICRR:
                case HULK__STATICRR:
                case CHEERBOAT__STATICRR:
                    spriteXvel >>= 1;
                    break;
                }
            }
            else
            {
                switch (DYNAMICTILEMAP(vm.pSprite->picnum))
                {
                case BIKERB__STATICRR:
                case BIKERBV2__STATICRR:
                case CHEERB__STATICRR:
                    spriteXvel >>= 1;
                    break;
                }
            }
        }

        vec3_t const vect
        = { (spriteXvel * (sintable[(angDiff + 512) & 2047])) >> 14, (spriteXvel * (sintable[angDiff & 2047])) >> 14, vm.pSprite->zvel };

        vm.pActor->movflag = A_MoveSprite(vm.spriteNum, &vect, CLIPMASK0);
        if (DEER && vm.pSprite->picnum != TILE_DOGRUN)
        {
            if ((vm.pActor->movflag & 49152) == 32768)
            {
                int const wallnum = vm.pActor->movflag & (MAXWALLS-1);
                if (ghcons_isanimalescapewall(wallnum))
                {
                    vm.pSprite->z = sector[vm.pSprite->sectnum].ceilingz;
                    A_MoveSprite(vm.spriteNum, &vect, CLIPMASK0);
                    vm.pSprite->z = sector[vm.pSprite->sectnum].floorz;
                }
            }
            else if ((vm.pActor->movflag & 49152) == 16384)
            {
                int sectnum = vm.pActor->movflag & (MAXSECTORS-1);
                if (ghcons_isanimalescapesect(sectnum))
                {
                    A_MoveSprite(vm.spriteNum, &vect, CLIPMASK0);
                    vm.pSprite->z = sector[vm.pSprite->sectnum].floorz;
                }
            }
        }
    }

    if (!badguyp)
        return;

    vm.pSprite->shade += (sector[vm.pSprite->sectnum].ceilingstat & 1) ? ((g_shadedSector[vm.pSprite->sectnum] == 1 ? 16 : sector[vm.pSprite->sectnum].ceilingshade) - vm.pSprite->shade) >> 1
                                                                 : (sector[vm.pSprite->sectnum].floorshade - vm.pSprite->shade) >> 1;

    if (sector[vm.pSprite->sectnum].floorpicnum == TILE_MIRROR)
        A_DeleteSprite(vm.spriteNum);
}

static void VM_AddWeapon(DukePlayer_t * const pPlayer, int const weaponNum, int const nAmount)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)weaponNum >= MAX_WEAPONS))
    {
        CON_ERRPRINTF("invalid weapon %d\n", weaponNum);
        return;
    }

    if (!pPlayer->gotweapon[weaponNum])
    {
        P_AddWeapon(pPlayer, weaponNum);
    }
    else if (pPlayer->ammo_amount[weaponNum] >= pPlayer->max_ammo_amount[weaponNum])
    {
        vm.flags |= VM_NOEXECUTE;
        return;
    }

    P_AddAmmo(pPlayer, weaponNum, nAmount);

    if (pPlayer->curr_weapon == KNEE_WEAPON && (pPlayer->gotweapon[weaponNum]))
        P_AddWeapon(pPlayer, weaponNum);
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

    P_AddAmmo(pPlayer, weaponNum, nAmount);

    if (pPlayer->curr_weapon == KNEE_WEAPON && (pPlayer->gotweapon[weaponNum]))
        P_AddWeapon(pPlayer, weaponNum);
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
        if (RR)
        {
            switch (vm.pSprite->lotag)
            {
                case 100: pPlayer->keys[1] = 1; break;
                case 101: pPlayer->keys[2] = 1; break;
                case 102: pPlayer->keys[3] = 1; break;
                case 103: pPlayer->keys[4] = 1; break;
            }
        }
        else
        {
            switch (vm.pSprite->pal)
            {
                case 0: pPlayer->got_access |= 1; break;
                case 21: pPlayer->got_access |= 2; break;
                case 23: pPlayer->got_access |= 4; break;
            }
        }
        break;

        default: CON_ERRPRINTF("invalid inventory item %d\n", itemNum); break;
    }
}

static int32_t A_GetWaterZOffset(int const spriteNum)
{
    uspritetype const *const pSprite = (uspritetype *)&sprite[spriteNum];

    if (sector[pSprite->sectnum].lotag == ST_1_ABOVE_WATER)
    {
        if (RRRA)
        {
            switch (DYNAMICTILEMAP(pSprite->picnum))
            {
                case HULKBOAT__STATICRR:
                    return (12<<8);
                case MINIONBOAT__STATICRR:
                    return (3<<8);
                case CHEERBOAT__STATICRR:
                case EMPTYBOAT__STATICRR:
                    return (6<<8);
            }
        }
        if (A_CheckSpriteFlags(spriteNum, SFLAG_NOWATERDIP))
            return 0;

        return ACTOR_ONWATER_ADDZ;
    }

    return 0;
}

static void VM_Fall(int const spriteNum, spritetype * const pSprite)
{
    int spriteGravity = g_spriteGravity;
    int hitSprite = 0;

    pSprite->xoffset = pSprite->yoffset = 0;

    if (RR)
    {
        if (RRRA)
        {
            if (sector[vm.pSprite->sectnum].lotag == 801)
            {
                if (vm.pSprite->picnum == TILE_ROCK)
                {
                    A_Spawn(vm.spriteNum, TILE_ROCK2);
                    A_Spawn(vm.spriteNum, TILE_ROCK2);
                    vm.flags |= VM_SAFEDELETE;
                }
            }
            else if (sector[vm.pSprite->sectnum].lotag == 802)
            {
                if (vm.pSprite->picnum != TILE_APLAYER && A_CheckEnemySprite(vm.pSprite) && vm.pSprite->z == vm.pActor->floorz - ZOFFSET)
                {
                    A_DoGuts(vm.spriteNum, TILE_JIBS6, 5);
                    A_PlaySound(SQUISHED, vm.spriteNum);
                    vm.flags |= VM_SAFEDELETE;
                }
            }
            else if (sector[vm.pSprite->sectnum].lotag == 803)
            {
                if (vm.pSprite->picnum == TILE_ROCK2)
                {
                    vm.flags |= VM_SAFEDELETE;
                }
            }
        }
        if (sector[vm.pSprite->sectnum].lotag == 800)
        {
            if (vm.pSprite->picnum == TILE_AMMO)
            {
                vm.flags |= VM_SAFEDELETE;
                return;
            }
            if (vm.pSprite->picnum != TILE_APLAYER && (A_CheckEnemySprite(vm.pSprite) || vm.pSprite->picnum == TILE_COW) && g_spriteExtra[vm.spriteNum] < 128)
            {
                vm.pSprite->z = vm.pActor->floorz-ZOFFSET;
                vm.pSprite->zvel = 8000;
                vm.pSprite->extra = 0;
                g_spriteExtra[vm.spriteNum]++;
                hitSprite = 1;
            }
            else if (vm.pSprite->picnum != TILE_APLAYER)
            {
                if (!g_spriteExtra[vm.spriteNum])
                    vm.flags |= VM_SAFEDELETE;
                return;
            }
            vm.pActor->picnum = TILE_SHOTSPARK1;
            vm.pActor->extra = 1;
        }
        if (RRRA && EDUKE32_PREDICT_TRUE(sector[vm.pSprite->sectnum].lotag < 800 || sector[vm.pSprite->sectnum].lotag > 803)
            && (sector[vm.pSprite->sectnum].floorpicnum == TILE_RRTILE7820 || sector[vm.pSprite->sectnum].floorpicnum == TILE_RRTILE7768))
        {
            if (vm.pSprite->picnum != TILE_MINION && vm.pSprite->pal != 19)
            {
                if ((krand2()&3) == 1)
                {
                    vm.pActor->picnum = TILE_SHOTSPARK1;
                    vm.pActor->extra = 5;
                }
            }
        }
    }

    if (sector[pSprite->sectnum].lotag == ST_2_UNDERWATER || EDUKE32_PREDICT_FALSE(G_CheckForSpaceCeiling(pSprite->sectnum)))
        spriteGravity = g_spriteGravity/6;
    else if (EDUKE32_PREDICT_FALSE(G_CheckForSpaceFloor(pSprite->sectnum)))
        spriteGravity = 0;

    if (actor[spriteNum].cgg <= 0 || (sector[pSprite->sectnum].floorstat&2))
    {
        A_GetZLimits(spriteNum);
        actor[spriteNum].cgg = 6;
    }
    else actor[spriteNum].cgg--;

    if (pSprite->z < actor[spriteNum].floorz-ZOFFSET)
    {
        // Free fall.
        pSprite->zvel += spriteGravity;
        pSprite->z += pSprite->zvel;

#ifdef YAX_ENABLE
        if (yax_getbunch(pSprite->sectnum, YAX_FLOOR) >= 0 && (sector[pSprite->sectnum].floorstat & 512) == 0)
            setspritez(spriteNum, (vec3_t *)pSprite);
#endif

        if (pSprite->zvel > 6144) pSprite->zvel = 6144;
        return;
    }

    pSprite->z = actor[spriteNum].floorz - ZOFFSET;

    if (A_CheckEnemySprite(pSprite) || (pSprite->picnum == TILE_APLAYER && pSprite->owner >= 0))
    {
        if (pSprite->zvel > 3084 && pSprite->extra <= 1)
        {
            // I'm guessing this TILE_DRONE check is from a beta version of the game
            // where they crashed into the ground when killed
            if (!(pSprite->picnum == TILE_APLAYER && pSprite->extra > 0) && pSprite->pal != 1 && pSprite->picnum != TILE_DRONE)
            {
                A_PlaySound(SQUISHED,spriteNum);
                if (hitSprite)
                {
                    A_DoGuts(spriteNum,TILE_JIBS6,5);
                }
                else
                {
                    A_DoGuts(spriteNum,TILE_JIBS6,15);
                    A_Spawn(spriteNum,TILE_BLOODPOOL);
                }
            }
            actor[spriteNum].picnum = TILE_SHOTSPARK1;
            actor[spriteNum].extra = 1;
            pSprite->zvel = 0;
        }
        else if (pSprite->zvel > 2048 && sector[pSprite->sectnum].lotag != ST_1_ABOVE_WATER)
        {
            int16_t newsect = pSprite->sectnum;

            pushmove((vec3_t *)pSprite, &newsect, 128, 4<<8, 4<<8, CLIPMASK0);
            if ((unsigned)newsect < MAXSECTORS)
                changespritesect(spriteNum, newsect);

            if (!DEER)
                A_PlaySound(THUD, spriteNum);
        }
    }

    if (sector[pSprite->sectnum].lotag == ST_1_ABOVE_WATER)
    {
        pSprite->z += A_GetWaterZOffset(spriteNum);
        return;
    }

    pSprite->zvel = 0;
}

static int32_t VM_ResetPlayer(int const playerNum, int32_t vmFlags)
{
    //AddLog("resetplayer");
    if (!g_netServer && ud.multimode < 2)
    {
#if 0
        if (g_quickload && g_quickload->isValid() && ud.recstat != 2)
        {
			M_StartControlPanel(false);
			M_SetMenu(NAME_ConfirmPlayerReset);
		}
        else
#endif
            g_player[playerNum].ps->gm = MODE_RESTART;
        vmFlags |= VM_NOEXECUTE;
    }
    else
    {
        if (playerNum == myconnectindex)
        {
            CAMERADIST = 0;
            CAMERACLOCK = (int32_t) totalclock;
        }

        //if (g_fakeMultiMode)
            P_ResetPlayer(playerNum);
#ifndef NETCODE_DISABLE
        //if (g_netServer)
        //{
        //    P_ResetPlayer(playerNum);
        //    Net_SpawnPlayer(playerNum);
        //}
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

    // Printf("Time&date: %s\n",asctime (ti));

    pValues[0] = pTime->tm_sec;
    pValues[1] = pTime->tm_min;
    pValues[2] = pTime->tm_hour;
    pValues[3] = pTime->tm_mday;
    pValues[4] = pTime->tm_mon;
    pValues[5] = pTime->tm_year+1900;
    pValues[6] = pTime->tm_wday;
    pValues[7] = pTime->tm_yday;
}

void Screen_Play(void)
{
    int32_t running = 1;

    inputState.ClearAllInput();

	do
    {
        G_HandleAsync();

        ototalclock = totalclock + 1; // pause game like ANMs

        if (!G_FPSLimit())
            continue;

        videoClearScreen(0);
        if (inputState.CheckAllInput())
            running = 0;

        videoNextPage();
        inputState.ClearAllInput();
    } while (running);
}

GAMEEXEC_STATIC void VM_Execute(native_t loop)
{
    native_t            tw      = *insptr;
    DukePlayer_t *const pPlayer = vm.pPlayer;

    // jump directly into the loop, skipping branches during the first iteration
    goto skip_check;

    while (loop)
    {
        if (vm.flags & (VM_RETURN | VM_KILL | VM_NOEXECUTE))
            break;

        tw = *insptr;

    skip_check:
        //      Bsprintf(g_szBuf,"Parsing: %d",*insptr);
        //      AddLog(g_szBuf);

        g_errorLineNum = tw >> 12;
        g_tw           = tw &= VM_INSTMASK;

        if (tw == concmd_leftbrace)
        {
            insptr++, loop++;
            continue;
        }
        else if (tw == concmd_rightbrace)
        {
            insptr++, loop--;
            continue;
        }
        else if (tw == concmd_else)
        {
            insptr = (intptr_t *)*(insptr + 1);
            continue;
        }
        else if (tw == concmd_state)
        {
            intptr_t const *const tempscrptr = insptr + 2;
            insptr                           = (intptr_t *)*(insptr + 1);
            VM_Execute(1);
            insptr = tempscrptr;
            continue;
        }

        switch (tw)
        {
            case concmd_enda:
            case concmd_break:
            case concmd_ends:
            case concmd_endevent: return;

            case concmd_ifrnd: VM_CONDITIONAL(rnd(*(++insptr))); continue;

            case concmd_ifcanshoottarget:
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

#define CHECK(x)                                                                                                                                     \
    if (x >= 0 && sprite[x].picnum == vm.pSprite->picnum)                                                                                            \
    {                                                                                                                                                \
        VM_CONDITIONAL(0);                                                                                                                           \
        continue;                                                                                                                                    \
    }
#define CHECK2(x)                                                                                                                                    \
    do                                                                                                                                               \
    {                                                                                                                                                \
        vm.pSprite->ang += x;                                                                                                                        \
        tw = A_CheckHitSprite(vm.spriteNum, &temphit);                                                                                               \
        vm.pSprite->ang -= x;                                                                                                                        \
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
                    VM_CONDITIONAL(0);
                    continue;
                }
                VM_CONDITIONAL(1);
            }
                continue;

            case concmd_ifcanseetarget:
                tw = cansee(vm.pSprite->x, vm.pSprite->y, vm.pSprite->z - ((krand2() & 41) << 8), vm.pSprite->sectnum, pPlayer->pos.x, pPlayer->pos.y,
                            pPlayer->pos.z /*-((krand2()&41)<<8)*/, sprite[pPlayer->i].sectnum);
                VM_CONDITIONAL(tw);
                if (tw)
                    vm.pActor->timetosleep = SLEEPTIME;
                continue;

            case concmd_ifnocover:
                tw = cansee(vm.pSprite->x, vm.pSprite->y, vm.pSprite->z, vm.pSprite->sectnum, pPlayer->pos.x, pPlayer->pos.y,
                            pPlayer->pos.z, sprite[pPlayer->i].sectnum);
                VM_CONDITIONAL(tw);
                if (tw)
                    vm.pActor->timetosleep = SLEEPTIME;
                continue;

            case concmd_ifactornotstayput: VM_CONDITIONAL(vm.pActor->actorstayput == -1); continue;

            case concmd_ifcansee:
            {
                uspritetype *pSprite = (uspritetype *)&sprite[pPlayer->i];

                if (DEER)
                {
                    if (sintable[vm.pSprite->ang&2047] * (pSprite->y - vm.pSprite->y) + sintable[(vm.pSprite->ang+512)&2047] * (pSprite->x - vm.pSprite->x) >= 0)
                        tw = cansee(vm.pSprite->x, vm.pSprite->y, vm.pSprite->z - (krand2() % 13312), vm.pSprite->sectnum,
                            pSprite->x, pSprite->y, pPlayer->opos.z-(krand2() % 8192), pPlayer->cursectnum);
                    else
                        tw = 0;

                    VM_CONDITIONAL(tw);
                    continue;
                }

// select sprite for monster to target
// if holoduke is on, let them target holoduke first.
//
                if (!RR && pPlayer->holoduke_on >= 0)
                {
                    pSprite = (uspritetype *)&sprite[pPlayer->holoduke_on];
                    tw = cansee(vm.pSprite->x, vm.pSprite->y, vm.pSprite->z - (krand2() & (ZOFFSET5 - 1)), vm.pSprite->sectnum, pSprite->x, pSprite->y,
                                pSprite->z, pSprite->sectnum);

                    if (tw == 0)
                    {
                        // they can't see player's holoduke
                        // check for player...
                        pSprite = (uspritetype *)&sprite[pPlayer->i];
                    }
                }
                // can they see player, (or player's holoduke)
                tw = cansee(vm.pSprite->x, vm.pSprite->y, vm.pSprite->z - (krand2() & ((47 << 8))), vm.pSprite->sectnum, pSprite->x, pSprite->y,
                            pSprite->z - (RR ? (28 << 8) : (24 << 8)), pSprite->sectnum);

                if (tw == 0)
                {
                    // search around for target player

                    // also modifies 'target' x&y if found..

                    tw = 1;
                    if (A_FurthestVisiblePoint(vm.spriteNum, pSprite, &vm.pActor->lastv) == -1)
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

            case concmd_ifhitweapon:
                if (DEER)
                {
                    VM_CONDITIONAL(ghtrophy_isakill(vm.spriteNum));
                }
                else
                {
                    VM_CONDITIONAL(A_IncurDamage(vm.spriteNum) >= 0);
                }
                continue;

            case concmd_ifsquished: VM_CONDITIONAL(VM_CheckSquished()); continue;

            case concmd_ifdead: VM_CONDITIONAL(vm.pSprite->extra - (vm.pSprite->picnum == TILE_APLAYER) < 0); continue;

            case concmd_ai:
                insptr++;
                // Following changed to use pointersizes
                AC_AI_ID(vm.pData)     = *insptr++;                         // Ai
                AC_ACTION_ID(vm.pData) = *(apScript + AC_AI_ID(vm.pData));  // Action
                AC_MOVE_ID(vm.pData) = *(apScript + AC_AI_ID(vm.pData) + 1);  // move

                vm.pSprite->hitag = *(apScript + AC_AI_ID(vm.pData) + 2);  // move flags

                AC_COUNT(vm.pData)        = 0;
                AC_ACTION_COUNT(vm.pData) = 0;
                AC_CURFRAME(vm.pData)     = 0;

                if (vm.pSprite->hitag & random_angle)
                    vm.pSprite->ang = krand2() & 2047;
                continue;

            case concmd_action:
                insptr++;
                AC_ACTION_COUNT(vm.pData) = 0;
                AC_CURFRAME(vm.pData)     = 0;
                AC_ACTION_ID(vm.pData)    = *insptr++;
                continue;

            case concmd_ifpdistl:
                insptr++;
                VM_CONDITIONAL(!(DEER && sub_535EC()) && vm.playerDist < *(insptr));
                if (vm.playerDist > MAXSLEEPDIST && vm.pActor->timetosleep == 0)
                    vm.pActor->timetosleep = SLEEPTIME;
                continue;

            case concmd_ifpdistg:
                VM_CONDITIONAL(vm.playerDist > *(++insptr));
                if (vm.playerDist > MAXSLEEPDIST && vm.pActor->timetosleep == 0)
                    vm.pActor->timetosleep = SLEEPTIME;
                continue;

            case concmd_addstrength:
                insptr++;
                vm.pSprite->extra += *insptr++;
                continue;

            case concmd_strength:
                insptr++;
                vm.pSprite->extra = *insptr++;
                continue;

            case concmd_smacksprite:
                insptr++;
                if (krand2()&1)
                    vm.pSprite->ang = (vm.pSprite->ang-(512+(krand2()&511)))&2047;
                else
                    vm.pSprite->ang = (vm.pSprite->ang+(512+(krand2()&511)))&2047;
                continue;

            case concmd_fakebubba:
                insptr++;
                switch (++g_fakeBubbaCnt)
                {
                case 1:
                    A_Spawn(vm.spriteNum, TILE_PIG);
                    break;
                case 2:
                    A_Spawn(vm.spriteNum, TILE_MINION);
                    break;
                case 3:
                    A_Spawn(vm.spriteNum, TILE_CHEER);
                    break;
                case 4:
                    A_Spawn(vm.spriteNum, TILE_VIXEN);
                    operateactivators(666, vm.playerNum);
                    break;
                }
                continue;

            case concmd_rndmove:
                insptr++;
                vm.pSprite->ang = krand2()&2047;
                vm.pSprite->xvel = 25;
                continue;

            case concmd_mamatrigger:
                insptr++;
                operateactivators(667, vm.playerNum);
                continue;

            case concmd_mamaspawn:
                insptr++;
                if (g_mamaSpawnCnt)
                {
                    g_mamaSpawnCnt--;
                    A_Spawn(vm.spriteNum, TILE_RABBIT);
                }
                continue;

            case concmd_mamaquake:
                insptr++;
                if (vm.pSprite->pal == 31)
                    g_earthquakeTime = 4;
                else if(vm.pSprite->pal == 32)
                    g_earthquakeTime = 6;
                continue;

            case concmd_garybanjo:
                insptr++;
                if (g_banjoSong == 0)
                {
                    switch (krand2()&3)
                    {
                    case 3:
                        g_banjoSong = 262;
                        break;
                    case 0:
                        g_banjoSong = 272;
                        break;
                    default:
                        g_banjoSong = 273;
                        break;
                    }
                    A_PlaySound(g_banjoSong, vm.spriteNum);
                }
                else if (!S_CheckSoundPlaying(vm.spriteNum, g_banjoSong))
                    A_PlaySound(g_banjoSong, vm.spriteNum, CHAN_WEAPON);
                continue;
            case concmd_motoloopsnd:
                insptr++;
                if (!S_CheckSoundPlaying(vm.spriteNum, 411))
                    A_PlaySound(411, vm.spriteNum);
                continue;

            case concmd_ifgotweaponce:
                insptr++;

                if ((g_gametypeFlags[ud.coop] & GAMETYPE_WEAPSTAY) && (g_netServer || ud.multimode > 1))
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
                    else if (pPlayer->weapreccnt < MAX_WEAPON_RECS-1)
                    {
                        pPlayer->weaprecs[pPlayer->weapreccnt++] = vm.pSprite->picnum;
                        VM_CONDITIONAL(vm.pSprite->owner == vm.spriteNum);
                        continue;
                    }
                }
                VM_CONDITIONAL(0);
                continue;

            case concmd_getlastpal:
                insptr++;
                if (vm.pSprite->picnum == TILE_APLAYER)
                    vm.pSprite->pal = g_player[P_GetP(vm.pSprite)].ps->palookup;
                else
                    vm.pSprite->pal = vm.pActor->tempang;
                vm.pActor->tempang = 0;
                continue;

            case concmd_tossweapon:
                insptr++;
                // NOTE: assumes that current actor is TILE_APLAYER
                P_DropWeapon(P_GetP(vm.pSprite));
                continue;

            case concmd_mikesnd:
                insptr++;
                if (EDUKE32_PREDICT_FALSE(((unsigned)vm.pSprite->yvel >= MAXSOUNDS)))
                {
                    CON_ERRPRINTF("invalid sound %d\n", vm.pUSprite->yvel);
                    continue;
                }
                if (!S_CheckSoundPlaying(vm.spriteNum, vm.pSprite->yvel))
                    A_PlaySound(vm.pSprite->yvel, vm.spriteNum, CHAN_VOICE);
                continue;

            case concmd_pkick:
                insptr++;

                if ((g_netServer || ud.multimode > 1) && vm.pSprite->picnum == TILE_APLAYER)
                {
                    if (g_player[otherp].ps->quick_kick == 0)
                        g_player[otherp].ps->quick_kick = 14;
                }
                else if (vm.pSprite->picnum != TILE_APLAYER && pPlayer->quick_kick == 0)
                    pPlayer->quick_kick = 14;
                continue;

            case concmd_sizeto:
                insptr++;

                tw = (*insptr++ - vm.pSprite->xrepeat) << 1;
                vm.pSprite->xrepeat += ksgn(tw);

                if ((vm.pSprite->picnum == TILE_APLAYER && vm.pSprite->yrepeat < 36) || *insptr < vm.pSprite->yrepeat
                    || ((vm.pSprite->yrepeat * (tilesiz[vm.pSprite->picnum].y + 8)) << 2) < (vm.pActor->floorz - vm.pActor->ceilingz))
                {
                    tw = ((*insptr) - vm.pSprite->yrepeat) << 1;
                    if (klabs(tw))
                        vm.pSprite->yrepeat += ksgn(tw);
                }

                insptr++;

                continue;

            case concmd_sizeat:
                insptr++;
                vm.pSprite->xrepeat = (uint8_t)*insptr++;
                vm.pSprite->yrepeat = (uint8_t)*insptr++;
                continue;

            case concmd_shoot:
                insptr++;
                if (EDUKE32_PREDICT_FALSE((unsigned)vm.pSprite->sectnum >= (unsigned)numsectors))
                {
                    CON_ERRPRINTF("invalid sector %d\n", vm.pUSprite->sectnum);
                    continue;
                }
                A_Shoot(vm.spriteNum, *insptr++);
                continue;

            case concmd_ifsoundid:
                insptr++;
                VM_CONDITIONAL((int16_t)*insptr == g_ambientLotag[vm.pSprite->ang]);
                continue;

            case concmd_ifsounddist:
                insptr++;
                if (*insptr == 0)
                {
                    VM_CONDITIONAL(g_ambientHitag[vm.pSprite->ang] > vm.playerDist);
                }
                else if (*insptr == 1)
                {
                    VM_CONDITIONAL(g_ambientHitag[vm.pSprite->ang] < vm.playerDist);
                }
                else
                {
                    VM_CONDITIONAL(0);
                }

                continue;

            case concmd_soundtag:
                insptr++;
                A_PlaySound(g_ambientLotag[vm.pSprite->ang], vm.spriteNum);
                continue;

            case concmd_soundtagonce:
                insptr++;
                if (!S_CheckSoundPlaying(vm.spriteNum, g_ambientLotag[vm.pSprite->ang]))
                    A_PlaySound(g_ambientLotag[vm.pSprite->ang], vm.spriteNum);
                continue;

            case concmd_soundonce:
                if (EDUKE32_PREDICT_FALSE((unsigned)*(++insptr) >= MAXSOUNDS))
                {
                    CON_ERRPRINTF("invalid sound %d\n", (int32_t)*insptr++);
                    continue;
                }

                if (!S_CheckSoundPlaying(vm.spriteNum, *insptr++))
                    A_PlaySound(*(insptr - 1), vm.spriteNum);

                continue;

            case concmd_stopsound:
                if (EDUKE32_PREDICT_FALSE((unsigned)*(++insptr) >= MAXSOUNDS))
                {
                    CON_ERRPRINTF("invalid sound %d\n", (int32_t)*insptr);
                    insptr++;
                    continue;
                }
                if (S_CheckSoundPlaying(vm.spriteNum, *insptr))
                    S_StopSound((int16_t)*insptr);
                insptr++;
                continue;

            case concmd_globalsound:
                if (EDUKE32_PREDICT_FALSE((unsigned)*(++insptr) >= MAXSOUNDS))
                {
                    CON_ERRPRINTF("invalid sound %d\n", (int32_t)*insptr);
                    insptr++;
                    continue;
                }
                if (vm.playerNum == screenpeek || (g_gametypeFlags[ud.coop] & GAMETYPE_COOPSOUND)
#ifdef SPLITSCREEN_MOD_HACKS
                    || (g_fakeMultiMode == 2)
#endif
                    )
                    A_PlaySound(*insptr, g_player[screenpeek].ps->i);
                insptr++;
                continue;

            case concmd_smackbubba:
                insptr++;
                if (!RRRA || vm.pSprite->pal != 105)
                {
                    for (bssize_t TRAVERSE_CONNECT(playerNum))
                        g_player[playerNum].ps->gm = MODE_EOL;
                    if (++ud.level_number > 6)
                        ud.level_number = 0;
                    m_level_number = ud.level_number;
                }
                continue;

#if 0 // RRDH only
            case concmd_deploybias:
                insptr++;
                ghdeploy_bias(vm.spriteNum);
                continue;
#endif

            case concmd_mamaend:
                insptr++;
                g_player[myconnectindex].ps->level_end_timer = 150;
                continue;

            case concmd_ifactorhealthg:
                insptr++;
                VM_CONDITIONAL(vm.pSprite->extra > (int16_t)*insptr);
                continue;

            case concmd_ifactorhealthl:
                insptr++;
                VM_CONDITIONAL(vm.pSprite->extra < (int16_t)*insptr);
                continue;

            case concmd_sound:
                if (EDUKE32_PREDICT_FALSE((unsigned)*(++insptr) >= MAXSOUNDS))
                {
                    CON_ERRPRINTF("invalid sound %d\n", (int32_t)*insptr);
                    insptr++;
                    continue;
                }
                A_PlaySound(*insptr++, vm.spriteNum);
                continue;

            case concmd_tip:
                insptr++;
                pPlayer->tipincs = GAMETICSPERSEC;
                continue;

            case concmd_iftipcow:
                if (g_spriteExtra[vm.spriteNum] == 1)
                {
                    g_spriteExtra[vm.spriteNum]++;
                    VM_CONDITIONAL(1);
                }
                else
                    VM_CONDITIONAL(0);
                continue;

            case concmd_ifhittruck:
                if (g_spriteExtra[vm.spriteNum] == 1)
                {
                    g_spriteExtra[vm.spriteNum]++;
                    VM_CONDITIONAL(1);
                }
                else
                    VM_CONDITIONAL(0);
                continue;

#if 0 // RRDH only
            case concmd_iffindnewspot:
                VM_CONDITIONAL(ghcons_findnewspot(vm.spriteNum));
                continue;

            case concmd_leavedroppings:
                insptr++;
                ghtrax_leavedroppings(vm.spriteNum);
                continue;
#endif

            case concmd_tearitup:
                insptr++;
                for (bssize_t SPRITES_OF_SECT(vm.pSprite->sectnum, spriteNum))
                {
                    if (sprite[spriteNum].picnum == TILE_DESTRUCTO)
                    {
                        actor[spriteNum].picnum = TILE_SHOTSPARK1;
                        actor[spriteNum].extra = 1;
                    }
                }
                continue;

            case concmd_fall:
                insptr++;
                VM_Fall(vm.spriteNum, vm.pSprite);
                continue;

            case concmd_nullop: insptr++; continue;

            case concmd_addammo:
                insptr++;
                {
                    int const weaponNum = *insptr++;
                    int const addAmount = *insptr++;

                    VM_AddAmmo(pPlayer, weaponNum, addAmount);

                    continue;
                }

            case concmd_money:
                insptr++;
                A_SpawnMultiple(vm.spriteNum, TILE_MONEY, *insptr++);
                continue;

            case concmd_mail:
                insptr++;
                A_SpawnMultiple(vm.spriteNum, RR ? TILE_MONEY : TILE_MAIL, *insptr++);
                continue;

            case concmd_sleeptime:
                insptr++;
                vm.pActor->timetosleep = (int16_t)*insptr++;
                continue;

            case concmd_paper:
                insptr++;
                A_SpawnMultiple(vm.spriteNum, RR ? TILE_MONEY : TILE_PAPER, *insptr++);
                continue;

            case concmd_addkills:
                if (DEER)
                {
                    // no op
                    insptr++;
                    insptr++;
                    continue;
                }
                insptr++;
                if (RR)
                {
                    // This check does not exist in Duke Nukem.
                    if ((g_spriteExtra[vm.spriteNum] < 1 || g_spriteExtra[vm.spriteNum] == 128)
                    && (!RR || A_CheckSpriteFlags(vm.spriteNum, SFLAG_KILLCOUNT)))
                        P_AddKills(pPlayer, *insptr);
                }
                else P_AddKills(pPlayer, *insptr);
                insptr++;
                vm.pActor->actorstayput = -1;
                continue;

            case concmd_lotsofglass:
                insptr++;
                A_SpawnGlass(vm.spriteNum, *insptr++);
                continue;

            case concmd_killit:
                insptr++;
                vm.flags |= VM_KILL;
                return;

            case concmd_addweapon:
                insptr++;
                {
                    int const weaponNum = *insptr++;
                    VM_AddWeapon(pPlayer, weaponNum, *insptr++);
                    continue;
                }

            case concmd_debug:
                insptr++;
                buildprint(*insptr++, "\n");
                continue;

            case concmd_endofgame:
                insptr++;
                pPlayer->timebeforeexit  = *insptr++;
                pPlayer->customexitsound = -1;
                ud.eog                   = 1;
                continue;

            case concmd_isdrunk:
                insptr++;
                {
                    pPlayer->drink_amt += *insptr;

                    int newHealth = sprite[pPlayer->i].extra;

                    if (newHealth > 0)
                        newHealth += *insptr;
                    if (newHealth > (pPlayer->max_player_health << 1))
                        newHealth = (pPlayer->max_player_health << 1);
                    if (newHealth < 0)
                        newHealth = 0;

                    if (ud.god == 0)
                    {
                        if (*insptr > 0)
                        {
                            if ((newHealth - *insptr) < (pPlayer->max_player_health >> 2) && newHealth >= (pPlayer->max_player_health >> 2))
                                A_PlaySound(DUKE_GOTHEALTHATLOW, pPlayer->i);
                            pPlayer->last_extra = newHealth;
                        }

                        sprite[pPlayer->i].extra = newHealth;
                    }
                    if (pPlayer->drink_amt > 100)
                        pPlayer->drink_amt = 100;

                    if (sprite[pPlayer->i].extra >= pPlayer->max_player_health)
                    {
                        sprite[pPlayer->i].extra = pPlayer->max_player_health;
                        pPlayer->last_extra = pPlayer->max_player_health;
                    }
                }
                insptr++;
                continue;

            case concmd_strafeleft:
                insptr++;
                {
                    vec3_t const vect = { sintable[(vm.pSprite->ang+1024)&2047]>>10, sintable[(vm.pSprite->ang+512)&2047]>>10, vm.pSprite->zvel };
                    A_MoveSprite(vm.spriteNum, &vect, CLIPMASK0);
                }
                continue;

            case concmd_straferight:
                insptr++;
                {
                    vec3_t const vect = { sintable[(vm.pSprite->ang-0)&2047]>>10, sintable[(vm.pSprite->ang-512)&2047]>>10, vm.pSprite->zvel };
                    A_MoveSprite(vm.spriteNum, &vect, CLIPMASK0);
                }
                continue;

            case concmd_larrybird:
                insptr++;
                pPlayer->pos.z = sector[sprite[pPlayer->i].sectnum].ceilingz;
                sprite[pPlayer->i].z = pPlayer->pos.z;
                continue;
                
#if 0 // RRDH only
            case concmd_leavetrax:
                insptr++;
                ghtrax_leavetrax(vm.spriteNum);
                continue;
#endif

            case concmd_destroyit:
                insptr++;
                {
                    int16_t hitag, lotag, spr, jj, k, nextk;
                    hitag = 0;
                    for (SPRITES_OF_SECT(vm.pSprite->sectnum,k))
                    {
                        if (sprite[k].picnum == TILE_RRTILE63)
                        {
                            lotag = sprite[k].lotag;
                            spr = k;
                            if (sprite[k].hitag)
                                hitag = sprite[k].hitag;
                        }
                    }
                    for (SPRITES_OF(100, jj))
                    {
                        spritetype const *js = &sprite[jj];
                        if (hitag && hitag == js->hitag)
                        {
                            for (SPRITES_OF_SECT(js->sectnum,k))
                            {
                                if (sprite[k].picnum == TILE_DESTRUCTO)
                                {
                                    actor[k].picnum = TILE_SHOTSPARK1;
                                    actor[k].extra = 1;
                                }
                            }
                        }
                        if (sprite[spr].sectnum != js->sectnum && lotag == js->lotag)
                        {
                            int16_t const sectnum = sprite[spr].sectnum;
                            int16_t const wallstart = sector[sectnum].wallptr;
                            int16_t const wallend = wallstart + sector[sectnum].wallnum;
                            int16_t const wallstart2 = sector[js->sectnum].wallptr;
                            //int16_t const wallend2 = wallstart2 + sector[js->sectnum].wallnum;
                            for (bssize_t wi = wallstart, wj = wallstart2; wi < wallend; wi++, wj++)
                            {
                                wall[wi].picnum = wall[wj].picnum;
                                wall[wi].overpicnum = wall[wj].overpicnum;
                                wall[wi].shade = wall[wj].shade;
                                wall[wi].xrepeat = wall[wj].xrepeat;
                                wall[wi].yrepeat = wall[wj].yrepeat;
                                wall[wi].xpanning = wall[wj].xpanning;
                                wall[wi].ypanning = wall[wj].ypanning;
                                if (RRRA && wall[wi].nextwall != -1)
                                {
                                    wall[wi].cstat = 0;
                                    wall[wall[wi].nextwall].cstat = 0;
                                }
                            }
                            sector[sectnum].floorz = sector[js->sectnum].floorz;
                            sector[sectnum].ceilingz = sector[js->sectnum].ceilingz;
                            sector[sectnum].ceilingstat = sector[js->sectnum].ceilingstat;
                            sector[sectnum].floorstat = sector[js->sectnum].floorstat;
                            sector[sectnum].ceilingpicnum = sector[js->sectnum].ceilingpicnum;
                            sector[sectnum].ceilingheinum = sector[js->sectnum].ceilingheinum;
                            sector[sectnum].ceilingshade = sector[js->sectnum].ceilingshade;
                            sector[sectnum].ceilingpal = sector[js->sectnum].ceilingpal;
                            sector[sectnum].ceilingxpanning = sector[js->sectnum].ceilingxpanning;
                            sector[sectnum].ceilingypanning = sector[js->sectnum].ceilingypanning;
                            sector[sectnum].floorpicnum = sector[js->sectnum].floorpicnum;
                            sector[sectnum].floorheinum = sector[js->sectnum].floorheinum;
                            sector[sectnum].floorshade = sector[js->sectnum].floorshade;
                            sector[sectnum].floorpal = sector[js->sectnum].floorpal;
                            sector[sectnum].floorxpanning = sector[js->sectnum].floorxpanning;
                            sector[sectnum].floorypanning = sector[js->sectnum].floorypanning;
                            sector[sectnum].visibility = sector[js->sectnum].visibility;
                            g_sectorExtra[sectnum] = g_sectorExtra[js->sectnum];
                            sector[sectnum].lotag = sector[js->sectnum].lotag;
                            sector[sectnum].hitag = sector[js->sectnum].hitag;
                            sector[sectnum].extra = sector[js->sectnum].extra;
                        }
                    }
                    for (SPRITES_OF_SECT_SAFE(vm.pSprite->sectnum, k, nextk))
                    {
                        switch (DYNAMICTILEMAP(sprite[k].picnum))
                        {
                            case DESTRUCTO__STATICRR:
                            case RRTILE63__STATICRR:
                            case TORNADO__STATICRR:
                            case APLAYER__STATIC:
                            case COOT__STATICRR:
                                break;
                            default:
                                A_DeleteSprite(k);
                                break;
                        }
                    }
                }
                continue;

            case concmd_iseat:
                insptr++;

                {
                    pPlayer->eat_amt += *insptr;
                    if (pPlayer->eat_amt > 100)
                        pPlayer->eat_amt = 100;

                    pPlayer->drink_amt -= *insptr;
                    if (pPlayer->drink_amt < 0)
                        pPlayer->drink_amt = 0;

                    int newHealth = sprite[pPlayer->i].extra;

                    if (vm.pSprite->picnum != TILE_ATOMICHEALTH)
                    {
                        if (newHealth > pPlayer->max_player_health && *insptr > 0)
                        {
                            insptr++;
                            continue;
                        }
                        else
                        {
                            if (newHealth > 0)
                                newHealth += (*insptr)*3;
                            if (newHealth > pPlayer->max_player_health && *insptr > 0)
                                newHealth = pPlayer->max_player_health;
                        }
                    }
                    else
                    {
                        if (newHealth > 0)
                            newHealth += *insptr;
                        if (newHealth > (pPlayer->max_player_health << 1))
                            newHealth = (pPlayer->max_player_health << 1);
                    }

                    if (newHealth < 0)
                        newHealth = 0;

                    if (ud.god == 0)
                    {
                        if (*insptr > 0)
                        {
                            if ((newHealth - *insptr) < (pPlayer->max_player_health >> 2) && newHealth >= (pPlayer->max_player_health >> 2))
                                A_PlaySound(DUKE_GOTHEALTHATLOW, pPlayer->i);
                            pPlayer->last_extra = newHealth;
                        }

                        sprite[pPlayer->i].extra = newHealth;
                    }
                }

                insptr++;
                continue;

            case concmd_addphealth:
                insptr++;

                {
                    if (!RR && pPlayer->newowner >= 0)
                        G_ClearCameraView(pPlayer);

                    int newHealth = sprite[pPlayer->i].extra;

                    if (vm.pSprite->picnum != TILE_ATOMICHEALTH)
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
                        if (newHealth > (pPlayer->max_player_health << 1))
                            newHealth = (pPlayer->max_player_health << 1);
                    }

                    if (newHealth < 0)
                        newHealth = 0;

                    if (ud.god == 0)
                    {
                        if (*insptr > 0)
                        {
                            if ((newHealth - *insptr) < (pPlayer->max_player_health >> 2) && newHealth >= (pPlayer->max_player_health >> 2))
                                A_PlaySound(DUKE_GOTHEALTHATLOW, pPlayer->i);
                            pPlayer->last_extra = newHealth;
                        }

                        sprite[pPlayer->i].extra = newHealth;
                    }
                }

                insptr++;
                continue;

            case concmd_move:
                insptr++;
                AC_COUNT(vm.pData)   = 0;
                AC_MOVE_ID(vm.pData) = *insptr++;
                vm.pSprite->hitag    = *insptr++;
                if (vm.pSprite->hitag & random_angle)
                    vm.pSprite->ang = krand2() & 2047;
                continue;

            case concmd_spawn:
                insptr++;
                if ((unsigned)vm.pSprite->sectnum >= MAXSECTORS)
                {
                    CON_ERRPRINTF("invalid sector %d\n", vm.pUSprite->sectnum);
                    insptr++;
                    continue;
                }
                A_Spawn(vm.spriteNum, *insptr++);
                continue;

            case concmd_ifwasweapon:
            case concmd_ifspawnedby:
                insptr++;
                VM_CONDITIONAL(vm.pActor->picnum == *insptr);
                continue;

            case concmd_ifai:
                insptr++;
                VM_CONDITIONAL(AC_AI_ID(vm.pData) == *insptr);
                continue;

            case concmd_ifaction:
                insptr++;
                VM_CONDITIONAL(AC_ACTION_ID(vm.pData) == *insptr);
                continue;

            case concmd_ifactioncount:
                insptr++;
                VM_CONDITIONAL(AC_ACTION_COUNT(vm.pData) >= *insptr);
                continue;

            case concmd_resetactioncount:
                insptr++;
                AC_ACTION_COUNT(vm.pData) = 0;
                continue;

            case concmd_debris:
                insptr++;
                {
                    int debrisTile = *insptr++;

                    if ((unsigned)vm.pSprite->sectnum < MAXSECTORS)
                        for (native_t cnt = (*insptr) - 1; cnt >= 0; cnt--)
                        {
                            int const tileOffset = ((RR || vm.pSprite->picnum == TILE_BLIMP) && debrisTile == TILE_SCRAP1) ? 0 : (krand2() % 3);

                            int32_t const r1 = krand2(), r2 = krand2(), r3 = krand2(), r4 = krand2(), r5 = krand2(), r6 = krand2(), r7 = krand2(), r8 = krand2();
                            int const spriteNum = A_InsertSprite(vm.pSprite->sectnum, vm.pSprite->x + (r8 & 255) - 128,
                                                                 vm.pSprite->y + (r7 & 255) - 128, vm.pSprite->z - (8 << 8) - (r6 & 8191),
                                                                 debrisTile + tileOffset, vm.pSprite->shade, 32 + (r5 & 15), 32 + (r4 & 15),
                                                                 r3 & 2047, (r2 & 127) + 32, -(r1 & 2047), vm.spriteNum, 5);

                            sprite[spriteNum].yvel = ((RR || vm.pSprite->picnum == TILE_BLIMP) && debrisTile == TILE_SCRAP1) ? g_blimpSpawnItems[cnt % 14] : -1;
                            sprite[spriteNum].pal  = vm.pSprite->pal;
                        }
                    insptr++;
                }
                continue;

            case concmd_count:
                insptr++;
                AC_COUNT(vm.pData) = (int16_t)*insptr++;
                continue;

            case concmd_cstator:
                insptr++;
                vm.pSprite->cstat |= (int16_t)*insptr++;
                continue;

            case concmd_clipdist:
                insptr++;
                vm.pSprite->clipdist = (int16_t)*insptr++;
                continue;

            case concmd_cstat:
                insptr++;
                vm.pSprite->cstat = (int16_t)*insptr++;
                continue;

            case concmd_newpic:
                insptr++;
                vm.pSprite->picnum = (int16_t)*insptr++;
                continue;

            case concmd_ifmove:
                insptr++;
                VM_CONDITIONAL(AC_MOVE_ID(vm.pData) == *insptr);
                continue;

            case concmd_resetplayer:
                insptr++;
                vm.flags = VM_ResetPlayer(vm.playerNum, vm.flags);
                continue;

            case concmd_ifcoop:
                VM_CONDITIONAL(GTFLAGS(GAMETYPE_COOP) || numplayers > 2);
                continue;

            case concmd_ifonmud:
                VM_CONDITIONAL(sector[vm.pSprite->sectnum].floorpicnum == TILE_RRTILE3073
                               && klabs(vm.pSprite->z - sector[vm.pSprite->sectnum].floorz) < ZOFFSET5);
                continue;

            case concmd_ifonwater:
                if (DEER)
                {
                    VM_CONDITIONAL(sector[vm.pSprite->sectnum].hitag == 2003);
                    continue;
                }
                VM_CONDITIONAL(sector[vm.pSprite->sectnum].lotag == ST_1_ABOVE_WATER
                               && klabs(vm.pSprite->z - sector[vm.pSprite->sectnum].floorz) < ZOFFSET5);
                continue;

            case concmd_ifmotofast:
                VM_CONDITIONAL(pPlayer->MotoSpeed > 60);
                continue;

            case concmd_ifonmoto:
                VM_CONDITIONAL(pPlayer->OnMotorcycle == 1);
                continue;

            case concmd_ifonboat:
                VM_CONDITIONAL(pPlayer->OnBoat == 1);
                continue;

            case concmd_ifsizedown:
                vm.pSprite->xrepeat--;
                vm.pSprite->yrepeat--;
                VM_CONDITIONAL(vm.pSprite->xrepeat <= 5);
                continue;

            case concmd_ifwind:
                VM_CONDITIONAL(g_windTime > 0);
                continue;
#if 0 // RRDH only
            case concmd_ifpupwind:
                VM_CONDITIONAL(ghtrax_isplrupwind(vm.spriteNum, vm.playerNum));
                continue;
#endif

            case concmd_ifinwater:
                if (DEER)
                {
                    VM_CONDITIONAL(sector[vm.pSprite->sectnum].hitag == 2003 && klabs(vm.pSprite->z - sector[vm.pSprite->sectnum].floorz) < ZOFFSET5);
                    continue;
                }
                VM_CONDITIONAL(sector[vm.pSprite->sectnum].lotag == ST_2_UNDERWATER);
                continue;

            case concmd_ifcount:
                insptr++;
                VM_CONDITIONAL(AC_COUNT(vm.pData) >= *insptr);
                continue;

            case concmd_ifactor:
                insptr++;
                VM_CONDITIONAL(vm.pSprite->picnum == *insptr);
                continue;

            case concmd_resetcount:
                insptr++;
                AC_COUNT(vm.pData) = 0;
                continue;

            case concmd_addinventory:
                insptr += 2;

                VM_AddInventory(pPlayer, *(insptr - 1), *insptr);

                insptr++;
                continue;

            case concmd_hitradius:
                A_RadiusDamage(vm.spriteNum, *(insptr + 1), *(insptr + 2), *(insptr + 3), *(insptr + 4), *(insptr + 5));
                insptr += 6;
                continue;

            case concmd_ifp:
            {
                int const moveFlags  = *(++insptr);
                int       nResult    = 0;
                int const playerXVel = sprite[pPlayer->i].xvel;
                int const syncBits   = g_player[vm.playerNum].input->bits;

                if (((moveFlags & pducking) && pPlayer->on_ground && (TEST_SYNC_KEY(syncBits, SK_CROUCH) ^ vm.pPlayer->crouch_toggle))
                    || ((moveFlags & pfalling) && pPlayer->jumping_counter == 0 && !pPlayer->on_ground && pPlayer->vel.z > 2048)
                    || ((moveFlags & pjumping) && pPlayer->jumping_counter > 348)
                    || ((moveFlags & pstanding) && playerXVel >= 0 && playerXVel < 8)
                    || ((moveFlags & pwalking) && playerXVel >= 8 && !TEST_SYNC_KEY(syncBits, SK_RUN))
                    || ((moveFlags & prunning) && playerXVel >= 8 && TEST_SYNC_KEY(syncBits, SK_RUN))
                    || ((moveFlags & phigher) && pPlayer->pos.z < (vm.pSprite->z - (48 << 8)))
                    || ((moveFlags & pwalkingback) && playerXVel <= -8 && !TEST_SYNC_KEY(syncBits, SK_RUN))
                    || ((moveFlags & prunningback) && playerXVel <= -8 && TEST_SYNC_KEY(syncBits, SK_RUN))
                    || ((moveFlags & pkicking)
                        && (DEER ? ghsound_pfiredgunnear(vm.pSprite, vm.playerNum) :(pPlayer->quick_kick > 0
                            || (pPlayer->curr_weapon == KNEE_WEAPON && pPlayer->kickback_pic > 0))))
                    || ((moveFlags & pshrunk) && (DEER ? pPlayer->dhat60f && !sub_535EC() : sprite[pPlayer->i].xrepeat < (RR ? 8 : 32)))
                    || ((moveFlags & pjetpack) && pPlayer->jetpack_on)
                    || ((moveFlags & ponsteroids) && (DEER ? ghsound_pmadesound(vm.pSprite, vm.playerNum) :
                        pPlayer->inv_amount[GET_STEROIDS] > 0 && pPlayer->inv_amount[GET_STEROIDS] < 400))
                    || ((moveFlags & ponground) && (DEER ? ghsound_pmadecall(vm.pSprite, vm.playerNum) : pPlayer->on_ground))
                    || ((moveFlags & palive) && sprite[pPlayer->i].xrepeat > (RR ? 8 : 32) && sprite[pPlayer->i].extra > 0 && pPlayer->timebeforeexit == 0)
                    || ((moveFlags & pdead) && sprite[pPlayer->i].extra <= 0))
                    nResult = 1;
                else if ((moveFlags & pfacing))
                {
                    nResult
                    = (vm.pSprite->picnum == TILE_APLAYER && (g_netServer || ud.multimode > 1))
                      ? G_GetAngleDelta(fix16_to_int(g_player[otherp].ps->q16ang),
                                        getangle(pPlayer->pos.x - g_player[otherp].ps->pos.x, pPlayer->pos.y - g_player[otherp].ps->pos.y))
                      : G_GetAngleDelta(fix16_to_int(pPlayer->q16ang), getangle(vm.pSprite->x - pPlayer->pos.x, vm.pSprite->y - pPlayer->pos.y));

                    nResult = (nResult > -128 && nResult < 128);
                }
                VM_CONDITIONAL(nResult);
            }
                continue;

            case concmd_ifstrength:
                insptr++;
                VM_CONDITIONAL(vm.pSprite->extra <= *insptr);
                continue;

            case concmd_guts:
                A_DoGuts(vm.spriteNum, *(insptr + 1), *(insptr + 2));
                insptr += 3;
                continue;

            case concmd_slapplayer:
                insptr++;
                P_ForceAngle(pPlayer);
                pPlayer->vel.x -= sintable[(fix16_to_int(pPlayer->q16ang)+512)&2047]<<7;
                pPlayer->vel.y -= sintable[fix16_to_int(pPlayer->q16ang)&2047]<<7;
                continue;

            case concmd_wackplayer:
                insptr++;
                if (RR)
                {
                    pPlayer->vel.x -= sintable[(fix16_to_int(pPlayer->q16ang)+512)&2047]<<7;
                    pPlayer->vel.y -= sintable[fix16_to_int(pPlayer->q16ang)&2047]<<7;
                    pPlayer->jumping_counter = 767;
                    pPlayer->jumping_toggle = 1;
                }
                else
                    P_ForceAngle(pPlayer);
                continue;

            case concmd_ifgapzl:
                insptr++;
                VM_CONDITIONAL(((vm.pActor->floorz - vm.pActor->ceilingz) >> 8) < *insptr);
                continue;

            case concmd_ifhitspace: VM_CONDITIONAL(TEST_SYNC_KEY(g_player[vm.playerNum].input->bits, SK_OPEN)); continue;

            case concmd_ifoutside:
                if (DEER)
                {
                    VM_CONDITIONAL(sector[vm.pSprite->sectnum].hitag = 2000);
                    continue;
                }
                VM_CONDITIONAL(sector[vm.pSprite->sectnum].ceilingstat & 1);
                continue;

            case concmd_ifmultiplayer: VM_CONDITIONAL((g_netServer || g_netClient || ud.multimode > 1)); continue;

            case concmd_operate:
                insptr++;
                if (sector[vm.pSprite->sectnum].lotag == 0)
                {
                    int16_t foundSect, foundWall, foundSprite;
                    int32_t foundDist;

                    neartag(vm.pSprite->x, vm.pSprite->y, vm.pSprite->z - ZOFFSET5, vm.pSprite->sectnum, vm.pSprite->ang, &foundSect, &foundWall,
                            &foundSprite, &foundDist, 768, 4 + 1, NULL);

                    if (foundSect >= 0 && isanearoperator(sector[foundSect].lotag))
                        if ((sector[foundSect].lotag & 0xff) == ST_23_SWINGING_DOOR || sector[foundSect].floorz == sector[foundSect].ceilingz)
                            if ((sector[foundSect].lotag & (16384u | 32768u)) == 0)
                            {
                                int32_t j;

                                for (SPRITES_OF_SECT(foundSect, j))
                                    if (sprite[j].picnum == TILE_ACTIVATOR)
                                        break;

                                if (j == -1)
                                    operatesectors(foundSect, vm.spriteNum);
                            }
                }
                continue;

            case concmd_ifinspace: VM_CONDITIONAL(G_CheckForSpaceCeiling(vm.pSprite->sectnum)); continue;

            case concmd_spritepal:
                insptr++;
                if (vm.pSprite->picnum != TILE_APLAYER)
                    vm.pActor->tempang = vm.pSprite->pal;
                vm.pSprite->pal        = *insptr++;
                continue;

            case concmd_cactor:
                insptr++;
                vm.pSprite->picnum = *insptr++;
                continue;

            case concmd_ifbulletnear: VM_CONDITIONAL(A_Dodge(vm.pSprite) == 1); continue;

            case concmd_ifrespawn:
                if (A_CheckEnemySprite(vm.pSprite))
                    VM_CONDITIONAL(ud.respawn_monsters)
                else if (A_CheckInventorySprite(vm.pSprite))
                    VM_CONDITIONAL(ud.respawn_inventory)
                else
                    VM_CONDITIONAL(ud.respawn_items)
                continue;

            case concmd_iffloordistl:
                insptr++;
                VM_CONDITIONAL((vm.pActor->floorz - vm.pSprite->z) <= ((*insptr) << 8));
                continue;

            case concmd_ifceilingdistl:
                insptr++;
                VM_CONDITIONAL((vm.pSprite->z - vm.pActor->ceilingz) <= ((*insptr) << 8));
                continue;

            case concmd_palfrom:
                insptr++;
                if (EDUKE32_PREDICT_FALSE((unsigned)vm.playerNum >= (unsigned)g_mostConcurrentPlayers))
                {
                    CON_ERRPRINTF("invalid player %d\n", vm.playerNum);
                    insptr += 4;
                }
                else
                {
                    palette_t const pal = { (uint8_t) * (insptr + 1), (uint8_t) * (insptr + 2), (uint8_t) * (insptr + 3), (uint8_t) * (insptr) };
                    insptr += 4;
                    P_PalFrom(pPlayer, pal.f, pal.r, pal.g, pal.b);
                }
                continue;

            case concmd_ifphealthl:
                insptr++;
                VM_CONDITIONAL(sprite[pPlayer->i].extra < *insptr);
                continue;

            case concmd_ifpinventory:
                insptr++;

                switch (*insptr++)
                {
                    case GET_STEROIDS:
                    case GET_SCUBA:
                    case GET_HOLODUKE:
                    case GET_HEATS:
                    case GET_FIRSTAID:
                    case GET_BOOTS:
                    case GET_JETPACK: tw = (pPlayer->inv_amount[*(insptr - 1)] != *insptr); break;

                    case GET_SHIELD:
                        tw = (pPlayer->inv_amount[GET_SHIELD] != pPlayer->max_player_health); break;
                    case GET_ACCESS:
                        if (RR)
                        {
                            switch (vm.pSprite->lotag)
                            {
                                case 100: tw = pPlayer->keys[1]; break;
                                case 101: tw = pPlayer->keys[2]; break;
                                case 102: tw = pPlayer->keys[3]; break;
                                case 103: tw = pPlayer->keys[4]; break;
                            }
                        }
                        else
                        {
                            switch (vm.pSprite->pal)
                            {
                                case 0: tw  = (pPlayer->got_access & 1); break;
                                case 21: tw = (pPlayer->got_access & 2); break;
                                case 23: tw = (pPlayer->got_access & 4); break;
                            }
                        }
                        break;
                    default: tw = 0; CON_ERRPRINTF("invalid inventory item %d\n", (int32_t) * (insptr - 1));
                }

                VM_CONDITIONAL(tw);
                continue;

            case concmd_pstomp:
                insptr++;
                if (pPlayer->knee_incs == 0 && sprite[pPlayer->i].xrepeat >= (RR ? 9 : 40))
                    if (cansee(vm.pSprite->x, vm.pSprite->y, vm.pSprite->z - ZOFFSET6, vm.pSprite->sectnum, pPlayer->pos.x, pPlayer->pos.y,
                               pPlayer->pos.z + ZOFFSET2, sprite[pPlayer->i].sectnum))
                    {
                        if (pPlayer->weapon_pos == 0)
                            pPlayer->weapon_pos = -1;

                        pPlayer->actorsqu  = vm.spriteNum;
                        pPlayer->knee_incs = 1;
                    }
                continue;

            case concmd_ifawayfromwall:
            {
                int16_t otherSectnum = vm.pSprite->sectnum;
                tw                   = 0;

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

            case concmd_quote:
                insptr++;

                if (EDUKE32_PREDICT_FALSE((unsigned)(*insptr) >= MAXQUOTES))
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

            case concmd_ifinouterspace: VM_CONDITIONAL(G_CheckForSpaceFloor(vm.pSprite->sectnum)); continue;

            case concmd_ifnotmoving: VM_CONDITIONAL((vm.pActor->movflag & 49152) > 16384); continue;

            case concmd_respawnhitag:
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
                    case PODFEM1__STATIC:
                        if (RR) break;
                        fallthrough__;
                    case FEM10__STATIC:
                    case NAKED1__STATIC:
                    case STATUE__STATIC:
                        if (vm.pSprite->yvel)
                            operaterespawns(vm.pSprite->yvel);
                        break;
                    default:
                        if (vm.pSprite->hitag >= 0)
                            operaterespawns(vm.pSprite->hitag);
                        break;
                }
                continue;

            case concmd_ifspritepal:
                insptr++;
                VM_CONDITIONAL(vm.pSprite->pal == *insptr);
                continue;

            case concmd_ifangdiffl:
                insptr++;
                tw = klabs(G_GetAngleDelta(fix16_to_int(pPlayer->q16ang), vm.pSprite->ang));
                VM_CONDITIONAL(tw <= *insptr);
                continue;

            case concmd_ifnosounds: VM_CONDITIONAL(!A_CheckAnySoundPlaying(vm.spriteNum)); continue;
                

            case concmd_ifvarg:
                insptr++;
                tw = Gv_GetVar(*insptr++);
                VM_CONDITIONAL(tw > *insptr);
                continue;

            case concmd_ifvarl:
                insptr++;
                tw = Gv_GetVar(*insptr++);
                VM_CONDITIONAL(tw < *insptr);
                continue;

            case concmd_setvarvar:
                insptr++;
                {
                    tw = *insptr++;
                    int const nValue = Gv_GetVar(*insptr++);

                    if ((aGameVars[tw].flags & (GAMEVAR_USER_MASK | GAMEVAR_PTR_MASK)) == 0)
                        aGameVars[tw].global = nValue;
                    else
                        Gv_SetVar(tw, nValue);
                }
                continue;

            case concmd_setvar:
                Gv_SetVar(insptr[1], insptr[2]);
                insptr += 3;
                continue;

            case concmd_addvarvar:
                insptr++;
                tw = *insptr++;
                Gv_AddVar(tw, Gv_GetVar(*insptr++));
                continue;

            case concmd_addvar:
                Gv_AddVar(insptr[1], insptr[2]);
                insptr += 3;
                continue;

            case concmd_ifvarvarl:
                insptr++;
                tw = Gv_GetVar(*insptr++);
                tw = (tw < Gv_GetVar(*insptr++));
                insptr--;
                VM_CONDITIONAL(tw);
                continue;

            case concmd_ifvarvarg:
                insptr++;
                tw = Gv_GetVar(*insptr++);
                tw = (tw > Gv_GetVar(*insptr++));
                insptr--;
                VM_CONDITIONAL(tw);
                continue;

            case concmd_addlogvar:
                insptr++;
                {
                    int32_t m = 1;
                    char    szBuf[256];
                    int32_t lVarID = *insptr;

                    if ((lVarID >= g_gameVarCount) || lVarID < 0)
                    {
                        if (*insptr == MAXGAMEVARS)  // addlogvar for a constant?  Har.
                            insptr++;
                        else if (EDUKE32_PREDICT_TRUE(*insptr & GV_FLAG_NEGATIVE))
                        {
                            m = -m;
                            lVarID ^= GV_FLAG_NEGATIVE;
                        }
                        else
                        {
                            // invalid varID
                            CON_ERRPRINTF("invalid variable\n");
                            continue;
                        }
                    }
                    Bsprintf(tempbuf, "CONLOGVAR: L=%d %s ", VM_DECODE_LINE_NUMBER(g_tw), aGameVars[lVarID].szLabel);

                    if (aGameVars[lVarID].flags & GAMEVAR_READONLY)
                    {
                        Bsprintf(szBuf, " (read-only)");
                        Bstrcat(tempbuf, szBuf);
                    }
                    if (aGameVars[lVarID].flags & GAMEVAR_PERPLAYER)
                    {
                        Bsprintf(szBuf, " (Per Player. Player=%d)", vm.playerNum);
                    }
                    else if (aGameVars[lVarID].flags & GAMEVAR_PERACTOR)
                    {
                        Bsprintf(szBuf, " (Per Actor. Actor=%d)", vm.spriteNum);
                    }
                    else
                    {
                        Bsprintf(szBuf, " (Global)");
                    }
                    Bstrcat(tempbuf, szBuf);
                    Bsprintf(szBuf, " =%d\n", Gv_GetVar(lVarID) * m);
                    Bstrcat(tempbuf, szBuf);
                    Printf(TEXTCOLOR_GREEN "%s", tempbuf);
                    insptr++;
                    continue;
                }

            case concmd_ifvare:
                insptr++;
                tw = Gv_GetVar(*insptr++);
                VM_CONDITIONAL(tw == *insptr);
                continue;

            case concmd_ifvarvare:
                insptr++;
                tw = Gv_GetVar(*insptr++);
                tw = (tw == Gv_GetVar(*insptr++));
                insptr--;
                VM_CONDITIONAL(tw);
                continue;

            default:  // you aren't supposed to be here!
                if (RR && ud.recstat == 2)
                {
                    vm.flags |= VM_KILL;
                    return;
                }
                debug_break();
                VM_ScriptInfo(insptr, 64);
                G_GameExit("An error has occurred in the " GAMENAME " virtual machine.\n\n");
#if 0
                           "If you are an end user, please e-mail the file " GAMENAMELOWERCASE ".log\n"
                           "along with links to any mods you're using to development@voidpoint.com.\n\n"
                           "If you are a developer, please attach all of your script files\n"
                           "along with instructions on how to reproduce this error.\n\n"
                           "Thank you!");
#endif
                break;
        }
    }
}

void VM_UpdateAnim(int spriteNum, int32_t *pData)
{
    size_t const actionofs = AC_ACTION_ID(pData);
    intptr_t const *actionptr = (actionofs != 0 && actionofs + (ACTION_PARAM_COUNT-1) < (unsigned) g_scriptSize) ? &apScript[actionofs] : NULL;

    if (actionptr != NULL)
    {
        int const action_frames = actionptr[ACTION_NUMFRAMES];
        int const action_incval = actionptr[ACTION_INCVAL];
        int const action_delay  = actionptr[ACTION_DELAY];
        auto actionticsptr = &AC_ACTIONTICS(&sprite[spriteNum], &actor[spriteNum]);
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
    if (!G_HaveActor(sprite[spriteNum].picnum)) return;

    vmstate_t tempvm
    = { spriteNum, playerNum, playerDist, 0, &sprite[spriteNum], &actor[spriteNum].t_data[0], g_player[playerNum].ps, &actor[spriteNum] };
    vm = tempvm;

/*
    if (g_netClient && A_CheckSpriteFlags(spriteNum, SFLAG_NULL))
    {
        A_DeleteSprite(spriteNum);
        return;
    }
*/

    //if (g_netClient) // [75] The server should not overwrite its own randomseed
    //    randomseed = ticrandomseed;

    if (EDUKE32_PREDICT_FALSE((unsigned)vm.pSprite->sectnum >= MAXSECTORS))
    {
        if (A_CheckEnemySprite(vm.pSprite))
            P_AddKills(vm.pPlayer, 1);

        A_DeleteSprite(vm.spriteNum);
        return;
    }

    VM_UpdateAnim(vm.spriteNum, vm.pData);

    double t = timerGetHiTicks();
    int const picnum = vm.pSprite->picnum;
    insptr = 4 + (g_tile[vm.pSprite->picnum].execPtr);
    VM_Execute(1);
    insptr = NULL;

    t = timerGetHiTicks()-t;
    g_actorTotalMs[picnum] += t;
    g_actorMinMs[picnum] = min(g_actorMinMs[picnum], t);
    g_actorMaxMs[picnum] = max(g_actorMaxMs[picnum], t);
    g_actorCalls[picnum]++;

    if (vm.flags & VM_KILL)
    {
        VM_DeleteSprite(spriteNum, playerNum);
        return;
    }

    VM_Move();

    if (DEER || vm.pSprite->statnum != STAT_ACTOR)
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
        goto safe_delete;
    }

    if (A_CheckEnemySprite(vm.pSprite))
    {
        if (vm.pSprite->xrepeat > 60 || (ud.respawn_monsters == 1 && vm.pSprite->extra <= 0))
            goto safe_delete;
    }
    else if (EDUKE32_PREDICT_FALSE(ud.respawn_items == 1 && (vm.pSprite->cstat & 32768)))
        goto safe_delete;

    if (A_CheckSpriteFlags(vm.spriteNum, SFLAG_USEACTIVATOR) && sector[vm.pSprite->sectnum].lotag & 16384)
        changespritestat(vm.spriteNum, STAT_ZOMBIEACTOR);
    else if (vm.pActor->timetosleep > 1)
        vm.pActor->timetosleep--;
    else if (vm.pActor->timetosleep == 1)
        changespritestat(vm.spriteNum, STAT_ZOMBIEACTOR);

safe_delete:
    if (vm.flags & VM_SAFEDELETE)
        A_DeleteSprite(spriteNum);
}
END_DUKE_NS
