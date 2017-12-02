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
#define XXH_STATIC_LINKING_ONLY
#include "xxhash.h"
#include "common.h"
#include "editor.h"

static osdsymbol_t *symbols = NULL;
static osdsymbol_t *osd_addsymbol(const char *name);
static osdsymbol_t *osd_findsymbol(const char *pszName, osdsymbol_t *pSymbol);
static osdsymbol_t *osd_findexactsymbol(const char *pszName);

// static int32_t _validate_osdlines(void *);

static int32_t osdfunc_listsymbols(osdfuncparm_t const * const);
static int32_t osdfunc_help(osdfuncparm_t const * const);
static int32_t osdfunc_alias(osdfuncparm_t const * const);
// static int32_t osdfunc_dumpbuildinfo(osdfuncparm_t const * const);
// static int32_t osdfunc_setrendermode(osdfuncparm_t const * const);

static int32_t whiteColorIdx=-1;            // colour of white (used by default display routines)
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
static int32_t  osdmaxrows=20;      // maximum number of lines which can fit on the screen
BFILE *osdlog;      // log filehandle
const char* osdlogfn;
static int32_t  keytime=0;
static int32_t osdscrtime = 0;


#define editlinewidth (osd->draw.cols-1-3)

static hashtable_t h_osd = { OSDMAXSYMBOLS << 1, NULL };

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

void OSD_RegisterCvar(osdcvardata_t * const cvar, int32_t (*func)(osdfuncparm_t const * const))
{
    if (!osd)
        OSD_Init();

    osd->cvars = (osdcvar_t *)Xrealloc(osd->cvars, (osd->numcvars + 1) * sizeof(osdcvar_t));

    hash_add(&h_cvars, cvar->name, osd->numcvars, 1);

    switch (cvar->flags & CVAR_TYPEMASK)
    {
    case CVAR_FLOAT:
#if defined __POWERPC__ || defined GEKKO
        osd->cvars[osd->numcvars].defaultValue.f = *cvar->f;
        break;
#endif
    case CVAR_BOOL:
    case CVAR_INT:
    case CVAR_UINT:
        osd->cvars[osd->numcvars].defaultValue.u32 = *cvar->u32;
        break;
    case CVAR_DOUBLE:
        osd->cvars[osd->numcvars].defaultValue.d = *cvar->d;
        break;
    }

    osd->cvars[osd->numcvars++].pData = cvar;

    OSD_RegisterFunction(cvar->name, cvar->desc, func);
}

static int OSD_CvarModified(const osdcvar_t * const pCvar)
{
    if (!osd || !pCvar->pData->ptr)
        return 0;

    int rv = 0;

    switch (pCvar->pData->flags & CVAR_TYPEMASK)
    {
        case CVAR_FLOAT:
#if defined __POWERPC__ || defined GEKKO
            rv = (pCvar->defaultValue.f != *pCvar->pData->f); break;
#endif
        case CVAR_BOOL:
        case CVAR_INT:
        case CVAR_UINT:
            rv = (pCvar->defaultValue.u32 != *pCvar->pData->u32); break;
        case CVAR_DOUBLE:
            rv = (pCvar->defaultValue.d != *pCvar->pData->d); break;
        case CVAR_STRING:
            rv = 1; break;
        default:
            EDUKE32_UNREACHABLE_SECTION(break);
    }

    return rv || ((pCvar->pData->flags & CVAR_MODIFIED) == CVAR_MODIFIED);
}

// color code format is as follows:
// ^## sets a color, where ## is the palette number
// ^S# sets a shade, range is 0-7 equiv to shades 0-14
// ^O resets formatting to defaults

const char * OSD_StripColors(char *outBuf, const char *inBuf)
{
    const char *ptr = outBuf;

    while (*inBuf)
    {
        if (*inBuf == '^')
        {
            if (isdigit(*(inBuf+1)))
            {
                inBuf += 2 + !!isdigit(*(inBuf+2));
                continue;
            }
            else if ((Btoupper(*(inBuf+1)) == 'O'))
            {
                inBuf += 2;
                continue;
            }
            else if ((Btoupper(*(inBuf+1)) == 'S') && isdigit(*(inBuf+2)))
            {
                inBuf += 3;
                continue;
            }
        }
        *(outBuf++) = *(inBuf++);
    }

    *outBuf = '\0';
    return ptr;
}

int OSD_Exec(const char *szScript)
{
    int err = 0;
    int32_t handle, len;
    char *buf = NULL;

    if ((handle = kopen4load(szScript, 0)) == -1)
        err = 1;
    else if ((len = kfilelength(handle)) <= 0)
        err = 2; // blank file
    else if ((buf = (char *) Xmalloc(len + 1)) == NULL)
        err = 3;

    if (!err || err == 3)
        OSD_Printf("Executing \"%s\"\n", szScript);

    if (err || kread(handle, buf, len) != len)
    {
        if (!err || err == 3) // no error message for blank file
            OSD_Printf("Error executing \"%s\"!\n", szScript);
        if (handle != -1) kclose(handle);
        Bfree(buf);
        return 1;
    }

    kclose(handle);

    buf[len] = 0;
    osd->execdepth++;

    char const *cp = strtok(buf, "\r\n");

    while (cp != NULL)
    {
        OSD_Dispatch(cp);
        cp = strtok(NULL, "\r\n");
    }

    osd->execdepth--;
    Bfree(buf);
    return 0;
}

int32_t OSD_ParsingScript(void) { return osd->execdepth; }
int32_t OSD_OSDKey(void)        { return osd->keycode; }
int32_t OSD_GetCols(void)       { return osd->draw.cols; }
int32_t OSD_IsMoving(void)      { return (osdrowscur != -1 && osdrowscur != osd->draw.rows); }
int32_t OSD_GetRowsCur(void)    { return osdrowscur; }
int32_t OSD_GetTextMode(void)   { return osd->draw.mode; }

void OSD_GetShadePal(const char *ch, int32_t *shadeptr, int32_t *palptr)
{
    if (ch < osd->text.buf || ch >= osd->text.buf + OSDBUFFERSIZE)
        return;

    *shadeptr = (osd->text.fmt[ch - osd->text.buf] & ~0x1F) >> 4;
    *palptr   = osd->text.fmt[ch - osd->text.buf] & ~0xE0;
}

// XXX: well, converting function pointers to "data pointers" (void *) is
// undefined behavior. See
//  http://blog.frama-c.com/index.php?post/2013/08/24/Function-pointers-in-C
// Then again, my GCC just crashed (any kept on crashing until after a reboot!)
// when I tried to rewrite this into something different.

static inline void swaposdptrs(void)
{
    swapptr(&_drawosdchar,    &drawosdchar);
    swapptr(&_drawosdstr,     &drawosdstr);
    swapptr(&_drawosdcursor,  &drawosdcursor);
    swapptr(&_getcolumnwidth, &getcolumnwidth);
    swapptr(&_getrowheight,   &getrowheight);
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

static int32_t osdfunc_exec(osdfuncparm_t const * const parm)
{
    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    char fn[BMAX_PATH];

    Bstrcpy(fn,parm->parms[0]);

    if (OSD_Exec(fn))
        OSD_Printf("%sexec: file \"%s\" not found.\n", osd->draw.errorfmt, fn);

    return OSDCMD_OK;
}

static int32_t osdfunc_echo(osdfuncparm_t const * const parm)
{
    OSD_Printf("%s\n", parm->raw + 5);

    return OSDCMD_OK;
}

static int32_t osdfunc_fileinfo(osdfuncparm_t const * const parm)
{
    int32_t i,j;

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    if ((i = kopen4load(parm->parms[0],0)) < 0)
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
    XXH32_state_t xxh;
    XXH32_reset(&xxh, 0x1337);
    do
    {
        j = kread(i, buf, 256);
        XXH32_update(&xxh, (uint8_t *) buf, j);
    }
    while (j == 256);
    uint32_t xxhash = XXH32_digest(&xxh);
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

    printext256(4+(x<<3),4+(y<<3), whiteColorIdx, -1, st, 0);
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
        int32_t colidx = whiteColorIdx >= 0 ? palookup[(uint8_t)pal][whiteColorIdx] : whiteColorIdx;
        printext256(4+(x<<3),4+(y<<3), colidx, -1, st, 0);
    }
}

static void _internal_drawosdcursor(int32_t x, int32_t y, int32_t flags, int32_t lastkeypress)
{
    char st[2] = { '_',0 };

    UNREFERENCED_PARAMETER(lastkeypress);

    if (flags) st[0] = '#';

    if (whiteColorIdx > -1)
    {
        printext256(4+(x<<3),4+(y<<3)+2, whiteColorIdx, -1, st, 0);
        return;
    }

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
        if (j < k) { k = j; whiteColorIdx = i; }
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

static int32_t osdfunc_alias(osdfuncparm_t const * const parm)
{
    osdsymbol_t *i;

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

static int32_t osdfunc_unalias(osdfuncparm_t const * const parm)
{
    osdsymbol_t *i;

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

static int32_t osdfunc_listsymbols(osdfuncparm_t const * const parm)
{
    osdsymbol_t *i;
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
            OSD_Printf("%sSymbol listing for %s:\n", osd->draw.highlight, parm->parms[0]);
        else
            OSD_Printf("%sSymbol listing:\n", osd->draw.highlight);

        for (i=symbols; i!=NULL; i=i->next)
        {
            if (i->func == OSD_UNALIASED || (parm->numparms == 1 && Bstrncmp(parm->parms[0], i->name, Bstrlen(parm->parms[0]))))
                continue;

            {
                int32_t j = hash_find(&h_cvars, i->name);

                if (j != -1 && OSD_CvarModified(&osd->cvars[j]))
                {
                    OSD_Printf("%s*", osd->draw.highlight);
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
        OSD_Printf("%sFound %d symbols\n", osd->draw.highlight, count);
    }
    return OSDCMD_OK;
}

static int32_t osdfunc_help(osdfuncparm_t const * const parm)
{
    osdsymbol_t *symb;

    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    symb = osd_findexactsymbol(parm->parms[0]);

    if (!symb)
        OSD_Printf("Error: no help for undefined symbol \"%s\"\n", parm->parms[0]);
    else
        OSD_Printf("%s\n", symb->help);

    return OSDCMD_OK;
}

static int32_t osdfunc_clear(osdfuncparm_t const * const UNUSED(parm))
{
    osdtext_t *t = &osd->text;
    UNREFERENCED_CONST_PARAMETER(parm);

    Bmemset(t->buf, 0, OSDBUFFERSIZE);
    Bmemset(t->fmt, osd->draw.textpal + (osd->draw.textshade<<5), OSDBUFFERSIZE);
    t->lines = 1;

    return OSDCMD_OK;
}

static int32_t osdfunc_history(osdfuncparm_t const * const UNUSED(parm))
{
    int32_t j = 0;
    osdhist_t *h = &osd->history;
    UNREFERENCED_CONST_PARAMETER(parm);

    OSD_Printf("%sCommand history:\n", osd->draw.highlight);

    for (bssize_t i=osd->history.maxlines-1; i>=0; i--)
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

    for (osdsymbol_t *s; symbols; symbols=s)
    {
        s=symbols->next;
        Bfree(symbols);
    }

    MAYBE_FCLOSE_AND_NULL(osdlog);
    DO_FREE_AND_NULL(osd->cvars);
    DO_FREE_AND_NULL(osd->editor.buf);
    DO_FREE_AND_NULL(osd->editor.tmp);
    for (bssize_t i=0; i<OSDMAXHISTORYDEPTH; i++)
        DO_FREE_AND_NULL(osd->history.buf[i]);
    DO_FREE_AND_NULL(osd->text.buf);
    DO_FREE_AND_NULL(osd->text.fmt);
    DO_FREE_AND_NULL(osd->version.buf);
    DO_FREE_AND_NULL(osd);
}


static int32_t osdcmd_cvar_set_osd(osdfuncparm_t const * const parm)
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
        for (bssize_t i = OSDMAXHISTORYDEPTH - 1; i >= osd->history.maxlines; i--)
            DO_FREE_AND_NULL(osd->history.buf[i]);
    }

    return OSDCMD_OK;
}

static int32_t osdfunc_toggle(osdfuncparm_t const * const parm)
{
    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    intptr_t i = hash_find(&h_cvars, parm->parms[0]);

    if (i == -1)
        for (i = osd->numcvars-1; i >= 0; i--)
            if (!Bstrcasecmp(parm->parms[0], osd->cvars[i].pData->name)) break;

    if (i == -1 || (osd->cvars[i].pData->flags & CVAR_TYPEMASK) != CVAR_BOOL)
    {
        OSD_Printf("Bad cvar name or cvar not boolean\n");
        return OSDCMD_OK;
    }

    *osd->cvars[i].pData->i32 = 1 - *osd->cvars[i].pData->i32;
    osd->cvars[i].pData->flags |= CVAR_MODIFIED;

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

    osd->text.buf   = (char *)Bmalloc(OSDBUFFERSIZE);
    osd->text.fmt   = (char *)Bmalloc(OSDBUFFERSIZE);
    osd->editor.buf = (char *)Bmalloc(OSDEDITLENGTH);
    osd->editor.tmp = (char *)Bmalloc(OSDEDITLENGTH);

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

    static osdcvardata_t cvars_osd [] =
    {
        { "osdeditpal", "sets the palette of the OSD input text", (void *) &osd->draw.editpal, CVAR_INT, 0, MAXPALOOKUPS-1 },
        { "osdpromptpal", "sets the palette of the OSD prompt", (void *) &osd->draw.promptpal, CVAR_INT, 0, MAXPALOOKUPS-1 },
        { "osdtextpal", "sets the palette of the OSD text", (void *) &osd->draw.textpal, CVAR_INT, 0, MAXPALOOKUPS-1 },
        { "osdeditshade", "sets the shade of the OSD input text", (void *) &osd->draw.editshade, CVAR_INT, 0, 7 },
        { "osdtextshade", "sets the shade of the OSD text", (void *) &osd->draw.textshade, CVAR_INT, 0, 7 },
        { "osdpromptshade", "sets the shade of the OSD prompt", (void *) &osd->draw.promptshade, CVAR_INT, INT8_MIN, INT8_MAX },
        { "osdrows", "sets the number of visible lines of the OSD", (void *) &osd->draw.rows, CVAR_INT|CVAR_FUNCPTR, 1, 400 },
        { "osdtextmode", "set OSD text mode (0:graphical, 1:fast)", (void *) &osd->draw.mode, CVAR_BOOL|CVAR_FUNCPTR, 0, 1 },
        { "osdlogcutoff", "sets the maximal line count of the log file", (void *) &osd->log.cutoff, CVAR_INT, 0, 262144 },
        { "osdhistorydepth", "sets the history depth, in lines", (void *) &osd->history.maxlines, CVAR_INT|CVAR_FUNCPTR, OSDMINHISTORYDEPTH, OSDMAXHISTORYDEPTH },
    };

    for (unsigned i=0; i<ARRAY_SIZE(cvars_osd); i++)
        OSD_RegisterCvar(&cvars_osd[i], cvars_osd[i].flags & CVAR_FUNCPTR ? osdcmd_cvar_set_osd : osdcmd_cvar_set);

    OSD_RegisterFunction("alias", "alias: creates an alias for calling multiple commands", osdfunc_alias);
    OSD_RegisterFunction("clear", "clear: clears the console text buffer", osdfunc_clear);
    OSD_RegisterFunction("echo", "echo [text]: echoes text to the console", osdfunc_echo);
    OSD_RegisterFunction("exec", "exec <scriptfile>: executes a script", osdfunc_exec);
    OSD_RegisterFunction("fileinfo", "fileinfo <file>: gets a file's information", osdfunc_fileinfo);
    OSD_RegisterFunction("help", "help: displays help for a cvar or command; \"listsymbols\" to show all commands", osdfunc_help);
    OSD_RegisterFunction("history", "history: displays the console command history", osdfunc_history);
    OSD_RegisterFunction("listsymbols", "listsymbols: lists all registered functions, cvars and aliases", osdfunc_listsymbols);
    OSD_RegisterFunction("toggle", "toggle: toggles the value of a boolean cvar", osdfunc_toggle);
    OSD_RegisterFunction("unalias", "unalias: removes a command alias", osdfunc_unalias);

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
void OSD_SetFunctions(void (*drawchar)(int32_t, int32_t, char, int32_t, int32_t),
                      void (*drawstr)(int32_t, int32_t, const char *, int32_t, int32_t, int32_t),
                      void (*drawcursor)(int32_t, int32_t, int32_t, int32_t),
                      int32_t (*colwidth)(int32_t),
                      int32_t (*rowheight)(int32_t),
                      void (*clearbg)(int32_t, int32_t),
                      int32_t (*gtime)(void),
                      void (*showosd)(int32_t))
{
    drawosdchar     = drawchar   ? drawchar   : _internal_drawosdchar;
    drawosdstr      = drawstr    ? drawstr    : _internal_drawosdstr;
    drawosdcursor   = drawcursor ? drawcursor : _internal_drawosdcursor;
    getcolumnwidth  = colwidth   ? colwidth   : _internal_getcolumnwidth;
    getrowheight    = rowheight  ? rowheight  : _internal_getrowheight;
    clearbackground = clearbg    ? clearbg    : _internal_clearbackground;
    gettime         = gtime      ? gtime      : _internal_gettime;
    onshowosd       = showosd    ? showosd    : _internal_onshowosd;
}


//
// OSD_SetParameters() -- Sets the parameters for presenting the text
//
void OSD_SetParameters(int32_t promptShade, int32_t promptPal, int32_t editShade, int32_t editPal, int32_t textShade, int32_t textPal,
                       char const *const errorStr, char const *const highlight, uint32_t flags)
{
    osddraw_t &draw = osd->draw;

    draw.promptshade = promptShade;
    draw.promptpal   = promptPal;
    draw.editshade   = editShade;
    draw.editpal     = editPal;
    draw.textshade   = textShade;
    draw.textpal     = textPal;
    draw.errorfmt    = errorStr;
    draw.highlight   = highlight;

    osd->flags |= flags;
}


//
// OSD_CaptureKey() -- Sets the scancode for the key which activates the onscreen display
//
void OSD_CaptureKey(uint8_t scanCode)
{
    osd->keycode = scanCode;
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
    osdhist_t &history = osd->history;

    if (history.pos >= history.lines-1) return;

    osdedit_t &editor = osd->editor;

    Bmemcpy(editor.buf, history.buf[++history.pos], OSDEDITLENGTH);

    editor.pos = 0;
    while (editor.buf[editor.pos]) editor.pos++;
    editor.len = editor.pos;

    if (editor.pos < editor.start)
    {
        editor.end   = editor.pos;
        editor.start = editor.end - editlinewidth;

        if (editor.start < 0)
        {
            editor.end -= editor.start;
            editor.start = 0;
        }
    }
    else if (editor.pos >= editor.end)
    {
        editor.start += (editor.pos - editor.end);
        editor.end += (editor.pos - editor.end);
    }
}

static void OSD_HistoryNext(void)
{
    osdhist_t &history = osd->history;

    if (history.pos < 0) return;

    osdedit_t &editor = osd->editor;

    if (history.pos == 0)
    {
        editor.len   = 0;
        editor.pos   = 0;
        editor.start = 0;
        editor.end   = editlinewidth;
        history.pos  = -1;
        return;
    }

    Bmemcpy(editor.buf, history.buf[--history.pos], OSDEDITLENGTH);

    editor.pos = 0;
    while (editor.buf[editor.pos]) editor.pos++;
    editor.len = editor.pos;

    if (editor.pos < editor.start)
    {
        editor.end   = editor.pos;
        editor.start = editor.end - editlinewidth;

        if (editor.start < 0)
        {
            editor.end -= editor.start;
            editor.start = 0;
        }
    }
    else if (editor.pos >= editor.end)
    {
        editor.start += (editor.pos - editor.end);
        editor.end += (editor.pos - editor.end);
    }
}

//
// OSD_HandleKey() -- Handles keyboard input when capturing input.
//  Returns 0 if the key was handled internally, or the scancode if it should
//  be passed on to the game.
//

int32_t OSD_HandleChar(char ch)
{
    if (!osd || (osd->flags & OSD_CAPTURE) != OSD_CAPTURE)
        return ch;

    osdhist_t &history = osd->history;
    osdedit_t &editor  = osd->editor;

    osdsymbol_t *       tabc      = NULL;
    static osdsymbol_t *lastmatch = NULL;

    if (ch != asc_Tab)
        lastmatch = NULL;

    switch (ch)
    {
        case asc_Ctrl_A:  // jump to beginning of line
            editor.pos   = 0;
            editor.start = 0;
            editor.end   = editlinewidth;
            return 0;

        case asc_Ctrl_B:  // move one character left
            if (editor.pos > 0)
                editor.pos--;
            return 0;

        case asc_Ctrl_C:  // discard line
            editor.buf[editor.len] = 0;
            OSD_Printf("%s\n", editor.buf);

            editor.len    = 0;
            editor.pos    = 0;
            editor.start  = 0;
            editor.end    = editlinewidth;
            editor.buf[0] = 0;
            return 0;

        case asc_Ctrl_E:  // jump to end of line
            editor.pos   = editor.len;
            editor.end   = editor.pos;
            editor.start = editor.end - editlinewidth;

            if (editor.start < 0)
            {
                editor.start = 0;
                editor.end   = editlinewidth;
            }
            return 0;

        case asc_Ctrl_F:  // move one character right
            if (editor.pos < editor.len)
                editor.pos++;
            return 0;

        case asc_BackSpace:
#ifdef __APPLE__
        case 127:  // control h, backspace
#endif
            if (!editor.pos || !editor.len)
                return 0;

            if ((osd->flags & OSD_OVERTYPE) == 0)
            {
                if (editor.pos < editor.len)
                    Bmemmove(editor.buf + editor.pos - 1, editor.buf + editor.pos, editor.len - editor.pos);
                editor.len--;
            }

            if (--editor.pos < editor.start)
                editor.start--, editor.end--;
#ifndef __APPLE__
        fallthrough__;
        case 127:  // handled in OSD_HandleScanCode (delete)
#endif
            return 0;

        case asc_Tab:  // tab
        {
            int32_t commonsize = 512;

            if (!lastmatch)
            {
                int editPos, iter;

                for (editPos = editor.pos; editPos > 0; editPos--)
                    if (editor.buf[editPos - 1] == ' ')
                        break;

                for (iter = 0; editPos < editor.len && editor.buf[editPos] != ' '; iter++, editPos++)
                    editor.tmp[iter] = editor.buf[editPos];

                editor.tmp[iter] = 0;

                if (iter > 0)
                {
                    tabc = osd_findsymbol(editor.tmp, NULL);

                    if (tabc && tabc->next && osd_findsymbol(editor.tmp, tabc->next))
                    {
                        osdsymbol_t *symb     = tabc;
                        int32_t      maxwidth = 0, x = 0, num = 0, diffpt;

                        while (symb && symb != lastmatch)
                        {
                            num++;

                            if (lastmatch)
                            {
                                diffpt = OSD_FindDiffPoint(symb->name, lastmatch->name);

                                if (diffpt < commonsize)
                                    commonsize = diffpt;
                            }

                            maxwidth  = max((unsigned)maxwidth, Bstrlen(symb->name));
                            lastmatch = symb;

                            if (!lastmatch->next)
                                break;

                            symb = osd_findsymbol(editor.tmp, lastmatch->next);
                        }

                        OSD_Printf("%sFound %d possible completions for \"%s\":\n", osd->draw.highlight, num, editor.tmp);
                        maxwidth += 3;
                        symb = tabc;
                        OSD_Printf("  ");

                        while (symb && (symb != lastmatch))
                        {
                            tabc = lastmatch = symb;
                            OSD_Printf("%-*s", maxwidth, symb->name);

                            if (!lastmatch->next)
                                break;

                            symb = osd_findsymbol(editor.tmp, lastmatch->next);
                            x += maxwidth;

                            if (x > (osd->draw.cols - maxwidth))
                            {
                                x = 0;
                                OSD_Printf("\n");

                                if (symb && (symb != lastmatch))
                                    OSD_Printf("  ");
                            }
                        }

                        if (x)
                            OSD_Printf("\n");

                        OSD_Printf("%sPress TAB again to cycle through matches\n", osd->draw.highlight);
                    }
                }
            }
            else
            {
                tabc = osd_findsymbol(editor.tmp, lastmatch->next);

                if (!tabc && lastmatch)
                    tabc = osd_findsymbol(editor.tmp, NULL);  // wrap */
            }

            if (tabc)
            {
                int editPos;

                for (editPos = editor.pos; editPos > 0; editPos--)
                    if (editor.buf[editPos - 1] == ' ')
                        break;

                editor.len = editPos;

                for (int iter = 0; tabc->name[iter] && editor.len <= OSDEDITLENGTH - 1 && (editor.len < commonsize); editPos++, iter++, editor.len++)
                    editor.buf[editPos] = tabc->name[iter];

                editor.pos   = editor.len;
                editor.end   = editor.pos;
                editor.start = editor.end - editlinewidth;

                if (editor.start < 0)
                {
                    editor.start = 0;
                    editor.end   = editlinewidth;
                }

                lastmatch = tabc;
            }
            return 0;
        }

        case asc_Ctrl_K:  // delete all to end of line
            Bmemset(editor.buf + editor.pos, 0, OSDEDITLENGTH - editor.pos);
            editor.len = editor.pos;
            return 0;

        case asc_Ctrl_L:  // clear screen
            Bmemset(osd->text.buf, 0, OSDBUFFERSIZE);
            Bmemset(osd->text.fmt, osd->draw.textpal + (osd->draw.textshade << 5), OSDBUFFERSIZE);
            osd->text.lines = 1;
            return 0;

        case asc_Enter:  // control m, enter
            if (editor.len > 0)
            {
                editor.buf[editor.len] = 0;
                if (!history.buf[0] || Bstrcmp(history.buf[0], editor.buf))
                {
                    DO_FREE_AND_NULL(history.buf[history.maxlines - 1]);
                    Bmemmove(&history.buf[1], &history.buf[0], sizeof(intptr_t) * history.maxlines - 1);
                    OSD_SetHistory(0, editor.buf);

                    if (history.lines < history.maxlines)
                        history.lines++;

                    history.total++;
                }

                if (history.exec++ == history.maxlines)
                    OSD_Printf("Buffer full! Consider increasing \"osdhistorydepth\" beyond %d.\n", --history.exec);

                history.pos = -1;
            }

            editor.len   = 0;
            editor.pos   = 0;
            editor.start = 0;
            editor.end   = editlinewidth;
            return 0;

        case asc_Ctrl_N:  // next (ie. down arrow)
            OSD_HistoryNext();
            return 0;

        case asc_Ctrl_P:  // previous (ie. up arrow)
            OSD_HistoryPrev();
            return 0;

        case asc_Ctrl_U:  // delete all to beginning
            if (editor.pos > 0 && editor.len)
            {
                if (editor.pos < editor.len)
                    Bmemmove(editor.buf, editor.buf + editor.pos, editor.len - editor.pos);

                editor.len -= editor.pos;
                editor.pos   = 0;
                editor.start = 0;
                editor.end   = editlinewidth;
            }
            return 0;

        case asc_Ctrl_W:  // delete one word back
            if (editor.pos > 0 && editor.len > 0)
            {
                int editPos = editor.pos;

                while (editPos > 0 && editor.buf[editPos - 1] == asc_Space) editPos--;
                while (editPos > 0 && editor.buf[editPos - 1] != asc_Space) editPos--;

                if (editor.pos < editor.len)
                    Bmemmove(editor.buf + editPos, editor.buf + editor.pos, editor.len - editor.pos);

                editor.len -= (editor.pos - editPos);
                editor.pos = editPos;

                if (editor.pos < editor.start)
                {
                    editor.start = editor.pos;
                    editor.end   = editor.start + editlinewidth;
                }
            }
            return 0;

        default:
            if (ch >= asc_Space)  // text char
            {
                if ((osd->flags & OSD_OVERTYPE) == 0)
                {
                    if (editor.len == OSDEDITLENGTH)  // buffer full, can't insert another char
                        return 0;

                    if (editor.pos < editor.len)
                        Bmemmove(editor.buf + editor.pos + 1, editor.buf + editor.pos, editor.len - editor.pos);

                    editor.len++;
                }
                else if (editor.pos == editor.len)
                    editor.len++;

                editor.buf[editor.pos++] = ch;

                if (editor.pos > editor.end)
                    editor.start++, editor.end++;
            }
            return 0;
    }
    return 0;
}

int OSD_HandleScanCode(uint8_t scanCode, int keyDown)
{
    if (!osd)
        return 1;

    osddraw_t &draw  = osd->draw;

    if (scanCode == osd->keycode && (keystatus[sc_LeftShift] || (osd->flags & OSD_CAPTURE) || (osd->flags & OSD_PROTECTED) != OSD_PROTECTED))
    {
        if (keyDown)
        {
            draw.scrolling = (osdrowscur == -1) ? 1 :
                             (osdrowscur == draw.rows) ? -1 :
                             -draw.scrolling;
            osdrowscur += draw.scrolling;
            OSD_CaptureInput(draw.scrolling == 1);
            osdscrtime = getticks();
        }
        return -1;
    }
    else if ((osd->flags & OSD_CAPTURE) == 0)
        return 2;

    if (!keyDown)
    {
        if (scanCode == sc_LeftShift || scanCode == sc_RightShift)
            osd->flags &= ~OSD_SHIFT;

        if (scanCode == sc_LeftControl || scanCode == sc_RightControl)
            osd->flags &= ~OSD_CTRL;

        return 0;
    }

    osdedit_t &editor  = osd->editor;
    keytime = gettime();

    switch (scanCode)
    {
    case sc_Escape:
        //        OSD_ShowDisplay(0);
        draw.scrolling = -1;
        osdrowscur--;
        OSD_CaptureInput(0);
        osdscrtime = getticks();
        break;

    case sc_PgUp:
        if (draw.head < osd->text.lines-1)
            draw.head++;
        break;

    case sc_PgDn:
        if (draw.head > 0)
            draw.head--;
        break;

    case sc_Home:
        if (osd->flags & OSD_CTRL)
            draw.head = osd->text.lines-1;
        else
        {
            editor.pos   = 0;
            editor.start = editor.pos;
            editor.end   = editor.start + editlinewidth;
        }
        break;

    case sc_End:
        if (osd->flags & OSD_CTRL)
            draw.head = 0;
        else
        {
            editor.pos   = editor.len;
            editor.end   = editor.pos;
            editor.start = editor.end - editlinewidth;

            if (editor.start < 0)
            {
                editor.start = 0;
                editor.end   = editlinewidth;
            }
        }
        break;

    case sc_Insert:
        osd->flags = (osd->flags & ~OSD_OVERTYPE) | (-((osd->flags & OSD_OVERTYPE) == 0) & OSD_OVERTYPE);
        break;

    case sc_LeftArrow:
        if (editor.pos > 0)
        {
            if (osd->flags & OSD_CTRL)
            {
                while (editor.pos > 0)
                {
                    if (editor.buf[editor.pos-1] != asc_Space)
                        break;

                    editor.pos--;
                }

                while (editor.pos > 0)
                {
                    if (editor.buf[editor.pos-1] == asc_Space)
                        break;

                    editor.pos--;
                }
            }
            else editor.pos--;
        }

        if (editor.pos < editor.start)
        {
            editor.end -= (editor.start - editor.pos);
            editor.start -= (editor.start - editor.pos);
        }
        break;

    case sc_RightArrow:
        if (editor.pos < editor.len)
        {
            if (osd->flags & OSD_CTRL)
            {
                while (editor.pos < editor.len)
                {
                    if (editor.buf[editor.pos] == asc_Space)
                        break;

                    editor.pos++;
                }

                while (editor.pos < editor.len)
                {
                    if (editor.buf[editor.pos] != asc_Space)
                        break;

                    editor.pos++;
                }
            }
            else editor.pos++;
        }

        if (editor.pos >= editor.end)
        {
            editor.start += (editor.pos - editor.end);
            editor.end += (editor.pos - editor.end);
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
        if (editor.pos == editor.len || !editor.len)
            return 0;

        if (editor.pos <= editor.len-1)
            Bmemmove(editor.buf+editor.pos, editor.buf+editor.pos+1, editor.len-editor.pos-1);

        editor.len--;
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
    whiteColorIdx = -1;
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
        osd->draw.scrolling = 0;
    else
    {
        if ((osdrowscur < osd->draw.rows && osd->draw.scrolling == 1) || osdrowscur < -1)
        {
            int32_t j = (getticks()-osdscrtime);
            while (j >= 0)
            {
                osdrowscur++;
                j -= tabledivide32_noinline(200, osd->draw.rows);
                if (osdrowscur > osd->draw.rows-1)
                    break;
            }
        }
        else if ((osdrowscur > -1 && osd->draw.scrolling == -1) || osdrowscur > osd->draw.rows)
        {
            int32_t j = (getticks()-osdscrtime);
            while (j >= 0)
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
            drawosdchar(3+x+offset,osdrowscur, osd->editor.buf[osd->editor.start+x],osd->draw.editshade<<1,osd->draw.editpal);

        offset += 3+osd->editor.pos-osd->editor.start;

        drawosdcursor(offset,osdrowscur,osd->flags & OSD_OVERTYPE,keytime);

        if (osd->version.buf)
            drawosdstr(osd->draw.cols - osd->version.len, osdrowscur - (offset >= osd->draw.cols - osd->version.len),
                       osd->version.buf, osd->version.len, (sintable[(totalclock<<4)&2047]>>11), osd->version.pal);
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
    Bmemmove(osd->text.buf + osd->draw.cols, osd->text.buf, OSDBUFFERSIZE - osd->draw.cols);
    Bmemset(osd->text.buf, asc_Space, osd->draw.cols);
    Bmemmove(osd->text.fmt + osd->draw.cols, osd->text.fmt, OSDBUFFERSIZE - osd->draw.cols);
    Bmemset(osd->text.fmt, osd->draw.textpal, osd->draw.cols);

    if (osd->text.lines < osd->text.maxlines)
        osd->text.lines++;
}

#define MAX_ERRORS 4096

void OSD_Puts(const char *tmpstr)
{
    if (tmpstr[0]==0)
        return;

    const char *chp;
    uint8_t     textPal = osd->draw.textpal;
    uint8_t     textShade = osd->draw.textshade;

    mutex_lock(&osd->mutex);

    osdlog_t &log = osd->log;

    if (tmpstr[0]=='^' && tmpstr[1]=='1' && tmpstr[2]=='0' && ++log.errors > MAX_ERRORS)
    {
        if (log.errors == MAX_ERRORS + 1)
            tmpstr = "\nToo many errors. Logging errors stopped.\n";
        else
        {
            log.errors = MAX_ERRORS + 2;
            mutex_unlock(&osd->mutex);
            return;
        }
    }

    if (log.lines < log.cutoff)
    {
        if (osdlog && (!log.cutoff || log.lines < log.cutoff))
        {
            char *chp2 = Xstrdup(tmpstr);
            Bfputs(OSD_StripColors(chp2, tmpstr), osdlog);
            Bprintf("%s", chp2);
            Bfree(chp2);
        }
    }
    else if (log.lines == log.cutoff)
    {
        Bfputs("\nLog file full! Consider increasing \"osdlogcutoff\".\n", osdlog);
        log.lines = log.cutoff + 1;
    }

    chp = tmpstr;
    do
    {
        if (*chp == '\n')
        {
            osd->text.pos=0;
            log.lines++;
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
                    textPal = Batoi(smallbuf);
                    continue;
                }

                smallbuf[0] = *(chp++);
                smallbuf[1] = *(chp);
                smallbuf[2] = '\0';
                textPal = Batoi(smallbuf);
                continue;
            }

            if (Btoupper(*(chp+1)) == 'S')
            {
                chp++;
                if (isdigit(*(++chp)))
                    textShade = *chp;
                continue;
            }

            if (Btoupper(*(chp+1)) == 'O')
            {
                chp++;
                textPal = osd->draw.textpal;
                textShade = osd->draw.textshade;
                continue;
            }
        }

        osd->text.buf[osd->text.pos] = *chp;
        osd->text.fmt[osd->text.pos++] = textPal+(textShade<<5);

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

    cmd = osd->history.exec-1;
    osd->history.exec = 0;

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

#define MAXPARMS 256
int32_t OSD_Dispatch(const char *cmd)
{
    char *workbuf, *wtp, *state;
    char const *pszParam;
    int32_t restart = 0;

    workbuf = state = Xstrdup(cmd);

    do
    {
        if ((pszParam = strtoken(state, &wtp, &restart)) == NULL)
        {
            state = wtp;
            continue;
        }

        // cheap hack for comments in cfgs
        if (pszParam[0] == '/' && pszParam[1] == '/')
        {
            Bfree(workbuf);
            return -1;
        }

        osdsymbol_t const * const pSymbol = osd_findexactsymbol(pszParam);

        if (pSymbol == NULL)
        {
            static char const s_gamefunc_[]    = "gamefunc_";
            size_t const      strlen_gamefunc_ = ARRAY_SIZE(s_gamefunc_) - 1;
            size_t const      strlen_token     = Bstrlen(pszParam);

            if ((strlen_gamefunc_ >= strlen_token || Bstrncmp(pszParam, s_gamefunc_, strlen_gamefunc_)) && !m32_osd_tryscript)
                OSD_Printf("%s\"%s\" is not a valid command or cvar\n", osd->draw.highlight, pszParam);
            else if (m32_osd_tryscript)
                M32RunScript(cmd);

            Bfree(workbuf);
            return -1;
        }

        char const * const pszName  = pszParam;
        char const *parms[MAXPARMS];

        Bmemset(parms, 0, sizeof(parms));

        int32_t numparms = 0;

        while (wtp && !restart)
        {
            pszParam = strtoken(NULL, &wtp, &restart);
            if (pszParam && numparms < MAXPARMS) parms[numparms++] = pszParam;
        }

        osdfuncparm_t const ofp ={ numparms, pszName, (const char **) parms, cmd };

        if (pSymbol->func == OSD_ALIAS)
            OSD_Dispatch(pSymbol->help);
        else if (pSymbol->func != OSD_UNALIASED)
        {
            switch (pSymbol->func(&ofp))
            {
                case OSDCMD_OK: break;
                case OSDCMD_SHOWHELP: OSD_Printf("%s\n", pSymbol->help); break;
            }
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
int32_t OSD_RegisterFunction(const char *pszName, const char *pszDesc, int32_t (*func)(const osdfuncparm_t*))
{
    osdsymbol_t *symb;

    if (!osd)
        OSD_Init();

    symb = osd_findexactsymbol(pszName);

    if (symb) // allow this now for reusing an alias name
    {
        symb->help = pszDesc;
        symb->func = func;
        return 0;
    }

    symb = osd_addsymbol(pszName);

    symb->name = pszName;
    symb->help = pszDesc;
    symb->func = func;

    return 0;
}

//
// OSD_SetVersionString()
//
void OSD_SetVersion(const char *pszVersion, int osdShade, int osdPal)
{
    DO_FREE_AND_NULL(osd->version.buf);
    osd->version.buf   = Bstrdup(pszVersion);
    osd->version.len   = Bstrlen(pszVersion);
    osd->version.shade = osdShade;
    osd->version.pal   = osdPal;
}

//
// addnewsymbol() -- Allocates space for a new symbol and attaches it
//   appropriately to the lists, sorted.
//

static osdsymbol_t *osd_addsymbol(const char *pszName)
{
    if (osd->numsymbols >= OSDMAXSYMBOLS)
        return NULL;
    osdsymbol_t *const pSymbol = (osdsymbol_t *)Xcalloc(1, sizeof(osdsymbol_t));

    // link it to the main chain
    if (!symbols)
        symbols = pSymbol;
    else
    {
        if (Bstrcasecmp(pszName, symbols->name) <= 0)
        {
            osdsymbol_t * const t = symbols;
            symbols = pSymbol;
            symbols->next = t;
        }
        else
        {
            osdsymbol_t *s = symbols;

            while (s->next)
            {
                if (Bstrcasecmp(s->next->name, pszName) > 0) break;
                s=s->next;
            }

            osdsymbol_t * const t = s->next;

            s->next = pSymbol;
            pSymbol->next = t;
        }
    }

    char * const lowercase = Bstrtolower(Xstrdup(pszName));

    hash_add(&h_osd, pszName, osd->numsymbols, 1);
    hash_add(&h_osd, lowercase, osd->numsymbols, 1);
    Bfree(lowercase);

    osd->symbptrs[osd->numsymbols++] = pSymbol;

    return pSymbol;
}


//
// findsymbol() -- Finds a symbol, possibly partially named
//
static osdsymbol_t * osd_findsymbol(const char *pszName, osdsymbol_t *pSymbol)
{
    if (!pSymbol) pSymbol = symbols;
    if (!pSymbol) return NULL;

    for (; pSymbol; pSymbol=pSymbol->next)
        if (pSymbol->func != OSD_UNALIASED && !Bstrncasecmp(pszName, pSymbol->name, Bstrlen(pszName))) return pSymbol;

    return NULL;
}

//
// findexactsymbol() -- Finds a symbol, complete named
//
static osdsymbol_t * osd_findexactsymbol(const char *pszName)
{
    if (!symbols) return NULL;
    int symbolNum = hash_find(&h_osd, pszName);

    if (symbolNum < 0)
    {
        char *const lname = Xstrdup(pszName);
        Bstrtolower(lname);
        symbolNum = hash_find(&h_osd, lname);
        Bfree(lname);
    }

    return (symbolNum >= 0) ? osd->symbptrs[symbolNum] : NULL;
}

int32_t osdcmd_cvar_set(osdfuncparm_t const * const parm)
{
    int const printValue = (parm->numparms == 0);
    int const cvaridx = hash_find(&h_cvars, parm->name);

#if 0
    if (i < 0)
        for (i = osd->numcvars-1; i >= 0; i--)
            if (!Bstrcasecmp(parm->name, pData.name)) break;
#endif

    if (cvaridx >= 0)
    {
        osdcvardata_t &pData = *osd->cvars[cvaridx].pData;

        if (pData.flags & CVAR_READONLY)
        {
            OSD_Printf("Cvar \"%s\" is read only.\n", pData.name);
            return OSDCMD_OK;
        }

        switch (pData.flags & CVAR_TYPEMASK)
        {
        case CVAR_FLOAT:
        {
            if (printValue)
            {
                OSD_Printf("\"%s\" is \"%f\"\n%s: %s\n", pData.name, *pData.f, pData.name, pData.desc);
                return OSDCMD_OK;
            }

            Bsscanf(parm->parms[0], "%f", pData.f);
            *pData.f = clamp(*pData.f, pData.min, pData.max);
            pData.flags |= CVAR_MODIFIED;

            if (!OSD_ParsingScript())
                OSD_Printf("%s %f",pData.name, *pData.f);
        }
        break;
        case CVAR_DOUBLE:
        {
            if (printValue)
            {
                OSD_Printf("\"%s\" is \"%f\"\n%s: %s\n", pData.name, *pData.d, pData.name, pData.desc);
                return OSDCMD_OK;
            }

            Bsscanf(parm->parms[0], "%lf", pData.d);
            *pData.d = clamp(*pData.d, pData.min, pData.max);
            pData.flags |= CVAR_MODIFIED;

            if (!OSD_ParsingScript())
                OSD_Printf("%s %f",pData.name, *pData.d);
        }
        break;
        case CVAR_INT:
        case CVAR_BOOL:
        {
            if (printValue)
            {
                OSD_Printf("\"%s\" is \"%d\"\n%s: %s\n", pData.name, *pData.i32, pData.name, pData.desc);
                return OSDCMD_OK;
            }

            *pData.i32 = clamp(Batoi(parm->parms[0]), pData.min, pData.max);

            if ((pData.flags & CVAR_TYPEMASK) == CVAR_BOOL)
                *pData.i32 = (*pData.i32 != 0);

            pData.flags |= CVAR_MODIFIED;

            if (!OSD_ParsingScript())
                OSD_Printf("%s %d",pData.name, *pData.i32);
        }
        break;
        case CVAR_UINT:
        {
            if (printValue)
            {
                OSD_Printf("\"%s\" is \"%u\"\n%s: %s\n", pData.name, *pData.u32, pData.name, pData.desc);
                return OSDCMD_OK;
            }

            *pData.u32 = clamp(Bstrtoul(parm->parms[0], NULL, 0), pData.min, pData.max);
            pData.flags |= CVAR_MODIFIED;

            if (!OSD_ParsingScript())
                OSD_Printf("%s %d", pData.name, *pData.u32);
        }
        break;
        case CVAR_STRING:
        {
            if (printValue)
            {
                OSD_Printf("\"%s\" is \"%s\"\n%s: %s\n", pData.name, pData.string, pData.name, pData.desc);
                return OSDCMD_OK;
            }

            Bstrncpy(pData.string, parm->parms[0], pData.max-1);
            (pData.string)[pData.max-1] = 0;
            pData.flags |= CVAR_MODIFIED;

            if (!OSD_ParsingScript())
                OSD_Printf("%s %s",pData.name, pData.string);
        }
        break;
        default:
            EDUKE32_UNREACHABLE_SECTION(break);
        }

#ifdef USE_OPENGL
        if (!OSD_ParsingScript())
        {
            switch (pData.flags&(CVAR_RESTARTVID|CVAR_INVALIDATEALL|CVAR_INVALIDATEART))
            {
            case CVAR_RESTARTVID:
                osdcmd_restartvid(NULL);
                break;
            case CVAR_INVALIDATEALL:
                gltexinvalidatetype(INVALIDATE_ALL);
                fallthrough__;
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
    for (osdsymbol_t *symb=symbols; symb!=NULL; symb=symb->next)
        if (symb->func == (void *)OSD_ALIAS)
            Bfprintf(fp, "alias \"%s\" \"%s\"\n", symb->name, symb->help);
}

void OSD_WriteCvars(FILE *fp)
{
    if (!fp)
        return;

    for (unsigned i = 0; i < osd->numcvars; i++)
    {
        osdcvardata_t const &pData = *osd->cvars[i].pData;

        if (!(pData.flags & CVAR_NOSAVE) && OSD_CvarModified(&osd->cvars[i]))
            switch (pData.flags & CVAR_TYPEMASK)
            {
            case CVAR_FLOAT:  fprintf(fp, "%s \"%f\"\n", pData.name, *pData.f); break;
            case CVAR_DOUBLE: fprintf(fp, "%s \"%f\"\n", pData.name, *pData.d); break;
            case CVAR_INT:
            case CVAR_BOOL:   fprintf(fp, "%s \"%d\"\n", pData.name, *pData.i32); break;
            case CVAR_UINT:   fprintf(fp, "%s \"%u\"\n", pData.name, *pData.u32); break;
            case CVAR_STRING: fprintf(fp, "%s \"%s\"\n", pData.name, pData.string); break;
            default: EDUKE32_UNREACHABLE_SECTION(break);
            }
    }
}
