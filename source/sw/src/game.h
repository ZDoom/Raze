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

#ifndef GAME_H

#define GAME_H

#ifndef DEBUG
#define DEBUG 0
#endif

#include "compat.h"
#include "baselayer.h"
#include "mmulti.h"

#include "mytypes.h"
#include "keyboard.h"
#include "sounds.h"
#include "settings.h"

//#define SW_SHAREWARE 1     // This determines whether game is shareware compile or not!
extern char isShareware, useDarts;
#define SW_SHAREWARE (isShareware)

// Turn warning off for unreferenced variables.
// I really should fix them at some point
//#pragma off(unreferenced)


#define ERR_STD_ARG __FILE__, __LINE__

#if DEBUG
void HeapCheck(char *, int);
#define HEAP_CHECK() HeapCheck(__FILE__, __LINE__)

void _Assert(char *, char *, unsigned);
#define ASSERT(f) \
    if (f)        \
        do { } while(0);         \
    else          \
        _Assert(#f,ERR_STD_ARG);

#define PRODUCTION_ASSERT(f) ASSERT(f)

void dsprintf(char *, char *, ...);
#define DSPRINTF dsprintf

void PokeStringMono(uint8_t Attr, uint8_t* String);

#if 1  // !JIM! Frank, I redirect this for me you'll want to set this back for you
extern int DispMono;
#define MONO_PRINT(str) if (DispMono) PokeStringMono(/*MDA_NORMAL*/ 0, str)
#else
void adduserquote(char *daquote);
extern int DispMono;
#define MONO_PRINT(str) if (DispMono) CON_ConMessage(str); // Put it in my userquote stuff!
//#define MONO_PRINT(str) if (DispMono) printf(str);
#endif

#define RANDOM_DEBUG 1 // Set this to 1 for network testing.
#else
#define ASSERT(f) do { } while(0)
#define MONO_PRINT(str)

void _Assert(char *, char *, unsigned);
#define PRODUCTION_ASSERT(f) \
    if (f)        \
        do { } while(0);         \
    else          \
        _Assert(#f,ERR_STD_ARG);

void dsprintf_null(char *, char *, ...);
#define DSPRINTF dsprintf_null
//#define DSPRINTF()

#define HEAP_CHECK()
#define RANDOM_DEBUG 0
#endif


#if RANDOM_DEBUG
int RandomRange(int, char *, unsigned);
int krand1(char *, unsigned);
#define RANDOM_P2(pwr_of_2) (MOD_P2(krand1(__FILE__,__LINE__),(pwr_of_2)))
#define RANDOM_RANGE(range) (RandomRange(range,__FILE__,__LINE__))
#define RANDOM() (krand1(__FILE__,__LINE__))
#else
int RandomRange(int);
int krand1(void);
#define RANDOM_P2(pwr_of_2) (MOD_P2(krand1(),(pwr_of_2)))
#define RANDOM_RANGE(range) (RandomRange(range))
#define RANDOM() (krand1())
#endif


#define PRINT(line,str) DebugPrint(line,str)

#include "pragmas.h"


//
// Map directions/degrees
//

#if 0
y--
^ 1536
|
|
|
|
|
|            2047
<---------------------------->
1024         |              0
x--          |             x++
|
|
|
|
V 512
y++

#endif

//////////////////////////////////////////////////////
//
// KEYBOARD
//
//////////////////////////////////////////////////////

extern SWBOOL MenuInputMode;
extern SWBOOL MessageInputMode;
extern SWBOOL ConInputMode;
extern SWBOOL ConPanel;
extern SWBOOL InputMode;
extern char MessageInputString[256];
extern char MessageOutputString[256];

//
// Defines
//

#define CIRCLE_CAMERA_DIST_MIN 12000

// dist at which actors will not move (unless shot?? to do)
#define MAX_ACTIVE_RANGE 42000
// dist at which actors roam about on their own
#define MIN_ACTIVE_RANGE 20000

#undef  KEYSC_UP
#define KEYSC_UP        sc_UpArrow
#undef  KEYSC_DOWN
#define KEYSC_DOWN      sc_DownArrow
#undef  KEYSC_LEFT
#define KEYSC_LEFT      sc_LeftArrow
#undef  KEYSC_RIGHT
#define KEYSC_RIGHT     sc_RightArrow
#undef  KEYSC_INS
#define KEYSC_INS       sc_Insert
#undef  KEYSC_DEL
#define KEYSC_DEL       sc_Delete
#undef  KEYSC_HOME
#define KEYSC_HOME      sc_Home
#undef  KEYSC_END
#define KEYSC_END       sc_End
#undef  KEYSC_PGUP
#define KEYSC_PGUP      sc_PgUp
#undef  KEYSC_PGDN
#define KEYSC_PGDN      sc_PgDn

#define KEYSC_RALT      sc_RightAlt
#define KEYSC_RCTRL     sc_RightControl
#define KEYSC_KPSLASH   sc_kpad_Slash
#define KEYSC_KPENTER   sc_kpad_Enter
#define KEYSC_PRINTSCREEN sc_PrintScreen
#define KEYSC_LASTSC      sc_LastScanCode

#define KEYSC_KP_1      sc_kpad_1
#define KEYSC_KP_2      sc_kpad_2
#define KEYSC_KP_3      sc_kpad_3
#define KEYSC_KP_4      sc_kpad_4
#define KEYSC_KP_6      sc_kpad_6
#define KEYSC_KP_5     sc_kpad_5
#define KEYSC_KP_7      sc_kpad_7
#define KEYSC_KP_8      sc_kpad_8
#define KEYSC_KP_9      sc_kpad_9
#define KEYSC_KP_0      sc_kpad_0
#define KEYSC_KPMINUS  sc_kpad_Minus
#define KEYSC_KPPLUS   sc_kpad_Plus
#define KEYSC_KPPERIOD sc_kpad_Period

#define KEYSC_EUP        sc_UpArrow
#define KEYSC_EDOWN      sc_DownArrow
#define KEYSC_ELEFT      sc_LeftArrow
#define KEYSC_ERIGHT     sc_RightArrow
#define KEYSC_EINS       sc_Insert
#define KEYSC_EDEL       sc_Delete
#define KEYSC_EHOME      sc_Home
#define KEYSC_EEND       sc_End
#define KEYSC_EPGUP      sc_PgUp
#define KEYSC_EPGDN      sc_PgDn

#undef KB_KeyPressed
#define KB_KeyPressed( scan ) \
    ( KB_KeyDown[( scan )])
#define KEY_PRESSED(sc) KB_KeyPressed((sc))
#define PKEY_PRESSED(sc) KB_KeyPressed((sc))

//
// NETWORK - REDEFINABLE SHARED (SYNC) KEYS BIT POSITIONS
//

// weapons takes up 4 bits
#define SK_WEAPON_BIT0 0
#define SK_WEAPON_BIT1 1
#define SK_WEAPON_BIT2 2
#define SK_WEAPON_BIT3 3
#define SK_WEAPON_MASK (BIT(SK_WEAPON_BIT0)| \
                        BIT(SK_WEAPON_BIT1)| \
                        BIT(SK_WEAPON_BIT2)| \
                        BIT(SK_WEAPON_BIT3))     // 16 possible numbers 0-15

#define SK_INV_HOTKEY_BIT0 4
#define SK_INV_HOTKEY_BIT1 5
#define SK_INV_HOTKEY_BIT2 6
#define SK_INV_HOTKEY_MASK (BIT(SK_INV_HOTKEY_BIT0)|BIT(SK_INV_HOTKEY_BIT1)|BIT(SK_INV_HOTKEY_BIT2))

#define SK_AUTO_AIM    7
#define SK_CENTER_VIEW 8
#define SK_PAUSE       9
#define SK_RUN_LOCK   10

#define SK_MESSAGE    11
#define SK_LOOK_UP    12
#define SK_LOOK_DOWN  13
#define SK_CRAWL_LOCK 14
#define SK_FLY        15

#define SK_RUN        16
#define SK_SHOOT      17
#define SK_OPERATE    18
#define SK_JUMP       19
#define SK_CRAWL      20
#define SK_SNAP_UP    21
#define SK_SNAP_DOWN  22
#define SK_QUIT_GAME  23

#define SK_MULTI_VIEW 24

#define SK_TURN_180   25

#define SK_INV_LEFT   26
#define SK_INV_RIGHT  27

#define SK_INV_USE   29
#define SK_HIDE_WEAPON  30
#define SK_SPACE_BAR  31


// REDEFINABLE PLAYER KEYS NUMBERS

#define PK_FORWARD      0
#define PK_BACKWARD     1
#define PK_LEFT         2
#define PK_RIGHT        3
#define PK_RUN          4
#define PK_STRAFE       5
#define PK_SHOOT        6
#define PK_OPERATE      7
#define PK_JUMP         8
#define PK_CRAWL        9
#define PK_LOOK_UP      10
#define PK_LOOK_DOWN    11
#define PK_STRAFE_LEFT  12
#define PK_STRAFE_RIGHT 13
#define PK_MAP          14
#define PK_MULTI_VIEW   15
#define PK_ZOOM_IN      16
#define PK_ZOOM_OUT     17
#define PK_MESSAGE      18

//    #define PKEY(num) KEY_PRESSED(keys[num])

#define MK_FIXED(msw,lsw) (((int32_t)(msw)<<16)|(lsw))
#define FIXED(msw,lsw) MK_FIXED(msw,lsw)

#if B_LITTLE_ENDIAN != 0
# define MSW_VAR(fixed) (*(((uint16_t*)&(fixed)) + 1))
# define LSW_VAR(fixed) (*((uint16_t*)&(fixed)))

# define MSB_VAR(fixed) (*(((uint8_t*)&(fixed)) + 1))
# define LSB_VAR(fixed) (*((uint8_t*)&(fixed)))
#else
# define LSW_VAR(fixed) (*(((uint16_t*)&(fixed)) + 1))
# define MSW_VAR(fixed) (*((uint16_t*)&(fixed)))

# define LSB_VAR(fixed) (*(((uint8_t*)&(fixed)) + 1))
# define MSB_VAR(fixed) (*((uint8_t*)&(fixed)))
#endif

#define MSW(fixed) ((fixed)>>16)
#define LSW(fixed) (((int16_t)(fixed)))

// Defines for reading in ST1 sprite tagging
#define SP_TAG1(sp) ((sp)->hitag)
#define SP_TAG2(sp) ((sp)->lotag)
#define SP_TAG3(sp) ((sp)->clipdist)
#define SP_TAG4(sp) ((sp)->ang)
#define SP_TAG5(sp) ((sp)->xvel)
#define SP_TAG6(sp) ((sp)->yvel)
#define SP_TAG7(sp) (MSB_VAR((sp)->zvel))
#define SP_TAG8(sp) (LSB_VAR((sp)->zvel))
#define SP_TAG9(sp) (MSB_VAR((sp)->owner))
#define SP_TAG10(sp) (LSB_VAR((sp)->owner))
#define SP_TAG11(sp) ((sp)->shade)
#define SP_TAG12(sp) ((sp)->pal)
#define SP_TAG13(sp) B_LITTLE16(*((short*)&(sp)->xoffset))
#define SP_TAG14(sp) B_LITTLE16(*((short*)&(sp)->xrepeat))
#define SP_TAG15(sp) ((sp)->z)
#define SET_SP_TAG13(sp,val) (*((short*)&(sp)->xoffset)) = B_LITTLE16(val)
#define SET_SP_TAG14(sp,val) (*((short*)&(sp)->xrepeat)) = B_LITTLE16(val)

#define SPRITE_TAG1(sp) (sprite[sp].hitag)
#define SPRITE_TAG2(sp) (sprite[sp].lotag)
#define SPRITE_TAG3(sp) (sprite[sp].clipdist)
#define SPRITE_TAG4(sp) (sprite[sp].ang)
#define SPRITE_TAG5(sp) (sprite[sp].xvel)
#define SPRITE_TAG6(sp) (sprite[sp].yvel)
#define SPRITE_TAG7(sp) (MSB_VAR(sprite[sp].zvel))
#define SPRITE_TAG8(sp) (LSB_VAR(sprite[sp].zvel))
#define SPRITE_TAG9(sp) (MSB_VAR(sprite[sp].owner))
#define SPRITE_TAG10(sp) (LSB_VAR(sprite[sp].owner))
#define SPRITE_TAG11(sp) (sprite[sp].shade)
#define SPRITE_TAG12(sp) (sprite[sp].pal)
#define SPRITE_TAG13(sp) B_LITTLE16(*((short*)&sprite[sp].xoffset))
#define SPRITE_TAG14(sp) B_LITTLE16(*((short*)&sprite[sp].xrepeat))
#define SPRITE_TAG15(sp) (sprite[sp].z)
#define SET_SPRITE_TAG13(sp,val) (*((short*)&sprite[sp].xoffset)) = B_LITTLE16(val)
#define SET_SPRITE_TAG14(sp,val) (*((short*)&sprite[sp].xrepeat)) = B_LITTLE16(val)

// this will get you the other wall moved by dragpoint
#define DRAG_WALL(w) (wall[wall[(w)].nextwall].point2)

// OVER and UNDER water macros
#define SpriteInDiveArea(sp) (TEST(sector[(sp)->sectnum].extra, SECTFX_DIVE_AREA) ? TRUE : FALSE)
#define SpriteInUnderwaterArea(sp) (TEST(sector[(sp)->sectnum].extra, SECTFX_UNDERWATER|SECTFX_UNDERWATER2) ? TRUE : FALSE)

#define SectorIsDiveArea(sect) (TEST(sector[sect].extra, SECTFX_DIVE_AREA) ? TRUE : FALSE)
#define SectorIsUnderwaterArea(sect) (TEST(sector[sect].extra, SECTFX_UNDERWATER|SECTFX_UNDERWATER2) ? TRUE : FALSE)

// Key Press Flags macros
#define FLAG_KEY_PRESSED(pp,sync_key) TEST(pp->KeyPressFlags,1<<sync_key)
#define FLAG_KEY_RELEASE(pp,sync_key) RESET(pp->KeyPressFlags,1<<sync_key)
#define FLAG_KEY_RESET(pp,sync_key) SET(pp->KeyPressFlags,1<<sync_key)

// syncbit manipulation macros
// key_test MUST be a boolean - force it to be
#define SET_SYNC_KEY(player, sync_num, key_test) SET((player)->input.bits, ((!!(key_test)) << (sync_num)))
#define TEST_SYNC_KEY(player, sync_num) TEST((player)->input.bits, (1 << (sync_num)))
#define RESET_SYNC_KEY(player, sync_num) RESET((player)->input.bits, (1 << (sync_num)))

#define TRAVERSE_SPRITE_SECT(l, o, n)    for ((o) = (l); (n) = nextspritesect[o], (o) != -1; (o) = (n))
#define TRAVERSE_SPRITE_STAT(l, o, n)    for ((o) = (l); (n) = nextspritestat[o], (o) != -1; (o) = (n))
#define TRAVERSE_CONNECT(i)   for (i = connecthead; i != -1; i = connectpoint2[i])


#define NORM_ANGLE(ang) ((ang) & 2047)
#define ANGLE_2_PLAYER(pp,x,y) (NORM_ANGLE(getangle(pp->posx-(x), pp->posy-(y))))


int StdRandomRange(int range);
#define STD_RANDOM_P2(pwr_of_2) (MOD_P2(rand(),(pwr_of_2)))
#define STD_RANDOM_RANGE(range) (StdRandomRange(range))
#define STD_RANDOM() (rand())

#define MOVEx(vel,ang) (((int)(vel) * (int)sintable[NORM_ANGLE((ang) + 512)]) >> 14)
#define MOVEy(vel,ang) (((int)(vel) * (int)sintable[NORM_ANGLE((ang))]) >> 14)

#define DIST(x1, y1, x2, y2) ksqrt( SQ((x1) - (x2)) + SQ((y1) - (y2)) )

#define PIC_SIZX(sn) (tilesiz[sprite[sn].picnum].x)
#define PIC_SIZY(sn) (tilesiz[sprite[sn].picnum].y)

// Distance macro - tx, ty, tmin are holding vars that must be declared in the routine
// that uses this macro
#define DISTANCE(x1, y1, x2, y2, dist, tx, ty, tmin) \
    {                                    \
        tx = labs(x2-x1);                    \
        ty = labs(y2-y1);                    \
        tmin = min(tx,ty);                   \
        dist = tx + ty - DIV2(tmin);         \
    }

#define SPRITE_SIZE_X(sp_num)   ((sprite[sp_num].xrepeat == 64) ?                         \
                                 tilesiz[sprite[sp_num].picnum].x :                   \
                                 ((sprite[sp_num].xrepeat * tilesiz[sprite[sp_num].picnum].x) >> 6) \
                                 )

#define SPRITE_SIZE_Y(sp_num)   ((sprite[sp_num].yrepeat == 64) ?                          \
                                 tilesiz[sprite[sp_num].picnum].y :                    \
                                 ((sprite[sp_num].yrepeat * tilesiz[sprite[sp_num].picnum].y) >> 6) \
                                 )

#define SPRITE_SIZE_Z(sp_num)   ((sprite[sp_num].yrepeat == 64) ?                          \
                                 Z(tilesiz[sprite[sp_num].picnum].y) :                 \
                                 ((sprite[sp_num].yrepeat * tilesiz[sprite[sp_num].picnum].y) << 2) \
                                 )

#define SPRITEp_SIZE_X(sp)   (((sp)->xrepeat == 64) ?                         \
                              tilesiz[(sp)->picnum].x :                   \
                              (((sp)->xrepeat * tilesiz[(sp)->picnum].x) >> 6) \
                              )

#define SPRITEp_SIZE_Y(sp)   (((sp)->yrepeat == 64) ?                          \
                              tilesiz[(sp)->picnum].y :                    \
                              (((sp)->yrepeat * tilesiz[(sp)->picnum].y) >> 6) \
                              )

#define SPRITEp_SIZE_Z(sp)   (((sp)->yrepeat == 64) ?                          \
                              Z(tilesiz[(sp)->picnum].y) :                 \
                              (((sp)->yrepeat * tilesiz[(sp)->picnum].y) << 2) \
                              )

// Given a z height and sprite return the correct x repeat value
#define SPRITEp_SIZE_X_2_XREPEAT(sp, x) (((x)*64)/tilesiz[(sp)->picnum].x)
// Given a z height and sprite return the correct y repeat value
#define SPRITEp_SIZE_Z_2_YREPEAT(sp, zh) ((zh)/(4*tilesiz[(sp)->picnum].y))
#define SPRITEp_SIZE_Y_2_YREPEAT(sp, y) (((y)*64)/tilesiz[(sp)->picnum].y)


// x & y offset of tile
#define TILE_XOFF(picnum) (picanm[(picnum)].xofs)
#define TILE_YOFF(picnum) (picanm[(picnum)].yofs)

// x & y offset of current sprite tile
#define SPRITEp_XOFF(sp) (picanm[(sp)->picnum].xofs)
#define SPRITEp_YOFF(sp) (picanm[(sp)->picnum].yofs)

// Z size of top (TOS) and bottom (BOS) part of sprite
#define SPRITEp_SIZE_TOS(sp) (DIV2(SPRITEp_SIZE_Z(sp)) + Z(SPRITEp_YOFF(sp)))
#define SPRITEp_SIZE_BOS(sp) (DIV2(SPRITEp_SIZE_Z(sp)) - Z(SPRITEp_YOFF(sp)))

// acual Z for TOS and BOS - handles both WYSIWYG and old style
#define SPRITEp_TOS(sp) (TEST((sp)->cstat, CSTAT_SPRITE_YCENTER) ? \
                         ((sp)->z - SPRITEp_SIZE_TOS(sp)) :         \
                         ((sp)->z - SPRITEp_SIZE_Z(sp)))

#define SPRITEp_BOS(sp) (TEST((sp)->cstat, CSTAT_SPRITE_YCENTER) ? \
                         ((sp)->z + SPRITEp_SIZE_BOS(sp)) :         \
                         (sp)->z)

// mid and upper/lower sprite caluculations
#define SPRITEp_MID(sp) (DIV2(SPRITEp_TOS(sp) + SPRITEp_BOS(sp)))
#define SPRITEp_UPPER(sp) (SPRITEp_TOS(sp) + DIV4(SPRITEp_SIZE_Z(sp)))
#define SPRITEp_LOWER(sp) (SPRITEp_BOS(sp) - DIV4(SPRITEp_SIZE_Z(sp)))

#define Z(value) ((int)(value) << 8)
#define PIXZ(value) ((int)(value) >> 8)

#define SQ(val) ((val) * (val))

#define KENFACING_PLAYER(pp,sp) (sintable[NORM_ANGLE(sp->ang+512)]*(pp->posy-sp->y) >= sintable[NORM_ANGLE(sp-ang)]*(pp->posx-sp->x))
#define FACING_PLAYER(pp,sp) (labs(GetDeltaAngle((sp)->ang, NORM_ANGLE(getangle((pp)->posx - (sp)->x, (pp)->posy - (sp)->y)))) < 512)
#define PLAYER_FACING(pp,sp) (labs(GetDeltaAngle((pp)->pang, NORM_ANGLE(getangle((sp)->x - (pp)->posx, (sp)->y - (pp)->posy)))) < 320)
#define FACING(sp1,sp2) (labs(GetDeltaAngle((sp2)->ang, NORM_ANGLE(getangle((sp1)->x - (sp2)->x, (sp1)->y - (sp2)->y)))) < 512)

#define FACING_PLAYER_RANGE(pp,sp,range) (labs(GetDeltaAngle((sp)->ang, NORM_ANGLE(getangle((pp)->posx - (sp)->x, (pp)->posy - (sp)->y)))) < (range))
#define PLAYER_FACING_RANGE(pp,sp,range) (labs(GetDeltaAngle((pp)->pang, NORM_ANGLE(getangle((sp)->x - (pp)->posx, (sp)->y - (pp)->posy)))) < (range))
#define FACING_RANGE(sp1,sp2,range) (labs(GetDeltaAngle((sp2)->ang, NORM_ANGLE(getangle((sp1)->x - (sp2)->x, (sp1)->y - (sp2)->y)))) < (range))

// two vectors
// can determin direction
#define DOT_PRODUCT_2D(x1,y1,x2,y2) (mulscale16((x1), (x2)) + mulscale16((y1), (y2)))
#define DOT_PRODUCT_3D(x1,y1,z1,x2,y2,z2) (mulscale16((x1), (x2)) + mulscale16((y1), (y2)) + mulscale16((z1), (z2)))

// just determine if the player is moving
#define PLAYER_MOVING(pp) ((pp)->xvect|(pp)->yvect)

#define KEY_EXT(scan) (KEY_PRESSED(scan) | KEY_PRESSED(scan+128))

#define TEST_GOTSECTOR(sect_num) (TEST(gotsector[(sect_num) >> 3], 1 << ((sect_num) & 7)))
#define RESET_GOTSECTOR(sect_num) (RESET(gotsector[(sect_num) >> 3], 1 << ((sect_num) & 7)))
#define SET_GOTSECTOR(sect_num) (SET(gotsector[(sect_num) >> 3], 1 << ((sect_num) & 7)))

#define TEST_GOTPIC(tile_num) (TEST(gotpic[(tile_num) >> 3], 1 << ((tile_num) & 7)))
#define RESET_GOTPIC(tile_num) (RESET(gotpic[(tile_num) >> 3], 1 << ((tile_num) & 7)))
#define SET_GOTPIC(tile_num) (SET(gotpic[(tile_num) >> 3], 1 << ((tile_num) & 7)))

#define LOW_TAG(sectnum) ( sector[sectnum].lotag )
#define HIGH_TAG(sectnum) ( sector[sectnum].hitag )

#define LOW_TAG_SPRITE(spnum) ( sprite[(spnum)].lotag )
#define HIGH_TAG_SPRITE(spnum) ( sprite[(spnum)].hitag )

#define LOW_TAG_WALL(wallnum) ( wall[(wallnum)].lotag )
#define HIGH_TAG_WALL(wallnum) ( wall[(wallnum)].hitag )

#define SEC(value) ((value)*120)

#define CEILING_DIST (Z(4))
#define FLOOR_DIST (Z(4))

// Attributes for monochrome text
#define MDA_BLANK          0x00
#define MDA_NORMAL         0x07
#define MDA_BLINK          0x87
#define MDA_HIGH           0x0F
#define MDA_HIGHBLINK      0x8F
#define MDA_UNDER          0x01
#define MDA_UNDERBLINK     0x81
#define MDA_UNDERHIGH      0x09
#define MDA_UNDERHIGHBLINK 0x89
#define MDA_REVERSE        0x70
#define MDA_REVERSEBLINK   0xF0

// defines for move_sprite return value
#define HIT_MASK (BIT(13)|BIT(14)|BIT(15))
#define HIT_SPRITE (BIT(14)|BIT(15))
#define HIT_WALL   BIT(15)
#define HIT_SECTOR BIT(14)
#define HIT_PLAX_WALL BIT(13)

#define NORM_SPRITE(val) ((val) & (SIZ(sprite) - 1))
#define NORM_WALL(val) ((val) & (SIZ(wall) - 1))
#define NORM_SECTOR(val) ((val) & (SIZ(sector) - 1))

// overwritesprite flags
#define OVER_SPRITE_MIDDLE      (BIT(0))
#define OVER_SPRITE_VIEW_CLIP   (BIT(1))
#define OVER_SPRITE_TRANSLUCENT (BIT(2))
#define OVER_SPRITE_XFLIP       (BIT(3))
#define OVER_SPRITE_YFLIP       (BIT(4))

// rotatesprite flags
#define ROTATE_SPRITE_TRANSLUCENT   (BIT(0))
#define ROTATE_SPRITE_VIEW_CLIP     (BIT(1)) // clip to view
#define ROTATE_SPRITE_YFLIP         (BIT(2))
#define ROTATE_SPRITE_IGNORE_START_MOST (BIT(3)) // don't clip to startumost
#define ROTATE_SPRITE_SCREEN_CLIP   (BIT(1)|BIT(3)) // use window
#define ROTATE_SPRITE_CORNER        (BIT(4)) // place sprite from upper left corner
#define ROTATE_SPRITE_TRANS_FLIP    (BIT(5))
#define ROTATE_SPRITE_NON_MASK      (BIT(6)) // non masked sprites
#define ROTATE_SPRITE_ALL_PAGES     (BIT(7)) // copies to all pages

#define RS_SCALE                    BIT(16)

// system defines for status bits
#define CEILING_STAT_PLAX           BIT(0)
#define CEILING_STAT_SLOPE          BIT(1)
#define CEILING_STAT_SWAPXY         BIT(2)
#define CEILING_STAT_SMOOSH         BIT(3)
#define CEILING_STAT_XFLIP          BIT(4)
#define CEILING_STAT_YFLIP          BIT(5)
#define CEILING_STAT_RELATIVE       BIT(6)
#define CEILING_STAT_TYPE_MASK     (BIT(7)|BIT(8))
#define CEILING_STAT_MASKED         BIT(7)
#define CEILING_STAT_TRANS          BIT(8)
#define CEILING_STAT_TRANS_FLIP     (BIT(7)|BIT(8))
#define CEILING_STAT_FAF_BLOCK_HITSCAN      BIT(15)

#define FLOOR_STAT_PLAX           BIT(0)
#define FLOOR_STAT_SLOPE          BIT(1)
#define FLOOR_STAT_SWAPXY         BIT(2)
#define FLOOR_STAT_SMOOSH         BIT(3)
#define FLOOR_STAT_XFLIP          BIT(4)
#define FLOOR_STAT_YFLIP          BIT(5)
#define FLOOR_STAT_RELATIVE       BIT(6)
#define FLOOR_STAT_TYPE_MASK     (BIT(7)|BIT(8))
#define FLOOR_STAT_MASKED         BIT(7)
#define FLOOR_STAT_TRANS          BIT(8)
#define FLOOR_STAT_TRANS_FLIP     (BIT(7)|BIT(8))
#define FLOOR_STAT_FAF_BLOCK_HITSCAN      BIT(15)

#define CSTAT_WALL_BLOCK            BIT(0)
#define CSTAT_WALL_BOTTOM_SWAP      BIT(1)
#define CSTAT_WALL_ALIGN_BOTTOM     BIT(2)
#define CSTAT_WALL_XFLIP            BIT(3)
#define CSTAT_WALL_MASKED           BIT(4)
#define CSTAT_WALL_1WAY             BIT(5)
#define CSTAT_WALL_BLOCK_HITSCAN    BIT(6)
#define CSTAT_WALL_TRANSLUCENT      BIT(7)
#define CSTAT_WALL_YFLIP            BIT(8)
#define CSTAT_WALL_TRANS_FLIP       BIT(9)
#define CSTAT_WALL_BLOCK_ACTOR (BIT(14)) // my def
#define CSTAT_WALL_WARP_HITSCAN (BIT(15)) // my def

//cstat, bit 0: 1 = Blocking sprite (use with clipmove, getzrange)    "B"
//       bit 1: 1 = 50/50 transluscence, 0 = normal                   "T"
//       bit 2: 1 = x-flipped, 0 = normal                             "F"
//       bit 3: 1 = y-flipped, 0 = normal                             "F"
//       bits 5-4: 00 = FACE sprite (default)                         "R"
//                 01 = WALL sprite (like masked walls)
//                 10 = FLOOR sprite (parallel to ceilings&floors)
//                 11 = SPIN sprite (face sprite that can spin 2draw style - not done yet)
//       bit 6: 1 = 1-sided sprite, 0 = normal                        "1"
//       bit 7: 1 = Real centered centering, 0 = foot center          "C"
//       bit 8: 1 = Blocking sprite (use with hitscan)                "H"
//       bit 9: reserved
//       bit 10: reserved
//       bit 11: reserved
//       bit 12: reserved
//       bit 13: reserved
//       bit 14: reserved
//       bit 15: 1 = Invisible sprite, 0 = not invisible

#define CSTAT_SPRITE_BLOCK          BIT(0)
#define CSTAT_SPRITE_TRANSLUCENT    BIT(1)
#define CSTAT_SPRITE_XFLIP          BIT(2)
#define CSTAT_SPRITE_YFLIP          BIT(3)
#define CSTAT_SPRITE_WALL           BIT(4)
#define CSTAT_SPRITE_FLOOR          BIT(5)
#define CSTAT_SPRITE_SLAB           (BIT(4)|BIT(5))
#define CSTAT_SPRITE_ONE_SIDE       BIT(6)
#define CSTAT_SPRITE_YCENTER        BIT(7)
#define CSTAT_SPRITE_BLOCK_HITSCAN  BIT(8)
#define CSTAT_SPRITE_TRANS_FLIP     BIT(9)

#define CSTAT_SPRITE_RESTORE        BIT(12) // my def
#define CSTAT_SPRITE_CLOSE_FLOOR    BIT(13) // my def - tells whether a sprite
// started out close to a ceiling or floor
#define CSTAT_SPRITE_BLOCK_MISSILE  BIT(14) // my def
#define CSTAT_SPRITE_INVISIBLE      BIT(15)

#define CSTAT_SPRITE_BREAKABLE (CSTAT_SPRITE_BLOCK_HITSCAN|CSTAT_SPRITE_BLOCK_MISSILE)

#undef CLIPMASK0 // defined in build.h
#undef CLIPMASK1

// new define more readable defines

// Clip Sprite adjustment
#define CS(sprite_bit) ((sprite_bit)<<16)

// for players to clip against walls
#define CLIPMASK_PLAYER (CS(CSTAT_SPRITE_BLOCK) | CSTAT_WALL_BLOCK)

// for actors to clip against walls
#define CLIPMASK_ACTOR                   \
    (                                    \
        CS(CSTAT_SPRITE_BLOCK) |             \
        CSTAT_WALL_BLOCK |                   \
        CSTAT_WALL_BLOCK_ACTOR               \
    )

// for missiles to clip against actors
#define CLIPMASK_MISSILE                                            \
    (                                                               \
        CS(CSTAT_SPRITE_BLOCK_HITSCAN|CSTAT_SPRITE_BLOCK_MISSILE) |     \
        CSTAT_WALL_BLOCK_HITSCAN                                        \
    )

#define CLIPMASK_WARP_HITSCAN            \
    (                                    \
        CS(CSTAT_SPRITE_BLOCK_HITSCAN) |     \
        CSTAT_WALL_BLOCK_HITSCAN |           \
        CSTAT_WALL_WARP_HITSCAN              \
    )


#define SIZ ARRAY_SIZE


//
// Directions
//

#define DEGREE_45 256
#define DEGREE_90 512

////
//
// Directional enumerations
//
////

enum DirOrd
{
    ORD_NORTH, ORD_NE, ORD_EAST, ORD_SE, ORD_SOUTH, ORD_SW, ORD_WEST, ORD_NW
};

enum Dir8
{
    NORTH   = ORD_NORTH * DEGREE_45,
    NE      = ORD_NE    * DEGREE_45,
    EAST    = ORD_EAST  * DEGREE_45,
    SE      = ORD_SE    * DEGREE_45,
    SOUTH   = ORD_SOUTH * DEGREE_45,
    SW      = ORD_SW    * DEGREE_45,
    WEST    = ORD_WEST  * DEGREE_45,
    NW      = ORD_NW    * DEGREE_45,
};

typedef enum Dir8 DIR8;

// Auto building enumerations

#define DIGI_ENUM
enum digi
{
#include "digi.h"
    DIGI_MAX
};
#undef DIGI_ENUM

#define DAMAGE_ENUM
enum dam
{
#include "damage.h"
};
#undef DAMAGE_ENUM

////
//
// State declarations
//
////


// Forward declarations
struct STATEstruct;
typedef struct STATEstruct STATE, *STATEp, * *STATEpp;

//struct PIC_STATEstruct;
//typedef struct PIC_STATEstruct PIC_STATE, *PIC_STATEp;

struct PANEL_STATEstruct;
typedef struct PANEL_STATEstruct PANEL_STATE, *PANEL_STATEp;

struct PLAYERstruct;
typedef struct PLAYERstruct PLAYER, *PLAYERp;

struct PERSONALITYstruct;
typedef struct PERSONALITYstruct PERSONALITY, *PERSONALITYp;

struct ATTRIBUTEstruct;
typedef struct ATTRIBUTEstruct ATTRIBUTE, *ATTRIBUTEp;

struct SECTOR_OBJECTstruct;
typedef struct SECTOR_OBJECTstruct SECTOR_OBJECT, *SECTOR_OBJECTp;

struct PANEL_SPRITEstruct;
typedef struct PANEL_SPRITEstruct PANEL_SPRITE, *PANEL_SPRITEp;

struct ANIMstruct;
typedef struct ANIMstruct ANIM, *ANIMp;

typedef int ANIMATOR (int16_t SpriteNum);
typedef ANIMATOR *ANIMATORp;

typedef void pANIMATOR (PANEL_SPRITEp);
typedef pANIMATOR *pANIMATORp;

typedef void soANIMATOR (SECTOR_OBJECTp);
typedef soANIMATOR *soANIMATORp;

typedef spritetype SPRITE, *SPRITEp;
typedef sectortype SECTOR, *SECTORp;
typedef walltype WALL, *WALLp;

struct STATEstruct
{
    short     Pic;
    int       Tics;
    ANIMATORp Animator;

    STATEp   NextState;
};

//
// State Flags
//

#define SF_TICS_MASK 0xFFFF
#define SF_QUICK_CALL BIT(16)
#define SF_PLAYER_FUNC BIT(17) // only for players to execute
#define SF_TIC_ADJUST BIT(18) // use tic adjustment for these frames
#define SF_WALL_STATE BIT(19) // use for walls instead of sprite

///////////////////////////////////////////////////////////////////////////////
// Jim's MISC declarations from other files
///////////////////////////////////////////////////////////////////////////////

typedef enum {WATER_FOOT, BLOOD_FOOT} FOOT_TYPE;

extern FOOT_TYPE FootMode;
extern SWBOOL InGame;                                  // Declared in game.c
extern SWBOOL Global_PLock;                            // Game.c
int QueueFloorBlood(short hit_sprite);                // Weapon.c
int QueueFootPrint(short hit_sprite);                 // Weapon.c
int QueueGeneric(short SpriteNum, short pic);        // Weapon.c
int QueueLoWangs(short SpriteNum);                   // Weapon.c
int SpawnShell(short SpriteNum, short ShellNum);     // Weapon.c
void UnlockKeyLock(short key_num, short hit_sprite);  // JSector.c

#define MAX_PAIN 5
extern int PlayerPainVocs[MAX_PAIN];
extern int PlayerLowHealthPainVocs[MAX_PAIN];

#define MAX_TAUNTAI 33
extern int TauntAIVocs[MAX_TAUNTAI];

#define MAX_GETSOUNDS 5
extern int PlayerGetItemVocs[MAX_GETSOUNDS];

#define MAX_YELLSOUNDS 3
extern int PlayerYellVocs[MAX_YELLSOUNDS];

void BossHealthMeter(void);

// Global variables used for modifying variouse things from the Console

///////////////////////////////////////////////////////////////////////////////////////////
//
// CALLER - DLL handler
//
///////////////////////////////////////////////////////////////////////////////////////////
extern unsigned char DLL_Loaded;
extern int DLL_Handle; // Global DLL handle
extern char *DLL_path; // DLL path name

int DLL_Load(char *DLLpathname);
SWBOOL DLL_Unload(int procHandle);
SWBOOL DLL_ExecFunc(int procHandle, char *fName);

///////////////////////////////////////////////////////////////////////////////////////////
//
// JPlayer
//
///////////////////////////////////////////////////////////////////////////////////////////
#define MESSAGE_LINE 142    // Used to be 164
#define MAXUSERQUOTES 6
#define MAXCONQUOTES 13

extern int quotebot, quotebotgoal;
extern short user_quote_time[MAXUSERQUOTES];
extern char user_quote[MAXUSERQUOTES][256];

extern int conbot, conbotgoal;
extern char con_quote[MAXCONQUOTES][256];

int minitext(int x,int y,char *t,char p,char sb);
int minitextshade(int x,int y,char *t,char s,char p,char sb);
void operatefta(void);
void adduserquote(char *daquote);
void operateconfta(void);
void addconquote(char *daquote);

///////////////////////////////////////////////////////////////////////////////////////////
//
// Console
//
///////////////////////////////////////////////////////////////////////////////////////////
void CON_Message(const char *message, ...) PRINTF_FORMAT(1, 2);
void CON_ConMessage(const char *message, ...) PRINTF_FORMAT(1, 2);
void CON_StoreArg(const char *userarg);
SWBOOL CON_CheckParm(const char *userarg);
void CON_CommandHistory(signed char dir);
SWBOOL CON_AddCommand(const char *command, void (*function)(void));
void CON_ProcessUserCommand(void);
void CON_InitConsole(void);

///////////////////////////////////////////////////////////////////////////////////////////
//
// Weapon
//
///////////////////////////////////////////////////////////////////////////////////////////

#define MAX_WEAPONS_KEYS 10
#define MAX_WEAPONS_EXTRA 4 // extra weapons like the two extra head attacks
#define MAX_WEAPONS (MAX_WEAPONS_KEYS + MAX_WEAPONS_EXTRA)

// weapons that not missile type sprites
#define WPN_NM_LAVA (-8)
#define WPN_NM_SECTOR_SQUISH (-9)

//#define WEAP_ENTRY(id, init_func, damage_lo, damage_hi, radius)

typedef struct
{
    void (*Init)(PLAYERp);
    int16_t damage_lo;
    int16_t damage_hi;
    unsigned int radius;
    int16_t max_ammo;
    int16_t min_ammo;
    int16_t with_weapon;
    char *weapon_name;
    char *ammo_name;
    int16_t weapon_pickup;
    int16_t ammo_pickup;
} DAMAGE_DATA, *DAMAGE_DATAp;

extern DAMAGE_DATA DamageData[];

// bit arrays that determine if a) Weapon has no ammo b) Weapon is the ammo (no weapon exists)
extern int WeaponHasNoAmmo, WeaponIsAmmo;


void InitWeaponFist(PLAYERp);
void InitWeaponStar(PLAYERp);
void InitWeaponShotgun(PLAYERp);
void InitWeaponRocket(PLAYERp);
void InitWeaponRail(PLAYERp);
void InitWeaponMicro(PLAYERp);
void InitWeaponUzi(PLAYERp);
void InitWeaponSword(PLAYERp);
void InitWeaponHothead(PLAYERp);
void InitWeaponElectro(PLAYERp);
void InitWeaponHeart(PLAYERp);
void InitWeaponGrenade(PLAYERp);
void InitWeaponMine(PLAYERp);

void InitWeaponNapalm(PLAYERp);
void InitWeaponRing(PLAYERp);

extern void (*InitWeapon[MAX_WEAPONS]) (PLAYERp);

///////////////////////////////////////////////////////////////////////////////////////////
//
// Player
//
///////////////////////////////////////////////////////////////////////////////////////////

#define MAX_SW_PLAYERS_SW  (4)
#define MAX_SW_PLAYERS_REG (8)
#define MAX_SW_PLAYERS (isShareware ? MAX_SW_PLAYERS_SW : MAX_SW_PLAYERS_REG)

typedef struct
{
    char map_name[16];
    char numplayers;
    char Episode,Level;
    char LevelSong[16];
} DEMO_HEADER, *DEMO_HEADERp;

typedef struct
{
    int x,y,z;
} DEMO_START_POS, *DEMO_START_POSp;

#define MAX_LEVELS_REG 29
#define MAX_LEVELS_SW 4
#define MAX_LEVELS (isShareware ? MAX_LEVELS_SW : MAX_LEVELS_REG)

typedef struct
{
    char *LevelName;
    char *SongName;
    char *Description;
    char *BestTime;
    char *ParTime;
} LEVEL_INFO, *LEVEL_INFOp, * *LEVEL_INFOpp;

extern LEVEL_INFO LevelInfo[MAX_LEVELS_REG+2];

#define MAX_EPISODE_NAME_LEN 24
extern char EpisodeNames[2][MAX_EPISODE_NAME_LEN+2];    // +2 = leading '^' and trailing NULL

#define MAX_EPISODE_SUBTITLE_LEN 40
extern char EpisodeSubtitles[2][MAX_EPISODE_SUBTITLE_LEN+1];

#define MAX_SKILL_NAME_LEN 24
extern char SkillNames[4][MAX_SKILL_NAME_LEN+2];

#define MAX_FORTUNES 16
extern char *ReadFortune[MAX_FORTUNES];

#define MAX_KEYS 8
extern char *KeyMsg[MAX_KEYS];
extern char *KeyDoorMessage[MAX_KEYS];

typedef struct
{
    int16_t vel;
    int16_t svel;
    int8_t angvel;
    int8_t aimvel;
    int32_t bits;
} SW_PACKET;

extern SW_PACKET loc;

#define PACK 1

extern SWBOOL CameraTestMode;

enum PlayerDeathTypes
{
    PLAYER_DEATH_FLIP, PLAYER_DEATH_CRUMBLE, PLAYER_DEATH_EXPLODE, PLAYER_DEATH_RIPPER, PLAYER_DEATH_SQUISH, PLAYER_DEATH_DROWN, MAX_PLAYER_DEATHS
};

typedef void (*PLAYER_ACTION_FUNCp)(PLAYERp);

#include "inv.h"

typedef struct
{
    short cursectnum,lastcursectnum,pang,filler;
    int xvect,yvect,oxvect,oyvect,slide_xvect,slide_yvect;
    int posx,posy,posz;
    SECTOR_OBJECTp sop_control;
} REMOTE_CONTROL, *REMOTE_CONTROLp;

struct PLAYERstruct
{
    // variable that fit in the sprite or user structure
    int32_t posx, posy, posz;
    // interpolation
    int
        oposx, oposy, oposz;
    short oang;
    short ohoriz;

    // holds last valid move position
    short lv_sectnum;
    int lv_x,lv_y,lv_z;

    SPRITEp remote_sprite;
    REMOTE_CONTROL remote;
    SECTOR_OBJECTp sop_remote;
    SECTOR_OBJECTp sop;  // will either be sop_remote or sop_control

    int jump_count, jump_speed;     // jumping
    short down_speed, up_speed; // diving
    int z_speed,oz_speed; // used for diving and flying instead of down_speed, up_speed
    int climb_ndx;
    int hiz,loz;
    int ceiling_dist,floor_dist;
    SECTORp hi_sectp, lo_sectp;
    SPRITEp hi_sp, lo_sp;

    SPRITEp last_camera_sp;
    int camera_dist; // view mode dist
    int circle_camera_dist;
    int six,siy,siz; // save player interp position for PlayerSprite
    short siang;

    int xvect, yvect;
    int oxvect, oyvect;
    int friction;
    int slide_xvect, slide_yvect;
    short slide_ang;
    int slide_dec;
    int drive_angvel;
    int drive_oangvel;



    // scroll 2D mode stuff
    int scr_x, scr_y, oscr_x, oscr_y;
    int scr_xvect, scr_yvect;
    short scr_ang, oscr_ang, scr_sectnum;

    short view_outside_dang;  // outside view delta ang
    short circle_camera_ang;
    short camera_check_time_delay;

    short pang,cursectnum,lastcursectnum;
    short turn180_target; // 180 degree turn

    // variables that do not fit into sprite structure
    int horizbase,horiz,horizoff,hvel,tilt,tilt_dest;
    short recoil_amt;
    short recoil_speed;
    short recoil_ndx;
    short recoil_horizoff;

    int oldposx,oldposy,oldposz;
    int RevolveX, RevolveY;
    short RevolveDeltaAng, RevolveAng;

    // under vars are for wading and swimming
    short PlayerSprite, PlayerUnderSprite;
    SPRITEp SpriteP, UnderSpriteP;


    short pnum; // carry along the player number

    short LadderSector,LadderAngle;
    int lx,ly; // ladder x and y
    short JumpDuration;
    short WadeDepth;
    short bob_amt;
    short bob_ndx;
    short bcnt; // bob count
    int bob_z;

    //Multiplayer variables
    SW_PACKET input;

    //FIFO queue to hold values while faketimerhandler is called from within the drawing routing
#define MOVEFIFOSIZ 256
    SW_PACKET inputfifo[MOVEFIFOSIZ];


    int movefifoend;
    int myminlag;
    int syncvalhead;
#define MAXSYNCBYTES 16
    // TENSW: on really bad network connections, the sync FIFO queue can overflow if it is the
    // same size as the move fifo.
#define SYNCFIFOSIZ 1024
    uint8_t syncval[SYNCFIFOSIZ][MAXSYNCBYTES];

    // must start out as 0
    int playerreadyflag;

    PLAYER_ACTION_FUNCp DoPlayerAction;
    int Flags, Flags2;
    int KeyPressFlags;

    SECTOR_OBJECTp sop_control; // sector object pointer
    SECTOR_OBJECTp sop_riding; // sector object pointer

    struct
    {
        PANEL_SPRITEp Next, Prev;
    } PanelSpriteList;

    // Key stuff
#define NUM_KEYS 8
    unsigned char HasKey[NUM_KEYS];

    // Weapon stuff
    short SwordAng;
    int WpnGotOnceFlags; // for no respawn mode where weapons are allowed grabbed only once
    int WpnFlags;
    short WpnAmmo[MAX_WEAPONS];
    short WpnNum;
    PANEL_SPRITEp CurWpn;
    PANEL_SPRITEp Wpn[MAX_WEAPONS];
    PANEL_SPRITEp Chops;
    unsigned char WpnRocketType; // rocket type
    unsigned char WpnRocketHeat; // 5 to 0 range
    unsigned char WpnRocketNuke; // 1, you have it, or you don't
    unsigned char WpnFlameType; // Guardian weapons fire
    unsigned char WpnFirstType; // First weapon type - Sword/Shuriken
    unsigned char WeaponType; // for weapons with secondary functions
    short FirePause; // for sector objects - limits rapid firing
    //
    // Inventory Vars
    //
    short InventoryNum;
    short InventoryBarTics;
    PANEL_SPRITEp InventorySprite[MAX_INVENTORY];
    PANEL_SPRITEp InventorySelectionBox;
    PANEL_SPRITEp MiniBarHealthBox, MiniBarAmmo;
    PANEL_SPRITEp MiniBarHealthBoxDigit[3], MiniBarAmmoDigit[3];
    short InventoryTics[MAX_INVENTORY];
    short InventoryPercent[MAX_INVENTORY];
    int8_t InventoryAmount[MAX_INVENTORY];
    SWBOOL InventoryActive[MAX_INVENTORY];

    short DiveTics;
    short DiveDamageTics;

    // Death stuff
    short DeathType;
    short Kills;
    short Killer;  //who killed me
    short KilledPlayer[MAX_SW_PLAYERS_REG];
    short SecretsFound;

    // Health
    short Armor;
    short MaxHealth;

    //char RocketBarrel;
    char PlayerName[32];

    unsigned char UziShellLeftAlt;
    unsigned char UziShellRightAlt;
    unsigned char TeamColor;  // used in team play and also used in regular mulit-play for show

    // palette fading up and down for player hit and get items
    short FadeTics;                 // Tics between each fade cycle
    short FadeAmt;                  // Current intensity of fade
    SWBOOL NightVision;               // Is player's night vision active?
    unsigned char StartColor;       // Darkest color in color range being used
    //short electro[64];
    unsigned char temp_pal[768];    // temporary working palette
    SWBOOL IsAI;                      // Is this and AI character?
    short fta,ftq;                  // First time active and first time quote, for talking in multiplayer games
    short NumFootPrints;            // Number of foot prints left to lay down
    SWBOOL PlayerTalking;             // Is player currently talking
    int TalkVocnum;                 // Number of sound that player is using
    int TalkVocHandle;              // Handle of sound in sound queue, to access in Dose's code
    unsigned char WpnUziType;                // Toggle between single or double uzi's if you own 2.
    unsigned char WpnShotgunType;            // Shotgun has normal or fully automatic fire
    unsigned char WpnShotgunAuto;            // 50-0 automatic shotgun rounds
    unsigned char WpnShotgunLastShell;       // Number of last shell fired
    unsigned char WpnRailType;               // Normal Rail Gun or EMP Burst Mode
    SWBOOL Bloody;                    // Is player gooey from the slaughter?
    int nukevochandle;              // Stuff for the Nuke
    SWBOOL InitingNuke;
    SWBOOL TestNukeInit;
    SWBOOL NukeInitialized;           // Nuke already has counted down
    short FistAng;                  // KungFu attack angle
    unsigned char WpnKungFuMove;             // KungFu special moves
    SWBOOL BunnyMode;                 // Can shoot Bunnies out of rocket launcher
    short HitBy;                    // SpriteNum of whatever player was last hit by
    short Reverb;                   // Player's current reverb setting
    short Heads;                    // Number of Accursed Heads orbiting player
    int PlayerVersion;
};

extern PLAYER Player[MAX_SW_PLAYERS_REG+1];


//
// Player Flags
//

#define PF_DEAD             (BIT(1))
#define PF_JUMPING          (BIT(2))
#define PF_FALLING          (BIT(3))
#define PF_LOCK_CRAWL       (BIT(4))
#define PF_LOCK_HORIZ       (BIT(5))
#define PF_LOOKING          (BIT(6))
#define PF_PLAYER_MOVED     (BIT(7))
#define PF_PLAYER_RIDING    (BIT(8))
#define PF_AUTO_AIM         (BIT(9))
#define PF_RECOIL           (BIT(10))

#define PF_FLYING           (BIT(11))
#define PF_WEAPON_RETRACT   (BIT(12))
#define PF_PICKED_UP_AN_UZI (BIT(13))
#define PF_CRAWLING         (BIT(14))
#define PF_CLIMBING         (BIT(15))
#define PF_SWIMMING         (BIT(16))
#define PF_DIVING           (BIT(17))
#define PF_DIVING_IN_LAVA   (BIT(18))
#define PF_TWO_UZI          (BIT(19))
#define PF_LOCK_RUN         (BIT(20))
#define PF_TURN_180         (BIT(21))
#define PF_DEAD_HEAD        (BIT(22)) // are your a dead head
#define PF_HEAD_CONTROL     (BIT(23)) // have control of turning when a head?
#define PF_CLIP_CHEAT       (BIT(24)) // cheat for wall clipping
#define PF_SLIDING          (BIT(25)) // cheat for wall clipping
#define PF_VIEW_FROM_OUTSIDE   (BIT(26))
#define PF_VIEW_OUTSIDE_WEAPON (BIT(27))
#define PF_VIEW_FROM_CAMERA   (BIT(28))
#define PF_TANK             (BIT(29)) // Doin the tank thang
#define PF_MOUSE_AIMING_ON (BIT(30))
#define PF_WEAPON_DOWN       (BIT(31))

#define PF2_TELEPORTED        (BIT(0))

///////////////////////////////////////////////////////////////////////////////////////////
//
// Actor
//
///////////////////////////////////////////////////////////////////////////////////////////

//
// Hit Points
//

#define HEALTH_RIPPER            70
#define HEALTH_RIPPER2           200
#define HEALTH_MOMMA_RIPPER      500
#define HEALTH_NINJA             40
#define HEALTH_RED_NINJA         160
#define HEALTH_COOLIE            120
#define HEALTH_COOLIE_GHOST      65
#define HEALTH_SKEL_PRIEST       90
#define HEALTH_GORO              200
#define HEALTH_HORNET            4
#define HEALTH_SKULL             4
#define HEALTH_EEL               100

#define HEALTH_SERP_GOD          3800

//
// Action Set Structure
//

typedef struct
{
#define MAX_ACTOR_CLOSE_ATTACK 2
#define MAX_ACTOR_ATTACK 6
    STATEp *Stand;
    STATEp *Run;
    STATEp *Jump;
    STATEp *Fall;
    STATEp *Crawl;
    STATEp *Swim;
    STATEp *Fly;
    STATEp *Rise;
    STATEp *Sit;
    STATEp *Look;
    STATEp *Climb;
    STATEp *Pain;
    STATEp *Death1;
    STATEp *Death2;
    STATEp *Dead;
    STATEp *DeathJump;
    STATEp *DeathFall;

    STATEp *CloseAttack[MAX_ACTOR_CLOSE_ATTACK];
    short  CloseAttackPercent[MAX_ACTOR_CLOSE_ATTACK];

    STATEp *Attack[MAX_ACTOR_ATTACK];
    short  AttackPercent[MAX_ACTOR_ATTACK];

    STATEp *Special[2];
    STATEp *Duck;
    STATEp *Dive;
} ACTOR_ACTION_SET,*ACTOR_ACTION_SETp;

typedef struct
{
    int pos;           // current position - always moves toward tgt
    int open_dest;     // destination of open position
    int tgt;           // current target
    int speed;         // speed of movement
    int orig_speed;    // original speed - vel jacks with speed
    int vel;           // velocity adjuments
    int num_walls;     // save off positions of walls for rotator
    int *origx;
    int *origy;
} ROTATOR, *ROTATORp;

//
// User Extension record
//

typedef struct
{
    //
    // Variables that can be used by actors and Player
    //
    ROTATORp rotator;

    // wall vars for lighting
    int WallCount;
    int8_t* WallShade; // malloced - save off wall shades for lighting

    WALLp WallP; // operate on wall instead of sprite
    STATEp State;
    STATEp *Rot;
    STATEp StateStart;
    STATEp StateEnd;
    STATEp *StateFallOverride; // a bit kludgy - override std fall state

    ANIMATORp ActorActionFunc;
    ACTOR_ACTION_SETp ActorActionSet;
    PERSONALITYp Personality;
    ATTRIBUTEp Attrib;
    SECTOR_OBJECTp sop_parent;  // denotes that this sprite is a part of the
    // sector object - contains info for the SO

    int ox, oy, oz;

    int Flags;
    int Flags2;
    int Tics;

    short RotNum;
    short ID;

    // Health/Pain related
    short Health;
    short MaxHealth;

    short LastDamage;           // last damage amount taken
    short PainThreshold;       // amount of damage that can be taken before
    // going into pain frames.

    // jump & fall
    short jump_speed;
    short jump_grav;

    // clipmove
    short ceiling_dist;
    short floor_dist;
    short lo_step;
    int hiz,loz;
    int zclip; // z height to move up for clipmove
    SECTORp hi_sectp, lo_sectp;
    SPRITEp hi_sp, lo_sp;

    int active_range;

    short   SpriteNum;
    short   Attach;  // attach to sprite if needed - electro snake
    SPRITEp SpriteP;

    // if a player's sprite points to player structure
    PLAYERp PlayerP;
    short Sibling;


    //
    // Possibly used by both.
    //

    // precalculated vectors
    int xchange,ychange,zchange;

    int  z_tgt;

    // velocity
    int  vel_tgt;
    short vel_rate;
    uint8_t speed; // Ordinal Speed Range 0-3 from slow to fast

    short Counter;
    short Counter2;
    short Counter3;
    short DamageTics;
    short BladeDamageTics;

    short WpnGoal;
    unsigned int Radius;    // for distance checking
    int  OverlapZ;  // for z overlap variable

    //
    // Only have a place for actors
    //

    // For actors on fire
    short flame;

    // target player for the enemy - can only handle one player at at time
    //PLAYERp tgt_player;
    SPRITEp tgt_sp;

    // scaling
    short scale_speed;
    unsigned short scale_value;
    short scale_tgt;

    // zig zagging
    short DistCheck;
    //short ZigZagDist;
    //short ZigZagAng;
    //short ZigZagDir;

    short Dist;
    short TargetDist;
    short WaitTics;

    // track
    short track;
    short point;
    short track_dir;
    int  track_vel;

    // sliding variables - slide backwards etc
    short slide_ang;
    int  slide_vel;
    short slide_dec;

    short motion_blur_dist;
    short motion_blur_num;

    short wait_active_check;  // for enemy checking of player
    short inactive_time; // length of time actor has been unaware of his tgt
    int  sx,sy,sz;
    short sang;
    char spal;  // save off default palette number

    int ret; //holder for move_sprite return value

    // Need to get rid of these flags
    int  Flag1;

    int8_t  LastWeaponNum;
    int8_t  WeaponNum;

    short bounce;           // count bounces off wall for killing shrap stuff
    // !JIM! my extensions
    int ShellNum;          // This is shell no. 0 to whatever
    // Shell gets deleted when ShellNum < (ShellCount - MAXSHELLS)
    short FlagOwner;        // The spritenum of the original flag
    short Vis;              // Shading upgrade, for shooting, etc...
    SWBOOL DidAlert;          // Has actor done his alert noise before?

    uint8_t filler;
} USER,*USERp;

// sprite->extra flags
// BUILD AND GAME - DO NOT MOVE THESE
#define SPRX_SKILL              (BIT(0) | BIT(1) | BIT(2))

// BIT(4) ST1 BUILD AND GAME
#define SPRX_STAY_PUT_VATOR     (BIT(5))    // BUILD AND GAME - will not move with vators etc
// DO NOT MOVE THIS

#define SPRX_STAG               (BIT(6))    // BUILD AND GAME - NON-ST1 sprite with ST1 type tagging
// DO NOT MOVE

#define SPRX_QUEUE_SPRITE       (BIT(7))    // Queue sprite -check queue when deleting
#define SPRX_MULTI_ITEM         (BIT(9))    // BUILD AND GAME - multi player item

// have users - could be moved
#define SPRX_PLAYER_OR_ENEMY    (BIT(11))   // for checking quickly if sprite is a
// player or actor
// do not need Users
#define SPRX_FOUND              (BIT(12))   // BUILD ONLY INTERNAL - used for finding sprites
#define SPRX_BLADE              (BIT(12))   // blade sprite
#define SPRX_BREAKABLE          (BIT(13))   // breakable items
#define SPRX_BURNABLE           (BIT(14))   // used for burnable sprites in the game

// temp use
#define SPRX_BLOCK              (BIT(15))   // BUILD AND GAME
// BUILD - tell which actors should not spawn
// GAME - used for internal game code
// ALT-M debug mode

// !LIGHT
// all three bits set - should never happen with skill
// #define SPRX_USER_NON_STANDARD  (BIT(0)|BIT(1)|BIT(2))   // used for lighting

// boolean flags carried over from build
#define SPRX_BOOL11 (BIT(5))
#define SPRX_BOOL1 (BIT(6))
#define SPRX_BOOL2 (BIT(7))
#define SPRX_BOOL3 (BIT(8))
#define SPRX_BOOL4 (BIT(9))
#define SPRX_BOOL5 (BIT(10))
#define SPRX_BOOL6 (BIT(11))
#define SPRX_BOOL7 (BIT(4))  // bit 12 was used build
#define SPRX_BOOL8 (BIT(13))
#define SPRX_BOOL9 (BIT(14))
#define SPRX_BOOL10 (BIT(15))

#define SET_BOOL1(sp) SET((sp)->extra, SPRX_BOOL1)
#define SET_BOOL2(sp) SET((sp)->extra, SPRX_BOOL2)
#define SET_BOOL3(sp) SET((sp)->extra, SPRX_BOOL3)
#define SET_BOOL4(sp) SET((sp)->extra, SPRX_BOOL4)
#define SET_BOOL5(sp) SET((sp)->extra, SPRX_BOOL5)
#define SET_BOOL6(sp) SET((sp)->extra, SPRX_BOOL6)
#define SET_BOOL7(sp) SET((sp)->extra, SPRX_BOOL7)
#define SET_BOOL8(sp) SET((sp)->extra, SPRX_BOOL8)
#define SET_BOOL9(sp) SET((sp)->extra, SPRX_BOOL9)
#define SET_BOOL10(sp) SET((sp)->extra, SPRX_BOOL10)
#define SET_BOOL11(sp) SET((sp)->extra, SPRX_BOOL11)

#define RESET_BOOL1(sp) RESET((sp)->extra, SPRX_BOOL1)
#define RESET_BOOL2(sp) RESET((sp)->extra, SPRX_BOOL2)
#define RESET_BOOL3(sp) RESET((sp)->extra, SPRX_BOOL3)
#define RESET_BOOL4(sp) RESET((sp)->extra, SPRX_BOOL4)
#define RESET_BOOL5(sp) RESET((sp)->extra, SPRX_BOOL5)
#define RESET_BOOL6(sp) RESET((sp)->extra, SPRX_BOOL6)
#define RESET_BOOL7(sp) RESET((sp)->extra, SPRX_BOOL7)
#define RESET_BOOL8(sp) RESET((sp)->extra, SPRX_BOOL8)
#define RESET_BOOL9(sp) RESET((sp)->extra, SPRX_BOOL9)
#define RESET_BOOL10(sp) RESET((sp)->extra, SPRX_BOOL10)
#define RESET_BOOL11(sp) RESET((sp)->extra, SPRX_BOOL11)

#define TEST_BOOL1(sp) TEST((sp)->extra, SPRX_BOOL1)
#define TEST_BOOL2(sp) TEST((sp)->extra, SPRX_BOOL2)
#define TEST_BOOL3(sp) TEST((sp)->extra, SPRX_BOOL3)
#define TEST_BOOL4(sp) TEST((sp)->extra, SPRX_BOOL4)
#define TEST_BOOL5(sp) TEST((sp)->extra, SPRX_BOOL5)
#define TEST_BOOL6(sp) TEST((sp)->extra, SPRX_BOOL6)
#define TEST_BOOL7(sp) TEST((sp)->extra, SPRX_BOOL7)
#define TEST_BOOL8(sp) TEST((sp)->extra, SPRX_BOOL8)
#define TEST_BOOL9(sp) TEST((sp)->extra, SPRX_BOOL9)
#define TEST_BOOL10(sp) TEST((sp)->extra, SPRX_BOOL10)
#define TEST_BOOL11(sp) TEST((sp)->extra, SPRX_BOOL11)

// User->Flags flags
#define SPR_MOVED               BIT(0) // Did actor move
#define SPR_ATTACKED            BIT(1) // Is sprite being attacked?
#define SPR_TARGETED            BIT(2) // Is sprite a target of a weapon?
#define SPR_ACTIVE              BIT(3) // Is sprite aware of the player?
#define SPR_ELECTRO_TOLERANT    BIT(4) // Electro spell does not slow actor
#define SPR_JUMPING             BIT(5) // Actor is jumping
#define SPR_FALLING             BIT(6) // Actor is falling
#define SPR_CLIMBING            BIT(7) // Actor is falling
#define SPR_DEAD               BIT(8) // Actor is dying

#define SPR_ZDIFF_MODE          BIT(10) // For following tracks at different z heights
#define SPR_SPEED_UP            BIT(11) // For following tracks at different speeds
#define SPR_SLOW_DOWN           BIT(12) // For following tracks at different speeds
#define SPR_DONT_UPDATE_ANG     BIT(13) // For tracks - don't update the angle for a while

#define SPR_SO_ATTACHED            BIT(14) // sprite is part of a sector object
#define SPR_SUICIDE             BIT(15) // sprite is set to kill itself

#define SPR_RUN_AWAY            BIT(16) // sprite is in "Run Away" track mode.
#define SPR_FIND_PLAYER         BIT(17) // sprite is in "Find Player" track mode.

#define SPR_SWIMMING            BIT(18) // Actor is swimming
#define SPR_WAIT_FOR_PLAYER     BIT(19) // Track Mode - Actor is waiting for player to come close
#define SPR_WAIT_FOR_TRIGGER    BIT(20) // Track Mode - Actor is waiting for player to trigger
#define SPR_SLIDING             BIT(21) // Actor is sliding
#define SPR_ON_SO_SECTOR        BIT(22) // sprite is on a sector object sector

#define SPR_SHADE_DIR           BIT(23) // sprite is on a sector object sector
#define SPR_XFLIP_TOGGLE        BIT(24) // sprite rotation xflip bit
#define SPR_NO_SCAREDZ          BIT(25) // not afraid of falling

#define SPR_SET_POS_DONT_KILL   BIT(26) // Don't kill sprites in MissileSetPos
#define SPR_SKIP2               BIT(27) // 20 moves ps
#define SPR_SKIP4               BIT(28) // 10 moves ps

#define SPR_BOUNCE              BIT(29) // For shrapnel types that can bounce once
#define SPR_UNDERWATER          BIT(30) // For missiles etc

#define SPR_SHADOW              BIT(31) // Sprites that have shadows

// User->Flags2 flags
#define SPR2_BLUR_TAPER         (BIT(13)|BIT(14))   // taper type
#define SPR2_BLUR_TAPER_FAST    (BIT(13))   // taper fast
#define SPR2_BLUR_TAPER_SLOW    (BIT(14))   // taper slow
#define SPR2_SPRITE_FAKE_BLOCK  (BIT(15))   // fake blocking bit for damage
#define SPR2_NEVER_RESPAWN      (BIT(16))   // for item respawning
#define SPR2_ATTACH_WALL        (BIT(17))
#define SPR2_ATTACH_FLOOR       (BIT(18))
#define SPR2_ATTACH_CEILING     (BIT(19))
#define SPR2_CHILDREN           (BIT(20))   // sprite OWNS children
#define SPR2_SO_MISSILE         (BIT(21))   // this is a missile from a SO
#define SPR2_DYING              (BIT(22))   // Sprite is currently dying
#define SPR2_VIS_SHADING        (BIT(23))   // Sprite shading to go along with vis adjustments
#define SPR2_DONT_TARGET_OWNER  (BIT(24))


extern USERp User[MAXSPRITES];

typedef struct
{
    short Xdim, Ydim, ScreenSize;
} BORDER_INFO,*BORDER_INFOp;


typedef struct
{
    short high;
} RANGE,*RANGEp;


///////////////////////////////////////////////////////////////////////////////////////////
//
// Sector Stuff - Sector Objects and Tracks
//
///////////////////////////////////////////////////////////////////////////////////////////

// flags in EXTRA variable
#define SECTFX_SINK                  BIT(0)
#define SECTFX_OPERATIONAL           BIT(1)
#define SECTFX_WARP_SECTOR           BIT(2)
#define SECTFX_CURRENT               BIT(3)
#define SECTFX_Z_ADJUST              BIT(4) // adjust ceiling/floor
#define SECTFX_NO_RIDE               BIT(5) // moving sector - don't ride it
#define SECTFX_DYNAMIC_AREA          BIT(6)
#define SECTFX_DIVE_AREA             BIT(7) // Diving area
#define SECTFX_UNDERWATER            BIT(8) // Underwater area
#define SECTFX_UNDERWATER2           BIT(9) // Underwater area

#define SECTFX_LIQUID_MASK           (BIT(10)|BIT(11)) // only valid for sectors with depth
#define SECTFX_LIQUID_NONE           (0)
#define SECTFX_LIQUID_LAVA           BIT(10)
#define SECTFX_LIQUID_WATER          BIT(11)
#define SECTFX_SECTOR_OBJECT         BIT(12)  // for collision detection
#define SECTFX_VATOR                 BIT(13)  // denotes that this is a vertical moving sector
// vator type
#define SECTFX_TRIGGER               BIT(14)  // trigger type to replace tags.h trigger types

// flags in sector USER structure
#define SECTFU_SO_DONT_BOB          BIT(0)
#define SECTFU_SO_SINK_DEST         BIT(1)
#define SECTFU_SO_DONT_SINK         BIT(2)
#define SECTFU_DONT_COPY_PALETTE    BIT(3)
#define SECTFU_SO_SLOPE_FLOOR_TO_POINT BIT(4)
#define SECTFU_SO_SLOPE_CEILING_TO_POINT BIT(5)
#define SECTFU_DAMAGE_ABOVE_SECTOR  BIT(6)
#define SECTFU_VATOR_BOTH           BIT(7)  // vators set up for both ceiling and floor
#define SECTFU_CANT_SURFACE         BIT(8)  // for diving
#define SECTFU_SLIDE_SECTOR         BIT(9)  // for diving

#define MAKE_STAG_ENUM
enum stag_id
{
#include "stag.h"
};
typedef enum stag_id STAG_ID;
#undef MAKE_STAG_ENUM


#define WALLFX_LOOP_DONT_SPIN            BIT(0)
#define WALLFX_LOOP_REVERSE_SPIN         BIT(1)
#define WALLFX_LOOP_SPIN_2X              BIT(2)
#define WALLFX_LOOP_SPIN_4X              BIT(3)
#define WALLFX_LOOP_OUTER                BIT(4) // for sector object
#define WALLFX_DONT_MOVE                 BIT(5) // for sector object
#define WALLFX_SECTOR_OBJECT             BIT(6) // for collision detection
#define WALLFX_DONT_STICK                BIT(7) // for bullet holes and stars
#define WALLFX_DONT_SCALE                BIT(8) // for sector object
#define WALLFX_LOOP_OUTER_SECONDARY      BIT(9) // for sector object

enum ShrapType
{
    SHRAP_NONE              = 0,
    SHRAP_GLASS             = 1,  //
    SHRAP_TREE_BARK         = 2,  // (NEED) outside tree bark
    SHRAP_SO_SMOKE          = 3,  // only used for damaged SO's
    SHRAP_PAPER             = 4,  //
    SHRAP_BLOOD             = 5,  // std blood from gibs
    SHRAP_EXPLOSION         = 6,  // small explosion
    SHRAP_LARGE_EXPLOSION   = 7,  // large explosion
    SHRAP_METAL             = 8,  //
    SHRAP_STONE             = 9,  // what we have might be ok
    SHRAP_PLANT             = 10, // (NEED)
    SHRAP_GIBS              = 11, // std blood and guts
    SHRAP_WOOD              = 12, //
    SHRAP_GENERIC           = 13, // what we have might be ok - sort of gray brown rock look
    SHRAP_TREE_PULP         = 14, // (NEED) inside tree wood
    SHRAP_COIN              = 15,
    SHRAP_METALMIX          = 16,
    SHRAP_WOODMIX           = 17,
    SHRAP_MARBELS           = 18,
    SHRAP_PAPERMIX          = 19,
    SHRAP_USER_DEFINED      = 99
};

typedef struct
{
    int dist, flags;
    short depth_fract, depth; // do NOT change this, doubles as a long FIXED point number
    short stag,    // ST? tag number - for certain things it helps to know it
          ang,
          height,
          speed,
          damage,
          number;  // usually used for matching number
    uint8_t    flags2;
} SECT_USER, *SECT_USERp;

extern SECT_USERp SectUser[MAXSECTORS];
SECT_USERp SpawnSectUser(short sectnum);


typedef struct
{
    unsigned int size, checksum;
} MEM_HDR,*MEM_HDRp;

SWBOOL ValidPtr(void *ptr);
void *AllocMem(int size);
void *CallocMem(int size, int num);
void *ReAllocMem(void *ptr, int size);
void FreeMem(void *ptr);

typedef struct
{
    short sprite_num;
    short dang;
    int dist;
    int weight;
} TARGET_SORT, *TARGET_SORTp;

#define MAX_TARGET_SORT 16
extern TARGET_SORT TargetSort[MAX_TARGET_SORT];
extern unsigned TargetSortCount;

enum DoorType
{
    OPERATE_TYPE,
    DOOR_HORIZ_TYPE,
    DOOR_SLIDE_TYPE,
    DOOR_SWING_TYPE,
    DOOR_ROTATE_TYPE
};

typedef enum DoorType DOOR_TYPE;

typedef struct
{
    DOOR_TYPE Type;
    short Sector;
    short Speed;
    short TimeOut;
} DOOR_AUTO_CLOSE, *DOOR_AUTO_CLOSEp;

#define MAX_DOOR_AUTO_CLOSE 16

typedef struct
{
    int origx[17], origy[17];
    short sector, angopen, angclosed, angopendir, sang, anginc, wall[17];
} SWING;

typedef struct
{
    int floor_origz, ceiling_origz, range;
    short sector, sintable_ndx, speed_shift;
    char flags;
} SINE_WAVE_FLOOR, *SINE_WAVE_FLOORp;

#define MAX_SINE_WAVE 6
extern SINE_WAVE_FLOOR SineWaveFloor[MAX_SINE_WAVE][21];

typedef struct
{
    int orig_xy, range;
    short wall, sintable_ndx, speed_shift, type;
} SINE_WALL, *SINE_WALLp;

#define MAX_SINE_WALL 10
#define MAX_SINE_WALL_POINTS 64
extern SINE_WALL SineWall[MAX_SINE_WALL][MAX_SINE_WALL_POINTS];

typedef struct
{
    short Sector, TimeOut;
} SPRING_BOARD;

extern SPRING_BOARD SpringBoard[20];
extern SWING Rotate[17];

typedef struct
{
    short sector, speed;
    int xmid, ymid;
} SPIN;

extern SPIN Spin[17];
extern DOOR_AUTO_CLOSE DoorAutoClose[MAX_DOOR_AUTO_CLOSE];
extern int x_min_bound, y_min_bound, x_max_bound, y_max_bound;

#define MAXANIM 256
typedef void ANIM_CALLBACK (ANIMp, void *);
typedef ANIM_CALLBACK *ANIM_CALLBACKp;
typedef void *ANIM_DATAp;

struct ANIMstruct
{
    int *ptr, goal;
    int vel;
    short vel_adj;
    ANIM_CALLBACKp callback;
    ANIM_DATAp callbackdata;
};

extern ANIM Anim[MAXANIM];
extern short AnimCnt;


typedef struct
{
    int x,y,z;
    short ang, tag_low, tag_high, filler;
} TRACK_POINT, *TRACK_POINTp;

typedef struct
{
    TRACK_POINTp TrackPoint;
    int ttflags;
    short flags;
    short NumPoints;
} TRACK, *TRACKp;

// Most track type flags are in tags.h

// Regular track flags
#define TF_TRACK_OCCUPIED BIT(0)

typedef struct
{
    uint8_t FromRange,ToRange,FromColor,ToColor;
} COLOR_MAP, *COLOR_MAPp;

#define MAX_TRACKS 100

extern TRACK Track[MAX_TRACKS];

struct SECTOR_OBJECTstruct
{
#define MAX_SO_SECTOR 40
#define MAX_SO_POINTS (MAX_SO_SECTOR*15)
#define MAX_SO_SPRITE 60
#define MAX_CLIPBOX 32

    SECTORp sectp[MAX_SO_SECTOR];
    soANIMATORp PreMoveAnimator;
    soANIMATORp PostMoveAnimator;
    soANIMATORp Animator;
    SPRITEp controller;

    SPRITEp sp_child;  // child sprite that holds info for the sector object

    int    xmid,ymid,zmid, // midpoints of the sector object
           vel,            // velocity
           vel_tgt,        // target velocity
           player_xoff,    // player x offset from the xmid
           player_yoff,    // player y offset from the ymid
           zorig_floor[MAX_SO_SECTOR],      // original z values for all sectors
           zorig_ceiling[MAX_SO_SECTOR],      // original z values for all sectors
           zdelta,         // z delta from original
           z_tgt,          // target z delta
           z_rate,         // rate at which z aproaches target
           update,         // Distance from player at which you continue updating
    // only works for single player.
           bob_diff,       // bobbing difference for the frame
           target_dist,    // distance to next point
           floor_loz,      // floor low z
           floor_hiz,      // floor hi z
           morph_z,        // morphing point z
           morph_z_min,    // morphing point z min
           morph_z_max,
           bob_amt,        // bob amount max in z coord
    // variables set by mappers for drivables
           drive_angspeed,
           drive_angslide,
           drive_speed,
           drive_slide,
           crush_z,
           flags;

    short   sector[MAX_SO_SECTOR],     // hold the sector numbers of the sector object
            sp_num[MAX_SO_SPRITE],     // hold the sprite numbers of the object
            xorig[MAX_SO_POINTS],   // save the original x & y location of each wall so it can be
            yorig[MAX_SO_POINTS],   // refreshed
            sectnum,        // current secnum of midpoint
            mid_sector,     // middle sector
            max_damage,     // max damage
            ram_damage,     // damage taken by ramming
            wait_tics,      //
            num_sectors,    // number of sectors
            num_walls,      // number of sectors
            track,          // the track # 0 to 20
            point,          // the point on the track that the sector object is headed toward
            vel_rate,       // rate at which velocity aproaches target
            dir,            // direction traveling on the track
            ang,            // angle facing
            ang_moving,     // angle the SO is facing
            clipdist,       // cliping distance for operational sector objects
            clipbox_dist[MAX_CLIPBOX], // mult-clip box variables
            clipbox_xoff[MAX_CLIPBOX], // mult-clip box variables
            clipbox_yoff[MAX_CLIPBOX], // mult-clip box variables
            clipbox_ang[MAX_CLIPBOX], // mult-clip box variables
            clipbox_vdist[MAX_CLIPBOX], // mult-clip box variables
            clipbox_num,
            ang_tgt,        // target angle
            ang_orig,       // original angle
            last_ang,       // last angle before started spinning
            old_ang,        // holding variable for the old angle
            spin_speed,     // spin_speed
            spin_ang,       // spin angle
            turn_speed,     // shift value determines how fast SO turns to match new angle
            bob_sine_ndx,   // index into sine table
            bob_speed,      // shift value for speed
            op_main_sector, // main sector operational SO moves in - for speed purposes
            save_vel,       // save velocity
            save_spin_speed, // save spin speed
            match_event,    // match number
            match_event_sprite, // spritenum of the match event sprite
    // SO Scaling Vector Info
            scale_type,         // type of scaling - enum controled
            scale_active_type,  // activated by a switch or trigger

    // values for whole SO
            scale_dist,         // distance from center
            scale_speed,        // speed of scaling
            scale_dist_min,     // absolute min
            scale_dist_max,     // absolute max
            scale_rand_freq,    // freqency of direction change - based on rand(1024)

    // values for single point scaling
            scale_point_dist[MAX_SO_POINTS],         // distance from center
            scale_point_speed[MAX_SO_POINTS],        // speed of scaling
            scale_point_base_speed,                       // base speed of scaling
            scale_point_dist_min,     // absolute min
            scale_point_dist_max,     // absolute max
            scale_point_rand_freq,    // freqency of direction change - based on rand(1024)

            scale_x_mult,           // x multiplyer for scaling
            scale_y_mult,           // y multiplyer for scaling

    // Used for center point movement
            morph_wall_point,       // actual wall point to drag
            morph_ang,              // angle moving from CENTER
            morph_speed,            // speed of movement
            morph_dist_max,         // radius boundry
            morph_rand_freq,        // freq of dir change
            morph_dist,             // dist from CENTER
            morph_z_speed,          // z speed for morph point
            morph_xoff,             // save xoff from center
            morph_yoff,             // save yoff from center

    //scale_rand_reverse,            // random at random interval
    // limit rotation angle
            limit_ang_center, // for limiting the angle of turning - turrets etc
            limit_ang_delta; //
};

#define MAX_SECTOR_OBJECTS 20

#define SOBJ_SPEED_UP           BIT(0)
#define SOBJ_SLOW_DOWN          BIT(1)
#define SOBJ_ZUP                BIT(2)
#define SOBJ_ZDOWN              BIT(3)
#define SOBJ_ZDIFF_MODE         BIT(4)
#define SOBJ_MOVE_VERTICAL      BIT(5) // for sprite objects - move straight up/down
#define SOBJ_ABSOLUTE_ANGLE     BIT(7)
#define SOBJ_SPRITE_OBJ         BIT(8)
#define SOBJ_DONT_ROTATE        BIT(9)
#define SOBJ_WAIT_FOR_EVENT     BIT(10)
#define SOBJ_HAS_WEAPON         BIT(11)
#define SOBJ_SYNC1              BIT(12) // for syncing up several SO's perfectly
#define SOBJ_SYNC2              BIT(13) // for syncing up several SO's perfectly
#define SOBJ_DYNAMIC            BIT(14) // denotes scaling or morphing object
#define SOBJ_ZMID_FLOOR         BIT(15) // can't remember which sector objects need this
// think its the bobbing and sinking ones
#define SOBJ_SLIDE              BIT(16)

#define SOBJ_OPERATIONAL        BIT(17)
#define SOBJ_KILLABLE           BIT(18)
#define SOBJ_DIE_HARD           BIT(19)
#define SOBJ_UPDATE_ONCE        BIT(20)
#define SOBJ_UPDATE             BIT(21)
#define SOBJ_NO_QUAKE           BIT(22)
#define SOBJ_REMOTE_ONLY        BIT(23)
#define SOBJ_RECT_CLIP          BIT(24)
#define SOBJ_BROKEN               BIT(25)

// track set to these to tell them apart
#define SO_OPERATE_TRACK_START 90
#define SO_TURRET_MGUN 96 // machine gun
#define SO_TURRET 97
#define SO_TANK 98
#define SO_SPEED_BOAT 99

extern SECTOR_OBJECT SectorObject[MAX_SECTOR_OBJECTS];

///////////////////////////////////////////////////////////////////////////////////////////
//
// Prototypes
//
///////////////////////////////////////////////////////////////////////////////////////////

ANIMATOR NullAnimator;

void SetBorder(PLAYERp pp, int);
void SetFragBar(PLAYERp pp);
int Distance(int x1, int y1, int x2, int y2);
short GetDeltaAngle(short, short);

int SetActorRotation(short SpriteNum,int,int);
int NewStateGroup(short SpriteNum, STATEp SpriteGroup[]);
void SectorMidPoint(short sectnum, int *xmid, int *ymid, int *zmid);
USERp SpawnUser(short SpriteNum, short id, STATEp state);

short ActorFindTrack(short SpriteNum, int8_t player_dir, int track_type, short *track_point_num, short *track_dir);

SECT_USERp GetSectUser(short sectnum);

short SoundDist(int x, int y, int z, int basedist);
short SoundAngle(int x, int  y);
//void PlaySound(int num, short angle, short vol);
int PlaySound(int num, int *x, int *y, int *z, Voc3D_Flags flags);
void PlayerSound(int num, int *x, int *y, int *z, Voc3D_Flags flags, PLAYERp pp);

ANIMATOR DoActorBeginJump,DoActorJump,DoActorBeginFall,DoActorFall,DoActorDeathMove;

int SpawnShrap(short,short);

void PlayerUpdateHealth(PLAYERp pp, short value);
void PlayerUpdateAmmo(PLAYERp pp, short WeaponNum, short value);
void PlayerUpdateWeapon(PLAYERp pp, short WeaponNum);
void PlayerUpdateKills(PLAYERp pp, short value);
void PlayerUpdatePanelInfo(PLAYERp pp);
void RefreshInfoLine(PLAYERp pp);

void DoAnim(int numtics);
void AnimDelete(int *animptr);
short AnimGetGoal(int *animptr);
short AnimSet(int *animptr, int thegoal, int thevel);
//short AnimSetCallback(int *animptr, int thegoal, int thevel, ANIM_CALLBACKp call, ANIM_DATAp data);
short AnimSetCallback(short anim_ndx, ANIM_CALLBACKp call, ANIM_DATAp data);
short AnimSetVelAdj(short anim_ndx, short vel_adj);

void EnemyDefaults(short SpriteNum, ACTOR_ACTION_SETp action, PERSONALITYp person);

void getzrangepoint(int x, int y, int z, short sectnum, int32_t* ceilz, int32_t* ceilhit, int32_t* florz, int32_t* florhit);
int move_sprite(short spritenum, int xchange, int ychange, int zchange, int ceildist, int flordist, uint32_t cliptype, int numtics);
int move_missile(short spritenum, int xchange, int ychange, int zchange, int ceildist, int flordist, uint32_t cliptype, int numtics);
int DoPickTarget(SPRITEp sp, uint32_t max_delta_ang, SWBOOL skip_targets);

void change_sprite_stat(short, short);
void SetOwner(short, short);
void SetAttach(short, short);
void analyzesprites(int,int,int,SWBOOL);
void ChangeState(short SpriteNum, STATEp statep);

void UpdateSectorFAF_Connect(short SpriteNum, int newz);
#if 0
SWBOOL FAF_ConnectCeiling(short sectnum);
SWBOOL FAF_ConnectFloor(short sectnum);
#else
#define FAF_PLACE_MIRROR_PIC 341
#define FAF_MIRROR_PIC 2356
#define FAF_ConnectCeiling(sectnum) (sector[(sectnum)].ceilingpicnum == FAF_MIRROR_PIC)
#define FAF_ConnectFloor(sectnum) (sector[(sectnum)].floorpicnum == FAF_MIRROR_PIC)
#define FAF_ConnectArea(sectnum) (FAF_ConnectCeiling(sectnum) || FAF_ConnectFloor(sectnum))
#endif
void updatesectorz(int, int, int, short *);
void FAF_ConnectPlayerCeiling(PLAYERp pp);
void FAF_ConnectPlayerFloor(PLAYERp pp);
SWBOOL PlayerCeilingHit(PLAYERp pp, int zlimit);
SWBOOL PlayerFloorHit(PLAYERp pp, int zlimit);

void FAFhitscan(int32_t x, int32_t y, int32_t z, int16_t sectnum,
                int32_t xvect, int32_t yvect, int32_t zvect,
                hitdata_t* hitinfo, int32_t clipmask);

SWBOOL FAFcansee(int32_t xs, int32_t ys, int32_t zs, int16_t sects, int32_t xe, int32_t ye, int32_t ze, int16_t secte);

void FAFgetzrange(int32_t x, int32_t y, int32_t z, int16_t sectnum,
                  int32_t* hiz, int32_t* ceilhit,
                  int32_t* loz, int32_t* florhit,
                  int32_t clipdist, int32_t clipmask);

void FAFgetzrangepoint(int32_t x, int32_t y, int32_t z, int16_t sectnum,
                       int32_t* hiz, int32_t* ceilhit,
                       int32_t* loz, int32_t* florhit);

void COVERupdatesector(int32_t x, int32_t y, int16_t* newsector);

void updatesectorz(int,int,int,int16_t*);


void short_setinterpolation(short *posptr);
void short_stopinterpolation(short *posptr);
void short_updateinterpolations(void);
void short_dointerpolations(int smoothratio);
void short_restoreinterpolations(void);

enum SoundType
{
    SOUND_OBJECT_TYPE,
    SOUND_EVERYTHING_TYPE
};

void DoSoundSpotMatch(short match, short sound_num, short sound_type);

#define ACTOR_GRAVITY 8

///////////////////////////////////////////////////////////////////////////////////////////
//
//  Externs
//
///////////////////////////////////////////////////////////////////////////////////////////

extern SWBOOL ExitLevel;
extern SWBOOL Warping;
extern uint8_t CommPlayers;
extern SWBOOL CommEnabled;
extern char CommPlayerName[];
extern short Level;
extern short Episode;

extern int LastFrameTics;
extern char ds[];
extern short Skill;
extern int GodMode;

extern int x_min_bound, y_min_bound, x_max_bound, y_max_bound;

//extern unsigned char synctics, lastsynctics;
extern BORDER_INFO BorderInfo;
extern short snum;

extern int lockspeed,totalsynctics;

#define synctics 3
#define ACTORMOVETICS (synctics<<1)
#define TICSPERMOVEMENT synctics
#define FAKETIMERHANDLER()  if (totalclock >= ototalclock + synctics) faketimerhandler()

// subtract value from clipdist on getzrange calls
#define GETZRANGE_CLIP_ADJ 8
//#define GETZRANGE_CLIP_ADJ 0

// MULTIPLAYER
// VARIABLES:  (You should extern these in your game.c)
/*
extern short numplayers, myconnectindex;
extern short connecthead, connectpoint2[MAXPLAYERS];
*/
extern int *lastpacket2clock;
extern char username[MAXPLAYERS][50];

// save player info when moving to a new level
extern USER puser[MAX_SW_PLAYERS_REG];

///////////////////////////
//
// TEXT PRINTING
//
///////////////////////////

#define TEXT_TEST_LINE (200/2)
#define TEXT_XCENTER(width) ((320 - width)/2)
#define TEXT_YCENTER(h) ((200 - height)/2)
#define TEXT_TEST_COL(width) TEXT_XCENTER(width)
#define TEXT_TEST_TIME 2

void PutStringTimer(PLAYERp pp, short x, short y, const char *string, short seconds);

///////////////////////////
//
// OLDER network additions
//
///////////////////////////

/*
int initmultiplayers(int, int, int);
void uninitmultiplayers(void);

void sendlogon(void);
void sendlogoff(void);
*/


///////////////////////////
//
// RECENT network additions
//
///////////////////////////

extern int ototalclock, save_totalclock, gotlastpacketclock,smoothratio;
extern SWBOOL ready2send;

// local copy of variables updated by faketimerhandler
extern int locselectedgun;

//FIFO queue to hold values while faketimerhandler is called from within the drawing routing
extern int movefifoplc, movefifoend[];


extern SWBOOL MoveSkip4, MoveSkip2, MoveSkip8;

#define MASTER_SWITCHING 1

extern char option[];
extern char keys[];

extern short screenpeek;

extern int dimensionmode, zoom;
extern int vel,svel,angvel;

#define STAT_DAMAGE_LIST_SIZE 20
extern int16_t StatDamageList[STAT_DAMAGE_LIST_SIZE];

///////////////////////////////////////////////////////////////
//
// Stuff for player palette flashes when hurt or getting items
//
///////////////////////////////////////////////////////////////

#define COLOR_PAIN  128  // Light red range
extern void SetFadeAmt(PLAYERp pp, short damage, unsigned char startcolor);
extern void DoPaletteFlash(PLAYERp pp);
extern unsigned char palette_data[256][3];
extern SWBOOL NightVision;
#endif

int _PlayerSound(char *file, int line, int num, int *x, int *y, int *z, Voc3D_Flags flags, PLAYERp pp);
#define PlayerSound(num, x, y, z, flags, pp) _PlayerSound(__FILE__, __LINE__, (num), (x), (y), (z), (flags), (pp))

#define MAXSO (MAXLONG)

///////////////////////////////////////////////////////////////
//
// Stuff added by JonoF. These should get put into their own
// headers and included by that which needs them.
//
///////////////////////////////////////////////////////////////

int PickJumpMaxSpeed(short SpriteNum, short max_speed); // ripper.c
int DoRipperRipHeart(short SpriteNum);  // ripper.c
int DoRipper2RipHeart(short SpriteNum); // ripper2.c
int BunnyHatch2(short Weapon);  // bunny.c
int DoSkullBeginDeath(int16_t SpriteNum); // skull.c

void AnimateCacheCursor(void);  // game.c
void MapSetAll2D(uint8_t fill);    // game.c
void TerminateGame(void);   // game.c
void TerminateLevel(void);  // game.c
void ResetKeys(void);   // game.c
SWBOOL KeyPressed(void);  // game.c
void drawoverheadmap(int cposx,int cposy,int czoom,short cang); // game.c
void COVERsetbrightness(int bright, unsigned char *pal);    // game.c
void DrawMenuLevelScreen(void); // game.c
void DebugWriteString(char *string);    // game.c
void ManualPlayerInsert(PLAYERp pp);    // game.c

void SetRedrawScreen(PLAYERp pp);   // border.c
void SetupAspectRatio(void);    // border.c
void ClearStartMost(void);  // border.c
void SetCrosshair(void);    // border.c

void initsynccrc(void);     // sync.c
void demosync_record(void); // sync.c
void demosync_test(int cnt);    // sync.c
void getsyncstat(void); // sync.c
void SyncStatMessage(void); // sync.c

void drawscreen(PLAYERp pp);    // draw.c
void post_analyzesprites(void); // draw.c
int COVERsetgamemode(int mode, int xdim, int ydim, int bpp);    // draw.c
void ScreenCaptureKeys(void);   // draw.c

int minigametext(int x,int y,char *t,char s,short dabits);  // jplayer.c
void computergetinput(int snum,SW_PACKET *syn); // jplayer.c

void DrawOverlapRoom(int tx,int ty,int tz,short tang,int thoriz,short tsectnum);    // rooms.c
void SetupMirrorTiles(void);    // rooms.c
SWBOOL FAF_Sector(short sectnum); // rooms.c
int GetZadjustment(short sectnum,short hitag);  // rooms.c

void TermSetup(void);   // swconfig.c

void InitSetup(void);   // setup.c

void LoadKVXFromScript(char *filename); // scrip2.c
void LoadPLockFromScript(char *filename);   // scrip2.c
void LoadCustomInfoFromScript(char *filename);  // scrip2.c

void EveryCheatToggle(PLAYERp pp,char *cheat_string);   // cheats.c

int PlayerInitChemBomb(PLAYERp pp); // jweapon.c
int PlayerInitFlashBomb(PLAYERp pp);    // jweapon.c
int PlayerInitCaltrops(PLAYERp pp); // jweapon.c
int InitPhosphorus(int16_t SpriteNum);    // jweapon.c
void SpawnFloorSplash(short SpriteNum); // jweapon.c

int SaveGame(short save_num);   // save.c
int LoadGame(short save_num);   // save.c
int LoadGameFullHeader(short save_num, char *descr, short *level, short *skill);    // save,c
void LoadGameDescr(short save_num, char *descr);    // save.c

void SetRotatorActive(short SpriteNum); // rotator.c

SWBOOL VatorSwitch(short match, short setting); // vator.c
void MoveSpritesWithSector(short sectnum,int z_amt,SWBOOL type);  // vator.c
void SetVatorActive(short SpriteNum);   // vator.c

short DoSpikeMatch(PLAYERp pp,short match); // spike.c
void SpikeAlign(short SpriteNum);   // spike.c

short DoSectorObjectSetScale(short match);  // morph.c
short DoSOevent(short match,short state);   // morph.c
void SOBJ_AlignCeilingToPoint(SECTOR_OBJECTp sop,int x,int y,int z);    // morph.c
void SOBJ_AlignFloorToPoint(SECTOR_OBJECTp sop,int x,int y,int z);  // morph.c
void ScaleSectorObject(SECTOR_OBJECTp sop); // morph.c
void MorphTornado(SECTOR_OBJECTp sop);  // morph.c
void MorphFloor(SECTOR_OBJECTp sop);    // morph.c
void ScaleRandomPoint(SECTOR_OBJECTp sop,short k,short ang,int x,int y,int *dx,int *dy);    // morph.c

void CopySectorMatch(short match);  // copysect.c

int DoWallMoveMatch(short match);   // wallmove.c
int DoWallMove(SPRITEp sp); // wallmove.c
SWBOOL CanSeeWallMove(SPRITEp wp,short match);    // wallmove.c

short DoSpikeOperate(PLAYERp pp,short sectnum); // spike.c
void SetSpikeActive(short SpriteNum);   // spike.c

#define NTAG_SEARCH_LO 1
#define NTAG_SEARCH_HI 2
#define NTAG_SEARCH_LO_HI 3

int COVERinsertsprite(short sectnum, short statnum);   //returns (short)spritenum;

void AudioUpdate(void); // stupid
