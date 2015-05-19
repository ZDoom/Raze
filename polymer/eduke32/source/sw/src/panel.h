//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

#ifndef PANEL_H

#define PANEL_H

#include "mytypes.h"
#include "game.h"

#define FRAG_BAR 2920

#define PRI_FRONT_MAX   250
#define PRI_FRONT       192
#define PRI_MID         128
#define PRI_BACK        64
#define PRI_BACK_MAX    0

#define MAKE_CONPIC_ENUM
enum conpic_id
{
#include "conpic.h"
};
typedef enum conpic_id CONPIC_ID;
#undef MAKE_CONPIC_ENUM

enum PanelSpriteIDs
{
    ID_BORDER_TOP = 1, ID_BORDER_BOTTOM, ID_BORDER_LEFT, ID_BORDER_RIGHT, ID_BORDER_SHADE,
    ID_PANEL_BORDER_LEFT, ID_PANEL_BORDER_RIGHT,
    ID_TEXT, ID_TEXT2, ID_TEXT3, ID_TEXT4
};

struct PANEL_STATEstruct
{
    short picndx;                       // for pip stuff in conpic.h
    int tics;
    void (*Animator)(PANEL_SPRITEp);    // JBF: return type was long
    PANEL_STATEp NextState;
    uint32_t flags;
    uint8_t xvel;
    uint8_t yvel;
};

#define PANF_PRIMARY         (BIT(0)) // denotes primary weapon
#define PANF_SECONDARY       (BIT(1)) // denotes secondary weapon
#define PANF_BOB             (BIT(2))
#define PANF_REST_POS        (BIT(3)) // used for certain weapons - fireball
#define PANF_RELOAD          (BIT(4)) // reload flag used for uzi
#define PANF_TRANS_FLIP      (BIT(5)) // translucent flip - matches rotate sprite
#define PANF_ACTION_POS      (BIT(6)) // used for certain weapons - fireball
#define PANF_WEAPON_HIDE     (BIT(7)) // hide when climbing/driving
#define PANF_TRANSLUCENT     (BIT(8)) // turn invisible
#define PANF_INVISIBLE       (BIT(9)) // turn invisible
#define PANF_DEATH_HIDE      (BIT(10)) // hide done when dead
#define PANF_KILL_AFTER_SHOW (BIT(11)) // kill after showing numpages times
#define PANF_SCREEN_CLIP     (BIT(12)) // maintain aspect to the screen
#define PANF_STATUS_AREA     (BIT(13)) // maintain aspect to the screen
#define PANF_IGNORE_START_MOST (BIT(14)) // maintain aspect to the screen
#define PANF_XFLIP           (BIT(15)) // xflip
#define PANF_SUICIDE         (BIT(16)) // kill myself
#define PANF_WEAPON_SPRITE   (BIT(17)) // its a weapon sprite - for V mode
#define PANF_CORNER          (BIT(18)) // draw from the corner
#define PANF_NOT_IN_VIEW     (BIT(19)) // not in view
#define PANF_UNHIDE_SHOOT    (BIT(20)) // shoot after un-hiding a weapon
#define PANF_JUMPING         (BIT(21))
#define PANF_FALLING         (BIT(22))
#define PANF_DRAW_BEFORE_VIEW (BIT(30)) // draw before drawrooms
#define PANF_NOT_ALL_PAGES       (BIT(31)) // DONT use permanentwritesprite bit for rotatesprite

typedef void (*PANEL_SPRITE_FUNCp)(PANEL_SPRITEp);

typedef struct
{
    PANEL_STATEp State;
    int flags;
    short tics;
    short pic;
    short xoff; // from panel sprite center x
    short yoff; // from panel sprite center y
} PANEL_SPRITE_OVERLAY, *PANEL_SPRITE_OVERLAYp;

struct PANEL_SPRITEstruct
{
    PANEL_SPRITEp Next, Prev;
    PANEL_SPRITEp sibling;
    PANEL_STATEp State, RetractState, PresentState, ActionState, RestState;
    PLAYERp PlayerP;
    // Do not change the order of this line
    uint16_t xfract;
    int16_t x;
    uint16_t yfract;
    int16_t y;                            // Do not change the order of this
    // line

    PANEL_SPRITE_OVERLAY over[8];
    PANEL_SPRITE_FUNCp PanelSpriteFunc;
    short ID;                           // id for finding sprite types on the
    // list
    short picndx;                       // for pip stuff in conpic.h
    short picnum;                       // bypass pip stuff in conpic.h
    short x1, y1, x2, y2;               // for rotatesprites box cliping
    short vel, vel_adj;
    short numpages;
    int xorig, yorig, flags, priority;
    int scale;
    int jump_speed, jump_grav;         // jumping vars
    int xspeed;
    short tics, delay;                  // time vars
    short ang, rotate_ang;
    short sin_ndx, sin_amt, sin_arc_speed;
    short bob_height_shift;
    short shade, pal;
    short kill_tics;
    short WeaponType; // remember my own weapon type for weapons with secondary function
};

typedef struct
{
    PANEL_STATEp pstate;
    short state_size;
} PANEL_STATE_TABLE, *PANEL_STATE_TABLEp;

extern PANEL_STATE_TABLE PanelStateTable[];

// Panel State flags - also used for
#define psf_Invisible    BIT(16)
#define psf_QuickCall    BIT(23)
#define psf_Xflip        BIT(24)
#define psf_ShadeHalf    BIT(25)
#define psf_ShadeNone    BIT(26)

enum BorderTypes
{
    BORDER_NONE = 0,
    BORDER_MINI_BAR = 1,
    BORDER_BAR = 2
};

#define MICRO_SIGHT_NUM 0
#define MICRO_SIGHT 2075

#define MICRO_SHOT_NUM 2
#define MICRO_SHOT_20 2076
#define MICRO_SHOT_1 2077

#define MICRO_HEAT_NUM 1
#define MICRO_HEAT 2084

#define UZI_COPEN   2040
#define UZI_CCLOSED 2041
#define UZI_CLIT    2042
#define UZI_CRELOAD 2043

#define HEAD_MODE1 2055
#define HEAD_MODE2 2056
#define HEAD_MODE3 2057

#define SHOTGUN_AUTO_NUM 0
#define SHOTGUN_AUTO 2078

PANEL_SPRITEp pSpawnSprite(PLAYERp pp, PANEL_STATEp state, uint8_t priority, int x, int y);
PANEL_SPRITEp pSpawnFullScreenSprite(PLAYERp pp, short pic, short pri, int x, int y);
PANEL_SPRITEp pSpawnFullViewSprite(PLAYERp pp,short pic,short pri,int x,int y);
void pSetSuicide(PANEL_SPRITEp psp);
SWBOOL pKillScreenSpiteIDs(PLAYERp pp, short id);
void pFlushPerms(PLAYERp pp);
void PreUpdatePanel(void);
void UpdatePanel(void);
void PlayerUpdateKeys(PLAYERp pp);
void PlayerUpdateArmor(PLAYERp pp,short value);
void pToggleCrosshair(PLAYERp pp);
void pKillSprite(PANEL_SPRITEp psp);
void InitChops(PLAYERp pp);
void ChopsSetRetract(PLAYERp pp);
void PlayerUpdateTimeLimit(PLAYERp pp);

#endif
