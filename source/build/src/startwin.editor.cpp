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
#include "editor.h"
#include "winlayer.h"

#include "startwin.editor.h"

#define TAB_CONFIG 0
#define TAB_MESSAGES 1

static struct
{
    int32_t fullscreen;
    int32_t xdim2d, ydim2d;
    int32_t xdim3d, ydim3d, bpp3d;
    int32_t forcesetup;
} settings;

static HWND startupdlg = NULL;
static HWND pages[2] = { NULL, NULL};
static int32_t done = -1, mode = TAB_CONFIG;

static void PopulateForm(void)
{
    int32_t i,j;
    char buf[64];
    int32_t mode2d, mode3d;
    HWND hwnd2d, hwnd3d;

    hwnd2d = GetDlgItem(pages[TAB_CONFIG], IDC2DVMODE);
    hwnd3d = GetDlgItem(pages[TAB_CONFIG], IDC3DVMODE);

    mode2d = checkvideomode(&settings.xdim2d, &settings.ydim2d, 8, settings.fullscreen, 1);
    mode3d = checkvideomode(&settings.xdim3d, &settings.ydim3d, settings.bpp3d, settings.fullscreen, 1);
    if (mode2d < 0) mode2d = 0;
    if (mode3d < 0)
    {
        int32_t cd[] = { 32, 24, 16, 15, 8, 0 };
        for (i=0; cd[i];) { if (cd[i] >= settings.bpp3d) i++; else break; }
        for (; cd[i]; i++)
        {
            mode3d = checkvideomode(&settings.xdim3d, &settings.ydim3d, cd[i], settings.fullscreen, 1);
            if (mode3d < 0) continue;
            settings.bpp3d = cd[i];
            break;
        }
    }

    Button_SetCheck(GetDlgItem(pages[TAB_CONFIG], IDCFULLSCREEN), (settings.fullscreen ? BST_CHECKED : BST_UNCHECKED));
    Button_SetCheck(GetDlgItem(pages[TAB_CONFIG], IDCALWAYSSHOW), (settings.forcesetup ? BST_CHECKED : BST_UNCHECKED));

    (void)ComboBox_ResetContent(hwnd2d);
    (void)ComboBox_ResetContent(hwnd3d);
    for (i=0; i<validmodecnt; i++)
    {
        if (validmode[i].fs != settings.fullscreen) continue;

        // all modes get added to the 3D mode list
        Bsprintf(buf, "%dx%d %s", validmode[i].xdim, validmode[i].ydim, validmode[i].bpp == 8 ? "software" : "OpenGL");
        j = ComboBox_AddString(hwnd3d, buf);
        (void)ComboBox_SetItemData(hwnd3d, j, i);
        if (i == mode3d)(void)ComboBox_SetCurSel(hwnd3d, j);

        // only 8-bit modes get used for 2D
        if (validmode[i].bpp != 8 || validmode[i].xdim < 640 || validmode[i].ydim < 480) continue;
        Bsprintf(buf, "%d x %d", validmode[i].xdim, validmode[i].ydim);
        j = ComboBox_AddString(hwnd2d, buf);
        (void)ComboBox_SetItemData(hwnd2d, j, i);
        if (i == mode2d)(void)ComboBox_SetCurSel(hwnd2d, j);
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
            settings.fullscreen = !settings.fullscreen;
            PopulateForm();
            return TRUE;
        case IDC2DVMODE:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                int32_t i;
                i = ComboBox_GetCurSel((HWND)lParam);
                if (i != CB_ERR) i = ComboBox_GetItemData((HWND)lParam, i);
                if (i != CB_ERR)
                {
                    settings.xdim2d = validmode[i].xdim;
                    settings.ydim2d = validmode[i].ydim;
                }
            }
            return TRUE;
        case IDC3DVMODE:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                int32_t i;
                i = ComboBox_GetCurSel((HWND)lParam);
                if (i != CB_ERR) i = ComboBox_GetItemData((HWND)lParam, i);
                if (i != CB_ERR)
                {
                    settings.xdim3d = validmode[i].xdim;
                    settings.ydim3d = validmode[i].ydim;
                    settings.bpp3d  = validmode[i].bpp;
                }
            }
            return TRUE;
        case IDCALWAYSSHOW:
            settings.forcesetup = IsDlgButtonChecked(hwndDlg, IDCALWAYSSHOW) == BST_CHECKED;
            return TRUE;
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
    EnableWindow(GetDlgItem(pages[TAB_CONFIG], IDC2DVMODE), n);
    EnableWindow(GetDlgItem(pages[TAB_CONFIG], IDC3DVMODE), n);
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
        rdlg.left -= chrome.left; rdlg.top -= chrome.top;
        rdlg.right -= chrome.right; rdlg.bottom -= chrome.bottom;

        // Translate them to client-relative coordinates wrt the main dialogue window
        rtab.right -= rtab.left - 1; rtab.bottom -= rtab.top - 1;
        rtab.left  -= rdlg.left; rtab.top -= rdlg.top;

        rcancel.right -= rcancel.left - 1; rcancel.bottom -= rcancel.top - 1;
        rcancel.left -= rdlg.left; rcancel.top -= rdlg.top;

        rstart.right -= rstart.left - 1; rstart.bottom -= rstart.top - 1;
        rstart.left -= rdlg.left; rstart.top -= rdlg.top;

        // And then convert the main dialogue coordinates to just width/length
        rdlg.right -= rdlg.left - 1; rdlg.bottom -= rdlg.top - 1;
        rdlg.left = 0; rdlg.top = 0;

        // Load the bitmap into the bitmap control and fetch its dimensions
        hbmp = LoadBitmap((HINSTANCE)win_gethinstance(), MAKEINTRESOURCE(RSRC_BMP));
        hwnd = GetDlgItem(hwndDlg,WIN_STARTWIN_BITMAP);
        SendMessage(hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmp);
        GetClientRect(hwnd, &r);
        xoffset = r.right;
        yoffset = r.bottom - rdlg.bottom;

        // Shift and resize the controls that require it
        rtab.left += xoffset; rtab.bottom += yoffset;
        rcancel.left += xoffset; rcancel.top += yoffset;
        rstart.left += xoffset; rstart.top += yoffset;
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
            SendMessage(hwnd, TCM_INSERTITEM, (WPARAM)0, (LPARAM)&tab);
            tab.mask = TCIF_TEXT;
            static char textMessageLog[] = TEXT("Message Log");
            tab.pszText = textMessageLog;
            SendMessage(hwnd, TCM_INSERTITEM, (WPARAM)1, (LPARAM)&tab);
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

            SetFocus(GetDlgItem(hwndDlg, WIN_STARTWIN_START));
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
            done = 1; return TRUE;
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

    settings.fullscreen = fullscreen;
    settings.xdim2d = xdim2d;
    settings.ydim2d = ydim2d;
    settings.xdim3d = xdimgame;
    settings.ydim3d = ydimgame;
    settings.bpp3d = bppgame;
    settings.forcesetup = forcesetup;
    PopulateForm();

    while (done < 0)
    {
        switch (GetMessage(&msg, NULL, 0,0))
        {
        case 0:
            done = 1; break;
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
        fullscreen = settings.fullscreen;
        xdim2d = settings.xdim2d;
        ydim2d = settings.ydim2d;
        xdimgame = settings.xdim3d;
        ydimgame = settings.ydim3d;
        bppgame = settings.bpp3d;
        forcesetup = settings.forcesetup;
    }

    return done;
}

#endif // STARTUP_SETUP_WINDOW