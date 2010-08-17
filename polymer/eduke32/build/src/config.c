// Evil and Nasty Configuration File Reader for KenBuild
// by Jonathon Fowler

#include "compat.h"
#include "build.h"
#include "editor.h"
#include "osd.h"

#ifdef RENDERTYPEWIN
#include "winlayer.h"
#endif
#include "baselayer.h"

static int32_t vesares[13][2] = {{320,200},{360,200},{320,240},{360,240},{320,400},
    {360,400},{640,350},{640,400},{640,480},{800,600},
    {1024,768},{1280,1024},{1600,1200}
};

static int32_t readconfig(BFILE *fp, const char *key, char *value, uint32_t len)
{
    char buf[1000], *k, *v, *eq;
    int32_t x=0;

    if (len < 1) return 0;

    Brewind(fp);

    while (1)
    {
        if (!Bfgets(buf, 1000, fp)) return 0;

        if (buf[0] == ';') continue;

        eq = Bstrchr(buf, '=');
        if (!eq) continue;

        k = buf;
        v = eq+1;

        while (*k == ' ' || *k == '\t') k++;
        *(eq--) = 0;
        while ((*eq == ' ' || *eq == '\t') && eq>=k) *(eq--) = 0;

        if (Bstrcasecmp(k, key)) continue;

        while (*v == ' ' || *k == '\t') v++;
        eq = v + Bstrlen(v)-1;

        while ((*eq == ' ' || *eq == '\t' || *eq == '\r' || *eq == '\n') && eq>=v) *(eq--) = 0;

        value[--len] = 0;
        do value[x] = v[x]; while (v[x++] != 0 && len-- > 0);

        return x-1;
    }
}

extern int16_t brightness;
extern int32_t vsync;
extern char game_executable[BMAX_PATH];
extern int32_t fullscreen;
extern char option[9];
extern char keys[NUMBUILDKEYS];
extern char remap[256];
extern int32_t remapinit;
extern double msens;
extern int32_t editorgridextent, grid, autogrid;
static int32_t default_grid=3;
extern int32_t graphicsmode;
extern int32_t AmbienceToggle, MixRate;
extern int32_t ParentalLock;

/*
 * SETUP.DAT
 * 0      = video mode (0:chained 1:vesa 2:screen buffered 3/4/5:tseng/paradise/s3 6:red-blue)
 * 1      = sound (0:none)
 * 2      = music (0:none)
 * 3      = input (0:keyboard 1:+mouse)
 * 4      = multiplayer (0:single 1-4:com 5-11:ipx)
 * 5&0xf0 = com speed
 * 5&0x0f = com irq
 * 6&0xf0 = chained y-res
 * 6&0x0f = chained x-res or vesa mode
 * 7&0xf0 = sound samplerate
 * 7&0x01 = sound quality
 * 7&0x02 = 8/16 bit
 * 7&0x04 = mono/stereo
 *
 * bytes 8 to 26 are key settings:
 * 0      = Forward (0xc8)
 * 1      = Backward (0xd0)
 * 2      = Turn left (0xcb)
 * 3      = Turn right (0xcd)
 * 4      = Run (0x2a)
 * 5      = Strafe (0x9d)
 * 6      = Fire (0x1d)
 * 7      = Use (0x39)
 * 8      = Stand high (0x1e)
 * 9      = Stand low (0x2c)
 * 10     = Look up (0xd1)
 * 11     = Look down (0xc9)
 * 12     = Strafe left (0x33)
 * 13     = Strafe right (0x34)
 * 14     = 2D/3D switch (0x9c)
 * 15     = View cycle (0x1c)
 * 16     = 2D Zoom in (0xd)
 * 17     = 2D Zoom out (0xc)
 * 18     = Chat (0xf)
 */

int32_t loadsetup(const char *fn)
{
    BFILE *fp;
#define VL 1024
    char val[VL];
    int32_t i;

    if ((fp = Bfopen(fn, "rt")) == NULL) return -1;

    if (readconfig(fp, "forcesetup", val, VL) > 0) { if (Batoi(val) != 0) forcesetup = 1; else forcesetup = 0; }
    if (readconfig(fp, "fullscreen", val, VL) > 0) { if (Batoi(val) != 0) fullscreen = 1; else fullscreen = 0; }
    if (readconfig(fp, "resolution", val, VL) > 0)
    {
        i = Batoi(val) & 0x0f;
        if ((unsigned)i<13) { xdimgame = xdim2d = vesares[i][0]; ydimgame = ydim2d = vesares[i][1]; }
    }
    if (readconfig(fp, "2dresolution", val, VL) > 0)
    {
        i = Batoi(val) & 0x0f;
        if ((unsigned)i<13) { xdim2d = vesares[i][0]; ydim2d = vesares[i][1]; }
    }
    if (readconfig(fp, "xdim2d", val, VL) > 0) xdim2d = Batoi(val);
    if (readconfig(fp, "ydim2d", val, VL) > 0) ydim2d = Batoi(val);
    if (readconfig(fp, "xdim3d", val, VL) > 0) xdimgame = Batoi(val);
    if (readconfig(fp, "ydim3d", val, VL) > 0) ydimgame = Batoi(val);
//    if (readconfig(fp, "samplerate", val, VL) > 0) option[7] = (Batoi(val) & 0x0f) << 4;
    if (readconfig(fp, "music", val, VL) > 0) { if (Batoi(val) != 0) option[2] = 1; else option[2] = 0; }
    if (readconfig(fp, "mouse", val, VL) > 0) { if (Batoi(val) != 0) option[3] = 1; else option[3] = 0; }
    if (readconfig(fp, "bpp", val, VL) > 0) bppgame = Batoi(val);
    if (readconfig(fp, "vsync", val, VL) > 0) vsync = Batoi(val)?1:0;
    if (readconfig(fp, "editorgridextent", val, VL) > 0) editorgridextent = max(min(262144,Batoi(val)),32768);
    if (readconfig(fp, "grid", val, VL) > 0)
    {
        grid = Batoi(val);
        default_grid = grid;
        autogrid = (grid==9);
        grid = min(max(0, grid), 8);
    }
#ifdef POLYMER
    if (readconfig(fp, "rendmode", val, VL) > 0) { i = Batoi(val); glrendmode = i; }
#endif
    if (readconfig(fp, "vid_gamma", val, VL) > 0) vid_gamma = Bstrtod(val, NULL);
    if (readconfig(fp, "vid_brightness", val, VL) > 0) vid_brightness = Bstrtod(val, NULL);
    if (readconfig(fp, "vid_contrast", val, VL) > 0) vid_contrast = Bstrtod(val, NULL);
#ifdef RENDERTYPEWIN
    if (readconfig(fp, "maxrefreshfreq", val, VL) > 0) maxrefreshfreq = Batoi(val);
#endif
#if defined(POLYMOST) && defined(USE_OPENGL)
    if (readconfig(fp, "usemodels", val, VL) > 0) usemodels = Batoi(val)?1:0;
    if (readconfig(fp, "usehightile", val, VL) > 0) usehightile = Batoi(val)?1:0;

    glusetexcache = -1;
    if (readconfig(fp, "glusetexcache", val, VL) > 0)
    {
        glusetexcache = clamp(Batoi(val), 0, 2);
    }
    if (readconfig(fp, "gltexfiltermode", val, VL) > 0)
    {
        gltexfiltermode = Batoi(val);
    }
    if (readconfig(fp, "glanisotropy", val, VL) > 0)
    {
        glanisotropy = Batoi(val);
    }
#endif

    if (readconfig(fp, "gameexecutable", val, VL) > 0)
        Bstrcpy(game_executable, val);

    option[0] = 1;	// vesa all the way...
    option[1] = 1;	// sound all the way...
    option[4] = 0;	// no multiplayer
    option[5] = 0;

#if 1
    if (readconfig(fp, "keyforward", val, VL) > 0) keys[0] = Bstrtol(val, NULL, 16);
    if (readconfig(fp, "keybackward", val, VL) > 0) keys[1] = Bstrtol(val, NULL, 16);
    if (readconfig(fp, "keyturnleft", val, VL) > 0) keys[2] = Bstrtol(val, NULL, 16);
    if (readconfig(fp, "keyturnright", val, VL) > 0) keys[3] = Bstrtol(val, NULL, 16);
    if (readconfig(fp, "keyrun", val, VL) > 0) keys[4] = Bstrtol(val, NULL, 16);
    if (readconfig(fp, "keystrafe", val, VL) > 0) keys[5] = Bstrtol(val, NULL, 16);
    if (readconfig(fp, "keyfire", val, VL) > 0) keys[6] = Bstrtol(val, NULL, 16);
    if (readconfig(fp, "keyuse", val, VL) > 0) keys[7] = Bstrtol(val, NULL, 16);
    if (readconfig(fp, "keystandhigh", val, VL) > 0) keys[8] = Bstrtol(val, NULL, 16);
    if (readconfig(fp, "keystandlow", val, VL) > 0) keys[9] = Bstrtol(val, NULL, 16);
    if (readconfig(fp, "keylookup", val, VL) > 0) keys[10] = Bstrtol(val, NULL, 16);
    if (readconfig(fp, "keylookdown", val, VL) > 0) keys[11] = Bstrtol(val, NULL, 16);
    if (readconfig(fp, "keystrafeleft", val, VL) > 0) keys[12] = Bstrtol(val, NULL, 16);
    if (readconfig(fp, "keystraferight", val, VL) > 0) keys[13] = Bstrtol(val, NULL, 16);
    if (readconfig(fp, "key2dmode", val, VL) > 0) keys[14] = Bstrtol(val, NULL, 16);
    if (readconfig(fp, "keyviewcycle", val, VL) > 0) keys[15] = Bstrtol(val, NULL, 16);
    if (readconfig(fp, "key2dzoomin", val, VL) > 0) keys[16] = Bstrtol(val, NULL, 16);
    if (readconfig(fp, "key2dzoomout", val, VL) > 0) keys[17] = Bstrtol(val, NULL, 16);
    if (readconfig(fp, "keychat", val, VL) > 0) keys[18] = Bstrtol(val, NULL, 16);
#endif

#ifdef RENDERTYPEWIN
    if (readconfig(fp, "windowpositioning", val, VL) > 0) windowpos = Batoi(val);
    windowx = -1;
    if (readconfig(fp, "windowposx", val, VL) > 0) windowx = Batoi(val);
    windowy = -1;
    if (readconfig(fp, "windowposy", val, VL) > 0) windowy = Batoi(val);
#endif

    if (readconfig(fp, "keyconsole", val, VL) > 0) { keys[19] = Bstrtol(val, NULL, 16); OSD_CaptureKey(keys[19]); }

    if (readconfig(fp, "mousesensitivity", val, VL) > 0) msens = Bstrtod(val, NULL);

    if (readconfig(fp, "mousenavigation", val, VL) > 0) unrealedlook = Batoi(val);

    if (readconfig(fp, "mousenavigationaccel", val, VL) > 0) pk_uedaccel = Batoi(val);

    if (readconfig(fp, "quickmapcycling", val, VL) > 0) quickmapcycling = Batoi(val);

    if (readconfig(fp, "revertCTRL", val, VL) > 0) revertCTRL = Batoi(val);

    if (readconfig(fp, "scrollamount", val, VL) > 0) scrollamount = Batoi(val);

    if (readconfig(fp, "turnaccel", val, VL) > 0) pk_turnaccel = Batoi(val);

    if (readconfig(fp, "turndecel", val, VL) > 0) pk_turndecel = Batoi(val);

    if (readconfig(fp, "autosave", val, VL) > 0) autosave = Batoi(val)*60;
    if (readconfig(fp, "autosavesec", val, VL) > 0) autosave = Batoi(val);

    if (readconfig(fp, "showheightindicators", val, VL) > 0)
        showheightindicators = min(max(Batoi(val),0),2);
    if (readconfig(fp, "showambiencesounds", val, VL) > 0)
        showambiencesounds = min(max(Batoi(val),0),2);

    if (readconfig(fp, "graphicsmode", val, VL) > 0)
        graphicsmode = min(max(Batoi(val),0),2);

    if (readconfig(fp, "samplerate", val, VL) > 0) MixRate = min(max(8000, Batoi(val)), 48000);
    if (readconfig(fp, "ambiencetoggle", val, VL) > 0) AmbienceToggle = Batoi(val);
    if (readconfig(fp, "parlock", val, VL) > 0) ParentalLock = Batoi(val);

    if (readconfig(fp, "osdtryscript", val, VL) > 0) m32_osd_tryscript = Batoi(val);

    for (i=0; i<256; i++)
        remap[i]=i;

    remapinit=1;
    if (readconfig(fp, "remap", val, VL) > 0)
    {
        char *p=val; int32_t v1,v2;
        while (*p)
        {
            if (!sscanf(p,"%x",&v1))break;
            if ((p=strchr(p,'-'))==0)break; p++;
            if (!sscanf(p,"%x",&v2))break;
            remap[v1]=v2;
            initprintf("Remap %X key to %X\n",v1,v2);
            if ((p=strchr(p,','))==0)break; p++;
        }
    }

    // load m32script history
    for (i=0; i<SCRIPTHISTSIZ; i++)
    {
        Bsprintf(val, "hist%d", i);
        if (readconfig(fp, val, val, VL) <= 0)
            break;

        scripthist[i] = Bstrdup(val);
    }

    scripthistend = i;

    // copy script history into OSD history
    for (i=0; i<min(scripthistend, OSD_HISTORYDEPTH); i++)
    {
        Bstrncpy(osdhistorybuf[i], scripthist[scripthistend-1-i], OSD_EDITLENGTH+1);
        osdhistorybuf[i][OSD_EDITLENGTH] = 0;
        osdhistorysize++;
        osdhistorytotal++;
    }

    scripthistend %= SCRIPTHISTSIZ;

    Bfclose(fp);

    return 0;
}

int32_t writesetup(const char *fn)
{
    BFILE *fp;
    int32_t i,j,first=1;

    fp = Bfopen(fn,"wt");
    if (!fp) return -1;

    Bfprintf(fp,
             "; Always show configuration options on startup\n"
             ";   0 - No\n"
             ";   1 - Yes\n"
             "forcesetup = %d\n"
             "\n"
             "; Video mode selection\n"
             ";   0 - Windowed\n"
             ";   1 - Fullscreen\n"
             "fullscreen = %d\n"
             "\n"
             "; Video resolution\n"
             "xdim2d = %d\n"
             "ydim2d = %d\n"
             "xdim3d = %d\n"
             "ydim3d = %d\n"
             "\n"
             "; 3D-mode colour depth\n"
             "bpp = %d\n"
             "\n"
             "vsync = %d\n"
             "\n"
#ifdef POLYMER
             "; Rendering mode\n"
             "rendmode = %d\n"
             "\n"
#endif
             "; Grid limits\n"
             "editorgridextent = %d\n"
             "; Startup grid size (0-8, 9 is automatic)\n"
             "grid = %d\n"
             "\n"
#if defined(POLYMOST) && defined(USE_OPENGL)
             "; OpenGL mode options\n"
             "usemodels = %d\n"
             "usehightile = %d\n"
             "glusetexcache = %d\n"
             "gltexfiltermode = %d\n"
             "glanisotropy = %d\n"
             "\n"
#endif

#ifdef RENDERTYPEWIN
             "; Maximum OpenGL mode refresh rate (Windows only, in Hertz)\n"
             "maxrefreshfreq = %d\n"
             "\n"
             "; Window positioning, 0 = center, 1 = memory\n"
             "windowpositioning = %d\n"
             "windowposx = %d\n"
             "windowposy = %d\n"
             "\n"
#endif
             "; 3D mode brightness setting\n"
             "vid_gamma = %g\n"
             "vid_brightness = %g\n"
             "vid_contrast = %g\n"
             "\n"
             "; Game executable used for map testing\n"
             "gameexecutable = %s\n"
             "\n"
#if 0
             "; Sound sample frequency\n"
             ";   0 - 6 KHz\n"
             ";   1 - 8 KHz\n"
             ";   2 - 11.025 KHz\n"
             ";   3 - 16 KHz\n"
             ";   4 - 22.05 KHz\n"
             ";   5 - 32 KHz\n"
             ";   6 - 44.1 KHz\n"
             "samplerate = %d\n"
             "\n"
             "; Music playback\n"
             ";   0 - Off\n"
             ";   1 - On\n"
             "music = %d\n"
             "\n"
#endif
             "; Enable mouse\n"
             ";   0 - No\n"
             ";   1 - Yes\n"
             "mouse = %d\n"
             "\n"
             "; Mouse sensitivity\n"
             "mousesensitivity = %g\n"
             "\n"
             "; Mouse navigation\n"
             ";   0 - No\n"
             ";   1 - Yes\n"
             "mousenavigation = %d\n"
             "\n"
             "; Mouse navigation acceleration\n"
             "mousenavigationaccel = %d\n"
             "\n"
             "; Quick map cycling (SHIFT)+CTRL+X\n"
             ";   0 - No\n"
             ";   1 - Yes\n"
             "quickmapcycling = %d\n"
             "\n"
             "; Revert CTRL for tile selction\n"
             ";   0 - WHEEL:scrolling, CTRL+WHEEL:zooming\n"
             ";   1 - CTRL+WHEEL:scrolling, WHEEL:zooming\n"
             "revertCTRL = %d\n"
             "\n"
             "; Scroll amount for WHEEL in the tile selcetion\n"
             "scrollamount = %d\n"
             "\n"
             "; Turning acceleration+declaration\n"
             "turnaccel = %d\n"
             "\n"
             "; Turning deceleration\n"
             "turndecel = %d\n"
             "\n"
             "; Autosave map interval (seconds)\n"
             "autosavesec = %d\n"
             "\n"
             "; Height indicators (0:none, 1:only 2-sided&different, 2:all)\n"
             "showheightindicators = %d\n"
             "\n"
             "; Ambience sound circles (0:none, 1:only in current sector, 2:all)\n"
             "showambiencesounds = %d\n"
             "\n"
             "; 2D mode display type (0:classic, 1:textured, 2:textured/animated)\n"
             "graphicsmode = %d\n"
             "\n"
             "; Sample rate in Hz\n"
             "samplerate = %d\n"
             "; Ambient sounds in 3D mode (0:off, 1:on)\n"
             "ambiencetoggle = %d\n"
             "parlock = %d\n"
             "\n"
             "; Try executing m32script on invalid command in the OSD? This makes\n"
             "; typing m32script commands into the OSD directly possible.\n"
             "osdtryscript = %d\n"
             "\n"
#if 1
             "; Key Settings\n"
             ";  Here's a map of all the keyboard scan codes: NOTE: values are listed in hex!\n"
             "; +---------------------------------------------------------------------------------------------+\n"
             "; | 01   3B  3C  3D  3E   3F  40  41  42   43  44  57  58          46                           |\n"
             "; |ESC   F1  F2  F3  F4   F5  F6  F7  F8   F9 F10 F11 F12        SCROLL                         |\n"
             "; |                                                                                             |\n"
             "; |29  02  03  04  05  06  07  08  09  0A  0B  0C  0D   0E     D2  C7  C9      45  B5  37  4A   |\n"
             "; | ` '1' '2' '3' '4' '5' '6' '7' '8' '9' '0'  -   =  BACK    INS HOME PGUP  NUMLK KP/ KP* KP-  |\n"
             "; |                                                                                             |\n"
             "; | 0F  10  11  12  13  14  15  16  17  18  19  1A  1B  2B     D3  CF  D1      47  48  49  4E   |\n"
             "; |TAB  Q   W   E   R   T   Y   U   I   O   P   [   ]    \\    DEL END PGDN    KP7 KP8 KP9 KP+   |\n"
             "; |                                                                                             |\n"
             "; | 3A   1E  1F  20  21  22  23  24  25  26  27  28     1C                     4B  4C  4D       |\n"
             "; |CAPS  A   S   D   F   G   H   J   K   L   ;   '   ENTER                    KP4 KP5 KP6    9C |\n"
             "; |                                                                                      KPENTER|\n"
             "; |  2A    2C  2D  2E  2F  30  31  32  33  34  35    36            C8          4F  50  51       |\n"
             "; |LSHIFT  Z   X   C   V   B   N   M   ,   .   /   RSHIFT          UP         KP1 KP2 KP3       |\n"
             "; |                                                                                             |\n"
             "; | 1D     38              39                  B8     9D       CB  D0   CD      52    53        |\n"
             "; |LCTRL  LALT           SPACE                RALT   RCTRL   LEFT DOWN RIGHT    KP0    KP.      |\n"
             "; +---------------------------------------------------------------------------------------------+\n"
             "\n"
             "keyforward = %X\n"
             "keybackward = %X\n"
             "keyturnleft = %X\n"
             "keyturnright = %X\n"
             "keyrun = %X\n"
             "keystrafe = %X\n"
             "keyfire = %X\n"
             "keyuse = %X\n"
             "keystandhigh = %X\n"
             "keystandlow = %X\n"
             "keylookup = %X\n"
             "keylookdown = %X\n"
             "keystrafeleft = %X\n"
             "keystraferight = %X\n"
             "key2dmode = %X\n"
             "keyviewcycle = %X\n"
             "key2dzoomin = %X\n"
             "key2dzoomout = %X\n"
             "keychat = %X\n"
#endif
//             "; Console key scancode, in hex\n"
             "keyconsole = %X\n"
             "; example: make 'Q' function as CapsLock, KP. as AltGr\n"
             "; and KP0 as KP5 (counters inability to pan using Shift-KP5-KP8/2\n"
             "; in 3D mode)\n"
             "; remap = 10-3A,52-4C,53-B8\n"
             "remap = ",

             forcesetup, fullscreen, xdim2d, ydim2d, xdimgame, ydimgame, bppgame, vsync,
#ifdef POLYMER
             glrendmode,
#endif
             editorgridextent, min(max(0, default_grid), 9),
#if defined(POLYMOST) && defined(USE_OPENGL)
             usemodels, usehightile,
             glusetexcache, gltexfiltermode, glanisotropy,
#endif
#ifdef RENDERTYPEWIN
             maxrefreshfreq, windowpos, windowx, windowy,
#endif
             vid_gamma_3d>=0?vid_gamma_3d:vid_gamma,
             vid_brightness_3d>=0?vid_brightness_3d:vid_brightness,
             vid_contrast_3d>=0?vid_contrast_3d:vid_contrast,
             game_executable,
#if 0
             option[7]>>4, option[2],
#endif
             option[3], msens, unrealedlook, pk_uedaccel, quickmapcycling,
             revertCTRL,scrollamount,pk_turnaccel,pk_turndecel,autosave,
             showheightindicators,showambiencesounds,graphicsmode,
             MixRate,AmbienceToggle,ParentalLock, !!m32_osd_tryscript,
#if 1
             keys[0], keys[1], keys[2], keys[3], keys[4], keys[5],
             keys[6], keys[7], keys[8], keys[9], keys[10], keys[11],
             keys[12], keys[13], keys[14], keys[15], keys[16], keys[17],
             keys[18],
#endif
             keys[19]
            );

    for (i=0; i<256; i++)
        if (remap[i]!=i)
        {
            Bfprintf(fp,first?"%02X-%02X":",%02X-%02X",i,remap[i]);
            first=0;
        }
    Bfprintf(fp,"\n\n");

    // save m32script history
    first = 1;
    for (i=scripthistend, j=0; first || i!=scripthistend; i=(i+1)%SCRIPTHISTSIZ, first=0)
    {
        if (scripthist[i])
            Bfprintf(fp, "hist%d = %s\n", j++, scripthist[i]);
    }

    Bfclose(fp);

    return 0;
}
