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

#include "duke3d.h"
#include "compat.h"
#include "screens.h"

#include "sbar.h"
#include "mdsprite.h"
#include "gamecvars.h"
#include "menu/menu.h"
#include "mapinfo.h"
#include "v_2ddrawer.h"
#include "screenjob.h"

BEGIN_DUKE_NS

#define quotepulseshade (sintable[((uint32_t)totalclock<<5)&2047]>>11)

void drawstatusbar_d(int snum);
void drawstatusbar_r(int snum);
void drawoverheadmap(int cposx, int cposy, int czoom, int cang);
void cameratext(int i);

int32_t g_crosshairSum = -1;
// yxaspect and viewingrange just before the 'main' drawrooms call
int32_t dr_yxaspect, dr_viewingrange;
double g_moveActorsTime, g_moveWorldTime;  // in ms, smoothed
int32_t g_noLogoAnim = 0;
int32_t g_noLogo = 0;


void P_SetGamePalette(DukePlayer_t *player, uint32_t palid, ESetPalFlags set)
{
    if (palid >= MAXBASEPALS)
        palid = 0;

    player->palette = palid;

    if (player != g_player[screenpeek].ps)
        return;

    videoSetPalette(palid, set);
}


////////// TINT ACCUMULATOR //////////

typedef struct {
    int32_t r, g, b;
    // f: 0-63 scale
    int32_t maxf, sumf;
} palaccum_t;

#define PALACCUM_INITIALIZER { 0, 0, 0, 0, 0 }

/* For a picture frame F and n tints C_1, C_2, ... C_n weighted a_1, a_2,
* ... a_n (on a 0-1 scale), the faded frame is calculated as
*
*    F_new := (1-max_i(a_i))*F + d*sum_i(a_i), where
*
*    d := max_i(a_i)/sum_i(a_i).
*
* This means that
*  1) tint application is independent of their order.
*  2) going from n+1 to n tints is continuous when the leaving one has faded.
*
* But note that for more than one tint, the composite tint will in general
* change its hue as the ratio of the weights of the individual ones changes.
*/
static void palaccum_add(palaccum_t *pa, const palette_t *pal, int32_t f)
{
    f = clamp(f, 0, 63);
    if (f == 0)
        return;

    pa->maxf = max(pa->maxf, f);
    pa->sumf += f;

    pa->r += f*clamp(pal->r, 0, 63);
    pa->g += f*clamp(pal->g, 0, 63);
    pa->b += f*clamp(pal->b, 0, 63);
}

static void G_FadePalaccum(const palaccum_t *pa)
{
    videoFadePalette(tabledivide32_noinline(pa->r, pa->sumf)<<2,
        tabledivide32_noinline(pa->g, pa->sumf)<<2,
        tabledivide32_noinline(pa->b, pa->sumf)<<2, pa->maxf<<2);
}


static int32_t gtextsc(int32_t sc)
{
    return scale(sc, hud_textscale, 400);
}

////////// DISPLAYREST //////////

static inline void G_MoveClouds(void)
{
    int32_t i;

    if (totalclock <= g_cloudClock && totalclock >= (g_cloudClock-7))
        return;

    g_cloudClock = totalclock+6;

    g_cloudX += sintable[(fix16_to_int(g_player[screenpeek].ps->q16ang)+512)&2047]>>9;
    g_cloudY += sintable[fix16_to_int(g_player[screenpeek].ps->q16ang)&2047]>>9;

    for (i=numclouds-1; i>=0; i--)
    {
        sector[clouds[i]].ceilingxpanning = g_cloudX>>6;
        sector[clouds[i]].ceilingypanning = g_cloudY>>6;
    }
}



void displayweapon(int snum);

void G_DisplayRest(int32_t smoothratio)
{
    int32_t i, j;
    palaccum_t tint = PALACCUM_INITIALIZER;

    DukePlayer_t *const pp = g_player[screenpeek].ps;
    int32_t cposx, cposy, cang;

    // this takes care of fullscreen tint for OpenGL
    if (videoGetRenderMode() >= REND_POLYMOST)
    {
        auto & fstint = lookups.tables[MAXPALOOKUPS-1];

        if (pp->palette == WATERPAL)
        {
            fstint.tintColor.r = 224;
            fstint.tintColor.g = 192;
            fstint.tintColor.b = 255;
            fstint.tintFlags = 0;
        }
        else if (pp->palette == SLIMEPAL)
        {
            fstint.tintColor.r = 208;
            fstint.tintColor.g = 255;
            fstint.tintColor.b = 192;
            fstint.tintFlags = 0;
        }
        else
        {
            fstint.tintColor.r = 255;
            fstint.tintColor.g = 255;
            fstint.tintColor.b = 255;
            fstint.tintFlags = 0;
        }
    }
    palaccum_add(&tint, &pp->pals, pp->pals.f);
    if (!RR)
    {
        static const palette_t loogiepal = { 0, 63, 0, 0 };

        palaccum_add(&tint, &loogiepal, pp->loogcnt>>1);
    }

    if (restorepalette)
    {
        // reset a normal palette
        static uint32_t omovethingscnt;

        if (restorepalette < 2 || omovethingscnt+1 == g_moveThingsCount)
        {
            int32_t pal = pp->palette;

            // restorepalette < 0: reset tinting, too (e.g. when loading new game)
            P_SetGamePalette(pp, pal, (restorepalette > 0) ? Pal_DontResetFade : ESetPalFlags::FromInt(0));
            restorepalette = 0;
        }
        else
        {
            // delay setting the palette by one game tic
            omovethingscnt = g_moveThingsCount;
        }
    }

    i = pp->cursectnum;
    if (i > -1)
    {
        const walltype *wal = &wall[sector[i].wallptr];

        show2dsector.Set(i);
        for (j=sector[i].wallnum; j>0; j--, wal++)
        {
            i = wal->nextsector;
            if (i < 0) continue;
            if (wal->cstat&0x0071) continue;
            if (wall[wal->nextwall].cstat&0x0071) continue;
            if (sector[i].lotag == 32767) continue;
            if (sector[i].ceilingz >= sector[i].floorz) continue;
            show2dsector.Set(i);
        }
    }

    if (ud.camerasprite == -1)
    {
        if (ud.overhead_on != 2)
        {
            if (!RR && pp->newowner >= 0)
                cameratext(pp->newowner);
            else
            {
                PspTwoDSetter set;
                fi.displayweapon(screenpeek);

                if (pp->over_shoulder_on == 0)
                    fi.displaymasks(screenpeek);
    }
            if (!RR)
                G_MoveClouds();
        }

        if (ud.overhead_on > 0)
        {
            // smoothratio = min(max(smoothratio,0),65536);
            smoothratio = calc_smoothratio(totalclock, ototalclock);
            dointerpolations(smoothratio);

            if (ud.scrollmode == 0)
            {
                if (pp->newowner == -1 && !ud.pause_on)
                {
                    if (screenpeek == myconnectindex && numplayers > 1)
                    {
                        cposx = omypos.x + mulscale16(mypos.x-omypos.x, smoothratio);
                        cposy = omypos.y + mulscale16(mypos.y-omypos.y, smoothratio);
                        cang = fix16_to_int(omyang) + mulscale16((fix16_to_int(myang+F16(1024)-omyang)&2047)-1024, smoothratio);
                    }
                    else
                    {
                        cposx = pp->opos.x + mulscale16(pp->pos.x-pp->opos.x, smoothratio);
                        cposy = pp->opos.y + mulscale16(pp->pos.y-pp->opos.y, smoothratio);
                        cang = fix16_to_int(pp->oq16ang) + mulscale16((fix16_to_int(pp->q16ang+F16(1024)-pp->oq16ang)&2047)-1024, smoothratio);
                    }
                }
                else
                {
                    cposx = pp->opos.x;
                    cposy = pp->opos.y;
                    cang = fix16_to_int(pp->oq16ang);
                }
            }
            else
            {
                if (!ud.pause_on)
                {
                    ud.fola += ud.folavel>>3;
                    ud.folx += (ud.folfvel*sintable[(512+2048-ud.fola)&2047])>>14;
                    ud.foly += (ud.folfvel*sintable[(512+1024-512-ud.fola)&2047])>>14;
                }
                cposx = ud.folx;
                cposy = ud.foly;
                cang = ud.fola;
            }

            if (ud.overhead_on == 2)
            {
                twod->ClearScreen();
                drawbackground();
                renderDrawMapView(cposx, cposy, pp->zoom, cang);
            }
            drawoverheadmap(cposx, cposy, pp->zoom, cang);

            restoreinterpolations();
        }
    }

    if (isRR()) drawstatusbar_r(screenpeek);
    else drawstatusbar_d(screenpeek);

    if (!DEER && g_player[myconnectindex].ps->newowner == -1 && ud.overhead_on == 0 && cl_crosshair && ud.camerasprite == -1)
    {
        int32_t a = TILE_CROSSHAIR;

        if ((unsigned) a < MAXTILES)
        {
            vec2_t crosshairpos = { (160<<16) - (g_player[myconnectindex].ps->look_ang<<15), 100<<16 };
            //vec2_t crosshairpos = { ud.returnvar[0], ud.returnvar[1] };
            uint32_t crosshair_o = 1|2;
            uint32_t crosshair_scale = divscale16(cl_crosshairscale, 100);
            if (RR)
                crosshair_scale >>= 1;

            rotatesprite_win(crosshairpos.x, crosshairpos.y, crosshair_scale, 0, a, 0, 0, crosshair_o);
        }
    }

    if (ud.pause_on == 1 && (g_player[myconnectindex].ps->gm & MODE_MENU) == 0)
        fi.PrintPaused();

    mdpause = (ud.pause_on || (g_player[myconnectindex].ps->gm&MODE_MENU && numplayers < 2));

    Net_DisplaySyncMsg();

    if (VOLUMEONE)
    {
        if (g_showShareware > 0 && (g_player[myconnectindex].ps->gm&MODE_MENU) == 0)
            rotatesprite_fs((320-50)<<16, 9<<16, 65536L, 0, TILE_BETAVERSION, 0, 0, 2+8+16+128);
    }

    {
        if (g_player[myconnectindex].ps->gm&MODE_TYPE)
            Net_SendMessage();
        //else
            //M_DisplayMenus();
    }

    {
        static int32_t applied = 0;

        if (tint.maxf)
        {
            G_FadePalaccum(&tint);
            applied = 1;
        }
        else if (applied)
        {
            // be sure to always un-apply a tint.
            videoFadePalette(0, 0, 0, 0);
            applied = 0;
        }
    }
}

void dobonus_d(bool bonusonly, CompletionFunc completion);
void dobonus_r(bool bonusonly, CompletionFunc completion);

void G_BonusScreen(int32_t bonusonly)
{
    if (isRRRA());
    else if (isRR()) dobonus_r(bonusonly, nullptr);
    else dobonus_d(bonusonly, nullptr);

    // This hack needs to go away!
    if (RRRA_EndEpisode)
    {
        RRRA_EndEpisode = 0;
        ud.m_volume_number = ud.volume_number = 1;
        m_level_number = ud.level_number = 0;
        ud.eog = 0;
    }
}

END_DUKE_NS
