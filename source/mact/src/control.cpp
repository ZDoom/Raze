/*
 * control.c
 * MACT library controller handling
 *
 * Derived from MACT386.LIB disassembly by Jonathon Fowler
 *
 */

#include "compat.h"

#include "keyboard.h"
#include "mouse.h"
#include "joystick.h"
#include "control.h"
#include "_control.h"

#include "baselayer.h"
#include "osd.h"
#include "pragmas.h"

#ifdef __ANDROID__
#include "android.h"
#endif

int32_t  CONTROL_JoyPresent      = FALSE;
int32_t  CONTROL_JoystickEnabled = FALSE;
int32_t  CONTROL_MousePresent    = FALSE;
int32_t  CONTROL_MouseEnabled    = FALSE;
uint64_t CONTROL_ButtonState     = 0;
uint64_t CONTROL_ButtonHeldState = 0;

// static int32_t CONTROL_UserInputDelay = -1;
float          CONTROL_MouseSensitivity = DEFAULTMOUSESENSITIVITY;
static int32_t CONTROL_NumMouseButtons  = 0;
static int32_t CONTROL_NumMouseAxes     = 0;
static int32_t CONTROL_NumJoyButtons    = 0;
static int32_t CONTROL_NumJoyAxes       = 0;

static controlflags      CONTROL_Flags[CONTROL_NUM_FLAGS];
static controlbuttontype CONTROL_MouseButtonMapping[MAXMOUSEBUTTONS];
static controlbuttontype CONTROL_JoyButtonMapping[MAXJOYBUTTONS];

// static controlkeymaptype  CONTROL_KeyMapping[CONTROL_NUM_FLAGS];
static controlaxismaptype CONTROL_MouseAxesMap[MAXMOUSEAXES];  // maps physical axes onto virtual ones
static controlaxismaptype CONTROL_JoyAxesMap[MAXJOYAXES];

static controlaxistype CONTROL_MouseAxes[MAXMOUSEAXES];  // physical axes
static controlaxistype CONTROL_JoyAxes[MAXJOYAXES];
static controlaxistype CONTROL_LastMouseAxes[MAXMOUSEAXES];
static controlaxistype CONTROL_LastJoyAxes[MAXJOYAXES];

static int32_t CONTROL_MouseAxesScale[MAXMOUSEAXES];
static int32_t CONTROL_JoyAxesScale[MAXJOYAXES];

#ifndef __ANDROID__
static int32_t CONTROL_MouseButtonState[MAXMOUSEBUTTONS];
static int32_t CONTROL_MouseButtonClickedTime[MAXMOUSEBUTTONS];
static int32_t CONTROL_MouseButtonClickedState[MAXMOUSEBUTTONS];
static int32_t CONTROL_MouseButtonClicked[MAXMOUSEBUTTONS];
static uint8_t CONTROL_MouseButtonClickedCount[MAXMOUSEBUTTONS];

static int32_t CONTROL_JoyButtonState[MAXJOYBUTTONS];
static int32_t CONTROL_JoyButtonClickedTime[MAXJOYBUTTONS];
static int32_t CONTROL_JoyButtonClickedState[MAXJOYBUTTONS];
static int32_t CONTROL_JoyButtonClicked[MAXJOYBUTTONS];
static uint8_t CONTROL_JoyButtonClickedCount[MAXJOYBUTTONS];
#endif

static int32_t(*ExtGetTime)(void);
int32_t CONTROL_Started = FALSE;
//static int32_t ticrate;
static int32_t CONTROL_DoubleClickSpeed;

int32_t CONTROL_OSDInput[CONTROL_NUM_FLAGS];
keybind CONTROL_KeyBinds[MAXBOUNDKEYS + MAXMOUSEBUTTONS];
int32_t CONTROL_BindsEnabled = 0;
int32_t CONTROL_SmoothMouse  = 0;

#define CONTROL_CheckRange(which) ((unsigned)which >= (unsigned)CONTROL_NUM_FLAGS)
#define BIND(x, s, r, k) do { Bfree(x.cmdstr); x.cmdstr = s; x.repeat = r; x.key = k; } while (0)

void CONTROL_ClearAllBinds(void)
{
    int32_t i;
    for (i=0; i<MAXBOUNDKEYS; i++)
        CONTROL_FreeKeyBind(i);
    for (i=0; i<MAXMOUSEBUTTONS; i++)
        CONTROL_FreeMouseBind(i);
}

void CONTROL_BindKey(int i, char const * const cmd, int repeat, char const * const keyname)
{
    BIND(CONTROL_KeyBinds[i], Bstrdup(cmd), repeat, keyname);
}

void CONTROL_BindMouse(int i, char const * const cmd, int repeat, char const * const keyname)
{
    BIND(CONTROL_KeyBinds[MAXBOUNDKEYS + i], Bstrdup(cmd), repeat, keyname);
}

void CONTROL_FreeKeyBind(int i)
{
    BIND(CONTROL_KeyBinds[i], NULL, 0, NULL);
}

void CONTROL_FreeMouseBind(int i)
{
    BIND(CONTROL_KeyBinds[MAXBOUNDKEYS + i], NULL, 0, NULL);
}

#ifndef __ANDROID__
static void CONTROL_GetMouseDelta(void)
{
    int32_t x,y;

    MOUSE_GetDelta(&x, &y);

    if (CONTROL_SmoothMouse)
    {
        static int32_t lastx = 0, lasty = 0;

        CONTROL_MouseAxes[0].analog = (int32_t)(((x + lastx) / 2.0f) * 4.0f * CONTROL_MouseSensitivity);
        CONTROL_MouseAxes[1].analog = (int32_t)((((y + lasty) / 2.0f) * 4.0f * CONTROL_MouseSensitivity) * 2.0f);
        lastx = x;
        lasty = y;
        return;
    }

    CONTROL_MouseAxes[0].analog = (int32_t)(x * 4.0f * CONTROL_MouseSensitivity);
    CONTROL_MouseAxes[1].analog = (int32_t)((y * 4.0f * CONTROL_MouseSensitivity) * 2.0f);
}
#endif

static int32_t CONTROL_GetTime(void)
{
    static int32_t t = 0;
    t += 5;
    return t;
}

#ifndef __ANDROID__
static void CONTROL_SetFlag(int32_t which, int32_t active)
{
    if (CONTROL_CheckRange(which)) return;

    controlflags &flags = CONTROL_Flags[which];

    if (flags.toggle == INSTANT_ONOFF)
        flags.active = active;
    else if (active)
        flags.buttonheld = FALSE;
    else if (flags.buttonheld == FALSE)
    {
        flags.buttonheld = TRUE;
        flags.active = (flags.active ? FALSE : TRUE);
    }
}
#endif

#if 0
int32_t CONTROL_KeyboardFunctionPressed(int32_t which)
{
    int32_t key1 = 0, key2 = 0;

    if (CONTROL_CheckRange(which)) return FALSE;

    if (!CONTROL_Flags[which].used) return FALSE;

    if (CONTROL_KeyMapping[which].key1 != KEYUNDEFINED && !KeyBindings[CONTROL_KeyMapping[which].key1].cmdstr)
        key1 = KB_KeyDown[ CONTROL_KeyMapping[which].key1 ] ? TRUE : FALSE;

    if (CONTROL_KeyMapping[which].key2 != KEYUNDEFINED && !KeyBindings[CONTROL_KeyMapping[which].key2].cmdstr)
        key2 = KB_KeyDown[ CONTROL_KeyMapping[which].key2 ] ? TRUE : FALSE;

    return key1 | key2;
}

void CONTROL_ClearKeyboardFunction(int32_t which)
{
    if (CONTROL_CheckRange(which)) return;

    if (!CONTROL_Flags[which].used) return;

    if (CONTROL_KeyMapping[which].key1 != KEYUNDEFINED)
        KB_KeyDown[ CONTROL_KeyMapping[which].key1 ] = 0;

    if (CONTROL_KeyMapping[which].key2 != KEYUNDEFINED)
        KB_KeyDown[ CONTROL_KeyMapping[which].key2 ] = 0;
}
#endif

void CONTROL_DefineFlag(int which, int toggle)
{
    if (CONTROL_CheckRange(which)) return;

    controlflags &flags = CONTROL_Flags[which];

    flags.active     = FALSE;
    flags.used       = TRUE;
    flags.toggle     = toggle;
    flags.buttonheld = FALSE;
    flags.cleared    = 0;
}

int CONTROL_FlagActive(int which)
{
    if (CONTROL_CheckRange(which)) return FALSE;

    return CONTROL_Flags[which].used;
}

#if 0
void CONTROL_MapKey(int32_t which, kb_scancode key1, kb_scancode key2)
{
    if (CONTROL_CheckRange(which)) return;

    CONTROL_KeyMapping[which].key1 = key1 ? key1 : KEYUNDEFINED;
    CONTROL_KeyMapping[which].key2 = key2 ? key2 : KEYUNDEFINED;
}

void CONTROL_PrintKeyMap(void)
{
    int32_t i;

    for (i=0; i<CONTROL_NUM_FLAGS; i++)
    {
        initprintf("function %2d key1=%3x key2=%3x\n",
                   i, CONTROL_KeyMapping[i].key1, CONTROL_KeyMapping[i].key2);
    }
}

void CONTROL_PrintControlFlag(int32_t which)
{
    initprintf("function %2d active=%d used=%d toggle=%d buttonheld=%d cleared=%d\n",
               which, CONTROL_Flags[which].active, CONTROL_Flags[which].used,
               CONTROL_Flags[which].toggle, CONTROL_Flags[which].buttonheld,
               CONTROL_Flags[which].cleared);
}

void CONTROL_PrintAxes(void)
{
    int32_t i;

    initprintf("nummouseaxes=%d\n", CONTROL_NumMouseAxes);
    for (i=0; i<CONTROL_NumMouseAxes; i++)
    {
        initprintf("axis=%d analog=%d digital1=%d digital2=%d\n",
                   i, CONTROL_MouseAxesMap[i].analogmap,
                   CONTROL_MouseAxesMap[i].minmap, CONTROL_MouseAxesMap[i].maxmap);
    }

    initprintf("numjoyaxes=%d\n", CONTROL_NumJoyAxes);
    for (i=0; i<CONTROL_NumJoyAxes; i++)
    {
        initprintf("axis=%d analog=%d digital1=%d digital2=%d\n",
                   i, CONTROL_JoyAxesMap[i].analogmap,
                   CONTROL_JoyAxesMap[i].minmap, CONTROL_JoyAxesMap[i].maxmap);
    }
}
#endif

void CONTROL_MapButton(int whichfunction, int whichbutton, int doubleclicked, controldevice device)
{
    controlbuttontype *set;

    if (CONTROL_CheckRange(whichfunction)) whichfunction = BUTTONUNDEFINED;

    switch (device)
    {
    case controldevice_mouse:
        if ((unsigned)whichbutton >= (unsigned)MAXMOUSEBUTTONS)
        {
            //Error("CONTROL_MapButton: button %d out of valid range for %d mouse buttons.",
            //		whichbutton, CONTROL_NumMouseButtons);
            return;
        }
        set = CONTROL_MouseButtonMapping;
        break;

    case controldevice_joystick:
        if ((unsigned)whichbutton >= (unsigned)MAXJOYBUTTONS)
        {
            //Error("CONTROL_MapButton: button %d out of valid range for %d joystick buttons.",
            //		whichbutton, CONTROL_NumJoyButtons);
            return;
        }
        set = CONTROL_JoyButtonMapping;
        break;

    default:
        //Error("CONTROL_MapButton: invalid controller device type");
        return;
    }

    if (doubleclicked)
        set[whichbutton].doubleclicked = whichfunction;
    else
        set[whichbutton].singleclicked = whichfunction;
}

void CONTROL_MapAnalogAxis(int whichaxis, int whichanalog, controldevice device)
{
    controlaxismaptype *set;

    if ((unsigned)whichanalog >= (unsigned)analog_maxtype)
    {
        //Error("CONTROL_MapAnalogAxis: analog function %d out of valid range for %d analog functions.",
        //		whichanalog, analog_maxtype);
        return;
    }

    switch (device)
    {
    case controldevice_mouse:
        if ((unsigned)whichaxis >= (unsigned)MAXMOUSEAXES)
        {
            //Error("CONTROL_MapAnalogAxis: axis %d out of valid range for %d mouse axes.",
            //		whichaxis, MAXMOUSEAXES);
            return;
        }

        set = CONTROL_MouseAxesMap;
        break;

    case controldevice_joystick:
        if ((unsigned)whichaxis >= (unsigned)MAXJOYAXES)
        {
            //Error("CONTROL_MapAnalogAxis: axis %d out of valid range for %d joystick axes.",
            //		whichaxis, MAXJOYAXES);
            return;
        }

        set = CONTROL_JoyAxesMap;
        break;

    default:
        //Error("CONTROL_MapAnalogAxis: invalid controller device type");
        return;
    }

    set[whichaxis].analogmap = whichanalog;
}

void CONTROL_SetAnalogAxisScale(int32_t whichaxis, int32_t axisscale, controldevice device)
{
    int32_t *set;

    switch (device)
    {
    case controldevice_mouse:
        if ((unsigned) whichaxis >= (unsigned) MAXMOUSEAXES)
        {
            //Error("CONTROL_SetAnalogAxisScale: axis %d out of valid range for %d mouse axes.",
            //		whichaxis, MAXMOUSEAXES);
            return;
        }

        set = CONTROL_MouseAxesScale;
        break;

    case controldevice_joystick:
        if ((unsigned) whichaxis >= (unsigned) MAXJOYAXES)
        {
            //Error("CONTROL_SetAnalogAxisScale: axis %d out of valid range for %d joystick axes.",
            //		whichaxis, MAXJOYAXES);
            return;
        }

        set = CONTROL_JoyAxesScale;
        break;

    default:
        //Error("CONTROL_SetAnalogAxisScale: invalid controller device type");
        return;
    }

    set[whichaxis] = axisscale;
}

void CONTROL_MapDigitalAxis(int32_t whichaxis, int32_t whichfunction, int32_t direction, controldevice device)
{
    controlaxismaptype *set;

    if (CONTROL_CheckRange(whichfunction)) whichfunction = AXISUNDEFINED;

    switch (device)
    {
    case controldevice_mouse:
        if ((unsigned) whichaxis >= (unsigned) MAXMOUSEAXES)
        {
            //Error("CONTROL_MapDigitalAxis: axis %d out of valid range for %d mouse axes.",
            //		whichaxis, MAXMOUSEAXES);
            return;
        }

        set = CONTROL_MouseAxesMap;
        break;

    case controldevice_joystick:
        if ((unsigned) whichaxis >= (unsigned) MAXJOYAXES)
        {
            //Error("CONTROL_MapDigitalAxis: axis %d out of valid range for %d joystick axes.",
            //		whichaxis, MAXJOYAXES);
            return;
        }

        set = CONTROL_JoyAxesMap;
        break;

    default:
        //Error("CONTROL_MapDigitalAxis: invalid controller device type");
        return;
    }

    switch (direction)  	// JBF: this is all very much a guess. The ASM puzzles me.
    {
    case axis_up:
    case axis_left:
        set[whichaxis].minmap = whichfunction;
        break;
    case axis_down:
    case axis_right:
        set[whichaxis].maxmap = whichfunction;
        break;
    default:
        break;
    }
}

void CONTROL_ClearAssignments(void)
{
    int32_t i;

    memset(CONTROL_MouseButtonMapping,  BUTTONUNDEFINED, sizeof(CONTROL_MouseButtonMapping));
    memset(CONTROL_JoyButtonMapping,    BUTTONUNDEFINED, sizeof(CONTROL_JoyButtonMapping));
//    memset(CONTROL_KeyMapping,          KEYUNDEFINED,    sizeof(CONTROL_KeyMapping));
    memset(CONTROL_MouseAxesMap,        AXISUNDEFINED,   sizeof(CONTROL_MouseAxesMap));
    memset(CONTROL_JoyAxesMap,          AXISUNDEFINED,   sizeof(CONTROL_JoyAxesMap));
    memset(CONTROL_MouseAxes,           0,               sizeof(CONTROL_MouseAxes));
    memset(CONTROL_JoyAxes,             0,               sizeof(CONTROL_JoyAxes));
    memset(CONTROL_LastMouseAxes,       0,               sizeof(CONTROL_LastMouseAxes));
    memset(CONTROL_LastJoyAxes,         0,               sizeof(CONTROL_LastJoyAxes));
    for (i=0; i<MAXMOUSEAXES; i++)
        CONTROL_MouseAxesScale[i] = NORMALAXISSCALE;
    for (i=0; i<MAXJOYAXES; i++)
        CONTROL_JoyAxesScale[i] = NORMALAXISSCALE;
}

#ifndef __ANDROID__
static void DoGetDeviceButtons(
    int32_t buttons, int32_t tm,
    int32_t NumButtons,
    int32_t *DeviceButtonState,
    int32_t *ButtonClickedTime,
    int32_t *ButtonClickedState,
    int32_t *ButtonClicked,
    uint8_t *ButtonClickedCount
)
{
    int32_t i=NumButtons-1;

    for (; i>=0; i--)
    {
        int32_t bs = (buttons >> i) & 1;

        DeviceButtonState[i] = bs;
        ButtonClickedState[i] = FALSE;

        if (bs)
        {
            if (ButtonClicked[i] == FALSE)
            {
                ButtonClicked[i] = TRUE;

                if (ButtonClickedCount[i] == 0 || tm > ButtonClickedTime[i])
                {
                    ButtonClickedTime[i] = tm + CONTROL_DoubleClickSpeed;
                    ButtonClickedCount[i] = 1;
                }
                else if (tm < ButtonClickedTime[i])
                {
                    ButtonClickedState[i] = TRUE;
                    ButtonClickedTime[i]  = 0;
                    ButtonClickedCount[i] = 2;
                }
            }
            else if (ButtonClickedCount[i] == 2)
            {
                ButtonClickedState[i] = TRUE;
            }

            continue;
        }

        if (ButtonClickedCount[i] == 2)
            ButtonClickedCount[i] = 0;

        ButtonClicked[i] = FALSE;
    }
}

static void CONTROL_GetDeviceButtons(void)
{
    int32_t t = ExtGetTime();

    if (CONTROL_MouseEnabled)
    {
        DoGetDeviceButtons(
            MOUSE_GetButtons(), t,
            CONTROL_NumMouseButtons,
            CONTROL_MouseButtonState,
            CONTROL_MouseButtonClickedTime,
            CONTROL_MouseButtonClickedState,
            CONTROL_MouseButtonClicked,
            CONTROL_MouseButtonClickedCount
        );
    }

    if (CONTROL_JoystickEnabled)
    {
        DoGetDeviceButtons(
            JOYSTICK_GetButtons(), t,
            CONTROL_NumJoyButtons,
            CONTROL_JoyButtonState,
            CONTROL_JoyButtonClickedTime,
            CONTROL_JoyButtonClickedState,
            CONTROL_JoyButtonClicked,
            CONTROL_JoyButtonClickedCount
        );
    }
}

static void CONTROL_DigitizeAxis(int32_t axis, controldevice device)
{
    controlaxistype *set, *lastset;

    switch (device)
    {
    case controldevice_mouse:
        set = CONTROL_MouseAxes;
        lastset = CONTROL_LastMouseAxes;
        break;

    case controldevice_joystick:
        set = CONTROL_JoyAxes;
        lastset = CONTROL_LastJoyAxes;
        break;

    default: return;
    }

    if (set[axis].analog > 0)
    {
        if (set[axis].analog > THRESHOLD || (set[axis].analog > MINTHRESHOLD && lastset[axis].digital == 1))
            set[axis].digital = 1;
    }
    else
    {
        if (set[axis].analog < -THRESHOLD || (set[axis].analog < -MINTHRESHOLD && lastset[axis].digital == -1))
            set[axis].digital = -1;
    }
}

static void CONTROL_ScaleAxis(int32_t axis, controldevice device)
{
    controlaxistype *set;
    int32_t *scale;

    switch (device)
    {
    case controldevice_mouse:
        set = CONTROL_MouseAxes;
        scale = CONTROL_MouseAxesScale;
        break;

    case controldevice_joystick:
        set = CONTROL_JoyAxes;
        scale = CONTROL_JoyAxesScale;
        break;

    default: return;
    }

    set[axis].analog = mulscale16(set[axis].analog, scale[axis]);
}

static void CONTROL_ApplyAxis(int32_t axis, ControlInfo *info, controldevice device)
{
    controlaxistype *set;
    controlaxismaptype *map;

    switch (device)
    {
    case controldevice_mouse:
        set = CONTROL_MouseAxes;
        map = CONTROL_MouseAxesMap;
        break;

    case controldevice_joystick:
        set = CONTROL_JoyAxes;
        map = CONTROL_JoyAxesMap;
        break;

    default: return;
    }

    switch (map[axis].analogmap)
    {
    case analog_turning:          info->dyaw   += set[axis].analog; break;
    case analog_strafing:         info->dx     += set[axis].analog; break;
    case analog_lookingupanddown: info->dpitch += set[axis].analog; break;
    case analog_elevation:        info->dy     += set[axis].analog; break;
    case analog_rolling:          info->droll  += set[axis].analog; break;
    case analog_moving:           info->dz     += set[axis].analog; break;
    default: break;
    }
}

static void CONTROL_PollDevices(ControlInfo *info)
{
    memset(info, 0, sizeof(ControlInfo));

#ifdef __ANDROID__
    CONTROL_Android_PollDevices(info);
#endif

    if (CONTROL_MouseEnabled)
    {
        int32_t i = MAXMOUSEAXES-1;

        Bmemcpy(CONTROL_LastMouseAxes, CONTROL_MouseAxes, sizeof(CONTROL_MouseAxes));
        memset(CONTROL_MouseAxes, 0, sizeof(CONTROL_MouseAxes));

        CONTROL_GetMouseDelta();
        for (; i>=0; i--)
        {
            CONTROL_DigitizeAxis(i, controldevice_mouse);
            CONTROL_ScaleAxis(i, controldevice_mouse);
            LIMITCONTROL(&CONTROL_MouseAxes[i].analog);
            CONTROL_ApplyAxis(i, info, controldevice_mouse);
        }
    }

    if (CONTROL_JoystickEnabled)
    {
        int32_t i = joynumaxes-1;

        Bmemcpy(CONTROL_LastJoyAxes,   CONTROL_JoyAxes,   sizeof(CONTROL_JoyAxes));
        memset(CONTROL_JoyAxes,   0, sizeof(CONTROL_JoyAxes));

        for (; i>=0; i--)
        {
            CONTROL_JoyAxes[i].analog = joyaxis[i];

            CONTROL_DigitizeAxis(i, controldevice_joystick);
            CONTROL_ScaleAxis(i, controldevice_joystick);
            LIMITCONTROL(&CONTROL_JoyAxes[i].analog);
            CONTROL_ApplyAxis(i, info, controldevice_joystick);
        }
    }

    CONTROL_GetDeviceButtons();
}

static void CONTROL_AxisFunctionState(int32_t *p1)
{
    if (CONTROL_NumMouseAxes)
    {
        int32_t j, i = CONTROL_NumMouseAxes-1;

        do
        {
            if (!CONTROL_MouseAxes[i].digital) continue;

            if (CONTROL_MouseAxes[i].digital < 0)
                j = CONTROL_MouseAxesMap[i].minmap;
            else
                j = CONTROL_MouseAxesMap[i].maxmap;

            if (j != AXISUNDEFINED)
                p1[j] = 1;
        }
        while (i--);
    }

    if (CONTROL_NumJoyAxes)
    {
        int32_t j, i = CONTROL_NumJoyAxes-1;

        do
        {
            if (!CONTROL_JoyAxes[i].digital) continue;

            if (CONTROL_JoyAxes[i].digital < 0)
                j = CONTROL_JoyAxesMap[i].minmap;
            else
                j = CONTROL_JoyAxesMap[i].maxmap;

            if (j != AXISUNDEFINED)
                p1[j] = 1;
        }
        while (i--);
    }
}

static void CONTROL_ButtonFunctionState(int32_t *p1)
{
    if (CONTROL_NumMouseButtons)
    {
        int32_t i = CONTROL_NumMouseButtons-1, j;

        do
        {
            if (!CONTROL_KeyBinds[MAXBOUNDKEYS + i].cmdstr)
            {
                j = CONTROL_MouseButtonMapping[i].doubleclicked;
                if (j != KEYUNDEFINED)
                    p1[j] |= CONTROL_MouseButtonClickedState[i];

                j = CONTROL_MouseButtonMapping[i].singleclicked;
                if (j != KEYUNDEFINED)
                    p1[j] |= CONTROL_MouseButtonState[i];
            }

            if (!CONTROL_BindsEnabled)
                continue;

            if (CONTROL_KeyBinds[MAXBOUNDKEYS + i].cmdstr && CONTROL_MouseButtonState[i])
            {
                if (CONTROL_KeyBinds[MAXBOUNDKEYS + i].repeat || (CONTROL_KeyBinds[MAXBOUNDKEYS + i].laststate == 0))
                    OSD_Dispatch(CONTROL_KeyBinds[MAXBOUNDKEYS + i].cmdstr);
            }
            CONTROL_KeyBinds[MAXBOUNDKEYS + i].laststate = CONTROL_MouseButtonState[i];
        }
        while (i--);
    }

    if (CONTROL_NumJoyButtons)
    {
        int32_t i=CONTROL_NumJoyButtons-1, j;

        do
        {
            j = CONTROL_JoyButtonMapping[i].doubleclicked;
            if (j != KEYUNDEFINED)
                p1[j] |= CONTROL_JoyButtonClickedState[i];

            j = CONTROL_JoyButtonMapping[i].singleclicked;
            if (j != KEYUNDEFINED)
                p1[j] |= CONTROL_JoyButtonState[i];
        }
        while (i--);
    }
}
#endif

void CONTROL_ClearButton(int32_t whichbutton)
{
    if (CONTROL_CheckRange(whichbutton)) return;

#ifdef __ANDROID__
    CONTROL_Android_ClearButton(whichbutton);
#endif

    BUTTONCLEAR(whichbutton);
    CONTROL_Flags[whichbutton].cleared = TRUE;
}

void CONTROL_ProcessBinds(void)
{
    int32_t i=MAXBOUNDKEYS-1;

    if (!CONTROL_BindsEnabled)
        return;

    do
    {
        if (CONTROL_KeyBinds[i].cmdstr)
        {
            if (KB_KeyPressed(i) && (CONTROL_KeyBinds[i].repeat || (CONTROL_KeyBinds[i].laststate == 0)))
                OSD_Dispatch(CONTROL_KeyBinds[i].cmdstr);

            CONTROL_KeyBinds[i].laststate = KB_KeyPressed(i);
        }
    }
    while (i--);
}

#ifndef __ANDROID__
static void CONTROL_GetFunctionInput(void)
{
    int32_t periphs[CONTROL_NUM_FLAGS];
    int32_t i = CONTROL_NUM_FLAGS-1;

    memset(periphs, 0, sizeof(periphs));
    CONTROL_ButtonFunctionState(periphs);
    CONTROL_AxisFunctionState(periphs);

    CONTROL_ButtonHeldState = CONTROL_ButtonState;
    CONTROL_ButtonState = 0;

    do
    {
        CONTROL_SetFlag(i, /*CONTROL_KeyboardFunctionPressed(i) | */periphs[i] | CONTROL_OSDInput[i]);

        if (CONTROL_Flags[i].cleared == FALSE) BUTTONSET(i, CONTROL_Flags[i].active);
        else if (CONTROL_Flags[i].active == FALSE) CONTROL_Flags[i].cleared = 0;
    }
    while (i--);

    memset(CONTROL_OSDInput, 0, sizeof(CONTROL_OSDInput));
}
#endif

void CONTROL_GetInput(ControlInfo *info)
{
#ifdef __ANDROID__
    CONTROL_Android_PollDevices(info);
#else
    CONTROL_PollDevices(info);
    CONTROL_GetFunctionInput();
    inputchecked = 1;
#endif
}

int32_t CONTROL_Startup(controltype which, int32_t(*TimeFunction)(void), int32_t ticspersecond)
{
    int32_t i;

    UNREFERENCED_PARAMETER(which);

    if (CONTROL_Started) return FALSE;

    if (TimeFunction) ExtGetTime = TimeFunction;
    else ExtGetTime = CONTROL_GetTime;

//    ticrate = ticspersecond;

    CONTROL_DoubleClickSpeed = (ticspersecond*57)/100;
    if (CONTROL_DoubleClickSpeed <= 0)
        CONTROL_DoubleClickSpeed = 1;

    if (initinput()) return TRUE;

    CONTROL_MousePresent = CONTROL_MouseEnabled    = FALSE;
    CONTROL_JoyPresent   = CONTROL_JoystickEnabled = FALSE;
    CONTROL_NumMouseButtons = CONTROL_NumJoyButtons = 0;
    CONTROL_NumMouseAxes    = CONTROL_NumJoyAxes    = 0;
    KB_Startup();

    //switch (which) {
    //	case controltype_keyboard:
    //		break;

    //	case controltype_keyboardandmouse:
    CONTROL_NumMouseAxes      = MAXMOUSEAXES;
    CONTROL_NumMouseButtons   = MAXMOUSEBUTTONS;
    CONTROL_MousePresent      = Mouse_Init();
    CONTROL_MouseEnabled      = CONTROL_MousePresent;
    //		break;

    //	case controltype_keyboardandjoystick:
    CONTROL_NumJoyAxes    = min(MAXJOYAXES,joynumaxes);
    CONTROL_NumJoyButtons = min(MAXJOYBUTTONS,joynumbuttons + 4*(joynumhats>0));
    CONTROL_JoystickEnabled = CONTROL_JoyPresent = (inputdevices&4)>>2;
    //		break;
    //}

#ifdef GEKKO
    if (CONTROL_MousePresent)
        initprintf("CONTROL_Startup: Mouse Present\n");
    if (CONTROL_JoyPresent)
        initprintf("CONTROL_Startup: Joystick Present\n");
#endif

    CONTROL_ButtonState     = 0;
    CONTROL_ButtonHeldState = 0;

    for (i=0; i<CONTROL_NUM_FLAGS; i++)
        CONTROL_Flags[i].used = FALSE;

    CONTROL_Started = TRUE;

    return FALSE;
}

void CONTROL_Shutdown(void)
{
    if (!CONTROL_Started)
        return;

    CONTROL_ClearAllBinds();

    MOUSE_Shutdown();
    uninitinput();

    CONTROL_Started = FALSE;
}

