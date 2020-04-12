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

#include "anim.h"
#include "compat.h"
#include "demo.h"
#include "duke3d.h"

#include "mdsprite.h"
#include "sbar.h"
#include "screens.h"
#include "gamecvars.h"
#include "menu/menu.h"
#include "mapinfo.h"
#include "v_2ddrawer.h"

BEGIN_DUKE_NS

#define quotepulseshade (sintable[((uint32_t)totalclock<<5)&2047]>>11)

int32_t g_crosshairSum = -1;
// yxaspect and viewingrange just before the 'main' drawrooms call
int32_t dr_yxaspect, dr_viewingrange;
double g_moveActorsTime, g_moveWorldTime;  // in ms, smoothed
int32_t g_noLogoAnim = 0;
int32_t g_noLogo = 0;

void PlayBonusMusic()
{
    if (MusicEnabled() && mus_enabled)
        S_PlaySound(BONUSMUSIC, CHAN_AUTO, CHANF_UI);

}

////////// OFTEN-USED FEW-LINERS //////////
#ifndef EDUKE32_STANDALONE
static void G_HandleEventsWhileNoInput(void)
{
    inputState.ClearAllInput();

    while (!inputState.CheckAllInput())
        gameHandleEvents();
}

static int32_t G_PlaySoundWhileNoInput(int32_t soundnum)
{
    S_PlaySound(soundnum);
    inputState.ClearAllInput();
    while (S_CheckSoundPlaying(soundnum))
    {
        gameHandleEvents();
        if (inputState.CheckAllInput())
        {
            return 1;
        }
    }

    return 0;
}
#endif
//////////

void P_SetGamePalette(DukePlayer_t *player, uint32_t palid, ESetPalFlags set)
{
    if (palid >= MAXBASEPALS)
        palid = 0;

    player->palette = palid;

    if (player != g_player[screenpeek].ps)
        return;

    videoSetPalette(0, palid, set);
}


#define SCORESHEETOFFSET -20
static void G_ShowScores(void)
{
    int32_t t, i;

    if (g_mostConcurrentPlayers > 1 && (g_gametypeFlags[ud.coop]&GAMETYPE_SCORESHEET))
    {
        gametext_center(SCORESHEETOFFSET+58+2, GStrings("Multiplayer Totals"));
        gametext_center(SCORESHEETOFFSET+58+10, currentLevel->DisplayName());

        t = 0;
        minitext(70, SCORESHEETOFFSET+80, GStrings("Name"), 8, 2+8+16+ROTATESPRITE_MAX);
        minitext(170, SCORESHEETOFFSET+80, GStrings("Frags"), 8, 2+8+16+ROTATESPRITE_MAX);
        minitext(200, SCORESHEETOFFSET+80, GStrings("Deaths"), 8, 2+8+16+ROTATESPRITE_MAX);
        minitext(235, SCORESHEETOFFSET+80, GStrings("Ping"), 8, 2+8+16+ROTATESPRITE_MAX);

        for (i=g_mostConcurrentPlayers-1; i>=0; i--)
        {
            if (!g_player[i].playerquitflag)
                continue;

            minitext(70, SCORESHEETOFFSET+90+t, g_player[i].user_name, g_player[i].ps->palookup, 2+8+16+ROTATESPRITE_MAX);

            Bsprintf(tempbuf, "%-4d", g_player[i].ps->frag);
            minitext(170, SCORESHEETOFFSET+90+t, tempbuf, 2, 2+8+16+ROTATESPRITE_MAX);

            Bsprintf(tempbuf, "%-4d", g_player[i].frags[i] + g_player[i].ps->fraggedself);
            minitext(200, SCORESHEETOFFSET+90+t, tempbuf, 2, 2+8+16+ROTATESPRITE_MAX);

            Bsprintf(tempbuf, "%-4d", g_player[i].ping);
            minitext(235, SCORESHEETOFFSET+90+t, tempbuf, 2, 2+8+16+ROTATESPRITE_MAX);

            t += 7;
        }
    }
}
#undef SCORESHEETOFFSET

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

static void G_DrawCameraText(int16_t i)
{
    if (VM_OnEvent(EVENT_DISPLAYCAMERAOSD, i, screenpeek) != 0)
        return;

    if (!T1(i))
    {
        rotatesprite_win(24<<16, 33<<16, 65536L, 0, CAMCORNER, 0, 0, 2);
        rotatesprite_win((320-26)<<16, 34<<16, 65536L, 0, CAMCORNER+1, 0, 0, 2);
        rotatesprite_win(22<<16, 163<<16, 65536L, 512, CAMCORNER+1, 0, 0, 2+4);
        rotatesprite_win((310-10)<<16, 163<<16, 65536L, 512, CAMCORNER+1, 0, 0, 2);

        if ((int32_t) totalclock&16)
            rotatesprite_win(46<<16, 32<<16, 65536L, 0, CAMLIGHT, 0, 0, 2);
    }
    else
    {
        int32_t flipbits = ((int32_t) totalclock<<1)&48;

        for (bssize_t x=-64; x<394; x+=64)
            for (bssize_t y=0; y<200; y+=64)
                rotatesprite_win(x<<16, y<<16, 65536L, 0, STATIC, 0, 0, 2+flipbits);
    }
}

static inline void G_MoveClouds(void)
{
    int32_t i;

    if (totalclock <= g_cloudClock && totalclock >= (g_cloudClock-7))
        return;

    g_cloudClock = totalclock+6;

    g_cloudX += sintable[(fix16_to_int(g_player[screenpeek].ps->q16ang)+512)&2047]>>9;
    g_cloudY += sintable[fix16_to_int(g_player[screenpeek].ps->q16ang)&2047]>>9;

    for (i=g_cloudCnt-1; i>=0; i--)
    {
        sector[g_cloudSect[i]].ceilingxpanning = g_cloudX>>6;
        sector[g_cloudSect[i]].ceilingypanning = g_cloudY>>6;
    }
}

static void G_DrawOverheadMap(int32_t cposx, int32_t cposy, int32_t czoom, int16_t cang)
{
    int32_t i, j, k, l, x1, y1, x2=0, y2=0, x3, y3, x4, y4, ox, oy, xoff, yoff;
    int32_t dax, day, cosang, sinang, xspan, yspan, sprx, spry;
    int32_t xrepeat, yrepeat, z1, z2, startwall, endwall, tilenum, daang;
    int32_t xvect, yvect, xvect2, yvect2;
    int16_t p;
    PalEntry col;
    uwallptr_t wal, wal2;
    spritetype *spr;

    int32_t tmpydim = (xdim*5)/8;

    renderSetAspect(65536, divscale16(tmpydim*320, xdim*200));

    xvect = sintable[(-cang)&2047] * czoom;
    yvect = sintable[(1536-cang)&2047] * czoom;
    xvect2 = mulscale16(xvect, yxaspect);
    yvect2 = mulscale16(yvect, yxaspect);

    //Draw red lines
    for (i=numsectors-1; i>=0; i--)
    {
        if (!gFullMap && !show2dsector[i]) continue;

        startwall = sector[i].wallptr;
        endwall = sector[i].wallptr + sector[i].wallnum;

        z1 = sector[i].ceilingz;
        z2 = sector[i].floorz;

        for (j=startwall, wal=(uwallptr_t)&wall[startwall]; j<endwall; j++, wal++)
        {
            k = wal->nextwall;
            if (k < 0) continue;

            if (sector[wal->nextsector].ceilingz == z1 && sector[wal->nextsector].floorz == z2)
                    if (((wal->cstat|wall[wal->nextwall].cstat)&(16+32)) == 0) continue;

            if (!gFullMap && !show2dsector[wal->nextsector])
                col = PalEntry(170, 170, 170);
            else continue;

            ox = wal->x-cposx;
            oy = wal->y-cposy;
            x1 = dmulscale16(ox, xvect, -oy, yvect)+(xdim<<11);
            y1 = dmulscale16(oy, xvect2, ox, yvect2)+(ydim<<11);

            wal2 = (uwallptr_t)&wall[wal->point2];
            ox = wal2->x-cposx;
            oy = wal2->y-cposy;
            x2 = dmulscale16(ox, xvect, -oy, yvect)+(xdim<<11);
            y2 = dmulscale16(oy, xvect2, ox, yvect2)+(ydim<<11);

            drawlinergb(x1, y1, x2, y2, col);
        }
    }

    //Draw sprites
    k = g_player[screenpeek].ps->i;
    if (!FURY) for (i=numsectors-1; i>=0; i--)
    {
        if (!gFullMap && !show2dsector[i]) continue;
        for (j=headspritesect[i]; j>=0; j=nextspritesect[j])
        {
            spr = &sprite[j];

            if (j == k || (spr->cstat&0x8000) || spr->cstat == 257 || spr->xrepeat == 0) continue;

            col = PalEntry(0, 170, 170);
            if (spr->cstat & 1) col = PalEntry(170, 0, 170);

            sprx = spr->x;
            spry = spr->y;

            if ((spr->cstat&257) != 0) switch (spr->cstat&48)
            {
            case 0:
                //                    break;

                ox = sprx-cposx;
                oy = spry-cposy;
                x1 = dmulscale16(ox, xvect, -oy, yvect);
                y1 = dmulscale16(oy, xvect2, ox, yvect2);

                ox = (sintable[(spr->ang+512)&2047]>>7);
                oy = (sintable[(spr->ang)&2047]>>7);
                x2 = dmulscale16(ox, xvect, -oy, yvect);
                y2 = dmulscale16(oy, xvect, ox, yvect);

                x3 = mulscale16(x2, yxaspect);
                y3 = mulscale16(y2, yxaspect);

                drawlinergb(x1-x2+(xdim<<11), y1-y3+(ydim<<11),
                    x1+x2+(xdim<<11), y1+y3+(ydim<<11), col);
                drawlinergb(x1-y2+(xdim<<11), y1+x3+(ydim<<11),
                    x1+x2+(xdim<<11), y1+y3+(ydim<<11), col);
                drawlinergb(x1+y2+(xdim<<11), y1-x3+(ydim<<11),
                    x1+x2+(xdim<<11), y1+y3+(ydim<<11), col);
                break;

            case 16:
                if (spr->picnum == LASERLINE)
                {
                    x1 = sprx;
                    y1 = spry;
                    tilenum = spr->picnum;
                    xoff = picanm[tilenum].xofs + spr->xoffset;
                    if ((spr->cstat&4) > 0) xoff = -xoff;
                    k = spr->ang;
                    l = spr->xrepeat;
                    dax = sintable[k&2047]*l;
                    day = sintable[(k+1536)&2047]*l;
                    l = tilesiz[tilenum].x;
                    k = (l>>1)+xoff;
                    x1 -= mulscale16(dax, k);
                    x2 = x1+mulscale16(dax, l);
                    y1 -= mulscale16(day, k);
                    y2 = y1+mulscale16(day, l);

                    ox = x1-cposx;
                    oy = y1-cposy;
                    x1 = dmulscale16(ox, xvect, -oy, yvect);
                    y1 = dmulscale16(oy, xvect2, ox, yvect2);

                    ox = x2-cposx;
                    oy = y2-cposy;
                    x2 = dmulscale16(ox, xvect, -oy, yvect);
                    y2 = dmulscale16(oy, xvect2, ox, yvect2);

                    drawlinergb(x1+(xdim<<11), y1+(ydim<<11),
                        x2+(xdim<<11), y2+(ydim<<11), col);
                }

                break;

            case 32:
                tilenum = spr->picnum;
                xoff = picanm[tilenum].xofs + spr->xoffset;
                yoff = picanm[tilenum].yofs + spr->yoffset;
                if ((spr->cstat&4) > 0) xoff = -xoff;
                if ((spr->cstat&8) > 0) yoff = -yoff;

                k = spr->ang;
                cosang = sintable[(k+512)&2047];
                sinang = sintable[k&2047];
                xspan = tilesiz[tilenum].x;
                xrepeat = spr->xrepeat;
                yspan = tilesiz[tilenum].y;
                yrepeat = spr->yrepeat;

                dax = ((xspan>>1)+xoff)*xrepeat;
                day = ((yspan>>1)+yoff)*yrepeat;
                x1 = sprx + dmulscale16(sinang, dax, cosang, day);
                y1 = spry + dmulscale16(sinang, day, -cosang, dax);
                l = xspan*xrepeat;
                x2 = x1 - mulscale16(sinang, l);
                y2 = y1 + mulscale16(cosang, l);
                l = yspan*yrepeat;
                k = -mulscale16(cosang, l);
                x3 = x2+k;
                x4 = x1+k;
                k = -mulscale16(sinang, l);
                y3 = y2+k;
                y4 = y1+k;

                ox = x1-cposx;
                oy = y1-cposy;
                x1 = dmulscale16(ox, xvect, -oy, yvect);
                y1 = dmulscale16(oy, xvect2, ox, yvect2);

                ox = x2-cposx;
                oy = y2-cposy;
                x2 = dmulscale16(ox, xvect, -oy, yvect);
                y2 = dmulscale16(oy, xvect2, ox, yvect2);

                ox = x3-cposx;
                oy = y3-cposy;
                x3 = dmulscale16(ox, xvect, -oy, yvect);
                y3 = dmulscale16(oy, xvect2, ox, yvect2);

                ox = x4-cposx;
                oy = y4-cposy;
                x4 = dmulscale16(ox, xvect, -oy, yvect);
                y4 = dmulscale16(oy, xvect2, ox, yvect2);

                drawlinergb(x1+(xdim<<11), y1+(ydim<<11),
                    x2+(xdim<<11), y2+(ydim<<11), col);

                drawlinergb(x2+(xdim<<11), y2+(ydim<<11),
                    x3+(xdim<<11), y3+(ydim<<11), col);

                drawlinergb(x3+(xdim<<11), y3+(ydim<<11),
                    x4+(xdim<<11), y4+(ydim<<11), col);

                drawlinergb(x4+(xdim<<11), y4+(ydim<<11),
                    x1+(xdim<<11), y1+(ydim<<11), col);

                break;
            }
        }
    }

    //Draw white lines
    for (i=numsectors-1; i>=0; i--)
    {
        if (!gFullMap && !show2dsector[i]) continue;

        startwall = sector[i].wallptr;
        endwall = sector[i].wallptr + sector[i].wallnum;

        k = -1;
        for (j=startwall, wal=(uwallptr_t)&wall[startwall]; j<endwall; j++, wal++)
        {
            if (wal->nextwall >= 0) continue;

            if (tilesiz[wal->picnum].x == 0) continue;
            if (tilesiz[wal->picnum].y == 0) continue;

            if (j == k)
            {
                x1 = x2;
                y1 = y2;
            }
            else
            {
                ox = wal->x-cposx;
                oy = wal->y-cposy;
                x1 = dmulscale16(ox, xvect, -oy, yvect)+(xdim<<11);
                y1 = dmulscale16(oy, xvect2, ox, yvect2)+(ydim<<11);
            }

            k = wal->point2;
            wal2 = (uwallptr_t)&wall[k];
            ox = wal2->x-cposx;
            oy = wal2->y-cposy;
            x2 = dmulscale16(ox, xvect, -oy, yvect)+(xdim<<11);
            y2 = dmulscale16(oy, xvect2, ox, yvect2)+(ydim<<11);

            drawlinergb(x1, y1, x2, y2, PalEntry(170, 170, 170));
        }
    }

    videoSetCorrectedAspect();

    for (TRAVERSE_CONNECT(p))
    {
        if (ud.scrollmode && p == screenpeek) continue;

        auto const pPlayer = g_player[p].ps;
        auto const pSprite = (uspriteptr_t)&sprite[pPlayer->i];

        ox = pSprite->x - cposx;
        oy = pSprite->y - cposy;
        daang = (pSprite->ang - cang) & 2047;
        if (p == screenpeek)
        {
            ox = 0;
            oy = 0;
            daang = 0;
        }
        x1 = mulscale16(ox, xvect) - mulscale16(oy, yvect);
        y1 = mulscale16(oy, xvect2) + mulscale16(ox, yvect2);

        if (p == screenpeek || GTFLAGS(GAMETYPE_OTHERPLAYERSINMAP))
        {
            if (pSprite->xvel > 16 && pPlayer->on_ground)
                i = APLAYERTOP+(((int32_t) totalclock>>4)&3);
            else
                i = APLAYERTOP;

            i = VM_OnEventWithReturn(EVENT_DISPLAYOVERHEADMAPPLAYER, pPlayer->i, p, i);

            if (i < 0)
                continue;

            j = klabs(pPlayer->truefz - pPlayer->pos.z) >> 8;
            j = mulscale16(czoom * (pSprite->yrepeat + j), yxaspect);

            if (j < 22000) j = 22000;
            else if (j > (65536<<1)) j = (65536<<1);

            rotatesprite_win((x1<<4)+(xdim<<15), (y1<<4)+(ydim<<15), j, daang, i, pSprite->shade,
                P_GetOverheadPal(pPlayer), 0);
        }
    }
}

#ifdef DEBUGGINGAIDS
sprstat_t g_spriteStat;
#endif

FString G_PrintCoords(int32_t snum)
{
    const int32_t x = g_Debug ? 288 : 0;
    int32_t y = 0;

    auto const ps = g_player[snum].ps;
    const int32_t sectnum = ps->cursectnum;

    if ((g_gametypeFlags[ud.coop] & GAMETYPE_FRAGBAR))
    {
        if (ud.multimode > 4)
            y = 32;
        else if (g_netServer || ud.multimode > 1)
            y = 24;
    }
    FString out;

    out.AppendFormat("XYZ= (%d, %d, %d)\n", ps->pos.x, ps->pos.y, ps->pos.z);
    char ang[16], horiz[16], horizoff[16];
    fix16_to_str(ps->q16ang, ang, 2);
    fix16_to_str(ps->q16horiz, horiz, 2);
    fix16_to_str(ps->q16horizoff, horizoff, 2);
    out.AppendFormat("A/H/HO= %s, %s, %s\n", ang, horiz, horizoff);
    out.AppendFormat("VEL= (%d, %d, %d) + (%d, %d, 0)\n", ps->vel.x>>14, ps->vel.y>>14, ps->vel.z, ps->fric.x>>5, ps->fric.y>>5);
    out.AppendFormat("OG= %d  SBRIDGE=%d SBS=%d\n", ps->on_ground, ps->spritebridge, ps->sbs);
    if (sectnum >= 0)
        out.AppendFormat("SECT= %d (LO=%d EX=%d)\n", sectnum, TrackerCast(sector[sectnum].lotag), TrackerCast(sector[sectnum].extra));
    else
        out.AppendFormat("SECT= %d\n", sectnum);

    out.AppendFormat("\nTHOLD= %d ", ps->transporter_hold);
    out.AppendFormat("GAMETIC= %u, TOTALCLOCK=%d\n", g_moveThingsCount, (int32_t) totalclock);
#ifdef DEBUGGINGAIDS
    out.AppendFormat("NUMSPRITES= %d\n", Numsprites);
    if (g_moveThingsCount > g_spriteStat.lastgtic + REALGAMETICSPERSEC)
    {
        g_spriteStat.lastgtic = g_moveThingsCount;
        g_spriteStat.lastnumins = g_spriteStat.numins;
        g_spriteStat.numins = 0;
    }
    out.AppendFormat("INSERTIONS/s= %u\n", g_spriteStat.lastnumins);
    out.AppendFormat("ONSCREEN= %d\n", g_spriteStat.numonscreen);
#endif
    out.AppendFormat("\nVR=%.03f  YX=%.03f", (double) dr_viewingrange/65536.0, (double) dr_yxaspect/65536.0);
    return out;
}

FString GameInterface::GetCoordString()
{
    return G_PrintCoords(screenpeek);
}


#define LOW_FPS ((videoGetRenderMode() == REND_CLASSIC) ? 35 : 50)
#define SLOW_FRAME_TIME 20

#if defined GEKKO
# define FPS_YOFFSET 16
#else
# define FPS_YOFFSET 0
#endif

FString GameInterface::statFPS(void)
{
	FString output;
	static int32_t frameCount;
    static double cumulativeFrameDelay;
    static double lastFrameTime;
    static float lastFPS, minFPS = std::numeric_limits<float>::max(), maxFPS;
    static double minGameUpdate = std::numeric_limits<double>::max(), maxGameUpdate;

    double frameTime = timerGetHiTicks();
    double frameDelay = frameTime - lastFrameTime;
    cumulativeFrameDelay += frameDelay;

    if (frameDelay >= 0)
    {
        int32_t x = (xdim <= 640);

        //if (r_showfps)
        {
			output.AppendFormat("%.1f ms, %5.1f fps\n", frameDelay, lastFPS);

            if (r_showfps > 1)
            {
				output.AppendFormat("max: %5.1f fps\n", maxFPS);
				output.AppendFormat("min: %5.1f fps\n", minFPS);
            }
            if (r_showfps > 2)
            {
                if (g_gameUpdateTime > maxGameUpdate) maxGameUpdate = g_gameUpdateTime;
                if (g_gameUpdateTime < minGameUpdate) minGameUpdate = g_gameUpdateTime;

				output.AppendFormat("Game Update: %2.2f ms + draw: %2.2f ms\n", g_gameUpdateTime, g_gameUpdateAndDrawTime - g_gameUpdateTime);
				output.AppendFormat("GU min/max/avg: %5.2f/%5.2f/%5.2f ms\n", minGameUpdate, maxGameUpdate, g_gameUpdateAvgTime);
				output.AppendFormat("G_MoveActors(): %.3f ms\n", g_moveActorsTime);
				output.AppendFormat("G_MoveWorld(): %.3f ms\n", g_moveWorldTime);
            }

#ifndef NETCODE_DISABLE
            Net_PrintLag(output);
#endif
        }

        if (cumulativeFrameDelay >= 1000.0)
        {
            lastFPS = 1000.f * frameCount / cumulativeFrameDelay;
            g_frameRate = Blrintf(lastFPS);
            frameCount = 0;
            cumulativeFrameDelay = 0.0;

            if (r_showfps > 1)
            {
                if (lastFPS > maxFPS) maxFPS = lastFPS;
                if (lastFPS < minFPS) minFPS = lastFPS;

                static int secondCounter;

                if (++secondCounter >= r_showfpsperiod)
                {
                    maxFPS = (lastFPS + maxFPS) * .5f;
                    minFPS = (lastFPS + minFPS) * .5f;
                    maxGameUpdate = (g_gameUpdateTime + maxGameUpdate) * 0.5;
                    minGameUpdate = (g_gameUpdateTime + minGameUpdate) * 0.5;
                    secondCounter = 0;
                }
            }
        }
        frameCount++;
    }
    lastFrameTime = frameTime;
	return output;
}

void G_DisplayRest(int32_t smoothratio)
{
    int32_t i, j;
    palaccum_t tint = PALACCUM_INITIALIZER;

    auto const pp = g_player[screenpeek].ps;
#ifdef SPLITSCREEN_MOD_HACKS
    auto const pp2 = g_fakeMultiMode==2 ? g_player[1].ps : NULL;
#endif
    int32_t cposx, cposy, cang;

#ifdef USE_OPENGL
    // this takes care of fullscreen tint for OpenGL
    if (videoGetRenderMode() >= REND_POLYMOST)
    {
        polytint_t & fstint = hictinting[MAXPALOOKUPS-1];

        if (pp->palette == WATERPAL)
        {
            fstint.tint.r = 224;
            fstint.tint.g = 192;
            fstint.tint.b = 255;
            fstint.f = 0;
        }
        else if (pp->palette == SLIMEPAL)
        {
            fstint.tint.r = 208;
            fstint.tint.g = 255;
            fstint.tint.b = 192;
            fstint.f = 0;
        }
        else
        {
            fstint.tint.r = 255;
            fstint.tint.g = 255;
            fstint.tint.b = 255;
            fstint.f = 0;
        }
    }
#endif  // USE_OPENGL
    palaccum_add(&tint, &pp->pals, pp->pals.f);
    {
        static const palette_t loogiepal = { 0, 63, 0, 0 };

        palaccum_add(&tint, &loogiepal, pp->loogcnt>>1);
    }

    if (g_restorePalette)
    {
        // reset a normal palette
        static uint32_t omovethingscnt;

        if (g_restorePalette < 2 || omovethingscnt+1 == g_moveThingsCount)
        {
            int32_t pal = pp->palette;

            // g_restorePalette < 0: reset tinting, too (e.g. when loading new game)
            P_SetGamePalette(pp, pal, (g_restorePalette > 0) ? Pal_DontResetFade : ESetPalFlags::FromInt(0));
            g_restorePalette = 0;
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
            if (pp->newowner >= 0)
                G_DrawCameraText(pp->newowner);
            else
            {
                PspTwoDSetter set;
                P_DisplayWeapon();
#ifdef SPLITSCREEN_MOD_HACKS
                if (pp2)  // HACK
                {
                    const int32_t oscreenpeek = screenpeek;
                    screenpeek = 1;
                    P_DisplayWeapon();
                    screenpeek = oscreenpeek;
                }
#endif

                if (pp->over_shoulder_on == 0)
                    P_DisplayScuba();
#ifdef SPLITSCREEN_MOD_HACKS
                if (pp2 && pp2->over_shoulder_on == 0)  // HACK
                {
                    const int32_t oscreenpeek = screenpeek;
                    screenpeek = 1;
                    P_DisplayScuba();
                    screenpeek = oscreenpeek;
                }
#endif
    }
            G_MoveClouds();
        }

        if (ud.overhead_on > 0)
        {
            // smoothratio = min(max(smoothratio,0),65536);
            smoothratio = calc_smoothratio(totalclock, ototalclock);
            G_DoInterpolations(smoothratio);

            if (ud.scrollmode == 0)
            {
                if (pp->newowner == -1 && !ud.pause_on)
                {
                    cposx = pp->opos.x + mulscale16(pp->pos.x-pp->opos.x, smoothratio);
                    cposy = pp->opos.y + mulscale16(pp->pos.y-pp->opos.y, smoothratio);
                    cang = fix16_to_int(pp->oq16ang) + mulscale16((fix16_to_int(pp->q16ang+F16(1024)-pp->oq16ang)&2047)-1024, smoothratio);
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
                G_DrawBackground(); // Necessary GL fills the entire screen with black
                renderDrawMapView(cposx, cposy, pp->zoom, cang);
            }
            G_DrawOverheadMap(cposx, cposy, pp->zoom, cang);

            G_RestoreInterpolations();

            int32_t const textret = VM_OnEvent(EVENT_DISPLAYOVERHEADMAPTEXT, g_player[screenpeek].ps->i, screenpeek);

            if (textret == 0 && ud.overhead_on == 2)
            {
                const int32_t a = (ud.screen_size > 0) ? 147 : 179;
                char const * levelname = currentLevel->DisplayName();

                if (!Menu_HaveUserMap() && !(G_GetLogoFlags() & LOGO_HIDEEPISODE))
                    minitext(5, a+6, GStrings.localize(gVolumeNames[ud.volume_number]), 0, 2+8+16+256);
                minitext(5, a+6+6, levelname, 0, 2+8+16+256);
            }
        }
    }

    if (pp->invdisptime > 0) G_DrawInventory(pp);

    if (VM_OnEvent(EVENT_DISPLAYSBAR, g_player[screenpeek].ps->i, screenpeek) == 0)
        G_DrawStatusBar(screenpeek);

#ifdef SPLITSCREEN_MOD_HACKS
    // HACK
    if (g_fakeMultiMode==2)
    {
        G_DrawStatusBar(1);
        G_PrintGameQuotes(1);
    }
#endif

    G_PrintGameQuotes(screenpeek);

    if (ud.show_level_text && hud_showmapname && g_levelTextTime > 1 && !M_Active())
    {
        int32_t o = 10|16;

        if (g_levelTextTime < 3)
            o |= 1|32;
        else if (g_levelTextTime < 5)
            o |= 1;
		
		menutext_(160<<16, (90+16+8)<<16, -g_levelTextTime+22/*quotepulseshade*/, currentLevel->DisplayName(), o, TEXT_XCENTER);
    }

    if (g_player[myconnectindex].ps->newowner == -1 && ud.overhead_on == 0 && cl_crosshair && ud.camerasprite == -1)
    {
        ud.returnvar[0] = (160<<16) - (fix16_to_int(g_player[myconnectindex].ps->q16look_ang)<<15);
        ud.returnvar[1] = 100<<16;
        int32_t a = VM_OnEventWithReturn(EVENT_DISPLAYCROSSHAIR, g_player[screenpeek].ps->i, screenpeek, CROSSHAIR);
        if ((unsigned) a < MAXTILES)
        {
            vec2_t crosshairpos = { ud.returnvar[0], ud.returnvar[1] };
            uint32_t crosshair_o = 1|2;
            uint32_t crosshair_scale = divscale16(cl_crosshairscale, 100);

            auto const oyxaspect = yxaspect;
            if (FURY)
            {
                crosshairpos.x = scale(crosshairpos.x - (320<<15), ydim << 2, xdim * 3) + (320<<15);
                crosshairpos.y = scale(crosshairpos.y - (200<<15), (ydim << 2) * 6, (xdim * 3) * 5) + (200<<15);
                crosshair_scale = scale(crosshair_scale, ydim << 2, xdim * 3) >> 1;
                crosshair_o |= 1024;
                renderSetAspect(viewingrange, 65536);
            }

            rotatesprite_win(crosshairpos.x, crosshairpos.y, crosshair_scale, 0, a, 0, 0, crosshair_o);

            if (FURY)
                renderSetAspect(viewingrange, oyxaspect);
        }
    }


    if (VM_HaveEvent(EVENT_DISPLAYREST))
    {
        int32_t vr=viewingrange, asp=yxaspect;
        VM_ExecuteEvent(EVENT_DISPLAYREST, g_player[screenpeek].ps->i, screenpeek);
        renderSetAspect(vr, asp);
    }

    if (ud.pause_on==1 && (g_player[myconnectindex].ps->gm&MODE_MENU) == 0)
        menutext_center(100, GStrings("Game Paused"));

    mdpause = (ud.pause_on || (ud.recstat==2 && (g_demo_paused && g_demo_goalCnt==0)) || (g_player[myconnectindex].ps->gm&MODE_MENU && numplayers < 2));

    // JBF 20040124: display level stats in screen corner
    if (ud.overhead_on != 2 && hud_stats && VM_OnEvent(EVENT_DISPLAYLEVELSTATS, g_player[screenpeek].ps->i, screenpeek) == 0)
    {
        auto const myps = g_player[myconnectindex].ps;

        i = 198<<16;

        if (ud.screen_size == 4)
        {
            if (ud.althud == 0 || hud_position == 0)
                i -= sbarsc(ud.althud ? (tilesiz[BIGALPHANUM].y+8)<<16 : tilesiz[INVENTORYBOX].y<<16);
        }
        else if (ud.screen_size > 2)
            i -= sbarsc(tilesiz[sbartile()].y<<16);

        int32_t const xbetween = (tilesiz[MF_Bluefont.tilenum + 'A' - '!'].x<<16) + MF_Bluefont.between.x;

        Bsprintf(tempbuf, "T:^15%d:%02d.%02d",
            (myps->player_par/(REALGAMETICSPERSEC*60)),
            (myps->player_par/REALGAMETICSPERSEC)%60,
            ((myps->player_par%REALGAMETICSPERSEC)*33)/10
            );
        G_ScreenText(MF_Bluefont.tilenum, 2<<16, i-gtextsc(21<<16), gtextsc(MF_Bluefont.zoom), 0, 0, tempbuf, 0, 10, 2|8|16|256|ROTATESPRITE_FULL16, 0, MF_Bluefont.emptychar.x, MF_Bluefont.emptychar.y, xbetween, MF_Bluefont.between.y, MF_Bluefont.textflags|TEXT_XOFFSETZERO|TEXT_GAMETEXTNUMHACK, 0, 0, xdim-1, ydim-1);

        if (ud.player_skill > 3 || ((g_netServer || ud.multimode > 1) && !GTFLAGS(GAMETYPE_PLAYERSFRIENDLY)))
            Bsprintf(tempbuf, "K:^15%d", (ud.multimode>1 &&!GTFLAGS(GAMETYPE_PLAYERSFRIENDLY)) ?
                myps->frag-myps->fraggedself : myps->actors_killed);
        else
        {
            if (myps->actors_killed >= myps->max_actors_killed)
                Bsprintf(tempbuf, "K:%d/%d", myps->actors_killed, myps->actors_killed);
            else
                Bsprintf(tempbuf, "K:^15%d/%d", myps->actors_killed, myps->max_actors_killed);
        }
        G_ScreenText(MF_Bluefont.tilenum, 2<<16, i-gtextsc(14<<16), gtextsc(MF_Bluefont.zoom), 0, 0, tempbuf, 0, 10, 2|8|16|256|ROTATESPRITE_FULL16, 0, MF_Bluefont.emptychar.x, MF_Bluefont.emptychar.y, xbetween, MF_Bluefont.between.y, MF_Bluefont.textflags|TEXT_XOFFSETZERO|TEXT_GAMETEXTNUMHACK, 0, 0, xdim-1, ydim-1);

        if (myps->secret_rooms == myps->max_secret_rooms)
            Bsprintf(tempbuf, "S:%d/%d", myps->secret_rooms, myps->max_secret_rooms);
        else Bsprintf(tempbuf, "S:^15%d/%d", myps->secret_rooms, myps->max_secret_rooms);
        G_ScreenText(MF_Bluefont.tilenum, 2<<16, i-gtextsc(7<<16), gtextsc(MF_Bluefont.zoom), 0, 0, tempbuf, 0, 10, 2|8|16|256|ROTATESPRITE_FULL16, 0, MF_Bluefont.emptychar.x, MF_Bluefont.emptychar.y, xbetween, MF_Bluefont.between.y, MF_Bluefont.textflags|TEXT_XOFFSETZERO|TEXT_GAMETEXTNUMHACK, 0, 0, xdim-1, ydim-1);
    }

    if (g_player[myconnectindex].gotvote == 0 && voting != -1 && voting != myconnectindex)
    {
        Bsprintf(tempbuf, "%s^00 has called a vote for map", g_player[voting].user_name);
        gametext_center(40, tempbuf);
        Bsprintf(tempbuf, "%s (E%dL%d)", mapList[vote_episode*MAXLEVELS + vote_map].DisplayName(), vote_episode+1, vote_map+1);
        gametext_center(48, tempbuf);
        gametext_center(70, "Press F1 to Accept, F2 to Decline");
    }

    if (buttonMap.ButtonDown(gamefunc_Show_DukeMatch_Scores))
        G_ShowScores();


#ifdef LUNATIC
    El_DisplayErrors();
#endif

#ifndef EDUKE32_TOUCH_DEVICES
    if (VOLUMEONE)
    {
        if (g_showShareware > 0 && (g_player[myconnectindex].ps->gm&MODE_MENU) == 0)
            rotatesprite_fs((320-50)<<16, 9<<16, 65536L, 0, BETAVERSION, 0, 0, 2+8+16+128);
    }
#endif

    if (!Demo_IsProfiling())
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

    VM_OnEvent(EVENT_DISPLAYEND, g_player[screenpeek].ps->i, screenpeek);
}

GameStats GameInterface::getStats()
{
	DukePlayer_t* p = g_player[myconnectindex].ps;
	return { p->actors_killed, p->max_actors_killed, p->secret_rooms, p->max_secret_rooms, p->player_par / REALGAMETICSPERSEC, p->frag };
}



void G_FadePalette(int32_t r, int32_t g, int32_t b, int32_t e)
{
    if (ud.screenfade == 0)
      return;
    videoFadePalette(r, g, b, e);
    videoNextPage();

    int32_t tc = (int32_t) totalclock;
    while (totalclock < tc + 4)
        gameHandleEvents();
}

// START and END limits are always inclusive!
// STEP must evenly divide END-START, i.e. abs(end-start)%step == 0
void fadepal(int32_t r, int32_t g, int32_t b, int32_t start, int32_t end, int32_t step)
{
    if (ud.screenfade == 0)
      return;

    if (videoGetRenderMode() >= REND_POLYMOST)
    {
        G_FadePalette(r, g, b, end);
        return;
    }

    // (end-start)/step + 1 iterations
    do
    {
        if (inputState.CheckAllInput())
        {
            videoFadePalette(r, g, b, end);  // have to set to end fade value if we break!
            return;
        }

        G_FadePalette(r, g, b, start);
        start += step;
    } while (start != end+step);
}

// START and END limits are always inclusive!
static void fadepaltile(int32_t r, int32_t g, int32_t b, int32_t start, int32_t end, int32_t step, int32_t tile)
{
    if (ud.screenfade == 0)
      return;

    // STEP must evenly divide END-START
    Bassert(klabs(end-start)%step == 0);

    videoClearScreen(0);

    // (end-start)/step + 1 iterations
    do
    {
        if (inputState.CheckAllInput())
        {
            videoFadePalette(r, g, b, end);  // have to set to end fade value if we break!
            return;
        }

        rotatesprite_fs(160<<16, 100<<16, 65536L, 0, tile, 0, 0, 2+8+64+BGSTRETCH);
        G_FadePalette(r, g, b, start);
        start += step;
    } while (start != end+step);
}

#ifdef LUNATIC
int32_t g_logoFlags = 255;
#endif

#ifdef __ANDROID__
int inExtraScreens = 0;
#endif

void gameDisplayTENScreen()
{
#ifdef __ANDROID__
    inExtraScreens = 1;
#endif
    videoSetViewableArea(0, 0, xdim - 1, ydim - 1);
    renderFlushPerms();
    // g_player[myconnectindex].ps->palette = palette;
    P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);  // JBF 20040308
    fadepal(0, 0, 0, 0, 252, 28);
    inputState.ClearAllInput();
    totalclock = 0;
    rotatesprite_fs(160 << 16, 100 << 16, 65536L, 0, TENSCREEN, 0, 0, 2 + 8 + 64 + BGSTRETCH);
    fadepaltile(0, 0, 0, 252, 0, -28, TENSCREEN);
    while (!inputState.CheckAllInput() && totalclock < 2400)
        gameHandleEvents();

    fadepaltile(0, 0, 0, 0, 252, 28, TENSCREEN);
    inputState.ClearAllInput();
#ifdef __ANDROID__
    inExtraScreens = 0;
#endif
}

void gameDisplaySharewareScreens()
{
#ifdef __ANDROID__
    inExtraScreens = 1;
#endif
    videoSetViewableArea(0, 0, xdim - 1, ydim - 1);
    renderFlushPerms();
    // g_player[myconnectindex].ps->palette = palette;
    P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);  // JBF 20040308
    fadepal(0, 0, 0, 0, 252, 28);
    inputState.ClearAllInput();
    rotatesprite_fs(160 << 16, 100 << 16, 65536L, 0, 3291, 0, 0, 2 + 8 + 64 + BGSTRETCH);
    fadepaltile(0, 0, 0, 252, 0, -28, 3291);
    while (!inputState.CheckAllInput())
        gameHandleEvents();

    fadepaltile(0, 0, 0, 0, 252, 28, 3291);
    inputState.ClearAllInput();
    rotatesprite_fs(160 << 16, 100 << 16, 65536L, 0, 3290, 0, 0, 2 + 8 + 64 + BGSTRETCH);
    fadepaltile(0, 0, 0, 252, 0, -28, 3290);
    while (!inputState.CheckAllInput())
        gameHandleEvents();

#ifdef __ANDROID__
    inExtraScreens = 0;
#endif
}

void G_DisplayExtraScreens(void)
{
    Mus_Stop();
    FX_StopAllSounds();

    if (!DUKEBETA && (!VOLUMEALL || G_GetLogoFlags() & LOGO_SHAREWARESCREENS))
        gameDisplaySharewareScreens();

    if (G_GetLogoFlags() & LOGO_TENSCREEN)
        gameDisplayTENScreen();
}

void gameDisplay3DRScreen()
{
    if (!inputState.CheckAllInput() && g_noLogoAnim == 0 && !userConfig.nologo)
    {
        Net_GetPackets();

		if (fileSystem.FileExists("3dr.ivf") || fileSystem.FileExists("3dr.anm"))
		{
			Anim_Play("3dr.anm");
            G_FadePalette(0, 0, 0, 252);
            inputState.ClearAllInput();
        }
        else
        {
            videoClearScreen(0);

            P_SetGamePalette(g_player[myconnectindex].ps, DREALMSPAL, Pal_Fullscreen);  // JBF 20040308
            fadepal(0, 0, 0, 0, 252, 28);
            renderFlushPerms();
            rotatesprite_fs(160 << 16, 100 << 16, 65536L, 0, DREALMS, 0, 0, 2 + 8 + 64 + BGSTRETCH);
            videoNextPage();
            fadepaltile(0, 0, 0, 252, 0, -28, DREALMS);
            totalclock = 0;

            while (totalclock < (120 * 7) && !inputState.CheckAllInput())
            {
                if (G_FPSLimit())
                {
                    videoClearScreen(0);
                    rotatesprite_fs(160 << 16, 100 << 16, 65536L, 0, DREALMS, 0, 0, 2 + 8 + 64 + BGSTRETCH);
                    gameHandleEvents();
                    videoNextPage();
                }
            }

            fadepaltile(0, 0, 0, 0, 252, 28, DREALMS);
        }
    }
}

void gameDisplayTitleScreen(void)
{
    int titlesound  = 0;
    int32_t const logoflags = G_GetLogoFlags();

    videoClearScreen(0);

    // g_player[myconnectindex].ps->palette = titlepal;
    P_SetGamePalette(g_player[myconnectindex].ps, TITLEPAL, Pal_2D);  // JBF 20040308
    renderFlushPerms();
    rotatesprite_fs(160 << 16, 100 << 16, 65536L, 0, BETASCREEN, 0, 0, 2 + 8 + 64 + BGSTRETCH);
    inputState.keyFlushChars();
    fadepaltile(0, 0, 0, 252, 0, -28, BETASCREEN);
    totalclock = 0;

    while (
#ifndef EDUKE32_SIMPLE_MENU
    totalclock < (860 + 120) &&
#endif
    !inputState.CheckAllInput())
    {
        if (G_FPSLimit())
        {
            videoClearScreen(0);
            rotatesprite_fs(160 << 16, 100 << 16, 65536L, 0, BETASCREEN, 0, 0, 2 + 8 + 64 + BGSTRETCH);
            if (logoflags & LOGO_DUKENUKEM)
            {
                if (totalclock > 120 && totalclock < (120 + 60))
                {
                    if (titlesound == 0)
                    {
                        titlesound++;
                        S_PlaySound(PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
                    }
                    rotatesprite_fs(160 << 16, 104 << 16, ((int32_t) totalclock - 120) << 10, 0, DUKENUKEM, 0, 0, 2 + 8);
                }
                else if (totalclock >= (120 + 60))
                    rotatesprite_fs(160 << 16, (104) << 16, 60 << 10, 0, DUKENUKEM, 0, 0, 2 + 8);
            }
            else
                titlesound++;

            if (logoflags & LOGO_THREEDEE)
            {
                if (totalclock > 220 && totalclock < (220 + 30))
                {
                    if (titlesound == 1)
                    {
                        titlesound++;
                        S_PlaySound(PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
                    }

                    rotatesprite_fs(160 << 16, (104) << 16, 60 << 10, 0, DUKENUKEM, 0, 0, 2 + 8);
                    rotatesprite_fs(160 << 16, (129) << 16, ((int32_t) totalclock - 220) << 11, 0, THREEDEE, 0, 0, 2 + 8);
                }
                else if (totalclock >= (220 + 30))
                    rotatesprite_fs(160 << 16, (129) << 16, 30 << 11, 0, THREEDEE, 0, 0, 2 + 8);
            }
            else
                titlesound++;

            if (PLUTOPAK && (logoflags & LOGO_PLUTOPAKSPRITE))
            {
                // JBF 20030804
                if (totalclock >= 280 && totalclock < 395)
                {
                    rotatesprite_fs(160 << 16, (151) << 16, (410 - (int32_t) totalclock) << 12, 0, PLUTOPAKSPRITE + 1,
                                    (sintable[((int32_t) totalclock << 4) & 2047] >> 11), 0, 2 + 8);
                    if (titlesound == 2)
                    {
                        titlesound++;
                        S_PlaySound(FLY_BY, CHAN_AUTO, CHANF_UI);
                    }
                }
                else if (totalclock >= 395)
                {
                    if (titlesound == 3)
                    {
                        titlesound++;
                        S_PlaySound(PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
                    }
                    rotatesprite_fs(160 << 16, (151) << 16, 30 << 11, 0, PLUTOPAKSPRITE + 1, (sintable[((int32_t) totalclock << 4) & 2047] >> 11), 0,
                                    2 + 8);
                }
            }

#ifdef LUNATIC
            g_elEventError = 0;
#endif
            VM_OnEvent(EVENT_LOGO, -1, screenpeek);

            videoNextPage();

#ifdef LUNATIC
            if (g_elEventError)
                break;
#endif
        }

        gameHandleEvents();
    }
}

void G_DisplayLogo(void)
{
    int32_t const logoflags = G_GetLogoFlags();

    ready2send = 0;

    inputState.ClearAllInput();

    videoSetViewableArea(0, 0, xdim-1, ydim-1);
    videoClearScreen(0L);
    G_FadePalette(0, 0, 0, 252);

    renderFlushPerms();
    videoNextPage();

    Mus_Stop();
    FX_StopAllSounds(); // JBF 20031228
    S_ClearSoundLocks();  // JBF 20031228

    if (!g_noLogo && !userConfig.nologo /* && (!g_netServer && ud.multimode < 2) */ &&
        VM_OnEventWithReturn(EVENT_MAINMENUSCREEN, g_player[myconnectindex].ps->i, myconnectindex, 0) == 0 &&
        (logoflags & LOGO_ENABLED))
    {
        if (
#ifndef EDUKE32_TOUCH_DEVICES
            VOLUMEALL &&
#endif
            (logoflags & LOGO_PLAYANIM))
        {
            if (!inputState.CheckAllInput() && g_noLogoAnim == 0)
            {
                Net_GetPackets();
                Anim_Play("logo.anm");
                G_FadePalette(0, 0, 0, 252);
                inputState.ClearAllInput();
            }

            videoClearScreen(0L);
            videoNextPage();

            if (logoflags & LOGO_STOPANIMSOUNDS)
            {
                FX_StopAllSounds();
                S_ClearSoundLocks();
            }
        }

        if (logoflags & LOGO_PLAYMUSIC)
            S_PlaySpecialMusicOrNothing(MUS_INTRO);

        if (!NAM)
        {
            //g_player[myconnectindex].ps->palette = drealms;
            //G_FadePalette(0,0,0,252);

            if (logoflags & LOGO_3DRSCREEN)
            {
                gameDisplay3DRScreen();

                videoClearScreen(0L);
                videoNextPage();
            }

            inputState.ClearAllInput();
        }

        videoClearScreen(0L);
        videoNextPage();

        if (logoflags & LOGO_TITLESCREEN)
            gameDisplayTitleScreen();

        inputState.ClearAllInput();
    }

    renderFlushPerms();
    videoClearScreen(0L);
    videoNextPage();

    P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);

    if ((G_GetLogoFlags() & LOGO_STOPMISCSOUNDS) == 0)
        S_PlaySound(NITEVISION_ONOFF, CHAN_AUTO, CHANF_UI);

    //G_FadePalette(0,0,0,0);
    videoClearScreen(0L);
}

#ifndef EDUKE32_STANDALONE
void G_DoOrderScreen(void)
{
    videoSetViewableArea(0, 0, xdim-1, ydim-1);
    P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);    // JBF 20040308

    for (int i=0; i<4; i++)
    {
        fadepal(0, 0, 0, 0, 252, 28);
        inputState.ClearAllInput();
        rotatesprite_fs(160<<16, 100<<16, 65536L, 0, ORDERING+i, 0, 0, 2+8+64+BGSTRETCH);
        fadepal(0, 0, 0, 252, 0, -28);
        while (!inputState.CheckAllInput())
            gameHandleEvents();
    }

    inputState.ClearAllInput();
}


static void G_BonusCutscenes(void)
{
    if (!(numplayers < 2 && ud.eog && ud.from_bonus == 0))
        return;

    switch (ud.volume_number)
    {
    case 0:
        if ((G_GetLogoFlags() & LOGO_NOE1BONUSSCENE) && (G_GetLogoFlags() & LOGO_NOE1ENDSCREEN))
            return;

        if (adult_lockout == 0 && !(G_GetLogoFlags() & LOGO_NOE1BONUSSCENE))
        {
            int bonuscnt=0;
            int const bossmove [] =
            {
                0, 120, VICTORY1+3, 86, 59,
                220, 260, VICTORY1+4, 86, 59,
                260, 290, VICTORY1+5, 86, 59,
                290, 320, VICTORY1+6, 86, 59,
                320, 350, VICTORY1+7, 86, 59,
                350, 380, VICTORY1+8, 86, 59,
                350, 380, VICTORY1+8, 86, 59 // duplicate row to alleviate overflow in the for loop below "boss"
            };

            P_SetGamePalette(g_player[myconnectindex].ps, ENDINGPAL, Pal_Fullscreen); // JBF 20040308
            videoClearScreen(0L);
            rotatesprite_fs(0, 50<<16, 65536L, 0, VICTORY1, 0, 0, 2+8+16+64+128+BGSTRETCH);
            videoNextPage();
            fadepal(0, 0, 0, 252, 0, -4);

            inputState.ClearAllInput();
            totalclock = 0;

            do
            {
                if (G_FPSLimit())
                {
                    videoClearScreen(0L);
                    rotatesprite_fs(0, 50<<16, 65536L, 0, VICTORY1, 0, 0, 2+8+16+64+128+BGSTRETCH);

                    // boss
                    if (totalclock > 390 && totalclock < 780)
                        for (bssize_t t=0; t<35; t+=5) if (bossmove[t+2] && (totalclock%390) > bossmove[t] && (totalclock%390) <= bossmove[t+1])
                        {
                            if (t==10 && bonuscnt == 1)
                            {
                                S_PlaySound(SHOTGUN_FIRE, CHAN_AUTO, CHANF_UI);
                                S_PlaySound(SQUISHED, CHAN_AUTO, CHANF_UI);
                                bonuscnt++;
                            }
                            rotatesprite_fs(bossmove[t+3]<<16, bossmove[t+4]<<16, 65536L, 0, bossmove[t+2], 0, 0, 2+8+16+64+128+BGSTRETCH);
                        }

                    // Breathe
                    if (totalclock < 450 || totalclock >= 750)
                    {
                        int32_t const breathe [] =
                        {
                            0,  30, VICTORY1+1, 176, 59,
                            30,  60, VICTORY1+2, 176, 59,
                            60,  90, VICTORY1+1, 176, 59,
                            90, 120,          0, 176, 59
                        };

                        if (totalclock >= 750)
                        {
                            rotatesprite_fs(86<<16, 59<<16, 65536L, 0, VICTORY1+8, 0, 0, 2+8+16+64+128+BGSTRETCH);
                            if (totalclock >= 750 && bonuscnt == 2)
                            {
                                S_PlaySound(DUKETALKTOBOSS, CHAN_AUTO, CHANF_UI);
                                bonuscnt++;
                            }

                        }
                        for (bssize_t t=0; t<20; t+=5)
                            if (breathe[t+2] && (totalclock%120) > breathe[t] && (totalclock%120) <= breathe[t+1])
                            {
                                if (t==5 && bonuscnt == 0)
                                {
                                    S_PlaySound(BOSSTALKTODUKE, CHAN_AUTO, CHANF_UI);
                                    bonuscnt++;
                                }
                                rotatesprite_fs(breathe[t+3]<<16, breathe[t+4]<<16, 65536L, 0, breathe[t+2], 0, 0, 2+8+16+64+128+BGSTRETCH);
                            }
                    }
                    videoNextPage();
                }

                gameHandleEvents();

                if (inputState.CheckAllInput()) break;
            } while (1);

            fadepal(0, 0, 0, 0, 252, 4);
        }

        if (G_GetLogoFlags() & LOGO_NOE1ENDSCREEN)
            goto VOL1_END;

        inputState.ClearAllInput();
        P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);   // JBF 20040308

        rotatesprite_fs(160<<16, 100<<16, 65536L, 0, 3292, 0, 0, 2+8+64+BGSTRETCH);
        fadepal(0, 0, 0, 252, 0, -4);
        G_HandleEventsWhileNoInput();
        fadepal(0, 0, 0, 0, 252, 4);

    VOL1_END:
        Mus_Stop();
        FX_StopAllSounds();
        S_ClearSoundLocks();
        break;

    case 1:
        if ((G_GetLogoFlags() & LOGO_NOE2BONUSSCENE) && (G_GetLogoFlags() & LOGO_NOE2ENDSCREEN))
            return;

        videoSetViewableArea(0, 0, xdim-1, ydim-1);

        Mus_Stop();
        videoClearScreen(0L);
        videoNextPage();

        if (adult_lockout == 0 && !(G_GetLogoFlags() & LOGO_NOE2BONUSSCENE))
        {
            fadepal(0, 0, 0, 252, 0, -4);
            Anim_Play("cineov2.anm");
            inputState.ClearAllInput();
            videoClearScreen(0L);
            videoNextPage();

            S_PlaySound(PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
            fadepal(0, 0, 0, 0, 252, 4);
        }

        if (G_GetLogoFlags() & LOGO_NOE2ENDSCREEN)
            return;

        inputState.ClearAllInput();
        P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);   // JBF 20040308
        rotatesprite_fs(160<<16, 100<<16, 65536L, 0, 3293, 0, 0, 2+8+64+BGSTRETCH);
        fadepal(0, 0, 0, 252, 0, -4);
        G_HandleEventsWhileNoInput();
        fadepal(0, 0, 0, 0, 252, 4);

        break;

    case 3:
        if ((G_GetLogoFlags() & LOGO_NOE4BONUSSCENE) && (G_GetLogoFlags() & LOGO_NODUKETEAMTEXT) && (G_GetLogoFlags() & LOGO_NODUKETEAMPIC))
            return;

        videoSetViewableArea(0, 0, xdim-1, ydim-1);

        Mus_Stop();
        videoClearScreen(0L);
        videoNextPage();

        if (adult_lockout == 0 && !(G_GetLogoFlags() & LOGO_NOE4BONUSSCENE))
        {
            fadepal(0, 0, 0, 252, 0, -4);

            inputState.ClearAllInput();
            int t = Anim_Play("vol4e1.anm");
            videoClearScreen(0L);
            videoNextPage();
            if (t)
                goto end_vol4e;

            t = Anim_Play("vol4e2.anm");
            videoClearScreen(0L);
            videoNextPage();
            if (t)
                goto end_vol4e;

            Anim_Play("vol4e3.anm");
            videoClearScreen(0L);
            videoNextPage();
        }

    end_vol4e:
        if ((G_GetLogoFlags() & LOGO_NODUKETEAMTEXT) && (G_GetLogoFlags() & LOGO_NODUKETEAMPIC))
            goto VOL4_END;

        FX_StopAllSounds();
        S_ClearSoundLocks();
        S_PlaySound(ENDSEQVOL3SND4, CHAN_AUTO, CHANF_UI);
        inputState.ClearAllInput();

        if (G_GetLogoFlags() & LOGO_NODUKETEAMTEXT)
            goto VOL4_DUKETEAM;

        G_FadePalette(0, 0, 0, 0);
        P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);   // JBF 20040308
                                                                         //        G_FadePalette(0,0,0,252);
        videoClearScreen(0L);
        menutext_center(60, GStrings("Thanks to all our"));
        menutext_center(60+16, GStrings("fans for giving"));
        menutext_center(60+16+16, GStrings("us big heads."));
        menutext_center(70+16+16+16, GStrings("Look for a Duke Nukem 3D"));
        menutext_center(70+16+16+16+16, GStrings("sequel soon."));
        videoNextPage();

        fadepal(0, 0, 0, 252, 0, -12);
        videoNextPage();
        inputState.ClearAllInput();
        G_HandleEventsWhileNoInput();
        fadepal(0, 0, 0, 0, 252, 12);

        if (G_GetLogoFlags() & LOGO_NODUKETEAMPIC)
            goto VOL4_END;

    VOL4_DUKETEAM:
        videoClearScreen(0L);
        videoNextPage();

        Anim_Play("DUKETEAM.ANM");

        inputState.ClearAllInput();
        G_HandleEventsWhileNoInput();

        videoClearScreen(0L);
        videoNextPage();
        G_FadePalette(0, 0, 0, 252);

    VOL4_END:
        FX_StopAllSounds();
        S_ClearSoundLocks();
        inputState.ClearAllInput();

        break;

    case 2:
        if ((G_GetLogoFlags() & LOGO_NOE3BONUSSCENE) && (G_GetLogoFlags() & LOGO_NOE3RADLOGO) && (PLUTOPAK || (G_GetLogoFlags() & LOGO_NODUKETEAMPIC)))
            return;

        Mus_Stop();
        videoClearScreen(0L);
        videoNextPage();
        if (adult_lockout == 0 && !(G_GetLogoFlags() & LOGO_NOE3BONUSSCENE))
        {
            fadepal(0, 0, 0, 252, 0, -4);
            Anim_Play("cineov3.anm");
            inputState.ClearAllInput();
            ototalclock = totalclock+200;
            while (totalclock < ototalclock)
                gameHandleEvents();
            videoClearScreen(0L);
            videoNextPage();

            FX_StopAllSounds();
            S_ClearSoundLocks();
        }

        if (G_GetLogoFlags() & LOGO_NOE3RADLOGO)
            goto ENDANM;

        Anim_Play("RADLOGO.ANM");

        if (adult_lockout == 0 && !inputState.CheckAllInput())
        {
            if (G_PlaySoundWhileNoInput(ENDSEQVOL3SND5)) goto ENDANM;
            if (G_PlaySoundWhileNoInput(ENDSEQVOL3SND6)) goto ENDANM;
            if (G_PlaySoundWhileNoInput(ENDSEQVOL3SND7)) goto ENDANM;
            if (G_PlaySoundWhileNoInput(ENDSEQVOL3SND8)) goto ENDANM;
            if (G_PlaySoundWhileNoInput(ENDSEQVOL3SND9)) goto ENDANM;
        }

        inputState.ClearAllInput();

        totalclock = 0;
        if (PLUTOPAK || (G_GetLogoFlags() & LOGO_NODUKETEAMPIC))
        {
            while (totalclock < 120 && !inputState.CheckAllInput())
                gameHandleEvents();

            inputState.ClearAllInput();
        }
        else
        {
            G_HandleEventsWhileNoInput();
        }

    ENDANM:
        if (!PLUTOPAK && !(G_GetLogoFlags() & LOGO_NODUKETEAMPIC))
        {
            FX_StopAllSounds();
            S_ClearSoundLocks();
            S_PlaySound(ENDSEQVOL3SND4, CHAN_AUTO, CHANF_UI);

            videoClearScreen(0L);
            videoNextPage();

            Anim_Play("DUKETEAM.ANM");

            inputState.ClearAllInput();
            G_HandleEventsWhileNoInput();

            videoClearScreen(0L);
            videoNextPage();
            G_FadePalette(0, 0, 0, 252);
        }

        inputState.ClearAllInput();
        FX_StopAllSounds();
        S_ClearSoundLocks();

        videoClearScreen(0L);

        break;

    case 4:
        if (!WORLDTOUR)
            return;

        if (adult_lockout == 0)
        {
            Mus_Stop();
            totalclocklock = totalclock = 0;

            videoClearScreen(0L);
            rotatesprite_fs(160<<16, 100<<16, 65536L, 0, FIREFLYGROWEFFECT, 0, 0, 2+8+64+BGSTRETCH);
            videoNextPage();

            fadepal(0, 0, 0, 252, 0, -4);

            inputState.ClearAllInput();

            S_PlaySound(E5L7_DUKE_QUIT_YOU);

            do
            {
                if (G_FPSLimit())
                {
                    totalclocklock = totalclock;

                    videoClearScreen(0L);
                    rotatesprite_fs(160<<16, 100<<16, 65536L, 0, FIREFLYGROWEFFECT, 0, 0, 2+8+64+BGSTRETCH);
                    videoNextPage();
                }

                gameHandleEvents();

                if (inputState.CheckAllInput()) break;
            } while (1);

            fadepal(0, 0, 0, 0, 252, 4);
        }

        Mus_Stop();
        FX_StopAllSounds();
        S_ClearSoundLocks();
        break;
    }
}
#endif

static void G_DisplayMPResultsScreen(void)
{
    int32_t i, y, t = 0;

    rotatesprite_fs(160<<16, 100<<16, 65536L, 0, MENUSCREEN, 16, 0, 2+8+64+BGSTRETCH);
    rotatesprite_fs(160<<16, 34<<16, 65536L, 0, INGAMEDUKETHREEDEE, 0, 0, 10);
    if (PLUTOPAK)   // JBF 20030804
        rotatesprite_fs((260)<<16, 36<<16, 65536L, 0, PLUTOPAKSPRITE+2, 0, 0, 2+8);
    gametext_center(58+2, GStrings("Multiplayer Totals"));
    gametext_center(58+10, currentLevel->DisplayName());

    gametext_center_shade(165, GStrings("Presskey"), quotepulseshade);

    minitext(38, 80, GStrings("Name"), 8, 2+8+16+128);
    minitext(269, 80, GStrings("Kills"), 8, 2+8+16+128);
    for (i=0; i<g_mostConcurrentPlayers; i++)
    {
        Bsprintf(tempbuf, "%-4d", i+1);
        minitext(92+(i*23), 80, tempbuf, 3, 2+8+16+128);
    }

    for (i=0; i<g_mostConcurrentPlayers; i++)
    {
        int32_t xfragtotal = 0;
        Bsprintf(tempbuf, "%d", i+1);

        minitext(30, 90+t, tempbuf, 0, 2+8+16+128);
        minitext(38, 90+t, g_player[i].user_name, g_player[i].ps->palookup, 2+8+16+128);

        for (y=0; y<g_mostConcurrentPlayers; y++)
        {
            if (i == y)
            {
                Bsprintf(tempbuf, "%-4d", g_player[y].ps->fraggedself);
                minitext(92+(y*23), 90+t, tempbuf, 2, 2+8+16+128);
                xfragtotal -= g_player[y].ps->fraggedself;
            }
            else
            {
                Bsprintf(tempbuf, "%-4d", g_player[i].frags[y]);
                minitext(92+(y*23), 90+t, tempbuf, 0, 2+8+16+128);
                xfragtotal += g_player[i].frags[y];
            }
        }

        Bsprintf(tempbuf, "%-4d", xfragtotal);
        minitext(101+(8*23), 90+t, tempbuf, 2, 2+8+16+128);

        t += 7;
    }

    for (y=0; y<g_mostConcurrentPlayers; y++)
    {
        int32_t yfragtotal = 0;
        for (i=0; i<g_mostConcurrentPlayers; i++)
        {
            if (i == y)
                yfragtotal += g_player[i].ps->fraggedself;
            else
                yfragtotal += g_player[i].frags[y];
        }
        Bsprintf(tempbuf, "%-4d", yfragtotal);
        minitext(92+(y*23), 96+(8*7), tempbuf, 2, 2+8+16+128);
    }

    minitext(45, 96+(8*7), GStrings("Deaths"), 8, 2+8+16+128);
}

static int32_t G_PrintTime_ClockPad(void)
{
    int32_t clockpad = 2;
    int32_t ii, ij;

    for (ii=g_player[myconnectindex].ps->player_par/(REALGAMETICSPERSEC*60), ij=1; ii>9; ii/=10, ij++) { }
    clockpad = max(clockpad, ij);
    if (!(ud.volume_number == 0 && ud.last_level-1 == 7 && boardfilename[0]))
    {
        for (ii=currentLevel->parTime/(60), ij=1; ii>9; ii/=10, ij++) { }
        clockpad = max(clockpad, ij);
        if (!NAM_WW2GI && currentLevel->designerTime)
        {
            for (ii= currentLevel->designerTime/(60), ij=1; ii>9; ii/=10, ij++) { }
            clockpad = max(clockpad, ij);
        }
    }
    if (ud.playerbest > 0) for (ii=ud.playerbest/(REALGAMETICSPERSEC*60), ij=1; ii>9; ii/=10, ij++) { }
    clockpad = max(clockpad, ij);

    return clockpad;
}

static const char* G_PrintTime2(int32_t time)
{
    Bsprintf(tempbuf, "%0*d:%02d", G_PrintTime_ClockPad(), time/(REALGAMETICSPERSEC*60), (time/REALGAMETICSPERSEC)%60);
    return tempbuf;
}
static const char* G_PrintTime3(int32_t time)
{
    Bsprintf(tempbuf, "%0*d:%02d.%02d", G_PrintTime_ClockPad(), time/(REALGAMETICSPERSEC*60), (time/REALGAMETICSPERSEC)%60, ((time%REALGAMETICSPERSEC)*33)/10);
    return tempbuf;
}

const char* G_PrintYourTime(void)
{
    return G_PrintTime3(g_player[myconnectindex].ps->player_par);
}
const char* G_PrintParTime(void)
{
    if (ud.last_level < 1)
        return "<invalid>";
    return G_PrintTime2(currentLevel->parTime*REALGAMETICSPERSEC);
}
const char* G_PrintDesignerTime(void)
{
    if (ud.last_level < 1)
        return "<invalid>";
    return G_PrintTime2(currentLevel->designerTime*REALGAMETICSPERSEC);
}
const char* G_PrintBestTime(void)
{
    return G_PrintTime3(ud.playerbest);
}

void G_BonusScreen(int32_t bonusonly)
{
    int32_t gfx_offset;
    int32_t bonuscnt;
    int32_t clockpad = 2;
    const char *lastmapname;

    if (g_networkMode == NET_DEDICATED_SERVER)
        return;

    if (ud.volume_number == 0 && ud.last_level == 8 && boardfilename[0])
    {
        lastmapname = Bstrrchr(boardfilename, '\\');
        if (!lastmapname) lastmapname = Bstrrchr(boardfilename, '/');
        if (!lastmapname) lastmapname = boardfilename;
    }
    else
    {
        lastmapname = currentLevel->name;
        if (!lastmapname || !*lastmapname) // this isn't right but it's better than no name at all
            lastmapname = currentLevel->fileName;
    }


    fadepal(0, 0, 0, 0, 252, 28);
    videoSetViewableArea(0, 0, xdim-1, ydim-1);
    videoClearScreen(0L);
    videoNextPage();
    renderFlushPerms();

    FX_StopAllSounds();
    S_ClearSoundLocks();
    FX_SetReverb(0L);
    inputState.SetBindsEnabled(1); // so you can use your screenshot bind on the score screens

#ifndef EDUKE32_STANDALONE
    if (!bonusonly)
        G_BonusCutscenes();
#endif

    P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);   // JBF 20040308
    G_FadePalette(0, 0, 0, 252);   // JBF 20031228
    inputState.keyFlushChars();
    totalclock = 0;
    bonuscnt = 0;

    Mus_Stop();
    FX_StopAllSounds();
    S_ClearSoundLocks();

    if (g_mostConcurrentPlayers > 1 && (g_gametypeFlags[ud.coop]&GAMETYPE_SCORESHEET))
    {
        videoClearScreen(0);
        G_DisplayMPResultsScreen();

        PlayBonusMusic();

        videoNextPage();
        inputState.ClearAllInput();
        fadepal(0, 0, 0, 252, 0, -28);
        totalclock = 0;

        while (totalclock < TICRATE*10)
        {
            gameHandleEvents();

            if (G_FPSLimit())
            {
                videoClearScreen(0);
                G_DisplayMPResultsScreen();
                videoNextPage();
            }

            if (inputState.CheckAllInput())
            {
                break;
            }
        }

        fadepal(0, 0, 0, 0, 252, 28);
    }

    if (bonusonly || (g_netServer || ud.multimode > 1)) return;

    gfx_offset = (ud.volume_number==1) ? 5 : 0;
    rotatesprite_fs(160<<16, 100<<16, 65536L, 0, BONUSSCREEN+gfx_offset, 0, 0, 2+8+64+128+BGSTRETCH);

    if (lastmapname)
        menutext_center(20-6, lastmapname);
    menutext_center(36-6, GStrings("Completed"));

    gametext_center_shade(192, GStrings("PRESSKEY"), quotepulseshade);

    PlayBonusMusic();

    videoNextPage();
    inputState.ClearAllInput();
    fadepal(0, 0, 0, 252, 0, -4);
    bonuscnt = 0;
    totalclock = 0;

    do
    {
        int32_t yy = 0, zz;

        gameHandleEvents();

        if (G_FPSLimit())
        {
            if (g_player[myconnectindex].ps->gm&MODE_EOL)
            {
                videoClearScreen(0);
                rotatesprite_fs(160<<16, 100<<16, 65536L, 0, BONUSSCREEN+gfx_offset, 0, 0, 2+8+64+128+BGSTRETCH);

                if (totalclock >= 1000000000 && totalclock < 1000000320)
                {
                    switch (((int32_t) totalclock>>4)%15)
                    {
                    case 0:
                        if (bonuscnt == 6)
                        {
                            bonuscnt++;
                            S_PlaySound(SHOTGUN_COCK, CHAN_AUTO, CHANF_UI);
                            switch (rand()&3)
                            {
                            case 0:
                                S_PlaySound(BONUS_SPEECH1, CHAN_AUTO, CHANF_UI);
                                break;
                            case 1:
                                S_PlaySound(BONUS_SPEECH2, CHAN_AUTO, CHANF_UI);
                                break;
                            case 2:
                                S_PlaySound(BONUS_SPEECH3, CHAN_AUTO, CHANF_UI);
                                break;
                            case 3:
                                S_PlaySound(BONUS_SPEECH4, CHAN_AUTO, CHANF_UI);
                                break;
                            }
                        }
                        fallthrough__;
                    case 1:
                    case 4:
                    case 5:
                        rotatesprite_fs(199<<16, 31<<16, 65536L, 0, BONUSSCREEN+3+gfx_offset, 0, 0, 2+8+16+64+128+BGSTRETCH);
                        break;
                    case 2:
                    case 3:
                        rotatesprite_fs(199<<16, 31<<16, 65536L, 0, BONUSSCREEN+4+gfx_offset, 0, 0, 2+8+16+64+128+BGSTRETCH);
                        break;
                    }
                }
                else if (totalclock > (10240+120L)) break;
                else
                {
                    switch (((int32_t) totalclock>>5)&3)
                    {
                    case 1:
                    case 3:
                        rotatesprite_fs(199<<16, 31<<16, 65536L, 0, BONUSSCREEN+1+gfx_offset, 0, 0, 2+8+16+64+128+BGSTRETCH);
                        break;
                    case 2:
                        rotatesprite_fs(199<<16, 31<<16, 65536L, 0, BONUSSCREEN+2+gfx_offset, 0, 0, 2+8+16+64+128+BGSTRETCH);
                        break;
                    }
                }

                if (lastmapname)
                    menutext_center(20-6, lastmapname);
                menutext_center(36-6, GStrings("Completed"));

                gametext_center_shade(192, GStrings("PRESSKEY"), quotepulseshade);

                if (totalclock > (60*3))
                {
                    yy = zz = 59;

                    gametext(10, yy+9, GStrings("TXT_YourTime"));

                    yy+=10;
                    if (!(ud.volume_number == 0 && ud.last_level-1 == 7 && boardfilename[0]))
                    {
                        if (currentLevel->parTime)
                        {
                            gametext(10, yy+9, GStrings("TXT_ParTime"));
                            yy+=10;
                        }
                        if (!NAM_WW2GI && !DUKEBETA && currentLevel->designerTime)
                        {
                            // EDuke 2.0 / NAM source suggests "Green Beret's Time:"
                            gametext(10, yy+9, GStrings("TXT_3DRTIME"));
                            yy+=10;
                        }

                    }
                    if (ud.playerbest > 0)
                    {
                        gametext(10, yy+9, (g_player[myconnectindex].ps->player_par > 0 && g_player[myconnectindex].ps->player_par < ud.playerbest) ? GStrings("TXT_PREVBEST") : GStrings("TXT_YourBest"));
                        yy += 10;
                    }

                    if (bonuscnt == 0)
                        bonuscnt++;

                    yy = zz;
                    if (totalclock >(60*4))
                    {
                        if (bonuscnt == 1)
                        {
                            bonuscnt++;
                            S_PlaySound(PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
                        }

                        if (g_player[myconnectindex].ps->player_par > 0)
                        {
                            G_PrintYourTime();
                            gametext_number((320>>2)+71, yy+9, tempbuf);
                            if (g_player[myconnectindex].ps->player_par < ud.playerbest)
                                gametext((320>>2)+89+(clockpad*24), yy+9, GStrings("TXT_NEWRECORD"));
                        }
                        else
                            gametext_pal((320>>2)+71, yy+9, GStrings("TXT_Cheated"), 2);
                        yy+=10;

                        if (!(ud.volume_number == 0 && ud.last_level-1 == 7 && boardfilename[0]))
                        {
                            if (currentLevel->parTime)
                            {
                                G_PrintParTime();
                                gametext_number((320>>2)+71, yy+9, tempbuf);
                                yy+=10;
                            }
                            if (!NAM_WW2GI && !DUKEBETA && currentLevel->designerTime)
                            {
                                G_PrintDesignerTime();
                                gametext_number((320>>2)+71, yy+9, tempbuf);
                                yy+=10;
                            }
                        }

                        if (ud.playerbest > 0)
                        {
                            G_PrintBestTime();
                            gametext_number((320>>2)+71, yy+9, tempbuf);
                            yy+=10;
                        }
                    }
                }

                zz = yy += 5;
                if (totalclock > (60*6))
                {
                    gametext(10, yy+9, GStrings("TXT_EnemiesKilled"));
                    yy += 10;
                    gametext(10, yy+9, GStrings("TXT_EnemiesLeft"));
                    yy += 10;

                    if (bonuscnt == 2)
                    {
                        bonuscnt++;
                        S_PlaySound(FLY_BY, CHAN_AUTO, CHANF_UI);
                    }

                    yy = zz;

                    if (totalclock > (60*7))
                    {
                        if (bonuscnt == 3)
                        {
                            bonuscnt++;
                            S_PlaySound(PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
                        }
                        Bsprintf(tempbuf, "%-3d", g_player[myconnectindex].ps->actors_killed);
                        gametext_number((320>>2)+70, yy+9, tempbuf);
                        yy += 10;
                        if (ud.player_skill > 3)
                        {
                            gametext((320>>2)+70, yy+9, GStrings("TXT_N_A"));
                            yy += 10;
                        }
                        else
                        {
                            if ((g_player[myconnectindex].ps->max_actors_killed-g_player[myconnectindex].ps->actors_killed) < 0)
                                Bsprintf(tempbuf, "%-3d", 0);
                            else Bsprintf(tempbuf, "%-3d", g_player[myconnectindex].ps->max_actors_killed-g_player[myconnectindex].ps->actors_killed);
                            gametext_number((320>>2)+70, yy+9, tempbuf);
                            yy += 10;
                        }
                    }
                }

                zz = yy += 5;
                if (totalclock > (60*9))
                {
                    gametext(10, yy+9, GStrings("TXT_SECFND"));
                    yy += 10;
                    gametext(10, yy+9, GStrings("TXT_SECMISS"));
                    yy += 10;
                    if (bonuscnt == 4) bonuscnt++;

                    yy = zz;
                    if (totalclock > (60*10))
                    {
                        if (bonuscnt == 5)
                        {
                            bonuscnt++;
                            S_PlaySound(PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
                        }
                        Bsprintf(tempbuf, "%-3d", g_player[myconnectindex].ps->secret_rooms);
                        gametext_number((320>>2)+70, yy+9, tempbuf);
                        yy += 10;
#if 0
                        // Always overwritten.
                        if (g_player[myconnectindex].ps->secret_rooms > 0)
                            Bsprintf(tempbuf, "%-3d%%", (100*g_player[myconnectindex].ps->secret_rooms/g_player[myconnectindex].ps->max_secret_rooms));
#endif
                        Bsprintf(tempbuf, "%-3d", g_player[myconnectindex].ps->max_secret_rooms-g_player[myconnectindex].ps->secret_rooms);
                        gametext_number((320>>2)+70, yy+9, tempbuf);
                        yy += 10;
                        }
                    }

                if (totalclock > 10240 && totalclock < 10240+10240)
                    totalclock = 1024;

                if (inputState.CheckAllInput() && totalclock >(60*2)) // JBF 20030809
                {
                    if (totalclock < (60*13))
                    {
                        inputState.keyFlushChars();
                        totalclock = (60*13);
                    }
                    else if (totalclock < 1000000000)
                        totalclock = 1000000000;
                }
            }
            else
                break;

            VM_OnEvent(EVENT_DISPLAYBONUSSCREEN, g_player[screenpeek].ps->i, screenpeek);
            videoNextPage();
        }
    } while (1);
}

END_DUKE_NS
