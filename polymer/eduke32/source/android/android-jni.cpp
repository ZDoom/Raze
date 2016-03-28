//-------------------------------------------------------------------------
/*
Copyright (C) 2015 EDuke32 developers and contributors
Copyright (C) 2015 Voidpoint, LLC

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

#include <jni.h>
#include <android/log.h>

#include "SDL_scancode.h"
#include "SDL_main.h"

#include "TouchControlsContainer.h"
#include "JNITouchControlsUtils.h"

#include "inv.h"

using namespace touchcontrols;

extern "C" {

#include "in_android.h"
#include "function.h"

#if defined __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#define DEFAULT_FADE_FRAMES 10

#ifndef LOGI
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "DUKE", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "DUKE", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "DUKE", __VA_ARGS__))
#endif

droidsysinfo_t droidinfo;

static bool hideTouchControls = true;
static bool hasTouch = false;
static bool weaponWheelVisible = false;
static bool hwScaling = false;
static bool controlsCreated = false;

TouchControlsContainer controlsContainer;

TouchControls *tcBlankTap;
TouchControls *tcYesNo;
TouchControls *tcMenuMain;
TouchControls *tcBackButton;
TouchControls *tcGameMain;
TouchControls *tcGameWeapons;
TouchControls *tcAutomap;

TouchStick *touchJoyLeft;
WheelSelect *weaponWheel;

Button *invButtonPtrs[GET_MAX];

std::string gamePath;

void AndroidPreDrawButtons()
{
    glPushMatrix();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrthof(0, droidinfo.screen_width, droidinfo.screen_height, 0, 0, 1);

    // glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
    // glClear(GL_COLOR_BUFFER_BIT);
    // LOGI("AndroidPreDrawButtons");
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glColor4f(1, 1, 1, 1);

    glDisableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_CULL_FACE);

    glMatrixMode(GL_MODELVIEW);
}

void AndroidPostDrawButtons()
{
    glPopMatrix();
}

void AndroidShowWheel(bool show)
{
    if (!controlsCreated)
        return;

    if (show)
    {
        int weap = AndroidRead(R_PLAYER_GOTWEAPON);

        for (int n = 0; n < 10; n++)
            weaponWheel->setSegmentEnabled(n, (weap >> n) & 0x1);

        // Show inventory buttons
        tcGameWeapons->setAllButtonsEnabled(true);
//        tcGameWeapons->fade(FADE_IN, 5);
        weaponWheelVisible = true;

        AndroidTimer(0);
    }
    else  // hide
    {
        tcGameWeapons->setAllButtonsEnabled(false);
        weaponWheel->setTapMode(false);
        weaponWheelVisible = false;

        AndroidTimer(120);
    }
}

void AndroidShowKeyboard(int onf)
{
    JNI_ShowKeyboard(onf);
}

namespace callbacks
{
    void in_game(int state, int code)
    {
        if (!controlsCreated)
            return;

        switch (code)
        {
        case gamefunc_Fire:
            if (state && AndroidRead(R_SOMETHINGONPLAYER))
            {
                AndroidAction(1, gamefunc_Quick_Kick);
                AndroidAction(0, gamefunc_Quick_Kick);
            }
            else AndroidAction(state, code);
            break;

        case KEY_SHOW_KBRD:
            if (state)
                JNI_ToggleKeyboard();

            AndroidKeyEvent(state, SDL_SCANCODE_GRAVE, 0);
            break;

        case KEY_QUICK_CMD:
            if (state == 1)
                JNI_ShowCustomCommands();
            break;

        case KEY_SHOW_INVEN:
            if (state)
            {
                if (!weaponWheelVisible)
                {
                    AndroidShowWheel(true);
                    weaponWheel->setTapMode(true);
                }
                else
                {
                    AndroidShowWheel(false);
                }
            }
            break;

        case KEY_QUICK_SAVE:
            LOGI("QUICK SAVE");
            AndroidKeyEvent(state, SDL_SCANCODE_F6, 0);
            break;

        case KEY_QUICK_LOAD:
            LOGI("QUICK LOAD");
            AndroidKeyEvent(state, SDL_SCANCODE_F9, 0);
            break;

        default: AndroidAction(state, code); break;
        }
    }

    void inv_button(int state, int code)
    {
        AndroidAction(state, code);

        if (state == 0)
        {
            // Inventory chosen, hide them and wheel
            AndroidShowWheel(false);
        }
    }

    void menu_button(int state, int code)
    {
        if (weaponWheelVisible)
            AndroidShowWheel(false);

        AndroidKeyEvent(state, code, code);

        if (controlsContainer.editingControls)
            controlsContainer.editorButtonPress();
    }

    void menu_select(int action, float x, float y, float dx, float dy)
    {
        AndroidMouseMenu(x * droidinfo.screen_width, y * droidinfo.screen_height);

        if (action == MULTITOUCHMOUSE_DOWN || action == MULTITOUCHMOUSE_UP)
            AndroidMouseMenuButton(action == MULTITOUCHMOUSE_DOWN, 0);
    }

    void proceed(int state, int code)
    {
        if (AndroidRead(R_PLAYER_DEAD_FLAG))
        {
            if (state)
            {
                AndroidAction(1, gamefunc_Open);
                AndroidAction(0, gamefunc_Open);
            }
        }
        else
            AndroidKeyEvent(state, SDL_SCANCODE_RETURN, 0);
    }

    void show_wheel(int enabled) { AndroidShowWheel(enabled ? true : false); }
    void select_weapon(int segment)
    {
        // LOGI("weaponWheel %d",segment);
        if (segment == -1 && droidinput.quickSelectWeapon)
            segment = AndroidRead(R_PLAYER_LASTWEAPON);

        if (segment != -1)
            AndroidAction(2, gamefunc_Weapon_1 + segment);
    }

    void left_double_tap(int state)
    {
        // LOGI("L double %d, droidinput.left_double_action = %d",state,droidinput.left_double_action);
        if (droidinput.left_double_action != -1)
            AndroidAction(state, droidinput.left_double_action);
    }

    void right_double_tap(int state)
    {
        // LOGTOUCH("R double %d",state);
        if (droidinput.right_double_action != -1)
            AndroidAction(state, droidinput.right_double_action);
    }

    void look(int action, float x, float y, float dx, float dy)
    {
        // LOGI(" mouse dx = %f, dy = %f",dx,dy);

        if (weaponWheelVisible)
            return;

        float const scale = 10.f;

        AndroidLook(dx * droidinput.yaw_sens * scale, -dy * droidinput.pitch_sens * scale * (droidinput.invertLook ? -1.f : 1.f));
    }

    void automap_multitouch_mouse_move(int action, float x, float y, float dx, float dy)
    {
        if (action == MULTITOUCHMOUSE_MOVE)
            AndroidAutomapControl(0, dx, dy);
        else if (action == MULTITOUCHMOUSE_ZOOM)
            AndroidAutomapControl(x, 0, 0);
    }

    void move(float joy_x, float joy_y, float mouse_x, float mouse_y)
    {
        // LOGI("left_stick joy_x = %f, joy_y = %f",joy_x,joy_y);
        AndroidMove(joy_y * droidinput.forward_sens, -joy_x * droidinput.strafe_sens);
    }
}

void AndroidToggleButtonEditor(void)
{
    controlsContainer.editorButtonPress();
}

void AndroidTouchInit(int width, int height, const char *graphics_path)
{
    GLScaleWidth = (float)width;
    GLScaleHeight = (float)-height;

    GLAspectRatio = GLScaleWidth/GLScaleHeight;

    ScaleY = nearbyintf(((float) height/(float) width) * ScaleX);

    LOGI("initControls %d x %d,x path = %s", width, height, graphics_path);

    if (controlsCreated)
        return;

    LOGI("creating controls");

    setGraphicsBasePath(graphics_path);

    controlsContainer.pre_draw.connect(sigc::ptr_fun(&AndroidPreDrawButtons));
    controlsContainer.post_draw.connect(sigc::ptr_fun(&AndroidPostDrawButtons));

    tcBlankTap = new TouchControls("blank_tap", false, false);
    tcYesNo = new TouchControls("yes_no", false, false);
    tcMenuMain = new TouchControls("menu", false, false);
    tcBackButton = new TouchControls("back", false, false);
    tcGameMain = new TouchControls("game", false, true, 1);
    tcGameWeapons = new TouchControls("weapons", false, false);
    tcAutomap = new TouchControls("automap", false, false);

    ///////////////////////// BLANK TAP SCREEN //////////////////////

    // One button on whole screen with no graphic, send a return key
    tcBlankTap->addControl(new Button("whole_screen", RectF(0, 0, ScaleX, ScaleY), "", SDL_SCANCODE_RETURN));
    tcBlankTap->signal_button.connect(sigc::ptr_fun(&callbacks::proceed));


    ///////////////////////// YES NO SCREEN /////////////////////

    tcYesNo->addControl(new Button("enter", RectF(37, 19, 43, 25), "button_yes", SDL_SCANCODE_RETURN));
    tcYesNo->addControl(new Button("esc", RectF(21, 19, 27, 25), "button_no", SDL_SCANCODE_ESCAPE));
    tcYesNo->signal_button.connect(sigc::ptr_fun(&callbacks::menu_button));


    // menu and shared back button

    MultitouchMouse *mouseMenu = new MultitouchMouse("mouse", RectF(0, 0, ScaleX, ScaleY), "");
    mouseMenu->signal_action.connect(sigc::ptr_fun(&callbacks::menu_select));
    mouseMenu->setHideGraphics(true);
    tcMenuMain->addControl(mouseMenu);

    Button *consoleButton = new Button("keyboard", "Development console", RectF(4, 0, 8, 4), "button_console", KEY_SHOW_KBRD, false, true);
    tcMenuMain->addControl(consoleButton);

    tcBackButton->addControl(new Button("menu_back", RectF(0, 0, 4, 4), "button_left", SDL_SCANCODE_ESCAPE));
    tcBackButton->signal_button.connect(sigc::ptr_fun(&callbacks::menu_button));

    // main controls

    tcGameMain->addControl(new Button("menu_game", RectF(0, 0, 4, 4), "button_menu", SDL_SCANCODE_ESCAPE));
    tcGameMain->signal_button.connect(sigc::ptr_fun(&callbacks::menu_button));

    tcGameMain->addControl(new Button("use", RectF(50, 14, 55, 19), "button_use", gamefunc_Open));
    tcGameMain->addControl(new Button("fire", RectF(50, 20, 55, 25), "button_fire", gamefunc_Fire));
    tcGameMain->addControl(new Button("jump", RectF(56, 17, 61, 22), "button_jump", gamefunc_Jump));
    tcGameMain->addControl(new Button("kick", "Mighty Foot", RectF(56, 11, 61, 16), "button_kick", gamefunc_Quick_Kick, false, true));

    tcGameMain->addControl(new Button("crouch", RectF(59, ScaleY - 5, 62, ScaleY - 2), "button_crouch", gamefunc_Crouch));

    Button *mapButton = new Button("map", "Overhead map", RectF(6, 0, 8, 2), "button_map", gamefunc_Map, false, true);
    tcGameMain->addControl(mapButton);

    Button *invButton = new Button("show_inventory", "Inventory", RectF(60, 0, 64, 4), "button_inv", KEY_SHOW_INVEN);
    tcGameMain->addControl(invButton);
    tcGameMain->addControl(new Button("next_weapon", "Next weapon", RectF(0, 3, 3, 5), "button_up", gamefunc_Next_Weapon, false, true));
    tcGameMain->addControl(new Button("prev_weapon", "Previous weapon", RectF(0, 5, 3, 7), "button_down", gamefunc_Previous_Weapon, false, true));
    tcGameMain->addControl(new Button("quick_save", "Save game", RectF(22, 0, 24, 2), "button_save", KEY_QUICK_SAVE, false, true));
    tcGameMain->addControl(new Button("quick_load", "Load game", RectF(20, 0, 22, 2), "button_load", KEY_QUICK_LOAD, false, true));

    tcGameMain->addControl(consoleButton);

    // Left stick
    touchJoyLeft = new TouchStick("stick", RectF(4, ScaleY - 20, 24, ScaleY), "button_move");
    tcGameMain->addControl(touchJoyLeft);
    touchJoyLeft->signal_move.connect(sigc::ptr_fun(&callbacks::move));
    touchJoyLeft->signal_double_tap.connect(sigc::ptr_fun(&callbacks::left_double_tap));

    // Mouse look for whole screen
    Mouse *mouse = new Mouse("mouse", RectF(3, 0, ScaleX, ScaleY), "");
    mouse->signal_action.connect(sigc::ptr_fun(&callbacks::look));
    mouse->signal_double_tap.connect(sigc::ptr_fun(&callbacks::right_double_tap));

    mouse->setHideGraphics(true);
    tcGameMain->addControl(mouse);

    tcGameMain->signal_button.connect(sigc::ptr_fun(&callbacks::in_game));

    // Automap
    MultitouchMouse *mapMouse = new MultitouchMouse("gamemouse", RectF(0, 0, ScaleX, ScaleY), "");
    mapMouse->setHideGraphics(true);
    tcAutomap->addControl(mapMouse);
    mapMouse->signal_action.connect(sigc::ptr_fun(&callbacks::automap_multitouch_mouse_move));
    tcAutomap->addControl(mapButton);
    tcAutomap->signal_button.connect(sigc::ptr_fun(&callbacks::in_game));
    tcAutomap->setAlpha(0.5);

    // Weapons
    // Now inventory in the weapons control group!

    weaponWheel = new WheelSelect("weapon_wheel", RectF(17, (ScaleY / 2) - 15, 47, (ScaleY / 2) + 15), "weapon_wheel_orange_blank", 10);
    weaponWheel->signal_selected.connect(sigc::ptr_fun(&callbacks::select_weapon));
    weaponWheel->signal_enabled.connect(sigc::ptr_fun(&callbacks::show_wheel));
    tcGameWeapons->addControl(weaponWheel);

    invButtonPtrs[GET_JETPACK] = new Button("jetpack", RectF(0, 6, 4, 10), "button_inv4", gamefunc_Jetpack, false, false, true);
    invButtonPtrs[GET_STEROIDS] = new Button("steroids", RectF(0, 11, 4, 15), "button_inv2", gamefunc_Steroids, false, false, true);
    invButtonPtrs[GET_HEATS] = new Button("nightv", RectF(0, 16, 4, 20), "button_inv5", gamefunc_NightVision, false, false, true);
    invButtonPtrs[GET_HOLODUKE] = new Button("holoduke", RectF(0, 21, 4, 25), "button_inv3", gamefunc_Holo_Duke, false, false, true);
    invButtonPtrs[GET_FIRSTAID] = new Button("medkit", RectF(0, 26, 4, 30), "button_inv1", gamefunc_MedKit, false, false, true);

    tcGameWeapons->addControl(invButtonPtrs[GET_JETPACK]);
    tcGameWeapons->addControl(invButtonPtrs[GET_FIRSTAID]);
    tcGameWeapons->addControl(invButtonPtrs[GET_HEATS]);
    tcGameWeapons->addControl(invButtonPtrs[GET_HOLODUKE]);
    tcGameWeapons->addControl(invButtonPtrs[GET_STEROIDS]);

    // Inventory are the only buttons so safe to do this
    tcGameWeapons->signal_button.connect(sigc::ptr_fun(&callbacks::inv_button));


    controlsContainer.addControlGroup(tcMenuMain);
    controlsContainer.addControlGroup(tcBackButton);
    controlsContainer.addControlGroup(tcGameMain);
    controlsContainer.addControlGroup(tcGameWeapons);
    controlsContainer.addControlGroup(tcAutomap);
    controlsContainer.addControlGroup(tcYesNo);
    controlsContainer.addControlGroup(tcBlankTap);

    tcGameMain->setAlpha(droidinput.gameControlsAlpha);
    tcGameWeapons->setAlpha(0.f);
    tcGameWeapons->resetOutput();
    tcGameWeapons->setAllButtonsEnabled(false);
    weaponWheel->setTapMode(false);
    int segmap[10] ={ 3, 4, 5, 6, 7, 8, 9, 0, 1, 2 };
    weaponWheel->setSegmentMap(segmap);
    weaponWheelVisible = false;
    tcMenuMain->setAlpha(droidinput.gameControlsAlpha);
    tcBackButton->setAlpha(droidinput.gameControlsAlpha);

    tcGameMain->setXMLFile(gamePath + "/control_layout_main.xml");
    tcGameWeapons->setXMLFile(gamePath + "/control_layout_weapons.xml");
    tcAutomap->setXMLFile(gamePath + "/control_layout_automap.xml");

    setControlsContainer(&controlsContainer);

    controlsCreated = true;
}

void updateTouchScreenMode(touchscreemode_t mode)
{
    // LOGI("updateTouchScreenModeA %d",mode);
    if (!controlsCreated)
        return;

    static touchscreemode_t lastMode = TOUCH_SCREEN_BLANK;

    if (mode != lastMode)
    {
        // first disable the last screen and fade out is necessary
        switch (lastMode)
        {
            case TOUCH_SCREEN_BLANK:  // Does not exist yet break;
            case TOUCH_SCREEN_BLANK_TAP:
                tcBlankTap->resetOutput();
                tcBlankTap->setEnabled(false);  // Dont fade out as no graphics
                break;
            case TOUCH_SCREEN_YES_NO:
                tcYesNo->resetOutput();
                tcYesNo->setEnabled(false);
                break;
            case TOUCH_SCREEN_MENU_NOBACK:
                tcBackButton->fade(FADE_IN, DEFAULT_FADE_FRAMES);
            case TOUCH_SCREEN_MENU:
                tcMenuMain->resetOutput();
                tcMenuMain->fade(FADE_OUT, DEFAULT_FADE_FRAMES);
                break;
            case TOUCH_SCREEN_GAME:
                tcGameMain->resetOutput();
                tcGameMain->fade(FADE_OUT, DEFAULT_FADE_FRAMES);
                tcGameWeapons->setEnabled(false);
                break;
            case TOUCH_SCREEN_AUTOMAP:
                tcAutomap->resetOutput();
                tcAutomap->fade(FADE_OUT, DEFAULT_FADE_FRAMES);
                break;
            case TOUCH_SCREEN_CONSOLE: break;
        }

        // Enable the current new screen
        switch (mode)
        {
            case TOUCH_SCREEN_BLANK:  // Does not exist yet break;
            case TOUCH_SCREEN_BLANK_TAP:
                tcBlankTap->setEnabled(true);
                break;
            case TOUCH_SCREEN_YES_NO:
                tcYesNo->setEnabled(true);
                tcYesNo->fade(FADE_IN, DEFAULT_FADE_FRAMES);
                break;

            case TOUCH_SCREEN_MENU_NOBACK:
                tcBackButton->fade(FADE_OUT, DEFAULT_FADE_FRAMES);
            case TOUCH_SCREEN_MENU:
                tcMenuMain->setEnabled(true);
                tcMenuMain->fade(FADE_IN, DEFAULT_FADE_FRAMES);

                // This is a bit of a hack, we need to enable the inventory buttons so they can be edited, they will not
                // be seen anyway
                break;
            case TOUCH_SCREEN_GAME:
                tcGameMain->setEnabled(true);
                tcGameMain->fade(FADE_IN, DEFAULT_FADE_FRAMES);
                tcGameWeapons->setEnabled(true);
                break;
            case TOUCH_SCREEN_AUTOMAP:
                tcAutomap->setEnabled(true);
                tcAutomap->fade(FADE_IN, DEFAULT_FADE_FRAMES);
                break;
            case TOUCH_SCREEN_CONSOLE: break;
        }

        lastMode = mode;
    }

    tcBackButton->setEnabled(!tcBlankTap->enabled && mode != TOUCH_SCREEN_MENU_NOBACK && mode != TOUCH_SCREEN_GAME);
}

extern char videomodereset;

void AndroidDrawControls()
{
    static int loadedGLImages = 0;

    if (videomodereset)
    {
        loadedGLImages = -1;
        return;
    }

    // We need to do this here now because duke loads a new gl context

    if (loadedGLImages <= 0)
    {
        controlsContainer.initGL(loadedGLImages == -1);
        loadedGLImages = 1;
    }

    //  LOGI("AndroidDrawControls");

    if (!controlsCreated)
        return;

    updateTouchScreenMode((touchscreemode_t)AndroidRead(R_TOUCH_MODE));

    if (touchJoyLeft)
        touchJoyLeft->setHideGraphics(droidinput.hideStick);

    if (tcGameMain)
        tcGameMain->setAlpha(droidinput.gameControlsAlpha);

    if (tcMenuMain)
        tcMenuMain->setAlpha(droidinput.gameControlsAlpha);

    if (tcBackButton)
        tcBackButton->setAlpha(droidinput.gameControlsAlpha);

    if (tcGameWeapons)
    {
        tcGameWeapons->setAlpha(droidinput.gameControlsAlpha);

        int inv = AndroidRead(R_PLAYER_INV_AMOUNT);

        for (int i = 0; i < GET_MAX; ++i)
            if (invButtonPtrs[i])
            {
                if (!weaponWheelVisible)
                    invButtonPtrs[i]->setEnabled((inv & (1 << i)));

                invButtonPtrs[i]->setAlpha(tcGameWeapons->getFadedAlpha() * ((inv & (1 << i)) ? 1.f : 0.3f));
            }
    }

    controlsContainer.draw();

/*
    if (controlsContainer.editingControls)
        tcBackButton->draw();
*/
}


#define EXPORT_ME __NDK_FPABI__ __attribute__((visibility("default")))

int argc = 1;
const char *argv[32];

static inline const char *getGamePath() { return gamePath.c_str(); }

jint EXPORT_ME Java_com_voidpoint_duke3d_NativeLib_i(JNIEnv *env, jobject thiz, jstring graphics_dir,
                                                               jint audio_rate, jint audio_buffer_size,
                                                               jobjectArray argsArray,
                                                               jstring jduke3d_path)
{
    droidinfo.audio_sample_rate = audio_rate;
    droidinfo.audio_buffer_size = audio_buffer_size;

    argv[0] = "eduke32";
    int argCount = (env)->GetArrayLength(argsArray);
    LOGI("argCount = %d", argCount);
    for (int i = 0; i < argCount; i++)
    {
        jstring string = (jstring)(env)->GetObjectArrayElement(argsArray, i);
        argv[argc] = (char const *)(env)->GetStringUTFChars(string, 0);
        LOGI("arg = %s", argv[argc]);
        argc++;
    }

    gamePath = std::string(env->GetStringUTFChars(jduke3d_path, NULL));

    // Change working dir, save games etc
    // FIXME: potentially conflicts with chdirs in -game_dir support
    chdir(getGamePath());

    LOGI("duke3d_path = %s", getGamePath());

    if (hasTouch)
        AndroidTouchInit(droidinfo.screen_width, droidinfo.screen_height, "/assets/");
    else LOGI("skipping touch input");

    extern int SDL_main(int argc, char const *argv[]);

    SDL_main(argc, (char const **)argv);

    return 0;
}

__attribute__((visibility("default"))) jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    LOGI("JNI_OnLoad");
    JNI_SetEnv(vm);
    return JNI_VERSION_1_4;
}


void EXPORT_ME
Java_com_voidpoint_duke3d_NativeLib_kp(JNIEnv *env, jobject obj, jint down, jint keycode, jint unicode)
{
    LOGI("keypress %d", keycode);
    if (controlsContainer.isEditing())
    {
        if (down && (keycode == SDL_SCANCODE_ESCAPE))
            controlsContainer.finishEditing();
        return;
    }

    AndroidKeyEvent(down, keycode, unicode);
}


void EXPORT_ME Java_com_voidpoint_duke3d_NativeLib_te(JNIEnv *env, jobject obj, jint action, jint pid,
                                                                     jfloat x, jfloat y)
{
    if (tcBackButton && tcBackButton->enabled)
    {
        if (!tcBackButton->processPointer(action, pid, x, y))
            controlsContainer.processPointer(action, pid, x, y);
    }
    else controlsContainer.processPointer(action, pid, x, y);
}


void EXPORT_ME Java_com_voidpoint_duke3d_NativeLib_da(JNIEnv *env, jobject obj, jint state, jint action)
{
    LOGI("doAction %d %d", state, action);

    // gamepadButtonPressed();
    if (controlsCreated && hideTouchControls && tcGameMain)
    {
        if (tcGameMain->isEnabled())
            tcGameMain->setEnabled(false);

        if (tcGameWeapons && tcGameWeapons->isEnabled())
            tcGameWeapons->setEnabled(false);
    }

    AndroidAction(state, action);
}

void EXPORT_ME Java_com_voidpoint_duke3d_NativeLib_am(JNIEnv *env, jobject obj, jfloat fwd, jfloat strafe)
{
    AndroidMove(fwd, strafe);
}

void EXPORT_ME Java_com_voidpoint_duke3d_NativeLib_al(JNIEnv *env, jobject obj, jfloat pitch, jfloat yaw)
{
    AndroidLookJoystick(yaw, pitch);
}

void EXPORT_ME Java_com_voidpoint_duke3d_NativeLib_sts(JNIEnv *env, jobject obj, int other)
{
    // TODO: defined names for these values
    hasTouch = other & 0x4000 ? true : false;
    hideTouchControls = other & 0x8000 ? true : false;
    hwScaling = other & 0x10000 ? true : false;

    // keep in sync with Duke3d/res/values/strings.xml
    int doubletap_options[4] ={ -1, gamefunc_Quick_Kick, gamefunc_MedKit, gamefunc_Jetpack };

    droidinput.left_double_action = doubletap_options[((other >> 4) & 0xF)];
    droidinput.right_double_action = doubletap_options[((other >> 8) & 0xF)];

    LOGI("setTouchSettings left_double_action = %d", droidinput.left_double_action);
}

void EXPORT_ME Java_com_voidpoint_duke3d_NativeLib_rts(JNIEnv *env, jobject obj)
{
    controlsContainer.resetDefaults();
}

std::string quickCommandString;

jint EXPORT_ME Java_com_voidpoint_duke3d_NativeLib_qc(JNIEnv *env, jobject obj, jstring command)
{
    const char *p = env->GetStringUTFChars(command, NULL);
    quickCommandString = std::string(p) + "\n";
    env->ReleaseStringUTFChars(command, p);
    AndroidOSD(quickCommandString.c_str());

    return 0;
}

void EXPORT_ME Java_com_voidpoint_duke3d_NativeLib_sss(JNIEnv *env, jobject thiz, jint width, jint height)
{
    droidinfo.screen_width = width;
    droidinfo.screen_height = height;
}

extern void SDL_Android_Init(JNIEnv *env, jclass cls);

void EXPORT_ME Java_org_libsdl_app_SDLActivity_nativeInit(JNIEnv *env, jclass cls)
{
    /* This interface could expand with ABI negotiation, calbacks, etc. */
    SDL_Android_Init(env, cls);
    SDL_SetMainReady();
    // SDL_EventState(SDL_TEXTINPUT,SDL_ENABLE);
}

#if defined __GNUC__
# pragma GCC diagnostic pop
#endif

}
