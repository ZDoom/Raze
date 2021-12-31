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

#include "game.h"

BEGIN_SW_NS

enum
{
	PRI_FRONT_MAX   = 250,
	PRI_FRONT       = 192,
	PRI_MID         = 128,
	PRI_BACK        = 64,
	PRI_BACK_MAX    = 0
};

#define MAKE_CONPIC_ENUM
enum conpic_id
{
#include "conpic.h"
};
#undef MAKE_CONPIC_ENUM

enum PanelSpriteIDs
{
	ID_BORDER_TOP = 1, ID_BORDER_BOTTOM, ID_BORDER_LEFT, ID_BORDER_RIGHT, ID_BORDER_SHADE,
	ID_PANEL_BORDER_LEFT, ID_PANEL_BORDER_RIGHT,
	ID_TEXT, ID_TEXT2, ID_TEXT3, ID_TEXT4
};

struct PANEL_STATE
{
	short picndx;                       // for pip stuff in conpic.h
	int tics;
	void (*Animator)(PANEL_SPRITE*);    // JBF: return type was long
	PANEL_STATE* NextState;
	uint32_t flags;
	uint8_t xvel;
	uint8_t yvel;
};

enum
{
	PANF_PRIMARY         = (BIT(0)), // denotes primary weapon
	PANF_SECONDARY       = (BIT(1)), // denotes secondary weapon
	PANF_BOB             = (BIT(2)),
	PANF_REST_POS        = (BIT(3)), // used for certain weapons - fireball
	PANF_RELOAD          = (BIT(4)), // reload flag used for uzi
	PANF_TRANS_FLIP      = (BIT(5)), // translucent flip - matches rotate sprite
	PANF_ACTION_POS      = (BIT(6)), // used for certain weapons - fireball
	PANF_WEAPON_HIDE     = (BIT(7)), // hide when climbing/driving
	PANF_TRANSLUCENT     = (BIT(8)), // turn invisible
	PANF_INVISIBLE       = (BIT(9)), // turn invisible
	PANF_DEATH_HIDE      = (BIT(10)), // hide done when dead
	PANF_SCREEN_CLIP     = (BIT(12)), // maintain aspect to the screen
	PANF_IGNORE_START_MOST = (BIT(14)), // maintain aspect to the screen
	PANF_XFLIP           = (BIT(15)), // xflip
	PANF_SUICIDE         = (BIT(16)), // kill myself
	PANF_WEAPON_SPRITE   = (BIT(17)), // its a weapon sprite - for V mode
	PANF_CORNER          = (BIT(18)), // draw from the corner
	PANF_NOT_IN_VIEW     = (BIT(19)), // not in view
	PANF_UNHIDE_SHOOT    = (BIT(20)), // shoot after un-hiding a weapon
	PANF_JUMPING         = (BIT(21)),
	PANF_FALLING         = (BIT(22)),
	PANF_DRAW_BEFORE_VIEW = (BIT(30)), // draw before drawrooms
};

typedef void (*PANEL_SPRITE_FUNCp)(PANEL_SPRITE*);

struct PANEL_SPRITE_OVERLAY
{
	PANEL_STATE* State;
	int flags;
	short tics;
	short pic;
	short xoff; // from panel sprite center x
	short yoff; // from panel sprite center y
};

struct PANEL_SPRITE
{
	PANEL_SPRITE* Next, * Prev;
	PANEL_SPRITE* sibling;
	PANEL_STATE* State, *RetractState, *PresentState, *ActionState, *RestState;
	PLAYER* PlayerP;
	DVector2 pos, opos, bobpos;

	PANEL_SPRITE_OVERLAY over[8];
	PANEL_SPRITE_FUNCp PanelSpriteFunc;
	short ID;                           // id for finding sprite types on the list
	short picndx;                       // for pip stuff in conpic.h
	short picnum;                       // bypass pip stuff in conpic.h
	short vel, vel_adj;
	int flags, priority;
	int scale;
	int jump_speed, jump_grav;         // jumping vars
	int xspeed;
	short tics, delay;                  // time vars
	short ang, rotate_ang;
	short sin_ndx, sin_amt, sin_arc_speed;
	double bob_height_divider;
	short shade, pal;
	short kill_tics;
	short WeaponType; // remember my own weapon type for weapons with secondary function

	// Inline helpers.
	void backupx() { opos.X = pos.X; };
	void backupy() { opos.Y = pos.Y; };
	void backupcoords() { opos = pos; };
	void backupbobx() { bobpos.X = pos.X; };
	void backupboby() { bobpos.Y = pos.Y; };
	void backupbobcoords() { bobpos = pos; };
};

struct PANEL_STATE_TABLE
{
	PANEL_STATE* pstate;
	short state_size;
};

extern PANEL_STATE_TABLE PanelStateTable[];

// Panel State flags - also used for
enum
{
	psf_Invisible    = BIT(16),
	psf_QuickCall    = BIT(23),
	psf_Xflip        = BIT(24),
	psf_ShadeHalf    = BIT(25),
	psf_ShadeNone    = BIT(26),
};

enum BorderTypes
{
	BORDER_NONE = 0,
	BORDER_MINI_BAR = 1,
	BORDER_BAR = 2
};

enum
{
	MICRO_SIGHT_NUM = 0,
	MICRO_SIGHT = 2075,
	
	MICRO_SHOT_NUM = 2,
	MICRO_SHOT_20 = 2076,
	MICRO_SHOT_1 = 2077,
	
	MICRO_HEAT_NUM = 1,
	MICRO_HEAT = 2084,
	
	UZI_COPEN   = 2040,
	UZI_CCLOSED = 2041,
	UZI_CLIT    = 2042,
	UZI_CRELOAD = 2043,
	
	HEAD_MODE1 = 2055,
	HEAD_MODE2 = 2056,
	HEAD_MODE3 = 2057,
	
	SHOTGUN_AUTO_NUM = 0,
	SHOTGUN_AUTO = 2078,
};


PANEL_SPRITE* pSpawnSprite(PLAYER* pp, PANEL_STATE* state, uint8_t priority, double x, double y);
void pSetSuicide(PANEL_SPRITE* psp);
bool pKillScreenSpiteIDs(PLAYER* pp, short id);
void PreUpdatePanel(double smoothratio);
void UpdatePanel(double smoothratio);
void PlayerUpdateArmor(PLAYER* pp,short value);
void pToggleCrosshair(void);
void pKillSprite(PANEL_SPRITE* psp);
void InitChops(PLAYER* pp);
void ChopsSetRetract(PLAYER* pp);

END_SW_NS

#endif
