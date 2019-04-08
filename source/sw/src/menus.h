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

// MENUS.H
// Contains type definitions for all pop up menus

#ifndef MENUS_PUBLIC_
#define MENUS_PUBLIC_

#define MENU_SHADE_DEFAULT 0
#define MENU_SHADE_INACTIVE 20

typedef enum
{
    ct_mainmenu, ct_savemenu, ct_loadmenu, ct_soundmenu, ct_optionmenu, ct_quickloadmenu,
    ct_quitmenu, ct_ordermenu, ct_episodemenu, ct_max
} CTLType;

extern SWBOOL UsingMenus;
extern int SENSITIVITY;
extern CTLType ControlPanelType;
extern int16_t MenuTextShade;
extern int16_t MenuTextPalette;

// Prototypes
//void MNU_DoMenu( CTLType type, PLAYERp pp );
void MNU_InitMenus(void);
//void (*CustomRefresh)(void);
//void MNU_Refresh( void );
void MNU_DrawMenu(void);    // This is used in drawscreen to refresh menus in
// multiplay situations.
void MNU_CheckForMenus(void);
void MNU_CheckForMenusAnyKey(void);
void MNU_MeasureString(const char *string, short *w, short *h);
void MNU_DrawString(short x, short y, const char *string, short shade, short pal);
void MNU_MeasureSmallString(const char *string,short *w,short *h);
void MNU_DrawSmallString(short x,short y,const char *string,short shade,short pal);
void MNU_MeasureStringLarge(const char *string, short *w, short *h);
void MNU_DrawStringLarge(short x, short y, const char *string);

// Functions from my other engine
//void Get_Palette (unsigned char *pal);
//void Set_Palette(unsigned char *buff);
//void Fade_Timer(int clicks);
void FadeIn(unsigned char targetcolor, unsigned int clicks);
void FadeOut(unsigned char targetcolor, unsigned int clicks);
void ResetPalette(PLAYERp pp);

void ExitMenus(void);
void ResetMenuInput(void);

#define BUTTONSET(x,value) \
    ( \
        ((x)>31) ? \
        (CONTROL_ButtonState2 |= (value<<((x)-32)))  : \
        (CONTROL_ButtonState1 |= (value<<(x))) \
    )

extern SWBOOL BorderAdjust;
extern SWBOOL MultiPlayQuitFlag;

// Make memcpy an intrinsic function for an easy frame rate boost
//#pragma intrinsic( memcpy );

// L O C A L   V A R I A B L E S ////////////////////////////////////////////////////////////////

// Default menu pic brightness
#define m_defshade      2

#define FLASHTIME       60      // One second per icon flash

// Defines for permanentwritesprite clipping box
#define M_CX1   0
#define M_CY1   0
#define M_CX2   319
#define M_CY2   199

#define MZ              65536

#define asc_Esc         27
#define asc_Enter       13
#define asc_Space       32


#define pic_none 0
#define pic_radiobuttn1 2816
#define pic_radiobuttn2 2817
#define pic_newgame 2819
#define pic_load 2820
#define pic_save 2821
#define pic_options 2822
#define pic_orderinfo 2823
#define pic_todemo 2824
#define pic_togame 2825
#define pic_quit 2826
#define pic_newgametitl 2827
#define pic_training 2828
#define pic_easy 2829
#define pic_normal 2830
#define pic_hard 2831
#define pic_impossible 2832
#define pic_optionstitl 2833
#define pic_endgame 2834
#define pic_detail 2835
#define pic_high 2836
#define pic_low 2837
#define pic_mousesense 2838
#define pic_soundvol 2839
#define pic_toggles 2845
#define pic_togglestitl 2844
#define pic_mousenable 2840
#define pic_joyenable 2841
#define pic_bobbing 2842
#define pic_slidelend 2846
#define pic_slidebar 2847
#define pic_sliderend 2848
#define pic_sliderknob 2849
#define pic_shuriken1 2850
#define pic_yinyang 2870
#define pic_soundtitl 2870
#define pic_sndfxvol 2871
#define pic_musicvol 2872
#define pic_episode1 2873
#define pic_episode2 2874
#define pic_episode3 2875
#define pic_modem 2876
#define pic_scrsize 2877
#define pic_loadsavecursor 2918
#define pic_loadgame 2915
#define pic_savegame 2916
#define pic_loading 2917
#define pic_loadsavescreen 2919
#define pic_loadsavescreenbak 2922
#define pic_savedescr 2924
#define pic_shadow_warrior 2366

// This is the current values set with all slider bar functions
#define SENSE_DEFAULT   10  // Default mouse sensitivity ** should be 5!!!
#define FXVOL_DEFAULT   8   // Default sound fx volume
#define MUSIC_DEFAULT   8   // Default music volume
#define SCRSIZE_DEFAULT 9   // Default screen size, max is 10
#define BRIGHTNESS_DEFAULT 0 // Default is no gamma-correction
#define BORDERTILE_DEFAULT 0 // Default is no gamma-correction
#define GAMETYPE_DEFAULT 0  // Regular DeathMatch
#define NETLEVEL_DEFAULT 0  // Default is level 1 (0)
#define MONSTERS_DEFAULT 0  // No Monsters
#define KILLLIMIT_DEFAULT 0 // No kill limit
#define TIMELIMIT_DEFAULT 0 // None
#define PLAYERCOLOR_DEFAULT 0

typedef enum
{
    sldr_none,
    sldr_mouse, sldr_sndfxvolume, sldr_musicvolume, sldr_scrsize, sldr_brightness,
    sldr_bordertile, sldr_gametype, sldr_netlevel, sldr_monsters, sldr_killlimit,
    sldr_timelimit, sldr_playercolor, sldr_videores, sldr_videobpp,
    sldr_mousescalex, sldr_mousescaley,
    sldr_joyaxisscale, sldr_joyaxisanalog, sldr_joyaxisdead, sldr_joyaxissatur,
    sldr_max
} SLDRType;


#define MAX_SLDR_WIDTH  16  // maximum size of slider before x is compressed

#define SLDR_MOUSESENSEMAX              20
#define SLDR_SNDFXVOLMAX                16
#define SLDR_MUSICVOLMAX                16
#define SLDR_SCRSIZEMAX                 14
#define SLDR_BRIGHTNESSMAX              8
#define SLDR_BORDERTILEMAX              (isShareware ? 21 : 38) // counted from border.c
#define SLDR_GAMETYPEMAX                3

#define SLDR_NETLEVELMAX_REG             28
#define SLDR_NETLEVELMAX_SW              4
#define SLDR_NETLEVELMAX                (isShareware ? SLDR_NETLEVELMAX_SW : SLDR_NETLEVELMAX_REG)

#define SLDR_MONSTERSMAX                5   // Skill Levels
#define SLDR_KILLLIMITMAX               11  // Increments of 10 up to 100, 1 is no limit
#define SLDR_TIMELIMITMAX               9
#define SLDR_PLAYERCOLORMAX             8   // Up to 8 players different colors


#define MOUSE_SENS_MAX_VALUE (1<<16)
#define MUSIC_VOL_MAX_VALUE 255
#define FX_VOL_MAX_VALUE 255

// These are all the toggle button settings
typedef enum
{
    btn_none, btn_auto_run, btn_crosshair, btn_auto_aim,
    btn_mouse_aim, btn_messages, btn_mouse_invert, btn_bobbing, btn_shadows,
    btn_sound, btn_music, btn_talking, btn_ambience, btn_flipstereo,
    btn_res0, btn_res1, btn_res2, btn_res3, btn_res4, btn_res5, btn_res6,
    btn_markers, btn_teamplay, btn_friendlyfire,btn_parental,btn_nuke,
    btn_voxels, btn_stats, btn_playcd,
    btn_videofs,
    btn_max
} BTNType;

enum
{
    mf_normal = BIT(0),
    mf_pushed = BIT(1),
    mf_selected = BIT(2),
    mf_disabled = BIT(3),
    mf_separated = BIT(4)
};
typedef int MenuFlags;

#define MenuSelectFlags (mf_pushed | mf_selected | mf_disabled)
#define MenuDrawFlags (ROTATE_SPRITE_SCREEN_CLIP)

typedef enum
{
    mt_none,
    mt_inert, mt_slider, mt_button, mt_option, mt_layer
} MenuTag;

typedef enum
{
    uc_setup, uc_draw, uc_touchup, uc_hit
} UserCall;

struct MenuGroup;

typedef struct MENU_ITEM
{
    MenuTag type;                       // What kind of item is this on the
    // menu?
    MenuFlags flags;                    // Menu item flags
    SLDRType slider;                    // Slider type, if any
    BTNType button;                     // Button state, if any
    unsigned char hotkey;               // First letter of item
    const char *text;                   // Text appearing in item, if any.
    MenuGroup *child;                        // Should be menugroup, used to spawn
    // sub-groups from items.
    int x, y;                          // x,y position on screen.
    short pic;                        // Startpic to use
    char shade;                         // Shade of pic
    int tics;                          // Ticcount for item
    SWBOOL(*custom)(void);               // Work function on item select

    SWBOOL (*preprocess)(struct MENU_ITEM *);
    SWBOOL (*postprocess)(struct MENU_ITEM *); // Can do things on items select
} MenuItem, *MenuItem_p;

typedef struct MenuGroup
{
    int x, y;                          // Menu x,y position on screen.
    const char *text;
    MenuItem_p items;                   // Array of menu items for this menu.
    short titlepic;                   // Used to draw title on menu with.
    short cursorpic;                  // Pic used for menu cursor, 1st in
    // anim sequence if animated.
    char shade;                         // Title pic shade

    SWBOOL(*draw_custom)(UserCall, MenuItem *);       // Custom routine
    SWBOOL(*move_custom)(UserCall, MenuItem *);       // Custom routine

    short cursor;                       // This is the current menu item the
    // cursor is resting on.
} MenuGroup, *MenuGroup_p;

// Custom Routine Prototypes ////////////////////////////////////////////////////////////////////

SWBOOL MNU_QuitCustom(UserCall call, MenuItem *item);
SWBOOL MNU_QuickLoadCustom(UserCall call, MenuItem *item);
SWBOOL MNU_LoadSaveTouchupCustom(UserCall call, MenuItem *item);
SWBOOL MNU_DoParentalPassword(UserCall call, MenuItem *item);
SWBOOL MNU_OrderCustom(UserCall call, MenuItem *item);
SWBOOL MNU_DoEpisodeSelect(UserCall call, MenuItem *item);

SWBOOL MNU_MusicFxCheck(MenuItem_p item);
SWBOOL MNU_MusicCheck(MenuItem_p item);
SWBOOL MNU_FxCheck(MenuItem_p item);
SWBOOL MNU_MouseCheck(MenuItem_p item);
SWBOOL MNU_JoystickCheck(MenuItem_p item);
SWBOOL MNU_BorderCheck(MenuItem_p item);
SWBOOL MNU_ShareWareCheck(MenuItem_p item);
SWBOOL MNU_MenuLevelCheck(MenuItem_p item);
SWBOOL MNU_ShareWareMessage(MenuItem_p item);

// Custom MenuItem Routines /////////////////////////////////////////////////////////////////////

SWBOOL MNU_StartGame(void);
SWBOOL MNU_StartNetGame(void);
SWBOOL MNU_EpisodeCustom(void);
SWBOOL MNU_GetDescripCustom(void);
SWBOOL MNU_LoadGameCustom(void);
SWBOOL MNU_SaveGameCustom(void);
SWBOOL MNU_GetLoadCustom(void);
SWBOOL MNU_GetSaveCustom(void);
SWBOOL MNU_ParentalCustom(void);
SWBOOL MNU_KeySetupCustom(UserCall call, MenuItem *item);
SWBOOL MNU_LoadModernDefaults(void);
SWBOOL MNU_LoadClassicDefaults(void);

// Menu Definitions /////////////////////////////////////////////////////////////////////////////

#define DefInert(key,text)          mt_inert,mf_normal,sldr_none,btn_none,key,text,NULL
#define DefSlider(slider,key,text)  mt_slider,mf_normal,slider,btn_none,key,text,NULL
#define DefOption(key,text)         mt_option,mf_normal,sldr_none,btn_none,key,text,NULL
#define DefButton(button,key,text)  mt_button,mf_normal,sldr_none,button,key,text,NULL
#define DefLayer(key,text,child)    mt_layer,mf_normal,sldr_none,btn_none,key,text,child

#define DefDisabled(key,text,child)    mt_layer,mf_disabled,sldr_none,btn_none,key,text,child
#define DefNone mt_none,(MenuFlags)0,(SLDRType)0,(BTNType)0,0,NULL,NULL,0,0,0,0,0,NULL,NULL,NULL

#define OPT_XS 30
#define OPT_YS 30
#define OPT_XSIDE (OPT_XS + 120)
#define OPT_YINC 10
#define OPT_LINE(line) (OPT_YS + (OPT_YINC * (line)))

typedef struct
{
    int x,y;
} VMODE;

#endif
