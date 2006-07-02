// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jonof@edgenetwk.com)

#include "compat.h"
#include "a.h"
#include "build.h"
#include "editor.h"
#include "pragmas.h"
#include "baselayer.h"
#include "names.h"
#include "osd.h"
#include "cache1d.h"


static char tempbuf[256];

#define NUMOPTIONS 9
char option[NUMOPTIONS] = {0,0,0,0,0,0,1,0,0};
char keys[NUMBUILDKEYS] =
{
	0xc8,0xd0,0xcb,0xcd,0x2a,0x9d,0x1d,0x39,
	0x1e,0x2c,0xd1,0xc9,0x33,0x34,
	0x9c,0x1c,0xd,0xc,0xf,0x45
};



//static long hang = 0;
//static long rollangle = 0;

//Detecting 2D / 3D mode:
//   qsetmode is 200 in 3D mode
//   qsetmode is 350/480 in 2D mode
//
//You can read these variables when F5-F8 is pressed in 3D mode only:
//
//   If (searchstat == 0)  WALL        searchsector=sector, searchwall=wall
//   If (searchstat == 1)  CEILING     searchsector=sector
//   If (searchstat == 2)  FLOOR       searchsector=sector
//   If (searchstat == 3)  SPRITE      searchsector=sector, searchwall=sprite
//   If (searchstat == 4)  MASKED WALL searchsector=sector, searchwall=wall
//
//   searchsector is the sector of the selected item for all 5 searchstat's
//
//   searchwall is undefined if searchstat is 1 or 2
//   searchwall is the wall if searchstat = 0 or 4
//   searchwall is the sprite if searchstat = 3 (Yeah, I know - it says wall,
//                                      but trust me, it's the sprite number)

long averagefps;
#define AVERAGEFRAMES 32
static unsigned long frameval[AVERAGEFRAMES];
static long framecnt = 0;

char *defsfilename = "kenbuild.def";
char *startwin_labeltext = "Starting Build Editor...";
int nextvoxid = 0;

int ExtInit(void)
{
	long i, rv = 0;

	/*printf("------------------------------------------------------------------------------\n");
	printf("   BUILD.EXE copyright(c) 1996 by Ken Silverman.  You are granted the\n");
	printf("   right to use this software for your personal use only.  This is a\n");
	printf("   special version to be used with \"Happy Fun KenBuild\" and may not work\n");
	printf("   properly with other Build engine games.  Please refer to license.doc\n");
	printf("   for distribution rights\n");
	printf("------------------------------------------------------------------------------\n");
	getch();
	*/

	initgroupfile("stuff.dat");
	bpp = 8;
	if (loadsetup("build.cfg") < 0) initprintf("Configuration file not found, using defaults.\n"), rv = 1;
	Bmemcpy((void *)buildkeys,(void *)keys,NUMBUILDKEYS);   //Trick to make build use setup.dat keys
	if (option[4] > 0) option[4] = 0;
	if (initengine()) {
		wm_msgbox("Build Engine Initialisation Error",
				"There was a problem initialising the Build engine: %s", engineerrstr);
		return -1;
	}
	initinput();
	initmouse();

		//You can load your own palette lookup tables here if you just
		//copy the right code!
	for(i=0;i<256;i++)
		tempbuf[i] = ((i+32)&255);  //remap colors for screwy palette sectors
	makepalookup(16,tempbuf,0,0,0,1);

	kensplayerheight = 32;
	zmode = 0;
	defaultspritecstat = 0;
	pskyoff[0] = 0; pskyoff[1] = 0; pskybits = 1;

#ifdef SUPERBUILD
	tiletovox[PLAYER] = nextvoxid++;
	tiletovox[BROWNMONSTER] = nextvoxid++;
#endif
	
#ifdef _WIN32
//	allowtaskswitching(0);
#endif
	return rv;
}

void ExtUnInit(void)
{
	uninitgroupfile();
	writesetup("build.cfg");
}

//static long daviewingrange, daaspect, horizval1, horizval2;
void ExtPreCheckKeys(void)
{
	long /*cosang, sinang, dx, dy, mindx,*/ i, j, k;

	if (keystatus[0x3e])  //F4 - screen re-size
	{
		keystatus[0x3e] = 0;

			//cycle through all vesa modes, then screen-buffer mode
		if (keystatus[0x2a]|keystatus[0x36]) {
			setgamemode(!fullscreen, xdim, ydim, bpp);
		} else {

			//cycle through all modes
			j=-1;

			// work out a mask to select the mode
			for (i=0; i<validmodecnt; i++)
				if ((validmode[i].xdim == xdim) &&
					(validmode[i].ydim == ydim) &&
					(validmode[i].fs == fullscreen) &&
					(validmode[i].bpp == bpp))
					{ j=i; break; }
			
			for (k=0; k<validmodecnt; k++)
				if (validmode[k].fs == fullscreen && validmode[k].bpp == bpp) break;

			if (j==-1) j=k;
			else {
				j++;
				if (j==validmodecnt) j=k;
			}
			setgamemode(fullscreen,validmode[j].xdim,validmode[j].ydim,bpp);
		}
	}

#if 0
	if (keystatus[0x2a]|keystatus[0x36])
	{
		if (keystatus[0xcf]) hang = max(hang-1,-182);
		if (keystatus[0xc7]) hang = min(hang+1,182);
	}
	else
	{
		if (keystatus[0xcf]) hang = max(hang-8,-182);
		if (keystatus[0xc7]) hang = min(hang+8,182);
	}
	if (keystatus[0x4c]) { hang = 0; horiz = 100; }
	if (hang != 0)
	{
		walock[4094] = 255;

		// JBF 20031117: scale each dimension by a factor of 1.2, and work out
		// the aspect of the screen. Anywhere you see 'i' below was the value
		// '200' before I changed it. NOTE: This whole trick crashes in resolutions
		// above 800x600. I'm not sure why, and fixing it is not something I intend
		// to do in a real big hurry.
		dx = (xdim + (xdim >> 3) + (xdim >> 4) + (xdim >> 6)) & (~7);
		dy = (ydim + (ydim >> 3) + (ydim >> 4) + (ydim >> 6)) & (~7);
		i = scale(320,ydim,xdim);
		
		if (waloff[4094] == 0) allocache(&waloff[4094],/*240L*384L*/dx*dy,&walock[4094]);
		setviewtotile(4094,/*240L,384L*/dy,dx);

		cosang = sintable[(hang+512)&2047];
		sinang = sintable[hang&2047];

		dx = dmulscale1(320,cosang,i,sinang); mindx = dx;
		dy = dmulscale1(-i,cosang,320,sinang);
		horizval1 = dy*(320>>1)/dx-1;

		dx = dmulscale1(320,cosang,-i,sinang); mindx = min(dx,mindx);
		dy = dmulscale1(i,cosang,320,sinang);
		horizval2 = dy*(320>>1)/dx+1;

		daviewingrange = scale(65536,16384*(xdim>>1),mindx-16);
		daaspect = scale(daviewingrange,scale(320,tilesizx[4094],tilesizy[4094]),horizval2+6-horizval1);
		setaspect(daviewingrange,scale(daaspect,ydim*320,xdim*i));
		horiz = 100-divscale15(horizval1+horizval2,daviewingrange);
	}
#endif
}

#ifdef SUPERBUILD
#define MAXVOXMIPS 5
extern char *voxoff[][MAXVOXMIPS];
void ExtAnalyzeSprites(void)
{
	long i, *longptr;
	spritetype *tspr;

	for(i=0,tspr=&tsprite[0];i<spritesortcnt;i++,tspr++)
	{
		if (usevoxels && tiletovox[tspr->picnum] >= 0)
		{
		switch(tspr->picnum)
		{
			case PLAYER:
				if (!voxoff[ tiletovox[PLAYER] ][0]) {
					if (qloadkvx(tiletovox[PLAYER],"voxel000.kvx")) {
						tiletovox[PLAYER] = -1;
						break;
					}
				}
				//tspr->cstat |= 48; tspr->picnum = tiletovox[tspr->picnum];
				longptr = (long *)voxoff[ tiletovox[PLAYER] ][0];
				tspr->xrepeat = scale(tspr->xrepeat,56,longptr[2]);
				tspr->yrepeat = scale(tspr->yrepeat,56,longptr[2]);
				tspr->shade -= 6;
				break;
			case BROWNMONSTER:
				if (!voxoff[ tiletovox[BROWNMONSTER] ][0]) {
					if (qloadkvx(tiletovox[BROWNMONSTER],"voxel001.kvx")) {
						tiletovox[BROWNMONSTER] = -1;
						break;
					}
				}
				//tspr->cstat |= 48; tspr->picnum = tiletovox[tspr->picnum];
				break;
		}
		}

		tspr->shade += 6;
		if (sector[tspr->sectnum].ceilingstat&1)
			tspr->shade += sector[tspr->sectnum].ceilingshade;
		else
			tspr->shade += sector[tspr->sectnum].floorshade;
	}
}
#endif

void ExtCheckKeys(void)
{
	long i;//, p, y, dx, dy, cosang, sinang, bufplc, tsizy, tsizyup15;
	long j;

	if (qsetmode == 200)    //In 3D mode
	{
#if 0
		if (hang != 0)
		{
			bufplc = waloff[4094]+(mulscale16(horiz-100,xdimenscale)+(tilesizx[4094]>>1))*tilesizy[4094];
			setviewback();
			cosang = sintable[(hang+512)&2047];
			sinang = sintable[hang&2047];
			dx = dmulscale1(xdim,cosang,ydim,sinang);
			dy = dmulscale1(-ydim,cosang,xdim,sinang);

			begindrawing();
			tsizy = tilesizy[4094];
			tsizyup15 = (tsizy<<15);
			dx = mulscale14(dx,daviewingrange);
			dy = mulscale14(dy,daaspect);
			sinang = mulscale14(sinang,daviewingrange);
			cosang = mulscale14(cosang,daaspect);
			p = ylookup[windowy1]+frameplace+windowx2+1;
			for(y=windowy1;y<=windowy2;y++)
			{
				i = divscale16(tsizyup15,dx);
				stretchhline(0,(xdim>>1)*i+tsizyup15,xdim>>2,i,mulscale32(i,dy)*tsizy+bufplc,p);
				dx -= sinang; dy += cosang; p += ylookup[1];
			}
			walock[4094] = 1;

			Bsprintf(tempbuf,"%d",(hang*180)>>10);
			printext256(0L,8L,31,-1,tempbuf,1);
			enddrawing();
		}
#endif
		if (keystatus[0xa]) setaspect(viewingrange+(viewingrange>>8),yxaspect+(yxaspect>>8));
		if (keystatus[0xb]) setaspect(viewingrange-(viewingrange>>8),yxaspect-(yxaspect>>8));
		if (keystatus[0xc]) setaspect(viewingrange,yxaspect-(yxaspect>>8));
		if (keystatus[0xd]) setaspect(viewingrange,yxaspect+(yxaspect>>8));
		//if (keystatus[0x38]) setrollangle(rollangle+=((keystatus[0x2a]|keystatus[0x36])*6+2));
		//if (keystatus[0xb8]) setrollangle(rollangle-=((keystatus[0x2a]|keystatus[0x36])*6+2));
		//if (keystatus[0x1d]|keystatus[0x9d]) setrollangle(rollangle=0);

		begindrawing();
		
		i = frameval[framecnt&(AVERAGEFRAMES-1)];
		j = frameval[framecnt&(AVERAGEFRAMES-1)] = getticks(); framecnt++;
		if (i != j) averagefps = ((mul3(averagefps)+((AVERAGEFRAMES*1000)/(j-i)) )>>2);
		Bsprintf(tempbuf,"%ld",averagefps);
		printext256(0L,0L,31,-1,tempbuf,1);
		
		enddrawing();
		editinput();
	}
	else
	{
	}
}

void ExtCleanUp(void)
{
}

void ExtPreLoadMap(void)
{
}

void ExtLoadMap(const char *mapname)
{
	char title[256];
	Bsprintf(title, "BUILD by Ken Silverman - %s", mapname);
	wm_setapptitle(title);
}

void ExtPreSaveMap(void)
{
}

void ExtSaveMap(const char *mapname)
{
}

const char *ExtGetSectorCaption(short sectnum)
{
	if ((sector[sectnum].lotag|sector[sectnum].hitag) == 0)
	{
		tempbuf[0] = 0;
	}
	else
	{
		Bsprintf(tempbuf,"%hu,%hu",(unsigned short)sector[sectnum].hitag,
								  (unsigned short)sector[sectnum].lotag);
	}
	return(tempbuf);
}

const char *ExtGetWallCaption(short wallnum)
{
	if ((wall[wallnum].lotag|wall[wallnum].hitag) == 0)
	{
		tempbuf[0] = 0;
	}
	else
	{
		Bsprintf(tempbuf,"%hu,%hu",(unsigned short)wall[wallnum].hitag,
								  (unsigned short)wall[wallnum].lotag);
	}
	return(tempbuf);
}

const char *ExtGetSpriteCaption(short spritenum)
{
	if ((sprite[spritenum].lotag|sprite[spritenum].hitag) == 0)
	{
		tempbuf[0] = 0;
	}
	else
	{
		Bsprintf(tempbuf,"%hu,%hu",(unsigned short)sprite[spritenum].hitag,
								  (unsigned short)sprite[spritenum].lotag);
	}
	return(tempbuf);
}

//printext16 parameters:
//printext16(long xpos, long ypos, short col, short backcol,
//           char name[82], char fontsize)
//  xpos 0-639   (top left)
//  ypos 0-479   (top left)
//  col 0-15
//  backcol 0-15, -1 is transparent background
//  name
//  fontsize 0=8*8, 1=3*5

//drawline16 parameters:
// drawline16(long x1, long y1, long x2, long y2, char col)
//  x1, x2  0-639
//  y1, y2  0-143  (status bar is 144 high, origin is top-left of STATUS BAR)
//  col     0-15

void ExtShowSectorData(short sectnum)   //F5
{
	int i;
	if (qsetmode == 200)    //In 3D mode
	{
	}
	else
	{
		begindrawing();
		clearmidstatbar16();             //Clear middle of status bar

		Bsprintf(tempbuf,"Sector %d",sectnum);
		printext16(8,ydim16+32,11,-1,tempbuf,0);

		printext16(8,ydim16+48,11,-1,"8*8 font: ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz 0123456789",0);
		printext16(8,ydim16+56,11,-1,"3*5 font: ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz 0123456789",1);

		i=ydim16; ydim16=ydim;
		drawline16(320,i+68,344,i+80,4);       //Draw house
		drawline16(344,i+80,344,i+116,4);
		drawline16(344,i+116,296,i+116,4);
		drawline16(296,i+116,296,i+80,4);
		drawline16(296,i+80,320,i+68,4);
		ydim16=i;
		enddrawing();
	}
}

void ExtShowWallData(short wallnum)       //F6
{
	if (qsetmode == 200)    //In 3D mode
	{
	}
	else
	{
		begindrawing();
		clearmidstatbar16();             //Clear middle of status bar

		Bsprintf(tempbuf,"Wall %d",wallnum);
		printext16(8,ydim16+32,11,-1,tempbuf,0);
		enddrawing();
	}
}

void ExtShowSpriteData(short spritenum)   //F6
{
	if (qsetmode == 200)    //In 3D mode
	{
	}
	else
	{
		begindrawing();
		clearmidstatbar16();             //Clear middle of status bar

		Bsprintf(tempbuf,"Sprite %d",spritenum);
		printext16(8,ydim16+32,11,-1,tempbuf,0);
		enddrawing();
	}
}

void ExtEditSectorData(short sectnum)    //F7
{
	short nickdata;

	if (qsetmode == 200)    //In 3D mode
	{
			//Ceiling
		if (searchstat == 1)
			sector[searchsector].ceilingpicnum++;   //Just a stupid example

			//Floor
		if (searchstat == 2)
			sector[searchsector].floorshade++;      //Just a stupid example
	}
	else                    //In 2D mode
	{
		Bsprintf(tempbuf,"Sector (%d) Nick's variable: ",sectnum);
		nickdata = 0;
		nickdata = getnumber16(tempbuf,nickdata,65536L,0);

		printmessage16("");              //Clear message box (top right of status bar)
		ExtShowSectorData(sectnum);
	}
}

void ExtEditWallData(short wallnum)       //F8
{
	short nickdata;

	if (qsetmode == 200)    //In 3D mode
	{
	}
	else
	{
		Bsprintf(tempbuf,"Wall (%d) Nick's variable: ",wallnum);
		nickdata = 0;
		nickdata = getnumber16(tempbuf,nickdata,65536L,0);

		printmessage16("");              //Clear message box (top right of status bar)
		ExtShowWallData(wallnum);
	}
}

void ExtEditSpriteData(short spritenum)   //F8
{
	short nickdata;

	if (qsetmode == 200)    //In 3D mode
	{
	}
	else
	{
		Bsprintf(tempbuf,"Sprite (%d) Nick's variable: ",spritenum);
		nickdata = 0;
		nickdata = getnumber16(tempbuf,nickdata,65536L,0);
		printmessage16("");

		printmessage16("");              //Clear message box (top right of status bar)
		ExtShowSpriteData(spritenum);
	}
}

void faketimerhandler(void)
{
	sampletimer();
}

	//Just thought you might want my getnumber16 code
/*
getnumber16(char namestart[80], short num, long maxnumber)
{
	char buffer[80];
	long j, k, n, danum, oldnum;

	danum = (long)num;
	oldnum = danum;
	while ((keystatus[0x1c] != 2) && (keystatus[0x1] == 0))  //Enter, ESC
	{
		sprintf(&buffer,"%s%ld_ ",namestart,danum);
		printmessage16(buffer);

		for(j=2;j<=11;j++)                //Scan numbers 0-9
			if (keystatus[j] > 0)
			{
				keystatus[j] = 0;
				k = j-1;
				if (k == 10) k = 0;
				n = (danum*10)+k;
				if (n < maxnumber) danum = n;
			}
		if (keystatus[0xe] > 0)    // backspace
		{
			danum /= 10;
			keystatus[0xe] = 0;
		}
		if (keystatus[0x1c] == 1)   //L. enter
		{
			oldnum = danum;
			keystatus[0x1c] = 2;
			asksave = 1;
		}
	}
	keystatus[0x1c] = 0;
	keystatus[0x1] = 0;
	return((short)oldnum);
}
*/

/*
 * vim:ts=4:
 */

