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
#include "ns.h"

#include "build.h"

#include "names.h"
#include "names2.h"
#include "panel.h"
#include "game.h"
#include "tags.h"
#include "sector.h"
#include "player.h"
#include "sprite.h"
#include "jsector.h"
#include "jtags.h"
#include "lists.h"
#include "pal.h"
#include "parent.h"
#include "v_video.h"
#include "render.h"

EXTERN_CVAR(Bool, testnewrenderer)

BEGIN_SW_NS


// V A R I A B L E   D E C L A R A T I O N S //////////////////////////////////////////////////////

MIRRORTYPE mirror[MAXMIRRORS];

short mirrorcnt; //, floormirrorcnt;
//short floormirrorsector[MAXMIRRORS];
bool mirrorinview;
uint32_t oscilationclock;

// Voxel stuff
//bool bVoxelsOn = true;                  // Turn voxels on by default
bool bSpinBobVoxels = false;            // Do twizzly stuff to voxels, but
// not by default
bool bAutoSize = true;                  // Autosizing on/off

//extern int chainnumpages;
extern AMB_INFO ambarray[];
extern short NormalVisibility;

extern ParentalStruct aVoxelArray[MAXTILES];

// F U N C T I O N S //////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////
//  SpawnWallSound
/////////////////////////////////////////////////////
void SpawnWallSound(short sndnum, short i)
{
    short SpriteNum;
    vec3_t mid;
    SPRITEp sp;

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

    PlaySound(sndnum, sp, v3df_dontpan | v3df_doppler);
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

ANIMATOR GenerateDrips;

/////////////////////////////////////////////////////
//  Initialize any of my special use sprites
/////////////////////////////////////////////////////
void
JS_SpriteSetup(void)
{
    SPRITEp sp;
    USERp u;
    short i;

    SWStatIterator it(STAT_DEFAULT);
    while (auto actor = it.Next())
    {
        short tag;

        sp = &actor->s();
        tag = sp->hitag;

        // Non static camera. Camera sprite will be drawn!
        if (tag == MIRROR_CAM && sp->picnum != ST1)
        {
            // Just change it to static, sprite has all the info I need
//          u = SpawnUser(SpriteNum, sp->picnum, nullptr);
//          RESET(sp->cstat, CSTAT_SPRITE_BLOCK);
//          SET(sp->cstat, CSTAT_SPRITE_BLOCK_HITSCAN);
            change_actor_stat(actor, STAT_SPAWN_SPOT);
        }

        switch (sp->picnum)
        {
        case ST1:
            if (tag == MIRROR_CAM)
            {
                // Just change it to static, sprite has all the info I need
                // ST1 cameras won't move with SOBJ's!
                change_actor_stat(actor, STAT_ST1);
            }
            else if (tag == MIRROR_SPAWNSPOT)
            {
                // Just change it to static, sprite has all the info I need
                change_actor_stat(actor, STAT_ST1);
            }
            else if (tag == AMBIENT_SOUND)
            {
                change_actor_stat(actor, STAT_AMBIENT);
            }
            else if (tag == TAG_ECHO_SOUND)
            {
                change_actor_stat(actor, STAT_ECHO);
            }
            else if (tag == TAG_DRIPGEN)
            {
                u = SpawnUser(actor, 0, nullptr);

                ASSERT(u != nullptr);
                u->RotNum = 0;
                u->WaitTics = sp->lotag * 120;

                u->ActorActionFunc = GenerateDrips;

                change_actor_stat(actor, STAT_NO_STATE);
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
            PlaySound(DIGI_FIRE1, sp, v3df_follow|v3df_dontpan|v3df_doppler);
            break;
        case 795:
        case 880:
            PlaySound(DIGI_WATERFLOW1, sp, v3df_follow|v3df_dontpan|v3df_doppler);
            break;
        case 460:  // Wind Chimes
            InitAmbient(79, sp);
            break;

        }
    }
    // Check for certain walls to make sounds
    for (i = 0; i < numwalls; i++)
    {
        short picnum;


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
    }
}


/////////////////////////////////////////////////////
//  Initialize the mirrors
/////////////////////////////////////////////////////
void JS_InitMirrors(void)
{
    short startwall, endwall;
    int i, j, s;
    int SpriteNum;
    bool Found_Cam = false;


    // Set all the mirror struct values to -1
    memset(mirror, 0xFF, sizeof(mirror));

    mirrorinview = false;               // Initially set global mirror flag
    // to no mirrors seen

    // Scan wall tags for mirrors
    mirrorcnt = 0;
	tileDelete(MIRROR);
	oscilationclock = I_GetBuildTime();

    for (i = 0; i < MAXMIRRORS; i++)
    {
		tileDelete(i + MIRRORLABEL);
        mirror[i].campic = -1;
        mirror[i].camsprite = -1;
        mirror[i].camera = -1;
        mirror[i].ismagic = false;
    }

    for (i = 0; i < numwalls; i++)
    {
        s = wall[i].nextsector;
        if ((s >= 0) && (wall[i].overpicnum == MIRROR) && (wall[i].cstat & 32))
        {
            if ((sector[s].floorstat & 1) == 0)
            {
                if (mirrorcnt >= MAXMIRRORS)
                {
                    Printf("MAXMIRRORS reached! Skipping mirror wall[%d]\n", i);
                    wall[i].overpicnum = sector[s].ceilingpicnum;
                    continue;
                }

                wall[i].overpicnum = MIRRORLABEL + mirrorcnt;
                wall[i].picnum = MIRRORLABEL + mirrorcnt;
                sector[s].ceilingpicnum = MIRRORLABEL + mirrorcnt;
                sector[s].floorpicnum = MIRRORLABEL + mirrorcnt;
                sector[s].floorstat |= 1;
                mirror[mirrorcnt].mirrorwall = i;
                mirror[mirrorcnt].mirrorsector = s;
                mirror[mirrorcnt].numspawnspots = 0;
                mirror[mirrorcnt].ismagic = false;
                do if (wall[i].lotag == TAG_WALL_MAGIC_MIRROR)
                {
                    int ii;
                    SPRITEp sp;

                    Found_Cam = false;

                    StatIterator it(STAT_ST1);
                    while ((ii = it.NextIndex()) >= 0)
                    {
                        sp = &sprite[ii];
                        // if correct type and matches
                        if (sp->hitag == MIRROR_CAM && sp->lotag == wall[i].hitag)
                        {
                            mirror[mirrorcnt].camera = ii;
                            // Set up camera variables
                            SP_TAG5(sp) = sp->ang;      // Set current angle to
                            // sprite angle
                            Found_Cam = true;
                        }
                    }

                    it.Reset(STAT_SPAWN_SPOT);
                    while ((ii = it.NextIndex()) >= 0)
                    {
                        sp = &sprite[ii];

                        // if correct type and matches
                        if (sp->hitag == MIRROR_CAM && sp->lotag == wall[i].hitag)
                        {
                            mirror[mirrorcnt].camera = ii;
                            // Set up camera variables
                            SP_TAG5(sp) = sp->ang;      // Set current angle to
                            // sprite angle
                            Found_Cam = true;
                        }
                    }

                    if (!Found_Cam)
                    {
                        Printf("Cound not find the camera view sprite for match %d\n", wall[i].hitag);
                        Printf("Map Coordinates: x = %d, y = %d\n", wall[i].x, wall[i].y);
                        break;
                    }

                    mirror[mirrorcnt].ismagic = true;

                    Found_Cam = false;
                    if (TEST_BOOL1(&sprite[mirror[mirrorcnt].camera]))
                    {
                        StatIterator it(STAT_DEFAULT);
                        while ((SpriteNum = it.NextIndex()) >= 0)
                        {
                            sp = &sprite[SpriteNum];
                            if (sp->picnum >= CAMSPRITE && sp->picnum < CAMSPRITE + 8 &&
                                sp->hitag == wall[i].hitag)
                            {
                                mirror[mirrorcnt].campic = sp->picnum;
                                mirror[mirrorcnt].camsprite = SpriteNum;

                                // JBF: commenting out this line results in the screen in $BULLET being visible
								tileDelete(mirror[mirrorcnt].campic);

                                Found_Cam = true;
                            }
                        }

                        if (!Found_Cam)
                        {
                            Printf("Did not find drawtotile for camera number %d\n", mirrorcnt);
                            Printf("wall[%d].hitag == %d\n", i, wall[i].hitag);
                            Printf("Map Coordinates: x = %d, y = %d\n", wall[i].x, wall[i].y);
                            RESET_BOOL1(&sprite[mirror[mirrorcnt].camera]);
                        }
                    }

                    // For magic mirrors, set allowable viewing time to 30
                    // secs
                    // Base rate is supposed to be 120, but time is double
                    // what I expect
                    mirror[mirrorcnt].maxtics = 60 * 30;

                }
                while (0);

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
void drawroomstotile(int daposx, int daposy, int daposz,
                     binangle ang, fixedhoriz horiz, short dacursectnum, short tilenume, double smoothratio)
{
	TileFiles.MakeCanvas(tilenume, tileWidth(tilenume), tileHeight(tilenume));

    auto canvas = renderSetTarget(tilenume);
    if (!canvas) return;

    screen->RenderTextureView(canvas, [=](IntRect& rect)
        {
            if (!testnewrenderer)
            {
                renderDrawRoomsQ16(daposx, daposy, daposz, ang.asq16(), horiz.asq16(), dacursectnum, false);
                analyzesprites(pm_tsprite, pm_spritesortcnt, daposx, daposy, daposz, ang.asbuild());
                renderDrawMasks();
            }
            else
            {
                render_camtex(nullptr, { daposx, daposy, daposz }, dacursectnum, ang, horiz, buildang(0), tileGetTexture(tilenume), rect, smoothratio);
            }
        });

    renderRestoreTarget();
}

void
JS_ProcessEchoSpot()
{
    int i;
    SPRITEp tp;
    int j,dist;
    PLAYERp pp = Player+screenpeek;
    int16_t reverb;
    bool reverb_set = false;

    // Process echo sprites
    StatIterator it(STAT_ECHO);
    while ((i = it.NextIndex()) >= 0)
    {
        dist = 0x7fffffff;

        tp = &sprite[i];

        j = abs(tp->x - pp->posx);
        j += abs(tp->y - pp->posy);
        if (j < dist)
            dist = j;

        if (dist <= SP_TAG4(tp)) // tag4 = ang
        {
            reverb = SP_TAG2(tp);
            if (reverb > 200) reverb = 200;
            if (reverb < 100) reverb = 100;

            COVER_SetReverb(reverb);
            reverb_set = true;
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
int lastcamclock;
// views
short camplayerview = 1;                // Don't show yourself!


// Hack job alert!
// Mirrors and cameras are maintained in the same data structure, but for hardware rendering they cannot be interleaved.
// So this function replicates JS_DrawMirrors to only process the camera textures but not change any global state.
void JS_DrawCameras(PLAYERp pp, int tx, int ty, int tz, double smoothratio)
{
    int j, cnt;
    int dist;
    int tposx, tposy; // Camera
    int* longptr;

    bool bIsWallMirror = false;
    int camclock = I_GetBuildTime();

    camloopcnt += camclock - lastcamclock;
    if (camloopcnt > (60 * 5))          // 5 seconds per player view
    {
        camloopcnt = 0;
        camplayerview++;
        if (camplayerview >= numplayers)
            camplayerview = 1;
    }
    lastcamclock = camclock;

    {
        uint32_t oscilation_delta = camclock - oscilationclock;
        oscilation_delta -= oscilation_delta % 4;
        oscilationclock += oscilation_delta;
        oscilation_delta *= 2;
        for (cnt = MAXMIRRORS - 1; cnt >= 0; cnt--) 
        {
            if (!mirror[cnt].ismagic) continue; // these are definitely not camera textures.

            if (testgotpic(cnt + MIRRORLABEL) || ((unsigned)mirror[cnt].campic < MAXTILES && testgotpic(mirror[cnt].campic)))
            {
                // Do not change any global state here!
                bIsWallMirror = testgotpic(cnt + MIRRORLABEL);
                dist = 0x7fffffff;

                if (bIsWallMirror)
                {
                    j = abs(wall[mirror[cnt].mirrorwall].x - tx);
                    j += abs(wall[mirror[cnt].mirrorwall].y - ty);
                    if (j < dist)
                        dist = j;
                }
                else
                {
                    SPRITEp tp;

                    tp = &sprite[mirror[cnt].camsprite];

                    j = abs(tp->x - tx);
                    j += abs(tp->y - ty);
                    if (j < dist)
                        dist = j;
                }


                SPRITEp sp = nullptr;
                short w;
                int dx, dy, dz, tdx, tdy, tdz, midx, midy;


                ASSERT(mirror[cnt].camera != -1);

                sp = &sprite[mirror[cnt].camera];

                ASSERT(sp);

                // Calculate the angle of the mirror wall
                w = mirror[cnt].mirrorwall;

                // Get wall midpoint for offset in mirror view
                midx = (wall[w].x + wall[wall[w].point2].x) / 2;
                midy = (wall[w].y + wall[wall[w].point2].y) / 2;

                // Finish finding offsets
                tdx = abs(midx - tx);
                tdy = abs(midy - ty);

                if (midx >= tx)
                    dx = sp->x - tdx;
                else
                    dx = sp->x + tdx;

                if (midy >= ty)
                    dy = sp->y - tdy;
                else
                    dy = sp->y + tdy;

                tdz = abs(tz - sp->z);
                if (tz >= sp->z)
                    dz = sp->z + tdz;
                else
                    dz = sp->z - tdz;


                // Is it a TV cam or a teleporter that shows destination?
                // true = It's a TV cam
                mirror[cnt].mstate = m_normal;
                if (TEST_BOOL1(sp))
                    mirror[cnt].mstate = m_viewon;

                // Show teleport destination
                // NOTE: Adding MAXSECTORS lets you draw a room, even if
                // you are outside of it!
                if (mirror[cnt].mstate == m_viewon)
                {
                    bool DoCam = false;

                    if (mirror[cnt].campic == -1)
                    {
                        Printf("Missing campic for mirror %d\n",cnt);
                        Printf("Map Coordinates: x = %d, y = %d\n",midx,midy);
                        return;
                    }

                    // BOOL2 = Oscilate camera
                    if (TEST_BOOL2(sp) && MoveSkip2 == 0)
                    {
                        if (TEST_BOOL3(sp)) // If true add increment to
                        // angle else subtract
                        {
                            // Store current angle in TAG5
                            SP_TAG5(sp) = NORM_ANGLE((SP_TAG5(sp) + oscilation_delta));

                            // TAG6 = Turn radius
                            if (abs(getincangle(sp->ang, SP_TAG5(sp))) >= SP_TAG6(sp))
                            {
                                SP_TAG5(sp) = NORM_ANGLE((SP_TAG5(sp) - oscilation_delta));
                                RESET_BOOL3(sp);    // Reverse turn
                                // direction.
                            }
                        }
                        else
                        {
                            // Store current angle in TAG5
                            SP_TAG5(sp) = NORM_ANGLE((SP_TAG5(sp) - oscilation_delta));

                            // TAG6 = Turn radius
                            if (abs(getincangle(sp->ang, SP_TAG5(sp))) >= SP_TAG6(sp))
                            {
                                SP_TAG5(sp) = NORM_ANGLE((SP_TAG5(sp) + oscilation_delta));
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

                    // Set the horizon value.
                    auto camhoriz = q16horiz(clamp(IntToFixed(SP_TAG7(sp) - 100), gi->playerHorizMin(), gi->playerHorizMax()));

                    // If player is dead still then update at MoveSkip4
                    // rate.
                    if (pp->posx == pp->oposx && pp->posy == pp->oposy && pp->posz == pp->oposz)
                        DoCam = true;


                    // Set up the tile for drawing
                    TileFiles.MakeCanvas(mirror[cnt].campic, 128, 128);

                    {
                        if (dist < MAXCAMDIST)
                        {
                            PLAYERp cp = Player + camplayerview;

                            if (TEST_BOOL11(sp) && numplayers > 1)
                            {
                                drawroomstotile(cp->posx, cp->posy, cp->posz, cp->angle.ang, cp->horizon.horiz, cp->cursectnum, mirror[cnt].campic, smoothratio);
                            }
                            else
                            {
                                drawroomstotile(sp->x, sp->y, sp->z, buildang(SP_TAG5(sp)), camhoriz, sp->sectnum, mirror[cnt].campic, smoothratio);
                            }
                        }
                    }
                }
            }
        }
    }
}

// Workaround until the camera code can be refactored to process all camera textures that were visible last frame.
// Need to stash the parameters for later use. This is only used to find the nearest camera.
static PLAYERp cam_pp;
static int cam_tx, cam_ty, cam_tz;
static int oldstat;

void JS_CameraParms(PLAYERp pp, int tx, int ty, int tz)
{
    cam_pp = pp;
    cam_tx = tx;
    cam_ty = ty;
    cam_tz = tz;
}

void GameInterface::UpdateCameras(double smoothratio)
{
    JS_DrawCameras(cam_pp, cam_tx, cam_ty, cam_tz, smoothratio);
}

void GameInterface::EnterPortal(spritetype* viewer, int type)
{
    if (type == PORTAL_WALL_MIRROR) display_mirror++;
}

void GameInterface::LeavePortal(spritetype* viewer, int type)
{
    if (type == PORTAL_WALL_MIRROR) display_mirror--;
}


void
DoAutoSize(tspriteptr_t tspr)
{
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
JAnalyzeSprites(tspriteptr_t tspr)
{
    rotang += 4;
    if (rotang > 2047)
        rotang = 0;


    // Take care of autosizing
    DoAutoSize(tspr);

    if (md_tilehasmodel(tspr->picnum, 0) >= 0 && hw_models) return;

    // Check for voxels
    //if (bVoxelsOn)
    if (r_voxels)
    {
        if (aVoxelArray[tspr->picnum].Voxel >= 0 && !(spriteext[tspr->owner].flags & SPREXT_NOTMD))
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

            if (!r_voxels || (spriteext[tspr->owner].flags & SPREXT_NOTMD))
            {
                tspr->cstat |= 16;
                break;
            }

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

////////////////////////////////////////////////////////////////////////////////////////////////

void
UnlockKeyLock(short key_num, short hit_sprite)
{
    SPRITEp sp;
    int SpriteNum = 0, color = 0;

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

    StatIterator it(STAT_DEFAULT);
    while ((SpriteNum = it.NextIndex()) >= 0)
    {
        sp = &sprite[SpriteNum];

        switch (sp->picnum)
        {
        case SKEL_LOCKED:
            if (sp->pal == color)
            {
                PlaySound(DIGI_UNLOCK, sp, v3df_doppler | v3df_dontpan);
                if (SpriteNum == hit_sprite)
                    sp->picnum = SKEL_UNLOCKED;
            }
            break;
        case RAMCARD_LOCKED:
            if (sp->pal == color)
            {
                PlaySound(DIGI_CARDUNLOCK, sp, v3df_doppler | v3df_dontpan);
                sp->picnum = RAMCARD_UNLOCKED;
            }
            break;
        case CARD_LOCKED:
            if (sp->pal == color)
            {
                PlaySound(DIGI_RAMUNLOCK, sp, v3df_doppler | v3df_dontpan);
                if (SpriteNum == hit_sprite)
                    sp->picnum = CARD_UNLOCKED;
                else
                    sp->picnum = CARD_UNLOCKED+1;
            }
            break;
        }

    }
}
END_SW_NS
