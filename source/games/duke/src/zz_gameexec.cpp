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
        if ((xxx) || ((insptr = apScript + *(insptr + 1)) && (((*insptr) & VM_INSTMASK) == concmd_else)))                \
        {                                                                                                              \
            insptr += 2;                                                                                               \
            VM_Execute(0);                                                                                             \
        }                                                                                                              \
    }

void VM_ScriptInfo(intptr_t const *ptr, int range)
{
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

intptr_t apScriptGameEvent[EVENT_NUMEVENTS];
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

    auto &globalReturn = aGameVars[g_returnVarID].lValue;

    struct
    {
        vmstate_t vm;
        intptr_t globalReturn;
        int eventNum;
        intptr_t const *insptr;
    } const saved = { vm, globalReturn, g_currentEvent, insptr };

    vm = newVMstate;
    g_currentEvent = eventNum;
    insptr = apScript + apScriptGameEvent[eventNum];
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
int furthestcanseepoint(int i, spritetype* ts, int* dax, int* day);


static void VM_GetZRange(int const spriteNum, int32_t * const ceilhit, int32_t * const florhit, int const wallDist)
{
    uspritetype *const pSprite = (uspritetype *)&sprite[spriteNum];
    vec3_t const tempVect = {
        pSprite->x, pSprite->y, pSprite->z - ZOFFSET
    };
    getzrange(&tempVect, pSprite->sectnum, &actor[spriteNum].ceilingz, ceilhit, &actor[spriteNum].floorz, florhit, wallDist, CLIPMASK0);
}

static inline void VM_AddAngle(int const shift, int const goalAng)
{
    int angDiff = getincangle(vm.pSprite->ang, goalAng) >> shift;

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
void move_d(int g_i, int g_p, int g_x);
void move_r(int g_i, int g_p, int g_x);

GAMEEXEC_STATIC void VM_Move(void)
{
    if (isRR()) move_r(vm.spriteNum, vm.playerNum, vm.playerDist); else move_d(vm.spriteNum, vm.playerNum, vm.playerDist);
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
    else if (pPlayer->ammo_amount[weaponNum] >= max_ammo_amount[weaponNum])
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

    if (pPlayer->ammo_amount[weaponNum] >= max_ammo_amount[weaponNum])
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
        shield_amount = min(shield_amount + nAmount, max_player_health);
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

    if (sector[pSprite->sectnum].lotag == ST_2_UNDERWATER || EDUKE32_PREDICT_FALSE(fi.ceilingspace(pSprite->sectnum)))
        spriteGravity = g_spriteGravity/6;
    else if (EDUKE32_PREDICT_FALSE(fi.floorspace(pSprite->sectnum)))
        spriteGravity = 0;

    if (actor[spriteNum].cgg <= 0 || (sector[pSprite->sectnum].floorstat&2))
    {
        getglobalz(spriteNum);
        actor[spriteNum].cgg = 6;
    }
    else actor[spriteNum].cgg--;

    if (pSprite->z < actor[spriteNum].floorz-ZOFFSET)
    {
        // Free fi.fall.
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
            insptr = apScript + *(insptr + 1);
            continue;
        }
        else if (tw == concmd_state)
        {
            intptr_t const *const tempscrptr = insptr + 2;
            insptr                           = apScript + *(insptr + 1);
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

                    if ((tw = hitasprite(vm.spriteNum, &temphit)) == (1 << 30))
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
        tw = hitasprite(vm.spriteNum, &temphit);                                                                                               \
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
                    if (furthestcanseepoint(vm.spriteNum, (spritetype*)pSprite, &vm.pActor->lastv.x, &vm.pActor->lastv.y) == -1)
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
                    VM_CONDITIONAL(fi.ifhitbyweapon(vm.spriteNum) >= 0);
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
                switch (++fakebubba_spawn)
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
                if (mamaspawn_count)
                {
                    mamaspawn_count--;
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
                if (banjosound == 0)
                {
                    switch (krand2()&3)
                    {
                    case 3:
                        banjosound = 262;
                        break;
                    case 0:
                        banjosound = 272;
                        break;
                    default:
                        banjosound = 273;
                        break;
                    }
                    A_PlaySound(banjosound, vm.spriteNum, CHAN_WEAPON);
                }
                else if (!S_CheckSoundPlaying(vm.spriteNum, banjosound))
                    A_PlaySound(banjosound, vm.spriteNum, CHAN_WEAPON);
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
                checkweapons(&ps[P_GetP(vm.pSprite)]);
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
                VM_CONDITIONAL((int16_t)*insptr == ambientlotag[vm.pSprite->ang]);
                continue;

            case concmd_ifsounddist:
                insptr++;
                if (*insptr == 0)
                {
                    VM_CONDITIONAL(ambienthitag[vm.pSprite->ang] > vm.playerDist);
                }
                else if (*insptr == 1)
                {
                    VM_CONDITIONAL(ambienthitag[vm.pSprite->ang] < vm.playerDist);
                }
                else
                {
                    VM_CONDITIONAL(0);
                }

                continue;

            case concmd_soundtag:
                insptr++;
                A_PlaySound(ambientlotag[vm.pSprite->ang], vm.spriteNum);
                continue;

            case concmd_soundtagonce:
                insptr++;
                if (!S_CheckSoundPlaying(vm.spriteNum, ambientlotag[vm.pSprite->ang]))
                    A_PlaySound(ambientlotag[vm.pSprite->ang], vm.spriteNum);
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
                g_player[myconnectindex].ps->MamaEnd = 150;
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
                spriteglass(vm.spriteNum, *insptr++);
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
                    if (newHealth > (max_player_health << 1))
                        newHealth = (max_player_health << 1);
                    if (newHealth < 0)
                        newHealth = 0;

                    if (ud.god == 0)
                    {
                        if (*insptr > 0)
                        {
                            if ((newHealth - *insptr) < (max_player_health >> 2) && newHealth >= (max_player_health >> 2))
                                A_PlaySound(DUKE_GOTHEALTHATLOW, pPlayer->i);
                            pPlayer->last_extra = newHealth;
                        }

                        sprite[pPlayer->i].extra = newHealth;
                    }
                    if (pPlayer->drink_amt > 100)
                        pPlayer->drink_amt = 100;

                    if (sprite[pPlayer->i].extra >= max_player_health)
                    {
                        sprite[pPlayer->i].extra = max_player_health;
                        pPlayer->last_extra = max_player_health;
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
                    pPlayer->eat += *insptr;
                    if (pPlayer->eat > 100)
                        pPlayer->eat = 100;

                    pPlayer->drink_amt -= *insptr;
                    if (pPlayer->drink_amt < 0)
                        pPlayer->drink_amt = 0;

                    int newHealth = sprite[pPlayer->i].extra;

                    if (vm.pSprite->picnum != TILE_ATOMICHEALTH)
                    {
                        if (newHealth > max_player_health && *insptr > 0)
                        {
                            insptr++;
                            continue;
                        }
                        else
                        {
                            if (newHealth > 0)
                                newHealth += (*insptr)*3;
                            if (newHealth > max_player_health && *insptr > 0)
                                newHealth = max_player_health;
                        }
                    }
                    else
                    {
                        if (newHealth > 0)
                            newHealth += *insptr;
                        if (newHealth > (max_player_health << 1))
                            newHealth = (max_player_health << 1);
                    }

                    if (newHealth < 0)
                        newHealth = 0;

                    if (ud.god == 0)
                    {
                        if (*insptr > 0)
                        {
                            if ((newHealth - *insptr) < (max_player_health >> 2) && newHealth >= (max_player_health >> 2))
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
                        if (newHealth > max_player_health && *insptr > 0)
                        {
                            insptr++;
                            continue;
                        }
                        else
                        {
                            if (newHealth > 0)
                                newHealth += *insptr;
                            if (newHealth > max_player_health && *insptr > 0)
                                newHealth = max_player_health;
                        }
                    }
                    else
                    {
                        if (newHealth > 0)
                            newHealth += *insptr;
                        if (newHealth > (max_player_health << 1))
                            newHealth = (max_player_health << 1);
                    }

                    if (newHealth < 0)
                        newHealth = 0;

                    if (ud.god == 0)
                    {
                        if (*insptr > 0)
                        {
                            if ((newHealth - *insptr) < (max_player_health >> 2) && newHealth >= (max_player_health >> 2))
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

                            sprite[spriteNum].yvel = ((RR || vm.pSprite->picnum == TILE_BLIMP) && debrisTile == TILE_SCRAP1) ? weaponsandammosprites[cnt % 14] : -1;
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
                fi.hitradius(vm.spriteNum, *(insptr + 1), *(insptr + 2), *(insptr + 3), *(insptr + 4), *(insptr + 5));
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
                      ? getincangle(fix16_to_int(g_player[otherp].ps->q16ang),
                                        getangle(pPlayer->pos.x - g_player[otherp].ps->pos.x, pPlayer->pos.y - g_player[otherp].ps->pos.y))
                      : getincangle(fix16_to_int(pPlayer->q16ang), getangle(vm.pSprite->x - pPlayer->pos.x, vm.pSprite->y - pPlayer->pos.y));

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

            case concmd_ifinspace: VM_CONDITIONAL(fi.ceilingspace(vm.pSprite->sectnum)); continue;

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

            case concmd_ifbulletnear: VM_CONDITIONAL(dodge(vm.pSprite) == 1); continue;

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
                        tw = (pPlayer->inv_amount[GET_SHIELD] != max_player_health); break;
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

            case concmd_ifinouterspace: VM_CONDITIONAL(fi.floorspace(vm.pSprite->sectnum)); continue;

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
                            fi.operaterespawns(vm.pSprite->yvel);
                        break;
                    default:
                        if (vm.pSprite->hitag >= 0)
                            fi.operaterespawns(vm.pSprite->hitag);
                        break;
                }
                continue;

            case concmd_ifspritepal:
                insptr++;
                VM_CONDITIONAL(vm.pSprite->pal == *insptr);
                continue;

            case concmd_ifangdiffl:
                insptr++;
                tw = klabs(getincangle(fix16_to_int(pPlayer->q16ang), vm.pSprite->ang));
                VM_CONDITIONAL(tw <= *insptr);
                continue;

            case concmd_ifnosounds: VM_CONDITIONAL(!A_CheckAnySoundPlaying(vm.spriteNum)); continue;
                

            case concmd_ifvarg:
                insptr++;
                tw = GetGameVarID(*insptr++, vm.spriteNum, vm.playerNum);
                VM_CONDITIONAL(tw > *insptr);
                continue;

            case concmd_ifvarl:
                insptr++;
                tw = GetGameVarID(*insptr++, vm.spriteNum, vm.playerNum);
                VM_CONDITIONAL(tw < *insptr);
                continue;

            case concmd_setvarvar:
                insptr++;
                {
                    tw = *insptr++;
                    int const nValue = GetGameVarID(*insptr++, vm.spriteNum, vm.playerNum);
                    SetGameVarID(tw, nValue, vm.spriteNum, vm.playerNum);
                }
                continue;

            case concmd_setvar:
                SetGameVarID(insptr[1], insptr[2], vm.spriteNum, vm.playerNum);
                insptr += 3;
                continue;

            case concmd_addvarvar:
                insptr++;
                tw = *insptr++;
                SetGameVarID(tw, GetGameVarID(tw, vm.spriteNum, vm.playerNum) + GetGameVarID(*insptr++, vm.spriteNum, vm.playerNum), vm.spriteNum, vm.playerNum);
                continue;

            case concmd_addvar:
                SetGameVarID(insptr[1], GetGameVarID(insptr[1], vm.spriteNum, vm.playerNum) + insptr[2], vm.spriteNum, vm.playerNum);
                insptr += 3;
                continue;

            case concmd_ifvarvarl:
                insptr++;
                tw = GetGameVarID(*insptr++, vm.spriteNum, vm.playerNum);
                tw = (tw < GetGameVarID(*insptr++, vm.spriteNum, vm.playerNum));
                insptr--;
                VM_CONDITIONAL(tw);
                continue;

            case concmd_ifvarvarg:
                insptr++;
                tw = GetGameVarID(*insptr++, vm.spriteNum, vm.playerNum);
                tw = (tw > GetGameVarID(*insptr++, vm.spriteNum, vm.playerNum));
                insptr--;
                VM_CONDITIONAL(tw);
                continue;

            case concmd_addlogvar:
                insptr++;
                continue;

            case concmd_ifvare:
                insptr++;
                tw = GetGameVarID(*insptr++, vm.spriteNum, vm.playerNum);
                VM_CONDITIONAL(tw == *insptr);
                continue;

            case concmd_ifvarvare:
                insptr++;
                tw = GetGameVarID(*insptr++, vm.spriteNum, vm.playerNum);
                tw = (tw == GetGameVarID(*insptr++, vm.spriteNum, vm.playerNum));
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
