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
#include "ns.h"	// Must come before everything else!

#include "duke3d.h"
#include "compat.h"
#include "sbar.h"
#include "menus.h"
#include "gstrings.h"

BEGIN_DUKE_NS

// assign the character's tilenum
int GameInterface::GetStringTile(int font, const char* t, int f)
{
    if (f & TEXT_DIGITALNUMBER)
        return *t - '0' + font; // copied from digitalnumber
    else if (f & (TEXT_BIGALPHANUM|TEXT_GRAYFONT))
    {
        int32_t offset = (f & TEXT_GRAYFONT) ? 26 : 0;

        if (*t >= '0' && *t <= '9')
            return *t - '0' + font + ((f & TEXT_GRAYFONT) ? 26 : -10);
        else if (*t >= 'a' && *t <= 'z')
            return *t - 'a' + font + ((f & TEXT_GRAYFONT) ? -26 : 26);
        else if (*t >= 'A' && *t <= 'Z')
            return *t - 'A' + font;
        else switch (*t)
        {
        case '_':
        case '-':
            return font - (11 + offset);
            break;
        case '.':
            return font + (TILE_BIGPERIOD - (TILE_BIGALPHANUM + offset));
            break;
        case ',':
            return font + (TILE_BIGCOMMA - (TILE_BIGALPHANUM + offset));
            break;
        case '!':
            return font + (TILE_BIGX_ - (TILE_BIGALPHANUM + offset));
            break;
        case '?':
            return font + (TILE_BIGQ - (TILE_BIGALPHANUM + offset));
            break;
        case ';':
            return font + (TILE_BIGSEMI - (TILE_BIGALPHANUM + offset));
            break;
        case ':':
            return font + (TILE_BIGCOLIN - (TILE_BIGALPHANUM + offset));
            break;
        case '\\':
        case '/':
            return font + (68 - offset); // 3008-2940
            break;
        case '%':
            return font + (69 - offset); // 3009-2940
            break;
        case '`':
        case '\"': // could be better hacked in
        case '\'':
            return font + (TILE_BIGAPPOS - (TILE_BIGALPHANUM + offset));
            break;
        default: // unknown character
            fallthrough__;
        case '\t':
        case ' ':
        case '\n':
        case '\x7F':
            return font;
            break;
        }
    }
    else
        return *t - '!' + font; // uses ASCII order
}


vec2_t gametext_(int32_t x, int32_t y, const char *t, int32_t s, int32_t p, int32_t o, int32_t a, int32_t f)
{
    return G_ScreenText(MF_Bluefont.tilenum, x, y, MF_Bluefont.zoom, 0, 0, t, s, p, o|2|8|16|ROTATESPRITE_FULL16, a, MF_Bluefont.emptychar.x, MF_Bluefont.emptychar.y, MF_Bluefont.between.x, MF_Bluefont.between.y, MF_Bluefont.textflags|f, 0, 0, xdim-1, ydim-1);
}
void gametext_simple(int32_t x, int32_t y, const char *t)
{
    G_ScreenText(MF_Bluefont.tilenum, x, y, MF_Bluefont.zoom, 0, 0, t, 0, MF_Bluefont.pal, 2|8|16|ROTATESPRITE_FULL16, 0, MF_Bluefont.emptychar.x, MF_Bluefont.emptychar.y, MF_Bluefont.between.x, MF_Bluefont.between.y, MF_Bluefont.textflags, 0, 0, xdim-1, ydim-1);
}
vec2_t mpgametext(int32_t x, int32_t y, const char *t, int32_t s, int32_t o, int32_t a, int32_t f)
{
    return G_ScreenText(MF_Bluefont.tilenum, x, y, textsc(MF_Bluefont.zoom), 0, 0, t, s, MF_Bluefont.pal, o|2|8|16|ROTATESPRITE_FULL16, a, MF_Bluefont.emptychar.x, MF_Bluefont.emptychar.y, MF_Bluefont.between.x, MF_Bluefont.between.y, MF_Bluefont.textflags|f, 0, 0, xdim-1, ydim-1);
}
vec2_t mpgametextsize(const char *t, int32_t f)
{
    return G_ScreenTextSize(MF_Bluefont.tilenum, 0, 0, textsc(MF_Bluefont.zoom), 0, t, 2|8|16|ROTATESPRITE_FULL16, MF_Bluefont.emptychar.x, MF_Bluefont.emptychar.y, MF_Bluefont.between.x, MF_Bluefont.between.y, MF_Bluefont.textflags|f, 0, 0, xdim-1, ydim-1);
}

static int32_t sbarx16(int32_t x)
{
    if (ud.screen_size == 4) return sbarsc(x);
    return (((320 << 16) - sbarsc(320 << 16)) >> 1) + sbarsc(x);
}

static int32_t sbarxr16(int32_t x)
{
    if (ud.screen_size == 4) return (320 << 16) - sbarsc(x);
    return (((320 << 16) - sbarsc(320 << 16)) >> 1) + sbarsc(x);
}

static int32_t sbary16(int32_t y)
{
    return (100 << 16) - sbarsc(200 << 16) + sbarsc(y);
}

// minitext_yofs: in hud_scale-independent, (<<16)-scaled, 0-200-normalized y coords,
// (sb&ROTATESPRITE_MAX) only.
int32_t minitext_yofs = 0;
int32_t minitext_lowercase = 0;
int32_t minitext_(int32_t x, int32_t y, const char *t, int32_t s, int32_t p, int32_t sb)
{
    vec2_t dim;
    int32_t z = MF_Minifont.zoom;

    if (t == NULL)
    {
        Printf("minitext: NULL text!\n");
        return 0;
    }

    if (!(sb & ROTATESPRITE_FULL16))
    {
        x<<=16;
        y<<=16;
    }

    if (sb & ROTATESPRITE_MAX)
    {
        if (sb & RS_ALIGN_R)
            x = sbarxr16(x);
        else
            x = sbarx16(x);
        y = minitext_yofs+sbary16(y);
        z = sbarsc(z);
    }

    sb &= (ROTATESPRITE_MAX-1)|RS_CENTERORIGIN;

    dim = G_ScreenText(MF_Minifont.tilenum, x, y, z, 0, 0, t, s, p, sb|ROTATESPRITE_FULL16, 0, MF_Minifont.emptychar.x, MF_Minifont.emptychar.y, MF_Minifont.between.x, MF_Minifont.between.y, MF_Minifont.textflags, 0, 0, xdim-1, ydim-1);

    x += dim.x;

    if (!(sb & ROTATESPRITE_FULL16))
        x >>= 16;

    return x;
}

void menutext_(int32_t x, int32_t y, int32_t s, char const *t, int32_t o, int32_t f)
{
    if (RR) f |= TEXT_RRMENUTEXTHACK;
    G_ScreenText(MF_Redfont.tilenum, x, y - (12<<16), MF_Redfont.zoom, 0, 0, t, s, MF_Redfont.pal, o|ROTATESPRITE_FULL16, 0, MF_Redfont.emptychar.x, MF_Redfont.emptychar.y, MF_Redfont.between.x, MF_Redfont.between.y, f|MF_Redfont.textflags|TEXT_LITERALESCAPE, 0, 0, xdim-1, ydim-1);
}

void captionmenutext(int32_t x, int32_t y, char const *t)
{
    G_ScreenText(MF_Redfont.tilenum, x, y - (12<<16), MF_Redfont.zoom, 0, 0, t, 0, ud.menutitle_pal, 2|8|16|ROTATESPRITE_FULL16, 0, MF_Redfont.emptychar.x, MF_Redfont.emptychar.y, MF_Redfont.between.x, MF_Redfont.between.y, MF_Redfont.textflags|TEXT_LITERALESCAPE|TEXT_XCENTER|TEXT_YCENTER, 0, 0, xdim-1, ydim-1);
}


int32_t user_quote_time[MAXUSERQUOTES];
static char user_quote[MAXUSERQUOTES][178];

void G_AddUserQuote(const char* daquote)
{
	int32_t i;

	if (hud_messages == 0) return;
	Printf(PRINT_MEDIUM | PRINT_NOTIFY, "%s\n", daquote);
	if (hud_messages == 1)
	{
		for (i = MAXUSERQUOTES - 1; i > 0; i--)
		{
			Bstrcpy(user_quote[i], user_quote[i - 1]);
			user_quote_time[i] = user_quote_time[i - 1];
		}
		Bstrcpy(user_quote[0], daquote);

		user_quote_time[0] = hud_messagetime;
		pub = NUMPAGES;
	}
}

int32_t textsc(int32_t sc)
{
    return scale(sc, hud_textscale, 400);
}

#define FTAOPAQUETIME 30

// alpha increments of 8 --> 256 / 8 = 32 --> round up to power of 2 --> 32 --> divide by 2 --> 16 alphatabs required
static inline int32_t textsh(uint32_t t)
{
    return (hud_glowingquotes && ((videoGetRenderMode() == REND_CLASSIC && numalphatabs < 15) || t >= FTAOPAQUETIME))
        ? sintable[(t << 7) & 2047] >> 11
        : (sintable[(FTAOPAQUETIME << 7) & 2047] >> 11);
}

// orientation flags depending on time that a quote has still to be displayed
static inline int32_t texto(int32_t t)
{
    if (videoGetRenderMode() != REND_CLASSIC || numalphatabs >= 15 || t > 4)
        return 0;

    if (t > 2)
        return 1;

    return 1|32;
}

static inline int32_t texta(int32_t t)
{
    return 255 - clamp(t<<3, 0, 255);
}

static FORCE_INLINE int32_t text_ypos(void)
{
    if (hud_position == 1 && ud.screen_size == 4 && ud.althud == 1)
        return 32<<16;

#ifdef GEKKO
    return 16<<16;
#elif defined EDUKE32_TOUCH_DEVICES
    return 24<<16;
#else
    return 1<<16;
#endif
}

static FString text_quote;	// To put text into the quote display that does not come from the quote array. (Is it really necessary to implement everything as a hack??? :( )

// this handles both multiplayer and item pickup message type text
// both are passed on to gametext
void G_PrintGameQuotes(int32_t snum)
{
    const DukePlayer_t *const ps = g_player[snum].ps;
    const int32_t reserved_quote = (ps->ftq >= QUOTE_RESERVED && ps->ftq <= QUOTE_RESERVED3);
    // NOTE: QUOTE_RESERVED4 is not included.

    int32_t const ybase = (fragbarheight()<<16) + text_ypos();
    int32_t height = 0;
    int32_t k = ps->fta;


    // primary quote

    do
    {
        if (k <= 1)
            break;

        int32_t y = ybase;
        if (reserved_quote)
        {
#ifdef SPLITSCREEN_MOD_HACKS
            if (!g_fakeMultiMode)
                y = 140<<16;
            else
                y = 70<<16;
#else
            y = 140<<16;
#endif
        }

        int32_t pal = 0;
        int32_t x = 160<<16;

#ifdef SPLITSCREEN_MOD_HACKS
        if (g_fakeMultiMode)
        {
            pal = g_player[snum].pcolor;
            const int32_t sidebyside = ud.screen_size != 0;

            if (sidebyside)
                x = snum == 1 ? 240<<16 : 80<<16;
            else if (snum == 1)
                y += 100<<16;
        }
#endif

		if (text_quote.IsNotEmpty() && ps->ftq == -32768) height = gametext_(x, y, text_quote, textsh(k), pal, texto(k), texta(k), TEXT_XCENTER).y + (1 << 16);
		else height = gametext_(x, y, quoteMgr.GetQuote(ps->ftq), textsh(k), pal, texto(k), texta(k), TEXT_XCENTER).y + (1 << 16);
	}
    while (0);


    // userquotes

    int32_t y = ybase;

    if (k > 1 && !reserved_quote)
        y += k <= 8 ? (height * (k-1))>>3 : height;

    for (size_t i = MAXUSERQUOTES-1; i < MAXUSERQUOTES; --i)
    {
        k = user_quote_time[i];

        if (k <= 0)
            continue;

        // int32_t const sh = hud_glowingquotes ? sintable[((totalclock+(i<<2))<<5)&2047]>>11 : 0;

        height = mpgametext(mpgametext_x, y, user_quote[i], textsh(k), texto(k), texta(k), TEXT_LINEWRAP).y + textsc(1<<16);
        y += k <= 4 ? (height * (k-1))>>2 : height;
    }
}

void P_DoQuote(int32_t q, DukePlayer_t *p)
{
    int32_t cq = 0;

    if (hud_messages == 0 || q < 0 || !(p->gm & MODE_GAME))
        return;

    if (q & MAXQUOTES)
    {
        cq = 1;
        q &= ~MAXQUOTES;
    }

    if (p->fta > 0 && q != QUOTE_RESERVED && q != QUOTE_RESERVED2)
        if (p->ftq == QUOTE_RESERVED || p->ftq == QUOTE_RESERVED2) return;

    if (p->ftq != q)
    {
		auto qu = quoteMgr.GetQuote(q);
        if (p == g_player[screenpeek].ps && qu[0] != '\0')
			Printf((cq ? PRINT_LOW : PRINT_MEDIUM) | PRINT_NOTIFY, "%s\n", qu);

    }

	if (hud_messages == 1)
	{
		p->ftq = q;
		p->fta = 100;
		pub = NUMPAGES;
		pus = NUMPAGES;
	}
}

void GameInterface::DoPrintMessage(int prio, const char* t)
{
	auto p = g_player[myconnectindex].ps; // text quotes always belong to the local player.
	int32_t cq = 0;

	if (hud_messages == 0 || !(p->gm & MODE_GAME))
		return;

	if (p->fta > 0)
		if (p->ftq == QUOTE_RESERVED || p->ftq == QUOTE_RESERVED2) return;

	if (p == g_player[screenpeek].ps)
		Printf(prio|PRINT_NOTIFY, cq ? TEXTCOLOR_TAN "%s\n" : "%s\n", t);

	if (hud_messages == 1)
	{
		p->fta = 100;
		p->ftq = -32768;
		text_quote = t;
		pub = NUMPAGES;
		pus = NUMPAGES;
	}
}

END_DUKE_NS
