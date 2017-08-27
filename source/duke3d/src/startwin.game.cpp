//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

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

#ifndef _WIN32
#error Only for Windows
#endif

#include "compat.h"

#define NEED_WINDOWSX_H
#define NEED_COMMCTRL_H
#include "windows_inc.h"

#include "renderlayer.h"

#ifdef STARTUP_SETUP_WINDOW

#include "build.h"
#include "cache1d.h"

#include "grpscan.h"

#include "startwin.game.h"

// for ud
#include "keyboard.h"
#include "control.h"
#include "_control.h"
#include "function.h"
#include "inv.h"
#define ONLY_USERDEFS
#include "game.h"

#include "common_game.h"
#include "cmdline.h"

#define TAB_CONFIG 0
// #define TAB_GAME 1
#define TAB_MESSAGES 1

static struct
{
    struct grpfile_t const * grp;
    int32_t flags; // bitfield
    int32_t xdim, ydim, bpp;
    int32_t forcesetup;
    int32_t usemouse, usejoy;
    char *gamedir;
}
settings;

static HWND startupdlg = NULL;
static HWND pages[3] =
{
    NULL, NULL, NULL
};
static int32_t done = -1, mode = TAB_CONFIG;

static CACHE1D_FIND_REC *finddirs=NULL;

static inline void clearfilenames(void)
{
    klistfree(finddirs);
    finddirs = NULL;
}

static inline int32_t getfilenames(char const *path)
{
    clearfilenames();
    finddirs = klistpath(path,"*",CACHE1D_FIND_DIR);
    return 0;
}

#define POPULATE_VIDEO 1
#define POPULATE_CONFIG 2
#define POPULATE_GAME 4
#define POPULATE_GAMEDIRS 8

#ifdef INPUT_MOUSE
#undef INPUT_MOUSE
#endif

#define INPUT_KB 0
#define INPUT_MOUSE 1
#define INPUT_JOYSTICK 2
#define INPUT_ALL 3

const char *controlstrings[] = { "Keyboard only", "Keyboard and mouse", "Keyboard and joystick", "All supported devices" };

static void PopulateForm(int32_t pgs)
{
    HWND hwnd;
    char buf[512];
    int32_t i,j;

    if (pgs & POPULATE_GAMEDIRS)
    {
        CACHE1D_FIND_REC *dirs = NULL;

        hwnd = GetDlgItem(pages[TAB_CONFIG], IDCGAMEDIR);

        getfilenames("/");
        (void)ComboBox_ResetContent(hwnd);
        j = ComboBox_AddString(hwnd, "None");
        (void)ComboBox_SetItemData(hwnd, j, 0);
        (void)ComboBox_SetCurSel(hwnd, j);
        for (dirs=finddirs,i=1,j=1; dirs != NULL; dirs=dirs->next)
        {
            if (Bstrcasecmp(dirs->name, "autoload") == 0)
            {
                j++;
                continue;
            }

            (void)ComboBox_AddString(hwnd, dirs->name);
            (void)ComboBox_SetItemData(hwnd, i, j);
            if (Bstrcasecmp(dirs->name,settings.gamedir) == 0)
                (void)ComboBox_SetCurSel(hwnd, i);

            i++;
            j++;
        }
    }

    if (pgs & POPULATE_VIDEO)
    {
        int32_t mode;

        hwnd = GetDlgItem(pages[TAB_CONFIG], IDCVMODE);

        mode = checkvideomode(&settings.xdim, &settings.ydim, settings.bpp, settings.flags&1, 1);
        if (mode < 0 || (settings.bpp < 15 && (settings.flags & 2)))
        {
            int32_t cd[] = { 32, 24, 16, 15, 8, 0 };
            for (i=0; cd[i];)
            {
                if (cd[i] >= settings.bpp) i++;
                else break;
            }
            for (; cd[i]; i++)
            {
                mode = checkvideomode(&settings.xdim, &settings.ydim, cd[i], settings.flags&1, 1);
                if (mode < 0) continue;
                settings.bpp = cd[i];
                break;
            }
        }

        Button_SetCheck(GetDlgItem(pages[TAB_CONFIG], IDCFULLSCREEN), ((settings.flags&1) ? BST_CHECKED : BST_UNCHECKED));
        Button_SetCheck(GetDlgItem(pages[TAB_CONFIG], IDCPOLYMER), ((settings.flags&2) ? BST_CHECKED : BST_UNCHECKED));

        (void)ComboBox_ResetContent(hwnd);

        for (i=0; i<validmodecnt; i++)
        {
            if (validmode[i].fs != (settings.flags & 1)) continue;
            if ((validmode[i].bpp < 15) && (settings.flags & 2)) continue;

            // all modes get added to the 3D mode list
            Bsprintf(buf, "%dx%d %s", validmode[i].xdim, validmode[i].ydim, validmode[i].bpp == 8 ? "software" : "OpenGL");
            j = ComboBox_AddString(hwnd, buf);
            (void)ComboBox_SetItemData(hwnd, j, i);
            if (i == mode)(void)ComboBox_SetCurSel(hwnd, j);
        }
    }

    if (pgs & POPULATE_CONFIG)
    {
        Button_SetCheck(GetDlgItem(pages[TAB_CONFIG], IDCALWAYSSHOW), (settings.forcesetup ? BST_CHECKED : BST_UNCHECKED));
        Button_SetCheck(GetDlgItem(pages[TAB_CONFIG], IDCAUTOLOAD), (!(settings.flags & 4) ? BST_CHECKED : BST_UNCHECKED));

        hwnd = GetDlgItem(pages[TAB_CONFIG], IDCINPUT);

        (void)ComboBox_ResetContent(hwnd);
        (void)ComboBox_SetCurSel(hwnd, 0);

        j = 4;

#ifdef RENDERTYPEWIN
        if (di_disabled) j = 2;
#endif

        for (i=0; i<j; i++)
        {
            (void)ComboBox_InsertString(hwnd, i, controlstrings[i]);
            (void)ComboBox_SetItemData(hwnd, i, i);

            switch (i)
            {
            case INPUT_MOUSE:
                if (settings.usemouse && !settings.usejoy)(void)ComboBox_SetCurSel(hwnd, i);
                break;
            case INPUT_JOYSTICK:
                if (!settings.usemouse && settings.usejoy)(void)ComboBox_SetCurSel(hwnd, i);
                break;
            case INPUT_ALL:
                if (settings.usemouse && settings.usejoy)(void)ComboBox_SetCurSel(hwnd, i);
                break;
            }
        }
    }

    if (pgs & POPULATE_GAME)
    {
        int32_t j;
        char buf[1024];

        hwnd = GetDlgItem(pages[TAB_CONFIG], IDCDATA);

        for (grpfile_t const * fg = foundgrps; fg; fg=fg->next)
        {
            Bsprintf(buf, "%s\t%s", fg->type->name, fg->filename);
            j = ListBox_AddString(hwnd, buf);
            (void)ListBox_SetItemData(hwnd, j, (LPARAM)fg);
            if (settings.grp == fg)
            {
                (void)ListBox_SetCurSel(hwnd, j);
            }
        }
    }
}

static INT_PTR CALLBACK ConfigPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDCFULLSCREEN:
            if (settings.flags & 1) settings.flags &= ~1;
            else settings.flags |= 1;
            PopulateForm(POPULATE_VIDEO);
            return TRUE;
        case IDCPOLYMER:
            if (settings.flags & 2) settings.flags &= ~2;
            else settings.flags |= 2;
            if (settings.bpp == 8) settings.bpp = 32;
            PopulateForm(POPULATE_VIDEO);
            return TRUE;
        case IDCVMODE:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                int32_t i;
                i = ComboBox_GetCurSel((HWND)lParam);
                if (i != CB_ERR) i = ComboBox_GetItemData((HWND)lParam, i);
                if (i != CB_ERR)
                {
                    settings.xdim = validmode[i].xdim;
                    settings.ydim = validmode[i].ydim;
                    settings.bpp  = validmode[i].bpp;
                }
            }
            return TRUE;
        case IDCALWAYSSHOW:
            settings.forcesetup = IsDlgButtonChecked(hwndDlg, IDCALWAYSSHOW) == BST_CHECKED;
            return TRUE;
        case IDCAUTOLOAD:
            if (IsDlgButtonChecked(hwndDlg, IDCAUTOLOAD) == BST_CHECKED)
                settings.flags &= ~4;
            else settings.flags |= 4;
            return TRUE;
        case IDCINPUT:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                int32_t i;
                i = ComboBox_GetCurSel((HWND)lParam);
                if (i != CB_ERR) i = ComboBox_GetItemData((HWND)lParam, i);
                if (i != CB_ERR)
                {
                    switch (i)
                    {
                    case INPUT_KB:
                        settings.usemouse = settings.usejoy = 0;
                        break;
                    case INPUT_MOUSE:
                        settings.usemouse = 1;
                        settings.usejoy = 0;
                        break;
                    case INPUT_JOYSTICK:
                        settings.usemouse = 0;
                        settings.usejoy = 1;
                        break;
                    case INPUT_ALL:
                        settings.usemouse = settings.usejoy = 1;
                        break;
                    }
                }
            }
            return TRUE;

        case IDCGAMEDIR:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                int32_t i,j;
                CACHE1D_FIND_REC *dir = NULL;
                i = ComboBox_GetCurSel((HWND)lParam);
                if (i != CB_ERR) i = ComboBox_GetItemData((HWND)lParam, i);
                if (i != CB_ERR)
                {
                    if (i==0)
                        settings.gamedir = NULL;
                    else
                    {
                        for (j=1,dir=finddirs; dir != NULL; dir=dir->next,j++)
                            if (j == i)
                            {
                                settings.gamedir = dir->name;
                                break;
                            }
                    }
                }
            }
            return TRUE;
        case IDCDATA:
        {
            intptr_t i;
            if (HIWORD(wParam) != LBN_SELCHANGE) break;
            i = ListBox_GetCurSel((HWND)lParam);
            if (i != CB_ERR) i = ListBox_GetItemData((HWND)lParam, i);
            if (i != CB_ERR)
            {
                settings.grp = (grpfile_t const *)i;
            }
            return TRUE;
        }
        default:
            break;
        }
        break;
    default:
        break;
    }
    return FALSE;
}


static void SetPage(int32_t n)
{
    HWND tab;
    int32_t cur;
    tab = GetDlgItem(startupdlg, WIN_STARTWIN_TABCTL);
    cur = (int32_t)SendMessage(tab, TCM_GETCURSEL,0,0);
    ShowWindow(pages[cur],SW_HIDE);
    SendMessage(tab, TCM_SETCURSEL, n, 0);
    ShowWindow(pages[n],SW_SHOW);
    mode = n;

    SetFocus(GetDlgItem(startupdlg, WIN_STARTWIN_TABCTL));
}

static void EnableConfig(int32_t n)
{
    //EnableWindow(GetDlgItem(startupdlg, WIN_STARTWIN_CANCEL), n);
    EnableWindow(GetDlgItem(startupdlg, WIN_STARTWIN_START), n);
    EnableWindow(GetDlgItem(pages[TAB_CONFIG], IDCFULLSCREEN), n);
    EnableWindow(GetDlgItem(pages[TAB_CONFIG], IDCPOLYMER), n);
    EnableWindow(GetDlgItem(pages[TAB_CONFIG], IDCVMODE), n);
    EnableWindow(GetDlgItem(pages[TAB_CONFIG], IDCINPUT), n);

    EnableWindow(GetDlgItem(pages[TAB_CONFIG], IDCDATA), n);
    EnableWindow(GetDlgItem(pages[TAB_CONFIG], IDCGAMEDIR), n);
}

static INT_PTR CALLBACK startup_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HBITMAP hbmp = NULL;
    HDC hdc;

    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        HWND hwnd;
        RECT r, rdlg, chrome, rtab, rcancel, rstart;
        int32_t xoffset = 0, yoffset = 0;

        // Fetch the positions (in screen coordinates) of all the windows we need to tweak
        ZeroMemory(&chrome, sizeof(chrome));
        AdjustWindowRect(&chrome, GetWindowLong(hwndDlg, GWL_STYLE), FALSE);
        GetWindowRect(hwndDlg, &rdlg);
        GetWindowRect(GetDlgItem(hwndDlg, WIN_STARTWIN_TABCTL), &rtab);
        GetWindowRect(GetDlgItem(hwndDlg, WIN_STARTWIN_CANCEL), &rcancel);
        GetWindowRect(GetDlgItem(hwndDlg, WIN_STARTWIN_START), &rstart);

        // Knock off the non-client area of the main dialogue to give just the client area
        rdlg.left -= chrome.left;
        rdlg.top -= chrome.top;
        rdlg.right -= chrome.right;
        rdlg.bottom -= chrome.bottom;

        // Translate them to client-relative coordinates wrt the main dialogue window
        rtab.right -= rtab.left - 1;
        rtab.bottom -= rtab.top - 1;
        rtab.left  -= rdlg.left;
        rtab.top -= rdlg.top;

        rcancel.right -= rcancel.left - 1;
        rcancel.bottom -= rcancel.top - 1;
        rcancel.left -= rdlg.left;
        rcancel.top -= rdlg.top;

        rstart.right -= rstart.left - 1;
        rstart.bottom -= rstart.top - 1;
        rstart.left -= rdlg.left;
        rstart.top -= rdlg.top;

        // And then convert the main dialogue coordinates to just width/length
        rdlg.right -= rdlg.left - 1;
        rdlg.bottom -= rdlg.top - 1;
        rdlg.left = 0;
        rdlg.top = 0;

        // Load the bitmap into the bitmap control and fetch its dimensions
        hbmp = LoadBitmap((HINSTANCE)win_gethinstance(), MAKEINTRESOURCE(RSRC_BMP));
        hwnd = GetDlgItem(hwndDlg,WIN_STARTWIN_BITMAP);
        SendMessage(hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmp);
        GetClientRect(hwnd, &r);
        xoffset = r.right;
        yoffset = r.bottom - rdlg.bottom;

        // Shift and resize the controls that require it
        rtab.left += xoffset;
        rtab.bottom += yoffset;
        rcancel.left += xoffset;
        rcancel.top += yoffset;
        rstart.left += xoffset;
        rstart.top += yoffset;
        rdlg.right += xoffset;
        rdlg.bottom += yoffset;

        // Move the controls to their new positions
        MoveWindow(GetDlgItem(hwndDlg, WIN_STARTWIN_TABCTL), rtab.left, rtab.top, rtab.right, rtab.bottom, FALSE);
        MoveWindow(GetDlgItem(hwndDlg, WIN_STARTWIN_CANCEL), rcancel.left, rcancel.top, rcancel.right, rcancel.bottom, FALSE);
        MoveWindow(GetDlgItem(hwndDlg, WIN_STARTWIN_START), rstart.left, rstart.top, rstart.right, rstart.bottom, FALSE);

        // Move the main dialogue to the centre of the screen
        hdc = GetDC(NULL);
        rdlg.left = (GetDeviceCaps(hdc, HORZRES) - rdlg.right) / 2;
        rdlg.top = (GetDeviceCaps(hdc, VERTRES) - rdlg.bottom) / 2;
        ReleaseDC(NULL, hdc);
        MoveWindow(hwndDlg, rdlg.left + chrome.left, rdlg.top + chrome.left,
                   rdlg.right + (-chrome.left+chrome.right), rdlg.bottom + (-chrome.top+chrome.bottom), TRUE);

        // Add tabs to the tab control
        {
            TCITEM tab;

            hwnd = GetDlgItem(hwndDlg, WIN_STARTWIN_TABCTL);

            ZeroMemory(&tab, sizeof(tab));
            tab.mask = TCIF_TEXT;
            static char textSetup[] = TEXT("Setup");
            tab.pszText = textSetup;
            SendMessage(hwnd, TCM_INSERTITEM, (WPARAM)TAB_CONFIG, (LPARAM)&tab);
            tab.mask = TCIF_TEXT;
            static char textMessageLog[] = TEXT("Message Log");
            tab.pszText = textMessageLog;
            SendMessage(hwnd, TCM_INSERTITEM, (WPARAM)TAB_MESSAGES, (LPARAM)&tab);

            // Work out the position and size of the area inside the tab control for the pages
            ZeroMemory(&r, sizeof(r));
            GetClientRect(hwnd, &r);
            SendMessage(hwnd, TCM_ADJUSTRECT, FALSE, (LPARAM)&r);
            r.right -= r.left-1;
            r.bottom -= r.top-1;
            r.top += rtab.top;
            r.left += rtab.left;

            // Create the pages and position them in the tab control, but hide them
            pages[TAB_CONFIG] = CreateDialog((HINSTANCE)win_gethinstance(),
                                             MAKEINTRESOURCE(WIN_STARTWINPAGE_CONFIG), hwndDlg, ConfigPageProc);
            pages[TAB_MESSAGES] = GetDlgItem(hwndDlg, WIN_STARTWIN_MESSAGES);
            SetWindowPos(pages[TAB_CONFIG], hwnd,r.left,r.top,r.right,r.bottom,SWP_HIDEWINDOW);
            SetWindowPos(pages[TAB_MESSAGES], hwnd,r.left,r.top,r.right,r.bottom,SWP_HIDEWINDOW);

            // Tell the editfield acting as the console to exclude the width of the scrollbar
            GetClientRect(pages[TAB_MESSAGES],&r);
            r.right -= GetSystemMetrics(SM_CXVSCROLL)+4;
            r.left = r.top = 0;
            SendMessage(pages[TAB_MESSAGES], EM_SETRECTNP,0,(LPARAM)&r);

            // Set a tab stop in the game data listbox
            {
                DWORD tabs[1] = { 150 };
                (void)ListBox_SetTabStops(GetDlgItem(pages[TAB_CONFIG], IDCDATA), 1, tabs);
            }

            SetFocus(GetDlgItem(hwndDlg, WIN_STARTWIN_START));
            SetWindowText(hwndDlg, apptitle);
        }
        return FALSE;
    }

    case WM_NOTIFY:
    {
        LPNMHDR nmhdr = (LPNMHDR)lParam;
        int32_t cur;
        if (nmhdr->idFrom != WIN_STARTWIN_TABCTL) break;
        cur = (int32_t)SendMessage(nmhdr->hwndFrom, TCM_GETCURSEL,0,0);
        switch (nmhdr->code)
        {
        case TCN_SELCHANGING:
        {
            if (cur < 0 || !pages[cur]) break;
            ShowWindow(pages[cur],SW_HIDE);
            return TRUE;
        }
        case TCN_SELCHANGE:
        {
            if (cur < 0 || !pages[cur]) break;
            ShowWindow(pages[cur],SW_SHOW);
            return TRUE;
        }
        }
        break;
    }

    case WM_CLOSE:
        if (mode == TAB_CONFIG) done = 0;
        else quitevent++;
        return TRUE;

    case WM_DESTROY:
        if (hbmp)
        {
            DeleteObject(hbmp);
            hbmp = NULL;
        }

        if (pages[TAB_CONFIG])
        {
            DestroyWindow(pages[TAB_CONFIG]);
            pages[TAB_CONFIG] = NULL;
        }

        startupdlg = NULL;
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case WIN_STARTWIN_CANCEL:
            if (mode == TAB_CONFIG) done = 0;
            else quitevent++;
            return TRUE;
        case WIN_STARTWIN_START:
            done = 1;
            return TRUE;
        }
        return FALSE;

    case WM_CTLCOLORSTATIC:
        if ((HWND)lParam == pages[TAB_MESSAGES])
            return (BOOL)(intptr_t)GetSysColorBrush(COLOR_WINDOW);
        break;

    default:
        break;
    }

    return FALSE;
}


int32_t startwin_open(void)
{
    INITCOMMONCONTROLSEX icc;
    if (startupdlg) return 1;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_TAB_CLASSES;
    InitCommonControlsEx(&icc);
    startupdlg = CreateDialog((HINSTANCE)win_gethinstance(), MAKEINTRESOURCE(WIN_STARTWIN), NULL, startup_dlgproc);
    if (startupdlg)
    {
        SetPage(TAB_MESSAGES);
        EnableConfig(0);
        return 0;
    }
    return -1;
}

int32_t startwin_close(void)
{
    if (!startupdlg) return 1;
    DestroyWindow(startupdlg);
    startupdlg = NULL;
    return 0;
}

int32_t startwin_puts(const char *buf)
{
    const char *p = NULL, *q = NULL;
    static char workbuf[1024];
    static int32_t newline = 0;
    int32_t curlen, linesbefore, linesafter;
    HWND edctl;
    int32_t vis;
    static HWND dactrl = NULL;

    if (!startupdlg) return 1;

    edctl = pages[TAB_MESSAGES];
    if (!edctl) return -1;

    if (!dactrl) dactrl = GetDlgItem(startupdlg, WIN_STARTWIN_TABCTL);

    vis = ((int32_t)SendMessage(dactrl, TCM_GETCURSEL,0,0) == TAB_MESSAGES);

    if (vis) SendMessage(edctl, WM_SETREDRAW, FALSE,0);
    curlen = SendMessage(edctl, WM_GETTEXTLENGTH, 0,0);
    SendMessage(edctl, EM_SETSEL, (WPARAM)curlen, (LPARAM)curlen);
    linesbefore = SendMessage(edctl, EM_GETLINECOUNT, 0,0);
    p = buf;
    while (*p)
    {
        if (newline)
        {
            SendMessage(edctl, EM_REPLACESEL, 0, (LPARAM)"\r\n");
            newline = 0;
        }
        q = p;
        while (*q && *q != '\n') q++;
        Bmemcpy(workbuf, p, q-p);
        if (*q == '\n')
        {
            if (!q[1])
            {
                newline = 1;
                workbuf[q-p] = 0;
            }
            else
            {
                workbuf[q-p] = '\r';
                workbuf[q-p+1] = '\n';
                workbuf[q-p+2] = 0;
            }
            p = q+1;
        }
        else
        {
            workbuf[q-p] = 0;
            p = q;
        }
        SendMessage(edctl, EM_REPLACESEL, 0, (LPARAM)workbuf);
    }
    linesafter = SendMessage(edctl, EM_GETLINECOUNT, 0,0);
    SendMessage(edctl, EM_LINESCROLL, 0, linesafter-linesbefore);
    if (vis) SendMessage(edctl, WM_SETREDRAW, TRUE,0);
    return 0;
}

int32_t startwin_settitle(const char *str)
{
    if (!startupdlg) return 1;
    SetWindowText(startupdlg, str);
    return 0;
}

int32_t startwin_idle(void *v)
{
    if (!startupdlg || !IsWindow(startupdlg)) return 0;
    if (IsDialogMessage(startupdlg, (MSG *)v)) return 1;
    return 0;
}

int32_t startwin_run(void)
{
    MSG msg;
    if (!startupdlg) return 1;

    done = -1;

    SetPage(TAB_CONFIG);
    EnableConfig(1);

    settings.flags = 0;
    if (ud.config.ScreenMode) settings.flags |= 1;
#ifdef POLYMER
    if (glrendmode == REND_POLYMER) settings.flags |= 2;
#endif
    if (ud.config.NoAutoLoad) settings.flags |= 4;
    settings.xdim = ud.config.ScreenWidth;
    settings.ydim = ud.config.ScreenHeight;
    settings.bpp = ud.config.ScreenBPP;
    settings.forcesetup = ud.config.ForceSetup;
    settings.usemouse = ud.config.UseMouse;
    settings.usejoy = ud.config.UseJoystick;
    settings.grp = g_selectedGrp;
    settings.gamedir = g_modDir;
    PopulateForm(-1);

    while (done < 0)
    {
        switch (GetMessage(&msg, NULL, 0,0))
        {
        case 0:
            done = 1;
            break;
        case -1:
            return -1;
        default:
            if (IsWindow(startupdlg) && IsDialogMessage(startupdlg, &msg)) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            break;
        }
    }

    SetPage(TAB_MESSAGES);
    EnableConfig(0);
    if (done)
    {
        ud.config.ScreenMode = (settings.flags&1);
#ifdef POLYMER
        if (settings.flags & 2) glrendmode = REND_POLYMER;
        else glrendmode = REND_POLYMOST;
#endif
        if (settings.flags & 4) ud.config.NoAutoLoad = 1;
        else ud.config.NoAutoLoad = 0;
        ud.config.ScreenWidth = settings.xdim;
        ud.config.ScreenHeight = settings.ydim;
        ud.config.ScreenBPP = settings.bpp;
        ud.config.ForceSetup = settings.forcesetup;
        ud.config.UseMouse = settings.usemouse;
        ud.config.UseJoystick = settings.usejoy;
        g_selectedGrp = settings.grp;

        if (g_noSetup == 0 && settings.gamedir != NULL)
            Bstrcpy(g_modDir,settings.gamedir);
        else Bsprintf(g_modDir,"/");
    }

    return done;
}

#endif // STARTUP_SETUP_WINDOW
