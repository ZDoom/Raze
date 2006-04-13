#ifndef RENDERTYPEWIN
#error Only for Windows
#endif

#include "build.h"
#include "editor.h"
#include "winlayer.h"
#include "compat.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>


#define IDCFULLSCREEN	100
#define IDCVMODE	101

static void PopulateVideoModeLists(int fs, HWND list3d)
{
	int i,j;
	char buf[64];

	ComboBox_ResetContent(list3d);
	for (i=0; i<validmodecnt; i++) {
		if (validmode[i].fs != fs) continue;

		Bsprintf(buf, "%ldx%ld %dbpp", validmode[i].xdim, validmode[i].ydim, validmode[i].bpp);
		j = ComboBox_AddString(list3d, buf);
		ComboBox_SetItemData(list3d, j, i);
		if (xdimgame == validmode[i].xdim && ydimgame == validmode[i].ydim && bppgame == validmode[i].bpp)
			ComboBox_SetCurSel(list3d, j);
	}
}

static INT_PTR CALLBACK LaunchWindowProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_INITDIALOG: {
			char buf[64];

			if (lParam) {
				long *saferect = (long*)lParam;
				RECT unresized, playbutton, exitbutton;
				int dx, dy;

				GetWindowRect(hwndDlg, &unresized);
				GetWindowRect(GetDlgItem(hwndDlg, IDOK), &playbutton);
				GetWindowRect(GetDlgItem(hwndDlg, IDCANCEL), &exitbutton);

				playbutton.right  = 1 + playbutton.right  - playbutton.left;
				playbutton.bottom = 1 + playbutton.bottom - playbutton.top;
				exitbutton.right  = 1 + exitbutton.right  - exitbutton.left;
				exitbutton.bottom = 1 + exitbutton.bottom - exitbutton.top;
				
				playbutton.left = playbutton.left - unresized.left;
				playbutton.top  = playbutton.top  - unresized.top;
				exitbutton.left = exitbutton.left - unresized.left;
				exitbutton.top  = exitbutton.top  - unresized.top;

				dx = saferect[2] - (unresized.right  - unresized.left);
				dy = saferect[3] - (unresized.bottom - unresized.top);

				playbutton.left += dx; exitbutton.left += dx;
				playbutton.top  += dy; exitbutton.top  += dy;

				// reposition the dialog, play and exit buttons
				MoveWindow(hwndDlg, saferect[0], saferect[1], saferect[2], saferect[3], FALSE);
				MoveWindow(GetDlgItem(hwndDlg, IDOK),
					playbutton.left, playbutton.top, playbutton.right, playbutton.bottom, FALSE);
				MoveWindow(GetDlgItem(hwndDlg, IDCANCEL),
					exitbutton.left, exitbutton.top, exitbutton.right, exitbutton.bottom, FALSE);
			
			}

			// populate the controls
			Button_SetCheck(GetDlgItem(hwndDlg, IDCFULLSCREEN), fullscreen ? BST_CHECKED : BST_UNCHECKED);
			PopulateVideoModeLists(fullscreen, GetDlgItem(hwndDlg, IDCVMODE));

			ShowWindow(hwndDlg, SW_SHOW);
			if (GetDlgCtrlID((HWND)wParam) != IDOK) { 
				SetFocus(GetDlgItem(hwndDlg, IDOK)); 
				return FALSE; 
			} 
			return TRUE;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDCFULLSCREEN:
					fullscreen = Button_GetCheck((HWND)lParam) == BST_CHECKED ? 1:0;
					PopulateVideoModeLists(fullscreen, GetDlgItem(hwndDlg, IDCVMODE));
					break;
				case IDCVMODE:
					if (HIWORD(wParam) == CBN_SELCHANGE) {
						int i;
						i = ComboBox_GetCurSel((HWND)lParam);
						if (i != CB_ERR) i = ComboBox_GetItemData((HWND)lParam, i);
						if (i != CB_ERR) {
							xdimgame = validmode[i].xdim;
							ydimgame = validmode[i].ydim;
							bppgame  = validmode[i].bpp;
						}
					}
					break;
				case IDCANCEL:
					quitevent = 1;	// fall through
				case IDOK:
					DestroyWindow(hwndDlg);
					return TRUE;
				default: break;
			}
			break;
		default: break;
	}
	return FALSE;
}

int DoLaunchWindow(int initval)
{
	HWND hwndStart, hwndLaunch;
	MSG msg;
	long saferect[4];

	// only show config window if no config is loaded, or if it is, only if left control is held
	if (initval == 0 && !GetAsyncKeyState(VK_CONTROL)) return 0;

	if (win_getstartupwin((long*)&hwndStart, saferect, NULL)) return 0;

	hwndLaunch = CreateDialogParam((HINSTANCE)win_gethinstance(), MAKEINTRESOURCE(2000), hwndStart, LaunchWindowProc, (LPARAM)saferect);
	if (hwndLaunch) {
		EnableWindow(GetDlgItem(hwndStart,WIN_STARTWIN_ITEMLIST),FALSE);
		while (GetMessage(&msg, NULL, 0, 0) > 0) {
			if (!IsWindow(hwndLaunch) || quitevent) break;
			if (IsDialogMessage(hwndStart, &msg) /*|| IsDialogMessage(hwndLaunch, &msg)*/) continue;
		
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		EnableWindow(GetDlgItem(hwndStart,WIN_STARTWIN_ITEMLIST),TRUE);
	}
	if (quitevent) return 1;
	return 0;
}

