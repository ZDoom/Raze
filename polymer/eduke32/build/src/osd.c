// On-screen Display (ie. console)
// for the Build Engine
// by Jonathon Fowler (jf@jonof.id.au)

#include "build.h"
#include "osd.h"
#include "compat.h"
#include "baselayer.h"
#include "cache1d.h"
#include "pragmas.h"
#include "scancodes.h"
#include "crc32.h"

static mutex_t m_osdprintf;
symbol_t *symbols = NULL;
static symbol_t *addnewsymbol(const char *name);
static symbol_t *findsymbol(const char *name, symbol_t *startingat);
static symbol_t *findexactsymbol(const char *name);

// static int32_t _validate_osdlines(void *);

static int32_t _internal_osdfunc_listsymbols(const osdfuncparm_t *);
static int32_t _internal_osdfunc_help(const osdfuncparm_t *);
static int32_t _internal_osdfunc_alias(const osdfuncparm_t *);
// static int32_t _internal_osdfunc_dumpbuildinfo(const osdfuncparm_t *);
// static int32_t _internal_osdfunc_setrendermode(const osdfuncparm_t *);

static int32_t white=-1;            // colour of white (used by default display routines)
static void _internal_drawosdchar(int32_t, int32_t, char, int32_t, int32_t);
static void _internal_drawosdstr(int32_t, int32_t, const char *, int32_t, int32_t, int32_t);
static void _internal_drawosdcursor(int32_t,int32_t,int32_t,int32_t);
static int32_t _internal_getcolumnwidth(int32_t);
static int32_t _internal_getrowheight(int32_t);
static void _internal_clearbackground(int32_t,int32_t);
static int32_t _internal_gettime(void);
static void _internal_onshowosd(int32_t);

static uint32_t osdflags = 0;

// history display
static char osdtext[TEXTSIZE];
static char osdfmt[TEXTSIZE];
static char osdver[32];
static int32_t  osdverlen;
static int32_t  osdvershade;
static int32_t  osdverpal;
static int32_t  osdpos=0;           // position next character will be written at
static int32_t  osdlines=1;         // # lines of text in the buffer
static int32_t  osdrows=20;         // # lines of the buffer that are visible
static int32_t  osdrowscur=-1;
static int32_t  osdscroll=0;
static int32_t  osdcols=60;         // width of onscreen display in text columns
static int32_t  osdmaxrows=20;      // maximum number of lines which can fit on the screen
static int32_t  osdmaxlines=TEXTSIZE/60;    // maximum lines which can fit in the buffer
static int32_t  osdhead=0;          // topmost visible line number
static BFILE *osdlog=NULL;      // log filehandle
static int32_t  osdkey=sc_Tilde;
static int32_t  keytime=0;
static int32_t osdscrtime = 0;

// command prompt editing
static char osdeditbuf[OSD_EDITLENGTH+1];   // editing buffer
static char osdedittmp[OSD_EDITLENGTH+1];   // editing buffer temporary workspace
static int32_t  osdeditlen=0;       // length of characters in edit buffer
static int32_t  osdeditcursor=0;        // position of cursor in edit buffer
static int32_t  osdeditwinstart=0;
static int32_t  osdeditwinend=60-1-3;
#define editlinewidth (osdcols-1-3)

// command processing
static int32_t  osdhistorypos=-1;       // position we are at in the history buffer
char osdhistorybuf[OSD_HISTORYDEPTH][OSD_EDITLENGTH+1];  // history strings
int32_t  osdhistorysize=0;       // number of entries in history
int32_t  osdhistorytotal=0;      // number of total history entries

// execution buffer
// the execution buffer works from the command history
static int32_t  osdexeccount=0;     // number of lines from the head of the history buffer to execute

// maximal log line count
static int32_t logcutoff=120000;
int32_t OSD_errors=0;
static int32_t linecnt;
static int32_t osdexecscript=0;
#ifdef _WIN32
static int32_t osdtextmode=0;
#else
static int32_t osdtextmode=1;
#endif
// presentation parameters
static int32_t  osdpromptshade=0;
static int32_t  osdpromptpal=0;
static int32_t  osdeditshade=0;
static int32_t  osdeditpal=0;
static int32_t  osdtextshade=0;
static int32_t  osdtextpal=0;
/* static int32_t  osdcursorshade=0;
static int32_t  osdcursorpal=0; */

#define MAXSYMBOLS 256

static symbol_t *osdsymbptrs[MAXSYMBOLS];
static int32_t osdnumsymbols = 0;
static hashtable_t h_osd      = { MAXSYMBOLS<<1, NULL };

// application callbacks
static void (*drawosdchar)(int32_t, int32_t, char, int32_t, int32_t) = _internal_drawosdchar;
static void (*drawosdstr)(int32_t, int32_t, const char *, int32_t, int32_t, int32_t) = _internal_drawosdstr;
static void (*drawosdcursor)(int32_t, int32_t, int32_t, int32_t) = _internal_drawosdcursor;
static int32_t (*getcolumnwidth)(int32_t) = _internal_getcolumnwidth;
static int32_t (*getrowheight)(int32_t) = _internal_getrowheight;
static void (*clearbackground)(int32_t,int32_t) = _internal_clearbackground;
static int32_t (*gettime)(void) = _internal_gettime;
static void (*onshowosd)(int32_t) = _internal_onshowosd;

static void (*_drawosdchar)(int32_t, int32_t, char, int32_t, int32_t) = _internal_drawosdchar;
static void (*_drawosdstr)(int32_t, int32_t, const char *, int32_t, int32_t, int32_t) = _internal_drawosdstr;
static void (*_drawosdcursor)(int32_t, int32_t, int32_t, int32_t) = _internal_drawosdcursor;
static int32_t (*_getcolumnwidth)(int32_t) = _internal_getcolumnwidth;
static int32_t (*_getrowheight)(int32_t) = _internal_getrowheight;

static osdcvar_t *cvars = NULL;
static uint32_t osdnumcvars = 0;
static hashtable_t h_cvars      = { MAXSYMBOLS<<1, NULL };

int32_t m32_osd_tryscript=0;  // whether to try executing m32script on unkown command in the osd
extern void M32RunScript(const char *s);

int32_t OSD_RegisterCvar(const cvar_t *cvar)
{
    const char *cp;

    if ((osdflags & OSD_INITIALIZED) == 0)
        OSD_Init();

    if (!cvar->name || !cvar->name[0] || !cvar->vptr)
    {
        OSD_Printf("OSD_RegisterCvar(): can't register null cvar\n");
        return -1;
    }

    // check for illegal characters in name
    for (cp = cvar->name; *cp; cp++)
    {
        if ((cp == cvar->name) && (*cp >= '0') && (*cp <= '9'))
        {
            OSD_Printf("OSD_RegisterCvar(): first character of cvar name \"%s\" must not be a numeral\n", cvar->name);
            return -1;
        }
        if ((*cp < '0') ||
                (*cp > '9' && *cp < 'A') ||
                (*cp > 'Z' && *cp < 'a' && *cp != '_') ||
                (*cp > 'z'))
        {
            OSD_Printf("OSD_RegisterCvar(): illegal character in cvar name \"%s\"\n", cvar->name);
            return -1;
        }
    }

    cvars = (osdcvar_t *)Brealloc(cvars, (osdnumcvars + 1) * sizeof(osdcvar_t));

    hash_add(&h_cvars, cvar->name, osdnumcvars, 1);

    switch (cvar->type & (CVAR_BOOL|CVAR_INT|CVAR_UINT|CVAR_FLOAT|CVAR_DOUBLE))
    {
    case CVAR_BOOL:
    case CVAR_INT:
        cvars[osdnumcvars].dval.i = *(int32_t *)cvar->vptr;
        break;
    case CVAR_UINT:
        cvars[osdnumcvars].dval.uint = *(uint32_t *)cvar->vptr;
        break;
    case CVAR_FLOAT:
        cvars[osdnumcvars].dval.f = *(float *)cvar->vptr;
        break;
    case CVAR_DOUBLE:
        cvars[osdnumcvars].dval.d = *(double *)cvar->vptr;
        break;
    }

    Bmemcpy(&cvars[osdnumcvars++], cvar, sizeof(cvar_t));

    return 0;
}

static int32_t OSD_CvarModified(const osdcvar_t *cvar)
{
    if ((osdflags & OSD_INITIALIZED) == 0)
        return 0;

    if (!cvar->c.vptr)
    {
        OSD_Printf("OSD_CvarModified(): null cvar?!\n");
        return 0;
    }

    switch (cvar->c.type & (CVAR_BOOL|CVAR_INT|CVAR_UINT|CVAR_FLOAT|CVAR_DOUBLE))
    {
    case CVAR_BOOL:
    case CVAR_INT:
        return (cvar->dval.i != *(int32_t *)cvar->c.vptr);
    case CVAR_UINT:
        return (cvar->dval.uint != *(uint32_t *)cvar->c.vptr);
    case CVAR_FLOAT:
        return (cvar->dval.f != *(float *)cvar->c.vptr);
    case CVAR_DOUBLE:
        return (cvar->dval.d != *(double *)cvar->c.vptr);
    default:
        return 0;
    }
}

// color code format is as follows:
// ^## sets a color, where ## is the palette number
// ^S# sets a shade, range is 0-7 equiv to shades 0-14
// ^O resets formatting to defaults

const char *OSD_StripColors(char *out, const char *in)
{
    const char *ptr = out;

    while (*in)
    {
        if (*in == '^' && isdigit(*(in+1)))
        {
            in += 2;
            if (isdigit(*in))
                in++;
            continue;
        }
        if (*in == '^' && (Btoupper(*(in+1)) == 'S') && isdigit(*(in+2)))
        {
            in += 3;
            continue;
        }
        if (*in == '^' && (Btoupper(*(in+1)) == 'O'))
        {
            in += 2;
            continue;
        }
        *(out++) = *(in++);
    }

    *out = '\0';
    return ptr;
}

int32_t OSD_Exec(const char *szScript)
{
    FILE *fp = fopenfrompath(szScript, "r");

    if (fp != NULL)
    {
        char line[256], *cp;

        OSD_Printf("Executing \"%s\"\n", szScript);
        osdexecscript++;
        while (fgets(line, sizeof(line), fp) != NULL)
        {
            cp = strtok(line,"\r\n");
            if (cp)
                OSD_Dispatch(cp);
        }
        osdexecscript--;
        fclose(fp);
        return 0;
    }
    return 1;
}

int32_t OSD_ParsingScript(void)
{
    return osdexecscript;
}

int32_t OSD_OSDKey(void)
{
    return osdkey;
}

char *OSD_GetTextPtr(void)
{
    return osdtext;
}

char *OSD_GetFmtPtr(void)
{
    return osdfmt;
}

char *OSD_GetFmt(char *ptr)
{
    return (ptr - osdtext + osdfmt);
}

int32_t OSD_GetCols(void)
{
    return osdcols;
}

int32_t OSD_IsMoving(void)
{
    return (osdrowscur!=-1 && osdrowscur!=osdrows);
}

int32_t OSD_GetRowsCur(void)
{
    return osdrowscur;
}

int32_t OSD_GetTextMode(void)
{
    return osdtextmode;
}

static inline void swapptr(void *a, void *b)
{
	intptr_t t = *(intptr_t*)a;
	*(intptr_t*)a = *(intptr_t*)b;
	*(intptr_t*)b = t;
}

void OSD_SetTextMode(int32_t mode)
{
    osdtextmode = (mode != 0);

    if (osdtextmode)
    {
        if (drawosdchar != _internal_drawosdchar)
        {
            swapptr(&_drawosdchar,&drawosdchar);
            swapptr(&_drawosdstr,&drawosdstr);
            swapptr(&_drawosdcursor,&drawosdcursor);
            swapptr(&_getcolumnwidth,&getcolumnwidth);
            swapptr(&_getrowheight,&getrowheight);
        }
    }
    else if (drawosdchar == _internal_drawosdchar)
    {
        swapptr(&_drawosdchar,&drawosdchar);
        swapptr(&_drawosdstr,&drawosdstr);
        swapptr(&_drawosdcursor,&drawosdcursor);
        swapptr(&_getcolumnwidth,&getcolumnwidth);
        swapptr(&_getrowheight,&getrowheight);
    }

    if (in3dmode())
        OSD_ResizeDisplay(xdim, ydim);
}

static int32_t _internal_osdfunc_exec(const osdfuncparm_t *parm)
{
    char fn[BMAX_PATH];

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;
    Bstrcpy(fn,parm->parms[0]);

    if (OSD_Exec(fn))
    {
        OSD_Printf(OSD_ERROR "exec: file \"%s\" not found.\n", fn);
        return OSDCMD_OK;
    }
    return OSDCMD_OK;
}

static int32_t _internal_osdfunc_echo(const osdfuncparm_t *parm)
{
    int32_t i;
    for (i = 0; i < parm->numparms; i++)
    {
        if (i > 0) OSD_Printf(" ");
        OSD_Printf("%s", parm->parms[i]);
    }
    OSD_Printf("\n");

    return OSDCMD_OK;
}

static int32_t _internal_osdfunc_fileinfo(const osdfuncparm_t *parm)
{
    uint32_t crc, length;
    int32_t i,j;
    char buf[256];

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    if ((i = kopen4load((char *)parm->parms[0],0)) < 0)
    {
        OSD_Printf("fileinfo: File \"%s\" not found.\n", parm->parms[0]);
        return OSDCMD_OK;
    }

    length = kfilelength(i);

    crc32init(&crc);
    do
    {
        j = kread(i,buf,256);
        crc32block(&crc,(uint8_t *)buf,j);
    }
    while (j == 256);
    crc32finish(&crc);

    kclose(i);

    OSD_Printf("fileinfo: %s\n"
               "  File size: %d\n"
               "  CRC-32:    %08X\n",
               parm->parms[0], length, crc);

    return OSDCMD_OK;
}

static void _internal_drawosdchar(int32_t x, int32_t y, char ch, int32_t shade, int32_t pal)
{
    char st[2] = { 0,0 };

    UNREFERENCED_PARAMETER(shade);
    UNREFERENCED_PARAMETER(pal);

    st[0] = ch;

    printext256(4+(x<<3),4+(y<<3), white, -1, st, 0);
}

static void _internal_drawosdstr(int32_t x, int32_t y, const char *ch, int32_t len, int32_t shade, int32_t pal)
{
    char st[1024];

    UNREFERENCED_PARAMETER(shade);
    UNREFERENCED_PARAMETER(pal);

    if (len>1023) len=1023;
    Bmemcpy(st,ch,len);
    st[len]=0;

    printext256(4+(x<<3),4+(y<<3), white, -1, st, 0);
}

static void _internal_drawosdcursor(int32_t x, int32_t y, int32_t type, int32_t lastkeypress)
{
    char st[2] = { '_',0 };

    UNREFERENCED_PARAMETER(lastkeypress);

    if (type) st[0] = '#';

    if (white > -1)
    {
        printext256(4+(x<<3),4+(y<<3)+2, white, -1, st, 0);
        return;
    }

    {
        int32_t i, j, k;
        // find the palette index closest to white
        k=0;
        for (i=0; i<256; i++)
        {
            j = ((int32_t)curpalette[i].r)+((int32_t)curpalette[i].g)+((int32_t)curpalette[i].b);
            if (j > k) { k = j; white = i; }
        }
    }

}

static int32_t _internal_getcolumnwidth(int32_t w)
{
    return w/8 - 1;
}

static int32_t _internal_getrowheight(int32_t w)
{
    return w/8;
}

static void _internal_clearbackground(int32_t cols, int32_t rows)
{
    UNREFERENCED_PARAMETER(cols);
    UNREFERENCED_PARAMETER(rows);
}

static int32_t _internal_gettime(void)
{
    return 0;
}

static void _internal_onshowosd(int32_t a)
{
    UNREFERENCED_PARAMETER(a);
}

////////////////////////////

static int32_t _internal_osdfunc_alias(const osdfuncparm_t *parm)
{
    symbol_t *i;

    if (parm->numparms < 1)
    {
        int32_t j = 0;
        OSD_Printf("Alias listing:\n");
        for (i=symbols; i!=NULL; i=i->next)
            if (i->func == OSD_ALIAS)
            {
                j++;
                OSD_Printf("     %s \"%s\"\n", i->name, i->help);
            }
        if (j == 0)
            OSD_Printf("No aliases found.\n");
        return OSDCMD_OK;
    }

    for (i=symbols; i!=NULL; i=i->next)
    {
        if (!Bstrcasecmp(parm->parms[0],i->name))
        {
            if (parm->numparms < 2)
            {
                if (i->func == OSD_ALIAS)
                    OSD_Printf("alias %s \"%s\"\n", i->name, i->help);
                else OSD_Printf("%s is a function, not an alias\n",i->name);
                return OSDCMD_OK;
            }

            if (i->func != OSD_ALIAS && i->func != OSD_UNALIASED)
            {
                OSD_Printf("Cannot override function \"%s\" with alias\n",i->name);
                return OSDCMD_OK;
            }
        }
    }

    OSD_RegisterFunction(Bstrdup(parm->parms[0]),Bstrdup(parm->parms[1]),OSD_ALIAS);
    if (!osdexecscript)
        OSD_Printf("%s\n",parm->raw);
    return OSDCMD_OK;
}

static int32_t _internal_osdfunc_unalias(const osdfuncparm_t *parm)
{
    symbol_t *i;

    if (parm->numparms < 1)
        return OSDCMD_SHOWHELP;

    for (i=symbols; i!=NULL; i=i->next)
    {
        if (!Bstrcasecmp(parm->parms[0],i->name))
        {
            if (parm->numparms < 2)
            {
                if (i->func == OSD_ALIAS)
                {
                    OSD_Printf("Removed alias %s (\"%s\")\n", i->name, i->help);
                    i->func = OSD_UNALIASED;
                }
                else OSD_Printf("Invalid alias %s\n",i->name);
                return OSDCMD_OK;
            }
        }
    }
    OSD_Printf("Invalid alias %s\n",parm->parms[0]);
    return OSDCMD_OK;
}

static int32_t _internal_osdfunc_listsymbols(const osdfuncparm_t *parm)
{
    symbol_t *i;
    int32_t maxwidth = 0;

    UNREFERENCED_PARAMETER(parm);

    for (i=symbols; i!=NULL; i=i->next)
        if (i->func != OSD_UNALIASED)
            maxwidth = max((unsigned)maxwidth,Bstrlen(i->name));

    if (maxwidth > 0)
    {
        int32_t x = 0, count = 0;
        maxwidth += 3;
        OSD_Printf(OSDTEXT_RED "Symbol listing:\n");
        for (i=symbols; i!=NULL; i=i->next)
        {
            if (i->func == OSD_UNALIASED)
                continue;

            {
                int32_t j = hash_find(&h_cvars, i->name);

                if (j != -1 && OSD_CvarModified(&cvars[j]))
                {
                    OSD_Printf(OSDTEXT_RED "*");
                    OSD_Printf("%-*s",maxwidth-1,i->name);
                }
                else OSD_Printf("%-*s",maxwidth,i->name);

                x += maxwidth;
                count++;
            }

            if (x > osdcols - maxwidth)
            {
                x = 0;
                OSD_Printf("\n");
            }
        }
        if (x) OSD_Printf("\n");
        OSD_Printf(OSDTEXT_RED "Found %d symbols\n",count);
    }
    return OSDCMD_OK;
}

static int32_t _internal_osdfunc_help(const osdfuncparm_t *parm)
{
    symbol_t *symb;

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;
    symb = findexactsymbol(parm->parms[0]);
    if (!symb)
        OSD_Printf("Error: no help for undefined symbol \"%s\"\n", parm->parms[0]);
    else
        OSD_Printf("%s\n", symb->help);

    return OSDCMD_OK;
}

static int32_t _internal_osdfunc_clear(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
    Bmemset(osdtext,0,sizeof(osdtext));
    Bmemset(osdfmt,osdtextpal+(osdtextshade<<5),sizeof(osdfmt));
    osdlines = 1;
    return OSDCMD_OK;
}

static int32_t _internal_osdfunc_history(const osdfuncparm_t *parm)
{
    int32_t i, j = 0;
    UNREFERENCED_PARAMETER(parm);
    OSD_Printf(OSDTEXT_RED "Command history:\n");
    for (i=OSD_HISTORYDEPTH-1; i>=0; i--)
        if (osdhistorybuf[i][0])
            OSD_Printf("%4d \"%s\"\n",osdhistorytotal-osdhistorysize+(++j),osdhistorybuf[i]);
    return OSDCMD_OK;
}

////////////////////////////


//
// OSD_Cleanup() -- Cleans up the on-screen display
//
void OSD_Cleanup(void)
{
    symbol_t *s;

    hash_free(&h_osd);
    hash_free(&h_cvars);

    for (; symbols; symbols=s)
    {
        s=symbols->next;
        Bfree(symbols);
    }

    if (osdlog)
    {
        Bfclose(osdlog);
        osdlog = NULL;
    }

    if (cvars)
    {
        Bfree(cvars);
        cvars = NULL;
    }

    osdflags &= ~OSD_INITIALIZED;
}


static int32_t osdcmd_cvar_set_osd(const osdfuncparm_t *parm)
{
    int32_t r = osdcmd_cvar_set(parm);

    if (r == OSDCMD_OK)
    {
        if (!Bstrcasecmp(parm->name, "osdrows"))
        {
            if (osdrows > osdmaxrows) osdrows = osdmaxrows;
            if (osdrowscur!=-1)osdrowscur = osdrows;
            return r;
        }
        else if (!Bstrcasecmp(parm->name, "osdtextmode"))
        {
            OSD_SetTextMode(osdtextmode);
            return r;
        }
    }
    return r;
}

static int32_t _internal_osdfunc_toggle(const osdfuncparm_t *parm)
{
    int32_t i;

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    i = hash_find(&h_cvars, parm->parms[0]);

    if (i == -1)
        for (i = osdnumcvars-1; i >= 0; i--)
            if (!Bstrcasecmp(parm->parms[0], cvars[i].c.name)) break;

    if (i == -1 || (cvars[i].c.type & CVAR_BOOL) != CVAR_BOOL)
    {
        OSD_Printf("Bad cvar name or cvar not boolean\n");
        return OSDCMD_OK;
    }

    *(int32_t *)cvars[i].c.vptr = 1 - *(int32_t *)cvars[i].c.vptr;
    return OSDCMD_OK;
}

//
// OSD_Init() -- Initializes the on-screen display
//
void OSD_Init(void)
{
    uint32_t i;
    cvar_t cvars_osd[] =
    {
        { "osdeditpal","sets the palette of the OSD input text",(void *) &osdeditpal, CVAR_INT, 0, MAXPALOOKUPS-1 },
        { "osdpromptpal","sets the palette of the OSD prompt",(void *) &osdpromptpal, CVAR_INT, 0, MAXPALOOKUPS-1 },
        { "osdtextpal","sets the palette of the OSD text",(void *) &osdtextpal, CVAR_INT, 0, MAXPALOOKUPS-1 },
        { "osdeditshade","sets the shade of the OSD input text",(void *) &osdeditshade, CVAR_INT, 0, 7 },
        { "osdtextshade","sets the shade of the OSD text",(void *) &osdtextshade, CVAR_INT, 0, 7 },
        { "osdpromptshade","sets the shade of the OSD prompt",(void *) &osdpromptshade, CVAR_INT, INT8_MIN, INT8_MAX },
        { "osdrows","sets the number of visible lines of the OSD",(void *) &osdrows, CVAR_INT|CVAR_FUNCPTR, 0, MAXPALOOKUPS-1 },
        { "osdtextmode","set OSD text mode (0:graphical, 1:fast)",(void *) &osdtextmode, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },
        { "logcutoff","sets the maximal line count of the log file",(void *) &logcutoff, CVAR_INT, 0, 262144 },
    };

    mutex_init(&m_osdprintf);

    Bmemset(osdtext, asc_Space, TEXTSIZE);
    Bmemset(osdfmt, osdtextpal+(osdtextshade<<5), TEXTSIZE);
    Bmemset(osdsymbptrs, 0, sizeof(osdsymbptrs));

    osdnumsymbols = osdnumcvars = 0;
    osdlines = 1;
    osdflags |= OSD_INITIALIZED;

    hash_init(&h_osd);
    hash_init(&h_cvars);

    for (i=0; i<sizeof(cvars_osd)/sizeof(cvars_osd[0]); i++)
    {
        if (OSD_RegisterCvar(&cvars_osd[i]))
            continue;

        OSD_RegisterFunction(cvars_osd[i].name, cvars_osd[i].desc,
                             cvars_osd[i].type & CVAR_FUNCPTR ? osdcmd_cvar_set_osd : osdcmd_cvar_set);
    }

    OSD_RegisterFunction("alias","alias: creates an alias for calling multiple commands",_internal_osdfunc_alias);
    OSD_RegisterFunction("clear","clear: clears the console text buffer",_internal_osdfunc_clear);
    OSD_RegisterFunction("echo","echo [text]: echoes text to the console", _internal_osdfunc_echo);
    OSD_RegisterFunction("exec","exec <scriptfile>: executes a script", _internal_osdfunc_exec);
    OSD_RegisterFunction("fileinfo","fileinfo <file>: gets a file's information", _internal_osdfunc_fileinfo);
    OSD_RegisterFunction("help","help: displays help for the specified cvar or command; \"listsymbols\" to show all commands",_internal_osdfunc_help);
    OSD_RegisterFunction("history","history: displays the console command history",_internal_osdfunc_history);
    OSD_RegisterFunction("listsymbols","listsymbols: lists all registered functions, cvars and aliases",_internal_osdfunc_listsymbols);
    OSD_RegisterFunction("toggle","toggle: toggles the value of a boolean cvar",_internal_osdfunc_toggle);
    OSD_RegisterFunction("unalias","unalias: removes a command alias",_internal_osdfunc_unalias);

    atexit(OSD_Cleanup);
}


//
// OSD_SetLogFile() -- Sets the text file where printed text should be echoed
//
void OSD_SetLogFile(const char *fn)
{
    if (osdlog)
    {
        Bfclose(osdlog);
        osdlog = NULL;
    }

    if (fn) osdlog = Bfopen(fn,"w");
    if (osdlog) setvbuf(osdlog, (char *)NULL, _IONBF, 0);
}


//
// OSD_SetFunctions() -- Sets some callbacks which the OSD uses to understand its world
//
void OSD_SetFunctions(
    void (*drawchar)(int32_t,int32_t,char,int32_t,int32_t),
    void (*drawstr)(int32_t,int32_t,const char *,int32_t,int32_t,int32_t),
    void (*drawcursor)(int32_t,int32_t,int32_t,int32_t),
    int32_t (*colwidth)(int32_t),
    int32_t (*rowheight)(int32_t),
    void (*clearbg)(int32_t,int32_t),
    int32_t (*gtime)(void),
    void (*showosd)(int32_t)
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
    int32_t promptshade, int32_t promptpal,
    int32_t editshade, int32_t editpal,
    int32_t textshade, int32_t textpal
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
void OSD_CaptureKey(int32_t sc)
{
    osdkey = sc;
}

//
// OSD_FindDiffPoint() -- Finds the length of the longest common prefix of 2 strings, stolen from ZDoom
//
static int32_t OSD_FindDiffPoint(const char *str1, const char *str2)
{
    int32_t i;

    for (i = 0; Btolower(str1[i]) == Btolower(str2[i]); i++)
        if (str1[i] == 0 || str2[i] == 0)
            break;

    return i;
}

static void OSD_HistoryPrev(void)
{
    if (osdhistorypos >= osdhistorysize-1) return;

    osdhistorypos++;
    Bmemcpy(osdeditbuf, osdhistorybuf[osdhistorypos], OSD_EDITLENGTH+1);

    osdeditcursor = 0;
    while (osdeditbuf[osdeditcursor]) osdeditcursor++;
    osdeditlen = osdeditcursor;

    if (osdeditcursor<osdeditwinstart)
    {
        osdeditwinend = osdeditcursor;
        osdeditwinstart = osdeditwinend-editlinewidth;

        if (osdeditwinstart<0)
        {
            osdeditwinend-=osdeditwinstart;
            osdeditwinstart=0;
        }
    }
    else if (osdeditcursor>=osdeditwinend)
    {
        osdeditwinstart+=(osdeditcursor-osdeditwinend);
        osdeditwinend+=(osdeditcursor-osdeditwinend);
    }
}

static void OSD_HistoryNext(void)
{
    if (osdhistorypos < 0) return;

    if (osdhistorypos == 0)
    {
        osdeditlen=0;
        osdeditcursor=0;
        osdeditwinstart=0;
        osdeditwinend=editlinewidth;
        osdhistorypos = -1;
        return;
    }

    osdhistorypos--;
    Bmemcpy(osdeditbuf, osdhistorybuf[osdhistorypos], OSD_EDITLENGTH+1);

    osdeditcursor = 0;
    while (osdeditbuf[osdeditcursor]) osdeditcursor++;
    osdeditlen = osdeditcursor;

    if (osdeditcursor<osdeditwinstart)
    {
        osdeditwinend = osdeditcursor;
        osdeditwinstart = osdeditwinend-editlinewidth;

        if (osdeditwinstart<0)
        {
            osdeditwinend-=osdeditwinstart;
            osdeditwinstart=0;
        }
    }
    else if (osdeditcursor>=osdeditwinend)
    {
        osdeditwinstart+=(osdeditcursor-osdeditwinend);
        osdeditwinend+=(osdeditcursor-osdeditwinend);
    }
}

//
// OSD_HandleKey() -- Handles keyboard input when capturing input.
//  Returns 0 if the key was handled internally, or the scancode if it should
//  be passed on to the game.
//

int32_t OSD_HandleChar(char ch)
{
    int32_t i,j;
    symbol_t *tabc = NULL;
    static symbol_t *lastmatch = NULL;

    if ((osdflags & (OSD_INITIALIZED|OSD_CAPTURE)) != (OSD_INITIALIZED|OSD_CAPTURE))
        return ch;

    if (ch != 9)    // tab
        lastmatch = NULL;

    switch (ch)
    {
    case 1:    // control a. jump to beginning of line
        osdeditcursor=0;
        osdeditwinstart=0;
        osdeditwinend=editlinewidth;
        return 0;
    case 2:   // control b, move one character left
        if (osdeditcursor > 0) osdeditcursor--;
        return 0;
    case 3:   // control c
        osdeditbuf[osdeditlen] = 0;
        OSD_Printf("%s\n",osdeditbuf);
        osdeditlen=0;
        osdeditcursor=0;
        osdeditwinstart=0;
        osdeditwinend=editlinewidth;
        osdeditbuf[0] = 0;
        return 0;
    case 5:   // control e, jump to end of line
        osdeditcursor = osdeditlen;
        osdeditwinend = osdeditcursor;
        osdeditwinstart = osdeditwinend-editlinewidth;
        if (osdeditwinstart<0)
        {
            osdeditwinstart=0;
            osdeditwinend = editlinewidth;
        }
        return 0;
    case 6:   // control f, move one character right
        if (osdeditcursor < osdeditlen) osdeditcursor++;
        return 0;
    case 8:
#ifdef __APPLE__
    case 127:      // control h, backspace
#endif
        if (!osdeditcursor || !osdeditlen) return 0;
        if ((osdflags & OSD_OVERTYPE) == 0)
        {
            if (osdeditcursor < osdeditlen)
                Bmemmove(osdeditbuf+osdeditcursor-1, osdeditbuf+osdeditcursor, osdeditlen-osdeditcursor);
            osdeditlen--;
        }
        osdeditcursor--;
        if (osdeditcursor<osdeditwinstart) osdeditwinstart--,osdeditwinend--;
#ifndef __APPLE__
    case 127:  // handled in OSD_HandleScanCode (delete)
#endif
        return 0;
    case 9:   // tab
    {
        int32_t commonsize = 512;

        if (!lastmatch)
        {
            for (i=osdeditcursor; i>0; i--) if (osdeditbuf[i-1] == ' ') break;
            for (j=0; osdeditbuf[i] != ' ' && i < osdeditlen; j++,i++)
                osdedittmp[j] = osdeditbuf[i];
            osdedittmp[j] = 0;

            if (j > 0)
            {
                tabc = findsymbol(osdedittmp, NULL);

                if (tabc && tabc->next && findsymbol(osdedittmp, tabc->next))
                {
                    symbol_t *symb=tabc;
                    int32_t maxwidth = 0, x = 0, num = 0, diffpt;

                    while (symb && symb != lastmatch)
                    {
                        num++;

                        if (lastmatch)
                        {
                            diffpt = OSD_FindDiffPoint(symb->name,lastmatch->name);
                            if (diffpt < commonsize)
                                commonsize = diffpt;
                        }

                        maxwidth = max((unsigned)maxwidth,Bstrlen(symb->name));
                        lastmatch = symb;
                        if (!lastmatch->next) break;
                        symb=findsymbol(osdedittmp, lastmatch->next);
                    }
                    OSD_Printf(OSDTEXT_RED "Found %d possible completions for \"%s\":\n",num,osdedittmp);
                    maxwidth += 3;
                    symb = tabc;
                    OSD_Printf("  ");
                    while (symb && (symb != lastmatch))
                    {
                        tabc = lastmatch = symb;
                        OSD_Printf("%-*s",maxwidth,symb->name);
                        if (!lastmatch->next) break;
                        symb=findsymbol(osdedittmp, lastmatch->next);
                        x += maxwidth;
                        if (x > (osdcols - maxwidth))
                        {
                            x = 0;
                            OSD_Printf("\n");
                            if (symb && (symb != lastmatch))
                                OSD_Printf("  ");
                        }
                    }
                    if (x) OSD_Printf("\n");
                    OSD_Printf(OSDTEXT_RED "Press TAB again to cycle through matches\n");
                }
            }
        }
        else
        {
            tabc = findsymbol(osdedittmp, lastmatch->next);
            if (!tabc && lastmatch)
                tabc = findsymbol(osdedittmp, NULL);    // wrap */
        }

        if (tabc)
        {
            for (i=osdeditcursor; i>0; i--) if (osdeditbuf[i-1] == ' ') break;
            osdeditlen = i;
            for (j=0; tabc->name[j] && osdeditlen <= OSD_EDITLENGTH
                    && (osdeditlen < commonsize); i++,j++,osdeditlen++)
                osdeditbuf[i] = tabc->name[j];
            osdeditcursor = osdeditlen;
            osdeditwinend = osdeditcursor;
            osdeditwinstart = osdeditwinend-editlinewidth;
            if (osdeditwinstart<0)
            {
                osdeditwinstart=0;
                osdeditwinend = editlinewidth;
            }

            lastmatch = tabc;
        }
    }
    return 0;
    case 11:      // control k, delete all to end of line
        Bmemset(osdeditbuf+osdeditcursor,0,sizeof(osdeditbuf)-osdeditcursor);
        return 0;
    case 12:      // control l, clear screen
        Bmemset(osdtext,0,sizeof(osdtext));
        Bmemset(osdfmt,osdtextpal+(osdtextshade<<5),sizeof(osdfmt));
        osdlines = 1;
        return 0;
    case 13:      // control m, enter
        if (osdeditlen>0)
        {
            osdeditbuf[osdeditlen] = 0;
            if (Bstrcmp(osdhistorybuf[0], osdeditbuf))
            {
                Bmemmove(osdhistorybuf[1], osdhistorybuf[0], (OSD_HISTORYDEPTH-1)*(OSD_EDITLENGTH+1));
                Bmemmove(osdhistorybuf[0], osdeditbuf, OSD_EDITLENGTH+1);
                if (osdhistorysize < OSD_HISTORYDEPTH) osdhistorysize++;
                osdhistorytotal++;
                if (osdexeccount == OSD_HISTORYDEPTH)
                    OSD_Printf("Command Buffer Warning: Failed queueing command "
                               "for execution. Buffer full.\n");
                else
                    osdexeccount++;
            }
            else
            {
                if (osdexeccount == OSD_HISTORYDEPTH)
                    OSD_Printf("Command Buffer Warning: Failed queueing command "
                               "for execution. Buffer full.\n");
                else
                    osdexeccount++;
            }
            osdhistorypos=-1;
        }

        osdeditlen=0;
        osdeditcursor=0;
        osdeditwinstart=0;
        osdeditwinend=editlinewidth;
        return 0;
    case 14:      // control n, next (ie. down arrow)
        OSD_HistoryNext();
        return 0;
    case 16:      // control p, previous (ie. up arrow)
        OSD_HistoryPrev();
        return 0;
    case 21:      // control u, delete all to beginning
        if (osdeditcursor>0 && osdeditlen)
        {
            if (osdeditcursor<osdeditlen)
                Bmemmove(osdeditbuf, osdeditbuf+osdeditcursor, osdeditlen-osdeditcursor);
            osdeditlen-=osdeditcursor;
            osdeditcursor = 0;
            osdeditwinstart = 0;
            osdeditwinend = editlinewidth;
        }
        return 0;
    case 23:      // control w, delete one word back
        if (osdeditcursor>0 && osdeditlen>0)
        {
            i=osdeditcursor;
            while (i>0 && osdeditbuf[i-1]==asc_Space) i--;
            while (i>0 && osdeditbuf[i-1]!=asc_Space) i--;
            if (osdeditcursor<osdeditlen)
                Bmemmove(osdeditbuf+i, osdeditbuf+osdeditcursor, osdeditlen-osdeditcursor);
            osdeditlen -= (osdeditcursor-i);
            osdeditcursor = i;
            if (osdeditcursor < osdeditwinstart)
            {
                osdeditwinstart=osdeditcursor;
                osdeditwinend=osdeditwinstart+editlinewidth;
            }
        }
        return 0;
    default:
        if (ch >= asc_Space)      // text char
        {
            if ((osdflags & OSD_OVERTYPE) == 0)
            {
                if (osdeditlen == OSD_EDITLENGTH) // buffer full, can't insert another char
                    return 0;
                if (osdeditcursor < osdeditlen)
                    Bmemmove(osdeditbuf+osdeditcursor+1, osdeditbuf+osdeditcursor, osdeditlen-osdeditcursor);
                osdeditlen++;
            }
            else if (osdeditcursor == osdeditlen)
                osdeditlen++;

            osdeditbuf[osdeditcursor++] = ch;

            if (osdeditcursor > osdeditwinend)
                osdeditwinstart++, osdeditwinend++;
        }
        return 0;
    }
    return 0;
}

int32_t OSD_HandleScanCode(int32_t sc, int32_t press)
{
    if ((osdflags & OSD_INITIALIZED) == 0)
        return sc;

    if (sc == osdkey)
    {
        if (press)
        {
            osdscroll = -osdscroll;
            if (osdrowscur == -1)
                osdscroll = 1;
            else if (osdrowscur == osdrows)
                osdscroll = -1;
            osdrowscur += osdscroll;
            OSD_CaptureInput(osdscroll == 1);
            osdscrtime = getticks();
        }
        return 0;
    }
    else if ((osdflags & OSD_CAPTURE) == 0)
        return sc;

    if (!press)
    {
        if (sc == sc_LeftShift || sc == sc_RightShift)
            osdflags &= ~OSD_SHIFT;
        if (sc == sc_LeftControl || sc == sc_RightControl)
            osdflags &= ~OSD_CTRL;
        return 0;
    }

    keytime = gettime();

    switch (sc)
    {
    case sc_Escape:
        //        OSD_ShowDisplay(0);
        osdscroll = -1;
        osdrowscur--;
        OSD_CaptureInput(0);
        osdscrtime = getticks();
        break;
    case sc_PgUp:
        if (osdhead < osdlines-1)
            osdhead++;
        break;
    case sc_PgDn:
        if (osdhead > 0)
            osdhead--;
        break;
    case sc_Home:
        if (osdflags & OSD_CTRL)
            osdhead = osdlines-1;
        else
        {
            osdeditcursor = 0;
            osdeditwinstart = osdeditcursor;
            osdeditwinend = osdeditwinstart+editlinewidth;
        }
        break;
    case sc_End:
        if (osdflags & OSD_CTRL)
            osdhead = 0;
        else
        {
            osdeditcursor = osdeditlen;
            osdeditwinend = osdeditcursor;
            osdeditwinstart = osdeditwinend-editlinewidth;
            if (osdeditwinstart<0)
            {
                osdeditwinstart=0;
                osdeditwinend = editlinewidth;
            }
        }
        break;
    case sc_Insert:
        osdflags = (osdflags & ~OSD_OVERTYPE) | (-((osdflags & OSD_OVERTYPE) == 0) & OSD_OVERTYPE);
        break;
    case sc_LeftArrow:
        if (osdeditcursor>0)
        {
            if (osdflags & OSD_CTRL)
            {
                while (osdeditcursor>0)
                {
                    if (osdeditbuf[osdeditcursor-1] != asc_Space)
                        break;
                    osdeditcursor--;
                }
                while (osdeditcursor>0)
                {
                    if (osdeditbuf[osdeditcursor-1] == asc_Space)
                        break;
                    osdeditcursor--;
                }
            }
            else osdeditcursor--;
        }
        if (osdeditcursor<osdeditwinstart)
        {
            osdeditwinend-=(osdeditwinstart-osdeditcursor);
            osdeditwinstart-=(osdeditwinstart-osdeditcursor);
        }
        break;
    case sc_RightArrow:
        if (osdeditcursor<osdeditlen)
        {
            if (osdflags & OSD_CTRL)
            {
                while (osdeditcursor<osdeditlen)
                {
                    if (osdeditbuf[osdeditcursor] == asc_Space)
                        break;
                    osdeditcursor++;
                }
                while (osdeditcursor<osdeditlen)
                {
                    if (osdeditbuf[osdeditcursor] != asc_Space)
                        break;
                    osdeditcursor++;
                }
            }
            else osdeditcursor++;
        }
        if (osdeditcursor>=osdeditwinend)
        {
            osdeditwinstart+=(osdeditcursor-osdeditwinend);
            osdeditwinend+=(osdeditcursor-osdeditwinend);
        }
        break;
    case sc_UpArrow:
        OSD_HistoryPrev();
        break;
    case sc_DownArrow:
        OSD_HistoryNext();
        break;
    case sc_LeftShift:
    case sc_RightShift:
        osdflags |= OSD_SHIFT;
        break;
    case sc_LeftControl:
    case sc_RightControl:
        osdflags |= OSD_CTRL;
        break;
    case sc_CapsLock:
        osdflags = (osdflags & ~OSD_CAPS) | (-((osdflags & OSD_CAPS) == 0) & OSD_CAPS);
        break;
    case sc_Delete:
        if (osdeditcursor == osdeditlen || !osdeditlen)
            return 0;
        if (osdeditcursor <= osdeditlen-1)
            Bmemmove(osdeditbuf+osdeditcursor, osdeditbuf+osdeditcursor+1, osdeditlen-osdeditcursor-1);
        osdeditlen--;
        break;
    }
    return 0;
}


//
// OSD_ResizeDisplay() -- Handles readjustment of the display when the screen resolution
//  changes on us.
//
void OSD_ResizeDisplay(int32_t w, int32_t h)
{
    int32_t newcols;
    int32_t newmaxlines;
    char newtext[TEXTSIZE];
    char newfmt[TEXTSIZE];
    int32_t i,j,k;

    newcols = getcolumnwidth(w);
    newmaxlines = TEXTSIZE / newcols;

    j = min(newmaxlines, osdmaxlines);
    k = min(newcols, osdcols);

    Bmemset(newtext, asc_Space, TEXTSIZE);
    for (i=j-1; i>=0; i--)
    {
        Bmemcpy(newtext+newcols*i, osdtext+osdcols*i, k);
        Bmemcpy(newfmt+newcols*i, osdfmt+osdcols*i, k);
    }

    Bmemcpy(osdtext, newtext, TEXTSIZE);
    Bmemcpy(osdfmt, newfmt, TEXTSIZE);
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
// OSD_CaptureInput()
//
void OSD_CaptureInput(int32_t cap)
{
    osdflags = (osdflags & ~(OSD_CAPTURE|OSD_CTRL|OSD_SHIFT)) | (-cap & OSD_CAPTURE);

    grabmouse(cap == 0);
    onshowosd(cap);

    if (cap)
        releaseallbuttons();

    bflushchars();
}


//
// OSD_ShowDisplay() -- Shows or hides the onscreen display
//
void OSD_ShowDisplay(int32_t onf)
{
    osdflags = (osdflags & ~OSD_DRAW) | (-onf & OSD_DRAW);
    OSD_CaptureInput(onf);
}


//
// OSD_Draw() -- Draw the onscreen display
//

void OSD_Draw(void)
{
    uint32_t topoffs;
    int32_t row, lines, x, len;

    if ((osdflags & OSD_INITIALIZED) == 0)
        return;

    if (osdrowscur == 0)
        OSD_ShowDisplay(osdflags & OSD_DRAW ? 0 : 1);

    if (osdrowscur == osdrows)
        osdscroll = 0;
    else
    {
        int32_t j;

        if ((osdrowscur < osdrows && osdscroll == 1) || osdrowscur < -1)
        {
            j = (getticks()-osdscrtime);
            while (j > -1)
            {
                osdrowscur++;
                j -= 200/osdrows;
                if (osdrowscur > osdrows-1)
                    break;
            }
        }
        if ((osdrowscur > -1 && osdscroll == -1) || osdrowscur > osdrows)
        {
            j = (getticks()-osdscrtime);
            while (j > -1)
            {
                osdrowscur--;
                j -= 200/osdrows;
                if (osdrowscur < 1)
                    break;
            }
        }
        osdscrtime = getticks();
    }

    if ((osdflags & OSD_DRAW) == 0 || !osdrowscur) return;

    topoffs = osdhead * osdcols;
    row = osdrowscur-1;
    lines = min(osdlines-osdhead, osdrowscur);

    begindrawing();

    clearbackground(osdcols,osdrowscur+1);

    for (; lines>0; lines--, row--)
    {
        drawosdstr(0,row,osdtext+topoffs,osdcols,osdtextshade,osdtextpal);
        topoffs+=osdcols;
    }

    {
        int32_t offset = ((osdflags & (OSD_CAPS|OSD_SHIFT)) == (OSD_CAPS|OSD_SHIFT) && osdhead > 0);
        int32_t shade = osdpromptshade?osdpromptshade:(sintable[(totalclock<<4)&2047]>>11);

        if (osdhead == osdlines-1) drawosdchar(0,osdrowscur,'~',shade,osdpromptpal);
        else if (osdhead > 0) drawosdchar(0,osdrowscur,'^',shade,osdpromptpal);
        if (osdflags & OSD_CAPS) drawosdchar(0+(osdhead > 0),osdrowscur,'C',shade,osdpromptpal);
        if (osdflags & OSD_SHIFT) drawosdchar(1+(osdflags & OSD_CAPS && osdhead > 0),osdrowscur,'H',shade,osdpromptpal);

        drawosdchar(2+offset,osdrowscur,'>',shade,osdpromptpal);

        len = min(osdcols-1-3-offset, osdeditlen-osdeditwinstart);
        for (x=len-1; x>=0; x--)
            drawosdchar(3+x+offset,osdrowscur,osdeditbuf[osdeditwinstart+x],osdeditshade<<1,osdeditpal);

        offset += 3+osdeditcursor-osdeditwinstart;

        drawosdcursor(offset,osdrowscur,osdflags & OSD_OVERTYPE,keytime);

        if (osdver[0])
            drawosdstr(osdcols-osdverlen,osdrowscur - (offset >= osdcols-osdverlen),
                       osdver,osdverlen,(sintable[(totalclock<<4)&2047]>>11),osdverpal);
    }

    enddrawing();
}


//
// OSD_Printf() -- Print a string to the onscreen display
//   and write it to the log file
//

static inline void OSD_LineFeed(void)
{
    Bmemmove(osdtext+osdcols, osdtext, TEXTSIZE-osdcols);
    Bmemset(osdtext, asc_Space, osdcols);
    Bmemmove(osdfmt+osdcols, osdfmt, TEXTSIZE-osdcols);
    Bmemset(osdfmt, osdtextpal, osdcols);
    if (osdlines < osdmaxlines) osdlines++;
}
#define MAX_ERRORS 4096
void OSD_Printf(const char *fmt, ...)
{
    static char tmpstr[8192];
    char *chp, p=osdtextpal, s=osdtextshade;
    va_list va;

    mutex_lock(&m_osdprintf);

    va_start(va, fmt);
    Bvsnprintf(tmpstr, 8192, fmt, va);
    va_end(va);

    if (tmpstr[0]==0)
    {
        mutex_unlock(&m_osdprintf);
        return;
    }

    if (tmpstr[0]=='^' && tmpstr[1]=='1' && tmpstr[2]=='0' && ++OSD_errors > MAX_ERRORS)
    {
        if (OSD_errors == MAX_ERRORS + 1)
            Bstrcpy(tmpstr, OSD_ERROR "\nToo many errors. Logging errors stopped.\n");
        else
        {
            OSD_errors = MAX_ERRORS + 2;
            mutex_unlock(&m_osdprintf);
            return;
        }
    }

    if (linecnt < logcutoff)
    {
        if (osdlog && (!logcutoff || linecnt < logcutoff))
        {
            chp = Bstrdup(tmpstr);
            Bfputs(OSD_StripColors(chp, tmpstr), osdlog);
            Bprintf("%s", chp);
            Bfree(chp);
        }
    }
    else if (linecnt == logcutoff)
    {
        Bfputs("\nMaximal log size reached. Logging stopped.\nSet the \"logcutoff\" console variable to a higher value if you need a longer log.\n", osdlog);
        linecnt = logcutoff + 1;
    }

    chp = tmpstr;
    do
    {
        if (*chp == '\n')
        {
            osdpos=0;
            linecnt++;
            OSD_LineFeed();
            continue;
        }

        if (*chp == '\r')
        {
            osdpos=0;
            continue;
        }

        if (*chp == '^')
        {
            if (isdigit(*(chp+1)))
            {
                char smallbuf[4];
                if (!isdigit(*(++chp+1)))
                {
                    smallbuf[0] = *(chp);
                    smallbuf[1] = '\0';
                    p = Batol(smallbuf);
                    continue;
                }

                smallbuf[0] = *(chp++);
                smallbuf[1] = *(chp);
                smallbuf[2] = '\0';
                p = Batol(smallbuf);
                continue;
            }

            if (Btoupper(*(chp+1)) == 'S')
            {
                chp++;
                if (isdigit(*(++chp)))
                    s = *chp;
                continue;
            }

            if (Btoupper(*(chp+1)) == 'O')
            {
                chp++;
                p = osdtextpal;
                s = osdtextshade;
                continue;
            }
        }

        osdtext[osdpos] = *chp;
        osdfmt[osdpos++] = p+(s<<5);

        if (osdpos == osdcols)
        {
            osdpos = 0;
            OSD_LineFeed();
        }
    }
    while (*(++chp));

    mutex_unlock(&m_osdprintf);
}


//
// OSD_DispatchQueued() -- Executes any commands queued in the buffer
//
void OSD_DispatchQueued(void)
{
    int32_t cmd;

    if (!osdexeccount) return;

    cmd=osdexeccount-1;
    osdexeccount=0;

    for (; cmd>=0; cmd--)
        OSD_Dispatch((const char *)osdhistorybuf[cmd]);
}


//
// OSD_Dispatch() -- Executes a command string
//

static char *strtoken(char *s, char **ptrptr, int32_t *restart)
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
    if (*p == ';')
    {
        *restart = 1;
        *ptrptr = p+1;
        return NULL;
    }
    // or if we hit the end of the input, signal all done by nulling *ptrptr
    else if (*p == 0)
    {
        *ptrptr = NULL;
        return NULL;
    }

    if (*p == '\"')
    {
        // quoted string
        start = ++p;
        p2 = p;
        while (*p != 0)
        {
            if (*p == '\"')
            {
                p++;
                break;
            }
            else if (*p == '\\')
            {
                switch (*(++p))
                {
                case 'n':
                    *p2 = '\n'; break;
                case 'r':
                    *p2 = '\r'; break;
                default:
                    *p2 = *p; break;
                }
            }
            else
            {
                *p2 = *p;
            }
            p2++, p++;
        }
        *p2 = 0;
    }
    else
    {
        start = p;
        while (*p != 0 && *p != ';' && *p != ' ') p++;
    }

    // if we hit the end of input, signal all done by nulling *ptrptr
    if (*p == 0)
    {
        *ptrptr = NULL;
    }
    // or if we came upon a semicolon, signal caller to restart with the
    // string at *ptrptr
    else if (*p == ';')
    {
        *p = 0;
        *ptrptr = p+1;
        *restart = 1;
    }
    // otherwise, clip off the token and carry on
    else
    {
        *(p++) = 0;
        *ptrptr = p;
    }

    return start;
}

#define MAXPARMS 512
int32_t OSD_Dispatch(const char *cmd)
{
    char *workbuf, *wp, *wtp, *state;
    int32_t restart = 0;

    workbuf = state = Bstrdup(cmd);
    if (!workbuf) return -1;

    do
    {
        int32_t numparms = 0;
        symbol_t *symb;
        osdfuncparm_t ofp;
        char *parms[MAXPARMS];

        Bmemset(parms, 0, sizeof(parms));

        if ((wp = strtoken(state, &wtp, &restart)) == NULL)
        {
            state = wtp;
            continue;
        }

        if ((symb = findexactsymbol(wp)) == NULL)
        {
            if ((wp[0] != '/' || wp[1] != '/') && !m32_osd_tryscript) // cheap hack for comments in cfgs
            {
                OSD_Printf(OSDTEXT_RED "\"%s\" is not a valid command or cvar\n", wp);
            }
            else if (m32_osd_tryscript)
            {
                M32RunScript(cmd);
            }
            Bfree(workbuf);
            return -1;
        }

        ofp.name = wp;
        while (wtp && !restart)
        {
            wp = strtoken(NULL, &wtp, &restart);
            if (wp && numparms < MAXPARMS) parms[numparms++] = wp;
        }
        ofp.numparms = numparms;
        ofp.parms    = (const char **)parms;
        ofp.raw      = cmd;

        if ((intptr_t)symb->func == (intptr_t)OSD_ALIAS)
            OSD_Dispatch(symb->help);
        else if ((intptr_t)symb->func != (intptr_t)OSD_UNALIASED)
            switch (symb->func(&ofp))
            {
            case OSDCMD_OK:
                break;
            case OSDCMD_SHOWHELP:
                OSD_Printf("%s\n", symb->help);
                break;
            }

        state = wtp;
    }
    while (wtp && restart);

    Bfree(workbuf);

    return 0;
}


//
// OSD_RegisterFunction() -- Registers a new function
//
int32_t OSD_RegisterFunction(const char *name, const char *help, int32_t (*func)(const osdfuncparm_t *))
{
    symbol_t *symb;
    const char *cp;

    if ((osdflags & OSD_INITIALIZED) == 0)
        OSD_Init();

    if (!name || !name[0])
    {
        OSD_Printf("OSD_RegisterFunction(): can't register function with null name\n");
        return -1;
    }

    if (!func)
    {
        OSD_Printf("OSD_RegisterFunction(): can't register null function\n");
        return -1;
    }

    // check for illegal characters in name
    for (cp = name; *cp; cp++)
    {
        if ((cp == name) && (*cp >= '0') && (*cp <= '9'))
        {
            OSD_Printf("OSD_RegisterFunction(): first character of function name \"%s\" must not be a numeral\n", name);
            return -1;
        }
        if ((*cp < '0') ||
                (*cp > '9' && *cp < 'A') ||
                (*cp > 'Z' && *cp < 'a' && *cp != '_') ||
                (*cp > 'z'))
        {
            OSD_Printf("OSD_RegisterFunction(): illegal character in function name \"%s\"\n", name);
            return -1;
        }
    }

    if (!help) help = "(no description for this function)";

    symb = findexactsymbol(name);

    if (symb) // allow this now for reusing an alias name
    {
        /*
                if (symb->func != OSD_ALIAS && symb->func != OSD_UNALIASED)
                {
                    OSD_Printf("OSD_RegisterFunction(): \"%s\" is already defined\n", name);
                    return -1;
                }
        */
//        Bfree((char *)symb->help);
        symb->help = help;
        symb->func = func;
        return 0;
    }

    symb = addnewsymbol(name);

    if (!symb)
    {
        OSD_Printf("OSD_RegisterFunction(): Failed registering function \"%s\"\n", name);
        return -1;
    }

    symb->name = name;
    symb->help = help;
    symb->func = func;

    return 0;
}

//
// OSD_SetVersionString()
//
void OSD_SetVersion(const char *version, int32_t shade, int32_t pal)
{
    Bstrcpy(osdver,version);
    osdverlen = Bstrlen(osdver);
    osdvershade = shade;
    osdverpal = pal;
}

//
// addnewsymbol() -- Allocates space for a new symbol and attaches it
//   appropriately to the lists, sorted.
//

static symbol_t *addnewsymbol(const char *name)
{
    symbol_t *newsymb, *s, *t;

    if (osdnumsymbols >= MAXSYMBOLS) return NULL;
    newsymb = (symbol_t *)Bmalloc(sizeof(symbol_t));
    if (!newsymb) { return NULL; }
    Bmemset(newsymb, 0, sizeof(symbol_t));

    // link it to the main chain
    if (!symbols)
    {
        symbols = newsymb;
    }
    else
    {
        if (Bstrcasecmp(name, symbols->name) <= 0)
        {
            t = symbols;
            symbols = newsymb;
            symbols->next = t;
        }
        else
        {
            s = symbols;
            while (s->next)
            {
                if (Bstrcasecmp(s->next->name, name) > 0) break;
                s=s->next;
            }
            t = s->next;
            s->next = newsymb;
            newsymb->next = t;
        }
    }
    hash_add(&h_osd, name, osdnumsymbols, 1);
    name = Bstrtolower(Bstrdup(name));
    hash_add(&h_osd, name, osdnumsymbols, 1);
    Bfree((void *)name);
    osdsymbptrs[osdnumsymbols++] = newsymb;
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
        if (startingat->func != OSD_UNALIASED && !Bstrncasecmp(name, startingat->name, Bstrlen(name))) return startingat;

    return NULL;
}

//
// findexactsymbol() -- Finds a symbol, complete named
//
static symbol_t *findexactsymbol(const char *name)
{
    int32_t i;
    char *lname = Bstrdup(name);
    if (!symbols) return NULL;

    i = hash_find(&h_osd,lname);
    if (i > -1)
    {
//        if ((symbol_t *)osdsymbptrs[i]->func == OSD_UNALIASED)
//            return NULL;
        Bfree(lname);
        return osdsymbptrs[i];
    }

    // try it again
    Bstrtolower(lname);
    i = hash_find(&h_osd,lname);
    Bfree(lname);

    if (i > -1)
        return osdsymbptrs[i];
    return NULL;
}

int32_t osdcmd_cvar_set(const osdfuncparm_t *parm)
{
    int32_t showval = (parm->numparms == 0);
    int32_t i;

    i = hash_find(&h_cvars, parm->name);

    if (i < 0)
        for (i = osdnumcvars-1; i >= 0; i--)
            if (!Bstrcasecmp(parm->name, cvars[i].c.name)) break;

    if (i > -1)
    {
        if (cvars[i].c.type & CVAR_LOCKED)
        {
            // sound the alarm
            OSD_Printf("Cvar \"%s\" is read only.\n",cvars[i].c.name);
            return OSDCMD_OK;
        }

        switch (cvars[i].c.type&(CVAR_FLOAT|CVAR_DOUBLE|CVAR_INT|CVAR_UINT|CVAR_BOOL|CVAR_STRING))
        {
        case CVAR_FLOAT:
        {
            float val;
            if (showval)
            {
                OSD_Printf("\"%s\" is \"%f\"\n%s: %s\n",cvars[i].c.name,*(float *)cvars[i].c.vptr,cvars[i].c.name,(char *)cvars[i].c.desc);
                return OSDCMD_OK;
            }

            sscanf(parm->parms[0], "%f", &val);

            if (val < cvars[i].c.min || val > cvars[i].c.max)
            {
                OSD_Printf("%s value out of range\n",cvars[i].c.name);
                return OSDCMD_OK;
            }
            *(float *)cvars[i].c.vptr = val;
            if (!OSD_ParsingScript())
                OSD_Printf("%s %f",cvars[i].c.name,val);
        }
        break;
        case CVAR_DOUBLE:
        {
            double val;
            if (showval)
            {
                OSD_Printf("\"%s\" is \"%f\"\n%s: %s\n",cvars[i].c.name,*(double *)cvars[i].c.vptr,cvars[i].c.name,(char *)cvars[i].c.desc);
                return OSDCMD_OK;
            }

            sscanf(parm->parms[0], "%lf", &val);

            if (val < cvars[i].c.min || val > cvars[i].c.max)
            {
                OSD_Printf("%s value out of range\n",cvars[i].c.name);
                return OSDCMD_OK;
            }
            *(double *)cvars[i].c.vptr = val;
            if (!OSD_ParsingScript())
                OSD_Printf("%s %f",cvars[i].c.name,val);
        }
        break;
        case CVAR_INT:
        case CVAR_UINT:
        case CVAR_BOOL:
        {
            int32_t val;
            if (showval)
            {
                OSD_Printf("\"%s\" is \"%d\"\n%s: %s\n",cvars[i].c.name,*(int32_t *)cvars[i].c.vptr,cvars[i].c.name,(char *)cvars[i].c.desc);
                return OSDCMD_OK;
            }

            val = Batoi(parm->parms[0]);
            if (cvars[i].c.type & CVAR_BOOL) val = val != 0;

            if (val < cvars[i].c.min || val > cvars[i].c.max)
            {
                OSD_Printf("%s value out of range\n",cvars[i].c.name);
                return OSDCMD_OK;
            }
            *(int32_t *)cvars[i].c.vptr = val;
            if (!OSD_ParsingScript())
                OSD_Printf("%s %d",cvars[i].c.name,val);
        }
        break;
        case CVAR_STRING:
        {
            if (showval)
            {
                OSD_Printf("\"%s\" is \"%s\"\n%s: %s\n",cvars[i].c.name,(char *)cvars[i].c.vptr,cvars[i].c.name, (char *)cvars[i].c.desc);
                return OSDCMD_OK;
            }

            Bstrncpy((char *)cvars[i].c.vptr, parm->parms[0], cvars[i].c.max-1);
            ((char *)cvars[i].c.vptr)[cvars[i].c.max-1] = 0;
            if (!OSD_ParsingScript())
                OSD_Printf("%s %s",cvars[i].c.name,(char *)cvars[i].c.vptr);
        }
        break;
        default:
            break;
        }

#ifdef USE_OPENGL
        if (!OSD_ParsingScript())
        {
            switch (cvars[i].c.type&(CVAR_RESTARTVID|CVAR_INVALIDATEALL|CVAR_INVALIDATEART))
            {
            case CVAR_RESTARTVID:
                osdcmd_restartvid(NULL);
                break;
            case CVAR_INVALIDATEALL:
                gltexinvalidatetype(INVALIDATE_ALL);
            case CVAR_INVALIDATEART:
                gltexinvalidatetype(INVALIDATE_ART);
#ifdef POLYMER
                if (getrendermode() == REND_POLYMER)
                    polymer_texinvalidate();
#endif
                break;
            }
        }
#endif
    }

    if (!OSD_ParsingScript())
        OSD_Printf("\n");

    return OSDCMD_OK;
}

void OSD_WriteCvars(FILE *fp)
{
    uint32_t i;

    if (!fp)
        return;

    for (i=0; i<osdnumcvars; i++)
    {
        if (!(cvars[i].c.type & CVAR_NOSAVE) && OSD_CvarModified(&cvars[i]))
            switch (cvars[i].c.type&(CVAR_FLOAT|CVAR_DOUBLE|CVAR_INT|CVAR_UINT|CVAR_BOOL|CVAR_STRING))
            {
            case CVAR_FLOAT:
                fprintf(fp,"%s \"%f\"\n",cvars[i].c.name,*(float *)cvars[i].c.vptr);
                break;
            case CVAR_DOUBLE:
                fprintf(fp,"%s \"%f\"\n",cvars[i].c.name,*(double *)cvars[i].c.vptr);
                break;
            case CVAR_INT:
            case CVAR_UINT:
            case CVAR_BOOL:
                fprintf(fp,"%s \"%d\"\n",cvars[i].c.name,*(int32_t *)cvars[i].c.vptr);
                break;
            case CVAR_STRING:
                fprintf(fp,"%s \"%s\"\n",cvars[i].c.name,(char *)cvars[i].c.vptr);
                break;
            default:
                break;
            }
    }
}
