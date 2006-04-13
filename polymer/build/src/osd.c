// On-screen Display (ie. console)
// for the Build Engine
// by Jonathon Fowler (jonof@edgenetwk.com)

#include "build.h"
#include "osd.h"
#include "compat.h"
#include "baselayer.h"


typedef struct _symbol {
	const char *name;
	struct _symbol *next;

	const char *help;
	int (*func)(const osdfuncparm_t *);
} symbol_t;

static symbol_t *symbols = NULL;
static symbol_t *addnewsymbol(const char *name);
static symbol_t *findsymbol(const char *name, symbol_t *startingat);
static symbol_t *findexactsymbol(const char *name);

static int _validate_osdlines(void *);

static int _internal_osdfunc_listsymbols(const osdfuncparm_t *);
static int _internal_osdfunc_help(const osdfuncparm_t *);
static int _internal_osdfunc_dumpbuildinfo(const osdfuncparm_t *);
static int _internal_osdfunc_setrendermode(const osdfuncparm_t *);

static int white=-1;			// colour of white (used by default display routines)
static void _internal_drawosdchar(int, int, char, int, int);
static void _internal_drawosdstr(int, int, char*, int, int, int);
static void _internal_drawosdcursor(int,int,int,int);
static int _internal_getcolumnwidth(int);
static int _internal_getrowheight(int);
static void _internal_clearbackground(int,int);
static int _internal_gettime(void);
static void _internal_onshowosd(int);

#define TEXTSIZE 16384

// history display
static char osdtext[TEXTSIZE];
static int  osdpos=0;			// position next character will be written at
static int  osdlines=1;			// # lines of text in the buffer
static int  osdrows=20;			// # lines of the buffer that are visible
static int  osdcols=60;			// width of onscreen display in text columns
static int  osdmaxrows=20;		// maximum number of lines which can fit on the screen
static int  osdmaxlines=TEXTSIZE/60;	// maximum lines which can fit in the buffer
static char osdvisible=0;		// onscreen display visible?
static int  osdhead=0; 			// topmost visible line number
static BFILE *osdlog=NULL;		// log filehandle
static char osdinited=0;		// text buffer initialised?
static int  osdkey=0x45;		// numlock shows the osd
static int  keytime=0;

// command prompt editing
#define EDITLENGTH 512
static int  osdovertype=0;		// insert (0) or overtype (1)
static char osdeditbuf[EDITLENGTH+1];	// editing buffer
static char osdedittmp[EDITLENGTH+1];	// editing buffer temporary workspace
static int  osdeditlen=0;		// length of characters in edit buffer
static int  osdeditcursor=0;		// position of cursor in edit buffer
static int  osdeditshift=0;		// shift state
static int  osdeditcontrol=0;		// control state
static int  osdeditcaps=0;		// capslock
static int  osdeditwinstart=0;
static int  osdeditwinend=60-1-3;
#define editlinewidth (osdcols-1-3)

// command processing
#define HISTORYDEPTH 16
static int  osdhistorypos=-1;		// position we are at in the history buffer
static int  osdhistorybuf[HISTORYDEPTH][EDITLENGTH+1];	// history strings
static int  osdhistorysize=0;		// number of entries in history

// execution buffer
// the execution buffer works from the command history
static int  osdexeccount=0;		// number of lines from the head of the history buffer to execute

// presentation parameters
static int  osdpromptshade=0;
static int  osdpromptpal=0;
static int  osdeditshade=0;
static int  osdeditpal=0;
static int  osdtextshade=0;
static int  osdtextpal=0;
static int  osdcursorshade=0;
static int  osdcursorpal=0;

// application callbacks
static void (*drawosdchar)(int, int, char, int, int) = _internal_drawosdchar;
static void (*drawosdstr)(int, int, char*, int, int, int) = _internal_drawosdstr;
static void (*drawosdcursor)(int, int, int, int) = _internal_drawosdcursor;
static int (*getcolumnwidth)(int) = _internal_getcolumnwidth;
static int (*getrowheight)(int) = _internal_getrowheight;
static void (*clearbackground)(int,int) = _internal_clearbackground;
static int (*gettime)(void) = _internal_gettime;
static void (*onshowosd)(int) = _internal_onshowosd;


static void _internal_drawosdchar(int x, int y, char ch, int shade, int pal)
{
	int i,j,k;
	char st[2] = { 0,0 };

	st[0] = ch;

	if (white<0) {
		// find the palette index closest to white
		k=0;
		for(i=0;i<256;i++)
		{
			j = ((int)curpalette[i].r)+((int)curpalette[i].g)+((int)curpalette[i].b);
			if (j > k) { k = j; white = i; }
		}
	}

	printext256(4+(x<<3),4+(y<<3), white, -1, st, 0);
}

static void _internal_drawosdstr(int x, int y, char *ch, int len, int shade, int pal)
{
	int i,j,k;
	char st[1024];

	if (len>1023) len=1023;
	memcpy(st,ch,len);
	st[len]=0;
	
	if (white<0) {
		// find the palette index closest to white
		k=0;
		for(i=0;i<256;i++)
		{
			j = ((int)curpalette[i].r)+((int)curpalette[i].g)+((int)curpalette[i].b);
			if (j > k) { k = j; white = i; }
		}
	}

	printext256(4+(x<<3),4+(y<<3), white, -1, st, 0);
}

static void _internal_drawosdcursor(int x, int y, int type, int lastkeypress)
{
	int i,j,k;
	char st[2] = { '_',0 };

	if (type) st[0] = '#';

	if (white<0) {
		// find the palette index closest to white
		k=0;
		for(i=0;i<256;i++)
		{
			j = ((int)palette[i*3])+((int)palette[i*3+1])+((int)palette[i*3+2]);
			if (j > k) { k = j; white = i; }
		}
	}

	printext256(4+(x<<3),4+(y<<3)+2, white, -1, st, 0);
}

static int _internal_getcolumnwidth(int w)
{
	return w/8 - 1;
}

static int _internal_getrowheight(int w)
{
	return w/8;
}

static void _internal_clearbackground(int cols, int rows)
{
}

static int _internal_gettime(void)
{
	return 0;
}

static void _internal_onshowosd(int a)
{
}

////////////////////////////

static int _internal_osdfunc_vars(const osdfuncparm_t *parm)
{
	int showval = (parm->numparms < 1);

	if (!Bstrcasecmp(parm->name, "osdrows")) {
		if (showval) { OSD_Printf("osdrows is %d\n", osdrows); return OSDCMD_OK; }
		else {
			osdrows = atoi(parm->parms[0]);
			if (osdrows < 1) osdrows = 1;
			else if (osdrows > osdmaxrows) osdrows = osdmaxrows;
			return OSDCMD_OK;
		}
	}
	return OSDCMD_SHOWHELP;
}

static int _internal_osdfunc_listsymbols(const osdfuncparm_t *parm)
{
	symbol_t *i;

	OSD_Printf("Symbol listing:\n");
	for (i=symbols; i!=NULL; i=i->next)
		OSD_Printf("     %s\n", i->name);
	
	return OSDCMD_OK;
}

static int _internal_osdfunc_help(const osdfuncparm_t *parm)
{
	symbol_t *symb;

	if (parm->numparms != 1) return OSDCMD_SHOWHELP;
	symb = findexactsymbol(parm->parms[0]);
	if (!symb) {
		OSD_Printf("Help Error: \"%s\" is not a defined variable or function\n", parm->parms[0]);
	} else {
		OSD_Printf("%s\n", symb->help);
	}
	
	return OSDCMD_OK;
}



////////////////////////////


//
// OSD_Cleanup() -- Cleans up the on-screen display
//
void OSD_Cleanup(void)
{
	symbol_t *s;

	for (; symbols; symbols=s) {
		s=symbols->next;
		Bfree(symbols);
	}

	osdinited=0;
}


//
// OSD_Init() -- Initialises the on-screen display
//
void OSD_Init(void)
{
	Bmemset(osdtext, 32, TEXTSIZE);
	osdlines=1;

	osdinited=1;

	OSD_RegisterFunction("listsymbols","listsymbols: lists all the recognized symbols",_internal_osdfunc_listsymbols);
	OSD_RegisterFunction("help","help: displays help on the named symbol",_internal_osdfunc_help);
	OSD_RegisterFunction("osdrows","osdrows: sets the number of visible lines of the OSD",_internal_osdfunc_vars);

	atexit(OSD_Cleanup);
}


//
// OSD_SetLogFile() -- Sets the text file where printed text should be echoed
//
void OSD_SetLogFile(char *fn)
{
	if (osdlog) Bfclose(osdlog);
	osdlog = NULL;
	if (fn) osdlog = Bfopen(fn,"w");
	if (osdlog) setvbuf(osdlog, (char*)NULL, _IONBF, 0);
}


//
// OSD_SetFunctions() -- Sets some callbacks which the OSD uses to understand its world
//
void OSD_SetFunctions(
		void (*drawchar)(int,int,char,int,int),
		void (*drawstr)(int,int,char*,int,int,int),
		void (*drawcursor)(int,int,int,int),
		int (*colwidth)(int),
		int (*rowheight)(int),
		void (*clearbg)(int,int),
		int (*gtime)(void),
		void (*showosd)(int)
	)
{
	drawosdchar = drawchar;
	drawosdstr = drawstr;
	drawosdcursor = drawcursor;
	getcolumnwidth = colwidth;
	getrowheight = rowheight;
	clearbackground = clearbg;
	gettime = gtime;
	onshowosd = showosd;

	if (!drawosdchar) drawosdchar = _internal_drawosdchar;
	if (!drawosdstr) drawosdstr = _internal_drawosdstr;
	if (!drawosdcursor) drawosdcursor = _internal_drawosdcursor;
	if (!getcolumnwidth) getcolumnwidth = _internal_getcolumnwidth;
	if (!getrowheight) getrowheight = _internal_getrowheight;
	if (!clearbackground) clearbackground = _internal_clearbackground;
	if (!gettime) gettime = _internal_gettime;
	if (!onshowosd) onshowosd = _internal_onshowosd;
}


//
// OSD_SetParameters() -- Sets the parameters for presenting the text
//
void OSD_SetParameters(
		int promptshade, int promptpal,
		int editshade, int editpal,
		int textshade, int textpal
	)
{
	osdpromptshade = promptshade;
	osdpromptpal   = promptpal;
	osdeditshade   = editshade;
	osdeditpal     = editpal;
	osdtextshade   = textshade;
	osdtextpal     = textpal;
}


//
// OSD_CaptureKey() -- Sets the scancode for the key which activates the onscreen display
//
void OSD_CaptureKey(int sc)
{
	osdkey = sc;
}


//
// OSD_HandleKey() -- Handles keyboard input when capturing input.
// 	Returns 0 if the key was handled internally, or the scancode if it should
// 	be passed on to the game.
//
int OSD_HandleKey(int sc, int press)
{
	char ch;
	int i,j;
	symbol_t *tabc = NULL;
	static symbol_t *lastmatch = NULL;
	
	if (!osdinited) return sc;

	if (sc == osdkey) {
		if (press) {
			OSD_ShowDisplay(osdvisible ^ 1);
			bflushchars();
		}
		return 0;//sc;
	} else if (!osdvisible) {
		return sc;
	}

	if (!press) {
		if (sc == 42 || sc == 54) // shift
			osdeditshift = 0;
		if (sc == 29 || sc == 157)	// control
			osdeditcontrol = 0;
		return 0;//sc;
	}

	keytime = gettime();

	if (sc != 15) lastmatch = NULL;		// tab

	while ( (ch = bgetchar()) ) {
		if (ch == 1) {	// control a. jump to beginning of line
		} else if (ch == 2) {	// control b, move one character left
		} else if (ch == 5) {	// control e, jump to end of line
		} else if (ch == 6) {	// control f, move one character right
		} else if (ch == 8 || ch == 127) {	// control h, backspace
			if (!osdeditcursor || !osdeditlen) return 0;
			if (!osdovertype) {
				if (osdeditcursor < osdeditlen)
					Bmemmove(osdeditbuf+osdeditcursor-1, osdeditbuf+osdeditcursor, osdeditlen-osdeditcursor);
				osdeditlen--;
			}
			osdeditcursor--;
			if (osdeditcursor<osdeditwinstart) osdeditwinstart--,osdeditwinend--;
		} else if (ch == 9) {	// tab
			if (!lastmatch) {
				for (i=osdeditcursor;i>0;i--) if (osdeditbuf[i-1] == ' ') break;
				for (j=0;osdeditbuf[i] != ' ' && i < osdeditlen;j++,i++)
					osdedittmp[j] = osdeditbuf[i];
				osdedittmp[j] = 0;

				if (j > 0)
					tabc = findsymbol(osdedittmp, NULL);
			} else {
				tabc = findsymbol(osdedittmp, lastmatch->next);
				if (!tabc && lastmatch)
					tabc = findsymbol(osdedittmp, NULL);	// wrap
			}

			if (tabc) {
				for (i=osdeditcursor;i>0;i--) if (osdeditbuf[i-1] == ' ') break;
				osdeditlen = i;
				for (j=0;tabc->name[j] && osdeditlen <= EDITLENGTH;i++,j++,osdeditlen++)
					osdeditbuf[i] = tabc->name[j];
				osdeditcursor = osdeditlen;
				osdeditwinend = osdeditcursor;
				osdeditwinstart = osdeditwinend-editlinewidth;
				if (osdeditwinstart<0) {
					osdeditwinstart=0;
					osdeditwinend = editlinewidth;
				}
			
				lastmatch = tabc;
			}
		} else if (ch == 11) {	// control k, delete all to end of line
		} else if (ch == 12) {	// control l, clear screen
		} else if (ch == 13) {	// control m, enter
			if (osdeditlen>0) {
				osdeditbuf[osdeditlen] = 0;
				Bmemmove(osdhistorybuf[1], osdhistorybuf[0], HISTORYDEPTH*(EDITLENGTH+1));
				Bmemmove(osdhistorybuf[0], osdeditbuf, EDITLENGTH+1);
				if (osdhistorysize < HISTORYDEPTH) osdhistorysize++;
				if (osdexeccount == HISTORYDEPTH)
					OSD_Printf("Command Buffer Warning: Failed queueing command "
							"for execution. Buffer full.\n");
				else
					osdexeccount++;
				osdhistorypos=-1;
			}

			osdeditlen=0;
			osdeditcursor=0;
			osdeditwinstart=0;
			osdeditwinend=editlinewidth;
		} else if (ch == 16) {	// control p, previous (ie. up arrow)
		} else if (ch == 20) {	// control t, swap previous two chars
		} else if (ch == 21) {	// control u, delete all to beginning
			if (osdeditcursor>0 && osdeditlen) {
				if (osdeditcursor<osdeditlen)
					Bmemmove(osdeditbuf, osdeditbuf+osdeditcursor, osdeditlen-osdeditcursor);
				osdeditlen-=osdeditcursor;
				osdeditcursor = 0;
				osdeditwinstart = 0;
				osdeditwinend = editlinewidth;
			}
		} else if (ch == 23) {	// control w, delete one word back
			if (osdeditcursor>0 && osdeditlen>0) {
				i=osdeditcursor;
				while (i>0 && osdeditbuf[i-1]==32) i--;
				while (i>0 && osdeditbuf[i-1]!=32) i--;
				if (osdeditcursor<osdeditlen)
					Bmemmove(osdeditbuf+i, osdeditbuf+osdeditcursor, osdeditlen-osdeditcursor);
				osdeditlen -= (osdeditcursor-i);
				osdeditcursor = i;
				if (osdeditcursor < osdeditwinstart) {
					osdeditwinstart=osdeditcursor;
					osdeditwinend=osdeditwinstart+editlinewidth;
				}
			}
		} else if (ch >= 32) {	// text char
			if (!osdovertype && osdeditlen == EDITLENGTH)	// buffer full, can't insert another char
				return 0;

			if (!osdovertype) {
				if (osdeditcursor < osdeditlen) 
					Bmemmove(osdeditbuf+osdeditcursor+1, osdeditbuf+osdeditcursor, osdeditlen-osdeditcursor);
				osdeditlen++;
			} else {
				if (osdeditcursor == osdeditlen)
					osdeditlen++;
			}
			osdeditbuf[osdeditcursor] = ch;
			osdeditcursor++;
			if (osdeditcursor>osdeditwinend) osdeditwinstart++,osdeditwinend++;
		}
	}

	if (sc == 15) {		// tab
	} else if (sc == 1) {		// escape
		OSD_ShowDisplay(0);
	} else if (sc == 201) {	// page up
		if (osdhead < osdlines-1)
			osdhead++;
	} else if (sc == 209) {	// page down
		if (osdhead > 0)
			osdhead--;
	} else if (sc == 199) {	// home
		if (osdeditcontrol) {
			osdhead = osdlines-1;
		} else {
			osdeditcursor = 0;
			osdeditwinstart = osdeditcursor;
			osdeditwinend = osdeditwinstart+editlinewidth;
		}
	} else if (sc == 207) {	// end
		if (osdeditcontrol) {
			osdhead = 0;
		} else {
			osdeditcursor = osdeditlen;
			osdeditwinend = osdeditcursor;
			osdeditwinstart = osdeditwinend-editlinewidth;
			if (osdeditwinstart<0) {
				osdeditwinstart=0;
				osdeditwinend = editlinewidth;
			}
		}
	} else if (sc == 210) {	// insert
		osdovertype ^= 1;
	} else if (sc == 203) {	// left
		if (osdeditcursor>0) {
			if (osdeditcontrol) {
				while (osdeditcursor>0) {
					if (osdeditbuf[osdeditcursor-1] != 32) break;
					osdeditcursor--;
				}
				while (osdeditcursor>0) {
					if (osdeditbuf[osdeditcursor-1] == 32) break;
					osdeditcursor--;
				}
			} else osdeditcursor--;
		}
		if (osdeditcursor<osdeditwinstart)
			osdeditwinend-=(osdeditwinstart-osdeditcursor),
			osdeditwinstart-=(osdeditwinstart-osdeditcursor);
	} else if (sc == 205) {	// right
		if (osdeditcursor<osdeditlen) {
			if (osdeditcontrol) {
				while (osdeditcursor<osdeditlen) {
					if (osdeditbuf[osdeditcursor] == 32) break;
					osdeditcursor++;
				}
				while (osdeditcursor<osdeditlen) {
					if (osdeditbuf[osdeditcursor] != 32) break;
					osdeditcursor++;
				}
			} else osdeditcursor++;
		}
		if (osdeditcursor>=osdeditwinend)
			osdeditwinstart+=(osdeditcursor-osdeditwinend),
			osdeditwinend+=(osdeditcursor-osdeditwinend);
	} else if (sc == 200) {	// up
		if (osdhistorypos < osdhistorysize-1) {
			osdhistorypos++;
			memcpy(osdeditbuf, osdhistorybuf[osdhistorypos], EDITLENGTH+1);
			osdeditlen = osdeditcursor = 0;
			while (osdeditbuf[osdeditcursor]) osdeditlen++, osdeditcursor++;
			if (osdeditcursor<osdeditwinstart) {
				osdeditwinend = osdeditcursor;
				osdeditwinstart = osdeditwinend-editlinewidth;
				
				if (osdeditwinstart<0)
					osdeditwinend-=osdeditwinstart,
					osdeditwinstart=0;
			} else if (osdeditcursor>=osdeditwinend)
				osdeditwinstart+=(osdeditcursor-osdeditwinend),
				osdeditwinend+=(osdeditcursor-osdeditwinend);
		}
	} else if (sc == 208) {	// down
		if (osdhistorypos >= 0) {
			if (osdhistorypos == 0) {
				osdeditlen=0;
				osdeditcursor=0;
				osdeditwinstart=0;
				osdeditwinend=editlinewidth;
				osdhistorypos = -1;
			} else {
				osdhistorypos--;
				memcpy(osdeditbuf, osdhistorybuf[osdhistorypos], EDITLENGTH+1);
				osdeditlen = osdeditcursor = 0;
				while (osdeditbuf[osdeditcursor]) osdeditlen++, osdeditcursor++;
				if (osdeditcursor<osdeditwinstart) {
					osdeditwinend = osdeditcursor;
					osdeditwinstart = osdeditwinend-editlinewidth;
					
					if (osdeditwinstart<0)
						osdeditwinend-=osdeditwinstart,
						osdeditwinstart=0;
				} else if (osdeditcursor>=osdeditwinend)
					osdeditwinstart+=(osdeditcursor-osdeditwinend),
					osdeditwinend+=(osdeditcursor-osdeditwinend);
			}
		}
	} else if (sc == 42 || sc == 54) {	// shift
		osdeditshift = 1;
	} else if (sc == 29 || sc == 157) {	// control
		osdeditcontrol = 1;
	} else if (sc == 58) {	// capslock
		osdeditcaps ^= 1;
	} else if (sc == 28 || sc == 156) {	// enter
	} else if (sc == 14) {		// backspace
	} else if (sc == 211) {	// delete
		if (osdeditcursor == osdeditlen || !osdeditlen) return 0;
		if (osdeditcursor <= osdeditlen-1) Bmemmove(osdeditbuf+osdeditcursor, osdeditbuf+osdeditcursor+1, osdeditlen-osdeditcursor-1);
		osdeditlen--;
	}
	
	return 0;
}


//
// OSD_ResizeDisplay() -- Handles readjustment of the display when the screen resolution
// 	changes on us.
//
void OSD_ResizeDisplay(int w, int h)
{
	int newcols;
	int newmaxlines;
	char newtext[TEXTSIZE];
	int i,j,k;

	newcols = getcolumnwidth(w);
	newmaxlines = TEXTSIZE / newcols;

	j = min(newmaxlines, osdmaxlines);
	k = min(newcols, osdcols);

	memset(newtext, 32, TEXTSIZE);
	for (i=0;i<j;i++) {
		memcpy(newtext+newcols*i, osdtext+osdcols*i, k);
	}

	memcpy(osdtext, newtext, TEXTSIZE);
	osdcols = newcols;
	osdmaxlines = newmaxlines;
	osdmaxrows = getrowheight(h)-2;
	
	if (osdrows > osdmaxrows) osdrows = osdmaxrows;
	
	osdpos = 0;
	osdhead = 0;
	osdeditwinstart = 0;
	osdeditwinend = editlinewidth;
	white = -1;
}


//
// OSD_ShowDisplay() -- Shows or hides the onscreen display
//
void OSD_ShowDisplay(int onf)
{
	osdvisible = (onf != 0);
	osdeditcontrol = 0;
	osdeditshift = 0;

	grabmouse(osdvisible == 0);
	onshowosd(osdvisible);
	if (osdvisible) releaseallbuttons();
}


//
// OSD_Draw() -- Draw the onscreen display
//
void OSD_Draw(void)
{
	unsigned topoffs;
	int row, lines, x, len;
	
	if (!osdvisible || !osdinited) return;

	topoffs = osdhead * osdcols;
	row = osdrows-1;
	lines = min( osdlines-osdhead, osdrows );
	
	begindrawing();

	clearbackground(osdcols,osdrows+1);

	for (; lines>0; lines--, row--) {
		drawosdstr(0,row,osdtext+topoffs,osdcols,osdtextshade,osdtextpal);
		topoffs+=osdcols;
	}

	drawosdchar(2,osdrows,'>',osdpromptshade,osdpromptpal);
	if (osdeditcaps) drawosdchar(0,osdrows,'C',osdpromptshade,osdpromptpal);
	if (osdeditshift) drawosdchar(1,osdrows,'H',osdpromptshade,osdpromptpal);
	
	len = min(osdcols-1-3, osdeditlen-osdeditwinstart);
	for (x=0; x<len; x++)
		drawosdchar(3+x,osdrows,osdeditbuf[osdeditwinstart+x],osdeditshade,osdeditpal);
	
	drawosdcursor(3+osdeditcursor-osdeditwinstart,osdrows,osdovertype,keytime);

	enddrawing();
}


//
// OSD_Printf() -- Print a string to the onscreen display
//   and write it to the log file
//

static inline void linefeed(void)
{
	Bmemmove(osdtext+osdcols, osdtext, TEXTSIZE-osdcols);
	Bmemset(osdtext, 32, osdcols);

	if (osdlines < osdmaxlines) osdlines++;
}

void OSD_Printf(const char *fmt, ...)
{
	char tmpstr[1024], *chp;
	va_list va;
		
	if (!osdinited) OSD_Init();

	va_start(va, fmt);
	Bvsnprintf(tmpstr, 1024, fmt, va);
	va_end(va);

	if (osdlog) Bfputs(tmpstr, osdlog);

	for (chp = tmpstr; *chp; chp++) {
		if (*chp == '\r') osdpos=0;
		else if (*chp == '\n') {
			osdpos=0;
			linefeed();
		} else {
			osdtext[osdpos++] = *chp;
			if (osdpos == osdcols) {
				osdpos = 0;
				linefeed();
			}
		}
	}
}


//
// OSD_DispatchQueued() -- Executes any commands queued in the buffer
//
void OSD_DispatchQueued(void)
{
	int cmd;
	
	if (!osdexeccount) return;

	cmd=osdexeccount-1;
	osdexeccount=0;

	for (; cmd>=0; cmd--) {
		OSD_Dispatch((const char *)osdhistorybuf[cmd]);
	}
}


//
// OSD_Dispatch() -- Executes a command string
//

static char *strtoken(char *s, char **ptrptr, int *restart)
{
	char *p, *p2, *start;
	
	*restart = 0;
	if (!ptrptr) return NULL;
	
	// if s != NULL, we process from the start of s, otherwise
	// we just continue with where ptrptr points to
	if (s) p = s;
	else p = *ptrptr;
	
	if (!p) return NULL;
	
	// eat up any leading whitespace
	while (*p != 0 && *p != ';' && *p == ' ') p++;
	
	// a semicolon is an end of statement delimiter like a \0 is, so we signal
	// the caller to 'restart' for the rest of the string pointed at by *ptrptr
	if (*p == ';') {
		*restart = 1;
		*ptrptr = p+1;
		return NULL;
	}
	// or if we hit the end of the input, signal all done by nulling *ptrptr
	else if (*p == 0) {
		*ptrptr = NULL;
		return NULL;
	}
	
	if (*p == '\"') {
		// quoted string
		start = ++p;
		p2 = p;
		while (*p != 0) {
			if (*p == '\"') {
				p++;
				break;
			} else if (*p == '\\') {
				switch (*(++p)) {
					case 'n': *p2 = '\n'; break;
					case 'r': *p2 = '\r'; break;
					default: *p2 = *p; break;
				}
			} else {
				*p2 = *p;
			}
			p2++, p++;
		}
		*p2 = 0;
	} else {
		start = p;
		while (*p != 0 && *p != ';' && *p != ' ') p++;
	}
	
	// if we hit the end of input, signal all done by nulling *ptrptr
	if (*p == 0) {
		*ptrptr = NULL;
	}
	// or if we came upon a semicolon, signal caller to restart with the
	// string at *ptrptr
	else if (*p == ';') {
		*p = 0;
		*ptrptr = p+1;
		*restart = 1;
	}
	// otherwise, clip off the token and carry on
	else {
		*(p++) = 0;
		*ptrptr = p;
	}
	
	return start;
}

#define MAXPARMS 512
int OSD_Dispatch(const char *cmd)
{
	char *workbuf, *wp, *wtp, *state;
	char *parms[MAXPARMS];
	int  numparms, restart = 0;
	osdfuncparm_t ofp;
	symbol_t *symb;
	//int i;
	
	workbuf = state = Bstrdup(cmd);
	if (!workbuf) return -1;

	do {
		numparms = 0;
		Bmemset(parms, 0, sizeof(parms));
		wp = strtoken(state, &wtp, &restart);
		if (!wp) {
			state = wtp;
			continue;
		}

		symb = findexactsymbol(wp);
		if (!symb) {
			OSD_Printf("Error: \"%s\" is not defined\n", wp);
			free(workbuf);
			return -1;
		}
		
		ofp.name = wp;
		while (wtp && !restart) {
			wp = strtoken(NULL, &wtp, &restart);
			if (wp && numparms < MAXPARMS) parms[numparms++] = wp;
		}
		ofp.numparms = numparms;
		ofp.parms    = (const char **)parms;
		ofp.raw      = cmd;
		switch (symb->func(&ofp)) {
			case OSDCMD_OK: break;
			case OSDCMD_SHOWHELP: OSD_Printf("%s\n", symb->help); break;
		}
		
		state = wtp;
	} while (wtp && restart);
	
	free(workbuf);
	
	return 0;
}


//
// OSD_RegisterFunction() -- Registers a new function
//
int OSD_RegisterFunction(const char *name, const char *help, int (*func)(const osdfuncparm_t*))
{
	symbol_t *symb;
	const char *cp;

	if (!osdinited) OSD_Init();

	if (!name) {
		Bprintf("OSD_RegisterFunction(): may not register a function with a null name\n");
		return -1;
	}
	if (!name[0]) {
		Bprintf("OSD_RegisterFunction(): may not register a function with no name\n");
		return -1;
	}

	// check for illegal characters in name
	for (cp = name; *cp; cp++) {
		if ((cp == name) && (*cp >= '0') && (*cp <= '9')) {
			Bprintf("OSD_RegisterFunction(): first character of function name \"%s\" must not be a numeral\n", name);
			return -1;
		}
		if ((*cp < '0') ||
		    (*cp > '9' && *cp < 'A') ||
		    (*cp > 'Z' && *cp < 'a' && *cp != '_') ||
		    (*cp > 'z')) {
			Bprintf("OSD_RegisterFunction(): illegal character in function name \"%s\"\n", name);
			return -1;
		}
	}

	if (!help) help = "(no description for this function)";
	if (!func) {
		Bprintf("OSD_RegisterFunction(): may not register a null function\n");
		return -1;
	}

	symb = findexactsymbol(name);
	if (symb) {
		Bprintf("OSD_RegisterFunction(): \"%s\" is already defined\n", name);
		return -1;
	}
	
	symb = addnewsymbol(name);
	if (!symb) {
		Bprintf("OSD_RegisterFunction(): Failed registering function \"%s\"\n", name);
		return -1;
	}

	symb->name = name;
	symb->help = help;
	symb->func = func;

	return 0;
}


//
// addnewsymbol() -- Allocates space for a new symbol and attaches it
//   appropriately to the lists, sorted.
//
static symbol_t *addnewsymbol(const char *name)
{
	symbol_t *newsymb, *s, *t;

	newsymb = (symbol_t *)Bmalloc(sizeof(symbol_t));
	if (!newsymb) { return NULL; }
	Bmemset(newsymb, 0, sizeof(symbol_t));

	// link it to the main chain
	if (!symbols) {
		symbols = newsymb;
	} else {
		if (Bstrcasecmp(name, symbols->name) <= 0) {
			t = symbols;
			symbols = newsymb;
			symbols->next = t;
		} else {
			s = symbols;
			while (s->next) {
				if (Bstrcasecmp(s->next->name, name) > 0) break;
				s=s->next;
			}
			t = s->next;
			s->next = newsymb;
			newsymb->next = t;
		}
	}

	return newsymb;
}


//
// findsymbol() -- Finds a symbol, possibly partially named
// 
static symbol_t *findsymbol(const char *name, symbol_t *startingat)
{
	if (!startingat) startingat = symbols;
	if (!startingat) return NULL;

	for (; startingat; startingat=startingat->next)
		if (!Bstrncasecmp(name, startingat->name, Bstrlen(name))) return startingat;

	return NULL;
}


//
// findexactsymbol() -- Finds a symbol, complete named
// 
static symbol_t *findexactsymbol(const char *name)
{
	symbol_t *startingat;
	if (!symbols) return NULL;

	startingat = symbols;

	for (; startingat; startingat=startingat->next)
		if (!Bstrcasecmp(name, startingat->name)) return startingat;

	return NULL;
}

