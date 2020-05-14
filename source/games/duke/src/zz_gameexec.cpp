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

void VM_Execute(native_t loop);
int parse();

#define VM_CONDITIONAL(xxx)                                                                                            \
    {                                                                                                                  \
        if ((xxx) || ((insptr = apScript + *(insptr + 1)) && (((*insptr) & VM_INSTMASK) == concmd_else)))                \
        {                                                                                                              \
            insptr += 2;                                                                                               \
            VM_Execute(0);                                                                                             \
        }                                                                                                              \
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

void forceplayerangle(DukePlayer_t *pPlayer)
{
    int const nAngle = 128-(krand2()&255);

    pPlayer->q16horiz           += F16(64);
    pPlayer->return_to_center = 9;
    pPlayer->rotscrnang       = nAngle >> 1;
    pPlayer->look_ang         = pPlayer->rotscrnang;
}

// wow, this function sucks
int furthestcanseepoint(int i, spritetype* ts, int* dax, int* day);


////////// TROR get*zofslope //////////
// These rather belong into the engine.



////////////////////

static int32_t A_GetWaterZOffset(int spritenum);

GAMEEXEC_STATIC void VM_Move(void)
{
    fi.move(vm.spriteNum, vm.playerNum, vm.playerDist);
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
        int16_t & shieldamount = pPlayer->inv_amount[GET_SHIELD];
        shieldamount = min(shieldamount + nAmount, max_player_health);
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


extern uint8_t killit_flag;

void VM_Execute(native_t loop)
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


        switch (tw)
        {


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


            case concmd_addammo:
                insptr++;
                {
                    int const weaponNum = *insptr++;
                    int const addAmount = *insptr++;

                    VM_AddAmmo(pPlayer, weaponNum, addAmount);

                    continue;
                }

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

            case concmd_soundonce:
            case concmd_isdrunk:
            case concmd_strafeleft:
            case concmd_straferight:
            case concmd_larrybird:
            case concmd_destroyit:
            case concmd_iseat:
            case concmd_newpic:
            case concmd_ifonmud:
            case concmd_ifonwater:
            case concmd_ifmotofast:
            case concmd_ifonmoto:
            case concmd_ifonboat:
            case concmd_ifsizedown:
            case concmd_ifwind:
            case concmd_ifinwater:
            case concmd_ifcount:
            case concmd_ifactor:
            case concmd_resetcount:
            case concmd_addinventory:
            case concmd_hitradius:
            case concmd_ifp:
            case concmd_ifstrength:
            case concmd_guts:
            case concmd_slapplayer:
            case concmd_wackplayer:
            case concmd_ifgapzl:
            case concmd_ifhitspace: 
            case concmd_ifoutside:
            case concmd_ifmultiplayer: 
            case concmd_operate:
            case concmd_fall:
            case concmd_ifpinventory:
            case concmd_ifinspace:
            case concmd_spritepal:
            case concmd_cactor:
            case concmd_ifbulletnear: 
            case concmd_ifrespawn:
            case concmd_iffloordistl:
            case concmd_ifceilingdistl:
            case concmd_palfrom:
            case concmd_ifphealthl:
            case concmd_pstomp:
            case concmd_ifawayfromwall:
            case concmd_quote:
            case concmd_ifinouterspace:
            case concmd_ifnotmoving: 
            case concmd_respawnhitag:
            case concmd_ifspritepal:
            case concmd_ifangdiffl:
            case concmd_ifnosounds:
            case concmd_ifvarg:
            case concmd_ifvarl:
            case concmd_setvarvar:
            case concmd_setvar:
            case concmd_addvarvar:
            case concmd_addvar:
            case concmd_ifvarvarl:
            case concmd_ifvarvarg:
            case concmd_addlogvar:
            case concmd_ifvare:
            case concmd_ifvarvare:
            case concmd_enda:
            case concmd_break:
            case concmd_ends:
            case concmd_endevent:
            case concmd_ifrnd:
            case concmd_ifactornotstayput:
            case concmd_ifsquished:
            case concmd_ifdead:
            case concmd_ifhitweapon:
            case concmd_addstrength:
            case concmd_strength:
            case concmd_rndmove:
            case concmd_tip:
            case concmd_nullop:
            case concmd_money:
            case concmd_mail:
            case concmd_sleeptime:
            case concmd_paper:
            case concmd_lotsofglass:
            case concmd_killit:
            case concmd_leftbrace:
            case concmd_rightbrace:
            case concmd_else:
            case concmd_state:
            case concmd_ifcanshoottarget:
            case concmd_ifcanseetarget:
            case concmd_ifnocover:
            case concmd_ifcansee:
            case concmd_ai:
            case concmd_action:
            case concmd_ifpdistl:
            case concmd_ifpdistg:
            case concmd_ifgotweaponce:
            case concmd_getlastpal:
            case concmd_tossweapon:
            case concmd_mikesnd:
            case concmd_pkick:
            case concmd_sizeto:
            case concmd_sizeat:
            case concmd_addphealth:
            case concmd_move:
            case concmd_spawn:
            case concmd_ifwasweapon:
            case concmd_ifspawnedby:
            case concmd_ifai:
            case concmd_ifaction:
            case concmd_ifactioncount:
            case concmd_resetactioncount:
            case concmd_debris:
            case concmd_count:
            case concmd_cstator:
            case concmd_clipdist:
            case concmd_cstat:
            case concmd_ifmove:
            case concmd_resetplayer:
            case concmd_ifcoop:

                if (parse()) goto out;
                if (killit_flag & 1) vm.flags |= VM_KILL;
                if (killit_flag & 2) vm.flags |= VM_NOEXECUTE;
                killit_flag = 0;
                continue;

            default:  // you aren't supposed to be here!
                if (RR && ud.recstat == 2)
                {
                    vm.flags |= VM_KILL;
                    return;
                }
                debug_break();
                G_GameExit("An error has occurred in the " GAMENAME " virtual machine.\n\n");
                break;
        }
    }
out:
    if (killit_flag & 1) vm.flags |= VM_KILL;
    if (killit_flag & 2) vm.flags |= VM_NOEXECUTE;
    killit_flag = 0;
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

extern int g_i, g_p;
extern int g_x;
extern int* g_t;
extern uint8_t killit_flag;
extern spritetype* g_sp;

// NORECURSE
void A_Execute(int spriteNum, int playerNum, int playerDist)
{
    if (!G_HaveActor(sprite[spriteNum].picnum)) return;

    vmstate_t tempvm
    = { spriteNum, playerNum, playerDist, 0, &sprite[spriteNum], &actor[spriteNum].t_data[0], g_player[playerNum].ps, &actor[spriteNum] };
    vm = tempvm;

    g_i = spriteNum;
    g_p = playerNum;
    g_x = playerDist;
    g_sp = &sprite[spriteNum];
    g_t = &actor[spriteNum].t_data[0];
    killit_flag = 0;

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
