BEGIN_DUKE_NS

extern int tempsectorz[MAXSECTORS];
extern int tempsectorpicnum[MAXSECTORS];


void SE40_Draw(int tag, spritetype *spr, int x, int y, int z, binangle a, fixedhoriz h, int smoothratio)
{
	int i, j = 0, k = 0;
	int ok = 0, fofmode = 0;
	int offx, offy;
	spritetype* floor1, *floor2 = nullptr;

	if (spr->ang != 512) return;

	i = FOF;    //Effect TILE
	tileDelete(FOF);
	if (!(gotpic[i >> 3] & (1 << (i & 7)))) return;
	gotpic[i >> 3] &= ~(1 << (i & 7));

	floor1 = spr;

	if (spr->lotag == tag + 2) fofmode = tag + 0;
	if (spr->lotag == tag + 3) fofmode = tag + 1;
	if (spr->lotag == tag + 4) fofmode = tag + 0;
	if (spr->lotag == tag + 5) fofmode = tag + 1;

	ok++;

	DukeStatIterator it(STAT_RAROR);
	while (auto act = it.Next())
	{
		auto spr = &act->s;
		if (
			spr->picnum == SECTOREFFECTOR &&
			spr->lotag == fofmode &&
			spr->hitag == floor1->hitag
			) 
		{
			floor1 = spr; 
			fofmode = spr->lotag; 
			ok++; 
			break;
		}
	}
	// if(ok==1) { Message("no floor1",RED); return; }

	if (fofmode == tag + 0) k = tag + 1; else k = tag + 0;

	it.Reset(STAT_RAROR);
	while (auto act = it.Next())
	{
		auto spr = &act->s;
		if (
			spr->picnum == SECTOREFFECTOR &&
			spr->lotag == k &&
			spr->hitag == floor1->hitag
			) 
		{
			floor2 = spr; 
			ok++; 
			break;
		}
	}

	// if(ok==2) { Message("no floor2",RED); return; }

	it.Reset(STAT_RAROR);
	while (auto act = it.Next())
	{
		auto spr = &act->s;
		if (spr->picnum == SECTOREFFECTOR &&
			spr->lotag == k + 2 &&
			spr->hitag == floor1->hitag
			)
		{
			if (k == tag + 0)
			{
				tempsectorz[spr->sectnum] = sector[spr->sectnum].floorz;
				sector[spr->sectnum].floorz += (((z - sector[spr->sectnum].floorz) / 32768) + 1) * 32768;
				tempsectorpicnum[spr->sectnum] = sector[spr->sectnum].floorpicnum;
				sector[spr->sectnum].floorpicnum = 13;
			}
			if (k == tag + 1)
			{
				tempsectorz[spr->sectnum] = sector[spr->sectnum].ceilingz;
				sector[spr->sectnum].ceilingz += (((z - sector[spr->sectnum].ceilingz) / 32768) - 1) * 32768;
				tempsectorpicnum[spr->sectnum] = sector[spr->sectnum].ceilingpicnum;
				sector[spr->sectnum].ceilingpicnum = 13;
			}
		}
	}

	offx = x - floor1->x;
	offy = y - floor1->y;

	renderDrawRoomsQ16(floor2->x + offx, floor2->y + offy, z, a.asq16(), h.asq16(), floor2->sectnum);
	fi.animatesprites(pm_tsprite, pm_spritesortcnt, offx + floor2->x, offy + floor2->y, a.asbuild(), smoothratio);
	renderDrawMasks();

	it.Reset(STAT_RAROR);
	while (auto act = it.Next())
	{
		auto spr = &act->s;
		if (spr->picnum == 1 &&
			spr->lotag == k + 2 &&
			spr->hitag == floor1->hitag
			)
		{
			if (k == tag + 0)
			{
				sector[spr->sectnum].floorz = tempsectorz[spr->sectnum];
				sector[spr->sectnum].floorpicnum = tempsectorpicnum[spr->sectnum];
			}
			if (k == tag + 1)
			{
				sector[spr->sectnum].ceilingz = tempsectorz[spr->sectnum];
				sector[spr->sectnum].ceilingpicnum = tempsectorpicnum[spr->sectnum];
			}
		}// end if
	}// end for

} // end SE40


//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

void se40code(int x, int y, int z, binangle a, fixedhoriz h, int smoothratio)
{
	int tag;
	if (!isRR()) tag = 40;
	else if (isRRRA()) tag = 150;
	else return;

	DukeStatIterator it(STAT_RAROR);
	while (auto act = it.Next())
	{
		switch (act->s.lotag - tag + 40)
		{
			//            case 40:
			//            case 41:
			//                SE40_Draw(i,x,y,a,smoothratio);
			//                break;
		case 42:
		case 43:
		case 44:
		case 45:
			if (ps[screenpeek].cursectnum == act->s.sectnum)
				SE40_Draw(tag, &act->s, x, y, z, a, h, smoothratio);
			break;
		}
	}
}


//---------------------------------------------------------------------------
//
// split out so it can also be applied to camera views
//
//---------------------------------------------------------------------------

void renderMirror(int cposx, int cposy, int cposz, binangle cang, fixedhoriz choriz, int smoothratio)
{
	if ((gotpic[TILE_MIRROR >> 3] & (1 << (TILE_MIRROR & 7))) > 0)
	{
		int dst = 0x7fffffff, i = 0;
		for (int k = 0; k < mirrorcnt; k++)
		{
			int j = abs(wall[mirrorwall[k]].x - cposx) + abs(wall[mirrorwall[k]].y - cposy);
			if (j < dst) dst = j, i = k;
		}

		if (wall[mirrorwall[i]].overpicnum == TILE_MIRROR)
		{
			int tposx, tposy;
			fixed_t tang;

			renderPrepareMirror(cposx, cposy, cposz, cang.asq16(), choriz.asq16(), mirrorwall[i], &tposx, &tposy, &tang);

			int j = g_visibility;
			g_visibility = (j >> 1) + (j >> 2);

			renderDrawRoomsQ16(tposx, tposy, cposz, tang, choriz.asq16(), mirrorsector[i] + MAXSECTORS);

			display_mirror = 1;
			fi.animatesprites(pm_tsprite, pm_spritesortcnt, tposx, tposy, tang, smoothratio);
			display_mirror = 0;

			renderDrawMasks();
			renderCompleteMirror();   //Reverse screen x-wise in this function
			g_visibility = j;
		}
		gotpic[TILE_MIRROR >> 3] &= ~(1 << (TILE_MIRROR & 7));
	}
}

//---------------------------------------------------------------------------
//
// used by RR to inject some external geometry into a scene. 
//
//---------------------------------------------------------------------------

static void geometryEffect(int cposx, int cposy, int cposz, binangle cang, fixedhoriz choriz, int sect, int smoothratio)
{
	short gs, tgsect, geosect, geoid = 0;
	renderDrawRoomsQ16(cposx, cposy, cposz, cang.asq16(), choriz.asq16(), sect);
	fi.animatesprites(pm_tsprite, pm_spritesortcnt, cposx, cposy, cang.asbuild(), smoothratio);
	renderDrawMasks();
	for (gs = 0; gs < geocnt; gs++)
	{
		tgsect = geosector[gs];

		DukeSectIterator it(tgsect);
		while (auto act = it.Next())
		{
			changespritesect(act, geosectorwarp[gs]);
			setsprite(act, act->s.x -= geox[gs], act->s.y -= geoy[gs], act->s.z);
		}
		if (geosector[gs] == sect)
		{
			geosect = geosectorwarp[gs];
			geoid = gs;
		}
	}
	cposx -= geox[geoid];
	cposy -= geoy[geoid];
	renderDrawRoomsQ16(cposx, cposy, cposz, cang.asq16(), choriz.asq16(), sect);
	cposx += geox[geoid];
	cposy += geoy[geoid];
	for (gs = 0; gs < geocnt; gs++)
	{
		tgsect = geosectorwarp[gs];
		DukeSectIterator it(tgsect);
		while (auto act = it.Next())
		{
			changespritesect(act, geosector[gs]);
			setsprite(act, act->s.x += geox[gs], act->s.y += geoy[gs], act->s.z);
		}
	}
	fi.animatesprites(pm_tsprite, pm_spritesortcnt, cposx, cposy, cang.asbuild(), smoothratio);
	renderDrawMasks();
	for (gs = 0; gs < geocnt; gs++)
	{
		tgsect = geosector[gs];
		DukeSectIterator it(tgsect);
		while (auto act = it.Next())
		{
			changespritesect(act, geosectorwarp2[gs]);
			setsprite(act, act->s.x -= geox2[gs], act->s.y -= geoy2[gs], act->s.z);
		}
		if (geosector[gs] == sect)
		{
			geosect = geosectorwarp2[gs];
			geoid = gs;
		}
	}
	cposx -= geox2[geoid];
	cposy -= geoy2[geoid];
	renderDrawRoomsQ16(cposx, cposy, cposz, cang.asq16(), choriz.asq16(), sect);
	cposx += geox2[geoid];
	cposy += geoy2[geoid];
	for (gs = 0; gs < geocnt; gs++)
	{
		tgsect = geosectorwarp2[gs];
		DukeSectIterator it(tgsect);
		while (auto act = it.Next())
		{
			changespritesect(act, geosector[gs]);
			setsprite(act, act->s.x += geox2[gs], act->s.y += geoy2[gs], act->s.z);
		}
	}
	fi.animatesprites(pm_tsprite, pm_spritesortcnt, cposx, cposy, cang.asbuild(), smoothratio);
	renderDrawMasks();
}
END_DUKE_NS
