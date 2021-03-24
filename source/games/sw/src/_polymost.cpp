BEGIN_SW_NS

short GlobStackSect[2];

void
GetUpperLowerSector(short match, int x, int y, short* upper, short* lower)
{
    int i;
    short sectorlist[16];
    int sln = 0;
    int SpriteNum;
    SPRITEp sp;

    // keep a list of the last stacked sectors the view was in and
    // check those fisrt
    sln = 0;
    for (i = 0; i < (int)SIZ(GlobStackSect); i++)
    {
        // will not hurt if GlobStackSect is invalid - inside checks for this
        if (inside(x, y, GlobStackSect[i]) == 1)
        {
            bool found = false;

            SectIterator it(GlobStackSect[i]);
            while ((SpriteNum = it.NextIndex()) >= 0)
            {
                sp = &sprite[SpriteNum];

                if (sp->statnum == STAT_FAF &&
                    (sp->hitag >= VIEW_LEVEL1 && sp->hitag <= VIEW_LEVEL6)
                    && sp->lotag == match)
                {
                    found = true;
                }
            }

            if (!found)
                continue;

            sectorlist[sln] = GlobStackSect[i];
            sln++;
        }
    }

    // didn't find it yet so test ALL sectors
    if (sln < 2)
    {
        sln = 0;
        for (i = numsectors - 1; i >= 0; i--)
        {
            if (inside(x, y, (short)i) == 1)
            {
                bool found = false;

                SectIterator it(i);
                while ((SpriteNum = it.NextIndex()) >= 0)
                {
                    sp = &sprite[SpriteNum];

                    if (sp->statnum == STAT_FAF &&
                        (sp->hitag >= VIEW_LEVEL1 && sp->hitag <= VIEW_LEVEL6)
                        && sp->lotag == match)
                    {
                        found = true;
                    }
                }

                if (!found)
                    continue;

                if (sln < (int)SIZ(GlobStackSect))
                    GlobStackSect[sln] = i;
                if (sln < (int)SIZ(sectorlist))
                    sectorlist[sln] = i;
                sln++;
            }
        }
    }

    // might not find ANYTHING if not tagged right
    if (sln == 0)
    {
        *upper = -1;
        *lower = -1;
        return;
    }
    // Map rooms have NOT been dragged on top of each other
    else if (sln == 1)
    {
        *lower = sectorlist[0];
        *upper = sectorlist[0];
        return;
    }
    // Map rooms HAVE been dragged on top of each other
    // inside will somtimes find that you are in two different sectors if the x,y
    // is exactly on a sector line.
    else if (sln > 2)
    {
        //DSPRINTF(ds, "TOO MANY SECTORS FOUND: x=%d, y=%d, match=%d, num sectors %d, %d, %d, %d, %d, %d", x, y, match, sln, sectorlist[0], sectorlist[1], sectorlist[2], sectorlist[3], sectorlist[4]);
        MONO_PRINT(ds);
        // try again moving the x,y pos around until you only get two sectors
        GetUpperLowerSector(match, x - 1, y, upper, lower);
    }

    if (sln == 2)
    {
        if (sector[sectorlist[0]].floorz < sector[sectorlist[1]].floorz)
        {
            // swap
            // make sectorlist[0] the LOW sector
            short hold;

            hold = sectorlist[0];
            sectorlist[0] = sectorlist[1];
            sectorlist[1] = hold;
        }

        *lower = sectorlist[0];
        *upper = sectorlist[1];
    }
}


bool
FindCeilingView(short match, int32_t* x, int32_t* y, int32_t z, int16_t* sectnum)
{
    int xoff = 0;
    int yoff = 0;
    int i;
    SPRITEp sp = NULL;
    int pix_diff;
    int newz;

    save.zcount = 0;

    // Search Stat List For closest ceiling view sprite
    // Get the match, xoff, yoff from this point
    StatIterator it(STAT_FAF);
    while ((i = it.NextIndex()) >= 0)
    {
        sp = &sprite[i];

        if (sp->hitag == VIEW_THRU_CEILING && sp->lotag == match)
        {
            xoff = *x - sp->x;
            yoff = *y - sp->y;
            break;
        }
    }

    it.Reset(STAT_FAF);
    while ((i = it.NextIndex()) >= 0)
    {
        sp = &sprite[i];

        if (sp->lotag == match)
        {
            // determine x,y position
            if (sp->hitag == VIEW_THRU_FLOOR)
            {
                short upper, lower;

                *x = sp->x + xoff;
                *y = sp->y + yoff;

                // get new sector
                GetUpperLowerSector(match, *x, *y, &upper, &lower);
                *sectnum = upper;
                break;
            }
        }
    }

    if (*sectnum < 0)
        return false;

    ASSERT(sp);
    ASSERT(sp->hitag == VIEW_THRU_FLOOR);

    pix_diff = labs(z - sector[sp->sectnum].floorz) >> 8;
    newz = sector[sp->sectnum].floorz + ((pix_diff / 128) + 1) * Z(128);

    it.Reset(STAT_FAF);
    while ((i = it.NextIndex()) >= 0)
    {
        sp = &sprite[i];

        if (sp->lotag == match)
        {
            // move lower levels ceilings up for the correct view
            if (sp->hitag == VIEW_LEVEL2)
            {
                // save it off
                save.sectnum[save.zcount] = sp->sectnum;
                save.zval[save.zcount] = sector[sp->sectnum].floorz;
                save.pic[save.zcount] = sector[sp->sectnum].floorpicnum;
                save.slope[save.zcount] = sector[sp->sectnum].floorheinum;

                sector[sp->sectnum].floorz = newz;
                // don't change FAF_MIRROR_PIC - ConnectArea
                if (sector[sp->sectnum].floorpicnum != FAF_MIRROR_PIC)
                    sector[sp->sectnum].floorpicnum = FAF_MIRROR_PIC + 1;
                sector[sp->sectnum].floorheinum = 0;

                save.zcount++;
                PRODUCTION_ASSERT(save.zcount < ZMAX);
            }
        }
    }

    return true;
}


bool
FindFloorView(short match, int32_t* x, int32_t* y, int32_t z, int16_t* sectnum)
{
    int xoff = 0;
    int yoff = 0;
    int i;
    SPRITEp sp = NULL;
    int newz;
    int pix_diff;

    save.zcount = 0;

    // Search Stat List For closest ceiling view sprite
    // Get the match, xoff, yoff from this point
    StatIterator it(STAT_FAF);
    while ((i = it.NextIndex()) >= 0)
    {
        sp = &sprite[i];

        if (sp->hitag == VIEW_THRU_FLOOR && sp->lotag == match)
        {
            xoff = *x - sp->x;
            yoff = *y - sp->y;
            break;
        }
    }


    it.Reset(STAT_FAF);
    while ((i = it.NextIndex()) >= 0)
    {
        sp = &sprite[i];

        if (sp->lotag == match)
        {
            // determine x,y position
            if (sp->hitag == VIEW_THRU_CEILING)
            {
                short upper, lower;

                *x = sp->x + xoff;
                *y = sp->y + yoff;

                // get new sector
                GetUpperLowerSector(match, *x, *y, &upper, &lower);
                *sectnum = lower;
                break;
            }
        }
    }

    if (*sectnum < 0)
        return false;

    ASSERT(sp);
    ASSERT(sp->hitag == VIEW_THRU_CEILING);

    // move ceiling multiple of 128 so that the wall tile will line up
    pix_diff = labs(z - sector[sp->sectnum].ceilingz) >> 8;
    newz = sector[sp->sectnum].ceilingz - ((pix_diff / 128) + 1) * Z(128);

    it.Reset(STAT_FAF);
    while ((i = it.NextIndex()) >= 0)
    {
        sp = &sprite[i];

        if (sp->lotag == match)
        {
            // move upper levels floors down for the correct view
            if (sp->hitag == VIEW_LEVEL1)
            {
                // save it off
                save.sectnum[save.zcount] = sp->sectnum;
                save.zval[save.zcount] = sector[sp->sectnum].ceilingz;
                save.pic[save.zcount] = sector[sp->sectnum].ceilingpicnum;
                save.slope[save.zcount] = sector[sp->sectnum].ceilingheinum;

                sector[sp->sectnum].ceilingz = newz;

                // don't change FAF_MIRROR_PIC - ConnectArea
                if (sector[sp->sectnum].ceilingpicnum != FAF_MIRROR_PIC)
                    sector[sp->sectnum].ceilingpicnum = FAF_MIRROR_PIC + 1;
                sector[sp->sectnum].ceilingheinum = 0;

                save.zcount++;
                PRODUCTION_ASSERT(save.zcount < ZMAX);
            }
        }
    }

    return true;
}



short
ViewSectorInScene(short cursectnum, short level)
{
    int i;
    SPRITEp sp;
    short match;

    StatIterator it(STAT_FAF);
    while ((i = it.NextIndex()) >= 0)
    {
        sp = &sprite[i];

        if (sp->hitag == level)
        {
            if (cursectnum == sp->sectnum)
            {
                // ignore case if sprite is pointing up
                if (sp->ang == 1536)
                    continue;

                // only gets to here is sprite is pointing down

                // found a potential match
                match = sp->lotag;

                if (!PicInView(FAF_MIRROR_PIC, true))
                    return -1;

                return match;
            }
        }
    }

    return -1;
}



void
DrawOverlapRoom(int tx, int ty, int tz, fixed_t tq16ang, fixed_t tq16horiz, short tsectnum)
{
    short i;
    short match;

    save.zcount = 0;

    match = ViewSectorInScene(tsectnum, VIEW_LEVEL1);
    if (match != -1)
    {
        FindCeilingView(match, &tx, &ty, tz, &tsectnum);

        if (tsectnum < 0)
            return;

        renderDrawRoomsQ16(tx, ty, tz, tq16ang, tq16horiz, tsectnum);

        // reset Z's
        for (i = 0; i < save.zcount; i++)
        {
            sector[save.sectnum[i]].floorz = save.zval[i];
            sector[save.sectnum[i]].floorpicnum = save.pic[i];
            sector[save.sectnum[i]].floorheinum = save.slope[i];
        }

        analyzesprites(tx, ty, tz, false);
        post_analyzesprites();
        renderDrawMasks();

    }
    else
    {
        match = ViewSectorInScene(tsectnum, VIEW_LEVEL2);
        if (match != -1)
        {
            FindFloorView(match, &tx, &ty, tz, &tsectnum);

            if (tsectnum < 0)
                return;

            renderDrawRoomsQ16(tx, ty, tz, tq16ang, tq16horiz, tsectnum);

            // reset Z's
            for (i = 0; i < save.zcount; i++)
            {
                sector[save.sectnum[i]].ceilingz = save.zval[i];
                sector[save.sectnum[i]].ceilingpicnum = save.pic[i];
                sector[save.sectnum[i]].ceilingheinum = save.slope[i];
            }

            analyzesprites(tx, ty, tz, false);
            post_analyzesprites();
            renderDrawMasks();

        }
    }
}

void FAF_DrawRooms(int x, int y, int z, fixed_t q16ang, fixed_t q16horiz, short sectnum)
{
    int i;
    StatIterator it(STAT_CEILING_FLOOR_PIC_OVERRIDE);
    while ((i = it.NextIndex()) >= 0)
    {
        if (SPRITE_TAG3(i) == 0)
        {
            // back up ceilingpicnum and ceilingstat
            SPRITE_TAG5(i) = sector[sprite[i].sectnum].ceilingpicnum;
            sector[sprite[i].sectnum].ceilingpicnum = SPRITE_TAG2(i);
            SPRITE_TAG4(i) = sector[sprite[i].sectnum].ceilingstat;
            //SET(sector[sprite[i].sectnum].ceilingstat, ((int)SPRITE_TAG7(i))<<7);
            SET(sector[sprite[i].sectnum].ceilingstat, SPRITE_TAG6(i));
            RESET(sector[sprite[i].sectnum].ceilingstat, CEILING_STAT_PLAX);
        }
        else if (SPRITE_TAG3(i) == 1)
        {
            SPRITE_TAG5(i) = sector[sprite[i].sectnum].floorpicnum;
            sector[sprite[i].sectnum].floorpicnum = SPRITE_TAG2(i);
            SPRITE_TAG4(i) = sector[sprite[i].sectnum].floorstat;
            //SET(sector[sprite[i].sectnum].floorstat, ((int)SPRITE_TAG7(i))<<7);
            SET(sector[sprite[i].sectnum].floorstat, SPRITE_TAG6(i));
            RESET(sector[sprite[i].sectnum].floorstat, FLOOR_STAT_PLAX);
        }
    }

    renderDrawRoomsQ16(x,y,z,q16ang,q16horiz,sectnum);

    it.Reset(STAT_CEILING_FLOOR_PIC_OVERRIDE);
    while ((i = it.NextIndex()) >= 0)
    {
        // manually set gotpic
        if (TEST_GOTSECTOR(sprite[i].sectnum))
        {
            SET_GOTPIC(FAF_MIRROR_PIC);
        }

        if (SPRITE_TAG3(i) == 0)
        {
            // restore ceilingpicnum and ceilingstat
            sector[sprite[i].sectnum].ceilingpicnum = SPRITE_TAG5(i);
            sector[sprite[i].sectnum].ceilingstat = SPRITE_TAG4(i);
            //RESET(sector[sprite[i].sectnum].ceilingstat, CEILING_STAT_TYPE_MASK);
            RESET(sector[sprite[i].sectnum].ceilingstat, CEILING_STAT_PLAX);
        }
        else if (SPRITE_TAG3(i) == 1)
        {
            sector[sprite[i].sectnum].floorpicnum = SPRITE_TAG5(i);
            sector[sprite[i].sectnum].floorstat = SPRITE_TAG4(i);
            //RESET(sector[sprite[i].sectnum].floorstat, FLOOR_STAT_TYPE_MASK);
            RESET(sector[sprite[i].sectnum].floorstat, FLOOR_STAT_PLAX);
        }
    }
}

void polymost_drawscreen(PLAYERp pp, int tx, int ty, int tz, binangle tang, fixedhoriz thoriz, int tsectnum)
{
    videoSetCorrectedAspect();
    renderSetAspect(xs_CRoundToInt(double(viewingrange) * tan(r_fov * (pi::pi() / 360.))), yxaspect);
    OverlapDraw = true;
    DrawOverlapRoom(tx, ty, tz, tang.asq16(), thoriz.asq16(), tsectnum);
    OverlapDraw = false;

    if (automapMode != am_full)// && !ScreenSavePic)
    {
        // TEST this! Changed to camerapp
        //JS_DrawMirrors(camerapp, tx, ty, tz, tang.asq16(), thoriz.asq16());
        JS_DrawMirrors(pp, tx, ty, tz, tang.asq16(), thoriz.asq16());
    }

    // TODO: This call is redundant if the tiled overhead map is shown, but the
    // HUD elements should be properly outputted with hardware rendering first.
    if (!FAF_DebugView)
        FAF_DrawRooms(tx, ty, tz, tang.asq16(), thoriz.asq16(), tsectnum);

    analyzesprites(tx, ty, tz, false);
    post_analyzesprites();
    renderDrawMasks();

}

void JS_DrawMirrors(PLAYERp pp, int tx, int ty, int tz,  fixed_t tpq16ang, fixed_t tpq16horiz)
{
    int j, cnt;
    int dist;
    int tposx, tposy; // Camera
    int *longptr;
    fixed_t tang;

//    int tx, ty, tz, tpang;             // Interpolate so mirror doesn't
    // drift!
    bool bIsWallMirror = false;

    // WARNING!  Assuming (MIRRORLABEL&31) = 0 and MAXMIRRORS = 64 <-- JBF: wrong
    longptr = (int *)&gotpic[MIRRORLABEL >> 3];
    if (longptr && (longptr[0] || longptr[1]))
    {
        for (cnt = MAXMIRRORS - 1; cnt >= 0; cnt--)
            //if (TEST_GOTPIC(cnt + MIRRORLABEL) || TEST_GOTPIC(cnt + CAMSPRITE))
            if (TEST_GOTPIC(cnt + MIRRORLABEL) || ((unsigned)mirror[cnt].campic < MAXTILES && TEST_GOTPIC(mirror[cnt].campic)))
            {
                bIsWallMirror = false;
                if (TEST_GOTPIC(cnt + MIRRORLABEL))
                {
                    bIsWallMirror = true;
                    RESET_GOTPIC(cnt + MIRRORLABEL);
                }
                //else if (TEST_GOTPIC(cnt + CAMSPRITE))
                else if ((unsigned)mirror[cnt].campic < MAXTILES && TEST_GOTPIC(mirror[cnt].campic))
                {
                    //RESET_GOTPIC(cnt + CAMSPRITE);
                    RESET_GOTPIC(mirror[cnt].campic);
                }

                mirrorinview = true;

//                tx = pp->oposx + MulScale(pp->posx - pp->oposx, smoothratio, 16);
//                ty = pp->oposy + MulScale(pp->posy - pp->oposy, smoothratio, 16);
//                tz = pp->oposz + MulScale(pp->posz - pp->oposz, smoothratio, 16);
//                tpq16ang = pp->angle.ang.asq16();


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

                if (mirror[cnt].ismagic)
                {
                    SPRITEp sp=NULL;
                    int camhoriz;
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
                    if (mirror[cnt].mstate != m_viewon)
                    {
						tileDelete(MIRROR);
                        // Set TV camera sprite size to 0 to show mirror
                        // behind in this case!

                        if (mirror[cnt].campic != -1)
							tileDelete(mirror[cnt].campic);
                        renderDrawRoomsQ16(dx, dy, dz, tpq16ang, tpq16horiz, sp->sectnum + MAXSECTORS);
                        analyzesprites(dx, dy, dz, false);
                        renderDrawMasks();
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

                    renderPrepareMirror(tx, ty, tz, tpq16ang, tpq16horiz,
                                  mirror[cnt].mirrorwall, /*mirror[cnt].mirrorsector,*/ &tposx, &tposy, &tang);

                    renderDrawRoomsQ16(tposx, tposy, tz, (tang), tpq16horiz, mirror[cnt].mirrorsector + MAXSECTORS);

                    analyzesprites(tposx, tposy, tz, true);
                    renderDrawMasks();

                    renderCompleteMirror();   // Reverse screen x-wise in this
                    // function
                }


                // g_visibility = tvisibility;
                // g_visibility = NormalVisibility;

                // renderDrawRoomsQ16(tx, ty, tz, tpq16ang, tpq16horiz, pp->cursectnum);
                // Clean up anything that the camera view might have done
				tileDelete(MIRROR);
                wall[mirror[cnt].mirrorwall].overpicnum = MIRRORLABEL + cnt;
            }
            else
                mirrorinview = false;
    }
}


END_SW_NS
