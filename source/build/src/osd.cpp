// On-screen Display (ie. console)
// for the Build Engine
// by Jonathon Fowler (jf@jonof.id.au)

#include "build.h"
#include "cache1d.h"
#include "crc32.h"
#include "editor.h"
#include "osd.h"
#include "scancodes.h"

#define XXH_STATIC_LINKING_ONLY
#include "xxhash.h"

#include "vfs.h"

static osdsymbol_t *osd_addsymbol(const char *name);
static osdsymbol_t *osd_findsymbol(const char *pszName, osdsymbol_t *pSymbol);
static osdsymbol_t *osd_findexactsymbol(const char *pszName);

static int32_t whiteColorIdx=-1;            // colour of white (used by default display routines)
static void _internal_drawosdchar(int32_t, int32_t, char, int32_t, int32_t);
static void _internal_drawosdstr(int32_t, int32_t, const char *, int32_t, int32_t, int32_t);
static void _internal_drawosdcursor(int32_t,int32_t,int32_t,int32_t);
static int32_t _internal_getcolumnwidth(int32_t);
static int32_t _internal_getrowheight(int32_t);
static void _internal_clearbackground(int32_t,int32_t);
static int32_t _internal_gettime(void);
static void _internal_onshowosd(int32_t);

osdmain_t *osd;

static int osdrowscur = -1;
static int osdmaxrows = 20;

buildvfs_FILE osdlog;

const char *osdlogfn;

static uint32_t osdkeytime = 0;
static uint32_t osdscrtime = 0;

#define OSD_EDIT_LINE_WIDTH (osd->draw.cols - 1 - 3)

static hashtable_t h_osd = { OSDMAXSYMBOLS >> 1, NULL };

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

static hashtable_t h_cvars      = { OSDMAXSYMBOLS >> 1, NULL };
bool m32_osd_tryscript = false;  // whether to try executing m32script on unkown command in the osd

void OSD_RegisterCvar(osdcvardata_t * const cvar, int (*func)(osdcmdptr_t))
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
    int32_t len = 0;
    buildvfs_kfd handle;

    if ((handle = kopen4load(szScript, 0)) == buildvfs_kfd_invalid)
        err = 1;
    else if ((len = kfilelength(handle)) <= 0)
        err = 2; // blank file

    if (!err)
        OSD_Printf("Executing \"%s\"\n", szScript);

    auto buf = (char *) Xmalloc(len + 1);

    if (err || kread(handle, buf, len) != len)
    {
        if (!err) // no error message for blank file
            OSD_Printf("Error executing \"%s\"!\n", szScript);

        if (handle != buildvfs_kfd_invalid)
            kclose(handle);

        Xfree(buf);
        return 1;
    }

    kclose(handle);
    buf[len] = '\0';

    char const *cp = strtok(buf, "\r\n");

    ++osd->execdepth;
    while (cp != NULL)
    {
        OSD_Dispatch(cp);
        cp = strtok(NULL, "\r\n");
    }
    --osd->execdepth;

    Xfree(buf);
    return 0;
}

int OSD_ParsingScript(void) { return osd->execdepth; }
int OSD_OSDKey(void)        { return osd->keycode; }
int OSD_GetCols(void)       { return osd->draw.cols; }
int OSD_IsMoving(void)      { return (osdrowscur != -1 && osdrowscur != osd->draw.rows); }
int OSD_GetRowsCur(void)    { return osdrowscur; }
int OSD_GetTextMode(void)   { return osd->draw.mode; }

void OSD_GetShadePal(const char *ch, int *shd, int *pal)
{
    auto &t = osd->text;

    if (ch < t.buf || ch >= t.buf + OSDBUFFERSIZE)
        return;

    *shd = (t.fmt[ch-t.buf] & ~0x1F)>>4;
    *pal = (t.fmt[ch-t.buf] & ~0xE0);
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

void OSD_SetTextMode(int mode)
{
    osd->draw.mode = (mode != 0);

    if ((osd->draw.mode && drawosdchar != _internal_drawosdchar) ||
        (!osd->draw.mode && drawosdchar == _internal_drawosdchar))
        swaposdptrs();

    if (in3dmode())
        OSD_ResizeDisplay(xdim, ydim);
}

static int osdfunc_exec(osdcmdptr_t parm)
{
    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    if (OSD_Exec(parm->parms[0]))
        OSD_Printf("%sexec: file \"%s\" not found.\n", osd->draw.errorfmt, parm->parms[0]);

    return OSDCMD_OK;
}

static int osdfunc_echo(osdcmdptr_t parm)
{
    OSD_Printf("%s\n", parm->raw + 5);

    return OSDCMD_OK;
}

static int osdfunc_fileinfo(osdcmdptr_t parm)
{
    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    buildvfs_kfd h;

    if ((h = kopen4load(parm->parms[0],0)) == buildvfs_kfd_invalid)
    {
        OSD_Printf("fileinfo: File \"%s\" not found.\n", parm->parms[0]);
        return OSDCMD_OK;
    }

    int32_t  crctime = timerGetTicks();
    uint32_t crcval  = 0;
    int32_t  siz     = 0;

    static constexpr int ReadSize = 65536;
    auto buf = (uint8_t *)Xmalloc(ReadSize);

    do
    {
        siz   = kread(h, buf, ReadSize);
        crcval = Bcrc32((uint8_t *)buf, siz, crcval);
    }
    while (siz == ReadSize);

    crctime = timerGetTicks() - crctime;

    klseek(h, 0, BSEEK_SET);

    int32_t xxhtime = timerGetTicks();

    XXH32_state_t xxh;
    XXH32_reset(&xxh, 0x1337);

    do
    {
        siz = kread(h, buf, ReadSize);
        XXH32_update(&xxh, (uint8_t *)buf, siz);
    }
    while (siz == ReadSize);

    uint32_t const xxhash = XXH32_digest(&xxh);
    xxhtime = timerGetTicks() - xxhtime;

    Xfree(buf);

    OSD_Printf("fileinfo: %s\n"
               "  File size: %d\n"
               "  CRC-32:    %08X (%g sec)\n"
               "  xxHash:    %08X (%g sec)\n",
               parm->parms[0], kfilelength(h),
               crcval, (double)crctime/timerGetRate(),
               xxhash, (double)xxhtime/timerGetRate());

    kclose(h);

    return OSDCMD_OK;
}

static void _internal_drawosdchar(int x, int y, char ch, int shade, int pal)
{
    UNREFERENCED_PARAMETER(shade);
    UNREFERENCED_PARAMETER(pal);

    char st[2] = { ch, 0 };

    printext256(4+(x<<3),4+(y<<3), whiteColorIdx, -1, st, 0);
}

static void _internal_drawosdstr(int x, int y, const char *ch, int len, int shade, int pal)
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

static void _internal_drawosdcursor(int x, int y, int flags, int lastkeypress)
{
    char st[2] = { '_',0 };

    UNREFERENCED_PARAMETER(lastkeypress);

    if (flags) st[0] = '#';

    if (whiteColorIdx > -1)
    {
        printext256(4+(x<<3),4+(y<<3)+2, whiteColorIdx, -1, st, 0);
        return;
    }

    // Find the palette index closest to Duke3D's brightest blue
    // "foreground" color.  (Index 79, or the last column of the 5th row,
    // if the palette is laid out in a 16x16 pattern.)
    for (int i=0, k=UINT8_MAX+1; i<256; i++)
    {
        int const j =
            klabs(curpalette[i].r - 4*47) +
            klabs(curpalette[i].g - 4*55) +
            klabs(curpalette[i].b - 4*63);
        if (j < k) { k = j; whiteColorIdx = i; }
    }
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
    UNREFERENCED_PARAMETER(cols);
    UNREFERENCED_PARAMETER(rows);
}

static int32_t _internal_gettime(void)
{
    return 0;
}

static void _internal_onshowosd(int a)
{
    UNREFERENCED_PARAMETER(a);
}

////////////////////////////

static int osdfunc_alias(osdcmdptr_t parm)
{
    if (parm->numparms < 1)
    {
        int cnt = 0;

        OSD_Printf("Alias listing:\n");

        for (auto &symb : osd->symbptrs)
        {
            if (symb == NULL)
                break;
            else if (symb->func == OSD_ALIAS)
            {
                cnt++;
                OSD_Printf("     %s \"%s\"\n", symb->name, symb->help);
            }
        }

        if (cnt == 0)
            OSD_Printf("No aliases found.\n");

        return OSDCMD_OK;
    }

    for (auto &symb : osd->symbptrs)
    {
        if (symb == NULL)
            break;
        else if (!Bstrcasecmp(parm->parms[0], symb->name))
        {
            if (parm->numparms < 2)
            {
                if (symb->func == OSD_ALIAS)
                    OSD_Printf("alias %s \"%s\"\n", symb->name, symb->help);
                else
                    OSD_Printf("%s is not an alias\n", symb->name);

                return OSDCMD_OK;
            }
            else if (symb->func != OSD_ALIAS && symb->func != OSD_UNALIASED)
            {
                OSD_Printf("Cannot override a function or cvar with an alias\n");

                return OSDCMD_OK;
            }
        }
    }

    OSD_RegisterFunction(Xstrdup(parm->parms[0]), Xstrdup(parm->parms[1]), OSD_ALIAS);

    if (!osd->execdepth)
        OSD_Printf("%s\n", parm->raw);

    return OSDCMD_OK;
}

static int osdfunc_unalias(osdcmdptr_t parm)
{
    if (parm->numparms < 1)
        return OSDCMD_SHOWHELP;

    for (auto symb=osd->symbols; symb!=NULL; symb=symb->next)
    {
        if (!Bstrcasecmp(parm->parms[0], symb->name))
        {
            if (parm->numparms < 2)
            {
                if (symb->func == OSD_ALIAS)
                {
                    OSD_Printf("Removed alias %s (\"%s\")\n", symb->name, symb->help);
                    symb->func = OSD_UNALIASED;
                }
                else
                    OSD_Printf("Invalid alias %s\n", symb->name);

                return OSDCMD_OK;
            }
        }
    }

    OSD_Printf("Invalid alias %s\n", parm->parms[0]);
    return OSDCMD_OK;
}

static int osdfunc_listsymbols(osdcmdptr_t parm)
{
    if (parm->numparms > 1)
        return OSDCMD_SHOWHELP;

    int maxwidth = 0;

    for (auto symb=osd->symbols; symb!=NULL; symb=symb->next)
        if (symb->func != OSD_UNALIASED && symb->help != NULL)
            maxwidth = max<int>(maxwidth, Bstrlen(symb->name));

    if (maxwidth > 0)
    {
        int width = 0;
        int count = 0;

        maxwidth += 3;

        if (parm->numparms > 0)
            OSD_Printf("%sSymbol listing for %s:\n", osd->draw.highlight, parm->parms[0]);
        else
            OSD_Printf("%sSymbol listing:\n", osd->draw.highlight);

        int const parmlen = parm->numparms ? Bstrlen(parm->parms[0]) : 0;

        for (auto symb=osd->symbols; symb!=NULL; symb=symb->next)
        {
            if (symb->func == OSD_UNALIASED || symb->help == NULL || (parm->numparms == 1 && Bstrncmp(parm->parms[0], symb->name, parmlen)))
                continue;

            int const var = hash_find(&h_cvars, symb->name);

            if ((unsigned)var < OSDMAXSYMBOLS && OSD_CvarModified(&osd->cvars[var]))
            {
                OSD_Printf("%s*", osd->draw.highlight);
                OSD_Printf("%-*s", maxwidth-1, symb->name);
            }
            else
                OSD_Printf("%-*s", maxwidth, symb->name);

            width += maxwidth;
            count++;

            if (width > osd->draw.cols - maxwidth)
            {
                width = 0;
                OSD_Printf("\n");
            }
        }

        if (width)
            OSD_Printf("\n");

        OSD_Printf("%sFound %d symbols\n", osd->draw.highlight, count);
    }
    return OSDCMD_OK;
}

static int osdfunc_help(osdcmdptr_t parm)
{
    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    auto symb = osd_findexactsymbol(parm->parms[0]);

    if (!symb)
        OSD_Printf("Error: no help for undefined symbol \"%s\"\n", parm->parms[0]);
    else
        OSD_Printf("%s\n", symb->help);

    return OSDCMD_OK;
}

static int osdfunc_clear(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    auto &t = osd->text;

    Bmemset(t.buf, 0, OSDBUFFERSIZE);
    Bmemset(t.fmt, osd->draw.textpal + (osd->draw.textshade<<5), OSDBUFFERSIZE);
    t.lines = 1;

    return OSDCMD_OK;
}

static int osdfunc_history(osdcmdptr_t UNUSED(parm))
{
    UNREFERENCED_CONST_PARAMETER(parm);

    OSD_Printf("%sCommand history:\n", osd->draw.highlight);

    auto &h = osd->history;

    for (int i=osd->history.maxlines-1, j=0; i>=0; i--)
    {
        if (h.buf[i])
            OSD_Printf("%4d \"%s\"\n", h.total - h.lines + (++j), h.buf[i]);
    }

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

    for (auto &symb : osd->symbptrs)
        DO_FREE_AND_NULL(symb);

    osd->symbols = NULL;

    for (auto & i : osd->history.buf)
        DO_FREE_AND_NULL(i);

    DO_FREE_AND_NULL(osd->cvars);

    DO_FREE_AND_NULL(osd->editor.buf);
    DO_FREE_AND_NULL(osd->editor.tmp);

    DO_FREE_AND_NULL(osd->text.buf);
    DO_FREE_AND_NULL(osd->text.fmt);

    DO_FREE_AND_NULL(osd->version.buf);

    MAYBE_FCLOSE_AND_NULL(osdlog);
    DO_FREE_AND_NULL(osd);
}


static int osdcmd_cvar_set_osd(osdcmdptr_t parm)
{
    int const r = osdcmd_cvar_set(parm);

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
        for (auto &i : osd->history.buf)
            DO_FREE_AND_NULL(i);
    }

    return OSDCMD_OK;
}

static int osdfunc_toggle(osdcmdptr_t parm)
{
    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    int i = hash_find(&h_cvars, parm->parms[0]);

    if (i == -1)
    {
        for (i = osd->numcvars-1; i>=0; i--)
        {
            if (!Bstrcasecmp(parm->parms[0], osd->cvars[i].pData->name))
                break;
        }
    }

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
    osd = (osdmain_t *)Xcalloc(1, sizeof(osdmain_t));

    mutex_init(&osd->mutex);

    if (!osd->keycode)
        osd->keycode = sc_Tilde;

    osd->text.buf   = (char *)Xmalloc(OSDBUFFERSIZE);
    osd->text.fmt   = (char *)Xmalloc(OSDBUFFERSIZE);
    osd->editor.buf = (char *)Xmalloc(OSDEDITLENGTH);
    osd->editor.tmp = (char *)Xmalloc(OSDEDITLENGTH);

    Bmemset(osd->text.buf, asc_Space, OSDBUFFERSIZE);
    Bmemset(osd->text.fmt, osd->draw.textpal + (osd->draw.textshade<<5), OSDBUFFERSIZE);
    Bmemset(osd->symbptrs, 0, sizeof(osd->symbptrs));

    osd->numsymbols    = 0;
    osd->numcvars      = 0;
    osd->text.lines    = 1;
    osd->text.maxlines = OSDDEFAULTMAXLINES;  // overwritten later
    osd->draw.rows     = OSDDEFAULTROWS;
    osd->draw.cols     = OSDDEFAULTCOLS;
    osd->log.cutoff    = OSDLOGCUTOFF;

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

    for (auto & i : cvars_osd)
        OSD_RegisterCvar(&i, (i.flags & CVAR_FUNCPTR) ? osdcmd_cvar_set_osd : osdcmd_cvar_set);

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

    osdlog = buildvfs_fopen_write_text(fn);

    if (osdlog)
    {
#ifndef USE_PHYSFS
#ifdef DEBUGGINGAIDS
        const int bufmode = _IONBF;
#else
        const int bufmode = _IOLBF;
#endif
        setvbuf(osdlog, (char *)NULL, bufmode, BUFSIZ);
#endif
        osdlogfn = fn;
    }
}


//
// OSD_SetFunctions() -- Sets some callbacks which the OSD uses to understand its world
//
void OSD_SetFunctions(void (*drawchar)(int, int, char, int, int),
                      void (*drawstr)(int, int, const char *, int, int, int),
                      void (*drawcursor)(int, int, int, int),
                      int (*colwidth)(int),
                      int (*rowheight)(int),
                      void (*clearbg)(int, int),
                      int32_t (*gtime)(void),
                      void (*showosd)(int))
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
void OSD_SetParameters(int promptShade, int promptPal, int editShade, int editPal, int textShade, int textPal,
                       char const *errorStr, char const *highlight, uint32_t flags)
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
static int OSD_FindDiffPoint(const char *str1, const char *str2)
{
    int i;

    for (i = 0; Btolower(str1[i]) == Btolower(str2[i]); i++)
        if (str1[i] == 0 || str2[i] == 0)
            break;

    return i;
}

static void OSD_AdjustEditorPosition(osdedit_t &e)
{
    e.pos = 0;
    while (e.buf[e.pos])
        e.pos++;
    e.len = e.pos;

    if (e.pos < e.start)
    {
        e.end   = e.pos;
        e.start = e.end - OSD_EDIT_LINE_WIDTH;

        if (e.start < 0)
        {
            e.end -= e.start;
            e.start = 0;
        }
    }
    else if (e.pos >= e.end)
    {
        e.start += (e.pos - e.end);
        e.end += (e.pos - e.end);
    }
}

static void OSD_HistoryPrev(void)
{
    osdhist_t &h = osd->history;

    if (h.pos >= h.lines - 1)
        return;

    osdedit_t &e = osd->editor;

    Bmemcpy(e.buf, h.buf[++h.pos], OSDEDITLENGTH);

    OSD_AdjustEditorPosition(e);
}

static void OSD_HistoryNext(void)
{
    osdhist_t &h = osd->history;

    if (h.pos < 0)
        return;

    osdedit_t &e = osd->editor;

    if (h.pos == 0)
    {
        e.len   = 0;
        e.pos   = 0;
        e.start = 0;
        e.end   = OSD_EDIT_LINE_WIDTH;
        h.pos  = -1;

        return;
    }

    Bmemcpy(e.buf, h.buf[--h.pos], OSDEDITLENGTH);

    OSD_AdjustEditorPosition(e);
}

//
// OSD_HandleKey() -- Handles keyboard input when capturing input.
//  Returns 0 if the key was handled internally, or the scancode if it should
//  be passed on to the game.
//

int OSD_HandleChar(char ch)
{
    if (!osd || (osd->flags & OSD_CAPTURE) != OSD_CAPTURE)
        return ch;

    osdhist_t &h  = osd->history;
    osdedit_t &ed = osd->editor;

    osdsymbol_t *       tabc      = NULL;
    static osdsymbol_t *lastmatch = NULL;

    if (ch != asc_Tab)
        lastmatch = NULL;

    switch (ch)
    {
        case asc_Ctrl_A:  // jump to beginning of line
            ed.pos   = 0;
            ed.start = 0;
            ed.end   = OSD_EDIT_LINE_WIDTH;
            return 0;

        case asc_Ctrl_B:  // move one character left
            if (ed.pos > 0)
                --ed.pos;
            return 0;

        case asc_Ctrl_C:  // discard line
            ed.buf[ed.len] = 0;
            OSD_Printf("%s\n", ed.buf);

            ed.len    = 0;
            ed.pos    = 0;
            ed.start  = 0;
            ed.end    = OSD_EDIT_LINE_WIDTH;
            ed.buf[0] = 0;
            return 0;

        case asc_Ctrl_E:  // jump to end of line
            ed.pos   = ed.len;
            ed.end   = ed.pos;
            ed.start = ed.end - OSD_EDIT_LINE_WIDTH;

            if (ed.start < 0)
            {
                ed.start = 0;
                ed.end   = OSD_EDIT_LINE_WIDTH;
            }
            return 0;

        case asc_Ctrl_F:  // move one character right
            if (ed.pos < ed.len)
                ed.pos++;
            return 0;

        case asc_BackSpace:
#ifdef __APPLE__
        case 127:  // control h, backspace
#endif
            if (!ed.pos || !ed.len)
                return 0;

            if ((osd->flags & OSD_OVERTYPE) == 0)
            {
                if (ed.pos < ed.len)
                    Bmemmove(ed.buf + ed.pos - 1, ed.buf + ed.pos, ed.len - ed.pos);
                ed.len--;
            }

            if (--ed.pos < ed.start)
                ed.start--, ed.end--;
#ifndef __APPLE__
        fallthrough__;
        case 127:  // handled in OSD_HandleScanCode (delete)
#endif
            return 0;

        case asc_Tab:  // tab
        {
            int commonsize = INT_MAX;

            if (!lastmatch)
            {
                int editPos, iter;

                for (editPos = ed.pos; editPos > 0; editPos--)
                    if (ed.buf[editPos - 1] == ' ')
                        break;

                for (iter = 0; editPos < ed.len && ed.buf[editPos] != ' '; iter++, editPos++)
                    ed.tmp[iter] = ed.buf[editPos];

                ed.tmp[iter] = 0;

                if (iter > 0)
                {
                    tabc = osd_findsymbol(ed.tmp, NULL);

                    if (tabc && tabc->next && osd_findsymbol(ed.tmp, tabc->next))
                    {
                        auto symb     = tabc;
                        int  maxwidth = 0, x = 0, num = 0, diffpt;

                        while (symb && symb != lastmatch)
                        {
                            num++;

                            if (lastmatch)
                            {
                                diffpt = OSD_FindDiffPoint(symb->name, lastmatch->name);

                                if (diffpt < commonsize)
                                    commonsize = diffpt;
                            }

                            maxwidth  = max<int>(maxwidth, Bstrlen(symb->name));
                            lastmatch = symb;

                            if (!lastmatch->next)
                                break;

                            symb = osd_findsymbol(ed.tmp, lastmatch->next);
                        }

                        OSD_Printf("%sFound %d possible completions for \"%s\":\n", osd->draw.highlight, num, ed.tmp);
                        maxwidth += 3;
                        symb = tabc;
                        OSD_Printf("  ");

                        while (symb && (symb != lastmatch))
                        {
                            tabc = lastmatch = symb;
                            OSD_Printf("%-*s", maxwidth, symb->name);

                            if (!lastmatch->next)
                                break;

                            symb = osd_findsymbol(ed.tmp, lastmatch->next);
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
                tabc = osd_findsymbol(ed.tmp, lastmatch->next);

                if (!tabc)
                    tabc = osd_findsymbol(ed.tmp, NULL);  // wrap */
            }

            if (tabc)
            {
                int editPos;

                for (editPos = ed.pos; editPos > 0; editPos--)
                    if (ed.buf[editPos - 1] == ' ')
                        break;

                ed.len = editPos;

                for (int iter = 0; tabc->name[iter] && ed.len <= OSDEDITLENGTH - 1 && (ed.len < commonsize); editPos++, iter++, ed.len++)
                    ed.buf[editPos] = tabc->name[iter];

                ed.pos   = ed.len;
                ed.end   = ed.pos;
                ed.start = ed.end - OSD_EDIT_LINE_WIDTH;

                if (ed.start < 0)
                {
                    ed.start = 0;
                    ed.end   = OSD_EDIT_LINE_WIDTH;
                }

                lastmatch = tabc;
            }
            return 0;
        }

        case asc_Ctrl_K:  // delete all to end of line
            Bmemset(ed.buf + ed.pos, 0, OSDEDITLENGTH - ed.pos);
            ed.len = ed.pos;
            return 0;

        case asc_Ctrl_L:  // clear screen
            Bmemset(osd->text.buf, 0, OSDBUFFERSIZE);
            Bmemset(osd->text.fmt, osd->draw.textpal + (osd->draw.textshade << 5), OSDBUFFERSIZE);
            osd->text.lines = 1;
            return 0;

        case asc_Enter:  // control m, enter
            if (ed.len > 0)
            {
                ed.buf[ed.len] = 0;
                if (!h.buf[0] || Bstrcmp(h.buf[0], ed.buf))
                {
                    DO_FREE_AND_NULL(h.buf[h.maxlines - 1]);
                    Bmemmove(&h.buf[1], &h.buf[0], sizeof(intptr_t) * h.maxlines - 1);
                    OSD_SetHistory(0, ed.buf);

                    if (h.lines < h.maxlines)
                        h.lines++;

                    h.total++;
                }

                if (h.exec++ == h.maxlines)
                    OSD_Printf("Buffer full! Consider increasing \"osdhistorydepth\" beyond %d.\n", --h.exec);

                h.pos = -1;
            }

            ed.len   = 0;
            ed.pos   = 0;
            ed.start = 0;
            ed.end   = OSD_EDIT_LINE_WIDTH;
            return 0;

        case asc_Ctrl_N:  // next (ie. down arrow)
            OSD_HistoryNext();
            return 0;

        case asc_Ctrl_P:  // previous (ie. up arrow)
            OSD_HistoryPrev();
            return 0;

        case asc_Ctrl_U:  // delete all to beginning
            if (ed.pos > 0 && ed.len)
            {
                if (ed.pos < ed.len)
                    Bmemmove(ed.buf, ed.buf + ed.pos, ed.len - ed.pos);

                ed.len -= ed.pos;
                ed.pos   = 0;
                ed.start = 0;
                ed.end   = OSD_EDIT_LINE_WIDTH;
            }
            return 0;

        case asc_Ctrl_W:  // delete one word back
            if (ed.pos > 0 && ed.len > 0)
            {
                int editPos = ed.pos;

                while (editPos > 0 && ed.buf[editPos - 1] == asc_Space) editPos--;
                while (editPos > 0 && ed.buf[editPos - 1] != asc_Space) editPos--;

                if (ed.pos < ed.len)
                    Bmemmove(ed.buf + editPos, ed.buf + ed.pos, ed.len - ed.pos);

                ed.len -= (ed.pos - editPos);
                ed.pos = editPos;

                if (ed.pos < ed.start)
                {
                    ed.start = ed.pos;
                    ed.end   = ed.start + OSD_EDIT_LINE_WIDTH;
                }
            }
            return 0;

        default:
            if (ch >= asc_Space)  // text char
            {
                if ((osd->flags & OSD_OVERTYPE) == 0)
                {
                    if (ed.len == OSDEDITLENGTH)  // buffer full, can't insert another char
                        return 0;

                    if (ed.pos < ed.len)
                        Bmemmove(ed.buf + ed.pos + 1, ed.buf + ed.pos, ed.len - ed.pos);

                    ed.len++;
                }
                else if (ed.pos == ed.len)
                    ed.len++;

                ed.buf[ed.pos++] = ch;

                if (ed.pos > ed.end)
                    ed.start++, ed.end++;
            }
            return 0;
    }
    return 0;
}

int OSD_HandleScanCode(uint8_t scanCode, int keyDown)
{
    if (!osd)
        return 1;

    osddraw_t &draw = osd->draw;

    if (scanCode == osd->keycode && (keystatus[sc_LeftShift] || (osd->flags & OSD_CAPTURE) || (osd->flags & OSD_PROTECTED) != OSD_PROTECTED))
    {
        if (keyDown)
        {
            draw.scrolling = (osdrowscur == -1) ? 1 : (osdrowscur == draw.rows) ? -1 : -draw.scrolling;
            osdrowscur += draw.scrolling;
            OSD_CaptureInput(draw.scrolling == 1);
            osdscrtime = timerGetTicks();
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

    osdedit_t &ed = osd->editor;
    osdkeytime    = gettime();

    switch (scanCode)
    {
    case sc_Escape:
        //        OSD_ShowDisplay(0);
        draw.scrolling = -1;
        osdrowscur--;
        OSD_CaptureInput(0);
        osdscrtime = timerGetTicks();
        break;

    case sc_PgUp:
        if (draw.head < osd->text.lines-1)
            ++draw.head;
        break;

    case sc_PgDn:
        if (draw.head > 0)
            --draw.head;
        break;

    case sc_Home:
        if (osd->flags & OSD_CTRL)
            draw.head = osd->text.lines-1;
        else
        {
            ed.pos   = 0;
            ed.start = ed.pos;
            ed.end   = ed.start + OSD_EDIT_LINE_WIDTH;
        }
        break;

    case sc_End:
        if (osd->flags & OSD_CTRL)
            draw.head = 0;
        else
        {
            ed.pos   = ed.len;
            ed.end   = ed.pos;
            ed.start = ed.end - OSD_EDIT_LINE_WIDTH;

            if (ed.start < 0)
            {
                ed.start = 0;
                ed.end   = OSD_EDIT_LINE_WIDTH;
            }
        }
        break;

    case sc_Insert:
        osd->flags = (osd->flags & ~OSD_OVERTYPE) | (-((osd->flags & OSD_OVERTYPE) == 0) & OSD_OVERTYPE);
        break;

    case sc_LeftArrow:
        if (ed.pos > 0)
        {
            if (osd->flags & OSD_CTRL)
            {
                while (ed.pos > 0)
                {
                    if (ed.buf[ed.pos-1] != asc_Space)
                        break;

                    --ed.pos;
                }

                while (ed.pos > 0)
                {
                    if (ed.buf[ed.pos-1] == asc_Space)
                        break;

                    --ed.pos;
                }
            }
            else --ed.pos;
        }

        if (ed.pos < ed.start)
        {
            ed.end -= (ed.start - ed.pos);
            ed.start -= (ed.start - ed.pos);
        }
        break;

    case sc_RightArrow:
        if (ed.pos < ed.len)
        {
            if (osd->flags & OSD_CTRL)
            {
                while (ed.pos < ed.len)
                {
                    if (ed.buf[ed.pos] == asc_Space)
                        break;

                    ed.pos++;
                }

                while (ed.pos < ed.len)
                {
                    if (ed.buf[ed.pos] != asc_Space)
                        break;

                    ed.pos++;
                }
            }
            else ed.pos++;
        }

        if (ed.pos >= ed.end)
        {
            ed.start += (ed.pos - ed.end);
            ed.end += (ed.pos - ed.end);
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
        if (ed.pos == ed.len || !ed.len)
            return 0;

        if (ed.pos <= ed.len-1)
            Bmemmove(ed.buf+ed.pos, ed.buf+ed.pos+1, ed.len-ed.pos-1);

        ed.len--;
        break;
    }

    return 0;
}


//
// OSD_ResizeDisplay() -- Handles readjustment of the display when the screen resolution
//  changes on us.
//
void OSD_ResizeDisplay(int w, int h)
{
    auto &t = osd->text;
    auto &d = osd->draw;

    int const newcols     = getcolumnwidth(w);
    int const newmaxlines = OSDBUFFERSIZE / newcols;

    auto newtext = (char *)Xmalloc(OSDBUFFERSIZE);
    auto newfmt  = (char *)Xmalloc(OSDBUFFERSIZE);

    Bmemset(newtext, asc_Space, OSDBUFFERSIZE);

    int const copylines = min(newmaxlines, t.maxlines);
    int const copycols  = min(newcols, d.cols);

    for (int i = 0; i < copylines; ++i)
    {
        Bmemcpy(newtext + newcols * i, t.buf + d.cols * i, copycols);
        Bmemcpy(newfmt  + newcols * i, t.fmt + d.cols * i, copycols);
    }

    Xfree(t.buf);
    t.buf = newtext;

    Xfree(t.fmt);
    t.fmt = newfmt;

    t.maxlines = newmaxlines;
    osdmaxrows = getrowheight(h) - 2;
    d.cols     = newcols;

    if (d.rows > osdmaxrows)
        d.rows = osdmaxrows;

    t.pos  = 0;
    d.head = 0;

    osd->editor.start = 0;
    osd->editor.end   = OSD_EDIT_LINE_WIDTH;

    whiteColorIdx = -1;
}


//
// OSD_CaptureInput()
//
void OSD_CaptureInput(int cap)
{
    osd->flags = (osd->flags & ~(OSD_CAPTURE|OSD_CTRL|OSD_SHIFT)) | (-cap & OSD_CAPTURE);

    mouseGrabInput(cap == 0 ? g_mouseLockedToWindow : 0);
    onshowosd(cap);

    keyFlushChars();
}


//
// OSD_ShowDisplay() -- Shows or hides the onscreen display
//
void OSD_ShowDisplay(int onf)
{
    osd->flags = (osd->flags & ~OSD_DRAW) | (-onf & OSD_DRAW);
    OSD_CaptureInput(onf);
}


//
// OSD_Draw() -- Draw the onscreen display
//

void OSD_Draw(void)
{
    if (!osd)
        return;

    if (osdrowscur == 0)
        OSD_ShowDisplay((osd->flags & OSD_DRAW) != OSD_DRAW);

    if (osdrowscur == osd->draw.rows)
        osd->draw.scrolling = 0;
    else
    {
        if ((osdrowscur < osd->draw.rows && osd->draw.scrolling == 1) || osdrowscur < -1)
        {
            uint32_t j = (timerGetTicks() - osdscrtime);

            while (j < UINT32_MAX)
            {
                j -= tabledivide32_noinline(200, osd->draw.rows);
                if (++osdrowscur > osd->draw.rows-1)
                    break;
            }
        }
        else if ((osdrowscur > -1 && osd->draw.scrolling == -1) || osdrowscur > osd->draw.rows)
        {
            uint32_t j = (timerGetTicks() - osdscrtime);

            while (j < UINT32_MAX)
            {
                j -= tabledivide32_noinline(200, osd->draw.rows);
                if (--osdrowscur < 1)
                    break;
            }
        }

        osdscrtime = timerGetTicks();
    }

    if ((osd->flags & OSD_DRAW) == 0 || !osdrowscur) return;

    int topoffs = osd->draw.head * osd->draw.cols;
    int row     = osdrowscur - 1;
    int lines   = min(osd->text.lines - osd->draw.head, osdrowscur);

    videoBeginDrawing();

    clearbackground(osd->draw.cols,osdrowscur+1);

    for (; lines>0; lines--, row--)
    {
        // XXX: May happen, which would ensue an oob if not checked.
        // Last char accessed is osd->text.buf[topoffs + osd->draw.cols-1].
        // Reproducible by running test.lua with -Lopts=diag
        // and scrolling to the top.
        if (topoffs + osd->draw.cols-1 >= OSDBUFFERSIZE)
            break;

        drawosdstr(0, row, osd->text.buf + topoffs, osd->draw.cols, osd->draw.textshade, osd->draw.textpal);
        topoffs += osd->draw.cols;
    }

    int       offset = ((osd->flags & (OSD_CAPS | OSD_SHIFT)) == (OSD_CAPS | OSD_SHIFT) && osd->draw.head > 0);
    int const shade  = osd->draw.promptshade ? osd->draw.promptshade : (sintable[((int32_t) totalclock<<4)&2047]>>11);

    if (osd->draw.head == osd->text.lines-1)
        drawosdchar(0, osdrowscur, '~', shade, osd->draw.promptpal);
    else if (osd->draw.head > 0)
        drawosdchar(0, osdrowscur, '^', shade, osd->draw.promptpal);

    if (osd->flags & OSD_CAPS)
        drawosdchar((osd->draw.head > 0), osdrowscur, 'C', shade, osd->draw.promptpal);

    if (osd->flags & OSD_SHIFT)
        drawosdchar(1 + (osd->flags & OSD_CAPS && osd->draw.head > 0), osdrowscur, 'H', shade, osd->draw.promptpal);

    drawosdchar(2 + offset, osdrowscur, '>', shade, osd->draw.promptpal);

    int const len = min(osd->draw.cols-1-3 - offset, osd->editor.len - osd->editor.start);

    for (int x=len-1; x>=0; x--)
        drawosdchar(3 + x + offset, osdrowscur, osd->editor.buf[osd->editor.start+x], osd->draw.editshade<<1, osd->draw.editpal);

    offset += 3 + osd->editor.pos - osd->editor.start;

    drawosdcursor(offset,osdrowscur,osd->flags & OSD_OVERTYPE,osdkeytime);

    if (osd->version.buf)
        drawosdstr(osd->draw.cols - osd->version.len, osdrowscur - (offset >= osd->draw.cols - osd->version.len),
                    osd->version.buf, osd->version.len, (sintable[((int32_t) totalclock<<4)&2047]>>11), osd->version.pal);

    videoEndDrawing();
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
    auto &t = osd->text;
    auto &d = osd->draw;

    Bmemmove(t.buf + d.cols, t.buf, OSDBUFFERSIZE - d.cols);
    Bmemset(t.buf, asc_Space, d.cols);

    Bmemmove(t.fmt + d.cols, t.fmt, OSDBUFFERSIZE - d.cols);
    Bmemset(t.fmt, d.textpal, d.cols);

    if (t.lines < t.maxlines)
        t.lines++;
}

#define MAX_ERRORS 4096

void OSD_Puts(const char *tmpstr)
{
    if (tmpstr[0] == 0)
        return;

    if (!osd)
        OSD_Init();

    const char *chp;

    int textPal   = osd->draw.textpal;
    int textShade = osd->draw.textshade;

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
        char *chp2 = Xstrdup(tmpstr);
        buildvfs_fputs(OSD_StripColors(chp2, tmpstr), osdlog);
        Bprintf("%s", chp2);
        Xfree(chp2);
    }
    else if (log.lines == log.cutoff)
    {
        buildvfs_fputs("\nLog file full! Consider increasing \"osdlogcutoff\".\n", osdlog);
        log.lines = log.cutoff + 1;
    }

    chp = tmpstr;
    do
    {
        if (*chp == '\n')
        {
            osd->text.pos = 0;
            ++log.lines;
            OSD_LineFeed();
            continue;
        }

        if (*chp == '\r')
        {
            osd->text.pos = 0;
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
                textPal   = osd->draw.textpal;
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
    if (!osd->history.exec)
        return;

    int cmd = osd->history.exec - 1;

    osd->history.exec = 0;

    for (; cmd >= 0; cmd--)
        OSD_Dispatch((const char *)osd->history.buf[cmd]);
}

//
// OSD_Dispatch() -- Executes a command string
//
static char *osd_strtoken(char *s, char **ptrptr, int *restart)
{
    *restart = 0;
    if (!ptrptr) return NULL;

    // if s != NULL, we process from the start of s, otherwise
    // we just continue with where ptrptr points to

    char *p = s ? s : *ptrptr;

    if (!p) return NULL;

    // eat up any leading whitespace
    while (*p == ' ') p++;

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

    char *start;

    if (*p == '\"')
    {
        // quoted string
        start = ++p;
        char *p2 = p;
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
void OSD_Dispatch(const char *cmd)
{
    char *workbuf = Xstrdup(cmd);
    char *state   = workbuf;
    char *wtp;

    int restart = 0;

    do
    {
        char const *token;

        if ((token = osd_strtoken(state, &wtp, &restart)) == NULL)
        {
            state = wtp;
            continue;
        }

        // cheap hack for comments in cfgs
        if (token[0] == '/' && token[1] == '/')
        {
            Xfree(workbuf);
            return;
        }

        auto const *symbol = osd_findexactsymbol(token);

        if (symbol == NULL)
        {
            static char const s_gamefunc_[]    = "gamefunc_";
            size_t constexpr  strlen_gamefunc_ = ARRAY_SIZE(s_gamefunc_) - 1;
            size_t const      strlen_token     = Bstrlen(token);

            if ((strlen_gamefunc_ >= strlen_token || Bstrncmp(token, s_gamefunc_, strlen_gamefunc_)) && !m32_osd_tryscript)
                OSD_Printf("%s\"%s\" is not a valid command or cvar\n", osd->draw.highlight, token);
            else if (m32_osd_tryscript)
                M32RunScript(cmd);

            Xfree(workbuf);
            return;
        }

        auto name = token;
        char const *parms[MAXPARMS] = {};
        int numparms = 0;

        while (wtp && !restart)
        {
            token = osd_strtoken(NULL, &wtp, &restart);
            if (token && numparms < MAXPARMS) parms[numparms++] = token;
        }

        osdfuncparm_t const ofp = { numparms, name, parms, cmd };

        if (symbol->func == OSD_ALIAS)
            OSD_Dispatch(symbol->help);
        else if (symbol->func != OSD_UNALIASED)
        {
            switch (symbol->func(&ofp))
            {
                default:
                case OSDCMD_OK: break;
                case OSDCMD_SHOWHELP: OSD_Printf("%s\n", symbol->help); break;
            }
        }

        state = wtp;
    }
    while (wtp && restart);

    Xfree(workbuf);
}


//
// OSD_RegisterFunction() -- Registers a new function
//
int OSD_RegisterFunction(const char *pszName, const char *pszDesc, int (*func)(osdcmdptr_t))
{
    if (!osd)
        OSD_Init();

    auto symb = osd_findexactsymbol(pszName);

    if (!symb) // allow reusing an alias name
    {
        symb = osd_addsymbol(pszName);
        symb->name = pszName;
    }

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
    osd->version.buf   = Xstrdup(pszVersion);
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

    auto const newsymb = (osdsymbol_t *)Xcalloc(1, sizeof(osdsymbol_t));

    if (!osd->symbols)
        osd->symbols = newsymb;
    else
    {
        if (!Bstrcasecmp(pszName, osd->symbols->name))
        {
            auto t = osd->symbols;
            osd->symbols = newsymb;
            osd->symbols->next = t;
        }
        else
        {
            auto s = osd->symbols;

            while (s->next)
            {
                if (Bstrcasecmp(s->next->name, pszName))
                    break;

                s = s->next;
            }

            auto t = s->next;

            s->next = newsymb;
            newsymb->next = t;
        }
    }

    char * const lowercase = Bstrtolower(Xstrdup(pszName));

    hash_add(&h_osd, pszName, osd->numsymbols, 1);
    hash_add(&h_osd, lowercase, osd->numsymbols, 1);
    Xfree(lowercase);

    osd->symbptrs[osd->numsymbols++] = newsymb;

    return newsymb;
}


//
// findsymbol() -- Finds a symbol, possibly partially named
//
static osdsymbol_t * osd_findsymbol(const char * const pszName, osdsymbol_t *pSymbol)
{
    if (osd->symbols == NULL)
        return NULL;

    if (!pSymbol) pSymbol = osd->symbols;

    int const nameLen = Bstrlen(pszName);

    for (; pSymbol; pSymbol=pSymbol->next)
    {
        if (pSymbol->func != OSD_UNALIASED && pSymbol->help != NULL && !Bstrncasecmp(pszName, pSymbol->name, nameLen))
            return pSymbol;
    }

    return NULL;
}

//
// findexactsymbol() -- Finds a symbol, complete named
//
static osdsymbol_t * osd_findexactsymbol(const char *pszName)
{
    if (osd->symbols == NULL)
        return NULL;

    int symbolNum = hash_find(&h_osd, pszName);

    if (symbolNum < 0)
    {
        char *const lname = Xstrdup(pszName);
        Bstrtolower(lname);
        symbolNum = hash_find(&h_osd, lname);
        Xfree(lname);
    }

    return (symbolNum >= 0) ? osd->symbptrs[symbolNum] : NULL;
}

int osdcmd_cvar_set(osdcmdptr_t parm)
{
    int const printValue = (parm->numparms == 0);
    int const cvaridx    = hash_findcase(&h_cvars, parm->name);

#if 0
    if (i < 0)
        for (i = osd->numcvars-1; i >= 0; i--)
            if (!Bstrcasecmp(parm->name, pData.name)) break;
#endif

    Bassert(cvaridx >= 0);

    auto pData = *osd->cvars[cvaridx].pData;

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
        switch (pData.flags & (CVAR_RESTARTVID|CVAR_INVALIDATEALL|CVAR_INVALIDATEART))
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
            if (videoGetRenderMode() == REND_POLYMER)
                polymer_texinvalidate();
#endif
            break;
        }
    }
#endif

    if (!OSD_ParsingScript())
        OSD_Printf("\n");

    return OSDCMD_OK;
}

void OSD_WriteAliases(buildvfs_FILE fp)
{
    for (auto &symb : osd->symbptrs)
    {
        if (symb == NULL)
            break;
        else if (symb->func == (void *)OSD_ALIAS)
        {
            buildvfs_fputstr(fp, "alias \"");
            buildvfs_fputstrptr(fp, symb->name);
            buildvfs_fputstr(fp, "\" \"");
            buildvfs_fputstrptr(fp, symb->help);
            buildvfs_fputstr(fp, "\"\n");
        }
    }
}

void OSD_WriteCvars(buildvfs_FILE fp)
{
    Bassert(fp);

    char buf[64];

    for (int i = 0; i < osd->numcvars; i++)
    {
        osdcvardata_t const &pData = *osd->cvars[i].pData;

        if (!(pData.flags & CVAR_NOSAVE) && OSD_CvarModified(&osd->cvars[i]))
        {
            switch (pData.flags & CVAR_TYPEMASK)
            {
            case CVAR_FLOAT:
                buildvfs_fputstrptr(fp, pData.name);
                snprintf(buf, sizeof(buf), " \"%f\"\n", *pData.f);
                buildvfs_fputstrptr(fp, buf);
                break;
            case CVAR_DOUBLE:
                buildvfs_fputstrptr(fp, pData.name);
                snprintf(buf, sizeof(buf), " \"%f\"\n", *pData.d);
                buildvfs_fputstrptr(fp, buf);
                break;
            case CVAR_INT:
            case CVAR_BOOL:
                buildvfs_fputstrptr(fp, pData.name);
                snprintf(buf, sizeof(buf), " \"%d\"\n", *pData.i32);
                buildvfs_fputstrptr(fp, buf);
                break;
            case CVAR_UINT:
                buildvfs_fputstrptr(fp, pData.name);
                snprintf(buf, sizeof(buf), " \"%u\"\n", *pData.u32);
                buildvfs_fputstrptr(fp, buf);
                break;
            case CVAR_STRING:
                buildvfs_fputstrptr(fp, pData.name);
                buildvfs_fputstr(fp, " \"");
                buildvfs_fputstrptr(fp, pData.string);
                buildvfs_fputstr(fp, "\"\n");
                break;
            default: EDUKE32_UNREACHABLE_SECTION(break);
            }
        }
    }
}
