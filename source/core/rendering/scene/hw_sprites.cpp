
#if 0
void polymost_drawsprite(int32_t snum)
{

    vec2_t off = { 0, 0 };

    if ((globalorientation & 48) != 48)  // only non-voxel sprites should do this
    {
        int const flag = hw_hightile && TileFiles.tiledata[globalpicnum].h_xsize;
        off = { (int32_t)tspr->xoffset + (flag ? TileFiles.tiledata[globalpicnum].h_xoffs : tileLeftOffset(globalpicnum)),
                (int32_t)tspr->yoffset + (flag ? TileFiles.tiledata[globalpicnum].h_yoffs : tileTopOffset(globalpicnum)) };
    }

    int32_t method = DAMETH_MASK | DAMETH_CLAMPED;

    if (tspr->cstat & 2)
        method = DAMETH_CLAMPED | ((tspr->cstat & 512) ? DAMETH_TRANS2 : DAMETH_TRANS1);

    SetRenderStyleFromBlend(!!(tspr->cstat & 2), tspr->blend, !!(tspr->cstat & 512));

    drawpoly_alpha = spriteext[spritenum].alpha;
    drawpoly_blend = tspr->blend;

    sec = (usectorptr_t)&sector[tspr->sectnum];


    vec3_t pos = tspr->pos;


    vec2_t tsiz;

    if (hw_hightile && TileFiles.tiledata[globalpicnum].h_xsize)
        tsiz = { TileFiles.tiledata[globalpicnum].h_xsize, TileFiles.tiledata[globalpicnum].h_ysize };
    else 
        tsiz = { tileWidth(globalpicnum), tileHeight(globalpicnum) };

    if (tsiz.x <= 0 || tsiz.y <= 0)
        return;

    vec2f_t const ftsiz = { (float) tsiz.x, (float) tsiz.y };

    switch ((globalorientation >> 4) & 3)
    {
        case 0:  // Face sprite
        {
            // Project 3D to 2D
            if (globalorientation & 4)
                off.x = -off.x;
            // NOTE: yoff not negated not for y flipping, unlike wall and floor
            // aligned sprites.

            int const ang = (getangle(tspr->x - globalposx, tspr->y - globalposy) + 1024) & 2047;

            float foffs = TSPR_OFFSET(tspr);
            float foffs2 = TSPR_OFFSET(tspr);
			if (fabs(foffs2) < fabs(foffs)) foffs = foffs2;

            vec2f_t const offs = { float(bcosf(ang, -6) * foffs), float(bsinf(ang, -6) * foffs) };

            vec2f_t s0 = { (float)(tspr->x - globalposx) + offs.x,
                           (float)(tspr->y - globalposy) + offs.y};

            vec2f_t p0 = { s0.y * gcosang - s0.x * gsinang, s0.x * gcosang2 + s0.y * gsinang2 };

            if (p0.y <= SCISDIST)
                goto _drawsprite_return;

            float const ryp0 = 1.f / p0.y;
            s0 = { ghalfx * p0.x * ryp0 + ghalfx, ((float)(pos.z - globalposz)) * gyxscale * ryp0 + ghoriz };

            float const f = ryp0 * fxdimen * (1.0f / 160.f);

            vec2f_t ff = { ((float)tspr->xrepeat) * f,
                           ((float)tspr->yrepeat) * f * ((float)yxaspect * (1.0f / 65536.f)) };

            if (tsiz.x & 1)
                s0.x += ff.x * 0.5f;
            if (globalorientation & 128 && tsiz.y & 1)
                s0.y += ff.y * 0.5f;

            s0.x -= ff.x * (float) off.x;
            s0.y -= ff.y * (float) off.y;

            ff.x *= ftsiz.x;
            ff.y *= ftsiz.y;

            vec2f_t pxy[4];

            pxy[0].x = pxy[3].x = s0.x - ff.x * 0.5f;
            pxy[1].x = pxy[2].x = s0.x + ff.x * 0.5f;
            if (!(globalorientation & 128))
            {
                pxy[0].y = pxy[1].y = s0.y - ff.y;
                pxy[2].y = pxy[3].y = s0.y;
            }
            else
            {
                pxy[0].y = pxy[1].y = s0.y - ff.y * 0.5f;
                pxy[2].y = pxy[3].y = s0.y + ff.y * 0.5f;
            }

            xtex.d = ytex.d = ytex.u = xtex.v = 0;
            otex.d = ryp0 * gviewxrange;

            if (!(globalorientation & 4))
            {
                xtex.u = ftsiz.x * otex.d / (pxy[1].x - pxy[0].x + .002f);
                otex.u = -xtex.u * (pxy[0].x - .001f);
            }
            else
            {
                xtex.u = ftsiz.x * otex.d / (pxy[0].x - pxy[1].x - .002f);
                otex.u = -xtex.u * (pxy[1].x + .001f);
            }

            if (!(globalorientation & 8))
            {
                ytex.v = ftsiz.y * otex.d / (pxy[3].y - pxy[0].y + .002f);
                otex.v = -ytex.v * (pxy[0].y - .001f);
            }
            else
            {
                ytex.v = ftsiz.y * otex.d / (pxy[0].y - pxy[3].y - .002f);
                otex.v = -ytex.v * (pxy[3].y + .001f);
            }

            // Clip sprites to ceilings/floors when no parallaxing and not sloped
            if (!(sector[tspr->sectnum].ceilingstat & 3))
            {
                s0.y = ((float) (sector[tspr->sectnum].ceilingz - globalposz)) * gyxscale * ryp0 + ghoriz;
                if (pxy[0].y < s0.y)
                    pxy[0].y = pxy[1].y = s0.y;
            }

            if (!(sector[tspr->sectnum].floorstat & 3))
            {
                s0.y = ((float) (sector[tspr->sectnum].floorz - globalposz)) * gyxscale * ryp0 + ghoriz;
                if (pxy[2].y > s0.y)
                    pxy[2].y = pxy[3].y = s0.y;
            }

            vec2_16_t tempsiz = { (int16_t)tsiz.x, (int16_t)tsiz.y };
            pow2xsplit = 0;
            polymost_drawpoly(pxy, 4, method, tempsiz);

            drawpoly_srepeat = 0;
            drawpoly_trepeat = 0;
        }
        break;

        case 1:  // Wall sprite
        {
            // Project 3D to 2D
            if (globalorientation & 4)
                off.x = -off.x;

            if (globalorientation & 8)
                off.y = -off.y;

            vec2f_t const extent = { float(tspr->xrepeat * bsinf(tspr->ang, -16)),
                                     float(tspr->xrepeat * -bcosf(tspr->ang, -16)) };

            float f = (float)(tsiz.x >> 1) + (float)off.x;

            vec2f_t const vf = { extent.x * f, extent.y * f };

            vec2f_t vec0 = { (float)(pos.x - globalposx) - vf.x,
                             (float)(pos.y - globalposy) - vf.y };

            int32_t const s = tspr->owner;
            int32_t walldist = 1;
            int32_t w = (s == -1) ? -1 : wsprinfo[s].wall;

            vec2f_t p0 = { vec0.y * gcosang - vec0.x * gsinang,
                           vec0.x * gcosang2 + vec0.y * gsinang2 };

            vec2f_t const pp = { extent.x * ftsiz.x + vec0.x,
                                 extent.y * ftsiz.x + vec0.y };

            vec2f_t p1 = { pp.y * gcosang - pp.x * gsinang,
                           pp.x * gcosang2 + pp.y * gsinang2 };

            if ((p0.y <= SCISDIST) && (p1.y <= SCISDIST))
                goto _drawsprite_return;

            // Clip to close parallel-screen plane
            vec2f_t const op0 = p0;

            float t0 = 0.f, t1 = 1.f;

            if (p0.y < SCISDIST)
            {
                t0 = (SCISDIST - p0.y) / (p1.y - p0.y);
                p0 = { (p1.x - p0.x) * t0 + p0.x, SCISDIST };
            }

            if (p1.y < SCISDIST)
            {
                t1 = (SCISDIST - op0.y) / (p1.y - op0.y);
                p1 = { (p1.x - op0.x) * t1 + op0.x, SCISDIST };
            }

            f = 1.f / p0.y;
            const float ryp0 = f * gyxscale;
            float sx0 = ghalfx * p0.x * f + ghalfx;

            f = 1.f / p1.y;
            const float ryp1 = f * gyxscale;
            float sx1 = ghalfx * p1.x * f + ghalfx;

            pos.z -= ((off.y * tspr->yrepeat) << 2);

            if (globalorientation & 128)
            {
                pos.z += ((tsiz.y * tspr->yrepeat) << 1);

                if (tsiz.y & 1)
                    pos.z += (tspr->yrepeat << 1);  // Odd yspans
            }

            xtex.d = (ryp0 - ryp1) * gxyaspect / (sx0 - sx1);
            ytex.d = 0;
            otex.d = ryp0 * gxyaspect - xtex.d * sx0;

            if (globalorientation & 4)
            {
                t0 = 1.f - t0;
                t1 = 1.f - t1;
            }

            xtex.u = (t0 * ryp0 - t1 * ryp1) * gxyaspect * ftsiz.x / (sx0 - sx1);
            ytex.u = 0;
            otex.u = t0 * ryp0 * gxyaspect * ftsiz.x - xtex.u * sx0;

            f = ((float) tspr->yrepeat) * ftsiz.y * 4;

            float sc0 = ((float) (pos.z - globalposz - f)) * ryp0 + ghoriz;
            float sc1 = ((float) (pos.z - globalposz - f)) * ryp1 + ghoriz;
            float sf0 = ((float) (pos.z - globalposz)) * ryp0 + ghoriz;
            float sf1 = ((float) (pos.z - globalposz)) * ryp1 + ghoriz;

            // gvx*sx0 + gvy*sc0 + gvo = 0
            // gvx*sx1 + gvy*sc1 + gvo = 0
            // gvx*sx0 + gvy*sf0 + gvo = tsizy*(gdx*sx0 + gdo)
            f = ftsiz.y * (xtex.d * sx0 + otex.d) / ((sx0 - sx1) * (sc0 - sf0));

            if (!(globalorientation & 8))
            {
                xtex.v = (sc0 - sc1) * f;
                ytex.v = (sx1 - sx0) * f;
                otex.v = -xtex.v * sx0 - ytex.v * sc0;
            }
            else
            {
                xtex.v = (sf1 - sf0) * f;
                ytex.v = (sx0 - sx1) * f;
                otex.v = -xtex.v * sx0 - ytex.v * sf0;
            }

            // Clip sprites to ceilings/floors when no parallaxing
            if (!(sector[tspr->sectnum].ceilingstat & 1))
            {
                if (sector[tspr->sectnum].ceilingz > pos.z - (float)((tspr->yrepeat * tsiz.y) << 2))
                {
                    sc0 = (float)(sector[tspr->sectnum].ceilingz - globalposz) * ryp0 + ghoriz;
                    sc1 = (float)(sector[tspr->sectnum].ceilingz - globalposz) * ryp1 + ghoriz;
                }
            }
            if (!(sector[tspr->sectnum].floorstat & 1))
            {
                if (sector[tspr->sectnum].floorz < pos.z)
                {
                    sf0 = (float)(sector[tspr->sectnum].floorz - globalposz) * ryp0 + ghoriz;
                    sf1 = (float)(sector[tspr->sectnum].floorz - globalposz) * ryp1 + ghoriz;
                }
            }

            if (sx0 > sx1)
            {
                if (globalorientation & 64)
                    goto _drawsprite_return;  // 1-sided sprite

                std::swap(sx0, sx1);
                std::swap(sc0, sc1);
                std::swap(sf0, sf1);
            }

            vec2f_t const pxy[4] = { { sx0, sc0 }, { sx1, sc1 }, { sx1, sf1 }, { sx0, sf0 } };

			vec2_16_t tempsiz = { (int16_t)tsiz.x, (int16_t)tsiz.y };
			pow2xsplit = 0;
            polymost_drawpoly(pxy, 4, method, tempsiz);

            drawpoly_srepeat = 0;
            drawpoly_trepeat = 0;
        }
        break;

        case 2:  // Floor sprite
            else

            break;

        case 3:  // Voxel sprite
            break;
    }

    if (automapping == 1 && (unsigned)spritenum < MAXSPRITES)
        show2dsprite.Set(spritenum);

_drawsprite_return:
    ;
}
#endif
