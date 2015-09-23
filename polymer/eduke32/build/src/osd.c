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
#include "xxhash.h"
#include "common.h"
#include "editor.h"

static symbol_t *symbols = NULL;
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

osdmain_t *osd = NULL;

static int32_t  osdrowscur=-1;
static int32_t  osdscroll=0;
static int32_t  osdmaxrows=20;      // maximum number of lines which can fit on the screen
BFILE *osdlog;      // log filehandle
const char* osdlogfn;
static int32_t  keytime=0;
static int32_t osdscrtime = 0;


#define editlinewidth (osd->draw.cols-1-3)


static hashtable_t h_osd      = { OSDMAXSYMBOLS<<1, NULL };

// Application callbacks: these are the currently effective ones.
static void (*drawosdchar)(int32_t, int32_t, char, int32_t, int32_t) = _internal_drawosdchar;
static void (*drawosdstr)(int32_t, int32_t, const char *, int32_t, int32_t, int32_t) = _internal_drawosdstr;
static void (*drawosdcursor)(int32_t, int32_t, int32_t, int32_t) = _internal_drawosdcursor;
static int32_t (*getcolumnwidth)(int32_t) = _internal_getcolumnwidth;
static int32_t (*getrowheight)(int32_t) = _internal_getrowheight;

static void (*clearbackground)(int32_t,int32_t) = _internal_clearbackground;
static int32_t (*gettime)(void) = _internal_gettime;
static void (*onshowosd)(int32_t) = _internal_onshowosd;

// Application callbacks: these are the backed-up ones.
static void (*_drawosdchar)(int32_t, int32_t, char, int32_t, int32_t) = _internal_drawosdchar;
static void (*_drawosdstr)(int32_t, int32_t, const char *, int32_t, int32_t, int32_t) = _internal_drawosdstr;
static void (*_drawosdcursor)(int32_t, int32_t, int32_t, int32_t) = _internal_drawosdcursor;
static int32_t (*_getcolumnwidth)(int32_t) = _internal_getcolumnwidth;
static int32_t (*_getrowheight)(int32_t) = _internal_getrowheight;

static hashtable_t h_cvars      = { OSDMAXSYMBOLS<<1, NULL };
int32_t m32_osd_tryscript=0;  // whether to try executing m32script on unkown command in the osd

int32_t OSD_RegisterCvar(const cvar_t *cvar)
{
    if (!osd)
        OSD_Init();

    if (!cvar->name || !cvar->name[0] || !cvar->vptr)
    {
        OSD_Printf("OSD_RegisterCvar(): can't register null cvar\n");
        return -1;
    }

    // check for illegal characters in name
    for (const char *cp = cvar->name; *cp; cp++)
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

    osd->cvars = (osdcvar_t *)Xrealloc(osd->cvars, (osd->numcvars + 1) * sizeof(osdcvar_t));

    hash_add(&h_cvars, cvar->name, osd->numcvars, 1);

    switch (cvar->type & (CVAR_BOOL|CVAR_INT|CVAR_UINT|CVAR_FLOAT|CVAR_DOUBLE))
    {
    case CVAR_BOOL:
    case CVAR_INT:
        osd->cvars[osd->numcvars].dval.i = *(int32_t *)cvar->vptr;
        break;
    case CVAR_UINT:
        osd->cvars[osd->numcvars].dval.uint = *(uint32_t *)cvar->vptr;
        break;
    case CVAR_FLOAT:
        osd->cvars[osd->numcvars].dval.f = *(float *)cvar->vptr;
        break;
    case CVAR_DOUBLE:
        osd->cvars[osd->numcvars].dval.d = *(double *)cvar->vptr;
        break;
    }

    Bmemcpy(&osd->cvars[osd->numcvars++], cvar, sizeof(cvar_t));

    return 0;
}

static int32_t OSD_CvarModified(const osdcvar_t *cvar)
{
    if (!osd)
        return 0;

    if (!cvar->c.vptr)
    {
        OSD_Printf("OSD_CvarModified(): null cvar?!\n");
        return 0;
    }

    int rv = 0;

    switch (cvar->c.type & (CVAR_BOOL|CVAR_INT|CVAR_UINT|CVAR_FLOAT|CVAR_DOUBLE|CVAR_STRING))
    {
    case CVAR_BOOL:
    case CVAR_INT:
        rv = (cvar->dval.i != *(int32_t *)cvar->c.vptr); break;
    case CVAR_UINT:
        rv = (cvar->dval.uint != *(uint32_t *)cvar->c.vptr); break;
    case CVAR_FLOAT:
        rv = (cvar->dval.f != *(float *)cvar->c.vptr); break;
    case CVAR_DOUBLE:
        rv = (cvar->dval.d != *(double *)cvar->c.vptr); break;
    case CVAR_STRING:
        rv = 1; break;
    default:
        EDUKE32_UNREACHABLE_SECTION(break);
    }

    return rv;
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
    int32_t i, len, err = 0;
    char *buf = NULL;

    if ((i = kopen4load(szScript, 0)) == -1) err = 1;
    if (!err && (len = kfilelength(i)) <= 0) err = 2; // blank file
    if (!err && (buf = (char *)Xmalloc(len + 1)) == NULL) err = 3;

    if (!err || err == 3)
        OSD_Printf("Executing \"%s\"\n", szScript);

    if (err || kread(i, buf, len) != len)
    {
        if (!err || err == 3) // no error message for blank file
            OSD_Printf("Error executing \"%s\"!\n", szScript);
        if (i != -1) kclose(i);
        Bfree(buf);
        return 1;
    }

    osd->execdepth++;

    buf[len] = 0;
    char *cp = strtok(buf, "\r\n");

    while (cp != NULL)
    {
        OSD_Dispatch(cp);
        cp = strtok(NULL, "\r\n");
    }

    osd->execdepth--;
    kclose(i);
    Bfree(buf);
    return 0;
}

int32_t OSD_ParsingScript(void)
{
    return osd->execdepth;
}

int32_t OSD_OSDKey(void)
{
    return osd->keycode;
}

int32_t OSD_GetCols(void)
{
    return osd->draw.cols;
}

int32_t OSD_IsMoving(void)
{
    return (osdrowscur!=-1 && osdrowscur!=osd->draw.rows);
}

int32_t OSD_GetRowsCur(void)
{
    return osdrowscur;
}

int32_t OSD_GetTextMode(void)
{
    return osd->draw.mode;
}

void OSD_GetShadePal(const char *ch, int32_t *shadeptr, int32_t *palptr)
{
    // Use format buffer when 'ch' falls inside osd->text.buf[] bounds (well,
    // almost).
    // TODO: when is this false?
    if (ch > osd->text.buf && ch < osd->text.buf + OSDBUFFERSIZE)
    {
        *shadeptr = (osd->text.fmt[ch-osd->text.buf] & ~0x1F) >> 4;
        *palptr = osd->text.fmt[ch-osd->text.buf] & ~0xE0;
    }
}

// XXX: well, converting function pointers to "data pointers" (void *) is
// undefined behavior. See
//  http://blog.frama-c.com/index.php?post/2013/08/24/Function-pointers-in-C
// Then again, my GCC just crashed (any kept on crashing until after a reboot!)
// when I tried to rewrite this into something different.

static inline void swaposdptrs(void)
{
    swapptr(&_drawosdchar, &drawosdchar);
    swapptr(&_drawosdstr, &drawosdstr);
    swapptr(&_drawosdcursor, &drawosdcursor);
    swapptr(&_getcolumnwidth, &getcolumnwidth);
    swapptr(&_getrowheight, &getrowheight);
}

void OSD_SetTextMode(int32_t mode)
{
    osd->draw.mode = (mode != 0);

    if ((osd->draw.mode && drawosdchar != _internal_drawosdchar) ||
        (!osd->draw.mode && drawosdchar == _internal_drawosdchar))
        swaposdptrs();

    if (in3dmode())
        OSD_ResizeDisplay(xdim, ydim);
}

static int32_t _internal_osdfunc_exec(const osdfuncparm_t *parm)
{
    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    char fn[BMAX_PATH];

    Bstrcpy(fn,parm->parms[0]);

    if (OSD_Exec(fn))
        OSD_Printf(OSD_ERROR "exec: file \"%s\" not found.\n", fn);

    return OSDCMD_OK;
}

static int32_t _internal_osdfunc_echo(const osdfuncparm_t *parm)
{
    OSD_Printf("%s\n", parm->raw + 5);

    return OSDCMD_OK;
}

static int32_t _internal_osdfunc_fileinfo(const osdfuncparm_t *parm)
{
    int32_t i,j;

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    if ((i = kopen4load((char *)parm->parms[0],0)) < 0)
    {
        OSD_Printf("fileinfo: File \"%s\" not found.\n", parm->parms[0]);
        return OSDCMD_OK;
    }

    char buf[256];
    uint32_t length = kfilelength(i);
    int32_t crctime = getticks();
    uint32_t crc = 0;
    do
    {
        j = kread(i,buf,256);
        crc = Bcrc32((uint8_t *)buf,j,crc);
    }
    while (j == 256);
    crctime = getticks() - crctime;

    klseek(i, 0, BSEEK_SET);

    int32_t xxhtime = getticks();
    void *xxh = XXH32_init(0x1337);
    do
    {
        j = kread(i, buf, 256);
        XXH32_update(xxh, (uint8_t *) buf, j);
    }
    while (j == 256);
    uint32_t xxhash = XXH32_digest(xxh);
    xxhtime = getticks() - xxhtime;

    kclose(i);

    OSD_Printf("fileinfo: %s\n"
               "  File size: %d\n"
               "  CRC-32:    %08X (%g sec)\n"
               "  xxHash:    %08X (%g sec)\n",
               parm->parms[0], length,
               crc, (double)crctime/gettimerfreq(),
               xxhash, (double)xxhtime/gettimerfreq());

    return OSDCMD_OK;
}

static void _internal_drawosdchar(int32_t x, int32_t y, char ch, int32_t shade, int32_t pal)
{
    UNREFERENCED_PARAMETER(shade);
    UNREFERENCED_PARAMETER(pal);

    char st[2] = { ch, 0 };

    printext256(4+(x<<3),4+(y<<3), white, -1, st, 0);
}

static void _internal_drawosdstr(int32_t x, int32_t y, const char *ch, int32_t len, int32_t shade, int32_t pal)
{
    char st[1024];

    UNREFERENCED_PARAMETER(shade);

    if (len>1023) len=1023;
    Bmemcpy(st,ch,len);
    st[len]=0;

    OSD_GetShadePal(ch, &shade, &pal);

    {
        int32_t colidx = white >= 0 ? palookup[(uint8_t)pal][white] : white;
        printext256(4+(x<<3),4+(y<<3), colidx, -1, st, 0);
    }
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
        int32_t i, k;
        // Find the palette index closest to Duke3D's brightest blue
        // "foreground" color.  (Index 79, or the last column of the 5th row,
        // if the palette is laid out in a 16x16 pattern.)
        k = INT32_MAX;
        for (i=0; i<256; i++)
        {
            int32_t j =
                klabs(curpalette[i].r - 4*47) +
                klabs(curpalette[i].g - 4*55) +
                klabs(curpalette[i].b - 4*63);
            if (j < k) { k = j; white = i; }
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

    OSD_RegisterFunction(Xstrdup(parm->parms[0]), Xstrdup(parm->parms[1]), OSD_ALIAS);

    if (!osd->execdepth)
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

    if (parm->numparms > 1)
        return OSDCMD_SHOWHELP;

    for (i=symbols; i!=NULL; i=i->next)
        if (i->func != OSD_UNALIASED)
            maxwidth = max((unsigned)maxwidth,Bstrlen(i->name));

    if (maxwidth > 0)
    {
        int32_t x = 0, count = 0;
        maxwidth += 3;

        if (parm->numparms > 0)
            OSD_Printf(OSDTEXT_RED "Symbol listing for %s:\n", parm->parms[0]);
        else
            OSD_Printf(OSDTEXT_RED "Symbol listing:\n");

        for (i=symbols; i!=NULL; i=i->next)
        {
            if (i->func == OSD_UNALIASED || (parm->numparms == 1 && Bstrncmp(parm->parms[0], i->name, Bstrlen(parm->parms[0]))))
                continue;

            {
                int32_t j = hash_find(&h_cvars, i->name);

                if (j != -1 && OSD_CvarModified(&osd->cvars[j]))
                {
                    OSD_Printf(OSDTEXT_RED "*");
                    OSD_Printf("%-*s",maxwidth-1,i->name);
                }
                else OSD_Printf("%-*s",maxwidth,i->name);

                x += maxwidth;
                count++;
            }

            if (x > osd->draw.cols - maxwidth)
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

    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    symb = findexactsymbol(parm->parms[0]);

    if (!symb)
        OSD_Printf("Error: no help for undefined symbol \"%s\"\n", parm->parms[0]);
    else
        OSD_Printf("%s\n", symb->help);

    return OSDCMD_OK;
}

static int32_t _internal_osdfunc_clear(const osdfuncparm_t *parm)
{
    osdtext_t *t = &osd->text;
    UNREFERENCED_PARAMETER(parm);

    Bmemset(t->buf, 0, OSDBUFFERSIZE);
    Bmemset(t->fmt, osd->draw.textpal + (osd->draw.textshade<<5), OSDBUFFERSIZE);
    t->lines = 1;

    return OSDCMD_OK;
}

static int32_t _internal_osdfunc_history(const osdfuncparm_t *parm)
{
    int32_t j = 0;
    osdhist_t *h = &osd->history;
    UNREFERENCED_PARAMETER(parm);

    OSD_Printf(OSDTEXT_RED "Command history:\n");

    for (int32_t i=osd->history.maxlines-1; i>=0; i--)
        if (h->buf[i])
            OSD_Printf("%4d \"%s\"\n", h->total - h->lines + (++j), h->buf[i]);

    return OSDCMD_OK;
}

////////////////////////////


//
// OSD_Cleanup() -- Cleans up the on-screen display
//
void OSD_Cleanup(void)
{
    hash_free(&h_osd);
    hash_free(&h_cvars);

    for (symbol_t *s; symbols; symbols=s)
    {
        s=symbols->next;
        Bfree(symbols);
    }

    MAYBE_FCLOSE_AND_NULL(osdlog);
    DO_FREE_AND_NULL(osd->cvars);
    DO_FREE_AND_NULL(osd->editor.buf);
    DO_FREE_AND_NULL(osd->editor.tmp);
    for (int i=0; i<OSDMAXHISTORYDEPTH; i++)
        DO_FREE_AND_NULL(osd->history.buf[i]);
    DO_FREE_AND_NULL(osd->text.buf);
    DO_FREE_AND_NULL(osd->text.fmt);
    DO_FREE_AND_NULL(osd);
}


static int32_t osdcmd_cvar_set_osd(const osdfuncparm_t *parm)
{
    int32_t r = osdcmd_cvar_set(parm);

    if (r != OSDCMD_OK)
        return r;

    if (!Bstrcasecmp(parm->name, "osdrows"))
    {
        if (osd->draw.rows > osdmaxrows)
            osd->draw.rows = osdmaxrows;
        if (osdrowscur != -1)
            osdrowscur = osd->draw.rows;
    }
    else if (!Bstrcasecmp(parm->name, "osdtextmode"))
        OSD_SetTextMode(osd->draw.mode);
    else if (!Bstrcasecmp(parm->name, "osdhistorydepth"))
    {
        for (int i = OSDMAXHISTORYDEPTH - 1; i >= osd->history.maxlines; i--)
            DO_FREE_AND_NULL(osd->history.buf[i]);
    }

    return OSDCMD_OK;
}

static int32_t _internal_osdfunc_toggle(const osdfuncparm_t *parm)
{
    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    intptr_t i = hash_find(&h_cvars, parm->parms[0]);

    if (i == -1)
        for (i = osd->numcvars-1; i >= 0; i--)
            if (!Bstrcasecmp(parm->parms[0], osd->cvars[i].c.name)) break;

    if (i == -1 || (osd->cvars[i].c.type & CVAR_BOOL) != CVAR_BOOL)
    {
        OSD_Printf("Bad cvar name or cvar not boolean\n");
        return OSDCMD_OK;
    }

    *(int32_t *)osd->cvars[i].c.vptr = 1 - *(int32_t *)osd->cvars[i].c.vptr;
    return OSDCMD_OK;
}

//
// OSD_Init() -- Initializes the on-screen display
//
void OSD_Init(void)
{
    osd = (osdmain_t *)Bcalloc(1, sizeof(osdmain_t));

    mutex_init(&osd->mutex);

    if (!osd->keycode) osd->keycode = sc_Tilde;

    osd->text.buf = (char *) Bmalloc(OSDBUFFERSIZE);
    osd->text.fmt = (char *) Bmalloc(OSDBUFFERSIZE);
    osd->editor.buf = (char *) Bmalloc(OSDEDITLENGTH);
    osd->editor.tmp = (char *) Bmalloc(OSDEDITLENGTH);

    Bmemset(osd->text.buf, asc_Space, OSDBUFFERSIZE);
    Bmemset(osd->text.fmt, osd->draw.textpal + (osd->draw.textshade<<5), OSDBUFFERSIZE);
    Bmemset(osd->symbptrs, 0, sizeof(osd->symbptrs));

    osd->numsymbols = osd->numcvars = 0;
    osd->text.lines = 1;
    osd->text.maxlines = OSDDEFAULTMAXLINES; // overwritten later
    osd->draw.rows = OSDDEFAULTROWS;
    osd->draw.cols = OSDDEFAULTCOLS;
    osd->log.cutoff = OSDLOGCUTOFF;
    osd->history.maxlines = OSDMINHISTORYDEPTH;

    hash_init(&h_osd);
    hash_init(&h_cvars);

    cvar_t cvars_osd [] =
    {
        { "osdeditpal", "sets the palette of the OSD input text", (void *) &osd->draw.editpal, CVAR_INT, 0, MAXPALOOKUPS-1 },
        { "osdpromptpal", "sets the palette of the OSD prompt", (void *) &osd->draw.promptpal, CVAR_INT, 0, MAXPALOOKUPS-1 },
        { "osdtextpal", "sets the palette of the OSD text", (void *) &osd->draw.textpal, CVAR_INT, 0, MAXPALOOKUPS-1 },
        { "osdeditshade", "sets the shade of the OSD input text", (void *) &osd->draw.editshade, CVAR_INT, 0, 7 },
        { "osdtextshade", "sets the shade of the OSD text", (void *) &osd->draw.textshade, CVAR_INT, 0, 7 },
        { "osdpromptshade", "sets the shade of the OSD prompt", (void *) &osd->draw.promptshade, CVAR_INT, INT8_MIN, INT8_MAX },
        { "osdrows", "sets the number of visible lines of the OSD", (void *) &osd->draw.rows, CVAR_INT|CVAR_FUNCPTR, 1, MAXPALOOKUPS-1 },
        { "osdtextmode", "set OSD text mode (0:graphical, 1:fast)", (void *) &osd->draw.mode, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },
        { "osdlogcutoff", "sets the maximal line count of the log file", (void *) &osd->log.cutoff, CVAR_INT, 0, 262144 },
        { "osdhistorydepth", "sets the history depth, in lines", (void *) &osd->history.maxlines, CVAR_INT|CVAR_FUNCPTR, OSDMINHISTORYDEPTH, OSDMAXHISTORYDEPTH },
    };

    for (unsigned i=0; i<ARRAY_SIZE(cvars_osd); i++)
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

//    atexit(OSD_Cleanup);
}


//
// OSD_SetLogFile() -- Sets the text file where printed text should be echoed
//
void OSD_SetLogFile(const char *fn)
{
    MAYBE_FCLOSE_AND_NULL(osdlog);
    osdlogfn = NULL;

    if (!fn)
        return;

    osdlog = Bfopen(fn, "w");

    if (osdlog)
    {
#ifdef DEBUGGINGAIDS
        const int bufmode = _IONBF;
#else
        const int bufmode = _IOLBF;
#endif
        setvbuf(osdlog, (char *)NULL, bufmode, BUFSIZ);
        osdlogfn = fn;
    }
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
    drawosdchar = drawchar ? drawchar : _internal_drawosdchar;
    drawosdstr = drawstr ? drawstr : _internal_drawosdstr;
    drawosdcursor = drawcursor ? drawcursor : _internal_drawosdcursor;
    getcolumnwidth = colwidth ? colwidth : _internal_getcolumnwidth;
    getrowheight = rowheight ? rowheight : _internal_getrowheight;
    clearbackground = clearbg ? clearbg : _internal_clearbackground;
    gettime = gtime ? gtime : _internal_gettime;
    onshowosd = showosd ? showosd : _internal_onshowosd;
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
    osd->draw.promptshade = promptshade;
    osd->draw.promptpal   = promptpal;
    osd->draw.editshade   = editshade;
    osd->draw.editpal     = editpal;
    osd->draw.textshade   = textshade;
    osd->draw.textpal     = textpal;
}


//
// OSD_CaptureKey() -- Sets the scancode for the key which activates the onscreen display
//
void OSD_CaptureKey(int32_t sc)
{
    osd->keycode = sc;
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
    if (osd->history.pos >= osd->history.lines-1) return;

    Bmemcpy(osd->editor.buf, osd->history.buf[++osd->history.pos], OSDEDITLENGTH);

    osd->editor.pos = 0;
    while (osd->editor.buf[osd->editor.pos]) osd->editor.pos++;
    osd->editor.len = osd->editor.pos;

    if (osd->editor.pos<osd->editor.start)
    {
        osd->editor.end = osd->editor.pos;
        osd->editor.start = osd->editor.end-editlinewidth;

        if (osd->editor.start<0)
        {
            osd->editor.end-=osd->editor.start;
            osd->editor.start=0;
        }
    }
    else if (osd->editor.pos>=osd->editor.end)
    {
        osd->editor.start+=(osd->editor.pos-osd->editor.end);
        osd->editor.end+=(osd->editor.pos-osd->editor.end);
    }
}

static void OSD_HistoryNext(void)
{
    if (osd->history.pos < 0) return;

    if (osd->history.pos == 0)
    {
        osd->editor.len=0;
        osd->editor.pos=0;
        osd->editor.start=0;
        osd->editor.end=editlinewidth;
        osd->history.pos = -1;
        return;
    }

    Bmemcpy(osd->editor.buf, osd->history.buf[--osd->history.pos], OSDEDITLENGTH);

    osd->editor.pos = 0;
    while (osd->editor.buf[osd->editor.pos]) osd->editor.pos++;
    osd->editor.len = osd->editor.pos;

    if (osd->editor.pos<osd->editor.start)
    {
        osd->editor.end = osd->editor.pos;
        osd->editor.start = osd->editor.end-editlinewidth;

        if (osd->editor.start<0)
        {
            osd->editor.end-=osd->editor.start;
            osd->editor.start=0;
        }
    }
    else if (osd->editor.pos>=osd->editor.end)
    {
        osd->editor.start+=(osd->editor.pos-osd->editor.end);
        osd->editor.end+=(osd->editor.pos-osd->editor.end);
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

    if (!osd || (osd->flags & OSD_CAPTURE) != OSD_CAPTURE)
        return ch;

    if (ch != 9)    // tab
        lastmatch = NULL;

    switch (ch)
    {
    case 1:    // control a. jump to beginning of line
        osd->editor.pos=0;
        osd->editor.start=0;
        osd->editor.end=editlinewidth;
        return 0;
    case 2:   // control b, move one character left
        if (osd->editor.pos > 0) osd->editor.pos--;
        return 0;
    case 3:   // control c
        osd->editor.buf[osd->editor.len] = 0;
        OSD_Printf("%s\n",osd->editor.buf);
        osd->editor.len=0;
        osd->editor.pos=0;
        osd->editor.start=0;
        osd->editor.end=editlinewidth;
        osd->editor.buf[0] = 0;
        return 0;
    case 5:   // control e, jump to end of line
        osd->editor.pos = osd->editor.len;
        osd->editor.end = osd->editor.pos;
        osd->editor.start = osd->editor.end-editlinewidth;
        if (osd->editor.start<0)
        {
            osd->editor.start=0;
            osd->editor.end = editlinewidth;
        }
        return 0;
    case 6:   // control f, move one character right
        if (osd->editor.pos < osd->editor.len) osd->editor.pos++;
        return 0;
    case 8:
#ifdef __APPLE__
    case 127:      // control h, backspace
#endif
        if (!osd->editor.pos || !osd->editor.len) return 0;
        if ((osd->flags & OSD_OVERTYPE) == 0)
        {
            if (osd->editor.pos < osd->editor.len)
                Bmemmove(osd->editor.buf+osd->editor.pos-1, osd->editor.buf+osd->editor.pos, osd->editor.len-osd->editor.pos);
            osd->editor.len--;
        }
        osd->editor.pos--;
        if (osd->editor.pos<osd->editor.start) osd->editor.start--,osd->editor.end--;
#ifndef __APPLE__
    case 127:  // handled in OSD_HandleScanCode (delete)
#endif
        return 0;
    case 9:   // tab
    {
        int32_t commonsize = 512;

        if (!lastmatch)
        {
            for (i=osd->editor.pos; i>0; i--) if (osd->editor.buf[i-1] == ' ') break;
            for (j=0; i < osd->editor.len && osd->editor.buf[i] != ' '; j++,i++)
                osd->editor.tmp[j] = osd->editor.buf[i];
            osd->editor.tmp[j] = 0;

            if (j > 0)
            {
                tabc = findsymbol(osd->editor.tmp, NULL);

                if (tabc && tabc->next && findsymbol(osd->editor.tmp, tabc->next))
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
                        symb=findsymbol(osd->editor.tmp, lastmatch->next);
                    }
                    OSD_Printf(OSDTEXT_RED "Found %d possible completions for \"%s\":\n",num,osd->editor.tmp);
                    maxwidth += 3;
                    symb = tabc;
                    OSD_Printf("  ");
                    while (symb && (symb != lastmatch))
                    {
                        tabc = lastmatch = symb;
                        OSD_Printf("%-*s",maxwidth,symb->name);
                        if (!lastmatch->next) break;
                        symb=findsymbol(osd->editor.tmp, lastmatch->next);
                        x += maxwidth;
                        if (x > (osd->draw.cols - maxwidth))
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
            tabc = findsymbol(osd->editor.tmp, lastmatch->next);
            if (!tabc && lastmatch)
                tabc = findsymbol(osd->editor.tmp, NULL);    // wrap */
        }

        if (tabc)
        {
            for (i=osd->editor.pos; i>0; i--) if (osd->editor.buf[i-1] == ' ') break;
            osd->editor.len = i;
            for (j=0; tabc->name[j] && osd->editor.len <= OSDEDITLENGTH-1
                    && (osd->editor.len < commonsize); i++,j++,osd->editor.len++)
                osd->editor.buf[i] = tabc->name[j];
            osd->editor.pos = osd->editor.len;
            osd->editor.end = osd->editor.pos;
            osd->editor.start = osd->editor.end-editlinewidth;
            if (osd->editor.start<0)
            {
                osd->editor.start=0;
                osd->editor.end = editlinewidth;
            }

            lastmatch = tabc;
        }
    }
    return 0;
    case 11:      // control k, delete all to end of line
        Bmemset(osd->editor.buf + osd->editor.pos, 0, OSDEDITLENGTH - osd->editor.pos);
        osd->editor.len = osd->editor.pos;
        return 0;
    case 12:      // control l, clear screen
        Bmemset(osd->text.buf, 0, OSDBUFFERSIZE);
        Bmemset(osd->text.fmt, osd->draw.textpal + (osd->draw.textshade<<5), OSDBUFFERSIZE);
        osd->text.lines = 1;
        return 0;
    case 13:      // control m, enter
        if (osd->editor.len>0)
        {
            osd->editor.buf[osd->editor.len] = 0;
            if (!osd->history.buf[0] || Bstrcmp(osd->history.buf[0], osd->editor.buf))
            {
                DO_FREE_AND_NULL(osd->history.buf[osd->history.maxlines-1]);

                Bmemmove(&osd->history.buf[1], &osd->history.buf[0], sizeof(intptr_t) * osd->history.maxlines-1);

                OSD_SetHistory(0, osd->editor.buf);

                if (osd->history.lines < osd->history.maxlines)
                    osd->history.lines++;

                osd->history.total++;

                if (osd->history.exec == osd->history.maxlines)
                    OSD_Printf("Command Buffer Warning: Failed queueing command "
                               "for execution. Buffer full. Consider increasing \"osdhistorydepth\".\n");
                else
                    osd->history.exec++;
            }
            else
            {
                if (osd->history.exec == osd->history.maxlines)
                    OSD_Printf("Command Buffer Warning: Failed queueing command "
                               "for execution. Buffer full. Consider increasing \"osdhistorydepth\".\n");
                else
                    osd->history.exec++;
            }
            osd->history.pos=-1;
        }

        osd->editor.len=0;
        osd->editor.pos=0;
        osd->editor.start=0;
        osd->editor.end=editlinewidth;
        return 0;
    case 14:      // control n, next (ie. down arrow)
        OSD_HistoryNext();
        return 0;
    case 16:      // control p, previous (ie. up arrow)
        OSD_HistoryPrev();
        return 0;
    case 21:      // control u, delete all to beginning
        if (osd->editor.pos>0 && osd->editor.len)
        {
            if (osd->editor.pos<osd->editor.len)
                Bmemmove(osd->editor.buf, osd->editor.buf+osd->editor.pos, osd->editor.len-osd->editor.pos);
            osd->editor.len-=osd->editor.pos;
            osd->editor.pos = 0;
            osd->editor.start = 0;
            osd->editor.end = editlinewidth;
        }
        return 0;
    case 23:      // control w, delete one word back
        if (osd->editor.pos>0 && osd->editor.len>0)
        {
            i=osd->editor.pos;
            while (i>0 && osd->editor.buf[i-1]==asc_Space) i--;
            while (i>0 && osd->editor.buf[i-1]!=asc_Space) i--;
            if (osd->editor.pos<osd->editor.len)
                Bmemmove(osd->editor.buf+i, osd->editor.buf+osd->editor.pos, osd->editor.len-osd->editor.pos);
            osd->editor.len -= (osd->editor.pos-i);
            osd->editor.pos = i;
            if (osd->editor.pos < osd->editor.start)
            {
                osd->editor.start=osd->editor.pos;
                osd->editor.end=osd->editor.start+editlinewidth;
            }
        }
        return 0;
    default:
        if (ch >= asc_Space)      // text char
        {
            if ((osd->flags & OSD_OVERTYPE) == 0)
            {
                if (osd->editor.len == OSDEDITLENGTH) // buffer full, can't insert another char
                    return 0;
                if (osd->editor.pos < osd->editor.len)
                    Bmemmove(osd->editor.buf+osd->editor.pos+1, osd->editor.buf+osd->editor.pos, osd->editor.len-osd->editor.pos);
                osd->editor.len++;
            }
            else if (osd->editor.pos == osd->editor.len)
                osd->editor.len++;

            osd->editor.buf[osd->editor.pos++] = ch;

            if (osd->editor.pos > osd->editor.end)
                osd->editor.start++, osd->editor.end++;
        }
        return 0;
    }
    return 0;
}

int32_t OSD_HandleScanCode(int32_t sc, int32_t press)
{
    if (!osd)
        return 1;

    if (sc == osd->keycode)
    {
        if (press)
        {
            osdscroll = -osdscroll;
            if (osdrowscur == -1)
                osdscroll = 1;
            else if (osdrowscur == osd->draw.rows)
                osdscroll = -1;
            osdrowscur += osdscroll;
            OSD_CaptureInput(osdscroll == 1);
            osdscrtime = getticks();
        }
        return -1;
    }
    else if ((osd->flags & OSD_CAPTURE) == 0)
        return 2;

    if (!press)
    {
        if (sc == sc_LeftShift || sc == sc_RightShift)
            osd->flags &= ~OSD_SHIFT;
        if (sc == sc_LeftControl || sc == sc_RightControl)
            osd->flags &= ~OSD_CTRL;
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
        if (osd->draw.head < osd->text.lines-1)
            osd->draw.head++;
        break;
    case sc_PgDn:
        if (osd->draw.head > 0)
            osd->draw.head--;
        break;
    case sc_Home:
        if (osd->flags & OSD_CTRL)
            osd->draw.head = osd->text.lines-1;
        else
        {
            osd->editor.pos = 0;
            osd->editor.start = osd->editor.pos;
            osd->editor.end = osd->editor.start+editlinewidth;
        }
        break;
    case sc_End:
        if (osd->flags & OSD_CTRL)
            osd->draw.head = 0;
        else
        {
            osd->editor.pos = osd->editor.len;
            osd->editor.end = osd->editor.pos;
            osd->editor.start = osd->editor.end-editlinewidth;
            if (osd->editor.start<0)
            {
                osd->editor.start=0;
                osd->editor.end = editlinewidth;
            }
        }
        break;
    case sc_Insert:
        osd->flags = (osd->flags & ~OSD_OVERTYPE) | (-((osd->flags & OSD_OVERTYPE) == 0) & OSD_OVERTYPE);
        break;
    case sc_LeftArrow:
        if (osd->editor.pos>0)
        {
            if (osd->flags & OSD_CTRL)
            {
                while (osd->editor.pos>0)
                {
                    if (osd->editor.buf[osd->editor.pos-1] != asc_Space)
                        break;
                    osd->editor.pos--;
                }
                while (osd->editor.pos>0)
                {
                    if (osd->editor.buf[osd->editor.pos-1] == asc_Space)
                        break;
                    osd->editor.pos--;
                }
            }
            else osd->editor.pos--;
        }
        if (osd->editor.pos<osd->editor.start)
        {
            osd->editor.end-=(osd->editor.start-osd->editor.pos);
            osd->editor.start-=(osd->editor.start-osd->editor.pos);
        }
        break;
    case sc_RightArrow:
        if (osd->editor.pos<osd->editor.len)
        {
            if (osd->flags & OSD_CTRL)
            {
                while (osd->editor.pos<osd->editor.len)
                {
                    if (osd->editor.buf[osd->editor.pos] == asc_Space)
                        break;
                    osd->editor.pos++;
                }
                while (osd->editor.pos<osd->editor.len)
                {
                    if (osd->editor.buf[osd->editor.pos] != asc_Space)
                        break;
                    osd->editor.pos++;
                }
            }
            else osd->editor.pos++;
        }
        if (osd->editor.pos>=osd->editor.end)
        {
            osd->editor.start+=(osd->editor.pos-osd->editor.end);
            osd->editor.end+=(osd->editor.pos-osd->editor.end);
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
        osd->flags |= OSD_SHIFT;
        break;
    case sc_LeftControl:
    case sc_RightControl:
        osd->flags |= OSD_CTRL;
        break;
    case sc_CapsLock:
        osd->flags = (osd->flags & ~OSD_CAPS) | (-((osd->flags & OSD_CAPS) == 0) & OSD_CAPS);
        break;
    case sc_Delete:
        if (osd->editor.pos == osd->editor.len || !osd->editor.len)
            return 0;
        if (osd->editor.pos <= osd->editor.len-1)
            Bmemmove(osd->editor.buf+osd->editor.pos, osd->editor.buf+osd->editor.pos+1, osd->editor.len-osd->editor.pos-1);
        osd->editor.len--;
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
    int32_t newcols, newmaxlines;
    char *newtext, *newfmt;
    int32_t i,j,k;

    newcols = getcolumnwidth(w);
    newmaxlines = OSDBUFFERSIZE / newcols;

    j = min(newmaxlines, osd->text.maxlines);
    k = min(newcols, osd->draw.cols);

    newtext = (char *)Bmalloc(OSDBUFFERSIZE);
    newfmt = (char *)Bmalloc(OSDBUFFERSIZE);

    Bmemset(newtext, asc_Space, OSDBUFFERSIZE);

    for (i=j-1; i>=0; i--)
    {
        Bmemcpy(newtext+newcols*i, osd->text.buf+osd->draw.cols*i, k);
        Bmemcpy(newfmt+newcols*i, osd->text.fmt+osd->draw.cols*i, k);
    }

    Bfree(osd->text.buf);
    osd->text.buf = newtext;

    Bfree(osd->text.fmt);
    osd->text.fmt = newfmt;

    osd->text.maxlines = newmaxlines;

    osd->draw.cols = newcols;
    osdmaxrows = getrowheight(h)-2;

    if (osd->draw.rows > osdmaxrows) osd->draw.rows = osdmaxrows;

    osd->text.pos = 0;
    osd->draw.head = 0;
    osd->editor.start = 0;
    osd->editor.end = editlinewidth;
    white = -1;
}


//
// OSD_CaptureInput()
//
void OSD_CaptureInput(int32_t cap)
{
    osd->flags = (osd->flags & ~(OSD_CAPTURE|OSD_CTRL|OSD_SHIFT)) | (-cap & OSD_CAPTURE);

    grabmouse(cap == 0 ? AppMouseGrab : 0);
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
    osd->flags = (osd->flags & ~OSD_DRAW) | (-onf & OSD_DRAW);
    OSD_CaptureInput(onf);
}


//
// OSD_Draw() -- Draw the onscreen display
//

void OSD_Draw(void)
{
    uint32_t topoffs;
    int32_t row, lines, x, len;

    if (!osd)
        return;

    if (osdrowscur == 0)
        OSD_ShowDisplay(osd->flags & OSD_DRAW ? 0 : 1);

    if (osdrowscur == osd->draw.rows)
        osdscroll = 0;
    else
    {
        int32_t j;

        if ((osdrowscur < osd->draw.rows && osdscroll == 1) || osdrowscur < -1)
        {
            j = (getticks()-osdscrtime);
            while (j > -1)
            {
                osdrowscur++;
                j -= tabledivide32_noinline(200, osd->draw.rows);
                if (osdrowscur > osd->draw.rows-1)
                    break;
            }
        }
        if ((osdrowscur > -1 && osdscroll == -1) || osdrowscur > osd->draw.rows)
        {
            j = (getticks()-osdscrtime);
            while (j > -1)
            {
                osdrowscur--;
                j -= tabledivide32_noinline(200, osd->draw.rows);
                if (osdrowscur < 1)
                    break;
            }
        }
        osdscrtime = getticks();
    }

    if ((osd->flags & OSD_DRAW) == 0 || !osdrowscur) return;

    topoffs = osd->draw.head * osd->draw.cols;
    row = osdrowscur-1;
    lines = min(osd->text.lines-osd->draw.head, osdrowscur);

    begindrawing();

    clearbackground(osd->draw.cols,osdrowscur+1);

    for (; lines>0; lines--, row--)
    {
        // XXX: May happen, which would ensue an oob if not checked.
        // Last char accessed is osd->text.buf[topoffs + osd->draw.cols-1].
        // Reproducible by running test.lua with -Lopts=diag
        // and scrolling to the top.
        if (topoffs + osd->draw.cols-1 >= OSDBUFFERSIZE)
            break;
        drawosdstr(0,row,osd->text.buf+topoffs,osd->draw.cols,osd->draw.textshade,osd->draw.textpal);
        topoffs+=osd->draw.cols;
    }

    {
        int32_t offset = ((osd->flags & (OSD_CAPS|OSD_SHIFT)) == (OSD_CAPS|OSD_SHIFT) && osd->draw.head > 0);
        int32_t shade = osd->draw.promptshade?osd->draw.promptshade:(sintable[(totalclock<<4)&2047]>>11);

        if (osd->draw.head == osd->text.lines-1) drawosdchar(0,osdrowscur,'~',shade,osd->draw.promptpal);
        else if (osd->draw.head > 0) drawosdchar(0,osdrowscur,'^',shade,osd->draw.promptpal);
        if (osd->flags & OSD_CAPS) drawosdchar(0+(osd->draw.head > 0),osdrowscur,'C',shade,osd->draw.promptpal);
        if (osd->flags & OSD_SHIFT) drawosdchar(1+(osd->flags & OSD_CAPS && osd->draw.head > 0),osdrowscur,'H',shade,osd->draw.promptpal);

        drawosdchar(2+offset,osdrowscur,'>',shade,osd->draw.promptpal);

        len = min(osd->draw.cols-1-3-offset, osd->editor.len-osd->editor.start);
        for (x=len-1; x>=0; x--)
            drawosdchar(3+x+offset,osdrowscur,osd->editor.buf[osd->editor.start+x],osd->draw.editshade<<1,osd->draw.editpal);

        offset += 3+osd->editor.pos-osd->editor.start;

        drawosdcursor(offset,osdrowscur,osd->flags & OSD_OVERTYPE,keytime);

        if (osd->verstr.buf)
            drawosdstr(osd->draw.cols - osd->verstr.len, osdrowscur - (offset >= osd->draw.cols - osd->verstr.len),
                       osd->verstr.buf, osd->verstr.len, (sintable[(totalclock<<4)&2047]>>11), osd->verstr.pal);
    }

    enddrawing();
}


//
// OSD_Printf() -- Print a formatted string to the onscreen display
//   and write it to the log file
//

void OSD_Printf(const char *fmt, ...)
{
    static char tmpstr[8192];
    va_list va;

    va_start(va, fmt);
    Bvsnprintf(tmpstr, sizeof(tmpstr), fmt, va);
    va_end(va);

    OSD_Puts(tmpstr);
}


//
// OSD_Puts() -- Print a string to the onscreen display
//   and write it to the log file
//

static inline void OSD_LineFeed(void)
{
    Bmemmove(osd->text.buf+osd->draw.cols, osd->text.buf, OSDBUFFERSIZE-osd->draw.cols);
    Bmemset(osd->text.buf, asc_Space, osd->draw.cols);
    Bmemmove(osd->text.fmt+osd->draw.cols, osd->text.fmt, OSDBUFFERSIZE-osd->draw.cols);
    Bmemset(osd->text.fmt, osd->draw.textpal, osd->draw.cols);
    if (osd->text.lines < osd->text.maxlines) osd->text.lines++;
}

#define MAX_ERRORS 4096

void OSD_Puts(const char *tmpstr)
{
    const char *chp;
    char p=osd->draw.textpal, s=osd->draw.textshade;

    mutex_lock(&osd->mutex);

    if (tmpstr[0]==0)
    {
        mutex_unlock(&osd->mutex);
        return;
    }

    if (tmpstr[0]=='^' && tmpstr[1]=='1' && tmpstr[2]=='0' && ++osd->log.errors > MAX_ERRORS)
    {
        if (osd->log.errors == MAX_ERRORS + 1)
            tmpstr = "\nToo many errors. Logging errors stopped.\n";
        else
        {
            osd->log.errors = MAX_ERRORS + 2;
            mutex_unlock(&osd->mutex);
            return;
        }
    }

    if (osd->log.lines < osd->log.cutoff)
    {
        if (osdlog && (!osd->log.cutoff || osd->log.lines < osd->log.cutoff))
        {
            char *chp2 = Xstrdup(tmpstr);
            Bfputs(OSD_StripColors(chp2, tmpstr), osdlog);
            Bprintf("%s", chp2);
            Bfree(chp2);
        }
    }
    else if (osd->log.lines == osd->log.cutoff)
    {
        Bfputs("\nMaximal log size reached. Logging stopped.\nSet the \"osdlogcutoff\" console variable to a higher value if you need a longer log.\n", osdlog);
        osd->log.lines = osd->log.cutoff + 1;
    }

    chp = tmpstr;
    do
    {
        if (*chp == '\n')
        {
            osd->text.pos=0;
            osd->log.lines++;
            OSD_LineFeed();
            continue;
        }

        if (*chp == '\r')
        {
            osd->text.pos=0;
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
                p = osd->draw.textpal;
                s = osd->draw.textshade;
                continue;
            }
        }

        osd->text.buf[osd->text.pos] = *chp;
        osd->text.fmt[osd->text.pos++] = p+(s<<5);

        if (osd->text.pos == osd->draw.cols)
        {
            osd->text.pos = 0;
            OSD_LineFeed();
        }
    }
    while (*(++chp));

    mutex_unlock(&osd->mutex);
}


//
// OSD_DispatchQueued() -- Executes any commands queued in the buffer
//
void OSD_DispatchQueued(void)
{
    int32_t cmd;

    if (!osd->history.exec) return;

    cmd=osd->history.exec-1;
    osd->history.exec=0;

    for (; cmd>=0; cmd--)
        OSD_Dispatch((const char *)osd->history.buf[cmd]);
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

    workbuf = state = Xstrdup(cmd);

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

    if (!osd)
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
    osdstr_t *v = &osd->verstr;
    DO_FREE_AND_NULL(v->buf);
    v->buf = Bstrdup(version);
    v->len = Bstrlen(version);
    v->shade = shade;
    v->pal = pal;
}

//
// addnewsymbol() -- Allocates space for a new symbol and attaches it
//   appropriately to the lists, sorted.
//

static symbol_t *addnewsymbol(const char *name)
{
    symbol_t *newsymb, *s, *t;

    if (osd->numsymbols >= OSDMAXSYMBOLS) return NULL;
    newsymb = (symbol_t *)Xmalloc(sizeof(symbol_t));
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
    hash_add(&h_osd, name, osd->numsymbols, 1);
    char * const newname = Bstrtolower(Xstrdup(name));
    hash_add(&h_osd, newname, osd->numsymbols, 1);
    Bfree(newname);
    osd->symbptrs[osd->numsymbols++] = newsymb;
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
    if (!symbols) return NULL;

    char *lname = Xstrdup(name);

    intptr_t i = hash_find(&h_osd,lname);

    if (i > -1)
    {
//        if ((symbol_t *)osdsymbptrs[i]->func == OSD_UNALIASED)
//            return NULL;
        Bfree(lname);
        return osd->symbptrs[i];
    }

    // try it again
    Bstrtolower(lname);
    i = hash_find(&h_osd,lname);
    Bfree(lname);

    return (i > -1) ? osd->symbptrs[i] : NULL;
}

int32_t osdcmd_cvar_set(const osdfuncparm_t *parm)
{
    int32_t showval = (parm->numparms == 0);
    intptr_t i = hash_find(&h_cvars, parm->name);

    if (i < 0)
        for (i = osd->numcvars-1; i >= 0; i--)
            if (!Bstrcasecmp(parm->name, osd->cvars[i].c.name)) break;

    if (i > -1)
    {
        if (osd->cvars[i].c.type & CVAR_LOCKED)
        {
            // sound the alarm
            OSD_Printf("Cvar \"%s\" is read only.\n",osd->cvars[i].c.name);
            return OSDCMD_OK;
        }

        switch (osd->cvars[i].c.type&(CVAR_FLOAT|CVAR_DOUBLE|CVAR_INT|CVAR_UINT|CVAR_BOOL|CVAR_STRING))
        {
        case CVAR_FLOAT:
        {
            if (showval)
            {
                OSD_Printf("\"%s\" is \"%f\"\n%s: %s\n",osd->cvars[i].c.name,*(float *)osd->cvars[i].c.vptr,osd->cvars[i].c.name,(char *)osd->cvars[i].c.desc);
                return OSDCMD_OK;
            }

            float val;
            Bsscanf(parm->parms[0], "%f", &val);

            if (val < osd->cvars[i].c.min || val > osd->cvars[i].c.max)
            {
                OSD_Printf("%s value out of range\n",osd->cvars[i].c.name);
                return OSDCMD_OK;
            }
            *(float *)osd->cvars[i].c.vptr = val;
            if (!OSD_ParsingScript())
                OSD_Printf("%s %f",osd->cvars[i].c.name,val);
        }
        break;
        case CVAR_DOUBLE:
        {
            if (showval)
            {
                OSD_Printf("\"%s\" is \"%f\"\n%s: %s\n",osd->cvars[i].c.name,*(double *)osd->cvars[i].c.vptr,osd->cvars[i].c.name,(char *)osd->cvars[i].c.desc);
                return OSDCMD_OK;
            }

            double val;
            Bsscanf(parm->parms[0], "%lf", &val);

            if (val < osd->cvars[i].c.min || val > osd->cvars[i].c.max)
            {
                OSD_Printf("%s value out of range\n",osd->cvars[i].c.name);
                return OSDCMD_OK;
            }
            *(double *)osd->cvars[i].c.vptr = val;
            if (!OSD_ParsingScript())
                OSD_Printf("%s %f",osd->cvars[i].c.name,val);
        }
        break;
        case CVAR_INT:
        case CVAR_UINT:
        case CVAR_BOOL:
        {
            if (showval)
            {
                OSD_Printf((osd->cvars[i].c.type & CVAR_UINT) ? "\"%s\" is \"%u\"\n%s: %s\n" : "\"%s\" is \"%d\"\n%s: %s\n",osd->cvars[i].c.name,*(int32_t *)osd->cvars[i].c.vptr,osd->cvars[i].c.name,(char *)osd->cvars[i].c.desc);
                return OSDCMD_OK;
            }

            int32_t val = Batoi(parm->parms[0]);
            if (osd->cvars[i].c.type & CVAR_BOOL) val = val != 0;

            if (val < osd->cvars[i].c.min || ((osd->cvars[i].c.type & CVAR_UINT) ? ((unsigned) val > (unsigned) osd->cvars[i].c.max) : (val > osd->cvars[i].c.max)))
            {
                OSD_Printf("%s value out of range\n",osd->cvars[i].c.name);
                return OSDCMD_OK;
            }
            *(int32_t *)osd->cvars[i].c.vptr = val;
            if (!OSD_ParsingScript())
                OSD_Printf("%s %d",osd->cvars[i].c.name,val);
        }
        break;
        case CVAR_STRING:
        {
            if (showval)
            {
                OSD_Printf("\"%s\" is \"%s\"\n%s: %s\n",osd->cvars[i].c.name,(char *)osd->cvars[i].c.vptr,osd->cvars[i].c.name, (char *)osd->cvars[i].c.desc);
                return OSDCMD_OK;
            }

            Bstrncpy((char *)osd->cvars[i].c.vptr, parm->parms[0], osd->cvars[i].c.max-1);
            ((char *)osd->cvars[i].c.vptr)[osd->cvars[i].c.max-1] = 0;
            if (!OSD_ParsingScript())
                OSD_Printf("%s %s",osd->cvars[i].c.name,(char *)osd->cvars[i].c.vptr);
        }
        break;
        default:
            EDUKE32_UNREACHABLE_SECTION(break);
        }

#ifdef USE_OPENGL
        if (!OSD_ParsingScript())
        {
            switch (osd->cvars[i].c.type&(CVAR_RESTARTVID|CVAR_INVALIDATEALL|CVAR_INVALIDATEART))
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

void OSD_WriteAliases(FILE *fp)
{
    for (symbol_t *symb=symbols; symb!=NULL; symb=symb->next)
        if (symb->func == (void *)OSD_ALIAS)
            Bfprintf(fp, "alias \"%s\" \"%s\"\n", symb->name, symb->help);
}

void OSD_WriteCvars(FILE *fp)
{
    if (!fp)
        return;

    for (unsigned i=0; i<osd->numcvars; i++)
    {
        if (!(osd->cvars[i].c.type & CVAR_NOSAVE) && OSD_CvarModified(&osd->cvars[i]))
            switch (osd->cvars[i].c.type&(CVAR_FLOAT|CVAR_DOUBLE|CVAR_INT|CVAR_UINT|CVAR_BOOL|CVAR_STRING))
            {
            case CVAR_FLOAT:
                fprintf(fp,"%s \"%f\"\n",osd->cvars[i].c.name,*(float *)osd->cvars[i].c.vptr);
                break;
            case CVAR_DOUBLE:
                fprintf(fp,"%s \"%f\"\n",osd->cvars[i].c.name,*(double *)osd->cvars[i].c.vptr);
                break;
            case CVAR_INT:
            case CVAR_BOOL:
                fprintf(fp,"%s \"%d\"\n",osd->cvars[i].c.name,*(int32_t *)osd->cvars[i].c.vptr);
                break;
            case CVAR_UINT:
                fprintf(fp,"%s \"%u\"\n",osd->cvars[i].c.name,*(uint32_t *)osd->cvars[i].c.vptr);
                break;
            case CVAR_STRING:
                fprintf(fp,"%s \"%s\"\n",osd->cvars[i].c.name,(char *)osd->cvars[i].c.vptr);
                break;
            default:
                EDUKE32_UNREACHABLE_SECTION(break);
            }
    }
}
