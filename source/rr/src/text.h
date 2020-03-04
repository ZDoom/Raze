//-------------------------------------------------------------------------
/*
Copyright (C) 2016 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#pragma once

#include "menus.h"
#include "screentext.h"

BEGIN_RR_NS

#define MAXUSERQUOTES 6

extern int32_t user_quote_time[MAXUSERQUOTES];
extern int32_t minitext_lowercase;
extern int32_t minitext_yofs;


extern int32_t minitext_(int32_t x, int32_t y, const char *t, int32_t s, int32_t p, int32_t sb);
extern void menutext_(int32_t x, int32_t y, int32_t s, char const *t, int32_t o, int32_t f);
extern void captionmenutext(int32_t x, int32_t y, char const *t);
extern vec2_t gametext_(int32_t x, int32_t y, const char *t, int32_t s, int32_t p, int32_t o, int32_t a, int32_t f);
extern void gametext_simple(int32_t x, int32_t y, const char *t);
#define mpgametext_x (5<<16)
extern vec2_t mpgametext(int32_t x, int32_t y, char const * t, int32_t s, int32_t o, int32_t a, int32_t f);
extern vec2_t mpgametextsize(char const * t, int32_t f);
extern int32_t textsc(int32_t sc);

#define minitextshade(x, y, t, s, p, sb) minitext_(x,y,t,s,p,sb)
#define minitext(x, y, t, p, sb) minitext_(x,y,t,0,p,sb)
#define menutext(x, y, t) menutext_((x)<<16, (y)<<16, 0, (t), 10|16, 0)
#define menutext_centeralign(x, y, t) menutext_((x), (y), 0, (t), 10|16, TEXT_XCENTER|TEXT_YCENTER)
#define menutext_center(y, t) menutext_(160<<16, (y)<<16, 0, (t), 10|16, TEXT_XCENTER)
#define gametext(x, y, t) gametext_simple((x)<<16, (y)<<16, (t))
#define gametext_widenumber(x, y, t) gametext_((x)<<16, (y)<<16, (t), 0, MF_Bluefont.pal, 1024, 0, TEXT_GAMETEXTNUMHACK)
#define gametext_number(x, y, t) gametext_((x)<<16, (y)<<16, (t), 0, MF_Bluefont.pal, 0, 0, TEXT_GAMETEXTNUMHACK)
#define gametext_pal(x, y, t, p) gametext_((x)<<16, (y)<<16, (t), 0, (p), 0, 0, 0)
#define gametext_center(y, t) gametext_(160<<16, (y)<<16, (t), 0, MF_Bluefont.pal, 0, 0, TEXT_XCENTER)
#define gametext_center_number(y, t) gametext_(160<<16, (y)<<16, (t), 0, MF_Bluefont.pal, 0, 0, TEXT_XCENTER|TEXT_GAMETEXTNUMHACK)
#define gametext_center_shade(y, t, s) gametext_(160<<16, (y)<<16, (t), (s), MF_Bluefont.pal, 0, 0, TEXT_XCENTER)
#define gametext_center_shade_pal(y, t, s, p) gametext_(160<<16, (y)<<16, (t), (s), (p), 0, 0, TEXT_XCENTER)


END_RR_NS
