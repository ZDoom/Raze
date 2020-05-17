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
#include "ns.h"	// Must come before everything else!

#define sector_c_

#include "duke3d.h"

#include "secrets.h"
#include "v_video.h"
#include "glbackend/glbackend.h"

BEGIN_DUKE_NS

// PRIMITIVE
void operatejaildoors(int hitag);

static int g_haltSoundHack = 0;

uint8_t shadedsector[MAXSECTORS];


static void G_SetupCamTile(int spriteNum, int smoothRatio)
{
    int const viewscrTile = TILE_VIEWSCR;
    TileFiles.MakeCanvas(viewscrTile, tilesiz[PN(spriteNum)].x, tilesiz[PN(spriteNum)].y);

    vec3_t const camera = G_GetCameraPosition(spriteNum, smoothRatio);
    int const    saveMirror = display_mirror;

    auto canvas = renderSetTarget(viewscrTile);
    if (!canvas) return;

    screen->RenderTextureView(canvas, [=](IntRect& rect)
        {
            yax_preparedrawrooms();
            drawrooms(camera.x, camera.y, camera.z, SA(spriteNum), 100 + sprite[spriteNum].shade, SECT(spriteNum));
            yax_drawrooms(G_DoSpriteAnimations, SECT(spriteNum), 0, smoothRatio);

            display_mirror = 3;
            G_DoSpriteAnimations(camera.x, camera.y, camera.z, SA(spriteNum), smoothRatio);
            display_mirror = saveMirror;
            renderDrawMasks();

        });
    renderRestoreTarget();
    
}

void G_AnimateCamSprite(int smoothRatio)
{
#ifdef DEBUG_VALGRIND_NO_SMC
    return;
#endif

    if (g_curViewscreen < 0)
        return;

    int const spriteNum = g_curViewscreen;

    if (totalclock >= T1(spriteNum) + ud.camera_time)
    {
        DukePlayer_t const *const pPlayer = g_player[screenpeek].ps;

        if (pPlayer->newowner >= 0)
            OW(spriteNum) = pPlayer->newowner;

        if (OW(spriteNum) >= 0 && dist(&sprite[pPlayer->i], &sprite[spriteNum]) < VIEWSCREEN_ACTIVE_DISTANCE)
        {

            G_SetupCamTile(OW(spriteNum), smoothRatio);
#ifdef POLYMER
            // Force texture update on viewscreen sprite in Polymer!
            if (videoGetRenderMode() == REND_POLYMER)
                polymer_invalidatesprite(spriteNum);
#endif
        }

        T1(spriteNum) = (int32_t) totalclock;
    }
}


END_DUKE_NS
