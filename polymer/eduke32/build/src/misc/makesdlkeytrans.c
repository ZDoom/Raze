// gcc b.c -Lc:/mingw32/lib -lmingw32 -lSDLmain -lSDL

#include <stdio.h>
#include "SDL/SDL.h"

int keytranslation[SDLK_LAST];
char *keysyms[SDLK_LAST];

static int buildkeytranslationtable(void)
{
	memset(keytranslation,0,sizeof(keytranslation));
	memset(keysyms,0,sizeof(keysyms));

#define MAP(x,y) { \
	keytranslation[x] = y; \
	keysyms[x] = #x ; \
}
	MAP(SDLK_BACKSPACE,	0xe);
	MAP(SDLK_TAB,		0xf);
	MAP(SDLK_RETURN,	0x1c);
	MAP(SDLK_PAUSE,		0x59);	// 0x1d + 0x45 + 0x9d + 0xc5
	MAP(SDLK_ESCAPE,	0x1);
	MAP(SDLK_SPACE,		0x39);
	MAP(SDLK_EXCLAIM,	0x2);	// '1'
	MAP(SDLK_QUOTEDBL,	0x28);	// '''
	MAP(SDLK_HASH,		0x4);	// '3'
	MAP(SDLK_DOLLAR,	0x5);	// '4'
	MAP(37,			0x6);	// '5' <-- where's the keysym SDL guys?
	MAP(SDLK_AMPERSAND,	0x8);	// '7'
	MAP(SDLK_QUOTE,		0x28);	// '''
	MAP(SDLK_LEFTPAREN,	0xa);	// '9'
	MAP(SDLK_RIGHTPAREN,	0xb);	// '0'
	MAP(SDLK_ASTERISK,	0x9);	// '8'
	MAP(SDLK_PLUS,		0xd);	// '='
	MAP(SDLK_COMMA,		0x33);
	MAP(SDLK_MINUS,		0xc);
	MAP(SDLK_PERIOD,	0x34);
	MAP(SDLK_SLASH,		0x35);
	MAP(SDLK_0,		0xb);
	MAP(SDLK_1,		0x2);
	MAP(SDLK_2,		0x3);
	MAP(SDLK_3,		0x4);
	MAP(SDLK_4,		0x5);
	MAP(SDLK_5,		0x6);
	MAP(SDLK_6,		0x7);
	MAP(SDLK_7,		0x8);
	MAP(SDLK_8,		0x9);
	MAP(SDLK_9,		0xa);
	MAP(SDLK_COLON,		0x27);
	MAP(SDLK_SEMICOLON,	0x27);
	MAP(SDLK_LESS,		0x33);
	MAP(SDLK_EQUALS,	0xd);
	MAP(SDLK_GREATER,	0x34);
	MAP(SDLK_QUESTION,	0x35);
	MAP(SDLK_AT,		0x3);	// '2'
	MAP(SDLK_LEFTBRACKET,	0x1a);
	MAP(SDLK_BACKSLASH,	0x2b);
	MAP(SDLK_RIGHTBRACKET,	0x1b);
	MAP(SDLK_CARET,		0x7);	// '7'
	MAP(SDLK_UNDERSCORE,	0xc);
	MAP(SDLK_BACKQUOTE,	0x29);
	MAP(SDLK_a,		0x1e);
	MAP(SDLK_b,		0x30);
	MAP(SDLK_c,		0x2e);
	MAP(SDLK_d,		0x20);
	MAP(SDLK_e,		0x12);
	MAP(SDLK_f,		0x21);
	MAP(SDLK_g,		0x22);
	MAP(SDLK_h,		0x23);
	MAP(SDLK_i,		0x17);
	MAP(SDLK_j,		0x24);
	MAP(SDLK_k,		0x25);
	MAP(SDLK_l,		0x26);
	MAP(SDLK_m,		0x32);
	MAP(SDLK_n,		0x31);
	MAP(SDLK_o,		0x18);
	MAP(SDLK_p,		0x19);
	MAP(SDLK_q,		0x10);
	MAP(SDLK_r,		0x13);
	MAP(SDLK_s,		0x1f);
	MAP(SDLK_t,		0x14);
	MAP(SDLK_u,		0x16);
	MAP(SDLK_v,		0x2f);
	MAP(SDLK_w,		0x11);
	MAP(SDLK_x,		0x2d);
	MAP(SDLK_y,		0x15);
	MAP(SDLK_z,		0x2c);
	MAP(SDLK_DELETE,	0xd3);
	MAP(SDLK_KP0,		0x52);
	MAP(SDLK_KP1,		0x4f);
	MAP(SDLK_KP2,		0x50);
	MAP(SDLK_KP3,		0x51);
	MAP(SDLK_KP4,		0x4b);
	MAP(SDLK_KP5,		0x4c);
	MAP(SDLK_KP6,		0x4d);
	MAP(SDLK_KP7,		0x47);
	MAP(SDLK_KP8,		0x48);
	MAP(SDLK_KP9,		0x49);
	MAP(SDLK_KP_PERIOD,	0x53);
	MAP(SDLK_KP_DIVIDE,	0xb5);
	MAP(SDLK_KP_MULTIPLY,	0x37);
	MAP(SDLK_KP_MINUS,	0x4a);
	MAP(SDLK_KP_PLUS,	0x4e);
	MAP(SDLK_KP_ENTER,	0x9c);
	//MAP(SDLK_KP_EQUALS,	);
	MAP(SDLK_UP,		0xc8);
	MAP(SDLK_DOWN,		0xd0);
	MAP(SDLK_RIGHT,		0xcd);
	MAP(SDLK_LEFT,		0xcb);
	MAP(SDLK_INSERT,	0xd2);
	MAP(SDLK_HOME,		0xc7);
	MAP(SDLK_END,		0xcf);
	MAP(SDLK_PAGEUP,	0xc9);
	MAP(SDLK_PAGEDOWN,	0xd1);
	MAP(SDLK_F1,		0x3b);
	MAP(SDLK_F2,		0x3c);
	MAP(SDLK_F3,		0x3d);
	MAP(SDLK_F4,		0x3e);
	MAP(SDLK_F5,		0x3f);
	MAP(SDLK_F6,		0x40);
	MAP(SDLK_F7,		0x41);
	MAP(SDLK_F8,		0x42);
	MAP(SDLK_F9,		0x43);
	MAP(SDLK_F10,		0x44);
	MAP(SDLK_F11,		0x57);
	MAP(SDLK_F12,		0x58);
	MAP(SDLK_NUMLOCK,	0x45);
	MAP(SDLK_CAPSLOCK,	0x3a);
	MAP(SDLK_SCROLLOCK,	0x46);
	MAP(SDLK_RSHIFT,	0x36);
	MAP(SDLK_LSHIFT,	0x2a);
	MAP(SDLK_RCTRL,		0x9d);
	MAP(SDLK_LCTRL,		0x1d);
	MAP(SDLK_RALT,		0xb8);
	MAP(SDLK_LALT,		0x38);
	MAP(SDLK_LSUPER,	0xdb);	// win l
	MAP(SDLK_RSUPER,	0xdc);	// win r
	MAP(SDLK_PRINT,		-2);	// 0xaa + 0xb7
	MAP(SDLK_SYSREQ,	0x54);	// alt+printscr
	MAP(SDLK_BREAK,		0xb7);	// ctrl+pause
	MAP(SDLK_MENU,		0xdd);	// win menu?
#undef MAP

	return 0;
}

#undef main

int main(int argc, char **argv)
{
	int i;
	
	buildkeytranslationtable();

	for (i=0;i<SDLK_LAST;i++) {
		if (i>0) printf(", ");
		if (i%8 == 7) printf("\n");
		printf("%d",keytranslation[i]);
	}
	
	return 0;
}

