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
#include "control.h"
#include "_control.h"
#include "util_lib.h"

#include "baselayer.h"
#include "osd.h"
#include "pragmas.h"


int32_t CONTROL_JoyPresent = FALSE;
int32_t CONTROL_JoystickEnabled = FALSE;
int32_t CONTROL_MousePresent = FALSE;
int32_t CONTROL_MouseEnabled = FALSE;
uint64  CONTROL_ButtonState = 0;
uint64  CONTROL_ButtonHeldState = 0;

// static int32_t CONTROL_UserInputDelay = -1;
float  CONTROL_MouseSensitivity = DEFAULTMOUSESENSITIVITY;
static int32_t CONTROL_NumMouseButtons = 0;
static int32_t CONTROL_NumMouseAxes = 0;
static int32_t CONTROL_NumJoyButtons = 0;
static int32_t CONTROL_NumJoyAxes = 0;
static controlflags       CONTROL_Flags[CONTROL_NUM_FLAGS];
static controlbuttontype  CONTROL_MouseButtonMapping[MAXMOUSEBUTTONS],
CONTROL_JoyButtonMapping[MAXJOYBUTTONS];
static controlkeymaptype  CONTROL_KeyMapping[CONTROL_NUM_FLAGS];
static controlaxismaptype CONTROL_MouseAxesMap[MAXMOUSEAXES],	// maps physical axes onto virtual ones
CONTROL_JoyAxesMap[MAXJOYAXES];
static controlaxistype    CONTROL_MouseAxes[MAXMOUSEAXES],	// physical axes
CONTROL_JoyAxes[MAXJOYAXES];
static controlaxistype    CONTROL_LastMouseAxes[MAXMOUSEAXES],
CONTROL_LastJoyAxes[MAXJOYAXES];
static int32_t   CONTROL_MouseAxesScale[MAXMOUSEAXES],             CONTROL_JoyAxesScale[MAXJOYAXES];
static int32_t   CONTROL_MouseButtonState[MAXMOUSEBUTTONS],        CONTROL_JoyButtonState[MAXJOYBUTTONS];
static int32_t   CONTROL_MouseButtonClickedTime[MAXMOUSEBUTTONS],  CONTROL_JoyButtonClickedTime[MAXJOYBUTTONS];
static int32_t CONTROL_MouseButtonClickedState[MAXMOUSEBUTTONS], CONTROL_JoyButtonClickedState[MAXJOYBUTTONS];
static int32_t CONTROL_MouseButtonClicked[MAXMOUSEBUTTONS],      CONTROL_JoyButtonClicked[MAXJOYBUTTONS];
static uint8_t CONTROL_MouseButtonClickedCount[MAXMOUSEBUTTONS], CONTROL_JoyButtonClickedCount[MAXJOYBUTTONS];
static int32_t CONTROL_UserInputCleared[3];
static int32_t(*GetTime)(void);
static int32_t CONTROL_Started = FALSE;
static int32_t ticrate;
static int32_t CONTROL_DoubleClickSpeed;

int32_t extinput[CONTROL_NUM_FLAGS];
keybind KeyBindings[MAXBOUNDKEYS], MouseBindings[MAXMOUSEBUTTONS];
int32_t bindsenabled = 0;
int32_t control_smoothmouse = 0;

void CONTROL_GetMouseDelta(void)
{
    int32_t x,y;

    MOUSE_GetDelta(&x, &y);

    if (control_smoothmouse)
    {
        static int32_t lastx = 0, lasty = 0;

        CONTROL_MouseAxes[0].analog = (((x + lastx) / 2.0f) * 4.0f * CONTROL_MouseSensitivity);
        CONTROL_MouseAxes[1].analog = (((y + lasty) / 2.0f) * 4.0f * CONTROL_MouseSensitivity) * 2.0f;
        lastx = x;
        lasty = y;
        return;
    }

    CONTROL_MouseAxes[0].analog = (x * 4.0f * CONTROL_MouseSensitivity);
    CONTROL_MouseAxes[1].analog = (y * 4.0f * CONTROL_MouseSensitivity) * 2.0f;
}

int32_t CONTROL_StartMouse(void)
{
    CONTROL_NumMouseButtons = MAXMOUSEBUTTONS;
    return MOUSE_Init();
}

void CONTROL_GetJoyAbs(void)
{
}

void CONTROL_FilterJoyDelta(void)
{
}

void CONTROL_GetJoyDelta(void)
{
    int32_t i;

    for (i=0; i<joynumaxes; i++)
        CONTROL_JoyAxes[i].analog = joyaxis[i] >> 5;
}

int32_t CONTROL_StartJoy(int32_t joy)
{
    UNREFERENCED_PARAMETER(joy);
    return (inputdevices & 4) == 4;
}

void CONTROL_ShutJoy(int32_t joy)
{
    UNREFERENCED_PARAMETER(joy);
    CONTROL_JoyPresent = FALSE;
}

int32_t CONTROL_GetTime(void)
{
    static int32_t t = 0;
    t += 5;
    return t;
}

static inline int32_t CONTROL_CheckRange(int32_t which)
{
    if ((uint32_t)which >= (uint32_t)CONTROL_NUM_FLAGS) return TRUE;
    //Error("CONTROL_CheckRange: Index %d out of valid range for %d control flags.",
    //	which, CONTROL_NUM_FLAGS);
    return FALSE;
}

void CONTROL_SetFlag(int32_t which, int32_t active)
{
    if (CONTROL_CheckRange(which)) return;

    if (CONTROL_Flags[which].toggle == INSTANT_ONOFF)
    {
        CONTROL_Flags[which].active = active;
        return;
    }
    if (active)
    {
        CONTROL_Flags[which].buttonheld = FALSE;
    }
    else if (CONTROL_Flags[which].buttonheld == FALSE)
    {
        CONTROL_Flags[which].buttonheld = TRUE;
        CONTROL_Flags[which].active = (CONTROL_Flags[which].active ? FALSE : TRUE);
    }
}

int32_t CONTROL_KeyboardFunctionPressed(int32_t which)
{
    int32_t key1 = 0, key2 = 0;

    if (CONTROL_CheckRange(which)) return FALSE;

    if (!CONTROL_Flags[which].used) return FALSE;

    if (CONTROL_KeyMapping[which].key1 != KEYUNDEFINED && !KeyBindings[CONTROL_KeyMapping[which].key1].cmd[0])
        key1 = KB_KeyDown[ CONTROL_KeyMapping[which].key1 ] ? TRUE : FALSE;

    if (CONTROL_KeyMapping[which].key2 != KEYUNDEFINED && !KeyBindings[CONTROL_KeyMapping[which].key2].cmd[0])
        key2 = KB_KeyDown[ CONTROL_KeyMapping[which].key2 ] ? TRUE : FALSE;

    return (key1 | key2);
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

void CONTROL_DefineFlag(int32_t which, int32_t toggle)
{
    if (CONTROL_CheckRange(which)) return;

    CONTROL_Flags[which].active     = FALSE;
    CONTROL_Flags[which].used       = TRUE;
    CONTROL_Flags[which].toggle     = toggle;
    CONTROL_Flags[which].buttonheld = FALSE;
    CONTROL_Flags[which].cleared    = 0;
}

int32_t CONTROL_FlagActive(int32_t which)
{
    if (CONTROL_CheckRange(which)) return FALSE;

    return CONTROL_Flags[which].used;
}

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
        initprintf("function %2ld key1=%3x key2=%3x\n",
                   i, CONTROL_KeyMapping[i].key1, CONTROL_KeyMapping[i].key2);
    }
}

void CONTROL_PrintControlFlag(int32_t which)
{
    initprintf("function %2ld active=%d used=%d toggle=%d buttonheld=%d cleared=%d\n",
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

void CONTROL_MapButton(int32_t whichfunction, int32_t whichbutton, int32_t doubleclicked, controldevice device)
{
    controlbuttontype *set;

    if (CONTROL_CheckRange(whichfunction)) whichfunction = BUTTONUNDEFINED;

    switch (device)
    {
    case controldevice_mouse:
        if ((uint32_t)whichbutton >= (uint32_t)MAXMOUSEBUTTONS)
        {
            //Error("CONTROL_MapButton: button %d out of valid range for %d mouse buttons.",
            //		whichbutton, CONTROL_NumMouseButtons);
            return;
        }
        set = CONTROL_MouseButtonMapping;
        break;

    case controldevice_joystick:
        if ((uint32_t)whichbutton >= (uint32_t)MAXJOYBUTTONS)
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

void CONTROL_MapAnalogAxis(int32_t whichaxis, int32_t whichanalog, controldevice device)
{
    controlaxismaptype *set;

    if ((uint32_t)whichanalog >= (uint32_t)analog_maxtype)
    {
        //Error("CONTROL_MapAnalogAxis: analog function %d out of valid range for %d analog functions.",
        //		whichanalog, analog_maxtype);
        return;
    }

    switch (device)
    {
    case controldevice_mouse:
        if ((uint32_t)whichaxis >= (uint32_t)MAXMOUSEAXES)
        {
            //Error("CONTROL_MapAnalogAxis: axis %d out of valid range for %d mouse axes.",
            //		whichaxis, MAXMOUSEAXES);
            return;
        }

        set = CONTROL_MouseAxesMap;
        break;

    case controldevice_joystick:
        if ((uint32_t)whichaxis >= (uint32_t)MAXJOYAXES)
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
        if ((uint32_t)whichaxis >= (uint32_t)MAXMOUSEAXES)
        {
            //Error("CONTROL_SetAnalogAxisScale: axis %d out of valid range for %d mouse axes.",
            //		whichaxis, MAXMOUSEAXES);
            return;
        }

        set = CONTROL_MouseAxesScale;
        break;

    case controldevice_joystick:
        if ((uint32_t)whichaxis >= (uint32_t)MAXJOYAXES)
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
        if ((uint32_t)whichaxis >= (uint32_t)MAXMOUSEAXES)
        {
            //Error("CONTROL_MapDigitalAxis: axis %d out of valid range for %d mouse axes.",
            //		whichaxis, MAXMOUSEAXES);
            return;
        }

        set = CONTROL_MouseAxesMap;
        break;

    case controldevice_joystick:
        if ((uint32_t)whichaxis >= (uint32_t)MAXJOYAXES)
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
    memset(CONTROL_KeyMapping,          KEYUNDEFINED,    sizeof(CONTROL_KeyMapping));
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
    int32_t i=NumButtons-1, bs;

    for (; i>=0; i--)
    {
        bs = (buttons >> i) & 1;

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

void CONTROL_GetDeviceButtons(void)
{
    int32_t t;

    t = GetTime();

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
        int32_t buttons = joyb;
        if (joynumhats > 0 && joyhat[0] != -1)
        {
            static int32_t hatstate[] = { 1, 1|2, 2, 2|4, 4, 4|8, 8, 8|1 };
            int32_t val;

            // thanks SDL for this much more sensible method
            val = ((joyhat[0] + 4500 / 2) % 36000) / 4500;
            if (val < 8) buttons |= hatstate[val] << min(MAXJOYBUTTONS,joynumbuttons);
        }

        DoGetDeviceButtons(
            buttons, t,
            CONTROL_NumJoyButtons,
            CONTROL_JoyButtonState,
            CONTROL_JoyButtonClickedTime,
            CONTROL_JoyButtonClickedState,
            CONTROL_JoyButtonClicked,
            CONTROL_JoyButtonClickedCount
        );
    }
}

void CONTROL_DigitizeAxis(int32_t axis, controldevice device)
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

void CONTROL_ScaleAxis(int32_t axis, controldevice device)
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

void CONTROL_ApplyAxis(int32_t axis, ControlInfo *info, controldevice device)
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

void CONTROL_PollDevices(ControlInfo *info)
{
    memcpy(CONTROL_LastMouseAxes, CONTROL_MouseAxes, sizeof(CONTROL_MouseAxes));
    memcpy(CONTROL_LastJoyAxes,   CONTROL_JoyAxes,   sizeof(CONTROL_JoyAxes));

    memset(CONTROL_MouseAxes, 0, sizeof(CONTROL_MouseAxes));
    memset(CONTROL_JoyAxes,   0, sizeof(CONTROL_JoyAxes));
    memset(info, 0, sizeof(ControlInfo));

    if (CONTROL_MouseEnabled)
    {
        int32_t i = MAXMOUSEAXES-1;

        CONTROL_GetMouseDelta();
        do
        {
            CONTROL_DigitizeAxis(i, controldevice_mouse);
            CONTROL_ScaleAxis(i, controldevice_mouse);
            LIMITCONTROL(&CONTROL_MouseAxes[i].analog);
            CONTROL_ApplyAxis(i, info, controldevice_mouse);
        }
        while (--i);

        CONTROL_DigitizeAxis(0, controldevice_mouse);
        CONTROL_ScaleAxis(0, controldevice_mouse);
        LIMITCONTROL(&CONTROL_MouseAxes[0].analog);
        CONTROL_ApplyAxis(0, info, controldevice_mouse);
    }

    if (CONTROL_JoystickEnabled)
    {
        int32_t i = MAXJOYAXES-1;

        CONTROL_GetJoyDelta();
        do
        {
            CONTROL_DigitizeAxis(i, controldevice_joystick);
            CONTROL_ScaleAxis(i, controldevice_joystick);
            LIMITCONTROL(&CONTROL_JoyAxes[i].analog);
            CONTROL_ApplyAxis(i, info, controldevice_joystick);
        }
        while (--i);

        CONTROL_DigitizeAxis(0, controldevice_joystick);
        CONTROL_ScaleAxis(0, controldevice_joystick);
        LIMITCONTROL(&CONTROL_JoyAxes[0].analog);
        CONTROL_ApplyAxis(0, info, controldevice_joystick);
    }

    CONTROL_GetDeviceButtons();
}

void CONTROL_AxisFunctionState(int32_t *p1)
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

void CONTROL_ButtonFunctionState(int32_t *p1)
{
    if (CONTROL_NumMouseButtons)
    {
        int32_t i = CONTROL_NumMouseButtons-1, j;

        do
        {
            if (!MouseBindings[i].cmd[0])
            {
                j = CONTROL_MouseButtonMapping[i].doubleclicked;
                if (j != KEYUNDEFINED)
                    p1[j] |= CONTROL_MouseButtonClickedState[i];

                j = CONTROL_MouseButtonMapping[i].singleclicked;
                if (j != KEYUNDEFINED)
                    p1[j] |= CONTROL_MouseButtonState[i];
            }

            if (!bindsenabled)
                continue;

            if (MouseBindings[i].cmd[0] && CONTROL_MouseButtonState[i])
            {
                if (MouseBindings[i].repeat || (MouseBindings[i].laststate == 0))
                    OSD_Dispatch(MouseBindings[i].cmd);
            }
            MouseBindings[i].laststate = CONTROL_MouseButtonState[i];
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
/*
void CONTROL_GetUserInput( UserInput *info )
{
	ControlInfo ci;

	CONTROL_PollDevices( &ci );

	info->dir = dir_None;

	// checks if CONTROL_UserInputDelay is too far in the future due to clock skew?
	if (GetTime() + ((ticrate * USERINPUTDELAY) / 1000) < CONTROL_UserInputDelay)
		CONTROL_UserInputDelay = -1;

	if (GetTime() >= CONTROL_UserInputDelay) {
		if (CONTROL_MouseAxes[1].digital == -1)
			info->dir = dir_North;
		else if (CONTROL_MouseAxes[1].digital == 1)
			info->dir = dir_South;
		else if (CONTROL_MouseAxes[0].digital == -1)
			info->dir = dir_West;
		else if (CONTROL_MouseAxes[0].digital == 1)
			info->dir = dir_East;

		if (CONTROL_JoyAxes[1].digital == -1)
			info->dir = dir_North;
		else if (CONTROL_JoyAxes[1].digital == 1)
			info->dir = dir_South;
		else if (CONTROL_JoyAxes[0].digital == -1)
			info->dir = dir_West;
		else if (CONTROL_JoyAxes[0].digital == 1)
			info->dir = dir_East;
	}

	info->button0 = CONTROL_MouseButtonState[0] | CONTROL_JoyButtonState[0];
	info->button1 = CONTROL_MouseButtonState[1] | CONTROL_JoyButtonState[1];

	if (KB_KeyDown[sc_kpad_8] || KB_KeyDown[sc_UpArrow])
		info->dir = dir_North;
	else if (KB_KeyDown[sc_kpad_2] || KB_KeyDown[sc_DownArrow])
		info->dir = dir_South;
	else if (KB_KeyDown[sc_kpad_4] || KB_KeyDown[sc_LeftArrow])
		info->dir = dir_West;
	else if (KB_KeyDown[sc_kpad_6] || KB_KeyDown[sc_RightArrow])
		info->dir = dir_East;

	if (KB_KeyDown[BUTTON0_SCAN_1] || KB_KeyDown[BUTTON0_SCAN_2] || KB_KeyDown[BUTTON0_SCAN_3])
		info->button0 = 1;
	if (KB_KeyDown[BUTTON1_SCAN])
		info->button1 = 1;

	if (CONTROL_UserInputCleared[1]) {
		if (!info->button0)
			CONTROL_UserInputCleared[1] = false;
		else
			info->button0 = false;
	}
	if (CONTROL_UserInputCleared[2]) {
		if (!info->button1)
			CONTROL_UserInputCleared[2] = false;
		else
			info->button1 = false;
	}
}

void CONTROL_ClearUserInput( UserInput *info )
{
	switch (info->dir) {
		case dir_North:
		case dir_South:
		case dir_East:
		case dir_West:
			CONTROL_UserInputCleared[0] = true;
			CONTROL_UserInputDelay = GetTime() + ((ticrate * USERINPUTDELAY) / 1000);
			switch (info->dir) {
				case dir_North: KB_KeyDown[sc_UpArrow]    = KB_KeyDown[sc_kpad_8] = 0; break;
				case dir_South: KB_KeyDown[sc_DownArrow]  = KB_KeyDown[sc_kpad_2] = 0; break;
				case dir_East:  KB_KeyDown[sc_LeftArrow]  = KB_KeyDown[sc_kpad_4] = 0; break;
				case dir_West:  KB_KeyDown[sc_RightArrow] = KB_KeyDown[sc_kpad_6] = 0; break;
				default: break;
			}
			break;
		default: break;
	}
	if (info->button0) CONTROL_UserInputCleared[1] = true;
	if (info->button1) CONTROL_UserInputCleared[2] = true;
}
*/
void CONTROL_ClearButton(int32_t whichbutton)
{
    if (CONTROL_CheckRange(whichbutton)) return;
    BUTTONCLEAR(whichbutton);
    CONTROL_Flags[whichbutton].cleared = TRUE;
}

void CONTROL_ProcessBinds(void)
{
    if (!bindsenabled) return;

    {
        int32_t i=MAXBOUNDKEYS-1;

        do
        {
            if (KeyBindings[i].cmd[0] && KB_KeyPressed(i))
            {
                if (KeyBindings[i].repeat || (KeyBindings[i].laststate == 0))
                    OSD_Dispatch(KeyBindings[i].cmd);
            }
            KeyBindings[i].laststate = KB_KeyPressed(i);
        }
        while (--i);
    }

    if (KeyBindings[0].cmd[0] && KB_KeyPressed(0))
    {
        if (KeyBindings[0].repeat || (KeyBindings[0].laststate == 0))
            OSD_Dispatch(KeyBindings[0].cmd);
    }
    KeyBindings[0].laststate = KB_KeyPressed(0);
}


void CONTROL_GetInput(ControlInfo *info)
{
    int32_t periphs[CONTROL_NUM_FLAGS];

    CONTROL_PollDevices(info);

    memset(periphs, 0, sizeof(periphs));
    CONTROL_ButtonFunctionState(periphs);
    CONTROL_AxisFunctionState(periphs);

    CONTROL_ButtonHeldState = CONTROL_ButtonState;
    CONTROL_ButtonState = 0;

    CONTROL_ProcessBinds();

    {
        int32_t i = CONTROL_NUM_FLAGS-1;

        do
        {
            CONTROL_SetFlag(i, CONTROL_KeyboardFunctionPressed(i) | periphs[i] | extinput[i]);

            if (CONTROL_Flags[i].cleared == FALSE) BUTTONSET(i, CONTROL_Flags[i].active);
            else if (CONTROL_Flags[i].active == FALSE) CONTROL_Flags[i].cleared = 0;
        }
        while (--i);

        CONTROL_SetFlag(0, CONTROL_KeyboardFunctionPressed(0) | periphs[0] | extinput[0]);
        if (CONTROL_Flags[0].cleared == FALSE) BUTTONSET(0, CONTROL_Flags[0].active);
        else if (CONTROL_Flags[0].active == FALSE) CONTROL_Flags[0].cleared = 0;
    }

    memset(extinput, 0, sizeof(extinput));
}

void CONTROL_WaitRelease(void)
{
}

void CONTROL_Ack(void)
{
}

int32_t CONTROL_Startup(controltype which, int32_t(*TimeFunction)(void), int32_t ticspersecond)
{
    int32_t i;

    UNREFERENCED_PARAMETER(which);

    if (CONTROL_Started) return FALSE;

    if (TimeFunction) GetTime = TimeFunction;
    else GetTime = CONTROL_GetTime;

    ticrate = ticspersecond;

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
    CONTROL_MousePresent      = MOUSE_Init();
    CONTROL_MouseEnabled      = CONTROL_MousePresent;
    //		break;

    //	case controltype_keyboardandjoystick:
    CONTROL_NumJoyAxes    = min(MAXJOYAXES,joynumaxes);
    CONTROL_NumJoyButtons = min(MAXJOYBUTTONS,joynumbuttons + 4*(joynumhats>0));
    CONTROL_JoyPresent    = CONTROL_StartJoy(0);;
    CONTROL_JoystickEnabled = CONTROL_JoyPresent;
    //		break;
    //}

    /*
    if (CONTROL_MousePresent)
        initprintf("CONTROL_Startup: Mouse Present\n");
    if (CONTROL_JoyPresent)
        initprintf("CONTROL_Startup: Joystick Present\n");
    */

    CONTROL_ButtonState     = 0;
    CONTROL_ButtonHeldState = 0;

    memset(CONTROL_UserInputCleared, 0, sizeof(CONTROL_UserInputCleared));

    for (i=0; i<CONTROL_NUM_FLAGS; i++)
        CONTROL_Flags[i].used = FALSE;

    CONTROL_Started = TRUE;

    return FALSE;
}

void CONTROL_Shutdown(void)
{
    if (!CONTROL_Started) return;

    CONTROL_JoyPresent = FALSE;

    MOUSE_Shutdown();
    uninitinput();

    CONTROL_Started = FALSE;
}

