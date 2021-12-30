// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// This file has been modified from Ken Silverman's original release
// by Jonathon Fowler (jf@jonof.id.au)
// by the EDuke32 team (development@voidpoint.com)

#pragma once

#include "cmdlib.h"

#ifndef ENGINE_PRIV_H
#define ENGINE_PRIV_H

extern int32_t globalpal, globalfloorpal;
extern int32_t xdimen, xdimenscale, xdimscale, ydimen;
extern float fxdimen;
extern int32_t globalposx, globalposy, globalposz;
extern fixed_t qglobalhoriz, qglobalang;
extern float fglobalposx, fglobalposy, fglobalposz;
extern int16_t globalang, globalcursectnum;
extern int32_t globalpal, cosglobalang, singlobalang;
extern int32_t cosviewingrangeglobalang, sinviewingrangeglobalang;
extern int32_t xyaspect;
extern int32_t globalshade;
extern int16_t globalpicnum;

extern int32_t globalorientation;

extern int32_t hitallsprites;

int32_t animateoffs(int tilenum, int fakevar);


#endif	/* ENGINE_PRIV_H */
