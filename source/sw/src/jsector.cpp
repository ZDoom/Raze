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

// JSECTOR.C
// This is all Jim's programming having to do with sectors.

#include "build.h"

#include "keys.h"
#include "names2.h"
#include "jnames.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "sector.h"
#include "player.h"
#include "sprite.h"
#include "reserve.h"
#include "jsector.h"
#include "jtags.h"
#include "lists.h"
#include "pal.h"
#include "parent.h"


// V A R I A B L E   D E C L A R A T I O N S //////////////////////////////////////////////////////

MIRRORTYPE mirror[MAXMIRRORS];

short mirrorcnt; //, floormirrorcnt;
//short floormirrorsector[MAXMIRRORS];
SWBOOL mirrorinview;

static char tempbuf[/*max(576, */ MAXXDIM /*)*/];

SWBOOL MirrorMoveSkip16 = 0;

// Voxel stuff
//SWBOOL bVoxelsOn = TRUE;                  // Turn voxels on by default
SWBOOL bSpinBobVoxels = FALSE;            // Do twizzly stuff to voxels, but
// not by default
SWBOOL bAutoSize = TRUE;                  // Autosizing on/off

//extern int chainnumpages;
extern AMB_INFO ambarray[];
extern short NormalVisibility;

extern ParentalStruct aVoxelArray[MAXTILES];

// F U N C T I O N S //////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////
//  SpawnWallSound
/////////////////////////////////////////////////////
void
SpawnWallSound(short sndnum, short i)
{
    short SpriteNum;
    vec3_t mid;
    SPRITEp sp;
    int handle;

    SpriteNum = COVERinsertsprite(0, STAT_DEFAULT);
    if (SpriteNum < 0)
        return;

    sp = &sprite[SpriteNum];
    sp->cstat = 0;
    sp->extra = 0;
    // Get wall midpoint for offset in mirror view
    mid.x = (wall[i].x + wall[wall[i].point2].x) / 2;
    mid.y = (wall[i].y + wall[wall[i].point2].y) / 2;
    mid.z = (sector[wall[i].nextsector].ceilingz + sector[wall[i].nextsector].floorz) / 2;
    setspritez(SpriteNum, &mid);
    sp = &sprite[SpriteNum];

    handle = PlaySound(sndnum, &sp->x, &sp->y, &sp->z, v3df_dontpan | v3df_doppler);
    if (handle != -1)
        Set3DSoundOwner(SpriteNum);
}

short
CheckTileSound(short picnum)
{
    short sndnum = -1;

    switch (picnum)
    {
    case 163:                           // Sizzly Lava
    case 167:
        sndnum = DIGI_VOLCANOSTEAM1;
        break;
    case 175:                           // Flowing Lava
        sndnum = DIGI_ERUPTION;
        break;
    case 179:                           // Bubbly lava
        sndnum = DIGI_LAVAFLOW1;
        break;
    case 300:                           // Water fall tile
        sndnum = DIGI_WATERFALL1;
        break;
    case 334:                           // Teleporter Pad
        sndnum = DIGI_ENGROOM1;
        break;
    case 2690:                          // Jet engine fan
        sndnum = DIGI_JET;
        break;
    case 2672:                          // X-Ray Machine engine
        sndnum = DIGI_ENGROOM5;
        break;
    case 768:                           // Electricity
//          sndnum = DIGI_;
        break;
    case 2714:                          // Pachinko Machine
//          sndnum = DIGI_;
        break;
    case 2782:                          // Telepad
        sndnum = DIGI_ENGROOM4;
        break;
    case 3382:                          // Gears
        sndnum = DIGI_ENGROOM5;
        break;
    case 2801:                          // Computers
    case 2804:
    case 2807:
    case 3352:
    case 3385:
    case 3389:
    case 3393:
    case 3397:
    case 3401:
    case 3405:
//          sndnum = DIGI_;
        break;
    case 3478:                          // Radar screen
//          sndnum = DIGI_;
        break;
    default:
        sndnum = -1;
        break;
    }
    return sndnum;
}

/////////////////////////////////////////////////////
//  Initialize any of my special use sprites
/////////////////////////////////////////////////////
void
JS_SpriteSetup(void)
{
    SPRITEp sp;
    short SpriteNum = 0, NextSprite, ndx;
    USERp u;
    short i, num;
    int handle;


    TRAVERSE_SPRITE_STAT(headspritestat[0], SpriteNum, NextSprite)
    {
        short tag;
        short bit;

        sp = &sprite[SpriteNum];
        tag = sp->hitag;

        // Non static camera. Camera sprite will be drawn!
        if (tag == MIRROR_CAM && sprite[SpriteNum].picnum != ST1)
        {
            // Just change it to static, sprite has all the info I need
//          u = SpawnUser(SpriteNum, sp->picnum, NULL);
//          RESET(sp->cstat, CSTAT_SPRITE_BLOCK);
//          SET(sp->cstat, CSTAT_SPRITE_BLOCK_HITSCAN);
            change_sprite_stat(SpriteNum, STAT_SPAWN_SPOT);
        }

        switch (sprite[SpriteNum].picnum)
        {
        case ST1:
            if (tag == MIRROR_CAM)
            {
                // Just change it to static, sprite has all the info I need
                // ST1 cameras won't move with SOBJ's!
                change_sprite_stat(SpriteNum, STAT_ST1);
            }
            else if (tag == MIRROR_SPAWNSPOT)
            {
                // Just change it to static, sprite has all the info I need
                change_sprite_stat(SpriteNum, STAT_ST1);
            }
            else if (tag == AMBIENT_SOUND)
            {
                change_sprite_stat(SpriteNum, STAT_AMBIENT);
                // PlaySound(sp->lotag, &sp->x, &sp->y, &sp->z, v3df_ambient
                // | v3df_init | v3df_doppler);
            }
            else if (tag == TAG_ECHO_SOUND)
            {
                change_sprite_stat(SpriteNum, STAT_ECHO);
            }
            else if (tag == TAG_DRIPGEN)
            {
                ANIMATOR GenerateDrips;

                u = SpawnUser(SpriteNum, 0, NULL);

                ASSERT(u != NULL);
                u->RotNum = 0;
                u->WaitTics = sp->lotag * 120;

                u->ActorActionFunc = GenerateDrips;

                change_sprite_stat(SpriteNum, STAT_NO_STATE);
                SET(sp->cstat, CSTAT_SPRITE_INVISIBLE);
            }
            break;
        // Sprites in editart that should play ambient sounds
        // automatically
        case 380:
        case 396:
        case 430:
        case 443:
        case 512:
        case 521:
        case 541:
        case 2720:
        case 3143:
        case 3157:
            handle = PlaySound(DIGI_FIRE1, &sp->x, &sp->y, &sp->z, v3df_follow|v3df_dontpan|v3df_doppler);
            if (handle != -1)
                Set3DSoundOwner(SpriteNum);
            break;
        case 795:
        case 880:
            handle = PlaySound(DIGI_WATERFLOW1, &sp->x, &sp->y, &sp->z, v3df_follow|v3df_dontpan|v3df_doppler);
            if (handle != -1)
                Set3DSoundOwner(SpriteNum);
            break;
        case 460:  // Wind Chimes
            handle = PlaySound(79, &sp->x, &sp->y, &sp->z, v3df_ambient | v3df_init
                               | v3df_doppler | v3df_follow);
            if (handle != -1)
                Set3DSoundOwner(SpriteNum);
            break;

        }
    }
    // Check for certain walls to make sounds
    for (i = 0; i < numwalls; i++)
    {
        short picnum;
        short sndnum;


        picnum = wall[i].picnum;

        // Set the don't stick bit for liquid tiles
        switch (picnum)
        {
        case 175:
        case 179:
        case 300:
        case 320:
        case 330:
        case 352:
        case 780:
        case 890:
        case 2608:
        case 2616:
            //case 3834:
            SET(wall[i].extra, WALLFX_DONT_STICK);
            break;
        }

#if 0
        if ((sndnum = CheckTileSound(picnum)) != -1)
        {
            SpawnWallSound(sndnum, i);
        }
        picnum = wall[i].overpicnum;
        if ((sndnum = CheckTileSound(picnum)) != -1)
        {
            SpawnWallSound(sndnum, i);
        }
#endif
    }
}


/////////////////////////////////////////////////////
//  Initialize the mirrors
/////////////////////////////////////////////////////
void
JS_InitMirrors(void)
{
    short startwall, endwall, dasector;
    int i, j, k, s, dax, day, daz, dax2, day2;
    short SpriteNum = 0, NextSprite;
    SPRITEp sp;
    static short on_cam = 0;
    SWBOOL Found_Cam = FALSE;


    // Set all the mirror struct values to -1
    memset(mirror, 0xFF, sizeof(mirror));

    mirrorinview = FALSE;               // Initially set global mirror flag
    // to no mirrors seen

    // Scan wall tags for mirrors
    mirrorcnt = 0;
    tilesiz[MIRROR].x = 0;
    tilesiz[MIRROR].y = 0;

    for (i = 0; i < MAXMIRRORS; i++)
    {
        tilesiz[i + MIRRORLABEL].x = 0;
        tilesiz[i + MIRRORLABEL].y = 0;
        mirror[i].campic = -1;
        mirror[i].camsprite = -1;
        mirror[i].camera = -1;
        mirror[i].ismagic = FALSE;
    }

    for (i = 0; i < numwalls; i++)
    {
        s = wall[i].nextsector;
        if ((s >= 0) && (wall[i].overpicnum == MIRROR) && (wall[i].cstat & 32))
        {
            if ((sector[s].floorstat & 1) == 0)
            {
                wall[i].overpicnum = MIRRORLABEL + mirrorcnt;
                wall[i].picnum = MIRRORLABEL + mirrorcnt;
                sector[s].ceilingpicnum = MIRRORLABEL + mirrorcnt;
                sector[s].floorpicnum = MIRRORLABEL + mirrorcnt;
                sector[s].floorstat |= 1;
                mirror[mirrorcnt].mirrorwall = i;
                mirror[mirrorcnt].mirrorsector = s;
                mirror[mirrorcnt].numspawnspots = 0;
                mirror[mirrorcnt].ismagic = FALSE;
                if (wall[i].lotag == TAG_WALL_MAGIC_MIRROR)
                {
                    short ii, nextii;
                    SPRITEp sp;
                    USERp u;

                    mirror[mirrorcnt].ismagic = TRUE;
                    Found_Cam = FALSE;
                    TRAVERSE_SPRITE_STAT(headspritestat[STAT_ST1], ii, nextii)
                    {
                        sp = &sprite[ii];
                        // if correct type and matches
                        if (sp->hitag == MIRROR_CAM && sp->lotag == wall[i].hitag)
                        {
                            mirror[mirrorcnt].camera = ii;
                            // Set up camera varialbes
                            SP_TAG5(sp) = sp->ang;      // Set current angle to
                            // sprite angle
                            Found_Cam = TRUE;
                        }
                    }

                    ii = nextii = 0;
                    TRAVERSE_SPRITE_STAT(headspritestat[STAT_SPAWN_SPOT], ii, nextii)
                    {

                        sp = &sprite[ii];

                        // if correct type and matches
                        if (sp->hitag == MIRROR_CAM && sp->lotag == wall[i].hitag)
                        {
                            mirror[mirrorcnt].camera = ii;
                            // Set up camera varialbes
                            SP_TAG5(sp) = sp->ang;      // Set current angle to
                            // sprite angle
                            Found_Cam = TRUE;
                        }
                    }

                    if (!Found_Cam)
                    {
                        printf("Cound not find the camera view sprite for match %d\n",TrackerCast(wall[i].hitag));
                        printf("Map Coordinates: x = %d, y = %d\n",TrackerCast(wall[i].x),TrackerCast(wall[i].y));
                        exit(0);
                    }

                    Found_Cam = FALSE;
                    if (TEST_BOOL1(&sprite[mirror[mirrorcnt].camera]))
                    {
                        TRAVERSE_SPRITE_STAT(headspritestat[0], SpriteNum, NextSprite)
                        {
                            sp = &sprite[SpriteNum];
                            if (sp->picnum >= CAMSPRITE && sp->picnum < CAMSPRITE + 8 &&
                                sp->hitag == wall[i].hitag)
                            {
                                mirror[mirrorcnt].campic = sp->picnum;
                                mirror[mirrorcnt].camsprite = SpriteNum;

                                // JBF: commenting out this line results in the screen in $BULLET being visible
                                tilesiz[mirror[mirrorcnt].campic].x = tilesiz[mirror[mirrorcnt].campic].y = 0;

                                Found_Cam = TRUE;
                            }
                        }

                        if (!Found_Cam)
                        {
                            printf("Did not find drawtotile for camera number %d\n",mirrorcnt);
                            printf("wall[%d].hitag == %d\n",i,TrackerCast(wall[i].hitag));
                            printf("Map Coordinates: x = %d, y = %d\n", TrackerCast(wall[i].x), TrackerCast(wall[i].y));
                            exit(0);
                        }
                    }

                    // For magic mirrors, set allowable viewing time to 30
                    // secs
                    // Base rate is supposed to be 120, but time is double
                    // what I expect
                    mirror[mirrorcnt].maxtics = 60 * 30;

                }

                mirror[mirrorcnt].mstate = m_normal;

                // Set tics used to none
                mirror[mirrorcnt].tics = 0;

                if (mirror[mirrorcnt].ismagic)
                {
                    //DSPRINTF(ds, "mirror.mirrorwall %d", mirror[mirrorcnt].mirrorwall);
                    MONO_PRINT(ds);
                    //DSPRINTF(ds, "mirror.mirrorsector %d", mirror[mirrorcnt].mirrorsector);
                    MONO_PRINT(ds);
                    //DSPRINTF(ds, "mirror.camera %d", mirror[mirrorcnt].camera);
                    MONO_PRINT(ds);
                }

                mirrorcnt++;
                ASSERT(mirrorcnt < MAXMIRRORS);
            }
            else
                wall[i].overpicnum = sector[s].ceilingpicnum;
        }
    }

    // Invalidate textures in sector behind mirror
    for (i = 0; i < mirrorcnt; i++)
    {
        startwall = sector[mirror[i].mirrorsector].wallptr;
        endwall = startwall + sector[mirror[i].mirrorsector].wallnum;
        for (j = startwall; j < endwall; j++)
        {
            wall[j].picnum = MIRROR;
            wall[j].overpicnum = MIRROR;
        }
    }

}                                   // InitMirrors

/////////////////////////////////////////////////////
//  Draw a 3d screen to a specific tile
/////////////////////////////////////////////////////
#if 1
void drawroomstotile(int daposx, int daposy, int daposz,
                     short daang, int dahoriz, short dacursectnum, short tilenume)
{
    if (waloff[tilenume] == 0)
        tileLoad(tilenume);

    PRODUCTION_ASSERT(waloff[tilenume]);

    renderSetTarget(tilenume, tilesiz[tilenume].x, tilesiz[tilenume].y);

    drawrooms(daposx, daposy, daposz, daang, dahoriz, dacursectnum);
    analyzesprites(daposx, daposy, daposz, FALSE);
    renderDrawMasks();

    renderRestoreTarget();

    squarerotatetile(tilenume);

    tileInvalidate(tilenume, -1, -1);
}
#else
void
drawroomstotile(int daposx, int daposy, int daposz,
                short daang, int dahoriz, short dacursectnum, short tilenume)
{

    int i, j, k, bakchainnumpages, bakvidoption;
    intptr_t bakframeplace;
    vec2_t bakwindowxy1, bakwindowxy2;
    int xsiz, ysiz;
    char *ptr1, *ptr2;

    // DRAWROOMS TO TILE BACKUP&SET CODE
    xsiz = tilesiz[tilenume].x;
    ysiz = tilesiz[tilenume].y;
    // bakchainnumpages = chainnumpages;
    bakchainnumpages = numpages;
    // chainnumpages = 0;
    numpages = 0;
    bakvidoption = vidoption;
    vidoption = 1;
    if (waloff[tilenume] == 0)
        loadtile(tilenume);
    bakframeplace = frameplace;
    frameplace = waloff[tilenume];
    bakwindowxy1 = windowxy1;
    bakwindowxy2 = windowxy2;
    setview(0, 0, xsiz - 1, ysiz - 1);
    setvlinebpl(xsiz);
    j = 0;
    for (i = 0; i <= ysiz; i++)
    {
        ylookup[i] = j, j += xsiz;
    }

    // DRAWS TO TILE HERE
    drawrooms(daposx, daposy, daposz, daang, dahoriz, dacursectnum + MAXSECTORS);
    analyzesprites(daposx, daposy, daposz, FALSE);
    renderDrawMasks();

    setviewback();

    // ROTATE TILE (supports square tiles only for rotation part)
    if (xsiz == ysiz)
    {
        k = (xsiz << 1);
        for (i = xsiz - 1; i >= 0; i--)
        {
            ptr1 = (char *)(waloff[tilenume] + i * (xsiz + 1));
            ptr2 = ptr1;
            if ((i & 1) != 0)
            {
                ptr1--;
                ptr2 -= xsiz;
                swapchar(ptr1, ptr2);
            }
            for (j = (i >> 1) - 1; j >= 0; j--)
            {
                ptr1 -= 2;
                ptr2 -= k;
                swapchar2(ptr1, ptr2, xsiz);
                // swapchar(ptr1,ptr2);
                // swapchar(ptr1+1,ptr2+xsiz);
            }
        }
    }

    // DRAWROOMS TO TILE RESTORE CODE
    setview(bakwindowxy1.x, bakwindowxy1.y, bakwindowxy2.x, bakwindowxy2.y);
    // chainnumpages = bakchainnumpages;
    numpages = bakchainnumpages;
    vidoption = bakvidoption;
    frameplace = bakframeplace;
    j = 0;
    // if (chainnumpages >= 2)
    if (numpages >= 2)
    {
        for (i = 0; i <= ysiz; i++)
            ylookup[i] = j, j += (xdim >> 2);
    }
    else
    {
        for (i = 0; i <= ysiz; i++)
            ylookup[i] = j, j += xdim;
    }
    setvlinebpl(ylookup[1]);
}
#endif

void
JS_ProcessEchoSpot()
{
    short i,nexti;
    SPRITEp tp;
    int j,dist;
    PLAYERp pp = Player+screenpeek;
    int16_t reverb;
    SWBOOL reverb_set = FALSE;

    // Process echo sprites
    TRAVERSE_SPRITE_STAT(headspritestat[STAT_ECHO], i, nexti)
    {
        dist = 0x7fffffff;

        tp = &sprite[i];

        j = klabs(tp->x - pp->posx);
        j += klabs(tp->y - pp->posy);
        if (j < dist)
            dist = j;

        if (dist <= SP_TAG4(tp)) // tag4 = ang
        {
            reverb = SP_TAG2(tp);
            if (reverb > 200) reverb = 200;
            if (reverb < 100) reverb = 100;

            COVER_SetReverb(reverb);
            reverb_set = TRUE;
        }
    }
    if (!TEST(pp->Flags, PF_DIVING) && !reverb_set && pp->Reverb <= 0)
        COVER_SetReverb(0);
}

/////////////////////////////////////////////////////
//  Draw one mirror, the one closest to player
//  Cams and see to teleporters do NOT support room above room!
/////////////////////////////////////////////////////
#define MAXCAMDIST 8000

int camloopcnt = 0;                    // Timer to cycle through player
// views
short camplayerview = 1;                // Don't show yourself!

void
JS_DrawMirrors(PLAYERp pp, int tx, int ty, int tz, short tpang, int tphoriz)
{
    int j, dx, dy, top, bot, cnt;
    int x1, y1, x2, y2, ox1, oy1, ox2, oy2, dist, maxdist;
    int tposx, tposy, thoriz;
    int tcx, tcy, tcz;                 // Camera
    int tiltlock, *longptr;
    fix16_t tang;
    char ch, *ptr, *ptr2, *ptr3, *ptr4;
    char tvisibility, palok;

//    long tx, ty, tz, tpang;             // Interpolate so mirror doesn't
    // drift!
    SWBOOL bIsWallMirror = FALSE;

    MirrorMoveSkip16 = (MirrorMoveSkip16 + 1) & 15;

    camloopcnt += (int32_t) (totalclock - ototalclock);
    if (camloopcnt > (60 * 5))          // 5 seconds per player view
    {
        camloopcnt = 0;
        camplayerview++;
        if (camplayerview >= numplayers)
            camplayerview = 1;
    }

    // WARNING!  Assuming (MIRRORLABEL&31) = 0 and MAXMIRRORS = 64 <-- JBF: wrong
    longptr = (int *)&gotpic[MIRRORLABEL >> 3];
    if (longptr && (longptr[0] || longptr[1]))
    {
        for (cnt = MAXMIRRORS - 1; cnt >= 0; cnt--)
            //if (TEST_GOTPIC(cnt + MIRRORLABEL) || TEST_GOTPIC(cnt + CAMSPRITE))
            if (TEST_GOTPIC(cnt + MIRRORLABEL) || TEST_GOTPIC(mirror[cnt].campic))
            {
                bIsWallMirror = FALSE;
                if (TEST_GOTPIC(cnt + MIRRORLABEL))
                {
                    bIsWallMirror = TRUE;
                    RESET_GOTPIC(cnt + MIRRORLABEL);
                }
                //else if (TEST_GOTPIC(cnt + CAMSPRITE))
                else if (TEST_GOTPIC(mirror[cnt].campic))
                {
                    //RESET_GOTPIC(cnt + CAMSPRITE);
                    RESET_GOTPIC(mirror[cnt].campic);
                }

                mirrorinview = TRUE;

//                tx = pp->oposx + mulscale16(pp->posx - pp->oposx, smoothratio);
//                ty = pp->oposy + mulscale16(pp->posy - pp->oposy, smoothratio);
//                tz = pp->oposz + mulscale16(pp->posz - pp->oposz, smoothratio);
//                tpang = pp->oang + mulscale16(((pp->pang + 1024 - pp->oang) & 2047) - 1024, smoothratio);


                dist = 0x7fffffff;

                if (bIsWallMirror)
                {
                    j = klabs(wall[mirror[cnt].mirrorwall].x - tx);
                    j += klabs(wall[mirror[cnt].mirrorwall].y - ty);
                    if (j < dist)
                        dist = j;
                }
                else
                {
                    SPRITEp tp;

                    tp = &sprite[mirror[cnt].camsprite];

                    j = klabs(tp->x - tx);
                    j += klabs(tp->y - ty);
                    if (j < dist)
                        dist = j;
                }



//              //DSPRINTF(ds,"mirror.tics == %ul", mirror[i].tics);
//              MONO_PRINT(ds);


                if (mirror[cnt].ismagic)
                {
                    SPRITEp sp=NULL;
                    int camhoriz;
                    short wall_ang, w, nw, da, tda;
                    int dx, dy, dz, tdx, tdy, tdz, midx, midy;


                    ASSERT(mirror[cnt].camera != -1);

                    sp = &sprite[mirror[cnt].camera];

                    ASSERT(sp);

                    // tvisibility = g_visibility;
//                  g_visibility <<= 1;       // Make mirror darker

                    // Make TV cam style mirror seem to shimmer
//                  if (mirror[cnt].ismagic && STD_RANDOM_P2(256) > 128)
//                      g_visibility -= STD_RANDOM_P2(128);

                    // Calculate the angle of the mirror wall
                    w = mirror[cnt].mirrorwall;
                    nw = wall[w].point2;

                    // Get wall midpoint for offset in mirror view
                    midx = (wall[w].x + wall[wall[w].point2].x) / 2;
                    midy = (wall[w].y + wall[wall[w].point2].y) / 2;

                    // Finish finding offsets
                    tdx = klabs(midx - tx);
                    tdy = klabs(midy - ty);

                    if (midx >= tx)
                        dx = sp->x - tdx;
                    else
                        dx = sp->x + tdx;

                    if (midy >= ty)
                        dy = sp->y - tdy;
                    else
                        dy = sp->y + tdy;

                    tdz = klabs(tz - sp->z);
                    if (tz >= sp->z)
                        dz = sp->z + tdz;
                    else
                        dz = sp->z - tdz;


                    // Is it a TV cam or a teleporter that shows destination?
                    // TRUE = It's a TV cam
                    mirror[cnt].mstate = m_normal;
                    if (TEST_BOOL1(sp))
                        mirror[cnt].mstate = m_viewon;

                    // Show teleport destination
                    // NOTE: Adding MAXSECTORS lets you draw a room, even if
                    // you are outside of it!
                    if (mirror[cnt].mstate != m_viewon)
                    {
                        tilesiz[MIRROR].x = tilesiz[MIRROR].y = 0;
                        // Set TV camera sprite size to 0 to show mirror
                        // behind in this case!

                        if (mirror[cnt].campic != -1)
                            tilesiz[mirror[cnt].campic].x = tilesiz[mirror[cnt].campic].y = 0;
                        drawrooms(dx, dy, dz, tpang, tphoriz, sp->sectnum + MAXSECTORS);
                        analyzesprites(dx, dy, dz, FALSE);
                        renderDrawMasks();
                    }
                    else
                    {
                        SWBOOL DoCam = FALSE;

                        if (mirror[cnt].campic == -1)
                        {
                            TerminateGame();
                            printf("Missing campic for mirror %d\n",cnt);
                            printf("Map Coordinates: x = %d, y = %d\n",midx,midy);
                            exit(0);
                        }

                        // BOOL2 = Oscilate camera
                        if (TEST_BOOL2(sp) && MoveSkip2 == 0)
                        {
                            if (TEST_BOOL3(sp)) // If true add increment to
                            // angle else subtract
                            {
                                // Store current angle in TAG5
                                SP_TAG5(sp) = NORM_ANGLE((SP_TAG5(sp) + 4));

                                // TAG6 = Turn radius
                                if (klabs(GetDeltaAngle(SP_TAG5(sp), sp->ang)) >= SP_TAG6(sp))
                                {
                                    RESET_BOOL3(sp);    // Reverse turn
                                    // direction.
                                }
                            }
                            else
                            {
                                // Store current angle in TAG5
                                SP_TAG5(sp) = NORM_ANGLE((SP_TAG5(sp) - 4));

                                // TAG6 = Turn radius
                                if (klabs(GetDeltaAngle(SP_TAG5(sp), sp->ang)) >= SP_TAG6(sp))
                                {
                                    SET_BOOL3(sp);      // Reverse turn
                                    // direction.
                                }
                            }
                        }
                        else if (!TEST_BOOL2(sp))
                        {
                            SP_TAG5(sp) = sp->ang;      // Copy sprite angle to
                            // tag5
                        }

                        // See if there is a horizon value.  0 defaults to
                        // 100!
                        if (SP_TAG7(sp) != 0)
                        {
                            camhoriz = SP_TAG7(sp);
                            if (camhoriz > PLAYER_HORIZ_MAX)
                                camhoriz = PLAYER_HORIZ_MAX;
                            else if (camhoriz < PLAYER_HORIZ_MIN)
                                camhoriz = PLAYER_HORIZ_MIN;
                        }
                        else
                            camhoriz = 100;     // Default

                        // If player is dead still then update at MoveSkip4
                        // rate.
                        if (pp->posx == pp->oposx && pp->posy == pp->oposy && pp->posz == pp->oposz)
                            DoCam = TRUE;


                        // Set up the tile for drawing
                        tilesiz[mirror[cnt].campic].x = tilesiz[mirror[cnt].campic].y = 128;

                        if (MirrorMoveSkip16 == 0 || (DoCam && (MoveSkip4 == 0)))
                        {
                            if (dist < MAXCAMDIST)
                            {
                                PLAYERp cp = Player + camplayerview;

                                if (TEST_BOOL11(sp) && numplayers > 1)
                                {
                                    drawroomstotile(cp->posx, cp->posy, cp->posz, cp->pang, cp->horiz, cp->cursectnum, mirror[cnt].campic);
                                }
                                else
                                {
                                    drawroomstotile(sp->x, sp->y, sp->z, SP_TAG5(sp), camhoriz, sp->sectnum, mirror[cnt].campic);
                                }
                            }
                        }
                    }
                }
                else
                {
                    // It's just a mirror
                    // Prepare drawrooms for drawing mirror and calculate
                    // reflected
                    // position into tposx, tposy, and tang (tposz == cposz)
                    // Must call preparemirror before drawrooms and
                    // completemirror after drawrooms

                    renderPrepareMirror(tx, ty, tz, fix16_from_int(tpang), tphoriz,
                                  mirror[cnt].mirrorwall, /*mirror[cnt].mirrorsector,*/ &tposx, &tposy, &tang);

                    drawrooms(tposx, tposy, tz, fix16_to_int(tang), tphoriz, mirror[cnt].mirrorsector + MAXSECTORS);

                    analyzesprites(tposx, tposy, tz, TRUE);
                    renderDrawMasks();

                    renderCompleteMirror();   // Reverse screen x-wise in this
                    // function
                }


                // g_visibility = tvisibility;
                // g_visibility = NormalVisibility;

                // drawrooms(tx, ty, tz, tpang, tphoriz, pp->cursectnum);
                // Clean up anything that the camera view might have done
                SetFragBar(pp);
                tilesiz[MIRROR].x = tilesiz[MIRROR].y = 0;
                wall[mirror[cnt].mirrorwall].overpicnum = MIRRORLABEL + cnt;
            }
            else
                mirrorinview = FALSE;
    }
}

void
DoAutoSize(uspritetype * tspr)
{
    short i;

    if (!bAutoSize)
        return;

    switch (tspr->picnum)
    {
    case ICON_STAR:                     // 1793
        break;
    case ICON_UZI:                      // 1797
        tspr->xrepeat = 43;
        tspr->yrepeat = 40;
        break;
    case ICON_UZIFLOOR:         // 1807
        tspr->xrepeat = 43;
        tspr->yrepeat = 40;
        break;
    case ICON_LG_UZI_AMMO:              // 1799
        break;
    case ICON_HEART:                    // 1824
        break;
    case ICON_HEART_LG_AMMO:            // 1820
        break;
    case ICON_GUARD_HEAD:               // 1814
        break;
    case ICON_FIREBALL_LG_AMMO: // 3035
        break;
    case ICON_ROCKET:                   // 1843
        break;
    case ICON_SHOTGUN:                  // 1794
        tspr->xrepeat = 57;
        tspr->yrepeat = 58;
        break;
    case ICON_LG_ROCKET:                // 1796
        break;
    case ICON_LG_SHOTSHELL:             // 1823
        break;
    case ICON_MICRO_GUN:                // 1818
        break;
    case ICON_MICRO_BATTERY:            // 1800
        break;
    case ICON_GRENADE_LAUNCHER: // 1817
        tspr->xrepeat = 54;
        tspr->yrepeat = 52;
        break;
    case ICON_LG_GRENADE:               // 1831
        break;
    case ICON_LG_MINE:                  // 1842
        break;
    case ICON_RAIL_GUN:         // 1811
        tspr->xrepeat = 50;
        tspr->yrepeat = 54;
        break;
    case ICON_RAIL_AMMO:                // 1812
        break;
    case ICON_SM_MEDKIT:                // 1802
        break;
    case ICON_MEDKIT:                   // 1803
        break;
    case ICON_CHEMBOMB:
        tspr->xrepeat = 64;
        tspr->yrepeat = 47;
        break;
    case ICON_FLASHBOMB:
        tspr->xrepeat = 32;
        tspr->yrepeat = 34;
        break;
    case ICON_NUKE:
        break;
    case ICON_CALTROPS:
        tspr->xrepeat = 37;
        tspr->yrepeat = 30;
        break;
    case ICON_BOOSTER:                  // 1810
        tspr->xrepeat = 30;
        tspr->yrepeat = 38;
        break;
    case ICON_HEAT_CARD:                // 1819
        tspr->xrepeat = 46;
        tspr->yrepeat = 47;
        break;
    case ICON_REPAIR_KIT:               // 1813
        break;
    case ICON_EXPLOSIVE_BOX:            // 1801
        break;
    case ICON_ENVIRON_SUIT:             // 1837
        break;
    case ICON_FLY:                      // 1782
        break;
    case ICON_CLOAK:                    // 1826
        break;
    case ICON_NIGHT_VISION:             // 3031
        tspr->xrepeat = 59;
        tspr->yrepeat = 71;
        break;
    case ICON_NAPALM:                   // 3046
        break;
    case ICON_RING:                     // 3050
        break;
    case ICON_RINGAMMO:         // 3054
        break;
    case ICON_NAPALMAMMO:               // 3058
        break;
    case ICON_GRENADE:                  // 3059
        break;
    case ICON_ARMOR:                    // 3030
        tspr->xrepeat = 82;
        tspr->yrepeat = 84;
        break;
    case BLUE_KEY:                      // 1766
        break;
    case RED_KEY:                       // 1770
        break;
    case GREEN_KEY:                     // 1774
        break;
    case YELLOW_KEY:                    // 1778
        break;
    case BLUE_CARD:
    case RED_CARD:
    case GREEN_CARD:
    case YELLOW_CARD:
        tspr->xrepeat = 36;
        tspr->yrepeat = 33;
        break;
    case GOLD_SKELKEY:
    case SILVER_SKELKEY:
    case BRONZE_SKELKEY:
    case RED_SKELKEY:
        tspr->xrepeat = 39;
        tspr->yrepeat = 45;
        break;
    case SKEL_LOCKED:
    case SKEL_UNLOCKED:
        tspr->xrepeat = 47;
        tspr->yrepeat = 40;
        break;
    case RAMCARD_LOCKED:
    case RAMCARD_UNLOCKED:
    case CARD_LOCKED:
    case CARD_UNLOCKED:
        break;
    default:
        break;
    }
}

// Rotation angles for sprites
short rotang = 0;

void
JAnalyzeSprites(uspritetype * tspr)
{
    int i, currsprite;

    rotang += 4;
    if (rotang > 2047)
        rotang = 0;


    // Take care of autosizing
    DoAutoSize(tspr);

    if (videoGetRenderMode() >= REND_POLYMOST && md_tilehasmodel(tspr->picnum, 0) >= 0 && usemodels) return;

    // Check for voxels
    //if (bVoxelsOn)
    if (gs.Voxels)
    {
        if (aVoxelArray[tspr->picnum].Voxel >= 0)
        {
            // Turn on voxels
            tspr->picnum = aVoxelArray[tspr->picnum].Voxel;     // Get the voxel number
            tspr->cstat |= 48;          // Set stat to voxelize sprite
        }
    }
    else
    {
        switch (tspr->picnum)
        {
        case 764: // Gun barrel

            if (aVoxelArray[tspr->picnum].Voxel >= 0)
            {
                // Turn on voxels
                tspr->picnum = aVoxelArray[tspr->picnum].Voxel;     // Get the voxel number
                tspr->cstat |= 48;          // Set stat to voxelize sprite
            }
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Parental Lockout Stuff
//////////////////////////////////////////////////////////////////////////////////////////////
OrgTileList orgwalllist;                // The list containing orginal wall
// pics
OrgTileList orgwalloverlist;            // The list containing orginal wall
// over pics
OrgTileList orgsectorceilinglist;       // The list containing orginal sector
// ceiling pics
OrgTileList orgsectorfloorlist;         // The list containing orginal sector
// floor pics

void
InsertOrgTile(OrgTileP tp, OrgTileListP thelist)
{
    OrgTileP cur, nxt;

    ASSERT(tp);
    ASSERT(ValidPtr(tp));

    // if list is empty, insert at front
    if (EMPTY(thelist))
    {
        INSERT(thelist, tp);
        return;
    }

    // Otherwise insert it at end
    INSERT_TAIL(thelist, tp);
    return;
}


OrgTileP
InitOrgTile(OrgTileListP thelist)
{
    int i;
    OrgTileP tp;


    tp = (OrgTileP)CallocMem(sizeof(OrgTile), 1);

    ASSERT(tp);

    InsertOrgTile(tp, thelist);

    return tp;
}

void
KillOrgTile(OrgTileP tp)
{
    ASSERT(tp);
    ASSERT(ValidPtr(tp));

    REMOVE(tp);

    FreeMem(tp);
}

OrgTileP
FindOrgTile(short index, OrgTileListP thelist)
{
    OrgTileP tp, next_tp;

    if (EMPTY(thelist))
        return NULL;

    TRAVERSE(thelist, tp, next_tp)
    {
        if (tp->index == index)
            return tp;
    }

    return NULL;
}

// Call this at terminate game time
void
JS_UnInitLockouts(void)
{
    OrgTileP tp=NULL, next_tp=NULL;


    TRAVERSE(&orgwalllist, tp, next_tp)
    {
        KillOrgTile(tp);
    }
    TRAVERSE(&orgwalloverlist, tp, next_tp)
    {
        KillOrgTile(tp);
    }
    TRAVERSE(&orgsectorceilinglist, tp, next_tp)
    {
        KillOrgTile(tp);
    }
    TRAVERSE(&orgsectorfloorlist, tp, next_tp)
    {
        KillOrgTile(tp);
    }
}

/////////////////////////////////////////////////////
//  Initialize the original tiles list
//  Creates a list of all orginal tiles and their
//  replacements.  Several tiles can use the same
//  replacement tilenum, so the list is built
//  using the original tilenums as a basis for
//  memory allocation
//  t == 1 - wall
//  t == 2 - overpicnum
//  t == 3 - ceiling
//  t == 4 - floor
/////////////////////////////////////////////////////
void
JS_PlockError(short wall_num, short t)
{
    TerminateGame();
    printf("ERROR: JS_InitLockouts(), out of range tile number\n");
    switch (t)
    {
    case 1:
        printf("wall %d, x %d, y %d, pic %d\n", wall_num, TrackerCast(wall[wall_num].x), TrackerCast(wall[wall_num].y), TrackerCast(wall[wall_num].picnum));
        break;
    case 2:
        printf("wall %d, x %d, y %d, OVERpic %d\n", wall_num, TrackerCast(wall[wall_num].x), TrackerCast(wall[wall_num].y), TrackerCast(wall[wall_num].overpicnum));
        break;
    case 3:
        printf("sector %d, ceiling %d\n", wall_num, TrackerCast(sector[wall_num].ceilingpicnum));
        break;
    case 4:
        printf("sector %d, floor %d\n", wall_num, TrackerCast(sector[wall_num].floorpicnum));
        break;
    }
    exit(0);
}

void
JS_InitLockouts(void)
{
    SPRITEp sp;
    short i, num;
    OrgTileP tp;

    INITLIST(&orgwalllist);             // The list containing orginal wall
    // pics
    INITLIST(&orgwalloverlist);         // The list containing orginal wall
    // over pics
    INITLIST(&orgsectorceilinglist);    // The list containing orginal sector
    // ceiling pics
    INITLIST(&orgsectorfloorlist);      // The list containing orginal sector
    // floor pics

    // Check all walls
    for (i = 0; i < numwalls; i++)
    {
        short picnum;

        picnum = wall[i].picnum;
        if (aVoxelArray[picnum].Parental >= INVISTILE)
            JS_PlockError(i,1);

        if (aVoxelArray[picnum].Parental >= 0)
        {
            if ((tp = FindOrgTile(i, &orgwalllist)) == NULL)
                tp = InitOrgTile(&orgwalllist);
            tp->index = i;
            tp->orgpicnum = wall[i].picnum;
        }

        picnum = wall[i].overpicnum;
        if (aVoxelArray[picnum].Parental >= INVISTILE)
            JS_PlockError(i,2);

        if (aVoxelArray[picnum].Parental >= 0)
        {
            if ((tp = FindOrgTile(i, &orgwalloverlist)) == NULL)
                tp = InitOrgTile(&orgwalloverlist);
            tp->index = i;
            tp->orgpicnum = wall[i].overpicnum;
        }
    }
    // Check all ceilings and floors
    for (i = 0; i < numsectors; i++)
    {
        short picnum;

        picnum = sector[i].ceilingpicnum;
        if (aVoxelArray[picnum].Parental >= INVISTILE)
            JS_PlockError(i,3);

        if (aVoxelArray[picnum].Parental >= 0)
        {
            if ((tp = FindOrgTile(i, &orgsectorceilinglist)) == NULL)
                tp = InitOrgTile(&orgsectorceilinglist);
            tp->index = i;
            tp->orgpicnum = sector[i].ceilingpicnum;
        }

        picnum = sector[i].floorpicnum;
        if (aVoxelArray[picnum].Parental >= INVISTILE)
            JS_PlockError(i,2);

        if (aVoxelArray[picnum].Parental >= 0)
        {
            if ((tp = FindOrgTile(i, &orgsectorfloorlist)) == NULL)
                tp = InitOrgTile(&orgsectorfloorlist);
            tp->index = i;
            tp->orgpicnum = sector[i].floorpicnum;
        }
    }
}

/////////////////////////////////////////////////////
//  Switch back and forth between locked out stuff
/////////////////////////////////////////////////////
void
JS_ToggleLockouts(void)
{
    SPRITEp sp;
    short i, num;
    OrgTileP tp;


    // Check all walls
    for (i = 0; i < numwalls; i++)
    {
        short picnum;

        if (gs.ParentalLock)
        {
            picnum = wall[i].picnum;
            ASSERT(aVoxelArray[picnum].Parental < INVISTILE);   // Invalid, walls can't
            // be invisible
            if (aVoxelArray[picnum].Parental >= 0)
            {
                wall[i].picnum = aVoxelArray[picnum].Parental;
            }
        }
        else if ((tp = FindOrgTile(i, &orgwalllist)) != NULL)
            wall[i].picnum = tp->orgpicnum;     // Restore them


        if (gs.ParentalLock)
        {
            picnum = wall[i].overpicnum;
            ASSERT(aVoxelArray[picnum].Parental < INVISTILE);   // Invalid, walls can't
            // be invisible
            if (aVoxelArray[picnum].Parental >= 0)
            {
                wall[i].overpicnum = aVoxelArray[picnum].Parental;
            }
        }
        else if ((tp = FindOrgTile(i, &orgwalloverlist)) != NULL)
            wall[i].overpicnum = tp->orgpicnum; // Restore them
    }

    // Check all sectors
    for (i = 0; i < numsectors; i++)
    {
        short picnum;

        if (gs.ParentalLock)
        {
            picnum = sector[i].ceilingpicnum;
            ASSERT(aVoxelArray[picnum].Parental < INVISTILE);   // Invalid, walls can't
            // be invisible
            if (aVoxelArray[picnum].Parental >= 0)
            {
                sector[i].ceilingpicnum = aVoxelArray[picnum].Parental;
            }
        }
        else if ((tp = FindOrgTile(i, &orgsectorceilinglist)) != NULL)
            sector[i].ceilingpicnum = tp->orgpicnum;    // Restore them


        if (gs.ParentalLock)
        {
            picnum = sector[i].floorpicnum;
            ASSERT(aVoxelArray[picnum].Parental < INVISTILE);   // Invalid, walls can't
            // be invisible
            if (aVoxelArray[picnum].Parental >= 0)
            {
                sector[i].floorpicnum = aVoxelArray[picnum].Parental;
            }
        }
        else if ((tp = FindOrgTile(i, &orgsectorfloorlist)) != NULL)
            sector[i].floorpicnum = tp->orgpicnum;      // Restore them
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////

void
UnlockKeyLock(short key_num, short hit_sprite)
{
    SPRITEp sp;
    int SpriteNum = 0, NextSprite = 0, color = 0;

    // Get palette by looking at key number
    switch (key_num - 1)
    {
    case 0:                             // RED_KEY
        color = PALETTE_PLAYER9;
        break;
    case 1:                             // BLUE_KEY
        color = PALETTE_PLAYER7;
        break;
    case 2:                             // GREEN_KEY
        color = PALETTE_PLAYER6;
        break;
    case 3:                             // YELLOW_KEY
        color = PALETTE_PLAYER4;
        break;
    case 4:                             // SILVER_SKELKEY
        color = PALETTE_PLAYER4;
        break;
    case 5:                             // GOLD_SKELKEY
        color = PALETTE_PLAYER1;
        break;
    case 6:                             // BRONZE_SKELKEY
        color = PALETTE_PLAYER8;
        break;
    case 7:                             // RED_SKELKEY
        color = PALETTE_PLAYER9;
        break;
    }

    TRAVERSE_SPRITE_STAT(headspritestat[0], SpriteNum, NextSprite)
    {
        sp = &sprite[SpriteNum];

        switch (sp->picnum)
        {
        case SKEL_LOCKED:
            if (sp->pal == color)
            {
                PlaySound(DIGI_UNLOCK, &sp->x, &sp->y, &sp->z, v3df_doppler | v3df_dontpan);
                if (SpriteNum == hit_sprite)
                    sp->picnum = SKEL_UNLOCKED;
            }
            break;
        case RAMCARD_LOCKED:
            if (sp->pal == color)
            {
                PlaySound(DIGI_CARDUNLOCK, &sp->x, &sp->y, &sp->z, v3df_doppler | v3df_dontpan);
                sp->picnum = RAMCARD_UNLOCKED;
            }
            break;
        case CARD_LOCKED:
            if (sp->pal == color)
            {
                PlaySound(DIGI_RAMUNLOCK, &sp->x, &sp->y, &sp->z, v3df_doppler | v3df_dontpan);
                if (SpriteNum == hit_sprite)
                    sp->picnum = CARD_UNLOCKED;
                else
                    sp->picnum = CARD_UNLOCKED+1;
            }
            break;
        }

    }
}
