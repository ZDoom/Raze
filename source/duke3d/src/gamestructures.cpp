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

// this is all the crap for accessing the game's structs through the CON VM
// I got a 3-4 fps gain by inlining these...

#ifndef gamevars_c_
int32_t __fastcall VM_GetUserdef(int32_t labelNum, int const lParm2);
void __fastcall VM_SetUserdef(int const labelNum, int const lParm2, int32_t const newValue);
int32_t __fastcall VM_GetActiveProjectile(int const spriteNum, int32_t labelNum);
void __fastcall VM_SetActiveProjectile(int const spriteNum, int const labelNum, int32_t const newValue);
int32_t __fastcall VM_GetPlayer(int const playerNum, int32_t labelNum, int const lParm2);
void __fastcall VM_SetPlayer(int const playerNum, int const labelNum, int const lParm2, int32_t const newValue);
int32_t __fastcall VM_GetPlayerInput(int const playerNum, int32_t labelNum);
void __fastcall VM_SetPlayerInput(int const playerNum, int const labelNum, int32_t const newValue);
int32_t __fastcall VM_GetWall(int const wallNum, int32_t labelNum);
void __fastcall VM_SetWall(int const wallNum, int const labelNum, int32_t const newValue);
int32_t __fastcall VM_GetSector(int const sectNum, int32_t labelNum);
void __fastcall VM_SetSector(int const sectNum, int const labelNum, int32_t newValue);
int32_t __fastcall VM_GetSprite(int const spriteNum, int32_t labelNum, int const lParm2);
void __fastcall VM_SetSprite(int const spriteNum, int const labelNum, int const lParm2, int32_t const newValue);
int32_t __fastcall VM_GetProjectile(int const tileNum, int32_t labelNum);
void __fastcall VM_SetProjectile(int const tileNum, int const labelNum, int32_t const newValue);
int32_t __fastcall VM_GetTileData(int const tileNum, int32_t labelNum);
void __fastcall VM_SetTileData(int const tileNum, int const labelNum, int32_t const newValue);
int32_t __fastcall VM_GetPalData(int const palNum, int32_t labelNum);
#else
#define LABEL_SETUP_UNMATCHED(struct, memb, name, idx)                                                              \
    {                                                                                                               \
        name, idx, sizeof(struct[0].memb) | (is_unsigned<decltype(struct[0].memb)>::value ? LABEL_UNSIGNED : 0), 0, \
        offsetof(remove_pointer_t<decltype(&struct[0])>, memb)                                                      \
    }

#define LABEL_SETUP(struct, memb, idx) LABEL_SETUP_UNMATCHED(struct, memb, #memb, idx)

const memberlabel_t SectorLabels[] = {
    { "wallptr",                         SECTOR_WALLPTR, sizeof(sector[0].wallptr) | LABEL_WRITEFUNC, 0, offsetof(usectortype, wallptr) },
    LABEL_SETUP(sector, wallnum,         SECTOR_WALLNUM),

    LABEL_SETUP(sector, ceilingz,        SECTOR_CEILINGZ),
    { "ceilingzgoal",                    SECTOR_CEILINGZGOAL, 0, 0, -1 },
    { "ceilingzvel",                     SECTOR_CEILINGZVEL, 0, 0, -1 },

    LABEL_SETUP(sector, floorz,          SECTOR_FLOORZ),
    { "floorzgoal",                      SECTOR_FLOORZGOAL, 0, 0, -1 },
    { "floorzvel",                       SECTOR_FLOORZVEL, 0, 0, -1 },

    LABEL_SETUP(sector, ceilingstat,     SECTOR_CEILINGSTAT),
    LABEL_SETUP(sector, floorstat,       SECTOR_FLOORSTAT),

    LABEL_SETUP(sector, ceilingpicnum,   SECTOR_CEILINGPICNUM),
    LABEL_SETUP_UNMATCHED(sector, ceilingheinum, "ceilingslope", SECTOR_CEILINGSLOPE),

    LABEL_SETUP(sector, ceilingshade,    SECTOR_CEILINGSHADE),
    LABEL_SETUP(sector, ceilingpal,      SECTOR_CEILINGPAL),
    LABEL_SETUP(sector, ceilingxpanning, SECTOR_CEILINGXPANNING),
    LABEL_SETUP(sector, ceilingypanning, SECTOR_CEILINGYPANNING),

    LABEL_SETUP(sector, floorpicnum,     SECTOR_FLOORPICNUM),
    LABEL_SETUP_UNMATCHED(sector, floorheinum,   "floorslope",   SECTOR_FLOORSLOPE),
    LABEL_SETUP(sector, floorshade,      SECTOR_FLOORSHADE),
    LABEL_SETUP(sector, floorpal,        SECTOR_FLOORPAL),
    LABEL_SETUP(sector, floorxpanning,   SECTOR_FLOORXPANNING),
    LABEL_SETUP(sector, floorypanning,   SECTOR_FLOORYPANNING),

    LABEL_SETUP(sector, visibility,      SECTOR_VISIBILITY),
    LABEL_SETUP(sector, fogpal,          SECTOR_FOGPAL),

    LABEL_SETUP(sector, lotag,           SECTOR_LOTAG),
    LABEL_SETUP(sector, hitag,           SECTOR_HITAG),
    LABEL_SETUP(sector, extra,           SECTOR_EXTRA),

    { "ceilingbunch",                    SECTOR_CEILINGBUNCH, 0, 0, -1 },
    { "floorbunch",                      SECTOR_FLOORBUNCH, 0, 0, -1 },

    { "ulotag",                          SECTOR_ULOTAG, sizeof(sector[0].lotag) | LABEL_UNSIGNED, 0, offsetof(usectortype, lotag) },
    { "uhitag",                          SECTOR_UHITAG, sizeof(sector[0].hitag) | LABEL_UNSIGNED, 0, offsetof(usectortype, hitag) },

};

int32_t __fastcall VM_GetSector(int const sectNum, int32_t labelNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)sectNum >= (unsigned)numsectors))
    {
        CON_ERRPRINTF("invalid sector %d\n", sectNum);
        return -1;
    }

    auto const &s = *(usectorptr_t)&sector[sectNum];

    switch (labelNum)
    {
        case SECTOR_CEILINGZVEL:
            labelNum = (GetAnimationGoal(&s.ceilingz) == -1) ? 0 : s.extra; break;
        case SECTOR_CEILINGZGOAL:
            labelNum = GetAnimationGoal(&s.ceilingz); break;

        case SECTOR_FLOORZVEL:
            labelNum = (GetAnimationGoal(&s.floorz) == -1) ? 0 : s.extra; break;

        case SECTOR_FLOORZGOAL:
            labelNum = GetAnimationGoal(&s.floorz); break;

        case SECTOR_CEILINGBUNCH:
        case SECTOR_FLOORBUNCH:
#ifdef YAX_ENABLE
            labelNum = yax_getbunch(sectNum, labelNum == SECTOR_FLOORBUNCH);
#else
            labelNum = -1;
#endif
            break;

        default: EDUKE32_UNREACHABLE_SECTION(labelNum = -1; break);
    }

    return labelNum;
}

void __fastcall VM_SetSector(int const sectNum, int const labelNum, int32_t newValue)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)sectNum >= (unsigned)numsectors))
    {
        CON_ERRPRINTF("invalid sector %d\n", sectNum);
        return;
    }

    auto &s = sector[sectNum];

    switch (labelNum)
    {
        case SECTOR_WALLPTR:
            setfirstwall(sectNum, newValue); break;

        case SECTOR_CEILINGZVEL:
            s.extra = newValue;
            if ((newValue = GetAnimationGoal(&s.ceilingz)) != -1)
            {
        case SECTOR_CEILINGZGOAL:
                SetAnimation(sectNum, &s.ceilingz, newValue, s.extra);
            }
            break;

        case SECTOR_FLOORZVEL:
            s.extra = newValue;
            if ((newValue = GetAnimationGoal(&s.floorz)) != -1)
            {
        case SECTOR_FLOORZGOAL:
                SetAnimation(sectNum, &s.floorz, newValue, s.extra);
            }
            break;

        case SECTOR_CEILINGBUNCH:
        case SECTOR_FLOORBUNCH:
            break;

        default: EDUKE32_UNREACHABLE_SECTION(break);
    }
}

const memberlabel_t WallLabels[]=
{
    LABEL_SETUP(wall, x,          WALL_X),
    LABEL_SETUP(wall, y,          WALL_Y),
    LABEL_SETUP(wall, point2,     WALL_POINT2),
    LABEL_SETUP(wall, nextwall,   WALL_NEXTWALL),
    LABEL_SETUP(wall, nextsector, WALL_NEXTSECTOR),
    LABEL_SETUP(wall, cstat,      WALL_CSTAT),
    LABEL_SETUP(wall, picnum,     WALL_PICNUM),
    LABEL_SETUP(wall, overpicnum, WALL_OVERPICNUM),
    LABEL_SETUP(wall, shade,      WALL_SHADE),
    LABEL_SETUP(wall, pal,        WALL_PAL),
    LABEL_SETUP(wall, xrepeat,    WALL_XREPEAT),
    LABEL_SETUP(wall, yrepeat,    WALL_YREPEAT),
    LABEL_SETUP(wall, xpanning,   WALL_XPANNING),
    LABEL_SETUP(wall, ypanning,   WALL_YPANNING),
    LABEL_SETUP(wall, lotag,      WALL_LOTAG),
    LABEL_SETUP(wall, hitag,      WALL_HITAG),
    LABEL_SETUP(wall, extra,      WALL_EXTRA),

    { "ulotag", WALL_ULOTAG, sizeof(wall[0].lotag) | LABEL_UNSIGNED, 0, offsetof(uwalltype, lotag) },
    { "uhitag", WALL_UHITAG, sizeof(wall[0].hitag) | LABEL_UNSIGNED, 0, offsetof(uwalltype, hitag) },

    { "blend", WALL_BLEND, 0, 0, -1 },
};

int32_t __fastcall VM_GetWall(int const wallNum, int32_t labelNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)wallNum >= (unsigned)numwalls))
    {
        CON_ERRPRINTF("invalid wall %d\n", wallNum);
        return -1;
    }

    switch (labelNum)
    {
        case WALL_BLEND:
#ifdef NEW_MAP_FORMAT
            labelNum = w.blend;
#else
            labelNum = wallext[wallNum].blend;
#endif
            break;

        default: EDUKE32_UNREACHABLE_SECTION(labelNum = -1; break);
    }

    return labelNum;
}

void __fastcall VM_SetWall(int const wallNum, int const labelNum, int32_t const newValue)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)wallNum >= (unsigned)numwalls))
    {
        CON_ERRPRINTF("invalid wall %d\n", wallNum);
        return;
    }

    switch (labelNum)
    {
        case WALL_BLEND:
#ifdef NEW_MAP_FORMAT
            w.blend = newValue;
#else
            wallext[wallNum].blend = newValue;
#endif
            break;
    }

}

const memberlabel_t ActorLabels[]=
{
    LABEL_SETUP(sprite, x,        ACTOR_X),
    LABEL_SETUP(sprite, y,        ACTOR_Y),
    LABEL_SETUP(sprite, z,        ACTOR_Z),
    LABEL_SETUP(sprite, cstat,    ACTOR_CSTAT),
    LABEL_SETUP(sprite, picnum,   ACTOR_PICNUM),
    LABEL_SETUP(sprite, shade,    ACTOR_SHADE),
    LABEL_SETUP(sprite, pal,      ACTOR_PAL),
    LABEL_SETUP(sprite, clipdist, ACTOR_CLIPDIST),
    LABEL_SETUP(sprite, blend,    ACTOR_DETAIL),
    LABEL_SETUP(sprite, xrepeat,  ACTOR_XREPEAT),
    LABEL_SETUP(sprite, yrepeat,  ACTOR_YREPEAT),
    LABEL_SETUP(sprite, xoffset,  ACTOR_XOFFSET),
    LABEL_SETUP(sprite, yoffset,  ACTOR_YOFFSET),
    { "sectnum", ACTOR_SECTNUM, sizeof(sprite[0].sectnum) | LABEL_WRITEFUNC, 0, offsetof(uspritetype, sectnum) },
    { "statnum", ACTOR_STATNUM, sizeof(sprite[0].statnum) | LABEL_WRITEFUNC, 0, offsetof(uspritetype, statnum) },
    LABEL_SETUP(sprite, ang,      ACTOR_ANG),
    LABEL_SETUP(sprite, owner,    ACTOR_OWNER),
    LABEL_SETUP(sprite, xvel,     ACTOR_XVEL),
    LABEL_SETUP(sprite, yvel,     ACTOR_YVEL),
    LABEL_SETUP(sprite, zvel,     ACTOR_ZVEL),
    LABEL_SETUP(sprite, lotag,    ACTOR_LOTAG),
    LABEL_SETUP(sprite, hitag,    ACTOR_HITAG),
    LABEL_SETUP(sprite, extra,    ACTOR_EXTRA),

    { "ulotag", ACTOR_ULOTAG, sizeof(sprite[0].lotag) | LABEL_UNSIGNED, 0, offsetof(uspritetype, lotag) },
    { "uhitag", ACTOR_UHITAG, sizeof(sprite[0].hitag) | LABEL_UNSIGNED, 0, offsetof(uspritetype, hitag) },

    // ActorExtra labels...
    LABEL_SETUP_UNMATCHED(actor, cgg,         "htcgg",          ACTOR_HTCGG),
    LABEL_SETUP_UNMATCHED(actor, picnum,      "htpicnum",       ACTOR_HTPICNUM),
    LABEL_SETUP_UNMATCHED(actor, ang,         "htang",          ACTOR_HTANG),
    LABEL_SETUP_UNMATCHED(actor, extra,       "htextra",        ACTOR_HTEXTRA),
    LABEL_SETUP_UNMATCHED(actor, owner,       "htowner",        ACTOR_HTOWNER),
    LABEL_SETUP_UNMATCHED(actor, movflag,     "htmovflag",      ACTOR_HTMOVFLAG),
    { "htumovflag", ACTOR_HTUMOVFLAG, sizeof(actor[0].movflag) | LABEL_UNSIGNED, 0, offsetof(actor_t, movflag) },
    LABEL_SETUP_UNMATCHED(actor, tempang,     "httempang",      ACTOR_HTTEMPANG),
    LABEL_SETUP_UNMATCHED(actor, stayput,     "htactorstayput", ACTOR_HTSTAYPUT),
    LABEL_SETUP_UNMATCHED(actor, dispicnum,   "htdispicnum",    ACTOR_HTDISPICNUM),
    LABEL_SETUP_UNMATCHED(actor, timetosleep, "httimetosleep",  ACTOR_HTTIMETOSLEEP),
    LABEL_SETUP_UNMATCHED(actor, floorz,      "htfloorz",       ACTOR_HTFLOORZ),
    LABEL_SETUP_UNMATCHED(actor, ceilingz,    "htceilingz",     ACTOR_HTCEILINGZ),
    LABEL_SETUP_UNMATCHED(actor, lastv.x,     "htlastvx",       ACTOR_HTLASTVX),
    LABEL_SETUP_UNMATCHED(actor, lastv.y,     "htlastvy",       ACTOR_HTLASTVY),
    LABEL_SETUP_UNMATCHED(actor, bpos.x,      "htbposx",        ACTOR_HTBPOSX),
    LABEL_SETUP_UNMATCHED(actor, bpos.y,      "htbposy",        ACTOR_HTBPOSY),
    LABEL_SETUP_UNMATCHED(actor, bpos.z,      "htbposz",        ACTOR_HTBPOSZ),

    { "htg_t",          ACTOR_HTG_T,                  LABEL_HASPARM2, 10, -1 },
    LABEL_SETUP_UNMATCHED(actor, flags,       "htflags",        ACTOR_HTFLAGS),

    // model flags

    LABEL_SETUP(spriteext, angoff, ACTOR_ANGOFF),
    LABEL_SETUP(spriteext, pitch, ACTOR_PITCH),
    LABEL_SETUP(spriteext, roll, ACTOR_ROLL),

    LABEL_SETUP_UNMATCHED(spriteext, offset.x, "mdxoff",  ACTOR_MDXOFF),
    LABEL_SETUP_UNMATCHED(spriteext, offset.y, "mdyoff",  ACTOR_MDYOFF),
    LABEL_SETUP_UNMATCHED(spriteext, offset.z, "mdzoff",  ACTOR_MDZOFF),
    LABEL_SETUP_UNMATCHED(spriteext, flags,    "mdflags", ACTOR_MDFLAGS),

    LABEL_SETUP(spriteext, xpanning, ACTOR_XPANNING),
    LABEL_SETUP(spriteext, ypanning, ACTOR_YPANNING),

    { "alpha",          ACTOR_ALPHA,                  0, 0, -1 },

    { "isvalid",        ACTOR_ISVALID,                0, 0, -1 },
// aliases:
    { "movflags",       ACTOR_HITAG,                  0, 0, -1 },
    { "detail",         ACTOR_DETAIL,                 0, 0, -1 },  // deprecated name for 'blend'
};

void __fastcall VM_SetSprite(int const spriteNum, int const labelNum, int const lParm2, int32_t const newValue)
{
    auto &a   = actor[spriteNum];
    auto &ext = spriteext[spriteNum];

    switch (labelNum)
    {
        case ACTOR_SECTNUM: changespritesect(spriteNum, newValue); break;
        case ACTOR_STATNUM: changespritestat(spriteNum, newValue); break;
        case ACTOR_HTG_T: a.t_data[lParm2] = newValue; break;
        case ACTOR_ALPHA: ext.alpha = (float)newValue * (1.f / 255.0f); break;
        default: EDUKE32_UNREACHABLE_SECTION(break);
    }
}


int32_t __fastcall VM_GetSprite(int const spriteNum, int32_t labelNum, int const lParm2)
{
    auto const &a   = actor[spriteNum];
    auto const &s   = sprite[spriteNum];
    auto const &ext = spriteext[spriteNum];

    switch (labelNum)
    {
        case ACTOR_HTG_T: labelNum = a.t_data[lParm2]; break;
        case ACTOR_ALPHA: labelNum = (uint8_t)(ext.alpha * 255.0f); break;
        case ACTOR_ISVALID: labelNum = (s.statnum != MAXSTATUS); break;
        default: EDUKE32_UNREACHABLE_SECTION(labelNum = -1; break);
    }

    return labelNum;
}

const memberlabel_t TsprLabels[] =
{
    // tsprite access

    LABEL_SETUP_UNMATCHED(sprite, x,        "tsprx",        ACTOR_X),
    LABEL_SETUP_UNMATCHED(sprite, y,        "tspry",        ACTOR_Y),
    LABEL_SETUP_UNMATCHED(sprite, z,        "tsprz",        ACTOR_Z),
    LABEL_SETUP_UNMATCHED(sprite, cstat,    "tsprcstat",    ACTOR_CSTAT),
    LABEL_SETUP_UNMATCHED(sprite, picnum,   "tsprpicnum",   ACTOR_PICNUM),
    LABEL_SETUP_UNMATCHED(sprite, shade,    "tsprshade",    ACTOR_SHADE),
    LABEL_SETUP_UNMATCHED(sprite, pal,      "tsprpal",      ACTOR_PAL),
    LABEL_SETUP_UNMATCHED(sprite, clipdist, "tsprclipdist", ACTOR_CLIPDIST),
    LABEL_SETUP_UNMATCHED(sprite, blend,    "tsprblend",    ACTOR_DETAIL),
    LABEL_SETUP_UNMATCHED(sprite, xrepeat,  "tsprxrepeat",  ACTOR_XREPEAT),
    LABEL_SETUP_UNMATCHED(sprite, yrepeat,  "tspryrepeat",  ACTOR_YREPEAT),
    LABEL_SETUP_UNMATCHED(sprite, xoffset,  "tsprxoffset",  ACTOR_XOFFSET),
    LABEL_SETUP_UNMATCHED(sprite, yoffset,  "tspryoffset",  ACTOR_YOFFSET),
    LABEL_SETUP_UNMATCHED(sprite, sectnum,  "tsprsectnum",  ACTOR_SECTNUM),
    LABEL_SETUP_UNMATCHED(sprite, statnum,  "tsprstatnum",  ACTOR_STATNUM),
    LABEL_SETUP_UNMATCHED(sprite, ang,      "tsprang",      ACTOR_ANG),
    LABEL_SETUP_UNMATCHED(sprite, owner,    "tsprowner",    ACTOR_OWNER),
    LABEL_SETUP_UNMATCHED(sprite, xvel,     "tsprxvel",     ACTOR_XVEL),
    LABEL_SETUP_UNMATCHED(sprite, yvel,     "tspryvel",     ACTOR_YVEL),
    LABEL_SETUP_UNMATCHED(sprite, zvel,     "tsprzvel",     ACTOR_ZVEL),
    LABEL_SETUP_UNMATCHED(sprite, lotag,    "tsprlotag",    ACTOR_LOTAG),
    LABEL_SETUP_UNMATCHED(sprite, hitag,    "tsprhitag",    ACTOR_HITAG),
    LABEL_SETUP_UNMATCHED(sprite, extra,    "tsprextra",    ACTOR_EXTRA),
};

const memberlabel_t PlayerLabels[]=
{
    { "zoom",                  PLAYER_ZOOM,                  0, 0, -1 },
    { "loogiex",               PLAYER_LOOGIEX,               LABEL_HASPARM2, 64, -1 },
    { "loogiey",               PLAYER_LOOGIEY,               LABEL_HASPARM2, 64, -1 },
    { "numloogs",              PLAYER_NUMLOOGS,              0, 0, -1 },
    { "loogcnt",               PLAYER_LOOGCNT,               0, 0, -1 },
    { "posx",                  PLAYER_POSX,                  0, 0, -1 },
    { "posy",                  PLAYER_POSY,                  0, 0, -1 },
    { "posz",                  PLAYER_POSZ,                  0, 0, -1 },
    { "horiz",                 PLAYER_HORIZ,                 0, 0, -1 },
    { "horizoff",              PLAYER_HORIZOFF,              0, 0, -1 },
    { "ohoriz",                PLAYER_OHORIZ,                0, 0, -1 },
    { "ohorizoff",             PLAYER_OHORIZOFF,             0, 0, -1 },
    { "q16horiz",              PLAYER_Q16HORIZ,              0, 0, -1 },
    { "q16horizoff",           PLAYER_Q16HORIZOFF,           0, 0, -1 },
    { "oq16horiz",             PLAYER_OQ16HORIZ,             0, 0, -1 },
    { "oq16horizoff",          PLAYER_OQ16HORIZOFF,          0, 0, -1 },

    { "invdisptime",           PLAYER_INVDISPTIME,           0, 0, -1 },
    { "bobposx",               PLAYER_BOBPOSX,               0, 0, -1 },
    { "bobposy",               PLAYER_BOBPOSY,               0, 0, -1 },
    { "oposx",                 PLAYER_OPOSX,                 0, 0, -1 },
    { "oposy",                 PLAYER_OPOSY,                 0, 0, -1 },
    { "oposz",                 PLAYER_OPOSZ,                 0, 0, -1 },
    { "pyoff",                 PLAYER_PYOFF,                 0, 0, -1 },
    { "opyoff",                PLAYER_OPYOFF,                0, 0, -1 },
    { "posxv",                 PLAYER_POSXV,                 0, 0, -1 },
    { "posyv",                 PLAYER_POSYV,                 0, 0, -1 },
    { "poszv",                 PLAYER_POSZV,                 0, 0, -1 },
    { "last_pissed_time",      PLAYER_LAST_PISSED_TIME,      0, 0, -1 },
    { "truefz",                PLAYER_TRUEFZ,                0, 0, -1 },
    { "truecz",                PLAYER_TRUECZ,                0, 0, -1 },
    { "player_par",            PLAYER_PLAYER_PAR,            0, 0, -1 },
    { "visibility",            PLAYER_VISIBILITY,            0, 0, -1 },
    { "bobcounter",            PLAYER_BOBCOUNTER,            0, 0, -1 },
    { "weapon_sway",           PLAYER_WEAPON_SWAY,           0, 0, -1 },
    { "pals_time",             PLAYER_PALS_TIME,             0, 0, -1 },
    { "crack_time",            PLAYER_CRACK_TIME,            0, 0, -1 },
    { "aim_mode",              PLAYER_AIM_MODE,              0, 0, -1 },
    { "ang",                   PLAYER_ANG,                   0, 0, -1 },
    { "oang",                  PLAYER_OANG,                  0, 0, -1 },
    { "q16ang",                PLAYER_Q16ANG,                0, 0, -1 },
    { "oq16ang",               PLAYER_OQ16ANG,               0, 0, -1 },
    { "angvel",                PLAYER_ANGVEL,                0, 0, -1 },
    { "q16angvel",             PLAYER_Q16ANGVEL,             0, 0, -1 },
    { "cursectnum",            PLAYER_CURSECTNUM,            0, 0, -1 },
    { "look_ang",              PLAYER_LOOK_ANG,              0, 0, -1 },
    { "last_extra",            PLAYER_LAST_EXTRA,            0, 0, -1 },
    { "subweapon",             PLAYER_SUBWEAPON,             0, 0, -1 },
    { "ammo_amount",           PLAYER_AMMO_AMOUNT,           LABEL_HASPARM2, MAX_WEAPONS, -1 },
    { "wackedbyactor",         PLAYER_WACKEDBYACTOR,         0, 0, -1 },
    { "frag",                  PLAYER_FRAG,                  0, 0, -1 },
    { "fraggedself",           PLAYER_FRAGGEDSELF,           0, 0, -1 },
    { "curr_weapon",           PLAYER_CURR_WEAPON,           0, 0, -1 },
    { "last_weapon",           PLAYER_LAST_WEAPON,           0, 0, -1 },
    { "tipincs",               PLAYER_TIPINCS,               0, 0, -1 },
    { "wantweaponfire",        PLAYER_WANTWEAPONFIRE,        0, 0, -1 },
    { "holoduke_amount",       PLAYER_HOLODUKE_AMOUNT,       0, 0, -1 },
    { "newowner",              PLAYER_NEWOWNER,              0, 0, -1 },
    { "hurt_delay",            PLAYER_HURT_DELAY,            0, 0, -1 },
    { "hbomb_hold_delay",      PLAYER_HBOMB_HOLD_DELAY,      0, 0, -1 },
    { "jumping_counter",       PLAYER_JUMPING_COUNTER,       0, 0, -1 },
    { "airleft",               PLAYER_AIRLEFT,               0, 0, -1 },
    { "knee_incs",             PLAYER_KNEE_INCS,             0, 0, -1 },
    { "access_incs",           PLAYER_ACCESS_INCS,           0, 0, -1 },
    { "fta",                   PLAYER_FTA,                   0, 0, -1 },
    { "ftq",                   PLAYER_FTQ,                   0, 0, -1 },
    { "access_wallnum",        PLAYER_ACCESS_WALLNUM,        0, 0, -1 },
    { "access_spritenum",      PLAYER_ACCESS_SPRITENUM,      0, 0, -1 },
    { "kickback_pic",          PLAYER_KICKBACK_PIC,          0, 0, -1 },
    { "got_access",            PLAYER_GOT_ACCESS,            0, 0, -1 },
    { "weapon_ang",            PLAYER_WEAPON_ANG,            0, 0, -1 },
    { "firstaid_amount",       PLAYER_FIRSTAID_AMOUNT,       0, 0, -1 },
    { "somethingonplayer",     PLAYER_SOMETHINGONPLAYER,     0, 0, -1 },
    { "on_crane",              PLAYER_ON_CRANE,              0, 0, -1 },
    { "i",                     PLAYER_I,                     0, 0, -1 },
    { "one_parallax_sectnum",  PLAYER_PARALLAX_SECTNUM,      0, 0, -1 },
    { "over_shoulder_on",      PLAYER_OVER_SHOULDER_ON,      0, 0, -1 },
    { "random_club_frame",     PLAYER_RANDOM_CLUB_FRAME,     0, 0, -1 },
    { "fist_incs",             PLAYER_FIST_INCS,             0, 0, -1 },
    { "one_eighty_count",      PLAYER_ONE_EIGHTY_COUNT,      0, 0, -1 },
    { "cheat_phase",           PLAYER_CHEAT_PHASE,           0, 0, -1 },
    { "dummyplayersprite",     PLAYER_DUMMYPLAYERSPRITE,     0, 0, -1 },
    { "extra_extra8",          PLAYER_EXTRA_EXTRA8,          0, 0, -1 },
    { "quick_kick",            PLAYER_QUICK_KICK,            0, 0, -1 },
    { "heat_amount",           PLAYER_HEAT_AMOUNT,           0, 0, -1 },
    { "actorsqu",              PLAYER_ACTORSQU,              0, 0, -1 },
    { "timebeforeexit",        PLAYER_TIMEBEFOREEXIT,        0, 0, -1 },
    { "customexitsound",       PLAYER_CUSTOMEXITSOUND,       0, 0, -1 },
    { "weaprecs",              PLAYER_WEAPRECS,              LABEL_HASPARM2, MAX_WEAPONS, -1 },
    { "weapreccnt",            PLAYER_WEAPRECCNT,            0, 0, -1 },
    { "interface_toggle_flag", PLAYER_INTERFACE_TOGGLE,      0, 0, -1 },
    { "rotscrnang",            PLAYER_ROTSCRNANG,            0, 0, -1 },
    { "dead_flag",             PLAYER_DEAD_FLAG,             0, 0, -1 },
    { "show_empty_weapon",     PLAYER_SHOW_EMPTY_WEAPON,     0, 0, -1 },
    { "scuba_amount",          PLAYER_SCUBA_AMOUNT,          0, 0, -1 },
    { "jetpack_amount",        PLAYER_JETPACK_AMOUNT,        0, 0, -1 },
    { "steroids_amount",       PLAYER_STEROIDS_AMOUNT,       0, 0, -1 },
    { "shield_amount",         PLAYER_SHIELD_AMOUNT,         0, 0, -1 },
    { "holoduke_on",           PLAYER_HOLODUKE_ON,           0, 0, -1 },
    { "pycount",               PLAYER_PYCOUNT,               0, 0, -1 },
    { "weapon_pos",            PLAYER_WEAPON_POS,            0, 0, -1 },
    { "frag_ps",               PLAYER_FRAG_PS,               0, 0, -1 },
    { "transporter_hold",      PLAYER_TRANSPORTER_HOLD,      0, 0, -1 },
    { "clipdist",              PLAYER_CLIPDIST,              0, 0, -1 },
    { "last_full_weapon",      PLAYER_LAST_FULL_WEAPON,      0, 0, -1 },
    { "footprintshade",        PLAYER_FOOTPRINTSHADE,        0, 0, -1 },
    { "boot_amount",           PLAYER_BOOT_AMOUNT,           0, 0, -1 },
    { "scream_voice",          PLAYER_SCREAM_VOICE,          0, 0, -1 },
    { "gm",                    PLAYER_GM,                    0, 0, -1 },
    { "on_warping_sector",     PLAYER_ON_WARPING_SECTOR,     0, 0, -1 },
    { "footprintcount",        PLAYER_FOOTPRINTCOUNT,        0, 0, -1 },
    { "hbomb_on",              PLAYER_HBOMB_ON,              0, 0, -1 },
    { "jumping_toggle",        PLAYER_JUMPING_TOGGLE,        0, 0, -1 },
    { "rapid_fire_hold",       PLAYER_RAPID_FIRE_HOLD,       0, 0, -1 },
    { "on_ground",             PLAYER_ON_GROUND,             0, 0, -1 },
    { "name",                  PLAYER_NAME,                  LABEL_ISSTRING, 32, -1 },
    { "inven_icon",            PLAYER_INVEN_ICON,            0, 0, -1 },
    { "buttonpalette",         PLAYER_BUTTONPALETTE,         0, 0, -1 },
    { "jetpack_on",            PLAYER_JETPACK_ON,            0, 0, -1 },
    { "spritebridge",          PLAYER_SPRITEBRIDGE,          0, 0, -1 },
    { "scuba_on",              PLAYER_SCUBA_ON,              0, 0, -1 },
    { "footprintpal",          PLAYER_FOOTPRINTPAL,          0, 0, -1 },
    { "heat_on",               PLAYER_HEAT_ON,               0, 0, -1 },
    { "holster_weapon",        PLAYER_HOLSTER_WEAPON,        0, 0, -1 },
    { "falling_counter",       PLAYER_FALLING_COUNTER,       0, 0, -1 },
    { "gotweapon",             PLAYER_GOTWEAPON,             LABEL_HASPARM2, MAX_WEAPONS, -1 },
    { "palette",               PLAYER_PALETTE,               0, 0, -1 },
    { "toggle_key_flag",       PLAYER_TOGGLE_KEY_FLAG,       0, 0, -1 },
    { "knuckle_incs",          PLAYER_KNUCKLE_INCS,          0, 0, -1 },
    { "walking_snd_toggle",    PLAYER_WALKING_SND_TOGGLE,    0, 0, -1 },
    { "palookup",              PLAYER_PALOOKUP,              0, 0, -1 },
    { "hard_landing",          PLAYER_HARD_LANDING,          0, 0, -1 },
    { "max_secret_rooms",      PLAYER_MAX_SECRET_ROOMS,      0, 0, -1 },
    { "secret_rooms",          PLAYER_SECRET_ROOMS,          0, 0, -1 },
    { "pals",                  PLAYER_PALS,                  LABEL_HASPARM2, 3, -1 },
    { "max_actors_killed",     PLAYER_MAX_ACTORS_KILLED,     0, 0, -1 },
    { "actors_killed",         PLAYER_ACTORS_KILLED,         0, 0, -1 },
    { "return_to_center",      PLAYER_RETURN_TO_CENTER,      0, 0, -1 },
    { "runspeed",              PLAYER_RUNSPEED,              0, 0, -1 },
    { "sbs",                   PLAYER_SBS,                   0, 0, -1 },
    { "reloading",             PLAYER_RELOADING,             0, 0, -1 },
    { "auto_aim",              PLAYER_AUTO_AIM,              0, 0, -1 },
    { "movement_lock",         PLAYER_MOVEMENT_LOCK,         0, 0, -1 },
    { "sound_pitch",           PLAYER_SOUND_PITCH,           0, 0, -1 },
    { "weaponswitch",          PLAYER_WEAPONSWITCH,          0, 0, -1 },
    { "team",                  PLAYER_TEAM,                  0, 0, -1 },
    { "max_player_health",     PLAYER_MAX_PLAYER_HEALTH,     0, 0, -1 },
    { "max_shield_amount",     PLAYER_MAX_SHIELD_AMOUNT,     0, 0, -1 },
    { "max_ammo_amount",       PLAYER_MAX_AMMO_AMOUNT,       LABEL_HASPARM2, MAX_WEAPONS, -1 },
    { "last_quick_kick",       PLAYER_LAST_QUICK_KICK,       0, 0, -1 },
    { "autostep",              PLAYER_AUTOSTEP,              0, 0, -1 },
    { "autostep_sbw",          PLAYER_AUTOSTEP_SBW,          0, 0, -1 },
    { "hudpal",                PLAYER_HUDPAL,                0, 0, -1 },
    { "index",                 PLAYER_INDEX,                 0, 0, -1 },
    { "connected",             PLAYER_CONNECTED,             0, 0, -1 },
    { "frags",                 PLAYER_FRAGS,                 LABEL_HASPARM2, MAXPLAYERS, -1 },
    { "deaths",                PLAYER_DEATHS,                0, 0, -1 },
    { "last_used_weapon",      PLAYER_LAST_USED_WEAPON,      0, 0, -1 },
    { "bsubweapon",            PLAYER_BSUBWEAPON,            LABEL_HASPARM2, MAX_WEAPONS, -1 },
    { "crouch_toggle",         PLAYER_CROUCH_TOGGLE,         0, 0, -1 },
};

int32_t __fastcall VM_GetPlayer(int const playerNum, int32_t labelNum, int const lParm2)
{
    if (EDUKE32_PREDICT_FALSE(((unsigned) playerNum >= (unsigned) g_mostConcurrentPlayers)
        || (PlayerLabels[labelNum].flags & LABEL_HASPARM2 && (unsigned) lParm2 >= (unsigned) PlayerLabels[labelNum].maxParm2)))
    {
        CON_ERRPRINTF("%s[%d] invalid for player %d\n", PlayerLabels[labelNum].name, lParm2, playerNum);
        return -1;
    }

    auto const &ps = *g_player[playerNum].ps;

    switch (labelNum)
    {
        case PLAYER_ANG:       labelNum = fix16_to_int(ps.q16ang);       break;
        case PLAYER_OANG:      labelNum = fix16_to_int(ps.oq16ang);      break;
        case PLAYER_ANGVEL:    labelNum = fix16_to_int(ps.q16angvel);    break;
        case PLAYER_HORIZ:     labelNum = fix16_to_int(ps.q16horiz);     break;
        case PLAYER_OHORIZ:    labelNum = fix16_to_int(ps.oq16horiz);    break;
        case PLAYER_HORIZOFF:  labelNum = fix16_to_int(ps.q16horizoff);  break;
        case PLAYER_OHORIZOFF: labelNum = fix16_to_int(ps.oq16horizoff); break;

        case PLAYER_Q16ANG:       labelNum = ps.q16ang;       break;
        case PLAYER_OQ16ANG:      labelNum = ps.oq16ang;      break;
        case PLAYER_Q16ANGVEL:    labelNum = ps.q16angvel;    break;
        case PLAYER_Q16HORIZ:     labelNum = ps.q16horiz;     break;
        case PLAYER_OQ16HORIZ:    labelNum = ps.oq16horiz;    break;
        case PLAYER_Q16HORIZOFF:  labelNum = ps.q16horizoff;  break;
        case PLAYER_OQ16HORIZOFF: labelNum = ps.oq16horizoff; break;

        case PLAYER_ACCESS_INCS:        labelNum = ps.access_incs;        break;
        case PLAYER_ACCESS_SPRITENUM:   labelNum = ps.access_spritenum;   break;
        case PLAYER_ACCESS_WALLNUM:     labelNum = ps.access_wallnum;     break;
        case PLAYER_ACTORS_KILLED:      labelNum = ps.actors_killed;      break;
        case PLAYER_ACTORSQU:           labelNum = ps.actorsqu;           break;
        case PLAYER_AIM_MODE:           labelNum = ps.aim_mode;           break;
        case PLAYER_AIRLEFT:            labelNum = ps.airleft;            break;
        case PLAYER_AUTO_AIM:           labelNum = ps.auto_aim;           break;
        case PLAYER_AUTOSTEP:           labelNum = ps.autostep;           break;
        case PLAYER_AUTOSTEP_SBW:       labelNum = ps.autostep_sbw;       break;
        case PLAYER_BOBCOUNTER:         labelNum = ps.bobcounter;         break;
        case PLAYER_BOBPOSX:            labelNum = ps.bobpos.x;           break;
        case PLAYER_BOBPOSY:            labelNum = ps.bobpos.y;           break;
        case PLAYER_BUTTONPALETTE:      labelNum = ps.buttonpalette;      break;
        case PLAYER_CHEAT_PHASE:        labelNum = ps.cheat_phase;        break;
        case PLAYER_CLIPDIST:           labelNum = ps.clipdist;           break;
        case PLAYER_CRACK_TIME:         labelNum = ps.crack_time;         break;
        case PLAYER_CROUCH_TOGGLE:      labelNum = ps.crouch_toggle;      break;
        case PLAYER_CURR_WEAPON:        labelNum = ps.curr_weapon;        break;
        case PLAYER_CURSECTNUM:         labelNum = ps.cursectnum;         break;
        case PLAYER_CUSTOMEXITSOUND:    labelNum = ps.customexitsound;    break;
        case PLAYER_DEAD_FLAG:          labelNum = ps.dead_flag;          break;
        case PLAYER_DUMMYPLAYERSPRITE:  labelNum = ps.dummyplayersprite;  break;
        case PLAYER_EXTRA_EXTRA8:       labelNum = ps.extra_extra8;       break;
        case PLAYER_FALLING_COUNTER:    labelNum = ps.falling_counter;    break;
        case PLAYER_FIST_INCS:          labelNum = ps.fist_incs;          break;
        case PLAYER_FOOTPRINTCOUNT:     labelNum = ps.footprintcount;     break;
        case PLAYER_FOOTPRINTPAL:       labelNum = ps.footprintpal;       break;
        case PLAYER_FOOTPRINTSHADE:     labelNum = ps.footprintshade;     break;
        case PLAYER_FRAG:               labelNum = ps.frag;               break;
        case PLAYER_FRAG_PS:            labelNum = ps.frag_ps;            break;
        case PLAYER_FRAGGEDSELF:        labelNum = ps.fraggedself;        break;
        case PLAYER_FTA:                labelNum = ps.fta;                break;
        case PLAYER_FTQ:                labelNum = ps.ftq;                break;
        case PLAYER_GM:                 labelNum = ps.gm;                 break;
        case PLAYER_GOT_ACCESS:         labelNum = ps.got_access;         break;
        case PLAYER_HARD_LANDING:       labelNum = ps.hard_landing;       break;
        case PLAYER_HBOMB_HOLD_DELAY:   labelNum = ps.hbomb_hold_delay;   break;
        case PLAYER_HBOMB_ON:           labelNum = ps.hbomb_on;           break;
        case PLAYER_HEAT_ON:            labelNum = ps.heat_on;            break;
        case PLAYER_HOLODUKE_ON:        labelNum = ps.holoduke_on;        break;
        case PLAYER_HOLSTER_WEAPON:     labelNum = ps.holster_weapon;     break;
        case PLAYER_HUDPAL:             labelNum = P_GetHudPal(&ps);      break;
        case PLAYER_HURT_DELAY:         labelNum = ps.hurt_delay;         break;
        case PLAYER_I:                  labelNum = ps.i;                  break;
        case PLAYER_INDEX:              labelNum = playerNum;             break;
        case PLAYER_INTERFACE_TOGGLE:   labelNum = ps.interface_toggle;   break;
        case PLAYER_INVDISPTIME:        labelNum = ps.invdisptime;        break;
        case PLAYER_INVEN_ICON:         labelNum = ps.inven_icon;         break;
        case PLAYER_JETPACK_ON:         labelNum = ps.jetpack_on;         break;
        case PLAYER_JUMPING_COUNTER:    labelNum = ps.jumping_counter;    break;
        case PLAYER_JUMPING_TOGGLE:     labelNum = ps.jumping_toggle;     break;
        case PLAYER_KICKBACK_PIC:       labelNum = ps.kickback_pic;       break;
        case PLAYER_KNEE_INCS:          labelNum = ps.knee_incs;          break;
        case PLAYER_KNUCKLE_INCS:       labelNum = ps.knuckle_incs;       break;
        case PLAYER_LAST_EXTRA:         labelNum = ps.last_extra;         break;
        case PLAYER_LAST_FULL_WEAPON:   labelNum = ps.last_full_weapon;   break;
        case PLAYER_LAST_PISSED_TIME:   labelNum = ps.last_pissed_time;   break;
        case PLAYER_LAST_QUICK_KICK:    labelNum = ps.last_quick_kick;    break;
        case PLAYER_LAST_USED_WEAPON:   labelNum = ps.last_used_weapon;   break;
        case PLAYER_LAST_WEAPON:        labelNum = ps.last_weapon;        break;
        case PLAYER_LOOGCNT:            labelNum = ps.loogcnt;            break;
        case PLAYER_LOOGIEX:            labelNum = ps.loogiex[lParm2];    break;
        case PLAYER_LOOGIEY:            labelNum = ps.loogiey[lParm2];    break;
        case PLAYER_LOOK_ANG:           labelNum = ps.look_ang;           break;
        case PLAYER_MAX_ACTORS_KILLED:  labelNum = ps.max_actors_killed;  break;
        case PLAYER_MAX_PLAYER_HEALTH:  labelNum = ps.max_player_health;  break;
        case PLAYER_MAX_SECRET_ROOMS:   labelNum = ps.max_secret_rooms;   break;
        case PLAYER_MAX_SHIELD_AMOUNT:  labelNum = ps.max_shield_amount;  break;
        case PLAYER_MOVEMENT_LOCK:      labelNum = ps.movement_lock;      break;
        case PLAYER_NEWOWNER:           labelNum = ps.newowner;           break;
        case PLAYER_NUMLOOGS:           labelNum = ps.numloogs;           break;
        case PLAYER_ON_CRANE:           labelNum = ps.on_crane;           break;
        case PLAYER_ON_GROUND:          labelNum = ps.on_ground;          break;
        case PLAYER_ON_WARPING_SECTOR:  labelNum = ps.on_warping_sector;  break;
        case PLAYER_ONE_EIGHTY_COUNT:   labelNum = ps.one_eighty_count;   break;
        case PLAYER_PARALLAX_SECTNUM:   labelNum = ps.parallax_sectnum;   break;
        case PLAYER_OPOSX:              labelNum = ps.opos.x;             break;
        case PLAYER_OPOSY:              labelNum = ps.opos.y;             break;
        case PLAYER_OPOSZ:              labelNum = ps.opos.z;             break;
        case PLAYER_OPYOFF:             labelNum = ps.opyoff;             break;
        case PLAYER_OVER_SHOULDER_ON:   labelNum = ps.over_shoulder_on;   break;
        case PLAYER_PALETTE:            labelNum = ps.palette;            break;
        case PLAYER_PALOOKUP:           labelNum = ps.palookup;           break;
        case PLAYER_PALS_TIME:          labelNum = ps.pals.f;             break;
        case PLAYER_PLAYER_PAR:         labelNum = ps.player_par;         break;
        case PLAYER_POSX:               labelNum = ps.pos.x;              break;
        case PLAYER_POSXV:              labelNum = ps.vel.x;              break;
        case PLAYER_POSY:               labelNum = ps.pos.y;              break;
        case PLAYER_POSYV:              labelNum = ps.vel.y;              break;
        case PLAYER_POSZ:               labelNum = ps.pos.z;              break;
        case PLAYER_POSZV:              labelNum = ps.vel.z;              break;
        case PLAYER_PYCOUNT:            labelNum = ps.pycount;            break;
        case PLAYER_PYOFF:              labelNum = ps.pyoff;              break;
        case PLAYER_QUICK_KICK:         labelNum = ps.quick_kick;         break;
        case PLAYER_RANDOM_CLUB_FRAME:  labelNum = ps.random_club_frame;  break;
        case PLAYER_RAPID_FIRE_HOLD:    labelNum = ps.rapid_fire_hold;    break;
        case PLAYER_RELOADING:          labelNum = ps.reloading;          break;
        case PLAYER_RETURN_TO_CENTER:   labelNum = ps.return_to_center;   break;
        case PLAYER_ROTSCRNANG:         labelNum = ps.rotscrnang;         break;
        case PLAYER_RUNSPEED:           labelNum = ps.runspeed;           break;
        case PLAYER_SBS:                labelNum = ps.sbs;                break;
        case PLAYER_SCREAM_VOICE:       labelNum = ps.scream_voice;       break;
        case PLAYER_SCUBA_ON:           labelNum = ps.scuba_on;           break;
        case PLAYER_SECRET_ROOMS:       labelNum = ps.secret_rooms;       break;
        case PLAYER_SHOW_EMPTY_WEAPON:  labelNum = ps.show_empty_weapon;  break;
        case PLAYER_SOMETHINGONPLAYER:  labelNum = ps.somethingonplayer;  break;
        case PLAYER_SOUND_PITCH:        labelNum = ps.sound_pitch;        break;
        case PLAYER_SPRITEBRIDGE:       labelNum = ps.spritebridge;       break;
        case PLAYER_SUBWEAPON:          labelNum = ps.subweapon;          break;
        case PLAYER_TEAM:               labelNum = ps.team;               break;
        case PLAYER_TIMEBEFOREEXIT:     labelNum = ps.timebeforeexit;     break;
        case PLAYER_TIPINCS:            labelNum = ps.tipincs;            break;
        case PLAYER_TOGGLE_KEY_FLAG:    labelNum = ps.toggle_key_flag;    break;
        case PLAYER_TRANSPORTER_HOLD:   labelNum = ps.transporter_hold;   break;
        case PLAYER_TRUECZ:             labelNum = ps.truecz;             break;
        case PLAYER_TRUEFZ:             labelNum = ps.truefz;             break;
        case PLAYER_VISIBILITY:         labelNum = ps.visibility;         break;
        case PLAYER_WACKEDBYACTOR:      labelNum = ps.wackedbyactor;      break;
        case PLAYER_WALKING_SND_TOGGLE: labelNum = ps.walking_snd_toggle; break;
        case PLAYER_WANTWEAPONFIRE:     labelNum = ps.wantweaponfire;     break;
        case PLAYER_WEAPON_ANG:         labelNum = ps.weapon_ang;         break;
        case PLAYER_WEAPON_POS:         labelNum = ps.weapon_pos;         break;
        case PLAYER_WEAPON_SWAY:        labelNum = ps.weapon_sway;        break;
        case PLAYER_WEAPONSWITCH:       labelNum = ps.weaponswitch;       break;
        case PLAYER_WEAPRECCNT:         labelNum = ps.weapreccnt;         break;
        case PLAYER_WEAPRECS:           labelNum = ps.weaprecs[lParm2];   break;
        case PLAYER_ZOOM:               labelNum = ps.zoom;               break;

        case PLAYER_BOOT_AMOUNT:     labelNum = ps.inv_amount[GET_BOOTS];    break;
        case PLAYER_FIRSTAID_AMOUNT: labelNum = ps.inv_amount[GET_FIRSTAID]; break;
        case PLAYER_HEAT_AMOUNT:     labelNum = ps.inv_amount[GET_HEATS];    break;
        case PLAYER_HOLODUKE_AMOUNT: labelNum = ps.inv_amount[GET_HOLODUKE]; break;
        case PLAYER_JETPACK_AMOUNT:  labelNum = ps.inv_amount[GET_JETPACK];  break;
        case PLAYER_SCUBA_AMOUNT:    labelNum = ps.inv_amount[GET_SCUBA];    break;
        case PLAYER_SHIELD_AMOUNT:   labelNum = ps.inv_amount[GET_SHIELD];   break;
        case PLAYER_STEROIDS_AMOUNT: labelNum = ps.inv_amount[GET_STEROIDS]; break;

        case PLAYER_AMMO_AMOUNT:      labelNum = ps.ammo_amount[lParm2];     break;
        case PLAYER_MAX_AMMO_AMOUNT:  labelNum = ps.max_ammo_amount[lParm2]; break;

        case PLAYER_GOTWEAPON: labelNum = (ps.gotweapon & (1<<lParm2)) != 0; break;

        case PLAYER_PALS:
            switch (lParm2)
            {
                case 0: labelNum = ps.pals.r; break;
                case 1: labelNum = ps.pals.g; break;
                case 2: labelNum = ps.pals.b; break;
            }
            break;

        case PLAYER_CONNECTED: labelNum = g_player[playerNum].playerquitflag; break;
        case PLAYER_FRAGS:
            labelNum = (playerNum == lParm2) ? ps.fraggedself : g_player[playerNum].frags[lParm2]; break;
        case PLAYER_DEATHS: labelNum = g_player[playerNum].frags[playerNum]; break;

        case PLAYER_BSUBWEAPON: labelNum = (ps.subweapon & (1<<lParm2)) != 0; break;

        default: EDUKE32_UNREACHABLE_SECTION(labelNum = -1; break);
    }

    return labelNum;
}

void __fastcall VM_SetPlayer(int const playerNum, int const labelNum, int const lParm2, int32_t const newValue)
{
    if (EDUKE32_PREDICT_FALSE(((unsigned)playerNum >= (unsigned)g_mostConcurrentPlayers)
        || (PlayerLabels[labelNum].flags & LABEL_HASPARM2 && (unsigned)lParm2 >= (unsigned)PlayerLabels[labelNum].maxParm2)))
    {
        CON_ERRPRINTF("%s[%d] invalid for player %d\n", PlayerLabels[labelNum].name, lParm2, playerNum);
        return;
    }

    auto &ps = *g_player[playerNum].ps;

    switch (labelNum)
    {
        case PLAYER_HORIZ:     ps.q16horiz     = fix16_from_int(newValue); break;
        case PLAYER_OHORIZ:    ps.oq16horiz    = fix16_from_int(newValue); break;
        case PLAYER_OHORIZOFF: ps.oq16horizoff = fix16_from_int(newValue); break;
        case PLAYER_ANG:       ps.q16ang       = fix16_from_int(newValue); break;
        case PLAYER_OANG:      ps.oq16ang      = fix16_from_int(newValue); break;
        case PLAYER_ANGVEL:    ps.q16angvel    = fix16_from_int(newValue); break;
        case PLAYER_HORIZOFF:  ps.q16horizoff  = fix16_from_int(newValue); break;

        case PLAYER_Q16HORIZ:     ps.q16horiz     = newValue; break;
        case PLAYER_OQ16HORIZ:    ps.oq16horiz    = newValue; break;
        case PLAYER_OQ16HORIZOFF: ps.oq16horizoff = newValue; break;
        case PLAYER_Q16ANG:       ps.q16ang       = newValue; break;
        case PLAYER_OQ16ANG:      ps.oq16ang      = newValue; break;
        case PLAYER_Q16ANGVEL:    ps.q16angvel    = newValue; break;
        case PLAYER_Q16HORIZOFF:  ps.q16horizoff  = newValue; break;

        case PLAYER_ACCESS_INCS:        ps.access_incs        = newValue; break;
        case PLAYER_ACCESS_SPRITENUM:   ps.access_spritenum   = newValue; break;
        case PLAYER_ACCESS_WALLNUM:     ps.access_wallnum     = newValue; break;
        case PLAYER_ACTORS_KILLED:      ps.actors_killed      = newValue; break;
        case PLAYER_ACTORSQU:           ps.actorsqu           = newValue; break;
        case PLAYER_AIM_MODE:           ps.aim_mode           = newValue; break;
        case PLAYER_AIRLEFT:            ps.airleft            = newValue; break;
        case PLAYER_AUTO_AIM:           ps.auto_aim           = newValue; break;
        case PLAYER_AUTOSTEP:           ps.autostep           = newValue; break;
        case PLAYER_AUTOSTEP_SBW:       ps.autostep_sbw       = newValue; break;
        case PLAYER_BOBCOUNTER:         ps.bobcounter         = newValue; break;
        case PLAYER_BOBPOSX:            ps.bobpos.x           = newValue; break;
        case PLAYER_BOBPOSY:            ps.bobpos.y           = newValue; break;
        case PLAYER_BUTTONPALETTE:      ps.buttonpalette      = newValue; break;
        case PLAYER_CHEAT_PHASE:        ps.cheat_phase        = newValue; break;
        case PLAYER_CLIPDIST:           ps.clipdist           = newValue; break;
        case PLAYER_CRACK_TIME:         ps.crack_time         = newValue; break;
        case PLAYER_CROUCH_TOGGLE:      ps.crouch_toggle      = newValue; break;
        case PLAYER_CURR_WEAPON:        ps.curr_weapon        = newValue; break;
        case PLAYER_CURSECTNUM:         ps.cursectnum         = newValue; break;
        case PLAYER_CUSTOMEXITSOUND:    ps.customexitsound    = newValue; break;
        case PLAYER_DEAD_FLAG:          ps.dead_flag          = newValue; break;
        case PLAYER_DUMMYPLAYERSPRITE:  ps.dummyplayersprite  = newValue; break;
        case PLAYER_EXTRA_EXTRA8:       ps.extra_extra8       = newValue; break;
        case PLAYER_FALLING_COUNTER:    ps.falling_counter    = newValue; break;
        case PLAYER_FIST_INCS:          ps.fist_incs          = newValue; break;
        case PLAYER_FOOTPRINTCOUNT:     ps.footprintcount     = newValue; break;
        case PLAYER_FOOTPRINTPAL:       ps.footprintpal       = newValue; break;
        case PLAYER_FOOTPRINTSHADE:     ps.footprintshade     = newValue; break;
        case PLAYER_FRAG:               ps.frag               = newValue; break;
        case PLAYER_FRAG_PS:            ps.frag_ps            = newValue; break;
        case PLAYER_FRAGGEDSELF:        ps.fraggedself        = newValue; break;
        case PLAYER_FTA:                ps.fta                = newValue; break;
        case PLAYER_FTQ:                ps.ftq                = newValue; break;
        case PLAYER_GOT_ACCESS:         ps.got_access         = newValue; break;
        case PLAYER_HARD_LANDING:       ps.hard_landing       = newValue; break;
        case PLAYER_HBOMB_HOLD_DELAY:   ps.hbomb_hold_delay   = newValue; break;
        case PLAYER_HBOMB_ON:           ps.hbomb_on           = newValue; break;
        case PLAYER_HOLODUKE_ON:        ps.holoduke_on        = newValue; break;
        case PLAYER_HOLSTER_WEAPON:     ps.holster_weapon     = newValue; break;
        case PLAYER_HURT_DELAY:         ps.hurt_delay         = newValue; break;
        case PLAYER_I:                  ps.i                  = newValue; break;
        case PLAYER_INTERFACE_TOGGLE:   ps.interface_toggle   = newValue; break;
        case PLAYER_INVDISPTIME:        ps.invdisptime        = newValue; break;
        case PLAYER_INVEN_ICON:         ps.inven_icon         = newValue; break;
        case PLAYER_JETPACK_ON:         ps.jetpack_on         = newValue; break;
        case PLAYER_JUMPING_COUNTER:    ps.jumping_counter    = newValue; break;
        case PLAYER_JUMPING_TOGGLE:     ps.jumping_toggle     = newValue; break;
        case PLAYER_KICKBACK_PIC:       ps.kickback_pic       = newValue; break;
        case PLAYER_KNEE_INCS:          ps.knee_incs          = newValue; break;
        case PLAYER_KNUCKLE_INCS:       ps.knuckle_incs       = newValue; break;
        case PLAYER_LAST_EXTRA:         ps.last_extra         = newValue; break;
        case PLAYER_LAST_FULL_WEAPON:   ps.last_full_weapon   = newValue; break;
        case PLAYER_LAST_PISSED_TIME:   ps.last_pissed_time   = newValue; break;
        case PLAYER_LAST_QUICK_KICK:    ps.last_quick_kick    = newValue; break;
        case PLAYER_LAST_USED_WEAPON:   ps.last_used_weapon   = newValue; break;
        case PLAYER_LAST_WEAPON:        ps.last_weapon        = newValue; break;
        case PLAYER_LOOGCNT:            ps.loogcnt            = newValue; break;
        case PLAYER_LOOGIEX:            ps.loogiex[lParm2]    = newValue; break;
        case PLAYER_LOOGIEY:            ps.loogiey[lParm2]    = newValue; break;
        case PLAYER_LOOK_ANG:           ps.look_ang           = newValue; break;
        case PLAYER_MAX_ACTORS_KILLED:  ps.max_actors_killed  = newValue; break;
        case PLAYER_MAX_PLAYER_HEALTH:  ps.max_player_health  = newValue; break;
        case PLAYER_MAX_SECRET_ROOMS:   ps.max_secret_rooms   = newValue; break;
        case PLAYER_MAX_SHIELD_AMOUNT:  ps.max_shield_amount  = newValue; break;
        case PLAYER_MOVEMENT_LOCK:      ps.movement_lock      = newValue; break;
        case PLAYER_NEWOWNER:           ps.newowner           = newValue; break;
        case PLAYER_NUMLOOGS:           ps.numloogs           = newValue; break;
        case PLAYER_ON_CRANE:           ps.on_crane           = newValue; break;
        case PLAYER_ON_GROUND:          ps.on_ground          = newValue; break;
        case PLAYER_ON_WARPING_SECTOR:  ps.on_warping_sector  = newValue; break;
        case PLAYER_ONE_EIGHTY_COUNT:   ps.one_eighty_count   = newValue; break;
        case PLAYER_PARALLAX_SECTNUM:   ps.parallax_sectnum   = newValue; break;
        case PLAYER_OPOSX:              ps.opos.x             = newValue; break;
        case PLAYER_OPOSY:              ps.opos.y             = newValue; break;
        case PLAYER_OPOSZ:              ps.opos.z             = newValue; break;
        case PLAYER_OPYOFF:             ps.opyoff             = newValue; break;
        case PLAYER_OVER_SHOULDER_ON:   ps.over_shoulder_on   = newValue; break;
        case PLAYER_PALOOKUP:           ps.palookup           = newValue; break;
        case PLAYER_PALS_TIME:          ps.pals.f             = newValue; break;
        case PLAYER_PLAYER_PAR:         ps.player_par         = newValue; break;
        case PLAYER_POSX:               ps.pos.x              = newValue; break;
        case PLAYER_POSXV:              ps.vel.x              = newValue; break;
        case PLAYER_POSY:               ps.pos.y              = newValue; break;
        case PLAYER_POSYV:              ps.vel.y              = newValue; break;
        case PLAYER_POSZ:               ps.pos.z              = newValue; break;
        case PLAYER_POSZV:              ps.vel.z              = newValue; break;
        case PLAYER_PYCOUNT:            ps.pycount            = newValue; break;
        case PLAYER_PYOFF:              ps.pyoff              = newValue; break;
        case PLAYER_QUICK_KICK:         ps.quick_kick         = newValue; break;
        case PLAYER_RANDOM_CLUB_FRAME:  ps.random_club_frame  = newValue; break;
        case PLAYER_RAPID_FIRE_HOLD:    ps.rapid_fire_hold    = newValue; break;
        case PLAYER_RELOADING:          ps.reloading          = newValue; break;
        case PLAYER_RETURN_TO_CENTER:   ps.return_to_center   = newValue; break;
        case PLAYER_ROTSCRNANG:         ps.rotscrnang         = newValue; break;
        case PLAYER_RUNSPEED:           ps.runspeed           = newValue; break;
        case PLAYER_SBS:                ps.sbs                = newValue; break;
        case PLAYER_SCREAM_VOICE:       ps.scream_voice       = newValue; break;
        case PLAYER_SCUBA_ON:           ps.scuba_on           = newValue; break;
        case PLAYER_SECRET_ROOMS:       ps.secret_rooms       = newValue; break;
        case PLAYER_SHOW_EMPTY_WEAPON:  ps.show_empty_weapon  = newValue; break;
        case PLAYER_SOMETHINGONPLAYER:  ps.somethingonplayer  = newValue; break;
        case PLAYER_SOUND_PITCH:        ps.sound_pitch        = newValue; break;
        case PLAYER_SPRITEBRIDGE:       ps.spritebridge       = newValue; break;
        case PLAYER_SUBWEAPON:          ps.subweapon          = newValue; break;
        case PLAYER_TEAM:               ps.team               = newValue; break;
        case PLAYER_TIMEBEFOREEXIT:     ps.timebeforeexit     = newValue; break;
        case PLAYER_TIPINCS:            ps.tipincs            = newValue; break;
        case PLAYER_TOGGLE_KEY_FLAG:    ps.toggle_key_flag    = newValue; break;
        case PLAYER_TRANSPORTER_HOLD:   ps.transporter_hold   = newValue; break;
        case PLAYER_TRUECZ:             ps.truecz             = newValue; break;
        case PLAYER_TRUEFZ:             ps.truefz             = newValue; break;
        case PLAYER_VISIBILITY:         ps.visibility         = newValue; break;
        case PLAYER_WACKEDBYACTOR:      ps.wackedbyactor      = newValue; break;
        case PLAYER_WALKING_SND_TOGGLE: ps.walking_snd_toggle = newValue; break;
        case PLAYER_WANTWEAPONFIRE:     ps.wantweaponfire     = newValue; break;
        case PLAYER_WEAPON_ANG:         ps.weapon_ang         = newValue; break;
        case PLAYER_WEAPON_POS:         ps.weapon_pos         = newValue; break;
        case PLAYER_WEAPON_SWAY:        ps.weapon_sway        = newValue; break;
        case PLAYER_WEAPONSWITCH:       ps.weaponswitch       = newValue; break;
        case PLAYER_WEAPRECCNT:         ps.weapreccnt         = newValue; break;
        case PLAYER_WEAPRECS:           ps.weaprecs[lParm2]   = newValue; break;
        case PLAYER_ZOOM:               ps.zoom               = newValue; break;

        case PLAYER_BOOT_AMOUNT:     ps.inv_amount[GET_BOOTS]    = newValue; break;
        case PLAYER_FIRSTAID_AMOUNT: ps.inv_amount[GET_FIRSTAID] = newValue; break;
        case PLAYER_HEAT_AMOUNT:     ps.inv_amount[GET_HEATS]    = newValue; break;
        case PLAYER_HOLODUKE_AMOUNT: ps.inv_amount[GET_HOLODUKE] = newValue; break;
        case PLAYER_JETPACK_AMOUNT:  ps.inv_amount[GET_JETPACK]  = newValue; break;
        case PLAYER_SCUBA_AMOUNT:    ps.inv_amount[GET_SCUBA]    = newValue; break;
        case PLAYER_SHIELD_AMOUNT:   ps.inv_amount[GET_SHIELD]   = newValue; break;
        case PLAYER_STEROIDS_AMOUNT: ps.inv_amount[GET_STEROIDS] = newValue; break;

        case PLAYER_AMMO_AMOUNT:     ps.ammo_amount[lParm2]     = newValue; break;
        case PLAYER_MAX_AMMO_AMOUNT: ps.max_ammo_amount[lParm2] = newValue; break;

        case PLAYER_HEAT_ON:
            if (ps.heat_on != newValue)
            {
                ps.heat_on = newValue;
                P_UpdateScreenPal(&ps);
            }
            break;

        case PLAYER_GM:
            if (!(ps.gm & MODE_MENU) && (newValue & MODE_MENU))
                Menu_Open(playerNum);
            else if ((ps.gm & MODE_MENU) && !(newValue & MODE_MENU))
                Menu_Close(playerNum);
            ps.gm = newValue;
            break;

        case PLAYER_GOTWEAPON:
            if (newValue)
                ps.gotweapon |= (1 << lParm2);
            else
                ps.gotweapon &= ~(1 << lParm2);
            break;

        case PLAYER_PALETTE: P_SetGamePalette(&ps, newValue, 2 + 16); break;

        case PLAYER_PALS:
            switch (lParm2)
            {
                case 0: ps.pals.r = newValue; break;
                case 1: ps.pals.g = newValue; break;
                case 2: ps.pals.b = newValue; break;
            }
            break;

        case PLAYER_FRAGS:
            if (playerNum == lParm2)
                ps.fraggedself = newValue;
            else
                g_player[playerNum].frags[lParm2] = newValue;
            break;

        case PLAYER_DEATHS: g_player[playerNum].frags[playerNum] = newValue; break;

        case PLAYER_BSUBWEAPON:
            if (newValue)
                ps.subweapon |= (1 << lParm2);
            else
                ps.subweapon &= ~(1 << lParm2);
            break;
    }
}

const memberlabel_t ProjectileLabels[]=
{
    { "workslike",  PROJ_WORKSLIKE,   0, 0, -1 },
    { "spawns",     PROJ_SPAWNS,      0, 0, -1 },
    { "sxrepeat",   PROJ_SXREPEAT,    0, 0, -1 },
    { "syrepeat",   PROJ_SYREPEAT,    0, 0, -1 },
    { "sound",      PROJ_SOUND,       0, 0, -1 },
    { "isound",     PROJ_ISOUND,      0, 0, -1 },
    { "vel",        PROJ_VEL,         0, 0, -1 },
    { "extra",      PROJ_EXTRA,       0, 0, -1 },
    { "decal",      PROJ_DECAL,       0, 0, -1 },
    { "trail",      PROJ_TRAIL,       0, 0, -1 },
    { "txrepeat",   PROJ_TXREPEAT,    0, 0, -1 },
    { "tyrepeat",   PROJ_TYREPEAT,    0, 0, -1 },
    { "toffset",    PROJ_TOFFSET,     0, 0, -1 },
    { "tnum",       PROJ_TNUM,        0, 0, -1 },
    { "drop",       PROJ_DROP,        0, 0, -1 },
    { "cstat",      PROJ_CSTAT,       0, 0, -1 },
    { "clipdist",   PROJ_CLIPDIST,    0, 0, -1 },
    { "shade",      PROJ_SHADE,       0, 0, -1 },
    { "xrepeat",    PROJ_XREPEAT,     0, 0, -1 },
    { "yrepeat",    PROJ_YREPEAT,     0, 0, -1 },
    { "pal",        PROJ_PAL,         0, 0, -1 },
    { "extra_rand", PROJ_EXTRA_RAND,  0, 0, -1 },
    { "hitradius",  PROJ_HITRADIUS,   0, 0, -1 },
    { "velmult",    PROJ_MOVECNT,     0, 0, -1 },
    { "offset",     PROJ_OFFSET,      0, 0, -1 },
    { "bounces",    PROJ_BOUNCES,     0, 0, -1 },
    { "bsound",     PROJ_BSOUND,      0, 0, -1 },
    { "range",      PROJ_RANGE,       0, 0, -1 },
    { "flashcolor", PROJ_FLASH_COLOR, 0, 0, -1 },
    { "userdata",   PROJ_USERDATA,    0, 0, -1 },
};

int32_t __fastcall VM_GetProjectile(int const tileNum, int32_t labelNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)tileNum >= MAXTILES || g_tile[tileNum].proj == NULL))
    {
        CON_ERRPRINTF("invalid projectile %d\n", tileNum);
        return -1;
    }

    auto const &p = *g_tile[tileNum].proj;

    switch (labelNum)
    {
        case PROJ_BOUNCES:     labelNum = p.bounces;    break;
        case PROJ_BSOUND:      labelNum = p.bsound;     break;
        case PROJ_CLIPDIST:    labelNum = p.clipdist;   break;
        case PROJ_CSTAT:       labelNum = p.cstat;      break;
        case PROJ_DECAL:       labelNum = p.decal;      break;
        case PROJ_DROP:        labelNum = p.drop;       break;
        case PROJ_EXTRA:       labelNum = p.extra;      break;
        case PROJ_EXTRA_RAND:  labelNum = p.extra_rand; break;
        case PROJ_FLASH_COLOR: labelNum = p.flashcolor; break;
        case PROJ_HITRADIUS:   labelNum = p.hitradius;  break;
        case PROJ_ISOUND:      labelNum = p.isound;     break;
        case PROJ_MOVECNT:     labelNum = p.movecnt;    break;
        case PROJ_OFFSET:      labelNum = p.offset;     break;
        case PROJ_PAL:         labelNum = p.pal;        break;
        case PROJ_RANGE:       labelNum = p.range;      break;
        case PROJ_SHADE:       labelNum = p.shade;      break;
        case PROJ_SOUND:       labelNum = p.sound;      break;
        case PROJ_SPAWNS:      labelNum = p.spawns;     break;
        case PROJ_SXREPEAT:    labelNum = p.sxrepeat;   break;
        case PROJ_SYREPEAT:    labelNum = p.syrepeat;   break;
        case PROJ_TNUM:        labelNum = p.tnum;       break;
        case PROJ_TOFFSET:     labelNum = p.toffset;    break;
        case PROJ_TRAIL:       labelNum = p.trail;      break;
        case PROJ_TXREPEAT:    labelNum = p.txrepeat;   break;
        case PROJ_TYREPEAT:    labelNum = p.tyrepeat;   break;
        case PROJ_USERDATA:    labelNum = p.userdata;   break;
        case PROJ_VEL:         labelNum = p.vel;        break;
        case PROJ_WORKSLIKE:   labelNum = p.workslike;  break;
        case PROJ_XREPEAT:     labelNum = p.xrepeat;    break;
        case PROJ_YREPEAT:     labelNum = p.yrepeat;    break;

        default: EDUKE32_UNREACHABLE_SECTION(labelNum = -1; break);
    }

    return labelNum;
}

void __fastcall VM_SetProjectile(int const tileNum, int const labelNum, int32_t const newValue)
{
    if (EDUKE32_PREDICT_FALSE((unsigned) tileNum >= MAXTILES || g_tile[tileNum].proj == NULL))
    {
        CON_ERRPRINTF("invalid projectile %d\n", tileNum);
        return;
    }

    auto &p = *g_tile[tileNum].proj;

    switch (labelNum)
    {
        case PROJ_BOUNCES:     p.bounces    = newValue; break;
        case PROJ_BSOUND:      p.bsound     = newValue; break;
        case PROJ_CLIPDIST:    p.clipdist   = newValue; break;
        case PROJ_CSTAT:       p.cstat      = newValue; break;
        case PROJ_DECAL:       p.decal      = newValue; break;
        case PROJ_DROP:        p.drop       = newValue; break;
        case PROJ_EXTRA:       p.extra      = newValue; break;
        case PROJ_EXTRA_RAND:  p.extra_rand = newValue; break;
        case PROJ_FLASH_COLOR: p.flashcolor = newValue; break;
        case PROJ_HITRADIUS:   p.hitradius  = newValue; break;
        case PROJ_ISOUND:      p.isound     = newValue; break;
        case PROJ_MOVECNT:     p.movecnt    = newValue; break;
        case PROJ_OFFSET:      p.offset     = newValue; break;
        case PROJ_PAL:         p.pal        = newValue; break;
        case PROJ_RANGE:       p.range      = newValue; break;
        case PROJ_SHADE:       p.shade      = newValue; break;
        case PROJ_SOUND:       p.sound      = newValue; break;
        case PROJ_SPAWNS:      p.spawns     = newValue; break;
        case PROJ_SXREPEAT:    p.sxrepeat   = newValue; break;
        case PROJ_SYREPEAT:    p.syrepeat   = newValue; break;
        case PROJ_TNUM:        p.tnum       = newValue; break;
        case PROJ_TOFFSET:     p.toffset    = newValue; break;
        case PROJ_TRAIL:       p.trail      = newValue; break;
        case PROJ_TXREPEAT:    p.txrepeat   = newValue; break;
        case PROJ_TYREPEAT:    p.tyrepeat   = newValue; break;
        case PROJ_USERDATA:    p.userdata   = newValue; break;
        case PROJ_VEL:         p.vel        = newValue; break;
        case PROJ_WORKSLIKE:   p.workslike  = newValue; break;
        case PROJ_XREPEAT:     p.xrepeat    = newValue; break;
        case PROJ_YREPEAT:     p.yrepeat    = newValue; break;
    }
}

int32_t __fastcall VM_GetActiveProjectile(int const spriteNum, int32_t labelNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)spriteNum >= MAXSPRITES))
    {
        CON_ERRPRINTF("%s invalid for projectile %d\n", ProjectileLabels[labelNum].name, spriteNum);
        return -1;
    }

    auto const &p = SpriteProjectile[spriteNum];

    switch (labelNum)
    {
        case PROJ_WORKSLIKE:   labelNum = p.workslike;  break;
        case PROJ_SPAWNS:      labelNum = p.spawns;     break;
        case PROJ_SXREPEAT:    labelNum = p.sxrepeat;   break;
        case PROJ_SYREPEAT:    labelNum = p.syrepeat;   break;
        case PROJ_SOUND:       labelNum = p.sound;      break;
        case PROJ_ISOUND:      labelNum = p.isound;     break;
        case PROJ_VEL:         labelNum = p.vel;        break;
        case PROJ_EXTRA:       labelNum = p.extra;      break;
        case PROJ_DECAL:       labelNum = p.decal;      break;
        case PROJ_TRAIL:       labelNum = p.trail;      break;
        case PROJ_TXREPEAT:    labelNum = p.txrepeat;   break;
        case PROJ_TYREPEAT:    labelNum = p.tyrepeat;   break;
        case PROJ_TOFFSET:     labelNum = p.toffset;    break;
        case PROJ_TNUM:        labelNum = p.tnum;       break;
        case PROJ_DROP:        labelNum = p.drop;       break;
        case PROJ_CSTAT:       labelNum = p.cstat;      break;
        case PROJ_CLIPDIST:    labelNum = p.clipdist;   break;
        case PROJ_SHADE:       labelNum = p.shade;      break;
        case PROJ_XREPEAT:     labelNum = p.xrepeat;    break;
        case PROJ_YREPEAT:     labelNum = p.yrepeat;    break;
        case PROJ_PAL:         labelNum = p.pal;        break;
        case PROJ_EXTRA_RAND:  labelNum = p.extra_rand; break;
        case PROJ_HITRADIUS:   labelNum = p.hitradius;  break;
        case PROJ_MOVECNT:     labelNum = p.movecnt;    break;
        case PROJ_OFFSET:      labelNum = p.offset;     break;
        case PROJ_BOUNCES:     labelNum = p.bounces;    break;
        case PROJ_BSOUND:      labelNum = p.bsound;     break;
        case PROJ_RANGE:       labelNum = p.range;      break;
        case PROJ_FLASH_COLOR: labelNum = p.flashcolor; break;
        case PROJ_USERDATA:    labelNum = p.userdata;   break;

        default: EDUKE32_UNREACHABLE_SECTION(labelNum = -1; break);
    }

    return labelNum;
}

void __fastcall VM_SetActiveProjectile(int const spriteNum, int const labelNum, int32_t const newValue)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)spriteNum >= MAXSPRITES))
    {
        CON_ERRPRINTF("%s invalid for projectile %d\n", ProjectileLabels[labelNum].name, spriteNum);
        return;
    }

    auto &p = SpriteProjectile[spriteNum];

    switch (labelNum)
    {
        case PROJ_WORKSLIKE:   p.workslike  = newValue; break;
        case PROJ_SPAWNS:      p.spawns     = newValue; break;
        case PROJ_SXREPEAT:    p.sxrepeat   = newValue; break;
        case PROJ_SYREPEAT:    p.syrepeat   = newValue; break;
        case PROJ_SOUND:       p.sound      = newValue; break;
        case PROJ_ISOUND:      p.isound     = newValue; break;
        case PROJ_VEL:         p.vel        = newValue; break;
        case PROJ_EXTRA:       p.extra      = newValue; break;
        case PROJ_DECAL:       p.decal      = newValue; break;
        case PROJ_TRAIL:       p.trail      = newValue; break;
        case PROJ_TXREPEAT:    p.txrepeat   = newValue; break;
        case PROJ_TYREPEAT:    p.tyrepeat   = newValue; break;
        case PROJ_TOFFSET:     p.toffset    = newValue; break;
        case PROJ_TNUM:        p.tnum       = newValue; break;
        case PROJ_DROP:        p.drop       = newValue; break;
        case PROJ_CSTAT:       p.cstat      = newValue; break;
        case PROJ_CLIPDIST:    p.clipdist   = newValue; break;
        case PROJ_SHADE:       p.shade      = newValue; break;
        case PROJ_XREPEAT:     p.xrepeat    = newValue; break;
        case PROJ_YREPEAT:     p.yrepeat    = newValue; break;
        case PROJ_PAL:         p.pal        = newValue; break;
        case PROJ_EXTRA_RAND:  p.extra_rand = newValue; break;
        case PROJ_HITRADIUS:   p.hitradius  = newValue; break;
        case PROJ_MOVECNT:     p.movecnt    = newValue; break;
        case PROJ_OFFSET:      p.offset     = newValue; break;
        case PROJ_BOUNCES:     p.bounces    = newValue; break;
        case PROJ_BSOUND:      p.bsound     = newValue; break;
        case PROJ_RANGE:       p.range      = newValue; break;
        case PROJ_FLASH_COLOR: p.flashcolor = newValue; break;
        case PROJ_USERDATA:    p.userdata   = newValue; break;
    }
}

const memberlabel_t UserdefsLabels[]=
{
    { "god",                    USERDEFS_GOD,                    0, 0, -1 },
    { "warp_on",                USERDEFS_WARP_ON,                0, 0, -1 },
    { "cashman",                USERDEFS_CASHMAN,                0, 0, -1 },
    { "eog",                    USERDEFS_EOG,                    0, 0, -1 },
    { "showallmap",             USERDEFS_SHOWALLMAP,             0, 0, -1 },
    { "show_help",              USERDEFS_SHOW_HELP,              0, 0, -1 },
    { "scrollmode",             USERDEFS_SCROLLMODE,             0, 0, -1 },
    { "clipping",               USERDEFS_CLIPPING,               0, 0, -1 },
    { "user_name",              USERDEFS_USER_NAME,              LABEL_HASPARM2, MAXPLAYERS, -1 },
    { "ridecule",               USERDEFS_RIDECULE,               LABEL_HASPARM2 | LABEL_ISSTRING, 10, -1 },
    { "savegame",               USERDEFS_SAVEGAME,               LABEL_HASPARM2 | LABEL_ISSTRING, 10, -1 },
    { "pwlockout",              USERDEFS_PWLOCKOUT,              LABEL_ISSTRING, 128, -1 },
    { "rtsname;",               USERDEFS_RTSNAME,                LABEL_ISSTRING, 128, -1 },
    { "overhead_on",            USERDEFS_OVERHEAD_ON,            0, 0, -1 },
    { "last_overhead",          USERDEFS_LAST_OVERHEAD,          0, 0, -1 },
    { "showweapons",            USERDEFS_SHOWWEAPONS,            0, 0, -1 },
    { "pause_on",               USERDEFS_PAUSE_ON,               0, 0, -1 },
    { "from_bonus",             USERDEFS_FROM_BONUS,             0, 0, -1 },
    { "camerasprite",           USERDEFS_CAMERASPRITE,           0, 0, -1 },
    { "last_camsprite",         USERDEFS_LAST_CAMSPRITE,         0, 0, -1 },
    { "last_level",             USERDEFS_LAST_LEVEL,             0, 0, -1 },
    { "secretlevel",            USERDEFS_SECRETLEVEL,            0, 0, -1 },
    { "const_visibility",       USERDEFS_CONST_VISIBILITY,       0, 0, -1 },
    { "uw_framerate",           USERDEFS_UW_FRAMERATE,           0, 0, -1 },
    { "camera_time",            USERDEFS_CAMERA_TIME,            0, 0, -1 },
    { "folfvel",                USERDEFS_FOLFVEL,                0, 0, -1 },
    { "folavel",                USERDEFS_FOLAVEL,                0, 0, -1 },
    { "folx",                   USERDEFS_FOLX,                   0, 0, -1 },
    { "foly",                   USERDEFS_FOLY,                   0, 0, -1 },
    { "fola",                   USERDEFS_FOLA,                   0, 0, -1 },
    { "reccnt",                 USERDEFS_RECCNT,                 0, 0, -1 },
    { "entered_name",           USERDEFS_ENTERED_NAME,           0, 0, -1 },
    { "screen_tilting",         USERDEFS_SCREEN_TILTING,         0, 0, -1 },
    { "shadows",                USERDEFS_SHADOWS,                0, 0, -1 },
    { "fta_on",                 USERDEFS_FTA_ON,                 0, 0, -1 },
    { "executions",             USERDEFS_EXECUTIONS,             0, 0, -1 },
    { "auto_run",               USERDEFS_AUTO_RUN,               0, 0, -1 },
    { "coords",                 USERDEFS_COORDS,                 0, 0, -1 },
    { "tickrate",               USERDEFS_TICKRATE,               0, 0, -1 },
    { "m_coop",                 USERDEFS_M_COOP,                 0, 0, -1 },
    { "coop",                   USERDEFS_COOP,                   0, 0, -1 },
    { "screen_size",            USERDEFS_SCREEN_SIZE,            0, 0, -1 },
    { "lockout",                USERDEFS_LOCKOUT,                0, 0, -1 },
    { "crosshair",              USERDEFS_CROSSHAIR,              0, 0, -1 },
    { "playerai",               USERDEFS_PLAYERAI,               0, 0, -1 },
    { "respawn_monsters",       USERDEFS_RESPAWN_MONSTERS,       0, 0, -1 },
    { "respawn_items",          USERDEFS_RESPAWN_ITEMS,          0, 0, -1 },
    { "respawn_inventory",      USERDEFS_RESPAWN_INVENTORY,      0, 0, -1 },
    { "recstat",                USERDEFS_RECSTAT,                0, 0, -1 },
    { "monsters_off",           USERDEFS_MONSTERS_OFF,           0, 0, -1 },
    { "brightness",             USERDEFS_BRIGHTNESS,             0, 0, -1 },
    { "m_respawn_items",        USERDEFS_M_RESPAWN_ITEMS,        0, 0, -1 },
    { "m_respawn_monsters",     USERDEFS_M_RESPAWN_MONSTERS,     0, 0, -1 },
    { "m_respawn_inventory",    USERDEFS_M_RESPAWN_INVENTORY,    0, 0, -1 },
    { "m_recstat",              USERDEFS_M_RECSTAT,              0, 0, -1 },
    { "m_monsters_off",         USERDEFS_M_MONSTERS_OFF,         0, 0, -1 },
    { "detail",                 USERDEFS_DETAIL,                 0, 0, -1 },
    { "m_ffire",                USERDEFS_M_FFIRE,                0, 0, -1 },
    { "ffire",                  USERDEFS_FFIRE,                  0, 0, -1 },
    { "m_player_skill",         USERDEFS_M_PLAYER_SKILL,         0, 0, -1 },
    { "m_level_number",         USERDEFS_M_LEVEL_NUMBER,         0, 0, -1 },
    { "m_volume_number",        USERDEFS_M_VOLUME_NUMBER,        0, 0, -1 },
    { "multimode",              USERDEFS_MULTIMODE,              0, 0, -1 },
    { "player_skill",           USERDEFS_PLAYER_SKILL,           0, 0, -1 },
    { "level_number",           USERDEFS_LEVEL_NUMBER,           0, 0, -1 },
    { "volume_number",          USERDEFS_VOLUME_NUMBER,          0, 0, -1 },
    { "m_marker",               USERDEFS_M_MARKER,               0, 0, -1 },
    { "marker",                 USERDEFS_MARKER,                 0, 0, -1 },
    { "mouseflip",              USERDEFS_MOUSEFLIP,              0, 0, -1 },
    { "statusbarscale",         USERDEFS_STATUSBARSCALE,         0, 0, -1 },
    { "drawweapon",             USERDEFS_DRAWWEAPON,             0, 0, -1 },
    { "mouseaiming",            USERDEFS_MOUSEAIMING,            0, 0, -1 },
    { "weaponswitch",           USERDEFS_WEAPONSWITCH,           0, 0, -1 },
    { "democams",               USERDEFS_DEMOCAMS,               0, 0, -1 },
    { "color",                  USERDEFS_COLOR,                  0, 0, -1 },
    { "msgdisptime",            USERDEFS_MSGDISPTIME,            0, 0, -1 },
    { "statusbarmode",          USERDEFS_STATUSBARMODE,          0, 0, -1 },
    { "m_noexits",              USERDEFS_M_NOEXITS,              0, 0, -1 },
    { "noexits",                USERDEFS_NOEXITS,                0, 0, -1 },
    { "autovote",               USERDEFS_AUTOVOTE,               0, 0, -1 },
    { "automsg",                USERDEFS_AUTOMSG,                0, 0, -1 },
    { "idplayers",              USERDEFS_IDPLAYERS,              0, 0, -1 },
    { "team",                   USERDEFS_TEAM,                   0, 0, -1 },
    { "viewbob",                USERDEFS_VIEWBOB,                0, 0, -1 },
    { "weaponsway",             USERDEFS_WEAPONSWAY,             0, 0, -1 },
    { "angleinterpolation",     USERDEFS_ANGLEINTERPOLATION,     0, 0, -1 },
    { "obituaries",             USERDEFS_OBITUARIES,             0, 0, -1 },
    { "levelstats",             USERDEFS_LEVELSTATS,             0, 0, -1 },
    { "crosshairscale",         USERDEFS_CROSSHAIRSCALE,         0, 0, -1 },
    { "althud",                 USERDEFS_ALTHUD,                 0, 0, -1 },
    { "display_bonus_screen",   USERDEFS_DISPLAY_BONUS_SCREEN,   0, 0, -1 },
    { "show_level_text",        USERDEFS_SHOW_LEVEL_TEXT,        0, 0, -1 },
    { "weaponscale",            USERDEFS_WEAPONSCALE,            0, 0, -1 },
    { "textscale",              USERDEFS_TEXTSCALE,              0, 0, -1 },
    { "runkey_mode",            USERDEFS_RUNKEY_MODE,            0, 0, -1 },
    { "m_origin_x",             USERDEFS_M_ORIGIN_X,             0, 0, -1 },
    { "m_origin_y",             USERDEFS_M_ORIGIN_Y,             0, 0, -1 },
    { "playerbest",             USERDEFS_PLAYERBEST,             0, 0, -1 },
    { "musictoggle",            USERDEFS_MUSICTOGGLE,            0, 0, -1 },
    { "usevoxels",              USERDEFS_USEVOXELS,              0, 0, -1 },
    { "usehightile",            USERDEFS_USEHIGHTILE,            0, 0, -1 },
    { "usemodels",              USERDEFS_USEMODELS,              0, 0, -1 },
    { "gametypeflags",          USERDEFS_GAMETYPEFLAGS,          0, 0, -1 },
    { "m_gametypeflags",        USERDEFS_M_GAMETYPEFLAGS,        0, 0, -1 },
    { "globalflags",            USERDEFS_GLOBALFLAGS,            0, 0, -1 },
    { "globalgameflags",        USERDEFS_GLOBALGAMEFLAGS,        0, 0, -1 },
    { "vm_player",              USERDEFS_VM_PLAYER,              0, 0, -1 },
    { "vm_sprite",              USERDEFS_VM_SPRITE,              0, 0, -1 },
    { "vm_distance",            USERDEFS_VM_DISTANCE,            0, 0, -1 },
    { "soundtoggle",            USERDEFS_SOUNDTOGGLE,            0, 0, -1 },
    { "gametext_tracking",      USERDEFS_GAMETEXT_TRACKING,      0, 0, -1 },
    { "mgametext_tracking",     USERDEFS_MGAMETEXT_TRACKING,     0, 0, -1 },
    { "menutext_tracking",      USERDEFS_MENUTEXT_TRACKING,      0, 0, -1 },
    { "maxspritesonscreen",     USERDEFS_MAXSPRITESONSCREEN,     0, 0, -1 },
    { "screenarea_x1",          USERDEFS_SCREENAREA_X1,          0, 0, -1 },
    { "screenarea_y1",          USERDEFS_SCREENAREA_Y1,          0, 0, -1 },
    { "screenarea_x2",          USERDEFS_SCREENAREA_X2,          0, 0, -1 },
    { "screenarea_y2",          USERDEFS_SCREENAREA_Y2,          0, 0, -1 },
    { "screenfade",             USERDEFS_SCREENFADE,             0, 0, -1 },
    { "menubackground",         USERDEFS_MENUBACKGROUND,         0, 0, -1 },
    { "statusbarflags",         USERDEFS_STATUSBARFLAGS,         0, 0, -1 },
    { "statusbarrange",         USERDEFS_STATUSBARRANGE,         0, 0, -1 },
    { "statusbarcustom",        USERDEFS_STATUSBARCUSTOM,        0, 0, -1 },
    { "hudontop",               USERDEFS_HUDONTOP,               0, 0, -1 },
    { "menu_slidebarz",         USERDEFS_MENU_SLIDEBARZ,         0, 0, -1 },
    { "menu_slidebarmargin",    USERDEFS_MENU_SLIDEBARMARGIN,    0, 0, -1 },
    { "menu_slidecursorz",      USERDEFS_MENU_SLIDECURSORZ,      0, 0, -1 },
    { "global_r",               USERDEFS_GLOBAL_R,               0, 0, -1 },
    { "global_g",               USERDEFS_GLOBAL_G,               0, 0, -1 },
    { "global_b",               USERDEFS_GLOBAL_B,               0, 0, -1 },
    { "default_volume",         USERDEFS_DEFAULT_VOLUME,         0, 0, -1 },
    { "default_skill",          USERDEFS_DEFAULT_SKILL,          0, 0, -1 },
    { "menu_shadedeselected",   USERDEFS_MENU_SHADEDESELECTED,   0, 0, -1 },
    { "menu_shadedisabled",     USERDEFS_MENU_SHADEDISABLED,     0, 0, -1 },
    { "menutext_zoom",          USERDEFS_MENUTEXT_ZOOM,          0, 0, -1 },
    { "menutext_xspace",        USERDEFS_MENUTEXT_XSPACE,        0, 0, -1 },
    { "menutext_pal",           USERDEFS_MENUTEXT_PAL,           0, 0, -1 },
    { "menutext_palselected",   USERDEFS_MENUTEXT_PALSELECTED,   0, 0, -1 },
    { "menutext_paldeselected", USERDEFS_MENUTEXT_PALDESELECTED, 0, 0, -1 },
    { "menutext_paldisabled",   USERDEFS_MENUTEXT_PALDISABLED,   0, 0, -1 },
    { "menutext_palselected_right",   USERDEFS_MENUTEXT_PALSELECTED_RIGHT,   0, 0, -1 },
    { "menutext_paldeselected_right", USERDEFS_MENUTEXT_PALDESELECTED_RIGHT, 0, 0, -1 },
    { "menutext_paldisabled_right",   USERDEFS_MENUTEXT_PALDISABLED_RIGHT,   0, 0, -1 },
    { "gametext_zoom",          USERDEFS_GAMETEXT_ZOOM,          0, 0, -1 },
    { "gametext_xspace",        USERDEFS_GAMETEXT_XSPACE,        0, 0, -1 },
    { "gametext_pal",           USERDEFS_GAMETEXT_PAL,           0, 0, -1 },
    { "gametext_palselected",   USERDEFS_GAMETEXT_PALSELECTED,   0, 0, -1 },
    { "gametext_paldeselected", USERDEFS_GAMETEXT_PALDESELECTED, 0, 0, -1 },
    { "gametext_paldisabled",   USERDEFS_GAMETEXT_PALDISABLED,   0, 0, -1 },
    { "gametext_palselected_right",   USERDEFS_GAMETEXT_PALSELECTED_RIGHT,   0, 0, -1 },
    { "gametext_paldeselected_right", USERDEFS_GAMETEXT_PALDESELECTED_RIGHT, 0, 0, -1 },
    { "gametext_paldisabled_right",   USERDEFS_GAMETEXT_PALDISABLED_RIGHT,   0, 0, -1 },
    { "minitext_zoom",          USERDEFS_MINITEXT_ZOOM,          0, 0, -1 },
    { "minitext_xspace",        USERDEFS_MINITEXT_XSPACE,        0, 0, -1 },
    { "minitext_tracking",      USERDEFS_MINITEXT_TRACKING,      0, 0, -1 },
    { "minitext_pal",           USERDEFS_MINITEXT_PAL,           0, 0, -1 },
    { "minitext_palselected",   USERDEFS_MINITEXT_PALSELECTED,   0, 0, -1 },
    { "minitext_paldeselected", USERDEFS_MINITEXT_PALDESELECTED, 0, 0, -1 },
    { "minitext_paldisabled",   USERDEFS_MINITEXT_PALDISABLED,   0, 0, -1 },
    { "minitext_palselected_right",   USERDEFS_MINITEXT_PALSELECTED_RIGHT,   0, 0, -1 },
    { "minitext_paldeselected_right", USERDEFS_MINITEXT_PALDESELECTED_RIGHT, 0, 0, -1 },
    { "minitext_paldisabled_right",   USERDEFS_MINITEXT_PALDISABLED_RIGHT,   0, 0, -1 },
    { "menutitle_pal",          USERDEFS_MENUTITLE_PAL,          0, 0, -1 },
    { "slidebar_palselected",   USERDEFS_SLIDEBAR_PALSELECTED,   0, 0, -1 },
    { "slidebar_paldisabled",   USERDEFS_SLIDEBAR_PALDISABLED,   0, 0, -1 },
    { "user_map",               USERDEFS_USER_MAP,               0, 0, -1 },
    { "m_user_map",             USERDEFS_M_USER_MAP,             0, 0, -1 },
    { "music_episode",          USERDEFS_MUSIC_EPISODE,          0, 0, -1 },
    { "music_level",            USERDEFS_MUSIC_LEVEL,            0, 0, -1 },
    { "shadow_pal",             USERDEFS_SHADOW_PAL,             0, 0, -1 },
    { "menu_scrollbartilenum",  USERDEFS_MENU_SCROLLBARTILENUM,  0, 0, -1 },
    { "menu_scrollbarz",        USERDEFS_MENU_SCROLLBARZ,        0, 0, -1 },
    { "menu_scrollcursorz",     USERDEFS_MENU_SCROLLCURSORZ,     0, 0, -1 },
    { "return",                 USERDEFS_RETURN,                 LABEL_HASPARM2, MAX_RETURN_VALUES, -1 },
    { "userbyteversion",        USERDEFS_USERBYTEVERSION,        0, 0, -1 },
    { "autosave",               USERDEFS_AUTOSAVE,               0, 0, -1 },
    { "draw_y",                 USERDEFS_DRAW_Y,                 0, 0, -1 },
    { "draw_yxaspect",          USERDEFS_DRAW_YXASPECT,          0, 0, -1 },
    { "fov",                    USERDEFS_FOV,                    0, 0, -1 },
    { "newgamecustomopen",      USERDEFS_NEWGAMECUSTOMOPEN,      0, 0, -1 },
    { "newgamecustomsubopen",   USERDEFS_NEWGAMECUSTOMSUBOPEN,   LABEL_HASPARM2, MAXMENUGAMEPLAYENTRIES, -1 },
    { "gamepadactive",          USERDEFS_GAMEPADACTIVE,          0, 0, -1 },
};

int32_t __fastcall VM_GetUserdef(int32_t labelNum, int const lParm2)
{
    if (EDUKE32_PREDICT_FALSE(UserdefsLabels[labelNum].flags & LABEL_HASPARM2 && (unsigned) lParm2 >= (unsigned) UserdefsLabels[labelNum].maxParm2))
    {
        CON_ERRPRINTF("%s[%d] invalid for userdef", UserdefsLabels[labelNum].name, lParm2);
        return -1;
    }

    switch (labelNum)
    {
        case USERDEFS_GOD:                    labelNum = ud.god;                          break;
        case USERDEFS_WARP_ON:                labelNum = ud.warp_on;                      break;
        case USERDEFS_CASHMAN:                labelNum = ud.cashman;                      break;
        case USERDEFS_EOG:                    labelNum = ud.eog;                          break;
        case USERDEFS_SHOWALLMAP:             labelNum = ud.showallmap;                   break;
        case USERDEFS_SHOW_HELP:              labelNum = ud.show_help;                    break;
        case USERDEFS_SCROLLMODE:             labelNum = ud.scrollmode;                   break;
        case USERDEFS_CLIPPING:               labelNum = ud.noclip;                       break;
        //  case USERDEFS_USER_NAME:          labelNum = ud.user_name[MAXPLAYERS][32];    break;
        //  case USERDEFS_RIDECULE:           labelNum = ud.ridecule;                     break;
        //  case USERDEFS_PWLOCKOUT:          labelNum = ud.pwlockout;                    break;
        //  case USERDEFS_RTSNAME:            labelNum = ud.rtsname;                      break;
        case USERDEFS_OVERHEAD_ON:            labelNum = ud.overhead_on;                  break;
        case USERDEFS_LAST_OVERHEAD:          labelNum = ud.last_overhead;                break;
        case USERDEFS_SHOWWEAPONS:            labelNum = ud.showweapons;                  break;
        case USERDEFS_PAUSE_ON:               labelNum = ud.pause_on;                     break;
        case USERDEFS_FROM_BONUS:             labelNum = ud.from_bonus;                   break;
        case USERDEFS_CAMERASPRITE:           labelNum = ud.camerasprite;                 break;
        case USERDEFS_LAST_CAMSPRITE:         labelNum = ud.last_camsprite;               break;
        case USERDEFS_LAST_LEVEL:             labelNum = ud.last_level;                   break;
        case USERDEFS_SECRETLEVEL:            labelNum = ud.secretlevel;                  break;
        case USERDEFS_CONST_VISIBILITY:       labelNum = ud.const_visibility;             break;
        case USERDEFS_UW_FRAMERATE:           labelNum = ud.uw_framerate;                 break;
        case USERDEFS_CAMERA_TIME:            labelNum = ud.camera_time;                  break;
        case USERDEFS_FOLFVEL:                labelNum = ud.folfvel;                      break;
        case USERDEFS_FOLAVEL:                labelNum = ud.folavel;                      break;
        case USERDEFS_FOLX:                   labelNum = ud.folx;                         break;
        case USERDEFS_FOLY:                   labelNum = ud.foly;                         break;
        case USERDEFS_FOLA:                   labelNum = ud.fola;                         break;
        case USERDEFS_RECCNT:                 labelNum = ud.reccnt;                       break;
        case USERDEFS_ENTERED_NAME:           labelNum = ud.entered_name;                 break;
        case USERDEFS_SCREEN_TILTING:         labelNum = ud.screen_tilting;               break;
        case USERDEFS_SHADOWS:                labelNum = ud.shadows;                      break;
        case USERDEFS_FTA_ON:                 labelNum = ud.fta_on;                       break;
        case USERDEFS_EXECUTIONS:             labelNum = ud.executions;                   break;
        case USERDEFS_AUTO_RUN:               labelNum = ud.auto_run;                     break;
        case USERDEFS_COORDS:                 labelNum = ud.coords;                       break;
        case USERDEFS_TICKRATE:               labelNum = ud.showfps;                      break;
        case USERDEFS_M_COOP:                 labelNum = ud.m_coop;                       break;
        case USERDEFS_COOP:                   labelNum = ud.coop;                         break;
        case USERDEFS_SCREEN_SIZE:            labelNum = ud.screen_size;                  break;
        case USERDEFS_LOCKOUT:                labelNum = ud.lockout;                      break;
        case USERDEFS_CROSSHAIR:              labelNum = ud.crosshair;                    break;
        case USERDEFS_PLAYERAI:               labelNum = ud.playerai;                     break;
        case USERDEFS_RESPAWN_MONSTERS:       labelNum = ud.respawn_monsters;             break;
        case USERDEFS_RESPAWN_ITEMS:          labelNum = ud.respawn_items;                break;
        case USERDEFS_RESPAWN_INVENTORY:      labelNum = ud.respawn_inventory;            break;
        case USERDEFS_RECSTAT:                labelNum = ud.recstat;                      break;
        case USERDEFS_MONSTERS_OFF:           labelNum = ud.monsters_off;                 break;
        case USERDEFS_BRIGHTNESS:             labelNum = ud.brightness;                   break;
        case USERDEFS_M_RESPAWN_ITEMS:        labelNum = ud.m_respawn_items;              break;
        case USERDEFS_M_RESPAWN_MONSTERS:     labelNum = ud.m_respawn_monsters;           break;
        case USERDEFS_M_RESPAWN_INVENTORY:    labelNum = ud.m_respawn_inventory;          break;
        case USERDEFS_M_RECSTAT:              labelNum = ud.m_recstat;                    break;
        case USERDEFS_M_MONSTERS_OFF:         labelNum = ud.m_monsters_off;               break;
        case USERDEFS_DETAIL:                 labelNum = ud.detail;                       break;
        case USERDEFS_M_FFIRE:                labelNum = ud.m_ffire;                      break;
        case USERDEFS_FFIRE:                  labelNum = ud.ffire;                        break;
        case USERDEFS_M_PLAYER_SKILL:         labelNum = ud.m_player_skill;               break;
        case USERDEFS_M_LEVEL_NUMBER:         labelNum = ud.m_level_number;               break;
        case USERDEFS_M_VOLUME_NUMBER:        labelNum = ud.m_volume_number;              break;
        case USERDEFS_M_USER_MAP:             labelNum = Menu_HaveUserMap();              break;
        case USERDEFS_MULTIMODE:              labelNum = ud.multimode;                    break;
        case USERDEFS_PLAYER_SKILL:           labelNum = ud.player_skill;                 break;
        case USERDEFS_LEVEL_NUMBER:           labelNum = ud.level_number;                 break;
        case USERDEFS_VOLUME_NUMBER:          labelNum = ud.volume_number;                break;
        case USERDEFS_USER_MAP:               labelNum = G_HaveUserMap();                 break;
        case USERDEFS_M_MARKER:               labelNum = ud.m_marker;                     break;
        case USERDEFS_MARKER:                 labelNum = ud.marker;                       break;
        case USERDEFS_MOUSEFLIP:              labelNum = ud.mouseflip;                    break;
        case USERDEFS_STATUSBARSCALE:         labelNum = ud.statusbarscale;               break;
        case USERDEFS_DRAWWEAPON:             labelNum = ud.drawweapon;                   break;
        case USERDEFS_MOUSEAIMING:            labelNum = ud.mouseaiming;                  break;
        case USERDEFS_WEAPONSWITCH:           labelNum = ud.weaponswitch;                 break;
        case USERDEFS_DEMOCAMS:               labelNum = ud.democams;                     break;
        case USERDEFS_COLOR:                  labelNum = ud.color;                        break;
        case USERDEFS_MSGDISPTIME:            labelNum = ud.msgdisptime;                  break;
        case USERDEFS_STATUSBARMODE:          labelNum = ud.statusbarmode;                break;
        case USERDEFS_M_NOEXITS:              labelNum = ud.m_noexits;                    break;
        case USERDEFS_NOEXITS:                labelNum = ud.noexits;                      break;
        case USERDEFS_AUTOVOTE:               labelNum = ud.autovote;                     break;
        case USERDEFS_AUTOMSG:                labelNum = ud.automsg;                      break;
        case USERDEFS_IDPLAYERS:              labelNum = ud.idplayers;                    break;
        case USERDEFS_TEAM:                   labelNum = ud.team;                         break;
        case USERDEFS_VIEWBOB:                labelNum = ud.viewbob;                      break;
        case USERDEFS_WEAPONSWAY:             labelNum = ud.weaponsway;                   break;
        case USERDEFS_ANGLEINTERPOLATION:     labelNum = ud.angleinterpolation;           break;
        case USERDEFS_OBITUARIES:             labelNum = ud.obituaries;                   break;
        case USERDEFS_LEVELSTATS:             labelNum = ud.levelstats;                   break;
        case USERDEFS_CROSSHAIRSCALE:         labelNum = ud.crosshairscale;               break;
        case USERDEFS_ALTHUD:                 labelNum = ud.althud;                       break;
        case USERDEFS_DISPLAY_BONUS_SCREEN:   labelNum = ud.display_bonus_screen;         break;
        case USERDEFS_SHOW_LEVEL_TEXT:        labelNum = ud.show_level_text;              break;
        case USERDEFS_WEAPONSCALE:            labelNum = ud.weaponscale;                  break;
        case USERDEFS_TEXTSCALE:              labelNum = ud.textscale;                    break;
        case USERDEFS_RUNKEY_MODE:            labelNum = ud.runkey_mode;                  break;
        case USERDEFS_M_ORIGIN_X:             labelNum = ud.returnvar[0];                 break;
        case USERDEFS_M_ORIGIN_Y:             labelNum = ud.returnvar[1];                 break;
        case USERDEFS_PLAYERBEST:             labelNum = ud.playerbest;                   break;
        case USERDEFS_MUSICTOGGLE:            labelNum = ud.config.MusicToggle;           break;
        case USERDEFS_USEVOXELS:              labelNum = usevoxels;                       break;
        case USERDEFS_USEHIGHTILE:
#ifdef USE_OPENGL
                                              labelNum = usehightile;                     break;
#endif
        case USERDEFS_USEMODELS:
#ifdef USE_OPENGL
                                              labelNum = usemodels;                       break;
#else
                                              labelNum = 0;                               break;
#endif
        case USERDEFS_GAMETYPEFLAGS:          labelNum = g_gametypeFlags[ud.coop];        break;
        case USERDEFS_M_GAMETYPEFLAGS:        labelNum = g_gametypeFlags[ud.m_coop];      break;
        case USERDEFS_GLOBALFLAGS:            labelNum = globalflags;                     break;
        case USERDEFS_GLOBALGAMEFLAGS:        labelNum = duke3d_globalflags;              break;
        case USERDEFS_VM_PLAYER:              labelNum = vm.playerNum;                    break;
        case USERDEFS_VM_SPRITE:              labelNum = vm.spriteNum;                    break;
        case USERDEFS_VM_DISTANCE:            labelNum = vm.playerDist;                   break;
        case USERDEFS_SOUNDTOGGLE:            labelNum = ud.config.SoundToggle;           break;
        case USERDEFS_GAMETEXT_TRACKING:      labelNum = MF_Bluefont.between.x;           break;
        case USERDEFS_MENUTEXT_TRACKING:      labelNum = MF_Redfont.between.x;            break;
        case USERDEFS_MAXSPRITESONSCREEN:     labelNum = maxspritesonscreen;              break;
        case USERDEFS_SCREENAREA_X1:          labelNum = aGameVars[g_returnVarID].global; break;
        case USERDEFS_SCREENAREA_Y1:          labelNum = ud.returnvar[0];                 break;
        case USERDEFS_SCREENAREA_X2:          labelNum = ud.returnvar[1];                 break;
        case USERDEFS_SCREENAREA_Y2:          labelNum = ud.returnvar[2];                 break;
        case USERDEFS_SCREENFADE:             labelNum = ud.screenfade;                   break;
        case USERDEFS_MENUBACKGROUND:         labelNum = ud.menubackground;               break;
        case USERDEFS_STATUSBARFLAGS:         labelNum = ud.statusbarflags;               break;
        case USERDEFS_STATUSBARRANGE:         labelNum = ud.statusbarrange;               break;
        case USERDEFS_STATUSBARCUSTOM:        labelNum = ud.statusbarcustom;              break;
        case USERDEFS_HUDONTOP:               labelNum = ud.hudontop;                     break;
        case USERDEFS_MENU_SLIDEBARZ:         labelNum = ud.menu_slidebarz;               break;
        case USERDEFS_MENU_SLIDEBARMARGIN:    labelNum = ud.menu_slidebarmargin;          break;
        case USERDEFS_MENU_SLIDECURSORZ:      labelNum = ud.menu_slidecursorz;            break;
        case USERDEFS_GLOBAL_R:               labelNum = globalr;                         break;
        case USERDEFS_GLOBAL_G:               labelNum = globalg;                         break;
        case USERDEFS_GLOBAL_B:               labelNum = globalb;                         break;
        case USERDEFS_DEFAULT_VOLUME:         labelNum = ud.default_volume;               break;
        case USERDEFS_DEFAULT_SKILL:          labelNum = ud.default_skill;                break;
        case USERDEFS_MENU_SHADEDESELECTED:   labelNum = MF_Redfont.shade_deselected;     break;
        case USERDEFS_MENU_SHADEDISABLED:     labelNum = MF_Redfont.shade_disabled;       break;
        case USERDEFS_MENUTEXT_ZOOM:          labelNum = MF_Redfont.zoom;                 break;
        case USERDEFS_MENUTEXT_XSPACE:        labelNum = MF_Redfont.emptychar.x;          break;
        case USERDEFS_MENUTEXT_PALSELECTED:   labelNum = MF_Redfont.pal_selected;         break;
        case USERDEFS_MENUTEXT_PALDESELECTED: labelNum = MF_Redfont.pal_deselected;       break;
        case USERDEFS_MENUTEXT_PALDISABLED:   labelNum = MF_Redfont.pal_disabled;         break;
        case USERDEFS_GAMETEXT_ZOOM:          labelNum = MF_Bluefont.zoom;                break;
        case USERDEFS_GAMETEXT_XSPACE:        labelNum = MF_Bluefont.emptychar.x;         break;
        case USERDEFS_GAMETEXT_PALSELECTED:   labelNum = MF_Bluefont.pal_selected;        break;
        case USERDEFS_GAMETEXT_PALDESELECTED: labelNum = MF_Bluefont.pal_deselected;      break;
        case USERDEFS_GAMETEXT_PALDISABLED:   labelNum = MF_Bluefont.pal_disabled;        break;
        case USERDEFS_MINITEXT_ZOOM:          labelNum = MF_Minifont.zoom;                break;
        case USERDEFS_MINITEXT_XSPACE:        labelNum = MF_Minifont.emptychar.x;         break;
        case USERDEFS_MINITEXT_TRACKING:      labelNum = MF_Minifont.between.x;           break;
        case USERDEFS_MINITEXT_PALSELECTED:   labelNum = MF_Minifont.pal_selected;        break;
        case USERDEFS_MINITEXT_PALDESELECTED: labelNum = MF_Minifont.pal_deselected;      break;
        case USERDEFS_MINITEXT_PALDISABLED:   labelNum = MF_Minifont.pal_disabled;        break;
        case USERDEFS_MENUTITLE_PAL:          labelNum = ud.menutitle_pal;                break;
        case USERDEFS_SLIDEBAR_PALSELECTED:   labelNum = ud.slidebar_palselected;         break;
        case USERDEFS_SLIDEBAR_PALDISABLED:   labelNum = ud.slidebar_paldisabled;         break;
        case USERDEFS_MUSIC_EPISODE:          labelNum = ud.music_episode;                break;
        case USERDEFS_MUSIC_LEVEL:            labelNum = ud.music_level;                  break;
        case USERDEFS_SHADOW_PAL:             labelNum = ud.shadow_pal;                   break;
        case USERDEFS_MENU_SCROLLBARTILENUM:  labelNum = ud.menu_scrollbartilenum;        break;
        case USERDEFS_MENU_SCROLLBARZ:        labelNum = ud.menu_scrollbarz;              break;
        case USERDEFS_MENU_SCROLLCURSORZ:     labelNum = ud.menu_scrollcursorz;           break;
        case USERDEFS_RETURN:
            if (lParm2 == 0)
                labelNum = aGameVars[g_returnVarID].global;
            else
                labelNum = ud.returnvar[lParm2 - 1];
            break;
        case USERDEFS_USERBYTEVERSION:        labelNum = ud.userbytever;                  break;
        case USERDEFS_AUTOSAVE:               labelNum = ud.autosave;                     break;
        case USERDEFS_DRAW_Y:                 labelNum = rotatesprite_y_offset;           break;
        case USERDEFS_DRAW_YXASPECT:          labelNum = rotatesprite_yxaspect;           break;
        case USERDEFS_FOV:                    labelNum = ud.fov;                          break;
        case USERDEFS_GAMEPADACTIVE:          labelNum = (CONTROL_LastSeenInput == LastSeenInput::Joystick); break;

        default: EDUKE32_UNREACHABLE_SECTION(labelNum = -1; break);
    }

    return labelNum;
}

void __fastcall VM_SetUserdef(int const labelNum, int const lParm2, int32_t const iSet)
{
    if (EDUKE32_PREDICT_FALSE(UserdefsLabels[labelNum].flags & LABEL_HASPARM2 && (unsigned)lParm2 >= (unsigned)UserdefsLabels[labelNum].maxParm2))
    {
        CON_ERRPRINTF("%s[%d] invalid for userdef", UserdefsLabels[labelNum].name, lParm2);
        return;
    }

    switch (labelNum)
    {
        case USERDEFS_GOD:                          ud.god                           = iSet; break;
        case USERDEFS_WARP_ON:                      ud.warp_on                       = iSet; break;
        case USERDEFS_CASHMAN:                      ud.cashman                       = iSet; break;
        case USERDEFS_EOG:                          ud.eog                           = iSet; break;
        case USERDEFS_SHOWALLMAP:                   ud.showallmap                    = iSet; break;
        case USERDEFS_SHOW_HELP:                    ud.show_help                     = iSet; break;
        case USERDEFS_SCROLLMODE:                   ud.scrollmode                    = iSet; break;
        case USERDEFS_CLIPPING:                     ud.noclip                        = iSet; break;
        //  case USERDEFS_USER_NAME:                ud.user_name[MAXPLAYERS][32]     = lValue; break;
        //  case USERDEFS_RIDECULE:                 ud.ridecule                      = lValue; break;
        //  case USERDEFS_PWLOCKOUT:                ud.pwlockout                     = lValue; break;
        //  case USERDEFS_RTSNAME:                  ud.rtsname                       = lValue; break;
        case USERDEFS_OVERHEAD_ON:                  ud.overhead_on                   = iSet; break;
        case USERDEFS_LAST_OVERHEAD:                ud.last_overhead                 = iSet; break;
        case USERDEFS_SHOWWEAPONS:                  ud.showweapons                   = iSet; break;
        case USERDEFS_PAUSE_ON:                     ud.pause_on                      = iSet; break;
        case USERDEFS_FROM_BONUS:                   ud.from_bonus                    = iSet; break;
        case USERDEFS_CAMERASPRITE:                 ud.camerasprite                  = iSet; break;
        case USERDEFS_LAST_CAMSPRITE:               ud.last_camsprite                = iSet; break;
        case USERDEFS_LAST_LEVEL:                   ud.last_level                    = iSet; break;
        case USERDEFS_SECRETLEVEL:                  ud.secretlevel                   = iSet; break;
        case USERDEFS_CONST_VISIBILITY:             ud.const_visibility              = iSet; break;
        case USERDEFS_UW_FRAMERATE:                 ud.uw_framerate                  = iSet; break;
        case USERDEFS_CAMERA_TIME:                  ud.camera_time                   = iSet; break;
        case USERDEFS_FOLFVEL:                      ud.folfvel                       = iSet; break;
        case USERDEFS_FOLAVEL:                      ud.folavel                       = iSet; break;
        case USERDEFS_FOLX:                         ud.folx                          = iSet; break;
        case USERDEFS_FOLY:                         ud.foly                          = iSet; break;
        case USERDEFS_FOLA:                         ud.fola                          = iSet; break;
        case USERDEFS_RECCNT:                       ud.reccnt                        = iSet; break;
        case USERDEFS_ENTERED_NAME:                 ud.entered_name                  = iSet; break;
        case USERDEFS_SCREEN_TILTING:               ud.screen_tilting                = iSet; break;
        case USERDEFS_SHADOWS:                      ud.shadows                       = iSet; break;
        case USERDEFS_FTA_ON:                       ud.fta_on                        = iSet; break;
        case USERDEFS_EXECUTIONS:                   ud.executions                    = iSet; break;
        case USERDEFS_AUTO_RUN:                     ud.auto_run                      = iSet; break;
        case USERDEFS_COORDS:                       ud.coords                        = iSet; break;
        case USERDEFS_TICKRATE:                     ud.showfps                       = iSet; break;
        case USERDEFS_M_COOP:                       ud.m_coop                        = iSet; break;
        case USERDEFS_COOP:                         ud.coop                          = iSet; break;
        case USERDEFS_SCREEN_SIZE:
            if (ud.screen_size != iSet)
            {
                ud.screen_size = iSet;
                G_UpdateScreenArea();
            }
            break;
        case USERDEFS_LOCKOUT:                      ud.lockout                       = iSet; break;
        case USERDEFS_CROSSHAIR:                    ud.crosshair                     = iSet; break;
        case USERDEFS_PLAYERAI:                     ud.playerai                      = iSet; break;
        case USERDEFS_RESPAWN_MONSTERS:             ud.respawn_monsters              = iSet; break;
        case USERDEFS_RESPAWN_ITEMS:                ud.respawn_items                 = iSet; break;
        case USERDEFS_RESPAWN_INVENTORY:            ud.respawn_inventory             = iSet; break;
        case USERDEFS_RECSTAT:                      ud.recstat                       = iSet; break;
        case USERDEFS_MONSTERS_OFF:                 ud.monsters_off                  = iSet; break;
        case USERDEFS_BRIGHTNESS:                   ud.brightness                    = iSet; break;
        case USERDEFS_M_RESPAWN_ITEMS:              ud.m_respawn_items               = iSet; break;
        case USERDEFS_M_RESPAWN_MONSTERS:           ud.m_respawn_monsters            = iSet; break;
        case USERDEFS_M_RESPAWN_INVENTORY:          ud.m_respawn_inventory           = iSet; break;
        case USERDEFS_M_RECSTAT:                    ud.m_recstat                     = iSet; break;
        case USERDEFS_M_MONSTERS_OFF:               ud.m_monsters_off                = iSet; break;
        // REMINDER: must implement "boolean" setters like "!!iSet" in Lunatic, too.
        case USERDEFS_DETAIL:                       ud.detail                        = clamp(iSet, 1, 16); break;
        case USERDEFS_M_FFIRE:                      ud.m_ffire                       = iSet; break;
        case USERDEFS_FFIRE:                        ud.ffire                         = iSet; break;
        case USERDEFS_M_PLAYER_SKILL:               ud.m_player_skill                = iSet; break;
        case USERDEFS_M_LEVEL_NUMBER:               ud.m_level_number                = iSet; break;
        case USERDEFS_M_VOLUME_NUMBER:              ud.m_volume_number               = iSet; break;
        case USERDEFS_MULTIMODE:                    ud.multimode                     = iSet; break;
        case USERDEFS_PLAYER_SKILL:                 ud.player_skill                  = iSet; break;
        case USERDEFS_LEVEL_NUMBER:                 ud.level_number                  = iSet; break;
        case USERDEFS_VOLUME_NUMBER:                ud.volume_number                 = iSet; break;
        case USERDEFS_M_MARKER:                     ud.m_marker                      = iSet; break;
        case USERDEFS_MARKER:                       ud.marker                        = iSet; break;
        case USERDEFS_MOUSEFLIP:                    ud.mouseflip                     = iSet; break;
        case USERDEFS_STATUSBARSCALE:               ud.statusbarscale                = iSet; break;
        case USERDEFS_DRAWWEAPON:                   ud.drawweapon                    = iSet; break;
        case USERDEFS_MOUSEAIMING:                  ud.mouseaiming                   = iSet; break;
        case USERDEFS_WEAPONSWITCH:                 ud.weaponswitch                  = iSet; break;
        case USERDEFS_DEMOCAMS:                     ud.democams                      = iSet; break;
        case USERDEFS_COLOR:                        ud.color                         = iSet; break;
        case USERDEFS_MSGDISPTIME:                  ud.msgdisptime                   = iSet; break;
        case USERDEFS_STATUSBARMODE:                ud.statusbarmode                 = iSet; break;
        case USERDEFS_M_NOEXITS:                    ud.m_noexits                     = iSet; break;
        case USERDEFS_NOEXITS:                      ud.noexits                       = iSet; break;
        case USERDEFS_AUTOVOTE:                     ud.autovote                      = iSet; break;
        case USERDEFS_AUTOMSG:                      ud.automsg                       = iSet; break;
        case USERDEFS_IDPLAYERS:                    ud.idplayers                     = iSet; break;
        case USERDEFS_TEAM:                         ud.team                          = iSet; break;
        case USERDEFS_VIEWBOB:                      ud.viewbob                       = iSet; break;
        case USERDEFS_WEAPONSWAY:                   ud.weaponsway                    = iSet; break;
        case USERDEFS_ANGLEINTERPOLATION:           ud.angleinterpolation            = iSet; break;
        case USERDEFS_OBITUARIES:                   ud.obituaries                    = iSet; break;
        case USERDEFS_LEVELSTATS:                   ud.levelstats                    = iSet; break;
        case USERDEFS_CROSSHAIRSCALE:               ud.crosshairscale                = iSet; break;
        case USERDEFS_ALTHUD:                       ud.althud                        = iSet; break;
        case USERDEFS_DISPLAY_BONUS_SCREEN:         ud.display_bonus_screen          = iSet; break;
        case USERDEFS_SHOW_LEVEL_TEXT:              ud.show_level_text               = iSet; break;
        case USERDEFS_WEAPONSCALE:                  ud.weaponscale                   = iSet; break;
        case USERDEFS_TEXTSCALE:                    ud.textscale                     = iSet; break;
        case USERDEFS_RUNKEY_MODE:                  ud.runkey_mode                   = iSet; break;
        case USERDEFS_M_ORIGIN_X:                   ud.returnvar[0]                  = iSet; break;
        case USERDEFS_M_ORIGIN_Y:                   ud.returnvar[1]                  = iSet; break;
        case USERDEFS_GLOBALFLAGS:                  globalflags                      = iSet; break;
        case USERDEFS_GLOBALGAMEFLAGS:              duke3d_globalflags               = iSet; break;
        case USERDEFS_VM_PLAYER:
            vm.playerNum = iSet;
            vm.pPlayer   = g_player[iSet].ps;
            break;
        case USERDEFS_VM_SPRITE:
            vm.spriteNum = iSet;
            vm.pSprite   = &sprite[iSet];
            vm.pActor    = &actor[iSet];
            vm.pData     = &actor[iSet].t_data[0];
            break;
        case USERDEFS_VM_DISTANCE: vm.playerDist                                     = iSet; break;
        case USERDEFS_GAMETEXT_TRACKING:            MF_Bluefont.between.x            = iSet; break;
        case USERDEFS_MENUTEXT_TRACKING:            MF_Redfont.between.x             = iSet; break;
        case USERDEFS_MAXSPRITESONSCREEN:           maxspritesonscreen               = clamp(iSet, MAXSPRITESONSCREEN>>2, MAXSPRITESONSCREEN); break;
        case USERDEFS_SCREENAREA_X1:                aGameVars[g_returnVarID].global  = iSet; break;
        case USERDEFS_SCREENAREA_Y1:                ud.returnvar[0]                  = iSet; break;
        case USERDEFS_SCREENAREA_X2:                ud.returnvar[1]                  = iSet; break;
        case USERDEFS_SCREENAREA_Y2:                ud.returnvar[2]                  = iSet; break;
        case USERDEFS_SCREENFADE:                   ud.screenfade                    = iSet; break;
        case USERDEFS_MENUBACKGROUND:               ud.menubackground                = iSet; break;
        case USERDEFS_STATUSBARFLAGS:               ud.statusbarflags                = iSet; break;
        case USERDEFS_STATUSBARRANGE:               ud.statusbarrange                = iSet; break;
        case USERDEFS_STATUSBARCUSTOM:              ud.statusbarcustom               = iSet; break;
        case USERDEFS_HUDONTOP:                     ud.hudontop                      = iSet; break;
        case USERDEFS_MENU_SLIDEBARZ:               ud.menu_slidebarz                = iSet; break;
        case USERDEFS_MENU_SLIDEBARMARGIN:          ud.menu_slidebarmargin           = iSet; break;
        case USERDEFS_MENU_SLIDECURSORZ:            ud.menu_slidecursorz             = iSet; break;
        case USERDEFS_GLOBAL_R:                     globalr                          = iSet; break;
        case USERDEFS_GLOBAL_G:                     globalg                          = iSet; break;
        case USERDEFS_GLOBAL_B:                     globalb                          = iSet; break;
        case USERDEFS_DEFAULT_VOLUME:               ud.default_volume                = iSet; break;
        case USERDEFS_DEFAULT_SKILL:                ud.default_skill                 = iSet; break;
        case USERDEFS_MENU_SHADEDESELECTED:         MF_Redfont.shade_deselected      = MF_Bluefont.shade_deselected = MF_Minifont.shade_deselected = iSet; break;
        case USERDEFS_MENU_SHADEDISABLED:           MF_Redfont.shade_disabled        = MF_Bluefont.shade_disabled   = MF_Minifont.shade_disabled   = iSet; break;
        case USERDEFS_MENUTEXT_ZOOM:                MF_Redfont.zoom                  = iSet; break;
        case USERDEFS_MENUTEXT_XSPACE:              MF_Redfont.emptychar.x           = iSet; break;
        case USERDEFS_MENUTEXT_PAL:                 MF_Redfont.pal                   = iSet; break;
        case USERDEFS_MENUTEXT_PALSELECTED:         MF_Redfont.pal_selected          = iSet; break;
        case USERDEFS_MENUTEXT_PALDESELECTED:       MF_Redfont.pal_deselected        = iSet; break;
        case USERDEFS_MENUTEXT_PALDISABLED:         MF_Redfont.pal_disabled          = iSet; break;
        case USERDEFS_MENUTEXT_PALSELECTED_RIGHT:   MF_Redfont.pal_selected_right    = iSet; break;
        case USERDEFS_MENUTEXT_PALDESELECTED_RIGHT: MF_Redfont.pal_deselected_right  = iSet; break;
        case USERDEFS_MENUTEXT_PALDISABLED_RIGHT:   MF_Redfont.pal_disabled_right    = iSet; break;
        case USERDEFS_GAMETEXT_ZOOM:                MF_Bluefont.zoom                 = iSet; break;
        case USERDEFS_GAMETEXT_XSPACE:              MF_Bluefont.emptychar.x          = iSet; break;
        case USERDEFS_GAMETEXT_PAL:                 MF_Bluefont.pal                  = iSet; break;
        case USERDEFS_GAMETEXT_PALSELECTED:         MF_Bluefont.pal_selected         = iSet; break;
        case USERDEFS_GAMETEXT_PALDESELECTED:       MF_Bluefont.pal_deselected       = iSet; break;
        case USERDEFS_GAMETEXT_PALDISABLED:         MF_Bluefont.pal_disabled         = iSet; break;
        case USERDEFS_GAMETEXT_PALSELECTED_RIGHT:   MF_Bluefont.pal_selected_right   = iSet; break;
        case USERDEFS_GAMETEXT_PALDESELECTED_RIGHT: MF_Bluefont.pal_deselected_right = iSet; break;
        case USERDEFS_GAMETEXT_PALDISABLED_RIGHT:   MF_Bluefont.pal_disabled_right   = iSet; break;
        case USERDEFS_MINITEXT_ZOOM:                MF_Minifont.zoom                 = iSet; break;
        case USERDEFS_MINITEXT_XSPACE:              MF_Minifont.emptychar.x          = iSet; break;
        case USERDEFS_MINITEXT_TRACKING:            MF_Minifont.between.x            = iSet; break;
        case USERDEFS_MINITEXT_PAL:                 MF_Minifont.pal                  = iSet; break;
        case USERDEFS_MINITEXT_PALSELECTED:         MF_Minifont.pal_selected         = iSet; break;
        case USERDEFS_MINITEXT_PALDESELECTED:       MF_Minifont.pal_deselected       = iSet; break;
        case USERDEFS_MINITEXT_PALDISABLED:         MF_Minifont.pal_disabled         = iSet; break;
        case USERDEFS_MINITEXT_PALSELECTED_RIGHT:   MF_Minifont.pal_selected_right   = iSet; break;
        case USERDEFS_MINITEXT_PALDESELECTED_RIGHT: MF_Minifont.pal_deselected_right = iSet; break;
        case USERDEFS_MINITEXT_PALDISABLED_RIGHT:   MF_Minifont.pal_disabled_right   = iSet; break;
        case USERDEFS_MENUTITLE_PAL:                ud.menutitle_pal                 = iSet; break;
        case USERDEFS_SLIDEBAR_PALSELECTED:         ud.slidebar_palselected          = iSet; break;
        case USERDEFS_SLIDEBAR_PALDISABLED:         ud.slidebar_paldisabled          = iSet; break;
        case USERDEFS_SHADOW_PAL:                   ud.shadow_pal                    = iSet; break;
        case USERDEFS_MENU_SCROLLBARTILENUM:        ud.menu_scrollbartilenum         = iSet; break;
        case USERDEFS_MENU_SCROLLBARZ:              ud.menu_scrollbarz               = iSet; break;
        case USERDEFS_MENU_SCROLLCURSORZ:           ud.menu_scrollcursorz            = iSet; break;
        case USERDEFS_RETURN:
            if (lParm2 == 0)
                aGameVars[g_returnVarID].global = iSet;
            else
                ud.returnvar[lParm2 - 1] = iSet;
            break;
        case USERDEFS_USERBYTEVERSION:              ud.userbytever                   = iSet; break;
        case USERDEFS_AUTOSAVE:                     ud.autosave                      = iSet; break;
        case USERDEFS_DRAW_Y:                       rotatesprite_y_offset            = iSet; break;
        case USERDEFS_DRAW_YXASPECT:                rotatesprite_yxaspect            = iSet; break;
        case USERDEFS_FOV:                          ud.fov                           = iSet; break;
        case USERDEFS_NEWGAMECUSTOMOPEN:
            for (unsigned int b = 0; b < MAXMENUGAMEPLAYENTRIES; ++b)
                if (iSet & (1u<<b))
                    ME_NEWGAMECUSTOMENTRIES[b].flags = 0;
            break;
        case USERDEFS_NEWGAMECUSTOMSUBOPEN:
            for (unsigned int b = 0; b < MAXMENUGAMEPLAYENTRIES; ++b)
                if (iSet & (1u<<b))
                    ME_NEWGAMECUSTOMSUBENTRIES[lParm2][b].flags = 0;
            break;
    }
}

const memberlabel_t InputLabels[]=
{
    { "avel",    INPUT_AVEL,    0, 0, -1 },
    { "q16avel", INPUT_Q16AVEL, 0, 0, -1 },
    { "horz",    INPUT_HORZ,    0, 0, -1 },
    { "q16horz", INPUT_Q16HORZ, 0, 0, -1 },
    { "fvel",    INPUT_FVEL,    0, 0, -1 },
    { "svel",    INPUT_SVEL,    0, 0, -1 },
    { "bits",    INPUT_BITS,    0, 0, -1 },
    { "extbits", INPUT_EXTBITS, 0, 0, -1 },
};

int32_t __fastcall VM_GetPlayerInput(int const playerNum, int32_t labelNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)playerNum >= (unsigned)g_mostConcurrentPlayers))
    {
        CON_ERRPRINTF("invalid player %d\n", playerNum);
        return -1;
    }

    auto const &i = g_player[playerNum].input;

    switch (labelNum)
    {
        case INPUT_AVEL:
            labelNum = (i->q16avel >> 16); break;

        case INPUT_HORZ:
            labelNum = (i->q16horz >> 16); break;

        case INPUT_Q16AVEL: labelNum = i->q16avel; break;
        case INPUT_Q16HORZ: labelNum = i->q16horz; break;
        case INPUT_FVEL:    labelNum = i->fvel;    break;
        case INPUT_SVEL:    labelNum = i->svel;    break;
        case INPUT_BITS:    labelNum = i->bits;    break;
        case INPUT_EXTBITS: labelNum = i->extbits; break;

        default: EDUKE32_UNREACHABLE_SECTION(labelNum = -1; break);
    }

    return labelNum;
}

void __fastcall VM_SetPlayerInput(int const playerNum, int const labelNum, int32_t const newValue)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)playerNum >= (unsigned)g_mostConcurrentPlayers))
    {
        CON_ERRPRINTF("invalid player %d\n", playerNum);
        return;
    }

    auto &i = g_player[playerNum].input;

    switch (labelNum)
    {
        case INPUT_AVEL:
            i->q16avel = fix16_from_int(newValue); break;

        case INPUT_HORZ:
            i->q16horz = fix16_from_int(newValue); break;

        case INPUT_Q16AVEL: i->q16avel = newValue; break;
        case INPUT_Q16HORZ: i->q16horz = newValue; break;
        case INPUT_FVEL:    i->fvel    = newValue; break;
        case INPUT_SVEL:    i->svel    = newValue; break;
        case INPUT_BITS:    i->bits    = newValue; break;
        case INPUT_EXTBITS: i->extbits = newValue; break;
    }
}

const memberlabel_t TileDataLabels[]=
{
    // tilesiz[]
    { "xsize",      TILEDATA_XSIZE,      0, 0, -1 },
    { "ysize",      TILEDATA_YSIZE,      0, 0, -1 },

    // picanm[]
    { "animframes", TILEDATA_ANIMFRAMES, 0, 0, -1 },
    { "xoffset",    TILEDATA_XOFFSET,    0, 0, -1 },
    { "yoffset",    TILEDATA_YOFFSET,    0, 0, -1 },
    { "animspeed",  TILEDATA_ANIMSPEED,  0, 0, -1 },
    { "animtype",   TILEDATA_ANIMTYPE,   0, 0, -1 },

    // g_tile[]
    { "gameflags",  TILEDATA_GAMEFLAGS,  0, 0, -1 },
};

int32_t __fastcall VM_GetTileData(int const tileNum, int32_t labelNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)tileNum >= MAXTILES))
    {
        CON_ERRPRINTF("invalid tile %d\n", tileNum);
        return -1;
    }

    auto const &p = picanm[tileNum];

    switch (labelNum)
    {
        case TILEDATA_XSIZE: labelNum = tilesiz[tileNum].x; break;
        case TILEDATA_YSIZE: labelNum = tilesiz[tileNum].y; break;

        case TILEDATA_GAMEFLAGS: labelNum = g_tile[tileNum].flags; break;

        case TILEDATA_ANIMFRAMES: labelNum = p.num;  break;
        case TILEDATA_XOFFSET:    labelNum = p.xofs; break;
        case TILEDATA_YOFFSET:    labelNum = p.yofs; break;

        case TILEDATA_ANIMSPEED: labelNum = (p.sf & PICANM_ANIMSPEED_MASK); break;
        case TILEDATA_ANIMTYPE:  labelNum = (p.sf & PICANM_ANIMTYPE_MASK) >> PICANM_ANIMTYPE_SHIFT; break;

        default: EDUKE32_UNREACHABLE_SECTION(labelNum = -1; break);
    }

    return labelNum;
}

void __fastcall VM_SetTileData(int const tileNum, int const labelNum, int32_t newValue)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)tileNum >= MAXTILES))
    {
        CON_ERRPRINTF("invalid tile %d\n", tileNum);
        return;
    }

    auto &p = picanm[tileNum];

    switch (labelNum)
    {
        //case TILEDATA_XSIZE: tilesiz[tileNum].x = newValue; break;
        //case TILEDATA_YSIZE: tilesiz[tileNum].y = newValue; break;

        case TILEDATA_GAMEFLAGS: g_tile[tileNum].flags = newValue; break;

        case TILEDATA_ANIMFRAMES: p.num  = newValue; break;
        case TILEDATA_XOFFSET:    p.xofs = newValue; break;
        case TILEDATA_YOFFSET:    p.yofs = newValue; break;

        case TILEDATA_ANIMSPEED: p.sf = (p.sf & ~PICANM_ANIMSPEED_MASK) | (newValue & PICANM_ANIMSPEED_MASK); break;
        case TILEDATA_ANIMTYPE:  p.sf = (p.sf & ~PICANM_ANIMTYPE_MASK) | ((newValue << PICANM_ANIMTYPE_SHIFT) & PICANM_ANIMTYPE_MASK); break;
    }
}

const memberlabel_t PalDataLabels[]=
{
    // g_noFloorPal[]
    { "nofloorpal", PALDATA_NOFLOORPAL, 0, 0, -1 },
};

int32_t __fastcall VM_GetPalData(int const palNum, int32_t labelNum)
{
    if (EDUKE32_PREDICT_FALSE((unsigned)palNum >= MAXPALOOKUPS))
    {
        CON_ERRPRINTF("invalid palette %d\n", palNum);
        return -1;
    }

    switch (labelNum)
    {
        case PALDATA_NOFLOORPAL: labelNum = g_noFloorPal[palNum]; break;
        default: EDUKE32_UNREACHABLE_SECTION(labelNum = -1; break);
    }

    return labelNum;
}
#undef LABEL_SETUP
#undef LABEL_SETUP_UNMATCHED

hashtable_t h_actor      = { ACTOR_END>>1, NULL };
hashtable_t h_input      = { INPUT_END>>1, NULL };
hashtable_t h_paldata    = { PALDATA_END>>1, NULL };
hashtable_t h_player     = { PLAYER_END>>1, NULL };
hashtable_t h_projectile = { PROJ_END>>1, NULL };
hashtable_t h_sector     = { SECTOR_END>>1, NULL };
hashtable_t h_tiledata   = { TILEDATA_END>>1, NULL };
hashtable_t h_tsprite    = { ACTOR_END>>1, NULL };
hashtable_t h_userdef    = { USERDEFS_END>>1, NULL };
hashtable_t h_wall       = { WALL_END>>1, NULL };

static hashtable_t *const struct_tables[] = {
    &h_actor, &h_input, &h_paldata, &h_player, &h_projectile, &h_sector, &h_tiledata, &h_tsprite, &h_userdef, &h_wall,
};

#define STRUCT_HASH_SETUP(table, labels)                 \
    do                                                   \
    {                                                    \
        for (int i = 0; i < ARRAY_SSIZE(labels); i++)    \
            hash_add(&table, labels[i].name, i, 0);      \
        EDUKE32_STATIC_ASSERT(ARRAY_SSIZE(labels) != 0); \
    } while (0)

void scriptInitStructTables(void)
{
    for (auto table : struct_tables)
        hash_init(table);

    inithashnames();
    initsoundhashnames();

    STRUCT_HASH_SETUP(h_actor,      ActorLabels);
    STRUCT_HASH_SETUP(h_input,      InputLabels);
    STRUCT_HASH_SETUP(h_paldata,    PalDataLabels);
    STRUCT_HASH_SETUP(h_player,     PlayerLabels);
    STRUCT_HASH_SETUP(h_projectile, ProjectileLabels);
    STRUCT_HASH_SETUP(h_sector,     SectorLabels);
    STRUCT_HASH_SETUP(h_tiledata,   TileDataLabels);
    STRUCT_HASH_SETUP(h_tsprite,    TsprLabels);
    STRUCT_HASH_SETUP(h_userdef,    UserdefsLabels);
    STRUCT_HASH_SETUP(h_wall,       WallLabels);
}
#undef STRUCT_HASH_SETUP

#endif
