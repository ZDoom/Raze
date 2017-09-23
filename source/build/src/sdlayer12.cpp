// SDL 1.2 compatibility.

#include <SDL/SDL_events.h>

#ifdef _WIN32
HWND win_gethwnd(void)
{
    struct SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);

    if (SDL_GetWMInfo(&wmInfo) != 1)
    {
        initprintf("win_gethwnd: SDL_GetWindowWMInfo() failed: %s\n", SDL_GetError());
        return 0;
    }

    return wmInfo.window;
}
#endif

int32_t setvsync(int32_t newSync)
{
    if (vsync_renderlayer == newSync)
        return newSync;

    vsync_renderlayer = newSync;

    resetvideomode();
    if (setgamemode(fullscreen, xdim, ydim, bpp))
        OSD_Printf("restartvid: Reset failed...\n");

    return newSync;
}

int32_t sdlayer_checkversion(void)
{
    const SDL_version *linked = SDL_Linked_Version();
    SDL_version compiled;

    SDL_VERSION(&compiled);

    initprintf("Initializing SDL system interface "
               "(compiled against SDL version %d.%d.%d, found version %d.%d.%d)\n",
               compiled.major, compiled.minor, compiled.patch, linked->major, linked->minor, linked->patch);

    if (SDL_VERSIONNUM(linked->major, linked->minor, linked->patch) < SDL_REQUIREDVERSION)
    {
        /*reject running under SDL versions older than what is stated in sdl_inc.h */
        initprintf("You need at least v%d.%d.%d of SDL to run this game\n", SDL_MIN_X, SDL_MIN_Y, SDL_MIN_Z);
        return -1;
    }

    return 0;
}

//
// initsystem() -- init SDL systems
//
int32_t initsystem(void)
{
#if defined NOSDLPARACHUTE
    const int sdlinitflags = SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE;
#else
    const int sdlinitflags = SDL_INIT_VIDEO;
#endif

    mutex_init(&m_initprintf);

#ifdef _WIN32
    win_init();
#endif

    if (sdlayer_checkversion())
        return -1;

    if (SDL_Init(sdlinitflags))
    {
        initprintf("Initialization failed! (%s)\nNon-interactive mode enabled\n", SDL_GetError());
        novideo = 1;
#ifdef USE_OPENGL
        nogl = 1;
#endif
    }

    atexit(uninitsystem);

    frameplace = 0;
    lockcount = 0;


    if (!novideo)
    {
        char drvname[32];

#ifdef USE_OPENGL
        if (loadgldriver(getenv("BUILD_GLDRV")))
        {
            initprintf("Failed loading OpenGL driver. GL modes will be unavailable.\n");
            nogl = 1;
        }
#endif

        if (SDL_VideoDriverName(drvname, 32))
            initprintf("Using \"%s\" video driver\n", drvname);

        wm_setapptitle(apptitle);
    }

#if defined GEKKO
    SDL_ShowCursor(SDL_DISABLE);
#endif

    return 0;
}

#ifdef GEKKO
static const char *joynames[3][15] = {
        {
            "Left Stick X", "Left Stick Y", "Right Stick X", "Right Stick Y", "Axis 5", "Axis 6", "Axis 7", "Axis 8",
            "Axis 9", "Axis 10", "Axis 11", "Axis 12", "Axis 13", "Axis 14", "Axis 15",
        },
        {
            "Button A", "Button B", "Button 1", "Button 2", "Button -", "Button +", "Button HOME", "Button Z", "Button C",
            "Button X", "Button Y", "Trigger L", "Trigger R", "Trigger ZL", "Trigger ZR",
        },
        {
            "D-Pad Up", "D-Pad Right", "D-Pad Down", "D-Pad Left", "Hat 5", "Hat 6", "Hat 7", "Hat 8", "Hat 9", "Hat 10",
            "Hat 11", "Hat 12", "Hat 13", "Hat 14", "Hat 15",
        }
};
const char *getjoyname(int32_t what, int32_t num)
{
    switch (what)
    {
        case 0:	// axis
            if ((unsigned)num > (unsigned)joynumaxes) return NULL;
            return joynames[0][num];

        case 1: // button
            if ((unsigned)num > (unsigned)joynumbuttons) return NULL;
            return joynames[1][num];

        case 2: // hat
            if ((unsigned)num > (unsigned)joynumhats) return NULL;
            return joynames[2][num];

        default:
            return NULL;
    }
}
#endif

//
// grabmouse_low() -- show/hide mouse cursor, lower level (doesn't check state).
//                    furthermore return 0 if successful.
//

static inline char grabmouse_low(char a)
{
#if !defined GEKKO
    SDL_ShowCursor(a ? SDL_DISABLE : SDL_ENABLE);
    return (SDL_WM_GrabInput(a ? SDL_GRAB_ON : SDL_GRAB_OFF) != (a ? SDL_GRAB_ON : SDL_GRAB_OFF));
#else
    UNREFERENCED_PARAMETER(a);
    return 0;
#endif
}

// high-resolution timers for profiling
uint64_t getu64ticks(void)
{
# if defined _WIN32
    return win_getu64ticks();
# elif defined __APPLE__
    return mach_absolute_time();
# elif _POSIX_TIMERS>0 && defined _POSIX_MONOTONIC_CLOCK
    // This is SDL HG's SDL_GetPerformanceCounter() when clock_gettime() is
    // available.
    uint64_t ticks;
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC, &now);
    ticks = now.tv_sec;
    ticks *= 1000000000;
    ticks += now.tv_nsec;
    return ticks;
# elif defined GEKKO
    return ticks_to_nanosecs(gettime());
# else
    // Blar. This pragma is unsupported on earlier GCC versions.
    // At least we'll get a warning and a reference to this line...
#  pragma message "Using low-resolution (1ms) timer for getu64ticks. Profiling will work badly."
    return SDL_GetTicks();
# endif
}

uint64_t getu64tickspersec(void)
{
# if defined _WIN32
    return win_timerfreq;
# elif defined __APPLE__
    static mach_timebase_info_data_t ti;
    if (ti.denom == 0)
        (void) mach_timebase_info(&ti);  // ti.numer/ti.denom: nsec/(m_a_t() tick)
    return (1000000000LL*ti.denom)/ti.numer;
# elif _POSIX_TIMERS>0 && defined _POSIX_MONOTONIC_CLOCK
    return 1000000000;
# elif defined GEKKO
    return TB_NSPERSEC;
# else
    return 1000;
# endif
}

void getvalidmodes(void)
{
    int32_t i, maxx = 0, maxy = 0;
    int32_t j;
    static int32_t cdepths[] = { 8,
#ifdef USE_OPENGL
                                 16, 24, 32,
#endif
                                 0 };
    SDL_Rect **modes;
    SDL_PixelFormat pf;

    pf.palette = NULL;
    pf.BitsPerPixel = 8;
    pf.BytesPerPixel = 1;

    if (modeschecked || novideo)
        return;

    validmodecnt = 0;
    //    initprintf("Detecting video modes:\n");

    // do fullscreen modes first
    for (j = 0; cdepths[j]; j++)
    {
#ifdef USE_OPENGL
        if (nogl && cdepths[j] > 8)
            continue;
#endif
        pf.BitsPerPixel = cdepths[j];
        pf.BytesPerPixel = cdepths[j] >> 3;

#if !defined SDL_DISABLE_8BIT_BUFFER
        // We convert paletted contents to non-paletted
        modes = SDL_ListModes((cdepths[j] == 8) ? NULL : &pf, SURFACE_FLAGS | SDL_FULLSCREEN);
#else
        modes = SDL_ListModes(&pf, SURFACE_FLAGS | SDL_FULLSCREEN);
#endif

        if (modes == (SDL_Rect **)0)
        {
            if (cdepths[j] > 8)
                cdepths[j] = -1;
            continue;
        }

        if (modes == (SDL_Rect **)-1)
        {
            for (i = 0; defaultres[i][0]; i++)
                SDL_ADDMODE(defaultres[i][0], defaultres[i][1], cdepths[j], 1);
        }
        else
        {
            for (i = 0; modes[i]; i++)
            {
                if (!SDL_CHECKMODE(modes[i]->w, modes[i]->h))
                    continue;

                SDL_ADDMODE(modes[i]->w, modes[i]->h, cdepths[j], 1);

                if ((modes[i]->w > maxx) || (modes[i]->h > maxy))
                {
                    maxx = modes[i]->w;
                    maxy = modes[i]->h;
                }
            }
        }
    }

    SDL_CHECKFSMODES(maxx, maxy);

    // add windowed modes next
    for (j = 0; cdepths[j]; j++)
    {
#ifdef USE_OPENGL
        if (nogl && cdepths[j] > 8)
            continue;
#endif
        if (cdepths[j] < 0)
            continue;

        for (i = 0; defaultres[i][0]; i++)
        {
            if (!SDL_CHECKMODE(defaultres[i][0], defaultres[i][1]))
                continue;

            SDL_ADDMODE(defaultres[i][0], defaultres[i][1], cdepths[j], 0);
        }
    }

    qsort((void *)validmode, validmodecnt, sizeof(struct validmode_t), &sortmodes);

    modeschecked = 1;
}

//
// setvideomode() -- set SDL video mode
//
int32_t setvideomode(int32_t x, int32_t y, int32_t c, int32_t fs)
{
    int32_t regrab = 0, ret;
#ifdef USE_OPENGL
    static int32_t ovsync = 1;
#endif

    ret = setvideomode_sdlcommon(&x, &y, c, fs, &regrab);
    if (ret != 1) return ret;

    // restore gamma before we change video modes if it was changed
    if (sdl_surface && gammabrightness)
    {
        SDL_SetGammaRamp(sysgamma[0], sysgamma[1], sysgamma[2]);
        gammabrightness = 0;  // redetect on next mode switch
    }

    // deinit
    destroy_window_resources();

    initprintf("Setting video mode %dx%d (%d-bpp %s)\n", x, y, c, ((fs & 1) ? "fullscreen" : "windowed"));

#ifdef USE_OPENGL
    if (c > 8)
    {
        int32_t i, j, multisamplecheck = (glmultisample > 0);

        if (nogl)
            return -1;

# ifdef _WIN32
        win_setvideomode(c);
# endif

        struct glattribs
        {
            SDL_GLattr attr;
            int32_t value;
        } sdlayer_gl_attributes [] =
        {
            { SDL_GL_DOUBLEBUFFER, 1 },
            { SDL_GL_MULTISAMPLEBUFFERS, glmultisample > 0 },
            { SDL_GL_MULTISAMPLESAMPLES, glmultisample },
            { SDL_GL_STENCIL_SIZE, 1 },
            { SDL_GL_ACCELERATED_VISUAL, 1 },
            { SDL_GL_SWAP_CONTROL, vsync_renderlayer },
        };

        do
        {
            SDL_GL_ATTRIBUTES(i, sdlayer_gl_attributes);

            /* HACK: changing SDL GL attribs only works before surface creation,
               so we have to create a new surface in a different format first
               to force the surface we WANT to be recreated instead of reused. */
            if (vsync_renderlayer != ovsync)
            {
                if (sdl_surface)
                {
                    SDL_FreeSurface(sdl_surface);
                    sdl_surface =
                    SDL_SetVideoMode(1, 1, 8, SDL_NOFRAME | SURFACE_FLAGS | ((fs & 1) ? SDL_FULLSCREEN : 0));
                    SDL_FreeSurface(sdl_surface);
                }
                ovsync = vsync_renderlayer;
            }
            sdl_surface = SDL_SetVideoMode(x, y, c, SDL_OPENGL | ((fs & 1) ? SDL_FULLSCREEN : 0));
            if (!sdl_surface)
            {
                if (multisamplecheck)
                {
                    initprintf("Multisample mode not possible. Retrying without multisampling.\n");
                    glmultisample = 0;
                    continue;
                }
                initprintf("Unable to set video mode!\n");
                return -1;
            }

#ifdef _WIN32
            loadglextensions();
#endif
        } while (multisamplecheck--);
    }
    else
#endif  // defined USE_OPENGL
    {
#if !defined SDL_DISABLE_8BIT_BUFFER
        // We convert paletted contents to non-paletted
        sdl_surface = SDL_SetVideoMode(x, y, 0, SURFACE_FLAGS | ((fs & 1) ? SDL_FULLSCREEN : 0));
#else
        sdl_surface = SDL_SetVideoMode(x, y, c, SURFACE_FLAGS | ((fs & 1) ? SDL_FULLSCREEN : 0));
#endif
        if (!sdl_surface)
        {
            initprintf("Unable to set video mode!\n");
            return -1;
        }
#if !defined SDL_DISABLE_8BIT_BUFFER
        sdl_buffersurface = SDL_CreateRGBSurface(SURFACE_FLAGS, x, y, c, 0, 0, 0, 0);
        if (!sdl_buffersurface)
        {
            initprintf("Unable to set video mode: SDL_CreateRGBSurface failed: %s\n", SDL_GetError());
            return -1;
        }
#endif
    }

    setvideomode_sdlcommonpost(x, y, c, fs, regrab);

    return 0;
}

//
// showframe() -- update the display
//
void showframe(int32_t w)
{
    UNREFERENCED_PARAMETER(w);

#ifdef USE_OPENGL
    if (bpp > 8)
    {
        if (palfadedelta)
            fullscreen_tint_gl(palfadergb.r, palfadergb.g, palfadergb.b, palfadedelta);

        SDL_GL_SwapBuffers();
        return;
    }
#endif

    if (offscreenrendering) return;

    if (lockcount)
    {
        printf("Frame still locked %d times when showframe() called.\n", lockcount);
        while (lockcount) enddrawing();
    }

    // deferred palette updating
    if (needpalupdate)
    {
        SDL_SetColors(sdl_buffersurface, sdlayer_pal, 0, 256);
        needpalupdate = 0;
    }

#if !defined SDL_DISABLE_8BIT_BUFFER
    SDL_BlitSurface(sdl_buffersurface, NULL, sdl_surface, NULL);
#endif
    SDL_Flip(sdl_surface);
}


// SDL 1.2 specific event handling
int32_t handleevents_pollsdl(void)
{
    int32_t code, rv = 0, j;
    SDL_Event ev;

    while (SDL_PollEvent(&ev))
    {
        switch (ev.type)
        {
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                code = keytranslation[ev.key.keysym.sym];
#ifdef KEY_PRINT_DEBUG
                printf("keytranslation[%d] = %s (%d)  %s\n", ev.key.keysym.sym, key_names[code], code,
                       ev.key.type == SDL_KEYDOWN ? "DOWN" : "UP");
#endif
                if (code != OSD_OSDKey() && ev.key.keysym.unicode != 0 && ev.key.type == SDL_KEYDOWN &&
                    (ev.key.keysym.unicode & 0xff80) == 0 && !keyascfifo_isfull())
                {
                    if (OSD_HandleChar(ev.key.keysym.unicode & 0x7f))
                        keyascfifo_insert(ev.key.keysym.unicode & 0x7f);
                }

                // hook in the osd
                if ((j = OSD_HandleScanCode(code, (ev.key.type == SDL_KEYDOWN))) <= 0)
                {
                    if (j == -1)  // osdkey
                    {
                        for (j = 0; j < KEYSTATUSSIZ; ++j)
                        {
                            if (GetKey(j))
                            {
                                SetKey(j, 0);
                                if (keypresscallback)
                                    keypresscallback(j, 0);
                            }
                        }
                    }
                    break;
                }

                if (ev.key.type == SDL_KEYDOWN)
                {
                    if (!GetKey(code))
                    {
                        SetKey(code, 1);
                        if (keypresscallback)
                            keypresscallback(code, 1);
                    }
                }
                else
                {
#ifdef __linux
                    if (code == 0x59)  // pause
                        break;
#endif
                    SetKey(code, 0);
                    if (keypresscallback)
                        keypresscallback(code, 0);
                }
                break;

            case SDL_ACTIVEEVENT:
                if (ev.active.state & SDL_APPINPUTFOCUS)
                {
                    appactive = ev.active.gain;
                    if (mousegrab && moustat)
                        grabmouse_low(!!appactive);
# ifdef _WIN32
                    Win_SetKeyboardLayoutUS(appactive);

                    if (backgroundidle)
                        SetPriorityClass(GetCurrentProcess(),
                                         appactive ? NORMAL_PRIORITY_CLASS : IDLE_PRIORITY_CLASS);
# endif
                    rv = -1;

                    if (ev.active.state & SDL_APPMOUSEFOCUS)
                        mouseinwindow = ev.active.gain;
                }
                break;

            // SDL_MOUSEMOTION needs to fall through to default... this is just GEKKO processing!
            case SDL_MOUSEMOTION:
#ifdef GEKKO
                // check if it's a wiimote pointer pretending to be a mouse
                if (ev.motion.state & SDL_BUTTON_X2MASK)
                {
                    // the absolute values are used to draw the crosshair
                    mouseabs.x = ev.motion.x;
                    mouseabs.y = ev.motion.y;
                    // hack: reduce the scale of the "relative" motions
                    // to make it act more like a real mouse
                    ev.motion.xrel /= 16;
                    ev.motion.yrel /= 12;
                }
#endif
            default: // OSD_Printf("Got event (%d)\n", ev.type); break;
                rv = handleevents_sdlcommon(&ev);
                break;
        }
    }

    return rv;
}

// from SDL HG, modified
int32_t SDL_WaitEventTimeout(SDL_Event *event, int32_t timeout)
{
    uint32_t expiration = 0;

    if (timeout > 0)
        expiration = SDL_GetTicks() + timeout;

    for (;;)
    {
        SDL_PumpEvents();
        switch (SDL_PeepEvents(event, 1, SDL_GETEVENT, ~0))  // SDL_FIRSTEVENT, SDL_LASTEVENT)) {
        {
            case -1: return 0;
            case 1: return 1;
            case 0:
                if (timeout == 0)
                {
                    /* Polling and no events, just return */
                    return 0;
                }
                if (timeout > 0 && ((int32_t)(SDL_GetTicks() - expiration) >= 0))
                {
                    /* Timeout expired and no events */
                    return 0;
                }
                SDL_Delay(10);
                break;
        }
    }
}
