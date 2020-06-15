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

#include "anim.h"
#include "sbar.h"
#include "menus.h"
#include "demo.h"
#include "mdsprite.h"
#include "gamecvars.h"
#include "menu.h"
#include "mapinfo.h"
#include "v_2ddrawer.h"

BEGIN_RR_NS

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
static void G_HandleEventsWhileNoInput(void)
{
    inputState.ClearAllInput();

    while (!inputState.CheckAllInput())
        G_HandleAsync();

}

static int32_t G_PlaySoundWhileNoInput(int32_t soundnum)
{
    S_PlaySound(soundnum, CHAN_AUTO, CHANF_UI);
    inputState.ClearAllInput();
    while (S_CheckSoundPlaying(-1, soundnum))
    {
        G_HandleAsync();
        if (inputState.CheckAllInput())
        {
            return 1;
        }
    }

    return 0;
}
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

            //Bsprintf(tempbuf, "%-4d", g_player[i].ping);
            //minitext(235, SCORESHEETOFFSET+90+t, tempbuf, 2, 2+8+16+ROTATESPRITE_MAX);

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
    //if (VM_OnEvent(EVENT_DISPLAYCAMERAOSD, i, screenpeek) != 0)
    //    return;

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
    uwalltype *wal, *wal2;
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

        for (j=startwall, wal=(uwalltype *)&wall[startwall]; j<endwall; j++, wal++)
        {
            k = wal->nextwall;
            if (k < 0) continue;

            if (sector[wal->nextsector].ceilingz == z1 && sector[wal->nextsector].floorz == z2)
                    if (((wal->cstat|wall[wal->nextwall].cstat)&(16+32)) == 0) continue;

            if (!gFullMap && !show2dsector[wal->nextsector])
            {

                col = PalEntry(170, 170, 170);
                ox = wal->x-cposx;
                oy = wal->y-cposy;
                x1 = dmulscale16(ox, xvect, -oy, yvect)+(xdim<<11);
                y1 = dmulscale16(oy, xvect2, ox, yvect2)+(ydim<<11);

                wal2 = (uwalltype *)&wall[wal->point2];
                ox = wal2->x-cposx;
                oy = wal2->y-cposy;
                x2 = dmulscale16(ox, xvect, -oy, yvect)+(xdim<<11);
                y2 = dmulscale16(oy, xvect2, ox, yvect2)+(ydim<<11);

                drawlinergb(x1, y1, x2, y2, col);
            }
        }
    }

    //Draw sprites
    k = g_player[screenpeek].ps->i;
    /*if (!FURY)*/ for (i=numsectors-1; i>=0; i--) // todo - make a switchable flag.
    {
        if (!gFullMap || !show2dsector[i]) continue;
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
                    xoff = tileLeftOffset(tilenum) + spr->xoffset;
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
                xoff = tileLeftOffset(tilenum) + spr->xoffset;
                yoff = tileTopOffset(tilenum) + spr->yoffset;
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
        for (j=startwall, wal=(uwalltype *)&wall[startwall]; j<endwall; j++, wal++)
        {
            if (wal->nextwall >= 0) continue;

            if (!tileGetTexture(wal->picnum)->isValid()) continue;

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
            wal2 = (uwalltype *)&wall[k];
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

        DukePlayer_t const * const pPlayer = g_player[p].ps;
        uspritetype const * const pSprite = (uspritetype const *)&sprite[pPlayer->i];

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

            //i = VM_OnEventWithReturn(EVENT_DISPLAYOVERHEADMAPPLAYER, pPlayer->i, p, i);

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
    out.AppendFormat("VEL= (%d, %d, %d) + (%d, %d, 0)\n", ps->vel.x >> 14, ps->vel.y >> 14, ps->vel.z, ps->fric.x >> 5, ps->fric.y >> 5);
    out.AppendFormat("OG= %d  SBRIDGE=%d SBS=%d\n", ps->on_ground, ps->spritebridge, ps->sbs);
    if (sectnum >= 0)
        out.AppendFormat("SECT= %d (LO=%d EX=%d)\n", sectnum, TrackerCast(sector[sectnum].lotag), TrackerCast(sector[sectnum].extra));
    else
        out.AppendFormat("SECT= %d\n", sectnum);

    out.AppendFormat("\nTHOLD= %d ", ps->transporter_hold);
    out.AppendFormat("GAMETIC= %u, TOTALCLOCK=%d\n", g_moveThingsCount, (int32_t)totalclock);
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
    out.AppendFormat("\nVR=%.03f  YX=%.03f", (double)dr_viewingrange / 65536.0, (double)dr_yxaspect / 65536.0);
    return out;
}

FString GameInterface::GetCoordString()
{
    return G_PrintCoords(screenpeek);
}


FString GameInterface::statFPS()
{
	FString output;
    static int32_t frameCount;
    static double cumulativeFrameDelay;
    static double lastFrameTime;
    static float lastFPS, minFPS = FLT_MAX, maxFPS;
    static double minGameUpdate = DBL_MAX, maxGameUpdate;

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

            // lag meter
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

GameStats GameInterface::getStats()
{
	DukePlayer_t* p = g_player[myconnectindex].ps;
	return { p->actors_killed, p->max_actors_killed, p->secret_rooms, p->max_secret_rooms, p->player_par / REALGAMETICSPERSEC, p->frag };
}


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
            if (!RR && pp->newowner >= 0)
                G_DrawCameraText(pp->newowner);
            else
            {
                PspTwoDSetter set;
                P_DisplayWeapon();

                if (pp->over_shoulder_on == 0)
                    P_DisplayScuba();
    }
            if (!RR)
                G_MoveClouds();
        }

        if (DEER)
            sub_57B38(pp->opos.x, pp->opos.y, 20, 1536);

        if (ud.overhead_on > 0)
        {
            // smoothratio = min(max(smoothratio,0),65536);
            smoothratio = calc_smoothratio(totalclock, ototalclock);
            G_DoInterpolations(smoothratio);

            if (ud.scrollmode == 0)
            {
                if (pp->newowner == -1 && !paused)
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
                if (!paused)
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
                G_DrawBackground();
                renderDrawMapView(cposx, cposy, pp->zoom, cang);
            }
            G_DrawOverheadMap(cposx, cposy, pp->zoom, cang);

            G_RestoreInterpolations();

            //int32_t const textret = VM_OnEvent(EVENT_DISPLAYOVERHEADMAPTEXT, g_player[screenpeek].ps->i, screenpeek);
            if (/*textret == 0 &&*/ ud.overhead_on == 2)
            {
                const int32_t a = RR ? 0 : ((ud.screen_size > 0) ? 147 : 179);

                if (!G_HaveUserMap()) // && !(G_GetLogoFlags() & LOGO_HIDEEPISODE))
                    minitext(5, a+6, GStrings.localize(gVolumeNames[ud.volume_number]), 0, 2+8+16+256);
                minitext(5, a+6+6, currentLevel->DisplayName(), 0, 2+8+16+256);
            }
        }
    }

    if (pp->invdisptime > 0) G_DrawInventory(pp);

    //if (VM_OnEvent(EVENT_DISPLAYSBAR, g_player[screenpeek].ps->i, screenpeek) == 0)
        G_DrawStatusBar(screenpeek);

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

    if (!DEER && g_player[myconnectindex].ps->newowner == -1 && ud.overhead_on == 0 && cl_crosshair && ud.camerasprite == -1)
    {
        int32_t a = CROSSHAIR;
        //ud.returnvar[0] = (160<<16) - (fix16_to_int(g_player[myconnectindex].ps->q16look_ang)<<15);
        //ud.returnvar[1] = 100<<16;
        //int32_t a = VM_OnEventWithReturn(EVENT_DISPLAYCROSSHAIR, g_player[screenpeek].ps->i, screenpeek, CROSSHAIR);
        if ((unsigned) a < MAXTILES)
        {
            vec2_t crosshairpos = { (160<<16) - (fix16_to_int(g_player[myconnectindex].ps->q16look_ang)<<15), 100<<16 };
            //vec2_t crosshairpos = { ud.returnvar[0], ud.returnvar[1] };
            uint32_t crosshair_o = 1|2;
            uint32_t crosshair_scale = divscale16(cl_crosshairscale, 100);
            if (RR)
                crosshair_scale >>= 1;

            rotatesprite_win(crosshairpos.x, crosshairpos.y, crosshair_scale, 0, a, 0, 0, crosshair_o);
        }
    }

	/*
    if (VM_HaveEvent(EVENT_DISPLAYREST))
    {
        int32_t vr=viewingrange, asp=yxaspect;
        VM_ExecuteEvent(EVENT_DISPLAYREST, g_player[screenpeek].ps->i, screenpeek);
        renderSetAspect(vr, asp);
    }
	*/

    if (paused==1 && (g_player[myconnectindex].ps->gm&MODE_MENU) == 0)
        menutext_center(100, GStrings("Game Paused"));

    mdpause = (paused || (ud.recstat==2 && (g_demo_paused && g_demo_goalCnt==0)) || (g_player[myconnectindex].ps->gm&MODE_MENU && numplayers < 2));

    // JBF 20040124: display level stats in screen corner
    if (ud.overhead_on != 2 && hud_stats) // && VM_OnEvent(EVENT_DISPLAYLEVELSTATS, g_player[screenpeek].ps->i, screenpeek) == 0)
    {
        DukePlayer_t const * const myps = g_player[myconnectindex].ps;
        int const sbarshift = RR ? 15 : 16;
        int const ystep = RR ? (10<<16) : (7<<16);

        i = 198<<16;

        if (ud.screen_size == 4)
        {
            if (ud.althud == 0 || hud_position == 0)
                i -= sbarsc(ud.althud ? ((tilesiz[BIGALPHANUM].y<<sbarshift)+(8<<16)) : tilesiz[INVENTORYBOX].y<<sbarshift);
        }
        else if (RR && ud.screen_size == 12)
        {
            i -= sbarsc((tilesiz[BOTTOMSTATUSBAR].y+tilesiz[WEAPONBAR].y)<<sbarshift);
        }
        else if (ud.screen_size > 2)
            i -= sbarsc(tilesiz[BOTTOMSTATUSBAR].y<<sbarshift);

        int32_t const xbetween = (tilesiz[MF_Bluefont.tilenum + 'A' - '!'].x<<16) + MF_Bluefont.between.x;

        Bsprintf(tempbuf, "T:^15%d:%02d.%02d",
            (myps->player_par/(REALGAMETICSPERSEC*60)),
            (myps->player_par/REALGAMETICSPERSEC)%60,
            ((myps->player_par%REALGAMETICSPERSEC)*33)/10
            );
        G_ScreenText(MF_Bluefont.tilenum, 2<<16, i-gtextsc(ystep*3), gtextsc(MF_Bluefont.zoom), 0, 0, tempbuf, 0, 10, 2|8|16|256|ROTATESPRITE_FULL16, 0, MF_Bluefont.emptychar.x, MF_Bluefont.emptychar.y, xbetween, MF_Bluefont.between.y, MF_Bluefont.textflags|TEXT_XOFFSETZERO|TEXT_GAMETEXTNUMHACK, 0, 0, xdim-1, ydim-1);

        if ((!RR && ud.player_skill > 3) || ((g_netServer || ud.multimode > 1) && !GTFLAGS(GAMETYPE_PLAYERSFRIENDLY)))
            Bsprintf(tempbuf, "K:^15%d", (ud.multimode>1 &&!GTFLAGS(GAMETYPE_PLAYERSFRIENDLY)) ?
                myps->frag-myps->fraggedself : myps->actors_killed);
        else
        {
            if (myps->actors_killed >= myps->max_actors_killed)
                Bsprintf(tempbuf, "K:%d/%d", myps->actors_killed, myps->actors_killed);
            else
                Bsprintf(tempbuf, "K:^15%d/%d", myps->actors_killed, myps->max_actors_killed);
        }
        G_ScreenText(MF_Bluefont.tilenum, 2<<16, i-gtextsc(ystep*2), gtextsc(MF_Bluefont.zoom), 0, 0, tempbuf, 0, 10, 2|8|16|256|ROTATESPRITE_FULL16, 0, MF_Bluefont.emptychar.x, MF_Bluefont.emptychar.y, xbetween, MF_Bluefont.between.y, MF_Bluefont.textflags|TEXT_XOFFSETZERO|TEXT_GAMETEXTNUMHACK, 0, 0, xdim-1, ydim-1);

        if (myps->secret_rooms == myps->max_secret_rooms)
            Bsprintf(tempbuf, "S:%d/%d", myps->secret_rooms, myps->max_secret_rooms);
        else Bsprintf(tempbuf, "S:^15%d/%d", myps->secret_rooms, myps->max_secret_rooms);
        G_ScreenText(MF_Bluefont.tilenum, 2<<16, i-gtextsc(ystep), gtextsc(MF_Bluefont.zoom), 0, 0, tempbuf, 0, 10, 2|8|16|256|ROTATESPRITE_FULL16, 0, MF_Bluefont.emptychar.x, MF_Bluefont.emptychar.y, xbetween, MF_Bluefont.between.y, MF_Bluefont.textflags|TEXT_XOFFSETZERO|TEXT_GAMETEXTNUMHACK, 0, 0, xdim-1, ydim-1);
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
	
    Net_DisplaySyncMsg();

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
}

void G_FadePalette(int32_t r, int32_t g, int32_t b, int32_t e)
{
    if (ud.screenfade == 0)
      return;
    videoFadePalette(r, g, b, e);
    videoNextPage();

    int32_t tc = (int32_t) totalclock;
    while (totalclock < tc + 4)
        G_HandleAsync();
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
        if (inputState.GetKeyStatus(sc_Space))
        {
            inputState.ClearKeyStatus(sc_Space);
            videoFadePalette(r, g, b, end);  // have to set to end fade value if we break!
            return;
        }

        G_FadePalette(r, g, b, start);

        start += step;
    } while (start != end+step);
}

// START and END limits are always inclusive!
static void fadepaltile(int32_t r, int32_t g, int32_t b, int32_t start, int32_t end, int32_t step, int32_t tile, int basepal)
{
    if (ud.screenfade == 0)
      return;
    // STEP must evenly divide END-START
    Bassert(klabs(end-start)%step == 0);

    twod->ClearScreen();

    // (end-start)/step + 1 iterations
    do
    {
        if (inputState.GetKeyStatus(sc_Space))
        {
            inputState.ClearKeyStatus(sc_Space);
            videoFadePalette(r, g, b, end);  // have to set to end fade value if we break!
            return;
        }
        rotatesprite_fs(160<<16, 100<<16, 65536L, 0, tile, 0, 0, 2+8+64+BGSTRETCH, nullptr, basepal);
        G_FadePalette(r, g, b, start);

        start += step;
    } while (start != end+step);
}

void G_DisplayExtraScreens(void)
{
    Mus_Stop();
    FX_StopAllSounds();
    if (RR)
        return;

    if (!VOLUMEALL)
    {
        videoSetViewableArea(0, 0, xdim-1, ydim-1);
        renderFlushPerms();
        //g_player[myconnectindex].ps->palette = palette;
        P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);    // JBF 20040308
        fadepal(0, 0, 0, 0, 252, 28);
        inputState.ClearAllInput();
        rotatesprite_fs(160<<16, 100<<16, 65536L, 0, 3291, 0, 0, 2+8+64+BGSTRETCH);
        fadepaltile(0, 0, 0, 252, 0, -28, 3291, BASEPAL);
        while (!inputState.CheckAllInput())
            G_HandleAsync();

        fadepaltile(0, 0, 0, 0, 252, 28, 3291, BASEPAL);
        rotatesprite_fs(160<<16, 100<<16, 65536L, 0, 3290, 0, 0, 2+8+64+BGSTRETCH);
        fadepaltile(0, 0, 0, 252, 0, -28, 3290, BASEPAL);
        while (!inputState.CheckAllInput())
            G_HandleAsync();

    }

    if (0)
    {
        videoSetViewableArea(0, 0, xdim-1, ydim-1);
        renderFlushPerms();
        //g_player[myconnectindex].ps->palette = palette;
        P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);    // JBF 20040308
        fadepal(0, 0, 0, 0, 252, 28);
        inputState.ClearAllInput();
        totalclock = 0;
        rotatesprite_fs(160<<16, 100<<16, 65536L, 0, TENSCREEN, 0, 0, 2+8+64+BGSTRETCH);
        fadepaltile(0, 0, 0, 252, 0, -28, TENSCREEN, BASEPAL);
        while (!inputState.CheckAllInput() && totalclock < 2400)
            G_HandleAsync();

        fadepaltile(0, 0, 0, 0, 252, 28, TENSCREEN, BASEPAL);
        inputState.ClearAllInput();
    }
}

void G_DisplayLogo(void)
{
    int32_t soundanm = 0;
    //int32_t logoflags = G_GetLogoFlags();

    ready2send = 0;

    inputState.ClearAllInput();

    videoSetViewableArea(0, 0, xdim-1, ydim-1);
    twod->ClearScreen();
    G_FadePalette(0, 0, 0, 252);

    renderFlushPerms();
    videoNextPage();

    Mus_Stop();
    FX_StopAllSounds(); // JBF 20031228
    if (DEER)
    {
        if (!g_noLogo /* && (!g_netServer && ud.multimode < 2) */)
        {
            if (!inputState.CheckAllInput() && g_noLogoAnim == 0)
            {
                twod->ClearScreen();

                P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);    // JBF 20040308
                fadepal(0, 0, 0, 0, 252, 28);
                renderFlushPerms();
                rotatesprite_fs(160 << 16, 100 << 16, 65536L, 0, 7106, 0, 0, 2 + 8 + 64 + BGSTRETCH);
                videoNextPage();
                fadepaltile(0, 0, 0, 252, 0, -4, 7106, BASEPAL);
                totalclock = 0;

                while (totalclock < (120 * 3) && !inputState.CheckAllInput())
                {
                    if (G_FPSLimit())
                    {
                        twod->ClearScreen();
                        rotatesprite_fs(160 << 16, 100 << 16, 65536L, 0, 7106, 0, 0, 2 + 8 + 64 + BGSTRETCH);
                        G_HandleAsync();

                        if (g_restorePalette)
                        {
                            P_SetGamePalette(g_player[myconnectindex].ps, g_player[myconnectindex].ps->palette, 0);
                            g_restorePalette = 0;
                        }
                        videoNextPage();
                    }
                }

                fadepaltile(0, 0, 0, 0, 252, 4, 7106, BASEPAL);
            }

            twod->ClearScreen();
            videoNextPage();

            inputState.ClearAllInput();

            twod->ClearScreen();
            videoNextPage();

            twod->ClearScreen();

            //g_player[myconnectindex].ps->palette = drealms;
            //G_FadePalette(0,0,0,252);

            if (!inputState.CheckAllInput() && g_noLogoAnim == 0)
            {
                twod->ClearScreen();

                P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);    // JBF 20040308
                fadepal(0, 0, 0, 0, 252, 28);
                renderFlushPerms();
                rotatesprite_fs(160 << 16, 100 << 16, 65536L, 0, 7107, 0, 0, 2 + 8 + 64 + BGSTRETCH);
                videoNextPage();
                fadepaltile(0, 0, 0, 252, 0, -4, 7107, BASEPAL);
                totalclock = 0;

                while (totalclock < (120 * 3) && !inputState.CheckAllInput())
                {
                    if (G_FPSLimit())
                    {
                        twod->ClearScreen();
                        rotatesprite_fs(160 << 16, 100 << 16, 65536L, 0, 7107, 0, 0, 2 + 8 + 64 + BGSTRETCH);
                        G_HandleAsync();

                        if (g_restorePalette)
                        {
                            P_SetGamePalette(g_player[myconnectindex].ps, g_player[myconnectindex].ps->palette, 0);
                            g_restorePalette = 0;
                        }
                        videoNextPage();
                    }
                }

                fadepaltile(0, 0, 0, 0, 252, 4, 7107, BASEPAL);
            }

            inputState.ClearAllInput();
        }

        renderFlushPerms();
        twod->ClearScreen();
        videoNextPage();

        //g_player[myconnectindex].ps->palette = palette;
        P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);    // JBF 20040308

        //G_FadePalette(0,0,0,0);
        twod->ClearScreen();

        g_noLogo = 1; // Play intro only once
        return;
    }
    if (RRRA)
        return;
    if (RR)
    {
        if (!inputState.CheckAllInput() && g_noLogoAnim == 0 && !userConfig.nologo)
        {
            Net_GetPackets();
            Anim_Play("rr_intro.anm");
            G_FadePalette(0, 0, 0, 252);
            inputState.ClearAllInput();
        }

        twod->ClearScreen();
        videoNextPage();
        FX_StopAllSounds();
        S_ClearSoundLocks();

        if (!inputState.CheckAllInput() && g_noLogoAnim == 0 && !userConfig.nologo)
        {
            Net_GetPackets();
            Anim_Play("redneck.anm");
            G_FadePalette(0, 0, 0, 252);
            inputState.ClearAllInput();
        }

        twod->ClearScreen();
        videoNextPage();
        FX_StopAllSounds();
        S_ClearSoundLocks();

        if (!inputState.CheckAllInput() && g_noLogoAnim == 0 && !userConfig.nologo)
        {
            Net_GetPackets();
            Anim_Play("xatlogo.anm");
            G_FadePalette(0, 0, 0, 252);
            inputState.ClearAllInput();
        }

        twod->ClearScreen();
        videoNextPage();
        FX_StopAllSounds();
        S_ClearSoundLocks();

        //g_player[myconnectindex].ps->palette = palette;
        P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);    // JBF 20040308
        S_PlaySound(NITEVISION_ONOFF, CHAN_AUTO, CHANF_UI);

        //G_FadePalette(0,0,0,0);
        twod->ClearScreen();
        return;
    }
    if (!g_noLogo && !userConfig.nologo /* && (!g_netServer && ud.multimode < 2) */)
    {
        if (VOLUMEALL)
        {
            if (!inputState.CheckAllInput() && g_noLogoAnim == 0 && !userConfig.nologo)
            {
                Net_GetPackets();
                Anim_Play("logo.anm");
                G_FadePalette(0, 0, 0, 252);
                inputState.ClearAllInput();
            }

            twod->ClearScreen();
            videoNextPage();
            FX_StopAllSounds();
            S_ClearSoundLocks();
        }

        S_PlaySpecialMusicOrNothing(MUS_INTRO);

        //g_player[myconnectindex].ps->palette = drealms;
        //G_FadePalette(0,0,0,252);

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
                twod->ClearScreen();

                fadepal(0, 0, 0, 0, 252, 28);
                renderFlushPerms();
                rotatesprite_fs(160 << 16, 100 << 16, 65536L, 0, DREALMS, 0, 0, 2 + 8 + 64 + BGSTRETCH, nullptr, DREALMSPAL);
                videoNextPage();
                fadepaltile(0, 0, 0, 252, 0, -28, DREALMS, DREALMSPAL);
                totalclock = 0;

                while (totalclock < (120 * 7) && !inputState.CheckAllInput())
                {
                    if (G_FPSLimit())
                    {
                        twod->ClearScreen();
                        rotatesprite_fs(160 << 16, 100 << 16, 65536L, 0, DREALMS, 0, 0, 2 + 8 + 64 + BGSTRETCH, nullptr, DREALMSPAL);
                        G_HandleAsync();
                        videoNextPage();
                    }
                }

                fadepaltile(0, 0, 0, 0, 252, 28, DREALMS, DREALMSPAL);
            }
        }

        twod->ClearScreen();
        videoNextPage();

        inputState.ClearAllInput();

        twod->ClearScreen();
        videoNextPage();

        twod->ClearScreen();

        //g_player[myconnectindex].ps->palette = titlepal;
        renderFlushPerms();
        rotatesprite_fs(160<<16, 100<<16, 65536L, 0, BETASCREEN, 0, 0, 2+8+64+BGSTRETCH, nullptr, TITLEPAL);
        inputState.keyFlushChars();
        fadepaltile(0, 0, 0, 252, 0, -28, BETASCREEN, TITLEPAL);
        totalclock = 0;

        while (
            totalclock < (860+120) &&
            !inputState.CheckAllInput())
        {
            if (G_FPSLimit())
            {
                twod->ClearScreen();
                rotatesprite_fs(160<<16, 100<<16, 65536L, 0, BETASCREEN, 0, 0, 2+8+64+BGSTRETCH, nullptr, TITLEPAL);

                if (totalclock > 120 && totalclock < (120+60))
                {
                    if (soundanm == 0)
                    {
                        soundanm++;
                        S_PlaySound(PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
                    }
                    rotatesprite_fs(160<<16, 104<<16, ((int32_t) totalclock-120)<<10, 0, DUKENUKEM, 0, 0, 2+8, nullptr, TITLEPAL);
                }
                else if (totalclock >= (120+60))
                    rotatesprite_fs(160<<16, (104)<<16, 60<<10, 0, DUKENUKEM, 0, 0, 2+8, nullptr, TITLEPAL);

                if (totalclock > 220 && totalclock < (220+30))
                {
                    if (soundanm == 1)
                    {
                        soundanm++;
                        S_PlaySound(PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
                    }

                    rotatesprite_fs(160<<16, (104)<<16, 60<<10, 0, DUKENUKEM, 0, 0, 2+8, nullptr, TITLEPAL);
                    rotatesprite_fs(160<<16, (129)<<16, ((int32_t) totalclock - 220)<<11, 0, THREEDEE, 0, 0, 2+8, nullptr, TITLEPAL);
                }
                else if (totalclock >= (220+30))
                    rotatesprite_fs(160<<16, (129)<<16, 30<<11, 0, THREEDEE, 0, 0, 2+8, nullptr, TITLEPAL);

                if (PLUTOPAK)
                {
                    // JBF 20030804
                    if (totalclock >= 280 && totalclock < 395)
                    {
                        rotatesprite_fs(160<<16, (151)<<16, (410-(int32_t) totalclock)<<12, 0, PLUTOPAKSPRITE+1, (sintable[((int32_t) totalclock<<4)&2047]>>11), 0, 2+8, nullptr, TITLEPAL);
                        if (soundanm == 2)
                        {
                            soundanm++;
                            S_PlaySound(FLY_BY, CHAN_AUTO, CHANF_UI);
                        }
                    }
                    else if (totalclock >= 395)
                    {
                        if (soundanm == 3)
                        {
                            soundanm++;
                            S_PlaySound(PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
                        }
                        rotatesprite_fs(160<<16, (151)<<16, 30<<11, 0, PLUTOPAKSPRITE+1, (sintable[((int32_t) totalclock<<4)&2047]>>11), 0, 2+8, nullptr, TITLEPAL);
                    }
                }

                videoNextPage();
            }

            G_HandleAsync();
        }

        inputState.ClearAllInput();
    }

    renderFlushPerms();
    twod->ClearScreen();
    videoNextPage();

    //g_player[myconnectindex].ps->palette = palette;
    P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);    // JBF 20040308
    S_PlaySound(NITEVISION_ONOFF, CHAN_AUTO, CHANF_UI);

    //G_FadePalette(0,0,0,0);
    twod->ClearScreen();
}

void G_DoOrderScreen(void)
{
    int32_t i;

    videoSetViewableArea(0, 0, xdim-1, ydim-1);

    P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);    // JBF 20040308

    for (i=0; i<4; i++)
    {
        fadepal(0, 0, 0, 0, 252, 28);
        inputState.ClearAllInput();
        rotatesprite_fs(160<<16, 100<<16, 65536L, 0, ORDERING+i, 0, 0, 2+8+64+BGSTRETCH);
        fadepal(0, 0, 0, 252, 0, -28);
        while (!inputState.CheckAllInput())
            G_HandleAsync();
    }
}


static void G_BonusCutscenes(void)
{
    if (RRRA)
        return;
    if (!(numplayers < 2 && ud.eog && ud.from_bonus == 0))
        return;

    if (RR)
    {
        switch (ud.volume_number)
        {
        case 0:
            twod->ClearScreen();
            videoNextPage();
            if (adult_lockout == 0)
            {
                Anim_Play("turdmov.anm");
                inputState.ClearAllInput();
                twod->ClearScreen();
                videoNextPage();
            }
            m_level_number = ud.level_number = 0;
            ud.m_volume_number = ud.volume_number = 1;
            ud.eog = 0;
            fadepal(0, 0, 0, 0, 252, 4);
            inputState.ClearAllInput();
            P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);
            rotatesprite_fs(0, 0, 65536L, 0, TENSCREEN, 0, 0, 2+8+16+64+128+BGSTRETCH);
            videoNextPage();
            fadepal(0, 0, 0, 252, 0, -4);
            inputState.ClearAllInput();
            G_HandleEventsWhileNoInput();
            fadepal(0, 0, 0, 0, 252, 4);
            FX_StopAllSounds();
            S_ClearSoundLocks();
            break;
        case 1:
            twod->ClearScreen();
            videoNextPage();
            if (adult_lockout == 0)
            {
                Anim_Play("rr_outro.anm");
                inputState.ClearAllInput();
                twod->ClearScreen();
                videoNextPage();
            }
            g_lastLevel = 0;
            g_vixenLevel = 1;
            ud.level_number = 0;
            ud.volume_number = 0;
            fadepal(0, 0, 0, 0, 252, 4);
            videoSetViewableArea(0, 0, xdim-1, ydim-1);
            inputState.ClearAllInput();
            P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);
            rotatesprite_fs(0, 0, 65536L, 0, TENSCREEN, 0, 0, 2 + 8 + 16 + 64 + 128 + BGSTRETCH);
            videoNextPage();
            fadepal(0, 0, 0, 252, 0, -4);
            inputState.ClearAllInput();
            G_HandleEventsWhileNoInput();
            fadepal(0, 0, 0, 0, 252, 4);
            FX_StopAllSounds();
            S_ClearSoundLocks();
            break;
        }
        return;
    }

    switch (ud.volume_number)
    {
    case 0:
        if (adult_lockout == 0)
        {
            int32_t bonuscnt=0;
            int32_t const bossmove [] =
            {
                0, 120, VICTORY1+3, 86, 59,
                220, 260, VICTORY1+4, 86, 59,
                260, 290, VICTORY1+5, 86, 59,
                290, 320, VICTORY1+6, 86, 59,
                320, 350, VICTORY1+7, 86, 59,
                350, 380, VICTORY1+8, 86, 59,
                350, 380, VICTORY1+8, 86, 59 // duplicate row to alleviate overflow in the for loop below "boss"
            };

            twod->ClearScreen();
            rotatesprite_fs(0, 50<<16, 65536L, 0, VICTORY1, 0, 0, 2+8+16+64+128+BGSTRETCH, nullptr, ENDINGPAL);
            videoNextPage();
            fadepal(0, 0, 0, 252, 0, -4);

            inputState.ClearAllInput();
            totalclock = 0;

            while (1)
            {
                if (G_FPSLimit())
                {
                    twod->ClearScreen();
                    rotatesprite_fs(0, 50 << 16, 65536L, 0, VICTORY1, 0, 0, 2 + 8 + 16 + 64 + 128 + BGSTRETCH, nullptr, ENDINGPAL);

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
                            rotatesprite_fs(bossmove[t+3]<<16, bossmove[t+4]<<16, 65536L, 0, bossmove[t+2], 0, 0, 2+8+16+64+128+BGSTRETCH, nullptr, ENDINGPAL);
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
                            rotatesprite_fs(86<<16, 59<<16, 65536L, 0, VICTORY1+8, 0, 0, 2+8+16+64+128+BGSTRETCH, nullptr, ENDINGPAL);
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
                                rotatesprite_fs(breathe[t+3]<<16, breathe[t+4]<<16, 65536L, 0, breathe[t+2], 0, 0, 2+8+16+64+128+BGSTRETCH, nullptr, ENDINGPAL);
                            }
                    }

                    videoNextPage();
                }

                G_HandleAsync();

                if (inputState.CheckAllInput()) break;
            }

            fadepal(0, 0, 0, 0, 252, 4);
        }

        inputState.ClearAllInput();
        P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);   // JBF 20040308

        rotatesprite_fs(160<<16, 100<<16, 65536L, 0, 3292, 0, 0, 2+8+64+BGSTRETCH);
        fadepal(0, 0, 0, 252, 0, -4);
        G_HandleEventsWhileNoInput();
        fadepal(0, 0, 0, 0, 252, 4);

        Mus_Stop();
        FX_StopAllSounds();
        S_ClearSoundLocks();
        break;

    case 1:
        videoSetViewableArea(0, 0, xdim-1, ydim-1);

        Mus_Stop();
        twod->ClearScreen();
        videoNextPage();

        if (adult_lockout == 0)
        {
            fadepal(0, 0, 0, 252, 0, -4);
            Anim_Play("cineov2.anm");
            inputState.ClearAllInput();
            twod->ClearScreen();
            videoNextPage();

            S_PlaySound(PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
            fadepal(0, 0, 0, 0, 252, 4);
        }

        inputState.ClearAllInput();
        P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);   // JBF 20040308
        rotatesprite_fs(160<<16, 100<<16, 65536L, 0, 3293, 0, 0, 2+8+64+BGSTRETCH);
        fadepal(0, 0, 0, 252, 0, -4);
        G_HandleEventsWhileNoInput();
        fadepal(0, 0, 0, 0, 252, 4);

        break;

    case 3:
        videoSetViewableArea(0, 0, xdim-1, ydim-1);

        Mus_Stop();
        twod->ClearScreen();
        videoNextPage();

        if (adult_lockout == 0)
        {
            fadepal(0, 0, 0, 252, 0, -4);

            inputState.ClearAllInput();
            int t = Anim_Play("vol4e1.anm");
            twod->ClearScreen();
            videoNextPage();
            if (t)
                goto end_vol4e;

            t = Anim_Play("vol4e2.anm");
            twod->ClearScreen();
            videoNextPage();
            if (t)
                goto end_vol4e;

            Anim_Play("vol4e3.anm");
            twod->ClearScreen();
            videoNextPage();
        }

    end_vol4e:
        FX_StopAllSounds();
        S_ClearSoundLocks();
        S_PlaySound(ENDSEQVOL3SND4, CHAN_AUTO, CHANF_UI);
        inputState.ClearAllInput();

        G_FadePalette(0, 0, 0, 0);
        P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);   // JBF 20040308
                                                                         //        G_FadePalette(0,0,0,252);
        twod->ClearScreen();
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

        twod->ClearScreen();
        videoNextPage();

        Anim_Play("DUKETEAM.ANM");

        inputState.ClearAllInput();
        G_HandleEventsWhileNoInput();

        twod->ClearScreen();
        videoNextPage();
        G_FadePalette(0, 0, 0, 252);

        FX_StopAllSounds();
        S_ClearSoundLocks();
        inputState.ClearAllInput();

        break;

    case 2:
        Mus_Stop();
        twod->ClearScreen();
        videoNextPage();
        if (adult_lockout == 0)
        {
            fadepal(0, 0, 0, 252, 0, -4);
            Anim_Play("cineov3.anm");
            inputState.ClearAllInput();
            ototalclock = totalclock+200;
            while (totalclock < ototalclock)
                G_HandleAsync();
            twod->ClearScreen();
            videoNextPage();

            FX_StopAllSounds();
            S_ClearSoundLocks();
        }

        Anim_Play("RADLOGO.ANM");

        if (!inputState.CheckAllInput() && adult_lockout == 0)
        {
            if (G_PlaySoundWhileNoInput(ENDSEQVOL3SND5)) goto ENDANM;
            if (G_PlaySoundWhileNoInput(ENDSEQVOL3SND6)) goto ENDANM;
            if (G_PlaySoundWhileNoInput(ENDSEQVOL3SND7)) goto ENDANM;
            if (G_PlaySoundWhileNoInput(ENDSEQVOL3SND8)) goto ENDANM;
            if (G_PlaySoundWhileNoInput(ENDSEQVOL3SND9)) goto ENDANM;
        }

        totalclock = 0;
        if (PLUTOPAK)
        {
            while (!inputState.CheckAllInput() && totalclock < 120)
                G_HandleAsync();
        }
        else
        {
            G_HandleEventsWhileNoInput();
        }

    ENDANM:
        if (!PLUTOPAK)
        {
            FX_StopAllSounds();
            S_ClearSoundLocks();
            S_PlaySound(ENDSEQVOL3SND4, CHAN_AUTO, CHANF_UI);

            twod->ClearScreen();
            videoNextPage();

            Anim_Play("DUKETEAM.ANM");

            inputState.ClearAllInput();
            G_HandleEventsWhileNoInput();

            twod->ClearScreen();
            videoNextPage();
            G_FadePalette(0, 0, 0, 252);
        }

        inputState.ClearAllInput();
        FX_StopAllSounds();
        S_ClearSoundLocks();

        twod->ClearScreen();

        break;
    }
}

static void G_DisplayMPResultsScreen(void)
{
    int32_t i, y, t = 0;

    rotatesprite_fs(160<<16, 100<<16, 65536L, 0, MENUSCREEN, 16, 0, 2+8+64+BGSTRETCH);
    rotatesprite_fs(160<<16, 34<<16, RR ? 23592L : 65536L, 0, INGAMEDUKETHREEDEE, 0, 0, 10);
    if (!RR && PLUTOPAK)   // JBF 20030804
        rotatesprite_fs((260)<<16, 36<<16, 65536L, 0, PLUTOPAKSPRITE+2, 0, 0, 2+8);
    gametext_center(58+(RR ? 0 : 2), GStrings("Multiplayer Totals"));
    gametext_center(58+10, currentLevel->DisplayName());

    gametext_center_shade(RR ? 175 : 165, GStrings("Presskey"), quotepulseshade);

    minitext(38, 80, GStrings("Name"), 8, 2+8+16+128);
    minitext(269, 80, GStrings("Kills"), 8, 2+8+16+128);
    for (i=0; i<g_mostConcurrentPlayers; i++)
    {
        Bsprintf(tempbuf, "%-4d", i+1);
        minitext(92+(i*23), 80, tempbuf, RR ? 0 : 3, 2+8+16+128);
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
                minitext(92+(y*23), 90+t, tempbuf, RR ? 0 : 2, 2+8+16+128);
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
        minitext(101+(8*23), 90+t, tempbuf, RR ? 0 : 2, 2+8+16+128);

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
        minitext(92+(y*23), 96+(8*7), tempbuf, RR ? 0 : 2, 2+8+16+128);
    }

    minitext(45, 96+(8*7), GStrings("Deaths"), RR ? 0 : 8, 2+8+16+128);
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
        if (currentLevel->designerTime)
        {
            for (ii=currentLevel->designerTime/(60), ij=1; ii>9; ii/=10, ij++) { }
            clockpad = max(clockpad, ij);
        }
    }
    if (ud.playerbest > 0) for (ii=ud.playerbest/(REALGAMETICSPERSEC*60), ij=1; ii>9; ii/=10, ij++) { }
    clockpad = max(clockpad, ij);

    return clockpad;
}

static const char* G_PrintTime2(int32_t time)
{
    Bsprintf(tempbuf, RR ? "%0*d : %02d" : "%0*d:%02d", G_PrintTime_ClockPad(), time/(REALGAMETICSPERSEC*60), (time/REALGAMETICSPERSEC)%60);
    return tempbuf;
}
static const char* G_PrintTime3(int32_t time)
{
    Bsprintf(tempbuf, RR ? "%0*d : %02d . %02d" : "%0*d:%02d.%02d", G_PrintTime_ClockPad(), time/(REALGAMETICSPERSEC*60), (time/REALGAMETICSPERSEC)%60, ((time%REALGAMETICSPERSEC)*33)/10);
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
    return G_PrintTime2(currentLevel->parTime * REALGAMETICSPERSEC);
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

    //if (g_networkMode == NET_DEDICATED_SERVER)
    //    return;

    if (ud.volume_number == 0 && ud.last_level == 8 && boardfilename[0])
    {
        lastmapname = Bstrrchr(boardfilename, '\\');
        if (!lastmapname) lastmapname = Bstrrchr(boardfilename, '/');
        if (!lastmapname) lastmapname = boardfilename;
    }
    else
    {
        lastmapname = currentLevel->DisplayName();
    }

    fadepal(0, 0, 0, 0, 252, RR ? 4 : 28);
    videoSetViewableArea(0, 0, xdim-1, ydim-1);
    twod->ClearScreen();
    videoNextPage();
    renderFlushPerms();

    FX_StopAllSounds();
    S_ClearSoundLocks();
    FX_SetReverb(0L);
    inputState.SetBindsEnabled(1); // so you can use your screenshot bind on the score screens

    if (!bonusonly)
        G_BonusCutscenes();

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
        twod->ClearScreen();
        G_DisplayMPResultsScreen();

        PlayBonusMusic();

        videoNextPage();
        inputState.ClearAllInput();
        fadepal(0, 0, 0, 252, 0, RR ? -4 : -28);
        totalclock = 0;

        while (totalclock < TICRATE*10)
        {
            G_HandleAsync();

            if (G_FPSLimit())
            {
                twod->ClearScreen();
                G_DisplayMPResultsScreen();
                videoNextPage();
            }

            if (inputState.CheckAllInput())
            {
                break;
            }
        }

        fadepal(0, 0, 0, 0, 252, RR ? 4 : 28);
    }

    if (bonusonly || (g_netServer || ud.multimode > 1)) return;

    if (!RR)
    {
        gfx_offset = (ud.volume_number==1) ? 5 : 0;
        gfx_offset += BONUSSCREEN;
        rotatesprite_fs(160<<16, 100<<16, 65536L, 0, gfx_offset, 0, 0, 2+8+64+128+BGSTRETCH);

        if (lastmapname)
            menutext_center(20-6, lastmapname);
    	menutext_center(36-6, GStrings("Completed"));

        gametext_center_shade(192, GStrings("PRESSKEY"), quotepulseshade);

        PlayBonusMusic();
    }
    else
    {
        gfx_offset = (ud.volume_number==0) ? RRTILE403 : RRTILE409;
        gfx_offset += ud.level_number-1;

        if (g_lastLevel || g_vixenLevel)
            gfx_offset = RRTILE409+7;

        if (boardfilename[0])
            gfx_offset = RRTILE403;

        rotatesprite_fs(160<<16, 100<<16, 65536L, 0, gfx_offset, 0, 0, 2+8+64+128+BGSTRETCH);
        if (lastmapname)
            menutext(80,16, lastmapname);

        menutext(15, 192, GStrings("PRESSKEY"));
    }

    videoNextPage();
    inputState.ClearAllInput();
    fadepal(0, 0, 0, 252, 0, -4);
    bonuscnt = 0;
    totalclock = 0;

    do
    {
        int32_t yy = 0, zz;

        G_HandleAsync();

        if (G_FPSLimit())
        {
            if (g_player[myconnectindex].ps->gm&MODE_EOL)
            {
                twod->ClearScreen();
                rotatesprite_fs(160<<16, 100<<16, 65536L, 0, gfx_offset, 0, 0, 2+8+64+128+BGSTRETCH);

                if (totalclock >= 1000000000 && totalclock < 1000000320)
                {
                    switch (((int32_t) totalclock>>4)%15)
                    {
                    case 0:
                        if (bonuscnt == 6)
                        {
                            bonuscnt++;
                            S_PlaySound(RR ? 425 : SHOTGUN_COCK, CHAN_AUTO, CHANF_UI);
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
                        if (!RR)
                            rotatesprite_fs(199<<16, 31<<16, 65536L, 0, 3+gfx_offset, 0, 0, 2+8+16+64+128+BGSTRETCH);
                        break;
                    case 2:
                    case 3:
                        if (!RR)
                            rotatesprite_fs(199<<16, 31<<16, 65536L, 0, 4+gfx_offset, 0, 0, 2+8+16+64+128+BGSTRETCH);
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
                        if (!RR)
                            rotatesprite_fs(199<<16, 31<<16, 65536L, 0, 1+gfx_offset, 0, 0, 2+8+16+64+128+BGSTRETCH);
                        break;
                    case 2:
                        if (!RR)
                            rotatesprite_fs(199<<16, 31<<16, 65536L, 0, 2+gfx_offset, 0, 0, 2+8+16+64+128+BGSTRETCH);
                        break;
                    }
                }

                if (!RR)
                {
                    if (lastmapname)
                        menutext_center(20-6, lastmapname);
    	            menutext_center(36-6, GStrings("Completed"));

                    gametext_center_shade(192, GStrings("PRESSKEY"), quotepulseshade);
                }
                else
                {
                    if (lastmapname)
                        menutext(80, 16, lastmapname);

                    menutext(15, 192, GStrings("PRESSKEY"));
                }

                const int yystep = RR ? 16 : 10;
                if (totalclock > (60*3))
                {
                    yy = zz = RR ? 48 : 59;

                    if (!RR)
                        gametext(10, yy+9, GStrings("TXT_YourTime"));
                    else
                        menutext(30, yy, GStrings("TXT_YerTime"));

                    yy+= yystep;
                    if (!(ud.volume_number == 0 && ud.last_level-1 == 7 && boardfilename[0]))
                    {
                        if (currentLevel->parTime)
                        {
                            if (!RR)
                                gametext(10, yy+9, GStrings("TXT_ParTime"));
                            else
                                menutext(30, yy, GStrings("TXT_ParTime"));
                            yy+=yystep;
                        }
                        if (currentLevel->designerTime)
                        {
                            // EDuke 2.0 / NAM source suggests "Green Beret's Time:"
                            if (DUKE)
                                gametext(10, yy+9, GStrings("TXT_3DRTIME"));
                            else if (RR)
                                menutext(30, yy, GStrings("TXT_XTRTIME"));
                            yy+=yystep;
                        }

                    }
                    if (ud.playerbest > 0)
                    {
                        if (!RR)
                            gametext(10, yy+9, (g_player[myconnectindex].ps->player_par > 0 && g_player[myconnectindex].ps->player_par < ud.playerbest) ? "Prev Best Time:" : "Your Best Time:");
                        else
                            menutext(30, yy, (g_player[myconnectindex].ps->player_par > 0 && g_player[myconnectindex].ps->player_par < ud.playerbest) ? "Prev Best:" : "Yer Best:");
                        yy += yystep;
                    }

                    if (bonuscnt == 0)
                        bonuscnt++;

                    yy = zz;
                    if (totalclock >(60*4))
                    {
                        if (bonuscnt == 1)
                        {
                            bonuscnt++;
                            S_PlaySound(RR ? 404 : PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
                        }

                        if (g_player[myconnectindex].ps->player_par > 0)
                        {
                            G_PrintYourTime();
                            if (!RR)
                            {
                                gametext_number((320>>2)+71, yy+9, tempbuf);
                                if (g_player[myconnectindex].ps->player_par < ud.playerbest)
                                gametext((320>>2)+89+(clockpad*24), yy+9, GStrings("TXT_NEWRECORD"));
                            }
                            else
                            {
                                menutext(191, yy, tempbuf);
                                //if (g_player[myconnectindex].ps->player_par < ud.playerbest)
                                //    menutext(191 + 30 + (clockpad*24), yy, "New record!");
                            }
                        }
                        else if (!RR)
                            gametext_pal((320>>2)+71, yy+9, GStrings("TXT_Cheated"), 2);
                        else
                            menutext(191, yy, GStrings("TXT_Cheated"));
                        yy+=yystep;

                        if (!(ud.volume_number == 0 && ud.last_level-1 == 7 && boardfilename[0]))
                        {
                            if (currentLevel->parTime)
                            {
                                G_PrintParTime();
                                if (!RR)
                                    gametext_number((320>>2)+71, yy+9, tempbuf);
                                else
                                    menutext(191, yy, tempbuf);
                                yy+=yystep;
                            }
                            if (currentLevel->designerTime)
                            {
                                G_PrintDesignerTime();
                                if (DUKE)
                                    gametext_number((320>>2)+71, yy+9, tempbuf);
                                else if (RR)
                                    menutext(191, yy, tempbuf);
                                yy+=yystep;
                            }
                        }

                        if (ud.playerbest > 0)
                        {
                            G_PrintBestTime();
                            if (!RR)
                                gametext_number((320>>2)+71, yy+9, tempbuf);
                            else
                                menutext(191, yy, tempbuf);
                            yy+=yystep;
                        }
                    }
                }

                zz = yy += RR ? 16 : 5;
                if (totalclock > (60*6))
                {
                    if (!RR)
                        gametext(10, yy+9, GStrings("TXT_EnemiesKilled"));
                    else
                        menutext(30, yy, GStrings("TXT_VarmintsKilled"));
                    yy += yystep;
                    if (!RR)
                        gametext(10, yy+9, GStrings("TXT_EnemiesLeft"));
                    else
                        menutext(30, yy, GStrings("TXT_VarmintsLeft"));
                    yy += yystep;

                    if (bonuscnt == 2)
                    {
                        bonuscnt++;
                        if (!RR)
                            S_PlaySound(FLY_BY, CHAN_AUTO, CHANF_UI);
                    }

                    yy = zz;

                    if (totalclock > (60*7))
                    {
                        if (bonuscnt == 3)
                        {
                            bonuscnt++;
                            S_PlaySound(RR ? 422 : PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
                        }
                        Bsprintf(tempbuf, "%-3d", g_player[myconnectindex].ps->actors_killed);
                        if (!RR)
                            gametext_number((320>>2)+70, yy+9, tempbuf);
                        else
                            menutext(231,yy,tempbuf);
                        yy += yystep;
                        if (ud.player_skill > 3 && !RR)
                        {
                            if (!RR)
                      	      gametext((320>>2)+70, yy+9, GStrings("TXT_N_A"));
                            else
                                menutext(231,yy, GStrings("TXT_N_A"));
                            yy += yystep;
                        }
                        else
                        {
                            if ((g_player[myconnectindex].ps->max_actors_killed-g_player[myconnectindex].ps->actors_killed) < 0)
                                Bsprintf(tempbuf, "%-3d", 0);
                            else Bsprintf(tempbuf, "%-3d", g_player[myconnectindex].ps->max_actors_killed-g_player[myconnectindex].ps->actors_killed);
                            if (!RR)
                                gametext_number((320>>2)+70, yy+9, tempbuf);
                            else
                                menutext(231, yy, tempbuf);
                            yy += yystep;
                        }
                    }
                }

                zz = yy += RR ? 0 : 5;
                if (totalclock > (60*9))
                {
                    if (!RR)
                        gametext(10, yy+9, GStrings("TXT_SECFND"));
                    else
                        menutext(30, yy, GStrings("TXT_SECFND"));
                    yy += yystep;
                    if (!RR)
                        gametext(10, yy+9, GStrings("TXT_SECMISS"));
                    else
                        menutext(30, yy, GStrings("TXT_SECMISS"));
                    yy += yystep;
                    if (bonuscnt == 4) bonuscnt++;

                    yy = zz;
                    if (totalclock > (60*10))
                    {
                        if (bonuscnt == 5)
                        {
                            bonuscnt++;
                            S_PlaySound(RR ? 404 : PIPEBOMB_EXPLODE, CHAN_AUTO, CHANF_UI);
                        }
                        Bsprintf(tempbuf, "%-3d", g_player[myconnectindex].ps->secret_rooms);
                        if (!RR)
                            gametext_number((320>>2)+70, yy+9, tempbuf);
                        else
                            menutext(231, yy, tempbuf);
                        yy += yystep;
                        Bsprintf(tempbuf, "%-3d", g_player[myconnectindex].ps->max_secret_rooms-g_player[myconnectindex].ps->secret_rooms);
                        if (!RR)
                            gametext_number((320>>2)+70, yy+9, tempbuf);
                        else
                            menutext(231, yy, tempbuf);
                        yy += yystep;
                        }
                    }

                if (totalclock > 10240 && totalclock < 10240+10240)
                    totalclock = 1024;

                if (inputState.CheckAllInput() && totalclock >(60*2)) // JBF 20030809
                {
                    if (totalclock < (60*13))
                    {
                        totalclock = (60*13);
                    }
                    else if (totalclock < 1000000000)
                        totalclock = 1000000000;
                }
            }
            else
                break;

            videoNextPage();
        }
    } while (1);
    if (g_turdLevel)
        g_turdLevel = 0;
    if (g_vixenLevel)
        g_vixenLevel = 0;
}

void G_PlayMapAnim(void)
{
    const char *animFile;
    if (ud.volume_number == 0)
    {
        switch (ud.level_number)
        {
            case 1:
                animFile = "lvl1.anm";
                break;
            case 2:
                animFile = "lvl2.anm";
                break;
            case 3:
                animFile = "lvl3.anm";
                break;
            case 4:
                animFile = "lvl4.anm";
                break;
            case 5:
                animFile = "lvl5.anm";
                break;
            case 6:
                animFile = "lvl6.anm";
                break;
            default:
                animFile = "lvl7.anm";
                break;
        }
    }
    else
    {
        switch (ud.level_number)
        {
            case 1:
                animFile = "lvl8.anm";
                break;
            case 2:
                animFile = "lvl9.anm";
                break;
            case 3:
                animFile = "lvl10.anm";
                break;
            case 4:
                animFile = "lvl11.anm";
                break;
            case 5:
                animFile = "lvl12.anm";
                break;
            case 6:
                animFile = "lvl13.anm";
                break;
            default:
                animFile = NULL;
                break;
        }
    }

    if (animFile == NULL)
        return;

    Anim_Play(animFile);
}

void G_ShowMapFrame(void)
{
    int frame = -1;

    if (ud.volume_number == 0)
    {
        switch (ud.level_number)
        {
        case 1:
            frame = 0;
            break;
        case 2:
            frame = 1;
            break;
        case 3:
            frame = 2;
            break;
        case 4:
            frame = 3;
            break;
        case 5:
            frame = 4;
            break;
        case 6:
            frame = 5;
            break;
        default:
            frame = 6;
            break;
        }
    }
    else
    {
        switch (ud.level_number)
        {
        case 1:
            frame = 7;
            break;
        case 2:
            frame = 8;
            break;
        case 3:
            frame = 9;
            break;
        case 4:
            frame = 10;
            break;
        case 5:
            frame = 11;
            break;
        case 6:
            frame = 12;
            break;
        default:
            frame = -1;
            break;
        }
    }
    rotatesprite_fs(160<<16,100<<16,65536L,0,RRTILE8624+frame,0,0,10+64+128);
}

void G_BonusScreenRRRA(int32_t bonusonly)
{
    int32_t gfx_offset;
    int32_t bonuscnt;
    int32_t showMap = 0;
    const char *lastmapname;

    //if (g_networkMode == NET_DEDICATED_SERVER)
    //    return;

    if (ud.volume_number == 0 && ud.last_level == 8 && boardfilename[0])
    {
        lastmapname = Bstrrchr(boardfilename, '\\');
        if (!lastmapname) lastmapname = Bstrrchr(boardfilename, '/');
        if (!lastmapname) lastmapname = boardfilename;
    }
    else
    {
        lastmapname = currentLevel->DisplayName();
    }

    fadepal(0, 0, 0, 0, 252, 4);
    videoSetViewableArea(0, 0, xdim-1, ydim-1);
    twod->ClearScreen();
    videoNextPage();
    renderFlushPerms();

    FX_StopAllSounds();
    S_ClearSoundLocks();
    FX_SetReverb(0L);
    inputState.SetBindsEnabled(1); // so you can use your screenshot bind on the score screens

    if (boardfilename[0] == 0 && numplayers < 2)
    {
        if ((ud.eog == 0 || ud.volume_number != 1) && ud.volume_number <= 1)
        {
            showMap = 1;
            Mus_Stop();
            inputState.keyFlushChars();

            P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);
            G_ShowMapFrame();
            fadepal(0, 0, 0, 252, 0, -4);
            P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);
        }
    }

    if (!bonusonly)
        G_BonusCutscenes();

    P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);   // JBF 20040308
    //G_FadePalette(0, 0, 0, 252);   // JBF 20031228
    inputState.keyFlushChars();
    totalclock = 0;
    bonuscnt = 0;

    Mus_Stop();
    FX_StopAllSounds();
    S_ClearSoundLocks();

    if (g_mostConcurrentPlayers > 1 && (g_gametypeFlags[ud.coop]&GAMETYPE_SCORESHEET))
    {
        twod->ClearScreen();
        G_DisplayMPResultsScreen();

        PlayBonusMusic();

        videoNextPage();
        inputState.ClearAllInput();
        fadepal(0, 0, 0, 252, 0, -4);
        totalclock = 0;

        while (totalclock < TICRATE*10)
        {
            G_HandleAsync();

            if (G_FPSLimit())
            {
                twod->ClearScreen();
                G_DisplayMPResultsScreen();
                videoNextPage();
            }

            if (inputState.CheckAllInput())
            {
                break;
            }
        }

        fadepal(0, 0, 0, 0, 252, 4);
    }

    if (bonusonly || (g_netServer || ud.multimode > 1)) return;

    gfx_offset = (ud.volume_number==0) ? RRTILE403 : RRTILE409;
    gfx_offset += ud.level_number-1;

    if (g_lastLevel || g_vixenLevel)
        gfx_offset = RRTILE409+7;

    if (boardfilename[0])
        gfx_offset = RRTILE403;

    if (!showMap)
        rotatesprite_fs(160<<16, 100<<16, 65536L, 0, gfx_offset, 0, 0, 2+8+64+128+BGSTRETCH);
    if (lastmapname)
        menutext(80,16, lastmapname);

    menutext(15, 192, "Press any key to continue");

    inputState.ClearAllInput();
    if (!showMap)
    {
        videoNextPage();
        fadepal(0, 0, 0, 252, 0, -4);
    }
    bonuscnt = 0;
    totalclock = 0;

    do
    {
        int32_t yy = 0, zz;

        G_HandleAsync();

        if (G_FPSLimit())
        {
            if (g_player[myconnectindex].ps->gm&MODE_EOL)
            {
                twod->ClearScreen();
                if (showMap)
                    G_ShowMapFrame();
                else
                    rotatesprite_fs(160<<16, 100<<16, 65536L, 0, gfx_offset, 0, 0, 2+8+64+128+BGSTRETCH);

                if (showMap)
                {
                    if (bonuscnt == 7)
                    {
                        bonuscnt++;
                        Mus_Stop();
                        G_PlayMapAnim();
                        break;
                    }
                }

                if (totalclock >= 1000000000 && totalclock < 1000000320)
                {
                    switch (((int32_t) totalclock>>4)%15)
                    {
                    case 0:
                        if (bonuscnt == 6)
                        {
                            bonuscnt++;
                            S_PlaySound(425, CHAN_AUTO, CHANF_UI);
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
                        break;
                    case 2:
                    case 3:
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
                        break;
                    case 2:
                        break;
                    }
                }

                if (lastmapname)
                    menutext(80, 16, lastmapname);

                menutext(15, 192, GStrings("PRESSKEY"));

                const int yystep = 16;
                if (totalclock > (60*3))
                {
                    yy = zz = 48;

                    menutext(30, yy, GStrings("TXT_YERTIME"));

                    yy+= yystep;
                    if (!(ud.volume_number == 0 && ud.last_level-1 == 7 && boardfilename[0]))
                    {
                        if (currentLevel->parTime)
                        {
                            menutext(30, yy, GStrings("TXT_PARTIME"));
                            yy+=yystep;
                        }
                        if (currentLevel->designerTime)
                        {
                            menutext(30, yy, GStrings("TXT_XTRTIME"));
                            yy+=yystep;
                        }

                    }
                    if (ud.playerbest > 0)
                    {
                        menutext(30, yy, (g_player[myconnectindex].ps->player_par > 0 && g_player[myconnectindex].ps->player_par < ud.playerbest) ? "Prev Best:" : "Yer Best:");
                        yy += yystep;
                    }

                    if (bonuscnt == 0)
                        bonuscnt++;

                    yy = zz;
                    if (totalclock >(60*4))
                    {
                        if (bonuscnt == 1)
                        {
                            bonuscnt++;
                            S_PlaySound(404, CHAN_AUTO, CHANF_UI);
                        }

                        if (g_player[myconnectindex].ps->player_par > 0)
                        {
                            G_PrintYourTime();
                            menutext(191, yy, tempbuf);
                            //if (g_player[myconnectindex].ps->player_par < ud.playerbest)
                            //    menutext(191 + 30 + (clockpad*24), yy, "New record!");
                        }
                        else
                            menutext(191, yy, GStrings("TXT_Cheated"));
                        yy+=yystep;

                        if (!(ud.volume_number == 0 && ud.last_level-1 == 7 && boardfilename[0]))
                        {
                            if (currentLevel->parTime)
                            {
                                G_PrintParTime();
                                menutext(191, yy, tempbuf);
                                yy+=yystep;
                            }
                            if (currentLevel->designerTime)
                            {
                                G_PrintDesignerTime();
                                menutext(191, yy, tempbuf);
                                yy+=yystep;
                            }
                        }

                        if (ud.playerbest > 0)
                        {
                            G_PrintBestTime();
                            menutext(191, yy, tempbuf);
                            yy+=yystep;
                        }
                    }
                }

                zz = yy += 16;
                if (totalclock > (60*6))
                {
                    menutext(30, yy, GStrings("TXT_VARMINTSKILLED"));
                    yy += yystep;
                    menutext(30, yy, GStrings("TXT_VARMINTSLEFT"));
                    yy += yystep;

                    if (bonuscnt == 2)
                        bonuscnt++;

                    yy = zz;

                    if (totalclock > (60*7))
                    {
                        if (bonuscnt == 3)
                        {
                            bonuscnt++;
                            S_PlaySound(422, CHAN_AUTO, CHANF_UI);
                        }
                        Bsprintf(tempbuf, "%-3d", g_player[myconnectindex].ps->actors_killed);
                        menutext(231,yy,tempbuf);
                        yy += yystep;
                        //if (ud.player_skill > 3)
                        //{
                        //    menutext(231,yy, "N/A");
                        //    yy += yystep;
                        //}
                        //else
                        {
                            if ((g_player[myconnectindex].ps->max_actors_killed-g_player[myconnectindex].ps->actors_killed) < 0)
                                Bsprintf(tempbuf, "%-3d", 0);
                            else Bsprintf(tempbuf, "%-3d", g_player[myconnectindex].ps->max_actors_killed-g_player[myconnectindex].ps->actors_killed);
                            menutext(231, yy, tempbuf);
                            yy += yystep;
                        }
                    }
                }

                zz = yy += 0;
                if (totalclock > (60*9))
                {
                    menutext(30, yy, GStrings("TXT_SECFND"));
                    yy += yystep;
                    menutext(30, yy, GStrings("TXT_SECMISS"));
                    yy += yystep;
                    if (bonuscnt == 4) bonuscnt++;

                    yy = zz;
                    if (totalclock > (60*10))
                    {
                        if (bonuscnt == 5)
                        {
                            bonuscnt++;
                            S_PlaySound(404, CHAN_AUTO, CHANF_UI);
                        }
                        Bsprintf(tempbuf, "%-3d", g_player[myconnectindex].ps->secret_rooms);
                        menutext(231, yy, tempbuf);
                        yy += yystep;
                        Bsprintf(tempbuf, "%-3d", g_player[myconnectindex].ps->max_secret_rooms-g_player[myconnectindex].ps->secret_rooms);
                        menutext(231, yy, tempbuf);
                        yy += yystep;
                        }
                    }

                if (totalclock > 10240 && totalclock < 10240+10240)
                    totalclock = 1024;

                if (inputState.CheckAllInput() && totalclock >(60*2)) // JBF 20030809
                {
                    if (totalclock < (60*13))
                    {
                        totalclock = (60*13);
                    }
                    else if (totalclock < 1000000000)
                        totalclock = 1000000000;
                }
            }
            else
                break;
            videoNextPage();
        }
    } while (1);
    if (ud.eog)
    {
        fadepal(0, 0, 0, 0, 252, 4);
        twod->ClearScreen();
        videoNextPage();
        S_PlaySound(35, CHAN_AUTO, CHANF_UI);
        G_FadePalette(0, 0, 0, 0);
        P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);
        while (1)
        {
            switch (((int32_t) totalclock >> 4) & 1)
            {
            case 0:
                rotatesprite(0,0,65536,0,RRTILE8677,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                videoNextPage();
                G_FadePalette(0, 0, 0, 0);
                P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);
                Net_GetPackets();
                break;
            case 1:
                rotatesprite(0,0,65536,0,RRTILE8677+1,0,0,2+8+16+64+128,0,0,xdim-1,ydim-1);
                videoNextPage();
                G_FadePalette(0, 0, 0, 0);
                P_SetGamePalette(g_player[myconnectindex].ps, BASEPAL, 0);
                Net_GetPackets();
                break;
            }

            if (!S_CheckSoundPlaying(-1,35)) break;
            if (inputState.CheckAllInput())
            {
                S_StopSound(35);
                break;
            }
        }
    }
    if (g_RAendEpisode)
    {
        g_RAendEpisode = 0;
        ud.m_volume_number = ud.volume_number = 1;
        m_level_number = ud.level_number = 0;
        ud.eog = 0;
    }
    if (g_turdLevel)
        g_turdLevel = 0;
    if (g_vixenLevel)
        g_vixenLevel = 0;
}

END_RR_NS
