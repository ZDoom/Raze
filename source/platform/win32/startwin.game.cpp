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

#include "ns.h"	// Must come before everything else!

#ifndef _WIN32
#error Only for Windows
#endif

#include "renderlayer.h"

#ifdef STARTUP_SETUP_WINDOW

#define NEED_WINDOWSX_H
#define NEED_COMMCTRL_H
#define ONLY_USERDEFS

#include "_control.h"
#include "build.h"
#include "cache1d.h"
#include "compat.h"
#include "control.h"
#include "keyboard.h"
#include "startwin.game.h"
#include "windows_inc.h"
#include "gamecontrol.h"

#pragma warning(disable:4244) // There's just a bit too much of these in here...



#define TAB_CONFIG 0
#define TAB_MESSAGES 1

typedef struct {
	int32_t fullscreen;
	int32_t xdim;
	int32_t ydim;
	int32_t bpp;
} ud_setup_t;

static struct
{
    int grp;
    ud_setup_t shared;
    int polymer;
}
settings;

static HWND startupdlg;
static HWND pages[3];
static int done = -1;
static int mode = TAB_CONFIG;

static TArray<GrpEntry> *gamedata;

static CACHE1D_FIND_REC *finddirs;

static inline void clearfilenames(void)
{
    klistfree(finddirs);
    finddirs = NULL;
}

static inline void getfilenames(char const *path)
{
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

// Thanks, Microsoft for not providing alternatives for the dialog control macros. :(
#undef SNDMSG
#define SNDMSG ::SendMessageA

const char *controlstrings[] = { "Keyboard only", "Keyboard and mouse", "Keyboard and joystick", "All supported devices" };

static void PopulateForm(int32_t pgs)
{
    char buf[512];

    if (pgs & POPULATE_GAMEDIRS)
    {
        HWND hwnd = GetDlgItem(pages[TAB_CONFIG], IDCGAMEDIR);

#if 0 // This doesn't currently work and in its current form is useless anyway. It should offer a real directory picker.
        getfilenames("/");
        (void)ComboBox_ResetContent(hwnd);
        int const r = ComboBox_AddString(hwnd, "None");
        (void)ComboBox_SetItemData(hwnd, r, 0);
        (void)ComboBox_SetCurSel(hwnd, r);
        auto dirs = finddirs;
        for (int i=1, j=1; dirs != NULL; dirs=dirs->next)
        {
            if (Bstrcasecmp(dirs->name, "autoload") == 0)
            {
                j++;
                continue;
            }

            (void)ComboBox_AddString(hwnd, dirs->name);
            (void)ComboBox_SetItemData(hwnd, i, j);

            i++;
            j++;
        }
#endif
    }

    if (pgs & POPULATE_VIDEO)
    {
        HWND hwnd = GetDlgItem(pages[TAB_CONFIG], IDCVMODE);
        int mode = videoCheckMode(&settings.shared.xdim, &settings.shared.ydim, settings.shared.bpp, settings.shared.fullscreen, 1);

        if (mode < 0 || (settings.shared.bpp < 15))
        {
            int CONSTEXPR cd[] = { 32, 24, 16, 15, 8, 0 };
            int i;

            for (i=0; cd[i];)
            {
                if (cd[i] >= settings.shared.bpp) i++;
                else break;
            }
            for (; cd[i]; i++)
            {
                mode = videoCheckMode(&settings.shared.xdim, &settings.shared.ydim, cd[i], settings.shared.fullscreen, 1);
                if (mode < 0) continue;
                settings.shared.bpp = cd[i];
                break;
            }
        }

        Button_SetCheck(GetDlgItem(pages[TAB_CONFIG], IDCFULLSCREEN), ((settings.shared.fullscreen) ? BST_CHECKED : BST_UNCHECKED));
        //Button_SetCheck(GetDlgItem(pages[TAB_CONFIG], IDCPOLYMER), ((settings.polymer) ? BST_CHECKED : BST_UNCHECKED));

        (void)ComboBox_ResetContent(hwnd);

        for (int i=0; i<validmodecnt; i++)
        {
            if (validmode[i].fs != (settings.shared.fullscreen)) continue;
            if ((validmode[i].bpp < 15)) continue;

            // all modes get added to the 3D mode list
            Bsprintf(buf, "%dx%d %s", validmode[i].xdim, validmode[i].ydim, validmode[i].bpp == 8 ? "software" : "OpenGL");
            int const j = ComboBox_AddString(hwnd, buf);
            (void)ComboBox_SetItemData(hwnd, j, i);
            if (i == mode)(void)ComboBox_SetCurSel(hwnd, j);
        }
    }


    if (pgs & POPULATE_GAME)
    {
        HWND hwnd = GetDlgItem(pages[TAB_CONFIG], IDCDATA);

		int i=0;
		for (auto& grp : *gamedata)
		{
			FStringf grpinfo("%s %s", grp.FileInfo.name.GetChars(), grp.FileName.GetChars());
            int const j = ListBox_AddString(hwnd, grpinfo.GetChars());
            (void)ListBox_SetItemData(hwnd, j, i);
			i++;
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
            settings.shared.fullscreen = !settings.shared.fullscreen;
            PopulateForm(POPULATE_VIDEO);
            return TRUE;
        //case IDCPOLYMER:
        //    settings.polymer = !settings.polymer;
        //    if (settings.shared.bpp == 8) settings.shared.bpp = 32;
        //    PopulateForm(POPULATE_VIDEO);
        //    return TRUE;
        case IDCVMODE:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                int i = ComboBox_GetCurSel((HWND)lParam);
                if (i != CB_ERR) i = ComboBox_GetItemData((HWND)lParam, i);
                if (i != CB_ERR)
                {
                    settings.shared.xdim = validmode[i].xdim;
                    settings.shared.ydim = validmode[i].ydim;
                    settings.shared.bpp  = validmode[i].bpp;
                }
            }
            return TRUE;
        case IDCALWAYSSHOW:
            displaysetup = IsDlgButtonChecked(hwndDlg, IDCALWAYSSHOW) == BST_CHECKED;
            return TRUE;
        case IDCAUTOLOAD:
            noautoload = (IsDlgButtonChecked(hwndDlg, IDCAUTOLOAD) != BST_CHECKED);
            return TRUE;
        case IDCGAMEDIR:
            return TRUE;
        case IDCDATA:
        {
            if (HIWORD(wParam) != LBN_SELCHANGE) break;
            intptr_t i = ListBox_GetCurSel((HWND)lParam);
            if (i != CB_ERR) i = ListBox_GetItemData((HWND)lParam, i);
            if (i != CB_ERR)
            {
                settings.grp = i;
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


static void SetPage(int pageNum)
{
    HWND tab = GetDlgItem(startupdlg, WIN_STARTWIN_TABCTL);
    auto const cur = SendMessageA(tab, TCM_GETCURSEL, 0, 0);
    ShowWindow(pages[cur], SW_HIDE);
    SendMessageA(tab, TCM_SETCURSEL, pageNum, 0);
    ShowWindow(pages[pageNum], SW_SHOW);
    mode = pageNum;

    SetFocus(GetDlgItem(startupdlg, WIN_STARTWIN_TABCTL));
}

static void EnableConfig(bool n)
{
    //EnableWindow(GetDlgItem(startupdlg, WIN_STARTWIN_CANCEL), n);
    EnableWindow(GetDlgItem(startupdlg, WIN_STARTWIN_START), n);
    EnableWindow(GetDlgItem(pages[TAB_CONFIG], IDCDATA), n);
    EnableWindow(GetDlgItem(pages[TAB_CONFIG], IDCFULLSCREEN), n);
    EnableWindow(GetDlgItem(pages[TAB_CONFIG], IDCGAMEDIR), n);
    EnableWindow(GetDlgItem(pages[TAB_CONFIG], IDCINPUT), n);
    //EnableWindow(GetDlgItem(pages[TAB_CONFIG], IDCPOLYMER), n);
    EnableWindow(GetDlgItem(pages[TAB_CONFIG], IDCVMODE), n);
}

static INT_PTR CALLBACK startup_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HBITMAP hbmp = NULL;

    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        // Fetch the positions (in screen coordinates) of all the windows we need to tweak
        RECT chrome = {};
        AdjustWindowRect(&chrome, GetWindowLong(hwndDlg, GWL_STYLE), FALSE);
        RECT rdlg;
        GetWindowRect(hwndDlg, &rdlg);

        // Knock off the non-client area of the main dialogue to give just the client area
        rdlg.left -= chrome.left;
        rdlg.top -= chrome.top;
        rdlg.right -= chrome.right;
        rdlg.bottom -= chrome.bottom;

        RECT rtab;
        GetWindowRect(GetDlgItem(hwndDlg, WIN_STARTWIN_TABCTL), &rtab);

        // Translate them to client-relative coordinates wrt the main dialogue window
        rtab.right -= rtab.left - 1;
        rtab.bottom -= rtab.top - 1;
        rtab.left  -= rdlg.left;
        rtab.top -= rdlg.top;

        RECT rcancel;
        GetWindowRect(GetDlgItem(hwndDlg, WIN_STARTWIN_CANCEL), &rcancel);

        rcancel.right -= rcancel.left - 1;
        rcancel.bottom -= rcancel.top - 1;
        rcancel.left -= rdlg.left;
        rcancel.top -= rdlg.top;

        RECT rstart;
        GetWindowRect(GetDlgItem(hwndDlg, WIN_STARTWIN_START), &rstart);

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

        HWND hwnd = GetDlgItem(hwndDlg, WIN_STARTWIN_BITMAP);
        SendMessageA(hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmp);

        RECT r;
        GetClientRect(hwnd, &r);

        int const xoffset = r.right;
        int const yoffset = r.bottom - rdlg.bottom;

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
        HDC hdc = GetDC(NULL);
        rdlg.left = (GetDeviceCaps(hdc, HORZRES) - rdlg.right) / 2;
        rdlg.top = (GetDeviceCaps(hdc, VERTRES) - rdlg.bottom) / 2;
        ReleaseDC(NULL, hdc);
        MoveWindow(hwndDlg, rdlg.left + chrome.left, rdlg.top + chrome.left,
                   rdlg.right + (-chrome.left+chrome.right), rdlg.bottom + (-chrome.top+chrome.bottom), TRUE);

        // Add tabs to the tab control
        {
            static char textSetup[] = "Setup";
            static char textMessageLog[] = "Message Log";

            hwnd = GetDlgItem(hwndDlg, WIN_STARTWIN_TABCTL);

            TCITEMA tab = {};
            tab.mask = TCIF_TEXT;
            tab.pszText = textSetup;
            SendMessageA(hwnd, TCM_INSERTITEMA, (WPARAM)TAB_CONFIG, (LPARAM)&tab);
            tab.mask = TCIF_TEXT;
            tab.pszText = textMessageLog;
            SendMessageA(hwnd, TCM_INSERTITEMA, (WPARAM)TAB_MESSAGES, (LPARAM)&tab);

            // Work out the position and size of the area inside the tab control for the pages
            ZeroMemory(&r, sizeof(r));
            GetClientRect(hwnd, &r);
            SendMessageA(hwnd, TCM_ADJUSTRECT, FALSE, (LPARAM)&r);
            r.right -= r.left-1;
            r.bottom -= r.top-1;
            r.top += rtab.top;
            r.left += rtab.left;

            // Create the pages and position them in the tab control, but hide them
            pages[TAB_CONFIG] = CreateDialog((HINSTANCE)win_gethinstance(), MAKEINTRESOURCE(WIN_STARTWINPAGE_CONFIG), hwndDlg, ConfigPageProc);
            SetWindowPos(pages[TAB_CONFIG], hwnd, r.left, r.top, r.right, r.bottom, SWP_HIDEWINDOW);

            pages[TAB_MESSAGES] = GetDlgItem(hwndDlg, WIN_STARTWIN_MESSAGES);
            SetWindowPos(pages[TAB_MESSAGES], hwnd, r.left, r.top, r.right, r.bottom, SWP_HIDEWINDOW);

            // Tell the editfield acting as the console to exclude the width of the scrollbar
            GetClientRect(pages[TAB_MESSAGES], &r);
            r.right -= GetSystemMetrics(SM_CXVSCROLL)+4;
            r.left = r.top = 0;
            SendMessageA(pages[TAB_MESSAGES], EM_SETRECTNP,0,(LPARAM)&r);

            // Set a tab stop in the game data listbox
            {
                DWORD tabs[1] = { 150 };
                (void)ListBox_SetTabStops(GetDlgItem(pages[TAB_CONFIG], IDCDATA), 1, tabs);
            }

            SetFocus(GetDlgItem(hwndDlg, WIN_STARTWIN_START));
            SetWindowTextA(hwndDlg, apptitle);
        }
        return FALSE;
    }

    case WM_NOTIFY:
    {
        auto nmhdr = (LPNMHDR)lParam;
        if (nmhdr->idFrom != WIN_STARTWIN_TABCTL) break;
        int const cur = SendMessageA(nmhdr->hwndFrom, TCM_GETCURSEL,0,0);
        switch (nmhdr->code)
        {
            case TCN_SELCHANGING:
            case TCN_SELCHANGE:
                if (cur < 0 || !pages[cur])
                    break;
                ShowWindow(pages[cur], nmhdr->code == TCN_SELCHANGING ? SW_HIDE : SW_SHOW);
                return TRUE;
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
    if (startupdlg) return 1;
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_TAB_CLASSES };
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
    if (!startupdlg) return 1;

    const HWND edctl = pages[TAB_MESSAGES];

    if (!edctl) return -1;

    static HWND dactrl = NULL;
    if (!dactrl) dactrl = GetDlgItem(startupdlg, WIN_STARTWIN_TABCTL);

    int const vis = ((int)SendMessageA(dactrl, TCM_GETCURSEL,0,0) == TAB_MESSAGES);

    if (vis)
        SendMessageA(edctl, WM_SETREDRAW, FALSE, 0);

    int const curlen = SendMessageA(edctl, WM_GETTEXTLENGTH, 0,0);
    SendMessageA(edctl, EM_SETSEL, (WPARAM)curlen, (LPARAM)curlen);

    int const   numlines = SendMessageA(edctl, EM_GETLINECOUNT, 0, 0);
    static bool newline  = false;
    const char *p        = buf;

    while (*p)
    {
        if (newline)
        {
            SendMessageA(edctl, EM_REPLACESEL, 0, (LPARAM)"\r\n");
            newline = false;
        }
        const char *q = p;
        while (*q && *q != '\n') q++;
        static char workbuf[1024];
        Bmemcpy(workbuf, p, q-p);
        if (*q == '\n')
        {
            if (!q[1])
            {
                newline = true;
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
        SendMessageA(edctl, EM_REPLACESEL, 0, (LPARAM)workbuf);
    }

    int const newnumlines = SendMessageA(edctl, EM_GETLINECOUNT, 0, 0);
    SendMessageA(edctl, EM_LINESCROLL, 0, newnumlines - numlines);

    if (vis)
        SendMessageA(edctl, WM_SETREDRAW, TRUE, 0);

    return 0;
}

int32_t startwin_settitle(const char *str)
{
    if (!startupdlg) return 1;
    SetWindowTextA(startupdlg, str);
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
    if (!startupdlg) return 1;

    done = -1;

    SetPage(TAB_CONFIG);
    EnableConfig(1);

	settings.shared = { ScreenMode, ScreenWidth, ScreenHeight, ScreenBPP };
	settings.grp = 0;

    PopulateForm(-1);

    do
    {
        MSG msg;

        switch (GetMessage(&msg, NULL, 0,0))
        {
        case 0:
            done = 1;
            break;
        case -1:
            return -1;
        default:
            if (IsWindow(startupdlg) && IsDialogMessage(startupdlg, &msg))
                break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            break;
        }
    }
    while (done < 0);

    SetPage(TAB_MESSAGES);
    EnableConfig(0);

    if (done)
    {
		ScreenWidth = settings.shared.xdim;
		ScreenHeight = settings.shared.ydim;
		ScreenMode = settings.shared.fullscreen;
		ScreenBPP = settings.shared.bpp;
    }

    return done;
}

int ShowStartupWindow(TArray<GrpEntry> &groups)
{
	gamedata = &groups;
	startwin_open();
	startwin_settitle("Demolition");

	if (1)//readSetup < 0 || (!g_noSetup && (displaysetup)) || g_commandSetup)
	{
		auto choice = startwin_run();
		if (choice == 0)
		{
			startwin_close();
			return - 1;
		}
	}
	startwin_close();
	return settings.grp;
}

#endif // STARTUP_SETUP_WINDOW


