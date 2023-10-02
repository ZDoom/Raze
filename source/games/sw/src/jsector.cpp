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
#include "models/modeldata.h"

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

//extern int chainnumpages;
extern AMB_INFO ambarray[];
extern short NormalVisibility;

// F U N C T I O N S //////////////////////////////////////////////////////////////////////////////



short CheckTileSound(short picnum)
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
void JS_SpriteSetup(void)
{
    SWStatIterator it(STAT_DEFAULT);
    while (auto itActor = it.Next())
    {
        int tag;

        tag = itActor->spr.hitag;

        // Non static camera. Camera sprite will be drawn!
        if (tag == MIRROR_CAM && itActor->spr.picnum != ST1)
        {
            // Just change it to static, sprite has all the info I need
            change_actor_stat(itActor, STAT_SPAWN_SPOT);
        }

        switch (itActor->spr.picnum)
        {
        case ST1:
            if (tag == MIRROR_CAM)
            {
                // Just change it to static, sprite has all the info I need
                // ST1 cameras won't move with SOBJ's!
                change_actor_stat(itActor, STAT_ST1);
            }
            else if (tag == MIRROR_SPAWNSPOT)
            {
                // Just change it to static, sprite has all the info I need
                change_actor_stat(itActor, STAT_ST1);
            }
            else if (tag == AMBIENT_SOUND)
            {
                change_actor_stat(itActor, STAT_AMBIENT);
            }
            else if (tag == TAG_ECHO_SOUND)
            {
                change_actor_stat(itActor, STAT_ECHO);
            }
            else if (tag == TAG_DRIPGEN)
            {
                SpawnUser(itActor, 0, nullptr);

                itActor->user.__legacyState.RotNum = 0;
                itActor->user.WaitTics = itActor->spr.lotag * 120;

                itActor->user.ActorActionFunc = AF(GenerateDrips);

                change_actor_stat(itActor, STAT_NO_STATE);
                itActor->spr.cstat |= CSTAT_SPRITE_INVISIBLE;
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
            PlaySound(DIGI_FIRE1, itActor, v3df_follow|v3df_dontpan|v3df_doppler);
            break;
        case 795:
        case 880:
            PlaySound(DIGI_WATERFLOW1, itActor, v3df_follow|v3df_dontpan|v3df_doppler);
            break;
        case 460:  // Wind Chimes
            InitAmbient(79, itActor);
            break;

        }
    }
    // Check for certain walls to make sounds
    for(auto& wal : wall)
    {
        int surf = tilesurface(wal.walltexture);
        if (surf == TSURF_WATER || surf == TSURF_LAVA || surf == TSURF_SHALLOWWATER)
            wal.extra |= WALLFX_DONT_STICK;
    }
}


/////////////////////////////////////////////////////
//  Initialize the mirrors
/////////////////////////////////////////////////////
void JS_InitMirrors(void)
{
    short startwall, endwall;
    int i, j, s;
    bool Found_Cam = false;


    // Set all the mirror struct values to -1
    memset(mirror, 0xFF, sizeof(mirror));

    mirrorinview = false;               // Initially set global mirror flag
    // to no mirrors seen

    // Scan wall tags for mirrors
    mirrorcnt = 0;
	oscilationclock = I_GetBuildTime();

    for (i = 0; i < MAXMIRRORS; i++)
    {
        mirror[i].campic = -1;
        mirror[i].camspriteActor = nullptr;
        mirror[i].cameraActor = nullptr;
        mirror[i].ismagic = false;
    }

    auto mi = tileGetTextureID(MIRROR);
    for(auto& wal : wall)
    {
        if (wal.twoSided() && (wal.overtexture == mi) && (wal.cstat & CSTAT_WALL_1WAY))
        {
            auto sec = wal.nextSector();
            if ((sec->floorstat & CSTAT_SECTOR_SKY) == 0)
            {
                if (mirrorcnt >= MAXMIRRORS)
                {
                    Printf("MAXMIRRORS reached! Skipping mirror wall\n");
                    wal.setovertexture(sec->ceilingtexture);
                    continue;
                }
                auto ml = tileGetTextureID(MIRRORLABEL);
                wal.setovertexture(ml);
                wal.setwalltexture(ml);
                sec->floorstat |= CSTAT_SECTOR_SKY;
                mirror[mirrorcnt].mirrorWall = &wal;
                mirror[mirrorcnt].mirrorSector = sec;
                mirror[mirrorcnt].numspawnspots = 0;
                mirror[mirrorcnt].ismagic = false;
                do if (wal.lotag == TAG_WALL_MAGIC_MIRROR)
                {
                    int ii;

                    Found_Cam = false;

                    SWStatIterator it(STAT_ST1);
                    while (auto itActor = it.Next())
                    {
                        // if correct type and matches
                        if (itActor->spr.hitag == MIRROR_CAM && itActor->spr.lotag == wal.hitag)
                        {
                            mirror[mirrorcnt].cameraActor = itActor;
                            // Set up camera variables
                            itActor->user.sang = itActor->spr.Angles.Yaw;      // Set current angle to sprite angle
                            Found_Cam = true;
                        }
                    }

                    it.Reset(STAT_SPAWN_SPOT);
                    while (auto itActor = it.Next())
                    {
                        // if correct type and matches
                        if (itActor->spr.hitag == MIRROR_CAM && itActor->spr.lotag == wal.hitag)
                        {
                            mirror[mirrorcnt].cameraActor = itActor;
                            // Set up camera variables
                            itActor->user.sang = itActor->spr.Angles.Yaw;      // Set current angle to sprite angle
                            Found_Cam = true;
                        }
                    }

                    if (!Found_Cam)
                    {
                        Printf("Cound not find the camera view sprite for match %d\n", wal.hitag);
                        Printf("Map Coordinates: x = %d, y = %d\n", int(wal.pos.X), int(wal.pos.Y));
                        break;
                    }

                    mirror[mirrorcnt].ismagic = true;

                    Found_Cam = false;
                    if (TEST_BOOL1(mirror[mirrorcnt].cameraActor))
                    {
                        it.Reset(STAT_DEFAULT);
                        while (auto itActor = it.Next())
                        {
                            if (itActor->spr.picnum >= CAMSPRITE && itActor->spr.picnum < CAMSPRITE + 8 &&
                                itActor->spr.hitag == wal.hitag)
                            {
                                mirror[mirrorcnt].campic = itActor->spr.picnum;
                                mirror[mirrorcnt].camspriteActor = itActor;
                                Found_Cam = true;
                            }
                        }

                        if (!Found_Cam)
                        {
                            Printf("Did not find drawtotile for camera number %d\n", mirrorcnt);
                            Printf("wall(%d).hitag == %d\n", wallindex(&wal), wal.hitag);
                            Printf("Map Coordinates: x = %d, y = %d\n", int(wal.pos.X), int(wal.pos.Y));
                            RESET_BOOL1(mirror[mirrorcnt].cameraActor);
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

                mirrorcnt++;
            }
            else
                wal.setovertexture(sec->ceilingtexture);
        }
    }
}                                   // InitMirrors

/////////////////////////////////////////////////////
//  Draw a 3d screen to a specific tile
/////////////////////////////////////////////////////
void drawroomstotile(const DVector3& pos, DAngle ang, DAngle horiz, sectortype* dacursect, short tilenume, double smoothratio)
{
    auto tex = tileGetTexture(tilenume);
    auto canvas = dynamic_cast<FCanvasTexture*>(tex->GetTexture());
    if (!canvas) return;

    screen->RenderTextureView(canvas, [=](IntRect& rect)
        {
               render_camtex(nullptr, pos, dacursect, DRotator(horiz, ang, nullAngle), tex, rect, smoothratio);
        });

}

void JS_ProcessEchoSpot()
{
    SWPlayer* pp = &Player[screenpeek];
    int16_t reverb;
    bool reverb_set = false;

    // Process echo sprites
    SWStatIterator it(STAT_ECHO);
    while (auto actor = it.Next())
    {
        double maxdist = SP_TAG4(actor) * maptoworld;
        auto v = actor->spr.pos.XY() - pp->GetActor()->spr.pos.XY();
        double dist = abs(v.X) + abs(v.Y);

        if (dist <= maxdist) // tag4 = ang
        {
            reverb = SP_TAG2(actor);
            if (reverb > 200) reverb = 200;
            if (reverb < 100) reverb = 100;

            COVER_SetReverb(reverb);
            reverb_set = true;
        }
    }
    if (!(pp->Flags & PF_DIVING) && !reverb_set && pp->Reverb <= 0)
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
void JS_DrawCameras(SWPlayer* pp, const DVector3& campos, double smoothratio)
{
    int  cnt;
    double dist;
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
        DAngle oscillation_angle = mapangle(oscilation_delta);
        for (cnt = MAXMIRRORS - 1; cnt >= 0; cnt--) 
        {
            if (!mirror[cnt].ismagic || mirror[cnt].campic < 0) continue; // these are definitely not camera textures.

            auto tex = tileGetTexture(mirror[cnt].campic);
            if (tex && tex->isSeen(true))
            {
                DSWActor* camactor = mirror[cnt].camspriteActor;
                DVector2 vec = camactor->spr.pos - campos.XY();
                dist = abs(vec.X) + abs(vec.Y);


                short w;

                camactor = mirror[cnt].cameraActor;
                if (!camactor) continue;

                // Calculate the angle of the mirror wall
                auto wal = mirror[cnt].mirrorWall;

                // Get wall midpoint for offset in mirror view
                DVector2 mid = wal->center();

                // Finish finding offsets
                DVector3 dpos;
                DVector3 tdpos;
                tdpos.XY() = mid - campos;

                if (mid.X >= campos.X)
                    dpos.X = camactor->spr.pos.X - campos.X;
                else
                    dpos.X = camactor->spr.pos.X + campos.X;

                if (mid.Y >= campos.Y)
                    dpos.Y = camactor->spr.pos.Y - campos.Y;
                else
                    dpos.Y = camactor->spr.pos.Y + campos.Y;

                tdpos.Z = abs(campos.Z - camactor->spr.pos.Z);
                if (tdpos.Z >= camactor->spr.pos.Z)
                    dpos.Z = camactor->spr.pos.Z + tdpos.Z;
                else
                    dpos.Z = camactor->spr.pos.Z - tdpos.Z;


                // Is it a TV cam or a teleporter that shows destination?
                // true = It's a TV cam
                mirror[cnt].mstate = m_normal;
                if (TEST_BOOL1(camactor))
                    mirror[cnt].mstate = m_viewon;

                // Show teleport destination
                // NOTE: Adding true lets you draw a room, even if
                // you are outside of it!
                if (mirror[cnt].mstate == m_viewon)
                {
                    bool DoCam = false;

                    if (mirror[cnt].campic == -1)
                    {
                        Printf("Missing campic for mirror %d\n",cnt);
                        Printf("Map Coordinates: x = %d, y = %d\n",int(mid.X), int(mid.Y));
                        return;
                    }

                    auto maxang = mapangle(SP_TAG6(camactor));
                    // BOOL2 = Oscilate camera
                    if (TEST_BOOL2(camactor) && MoveSkip2 == 0)
                    {
                        if (TEST_BOOL3(camactor)) // If true add increment to
                        // angle else subtract
                        {
                            // Store current angle in TAG5
                            camactor->user.sang += oscillation_angle;

                            // TAG6 = Turn radius
                            if (absangle(camactor->spr.Angles.Yaw, camactor->user.sang) >= maxang)
                            {
                                camactor->user.sang -= oscillation_angle;
                                RESET_BOOL3(camactor);    // Reverse turn direction.
                            }
                        }
                        else
                        {
                            // Store current angle in TAG5
                            camactor->user.sang -= oscillation_angle;

                            // TAG6 = Turn radius
                            if (absangle(camactor->spr.Angles.Yaw, camactor->user.sang) >= maxang)
                            {
                                camactor->user.sang += oscillation_angle;
                                SET_BOOL3(camactor);      // Reverse turn direction.
                            }
                        }
                    }
                    else if (!TEST_BOOL2(camactor))
                    {
                        camactor->user.sang = camactor->spr.Angles.Yaw;      // Copy sprite angle to tag5
                    }

                    // Set the horizon value.
                    auto camhoriz = SP_TAG7(camactor);

                    // If player is dead still then update at MoveSkip4
                    // rate.
                    if (pp->GetActor()->spr.pos == pp->GetActor()->opos)
                        DoCam = true;


                    // Set up the tile for drawing

                    {
                        if (dist < MAXCAMDIST)
                        {
                            SWPlayer* cp = &Player[camplayerview];

                            if (TEST_BOOL11(camactor) && numplayers > 1)
                            {
                                drawroomstotile(cp->GetActor()->getPosWithOffsetZ(), cp->GetActor()->spr.Angles.Yaw, cp->GetActor()->spr.Angles.Pitch, cp->cursector, mirror[cnt].campic, smoothratio);
                            }
                            else
                            {
                                drawroomstotile(camactor->spr.pos, camactor->user.sang, maphoriz(150 - (camhoriz == 0 ? 100 : camhoriz)), camactor->sector(), mirror[cnt].campic, smoothratio);
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
static SWPlayer* cam_pp;
DVector3 cam_pos;
static int oldstat;

void JS_CameraParms(SWPlayer* pp, const DVector3& tpos)
{
    cam_pp = pp;
    cam_pos = tpos;
}

void GameInterface::UpdateCameras(double smoothratio)
{
    JS_DrawCameras(cam_pp, cam_pos, smoothratio);
}

void GameInterface::EnterPortal(DCoreActor* viewer, int type)
{
    if (type == PORTAL_WALL_MIRROR) display_mirror++;
}

void GameInterface::LeavePortal(DCoreActor* viewer, int type)
{
    if (type == PORTAL_WALL_MIRROR) display_mirror--;
}


void DoAutoSize(tspritetype* tspr)
{
    switch (tspr->picnum)
    {
    case ICON_STAR:                     // 1793
        break;
    case ICON_UZI:                      // 1797
		tspr->scale = DVector2(0.671875, 0.625);
        break;
    case ICON_UZIFLOOR:         // 1807
		tspr->scale = DVector2(0.671875, 0.625);
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
		tspr->scale = DVector2(0.890625, 0.90625);
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
		tspr->scale = DVector2(0.84375, 0.8125);
        break;
    case ICON_LG_GRENADE:               // 1831
        break;
    case ICON_LG_MINE:                  // 1842
        break;
    case ICON_RAIL_GUN:         // 1811
		tspr->scale = DVector2(0.78125, 0.84375);
        break;
    case ICON_RAIL_AMMO:                // 1812
        break;
    case ICON_SM_MEDKIT:                // 1802
        break;
    case ICON_MEDKIT:                   // 1803
        break;
    case ICON_CHEMBOMB:
		tspr->scale = DVector2(1, 0.734375);
        break;
    case ICON_FLASHBOMB:
		tspr->scale = DVector2(0.5, 0.53125);
        break;
    case ICON_NUKE:
        break;
    case ICON_CALTROPS:
		tspr->scale = DVector2(0.578125, 0.46875);
        break;
    case ICON_BOOSTER:                  // 1810
		tspr->scale = DVector2(0.46875, 0.59375);
        break;
    case ICON_HEAT_CARD:                // 1819
		tspr->scale = DVector2(0.71875, 0.734375);
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
		tspr->scale = DVector2(0.921875, 1.109375);
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
		tspr->scale = DVector2(1.28125, 1.3125);
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
		tspr->scale = DVector2(0.5625, 0.5155625);
        break;
    case GOLD_SKELKEY:
    case SILVER_SKELKEY:
    case BRONZE_SKELKEY:
    case RED_SKELKEY:
		tspr->scale = DVector2(0.609375, 0.703125);
        break;
    case SKEL_LOCKED:
    case SKEL_UNLOCKED:
		tspr->scale = DVector2(0.734375, 0.625);
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

void JAnalyzeSprites(tspritetype* tspr)
{
    // Take care of autosizing
    DoAutoSize(tspr);

    if (tspr->picnum == 764 && !tilehasmodelorvoxel(tspr->spritetexture(), 0))
    {
        tspr->cstat |= CSTAT_SPRITE_ALIGNMENT_WALL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////

void UnlockKeyLock(short key_num, DSWActor* hitActor)
{
    int color = 0;

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

    SWStatIterator it(STAT_DEFAULT);
    while (auto itActor = it.Next())
    {
        switch (itActor->spr.picnum)
        {
        case SKEL_LOCKED:
            if (itActor->spr.pal == color)
            {
                PlaySound(DIGI_UNLOCK, itActor, v3df_doppler | v3df_dontpan);
                if (itActor == hitActor)
                    itActor->spr.picnum = SKEL_UNLOCKED;
            }
            break;
        case RAMCARD_LOCKED:
            if (itActor->spr.pal == color)
            {
                PlaySound(DIGI_CARDUNLOCK, itActor, v3df_doppler | v3df_dontpan);
                itActor->spr.picnum = RAMCARD_UNLOCKED;
            }
            break;
        case CARD_LOCKED:
            if (itActor->spr.pal == color)
            {
                PlaySound(DIGI_RAMUNLOCK, itActor, v3df_doppler | v3df_dontpan);
                if (itActor == hitActor)
                    itActor->spr.picnum = CARD_UNLOCKED;
                else
                    itActor->spr.picnum = CARD_UNLOCKED+1;
            }
            break;
        }

    }
}
END_SW_NS
