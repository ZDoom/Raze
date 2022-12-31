#pragma once

#include "v_2ddrawer.h"

extern F2DDrawer twodpsp;

void DrawRateStuff();
void hud_drawsprite(double sx, double sy, double sz, double a, int picnum, int dashade, int dapalnum, int dastat, double alpha = 1);
void hud_drawsprite(double sx, double sy, double sz, double a, FTextureID texid, int dashade, int dapalnum, int dastat, double alpha = 1);

// orientation bits for hud_drawsprite.
enum {
    RS_TRANS1 = 1,
    RS_YFLIP = 4,
    RS_TOPLEFT = 16,
    RS_TRANS2 = 32,
    RS_NOMASK = 64,

    RS_ALIGN_L = 256,
    RS_ALIGN_R = 512,
    RS_ALIGN_MASK = 768,
    RS_STRETCH = 1024,

    RS_XFLIPHUD = RS_YFLIP,
    RS_YFLIPHUD = 16384, // this is for hud_drawsprite which uses RS_YFLIP for x-flipping but needs both flags

    RS_CENTER = (1 << 29),    // proper center align.
};

