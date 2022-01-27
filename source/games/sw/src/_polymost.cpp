BEGIN_SW_NS

bool FindCeilingView(int match, int* x, int* y, int z, sectortype** sect);
bool FindFloorView(int match, int* x, int* y, int z, sectortype** sect);


int ViewSectorInScene(sectortype* cursect, int level)
{
    SWStatIterator it(STAT_FAF);
    while (auto actor = it.Next())
    {
        if (actor->spr.hitag == level)
        {
            if (cursect == actor->sector())
            {
                // ignore case if sprite is pointing up
                if (actor->spr.ang == 1536)
                    continue;

                // only gets to here is sprite is pointing down

                // found a potential match
                int match = actor->spr.lotag;

                if (!testgotpic(FAF_MIRROR_PIC, true))
                    return -1;
                return match;
            }
        }
    }

    return -1;
}



void DrawOverlapRoom(int tx, int ty, int tz, fixed_t tq16ang, fixed_t tq16horiz, sectortype* tsect)
{
    save.zcount = 0;

    int match = ViewSectorInScene(tsect, VIEW_LEVEL1);
    if (match != -1)
    {
        FindCeilingView(match, &tx, &ty, tz, &tsect);

        if (tsect == nullptr)
            return;

        renderDrawRoomsQ16(tx, ty, tz, tq16ang, tq16horiz, sectnum(tsect), false);

        // reset Z's
        for (int i = 0; i < save.zcount; i++)
        {
            save.sect[i]->setfloorz(save.zval[i], true);
            save.sect[i]->floorpicnum = save.pic[i];
            save.sect[i]->setfloorslope(save.slope[i]);
        }

        analyzesprites(pm_tsprite, pm_spritesortcnt, tx, ty, tz, false);
        post_analyzesprites(pm_tsprite, pm_spritesortcnt);
        renderDrawMasks();

    }
    else
    {
        match = ViewSectorInScene(tsect, VIEW_LEVEL2);
        if (match != -1)
        {
            FindFloorView(match, &tx, &ty, tz, &tsect);

            if (tsect == nullptr)
                return;

            renderDrawRoomsQ16(tx, ty, tz, tq16ang, tq16horiz, sectnum(tsect), false);

            // reset Z's
            for (int i = 0; i < save.zcount; i++)
            {
                save.sect[i]->setceilingz(save.zval[i], true);
                save.sect[i]->ceilingpicnum = save.pic[i];
                save.sect[i]->setceilingslope(save.slope[i]);
            }

            analyzesprites(pm_tsprite, pm_spritesortcnt, tx, ty, tz, false);
            post_analyzesprites(pm_tsprite, pm_spritesortcnt);
            renderDrawMasks();

        }
    }
}

void FAF_DrawRooms(int x, int y, int z, fixed_t q16ang, fixed_t q16horiz, int sectnum)
{
    SWStatIterator it(STAT_CEILING_FLOOR_PIC_OVERRIDE);
    while (auto actor = it.Next())
    {
        if (SP_TAG3(actor) == 0)
        {
            // back up ceilingpicnum and ceilingstat
            SP_TAG5(actor) = actor->sector()->ceilingpicnum;
            actor->sector()->ceilingpicnum = SP_TAG2(actor);
            SP_TAG4(actor) = actor->sector()->ceilingstat;
            actor->sector()->ceilingstat |= (ESectorFlags::FromInt(SP_TAG6(actor)));
            actor->sector()->ceilingstat &= ~(CSTAT_SECTOR_SKY);
        }
        else if (SP_TAG3(actor) == 1)
        {
            SP_TAG5(actor) = actor->sector()->floorpicnum;
            actor->sector()->floorpicnum = SP_TAG2(actor);
            SP_TAG4(actor) = actor->sector()->floorstat;
            actor->sector()->floorstat |= (ESectorFlags::FromInt(SP_TAG6(actor)));
            actor->sector()->floorstat &= ~(CSTAT_SECTOR_SKY);
        }
    }

    renderDrawRoomsQ16(x,y,z,q16ang,q16horiz,sectnum, false);

    it.Reset(STAT_CEILING_FLOOR_PIC_OVERRIDE);
    while (auto actor = it.Next())
    {
        // manually set gotpic
        if (gotsector[actor->sectno()])
        {
            gotpic.Set(FAF_MIRROR_PIC);
        }

        if (SP_TAG3(actor) == 0)
        {
            // restore ceilingpicnum and ceilingstat
            actor->sector()->ceilingpicnum = SP_TAG5(actor);
            actor->sector()->ceilingstat = ESectorFlags::FromInt(SP_TAG4(actor));
            actor->sector()->ceilingstat &= ~(CSTAT_SECTOR_SKY);
        }
        else if (SP_TAG3(actor) == 1)
        {
            actor->sector()->floorpicnum = SP_TAG5(actor);
            actor->sector()->floorstat = ESectorFlags::FromInt(SP_TAG4(actor));
            actor->sector()->floorstat &= ~(CSTAT_SECTOR_SKY);
        }
    }
}

void polymost_drawscreen(PLAYER* pp, int tx, int ty, int tz, binangle tang, fixedhoriz thoriz, sectortype* tsect)
{
    videoSetCorrectedAspect();
    renderSetAspect(xs_CRoundToInt(double(viewingrange) * tan(r_fov * (pi::pi() / 360.))), yxaspect);
    OverlapDraw = true;
    DrawOverlapRoom(tx, ty, tz, tang.asq16(), thoriz.asq16(), tsect);
    OverlapDraw = false;

    if (automapMode != am_full)
    {
        // TEST this! Changed to camerapp
        //JS_DrawMirrors(camerapp, tx, ty, tz, tang.asq16(), thoriz.asq16());
        JS_DrawMirrors(pp, tx, ty, tz, tang.asq16(), thoriz.asq16());
    }

    // TODO: This call is redundant if the tiled overhead map is shown, but the
    // HUD elements should be properly outputted with hardware rendering first.
    if (!FAF_DebugView)
        FAF_DrawRooms(tx, ty, tz, tang.asq16(), thoriz.asq16(), sectnum(tsect));

    analyzesprites(pm_tsprite, pm_spritesortcnt, tx, ty, tz, tang.asbuild());
    post_analyzesprites(pm_tsprite, pm_spritesortcnt);
    renderDrawMasks();

}

void JS_DrawMirrors(PLAYER* pp, int tx, int ty, int tz,  fixed_t tpq16ang, fixed_t tpq16horiz)
{
    int j, cnt;
    int dist;
    int tposx, tposy; // Camera
    int *longptr;
    fixed_t tang;

//    int tx, ty, tz, tpang;             // Interpolate so mirror doesn't
    // drift!
    bool bIsWallMirror = false;

    {
        for (cnt = mirrorcnt - 1; cnt >= 0; cnt--)
            //if (testgotpic(cnt + MIRRORLABEL) || testgotpic(cnt + CAMSPRITE))
            if (testgotpic(cnt + MIRRORLABEL) || ((unsigned)mirror[cnt].campic < MAXTILES && testgotpic(mirror[cnt].campic)))
            {
                bIsWallMirror = false;
                if (testgotpic(cnt + MIRRORLABEL, true))
                {
                    bIsWallMirror = true;
                }
                else if ((unsigned)mirror[cnt].campic < MAXTILES && testgotpic(mirror[cnt].campic))
                {
                    gotpic.Clear(mirror[cnt].campic);
                }

                mirrorinview = true;

//                tx = interpolatedvalue(pp->oposx, pp->posx, smoothratio);
//                ty = interpolatedvalue(pp->oposy, pp->posy, smoothratio);
//                tz = interpolatedvalue(pp->oposz, pp->posz, smoothratio);
//                tpq16ang = pp->angle.ang.asq16();


                dist = 0x7fffffff;

                if (bIsWallMirror)
                {
                    j = abs(mirror[cnt].mirrorWall->wall_int_pos().X - tx);
                    j += abs(mirror[cnt].mirrorWall->wall_int_pos().Y - ty);
                    if (j < dist)
                        dist = j;
                }
                else if (mirror[cnt].camspriteActor)
                {
                    auto pos = mirror[cnt].camspriteActor->spr.pos;

                    j = abs(pos.X - tx);
                    j += abs(pos.Y - ty);
                    if (j < dist)
                        dist = j;
                }

                if (mirror[cnt].ismagic)
                {
                    int camhoriz;
                    int w;
                    int dx, dy, dz, tdx, tdy, tdz, midx, midy;

                    auto actor = mirror[cnt].cameraActor;
                    ASSERT(actor != nullptr);

                    // Calculate the angle of the mirror wall
                    auto wal = mirror[cnt].mirrorWall;

                    // Get wall midpoint for offset in mirror view
                    midx = (wal->wall_int_pos().X + wal->point2Wall()->wall_int_pos().X) / 2;
                    midy = (wal->wall_int_pos().Y + wal->point2Wall()->wall_int_pos().Y) / 2;

                    // Finish finding offsets
                    tdx = abs(midx - tx);
                    tdy = abs(midy - ty);

                    if (midx >= tx)
                        dx = actor->spr.pos.X - tdx;
                    else
                        dx = actor->spr.pos.X + tdx;

                    if (midy >= ty)
                        dy = actor->spr.pos.Y - tdy;
                    else
                        dy = actor->spr.pos.Y + tdy;

                    tdz = abs(tz - actor->spr.pos.Z);
                    if (tz >= actor->spr.pos.Z)
                        dz = actor->spr.pos.Z + tdz;
                    else
                        dz = actor->spr.pos.Z - tdz;


                    // Is it a TV cam or a teleporter that shows destination?
                    // true = It's a TV cam
                    mirror[cnt].mstate = m_normal;
                    if (TEST_BOOL1(actor))
                        mirror[cnt].mstate = m_viewon;

                    // Show teleport destination
                    // NOTE: Adding true lets you draw a room, even if
                    // you are outside of it!
                    if (mirror[cnt].mstate != m_viewon)
                    {
						tileDelete(MIRROR);
                        // Set TV camera sprite size to 0 to show mirror
                        // behind in this case!

                        if (mirror[cnt].campic != -1)
							tileDelete(mirror[cnt].campic);
                        renderDrawRoomsQ16(dx, dy, dz, tpq16ang, tpq16horiz, actor->sector(), true);
                        analyzesprites(pm_tsprite, pm_spritesortcnt, dx, dy, dz, false);
                        renderDrawMasks();
                    }
                }
                else if (mirror[cnt].mirrorWall)
                {
                    // It's just a mirror
                    // Prepare drawrooms for drawing mirror and calculate
                    // reflected
                    // position into tposx, tposy, and tang (tposz == cposz)
                    // Must call preparemirror before drawrooms and
                    // completemirror after drawrooms
					display_mirror = true;
                    renderPrepareMirror(tx, ty, tz, tpq16ang, tpq16horiz,
                                  wallnum(mirror[cnt].mirrorWall), /*mirror[cnt].mirrorsector,*/ &tposx, &tposy, &tang);

                    renderDrawRoomsQ16(tposx, tposy, tz, (tang), tpq16horiz, sectnum(mirror[cnt].mirrorSector), true);

                    analyzesprites(pm_tsprite, pm_spritesortcnt, tposx, tposy, tz, tang >> 16);
                    renderDrawMasks();

                    renderCompleteMirror();   // Reverse screen x-wise in this
					display_mirror = false;
                }

                // Clean up anything that the camera view might have done
				tileDelete(MIRROR);
                mirror[cnt].mirrorWall->overpicnum = MIRRORLABEL + cnt;
            }
            else
                mirrorinview = false;
    }
}


void SW_FloorPortalHack(DSWActor* actor, int z, int match)
{
    // move ceiling multiple of 128 so that the wall tile will line up
    int pix_diff = labs(z - actor->sector()->ceilingz) >> 8;
    int newz = actor->sector()->ceilingz - ((pix_diff / 128) + 1) * Z(128);

    SWStatIterator it(STAT_FAF);
    while ((actor = it.Next()))
    {
        if (actor->spr.lotag == match)
        {
            // move upper levels floors down for the correct view
            if (actor->spr.hitag == VIEW_LEVEL1)
            {
                // save it off
                save.sect[save.zcount] = actor->sector();
                save.zval[save.zcount] = actor->sector()->ceilingz;
                save.pic[save.zcount] = actor->sector()->ceilingpicnum;
                save.slope[save.zcount] = actor->sector()->ceilingheinum;

                actor->sector()->setceilingz(newz, true);

                // don't change FAF_MIRROR_PIC - ConnectArea
                if (actor->sector()->ceilingpicnum != FAF_MIRROR_PIC)
                    actor->sector()->ceilingpicnum = FAF_MIRROR_PIC + 1;
                actor->sector()->setceilingslope(0);

                save.zcount++;
                PRODUCTION_ASSERT(save.zcount < ZMAX);
            }
        }
    }
}

void SW_CeilingPortalHack(DSWActor* actor, int z, int match)
{
    int pix_diff = labs(z - actor->sector()->floorz) >> 8;
    int newz = actor->sector()->floorz + ((pix_diff / 128) + 1) * Z(128);

    SWStatIterator it(STAT_FAF);
    while ((actor = it.Next()))
    {
        if (actor->spr.lotag == match)
        {
            // move lower levels ceilings up for the correct view
            if (actor->spr.hitag == VIEW_LEVEL2)
            {
                // save it off
                save.sect[save.zcount] = actor->sector();
                save.zval[save.zcount] = actor->sector()->floorz;
                save.pic[save.zcount] = actor->sector()->floorpicnum;
                save.slope[save.zcount] = actor->sector()->floorheinum;

                actor->sector()->setfloorz(newz, true);
                // don't change FAF_MIRROR_PIC - ConnectArea
                if (actor->sector()->floorpicnum != FAF_MIRROR_PIC)
                    actor->sector()->floorpicnum = FAF_MIRROR_PIC + 1;
                actor->sector()->setfloorslope(0);

                save.zcount++;
                PRODUCTION_ASSERT(save.zcount < ZMAX);
            }
        }
    }

}
END_SW_NS
