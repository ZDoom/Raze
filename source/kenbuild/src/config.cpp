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

static int vesares[13][2] = {{320,200},{360,200},{320,240},{360,240},{320,400},
                             {360,400},{640,350},{640,400},{640,480},{800,600},
                             {1024,768},{1280,1024},{1600,1200}};

static int readconfig(BFILE *fp, const char *key, char *value, unsigned len)
{
    char buf[1000], *k, *v, *eq;
    int x=0;

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

extern short brightness;
extern int fullscreen;
extern unsigned char option[8];
extern unsigned char keys[NUMBUILDKEYS];
double msens;

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

int Ken_loadsetup(const char *fn)
{
    BFILE *fp;
#define VL 32
    char val[VL];
    int i;

    if ((fp = Bfopen(fn, "rt")) == NULL) return -1;

    if (readconfig(fp, "forcesetup", val, VL) > 0) { if (Batoi(val) != 0) forcesetup = 1; else forcesetup = 0; }
    if (readconfig(fp, "fullscreen", val, VL) > 0) { if (Batoi(val) != 0) fullscreen = 1; else fullscreen = 0; }
    if (readconfig(fp, "resolution", val, VL) > 0)
    {
        i = Batoi(val) & 0x0f;
        if ((unsigned)i<13) { xdimgame = xdim2d = vesares[i][0]; ydimgame = ydim2d = vesares[i][1]; }
    }
    if (readconfig(fp, "xdim", val, VL) > 0) xdimgame = xdim2d = Batoi(val);
    if (readconfig(fp, "ydim", val, VL) > 0) ydimgame = xdim2d = Batoi(val);
    if (readconfig(fp, "samplerate", val, VL) > 0) option[7] = (Batoi(val) & 0x0f) << 4;
    if (readconfig(fp, "music", val, VL) > 0) { if (Batoi(val) != 0) option[2] = 1; else option[2] = 0; }
    if (readconfig(fp, "mouse", val, VL) > 0) { if (Batoi(val) != 0) option[3] = 1; else option[3] = 0; }
    if (readconfig(fp, "bpp", val, VL) > 0) bppgame = Batoi(val);
    if (readconfig(fp, "renderer", val, VL) > 0) { i = Batoi(val); setrendermode(i); }
    if (readconfig(fp, "brightness", val, VL) > 0) brightness = min(max(Batoi(val),0),15);

#ifdef RENDERTYPEWIN
    if (readconfig(fp, "maxrefreshfreq", val, VL) > 0) maxrefreshfreq = Batoi(val);
#endif

    option[0] = 1;  // vesa all the way...
    option[1] = 1;  // sound all the way...
    option[4] = 0;  // no multiplayer
    option[5] = 0;

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
    if (readconfig(fp, "keyconsole", val, VL) > 0) { keys[19] = Bstrtol(val, NULL, 16); OSD_CaptureKey(keys[19]); }

    if (readconfig(fp, "mousesensitivity", val, VL) > 0) msens = Bstrtod(val, NULL);

    Bfclose(fp);

    return 0;
}

int Ken_writesetup(const char *fn)
{
    BFILE *fp;

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
             "xdim = %d\n"
             "ydim = %d\n"
             "\n"
             "; 3D-mode colour depth\n"
             "bpp = %d\n"
             "\n"
#ifdef USE_OPENGL
             "; OpenGL mode options\n"
             "glusetexcache = %d\n"
             "\n"
#endif
#ifdef RENDERTYPEWIN
             "; Maximum OpenGL mode refresh rate (Windows only, in Hertz)\n"
             "maxrefreshfreq = %d\n"
             "\n"
#endif
             "; 3D mode brightness setting\n"
             ";   0  - lowest\n"
             ";   15 - highest\n"
             "brightness = %d\n"
             "\n"
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
             "; Enable mouse\n"
             ";   0 - No\n"
             ";   1 - Yes\n"
             "mouse = %d\n"
             "\n"
             "; Mouse sensitivity\n"
             "mousesensitivity = %g\n"
             "\n"
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
             "keyconsole = %X\n"
             "\n",

             forcesetup, fullscreen, xdimgame, ydimgame, bppgame,
#ifdef USE_OPENGL
             glusetexcache,
#endif
#ifdef RENDERTYPEWIN
             maxrefreshfreq,
#endif
             brightness, option[7]>>4, option[2],
             option[3], msens,
             keys[0], keys[1], keys[2], keys[3], keys[4], keys[5],
             keys[6], keys[7], keys[8], keys[9], keys[10], keys[11],
             keys[12], keys[13], keys[14], keys[15], keys[16], keys[17],
             keys[18], keys[19]
             );

    Bfclose(fp);

    return 0;
}
