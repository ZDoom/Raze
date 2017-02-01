
#if (SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION < 3)
static char keytranslation[SDLK_LAST];
#else
static char keytranslation[SDL_NUM_SCANCODES];
#endif
static int32_t buildkeytranslationtable(void);

#if (SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION < 3) // SDL 1.2
static int32_t buildkeytranslationtable(void)
{
    memset(keytranslation,0,sizeof(keytranslation));

#define MAP(x,y) keytranslation[x] = y
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
#else // if SDL 1.3+
static int32_t buildkeytranslationtable(void)
{
    memset(keytranslation,0,sizeof(keytranslation));

#define MAP(x,y) keytranslation[x] = y
    MAP(SDL_SCANCODE_BACKSPACE,	0xe);
    MAP(SDL_SCANCODE_TAB,		0xf);
    MAP(SDL_SCANCODE_RETURN,	0x1c);
    MAP(SDL_SCANCODE_PAUSE,		0x59);	// 0x1d + 0x45 + 0x9d + 0xc5
    MAP(SDL_SCANCODE_ESCAPE,	0x1);
    MAP(SDL_SCANCODE_SPACE,		0x39);
    MAP(SDL_SCANCODE_COMMA,		0x33);
    MAP(SDL_SCANCODE_NONUSBACKSLASH, 0x33);
    MAP(SDL_SCANCODE_MINUS,		0xc);
    MAP(SDL_SCANCODE_PERIOD,	0x34);
    MAP(SDL_SCANCODE_SLASH,		0x35);
    MAP(SDL_SCANCODE_0,		0xb);
    MAP(SDL_SCANCODE_1,		0x2);
    MAP(SDL_SCANCODE_2,		0x3);
    MAP(SDL_SCANCODE_3,		0x4);
    MAP(SDL_SCANCODE_4,		0x5);
    MAP(SDL_SCANCODE_5,		0x6);
    MAP(SDL_SCANCODE_6,		0x7);
    MAP(SDL_SCANCODE_7,		0x8);
    MAP(SDL_SCANCODE_8,		0x9);
    MAP(SDL_SCANCODE_9,		0xa);
    MAP(SDL_SCANCODE_SEMICOLON,	0x27);
    MAP(SDL_SCANCODE_APOSTROPHE, 0x28);
    MAP(SDL_SCANCODE_EQUALS,	0xd);
    MAP(SDL_SCANCODE_LEFTBRACKET,	0x1a);
    MAP(SDL_SCANCODE_BACKSLASH,	0x2b);
    MAP(SDL_SCANCODE_RIGHTBRACKET,	0x1b);
    MAP(SDL_SCANCODE_A,		0x1e);
    MAP(SDL_SCANCODE_B,		0x30);
    MAP(SDL_SCANCODE_C,		0x2e);
    MAP(SDL_SCANCODE_D,		0x20);
    MAP(SDL_SCANCODE_E,		0x12);
    MAP(SDL_SCANCODE_F,		0x21);
    MAP(SDL_SCANCODE_G,		0x22);
    MAP(SDL_SCANCODE_H,		0x23);
    MAP(SDL_SCANCODE_I,		0x17);
    MAP(SDL_SCANCODE_J,		0x24);
    MAP(SDL_SCANCODE_K,		0x25);
    MAP(SDL_SCANCODE_L,		0x26);
    MAP(SDL_SCANCODE_M,		0x32);
    MAP(SDL_SCANCODE_N,		0x31);
    MAP(SDL_SCANCODE_O,		0x18);
    MAP(SDL_SCANCODE_P,		0x19);
    MAP(SDL_SCANCODE_Q,		0x10);
    MAP(SDL_SCANCODE_R,		0x13);
    MAP(SDL_SCANCODE_S,		0x1f);
    MAP(SDL_SCANCODE_T,		0x14);
    MAP(SDL_SCANCODE_U,		0x16);
    MAP(SDL_SCANCODE_V,		0x2f);
    MAP(SDL_SCANCODE_W,		0x11);
    MAP(SDL_SCANCODE_X,		0x2d);
    MAP(SDL_SCANCODE_Y,		0x15);
    MAP(SDL_SCANCODE_Z,		0x2c);
    MAP(SDL_SCANCODE_DELETE,	0xd3);
    MAP(SDL_SCANCODE_KP_0,		0x52);
    MAP(SDL_SCANCODE_KP_1,		0x4f);
    MAP(SDL_SCANCODE_KP_2,		0x50);
    MAP(SDL_SCANCODE_KP_3,		0x51);
    MAP(SDL_SCANCODE_KP_4,		0x4b);
    MAP(SDL_SCANCODE_KP_5,		0x4c);
    MAP(SDL_SCANCODE_KP_6,		0x4d);
    MAP(SDL_SCANCODE_KP_7,		0x47);
    MAP(SDL_SCANCODE_KP_8,		0x48);
    MAP(SDL_SCANCODE_KP_9,		0x49);
    MAP(SDL_SCANCODE_KP_PERIOD,	0x53);
    MAP(SDL_SCANCODE_KP_DIVIDE,	0xb5);
    MAP(SDL_SCANCODE_KP_MULTIPLY,	0x37);
    MAP(SDL_SCANCODE_KP_MINUS,	0x4a);
    MAP(SDL_SCANCODE_KP_PLUS,	0x4e);
    MAP(SDL_SCANCODE_KP_ENTER,	0x9c);
    //MAP(SDL_SCANCODE_KP_EQUALS,	);
    MAP(SDL_SCANCODE_UP,		0xc8);
    MAP(SDL_SCANCODE_DOWN,		0xd0);
    MAP(SDL_SCANCODE_RIGHT,		0xcd);
    MAP(SDL_SCANCODE_LEFT,		0xcb);
    MAP(SDL_SCANCODE_INSERT,	0xd2);
    MAP(SDL_SCANCODE_HOME,		0xc7);
    MAP(SDL_SCANCODE_END,		0xcf);
    MAP(SDL_SCANCODE_PAGEUP,	0xc9);
    MAP(SDL_SCANCODE_PAGEDOWN,	0xd1);
    MAP(SDL_SCANCODE_F1,		0x3b);
    MAP(SDL_SCANCODE_F2,		0x3c);
    MAP(SDL_SCANCODE_F3,		0x3d);
    MAP(SDL_SCANCODE_F4,		0x3e);
    MAP(SDL_SCANCODE_F5,		0x3f);
    MAP(SDL_SCANCODE_F6,		0x40);
    MAP(SDL_SCANCODE_F7,		0x41);
    MAP(SDL_SCANCODE_F8,		0x42);
    MAP(SDL_SCANCODE_F9,		0x43);
    MAP(SDL_SCANCODE_F10,		0x44);
    MAP(SDL_SCANCODE_F11,		0x57);
    MAP(SDL_SCANCODE_F12,		0x58);
    MAP(SDL_SCANCODE_NUMLOCKCLEAR,	0x45);
    MAP(SDL_SCANCODE_CAPSLOCK,	0x3a);
    MAP(SDL_SCANCODE_SCROLLLOCK,	0x46);
    MAP(SDL_SCANCODE_RSHIFT,	0x36);
    MAP(SDL_SCANCODE_LSHIFT,	0x2a);
    MAP(SDL_SCANCODE_RCTRL,		0x9d);
    MAP(SDL_SCANCODE_LCTRL,		0x1d);
    MAP(SDL_SCANCODE_RALT,		0xb8);
    MAP(SDL_SCANCODE_LALT,		0x38);
    MAP(SDL_SCANCODE_LGUI,	0xdb);	// win l
    MAP(SDL_SCANCODE_RGUI,	0xdc);	// win r
    MAP(SDL_SCANCODE_PRINTSCREEN,		-2);	// 0xaa + 0xb7
    MAP(SDL_SCANCODE_SYSREQ,	0x54);	// alt+printscr
//    MAP(SDL_SCANCODE_PAUSE,		0xb7);	// ctrl+pause
    MAP(SDL_SCANCODE_MENU,		0xdd);	// win menu?
    MAP(SDL_SCANCODE_GRAVE,     0x29);  // tilde
#undef MAP

    return 0;
}
#endif
